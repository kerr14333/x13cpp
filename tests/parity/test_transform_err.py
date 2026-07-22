"""Transform-stage error-channel parity (trnfcn.f two-channel diagnostics).

The pre-model transform application (run_m2 -> trnfcn) rejects illegal transforms
(e.g. log of a non-positive value) with a diagnostic written to TWO channels that
are worded differently: the Mt2/.err channel and the stdio::STDERR channel. The
oracle trnfcn.f:82-87 emits "Do not take log of a zero" to Mt2 but "Do not take
log of zero" (no "a") to STDERR. The ported trnfcn.cpp reproduces that split; this
test locks it -- a regression that unifies the two channels (the pre-fix bug, which
sent "log of a zero" to both) is caught here.

x13run_m2 emits both channels between the ===ERR===/===END ERR=== and
===STDERR===/===END STDERR=== markers.

Run:  python -m pytest tests/parity/test_transform_err.py -q
"""

from __future__ import annotations

import os
import re
import subprocess

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus")
_GOLDEN = os.path.join(_REPO, "tests", "golden")


def _find_binary() -> str:
    cands = [
        os.path.join(_REPO, "build", "x13run_m2.exe"),
        os.path.join(_REPO, "build", "x13run_m2"),
        os.path.join(_REPO, "build", "Release", "x13run_m2.exe"),
    ]
    for c in cands:
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_M2")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13run_m2 binary not found; build it first (cmake --build build).")


BIN = _find_binary()


def _block(out: str, name: str) -> str:
    m = re.search("===%s===\n(.*?)===END %s===" % (name, name), out, re.S)
    return m.group(1) if m else ""


def _error_lines(text: str):
    return [ln.strip() for ln in text.splitlines() if ln.strip().startswith("ERROR")]


def test_log_zero_two_channel():
    spec = os.path.join(_CORPUS, "edge", "log-zero-series.spc")
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    # Illegal transform -> the run must be FATAL.
    assert re.search(r"^OUTCOME:\s*FATAL", r.stdout, re.M), r.stdout[:300]

    err_lines = _error_lines(_block(r.stdout, "ERR"))       # Mt2 / .err channel
    se_lines = _error_lines(_block(r.stdout, "STDERR"))      # stdio::STDERR channel

    # Channel-exact text (trnfcn.f:86-87): Mt2 says "log of a zero", STDERR "log of
    # zero". Match the oracle goldens (.err = Mt2, .stdout.txt = STDERR channel).
    gdir = os.path.join(_GOLDEN, "edge", "log-zero-series")
    with open(os.path.join(gdir, "log-zero-series.err"), encoding="utf-8",
              errors="replace") as fh:
        gold_err = _error_lines(fh.read())
    assert err_lines == gold_err, f"Mt2 mismatch\n golden: {gold_err}\n cpp: {err_lines}"

    # The distinguishing assertion (locks the fix): STDERR must say "log of zero"
    # WITHOUT the article -- the pre-fix bug emitted "log of a zero" to both.
    assert any("take log of zero" in ln for ln in se_lines), se_lines
    assert not any("take log of a zero" in ln for ln in se_lines), (
        "STDERR channel wrongly carries the Mt2 wording ('a zero') -- the two-"
        "channel split has regressed")
