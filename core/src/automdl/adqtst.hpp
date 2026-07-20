// adqtst.hpp -- model-adequacy tests for automatic model selection (automd.f's
// finalization). This module will grow to hold tstmd1 (Ljung-Box adequacy),
// tstmd2 (unit-root nearness), and testodf (over-differencing); it starts with
// mdlchk, the shared Ljung-Box + residual-mean statistic used by all three and
// by automd's redo path.
#ifndef X13_AUTOMDL_ADQTST_HPP
#define X13_AUTOMDL_ADQTST_HPP

#include "common/x13context.hpp"

namespace x13 {

// mdlchk.f: Ljung-Box diagnostics for the current model's residuals a(1..na).
// Chooses the LB lag count bldf from the seasonal period (24 monthly, 8 for
// Sp==1, 4*Sp otherwise; a small-sample quarterly special case), capping it at
// nefobs/2 when there are too few observations, then runs acf over the residual
// tail to fill ctx.autoq. Returns blpct = 1 - Qpv(bldf), blq = Qs(bldf), the
// residual-mean t-value rtval, and rvr = sqrt(Var).
void mdlchk(X13Context& ctx, const double* a, int na, int nefobs, double& blpct,
            double& blq, int& bldf, double& rvr, double& rtval);

}  // namespace x13
#endif  // X13_AUTOMDL_ADQTST_HPP
