"""SEATS FORECAST decomposition -- the tfd/sfd/afd/yfd save tables.

ansub3.f:353-678 continues the Burman one-sided recursions past Nz,
sigsub.f:1586-1605 antilogs the result, and ansub4.f:3144-3210 (log) /
:2284-2337 (non-log) refolds the deterministic preadjustment factors back on
before punching trend/sc/sa/cycle into Setftr/Setfsf/Setfsa/Setfcy, which
seatpr.f saves as tfd/sfd/afd/yfd. Every SEATS corpus spec already ships the
goldens (52 tfd, 51 sfd, 52 afd, 12 yfd), so this gate blesses nothing new.

NOT sigex.f:3631-3636. Those USRENTRY 1409/1410/1411/1413 calls are inside
`if (Tramo .le. 0)` and are DEAD on an X-13 run (Tramo == 1); probing all five
call sites shows only ansub4.f's fire. That mis-identification is what left
the last six gaps: this port punched the antilogged components straight, with
no refold, and separately fed ansub3.f's Tramo block the transformed-scale
regARIMA forecast instead of `TramLin = Tram/TramDet`. The two errors cancel
except for the length-of-month/leap prior, so 46 of 52 specs passed anyway.

STATUS -- read this before touching the numbers. All 52 gate bit-exact and
`KNOWN_GAP` is empty. It is deliberately kept: this file asserts from BOTH
sides, so a spec listed there must stay ABOVE RTOL, and the gap list cannot
rot into a silent allowlist.
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
# the same recursions, so a correct port lands here too -- 48 of the 52 specs
# are at 1e-12 or better and the worst is payems_mean-td at 1.01e-11, which
# runs the ansub4.f refold through a trading-day factor series and so carries
# one extra rounding.
RTOL = 5e-11

# EMPTY, and kept that way on purpose -- the both-directions assertion below
# means a row here must stay ABOVE RTOL, so a stale entry fails loudly instead
# of quietly excusing a spec.
#
# History, because it is the shape of the defect rather than the arithmetic
# that is worth keeping. ansub3.f's Tramo block (:552-653) was believed
# unreachable and was unported, which left 39 specs wrong in what looked like
# two families. Measured against an instrumented oracle it is REACHABLE on an
# ordinary X-13 run (Tramo == 1) and rewrites z(Nz+1..) with LOG(TramLin);
# porting it took 39 gaps to 6. The last 6 were a SECOND unreached-code
# mistake in the same front: the saved tables do not come from
# sigex.f:3631-3636 (dead under Tramo == 1) but from ansub4.f, which refolds
# the deterministic factors. Feeding the block `ctx.forecasts.trnfct` instead
# of Tram/TramDet is wrong by exactly TramDet, and the missing refold is wrong
# by exactly TramDet the other way, so the two cancelled everywhere except the
# length-of-month/leap prior that only trnfct carries -- which is why the gap
# was 4 `mean-td` specs plus 2 near the floor and looked like two unrelated
# causes. Full record: tools/seats_forecast_scouting.md section 3.
KNOWN_GAP = {
    # spec: (family, measured worst relative error)
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
    """Worst deviation relative to the TABLE'S OWN SCALE, and where.

    Not per-value relative. On the non-log path sfd and yfd are additive
    DIFFERENCES that cross zero -- unrate_mean-d0's yfd runs down to 1.4e-08 --
    so dividing by |g| measures cancellation rather than accuracy: an absolute
    agreement of 1.3e-15 there reads as 9.1e-08 "relative". Normalising by
    max|g| over the table is the same yardstick ansub3.f:653-663 uses on the
    trend itself (`abs(trend) < 1e-15 * maxZ -> 0`), and it does not loosen the
    multiplicative tables, whose values are all within an order of magnitude of
    their own max.
    """
    scale = max((abs(g) for g in golden.values()), default=0.0)
    if scale <= 0.0:
        scale = 1e-30
    worst, where = 0.0, None
    for k, g in golden.items():
        rel = abs(engine[k] - g) / scale
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
