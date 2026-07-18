#!/usr/bin/env python
"""generate_all -- regenerate every code-generated artifact for the X13cpp port.

Runs, against the vendored Fortran oracle:
  * prm2hpp  -> core/prm/gen/*.hpp            (constexpr PARAMETER headers)
  * cmn2hpp  -> core/src/common/gen/*_cmn.hpp + core/src/common/x13context.hpp
  * coverage_map (refresh tools/ported.yaml)

Everything it writes is derived from oracle/fortran/ and is safe to delete and
regenerate. Hand-written headers (core/prm/srslen.hpp, core/prm/notset.hpp, the
core/include/x13 runtime helpers) are NOT touched.

Usage:  python tools/generate_all.py [<fortran_src_dir>]
"""
from __future__ import annotations

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

import prm2hpp          # noqa: E402
import cmn2hpp          # noqa: E402
import coverage_map     # noqa: E402


def main(argv):
    src = argv[0] if argv else os.path.join(ROOT, "oracle", "fortran")
    prm_gen = os.path.join(ROOT, "core", "prm", "gen")
    cmn_gen = os.path.join(ROOT, "core", "src", "common", "gen")
    cmn_ctx = os.path.join(ROOT, "core", "src", "common")

    print("[1/3] prm2hpp ->", prm_gen)
    prm2hpp.process(src, prm_gen)
    print("[2/3] cmn2hpp ->", cmn_gen)
    cmn2hpp.generate(src, cmn_gen, cmn_ctx)
    print("[3/3] coverage_map -> tools/ported.yaml")
    coverage_map.compute(src, write=True)
    print("\ngenerate_all: done.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
