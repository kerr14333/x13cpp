"""M5 X-11 gate: x11pt4.f's PART-E tables vs the oracle save goldens.

Part E is what the X-11 spine produces once the D tables are final -- the
"modified" series with the extreme values replaced, the period-to-period changes,
and the total adjustment ratios:

  * ``e1``   -- original series modified for extremes and priors (Stome)
  * ``e2``   -- modified seasonally adjusted series (Stcime)
  * ``e3``   -- modified irregular (Stime)
  * ``e5``   -- changes in the original series      (``pe5`` = the same in percent)
  * ``e6``   -- changes in the SA series            (``pe6``)
  * ``e6a``  -- changes in the FORCED SA series (force{}), ``e6r`` in the rounded one
  * ``e7``   -- changes in the final trend-cycle    (``pe7``)
  * ``e8``   -- changes in the calendar-adjusted original (``pe8``)
  * ``e11``  -- a more robust seasonally adjusted series
  * ``e18``  -- the final adjustment ratios A1 / D11

``e4`` (ratios of annual totals) is print-only in the oracle -- ``table`` with no
``punch`` -- so no golden exists and it is deliberately not produced.

TOLERANCE. These are 15-significant-digit save goldens, so the same two-tier
policy as test_x11_tables applies: pure-arithmetic (no-model) decomposition gates
at 1e-12, model-bearing specs at the estimation floor 1e-6. ``e5``-``e8`` are
period-to-period CHANGES, i.e. differences of neighbouring values, so they lose
relative precision wherever the change is near zero -- those tags therefore get
an absolute floor as well, scaled to the table's own magnitude.

Run:  python -m pytest tests/parity/test_x11_etables.py -q
"""
from __future__ import annotations

import functools
import os
import re
import subprocess

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus")
_GOLDEN = os.path.join(_REPO, "tests", "golden")

_TREES = ("generated", "extra", "census-examples")

RTOL_ARITHMETIC = 1e-12   # no-model / direct-X11 path
RTOL_ESTIMATION = 1e-6    # model-based (automdl/aictest/arima) runs

# Discovery requires e1: a spec that saved the E family saved that one.
_REQUIRED = "e1"
_TAGS = ("e1", "e2", "e3", "e5", "pe5", "e6", "pe6", "e6a", "e6r",
         "e7", "pe7", "e8", "pe8", "e11", "e18")

# The change tables are differences, so a row whose change is ~0 has no
# meaningful relative error. Compare those against an absolute floor derived from
# the golden column's own scale.
_CHANGE_TAGS = {"e5", "pe5", "e6", "pe6", "e6a", "e6r", "e7", "pe7", "e8", "pe8"}


def _find_binary() -> str:
    for c in (os.path.join(_REPO, "build", "x13run_x11.exe"),
              os.path.join(_REPO, "build", "x13run_x11"),
              os.path.join(_REPO, "build", "Release", "x13run_x11.exe")):
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_X11")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13run_x11 binary not found; build it first (cmake --build build).")


BIN = _find_binary()

_GOLD_RE = re.compile(r"(\d{6})\s+([+\-][0-9.EeDd+\-]+)")


def _read_golden(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _GOLD_RE.match(ln.strip())
            if m:
                out[m.group(1)] = float(
                    m.group(2).replace("D", "E").replace("d", "e"))
    return out


def _is_no_model(spec_path: str) -> bool:
    txt = open(spec_path, encoding="utf-8", errors="replace").read().lower()
    modeling = ("arima{", "automdl{", "estimate{", "regression{", "outlier{",
                "forecast{", "seats{", "pickmdl{", "check{")
    return "x11{" in txt and not any(k in txt for k in modeling)


def _discover() -> list[str]:
    out: list[str] = []
    for tree in _TREES:
        root = os.path.join(_CORPUS, tree)
        if not os.path.isdir(root):
            continue
        for dirpath, _dirs, files in os.walk(root):
            # A metafile means these specs are composite components: the oracle
            # ran them in ONE process with the aggregation COMMONs live.
            if any(f.endswith(".mta") for f in files):
                continue
            for fn in sorted(files):
                if not fn.endswith(".spc"):
                    continue
                base = fn[:-4]
                rel = os.path.relpath(os.path.join(dirpath, base), _CORPUS)
                if os.path.exists(os.path.join(_GOLDEN, rel,
                                               base + "." + _REQUIRED)):
                    out.append(rel.replace("\\", "/"))
    return sorted(out)


CASES = _discover()

# The one corpus spec that saves e6a/e6r but not the rest of the family (its
# golden bundle carries only the force{} tables), so discovery on e1 misses it.
_FORCE_CASES = sorted(
    rel for rel in (
        os.path.relpath(os.path.join(dp, fn[:-4]), _CORPUS).replace("\\", "/")
        for tree in _TREES
        for dp, _d, fs in os.walk(os.path.join(_CORPUS, tree))
        for fn in fs if fn.endswith(".spc"))
    if os.path.exists(os.path.join(_GOLDEN, rel,
                                   os.path.basename(rel) + ".e6a")))

ALL_CASES = sorted(set(CASES) | set(_FORCE_CASES))


@functools.lru_cache(maxsize=None)
def _run(rel: str) -> dict[str, dict[str, float]]:
    spec = os.path.join(_CORPUS, rel + ".spc")
    txt = open(spec, encoding="utf-8", errors="replace").read().lower()
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.returncode == 0, f"{rel}: harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:200]
    out: dict[str, dict[str, float]] = {}
    for ln in r.stdout.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] in _TAGS:
            out.setdefault(p[0], {})[p[1]] = float(p[2])
    return out


@pytest.mark.skipif(not ALL_CASES, reason="no corpus spec ships the E-table goldens")
@pytest.mark.parametrize("rel", ALL_CASES)
@pytest.mark.parametrize("tag", _TAGS)
def test_x11_etable(rel: str, tag: str) -> None:
    base = os.path.basename(rel)
    goldpath = os.path.join(_GOLDEN, rel, base + "." + tag)
    if not os.path.exists(goldpath):
        pytest.skip(f"{base} does not ship the {tag} golden")
    spec = os.path.join(_CORPUS, rel + ".spc")
    tol = RTOL_ARITHMETIC if _is_no_model(spec) else RTOL_ESTIMATION

    produced = _run(rel).get(tag)
    assert produced, f"{rel}: harness emitted no {tag} rows"
    gold = _read_golden(goldpath)
    assert gold, f"{base}.{tag}: empty golden"

    keys = sorted(set(gold) & set(produced))
    assert len(keys) == len(gold), (
        f"{base}.{tag}: produced {len(produced)} rows, golden {len(gold)}, "
        f"overlap {len(keys)}")

    # Absolute floor for the change tables, from the golden's own scale.
    afloor = 0.0
    if tag in _CHANGE_TAGS:
        afloor = tol * max(abs(v) for v in gold.values())

    worst, worst_k = 0.0, None
    for k in keys:
        g, v = gold[k], produced[k]
        if abs(v - g) <= afloor:
            continue
        rel_err = abs(v - g) / abs(g) if g else abs(v - g)
        if rel_err > worst:
            worst, worst_k = rel_err, k
    assert worst <= tol, (
        f"{base}.{tag}: max rel err {worst:.3e} at {worst_k} (tol {tol:.0e})")
