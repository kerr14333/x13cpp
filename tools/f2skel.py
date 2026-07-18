#!/usr/bin/env python
"""f2skel -- emit a C++ skeleton .cpp from a Fortran .f source file.

For each SUBROUTINE/FUNCTION in the input it produces a stub whose signature
takes X13Context& (COMMON blocks travel in the context) plus the routine's dummy
arguments, a doc comment listing each argument's Fortran type, the list of
INCLUDEd .cmn blocks, and the ORIGINAL Fortran body preserved as line comments
followed by `// TODO port`. EQUIVALENCE is flagged (those routines need hand
porting).

The stub compiles on its own, but skeletons live under core/src/unported/ which
the CMake library excludes until a routine is actually ported.

Usage:
  python f2skel.py <file.f> [out_dir]          # one file
  python f2skel.py --all <src_dir> [out_dir]   # every .f in src_dir
"""
from __future__ import annotations

import os
import re
import sys
from typing import Dict, List, Optional, Tuple

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fortran_parse import (  # noqa: E402
    logical_statements, strip_trailing_comment, split_top_level, has_equivalence,
    find_includes,
)

DEFAULT_OUT = os.path.join("core", "src", "unported")

# Optional subsystem routing by filename prefix; anything unmatched -> unported.
SUBSYS_PREFIX = {
    "x11": "x11",
    "seats": "seats",
    "otl": "outlier",
    "outlier": "outlier",
    "automx": "automdl",
    "automdl": "automdl",
    "force": "force",
}


def subsystem_dir(stem: str, base_out: str) -> str:
    for pref, sub in SUBSYS_PREFIX.items():
        if stem.startswith(pref):
            return os.path.join(os.path.dirname(base_out.rstrip("/\\")) or "core/src", sub)
    return base_out


def ctype_of(ftype: str, dims: Optional[List[str]], charlen: Optional[str]) -> str:
    ft = (ftype or "").upper()
    if ft.startswith("CHARACTER"):
        base = "x13::fstring<{}>".format(charlen) if charlen else "char"
    elif ft.startswith("DOUBLE") or ft in ("REAL*8", "REAL(8)"):
        base = "double"
    elif ft.startswith("REAL"):
        base = "double"  # x13as REAL is used as working precision; keep double
    elif ft.startswith("INTEGER"):
        base = "int"
    elif ft.startswith("LOGICAL"):
        base = "bool"
    elif ft.startswith("COMPLEX"):
        base = "std::complex<double>"
    else:
        base = "double"
    if dims:  # array dummy -> pass by pointer (Fortran passes by reference)
        return base + "*"
    return base + "&"       # scalar dummy -> reference


def parse_decls(stmts: List[str]) -> Dict[str, Tuple[str, Optional[List[str]], Optional[str]]]:
    """name(lower) -> (ftype, dims, charlen) from type + DIMENSION statements."""
    from cmn2hpp import parse_item  # reuse the balanced-paren item parser
    decls: Dict[str, Tuple[str, Optional[List[str]], Optional[str]]] = {}
    for s in stmts:
        u = strip_trailing_comment(s).strip()
        head = u.upper()
        mt = re.match(r"(DOUBLE\s+PRECISION|INTEGER(?:\*\d+)?|REAL(?:\*\d+)?|LOGICAL|CHARACTER|COMPLEX(?:\*\d+)?)\b", head)
        if mt:
            ftype = mt.group(1)
            rest = u[mt.end():]
            default_len = None
            mm = re.match(r"\s*\*\s*(\d+|\([^)]*\))", rest)
            if head.startswith("CHARACTER") and mm:
                default_len = mm.group(1).strip("()")
                rest = rest[mm.end():]
            for item in split_top_level(rest):
                name, clen, dims = parse_item(item)
                if not name:
                    continue
                prev = decls.get(name.lower())
                pd = prev[1] if prev else None
                decls[name.lower()] = (ftype, dims or pd, clen or default_len or (prev[2] if prev else None))
            continue
        if head.startswith("DIMENSION"):
            for item in split_top_level(u[len("DIMENSION"):]):
                name, clen, dims = parse_item(item)
                if name and dims:
                    prev = decls.get(name.lower())
                    ft = prev[0] if prev else None
                    cl = prev[2] if prev else None
                    decls[name.lower()] = (ft or "REAL", dims, cl)
    return decls


PROC_RE = re.compile(
    r"^\s*(?:(DOUBLE\s+PRECISION|INTEGER|REAL|LOGICAL|CHARACTER(?:\*\d+)?|COMPLEX)\s+)?"
    r"(SUBROUTINE|FUNCTION)\s+([A-Za-z]\w*)\s*(\(([^)]*)\))?",
    re.IGNORECASE,
)


def parse_procedures(stmts: List[str]):
    procs = []
    for s in stmts:
        m = PROC_RE.match(s)
        if m:
            ret = m.group(1)
            kind = m.group(2).upper()
            name = m.group(3)
            args = [a.strip() for a in (m.group(5) or "").split(",") if a.strip()]
            procs.append((kind, name, args, ret))
    return procs


def emit_skeleton(path: str) -> Tuple[str, str, List[str]]:
    """Return (stem, generated_cpp_text, cmn_includes)."""
    with open(path, "r", errors="replace") as fh:
        raw = fh.read()
    stem = os.path.splitext(os.path.basename(path))[0]
    stmts = logical_statements(raw)
    procs = parse_procedures(stmts)
    decls = parse_decls(stmts)
    includes = find_includes(raw)
    cmn_incs = [i for i in includes if i.endswith(".cmn")]
    equiv = has_equivalence(raw)

    out = []
    out.append("// Skeleton generated by tools/f2skel.py from oracle/fortran/{}.f".format(stem))
    out.append("// PORT STATUS: unported. Replace the commented Fortran with a C++ port.")
    if equiv:
        out.append("// WARNING: source contains EQUIVALENCE -- needs careful hand porting.")
    out.append('#include "common/x13context.hpp"')
    out.append("")
    if cmn_incs:
        out.append("// COMMON blocks used (now members of X13Context): " + ", ".join(cmn_incs))
    out.append("")
    out.append("namespace x13 {")
    out.append("")

    for kind, name, args, ret in procs:
        rettype = "void"
        if kind == "FUNCTION":
            rettype = ctype_of(ret or decls.get(name.lower(), ("REAL",))[0], None, None).rstrip("&")
        out.append("// --- {} {} ---".format(kind, name))
        if args:
            out.append("// Arguments:")
            for a in args:
                d = decls.get(a.lower())
                if d:
                    ft, dims, clen = d
                    kindstr = "array" + str(dims) if dims else "scalar"
                    out.append("//   {:<10} {} ({})".format(a, ft, kindstr))
                else:
                    out.append("//   {:<10} (type not found)".format(a))
        params = ["X13Context& ctx"]
        for a in args:
            d = decls.get(a.lower())
            if d:
                ft, dims, clen = d
                params.append("{} {}".format(ctype_of(ft, dims, clen), a.lower()))
            else:
                params.append("double& {}".format(a.lower()))
        out.append("{} {}({}) {{".format(rettype, name.lower(), ", ".join(params)))
        out.append("    (void)ctx;")
        for a in args:
            out.append("    (void){};".format(a.lower()))
        out.append("    // TODO port. Original Fortran below.")
        out.append("}")
        out.append("")

    # Original source, preserved verbatim as line comments.
    out.append("#if 0  // ==== ORIGINAL FORTRAN (reference) ====")
    for line in raw.splitlines():
        out.append(line.rstrip())
    out.append("#endif")
    out.append("")
    out.append("}  // namespace x13")
    return stem, "\n".join(out) + "\n", cmn_incs


def process_one(path: str, base_out: str) -> str:
    stem, text, _ = emit_skeleton(path)
    out_dir = subsystem_dir(stem, base_out)
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, stem + ".cpp")
    with open(out_path, "w") as fh:
        fh.write(text)
    return out_path


def main(argv: List[str]) -> int:
    if not argv:
        print(__doc__)
        return 2
    if argv[0] == "--all":
        src = argv[1]
        base_out = argv[2] if len(argv) > 2 else DEFAULT_OUT
        n = 0
        for fn in sorted(f for f in os.listdir(src) if f.endswith(".f")):
            process_one(os.path.join(src, fn), base_out)
            n += 1
        print("f2skel: wrote {} skeletons under {}".format(n, base_out))
        return 0
    path = argv[0]
    base_out = argv[1] if len(argv) > 1 else DEFAULT_OUT
    out = process_one(path, base_out)
    print("f2skel: wrote", out)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
