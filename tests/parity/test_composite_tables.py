"""Composite (metafile / indirect adjustment) gate -- INCREMENT 1: the DIRECT
composite total.

Composite adjustment is the one X-13 feature that does not fit in a single spec
run: the oracle executes every spec of a metafile in ONE process so the
aggregation COMMONs (/mq11/ agr.cmn, /agreg/ agrsrs.cmn) persist across them, and
that persistence IS the feature. ``x13run_composite`` reproduces it -- each spec
gets its own X13Context, with the two aggregation blocks carried from one to the
next -- and prints the composite total's X-11 tables unprefixed, each component's
prefixed ``<base>:``.

What increment 1 gates: the aggregate `O` (agr.f/agr1.f/getcmp.f + the direct
branch of agr2.f) is summed from the components' originals, handed to the
``composite{}`` spec as its series, and adjusted. So **total**'s d10-d13 must be
bit-exact. The indirect adjustment (agr3, the Ci/O1..O5 buffers) and the
direct-vs-indirect comparison statistics are increments 2 and 3 -- see
tools/composite_scouting.md.

The COMPONENT tables are deliberately not gated here: region_north/region_south
run automdl{} on a synthetic series and sit ~1e-4 from the oracle, a
model-selection/estimation-path difference that has nothing to do with
aggregation (the aggregate is exact precisely because it is built from the raw
originals, not from the component adjustments). That belongs to the automdl
front, tracked separately.

Run:  python -m pytest tests/parity/test_composite_tables.py -q
"""
from __future__ import annotations

import os
import re
import subprocess

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus", "census-examples", "composite")
_GOLDEN = os.path.join(_REPO, "tests", "golden", "census-examples", "composite",
                       "_metafile")

# The aggregate is a pure sum of the component originals followed by an ordinary
# X-11 decomposition, so it reaches the arithmetic floor. Measured worst 5.1e-15.
RTOL = 1e-12

_TAGS = ["d10", "d11", "d12", "d13"]
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
             for t in _TAGS))


@pytest.fixture(scope="module")
def run_output() -> str:
    r = subprocess.run([BIN, _MTA], cwd=_CORPUS, capture_output=True, text=True)
    assert r.returncode == 0, f"harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:400]
    return r.stdout


@pytest.mark.skipif(not _HAVE, reason="composite metafile corpus/golden not present")
@pytest.mark.parametrize("tag", _TAGS)
def test_composite_total_table(run_output: str, tag: str) -> None:
    produced: dict[str, float] = {}
    for ln in run_output.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == tag:     # unprefixed == the composite total
            produced[p[1]] = float(p[2])

    gold = _read_golden(os.path.join(_GOLDEN, _TOTAL, _TOTAL + "." + tag))
    assert gold, f"{_TOTAL}.{tag}: empty golden"

    keys = sorted(set(gold) & set(produced))
    assert len(keys) == len(gold), (
        f"{_TOTAL}.{tag}: produced {len(produced)} rows, golden {len(gold)}, "
        f"overlap {len(keys)}")

    worst = 0.0
    worst_k = None
    for k in keys:
        g, v = gold[k], produced[k]
        rel = abs(v - g) / abs(g) if g else abs(v - g)
        if rel > worst:
            worst, worst_k = rel, k
    assert worst <= RTOL, (
        f"{_TOTAL}.{tag}: max rel err {worst:.3e} at {worst_k} (tol {RTOL:.0e})")
