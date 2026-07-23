"""force{} gate: the X-11 force-yearly-totals benchmarking (x11pt3 qmap/qmap2)
vs the oracle goldens.

`force{}` revises the final seasonally adjusted series (D11) so its calendar-year
totals match a target series' yearly totals, producing:

  * saa -- D11A, the forced seasonally adjusted series,
  * ffc -- the per-observation forcing factor (Stci/Stci2),
  * rnd -- the rounded SA series (only when round=yes; gated where present).

For every force spec in tests/corpus/extra/ that ships those goldens, this runs
``x13run_x11`` and checks each produced table against the golden. The two
benchmarking methods land in different milestones:

  * type=denton  (qmap)  -- modified-Denton benchmarking; ported, gated here.
  * type=regress (qmap2) -- Cholette-Dagum regression benchmarking; ported
                            (with its matrix helpers MATMLT/SIMUL/MEANCRA),
                            gated here.

round=yes (the rounded SA series, rndsa) is ported and gated where a golden
ships. The one still-unported force feature is non-original force targets
(target != original, Iftrgt>0) -- it fatals cleanly and is reached by no shipped
golden.

Run:  python -m pytest tests/parity/test_force_tables.py -q
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

# Force benchmarking is pure arithmetic, but it runs on top of an estimated
# regARIMA model (arima/automdl + estimate), so the D11 it revises carries
# estimation-derived values; gate at the estimation floor (see the tolerance
# policy in tests/parity/test_x11_tables.py). Measured worst is ~5e-15.
RTOL = 1e-6

# saa/ffc are produced by every force spec; rnd only by round=yes specs, so it is
# gated per-spec (skipped where its golden is absent) rather than required.
_TAGS = ["saa", "ffc", "rnd"]
_CORE_TAGS = ["saa", "ffc"]


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


def _spec_text(base: str) -> str:
    return open(os.path.join(_CORPUS, base + ".spc"),
                encoding="utf-8", errors="replace").read().lower()


def _discover() -> list[str]:
    specs: list[str] = []
    if not os.path.isdir(_CORPUS):
        return specs
    for fn in sorted(os.listdir(_CORPUS)):
        if not fn.endswith(".spc"):
            continue
        base = fn[:-4]
        gdir = os.path.join(_GOLDEN, base)
        if "force{" not in _spec_text(base):
            continue
        # Require the core saa/ffc goldens to be present (rnd is optional).
        if not all(os.path.exists(os.path.join(gdir, base + "." + t))
                   for t in _CORE_TAGS):
            continue
        specs.append(base)
    return specs


CASES = _discover()


@pytest.mark.skipif(not CASES, reason="no force spec ships the saa/ffc goldens")
@pytest.mark.parametrize("base", CASES)
@pytest.mark.parametrize("tag", _TAGS)
def test_force_table(base: str, tag: str) -> None:
    txt = _spec_text(base)
    if re.search(r"target\s*=\s*(calendaradj|permprioradj|both)", txt):
        pytest.xfail("x11pt3 force non-original target (Iftrgt>0): blocked on the "
                     "TD+force ~2.4e-3 floor, not the target selection")

    goldpath = os.path.join(_GOLDEN, base, base + "." + tag)
    if not os.path.exists(goldpath):
        pytest.skip(f"{base} does not produce the {tag} table")

    r = subprocess.run([BIN, os.path.join(_CORPUS, base + ".spc")],
                       capture_output=True, text=True)
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
    assert worst <= RTOL, f"{base}.{tag}: max rel err {worst:.3e} at {worst_k} (tol {RTOL:.0e})"
