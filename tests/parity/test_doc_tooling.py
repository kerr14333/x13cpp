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
