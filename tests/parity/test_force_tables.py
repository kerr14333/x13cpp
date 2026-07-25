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
ships. All four force targets are ported and gated (x11pt3.f:715-722): original
(Series), calendaradj (Stocal, Iftrgt=1), permprioradj (Stopp, Iftrgt=2) and
both (Stopp/Faccal, Iftrgt=3).

`airline_force-constant-{regress,denton}` gate the negative-value correction
(x11pt3.f:750-782), which is reachable only with `transform{constant=}` in a
non-additive mode: subtracting the constant back out can drive D11 to or below
zero, so the forced series is clamped and then re-prorated against the target by
a SECOND qmap2 pass (with Rol=0/Lamda=0.5) -- run even when the primary pass was
Denton. Their series is airline shifted down by 150 so 24 observations cross
zero, and `constant=100` lifts it back. They are also the only specs that reach
the `ffc` DNOTST branch (x11pt3.f:841-850): where the corrected series is STILL
<= 0 the forcing factor is not formed at all and the golden carries -999.

The `airline_force-*` specs carrying a regARIMA trading-day regressor are the
load-bearing ones: force's qmap sums the target-vs-SA discrepancy over the
FORECAST year as well as the observed span, so they are the only gate that sees
the forecast tail of Series/Sprior/Factd. Two real bugs lived there behind an
`OUTCOME: OK` -- the prior-factor series was built only to Nobspf instead of
adjsrs.f's Nadj (leaving Sprior==0 over the forecast span, which zeroed
Factd/Faccal and made D11 infinite there), and Kfmt was never set on the model
path (so adjreg never folded the prior back into the forecast tail of Series).
Neither is visible in d10-d13, which print over the observed span only.

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

# saa/ffc are produced by every force spec; rnd (round=yes) and the d10-d13
# D-tables only by the specs that ask for them, so those are gated per-spec
# (skipped where the golden is absent) rather than required.
_TAGS = ["saa", "ffc", "rnd", "d10", "d11", "d12", "d13", "d16"]
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
