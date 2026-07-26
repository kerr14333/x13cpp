"""SEATS gate (scoping pass): the C++ SEATS decomposition (x13run_seats) vs
the oracle s-table goldens.

SEATS ("ARIMA-model-based" seasonal adjustment) decomposes the fitted
regARIMA model into trend/seasonal/SA/transitory/irregular components via
Wiener-Kolmogorov filtering (Burman's algorithm). The orchestrating driver
(SIGEX/ESTBUR/AUTOCOMP -- see tools/seats_scope.md) is now ported: **every
SEATS corpus spec's s10-s18 tables gate bit-exact** (added to `_gated` below),
including the near-non-invertible fixed-airline variants (payems_fixed /
expgs_fixed), whose forecast-extension seed residuals are computed by a
faithful CALCFX port (estbur.cpp `calcfx_last_residuals`).

The test still uses per-tag gating: a `(base, tag)` NOT in `_gated` short-
circuits via `pytest.xfail` (this remains for any future SEATS spec/table not
yet landed). The harness-subprocess + golden-diff plumbing (rtol=1e-8, plus an
ATOL=1e-9 floor for zero-crossing tables) mirrors test_x11_tables.py's shape.

Run:  python -m pytest tests/parity/test_seats_tables.py -q
"""
from __future__ import annotations

import os
import re
import subprocess

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus", "generated")
_GOLDEN = os.path.join(_REPO, "tests", "golden", "generated")

RTOL = 1e-8
# Absolute-tolerance floor (numpy-allclose style): a point passes if
# |v-g| <= RTOL*|g| + ATOL. Needed for tables whose values legitimately cross
# zero (the additive seasonal component s10/s16, the irregular s13), where a
# pure relative error explodes near the crossing even though the absolute error
# is double-precision noise (~1e-12). ATOL=1e-9 sits far above that noise and
# far below any real decomposition error (which run ~1e-3+), so it only ever
# forgives zero-crossings, never masks a genuine discrepancy.
ATOL = 1e-9

# The 8 SEATS corpus specs (tools/seats_scope.md section 4 / the task's gate
# list): the 4 base series x {default seats, fixed-airline-seats}.
_TAGS = ["s10", "s11", "s12", "s13", "s16", "s18"]


def _find_binary() -> str:
    cands = [
        os.path.join(_REPO, "build", "x13run_seats.exe"),
        os.path.join(_REPO, "build", "x13run_seats"),
        os.path.join(_REPO, "build", "Release", "x13run_seats.exe"),
    ]
    for c in cands:
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_SEATS")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13run_seats binary not found; build it first (cmake --build build "
        "--target x13run_seats).")


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


def _discover() -> list[str]:
    specs: list[str] = []
    if not os.path.isdir(_CORPUS):
        return specs
    for fn in sorted(os.listdir(_CORPUS)):
        if not fn.endswith(".spc"):
            continue
        base = fn[:-4]
        if not base.endswith("seats"):
            continue
        gdir = os.path.join(_GOLDEN, base)
        # Require at least one gated table's golden to exist -- some tags
        # (s18) are shipped but sometimes empty; s10-s13 always ship for the
        # SEATS corpus.
        if not os.path.isdir(gdir):
            continue
        specs.append(base)
    return specs


CASES = _discover()


@pytest.mark.skipif(not CASES, reason="no seats{} spec ships a golden bundle")
@pytest.mark.parametrize("base", CASES)
@pytest.mark.parametrize("tag", _TAGS)
def test_seats_table(base: str, tag: str) -> None:
    # SEATS decomposition (SIGEX/ESTBUR/AUTOCOMP) is bit-exact across the
    # airline family AND the general shape (p>0 ar2-seats / bp>0 sar-seats);
    # only imean!=0 remains unported (guarded-fatal). Historical trail below
    # for the session-by-session close -- see tools/seats_scope.md. Session 12
    # landed the
    # historical-span ESTBUR general-branch solve for the additive/no-
    # seasonal/no-cycle case (unrate_seats): s11 (SA)/s12 (trend) gate
    # bit-exact. Session 13 added the log back-transform and s10/s16/s18
    # (seasonal factor, z/sa), gating payems_seats (log-transform, real
    # cycle) as the second series. Session 14 CLOSED unrate_seats' s13
    # (irregular): the ~2.78e-8-at-2-boundary-dates gap (session 12/13) was
    # ground-truthed against the oracle directly (temporary analts.f
    # instrumentation, reverted) and found to be `estbur.cpp`'s
    # `armafl_last_residuals` applying an EXTRA `fac=exp(lndtcv/2/dnefob)`
    # scaling that CALCFX's own round trip (STEP9 `a(i)=Detpri*a(i)`,
    # analts.f's caller `a(i)=a(i)/Detpri`) cancels back out before ever
    # handing residuals to FCAST -- `Detpri` turned out to be EXACTLY
    # `fac`, bit-for-bit, so the fed-to-FCAST residual is the RAW
    # (unscaled) armafl() value, not the `fac`-scaled one. Dropping the
    # scaling closes unrate_seats' s12/s13 to double-precision noise
    # (worst ~4.8e-15 across all 776 dates) -- now gated. A second,
    # independent bug found+fixed the same session: FCAST's own end-
    # extension recursion needs the trailing `Qstar_f` historical residuals
    # as seeds (not just the very last one) whenever `Qstar_f=q+bq*mq>=2`;
    # `unrate_seats` (`Qstar_f=1`) never exercised the gap, but
    # `payems_seats` (`Qstar_f=2`) was silently treating `a(Na-1)` as 0.
    # Fixing this (`armafl_last_residuals` now returns a trailing vector,
    # `fcast_extend` seeds all of it) collapsed payems' s12/s13 error from a
    # ~2.1e-4 spike at the first point down to a UNIFORM ~2.2e-6 floor
    # across the whole 308-point series. That floor was NOT a residual gap:
    # it was the SEATS multiplicative bias correction (sigsub.f:1539-1585,
    # bias==1 default per ansub9.f:1118) missing from the log back-transform.
    # For payems (npsi==1) it reduces to log(mean(exp(ir))) applied trend-
    # only == +2.1978124351562656e-06. Porting bias1c/bias2c/bias3c into
    # estbur.cpp's is_log block closed payems' s12/s13 to double-precision
    # noise -- now gated. s10 doesn't ship for unrate_seats (npsi==1, no
    # seasonal component). The general (non-airline) decomposition branch IS
    # ported and bit-exact wherever the oracle ships a golden -- airline_seats,
    # every *-fixed-airline-seats variant, and unrate_seats s11/s12/s13/s18 all
    # gate below. The only remaining (base, tag) pairs without a golden are the
    # inadmissible-decomposition spec (expgs_seats: negative irregular spectrum
    # -> oracle aborts, writes no s-tables) and the no-seasonal tables
    # (unrate_seats s10/s16); those SKIP above (no parity target), they are not
    # an unported orchestrator. The engine now FATALS on expgs_seats instead of
    # emitting approximated s-tables -- seats_decomp_unported_reason
    # (seatopts.cpp) applies the oracle's own admissibility verdict
    # (spectrum.f:389/446-547 qt1<0, spectrum.f:1709-1737 cycle varwnc<0), which
    # with the default seats{noadmiss=no} is exactly "abort SEATS, write no
    # s-tables". Silent wrongness -> deliberate fatal; still no table golden, so
    # still a skip here.
    _gated = {
        ("unrate_seats", "s11"), ("unrate_seats", "s12"),
        ("unrate_seats", "s13"), ("unrate_seats", "s18"),
        ("payems_seats", "s10"), ("payems_seats", "s11"),
        ("payems_seats", "s12"), ("payems_seats", "s13"),
        ("payems_seats", "s16"), ("payems_seats", "s18"),
        ("airline_seats", "s10"), ("airline_seats", "s11"),
        ("airline_seats", "s12"), ("airline_seats", "s13"),
        ("airline_seats", "s16"), ("airline_seats", "s18"),
        # airline_fixed-airline-seats: same (0 1 1)(0 1 1) model, fixed (no
        # automdl) -- all 6 tables bit-exact.
        ("airline_fixed-airline-seats", "s10"),
        ("airline_fixed-airline-seats", "s11"),
        ("airline_fixed-airline-seats", "s12"),
        ("airline_fixed-airline-seats", "s13"),
        ("airline_fixed-airline-seats", "s16"),
        ("airline_fixed-airline-seats", "s18"),
        # unrate_fixed-airline-seats: additive (no transform) + real seasonal.
        # SA (s11)/trend (s12)/seasonal-factor (s18=z/sa ratio) bit-exact, and
        # s10/s16 (z-sa additive difference) now that both conventions are
        # wired. All 6 bit-exact in absolute terms (~4e-12); s10/s13/s16 rely
        # on the ATOL floor at their zero crossings.
        ("unrate_fixed-airline-seats", "s10"),
        ("unrate_fixed-airline-seats", "s11"),
        ("unrate_fixed-airline-seats", "s12"),
        ("unrate_fixed-airline-seats", "s13"),
        ("unrate_fixed-airline-seats", "s16"),
        ("unrate_fixed-airline-seats", "s18"),
        # payems_fixed / expgs_fixed-airline-seats: log-transform airline with a
        # near-NON-invertible seasonal MA (bth ~ -0.978, capped to -0.99). The
        # SEATS forecast-extension seed residuals are computed by a faithful
        # CALCFX port (estbur.cpp calcfx_last_residuals) on the CAPPED canonical
        # model -- armafl's exact-ML/GLS residuals diverged ~4.5%/~7e-4 at the
        # tail for this near-boundary MA. All 6 tables bit-exact (~5e-15).
        ("payems_fixed-airline-seats", "s10"),
        ("payems_fixed-airline-seats", "s11"),
        ("payems_fixed-airline-seats", "s12"),
        ("payems_fixed-airline-seats", "s13"),
        ("payems_fixed-airline-seats", "s16"),
        ("payems_fixed-airline-seats", "s18"),
        ("expgs_fixed-airline-seats", "s10"),
        ("expgs_fixed-airline-seats", "s11"),
        ("expgs_fixed-airline-seats", "s12"),
        ("expgs_fixed-airline-seats", "s13"),
        ("expgs_fixed-airline-seats", "s16"),
        ("expgs_fixed-airline-seats", "s18"),
        # *_ar2-seats: explicit nonseasonal-AR(2) model -- the first
        # GENERAL-SHAPE (p>0) SEATS decomposition to gate, beyond the airline
        # family. The AR roots land in the canonical TRANSITORY denominator
        # (oracle "STATIONARY AUTOREGRESSIVE TRANSITORY COMPONENT"); the only
        # port bug was the AR-polynomial sign in canonical_denoms.cpp (phis =
        # +mo.phi, mirroring the MA-side session-5 correction). All s10-s18
        # bit-exact (~5e-15) on airline/payems/unrate; expgs (quarterly) AR(2)
        # is inadmissible (negative irregular spectrum) -> no golden -> skips.
        ("airline_ar2-seats", "s10"), ("airline_ar2-seats", "s11"),
        ("airline_ar2-seats", "s12"), ("airline_ar2-seats", "s13"),
        ("airline_ar2-seats", "s16"), ("airline_ar2-seats", "s18"),
        ("payems_ar2-seats", "s10"), ("payems_ar2-seats", "s11"),
        ("payems_ar2-seats", "s12"), ("payems_ar2-seats", "s13"),
        ("payems_ar2-seats", "s16"), ("payems_ar2-seats", "s18"),
        ("unrate_ar2-seats", "s10"), ("unrate_ar2-seats", "s11"),
        ("unrate_ar2-seats", "s12"), ("unrate_ar2-seats", "s13"),
        ("unrate_ar2-seats", "s16"), ("unrate_ar2-seats", "s18"),
        # *_sar-seats: seasonal-AR model (0 1 1)(1 1 0) -- the Bp>0 branch. Same
        # sign-fix class as ar2 (bphis = +mo.bphi in canonical_denoms.cpp). All
        # 4 series admissible; all s10-s18 bit-exact (~5e-15).
        ("airline_sar-seats", "s10"), ("airline_sar-seats", "s11"),
        ("airline_sar-seats", "s12"), ("airline_sar-seats", "s13"),
        ("airline_sar-seats", "s16"), ("airline_sar-seats", "s18"),
        ("expgs_sar-seats", "s10"), ("expgs_sar-seats", "s11"),
        ("expgs_sar-seats", "s12"), ("expgs_sar-seats", "s13"),
        ("expgs_sar-seats", "s16"), ("expgs_sar-seats", "s18"),
        ("payems_sar-seats", "s10"), ("payems_sar-seats", "s11"),
        ("payems_sar-seats", "s12"), ("payems_sar-seats", "s13"),
        ("payems_sar-seats", "s16"), ("payems_sar-seats", "s18"),
        ("unrate_sar-seats", "s10"), ("unrate_sar-seats", "s11"),
        ("unrate_sar-seats", "s12"), ("unrate_sar-seats", "s13"),
        ("unrate_sar-seats", "s16"), ("unrate_sar-seats", "s18"),
        # *_mean-seats: a Constant (mean) regressor -> Imean=1. SEATS decomposes
        # the series WITH the mean present (its differenced mean is wm); the
        # drift folds back via the wm centering of the CALCFX-seed differenced
        # series + za/wmf/wmb (estbur.cpp) -- and, decisively, ctx.series.tsrs
        # is restored to the mean-inclusive series in run_seats (estimation had
        # left the regression-ADJUSTED series, drift removed). airline (0 1 1)
        # (0 1 1)+const isolates the pure drift case (d>=1, Pstar=0); all s10-
        # s18 bit-exact (~4e-15) on all 4 series.
        ("airline_mean-seats", "s10"), ("airline_mean-seats", "s11"),
        ("airline_mean-seats", "s12"), ("airline_mean-seats", "s13"),
        ("airline_mean-seats", "s16"), ("airline_mean-seats", "s18"),
        ("payems_mean-seats", "s10"), ("payems_mean-seats", "s11"),
        ("payems_mean-seats", "s12"), ("payems_mean-seats", "s13"),
        ("payems_mean-seats", "s16"), ("payems_mean-seats", "s18"),
        ("unrate_mean-seats", "s10"), ("unrate_mean-seats", "s11"),
        ("unrate_mean-seats", "s12"), ("unrate_mean-seats", "s13"),
        ("unrate_mean-seats", "s16"), ("unrate_mean-seats", "s18"),
        ("expgs_mean-seats", "s10"), ("expgs_mean-seats", "s11"),
        ("expgs_mean-seats", "s12"), ("expgs_mean-seats", "s13"),
        ("expgs_mean-seats", "s16"), ("expgs_mean-seats", "s18"),
        # *_mean-d0-seats: mean regressor with a d==0 model (2 0 0)(0 1 1). d==0
        # is bit-exact on its own (non-mean d0 probe ~4e-15), so the mean path
        # is fully d-agnostic -- the earlier "d==0 trend unit root drops" theory
        # was wrong. Exercises Pstar=2 (AR) + kd=(-1)^(d+bd)=-1 (backward
        # center/seed sign-flip). All s10-s18 bit-exact (~5e-15) on all 4.
        ("airline_mean-d0-seats", "s10"), ("airline_mean-d0-seats", "s11"),
        ("airline_mean-d0-seats", "s12"), ("airline_mean-d0-seats", "s13"),
        ("airline_mean-d0-seats", "s16"), ("airline_mean-d0-seats", "s18"),
        ("payems_mean-d0-seats", "s10"), ("payems_mean-d0-seats", "s11"),
        ("payems_mean-d0-seats", "s12"), ("payems_mean-d0-seats", "s13"),
        ("payems_mean-d0-seats", "s16"), ("payems_mean-d0-seats", "s18"),
        ("unrate_mean-d0-seats", "s10"), ("unrate_mean-d0-seats", "s11"),
        ("unrate_mean-d0-seats", "s12"), ("unrate_mean-d0-seats", "s13"),
        ("unrate_mean-d0-seats", "s16"), ("unrate_mean-d0-seats", "s18"),
        ("expgs_mean-d0-seats", "s10"), ("expgs_mean-d0-seats", "s11"),
        ("expgs_mean-d0-seats", "s12"), ("expgs_mean-d0-seats", "s13"),
        ("expgs_mean-d0-seats", "s16"), ("expgs_mean-d0-seats", "s18"),
        # mean-td-seats: a Constant (mean) regressor ALONGSIDE trading-day
        # regressors -- the mean is kept in the decomposition (add-back onto the
        # regression-adjusted series, run_seats.cpp) while the TD regression
        # effect AND the automatic lom/leap prior are removed. s10 (seasonal-only
        # factor) is bit-exact; s16/s18 (combined adjustment factor = raw a1/sa)
        # refold BOTH the TD regression effect and the lom/leap prior
        # (run_pre_model seats_combined_orig, estbur.cpp combined_*).
        ("airline_mean-td-seats", "s10"), ("airline_mean-td-seats", "s11"),
        ("airline_mean-td-seats", "s12"), ("airline_mean-td-seats", "s13"),
        ("airline_mean-td-seats", "s16"), ("airline_mean-td-seats", "s18"),
        ("payems_mean-td-seats", "s10"), ("payems_mean-td-seats", "s11"),
        ("payems_mean-td-seats", "s12"), ("payems_mean-td-seats", "s13"),
        ("payems_mean-td-seats", "s16"), ("payems_mean-td-seats", "s18"),
        ("unrate_mean-td-seats", "s10"), ("unrate_mean-td-seats", "s11"),
        ("unrate_mean-td-seats", "s12"), ("unrate_mean-td-seats", "s13"),
        ("unrate_mean-td-seats", "s16"), ("unrate_mean-td-seats", "s18"),
        ("expgs_mean-td-seats", "s10"), ("expgs_mean-td-seats", "s11"),
        ("expgs_mean-td-seats", "s12"), ("expgs_mean-td-seats", "s13"),
        ("expgs_mean-td-seats", "s16"), ("expgs_mean-td-seats", "s18"),
        # ---- seats{} OPTION gates (hand-authored specs, not genspecs.py) ----
        # *_imean-{yes,no}-seats: the seats{imean=} OVERRIDE of ansub9.f:1072-
        # 1080's default derivation (L_IMEAN = "does the fitted model carry a
        # Constant regressor group"). imean-yes forces the mean ON for a model
        # with NO const regressor; imean-no forces it OFF for a model that has
        # one. Both are LIVE in the oracle -- flipping the flag moves s12 by
        # 6.8e-4 (airline), 3.7e-5 (payems), 3.5e-3 (expgs), 1.7e-3 (unrate),
        # and unrate's additive s10/s16 by 8.5e-1. The series SEATS decomposes
        # is unaffected either way (regeff never strips the Constant); only the
        # wm centering / za mean seed in estbur.cpp is gated by L_IMEAN.
        ("airline_imean-yes-seats", "s10"), ("airline_imean-yes-seats", "s11"),
        ("airline_imean-yes-seats", "s12"), ("airline_imean-yes-seats", "s13"),
        ("airline_imean-yes-seats", "s16"), ("airline_imean-yes-seats", "s18"),
        ("payems_imean-yes-seats", "s10"), ("payems_imean-yes-seats", "s11"),
        ("payems_imean-yes-seats", "s12"), ("payems_imean-yes-seats", "s13"),
        ("payems_imean-yes-seats", "s16"), ("payems_imean-yes-seats", "s18"),
        ("expgs_imean-yes-seats", "s10"), ("expgs_imean-yes-seats", "s11"),
        ("expgs_imean-yes-seats", "s12"), ("expgs_imean-yes-seats", "s13"),
        ("expgs_imean-yes-seats", "s16"), ("expgs_imean-yes-seats", "s18"),
        ("unrate_imean-yes-seats", "s10"), ("unrate_imean-yes-seats", "s11"),
        ("unrate_imean-yes-seats", "s12"), ("unrate_imean-yes-seats", "s13"),
        ("unrate_imean-yes-seats", "s16"), ("unrate_imean-yes-seats", "s18"),
        ("airline_imean-no-seats", "s10"), ("airline_imean-no-seats", "s11"),
        ("airline_imean-no-seats", "s12"), ("airline_imean-no-seats", "s13"),
        ("airline_imean-no-seats", "s16"), ("airline_imean-no-seats", "s18"),
        ("payems_imean-no-seats", "s10"), ("payems_imean-no-seats", "s11"),
        ("payems_imean-no-seats", "s12"), ("payems_imean-no-seats", "s13"),
        ("payems_imean-no-seats", "s16"), ("payems_imean-no-seats", "s18"),
        ("expgs_imean-no-seats", "s10"), ("expgs_imean-no-seats", "s11"),
        ("expgs_imean-no-seats", "s12"), ("expgs_imean-no-seats", "s13"),
        ("expgs_imean-no-seats", "s16"), ("expgs_imean-no-seats", "s18"),
        ("unrate_imean-no-seats", "s10"), ("unrate_imean-no-seats", "s11"),
        ("unrate_imean-no-seats", "s12"), ("unrate_imean-no-seats", "s13"),
        ("unrate_imean-no-seats", "s16"), ("unrate_imean-no-seats", "s18"),
        # *_noadmiss-seats / *_statseas-seats / *_bias0-seats: INERTNESS gates.
        # Each of these three flags only bites under a specific condition that
        # these (deliberately ordinary) specs do NOT meet, and the oracle's
        # output is byte-identical to the same spec without the flag -- so what
        # they pin is that the port does not invent an effect:
        #   noadmiss  -- only consulted when the canonical decomposition is
        #                INADMISSIBLE (SPECTRU qt1<0 / cycle varwnc<0). Here it
        #                is admissible, so noadmiss=yes changes nothing. (When
        #                it IS inadmissible the port now FATALS either way:
        #                noadmiss=no is the oracle's own abort, noadmiss=yes
        #                needs APPROXIMATE + SEATS-internal re-estimation.)
        #   statseas  -- only consulted by CHANGEMODEL (ansub1.f:3634-3747).
        #                (1 0 1)(0 1 1) reaches the d==0/p==1/q==1 test but
        #                fails it, so no model rewrite happens. (When
        #                CHANGEMODEL WOULD rewrite the model the port fatals --
        #                seats_changemodel_cambiado in seatopts.cpp.)
        #   bias=0    -- PORTED CENSUS QUIRK: analts.f:1647-1653 silently
        #                promotes bias=0 back to bias=1 and prints "BIAS SET
        #                EQUAL TO 1", so seats{bias=0} is a documented no-op.
        #                This gate is what stops a future "implementation" of
        #                bias=0 (skip the bias1c/bias2c/bias3c correction) from
        #                landing. bias=-1 (the one live value) is fatal --
        #                BIASCORR (ansub4.f:1244-1581) is not ported.
        ("airline_noadmiss-seats", "s10"), ("airline_noadmiss-seats", "s11"),
        ("airline_noadmiss-seats", "s12"), ("airline_noadmiss-seats", "s13"),
        ("airline_noadmiss-seats", "s16"), ("airline_noadmiss-seats", "s18"),
        ("payems_noadmiss-seats", "s10"), ("payems_noadmiss-seats", "s11"),
        ("payems_noadmiss-seats", "s12"), ("payems_noadmiss-seats", "s13"),
        ("payems_noadmiss-seats", "s16"), ("payems_noadmiss-seats", "s18"),
        ("expgs_noadmiss-seats", "s10"), ("expgs_noadmiss-seats", "s11"),
        ("expgs_noadmiss-seats", "s12"), ("expgs_noadmiss-seats", "s13"),
        ("expgs_noadmiss-seats", "s16"), ("expgs_noadmiss-seats", "s18"),
        ("unrate_noadmiss-seats", "s10"), ("unrate_noadmiss-seats", "s11"),
        ("unrate_noadmiss-seats", "s12"), ("unrate_noadmiss-seats", "s13"),
        ("unrate_noadmiss-seats", "s16"), ("unrate_noadmiss-seats", "s18"),
        ("airline_statseas-seats", "s10"), ("airline_statseas-seats", "s11"),
        ("airline_statseas-seats", "s12"), ("airline_statseas-seats", "s13"),
        ("airline_statseas-seats", "s16"), ("airline_statseas-seats", "s18"),
        ("payems_statseas-seats", "s10"), ("payems_statseas-seats", "s11"),
        ("payems_statseas-seats", "s12"), ("payems_statseas-seats", "s13"),
        ("payems_statseas-seats", "s16"), ("payems_statseas-seats", "s18"),
        ("expgs_statseas-seats", "s10"), ("expgs_statseas-seats", "s11"),
        ("expgs_statseas-seats", "s12"), ("expgs_statseas-seats", "s13"),
        ("expgs_statseas-seats", "s16"), ("expgs_statseas-seats", "s18"),
        ("unrate_statseas-seats", "s10"), ("unrate_statseas-seats", "s11"),
        ("unrate_statseas-seats", "s12"), ("unrate_statseas-seats", "s13"),
        ("unrate_statseas-seats", "s16"), ("unrate_statseas-seats", "s18"),
        ("airline_bias0-seats", "s10"), ("airline_bias0-seats", "s11"),
        ("airline_bias0-seats", "s12"), ("airline_bias0-seats", "s13"),
        ("airline_bias0-seats", "s16"), ("airline_bias0-seats", "s18"),
        ("payems_bias0-seats", "s10"), ("payems_bias0-seats", "s11"),
        ("payems_bias0-seats", "s12"), ("payems_bias0-seats", "s13"),
        ("payems_bias0-seats", "s16"), ("payems_bias0-seats", "s18"),
        ("expgs_bias0-seats", "s10"), ("expgs_bias0-seats", "s11"),
        ("expgs_bias0-seats", "s12"), ("expgs_bias0-seats", "s13"),
        ("expgs_bias0-seats", "s16"), ("expgs_bias0-seats", "s18"),
        ("unrate_bias0-seats", "s10"), ("unrate_bias0-seats", "s11"),
        ("unrate_bias0-seats", "s12"), ("unrate_bias0-seats", "s13"),
        ("unrate_bias0-seats", "s16"), ("unrate_bias0-seats", "s18"),
        # *_hp-*-seats: the Hodrick-Prescott option family
        # (seats{hpcycle/hplan/hptarget}). These gate s10-s18 as a NON-
        # DISTURBANCE check, not as evidence that HP is ported -- it is not
        # (tools/seats_hp_scouting.md). Measured against the oracle, the whole
        # blast radius of every HP option is .cyc/.ltt/.tbs/.sum; s10-s18 are
        # bit-identical with the filter on, off, or retargeted. The option
        # bridge itself is gated in test_seats_hpopts.py. hp-short additionally
        # covers a 6-year span (sigex.f:2371's minimum-length rule).
        ("airline_hp-hplan-seats", "s10"), ("airline_hp-hplan-seats", "s11"),
        ("airline_hp-hplan-seats", "s12"), ("airline_hp-hplan-seats", "s13"),
        ("airline_hp-hplan-seats", "s16"), ("airline_hp-hplan-seats", "s18"),
        ("airline_hp-off-seats", "s10"), ("airline_hp-off-seats", "s11"),
        ("airline_hp-off-seats", "s12"), ("airline_hp-off-seats", "s13"),
        ("airline_hp-off-seats", "s16"), ("airline_hp-off-seats", "s18"),
        ("airline_hp-relock-seats", "s10"), ("airline_hp-relock-seats", "s11"),
        ("airline_hp-relock-seats", "s12"), ("airline_hp-relock-seats", "s13"),
        ("airline_hp-relock-seats", "s16"), ("airline_hp-relock-seats", "s18"),
        ("airline_hp-short-seats", "s10"), ("airline_hp-short-seats", "s11"),
        ("airline_hp-short-seats", "s12"), ("airline_hp-short-seats", "s13"),
        ("airline_hp-short-seats", "s16"), ("airline_hp-short-seats", "s18"),
        ("payems_hp-hplan-seats", "s10"), ("payems_hp-hplan-seats", "s11"),
        ("payems_hp-hplan-seats", "s12"), ("payems_hp-hplan-seats", "s13"),
        ("payems_hp-hplan-seats", "s16"), ("payems_hp-hplan-seats", "s18"),
        ("payems_hp-off-seats", "s10"), ("payems_hp-off-seats", "s11"),
        ("payems_hp-off-seats", "s12"), ("payems_hp-off-seats", "s13"),
        ("payems_hp-off-seats", "s16"), ("payems_hp-off-seats", "s18"),
        ("payems_hp-relock-seats", "s10"), ("payems_hp-relock-seats", "s11"),
        ("payems_hp-relock-seats", "s12"), ("payems_hp-relock-seats", "s13"),
        ("payems_hp-relock-seats", "s16"), ("payems_hp-relock-seats", "s18"),
        # *_finite-seats: seats{finite=yes} -> /setopt/ Lfinit, the finite-
        # sample signal-extraction filters. These are the *_fixed-airline-seats
        # specs plus that one argument, and they gate the INVARIANCE: Lfinit
        # never touches the decomposition, so s10-s18 must stay bit-exact. That
        # is measured, not assumed -- the blessed goldens here are byte-identical
        # (body, ignoring the spec-name header line) to the
        # *_fixed-airline-seats goldens for all 4 series x all 6 tables, and a
        # 24-configuration oracle probe (4 series x 2 model shapes x {default,
        # seats out=0, span<120 obs, forecasts on}) moved no table written in
        # both modes.
        #
        # The flag is emphatically LIVE, though, and NOT merely print surface --
        # it gates getDiag (sigex.f:1502), which alone sets the flags seatdg.f
        # needs before it will write TEN save tables (faf/fac/ftf/ftc filter
        # weights, gaf/gac/gtf/gtc squared gains, tac/ttc time shifts: absent
        # entirely without finite=yes, 146-1203 rows each with it), plus 44 .udg
        # keys -- 39 `oustat*` going 0 -> nonzero and `pctreductionyr1..5` going
        # real -> 0.0000 (a Census defect, tools/census_bugs.md CB-16). getDiag
        # and its ~7.6 kloc closure are unported, and so is every SEATS save
        # table beyond s10-s18, so there is no port surface for those yet; see
        # the measurement note in core/src/seats/seatopts.hpp.
        ("airline_finite-seats", "s10"), ("airline_finite-seats", "s11"),
        ("airline_finite-seats", "s12"), ("airline_finite-seats", "s13"),
        ("airline_finite-seats", "s16"), ("airline_finite-seats", "s18"),
        ("payems_finite-seats", "s10"), ("payems_finite-seats", "s11"),
        ("payems_finite-seats", "s12"), ("payems_finite-seats", "s13"),
        ("payems_finite-seats", "s16"), ("payems_finite-seats", "s18"),
        ("unrate_finite-seats", "s10"), ("unrate_finite-seats", "s11"),
        ("unrate_finite-seats", "s12"), ("unrate_finite-seats", "s13"),
        ("unrate_finite-seats", "s16"), ("unrate_finite-seats", "s18"),
        ("expgs_finite-seats", "s10"), ("expgs_finite-seats", "s11"),
        ("expgs_finite-seats", "s12"), ("expgs_finite-seats", "s13"),
        ("expgs_finite-seats", "s16"), ("expgs_finite-seats", "s18"),
    }
    # No golden shipped => the oracle produced no such table, so there is no
    # parity target (not an engine gap). This covers the inadmissible-
    # decomposition specs (expgs_seats: the (2 1 0) model gives a negative
    # irregular spectrum, so the oracle aborts with "DECOMPOSITION INVALID,
    # IRREGULAR SPECTRUM NEGATIVE" and writes no s-tables) and the no-seasonal-
    # component tables (unrate_seats s10/s16: npsi==1). Skip, don't xfail.
    gold_path = os.path.join(_GOLDEN, base, base + "." + tag)
    if not os.path.exists(gold_path):
        pytest.skip(f"{base}.{tag}: oracle shipped no golden "
                    "(inadmissible decomposition or no seasonal component)")
    if (base, tag) not in _gated:
        pytest.xfail("SEATS (base, tag) not in the gated allowlist -- a spec "
                     "whose golden is not yet blessed/verified here; see "
                     "tools/seats_scope.md")
    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.returncode == 0, f"{base}: harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:200]

    produced: dict[str, float] = {}
    for ln in r.stdout.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == tag:
            produced[p[1]] = float(p[2])

    gold_path = os.path.join(_GOLDEN, base, base + "." + tag)
    if not os.path.exists(gold_path):
        pytest.skip(f"{base}.{tag}: no golden shipped")
    gold = _read_golden(gold_path)
    assert gold, f"{base}.{tag}: empty golden"

    keys = sorted(set(gold) & set(produced))
    assert len(keys) == len(gold), (
        f"{base}.{tag}: produced {len(produced)} rows, golden {len(gold)}, "
        f"overlap {len(keys)}")

    worst = 0.0
    worst_k = None
    bad = 0
    for k in keys:
        g, v = gold[k], produced[k]
        if abs(v - g) <= RTOL * abs(g) + ATOL:
            continue
        bad += 1
        rel = abs(v - g) / abs(g) if g else abs(v - g)
        if rel > worst:
            worst, worst_k = rel, k
    assert bad == 0, (
        f"{base}.{tag}: {bad} pts exceed RTOL*|g|+ATOL; "
        f"max rel err {worst:.3e} at {worst_k}")


_ARIMAMDL_RE = re.compile(r"^arimamdl:\s*(.+)$")


def _read_golden_arimamdl(base: str) -> str | None:
    udg = os.path.join(_GOLDEN, base, base + ".udg")
    if not os.path.exists(udg):
        return None
    with open(udg, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _ARIMAMDL_RE.match(ln.strip())
            if m:
                return m.group(1).strip()
    return None


@pytest.mark.skipif(not CASES, reason="no seats{} spec ships a golden bundle")
@pytest.mark.parametrize("base", CASES)
def test_seats_model_decode(base: str) -> None:
    """Model-decode gate (tools/seats_scope.md next-increment #1, session 3):
    x13::seats_decode_model (nmlmdl.f port, core/src/seats/model_decode.cpp)
    reads ctx.model's Mdl/Opr/Arimal ARMA-operator-list representation --
    already populated by the shared regARIMA estimate phase that runs before
    run_seats's still-open SPECTRU fatal -- into plain (p,d,q)(P,D,Q) orders.
    x13run_seats prints that decode (best-effort, regardless of OUTCOME) as
    `DECODE_ARIMAMDL: ...`, formatted exactly like the oracle's `arimamdl`
    .udg key (mkmdsn.f's "(p d q)(P D Q)" shape, seasonal group omitted when
    all-zero). NOT xfailed: this is a real, currently-passing gate for all 8
    SEATS corpus specs, independent of the canonical decomposition itself.
    """
    gold = _read_golden_arimamdl(base)
    if gold is None:
        pytest.skip(f"{base}: no arimamdl key in golden .udg")
    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.stdout.splitlines(), f"{base}: no output at all (exit {r.returncode})\n{r.stderr}"
    produced = None
    for ln in r.stdout.splitlines():
        if ln.startswith("DECODE_ARIMAMDL:"):
            produced = ln[len("DECODE_ARIMAMDL:"):].strip()
            break
    assert produced is not None, (
        f"{base}: harness printed no DECODE_ARIMAMDL line\n{r.stdout}\n{r.stderr}")
    assert produced == gold, f"{base}: decoded {produced!r}, golden arimamdl {gold!r}"


_IRRVAR_RE = re.compile(r"^irrvar:\s*(.+)$")


def _read_golden_irrvar(base: str) -> float | None:
    mdc = os.path.join(_GOLDEN, base, base + ".mdc")
    if not os.path.exists(mdc):
        return None
    with open(mdc, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _IRRVAR_RE.match(ln.strip())
            if m:
                return float(m.group(1).strip().replace("D", "E").replace("d", "e"))
    return None


@pytest.mark.skipif(not CASES, reason="no seats{} spec ships a golden bundle")
@pytest.mark.parametrize("base", CASES)
def test_seats_qt1(base: str) -> None:
    """SPECTRU qt1 gate (tools/seats_scope.md session 4-5): x13::spectru
    (spectrum.f:558-1190 port, core/src/seats/spectru.cpp) computes qt1, the
    canonical-decomposition admissibility number, from the decoded model via
    x13::seats_canonical_denoms's chi/psi/cyc/thstar/pstar. x13run_seats
    prints it (best-effort) as `DECODE_QT1: <value>`; this diffs it against
    the golden `.mdc`'s `irrvar` key (traced this session, via ansub9.f's
    USRENTRY IFUNC=2010 dispatch, to be exactly SPECTRU's own qt1 output,
    unchanged, saved by ShowComp/DecompSpectrum).

    SESSION-5 FIX: this gate did not pass at the end of session 4 (computed
    qt1 was 14.9% high on unrate_seats, wrong-signed on payems_seats). Traced
    to a sign-convention bug in canonical_denoms.cpp's `thstar` (MA numerator
    polynomial) construction -- built as `ths(i+1)=-Th(i)`, literally
    matching analts.f:2854-2859's text, but empirically wrong; the fix
    (`ths(i+1)=+Th(i)`, no negation) makes qt1 match golden irrvar BIT-EXACT
    (every printed digit) on all 7 corpus specs with a golden `.mdc` (the
    8th, expgs_seats, has none -- the oracle itself rejects that model, see
    tools/seats_scope.md's session-2 note). WHY the literal Fortran text
    disagrees with the empirically-correct sign is not resolved (see
    canonical_denoms.cpp's comment); flagged for a future session. The `phi`
    side (phis/bphis, AR) was NOT touched -- p=0 for every admissible corpus
    target, so there is no data point to test that sign against.
    """
    gold = _read_golden_irrvar(base)
    if gold is None:
        pytest.skip(f"{base}: no irrvar key in golden .mdc (oracle likely rejected the model)")
    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.stdout.splitlines(), f"{base}: no output at all (exit {r.returncode})\n{r.stderr}"
    produced = None
    for ln in r.stdout.splitlines():
        if ln.startswith("DECODE_QT1:"):
            produced = float(ln[len("DECODE_QT1:"):].strip())
            break
    assert produced is not None, (
        f"{base}: harness printed no DECODE_QT1 line\n{r.stdout}\n{r.stderr}")
    rel = abs(produced - gold) / abs(gold) if gold else abs(produced - gold)
    assert rel <= 1e-8, f"{base}: qt1={produced!r}, golden irrvar={gold!r}, rel err {rel:.3e}"


# .mdc scalar/array keys DecompSpectrum/MAspectrum (core/src/seats/
# decompspectrum.cpp, session 6) produce, keyed by their golden .mdc name.
# `n<key>` count lines (e.g. `nsanum:`) are read too but only used to assert
# the array lengths agree; the comparison itself is per-element.
_MDC_ARRAY_KEYS = ["sanum", "saden", "snum", "sden", "trnum", "trden"]
_MDC_SCALAR_KEYS = ["savar", "svar", "trvar"]
_MDC_LINE_RE = re.compile(r"^(\w+)(?:\.(\d+))?:\s*(.+)$")


def _read_mdc_dict(path: str) -> dict[str, float]:
    """Parses a `.mdc`-shaped file (golden or, with an `MDC_` prefix
    stripped by the caller, the harness's own output) into a flat
    {key or key.NNN: float} dict, matching the file's own `key: value` /
    `key.NNN: value` line shape."""
    out: dict[str, float] = {}
    if not os.path.exists(path):
        return out
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _MDC_LINE_RE.match(ln.strip())
            if not m:
                continue
            key, idx, val = m.group(1), m.group(2), m.group(3)
            try:
                fval = float(val.strip().replace("D", "E").replace("d", "e"))
            except ValueError:
                continue
            out[f"{key}.{idx}" if idx is not None else key] = fval
    return out


def _read_produced_mdc_dict(stdout: str) -> dict[str, float]:
    out: dict[str, float] = {}
    for ln in stdout.splitlines():
        if not ln.startswith("MDC_"):
            continue
        m = _MDC_LINE_RE.match(ln[len("MDC_"):].strip())
        if not m:
            continue
        key, idx, val = m.group(1), m.group(2), m.group(3)
        out[f"{key}.{idx}" if idx is not None else key] = float(val)
    return out


@pytest.mark.skipif(not CASES, reason="no seats{} spec ships a golden bundle")
@pytest.mark.parametrize("base", CASES)
def test_seats_mdc(base: str) -> None:
    """DecompSpectrum/MAspectrum .mdc gate (tools/seats_scope.md session 6):
    x13::decomp_spectrum (spectrum.f:1380-2513 + spectrum.f:2710-2890 port,
    core/src/seats/decompspectrum.cpp) turns SPECTRU's Ut/V/Uc into the final
    per-component MA numerator polynomials (THETP/THETS/THETC/THADJ) and
    innovation variances via the already-ported MAK1. x13run_seats prints
    the ones that map to golden `.mdc` keys (traced via ansub9.f's USRENTRY
    IFUNC=2001-2013 dispatch: THADJ/CHCYC/VARWNA -> sanum/saden/savar,
    THETS/PSI/VARWNS -> snum/sden/svar when npsi!=1, THETC/CYC/VARWNC ->
    trnum/trden/trvar when ncycth!=0 or ncyc!=1) as `MDC_<key>[.NNN]: value`
    lines; this diffs every key BOTH sides have against the golden `.mdc`.

    Confirmed bit-exact (exact-string match, session 6) on every corpus spec
    checked: payems_seats/unrate_seats (sanum/saden/savar, no seasonal
    structure), payems_fixed-airline-seats/airline_fixed-airline-seats/
    airline_seats/unrate_fixed-airline-seats/expgs_fixed-airline-seats
    (sanum/saden/savar plus, for airline_seats and airline_fixed-airline-
    seats which have real seasonal structure, snum/sden/svar too). payems
    also exercises trnum/trden/trvar (its qstar>pstar ADDJ fold sets
    ncycth=1). expgs_seats has no golden `.mdc` at all (oracle rejects the
    model, unchanged since session 2) -- skipped.
    """
    gold_path = os.path.join(_GOLDEN, base, base + ".mdc")
    gold = _read_mdc_dict(gold_path)
    if not gold:
        pytest.skip(f"{base}: no .mdc golden (oracle likely rejected the model)")
    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.stdout.splitlines(), f"{base}: no output at all (exit {r.returncode})\n{r.stderr}"
    produced = _read_produced_mdc_dict(r.stdout)

    checked_any = False
    worst = 0.0
    worst_k = None
    for base_key in _MDC_ARRAY_KEYS + _MDC_SCALAR_KEYS:
        gold_keys = [k for k in gold if k == base_key or k.startswith(base_key + ".")]
        if not gold_keys:
            continue  # this .mdc doesn't ship this key (e.g. no seasonal structure)
        # Within a gated family, the produced key set must EQUAL the golden's.
        # Iterating gold_keys alone accepted produced > gold, so a component
        # polynomial emitted one degree too long -- an extra snum./sden. element
        # past what the oracle wrote -- matched on every element it shared and
        # passed. (Keys OUTSIDE the gated families are deliberately unchecked;
        # only these two lists are contract.)
        prod_keys = [k for k in produced
                     if k == base_key or k.startswith(base_key + ".")]
        extra = sorted(set(prod_keys) - set(gold_keys))
        assert not extra, (
            f"{base}: harness produced MDC keys the golden does not have: "
            f"{extra} (golden has {sorted(gold_keys)})")
        for k in gold_keys:
            assert k in produced, (
                f"{base}: golden has {k}={gold[k]!r} but harness produced no MDC_{k} line\n"
                f"{r.stdout}\n{r.stderr}")
            checked_any = True
            g, v = gold[k], produced[k]
            rel = abs(v - g) / abs(g) if g else abs(v - g)
            if rel > worst:
                worst, worst_k = rel, k
    assert checked_any, f"{base}: golden .mdc has none of the gated keys -- nothing to check"
    assert worst <= 1e-8, f"{base}: max rel err {worst:.3e} at {worst_k}"


def test_seats_harness_runs_cleanly() -> None:
    """Sanity gate (NOT xfailed): the harness must parse + estimate every
    SEATS corpus spec without crashing, even though it can't complete the
    decomposition yet. Catches regressions in the shared parse/estimate path
    independent of the (still-unported) SEATS-specific work."""
    assert CASES, "no seats{} spec discovered"
    for base in CASES:
        spec = os.path.join(_CORPUS, base + ".spc")
        r = subprocess.run([BIN, spec], capture_output=True, text=True)
        assert r.stdout.splitlines(), f"{base}: no output at all (exit {r.returncode})"
        outcome = r.stdout.splitlines()[0].strip()
        assert outcome in ("OUTCOME: OK", "OUTCOME: FATAL"), (
            f"{base}: harness produced unexpected first line {outcome!r} "
            f"(exit {r.returncode})\nstderr:\n{r.stderr}")
