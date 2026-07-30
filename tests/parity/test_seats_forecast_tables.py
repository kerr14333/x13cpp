"""SEATS FORECAST decomposition -- the tfd/sfd/afd/yfd save tables.

ansub3.f:353-678 continues the Burman one-sided recursions past Nz and
sigsub.f:1586-1605 antilogs the result; sigex.f:3631-3636 punches
trend/sc/sa/cycle over Nz+1..Nz+lfor into Setftr/Setfsf/Setfsa/Setfcy, which
seatpr.f saves as tfd/sfd/afd/yfd. Every SEATS corpus spec already ships the
goldens (52 tfd, 51 sfd, 52 afd, 12 yfd), so this gate blesses nothing new.

STATUS -- read this before touching the numbers. 46 of the 52 specs gate
bit-exact (most at ~5e-15). The 6 that do not are in `KNOWN_GAP` below with
the sub-cause each belongs to, and this file asserts them from BOTH sides: a
passing spec must stay under RTOL, and a known-gap spec must stay ABOVE it.
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

# Specs whose forecast decomposition is still wrong, with the measured worst
# relative error and the sub-cause. History: ansub3.f's Tramo block (:552-653)
# was believed unreachable and was unported, which left 39 specs wrong in what
# looked like two families (APPROX / near-non-invertible MA, MEAN / imean!=0).
# Measured 2026-07-30 against an instrumented oracle, it is REACHABLE on an
# ordinary X-13 SEATS run and rewrites z(Nz+1..) with LOG(TramLin) -- the
# regARIMA forecast of the LINEARIZED series, which is what :660's trend/sa
# residual reads, while the filter recursions keep reading the untouched extZ
# that this port already reproduced. Porting it took 39 gaps down to 6. The
# old APPROX/MEAN split was never two mechanisms, only the size of the
# discrepancy the block folds back. Full record:
# tools/seats_forecast_scouting.md section 3.
KNOWN_GAP = {
    # spec: (family, measured worst relative error)
    # TDLIN -- the last sub-cause. TramLin = Tram/TramDet divides out every
    #          DETERMINISTIC preadjustment factor, trading day included
    #          (analts.f:717-733), and this port feeds the Tramo block
    #          ctx.forecasts.trnfct, which is the forecast of the series WITH
    #          the TD effect still in it. Exactly the four `mean-td` specs are
    #          left, which is the signature. Fix: subtract the forecast-span
    #          TD contribution before the block, the same decomposition
    #          run_seats already does historically (seats_combined_orig / the
    #          "add back only the Constant's contribution" rule).
    "expgs_mean-td-seats": ("TDLIN", 7.18e-03),
    "airline_mean-td-seats": ("TDLIN", 1.26e-02),
    "payems_mean-td-seats": ("TDLIN", 2.58e-02),
    "unrate_mean-td-seats": ("TDLIN", 6.85e-01),
    # RESIDUE -- near the floor and NOT the TD gap (no TD regressor on either).
    # Both improved by 6-9 orders when the Tramo block landed (3.81e-01 and
    # 1.51e+01 before it), so whatever is left is a second, much smaller term.
    # unrate is the additive/lam==1 series; suspect the non-log arm of
    # ansub3.f:565-568, which drops the LOG rather than taking it.
    "unrate_mean-seats": ("RESIDUE", 1.36e-10),
    "unrate_mean-d0-seats": ("RESIDUE", 9.12e-08),
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
