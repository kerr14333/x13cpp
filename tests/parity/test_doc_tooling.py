"""The doc-honesty tooling must be able to fail. This tests that it can.

WHY THIS FILE EXISTS. `tools/metrics.py` is what keeps every countable claim in
this repo true, and on 2026-07-30 it was found to have been BROKEN under
PowerShell -- which is the shell `tools/build.ps1` uses -- for an unknown
length of time. The chain:

  worklog.py ran `git log` with `text=True` and no encoding, so Python decoded
  with locale.getpreferredencoding(): UTF-8 under this repo's Bash, cp1252
  under PowerShell. This repo's commit subjects are full of em dashes, so under
  PowerShell it raised UnicodeDecodeError. metrics.py runs worklog.py as a
  subprocess and SWALLOWED the failure, returning the error text as the
  command's output -- so its regexes simply did not match and it fell back to
  placeholders: active_time became the string "unknown", calendar_days became
  0. `--write` would have committed those as measurements.

The generalizable defect is not the encoding. It is that **the check could not
distinguish "I ran and found nothing wrong" from "I crashed"** -- the same
class as `run_parity.py` reporting PASS having compared nothing, and as this
repo's own rule that a null measured under the wrong preconditions is not a
null. A guardrail that cannot fail loudly is not a guardrail.

So these tests assert the tooling ACTUALLY WORKS, not merely that it exits.
They live in the parity suite on purpose: the suite is run and read, whereas
the build's doc check only warns, and a warning that is wrong is invisible.
"""
import os
import re
import subprocess
import sys

import pytest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
TOOLS = os.path.join(ROOT, "tools")

# metrics.py's own exit codes: 0 = up to date, 1 = a marker drifted (normal
# mid-session), 2 = could not derive a metric (the failure this file guards).
DERIVE_FAILED = 2


def _run_tool(args, env=None):
    e = dict(os.environ)
    if env:
        e.update(env)
    return subprocess.run([sys.executable] + args, cwd=ROOT, capture_output=True,
                          text=True, encoding="utf-8", errors="replace",
                          timeout=300, env=e)


# A non-UTF-8 default encoding is the condition that broke this. PYTHONUTF8=0
# turns off Python's UTF-8 mode so locale.getpreferredencoding() governs again,
# which on this machine is cp1252 -- i.e. it reproduces the PowerShell
# environment from inside the Bash-run suite.
ENVS = [
    pytest.param({}, id="default-encoding"),
    pytest.param({"PYTHONUTF8": "0"}, id="legacy-ansi-encoding"),
]


@pytest.mark.parametrize("env", ENVS)
def test_worklog_runs(env):
    """worklog.py reads `git log`, whose output carries non-ASCII. It must not
    depend on the ambient codepage to decode it."""
    p = _run_tool([os.path.join(TOOLS, "worklog.py")], env)
    assert p.returncode == 0, f"worklog.py failed:\n{p.stderr[-2000:]}"
    assert "Traceback" not in p.stderr, f"worklog.py raised:\n{p.stderr[-2000:]}"
    assert re.search(r"active \(gaps[^)]*\)\s*:\s*\S+\s+\S+", p.stdout), (
        "worklog.py printed no active-time line -- metrics.py parses this, and "
        f"a miss there is what produced the 'unknown' placeholder.\n{p.stdout[-1500:]}")


@pytest.mark.parametrize("env", ENVS)
def test_metrics_check_derives_every_metric(env):
    """The regression test proper: metrics must not silently degrade.

    Exit 2 means it could not derive something and (correctly) wrote nothing.
    0 or 1 both mean it did its job -- 1 is just drift, which is a normal
    mid-session state and is what the build warns about.
    """
    p = _run_tool([os.path.join(TOOLS, "metrics.py"), "--check", "--fast"], env)
    assert "Traceback" not in p.stderr, (
        f"metrics.py raised instead of reporting:\n{p.stderr[-2000:]}")
    assert p.returncode != DERIVE_FAILED, (
        f"metrics.py could not derive a metric:\n{p.stdout[-1500:]}")
    assert p.returncode in (0, 1), (
        f"unexpected exit {p.returncode}:\n{p.stdout[-1500:]}\n{p.stderr[-1500:]}")


@pytest.mark.parametrize("env", ENVS)
def test_metrics_values_are_measurements_not_placeholders(env):
    """Assert the VALUES, not just the exit code.

    This is the assertion that would have caught the original bug: metrics.py
    exited cleanly the whole time it was reporting `active_time: unknown`.
    """
    sys.path.insert(0, TOOLS)
    p = _run_tool(["-c", (
        "import sys, json; sys.path.insert(0, r'%s'); "
        "import metrics; metrics.FAST = True; "
        "print(json.dumps(metrics.collect()))" % TOOLS)], env)
    assert p.returncode == 0, f"collect() failed:\n{p.stderr[-2000:]}"
    import json
    values = json.loads(p.stdout.strip().splitlines()[-1])

    assert re.fullmatch(r"\d+h \d+m", values["active_time"]), (
        f"active_time is {values['active_time']!r} -- 'unknown' is the "
        "placeholder the swallowed-exception bug produced")
    assert int(values["calendar_days"]) > 0, (
        "calendar_days is 0 -- the other placeholder from the same bug")
    assert int(values["commits"]) > 0
    for name, v in values.items():
        assert v not in ("", "unknown", "None"), f"{name} did not derive: {v!r}"


def test_walls_check_runs():
    """walls.py is the other half of the guardrail and is regenerated FROM the
    engine's own refusal messages, so it cannot go stale silently -- but it can
    still fail to run."""
    p = _run_tool([os.path.join(TOOLS, "walls.py"), "--check"])
    assert "Traceback" not in p.stderr, f"walls.py raised:\n{p.stderr[-2000:]}"
    assert p.returncode in (0, 1), f"unexpected exit {p.returncode}: {p.stdout}"
    assert re.search(r"\d+ gaps", p.stdout), (
        f"walls.py printed no gap count: {p.stdout!r}")


# --- walls.py's own blind spots -------------------------------------------
#
# Two of them, and both were live for months.
#
# (1) The helper list was a typed tuple matched with `\b`, and `\bnot_ported`
#     cannot match inside `agr3_not_ported` -- `_` is a word character. Two
#     walls landed invisible and the printed count did not move. The list is
#     derived now; this asserts the derivation actually covers the tree.
# (2) A refusal that does not go through a helper was not a wall at all. Ten of
#     them existed: six on the parser's `inpter` channel (which refuses by
#     clearing `inptok`, never by `abend`) and four raw writln+abend pairs.
#
# The second test builds a synthetic source tree, because a checker that has
# never been shown to FAIL is not a checker.

def _walls():
    sys.path.insert(0, TOOLS)
    import walls
    return walls


def test_walls_helper_set_is_derived_over_the_whole_tree():
    w = _walls()
    core = os.path.join(ROOT, "core", "src")
    defined = set()
    for dp, _dirs, fns in os.walk(core):
        if os.sep + "gen" in dp:
            continue
        for fn in fns:
            if not fn.endswith((".cpp", ".hpp")):
                continue
            with open(os.path.join(dp, fn), errors="replace") as fh:
                for m in re.finditer(r"(\w*not_ported)\s*\(\s*X13Context\s*&",
                                     fh.read()):
                    defined.add(m.group(1))
    assert defined, "no *not_ported helpers found -- the probe itself broke"
    missing = sorted(defined - set(w.HELPERS))
    assert not missing, (
        "walls.py does not know these refusal helpers, so their call sites are "
        "invisible in the inventory: " + " ".join(missing))


def test_walls_sees_helperless_refusals_and_can_fail(tmp_path):
    """Four refusal shapes, one synthetic file, four different verdicts."""
    w = _walls()
    src = tmp_path / "zzq.cpp"
    src.write_text(
        "void zzq_gap_writln(X13Context& ctx) {\n"
        '    writln(ctx, "ERROR: zzq synthetic (zzq.f:1) not yet ported.",\n'
        "           stdio::STDERR, ctx.units.mt2, true);\n"
        "    abend(ctx);\n"
        "}\n"
        "void zzq_gap_parser(X13Context& ctx, bool& inptok) {\n"
        '    inpter(ctx, PERROR, ep, "zzq option is not yet supported.");\n'
        "    inptok = false;\n"
        "}\n"
        "void zzq_faithful(X13Context& ctx) {\n"
        '    writln(ctx, "ERROR: no ARIMA models stored in that file.",\n'
        "           stdio::STDERR, ctx.units.mt2, true);\n"
        "    abend(ctx);\n"
        "}\n"
        "void zzq_bare(X13Context& ctx) {\n"
        "    abend(ctx);\n"
        "}\n")
    old = (w.CORE, w.REPO)
    try:
        # REPO too: _row() relpaths against it, and pytest's tmp_path is on a
        # different drive on this machine.
        w.CORE = w.REPO = str(tmp_path)
        rows, bare = w.collect(with_bare=True)
    finally:
        w.CORE, w.REPO = old

    msgs = " | ".join(r["msg"] for r in rows)
    gaps = [r for r in rows if r["kind"] == "GAP"]
    assert len(gaps) == 2, f"expected the two GAP refusals, got: {msgs}"
    assert any("zzq synthetic" in r["msg"] for r in gaps), msgs
    assert any("zzq option" in r["msg"] for r in gaps), msgs
    # The messaged-but-not-GAP abend is excluded on purpose: the oracle refuses
    # the same input, so it is a port. It must not be counted as bare either --
    # `--audit` would then cry wolf on ~50 faithful error exits.
    assert not [r for r in rows if r["kind"] != "GAP"], msgs
    assert [r["line"] for r in bare] == [16], (
        f"only the messageless abend is bare, got {bare}")
    # The Fortran citation has to survive into the row, or WALLS.md loses the
    # one thing that makes an entry actionable.
    assert any(r["fref"] == "zzq.f:1" for r in gaps), gaps


# --- coverage_map's routine-level tier ------------------------------------
#
# The ledger is FILE-level; the port is ROUTINE-level, and 37 .f files hold more
# than one routine (matrix.f holds 90). Those files could not be described by a
# `stem: status` line at all, so they sat at `pending` however much of them was
# ported -- and the 58.8% that produced was read as "41% of the program left".
# The SPLIT tier and the routine-level figure exist to make that visible.
#
# The figure is computed with a one-pass symbol set (_def_names) rather than
# 1100+ anchored scans of a 2.7MB blob, purely for speed: 34.4s -> 0.03s. That
# is an optimisation of the MEASUREMENT, which is exactly the kind of thing that
# silently stops agreeing with what it replaced. So assert the two agree, over
# the real corpus of names, rather than trusting the comment that says they do.

def _coverage_map():
    sys.path.insert(0, TOOLS)
    import coverage_map
    return coverage_map


def test_def_names_agrees_with_defines():
    cm = _coverage_map()
    blob = "\n".join(t for _, t in cm._cpp_sources())
    routines = cm.f_routines(os.path.join(ROOT, "oracle", "fortran"))
    names = sorted({n for v in routines.values() for n in v}
                   | set(cm.ALIASES) | set(routines))
    assert len(names) > 500, f"only {len(names)} names -- the parse shrank"

    fast = cm._def_names(blob)
    disagree = [n for n in names if (n in fast) != cm._defines(blob, n)]
    assert not disagree, (
        "_def_names and _defines disagree on: " + " ".join(disagree[:20]))


def test_def_names_can_fail():
    """The check above is worthless if it cannot fail. Feed it a blob whose
    definition the fast path must see and the slow path must not."""
    cm = _coverage_map()
    assert cm._defines("void zzq_probe(int x) {", "zzq_probe")
    assert "zzq_probe" in cm._def_names("void zzq_probe(int x) {")
    assert not cm._defines("// zzq_probe(x) in a comment", "zzq_probe")
    assert "zzq_probe" not in cm._def_names("// zzq_probe(x) in a comment")


def test_cite_names_is_word_anchored_on_both_edges():
    """The per-stem CITE probe was `stem + r'\\.f\\b'` -- open on the LEFT -- so
    `dot` matched inside `amidot.f`. The hoisted version captures `(\\w+)`, which
    anchors both edges. This asserts the fix, not the old behaviour."""
    cm = _coverage_map()
    names = cm._cite_names("// see amidot.f:59 and ddot.f for the helper\n")
    assert "amidot" in names and "ddot" in names
    assert "dot" not in names, "substring match is back"


def test_routine_parse_finds_the_multi_routine_files():
    """A discovery predicate that SHRINKS reports green, so floor-assert it."""
    cm = _coverage_map()
    routines = cm.f_routines(os.path.join(ROOT, "oracle", "fortran"))
    total = sum(len(v) for v in routines.values())
    multi = [s for s, v in routines.items() if len(v) > 1]
    assert total > 1100, f"only {total} Fortran routines parsed"
    assert len(multi) >= 30, f"only {len(multi)} multi-routine .f found"
    # The file that motivated the tier. If this stops parsing, the routine
    # figure quietly loses 90 units and nothing else complains.
    assert len(routines["matrix"]) > 80, (
        f"matrix.f parsed as {len(routines['matrix'])} routines")
