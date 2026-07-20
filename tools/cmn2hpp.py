#!/usr/bin/env python
"""cmn2hpp -- convert Fortran COMMON-block include files (.cmn) to C++ structs.

For each .cmn it emits  core/src/common/gen/<name>_cmn.hpp  containing a struct
`<name>_cmn` whose members mirror the COMMON block variables (in COMMON-statement
order, which is the true memory layout), with Fortran array dimensions resolved
from the .prm parameter values. It then emits  core/src/common/x13context.hpp
aggregating one member per .cmn struct into `struct X13Context`.

Type mapping:
  DOUBLE PRECISION -> double     INTEGER -> int      LOGICAL -> bool
  REAL -> float                  CHARACTER*n (scalar) -> x13::fstring<n>
  arrays  -> x13::farray1 / farray1lb / farray2 / farray2lb (1-based, col-major)

EQUIVALENCE statements cannot be represented as plain struct members; any .cmn
containing one is reported and its EQUIVALENCE is skipped (the file then needs
hand porting). (In the vendored v1.1 b61 sources no .cmn actually uses one.)

Usage:  python cmn2hpp.py <fortran_src_dir> <out_dir> [<context_dir>]
"""
from __future__ import annotations

import os
import re
import sys
from typing import Dict, List, Optional, Tuple

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from fortran_parse import (  # noqa: E402
    logical_statements, strip_trailing_comment, split_top_level,
)
from prm2hpp import (  # noqa: E402
    gather_symbols, resolve_all, parse_parameters, parse_type_decls, Sym,
)

CPP_KEYWORDS = {
    "alignas", "alignof", "and", "asm", "auto", "bool", "break", "case", "catch",
    "char", "class", "const", "continue", "default", "delete", "do", "double",
    "else", "enum", "explicit", "export", "extern", "false", "float", "for",
    "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new",
    "not", "operator", "or", "private", "protected", "public", "register", "return",
    "short", "signed", "sizeof", "static", "struct", "switch", "template", "this",
    "throw", "true", "try", "typedef", "typename", "union", "unsigned", "using",
    "virtual", "void", "volatile", "while", "xor", "template",
}


def member_name(fortran: str) -> str:
    nm = fortran.lower()
    if nm in CPP_KEYWORDS:
        nm += "_"
    return nm


def canon_type(tok: str) -> str:
    t = tok.strip().upper()
    if t.startswith("DOUBLE"):
        return "double"
    if t.startswith("INTEGER"):
        return "int"
    if t.startswith("REAL"):
        return "float"
    if t.startswith("LOGICAL"):
        return "bool"
    if t.startswith("CHARACTER"):
        return "char"
    return "double"


def implicit_type(name: str) -> str:
    return "int" if name[0].lower() in "ijklmn" else "double"


_NAME_RE = re.compile(r"(?<![\w.])[A-Za-z_]\w*")


def eval_dim(expr: str, values: Dict[str, object]) -> Optional[int]:
    e = expr.strip()
    if e == "":
        return None
    env: Dict[str, object] = {}
    for nm in set(_NAME_RE.findall(e)):
        if nm in values:
            env[nm] = values[nm]
        else:
            found = [v for k, v in values.items() if k.lower() == nm.lower()]
            if not found:
                return None
            env[nm] = found[0]
    try:
        return int(eval(e, {"__builtins__": {}}, env))  # noqa: S307 (trusted input)
    except Exception:
        return None


def parse_item(item: str) -> Tuple[str, Optional[str], Optional[List[str]]]:
    """Parse a declared entity: NAME [*len] [(dims)] (in any order)."""
    item = item.strip()
    m = re.match(r"([A-Za-z]\w*)", item)
    if not m:
        return "", None, None
    name = m.group(1)
    rest = item[m.end():].strip()
    charlen = None
    # CHARACTER length: *n or *(expr) -- the (expr) may itself contain parens.
    if rest.startswith("*"):
        after = rest[1:].lstrip()
        if after.startswith("("):
            close = _match(after, 0)
            charlen = after[1:close]
            rest = after[close + 1:].strip()
        else:
            dm = re.match(r"(\d+)", after)
            if dm:
                charlen = dm.group(1)
                rest = after[dm.end():].strip()
    # Array dimensions: the next top-level parenthesis group (balanced).
    dims = None
    if rest.startswith("("):
        close = _match(rest, 0)
        dims = [d.strip() for d in split_top_level(rest[1:close])]
        rest = rest[close + 1:].strip()
    # A trailing *len after dims (NAME(dims)*len form).
    if charlen is None and rest.startswith("*"):
        after = rest[1:].lstrip()
        if after.startswith("("):
            close = _match(after, 0)
            charlen = after[1:close]
        else:
            dm = re.match(r"(\d+)", after)
            if dm:
                charlen = dm.group(1)
    return name, charlen, dims


def _match(s: str, open_idx: int) -> int:
    depth = 0
    for i in range(open_idx, len(s)):
        if s[i] == "(":
            depth += 1
        elif s[i] == ")":
            depth -= 1
            if depth == 0:
                return i
    return len(s) - 1


class Var:
    __slots__ = ("ftype", "charlen", "dims")

    def __init__(self):
        self.ftype: Optional[str] = None
        self.charlen: Optional[str] = None
        self.dims: Optional[List[str]] = None


def parse_cmn(stmts: List[str]):
    """Return (vars: {name->Var}, commons: [(cname, [names])], has_equiv)."""
    vars: Dict[str, Var] = {}
    commons: List[Tuple[str, List[str]]] = []
    has_equiv = False

    def get(name):
        key = name.lower()
        v = vars.get(key)
        if v is None:
            v = Var()
            vars[key] = v
        return v

    for s in stmts:
        u = strip_trailing_comment(s).strip()
        if u == "":
            continue
        head = u.upper()
        if head.startswith("EQUIVALENCE"):
            has_equiv = True
            continue
        mt = re.match(r"(DOUBLE\s+PRECISION|INTEGER|REAL|LOGICAL|CHARACTER)\b", head)
        if mt:
            ftype = canon_type(mt.group(1))
            rest = u[mt.end():]
            default_len = None
            mm = re.match(r"\s*\*\s*(\d+|\([^)]*\))", rest)
            if ftype == "char" and mm:
                default_len = mm.group(1).strip("()")
                rest = rest[mm.end():]
            for item in split_top_level(rest):
                name, clen, dims = parse_item(item)
                if not name:
                    continue
                v = get(name)
                v.ftype = ftype
                if ftype == "char":
                    v.charlen = clen or default_len
                if dims:
                    v.dims = dims
            continue
        if head.startswith("DIMENSION"):
            rest = u[len("DIMENSION"):]
            for item in split_top_level(rest):
                name, _clen, dims = parse_item(item)
                if name and dims:
                    get(name).dims = dims
            continue
        if head.startswith("COMMON"):
            body = u[len("COMMON"):]
            for m in re.finditer(r"/\s*([A-Za-z]\w*)?\s*/\s*([^/]*)", body):
                cname = m.group(1) or ""
                names = []
                for item in split_top_level(m.group(2)):
                    name, _clen, dims = parse_item(item)
                    if not name:
                        continue
                    if dims:
                        get(name).dims = dims
                    names.append(name)
                commons.append((cname, names))
            continue
    return vars, commons, has_equiv


def dim_bounds(expr: str, values) -> Optional[Tuple[int, int]]:
    """Return (lower_bound, size) for a single dimension expr, or None."""
    if ":" in expr:
        lo_s, hi_s = expr.split(":", 1)
        lo = eval_dim(lo_s, values)
        hi = eval_dim(hi_s, values)
        if lo is None or hi is None:
            return None
        return lo, hi - lo + 1
    n = eval_dim(expr, values)
    if n is None:
        return None
    return 1, n


def elem_type(v: Var, values) -> Optional[str]:
    ft = v.ftype
    if ft == "char":
        n = eval_dim(v.charlen, values) if v.charlen else 1
        if n is None:
            return None
        return "x13::fstring<{}>".format(n)
    return {"double": "double", "int": "int", "float": "float", "bool": "bool"}.get(ft or "double", "double")


def member_decl(name: str, v: Var, values) -> Tuple[str, str]:
    """Return (cpp_type, comment) for a variable, or (None, reason)."""
    et = elem_type(v, values)
    if et is None:
        return None, "unresolved char length '{}'".format(v.charlen)
    if not v.dims:
        return et, ""
    bounds = [dim_bounds(d, values) for d in v.dims]
    if any(b is None for b in bounds):
        return None, "unresolved dim(s) {}".format(v.dims)
    dimstr = ",".join(v.dims)
    if len(bounds) == 1:
        lo, n = bounds[0]
        if lo == 1:
            return "x13::farray1<{}, {}>".format(et, n), dimstr
        return "x13::farray1lb<{}, {}, {}>".format(et, lo, n), dimstr
    if len(bounds) == 2:
        (lo1, n1), (lo2, n2) = bounds
        if lo1 == 1 and lo2 == 1:
            return "x13::farray2<{}, {}, {}>".format(et, n1, n2), dimstr
        return "x13::farray2lb<{}, {}, {}, {}, {}>".format(et, lo1, n1, lo2, n2), dimstr
    return None, "unsupported {}-D array".format(len(bounds))


def build_dim_symbols(src_dir: str, cmn_files):
    """Build the parameter value table for resolving COMMON dimensions. Sources,
    in priority order: .prm and .i shared includes, PARAMETERs defined inside the
    .cmn files themselves, and finally a targeted scan of .f files for any leftover
    dimension symbols (e.g. PR = PLEN/4, defined per-routine, not in an include)."""
    # .prm BEFORE .i: gather_symbols is "first definition wins", and a stale
    # duplicate include (srslen.i: PYRS=PYR1+10=75) disagrees with the live one
    # (srslen.prm: PYRS=PYR1+20=85). srslen.prm is included by 393 .f files incl.
    # every .cmn consumer; srslen.i by bench.f alone. Gathering .prm first lets the
    # real value win (else "srslen.i" < "srslen.prm" sorts first and undersizes
    # PYRS-derived COMMON arrays, e.g. xtrm.cmn Stdev(PYRS+1) -> 76 not 86).
    table = gather_symbols(src_dir, (".prm",))
    gather_symbols(src_dir, (".i",), table)
    gather_symbols(src_dir, (".cmn",), table)
    resolve_all(table)
    values = {n: s.value for n, s in table.items() if s.value is not None}

    # Which dimension/char-length identifiers do the .cmn arrays still need?
    needed = set()
    for fn in cmn_files:
        with open(os.path.join(src_dir, fn), "r", errors="replace") as fh:
            vars, _c, _e = parse_cmn(logical_statements(fh.read()))
        for v in vars.values():
            for expr in list(v.dims or []) + ([v.charlen] if v.charlen else []):
                for nm in _NAME_RE.findall(expr):
                    needed.add(nm)
    missing = {n for n in needed if n not in values and not any(k.lower() == n.lower() for k in values)}
    if missing:
        # Scan .f PARAMETERs for the missing names; accept only consistent values.
        ftab: Dict[str, Sym] = dict(table)
        for fn in sorted(f for f in os.listdir(src_dir) if f.endswith(".f")):
            with open(os.path.join(src_dir, fn), "r", errors="replace") as fh:
                stmts = logical_statements(fh.read())
            decls = parse_type_decls(stmts)
            for name, expr in parse_parameters(stmts):
                if name.lower() in {m.lower() for m in missing} and name not in ftab:
                    ftype = decls.get(name, {}).get("ftype", "int")
                    ftab[name] = Sym(name, ftype, decls.get(name, {}).get("charlen"), expr, fn)
        resolve_all(ftab)
        for n, s in ftab.items():
            if s.value is not None and n not in values:
                values[n] = s.value
    return values


def generate(src_dir: str, out_dir: str, ctx_dir: str):
    cmn_files = sorted(f for f in os.listdir(src_dir) if f.endswith(".cmn"))
    values = build_dim_symbols(src_dir, cmn_files)
    os.makedirs(out_dir, exist_ok=True)
    os.makedirs(ctx_dir, exist_ok=True)

    equiv_files: List[str] = []
    warnings: List[str] = []
    structs: List[Tuple[str, str]] = []  # (struct_name, member_name)

    for fn in cmn_files:
        stem = os.path.splitext(fn)[0]
        with open(os.path.join(src_dir, fn), "r", errors="replace") as fh:
            stmts = logical_statements(fh.read())
        vars, commons, has_equiv = parse_cmn(stmts)
        if has_equiv:
            equiv_files.append(fn)

        struct = stem + "_cmn"
        guard = "X13_CMN_{}_HPP".format(re.sub(r"\W", "_", stem).upper())
        lines = [
            "// Generated by tools/cmn2hpp.py from oracle/fortran/{}. DO NOT EDIT.".format(fn),
            "#ifndef {g}\n#define {g}".format(g=guard),
            '#include "x13/farray.hpp"',
            '#include "x13/fstring.hpp"',
            "",
            "namespace x13 {",
            "",
            "// COMMON blocks: " + ", ".join("/{}/".format(c or "(blank)") for c, _ in commons),
            "struct {} {{".format(struct),
        ]
        seen = set()
        for cname, names in commons:
            for nm in names:
                if nm.lower() in seen:
                    continue
                seen.add(nm.lower())
                v = vars.get(nm.lower()) or Var()
                if v.ftype is None:
                    v.ftype = implicit_type(nm)
                    warnings.append("{}: implicit type for {} -> {}".format(fn, nm, v.ftype))
                ctype, comment = member_decl(nm, v, values)
                if ctype is None:
                    warnings.append("{}: {} {}".format(fn, nm, comment))
                    lines.append("    // SKIPPED {} ({})".format(nm, comment))
                    continue
                c = "  // {}({})".format(nm, comment) if comment else "  // {}".format(nm)
                lines.append("    {} {};{}".format(ctype, member_name(nm), c))
        lines.append("};")
        lines.append("")
        lines.append("}  // namespace x13")
        lines.append("#endif  // {}".format(guard))
        with open(os.path.join(out_dir, struct + ".hpp"), "w") as fh:
            fh.write("\n".join(lines) + "\n")
        structs.append((struct, stem))

    # Aggregate context header.
    guard = "X13_CONTEXT_HPP"
    ctx = [
        "// Generated by tools/cmn2hpp.py. Aggregates every COMMON-block struct. DO NOT EDIT.",
        "#ifndef {g}\n#define {g}".format(g=guard),
        '#include "x13/x13error.hpp"',
        '#include "x13/channels.hpp"',
    ]
    for struct, _ in structs:
        ctx.append('#include "common/gen/{}.hpp"'.format(struct))
    ctx += [
        "",
        "namespace x13 {",
        "",
        "// One member per COMMON-block include; every ported routine takes X13Context&.",
        "struct X13Context {",
        "    ErrorState error_state;    // program-wide fatal flag (see error_cmn too)",
        "    ChannelRegistry channels_; // Fortran unit-number output buffers",
    ]
    for struct, member in structs:
        ctx.append("    {} {};".format(struct, member_name(member)))
    ctx += ["};", "", "}  // namespace x13", "#endif  // {}".format(guard)]
    # WARNING: x13context.hpp is HAND-MAINTAINED beyond the generated members
    # (LexState/SaveState/FcstOut/TrnAicResult, extra includes). Overwriting it
    # wholesale silently drops those and breaks the build. So write the generated
    # struct to a sidecar when the real file exists; reconcile by hand. Only the
    # per-COMMON gen/*.hpp headers are safe to overwrite blindly.
    ctx_path = os.path.join(ctx_dir, "x13context.hpp")
    if os.path.exists(ctx_path):
        ctx_path += ".generated"
        print("  NOTE: x13context.hpp exists (hand-maintained); wrote generated "
              "struct to x13context.hpp.generated -- reconcile by hand.")
    with open(ctx_path, "w") as fh:
        fh.write("\n".join(ctx) + "\n")

    print("cmn2hpp: {} common blocks -> {} structs".format(len(cmn_files), len(structs)))
    if equiv_files:
        print("  EQUIVALENCE (needs hand porting):", ", ".join(equiv_files))
    else:
        print("  EQUIVALENCE: none in any .cmn")
    if warnings:
        print("  {} warnings:".format(len(warnings)))
        for w in warnings:
            print("    -", w)
    return 0


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(2)
    src = sys.argv[1]
    out = sys.argv[2]
    ctx = sys.argv[3] if len(sys.argv) > 3 else os.path.dirname(out)
    sys.exit(generate(src, out, ctx))
