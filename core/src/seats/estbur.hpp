// estbur.hpp -- ESTBUR (ansub3.f:54-355), historical-span-only. Ported after
// session 12's oracle-instrumentation ground-truth run pinned down the real
// ESTBUR inputs for unrate_seats: pstar=2, qstar=2 (== cd.pstar/cd.qstar
// DIRECTLY, no "B-J sign switch" needed -- sessions 10-11's switch chase was
// a red herring, see decompspectrum.hpp's own corrected header comment),
// general MLTSOL branch (qstar!=1), and ct/cs/cc from the newly-ported
// spectrum.f:1529-1533/1570-1573/1625-1628 filter-numerator construction
// (SeatsComponentModels::ct/cs/cc).
//
// SCOPE: only the historical span (i=1..Nz) is ported -- ansub3.f:356-678
// (Tramo passthrough + the npsi!=1/Nchi!=1/cycle FORECAST blocks) only ever
// write trend/sc/cycle(Nz+1..Nz+lf), never touching i<=Nz, confirmed by
// inspection of every forecast-block guard (`if (k.le.Nz+lf)` etc, all
// downstream of `k=Nz+i` for i>=1). A small forward/backward extension
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
// general ct/cs/cc/MLTSOL solve. The one port bug was the AR-polynomial sign
// in canonical_denoms.cpp (phis = +mo.phi). bp>0 (seasonal AR) and imean!=0
// (za!=0) remain unported (no admissible corpus target yet); isCloseToTD is
// always false in this port. npsi!=1 (real seasonal) / ncycth!=0||ncyc!=1
// (real cycle) flow through correctly and now HAVE coverage via the ar2
// specs' seasonal structure.
#ifndef X13_SEATS_ESTBUR_HPP
#define X13_SEATS_ESTBUR_HPP

#include <vector>

#include "common/x13context.hpp"
#include "seats/canonical_denoms.hpp"
#include "seats/decompspectrum.hpp"
#include "seats/model_decode.hpp"

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
    bool ok = false;
};

// estbur_historical -- z = ctx.series.tsrs(1..nspobs); bz = plain reversal.
// mo/cd/comp must already be populated (seats_decode_model ->
// seats_canonical_denoms -> spectru -> decomp_spectrum, the existing
// x13run_seats.cpp probe chain).
void estbur_historical(X13Context& ctx, const SeatsModelOrders& mo,
                        const SeatsCanonicalDenoms& cd,
                        const SeatsComponentModels& comp, EstburResult& out);

}  // namespace x13

#endif  // X13_SEATS_ESTBUR_HPP
