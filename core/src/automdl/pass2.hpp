// pass2.hpp -- pass2.f: automd.f's second-pass model comparison and the nloop
// re-entry decision. Called at automd.f:664 under `IF(Lidotl.and.nloop.le.2)`
// -- and NOTHING else, so with the default Lotmod (which forces Lidotl on via
// the BIGCV AO scan) the oracle runs this on every automdl spec. It is a no-op
// on the corpus's default configurations, which is why the port was green
// without it; it is NOT gated on an outlier scan finding anything.
//
// Two halves, both of which can change the reported model:
//
//  1. `ichk` (pass2.f:47-150) -- compare the IDENTIFIED model against the
//     DEFAULT airline on Ljung-Box confidence (`plbox`) and residual variance
//     (`rvr`), through six accept-the-default tests. If any fires, the default
//     is reinstated: mdlset back to it, bkdfmd(false) to restore the fitted
//     values, regvar + rgarma, and the caller's order/`lmu`/`a` variables are
//     overwritten from the `*0` copies.
//
//  2. `Pcr` + `igo` (pass2.f:160-330) -- the acceptance limit is RAISED
//     (+0.025 on the first pass, +0.015 after; this is the second consumer of
//     `automdl{ljungboxlimit=}`), and if the surviving model's `plbox` still
//     exceeds it the whole identification is redone: `igo` 1 sends the caller
//     back to label 10, 2 to label 40, 3 to label 50. When no re-entry is
//     allowed the oracle instead installs a MODEL OF LAST RESORT --
//     (3, idr, iqr, ips, ids, iqs) with the AR order walked down until the
//     estimation converges -- and returns igo=2.
#ifndef X13_AUTOMDL_PASS2_HPP
#define X13_AUTOMDL_PASS2_HPP

#include "common/x13context.hpp"

namespace x13 {

// pass2.f. `i*` are the identified model's orders and `l*` the default's; both
// sets, plus lmu/lmu0, plbox/rvr, naut0, na, a, ismd0, nloop, nround, are
// updated in place exactly as the Fortran's by-reference arguments are.
// `fct2` is automd.f:183-184's `1.0`, or `Fct` when an adjustment was asked
// for. Returns via `igo`: 0 = fall through to label 30's tail, 1/2/3 = re-enter
// at label 10/40/50.
void pass2(X13Context& ctx, double* trnsrs, int& frstry, int& ipr, int& idr,
           int& iqr, int& ips, int& ids, int& iqs, int& lpr, int& ldr,
           int& lqr, int& lps, int& lds, int& lqs, int& naut, int& naut0,
           double& plbox, double& plbox0, int& bldf, int bldf0, double& rvr,
           double& rvr0, bool& lmu, bool& lmu0, double* a, const double* a0,
           int& na, int na0, int aici0, bool pcktd0, int aicit0,
           const double* adj0, const double* trns0, double fct2, bool& ismd0,
           double* cvl0, int& nefobs, int& nloop, int& nround, int& igo);

}  // namespace x13
#endif  // X13_AUTOMDL_PASS2_HPP
