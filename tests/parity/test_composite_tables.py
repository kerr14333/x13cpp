"""Composite (metafile / indirect adjustment) gate -- increments 1 and 2:
the DIRECT composite total and the INDIRECT adjustment.

Composite adjustment is the one X-13 feature that does not fit in a single spec
run: the oracle executes every spec of a metafile in ONE process so the
aggregation COMMONs (/mq11/ agr.cmn, /agreg/ agrsrs.cmn) persist across them, and
that persistence IS the feature. ``x13run_composite`` reproduces it -- each spec
gets its own X13Context, with the two aggregation blocks carried from one to the
next -- and prints the composite total's X-11 tables unprefixed, each component's
prefixed ``<base>:``.

Gated here (all bit-exact, ~5e-15):

* each COMPONENT's d10-d13 (printed ``<base>:d10`` etc.),
* the composite total's DIRECT d10-d13 -- the aggregate `O` summed from the
  components' originals (agr.f/agr1.f/getcmp.f + agr2.f), handed to the
  ``composite{}`` spec as its series and adjusted like any other run,
* the composite total's INDIRECT isf/isa/itn/iir (agr3.f/agrxpt.f + the
  O1..O5/Ci/Omod buffers) -- the seasonal adjustment rebuilt from the aggregated
  component RESULTS rather than from the aggregate.

The two are genuinely different objects: `isa` is exactly the sum of the
components' d11 (verified to 5e-16 on both the oracle and the engine side),
while d11 is the aggregate adjusted in its own right.

The corpus case is ``census-examples/composite-fixed/`` -- the same synthetic
data as the shipped ``composite/`` example but with a FIXED airline model on the
components, precisely because the indirect adjustment is that sum: any component
estimation drift lands undiluted in it. (With ``composite/``'s automdl{} both
components sit ~1e-4 out and the indirect tables inherit exactly that -- an
automdl-front issue, not an aggregation one. ``composite/`` stays untouched as
the illustrative Census-style example and as an automdl identification case.)

Still deferred to increment 3 (tools/composite_scouting.md): the Iagr==4
direct-vs-indirect comparison statistics (aggmea/cmpchi), the indirect D8/D9 and
E-tables, and the aggregate-composition header table.

Run:  python -m pytest tests/parity/test_composite_tables.py -q
"""
from __future__ import annotations

import os
import re
import subprocess

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus", "census-examples", "composite-fixed")
_GOLDEN = os.path.join(_REPO, "tests", "golden", "census-examples",
                       "composite-fixed")

# The aggregate is a pure sum of the component originals followed by an ordinary
# X-11 decomposition, so it reaches the arithmetic floor. Measured worst 5.1e-15.
RTOL = 1e-12

_TAGS = ["d10", "d11", "d12", "d13"]              # direct (aggregate adjusted)
_IND_TAGS = ["isf", "isa", "itn", "iir"]          # indirect (sum of components)
_COMPONENTS = ["region_north", "region_south"]
_GOLD_RE = re.compile(r"(\d{6})\s+([+\-][0-9.EeDd+\-]+)")
_MTA = "composite.mta"
_TOTAL = "total"     # the last spec of the metafile: the composite total


def _find_binary() -> str:
    cands = [
        os.path.join(_REPO, "build", "x13run_composite.exe"),
        os.path.join(_REPO, "build", "x13run_composite"),
        os.path.join(_REPO, "build", "Release", "x13run_composite.exe"),
    ]
    for c in cands:
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_COMPOSITE")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13run_composite binary not found; build it first (cmake --build build).")


BIN = _find_binary()


def _read_golden(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _GOLD_RE.match(ln.strip())
            if m:
                out[m.group(1)] = float(m.group(2).replace("D", "E").replace("d", "e"))
    return out


_HAVE = (os.path.exists(os.path.join(_CORPUS, _MTA)) and
         all(os.path.exists(os.path.join(_GOLDEN, _TOTAL, _TOTAL + "." + t))
             for t in _TAGS + _IND_TAGS))


def _check(run_output: str, base: str, tag: str, prefix: str) -> None:
    """Compare one emitted table against its golden, period-key exact."""
    produced: dict[str, float] = {}
    want = prefix + tag
    for ln in run_output.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == want:
            produced[p[1]] = float(p[2])

    gold = _read_golden(os.path.join(_GOLDEN, base, base + "." + tag))
    assert gold, f"{base}.{tag}: empty golden"

    keys = sorted(set(gold) & set(produced))
    assert len(keys) == len(gold), (
        f"{base}.{tag}: produced {len(produced)} rows, golden {len(gold)}, "
        f"overlap {len(keys)}")

    worst = 0.0
    worst_k = None
    for k in keys:
        g, v = gold[k], produced[k]
        rel = abs(v - g) / abs(g) if g else abs(v - g)
        if rel > worst:
            worst, worst_k = rel, k
    assert worst <= RTOL, (
        f"{base}.{tag}: max rel err {worst:.3e} at {worst_k} (tol {RTOL:.0e})")


@pytest.fixture(scope="module")
def run_output() -> str:
    r = subprocess.run([BIN, _MTA], cwd=_CORPUS, capture_output=True, text=True)
    assert r.returncode == 0, f"harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:400]
    return r.stdout


@pytest.mark.skipif(not _HAVE, reason="composite metafile corpus/golden not present")
@pytest.mark.parametrize("tag", _TAGS)
def test_composite_total_direct(run_output: str, tag: str) -> None:
    """The aggregate adjusted in its own right (unprefixed d10-d13)."""
    _check(run_output, _TOTAL, tag, prefix="")


@pytest.mark.skipif(not _HAVE, reason="composite metafile corpus/golden not present")
@pytest.mark.parametrize("tag", _IND_TAGS)
def test_composite_total_indirect(run_output: str, tag: str) -> None:
    """The adjustment rebuilt from the aggregated component results."""
    _check(run_output, _TOTAL, tag, prefix="")


@pytest.mark.skipif(not _HAVE, reason="composite metafile corpus/golden not present")
@pytest.mark.parametrize("base", _COMPONENTS)
@pytest.mark.parametrize("tag", _TAGS)
def test_composite_component(run_output: str, base: str, tag: str) -> None:
    """Each component's own adjustment -- what the indirect tables are built from."""
    _check(run_output, base, tag, prefix=base + ":")
