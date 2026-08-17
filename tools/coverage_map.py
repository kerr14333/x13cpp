#!/usr/bin/env python
"""coverage_map -- track port coverage of the 712 Fortran .f files.

Statuses:  pending | ported | gated | partial | n-a
  pending  not yet ported (default)
  ported   C++ port complete and parity-checked
  gated    ported but behind a feature gate / not yet enabled
  partial  some branches ported, others still abend. ALWAYS carries a trailing
           `# ...` note saying which -- the note is the point, and is preserved
           verbatim. Counted as neither done nor pending.
  n-a      not applicable -- a .f that is NOT in makefile.gf's OBJS list (e.g.
           htmlout-only variants genindex/htmlutil/mkmetahtmlfile, or superseded
           .g77 variants). These are excluded from the coverage denominator.

Reads/writes tools/ported.yaml (a plain `stem: status` mapping -- no YAML library
needed).

READ THIS BEFORE TRUSTING THE LEDGER. `--write` DISCOVERS files; it does not
DERIVE status. New .f files are added with a computed default (n-a if absent
from OBJS, else pending) and every existing status is preserved verbatim. So a
routine ported months ago stays `pending` forever unless somebody edits the
file by hand -- which is exactly what happened: a 2026-07-29 audit found 523 of
the 524 `pending` entries stale by 216, including routines closed many sessions
earlier (genqs, gennpsa, amdfct, pass2, spcdrv, svpeak, savpk, svtukp).

`--audit` fixes that by deriving evidence from the C++ tree instead of trusting
the file:

  DEF    a C++ function DEFINITION whose name IS the Fortran stem. The port's
         convention is one .f -> one same-named C++ function, so this is strong
         evidence. Spot-checked against the collision-prone short names
         (apply/change/chisq/antilg/averag/ceilng/copy/punch): all eight are
         genuine ports of the same-named .f, no false positives.
  ALIAS  a stem whose port deliberately carries a DIFFERENT C++ name, listed in
         ALIASES below and verified to resolve. Hand-maintained, because there
         is no way to derive it -- and every entry is checked on each run, so a
         wrong guess shows up as UNRESOLVED rather than as coverage.
  SPLIT  the .f holds MORE THAN ONE Fortran routine and some -- but not all --
         of them resolve to a same-named C++ definition. Reported as `k/n
         routines`. See below; this tier exists because a file-level ledger
         cannot describe such a file at all.
  CITE   the stem is only mentioned in a comment (`foo.f`). NOT promoted: a
         comment citing a routine usually means "this is where its behaviour
         went", but it can equally mean "this is the thing we did NOT port".
         Reported for review.
  none   no trace in the C++ tree.

THE LEDGER IS FILE-LEVEL; THE PORT IS ROUTINE-LEVEL. 712 .f files hold 1192
SUBROUTINE/FUNCTION definitions, and 37 files hold more than one -- `matrix.f`
has 90, `ansub2.f` 44, `sigex.f` 10. One `stem: status` line cannot describe
those, and neither can one ALIASES entry, so every such file sat at `pending`
no matter how much of it was ported. That is not drift the audit could catch:
it is a shape the data model could not express, which is why `--audit` grew a
SPLIT tier and `compute()` now prints a SECOND, routine-level coverage figure
beside the file-level one. Neither replaces the other -- the file figure counts
units of work the ledger tracks, the routine figure counts units of work the
ORACLE has -- and quoting one where the other belongs is how 58.8% came to be
read as "41% of the program left to write" when most of the residue was the
deferred print engine and routines already ported under another name.

Both figures are derived from the two trees on every run, so neither can rot.

`--audit --promote` writes DEF + ALIAS as `ported` and leaves everything else
alone. It never DEMOTES: a hand-set `gated`/`ported` is a human judgement the
heuristic has no business overriding.

Usage:
  python coverage_map.py <fortran_src_dir> [--write] [--audit] [--promote]
"""
from __future__ import annotations

import os
import re
import sys
from typing import Dict, Set

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
PORTED_YAML = os.path.join(HERE, "ported.yaml")
# `partial` is in active use in ported.yaml with a trailing `# ...` note
# explaining which branches abend. It was NOT in this tuple, and compute()
# silently reset anything unrecognised to `pending` -- so three hand-written
# entries (adpdrg, fcnar, regvar) and their notes were destroyed by a single
# --write. Recognised now, and the note is preserved verbatim (see load_yaml).
STATUSES = ("pending", "ported", "gated", "partial", "n-a")

# Directories scanned for evidence. `tools/` is included because a few routines
# are print/emit surface realised in the CLI harnesses rather than in core/.
SRC_ROOTS = (os.path.join(REPO, "core", "src"), os.path.join(HERE,))

# Fortran stem -> the C++ symbol that IS its port, where the two names differ.
# ONLY for renames; a same-named port needs no entry. Every value is verified to
# resolve on each --audit, so a stale or wrong entry reports UNRESOLVED instead
# of quietly counting as coverage. Keep this list short and evidenced -- an
# unverifiable "I think that's where it went" belongs in CITE, not here.
ALIASES = {
    # --- parsers: gt<spec> -> gt_<spec> ------------------------------------
    "getfrc": "gt_force",
    "gtautx": "gt_pickmdl",
    "gtmtdt": "gt_metadata",
    "gtrvst": "gt_history",
    "gtspec": "gt_spectrum",
    "gtxreg": "gt_x11regression",
    # --- renamed leaves ----------------------------------------------------
    "adjsrs": "adjsrs_factors",      # regarima/priadj.cpp
    "amdfct": "aape_diagnostics",    # diag/amdfct.cpp (within-sample arm only)
    "mkback": "bcstout",             # regarima/forecast.cpp
    "mkpeak": "spectrum_peak_grid",  # driver/spectrum_peaks.cpp
    "mkspky": "spectrum_peaks",
    "pacf": "check_pacf",            # diag/checkres.cpp
    "qsdiff": "qs_diff",             # diag/genqs.cpp
    "round": "round_x13",
    "rplus": "rplus_mpd",
    # --- split into two or more C++ entry points (either resolves) ---------
    "agr2": "agr2_component",        # + agr2_compare
    "prtd8b": "prtd8b_savelog",
    "prtd9a": "prtd9a_savelog",
    "restor": "restor_model",        # + restor_span
    "setssp": "setssp_span",
    "sfmax": "sfmax_span",
    "spgrh": "spgrh_fit",            # + spgrh_eval (one fit, two grids)
    "ssprep": "ssprep_save",         # + ssprep_snapshot
    "x11pt4": "x11pt4_partf",        # + x11pt4_etables
    # --- several .f absorbed into ONE C++ driver ---------------------------
    # Faithful: this port collapses the oracle's driver + its checker into a
    # single function, so the ledger points several stems at the same symbol.
    "getrev": "run_history",
    "revchk": "run_history",
    "revdrv": "run_history",
    "ssap": "run_slidingspans",
    "sspdrv": "run_slidingspans",
    "spcdrv": "run_spectrum",
    "x11ari": "x11_prestage",
    "prtmdl": "est_diagnostics",     # diag/estdgn.cpp: prtmdl.f:752-934
    "prtrts": "est_diagnostics",     # ... + prtrts.f
    "savotl": "est_diagnostics",     # ... + savotl.f:79-152
}


# Names the SUBROUTINE/FUNCTION regex picks up that are not routines. It used to
# hold `to`, a continuation-line artefact out of ansub1/2/4.
#
# This was ALSO guarded by `len(n) < 3`, on the reasoning that a name that short
# would collide with unrelated C++ symbols. It had one other victim: `si.f:3`
# defines a REAL routine called `si`, it is ported at
# `core/src/x11/x11drv.cpp:354`, and the length rule dropped it from the
# numerator AND the denominator -- hiding a success rather than reporting a gap.
# A length heuristic added to kill one parse artefact cannot tell the artefact
# from a short real name; the deny list has to NAME what it drops, so that
# adding to it is a decision somebody makes rather than a side effect. `to` is
# the only entry, and the scan below confirms it is the only sub-3-char name
# besides `si`.
# EMPTY, and that is the result rather than an oversight: `to` was this list's
# only entry, and fixing the regex below removed it at the source. A deny list
# whose entries the parse no longer produces is a case list that has learned to
# hide -- it reads like knowledge and asserts nothing.
ROUTINE_DENY: set = set()

# One SUBROUTINE / FUNCTION header. Fortran fixed form puts the keyword in
# columns 7+, and a FUNCTION may carry a type and a length (`DOUBLE PRECISION
# FUNCTION foo`, `CHARACTER*8 FUNCTION bar`), hence the bounded prefix.
#
# TWO things here are load-bearing, and both were found by DELETING the
# `len(n) < 3` guard that used to sit above and hide them:
#
#   * every whitespace class is `[ \t]`, never `\s`. `\s` matches a NEWLINE, so
#     `\s+` after the keyword ran off the end of the line and captured the first
#     token of the NEXT one. That is where the phantom routine `c` came from --
#     `special.f:279` is `      end function` followed by a bare `c` comment
#     line -- and it is the same mechanism behind the `to` in ansub1/2/4.
#   * `(?!END\b)`. The optional type prefix is a class of capital letters, so it
#     matched the `END ` of `END FUNCTION` and turned every terminator into a
#     definition.
#
# A deny list would have papered over both. The parse is the thing to fix; the
# deny list stays empty unless something is genuinely ambiguous.
_ROUTINE_RE = re.compile(
    r"(?im)^[ \t]{6}[ \t]*(?!END\b)"
    r"(?:SUBROUTINE|(?:[A-Z*\d \t()]{0,20}?)\bFUNCTION)[ \t]+([A-Za-z0-9_]+)")


def f_routines(src_dir: str) -> Dict[str, list]:
    """stem -> the Fortran routines DEFINED in that .f, lowercased.

    A file with two or more entries here cannot be described by a single ledger
    status; that is what the SPLIT tier is for.
    """
    out: Dict[str, list] = {}
    for fn in sorted(os.listdir(src_dir)):
        if not fn.endswith(".f"):
            continue
        with open(os.path.join(src_dir, fn), "r", errors="replace") as fh:
            names = _ROUTINE_RE.findall(fh.read())
        seen, keep = set(), []
        for n in names:
            n = n.lower()
            if n in ROUTINE_DENY or n in seen:
                continue
            seen.add(n)
            keep.append(n)
        out[fn[:-2]] = keep
    return out


_CPP_CACHE = None


def _cpp_sources():
    # audit() and compute() both want the tree; walking and reading 180+ files
    # twice is the whole remaining cost of a run.
    global _CPP_CACHE
    if _CPP_CACHE is not None:
        return _CPP_CACHE
    out = []
    for base in SRC_ROOTS:
        for dp, _, fns in os.walk(base):
            if os.sep + "gen" in dp:
                continue
            for fn in fns:
                if fn.endswith((".cpp", ".hpp")):
                    p = os.path.join(dp, fn)
                    with open(p, "r", errors="replace") as fh:
                        out.append((p, fh.read()))
    _CPP_CACHE = out
    return out


def _defines(blob: str, name: str) -> bool:
    """A C++ function DEFINITION (or declaration) named `name`."""
    pat = (r"^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w:<>,\s*&]*?\b"
           + re.escape(name) + r"\s*\(")
    return re.search(pat, blob, re.M) is not None


# The same shape as _defines, with the name left as a capture. One pass over the
# blob yields every defined symbol, so the routine-level tiers do set lookups
# instead of 1100+ anchored scans of 2.7MB (which took over two minutes).
# _def_names_matches_defines() below asserts the two agree.
_DEF_CAPTURE = re.compile(
    r"^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w:<>,\s*&]*?\b(\w+)\s*\(", re.M)


def _def_names(blob: str) -> Set[str]:
    return set(_DEF_CAPTURE.findall(blob))


# The CITE tier's two probes, likewise hoisted out of the per-stem loop: it was
# scanning 2.7MB twice for each of ~260 pending stems (14s of a 15s run).
#
# Hoisting also FIXED a substring bug. The per-stem probe was
# `re.escape(stem) + r"\.f\b"` -- anchored on the right, open on the left -- so
# `dot` matched inside `amidot.f` and `ddot.f` and was reported as CITE though
# `dot.f` is cited nowhere. Capturing `(\w+)` instead makes the left edge a word
# boundary too. `dot` moves CITE -> none; it is the only stem the corpus
# currently discriminates, but the bug applied to every short stem that is a
# suffix of a longer routine name.
_CITE_F_RE = re.compile(r"(\w+)\.f\b")
_CITE_CALL_RE = re.compile(r"\b(\w+)\s*\(")


def _cite_names(blob: str) -> Set[str]:
    """Stems mentioned as `foo.f` OR called as `foo(` anywhere in the C++ tree."""
    return set(_CITE_F_RE.findall(blob)) | set(_CITE_CALL_RE.findall(blob))


def audit(status: Dict[str, str], promote: bool,
          routines: Dict[str, list] | None = None) -> Dict[str, str]:
    srcs = _cpp_sources()
    blob = "\n".join(t for _, t in srcs)
    routines = routines or {}

    # One pass for every symbol the C++ tree defines. Every tier below is a set
    # lookup against it -- _defines() is kept as the reference implementation the
    # equivalence test in tests/parity/test_doc_tooling.py checks this against.
    defined = _def_names(blob)
    cited = _cite_names(blob)

    unresolved = [s for s, sym in ALIASES.items() if sym not in defined]
    resolved: Dict[str, Set[str]] = {}
    for stem, names in routines.items():
        resolved[stem] = {n for n in names if n in defined}

    def split_frac(stem):
        """(resolved, total) for a multi-routine .f, else None."""
        names = routines.get(stem, [])
        return (len(resolved[stem]), len(names)) if len(names) > 1 else None

    tiers: Dict[str, list] = {"DEF": [], "ALIAS": [], "SPLIT": [],
                              "CITE": [], "none": []}
    # A file whose every routine resolves is DEF-strength evidence even when the
    # STEM itself names no C++ function -- the port simply kept the oracle's
    # routine names and dropped the container. Promoted with DEF.
    whole: list = []
    for stem, st in sorted(status.items()):
        if bare(st) != "pending":
            continue
        frac = split_frac(stem)
        if stem in defined:
            tiers["DEF"].append(stem)
        elif stem in ALIASES and stem not in unresolved:
            tiers["ALIAS"].append(stem)
        elif frac and frac[0] == frac[1]:
            whole.append(stem)
            tiers["DEF"].append(stem)
        elif frac and frac[0] > 0:
            tiers["SPLIT"].append((stem, frac[0], frac[1]))
        elif stem in cited:
            tiers["CITE"].append(stem)
        else:
            tiers["none"].append(stem)

    print("=== ported.yaml audit (evidence from the C++ tree) ===")
    for k in ("DEF", "ALIAS", "SPLIT", "CITE", "none"):
        print("  {:<6} {:>4}".format(k, len(tiers[k])))
    if unresolved:
        print("  !! ALIASES entries that do NOT resolve (fix or drop them):")
        for s in unresolved:
            print("       {} -> {}".format(s, ALIASES[s]))
    if whole:
        print("  DEF via ALL routines resolving (stem names no C++ symbol):")
        print("       " + " ".join(sorted(whole)))
    if tiers["SPLIT"]:
        print("  SPLIT -- multi-routine .f, PART ported. Not promoted: the")
        print("  ledger has no status for `most of it`. Ratio is the measure:")
        for stem, k, n in sorted(tiers["SPLIT"], key=lambda r: -(r[1])):
            print("       {:>3}/{:<3} {}.f".format(k, n, stem))
    if tiers["CITE"]:
        print("  CITE (mentioned only in comments -- review, do not assume):")
        print("       " + " ".join(tiers["CITE"]))

    if promote:
        n = 0
        for stem in tiers["DEF"] + tiers["ALIAS"]:
            status[stem] = "ported"
            n += 1
        print("  promoted {} pending -> ported".format(n))
    else:
        print("  (add --promote to write DEF + ALIAS as `ported`)")
    return status


def objs_from_makefile(src_dir: str) -> Set[str]:
    """Parse the OBJS = ... continuation block from makefile.gf into a stem set."""
    path = os.path.join(src_dir, "makefile.gf")
    objs: Set[str] = set()
    if not os.path.exists(path):
        return objs
    collecting = False
    with open(path, "r", errors="replace") as fh:
        for line in fh:
            if re.match(r"\s*OBJS\s*=", line):
                collecting = True
            if collecting:
                for m in re.findall(r"([A-Za-z0-9_.]+)\.o\b", line):
                    objs.add(m)
                if not line.rstrip().endswith("\\"):
                    break
    return objs


def f_stems(src_dir: str) -> Set[str]:
    return {os.path.splitext(f)[0] for f in os.listdir(src_dir) if f.endswith(".f")}


def bare(value: str) -> str:
    """The status word alone, dropping any trailing `# ...` note."""
    return value.split("#", 1)[0].strip()


def load_yaml(path: str) -> Dict[str, str]:
    """Values are kept VERBATIM, note and all.

    A status may carry an explanatory `# ...` suffix (`partial   # which
    branches abend`). Those notes are hand-written and irreplaceable, so
    everything downstream compares with bare() and writes the value back
    unchanged rather than reconstructing it.
    """
    status: Dict[str, str] = {}
    if not os.path.exists(path):
        return status
    with open(path, "r") as fh:
        for line in fh:
            line = line.rstrip()
            if not line or line.lstrip().startswith("#"):
                continue
            if ":" in line:
                k, v = line.split(":", 1)
                status[k.strip()] = v.strip()
    return status


HEADER = """\
# X13cpp port coverage map. Status: pending | ported | gated | partial | n-a
#
# DISCOVER new .f files:   python tools/coverage_map.py oracle/fortran --write
# CHECK the statuses:      python tools/coverage_map.py oracle/fortran --audit
# REFRESH them:            ... --audit --promote
#
# --write only DISCOVERS; it never derives status, so this file goes stale
# silently as routines land. --audit re-derives evidence from the C++ tree and
# promotes only what it can prove (a same-named definition, or a verified entry
# in coverage_map.py's ALIASES). A `partial` entry's trailing `# ...` note is
# hand-written -- preserve it.
"""


def write_yaml(path: str, status: Dict[str, str]) -> None:
    """Rewrites the file wholesale, HEADER included.

    Anything hand-added to the header is therefore lost on every --write, which
    is why the header lives here rather than in the data file: edit HEADER, not
    ported.yaml. (Found the hard way -- the first version of this docstring was
    written into ported.yaml and silently discarded by the next --write.)
    """
    with open(path, "w") as fh:
        fh.write(HEADER)
        for stem in sorted(status):
            fh.write("{}: {}\n".format(stem, status[stem]))


def compute(src_dir: str, write: bool, do_audit: bool = False,
            promote: bool = False) -> int:
    objs = objs_from_makefile(src_dir)
    stems = f_stems(src_dir)
    existing = load_yaml(PORTED_YAML)

    status: Dict[str, str] = {}
    unknown = []
    for stem in stems:
        if stem in existing:
            # Preserve the value VERBATIM when its status word is recognised --
            # including any `# ...` note. An UNRECOGNISED word is preserved too
            # and reported, never silently reset: that reset is what destroyed
            # three hand-written `partial` entries on 2026-07-29.
            if bare(existing[stem]) in STATUSES:
                status[stem] = existing[stem]
            else:
                status[stem] = existing[stem]
                unknown.append((stem, existing[stem]))
        else:
            status[stem] = "pending" if stem in objs else "n-a"
    if unknown:
        print("  !! unrecognised status words, preserved as-is -- add them to")
        print("     STATUSES or correct the file:")
        for stem, v in unknown:
            print("       {}: {}".format(stem, v))

    routines = f_routines(src_dir)

    if do_audit:
        status = audit(status, promote, routines)
        print()

    if write or (do_audit and promote):
        write_yaml(PORTED_YAML, status)

    counts = {s: 0 for s in STATUSES}
    for v in status.values():
        counts[bare(v)] = counts.get(bare(v), 0) + 1
    total = len(status)
    denom = total - counts["n-a"]
    # `partial` counts as neither: it is ported-with-branches-that-abend, and
    # rolling it into `done` would overstate the number the docs quote.
    done = counts["ported"] + counts["gated"]
    pct = (100.0 * done / denom) if denom else 0.0

    print("=== X13cpp port coverage ===")
    print("  total .f files : {}".format(total))
    for s in STATUSES:
        print("  {:<8}       : {}".format(s, counts[s]))
    print("  ---------------------------")
    print("  portable set   : {} (excludes n-a)".format(denom))
    print("  ported+gated   : {}  ({:.1f}%)".format(done, pct))

    # The routine-level figure. Counts what the ORACLE has (SUBROUTINE/FUNCTION
    # definitions) rather than what the ledger tracks (files), so a 90-routine
    # matrix.f weighs 90 and not 1. Derived on every run from both trees.
    #
    # It reads LOWER than the file figure and that is not a contradiction: a
    # renamed port satisfies the file-level DEF/ALIAS evidence but no
    # same-named routine, so renames count as missing here. Read the pair as a
    # bracket -- the truth is between them -- never either one alone.
    defined = _def_names("\n".join(t for _, t in _cpp_sources()))
    r_tot = r_got = 0
    for stem, names in routines.items():
        if bare(status.get(stem, "pending")) == "n-a":
            continue
        r_tot += len(names)
        r_got += sum(1 for n in names if n in defined)
    if r_tot:
        print("  ---------------------------")
        print("  oracle routines: {} (SUBROUTINE/FUNCTION, excl. n-a files)"
              .format(r_tot))
        print("  same-named C++ : {}  ({:.1f}%)  -- renames count as MISSING here"
              .format(r_got, 100.0 * r_got / r_tot))
    if not os.path.exists(PORTED_YAML):
        print("  (run with --write to create tools/ported.yaml)")
    return 0


def main(argv):
    if not argv:
        print(__doc__)
        return 2
    src = argv[0]
    flags = argv[1:]
    return compute(src, "--write" in flags, "--audit" in flags,
                   "--promote" in flags)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
