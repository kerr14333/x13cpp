#!/usr/bin/env python
"""coverage_map -- track port coverage of the 712 Fortran .f files.

Statuses:  pending | ported | gated | n-a
  pending  not yet ported (default)
  ported   C++ port complete and parity-checked
  gated    ported but behind a feature gate / not yet enabled
  n-a      not applicable -- a .f that is NOT in makefile.gf's OBJS list (e.g.
           htmlout-only variants genindex/htmlutil/mkmetahtmlfile, or superseded
           .g77 variants). These are excluded from the coverage denominator.

Reads/writes tools/ported.yaml (a plain `stem: status` mapping -- no YAML library
needed). New .f files are added on --write with a computed default (n-a if absent
from OBJS, else pending); existing statuses are preserved.

Usage:
  python coverage_map.py <fortran_src_dir> [--write]
"""
from __future__ import annotations

import os
import re
import sys
from typing import Dict, Set

HERE = os.path.dirname(os.path.abspath(__file__))
PORTED_YAML = os.path.join(HERE, "ported.yaml")
STATUSES = ("pending", "ported", "gated", "n-a")


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


def load_yaml(path: str) -> Dict[str, str]:
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


def write_yaml(path: str, status: Dict[str, str]) -> None:
    with open(path, "w") as fh:
        fh.write("# X13cpp port coverage map. Status: pending | ported | gated | n-a\n")
        fh.write("# Regenerate/refresh with: python tools/coverage_map.py oracle/fortran --write\n")
        for stem in sorted(status):
            fh.write("{}: {}\n".format(stem, status[stem]))


def compute(src_dir: str, write: bool) -> int:
    objs = objs_from_makefile(src_dir)
    stems = f_stems(src_dir)
    existing = load_yaml(PORTED_YAML)

    status: Dict[str, str] = {}
    for stem in stems:
        if stem in existing and existing[stem] in STATUSES:
            status[stem] = existing[stem]
        else:
            status[stem] = "pending" if stem in objs else "n-a"

    if write:
        write_yaml(PORTED_YAML, status)

    counts = {s: 0 for s in STATUSES}
    for v in status.values():
        counts[v] = counts.get(v, 0) + 1
    total = len(status)
    denom = total - counts["n-a"]
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
    write = "--write" in argv[1:]
    return compute(src, write)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
