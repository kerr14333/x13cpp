// estbur.hpp -- ESTBUR (ansub3.f:54-355), historical-span-only. Ported after
// session 12's oracle-instrumentation ground-truth run pinned down the real
// ESTBUR inputs for unrate_seats: pstar=2, qstar=2 (== cd.pstar/cd.qstar
// DIRECTLY, no "B-J sign switch" needed -- sessions 10-11's switch chase was
// a red herring, see decompspectrum.hpp's own corrected header comment),
// general MLTSOL branch (qstar!=1), and ct/cs/cc from the newly-ported
// spectrum.f:1529-1533/1570-1573/1625-1628 filter-numerator construction
// (SeatsComponentModels::ct/cs/cc).
//
// SCOPE: the FORECAST span (ansub3.f:353-678) is now ported too -- see
// EstburResult::f_trend/f_sc/f_sa/f_cycle. It never touches i<=Nz (every
// forecast-block guard is downstream of `k=Nz+i` for i>=1), so the historical
// results are unchanged by it.
//
// The Tramo block (ansub3.f:552-653) IS ported, and so is ansub4.f's refold of
// the deterministic factors onto the saved forecast tables. Both were once
// declared unreachable here, and both claims were wrong -- `Tramo` is 1 on an
// ordinary X-13 SEATS run, which makes the Tramo block live and makes
// sigex.f:3631-3636's punches (guarded `if (Tramo .le. 0)`) dead. Measured
// 2026-07-30 against an instrumented oracle. All 52 corpus specs now gate
// bit-exact over the forecast span; the record, including the way the two
// errors cancelled and hid each other, is in
// tools/seats_forecast_scouting.md section 3.
// A small forward/backward extension
// (FCAST-style, ansub1.f:2183-2201, `lext = qstar+maxpq-2` points) IS
// needed even for the historical output whenever maxpq>1, since the
// two-sided filter (`gt`) needs `extZ`/`bz` slightly beyond the sample
// boundary -- seeded by armafl()-derived residuals (session 9/10,
// armafl-reuse validated to ~1.5e-6 relative against golden `varres`).
//
// GENERAL-SHAPE (p>0) IS NOW SUPPORTED + GATED: the *_ar2-seats corpus specs
// ((2 1 0)(0 1 1)) gate s10-s18 bit-exact (~5e-15) on airline/payems/unrate.
// The AR roots flow through FCAST's bphist phist fill (build_bphist), the
// CALCFX stationary-AR filter (calcfx_last_residuals Pstar>0 branch, needed
// for payems_ar2's near-non-invertible seasonal MA ~0.985), and the ESTBUR
// general ct/cs/cc/MLTSOL solve. Both port bugs were AR-polynomial signs in
// canonical_denoms.cpp (phis = +mo.phi for p>0; bphis = +mo.bphi for bp>0).
// bp>0 (seasonal AR) is ALSO gated now: the *_sar-seats specs (0 1 1)(1 1 0)
// gate s10-s18 bit-exact on all 4 series. imean!=0 (a Constant/mean regressor)
// is ALSO supported + gated, and is d-AGNOSTIC: the *_mean-seats specs
// ((0 1 1)(0 1 1)+const, d>=1 drift) AND the *_mean-d0-seats specs
// ((2 0 0)(0 1 1)+const, d==0) both gate s10-s18 bit-exact on all 4 series. The
// mean is carried IN the decomposed series (run_seats restores the mean-
// inclusive tsrs); wm (the differenced-series mean) centers the CALCFX-seed
// differenced series (forward wm, backward kd*wm, kd=(-1)^(d+bd)) and folds back
// via za in FCAST + wmf/wmb in the ESTBUR boundary. (d==0 needs no special
// handling -- the non-mean d==0 path is itself bit-exact.) A mean ALONGSIDE
// OTHER regressors (TD/outliers) is ALSO gated now (the *_mean-td-seats specs):
// the Constant's fitted contribution is added back onto the regression-adjusted
// series (run_seats.cpp) while the other effects stay removed, and s16/s18 (the
// COMBINED adjustment factors = raw a1/sa) refold both the removed regression
// effects and the lom/leap prior via combined_factor/combined_add
// (run_pre_model seats_combined_orig). isCloseToTD is always false in this
// port. npsi!=1 (real seasonal) / ncycth!=0||ncyc!=1 (real cycle) flow through
// correctly and now have coverage via the ar2/sar specs' structure.
#ifndef X13_SEATS_ESTBUR_HPP
#define X13_SEATS_ESTBUR_HPP

#include <vector>

#include "common/x13context.hpp"
#include "seats/canonical_denoms.hpp"
#include "seats/decompspectrum.hpp"
#include "seats/model_decode.hpp"
#include "seats/seatopts.hpp"

namespace x13 {

struct EstburResult {
    std::vector<double> trend;  // 0-indexed, length Nz
    std::vector<double> sc;
    std::vector<double> cycle;
    std::vector<double> sa;
    std::vector<double> ir;
    // s10/s16/s18 (seasonal factor -- session 13 finding): all three save
    // tags are numerically IDENTICAL for every corpus spec checked (no
    // spec has a separate calendar/regression factor to distinguish
    // "decomposition-alone" (s10) from "combined final" (s16/s18) --
    // confirmed against unrate_fixed-airline-seats, airline_seats/
    // airline_fixed-airline-seats), and equal `z/sa` (original series over
    // the seasonally-adjusted series) UNIVERSALLY -- true in BOTH the
    // additive case (unrate_fixed-airline-seats: verified z/sa matches
    // s10/s16 bit-for-bit) and the multiplicative/log case (airline_seats:
    // verified same formula against s10/s16/s18, all three identical).
    // For npsi==1 (no seasonal component, sa==z identically -- unrate_seats
    // and payems_seats), this is trivially 1.0 at every point, matching
    // golden exactly. `z` here must be the ORIGINAL (untransformed) series,
    // NOT ctx.series.tsrs's log-linearized value for a log-transform model
    // -- see run_seats.cpp's wiring for how the untransformed value is
    // sourced.
    std::vector<double> seasonal_factor;
    // s10/s16 table: the seasonal "factor" in the SAVE convention -- z/sa
    // (multiplicative) for a log model, but z-sa (additive difference) for a
    // no-transform model. Differs from `seasonal_factor` (== s18, ALWAYS the
    // z/sa ratio) only in the additive case. For log specs the two arrays are
    // identical.
    std::vector<double> seasonal_add;
    // s16/s18 COMBINED adjustment factors = original / sa (the removed calendar/
    // outlier effects refolded on top of the seasonal). Equal to seasonal_add/
    // seasonal_factor when no regression effect was stripped from the
    // decomposition input (ctx.seats_combined_orig empty -> s10==s16==s18);
    // differ once TD/holiday/outliers are present (the original series carries
    // them, the linearized decomposition input does not). combined_factor is the
    // z/sa RATIO (s18); combined_add is z/sa (log/mult) or z-sa (additive) (s16).
    std::vector<double> combined_factor;
    std::vector<double> combined_add;
    // ---- FORECAST SPAN (ansub3.f:353-678 + sigsub.f:1586-1605) ----
    // The `tfd`/`sfd`/`afd`/`yfd` save tables: the decomposition over
    // Nz+1..Nz+lfor, in the units the oracle punches them in. `f_sc` and
    // `f_cycle` are in PERCENT (sigsub.f:1603-1604) on the log path, because
    // seatad.f:39/110 is what divides them by 100 when they are APPENDED onto
    // Seatsf/Seatcy -- seatpr.f saves the raw Setf* arrays. `f_trend`/`f_sa`
    // are levels. On the additive path all four are exactly what ESTBUR left
    // (sigsub.f:1525-1534's lamd==1 arm only rebuilds `sa` over 1..Nz).
    // Empty when the run produced no forecast span.
    std::vector<double> f_trend;   // tfd
    std::vector<double> f_sc;      // sfd
    std::vector<double> f_sa;      // afd
    std::vector<double> f_cycle;   // yfd
    bool ok = false;
};

// estbur_historical -- z = ctx.series.tsrs(1..nspobs); bz = plain reversal.
// mo/cd/comp must already be populated (seats_decode_model ->
// seats_canonical_denoms -> spectru -> decomp_spectrum, the existing
// x13run_seats.cpp probe chain). `opts` supplies L_IMEAN (the wm centering /
// FCAST mean seed gate) -- resolved by seats_resolve_options, which honours an
// explicit seats{imean=} override on top of the Constant-regressor derivation.
void estbur_historical(X13Context& ctx, const SeatsModelOrders& mo,
                        const SeatsCanonicalDenoms& cd,
                        const SeatsComponentModels& comp,
                        const SeatsOptions& opts, EstburResult& out);

}  // namespace x13

#endif  // X13_SEATS_ESTBUR_HPP
