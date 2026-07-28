// automd.hpp -- automd.f: the automatic model-selection driver (Gomez-Maravall
// TRAMO method). ONE path, as in the Fortran:
//
//   default airline model -> chkmu (mean test) -> residual diagnostics
//   (blpct0/rvr0/rtval0, and tair when !Lidotl) -> acceptdefault -> label 10
//   (ssprep/bkdfmd snapshot, rmfix strips the fixed regressors) -> iddiff
//   (differencing) -> amdid (ARMA orders) -> ismd0 -> put the regressors back
//   (addfix, or restor+a0 when the identified model IS the default) -> label 40
//   -> label 30 finalization (pass0 significance recheck, chkrt1 unit root +
//   redomd, testodf over-differencing, the residual-mean Constant add, tstmd2's
//   insignificant-coefficient drop, autoer) -> label 70.
//
// Label 40 dispatches `IF(Lidotl) amidot ELSE IF(.not.ismd0) tstmd1`. Lidotl is
// true by DEFAULT -- Lotmod (gtinpt.f:238) forces a BIGCV AO scan that finds
// nothing but still selects the amidot arm, keeping the identified model's
// order. `automdl{noautooutlier=tramo}` clears Lotmod and selects tstmd1, which
// reverts to the airline default when the identified model's coefficients are
// not significantly better.
//
// This is a LOOP, not a straight line: `pass2` (:664, see pass2.hpp) can send
// control back to label 10, 40 or 50, and `nloop` counts the passes. Three
// things only matter once it does, and all three were wrong while nloop was
// pinned at 1 -- `lidold` (:167, captured BEFORE the Lotmod override, so label
// 50 does not re-run amdid on a re-entry), the `nloop.eq.1` guards on the
// nbb/a0 revert (:458-467, :503), and the clrotl guard at :472 reading
// `nauto0` rather than `Natotl`.
//
// Deferred: the Lidotl outlier-ID block on the DEFAULT model (:280-321) --
// amidot + pass0 + the nauto0/cvl0 bookkeeping pass2 reads. Closed by the
// BIGCV scan finding nothing, which is why nauto0 stays 0 and cvl0 DNOTST.
// See automd_finalize.hpp for the tail's primitives and
// tools/automdl_scouting.md for the option-by-option state.
#ifndef X13_AUTOMDL_AUTOMD_HPP
#define X13_AUTOMDL_AUTOMD_HPP

#include "common/x13context.hpp"

namespace x13 {

// automd: drive automatic model identification on the transformed series trnsrs
// (a caller-owned buffer, kept distinct from ctx.series.tsrs which rgarma
// overwrites with residuals). Leaves the identified model estimated in ctx
// (arimap/var/lnlkhd/...) and its designation in ctx.arima.bstdsn. a/na are the
// ARMA residual scratch; frstry/nefobs are regvar/estimation outputs.
//
// do_aictest enables the three tdaic/easaic AIC-test blocks for a spec's
// regression{ aictest = ... } selection. In the Fortran those blocks are gated
// on Itdtst/Leastr, which the parser and editor set; this port has not ported
// that setup, so automd_aictest_block1 stands in for it -- which is why the
// LATER blocks are gated on this flag too rather than on Itdtst/Leastr, since
// without block-1 the candidate vectors (Tdayvc/Easvec/Neasvc) are
// uninitialised. Default false because the m4 --afterauto harness drives automd
// as a scaffold and runs the AIC tests itself; only the real pre-model/x11 path
// (run_pre_model) passes true.
void automd(X13Context& ctx, double* trnsrs, int& frstry, int& nefobs,
            double* a, int& na, bool do_aictest = false);

}  // namespace x13
#endif  // X13_AUTOMDL_AUTOMD_HPP
