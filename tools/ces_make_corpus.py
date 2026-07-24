#!/usr/bin/env python
"""Build a runnable CES corpus from the BLS spec zips + CE time-series flat files.

The BLS Current Employment Statistics program publishes the *actual* X-13 spec
files it uses in production (https://www.bls.gov/web/empsit/cesseasadj.htm) but
NOT the data those specs read: each spec points at a DOS path like
``FILE = 'c:\\AE1011330000.dat'``. This script pairs the two -- it pulls each
spec's NSA series out of the CE flat files, writes the ``.dat`` next to the spec,
and rewrites the ``FILE=`` paths to be relative -- producing a directory that runs
as-is under both the oracle and the C++ engine.

That makes it a robustness corpus of ~142 real production specs, which is a very
different test from the hand-generated parity corpus: uppercase keywords, wide
ARIMA model variety, user trading-day regressors, manual outlier lists, and
indirect (composite) adjustment.

Spec naming (readme.ae.txt):
  * ``AE<code>.spc``  -- adjusted DIRECTLY. 10-digit BLS NAICS-based tabcode.
  * ``CO<code>.spc``  -- indirect adjustment. A ``CO`` spec carrying
    ``series{comptype=}`` is a COMPONENT; one carrying ``composite{}`` instead of
    ``series{}`` is the composite TOTAL its components aggregate into. Components
    share the total's first 6 code digits, so they group by that prefix, and the
    group is run through a metafile (total LAST -- the oracle requires it).

Series id: ``CEU`` + the first 8 digits of the code + ``01`` (data type 01 = All
Employees, CEU = not seasonally adjusted).

Usage::

    python tools/ces_make_corpus.py --specs <dir with *.spc> \\
        --regressors <dir with FDUM8606.dat etc.> \\
        --ce-data <dir with ce.data.*.Employment> --out <corpus dir>

Fetching the inputs (bls.gov 403s default agents; a contact UA works -- see
tools/ces_seasadj_notes.md)::

    curl -A "x13cpp-research <you@example.com>" -O https://www.bls.gov/web/empsit/ces.spec.ae.zip
    curl -A "x13cpp-research <you@example.com>" -O https://www.bls.gov/web/empsit/ces.spec.other.zip
    curl -A "x13cpp-research <you@example.com>" -O https://download.bls.gov/pub/time.series/ce/ce.data.10a.MiningAndLogging.Employment
    # ... one ce.data.<NN>a.<name>.Employment per supersector (~75 MB total;
    #     ce.data.0.AllCESSeries is the 349 MB alternative)
"""
from __future__ import annotations

import argparse
import os
import re
import sys
from collections import defaultdict

FILE_RE = re.compile(r"""(FILE\s*=\s*)(['"])([^'"]*)\2""", re.I)
START_RE = re.compile(r"START\s*=\s*(\d{4})\.(\d{1,2})", re.I)


def series_id(code: str) -> str:
    """AE/CO 10-digit tabcode -> the CES NSA All-Employees series id."""
    return "CEU" + code[:8] + "01"


def load_ce_data(ce_dir: str, wanted: set[str]) -> dict[str, dict[tuple[int, int], str]]:
    """One pass per flat file: {series_id: {(year, month): value_text}}.

    Values are kept as TEXT so the .dat reproduces BLS's published precision
    exactly -- re-formatting a float here would silently change the input the
    engine and the oracle both read.
    """
    out: dict[str, dict[tuple[int, int], str]] = defaultdict(dict)
    files = sorted(f for f in os.listdir(ce_dir) if f.startswith("ce.data."))
    if not files:
        sys.exit(f"no ce.data.* files in {ce_dir}")
    for fn in files:
        with open(os.path.join(ce_dir, fn), errors="replace") as fh:
            next(fh, None)                      # header row
            for ln in fh:
                p = ln.split("\t")
                if len(p) < 4:
                    continue
                sid = p[0].strip()
                if sid not in wanted:
                    continue
                per = p[2].strip()
                if per == "M13":                # annual average -- not a period
                    continue
                out[sid][(int(p[1]), int(per[1:]))] = p[3].strip()
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--specs", required=True, help="directory holding the AE*/CO*.spc files")
    ap.add_argument("--regressors", required=True, help="directory holding FDUM8606.dat etc.")
    ap.add_argument("--ce-data", required=True, help="directory holding ce.data.*.Employment")
    ap.add_argument("--out", required=True, help="corpus directory to create")
    args = ap.parse_args()

    spec_paths = []
    for root, _, fs in os.walk(args.specs):
        spec_paths += [os.path.join(root, f) for f in fs if f.lower().endswith(".spc")]
    spec_paths.sort()
    if not spec_paths:
        sys.exit(f"no .spc files under {args.specs}")

    # Classify: direct (AE), composite total (CO with composite{}), component (CO
    # with comptype=).
    direct, totals, components = [], [], []
    text: dict[str, str] = {}
    for p in spec_paths:
        base = os.path.basename(p)[:-4]
        t = open(p, errors="replace").read()
        text[base] = t
        if re.search(r"\bcomposite\s*\{", t, re.I):
            totals.append(base)
        elif re.search(r"\bcomptype\s*=", t, re.I):
            components.append(base)
        else:
            direct.append(base)

    # Group components onto their total. The 10-digit tabcode is a NAICS hierarchy
    # zero-padded on the right, so a total's code is its components' code with the
    # extra NAICS digit(s) zeroed: CO1021200000 <- CO10212[1|2|3]0000 (5 digits
    # shared), CO6054000000 <- CO60541[1..9]0000 (only 4). Match on the LONGEST
    # zero-padded prefix that is itself a total, so the most specific total wins.
    tset = {t[2:]: t for t in totals}
    groups: dict[str, list[str]] = {t: [] for t in totals}
    orphans = []
    for c in components:
        code = c[2:]
        for k in range(len(code) - 1, 2, -1):        # never k=len: that is `c` itself
            cand = code[:k] + "0" * (len(code) - k)
            if cand in tset:
                groups[tset[cand]].append(c)
                break
        else:
            orphans.append(c)
    for t in groups:
        groups[t].sort()

    # Every spec that carries its own data needs a series pulled.
    data_specs = direct + components
    wanted = {series_id(b[2:]) for b in data_specs}
    print(f"specs: {len(spec_paths)}  direct={len(direct)} totals={len(totals)} "
          f"components={len(components)} orphan-components={len(orphans)}")
    if orphans:
        print("  orphan components (no unique total):", ", ".join(orphans))
    print(f"pulling {len(wanted)} series from {args.ce_data} ...")
    data = load_ce_data(args.ce_data, wanted)

    os.makedirs(args.out, exist_ok=True)
    datadir = os.path.join(args.out, "data")
    os.makedirs(datadir, exist_ok=True)

    # Regressor files, referenced by ~123 of the specs.
    n_reg = 0
    for fn in sorted(os.listdir(args.regressors)):
        if fn.lower().endswith(".dat"):
            with open(os.path.join(args.regressors, fn), errors="replace") as fh:
                open(os.path.join(datadir, fn), "w").write(fh.read())
            n_reg += 1

    missing, short = [], []
    for base in data_specs:
        sid = series_id(base[2:])
        obs = data.get(sid)
        if not obs:
            missing.append((base, sid))
            continue
        m = START_RE.search(text[base])
        y0, m0 = (int(m.group(1)), int(m.group(2))) if m else (min(obs)[0], 1)
        keys = sorted(k for k in obs if k >= (y0, m0))
        # The series must be contiguous from its start: X-13 free format reads
        # values positionally, so a hole would silently shift every later date.
        gaps = [keys[i] for i in range(1, len(keys))
                if (keys[i][0] * 12 + keys[i][1]) != (keys[i - 1][0] * 12 + keys[i - 1][1] + 1)]
        if gaps:
            short.append((base, sid, f"gap before {gaps[0]}"))
            continue
        if not keys:
            short.append((base, sid, f"no obs at/after {y0}.{m0:02d}"))
            continue
        with open(os.path.join(datadir, base + ".dat"), "w") as fh:
            fh.write("\n".join(obs[k] for k in keys) + "\n")

    # Rewrite the DOS FILE= paths to repo-relative ones.
    for base in sorted(text):
        def fix(mo: re.Match) -> str:
            name = os.path.basename(mo.group(3).replace("\\", "/"))
            return f'{mo.group(1)}"data/{name}"'
        open(os.path.join(args.out, base + ".spc"), "w").write(FILE_RE.sub(fix, text[base]))

    # Metafiles for the composite groups -- the total MUST come last.
    for t, comps in sorted(groups.items()):
        if not comps:
            continue
        with open(os.path.join(args.out, t + ".mta"), "w") as fh:
            fh.write("\n".join(comps + [t]) + "\n")

    print(f"wrote {len(text)} specs, {len(data_specs) - len(missing) - len(short)} data files, "
          f"{n_reg} regressor files, {sum(1 for c in groups.values() if c)} metafiles -> {args.out}")
    if missing:
        print(f"MISSING series ({len(missing)}):")
        for b, s in missing[:20]:
            print(f"  {b} -> {s}")
        if len(missing) > 20:
            print(f"  ... and {len(missing) - 20} more")
    if short:
        print(f"UNUSABLE series ({len(short)}):")
        for b, s, why in short[:20]:
            print(f"  {b} -> {s}: {why}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
