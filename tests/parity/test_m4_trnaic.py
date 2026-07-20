"""M4 gate: automatic transform selection (trnaic.f, transform{function=auto}).

Drives ``x13run_iddiff --trnaic``, which runs the pre-model phase (``x13::run_m2``)
on a ``transform{function=auto}`` spec. run_m2 invokes ``x13::trnaic`` when the
transform is left automatic (Fcntyp==0): it estimates the default airline model
untransformed (Fcntyp=4) and log-transformed (Fcntyp=1), computes each model's
AICC via prlkhd, and picks the transform with the lower AICC (log wins when
aiclog + Traicd < aicno, with Traicd defaulting to -2 for monthly/quarterly data).

The oracle reports the decision in its ``.udg`` as::

    aictest.trans.aicc.nolog:   <AICC of the untransformed model>
    aictest.trans.aicc.log:     <AICC of the log-transformed model>
    aictrans: Log(y)            (or "None")

Both AICC values match the oracle bit-for-bit (well within rtol 1e-6); the payems
case is the interesting one -- its log AICC is *higher* than nolog, yet Log(y) is
still selected because the -2 aicdiff threshold flips it, exercising Traicd.
"""
import os

import pytest

from test_m3_estimate import _REPO, _CORPUS, _find_binary

BIN = _find_binary("x13run_iddiff", "X13RUN_IDDIFF")

# spec (relative to the corpus root) -> (nolog AICC, log AICC, aictrans) from the
# oracle .udg. All four are transform{function=auto} + automdl specs; 03-automdl
# additionally carries an aictest=(td easter) preamble (the aictest regressors are
# automd candidates, absent from the base model trnaic estimates, so the transform
# decision is identical to the plain airline case).
_CASES = [
    ("generated/airline_automdl-x11.spc", 1021.19194610079, 987.384531442308, "Log(y)"),
    ("generated/expgs_automdl-x11.spc", 3383.89987203260, 2519.23559846648, "Log(y)"),
    ("generated/payems_automdl-x11.spc", 5086.44081829335, 5087.66162057374, "Log(y)"),
    ("census-examples/03-automdl.spc", 1021.19194610079, 987.384531442308, "Log(y)"),
]


def _run(spc_abs):
    import subprocess
    proc = subprocess.run([BIN, spc_abs, "--trnaic"], capture_output=True, text=True)
    assert proc.returncode == 0, f"x13run_iddiff failed: {proc.stderr}\n{proc.stdout}"
    vals = {}
    for line in proc.stdout.splitlines():
        if ":" in line:
            k, _, v = line.partition(":")
            vals[k.strip()] = v.strip()
    return vals


@pytest.mark.parametrize(
    "rel,nolog,log,trans", _CASES,
    ids=[c[0].split("/")[-1][:-4] for c in _CASES])
def test_trnaic_transform_selection(rel, nolog, log, trans):
    spc = os.path.join(_CORPUS, *rel.split("/"))
    vals = _run(spc)
    assert vals.get("OUTCOME") == "OK", vals
    assert vals.get("trnaic.ran") != "no", ("trnaic did not run", vals)

    got_nolog = float(vals["aictest.trans.aicc.nolog"])
    got_log = float(vals["aictest.trans.aicc.log"])
    assert abs(got_nolog - nolog) <= 1e-6 * abs(nolog), (got_nolog, nolog, vals)
    assert abs(got_log - log) <= 1e-6 * abs(log), (got_log, log, vals)
    assert vals["aictrans"] == trans, vals


def _udg_val(spc, key):
    base = os.path.basename(spc)[:-4]
    rel = os.path.relpath(spc, _CORPUS)
    udg = os.path.join(_REPO, "tests", "golden", rel[:-4], base + ".udg")
    for line in open(udg, encoding="utf-8", errors="replace"):
        if line.startswith(key):
            return line.split(":", 1)[1].strip()
    return None


@pytest.mark.parametrize(
    "rel,nolog,log,trans", _CASES,
    ids=[c[0].split("/")[-1][:-4] for c in _CASES])
def test_trnaic_matches_oracle_udg(rel, nolog, log, trans):
    """Cross-check the hard-coded expectations above against the live oracle .udg
    (guards against a stale golden regeneration)."""
    spc = os.path.join(_CORPUS, *rel.split("/"))
    ono = float(_udg_val(spc, "aictest.trans.aicc.nolog"))
    olog = float(_udg_val(spc, "aictest.trans.aicc.log"))
    otrans = _udg_val(spc, "aictrans")
    assert abs(ono - nolog) <= 1e-9 * abs(nolog), (ono, nolog)
    assert abs(olog - log) <= 1e-9 * abs(log), (olog, log)
    assert otrans == trans
