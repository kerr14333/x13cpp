// automd_finalize.hpp -- primitives for automd.f's l.360-982 finalization
// control flow: the a0/ismd0 save-strip-revert machinery (ssprep/restor/rmfix/
// addfix), the final AIC significance recheck (pass0 + its tstdrv helper), and
// the trivial automatic-outlier-regressor sweep (clrotl) and estimation-warning
// gate (autoer). automd.cpp is the only caller; split out here because each is
// a faithful, self-contained port of its own oracle/fortran/*.f file.
#ifndef X13_AUTOMDL_AUTOMD_FINALIZE_HPP
#define X13_AUTOMDL_AUTOMD_FINALIZE_HPP

#include "common/x13context.hpp"

namespace x13 {

// ssprep.f (Lmodel=T, Lx11=F, Lx11rg=F branch only -- the only call shape
// automd.f uses): snapshot the model-defining commons into ctx.ssprep, paired
// with restor_model below. Distinct storage from bkdfmd's ctx.ss2rv (used by
// tstmd1's default-model revert) -- automd.f keeps both snapshots alive
// simultaneously for different purposes.
void ssprep_save(X13Context& ctx);

// restor.f (Lmodel=T, Lx11=F, Lx11rg=F branch only): the ssprep_save inverse.
// NB (bug-for-bug with the oracle): ssprep saves Arimaf as PARIMA elements into
// Fxa, but restor restores only the first PB of them -- an asymmetry in the
// vendored source, reproduced exactly for bit-exactness.
void restor_model(X13Context& ctx);

// rmfix.f: Fxindx=2 strips ALL regressors (Fxindx=1's fixed-only mode is not
// used by automd.f and not implemented). Builds the fxreg dictionary
// (Cfxttl/Bfx/Fxtype/Fixind/Grpfix) that addfix below reads to restore them,
// and subtracts the combined regressor effect (skipping a Constant) from
// trnsrs via Fixfc2. The user-regressor dlusrg branch is UNPORTED (fails loud
// via abend) -- unreachable for the td/easter/Constant aictest corpus.
void rmfix(X13Context& ctx, double* trnsrs, int nbcst, int nrxy, int fxindx);

// addfix.f: the rmfix inverse -- re-adds the fixed regressors (via adrgef) and
// the Fixfc2 effect back into trnsrs. The user-regressor addusr branch is
// UNPORTED (fails loud) -- unreachable for the td/easter/Constant corpus.
void addfix(X13Context& ctx, double* trnsrs, int nbcst, int rind, int fxindx);

// tstdrv.f: a "derived" combined t-statistic for a regressor group (used by
// pass0 when no individual coefficient in the group is itself significant).
double tstdrv(X13Context& ctx, int igrp);

// rmlpyr.f: remove a leap-year/length-of-month PRIOR adjustment (as opposed to
// a regressor) from the series once pass0 drops the TD regressor it was
// riding along with (Picktd=T). Un-log/un-Box-Cox the series, divide out the
// stored prior factor, re-multiply by the freshly computed length-of-month
// factor, re-transform, and update the stored Adj/Sprior factors + Priadj/
// Kfmt/Lpradj bookkeeping to match.
void rmlpyr(X13Context& ctx, double* trnsrs, int nobspf);

// pass0.f: final significance recheck of the TD / Easter / Constant regressors
// on the (near-)final model; deletes any that fail the cval=1.96 (or Tsig when
// istep==1) threshold and regenerates the regression matrix. isig accumulates
// how many were dropped.
void pass0(X13Context& ctx, double* trnsrs, int& frstry, int& isig, int istep,
           bool lprt);

// clrotl.f: remove automatically identified outlier (AO/LS/TC) regressors.
// Always a no-op for the aictest-x11 corpus (Natotl stays 0 -- no outlier{}
// spec, so Lidotl=F and automatic outlier ID never runs); ported faithfully
// for correctness in case a future caller has Natotl>0.
void clrotl(X13Context& ctx, int nrxy);

// autoer.f: abend on select fatal ARMA-estimation warning codes (root/GPG/ACF/
// var(w) failures) left in ctx.mdldat.armaer; a no-op otherwise. Message
// prints are deferred (writln), matching the rest of the print engine.
void autoer(X13Context& ctx, int info);

}  // namespace x13
#endif  // X13_AUTOMDL_AUTOMD_FINALIZE_HPP
