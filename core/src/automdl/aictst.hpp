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
// `lsumm` is the oracle's Lsumm at the call site, and it gates the
// per-candidate AICC savelog table (aictest.td.num / .reg / .reg2 / .aicc.*)
// into ctx.aictest_log. Only arima.f's EXPLICIT aictest path passes it true:
// all five automd.f / automx.f call sites pass a literal 0, which is why a
// pickmdl or automdl golden carries `aictest.td` but never `aictest.td.num`.
void tdaic(X13Context& ctx, double* trnsrs, double* a, int& nefobs, int& na,
           int& frstry, int& tdmdl1, bool ltdlom, bool& lester, bool lsumm);

// easaic.f: Easter AIC test. Estimates the model over the Easvec(1..Neasvc)
// windows (no Easter, then windows 1/8/15 in the default case), keeps the
// lowest-AICC choice in Aicind, and stores the gap in ctx.arima.dfaice. Leaves
// the model rebuilt to the chosen candidate. Prints deferred.
void easaic(X13Context& ctx, double* trnsrs, double* a, int& nefobs, int& na,
            int& frstry, bool& lester, bool lsumm);

// lomaic.f: length-of-month / -quarter / leap-year AIC test. Estimates the
// model with and without the lom/loq/lpyear regressor (per ctx.arima.lomtst =
// 1/2/3), keeps the lower-AICC choice, stores the gap in ctx.arima.dfaicl.
// Leaves the model rebuilt to the chosen form. Prints deferred.
void lomaic(X13Context& ctx, double* trnsrs, double* a, int& nefobs, int& na,
            int& frstry, bool& lester, bool lsumm);

// The refusal helper for the AIC-test front. The NAME is load-bearing:
// walls.py matches an explicit HELPERS tuple with , so a helper it does not
// know about is a wall in neither the gap list nor the count (entry 94).
void aictest_not_ported(X13Context& ctx, const std::string& what);

// usraic.f: the user-defined-regressor AIC test. Estimates the model WITH the
// user regressors (the design as given -- they are always present when this is
// reached), removes every user group, re-estimates, and keeps the lower-AICC
// form; the gap lands in ctx.arima.dfaicu and the two AICCs in
// ctx.aictest_log.user_aicc. Leaves the model rebuilt to the chosen form.
//
// TWO asymmetries against addusr.f are transcribed, not tidied. Its group walk
// omits PRGTUS where addusr.f:34's includes it, so a `usertype=seasonal`
// column is never removed and the test compares a model against itself
// (CB-44); and its restore titles PRGUCY "User-defined Transitory" where
// addusr.f:122 titles the same type "User-defined Cycle".
void usraic(X13Context& ctx, double* trnsrs, double* a, int& nefobs, int& na,
            int& frstry, bool& lester, bool lsumm);

// addlom.f: add a lom/loq/lpyear regressor group (used by lomaic). aicrgm is the
// change-of-regime date (aicrgm[0]==NOTSET for a plain effect); aicln0 the
// regime zero indicator; lnindx = 1/2/3 for lom/loq/lpyear.
void addlom(X13Context& ctx, const int* aicrgm, int aicln0, int sp, int lnindx);

// editor.f:1151-1166 / :1410-1442 -- the AIC-test CANDIDATE vectors (Tdayvc /
// Easvec). The oracle builds these once in the editor; this port has no editor
// block for them, so each caller runs them at its own equivalent point. Call
// aictest_td_vectors only when Itdtst>0 and aictest_eas_vectors only when
// Leastr -- the first can rewrite Itdtst (the isrflw==2 stock-TD promotion).
void aictest_td_vectors(X13Context& ctx);
void aictest_eas_vectors(X13Context& ctx);

// arima.f:569-700 explicit-model AIC regressor test: when an explicit arima{}
// model carries aictest=(...), run the td/lom/easter (user/chi deferred) AIC
// tests in place of the plain rgarma estimate. The aic routines self-estimate.
//
// `lester` is arima.f's local of the same name (:116 `lester=F`): the five AIC
// routines set it when their own estimation fails, and it gates everything the
// arm does afterwards -- including arima.f:723's outlier identification, which
// sits outside the :569/:701 if-else and so belongs to the CALLER. It was a
// local here and discarded, which is the "argument computed and dropped" shape:
// the caller could not honour :723's `.not.lester` because it never saw it.
void explicit_aictest(X13Context& ctx, double* trnsrs, double* a, int& nefobs,
                      int& na, int& frstry, bool& lester);

}  // namespace x13

#endif  // X13_AUTOMDL_AICTST_HPP
