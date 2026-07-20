"""M5 X-11 gate: the C++ X-11 decomposition spine (x13run_x11) vs the oracle
goldens.

This slice assembles the classic X-11 decomposition (x11pt1 -> x11pt2) on the
no-model direct-X11 path and produces:

  * b1 -- the prior-adjusted B1 input series, and
  * d7 -- the final X-11 trend-cycle at the D7 return point of x11pt2.

For every corpus spec whose golden bundle contains BOTH a ``<base>.b1`` and a
``<base>.d7`` save file and that runs on the no-model path (series{} + x11{}
only), this test runs ``x13run_x11`` and checks each produced table matches the
golden numerically at rtol 1e-8, period-key exact. The D-pass finals
(d10/d11/d12/d13, from x11pt3) gate in a later slice.

Run:  python -m pytest tests/parity/test_x11_tables.py -q
"""
from __future__ import annotations

import os
import re
import subprocess
import sys

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus", "generated")
_GOLDEN = os.path.join(_REPO, "tests", "golden", "generated")

RTOL = 1e-8


def _find_binary() -> str:
    cands = [
        os.path.join(_REPO, "build", "x13run_x11.exe"),
        os.path.join(_REPO, "build", "x13run_x11"),
        os.path.join(_REPO, "build", "Release", "x13run_x11.exe"),
    ]
    for c in cands:
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
                out[m.group(1)] = float(m.group(2).replace("D", "E").replace("d", "e"))
    return out


def _is_no_model(spec_path: str) -> bool:
    """No-model direct-X11 spec: only series{} and x11{} present."""
    txt = open(spec_path, encoding="utf-8", errors="replace").read().lower()
    modeling = ("arima{", "automdl{", "estimate{", "regression{", "outlier{",
                "forecast{", "seats{", "pickmdl{", "check{")
    return "x11{" in txt and not any(k in txt for k in modeling)


def _discover() -> list[str]:
    specs: list[str] = []
    if not os.path.isdir(_CORPUS):
        return specs
    for fn in sorted(os.listdir(_CORPUS)):
        if not fn.endswith(".spc"):
            continue
        base = fn[:-4]
        gdir = os.path.join(_GOLDEN, base)
        if not (os.path.exists(os.path.join(gdir, base + ".b1"))
                and os.path.exists(os.path.join(gdir, base + ".d7"))):
            continue
        if _is_no_model(os.path.join(_CORPUS, fn)):
            specs.append(base)
    return specs


CASES = _discover()


@pytest.mark.skipif(not CASES, reason="no no-model x11 spec ships both .b1 and .d7 goldens")
@pytest.mark.parametrize("base", CASES)
@pytest.mark.parametrize("tag", ["b1", "d7"])
def test_x11_table(base: str, tag: str) -> None:
    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.returncode == 0, f"{base}: harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:200]

    produced: dict[str, float] = {}
    for ln in r.stdout.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == tag:
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
    assert worst <= RTOL, f"{base}.{tag}: max rel err {worst:.3e} at {worst_k}"
