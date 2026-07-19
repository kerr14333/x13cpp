// estimate.hpp -- the regARIMA IGLS estimation driver (rgarma.f) and its
// regression-solve helpers olsreg.f / resid.f. These sit above the exact ARMA
// filter (armafl.hpp) and drive the MINPACK optimizer. Routines that only touch
// plain array arguments still take X13Context& first because their error guards
// call abend/errhdr/writln through it.
#ifndef X13_REGARIMA_ESTIMATE_HPP
#define X13_REGARIMA_ESTIMATE_HPP

#include "common/x13context.hpp"

namespace x13 {

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

}  // namespace x13

#endif  // X13_REGARIMA_ESTIMATE_HPP
