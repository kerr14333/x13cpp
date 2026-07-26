"""M5 x11regression{} tdprior gate: user prior trading-day weights (Kswv=1).

`x11regression{ tdprior = (w1 .. w7) }` supplies seven user trading-day weights
(Mon..Sun). The editor standardizes them to sum 7.0 (multiplicative / log-additive)
and X-11 builds a prior-TD adjustment factor (pritd.f: td6var day-of-week contrasts
x the weights, normalized by the monthly day-count Xnstar). The factor is the a4
save table; it divides the series and folds into the D16 combined calendar factor.

Because in the oracle x11pt1 runs BEFORE the regARIMA estimate (x11ari.f:99-133),
the model sees the prior-TD-adjusted series (the airline model shifts from MA1~=0.40
to ~0.68). The C++ runs X-11 as a later phase, so the prior-TD division is mirrored
onto the pre-model estimation input (run_pre_model.cpp) as well as the X-11 buffer
(x11pt1.cpp) -- both bit-exact against the oracle.

GATED here, all bit-exact:
  * a4 -- the prior-TD factor (pure calendar arithmetic on the fixed weights, 1e-12)
  * d10/d11/d12/d13 -- the final seasonal / SA / trend / irregular decomposition on
    the prior-TD-adjusted (single fixed-model) series (estimation floor, 1e-6;
    measured ~5e-15).

Run:  python -m pytest tests/parity/test_x11_tdprior_tables.py -q
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

# a4 is exact day-contrast arithmetic on the fixed weights -> arithmetic floor.
# d10-d13 ride the single fixed-model estimate -> estimation floor (measured ~5e-15,
# gated looser for optimizer portability, matching the model-X11 table gates).
RTOL_A4 = 1e-12
RTOL_DTBL = 1e-6

# (tag, tolerance)
_TABLES = [("a4", RTOL_A4), ("d10", RTOL_DTBL), ("d11", RTOL_DTBL),
           ("d12", RTOL_DTBL), ("d13", RTOL_DTBL)]

_ROW = re.compile(r"^(\d{6})\s+([+\-][0-9.EeDd+\-]+)")


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


def _read_golden(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _ROW.match(ln.strip())
            if m:
                out[m.group(1)] = float(m.group(2).replace("D", "E").replace("d", "e"))
    return out


CASES = [
    b for b in ("airline_x11regression-tdprior",
                # x11{mode=logadd}. editor.f:1507 allows the weights for
                # multiplicative OR log-additive, and x11pt1.f:52 collapses
                # Muladd 2->0 for the prior stage, so logadd takes the identical
                # divide. run_pre_model had gated its half on muladd==0 and
                # x11pt1's own guard tests muladd AFTER that collapse, so logadd
                # fell through both and returned OUTCOME: OK with the prior TD
                # missing from B1 and d10-d13 -- off by exactly a factor of a4
                # (~2e-2..3.8e-2), while a4 itself stayed bit-exact.
                "airline_x11regression-tdprior-logadd",
                # x11pt1.f:229-230's ENTRY condition, not the block itself: with
                # the classic X-11 Easter on (Khol==2) and no x11-regression
                # prior calendar, the oracle SKIPS the prior-TD block and
                # adjusts without one. It does not reject the spec -- it writes
                # d10-d13 normally. The engine used to FATAL there (measured:
                # oracle 144 d10 rows, engine none), because the guard could not
                # tell "unported branch" from "branch the oracle declines to
                # enter". This spec therefore ships NO a4 golden by design --
                # pritd never runs -- which is why the filter below keys on d10.
                "airline_x11regression-tdprior-x11easter")
    if os.path.exists(os.path.join(_CORPUS, b + ".spc"))
    and os.path.exists(os.path.join(_GOLDEN, b, b + ".d10"))
]


@pytest.mark.skipif(not CASES, reason="no tdprior spec ships the d10 golden")
@pytest.mark.parametrize("base", CASES)
@pytest.mark.parametrize("tag,tol", _TABLES)
def test_tdprior_table(base: str, tag: str, tol: float) -> None:
    goldpath = os.path.join(_GOLDEN, base, base + "." + tag)
    if not os.path.exists(goldpath):
        pytest.skip(f"{base}: no {tag} golden")

    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], cwd=_CORPUS, capture_output=True, text=True)
    assert r.returncode == 0, f"{base}: harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:200]

    produced: dict[str, float] = {}
    for ln in r.stdout.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == tag:
            produced[p[1]] = float(p[2])

    gold = _read_golden(goldpath)
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
    assert worst <= tol, (
        f"{base}.{tag}: max rel err {worst:.3e} at {worst_k} (tol {tol:.0e})")
