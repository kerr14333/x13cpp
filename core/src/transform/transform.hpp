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

}  // namespace x13

#endif  // X13_TRANSFORM_TRANSFORM_HPP
