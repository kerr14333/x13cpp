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
  CITE   the stem is only mentioned in a comment (`foo.f`). NOT promoted: a
         comment citing a routine usually means "this is where its behaviour
         went", but it can equally mean "this is the thing we did NOT port".
         Reported for review.
  none   no trace in the C++ tree.

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


def _cpp_sources():
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
    return out


def _defines(blob: str, name: str) -> bool:
    """A C++ function DEFINITION (or declaration) named `name`."""
    pat = (r"^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w:<>,\s*&]*?\b"
           + re.escape(name) + r"\s*\(")
    return re.search(pat, blob, re.M) is not None


def audit(status: Dict[str, str], promote: bool) -> Dict[str, str]:
    srcs = _cpp_sources()
    blob = "\n".join(t for _, t in srcs)

    unresolved = [s for s, sym in ALIASES.items() if not _defines(blob, sym)]

    tiers: Dict[str, list] = {"DEF": [], "ALIAS": [], "CITE": [], "none": []}
    for stem, st in sorted(status.items()):
        if bare(st) != "pending":
            continue
        if _defines(blob, stem):
            tiers["DEF"].append(stem)
        elif stem in ALIASES and stem not in unresolved:
            tiers["ALIAS"].append(stem)
        elif re.search(re.escape(stem) + r"\.f\b", blob) or \
                re.search(r"\b" + re.escape(stem) + r"\s*\(", blob):
            tiers["CITE"].append(stem)
        else:
            tiers["none"].append(stem)

    print("=== ported.yaml audit (evidence from the C++ tree) ===")
    for k in ("DEF", "ALIAS", "CITE", "none"):
        print("  {:<6} {:>4}".format(k, len(tiers[k])))
    if unresolved:
        print("  !! ALIASES entries that do NOT resolve (fix or drop them):")
        for s in unresolved:
            print("       {} -> {}".format(s, ALIASES[s]))
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

    if do_audit:
        status = audit(status, promote)
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
