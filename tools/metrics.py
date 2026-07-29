#!/usr/bin/env python
"""metrics -- the ONE authoritative source for every countable claim about this
project, and the only place a document should get one from.

Why this exists. Numbers copied out of a terminal and pasted into prose go stale
silently and nobody notices until someone reads the document aloud. A
2026-07-29 audit found `docs/PROJECT_SUMMARY.md` -- the deliverable report --
advertising "1088 passed" as the CURRENT parity result against an actual 5,634,
five fronts out of date, next to a Census-bug count less than half the real one.
Both were true when written.

So: documents no longer carry numbers, they carry MARKERS, and this script fills
them in.

    The suite is at <!--x13:parity_pass-->5634<!--/x13--> passing.

`--check` recomputes everything and exits nonzero if any marker is stale, which
makes drift a build failure rather than an embarrassment. Run it beside the
suite before publishing anything.

Usage:
  python tools/metrics.py                 # print the table
  python tools/metrics.py --write         # regenerate docs/METRICS.md + markers
  python tools/metrics.py --check         # exit 1 if anything is stale
  python tools/metrics.py --fast          # skip the pytest/ctest runs (~90s)

Adding a metric: write one function, register it in METRICS, done. Keep each one
DERIVED -- if a number cannot be computed from the repository it does not belong
here, because that is exactly the kind that rots.
"""
from __future__ import annotations

import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
DOCS = os.path.join(REPO, "docs")
OUT = os.path.join(DOCS, "METRICS.md")

# Files scanned for <!--x13:name-->...<!--/x13--> markers.
MARKER_ROOTS = (DOCS, HERE, REPO)
MARKER_RE = re.compile(r"(<!--x13:([a-z0-9_]+)-->)(.*?)(<!--/x13-->)", re.S)


def _run(cmd, cwd=REPO, timeout=600):
    try:
        p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True,
                           timeout=timeout)
        return p.stdout + p.stderr
    except Exception as exc:                                    # noqa: BLE001
        return "metrics: {} failed: {}".format(" ".join(cmd), exc)


def _walk(root, exts, skip_gen=True):
    for dp, _, fns in os.walk(root):
        if skip_gen and (os.sep + "gen" in dp or os.sep + "build" in dp):
            continue
        for fn in fns:
            if fn.endswith(exts):
                yield os.path.join(dp, fn)


def _nonblank(paths):
    n = 0
    for p in paths:
        with open(p, "r", errors="replace") as fh:
            n += sum(1 for ln in fh if ln.strip())
    return n


# --- the metrics -----------------------------------------------------------

def m_cpp_files():
    return len(list(_walk(os.path.join(REPO, "core"), (".cpp", ".hpp"))))


def m_cpp_lines():
    return _nonblank(_walk(os.path.join(REPO, "core"), (".cpp", ".hpp")))


def m_fortran_files():
    d = os.path.join(REPO, "oracle", "fortran")
    return len([f for f in os.listdir(d) if f.endswith(".f")])


def m_fortran_lines():
    d = os.path.join(REPO, "oracle", "fortran")
    return _nonblank(os.path.join(d, f) for f in os.listdir(d) if f.endswith(".f"))


def m_corpus_specs():
    return len(list(_walk(os.path.join(REPO, "tests", "corpus"), (".spc",),
                          skip_gen=False)))


def m_parity_modules():
    d = os.path.join(REPO, "tests", "parity")
    return len([f for f in os.listdir(d)
                if f.startswith("test_") and f.endswith(".py")])


def m_census_bugs():
    p = os.path.join(HERE, "census_bugs.md")
    with open(p, "r", errors="replace") as fh:
        return sum(1 for ln in fh if ln.startswith("## CB-"))


def _ported_counts():
    p = os.path.join(HERE, "ported.yaml")
    counts = {}
    with open(p, "r", errors="replace") as fh:
        for ln in fh:
            ln = ln.strip()
            if not ln or ln.startswith("#") or ":" not in ln:
                continue
            v = ln.split(":", 1)[1].split("#", 1)[0].strip()
            counts[v] = counts.get(v, 0) + 1
    return counts


def m_routines_done():
    c = _ported_counts()
    return c.get("ported", 0) + c.get("gated", 0)


def m_routines_total():
    c = _ported_counts()
    return sum(c.values()) - c.get("n-a", 0)


def m_routines_pct():
    tot = m_routines_total()
    return "{:.1f}".format(100.0 * m_routines_done() / tot) if tot else "0.0"


def m_commits():
    return int(_run(["git", "rev-list", "--count", "HEAD"]).strip() or 0)


def m_last_commit_date():
    # `%cs` is git's committer-date-short. NOT `%Y-%m-%d` -- that is strftime
    # syntax, and git reads `%d` as REF NAMES, so it silently returns
    # "%Y->- (HEAD -> branch)". Caught only because --check printed the value.
    return _run(["git", "log", "-1", "--format=%cs"]).strip()


def m_active_time():
    out = _run([sys.executable, os.path.join(HERE, "worklog.py")])
    m = re.search(r"active \(gaps[^)]*\)\s*:\s*(\S+)\s*(\S+)", out)
    return "{} {}".format(m.group(1), m.group(2)) if m else "unknown"


def m_calendar_days():
    out = _run([sys.executable, os.path.join(HERE, "worklog.py")])
    m = re.search(r"calendar days worked\s*:\s*(\d+)", out)
    return int(m.group(1)) if m else 0


_SUITE_CACHE = {}


def _suite():
    """pytest's own summary line -- the authoritative parity numbers."""
    if _SUITE_CACHE:
        return _SUITE_CACHE
    if FAST:
        _SUITE_CACHE.update(dict(passed="?", failed="?", skipped="?", xfailed="?"))
        return _SUITE_CACHE
    out = _run([sys.executable, "-m", "pytest", "tests/parity", "-q", "-n", "8"])
    got = dict(passed=0, failed=0, skipped=0, xfailed=0)
    for k in list(got):
        m = re.search(r"(\d+)\s+" + k + r"\b", out)
        if m:
            got[k] = int(m.group(1))
    _SUITE_CACHE.update(got)
    return _SUITE_CACHE


def m_parity_pass():
    return _suite()["passed"]


def m_parity_fail():
    return _suite()["failed"]


def m_parity_skip():
    return _suite()["skipped"]


def m_parity_xfail():
    return _suite()["xfailed"]


def m_ctest():
    if FAST:
        return "?"
    out = _run(["ctest"], cwd=os.path.join(REPO, "build"))
    m = re.search(r"(\d+)% tests passed,\s*(\d+) tests failed out of (\d+)", out)
    if not m:
        return "unknown"
    total, failed = int(m.group(3)), int(m.group(2))
    return "{}/{}".format(total - failed, total)


# name -> (callable, human label). Order is the order METRICS.md prints them.
METRICS = [
    ("parity_pass",     m_parity_pass,     "Parity tests passing"),
    ("parity_fail",     m_parity_fail,     "Parity tests failing"),
    ("parity_skip",     m_parity_skip,     "Parity tests skipped"),
    ("parity_xfail",    m_parity_xfail,    "Parity tests xfailed"),
    ("ctest",           m_ctest,           "Unit tests (ctest)"),
    ("corpus_specs",    m_corpus_specs,    "Corpus spec files"),
    ("parity_modules",  m_parity_modules,  "Parity test modules"),
    ("cpp_lines",       m_cpp_lines,       "C++ non-blank lines (excl. generated)"),
    ("cpp_files",       m_cpp_files,       "C++ files (excl. generated)"),
    ("fortran_lines",   m_fortran_lines,   "Fortran reference, non-blank lines"),
    ("fortran_files",   m_fortran_files,   "Fortran reference, files"),
    ("routines_done",   m_routines_done,   "Fortran routines ported or gated"),
    ("routines_total",  m_routines_total,  "Fortran routines in scope (excl. n-a)"),
    ("routines_pct",    m_routines_pct,    "Percent of routines ported"),
    ("census_bugs",     m_census_bugs,     "Census bugs catalogued"),
    ("commits",         m_commits,         "Commits"),
    ("active_time",     m_active_time,     "Active development time"),
    ("calendar_days",   m_calendar_days,   "Calendar days worked"),
    ("last_commit",     m_last_commit_date, "Last commit"),
]

FAST = False


def collect():
    return {name: str(fn()) for name, fn, _ in METRICS}


HEADER = """\
# Project metrics

**Generated -- do not edit.** `python tools/metrics.py --write`

Every countable claim about this project is derived here and nowhere else. Prose
should LINK to this file rather than restate a number; where a number genuinely
has to appear inline, wrap it in a marker and let `--write` maintain it:

    the suite is at <!--x13:parity_pass-->NNNN<!--/x13--> passing

`python tools/metrics.py --check` fails if any marker anywhere has drifted, so a
stale figure is a build error instead of something a reader finds first.

"""


def render(values):
    out = [HEADER, "| Metric | Value |", "|---|---|"]
    for name, _, label in METRICS:
        out.append("| {} | **{}** |".format(label, values[name]))
    out.append("")
    out.append("Metric names for markers: " +
               ", ".join("`{}`".format(n) for n, _, _ in METRICS) + ".")
    out.append("")
    return "\n".join(out)


def marker_files():
    # METRICS.md is EXCLUDED: its header shows the marker syntax by example, and
    # substituting into that example would make --check permanently stale (the
    # rendered header says NNNN, the file on disk would say 5634). The generated
    # file is verified whole-file instead.
    seen = {OUT}
    for root in MARKER_ROOTS:
        depth0 = root == REPO
        for dp, dirs, fns in os.walk(root):
            if depth0:
                dirs[:] = []          # repo root: top-level files only
            if os.sep + ".git" in dp or os.sep + "build" in dp:
                continue
            for fn in fns:
                if fn.endswith((".md", ".txt")):
                    p = os.path.join(dp, fn)
                    if p not in seen:
                        seen.add(p)
                        yield p


def apply_markers(values, write):
    """Returns the list of (path, name, old, new) that are stale."""
    stale = []
    for p in marker_files():
        with open(p, "r", errors="replace", newline="") as fh:
            text = fh.read()
        if "<!--x13:" not in text:
            continue
        changed = False

        def sub(m):
            nonlocal changed
            name, old = m.group(2), m.group(3)
            if name not in values:
                return m.group(0)
            new = values[name]
            if old != new:
                stale.append((p, name, old, new))
                changed = True
            return m.group(1) + new + m.group(4)

        new_text = MARKER_RE.sub(sub, text)
        if write and changed:
            with open(p, "w", errors="replace", newline="") as fh:
                fh.write(new_text)
    return stale


def main(argv):
    global FAST
    FAST = "--fast" in argv
    write = "--write" in argv
    check = "--check" in argv

    values = collect()

    if check and FAST:
        print("metrics: --check with --fast cannot verify the suite counts")
        return 2

    stale = apply_markers(values, write=write and not check)

    if write and not check:
        os.makedirs(DOCS, exist_ok=True)
        with open(OUT, "w", newline="") as fh:
            fh.write(render(values))
        print("wrote {}".format(os.path.relpath(OUT, REPO)))
        for p, name, old, new in stale:
            print("  marker {}:{}  {} -> {}".format(
                os.path.relpath(p, REPO), name, old, new))
        return 0

    if check:
        if os.path.exists(OUT):
            with open(OUT, "r", newline="") as fh:
                if fh.read() != render(values):
                    print("STALE: docs/METRICS.md differs from the computed values")
                    stale.append((OUT, "(whole file)", "", ""))
        else:
            print("STALE: docs/METRICS.md does not exist")
            stale.append((OUT, "(missing)", "", ""))
        if stale:
            for p, name, old, new in stale:
                print("  {}:{}  {!r} -> {!r}".format(
                    os.path.relpath(p, REPO), name, old, new))
            print("\nrun: python tools/metrics.py --write")
            return 1
        print("metrics: up to date")
        return 0

    print(render(values))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
