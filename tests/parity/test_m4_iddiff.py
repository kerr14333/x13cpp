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

# Real automdl corpus specs with NO auto-transform / aictest preamble, so the
# harness (pre-model + iddiff [+ amdid]) reproduces the oracle path directly.
# (rel, idr, ids, expected amdid model). idr/ids is iddiff's differencing; the
# model is amdid's identified ARMA orders -- compared to the oracle's SELECTED
# automdl model (arimamdl) where automd leaves it unchanged, and to the oracle's
# BIC winner (best5.mdl1) for region_north, whose arimamdl (3 2 1)(0 1 1) is set
# by automd's later model-adequacy stage (not yet ported) rather than by amdid.
_AUTOMDL_CASES = [
    ("generated/airline_seats.spc", 1, 1, "(0 1 1)(0 1 1)"),
    ("generated/payems_seats.spc", 1, 0, "(0 1 2)"),
    ("generated/unrate_seats.spc", 1, 0, "(0 1 1)"),
    ("edge/span-modelspan.spc", 1, 0, "(0 1 2)"),
    ("generated/expgs_seats.spc", 1, 0, "(2 1 0)"),
    # amdid picks the oracle BIC winner (best5.mdl1); automd's adequacy stage
    # later revises arimamdl to (3 2 1)(0 1 1).
    ("census-examples/composite/region_north.spc", 2, 1, "(1 2 2)(0 1 1)"),
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


# Reduced automd driver (default -> chkmu -> iddiff -> amdid -> mean re-add ->
# final estimate) vs the oracle .udg -- the FULL identified-model estimation
# (arimamdl + variance + loglikelihood), not just the discrete orders. These four
# series need none of the deferred automd features (auto-transform, aictest,
# outlier, model-span, adequacy retry), so the reduced driver reproduces the
# oracle end to end. span-modelspan (model span != series span) and region_north
# (arimamdl set by the adequacy stage) are covered by the identification test
# above but excluded here until those features land.
_AUTOMD_EST_CASES = [
    ("generated/airline_seats.spc", "(0 1 1)(0 1 1)"),
    ("generated/expgs_seats.spc", "(2 1 0)"),
    ("generated/payems_seats.spc", "(0 1 2)"),
    ("generated/unrate_seats.spc", "(0 1 1)"),
]


def _udg_val(spc, key):
    base = os.path.basename(spc)[:-4]
    rel = os.path.relpath(spc, _CORPUS)
    udg = os.path.join(_REPO, "tests", "golden", rel[:-4], base + ".udg")
    for line in open(udg, encoding="utf-8", errors="replace"):
        if line.startswith(key):
            return line.split(":", 1)[1].strip()
    return None


@pytest.mark.parametrize(
    "rel,mdl", _AUTOMD_EST_CASES, ids=[c[0].split("/")[-1][:-4] for c in _AUTOMD_EST_CASES])
def test_automd_full_estimation(rel, mdl):
    spc = os.path.join(_CORPUS, *rel.split("/"))
    vals = _run(spc, "--automd")
    assert vals.get("OUTCOME") == "OK", vals
    assert vals["arimamdl"] == mdl, vals
    ovar = float(_udg_val(spc, "variance$mle"))
    oll = float(_udg_val(spc, "loglikelihood"))
    assert abs(float(vals["variance"]) - ovar) <= 1e-9 * abs(ovar), (vals, ovar)
    # .udg loglikelihood is printed to 4 decimals; match at that precision.
    assert abs(float(vals["loglikelihood"]) - oll) <= 5e-4 * max(1.0, abs(oll)), (vals, oll)


@pytest.mark.parametrize(
    "rel,idr,ids,mdl", _AUTOMDL_CASES,
    ids=[c[0].split("/")[-1][:-4] for c in _AUTOMDL_CASES])
def test_automdl_corpus_identification(rel, idr, ids, mdl):
    """Drive iddiff + amdid on real automdl corpus specs (no auto-transform /
    aictest) and check against their oracle .udg. Differencing (idr/ids) is gated
    for all series; the full amdid model is gated where it currently matches the
    oracle (mdl is None for the two ARMA-order gaps still under investigation)."""
    spc = os.path.join(_CORPUS, *rel.split("/"))
    vals = _run(spc, "--amdid")
    assert vals.get("OUTCOME") == "OK", vals
    assert int(vals["idnonseasonaldiff.first"]) == idr, vals
    assert int(vals["idseasonaldiff.first"]) == ids, vals
    assert vals["arimamdl"] == mdl, vals
