"""Composite + a COMPONENT outlier -- the spec that makes agr's ``Lindot`` live.

``gtinpt.f:311`` sets ``Lindot=T`` unconditionally (``composite{indoutlier=}``
defaults yes). This port's ``agr_cmn`` declares ``bool lindot;`` and only
``getcmp.f``'s parse arm ever assigned it, so on every run that did not spell
the option out it was false -- and all four guards keyed on it took their false
branch: ``agr3.f:200``'s indirect outlier-factor build, ``:222``'s level-shift
refold into the published trend, ``:227``'s AO factor and ``:298``'s D8 divide.

Nothing could see it. Every consumer is a CONJUNCTION (``Lindot.and.Lindls`` /
``.and.Lindao``), and no composite in the corpus carried an outlier, so
``Lindls``/``Lindao`` were false too and the two readings agreed everywhere.
Give one component a level shift and they part company: measured
``itn``/``iir``/``id8``/``id9`` **3.5e-04** out at ``OUTCOME: OK``, down to
~5e-15 once the default was written.

Same shape as entry 87's ``Irev`` -- a flag the port never advances turns every
guard on it into dead code, silently and in bulk, with nothing refusing and
nothing walled.

The shift is on NORTH only. The indirect trend is the aggregate of the
components, so a symmetric design risks the refold cancelling; an asymmetric one
cannot.

Run:  python -m pytest tests/parity/test_composite_outlier.py -q
"""
from __future__ import annotations

import os
import re
import subprocess

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus", "census-examples",
                       "composite-outlier")
_GOLDEN = os.path.join(_REPO, "tests", "golden", "census-examples",
                       "composite-outlier")
_MTA = "composite.mta"
_TOTAL = "total"
_COMPONENTS = ["region_north", "region_south"]

RTOL = 1e-12

# itn/iir/id8/id9 are the four the Lindot guards own; the rest are here so a
# change that fixed those by moving something else cannot pass.
_IND_TAGS = ["itn", "iir", "id8", "id9", "isf", "isa", "ie1", "ie2", "ie3",
             "iee", "i18"]
_DIRECT_TAGS = ["d10", "d11", "d12", "d13"]

_GOLD_RE = re.compile(r"(\d{6})\s+([+\-][0-9.EeDd+\-]+)")


def _find_binary() -> str:
    for c in (os.path.join(_REPO, "build", "x13run_composite.exe"),
              os.path.join(_REPO, "build", "x13run_composite"),
              os.path.join(_REPO, "build", "Release", "x13run_composite.exe")):
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_COMPOSITE")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13run_composite binary not found; build it first (cmake --build build).")


BIN = _find_binary()

_HAVE = (os.path.exists(os.path.join(_CORPUS, _MTA)) and
         os.path.exists(os.path.join(_GOLDEN, _TOTAL, _TOTAL + ".itn")))

pytestmark = pytest.mark.skipif(
    not _HAVE, reason="composite-outlier corpus/golden not present")


def _read_golden(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _GOLD_RE.match(ln.strip())
            if m:
                out[m.group(1)] = float(
                    m.group(2).replace("D", "E").replace("d", "e"))
    return out


@pytest.fixture(scope="module")
def run_output() -> str:
    r = subprocess.run([BIN, _MTA], cwd=_CORPUS, capture_output=True, text=True)
    assert r.returncode == 0, f"harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:400]
    return r.stdout


def _emitted(run_output: str, tag: str) -> dict[str, float]:
    out: dict[str, float] = {}
    for ln in run_output.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == tag and re.fullmatch(r"\d{6}", p[1]):
            out[p[1]] = float(p[2])
    return out


def _check(run_output: str, spec: str, tag: str, prefix: str) -> None:
    produced = _emitted(run_output, prefix + tag)
    gold = _read_golden(os.path.join(_GOLDEN, spec, spec + "." + tag))
    assert gold, f"{spec}.{tag}: empty golden"
    keys = sorted(set(gold) & set(produced))
    assert len(keys) == len(gold), (
        f"{spec}.{tag}: produced {len(produced)} rows, golden {len(gold)}, "
        f"overlap {len(keys)}")
    worst, worst_k = 0.0, None
    for k in keys:
        g, v = gold[k], produced[k]
        rel = abs(v - g) / abs(g) if g else abs(v - g)
        if rel > worst:
            worst, worst_k = rel, k
    assert worst <= RTOL, (
        f"{spec}.{tag}: max rel err {worst:.3e} at {worst_k} (tol {RTOL:.0e})")


def test_outlier_tags_discovered() -> None:
    missing = [t for t in _IND_TAGS
               if not os.path.exists(os.path.join(_GOLDEN, _TOTAL,
                                                  _TOTAL + "." + t))]
    assert not missing, f"composite-outlier ships no golden for {missing}"


@pytest.mark.parametrize("tag", _IND_TAGS)
def test_outlier_indirect(run_output: str, tag: str) -> None:
    _check(run_output, _TOTAL, tag, prefix="")


@pytest.mark.parametrize("tag", _DIRECT_TAGS)
def test_outlier_total_direct(run_output: str, tag: str) -> None:
    _check(run_output, _TOTAL, tag, prefix="")


@pytest.mark.parametrize("base", _COMPONENTS)
@pytest.mark.parametrize("tag", _DIRECT_TAGS)
def test_outlier_component(run_output: str, base: str, tag: str) -> None:
    _check(run_output, base, tag, prefix=base + ":")


def test_the_shift_is_asymmetric() -> None:
    """The gate above only bites while exactly one component carries the LS. If
    the spec is ever made symmetric the refold can cancel and every assertion
    goes quiet, so pin the design rather than the consequence."""
    north = open(os.path.join(_CORPUS, "region_north.spc"),
                 encoding="utf-8", errors="replace").read()
    south = open(os.path.join(_CORPUS, "region_south.spc"),
                 encoding="utf-8", errors="replace").read()
    assert "ls1995.jan" in north, "north lost its level shift"
    assert "regression{" not in south, "south gained a regression spec"
