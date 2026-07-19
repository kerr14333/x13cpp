// estimate.hpp -- the regARIMA IGLS estimation driver (rgarma.f) and its
// regression-solve helpers olsreg.f / resid.f. These sit above the exact ARMA
// filter (armafl.hpp) and drive the MINPACK optimizer. Routines that only touch
// plain array arguments still take X13Context& first because their error guards
// call abend/errhdr/writln through it.
#ifndef X13_REGARIMA_ESTIMATE_HPP
#define X13_REGARIMA_ESTIMATE_HPP

#include "common/x13context.hpp"

namespace x13 {

// strtvl.f: ARMA starting values. Walks every lag of the DIFF..MA operators;
// any free lag (arimaf false) whose arimap is still the not-set sentinel
// (dpeq(arimap, DNOTST)) is seeded to 0.1. Fixed lags and already-valued lags
// are left untouched. (Regression betas are computed elsewhere via olsreg.)
void strtvl(X13Context& ctx);

// stpitr.f: the IGLS step/convergence test (Fortran LOGICAL FUNCTION). Returns
// true to keep iterating, false to stop. convrg (out) is false only on the two
// hard-error stops; armaer (out) is written PMXIER/PCNTER/PDVTER on the error
// stops and left alone otherwise. It compares the relative deviance
// |oldobj/objfcn - 1| against devtol, where oldobj is the previous call's
// objfcn -- carried in ctx.saved.stpitr_oldobj (the Fortran SAVE). The first
// iteration (or objfcn==0) only records oldobj and returns "keep going". The
// deviance-increase warning print (Lprier path) is deferred to the print
// milestone, exactly as fcnar's diagnostics are -- it changes no output here.
bool stpitr(X13Context& ctx, bool lprier, double objfcn, double devtol, int iter,
            int nliter, int mxiter, bool& convrg, int& armaer, bool lhiddn);

// olsreg.f: ordinary least squares by the normal equations. Forms [X:y]'[X:y]
// (xprmx) into chlxpx, Cholesky-factors it (dppfa), and back-solves the upper-
// triangular system L'b = z for the nb = ncxy-1 regression estimates b. The
// data column y is the pcxy-th column of xy (nrxy rows). chlxpx returns packed:
// chol(X'X), then z, then sqrt(RSS) in the last element. info=0 on success.
void olsreg(X13Context& ctx, const double* xy, int nrxy, int ncxy, int pcxy,
            double* b, double* chlxpx, int pxpx, int& info);

// resid.f: regression residuals rsd = y + sign(fac)*X*b, where y is the pcxy-th
// column of the [X:y] matrix xy (nr rows) and columns begcol..endcol of X are
// used. fac's sign selects add (a+Xb) vs subtract (y-Xb). With nc==0 or an empty
// column range it returns a plain copy of y.
void resid(X13Context& ctx, const double* xy, int nr, int nc, int pc, int begcol,
           int endcol, double fac, const double* b, double* rsd);

// upespm.f: scatter the nonlinear optimizer's parameter vector estprm back into
// the ARIMA filter structures. Walking operators DIFF..MA in lag order, each
// non-fixed lag (arimaf false) consumes the next estprm element into arimap;
// fixed lags are skipped (their arimap value is left untouched).
void upespm(X13Context& ctx, const double* estprm);

// fcnar.f: the residual (objective) function driven by the nonlinear optimizer
// lmdif. Scatters estprm into the model (upespm), copies the working series
// tsrs into a, exact-ARMA-filters it (armafl), and returns the deviances in a
// (length na). On a filter failure it floods a with the large-residual sentinel
// lrgrsd (so a bad optimizer step is pulled back in bounds) and clears info/err;
// on success under exact ML (lextma) it scales the residuals by
// exp(lndtcv/2/dnefob), the likelihood Jacobian. lckinv gates armafl's root
// check. NB: the info!=0 diagnostic warnings (fcnar.f Lprier block) are not yet
// emitted -- deferred to the .out print milestone; the numerics here are exact.
void fcnar(X13Context& ctx, int& na, int testpm, const double* estprm, double* a,
           bool lauto, bool gudrun, int& err, bool lckinv);

// roots.f: modulus and frequency of the roots of a polynomial. thetab holds the
// degree+1 coefficients of theta(B) in INCREASING powers; it is reversed to
// decreasing powers and (leading near-zero coefficients stripped) handed to
// rpoly. On success allinv is set true iff every root has modulus >= 1 (all
// zeros invertible), and zerom/zerof receive each root's modulus and frequency
// (angle/2pi); complex roots fill their conjugate slot too. degree is in/out
// (reduced if leading coefficients are ~0, or if rpoly finds fewer roots). NB:
// the rpoly-failure warning (roots.f WRITE to Mt1/STDERR) is deferred to the
// .out print milestone; on failure allinv is left as the caller passed it,
// matching the oracle. See tools/census_bugs.md CB-3 for the off-by-typo 2pi.
void roots(X13Context& ctx, const double* thetab, int& degree, bool& allinv,
           double* zeror, double* zeroi, double* zerom, double* zerof);

// setmdl.f: pack the free AR/MA coefficients into estprm (setting model.nestpm)
// and root-check the starting values -- theta(B) for invertibility, phi(B) for
// stationarity. On the first call (ctx.saved.setmdl_first) an MA root ON the
// unit circle is an error; on later calls near-unit-circle MA operators are
// shrunk by PT9**lag instead. laumts is in/out: if any check fails and laumts
// is set it is cleared (signaling failed Hannan-Rissanen initial values),
// otherwise the routine abends. The header's "differences the X:y matrix"
// comment is stale -- this version does not touch Xy (census_bugs.md CB-5).
// Error/warning prints and getstr operator-title lookups are deferred to the
// .out milestone; the flag logic driving the abend/laumts handshake is kept.
void setmdl(X13Context& ctx, double* estprm, bool& laumts);

// chkrt2.f: re-check the roots of theta(B) (and phi(B) when exact AR) after an
// ARMA-filter failure. In the vendored version the only persistent effect is
// inverr=0 -- despite the name it does NOT invert anything; the ".not.allinv"
// block is entirely the Lprier-gated root-table print, deferred to the .out
// milestone. lprmsg/lhiddn only steer that deferred print. Kept as a named port
// so rgarma's filter-error branch stays a faithful call, not an inlined 0.
void chkrt2(X13Context& ctx, bool lprmsg, int& inverr, bool lhiddn);

// rgarma.f: THE regARIMA estimation engine. IGLS outer loop -- at each pass the
// regression betas are the GLS solution given the current ARMA parameters
// (olsreg, or y'y when Nb==0), then the ARMA parameters are re-estimated by the
// nonlinear least-squares core lmdif(fcnar). Convergence is on the deviance
// objfcn = a'a*exp(Lndtcv/n) via stpitr. On convergence the ML Var / Lnlkhd are
// formed and the ARMA parameter covariance (Armacm) is built from the optimizer
// QR (fdjac2 -> qrfac -> covar). Cumulative counters Nliter/Nfev accumulate
// across IGLS passes AND across repeated rgarma calls (mxiter is passed to lmdif
// as Nliter+tnlitr). Inputs: lestim (estimate the model), mxiter/mxnlit (max
// cumulative ARMA / nonlinear iterations), lprtit (iteration printing -- the
// prtitr/savitr output is deferred, so this only toggles the deferred hooks).
// a is the PA-length residual work vector, na (out) the residual count, nefobs
// (out) the effective-observation count, lauto (in/out: cleared instead of
// abending when an automatic-modeling pass hits an error). Diagnostic prints and
// the LESTIT iteration-save (savitr) are deferred to the .out/save milestone;
// all estimation numerics and the abend/lauto control flow are reproduced.
void rgarma(X13Context& ctx, bool lestim, int mxiter, int mxnlit, bool lprtit,
            double* a, int& na, int& nefobs, bool& lauto);

// xrlkhd.f: the corrected Akaike information criterion (AICC) for the estimated
// model. The parameter count dnp is Ncxy (regressors + the implicit variance
// column) less any held-fixed regression coefficients (regfx), and aicc =
// -2*(Lnlkhd - n*dnp/(n-dnp-1)) where n = Nspobs-nxcld. Returns the not-set
// sentinel (DNOTST) when the criterion is undefined (Var<=0, not converged, or
// too few effective observations). The commented-out "irregular regression"
// abort in the oracle is dead code and not reproduced.
void xrlkhd(X13Context& ctx, double& aicc, int nxcld);

// armats.f: t-statistics for the estimated ARMA parameters,
// tval(k) = Arimap(lag_k) / sqrt(Var * Armacm(k,k)), walking the AR..MA
// operators in lag order. If the ARMA covariance was flagged singular
// (Armaer==PACSER) it abends (the two-line diagnostic print is reproduced via
// writln). NB (census_bugs.md CB-6): the itv counter advances on EVERY lag,
// fixed or free, while Armacm is packed by the free params only -- so a model
// with a fixed ARMA coefficient misindexes the covariance. Ported verbatim.
void armats(X13Context& ctx, double* tval);

}  // namespace x13

#endif  // X13_REGARIMA_ESTIMATE_HPP
