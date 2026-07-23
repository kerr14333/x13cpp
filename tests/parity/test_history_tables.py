"""history{} gate: the revisions-history save tables (revchk.f/setrvp.f/
revdrv.f/getrev.f/prtrev.f) vs the oracle goldens.

`history{}` re-runs the regARIMA+X11 adjustment over expanding sub-spans of the
series -- span k ends at observation Begrev+k-1, all starting at the series
origin -- capturing, for each ending date, the *concurrent* estimate (the last
point of that span's adjustment). The *final* estimate is the same point from
the full-data span. Four save tables land per revision date:

  * sae -- concurrent + final seasonally-adjusted level (Conc_SA, Final_SA)
  * sar -- SA percent revision, (Final - Conc)/Conc * 100
  * tre -- concurrent + final trend level (Conc_TRND, Final_TRND)
  * trr -- trend percent revision

  * chr -- SA month-to-month %-change revision, (Final_chng - Conc_chng)
  * che -- concurrent + final SA %-change (Conc_chng, Final_chng)
  * tcr -- trend month-to-month %-change revision, (Final_chng - Conc_chng)
  * tce -- concurrent + final trend %-change (Conc_chng, Final_chng)
  * sfr -- seasonal-factor revision, two columns (Final-Conc, Final-Proj)
  * sfe -- concurrent + projected + final seasonal factor (Conc/Proj/Final_SF)

AICC (r07), forecast (r08) and ARMA/TD-coefficient histories are out of scope
(no goldens ship for this spec); see core/src/driver/run_history.hpp for the
full scope note.

STATUS: GATED at the estimation floor. Each span re-estimates the regARIMA
model (Revfix=F -- no fixmdl), so the concurrent/final *levels* (sae/tre) agree
to ~7e-6 relative (the same per-span re-estimation regime as the M3 estimate
gate / slidingspans, looser than a fixed-model replay because the model is
re-optimized over 70+ distinct span lengths). The *revisions* (sar/trr) are
differences of two near-equal levels, so they are gated on absolute error --
relative error is meaningless near a zero-crossing.

The port reuses the re-entrant sub-span driver (driver/run_x11_span.{hpp,cpp})
that slidingspans built. The one history-specific fix beyond that: the seasonal-
filter selector (Lterm) and Henderson trend length (Nterm) must be reset to
their parsed values per span (run_history.cpp) -- restor_span resets the
per-period Lter/Ktcopt/Tic but not those two scalars, so without the reset every
(differently-lengthed) span reused the first/shortest span's resolved filter
choice, producing a discrete ~1e-3 error jump at the 9-year span mark where the
oracle's own per-span selection diverged from the short-span choice.

Run:  python -m pytest tests/parity/test_history_tables.py -q
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

# Levels (sae/tre): each span re-estimates the model, so gate at the estimation
# floor (measured worst ~7e-6 over the airline_history spans). Revisions
# (sar/trr): absolute tolerance on the percent value -- they are (near-zero)
# differences of two levels that individually agree to the level tolerance.
RTOL_LEVEL = 1e-5
ATOL_REV = 5e-3

# (tag, n_value_columns, kind) -- kind "level" uses RTOL_LEVEL, "rev" uses ATOL_REV.
# che (month-to-month SA % change, conc+final) is a difference of two re-estimated
# levels, so it crosses zero and is gated on absolute error like the revisions.
_TABLES = [("sae", 2, "level"), ("tre", 2, "level"),
           ("sar", 1, "rev"), ("trr", 1, "rev"),
           ("chr", 1, "rev"), ("che", 2, "rev"),
           ("tcr", 1, "rev"), ("tce", 2, "rev"),
           ("sfr", 2, "rev"), ("sfe", 3, "level")]
_CORE_TAGS = ["sar", "sae", "trr", "tre"]

_NUM_RE = re.compile(r"[+\-][0-9.EeDd+\-]+")


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


def _num(s: str) -> float:
    return float(s.replace("D", "E").replace("d", "e"))


def _read_golden(path: str, ncol: int) -> dict[str, list[float]]:
    """{date: [v1, .. vncol]} from an oracle .sar/.sae/.trr/.tre save file."""
    out: dict[str, list[float]] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            parts = ln.split()
            if len(parts) < 1 + ncol or not re.match(r"^\d{6}$", parts[0]):
                continue
            vals = [v for v in parts[1:] if _NUM_RE.fullmatch(v)]
            if len(vals) < ncol:
                continue
            out[parts[0]] = [_num(v) for v in vals[:ncol]]
    return out


def _read_produced(text: str, tag: str, ncol: int) -> dict[str, list[float]]:
    """Parses tools/x13run_x11.cpp's `<tag> YYYYMM v1 .. vncol` lines."""
    out: dict[str, list[float]] = {}
    for ln in text.splitlines():
        parts = ln.split()
        if len(parts) < 2 + ncol or parts[0] != tag:
            continue
        out[parts[1]] = [float(v) for v in parts[2:2 + ncol]]
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
        if "history{" not in _spec_text(base):
            continue
        if not all(os.path.exists(os.path.join(gdir, base + "." + t))
                   for t in _CORE_TAGS):
            continue
        specs.append(base)
    return specs


CASES = _discover()


def _run(base: str) -> str:
    specpath = os.path.join(_CORPUS, base + ".spc")
    proc = subprocess.run([BIN, specpath], cwd=_CORPUS, capture_output=True,
                           text=True, timeout=180)
    assert proc.returncode == 0, (
        f"{base}: x13run_x11 exited {proc.returncode}\n"
        f"stdout tail: {proc.stdout[-2000:]}\nstderr: {proc.stderr[-2000:]}")
    return proc.stdout


@pytest.mark.skipif(not CASES, reason="no history spec ships the sar/sae/trr/tre goldens")
@pytest.mark.parametrize("base", CASES)
@pytest.mark.parametrize("tag,ncol,kind", _TABLES)
def test_history_table(base: str, tag: str, ncol: int, kind: str) -> None:
    goldpath = os.path.join(_GOLDEN, base, base + "." + tag)
    if not os.path.exists(goldpath):
        pytest.skip(f"{base} does not produce the {tag} table")

    gold = _read_golden(goldpath, ncol)
    assert gold, f"{base}.{tag}: empty golden"

    out = _run(base)
    prod = _read_produced(out, tag, ncol)
    assert prod, f"{base}.{tag}: produced no rows (history driver likely inert)"

    missing = set(gold) - set(prod)
    assert not missing, f"{base}.{tag}: missing {len(missing)} rows, e.g. {sorted(missing)[:5]}"

    worst = 0.0
    worst_key = None
    for d, gv in gold.items():
        pv = prod[d]
        for c in range(ncol):
            if kind == "level":
                err = abs(gv[c] - pv[c]) / abs(gv[c]) if gv[c] else abs(gv[c] - pv[c])
                tol = RTOL_LEVEL
            else:
                err = abs(gv[c] - pv[c])
                tol = ATOL_REV
            if err > worst:
                worst, worst_key = err, (d, c)
        assert worst <= tol, f"{base}.{tag}: worst {kind} err {worst} at {worst_key}"
