"""M5 x11regression{} gate: OLS-estimated prior trading day (Ixreg>=2 / xrgdrv).

`x11regression{}` with a modeled `-td` variable regresses trading-day contrasts on
the X-11 irregular and removes them as a prior calendar factor. Because the spec
also carries an arima model, gtinpt.f:1201 promotes Ixreg 1->2, and the oracle runs
a transparent (model-free) X-11 pass -- xrgdrv.f, ahead of x11pt1/arima -- whose
x11mdl OLS estimates the TD on the irregular, builds Faccal, and sets Ixreg=3. The
main x11pt1 then divides the series by Faccal, so the regARIMA model fits the
TD-adjusted series (converged nonseasonal MA1 ~= 0.2607, vs bare-airline ~0.40).

The C++ runs regARIMA in an earlier phase (run_pre_model) than the main X-11
(run_x11), so xrgdrv is hoisted ahead of the estimate: run_pre_model divides the
estimation input by the stashed Faccal, and x11pt1 restores Faccal for the Ixreg==3
divide. The whole chain is bit-exact -- including the two /x11/ state vars the
transparent pass must NOT leak into the main run (Lterm, which drives the editor's
per-period seasonal-filter re-resolution, and the Bundesbank Ksdev spread; both are
saved/restored in xrgdrv.cpp, same class as the slidingspans/history per-span reset).

GATED here, all bit-exact:
  * xrm -- the regression DESIGN matrix (the Nb trading-day contrast columns over
    the forecast-extended span). Pure integer/arithmetic day contrasts (1e-12).
  * d10/d11/d12/d13 -- the final seasonal / SA / trend / irregular decomposition on
    the OLS-prior-TD-adjusted series. These ride the estimated Faccal and the
    regARIMA fit end to end, so they gate the whole feature (estimation floor, 1e-6;
    measured ~5e-15).

Run:  python -m pytest tests/parity/test_x11regression_tables.py -q
"""
from __future__ import annotations

import os
import re
import subprocess

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus", "extra")
_GOLDEN = os.path.join(_REPO, "tests", "golden", "extra")

# The xrm design is exact integer day-contrast columns; gate at the arithmetic
# floor (three orders over text-truncation noise).
RTOL_XRM = 1e-12
# d10-d13 ride the OLS-estimated prior-TD Faccal + the regARIMA fit -> estimation
# floor (measured ~5e-15, gated looser for optimizer portability, matching the
# other model-X11 table gates).
RTOL_DTBL = 1e-6

_DTABLES = ("d10", "d11", "d12", "d13")


def _find_binary() -> str:
    for c in (
        os.path.join(_REPO, "build", "x13run_x11.exe"),
        os.path.join(_REPO, "build", "x13run_x11"),
    ):
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_X11")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError("x13run_x11 binary not found; build it first.")


BIN = _find_binary()

# Golden .xrm row: "YYYYMM<TAB>col1<TAB>col2..." (D/E exponent, CRLF tolerated).
_XRM_ROW = re.compile(r"^(\d{6})\t(.+)$")
# Golden d-table row: "YYYYMM  <signed value>".
_DROW = re.compile(r"^(\d{6})\s+([+\-][0-9.EeDd+\-]+)")


def _read_golden_xrm(path: str) -> dict[str, list[float]]:
    out: dict[str, list[float]] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _XRM_ROW.match(ln.rstrip("\r\n"))
            if m:
                out[m.group(1)] = [
                    float(v.replace("D", "E").replace("d", "e"))
                    for v in m.group(2).split("\t")
                ]
    return out


def _read_golden_dtable(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _DROW.match(ln.strip())
            if m:
                out[m.group(1)] = float(m.group(2).replace("D", "E").replace("d", "e"))
    return out


def _run(base: str) -> str:
    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], cwd=_CORPUS, capture_output=True, text=True)
    assert r.returncode == 0, f"{base}: harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:200]
    return r.stdout


CASES = [
    b for b in ("airline_x11regression-td",)
    if os.path.exists(os.path.join(_CORPUS, b + ".spc"))
    and os.path.exists(os.path.join(_GOLDEN, b, b + ".xrm"))
]


@pytest.mark.skipif(not CASES, reason="no x11regression spec ships the xrm golden")
@pytest.mark.parametrize("base", CASES)
def test_x11regression_xrm(base: str) -> None:
    stdout = _run(base)

    produced: dict[str, list[float]] = {}
    for ln in stdout.splitlines():
        p = ln.split()
        if p and p[0] == "xrm":
            produced[p[1]] = [float(x) for x in p[2:]]

    gold = _read_golden_xrm(os.path.join(_GOLDEN, base, base + ".xrm"))
    assert gold, f"{base}.xrm: empty golden"

    keys = sorted(set(gold) & set(produced))
    assert len(keys) == len(gold), (
        f"{base}.xrm: produced {len(produced)} rows, golden {len(gold)}, "
        f"overlap {len(keys)}")

    worst, worst_k = 0.0, None
    for k in keys:
        g, v = gold[k], produced[k]
        assert len(g) == len(v), f"{base}.xrm {k}: {len(v)} cols vs golden {len(g)}"
        for gi, vi in zip(g, v):
            rel = abs(vi - gi) / abs(gi) if gi else abs(vi - gi)
            if rel > worst:
                worst, worst_k = rel, k
    assert worst <= RTOL_XRM, (
        f"{base}.xrm: max rel err {worst:.3e} at {worst_k} (tol {RTOL_XRM:.0e})")


@pytest.mark.skipif(not CASES, reason="no x11regression spec ships the xrm golden")
@pytest.mark.parametrize("base", CASES)
@pytest.mark.parametrize("tag", _DTABLES)
def test_x11regression_dtable(base: str, tag: str) -> None:
    goldpath = os.path.join(_GOLDEN, base, base + "." + tag)
    if not os.path.exists(goldpath):
        pytest.skip(f"{base}: no {tag} golden")

    stdout = _run(base)
    produced: dict[str, float] = {}
    for ln in stdout.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == tag:
            produced[p[1]] = float(p[2])

    gold = _read_golden_dtable(goldpath)
    assert gold, f"{base}.{tag}: empty golden"

    keys = sorted(set(gold) & set(produced))
    assert len(keys) == len(gold), (
        f"{base}.{tag}: produced {len(produced)} rows, golden {len(gold)}, "
        f"overlap {len(keys)}")

    worst, worst_k = 0.0, None
    for k in keys:
        g, v = gold[k], produced[k]
        rel = abs(v - g) / abs(g) if g else abs(v - g)
        if rel > worst:
            worst, worst_k = rel, k
    assert worst <= RTOL_DTBL, (
        f"{base}.{tag}: max rel err {worst:.3e} at {worst_k} (tol {RTOL_DTBL:.0e})")
