#!/usr/bin/env python
"""walls -- inventory every place the engine REFUSES to run, straight from the
source.

The port's standing rule is that an unported branch must fatal with a message
naming the Fortran line, never silently do the default thing -- because the
characteristic defect here is an option the parser accepts and the engine
ignores, which is indistinguishable from a working feature until somebody diffs
against the oracle.

That rule has a useful side effect: **the walls are a machine-readable answer to
"what is not ported yet", written as code.** Unlike a prose "still open" list,
this one cannot go stale -- delete the wall and it leaves the inventory; add one
and it appears. `docs/WALLS.md` is generated from it so documents can link to a
current list instead of maintaining their own.

What this deliberately does NOT do: cross-check the inventory against prose.
There are ~236 "still open / deferred / not ported" phrases across tools/*.md
and CLAUDE.md against ~20 walls -- a 12:1 ratio, because most of that prose is
narrative history ("deferred, then closed"), which is correct and append-only. A
checker over that would be almost all false positives, and a noisy checker gets
ignored, which is worse than no checker.

WHAT COUNTS AS A REFUSAL (rewritten 2026-08-21). This used to be "a call to one
of nine named helpers", and it missed two whole shapes:

  * `inpter(ctx, PERROR, pos, "...")` -- the PARSER's refusal channel. It stops
    the run by clearing `inptok`, never by `abend`, so a matcher looking for
    helpers or for `abend` saw nothing. Six GAP-class parser refusals were
    invisible, including the one that declines every spec the M1 parser does not
    know.
  * a raw `writln(...)` + `abend(ctx)` pair -- a wall that simply did not use a
    helper.

And the helper list itself is now DERIVED rather than typed, because the typed
one had the bug described at HELPER_DEF_RE.

Messageless `abend(ctx)` calls are still excluded from the inventory, but no
longer silently: `--audit` lists them. Most reproduce the Fortran's own error
exits and are ports rather than gaps; the ones that are not are exactly the
"hole with the lights off" this project has been bitten by, and now there is a
command that names them.

Usage:
  python tools/walls.py            # print the inventory
  python tools/walls.py --write    # regenerate docs/WALLS.md
  python tools/walls.py --check    # exit 1 if docs/WALLS.md is stale
  python tools/walls.py --audit    # helper set + every messageless `abend`
"""
from __future__ import annotations

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
CORE = os.path.join(REPO, "core", "src")
OUT = os.path.join(REPO, "docs", "WALLS.md")

# One C++ string literal, adjacent-concatenation friendly.
_LIT = r'"(?:[^"\\]|\\.)*"'
LIT_RE = re.compile(_LIT)
_MSG = r'((?:\s*' + _LIT + r')+)'

# The helpers that mean "declining to run", each taking a human message.
#
# DERIVED, not a literal list. The tuple that used to live here was matched with
# `\b`, and `\bnot_ported` cannot match inside `agr3_not_ported` because `_` is a
# word character -- so two walls landed invisible and the printed count did not
# move (M5 note 94). A name-shaped rule cannot have that bug: anything DEFINED in
# core/src whose name ends in `not_ported` or is exactly `fatal`, and whose body
# calls `abend`, IS a refusal helper. Add one and it joins the inventory by
# itself; misspell one and it still joins, under its own name.
HELPER_DEF_RE = re.compile(
    r"^(?!\s*//)[\w:<>,&*\s\[\]]*?(\w*not_ported|fatal)\s*\(\s*X13Context\s*&",
    re.M)


def _discover_helpers():
    """(sorted helper names, {path: [(first_line, last_line), ...]}).

    The spans are the helper BODIES: a helper's own `abend` is the helper
    working as designed, not a wall, and must not be counted as one.
    """
    names, spans = set(), {}
    for dp, _, fns in os.walk(CORE):
        if os.sep + "gen" in dp:
            continue
        for fn in sorted(fns):
            if not fn.endswith((".cpp", ".hpp")):
                continue
            path = os.path.join(dp, fn)
            with open(path, "r", errors="replace") as fh:
                text = fh.read()
            for m in HELPER_DEF_RE.finditer(text):
                body = text[m.end():m.end() + 1500]
                if "abend(" not in body:
                    continue          # a declaration, or a helper that returns
                names.add(m.group(1))
                first = text.count("\n", 0, m.start()) + 1
                last = first + body.count("\n", 0, body.index("abend(")) + 2
                spans.setdefault(path, []).append((first, last))
    return sorted(names), spans


HELPERS, HELPER_SPANS = _discover_helpers()

# `<helper>(ctx, "..." "..." ...)`; the message may be split across lines.
CALL_RE = re.compile(r"\b(" + "|".join(HELPERS or ["__none__"]) +
                     r")\s*\(\s*ctx\s*,\s*" + _MSG, re.S)

# The two helper-less refusal shapes.
INPTER_RE = re.compile(r"\binpter\s*\(\s*ctx\s*,\s*(\w+)\s*,[^,]*,\s*" + _MSG,
                       re.S)
ABEND_RE = re.compile(r"\babend\s*\(\s*ctx\s*\)")
MSGWIN = 14          # lines to look back from an `abend` for its message

# A Fortran citation inside the message: `foo.f:123` or `foo.f:12-34`.
FREF_RE = re.compile(r"\b([a-z0-9_]+\.f:[\d\-,: ]*\d)")

# A messaged refusal is one of two very different things, and the inventory is
# only useful if it says which:
#   GAP       this port declines to do something the ORACLE does -- a real hole.
#   FAITHFUL  the oracle refuses the same input, so the refusal IS the port
#             (e.g. "No ARIMA models stored in <file>" is automx.f:1050).
#
# A third shape files as GAP too: the code IS transcribed but does not yet agree
# with the oracle, so it refuses rather than emitting wrong numbers ("not yet
# bit-exact"). That is a hole in the port, not a faithful refusal.
#
# The HELPER NAME decides it wherever there is one: `*not_ported` means exactly
# GAP, and its message carries no keyword because the name already said it --
# classifying on the message alone filed all 14 of them as faithful. The generic
# `fatal`, and both helper-less shapes, need the message inspected.
GAP_RE = re.compile(
    r"not (yet )?ported|unported|deferred|not yet supported|is walled|"
    r"not implemented|not (yet )?bit-exact", re.I)


def classify(helper, msg):
    if helper and helper.endswith("not_ported"):
        return "GAP"
    return "GAP" if GAP_RE.search(msg) else "FAITHFUL"


SUBSYSTEMS = {
    "regarima": "regARIMA", "automdl": "Automatic model selection",
    "x11": "X-11", "seats": "SEATS", "outlier": "Outliers",
    "transform": "Transform", "specparse": "Spec parser", "diag": "Diagnostics",
    "force": "Force", "numeric": "Numerics", "tables": "Tables",
    "driver": "Drivers", "common": "Common", "composite": "Composite",
    "api": "Public API",
}


def _msg_of(group):
    msg = " ".join(lit[1:-1] for lit in LIT_RE.findall(group))
    return re.sub(r"\s+", " ", msg).strip()


def _row(path, line, msg, kind):
    rel = os.path.relpath(path, REPO).replace(os.sep, "/")
    sub = rel.split("/")[2] if rel.startswith("core/src/") else "?"
    return dict(file=rel, line=line, sub=sub, msg=msg, kind=kind,
                fref=", ".join(sorted(set(FREF_RE.findall(msg)))))


def _in_helper_body(path, line):
    return any(a <= line <= b for a, b in HELPER_SPANS.get(path, ()))


def collect(with_bare=False):
    rows, bare = [], []
    for dp, _, fns in os.walk(CORE):
        if os.sep + "gen" in dp:
            continue
        for fn in sorted(fns):
            if not fn.endswith(".cpp"):
                continue
            path = os.path.join(dp, fn)
            with open(path, "r", errors="replace") as fh:
                text = fh.read()
            lines = text.split("\n")
            claimed = set()          # lines already explained by a row

            for m in CALL_RE.finditer(text):
                msg = _msg_of(m.group(2))
                if len(msg) < 15:
                    continue        # a bare re-raise, not a described wall
                line = text.count("\n", 0, m.start()) + 1
                if _in_helper_body(path, line):
                    continue
                rows.append(_row(path, line, msg, classify(m.group(1), msg)))
                claimed.update(range(line, line + 6))

            for m in INPTER_RE.finditer(text):
                msg = _msg_of(m.group(2))
                if len(msg) < 15 or not GAP_RE.search(msg):
                    continue        # 300+ inpter calls are ordinary parse errors
                line = text.count("\n", 0, m.start()) + 1
                rows.append(_row(path, line, msg, "GAP"))
                claimed.update(range(line, line + 6))

            for i, ln in enumerate(lines, start=1):
                if not ABEND_RE.search(ln) or _in_helper_body(path, i):
                    continue
                if i in claimed:
                    continue
                # Look back at most MSGWIN lines, and never past a top-level
                # `}` -- a message belonging to the function ABOVE is not this
                # refusal's message, and attributing it would both invent a
                # wall and hide a bare `abend` from --audit.
                top = max(0, i - 1 - MSGWIN)
                for j in range(i - 2, top - 1, -1):
                    if lines[j] == "}":
                        top = j + 1
                        break
                win = "\n".join(lines[top:i])
                if CALL_RE.search(win) or INPTER_RE.search(win):
                    continue        # already inventoried above
                msgs = [lit[1:-1] for lit in LIT_RE.findall(win)]
                gap = [t for t in msgs if GAP_RE.search(t)]
                if gap:
                    msg = re.sub(r"\s+", " ", " ".join(msgs)).strip()
                    rows.append(_row(path, i, msg, "GAP"))
                elif not msgs:
                    bare.append(_row(path, i, "", "BARE"))
    rows.sort(key=lambda r: (r["sub"], r["file"], r["line"]))
    bare.sort(key=lambda r: (r["file"], r["line"]))
    return (rows, bare) if with_bare else rows


HEADER = """\
# Where the engine declines to run

**Generated -- do not edit.** `python tools/walls.py --write`

This port's rule is that an unported branch **fatals with a message naming the
Fortran line**, never silently does the default thing. The characteristic defect
in this codebase is an option the parser accepts and the engine ignores --
`OUTCOME: OK` with wrong numbers -- and a wall is what makes that impossible.

So this list is the honest answer to *"what is not ported yet"*, derived from the
source rather than from anybody's memory. If a wall is here, a spec that reaches
it stops with this message. If a feature is NOT here and not gated by a test,
that is the dangerous case and worth a look.

Three refusal shapes are collected: the `*not_ported` / `fatal` helpers, the
parser's `inpter(ctx, PERROR, ...)` channel (which refuses by clearing `inptok`,
not by `abend`), and a raw message-then-`abend` pair. Messageless `abend(ctx)`
calls are NOT here -- they mostly reproduce the Fortran's own error exits, so
they are ports rather than gaps -- but they are no longer invisible either:
`python tools/walls.py --audit` lists every one of them.

"""


def render(rows):
    gaps = [r for r in rows if r["kind"] == "GAP"]
    faith = [r for r in rows if r["kind"] != "GAP"]
    out = [HEADER]
    _section(out, gaps, "Gaps -- the oracle does this, the port declines",
             "The honest to-do list. Each names the Fortran it would have to\n"
             "reproduce.")
    _section(out, faith, "Faithful refusals -- the oracle declines too",
             "Not gaps. The oracle rejects the same input, so refusing IS the\n"
             "port. Listed so the count above is not mistaken for the whole\n"
             "inventory.")
    out.append("\n---\n")
    out.append("*Classification is by keyword on the message text (`GAP_RE` in\n"
               "`tools/walls.py`) -- a heuristic, except for the `*not_ported`\n"
               "helpers, where the NAME decides. If a wall is filed under the\n"
               "wrong heading, fix its MESSAGE: that string is what a user\n"
               "actually sees when a run stops.*")
    return "\n".join(out).rstrip() + "\n"


def _section(out, rows, title, blurb):
    out.append("\n# {}\n".format(title))
    out.append(blurb)
    out.append("\n**{} walls.**\n".format(len(rows)))
    cur = None
    for r in rows:
        if r["sub"] != cur:
            cur = r["sub"]
            out.append("\n## {}\n".format(SUBSYSTEMS.get(cur, cur)))
        out.append("- **`{}:{}`**".format(r["file"], r["line"]))
        out.append("  {}".format(r["msg"]))
        if r["fref"]:
            out.append("  *Fortran:* `{}`".format(r["fref"]))
        out.append("")


def main(argv):
    if "--audit" in argv:
        rows, bare = collect(with_bare=True)
        print("refusal helpers discovered ({}): {}".format(
            len(HELPERS), ", ".join(HELPERS)))
        g = sum(1 for r in rows if r["kind"] == "GAP")
        print("inventoried: {} gaps, {} faithful".format(g, len(rows) - g))
        print("")
        print("messageless `abend(ctx)` -- NOT in the inventory ({}):".format(
            len(bare)))
        for r in bare:
            print("  {}:{}".format(r["file"], r["line"]))
        print("")
        print("These carry no message THIS TOOL CAN SEE -- the enclosing")
        print("function has no string literal between the previous top-level")
        print("`}` and the abend. Most are faithful: the Fortran abends there")
        print("too, and where it prints, it prints from the callee (rgarma,")
        print("rdotlr, ...), which this tool cannot follow. The ones that are")
        print("NOT are the 'hole with the lights off' -- an unported branch")
        print("that stops with an empty ===ERR=== block and appears in no")
        print("inventory. Check a new entry against its .f before leaving it.")
        return 0
    rows = collect()
    text = render(rows)
    if "--write" in argv:
        os.makedirs(os.path.dirname(OUT), exist_ok=True)
        with open(OUT, "w", newline="") as fh:
            fh.write(text)
        g = sum(1 for r in rows if r["kind"] == "GAP")
        print("wrote {} ({} gaps, {} faithful)".format(
            os.path.relpath(OUT, REPO), g, len(rows) - g))
        return 0
    if "--check" in argv:
        if not os.path.exists(OUT):
            print("STALE: docs/WALLS.md does not exist")
            return 1
        with open(OUT, "r", newline="") as fh:
            if fh.read() != text:
                print("STALE: docs/WALLS.md differs from the source")
                print("run: python tools/walls.py --write")
                return 1
        g = sum(1 for r in rows if r["kind"] == "GAP")
        print("walls: up to date ({} gaps, {} faithful)".format(g, len(rows) - g))
        return 0
    print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
