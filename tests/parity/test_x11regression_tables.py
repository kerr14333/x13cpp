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

# b16/c16 join the d-tables here rather than staying an aictest-only comparison
# (_AIC_TABLES below): they are the B and C iteration TD factors, and with an
# x11regression span that ends early the two carry DIFFERENT lengths -- 120 rows
# and 144 on airline_x11regression-span-end, x11mdl.f:514-517's `lastpr`.
# Nothing was checking either one on a non-aictest spec, so dropping that
# extension left the whole suite green.
_DTABLES = ("d10", "d11", "d12", "d13", "b16", "c16")


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


# AUTO-DISCOVERED, and it was a three-element literal until 2026-08-03 -- the
# same defect AIC_CASES below already carries a note about, one list over. Six
# `x11regression{span=}` specs had been added since without ever reaching this
# gate. Proved by mutation: dropping x11mdl.f:514-517's `lastpr` extension (c16
# comes out 120 rows where the oracle writes 144) and moving the .xrm save back
# to AFTER the span restore (132 rows -> 156) both left the whole suite green.
# A hand-maintained case list is an allowlist that silently stops growing.
CASES = sorted(
    b for b in (
        os.path.splitext(f)[0] for f in os.listdir(_CORPUS) if f.endswith(".spc")
    )
    if "x11regression" in b
    and os.path.exists(os.path.join(_GOLDEN, b, b + ".xrm"))
)


def test_xrm_cases_discovered() -> None:
    """An empty (or shrunken) parametrisation passes silently."""
    assert len(CASES) >= 9, f"only {len(CASES)} x11regression xrm specs found"


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


# --- aictest=(easter) : the x11aic Easter-window AICC selection -------------
# The modeled `-td aictest=(easter)` spec runs x11mdl's automatic Easter test on
# the irregular (x11aic.f): it scores the AICC of the no-Easter model and each
# candidate window (1/8/15), keeps the lowest, and reports the choice. The four
# AICC values print to the .udg at full precision (e29.15), so we gate them tight.
RTOL_AICC = 1e-9

# golden .udg tag -> harness "aicc_xe <window>" key (window 0 == noeaster).
_AICC_WIN = {"noeaster": 0, "easter01": 1, "easter08": 8, "easter15": 15}

# AUTO-DISCOVERED, and it used to be a one-element literal. Three
# `x11regression{aictest=}` specs were added on 2026-07-31 for the TRADING-DAY
# branch and none of them were compared here, because the list named only the
# Easter spec by hand. Proved by mutation: deleting x11mdl.f:308-381's
# no-regressors-left early return -- which makes the reject spec's c16 come out
# carrying the length-of-month prior instead of all ones -- left the whole
# suite green. A hand-maintained case list is an allowlist that silently stops
# growing; the floor assertion below is what keeps this one honest.
AIC_CASES = sorted(
    b for b in (
        os.path.splitext(f)[0] for f in os.listdir(_CORPUS) if f.endswith(".spc")
    )
    if "x11regression" in b and "aictest" in b
    and os.path.exists(os.path.join(_GOLDEN, b, b + ".udg"))
)


def test_aictest_cases_discovered() -> None:
    """An empty (or shrunken) parametrisation passes silently."""
    assert len(AIC_CASES) >= 4, f"only {len(AIC_CASES)} aictest specs found"

_UDG_AICC = re.compile(r"^aictest\.xe\.aicc\.(\w+):\s+([+\-][0-9.EeDd+\-]+)")
_UDG_WIN = re.compile(r"^aictest\.xe\.window:\s+(\d+)")


def _read_golden_aicc(udgpath: str) -> tuple[dict[int, float], int]:
    aicc: dict[int, float] = {}
    window = -1
    with open(udgpath, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _UDG_AICC.match(ln.strip())
            if m and m.group(1) in _AICC_WIN:
                aicc[_AICC_WIN[m.group(1)]] = float(
                    m.group(2).replace("D", "E").replace("d", "e"))
            mw = _UDG_WIN.match(ln.strip())
            if mw:
                window = int(mw.group(1))
    return aicc, window


@pytest.mark.skipif(not AIC_CASES, reason="no aictest spec ships the udg golden")
@pytest.mark.parametrize("base", AIC_CASES)
def test_x11regression_aictest_easter(base: str) -> None:
    gold_aicc, gold_win = _read_golden_aicc(
        os.path.join(_GOLDEN, base, base + ".udg"))
    if not gold_aicc:
        # A td-only aictest spec has no Easter table at all. The `xtd` half is
        # gated as text in test_aictest_savelog.py; this test owns the Easter
        # AICC canaries only.
        pytest.skip(f"{base}: no aictest.xe.aicc.* rows (not an Easter test)")

    stdout = _run(base)
    prod_aicc: dict[int, float] = {}
    prod_win = None
    for ln in stdout.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == "aicc_xe":
            prod_aicc[int(p[1])] = float(p[2])
        elif len(p) == 2 and p[0] == "aicc_xe_window":
            prod_win = int(p[1])

    assert prod_win == gold_win, (
        f"{base}: chosen Easter window {prod_win} vs golden {gold_win}")
    for win, g in gold_aicc.items():
        assert win in prod_aicc, f"{base}: no produced AICC for window {win}"
        rel = abs(prod_aicc[win] - g) / abs(g)
        assert rel <= RTOL_AICC, (
            f"{base}: AICC window {win} rel err {rel:.3e} "
            f"(mine {prod_aicc[win]!r} vs golden {g!r}, tol {RTOL_AICC:.0e})")


# --- aictest=(easter) : the 14-column design + b16/c16 TD-factor tables ------
# Once the Easter window is chosen, x11mdl runs automatic AO outlier ID on the
# irregular (idotlr Lxreg path): the 7 AOs join the 6 TD + Easter[15] columns for
# a 14-column design (xrm), and the AO-cleaned TD coefficients drive the B/C-16
# trading-day factor tables. The AO detection is the same routine as the regARIMA
# outlier engine (shared idotlr, lxreg=true -> regx11 OLS re-fits, no ARMA filter).
_AIC_TABLES = ("b16", "c16")


@pytest.mark.skipif(not AIC_CASES, reason="no aictest spec ships the xrm golden")
@pytest.mark.parametrize("base", AIC_CASES)
def test_x11regression_aictest_xrm(base: str) -> None:
    goldpath = os.path.join(_GOLDEN, base, base + ".xrm")
    if not os.path.exists(goldpath):
        pytest.skip(f"{base}: no xrm golden")
    stdout = _run(base)

    produced: dict[str, list[float]] = {}
    for ln in stdout.splitlines():
        p = ln.split()
        if p and p[0] == "xrm":
            produced[p[1]] = [float(x) for x in p[2:]]

    gold = _read_golden_xrm(goldpath)
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


@pytest.mark.skipif(not AIC_CASES, reason="no aictest spec ships the table goldens")
@pytest.mark.parametrize("base", AIC_CASES)
@pytest.mark.parametrize("tag", _AIC_TABLES)
def test_x11regression_aictest_table(base: str, tag: str) -> None:
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
