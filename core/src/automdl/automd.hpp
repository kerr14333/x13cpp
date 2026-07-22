// automd.hpp -- automd.f: the automatic model-selection driver (Gomez-Maravall
// TRAMO method). Two paths:
//
// 1. The non-aictest path (do_aictest=false, or an aictest-free spec): a
//    REDUCED port covering the identification spine exercised by automdl
//    specs with no auto-transform / aictest / outlier / pickmdl preamble:
//    build the default airline model, test the mean (chkmu), identify the
//    differencing (iddiff) and ARMA orders (amdid), then re-add the mean and
//    re-estimate. Deferred (documented in automdl_scouting.md): the outlier
//    family (amidot) and pass2 -- both require Lidotl (outlier{} in the
//    spec), unreachable without it.
//
// 2. The aictest path (do_aictest=true and the spec has regression{
//    aictest=... }): a faithful port of automd.f l.322-982 for the Lidotl=F
//    case -- block-1/2/3 tdaic/easaic AIC tests, the a0/ismd0 revert via the
//    oracle's own rmfix/addfix/ssprep/restor (not a ctx snapshot), the
//    tstmd1 model-adequacy loop, and the label-30 finalization tail (pass0
//    final significance recheck, chkrt1 unit-root check + redomd, testodf
//    over-differencing check, tstmd2 insignificant-coefficient drop, autoer).
//    Reaches bit-exact (rtol 1e-8) parity for both ismd0 (identified ==
//    default airline) and non-ismd0 (non-default identified model) series --
//    see automd_finalize.hpp for the primitives. amidot/pass2 (Lidotl-only)
//    remain unreachable: the aictest-x11 corpus has no outlier{} spec.
#ifndef X13_AUTOMDL_AUTOMD_HPP
#define X13_AUTOMDL_AUTOMD_HPP

#include "common/x13context.hpp"

namespace x13 {

// automd (reduced): drive automatic model identification on the transformed
// series trnsrs (a caller-owned buffer, kept distinct from ctx.series.tsrs which
// rgarma overwrites with residuals). Leaves the identified model estimated in
// ctx (arimap/var/lnlkhd/...) and its designation in ctx.arima.bstdsn. a/na are
// the ARMA residual scratch; frstry/nefobs are regvar/estimation outputs.
//
// do_aictest enables the block-1 regressor AIC tests (tdaic/easaic) on the
// default model plus the a0/ismd0 revert around identification (automd.f nloop),
// so a spec's regression{ aictest = ... } selection reaches the estimated model.
// It is gated (default false) because the m4 --afterauto harness drives automd as
// a scaffold and runs the AIC tests itself; only the real pre-model/x11 path
// (run_pre_model) passes true. Reaches parity for both ismd0 (identified ==
// default airline, e.g. airline) and non-ismd0 (non-default identified model,
// e.g. expgs/payems) series via the full label-30 finalization (see
// automd_finalize.hpp).
void automd(X13Context& ctx, double* trnsrs, int& frstry, int& nefobs,
            double* a, int& na, bool do_aictest = false);

}  // namespace x13
#endif  // X13_AUTOMDL_AUTOMD_HPP
