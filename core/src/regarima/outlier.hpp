// outlier.hpp -- automatic outlier identification (idotlr.f and its leaves).
//
// The regARIMA automatic-outlier scan: for each candidate time point it builds
// an AO/LS/TC regressor, ARMA-filters it, and forms the proportional
// t-statistic of adding it to the estimated regression via an augmented
// Cholesky update; the largest over the critical value is added and the model
// re-estimated (forward addition), then non-significant outliers are dropped
// (backward deletion). This header covers the pure numeric leaves; the idotlr
// driver is added alongside as the phase is built out.
#ifndef X13_REGARIMA_OUTLIER_HPP
#define X13_REGARIMA_OUTLIER_HPP

namespace x13 {

// makotl.f: build the outlier regressor column(s) for an effect at time t0 over
// nr rows. ltest is the 3-element [AO,LS,TC] test-flag slice (1 = build that
// type); the active types are written interleaved into otlvar with stride notlr
// (the number of active types, returned). AO = 1 at t0 else 0; LS = -1 for
// t<t0 else 0; TC = 0 before t0, 1 at t0, then geometric decay by tcalfa. sp is
// the seasonal period (only used by the disabled SO branch, kept for parity).
void makotl(int t0, int nr, const int* ltest, double* otlvar, int& notlr,
            double tcalfa, int sp);

// ttest.f: proportional t-statistics t*sqrt(mse) = b/se(b)*mse for adding each
// active outlier type at one time point, via an augmented Cholesky solve against
// chlxpx (the packed Cholesky of [X:y]'[X:y]). xy is the ARMA-filtered design
// (nspobs x ncxy, row-major); otlvar the ARMA-filtered outlier regressor(s)
// (interleaved, stride = #active types); ltstpt the [AO,LS,TC] flags. Outputs:
// propt[type] the proportional t-value, mxcol the types ranked by |t| (largest
// first), snglr[type] true when [X:o]'[X:o] is singular. All type-indexed arrays
// are 1-based (index by prm::AO/LS/TC).
void ttest(const double* xy, int nspobs, int ncxy, const double* chlxpx,
           const double* otlvar, const int* ltstpt, int* mxcol, double* propt,
           bool* snglr);

// coladd.f: open naddc = endcol-begcol+1 empty columns at begcol in the
// row-major nrxy x ncxy matrix xy (leading dim ncxy), shifting existing columns
// right. ncxy is updated to ncxy+naddc. The opened slots keep whatever was there
// (addotl fills them). peltxy is the Fortran storage bound (unused here). The
// caller must have sized xy for the new column count.
void coladd(int begcol, int endcol, int nrxy, int peltxy, double* xy, int& ncxy);

}  // namespace x13

#endif  // X13_REGARIMA_OUTLIER_HPP
