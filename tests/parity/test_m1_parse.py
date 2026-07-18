"""M1 gate: the C++ spec-parser (x13parse) vs the Fortran oracle goldens.

For every corpus spec (tests/corpus/**/*.spc) this checks that running x13parse
reproduces the oracle's fatal/ok outcome, and for the error case that the ERROR
text lines match the golden .err (whitespace-normalized). It also echoes and
verifies key parsed settings for five representative specs.

Run:  pytest tests/parity/test_m1_parse.py -v
"""
from __future__ import annotations

import json
import os
import re
import subprocess
import sys

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus")
_GOLDEN = os.path.join(_REPO, "tests", "golden")


def _find_binary() -> str:
    cands = [
        os.path.join(_REPO, "build", "x13parse.exe"),
        os.path.join(_REPO, "build", "x13parse"),
        os.path.join(_REPO, "build", "Release", "x13parse.exe"),
        os.path.join(_REPO, "build", "Debug", "x13parse.exe"),
    ]
    for c in cands:
        if os.path.exists(c):
            return c
    env = os.environ.get("X13PARSE")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13parse binary not found; build it first (cmake --build build).")


BIN = _find_binary()


# Spec blocks the M1 C++ parser does not yet dispatch. A corpus spec using any
# of these is expected to fatal at parse time until its milestone lands, so we
# xfail it (strict) keyed to the milestone. When a block is ported, remove it
# here: the strict xfail then turns the now-passing spec into a loud failure,
# forcing this list to stay honest (no silent scope drift).
_UNPARSED_BLOCKS = {
    "spectrum": "M6-M7",
    "history": "M9-M10",
    "slidingspans": "M9-M10",
    "x11regression": "M6-M7",
    "force": "M9-M10",
    "metadata": "M9-M10",
    "pickmdl": "M4-M5",
}

# NOTE: assumes the block name and its `{` share a line (the whole corpus
# convention). X-13's tokenizer also accepts `name\n{`, `} name{` mid-line, and
# `name #cmt\n{`; those forms would be missed, but the failure mode is loud (a
# plain FAILED, never a silent pass) since strict xfail can't be reached.
_BLOCK_RE = re.compile(r"^\s*([a-z0-9]+)\s*\{", re.M | re.I)


def _spec_blocks(spc: str):
    with open(spc, encoding="utf-8", errors="replace") as fh:
        return {m.group(1).lower() for m in _BLOCK_RE.finditer(fh.read())}


def _unparsed_reason(spc: str):
    """If the spec uses a not-yet-ported block, return an xfail reason, else None."""
    hits = _spec_blocks(spc) & _UNPARSED_BLOCKS.keys()
    if not hits:
        return None
    return "unported spec block(s): " + ", ".join(
        f"{b} ({_UNPARSED_BLOCKS[b]})" for b in sorted(hits))


def _corpus_specs():
    out = []
    for root, _dirs, files in os.walk(_CORPUS):
        for f in files:
            if f.endswith(".spc"):
                out.append(os.path.join(root, f))
    return sorted(out)


def _corpus_params():
    params = []
    for spc in _corpus_specs():
        rid = os.path.relpath(spc, _CORPUS)
        reason = _unparsed_reason(spc)
        marks = [pytest.mark.xfail(reason=reason, strict=True)] if reason else []
        params.append(pytest.param(spc, id=rid, marks=marks))
    return params


def _golden_dir(spc: str) -> str:
    rel = os.path.relpath(spc, _CORPUS)
    return os.path.join(_GOLDEN, rel[:-4])


def _err_error_lines(text: str):
    """Return the ' ERROR:'-and-following lines, whitespace-normalized."""
    lines = []
    for ln in text.splitlines():
        s = ln.strip()
        if s.startswith("ERROR"):
            lines.append(re.sub(r"\s+", " ", s))
    return lines


def _oracle_ok(spc: str):
    gdir = _golden_dir(spc)
    man = json.load(open(os.path.join(gdir, "manifest.json")))
    base = os.path.basename(spc)[:-4]
    errf = os.path.join(gdir, base + ".err")
    errtxt = ""
    if os.path.exists(errf):
        errtxt = open(errf, encoding="utf-8", errors="replace").read()
    has_error = len(_err_error_lines(errtxt)) > 0
    return (man.get("exit_code") == 0) and not has_error, errtxt


def _run(spc: str):
    proc = subprocess.run([BIN, spc], capture_output=True, text=True)
    out = proc.stdout
    m = re.search(r"^OUTCOME:\s*(\w+)", out, re.M)
    outcome = m.group(1) if m else "?"
    # Extract the ===ERR=== ... ===END ERR=== block.
    em = re.search(r"===ERR===\n(.*)===END ERR===", out, re.S)
    err = em.group(1) if em else ""
    return outcome, err, out


@pytest.mark.parametrize("spc", _corpus_params())
def test_outcome_matches_oracle(spc):
    oracle_ok, golden_err = _oracle_ok(spc)
    outcome, cpp_err, _ = _run(spc)
    parser_ok = outcome == "OK"
    assert parser_ok == oracle_ok, (
        f"{os.path.relpath(spc, _CORPUS)}: parser OK={parser_ok} vs oracle OK={oracle_ok}")

    # For error cases, the ERROR text lines must match the golden .err.
    if not oracle_ok:
        golden_lines = _err_error_lines(golden_err)
        cpp_lines = _err_error_lines(cpp_err)
        if golden_lines:  # total.spc has an empty .err (SIGFPE); only match when present
            assert cpp_lines == golden_lines, (
                f"{os.path.relpath(spc, _CORPUS)}: ERROR text mismatch\n"
                f"  golden: {golden_lines}\n  cpp:    {cpp_lines}")


def test_malformed_err_byte_identical():
    """The malformed spec's .err must match the golden byte-for-byte."""
    spc = os.path.join(_CORPUS, "edge", "malformed-unknown-arg.spc")
    _outcome, cpp_err, _ = _run(spc)
    gdir = _golden_dir(spc)
    golden = open(os.path.join(gdir, "malformed-unknown-arg.err"),
                  encoding="utf-8", errors="replace").read()
    # The golden .err is exactly the Mt2 channel content.
    assert cpp_err.rstrip("\n") == golden.rstrip("\n")


# --- Representative-spec key-setting echo checks ---------------------------
_EXPECTED = {
    "census-examples/01-basic-x11.spc": {
        "period": "12", "nobs": "144", "series_start": "1949.1",
        "span": "1949.1 - 1960.12",
    },
    "census-examples/02-airline-log-td-easter.spc": {
        "period": "12", "transform_function": "log",
        "arima_model": "(0 1 1)(0 1 1)", "forecast_maxlead": "12",
    },
    "edge/span-modelspan.spc": {
        "period": "12", "nobs": "308", "series_start": "2000.1",
        "span": "2005.1 - 2025.8", "forecast_maxlead": "24",
    },
    "generated/expgs_x11-default.spc": {
        "period": "4", "series_start": "1947.1", "x11_mode": "mult",
    },
    "generated/airline_fixed-airline-x11.spc": {
        "period": "12", "arima_model": "(0 1 1)(0 1 1)",
    },
}


def _captured(out: str):
    d = {}
    for ln in out.splitlines():
        m = re.match(r"\s+(\w+) = (.+)$", ln)
        if m:
            d[m.group(1)] = m.group(2).strip()
    return d


@pytest.mark.parametrize("relspec", sorted(_EXPECTED.keys()))
def test_key_settings_echo(relspec):
    spc = os.path.join(_CORPUS, *relspec.split("/"))
    _outcome, _err, out = _run(spc)
    cap = _captured(out)
    for k, v in _EXPECTED[relspec].items():
        assert cap.get(k) == v, f"{relspec}: {k}={cap.get(k)!r} expected {v!r}"


if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-v"]))
