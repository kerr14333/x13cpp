// transform.hpp -- transform subsystem (M2).
//
// Box-Cox / logit series transformation (trnfcn.f) reached from the pre-model
// phase. Produces the transformed (prior-adjusted) series that feeds regARIMA
// modeling -- table 'trn' (LTRNDT=20).
#ifndef X13_TRANSFORM_TRANSFORM_HPP
#define X13_TRANSFORM_TRANSFORM_HPP

#include "common/x13context.hpp"

namespace x13 {

// trnfcn.f: Box-Cox transform y(1..nsrs) into trny using (fcntyp, lam):
//   fcntyp==3            logit  trny = log(y/(1-y)), y in (0,1)
//   lam==1               identity (copy)
//   lam==0               log,  y>0
//   otherwise            box-cox  trny = lam^2 + (y^lam - 1)/lam,  y>0
// Out-of-domain values raise the same errors and abend the run as the Fortran.
// y and trny may alias (the Fortran calls trnfcn(trnsrs,...,trnsrs)).
void trnfcn(X13Context& ctx, const double* y, int nsrs, int fcntyp, double lam,
            double* trny);

// invfcn.f: the inverse Box-Cox / logit transform of trny(1..nsrs) into y --
//   fcntyp==3            inverse logit  y = e^t/(1+e^t)
//   lam==1 or fcntyp==4  identity (copy)
//   lam==0 or fcntyp==1  exp
//   otherwise            inverse box-cox  y = (lam*(t - lam^2) + 1)^(1/lam)
// In the box-cox branch a non-positive base leaves y(i) UNCHANGED (the Fortran
// only prints a diagnostic there and writes nothing) -- so callers must seed y.
// The diagnostic print is deferred to the .out milestone. y and trny may alias.
void invfcn(X13Context& ctx, const double* trny, int nsrs, int fcntyp,
            double lam, double* y);

// lgnrmc.f: lognormal mean-correction of forecasts. corfac = 0.5*se^2; with
// ltrans the corrected forecast on the ORIGINAL scale is exp(corfac + unc),
// otherwise the still-transformed corrected value corfac + unc. (Applied for the
// log transform so the forecast is the lognormal mean, not the median.)
void lgnrmc(int nfcst, const double* fctunc, const double* fctse,
            double* fctcor, bool ltrans);

}  // namespace x13

#endif  // X13_TRANSFORM_TRANSFORM_HPP
