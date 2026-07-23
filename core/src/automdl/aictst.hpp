// aictst.hpp -- the automatic-model-selection AIC-test regressor family.
//
// Ported from oracle/fortran: tdaic.f (trading-day AIC test), easaic.f (Easter
// AIC test), and the two regressor-construction helpers addtd.f / addeas.f they
// depend on. These routines decide whether to KEEP a trading-day / Easter
// regressor group in the automatic (automdl) model: each estimates the model
// WITH and WITHOUT the group via rgarma, computes each fit's AICC via prlkhd,
// and keeps the group iff it lowers AICC by more than the threshold Rgaicd.
//
// The decision and the AICC difference land in ctx.arima:
//   - tdaic  -> Dfaict (aictest.diff.td), Aicint (chosen TD index; 0 = none).
//   - easaic -> Dfaice (aictest.diff.e),  Aicind (chosen Easter window; <0 none).
//
// STANDALONE (M4): these are NOT yet wired into automd (the main thread owns
// automd.cpp). They are delivered as callable routines plus an oracle-gated
// harness (tools/x13run_iddiff --aictest) that reproduces the automd preamble
// and drives them on the aictest corpus specs.
//
// Reuses the ported engine: rgarma, regvar, prlkhd, adrgef, dlrgef, strinx,
// td7var, eltfcn, trnfcn, copy/setdp/setlg. Deferred (with the .out/save
// milestone): every WRITE/savelog/summary line, and the mktdlb/mkealb label
// builders (used only by those prints; the group titles strinx searches on are
// written by addtd/addeas).
//
// xrlkhd.f (the x11regression AICC variant that drops prlkhd's constant Jacobian
// adjustment) is already ported in core/src/regarima/estimate.cpp and is used by
// the x11regression path, not by tdaic/easaic -- those call prlkhd. It is NOT
// reproduced here.
//
// chkchi.f (chi-square group-significance test for user-defined holiday
// regressors) is DEFERRED: its callees chitst/dlusrg/savchi are not yet ported
// and no corpus spec exercises it. See tools/FABLE_REVIEW.md.
#ifndef X13_AUTOMDL_AICTST_HPP
#define X13_AUTOMDL_AICTST_HPP

#include "common/x13context.hpp"

namespace x13 {

// addtd.f: add the trading-day regressor group named by tdindx to the model for
// an AIC test. For the no-regime, no-stock default case (aicrgm[0]==NOTSET) this
// is a plain adrgef of the six weekday contrasts (or the one weekday coefficient
// for the td1coef family). Reuses adrgef.
void addtd(X13Context& ctx, int aicstk, const int* aicrgm, int aictd0, int sp,
           int tdindx);

// addeas.f: add the Easter regressor with window keastr to the model for an AIC
// test. Reuses adrgef.
void addeas(X13Context& ctx, int keastr, int easidx, int eastst);

// tdaic.f: trading-day AIC test. Estimates the model over the Tdayvc(1..Ntdvec)
// candidates (0 = no TD, then the TD variants), keeps the lowest-AICC choice in
// Aicint, and stores the no-TD-vs-best AICC gap in ctx.arima.dfaict. Leaves the
// model rebuilt to the chosen candidate. Prints deferred (lprt is honoured only
// to skip the deferred output; pass false).
void tdaic(X13Context& ctx, double* trnsrs, double* a, int& nefobs, int& na,
           int& frstry, int& tdmdl1, bool ltdlom, bool& lester);

// easaic.f: Easter AIC test. Estimates the model over the Easvec(1..Neasvc)
// windows (no Easter, then windows 1/8/15 in the default case), keeps the
// lowest-AICC choice in Aicind, and stores the gap in ctx.arima.dfaice. Leaves
// the model rebuilt to the chosen candidate. Prints deferred.
void easaic(X13Context& ctx, double* trnsrs, double* a, int& nefobs, int& na,
            int& frstry, bool& lester);

// lomaic.f: length-of-month / -quarter / leap-year AIC test. Estimates the
// model with and without the lom/loq/lpyear regressor (per ctx.arima.lomtst =
// 1/2/3), keeps the lower-AICC choice, stores the gap in ctx.arima.dfaicl.
// Leaves the model rebuilt to the chosen form. Prints deferred.
void lomaic(X13Context& ctx, double* trnsrs, double* a, int& nefobs, int& na,
            int& frstry, bool& lester);

// addlom.f: add a lom/loq/lpyear regressor group (used by lomaic). aicrgm is the
// change-of-regime date (aicrgm[0]==NOTSET for a plain effect); aicln0 the
// regime zero indicator; lnindx = 1/2/3 for lom/loq/lpyear.
void addlom(X13Context& ctx, const int* aicrgm, int aicln0, int sp, int lnindx);

// arima.f:569-700 explicit-model AIC regressor test: when an explicit arima{}
// model carries aictest=(...), run the td/lom/easter (user/chi deferred) AIC
// tests in place of the plain rgarma estimate. The aic routines self-estimate.
void explicit_aictest(X13Context& ctx, double* trnsrs, double* a, int& nefobs,
                      int& na, int& frstry);

}  // namespace x13

#endif  // X13_AUTOMDL_AICTST_HPP
