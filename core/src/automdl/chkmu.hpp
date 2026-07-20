// chkmu.hpp -- chkmu.f / genrtt.f: the automatic mean (Constant) test that runs
// on the default model before iddiff/amdid. chkmu adds a Constant regressor,
// estimates, and keeps it iff its t-statistic (genrtt) exceeds the threshold, so
// the ARMA-order search sees the correct regression set.
#ifndef X13_AUTOMDL_CHKMU_HPP
#define X13_AUTOMDL_CHKMU_HPP

#include "common/x13context.hpp"

namespace x13 {

// genrtt.f: regressor t-statistics tval(1..Nb) = B/se, where se comes from the
// packed inverse X'X (dppdi) scaled by the residual RMSE. Fixed regressors
// (regfx, when iregfx>=2) get t=0 and are skipped in the inverse indexing.
void genrtt(X13Context& ctx, double* tval);

// chkmu.f: add a Constant regressor if not present, rebuild the regression
// matrix, estimate (rgarma), and test the constant's t-statistic against 1.96
// (kstep 0) or 1.6 (kstep 1). If not significant (or non-convergent) the
// constant is removed and the matrix rebuilt. Leaves the model estimated with
// the constant present iff the mean is significant.
void chkmu(X13Context& ctx, double* trnsrs, double* a, int& nefobs, int& na,
           int& frstry, int kstep, bool lprt);

}  // namespace x13
#endif  // X13_AUTOMDL_CHKMU_HPP
