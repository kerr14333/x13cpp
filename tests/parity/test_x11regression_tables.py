"""M5 x11regression{} gate: the irregular-component trading-day regression.

x11regression{} regresses user-chosen calendar effects (here: trading day) on the
X-11 irregular and removes them. The compute is in progress; this gate currently
locks the parts that are bit-exact:

  * xrm -- the regression DESIGN matrix (the Nb trading-day contrast columns over
    the forecast-extended span). Pure integer/arithmetic day contrasts, so it
    matches the oracle exactly.

The estimated factor tables (b16/c16) and the TD-adjusted D-tables are NOT gated
here yet: they still carry a ~1.5e-3 leap-February coefficient residual (the
single-pass tdxtrm-exclusion coupling documented in tools/x11regression_scope.md).
They land once that residual closes.

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


CASES = [
    b for b in ("airline_x11regression-td",)
    if os.path.exists(os.path.join(_CORPUS, b + ".spc"))
    and os.path.exists(os.path.join(_GOLDEN, b, b + ".xrm"))
]


@pytest.mark.skipif(not CASES, reason="no x11regression spec ships the xrm golden")
@pytest.mark.parametrize("base", CASES)
def test_x11regression_xrm(base: str) -> None:
    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.returncode == 0, f"{base}: harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:200]

    produced: dict[str, list[float]] = {}
    for ln in r.stdout.splitlines():
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
