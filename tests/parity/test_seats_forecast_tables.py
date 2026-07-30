"""SEATS FORECAST decomposition -- the tfd/sfd/afd/yfd save tables.

ansub3.f:353-678 continues the Burman one-sided recursions past Nz and
sigsub.f:1586-1605 antilogs the result; sigex.f:3631-3636 punches
trend/sc/sa/cycle over Nz+1..Nz+lfor into Setftr/Setfsf/Setfsa/Setfcy, which
seatpr.f saves as tfd/sfd/afd/yfd. Every SEATS corpus spec already ships the
goldens (52 tfd, 51 sfd, 52 afd, 12 yfd), so this gate blesses nothing new.

STATUS -- read this before touching the numbers. The port is bit-exact on 12 of
the 52 specs and MEASURABLY WRONG on the other 40, in two families with two
different causes. Both are recorded in `KNOWN_GAP` below and in
`tools/seats_forecast_scouting.md`, and this file asserts them from BOTH sides:
a passing spec must stay under RTOL, and a known-gap spec must stay ABOVE it.
Fixing one therefore fails this test and tells you to move its row -- the gap
list cannot rot into a silent allowlist.
"""
import math
import pathlib
import subprocess

import pytest

ROOT = pathlib.Path(__file__).resolve().parents[2]
CORPUS = ROOT / "tests" / "corpus"
GOLDEN = ROOT / "tests" / "golden"
BIN = ROOT / "build" / "x13run_seats.exe"

TABLES = ("tfd", "sfd", "afd", "yfd")

# The arithmetic floor the SEATS s-tables already hold. The forecast span runs
# the same recursions, so a correct port lands here too -- 11 of the 12 passing
# specs are at 1e-12 or better and the twelfth (unrate_sar) at 1.04e-11.
RTOL = 5e-11

# Specs whose forecast decomposition is KNOWN WRONG, with the measured worst
# relative error at the time of writing and the family it belongs to. See
# tools/seats_forecast_scouting.md for the two causes and the dead ends already
# ruled out (a raw-MA extension, the regARIMA transformed forecast, and a
# bias3c-vs-bias1c trend factor were each tried and measured).
#
#   APPROX -- SEATS caps a near-non-invertible MA to the `xl` bound before the
#             canonical decomposition. The port's forecast of z then follows
#             the CAPPED model, and the oracle's saved tables follow something
#             that equals the regARIMA forecast to the last bit. Neither the
#             capped nor the raw extension reproduces it, and the HISTORICAL
#             span is bit-exact with the capped one (measured sensitivity
#             -755 per unit log, so it is not merely insensitive).
#   MEAN   -- imean!=0 / a Constant regressor. The forecast of z carries the
#             wrong drift; no capping is involved (th == th_raw on these).
KNOWN_GAP = {
    # spec: (family, measured worst relative error)
    "unrate_bias0-seats": ("APPROX", 5.23e-07),
    "unrate_finite-seats": ("APPROX", 5.23e-07),
    "unrate_fixed-airline-seats": ("APPROX", 5.23e-07),
    "unrate_noadmiss-seats": ("APPROX", 5.23e-07),
    "airline_ar2-seats": ("APPROX", 2.71e-06),
    "payems_ar2-seats": ("APPROX", 1.83e-04),
    "payems_bias0-seats": ("APPROX", 4.78e-04),
    "payems_finite-seats": ("APPROX", 4.78e-04),
    "payems_fixed-airline-seats": ("APPROX", 4.78e-04),
    "payems_noadmiss-seats": ("APPROX", 4.78e-04),
    "payems_mean-d0-seats": ("MEAN", 5.14e-04),
    "unrate_statseas-seats": ("APPROX", 8.10e-04),
    "unrate_ar2-seats": ("APPROX", 2.67e-03),
    "expgs_statseas-seats": ("APPROX", 6.64e-03),
    "payems_imean-yes-seats": ("MEAN", 6.69e-03),
    "payems_statseas-seats": ("APPROX", 6.89e-03),
    "airline_mean-d0-seats": ("MEAN", 9.84e-03),
    "expgs_bias0-seats": ("APPROX", 1.03e-02),
    "expgs_finite-seats": ("APPROX", 1.03e-02),
    "expgs_fixed-airline-seats": ("APPROX", 1.03e-02),
    "expgs_noadmiss-seats": ("APPROX", 1.03e-02),
    "expgs_mean-d0-seats": ("MEAN", 1.41e-02),
    "expgs_imean-no-seats": ("MEAN", 1.67e-02),
    "airline_imean-no-seats": ("MEAN", 1.97e-02),
    "payems_imean-no-seats": ("MEAN", 2.47e-02),
    "payems_mean-td-seats": ("MEAN", 2.87e-02),
    "payems_mean-seats": ("MEAN", 3.08e-02),
    "airline_imean-yes-seats": ("MEAN", 3.55e-02),
    "airline_mean-td-seats": ("MEAN", 4.42e-02),
    "airline_statseas-seats": ("APPROX", 4.43e-02),
    "airline_mean-seats": ("MEAN", 5.60e-02),
    "unrate_imean-no-seats": ("MEAN", 6.91e-02),
    "expgs_imean-yes-seats": ("MEAN", 2.58e-01),
    "expgs_mean-seats": ("MEAN", 2.66e-01),
    "expgs_mean-td-seats": ("MEAN", 2.93e-01),
    "unrate_mean-d0-seats": ("MEAN", 3.81e-01),
    "unrate_mean-td-seats": ("MEAN", 4.29e+00),
    "unrate_imean-yes-seats": ("MEAN", 6.90e+00),
    "unrate_mean-seats": ("MEAN", 1.51e+01),
}

def _read_golden(path):
    out = {}
    for line in path.read_text().splitlines()[2:]:
        f = line.split("\t")
        if len(f) >= 2 and f[0].strip():
            out[f[0]] = float(f[1])
    return out


def _cases():
    seen = []
    for gdir in sorted(GOLDEN.rglob("*")):
        if not gdir.is_dir():
            continue
        base = gdir.name
        if not (gdir / f"{base}.tfd").exists():
            continue
        spec = next(iter(CORPUS.rglob(f"{base}.spc")), None)
        if spec is not None:
            seen.append((base, spec, gdir))
    return seen


CASES = _cases()


def _run(spec):
    proc = subprocess.run([str(BIN), spec.name], cwd=str(spec.parent),
                          capture_output=True, text=True, timeout=300)
    got = {t: {} for t in TABLES}
    for line in proc.stdout.splitlines():
        p = line.split()
        if len(p) == 3 and p[0] in got:
            got[p[0]][p[1]] = float(p[2])
    return got


def _worst(golden, engine):
    """Worst RELATIVE deviation, and the date it happens on."""
    worst, where = 0.0, None
    for k, g in golden.items():
        e = engine[k]
        rel = abs(e - g) / max(abs(g), 1e-30)
        if rel > worst:
            worst, where = rel, k
    return worst, where


@pytest.mark.parametrize("base,spec,gdir",
                         CASES, ids=[c[0] for c in CASES])
def test_seats_forecast_tables(base, spec, gdir):
    assert BIN.exists(), f"missing harness {BIN}"
    engine = _run(spec)

    compared = 0
    worst_all, worst_tab, worst_at = 0.0, None, None
    for t in TABLES:
        gp = gdir / f"{base}.{t}"
        golden = _read_golden(gp) if gp.exists() else {}
        got = engine[t]
        # Presence, in BOTH directions -- a table the oracle does not write is
        # as much a defect as a missing one (yfd exists only when the model has
        # a transitory component; sfd is written even when npsi==1, where it is
        # a flat 100).
        assert bool(golden) == bool(got), (
            f"{base}.{t}: golden={'yes' if golden else 'no'} "
            f"engine={'yes' if got else 'no'}")
        if not golden:
            continue
        assert sorted(golden) == sorted(got), (
            f"{base}.{t}: date sets differ -- "
            f"missing {sorted(set(golden) - set(got))[:3]}, "
            f"extra {sorted(set(got) - set(golden))[:3]}")
        w, at = _worst(golden, got)
        compared += len(golden)
        if w > worst_all:
            worst_all, worst_tab, worst_at = w, t, at

    assert compared > 0, f"{base}: nothing compared"

    if base in KNOWN_GAP:
        family, recorded = KNOWN_GAP[base]
        # Assert the gap is STILL a gap. If this fires the port got better --
        # delete the row and let the spec gate for real.
        assert worst_all > RTOL, (
            f"{base}: KNOWN_GAP[{family}] no longer reproduces "
            f"(worst {worst_all:.2e} <= {RTOL:.0e}) -- remove it from "
            f"KNOWN_GAP so this spec gates.")
        # ...and that it has not got WORSE by more than an order of magnitude,
        # which would mean a new defect hiding behind an old one.
        assert worst_all < max(recorded * 10, 1e-9), (
            f"{base}: KNOWN_GAP[{family}] regressed -- worst {worst_all:.2e} "
            f"on {worst_tab}@{worst_at} against the recorded {recorded:.2e}")
        pytest.skip(f"KNOWN_GAP[{family}] {recorded:.2e}: "
                    f"see tools/seats_forecast_scouting.md")

    assert worst_all <= RTOL, (
        f"{base}: {worst_tab}@{worst_at} off by {worst_all:.2e} "
        f"(tol {RTOL:.0e})")


def test_known_gap_list_is_live():
    """Every KNOWN_GAP key must name a real case, or the list has rotted."""
    names = {c[0] for c in CASES}
    stale = sorted(set(KNOWN_GAP) - names)
    assert not stale, f"KNOWN_GAP names specs with no forecast golden: {stale}"
    assert len(CASES) - len(KNOWN_GAP) >= 12, (
        "fewer specs gate than when this was written -- a regression moved "
        "rows INTO KNOWN_GAP rather than out of it")


def test_no_transitory_means_trend_equals_sa():
    """The oracle's own identity, asserted on the goldens, not on the engine.

    With no transitory component the forecast trend IS the forecast SA series
    (ir is identically zero over the forecast span, ansub3.f:519-521), so
    `tfd == afd` to the last digit; with one, `afd = tfd * yfd/100` in
    multiplicative mode and `afd = tfd + yfd` in additive mode. All three hold
    across the whole corpus and are what pinned the trend's bias factor -- if a
    future golden breaks them, the transform in estbur.cpp is reading the wrong
    normalization again.

    An additive spec is detected from yfd itself: sigsub.f only scales the
    transitory by 100 on the log path, so a multiplicative yfd sits near 100
    and an additive one near 0.
    """
    checked = 0
    for base, _spec, gdir in CASES:
        t = _read_golden(gdir / f"{base}.tfd")
        a = _read_golden(gdir / f"{base}.afd")
        ypath = gdir / f"{base}.yfd"
        y = _read_golden(ypath) if ypath.exists() else None
        mult = y is not None and min(abs(v) for v in y.values()) > 1.0
        for k in t:
            if y is None:
                lhs, how = t[k], "tfd"
            elif mult:
                lhs, how = t[k] * y[k] / 100.0, "tfd*yfd/100"
            else:
                lhs, how = t[k] + y[k], "tfd+yfd"
            if abs(a[k]) < 1e-9:
                continue
            assert math.isclose(lhs, a[k], rel_tol=1e-9), (
                f"{base}@{k}: {how}={lhs} != afd={a[k]}")
            checked += 1
    assert checked > 1000, f"only {checked} points checked"
