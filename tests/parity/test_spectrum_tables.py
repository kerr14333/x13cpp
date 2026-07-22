"""M5 spectrum{} gate: the C++ periodogram tables (x13run_x11) vs the oracle
spectral goldens.

spectrum{} saves the data-based spectral estimates sp0/sp1/sp2 (and spr + the
Tukey st0/st1/st2). This slice gates the periodogram type's three save tables:

  * sp0 -- 10*Log(Spectrum_AdjOri): detrended original (Stcsi + extremes),
  * sp1 -- 10*Log(Spectrum_SA):     detrended E2 (SA modified for extremes),
  * sp2 -- 10*Log(Spectrum_Irr):    E3 (modified irregular).

Ported from spcdrv.f (periodogram path): the mkfreq.f frequency grid, the
gendff.f log+difference detrend, and the spgrh2.f periodogram. NOTE the SA and
irregular series are E2/E3 (x11pt3 Part E), not the D11/D13 saves -- x11pt4 runs
before spcdrv, so spcdrv's Stci/Sti hold the modified E-tables.

Gated for both types: periodogram (spgrh2) and arspec (spgrh/sautco/sicp2 AR
spectrum), all seven tables (sp0/sp1/sp2/spr + Tukey st0/st1/st2) each.

Run:  python -m pytest tests/parity/test_spectrum_tables.py -q
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

# The periodogram spectrum is a coherent-sum arithmetic pipeline over the
# already-bit-exact X-11 series; it reaches the estimation floor of the inputs.
# Measured worst-case is ~5e-14 dB across all three tables, so the universal 1e-8
# contract gate holds with six orders of margin (even the deepest cancellation
# troughs stay far inside it).
ABS_TOL = 1e-8

_TAGS = ["sp0", "sp1", "sp2", "spr", "st0", "st1", "st2"]


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

# Golden line: "<pos>\t<freq>\t<value>" (savspp.f /rdb format).
_GOLD_RE = re.compile(r"\s*(\d+)\t([-+0-9.EeDd]+)\t([-+0-9.EeDd]+)")


def _read_golden(path: str) -> dict[int, float]:
    out: dict[int, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _GOLD_RE.match(ln)
            if m:
                out[int(m.group(1))] = float(m.group(3).replace("D", "E").replace("d", "e"))
    return out


def _discover() -> list[str]:
    specs: list[str] = []
    if not os.path.isdir(_CORPUS):
        return specs
    for fn in sorted(os.listdir(_CORPUS)):
        if not fn.endswith(".spc"):
            continue
        base = fn[:-4]
        gdir = os.path.join(_GOLDEN, base)
        if not all(os.path.exists(os.path.join(gdir, base + "." + t)) for t in _TAGS):
            continue
        specs.append(base)
    return specs


CASES = _discover()


@pytest.mark.skipif(not CASES, reason="no spectrum{} spec ships the sp0/sp1/sp2 goldens")
@pytest.mark.parametrize("base", CASES)
@pytest.mark.parametrize("tag", _TAGS)
def test_spectrum_table(base: str, tag: str) -> None:
    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.returncode == 0, f"{base}: harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:200]

    produced: dict[int, float] = {}
    for ln in r.stdout.splitlines():
        p = ln.split()
        if len(p) == 4 and p[0] == tag:
            produced[int(p[1])] = float(p[3])

    gold = _read_golden(os.path.join(_GOLDEN, base, base + "." + tag))
    assert gold, f"{base}.{tag}: empty golden"

    keys = sorted(set(gold) & set(produced))
    assert len(keys) == len(gold), (
        f"{base}.{tag}: produced {len(produced)} rows, golden {len(gold)}, "
        f"overlap {len(keys)}")

    worst = 0.0
    worst_k = None
    for k in keys:
        d = abs(produced[k] - gold[k])
        if d > worst:
            worst, worst_k = d, k
    assert worst <= ABS_TOL, (
        f"{base}.{tag}: max abs err {worst:.3e} dB at pos {worst_k} (tol {ABS_TOL:.0e})")
