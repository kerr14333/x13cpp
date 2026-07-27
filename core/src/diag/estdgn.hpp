// estdgn.hpp -- the regARIMA ESTIMATION savelog block: the outlier counts
// (savotl.f) and the ARMA operator ROOTS (prtrts.f).
//
// Both are `.udg` canaries the oracle writes on any model run with `Lsumm > 0`
// -- 287 of the 331 goldens carry the outlier counts and 260 the roots -- and
// neither had any C++ at all. Diagnostic only: nothing downstream reads them.
#ifndef X13_DIAG_ESTDGN_HPP
#define X13_DIAG_ESTDGN_HPP

#include <string>
#include <vector>

namespace x13 {

struct X13Context;

// One row of the roots table: the operator it belongs to plus the root.
struct ArmaRoot {
    std::string filter;    // "ar" / "ma"      (prtrts.f's lower2)
    std::string period;    // "nonseasonal" / "seasonal" -- lowercased title tail
    int factor = 0;        // Oprfac(iopr)
    int index = 0;         // 1-based root number within the operator
    double real = 0.0, imag = 0.0, modulus = 0.0, frequency = 0.0;
};

struct EstDiagnostics {
    bool ran = false;

    // savotl.f:130-152 -- counts by regressor TYPE. `total` is the oracle's
    // `iall`, which is NOT the sum of the others in general: savotl increments
    // it inside each type test, and a regressor matching none of them (a
    // seasonal-outlier variant the counting block excludes) is missed by both.
    int ao = 0, ls = 0, tc = 0, so = 0, rp = 0, tls = 0, user = 0, total = 0;
    int autoout = 0;      // outliers the AUTOMATIC identification found
    bool have_autoout = false;
    bool have_user = false;   // emitted only when Ncusrx > 0

    std::vector<ArmaRoot> roots;
};

// arima.f's estimation savelog point: fill ctx.estdgn from the converged model.
void est_diagnostics(X13Context& ctx, bool lidotl);

}  // namespace x13

#endif  // X13_DIAG_ESTDGN_HPP
