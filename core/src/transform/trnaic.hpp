// trnaic.hpp -- automatic transform selection (transform{function=auto}).
#ifndef X13_TRANSFORM_TRNAIC_HPP
#define X13_TRANSFORM_TRNAIC_HPP

#include "common/x13context.hpp"

namespace x13 {

// trnaic.f: estimate the default airline model twice -- once with NO transform
// (Fcntyp=4, Lam=1) and once with a LOG transform (Fcntyp=1, Lam=0) -- compute
// each model's corrected Akaike information criterion (AICC) via prlkhd, and pick
// the transform with the lower AICC (log wins when aiclog+Traicd < aicno, where
// Traicd is the aicdiff threshold, defaulting to -2 for monthly/quarterly data).
// The chosen Fcntyp/Lam (and Adjmod/Priadj/Adj on the no-transform branch) are
// left in ctx for the subsequent regARIMA/automdl estimation.
//
// This is the reachable transform=auto path: the automatic-model case
// (Lmodel=false with automdl{}, so the default airline model drives the choice)
// with no model span (nbeg=nend=0) and no leap-year prior adjustment (picktd).
// The Fortran's Ixreg xreg toggle, picktd leap-year branches, x11 prior-factor
// re-initialization, and all WRITE/save output are deferred (they touch no state
// on this path) -- see the .cpp for the mapping.
//
// y      : the untransformed span series, Y(Frstsy) (== run_m2's aptr).
// frstsy : 1-based index of the first span observation in Y (unused on the
//          reduced path; retained to mirror the Fortran signature intent).
// nspobs : number of span observations (Nspobs; also drives prlkhd's Jacobian).
// nobspf : span + retained-forecast observation count (regvar/rgarma extent).
// lmodel : whether an explicit ARIMA model was specified (false for automdl).
// aicno  : (out) AICC of the untransformed model (aictest.trans.aicc.nolog).
// aiclog : (out) AICC of the log-transformed model (aictest.trans.aicc.log).
void trnaic(X13Context& ctx, const double* y, int frstsy, int nspobs, int nobspf,
            bool lmodel, double& aicno, double& aiclog);

}  // namespace x13

#endif  // X13_TRANSFORM_TRNAIC_HPP
