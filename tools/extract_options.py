#!/usr/bin/env python
"""extract_options -- build the X-13 spec-option inventory from the oracle
Fortran readers (the authoritative ARGDIC + argptr dictionaries).

For each get*.f/gt*.f reader that uses the standard `PARAMETER(ARGDIC='...')`
+ `DATA argptr/.../` pattern, reassemble the (continuation-wrapped) ARGDIC
string and split it at the argptr byte boundaries to recover the exact argument
name list that spec accepts. Emit tools/option_matrix.json:

  { spec: { "reader": <stem>, "args": [name, ...] }, ... }

This is the DENOMINATOR for spec-option coverage: every documented argument the
parser accepts. A companion step diffs it against the corpus to find untested
args. Allowed-value sub-dictionaries (xxxDIC/xxxptr) are a later enrichment.

Usage:  python tools/extract_options.py oracle/fortran [--json out.json]
"""
from __future__ import annotations

import json
import os
import re
import sys

# reader stem -> the spec{} block it parses (Reference Manual ch.7).
READER_SPEC = {
    "getadj": "transform", "getchk": "check", "getcmp": "composite",
    "getfrc": "force", "getid": "identify", "getreg": "regression",
    "getsrs": "series", "getssp": "slidingspans", "gtarma": "arima",
    "gtauto": "automdl", "gtautx": "x11.automdl", "gtestm": "estimate",
    "gtfcst": "forecast", "gtotlr": "outlier", "gtrvst": "history",
    "gtseat": "seats", "gtspec": "spectrum", "gtxreg": "x11regression",
    "getx11": "x11", "gtmtdt": "metadata",
}

# Readers whose dictionary/pointer are not named ARGDIC/argptr.
DICT_OVERRIDE = {
    "getx11": ("X11DIC", "x11ptr"),
    "gtmtdt": ("MDTDIC", "mdtptr"),
}

# pickmdl (gtmdfl) parses its handful of args inline with no ARGDIC dictionary,
# so it is not machine-extractable here -- tracked manually.


def _reassemble_quoted(text: str, start_idx: int) -> str:
    """From the char index of the opening quote, gather a Fortran fixed-form
    quoted string across `&`-continuation lines into one Python string."""
    out = []
    i = text.index("'", start_idx) + 1
    while i < len(text):
        c = text[i]
        if c == "'":
            # '' is an escaped quote; a lone ' ends the literal.
            if i + 1 < len(text) and text[i + 1] == "'":
                out.append("'")
                i += 2
                continue
            break
        if c == "\n":
            # Skip to the continuation column (col 6 marker) of the next line.
            j = text.index("\n", i) + 1
            # A continuation line has a non-blank in col 6 (index 5). Skip the
            # leading 5 cols + the marker.
            line = text[j:text.index("\n", j)] if "\n" in text[j:] else text[j:]
            if len(line) > 5 and line[5] not in (" ", "\t"):
                i = j + 6
                continue
            i = j
            continue
        out.append(c)
        i += 1
    return "".join(out)


def extract_argdic(src: str, dic: str = "ARGDIC") -> str | None:
    m = re.search(re.escape(dic) + r"\s*=\s*'", src)
    if not m:
        return None
    return _reassemble_quoted(src, m.start())


def extract_argptr(src: str, ptr: str = "argptr") -> list[int] | None:
    # DATA <ptr>/1,5,10,.../  (may wrap across continuation lines)
    m = re.search(r"DATA\s+" + re.escape(ptr) + r"\s*/", src, re.IGNORECASE)
    if not m:
        return None
    tail = src[m.end():]
    end = tail.index("/")
    nums = re.findall(r"-?\d+", tail[:end])
    return [int(n) for n in nums]


def split_args(argdic: str, argptr: list[int]) -> list[str]:
    # argptr is 1-based Fortran positions, 0:PARG (PARG+1 entries).
    out = []
    for k in range(len(argptr) - 1):
        lo, hi = argptr[k] - 1, argptr[k + 1] - 1
        out.append(argdic[lo:hi])
    return out


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    src_dir = sys.argv[1]
    out_path = None
    if "--json" in sys.argv:
        out_path = sys.argv[sys.argv.index("--json") + 1]

    matrix = {}
    for stem, spec in sorted(READER_SPEC.items()):
        path = os.path.join(src_dir, stem + ".f")
        if not os.path.exists(path):
            print(f"  [miss] {stem}.f not found", file=sys.stderr)
            continue
        src = open(path, encoding="utf-8", errors="replace").read()
        dic, ptr = DICT_OVERRIDE.get(stem, ("ARGDIC", "argptr"))
        argdic = extract_argdic(src, dic)
        argptr = extract_argptr(src, ptr)
        if not argdic or not argptr:
            print(f"  [skip] {stem}: no ARGDIC/argptr", file=sys.stderr)
            continue
        args = split_args(argdic, argptr)
        matrix[spec] = {"reader": stem, "args": args}
        print(f"  {spec:20s} ({stem}): {len(args):2d} args  {args}")

    total = sum(len(v["args"]) for v in matrix.values())
    print(f"\n=== {len(matrix)} specs, {total} total arguments ===")
    if out_path:
        json.dump(matrix, open(out_path, "w"), indent=2)
        print(f"wrote {out_path}")

    if "--corpus" in sys.argv:
        corpus = sys.argv[sys.argv.index("--corpus") + 1]
        _coverage(matrix, corpus)
    return 0


_BLOCK_RE = re.compile(r"([a-z][a-z0-9]*)\s*\{", re.IGNORECASE)


def _args_by_block(txt: str) -> dict[str, set[str]]:
    """Map each spec block present in one .spc to the set of `arg=` names it
    sets. Brace-matched so args are attributed to the right block."""
    out: dict[str, set[str]] = {}
    for m in _BLOCK_RE.finditer(txt):
        name = m.group(1).lower()
        i = m.end()
        depth = 1
        while i < len(txt) and depth:
            if txt[i] == "{":
                depth += 1
            elif txt[i] == "}":
                depth -= 1
            i += 1
        body = txt[m.end():i]
        args = {a.group(1).lower() for a in re.finditer(r"([a-z][a-z0-9]+)\s*=", body)}
        out.setdefault(name, set()).update(args)
    return out


# spec key in the matrix -> the .spc block name that carries its args.
SPEC_BLOCK = {
    "series": "series", "transform": "transform", "arima": "arima",
    "automdl": "automdl", "check": "check", "composite": "composite",
    "estimate": "estimate", "force": "force", "forecast": "forecast",
    "history": "history", "identify": "identify", "outlier": "outlier",
    "regression": "regression", "seats": "seats", "slidingspans": "slidingspans",
    "spectrum": "spectrum", "x11regression": "x11regression",
    "x11.automdl": "x11", "x11": "x11", "metadata": "metadata",
}


def _coverage(matrix: dict, corpus_dir: str) -> None:
    """Per-spec-block coverage: for each spec, which accepted args never appear
    inside that block in any corpus .spc. Brace-matched, so accurate."""
    seen: dict[str, set[str]] = {}
    nspecs = 0
    for root, _d, files in os.walk(corpus_dir):
        for fn in files:
            if not fn.endswith(".spc"):
                continue
            nspecs += 1
            txt = open(os.path.join(root, fn), encoding="utf-8",
                       errors="replace").read().lower()
            for block, args in _args_by_block(txt).items():
                seen.setdefault(block, set()).update(args)
    print(f"\n=== corpus coverage ({nspecs} specs scanned, per-block) ===")
    grand_untested = 0
    for spec, info in sorted(matrix.items()):
        block = SPEC_BLOCK.get(spec)
        block_seen = seen.get(block, set()) if block else set()
        args = info["args"]
        untested = [a for a in args if a not in block_seen]
        grand_untested += len(untested)
        cov = 100 * (len(args) - len(untested)) / len(args) if args else 0
        flag = "" if not untested else "  UNTESTED: " + " ".join(untested)
        print(f"  {spec:20s} {len(args)-len(untested):2d}/{len(args):2d} "
              f"({cov:3.0f}%){flag}")
    total = sum(len(v["args"]) for v in matrix.values())
    print(f"\n  {total - grand_untested}/{total} args tested in-block; "
          f"{grand_untested} never exercised in the corpus.")


if __name__ == "__main__":
    sys.exit(main())
