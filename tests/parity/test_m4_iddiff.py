"""M4 gate: automatic differencing-order identification (iddiff.f).

Drives ``x13run_iddiff`` -- pre-model (series read + log transform + regvar) then
``x13::iddiff`` -- on the clean log-airline series and checks the identified
regular/seasonal differencing orders against the oracle. The census automdl
example (``census-examples/03-automdl``) reports iddiff's decision for the
airline series in its ``.udg`` as::

    idnonseasonaldiff.first:   1
    idseasonaldiff.first:   1

The corpus fixture ``extra/airline_iddiff.spc`` isolates iddiff on that same
series with a fixed log transform and no regression, so the identified orders
must be (1, 1). This exercises the full identification chain end to end --
mdlint/mdlset (trial-model construction), amdest/hrest (Hannan-Rissen initial
estimates), chkrt1 (AR-root check), rgarma (exact-MLE re-estimate), and the
differencing-counter loop -- as a single discrete-decision gate.
"""
import os
import subprocess

import pytest

from test_m3_estimate import _REPO, _CORPUS, _find_binary

BIN = _find_binary("x13run_iddiff", "X13RUN_IDDIFF")

# spec (relative to the corpus root) -> (expected idr, ids) from the oracle .udg.
_CASES = [
    ("extra/airline_iddiff.spc", 1, 1),
]


def _run(spc_abs, *extra):
    proc = subprocess.run([BIN, spc_abs, *extra], capture_output=True, text=True)
    assert proc.returncode == 0, f"x13run_iddiff failed: {proc.stderr}\n{proc.stdout}"
    vals = {}
    for line in proc.stdout.splitlines():
        if ":" in line:
            k, _, v = line.partition(":")
            vals[k.strip()] = v.strip()
    return vals


@pytest.mark.parametrize(
    "rel,idr,ids", _CASES, ids=lambda x: x if isinstance(x, str) else "")
def test_iddiff_orders(rel, idr, ids):
    spc = os.path.join(_CORPUS, *rel.split("/"))
    vals = _run(spc)
    assert vals.get("OUTCOME") == "OK", vals
    assert int(vals["idnonseasonaldiff.first"]) == idr, vals
    assert int(vals["idseasonaldiff.first"]) == ids, vals


def test_amdid_full_model():
    """iddiff + amdid on the log-airline automdl spec must identify the canonical
    (0 1 1)(0 1 1) -- the census 03-automdl oracle .udg ``arimamdl``. Exercises
    the full ARMA-order grid (amdid2/bestmd/mdlmch) on top of iddiff."""
    spc = os.path.join(_CORPUS, "extra", "airline_automdl.spc")
    vals = _run(spc, "--amdid")
    assert vals.get("OUTCOME") == "OK", vals
    assert vals["idnonseasonaldiff.first"] == "1", vals
    assert vals["idseasonaldiff.first"] == "1", vals
    assert vals["arimamdl"] == "(0 1 1)(0 1 1)", vals
