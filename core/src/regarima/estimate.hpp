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

}  // namespace x13

#endif  // X13_REGARIMA_ESTIMATE_HPP
