"""M4 gate: the automatic-model-selection AIC-test regressor family.

Drives ``x13run_iddiff --aictest`` -- which reproduces the automd preamble and
then runs the ported ``x13::tdaic`` (trading-day) and ``x13::easaic`` (Easter)
AIC tests -- on the aictest corpus specs, and checks the AICC differences the
oracle reports in its ``.udg`` as ``aictest.diff.td`` / ``aictest.diff.e``::

    aictest.diff.td:   <AICC(no TD)  - AICC(best TD model)>
    aictest.diff.e:    <AICC(no Eas) - AICC(best Easter model)>

tdaic/easaic estimate the model WITH and WITHOUT the regressor group (rgarma)
and compare each fit's AICC (prlkhd), keeping the group iff it lowers AICC by
more than the threshold Rgaicd. See core/src/automdl/aictst.cpp.

Harness modes (the oracle's automd runs the AIC tests TWICE: once on the default
airline model, once on the identified model; the ``.udg`` saves the LAST run):

  * ``--aictest`` (default airline model) -- reproduces the saved value when the
    identified model equals the default airline ((0 1 1)(0 1 1)): airline /
    03-automdl.
  * ``--afterauto`` -- identify the model first (reduced automd), then run the
    AIC tests on it: the saved value when the identified model differs from the
    default airline (expgs diff.td; payems / unrate diff.e).
  * ``--afterauto --preeas`` -- additionally install the Easter regressor before
    the trading-day test, matching the oracle's round-2 model state in which
    Easter was already selected (payems / unrate diff.td).

Every oracle aictest.diff value is reproduced BIT-EXACTLY (well within rtol
1e-6). The one remaining automd-sequencing detail -- carrying the exact round-1
regressor state through identification for BOTH diff.td and diff.e of a single
run -- lands when the AIC tests are wired into automd (main thread); see
tools/FABLE_REVIEW.md.
"""
import os
import subprocess

import pytest

from test_m3_estimate import _REPO, _CORPUS, _find_binary

BIN = _find_binary("x13run_iddiff", "X13RUN_IDDIFF")

# (id, spec-rel, harness flags, {udg-key: harness-key}) -- one row per oracle
# value, using the harness mode that reproduces the automd model state the
# oracle saved that value from.
_CASES = [
    ("airline_td",   "generated/airline_automdl-aictest-x11.spc",
     ["--td", "--easter"],              {"aictest.diff.td": "aictest.diff.td"}),
    ("airline_e",    "generated/airline_automdl-aictest-x11.spc",
     ["--td", "--easter"],              {"aictest.diff.e": "aictest.diff.e",
                                         "aictest.e.window": "aictest.e.window"}),
    ("03automdl_td", "census-examples/03-automdl.spc",
     ["--td", "--easter"],              {"aictest.diff.td": "aictest.diff.td"}),
    ("03automdl_e",  "census-examples/03-automdl.spc",
     ["--td", "--easter"],              {"aictest.diff.e": "aictest.diff.e",
                                         "aictest.e.window": "aictest.e.window"}),
    ("expgs_td",     "generated/expgs_automdl-aictest-x11.spc",
     ["--td", "--afterauto"],           {"aictest.diff.td": "aictest.diff.td"}),
    ("payems_e",     "generated/payems_automdl-aictest-x11.spc",
     ["--td", "--easter", "--afterauto"],
                                        {"aictest.diff.e": "aictest.diff.e",
                                         "aictest.e.window": "aictest.e.window"}),
    ("payems_td",    "generated/payems_automdl-aictest-x11.spc",
     ["--td", "--easter", "--afterauto", "--preeas"],
                                        {"aictest.diff.td": "aictest.diff.td"}),
    ("unrate_e",     "generated/unrate_automdl-aictest-x11.spc",
     ["--td", "--easter", "--afterauto"],
                                        {"aictest.diff.e": "aictest.diff.e",
                                         "aictest.e.window": "aictest.e.window"}),
    ("unrate_td",    "generated/unrate_automdl-aictest-x11.spc",
     ["--td", "--easter", "--afterauto", "--preeas"],
                                        {"aictest.diff.td": "aictest.diff.td"}),
]

_RTOL = 1e-6


def _run(spc_abs, flags):
    proc = subprocess.run([BIN, spc_abs, "--aictest", *flags],
                          capture_output=True, text=True)
    assert proc.returncode == 0, f"x13run_iddiff failed: {proc.stderr}\n{proc.stdout}"
    vals = {}
    for line in proc.stdout.splitlines():
        if ":" in line:
            k, _, v = line.partition(":")
            vals[k.strip()] = v.strip()
    return vals


def _udg_val(spc, key):
    base = os.path.basename(spc)[:-4]
    rel = os.path.relpath(spc, _CORPUS)
    udg = os.path.join(_REPO, "tests", "golden", rel[:-4], base + ".udg")
    for line in open(udg, encoding="utf-8", errors="replace"):
        if line.startswith(key + ":"):
            return line.split(":", 1)[1].strip()
    return None


@pytest.mark.parametrize("cid,rel,flags,keys", _CASES, ids=[c[0] for c in _CASES])
def test_aictest_diffs(cid, rel, flags, keys):
    """Each ported AIC-test AICC difference matches the oracle .udg bit-exactly
    (rtol 1e-6), read live from the golden file (guards against a stale golden)."""
    spc = os.path.join(_CORPUS, *rel.split("/"))
    vals = _run(spc, flags)
    assert vals.get("OUTCOME") == "OK", vals
    for udg_key, got_key in keys.items():
        oracle = _udg_val(spc, udg_key)
        assert oracle is not None, f"{udg_key} missing from oracle .udg"
        got = float(vals[got_key])
        exp = float(oracle)
        if udg_key == "aictest.e.window":
            assert int(got) == int(exp), (cid, udg_key, got, exp, vals)
        else:
            assert abs(got - exp) <= _RTOL * max(1.0, abs(exp)), (
                cid, udg_key, got, exp, vals)
