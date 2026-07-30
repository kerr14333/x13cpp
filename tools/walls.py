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

Bare `abend(ctx)` calls are also excluded. There are ~90, and they are mostly
FAITHFUL reproductions of the Fortran's own error exits -- the oracle refuses
the same input. Those are ports, not gaps. Only the messaged helpers below mean
"this port declines to do something the oracle does".

Usage:
  python tools/walls.py            # print the inventory
  python tools/walls.py --write    # regenerate docs/WALLS.md
  python tools/walls.py --check    # exit 1 if docs/WALLS.md is stale
"""
from __future__ import annotations

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
CORE = os.path.join(REPO, "core", "src")
OUT = os.path.join(REPO, "docs", "WALLS.md")

# The helpers that mean "declining to run", each taking a human message.
HELPERS = ("x11_not_ported", "seats_not_ported", "xrg_not_ported", "not_ported",
           "fatal")

# `<helper>(ctx, "..." "..." ...)` -- the message may be split across lines by
# the usual C++ adjacent-literal concatenation.
CALL_RE = re.compile(
    r"\b(" + "|".join(HELPERS) + r")\s*\(\s*ctx\s*,\s*((?:\s*\"(?:[^\"\\]|\\.)*\")+)",
    re.S)
LIT_RE = re.compile(r"\"((?:[^\"\\]|\\.)*)\"")
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
# The HELPER NAME decides it wherever there is one: `*_not_ported` means exactly
# GAP, and its message carries no keyword because the name already said it --
# classifying on the message alone filed all 14 of them as faithful. Only the
# generic `fatal` needs the message inspected, since it is used for both.
GAP_HELPERS = ("x11_not_ported", "seats_not_ported", "xrg_not_ported",
               "not_ported")
GAP_RE = re.compile(
    r"not (yet )?ported|unported|deferred|not yet supported|is walled|"
    r"not implemented|not (yet )?bit-exact", re.I)


def classify(helper, msg):
    if helper in GAP_HELPERS:
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


def collect():
    rows = []
    for dp, _, fns in os.walk(CORE):
        if os.sep + "gen" in dp:
            continue
        for fn in sorted(fns):
            if not fn.endswith(".cpp"):
                continue
            path = os.path.join(dp, fn)
            with open(path, "r", errors="replace") as fh:
                text = fh.read()
            for m in CALL_RE.finditer(text):
                msg = " ".join(LIT_RE.findall(m.group(2)))
                msg = re.sub(r"\s+", " ", msg).strip()
                if len(msg) < 15:
                    continue        # a bare re-raise, not a described wall
                line = text.count("\n", 0, m.start()) + 1
                rel = os.path.relpath(path, REPO).replace(os.sep, "/")
                sub = rel.split("/")[2] if rel.startswith("core/src/") else "?"
                rows.append(dict(file=rel, line=line, sub=sub, msg=msg,
                                 kind=classify(m.group(1), msg),
                                 fref=", ".join(sorted(set(FREF_RE.findall(msg))))))
    rows.sort(key=lambda r: (r["sub"], r["file"], r["line"]))
    return rows


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

Not included, deliberately: ~90 bare `abend(ctx)` calls, which mostly reproduce
the Fortran's own error exits -- the oracle refuses those inputs too, so they are
ports rather than gaps.

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
               "`tools/walls.py`) -- a heuristic. If a wall is filed under the\n"
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
