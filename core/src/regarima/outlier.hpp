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

#include <string>

#include "common/x13context.hpp"

namespace x13 {

// wrtdat.f: date (idate = {yr, mo}) -> "yyyy" (sp<=1), "yyyy.Mon" (sp==12), or
// "yyyy.mm" (other sp). Used to name outlier regressor columns.
std::string wrtdat(const int* idate, int sp);

// wrtotl.f: outlier column title, e.g. "AO1951.May" -- the 2-char type
// (AO/LS/TC/SO/Rp/MV, itype 1..6) followed by the date (begotl, relative to
// begdat). Ramp (itype==5) appends "-<enddate>".
std::string wrtotl(int itype, int begotl, int endotl, const int* begdat, int sp);

// rdotlr.f: parse an outlier column title back to its type (otlind: AO=1, LS=2,
// TC=3, RP=4, MV=5, TLS=6, SO=7, QI=8, QD=9) and time index begotl (1-based row
// in the span starting at begspn); endotl is the ramp end (0 otherwise). Sets
// locok=false on a malformed title.
void rdotlr(X13Context& ctx, const std::string& otlttl, const int* begspn,
            int sp, int& otlind, int& begotl, int& endotl, bool& locok);

// setcv.f: default outlier critical value from the outlier-test span length
// nspobs and the alpha level cvalfa (Ljung-style approximation). For nspobs==1
// it is the normal deviate; otherwise a 3-point (n=2,100,200) log-scale fit is
// solved (lassol) and extrapolated. Returns prm::DNOTST on internal failure.
double setcv(int nspobs, double cvalfa);

// setcvl.f: the large-sample (Ljung) variant of the same critical value, chosen
// by Cvxtyp at editor.f:1752. Returns prm::DNOTST for a 1-point span.
double setcvl(int nspobs, double cvalfa);

// idotlr.f: automatic outlier identification driver. Forward-addition (add the
// largest AO/LS/TC over the per-type critical value, re-estimate, repeat) then
// backward-deletion (drop any auto-outlier whose non-robust t falls below the
// critical value, re-estimate). Adds the identified outliers to the regression
// (adrgef/coladd/addotl) and leaves the model re-estimated in ctx. ltstao/ls/tc
// select the types tested; ladd1 adds one outlier per pass (else all over the
// threshold); critvl is the per-type critical value (0-based by type-1);
// begtst/endtst bound the test span; a is the residual work vector (from the
// prior rgarma, updated on each re-estimation). Deferred vs the Fortran: all
// iteration/table printing and save files and the diagnostic "almost outlier"
// reduced-critical-value re-scan (which never changes the model). cvrduc is
// accepted for signature parity but unused here.
//
// lxreg selects the X-11 irregular-regression path (idotlr.f Lxreg=T): the model
// is a pure OLS (no ARMA), so the design copy is used unfiltered (no armafl), the
// robust mse is taken from the raw residuals, and each re-estimation is a regx11
// OLS instead of rgarma. Requires nxcld==0 (with a holiday/AO group present the
// upstream tdxtrm exclusion is skipped, so the excluded-row lxreg case is unported).
void idotlr(X13Context& ctx, bool ltstao, bool ltstls, bool ltsttc, bool ladd1,
            const double* critvl, double cvrduc, const int* begtst,
            const int* endtst, int& nefobs, bool lestim, int mxiter, int mxnlit,
            bool lauto, double* a, bool lxreg = false);

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

// deltst.f: backward-deletion test. For the auto-outlier columns begcol..endcol
// of the estimated regression, computes the t-statistic b_i/se(b_i)/rmse (se
// from the packed (X'X)^-1 via dppdi) and records, per outlier type, the column
// with the smallest |t| (mini/mint, 0-based by type-1) plus minptr, the types
// ranked by |t| ascending (smallest first). Reads the estimated model off ctx
// (Chlxpx/B/Nb/Ncxy/Colttl/Begspn). Sets lfatal if the residual rmse is zero.
void deltst(X13Context& ctx, int nefobs, int begcol, int endcol, double* mint,
            int* mini, int* minptr, bool lauto, bool lxreg);

// addotl.f: (re)build the outlier regressor columns begcol..endcol of the design
// matrix (ctx.mdldat.xy) from their column titles. Each column's title is parsed
// (rdotlr) and filled per type: AO = 1 at t0; LS = -1 before t0, 0 after; TC =
// geometric decay by Tcalfa from t0; plus ramp/TLS/SO/quadratic-ramp shapes.
// bgdtxy is the design's start date (yr,mo); iymx the series-vs-regressor start
// offset (0 in the outlier-ID phase); nrxy the row count. A column whose
// effect falls outside the span is dropped (dlrgef) and endcol decremented.
void addotl(X13Context& ctx, const int* bgdtxy, int nrxy, int iymx, int begcol,
            int& endcol);

// coladd.f: open naddc = endcol-begcol+1 empty columns at begcol in the
// row-major nrxy x ncxy matrix xy (leading dim ncxy), shifting existing columns
// right. ncxy is updated to ncxy+naddc. The opened slots keep whatever was there
// (addotl fills them). peltxy is the Fortran storage bound (unused here). The
// caller must have sized xy for the new column count.
void coladd(int begcol, int endcol, int nrxy, int peltxy, double* xy, int& ncxy);

}  // namespace x13

#endif  // X13_REGARIMA_OUTLIER_HPP
