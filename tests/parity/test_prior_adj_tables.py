"""M5 user PRIOR-adjustment gate: transform{} user prior factors (Usrpad/Usrtad).

`transform{}` can carry a series of user prior-adjustment factors -- inline via
``data=`` or read from a file via ``file=`` -- that divide the series before it is
modelled and adjusted. Two kinds (getadj.f TYPDIC):

* ``type=permanent`` (Usrpad): removed for good. x11pt3's rmpadj takes it back
  out of the final SA series (D11).
* ``type=temporary`` (Usrtad): removed before the adjustment but PUT BACK -- it
  stays in D11 and is stripped from the irregular (D13) instead
  (x11pt3.f:591-604). With ``temppriortrend=yes`` it also folds into the
  published trend, D12 (x11pt3.f:937-946).

Both combine with the predefined length-of-period priors (``adjust=lom/loq/
lpyear``) into one factor series (adjsrs.f -> /adjcmn/ Adj -> x11int -> Sprior ->
x11pt1's ``Sto /= Sprior``).

The specs here deliberately cover both driver paths, which reach the prior by
different routes: WITH a model, run_pre_model builds the factors and arima.f's
extend/adjreg chain carries the prior-adjusted series into X-11; with NO model
nothing upstream runs, so run_x11 builds the /adjcmn/ record itself and x11int +
x11pt1 do the removal. The no-model path used to decompose the RAW series and
then throw indexing Usrpad at Frstap==0.

GATED here, all bit-exact (measured ~5e-15):
  * ``airline_prior-perm-file``  -- file= permanent factors, NO model
  * ``airline_prior-temp``       -- temporary factors (D13 strip), with a model
  * ``airline_prior-temp-trend`` -- ... plus temppriortrend=yes (D12 fold)
  * ``airline_prior-lom-user``   -- predefined lom prior + a user permanent one

Not ported, and fataling cleanly rather than silently dropping anything: more
than one prior set (Nprtyp>1), ``mode=diff``/additive factors (Adjmod=2),
``format=`` (formatted read), and the addadj span shift (Frstad!=0 -- which the
oracle rejects outright anyway).

Run:  python -m pytest tests/parity/test_prior_adj_tables.py -q
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

# The prior factors are exact arithmetic, but three of the four specs ride a
# single fixed-model estimate -> estimation floor, matching the other model-X11
# table gates. Measured worst 5.1e-15.
RTOL = 1e-6

_TAGS = ["d10", "d11", "d12", "d13"]
_BASES = [
    "airline_prior-perm-file",
    "airline_prior-temp",
    "airline_prior-temp-trend",
    "airline_prior-lom-user",
]
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

CASES = [
    b for b in _BASES
    if os.path.exists(os.path.join(_CORPUS, b + ".spc"))
    and os.path.exists(os.path.join(_GOLDEN, b, b + ".d11"))
]


def _read_golden(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _ROW.match(ln.strip())
            if m:
                out[m.group(1)] = float(m.group(2).replace("D", "E").replace("d", "e"))
    return out


@pytest.fixture(scope="module")
def runs() -> dict[str, str]:
    """One harness run per spec, shared across its four table cases."""
    out: dict[str, str] = {}
    for base in CASES:
        spec = os.path.join(_CORPUS, base + ".spc")
        r = subprocess.run([BIN, spec], cwd=_CORPUS, capture_output=True, text=True)
        assert r.returncode == 0, f"{base}: harness exit {r.returncode}\n{r.stderr}"
        assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:300]
        out[base] = r.stdout
    return out


@pytest.mark.skipif(not CASES, reason="no prior-adjustment spec ships its goldens")
@pytest.mark.parametrize("base", CASES)
@pytest.mark.parametrize("tag", _TAGS)
def test_prior_table(runs: dict[str, str], base: str, tag: str) -> None:
    goldpath = os.path.join(_GOLDEN, base, base + "." + tag)
    if not os.path.exists(goldpath):
        pytest.skip(f"{base}: no {tag} golden")

    produced: dict[str, float] = {}
    for ln in runs[base].splitlines():
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
    assert worst <= RTOL, (
        f"{base}.{tag}: max rel err {worst:.3e} at {worst_k} (tol {RTOL:.0e})")
