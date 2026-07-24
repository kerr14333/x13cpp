// x11reg.hpp -- x11regression{} irregular-component regression (x11mdl.f et al).
//
// Wired into x11pt2 (x11mdl_td runs at the B/C iterations; gates bit-exact).
// Ports the multiplicative path of the X-11 irregular regression: the user's
// trading-day (and AIC-tested Easter) regressors are OLS-fit to the X-11
// irregular (Sti) at the B (Kpart=2) and C (Kpart=3) iterations, producing the
// b16/c16 TD-factor tables + the xrm design, and the calendar effect is divided
// out of the irregular so the seasonal adjustment re-iterates without it.
// Automatic AO outlier identification on the irregular runs via the shared
// idotlr (lxreg path). tdprior (Kswv=1) and OLS prior-TD (xrgdrv) also land here.
//
// Faithful to x11mdl.f (orchestration) + tdset/xrgtrn/tdxtrm/dlrgrw/regx11/
// x11ref/mulref/x11aic. Reuses the already-ported olsreg/resid/regvar/daxpy/
// idotlr/xrlkhd/addeas. See tools/x11regression_scope.md +
// x11regression_aictest_scope.md for the routine maps. Still follow-on:
// stock-TD/pseudo-additive/log-additive modes, aictest td/user variants.
#ifndef X13_X11_X11REG_HPP
#define X13_X11_X11REG_HPP

namespace x13 {

struct X13Context;

// tdset.f: fill the trading-day calendar quantities Tday / Xn / Xnstar / Xlpyr /
// Daybar over the 1-based span [lfda, llda] anchored at date begdat (sp period).
// Xn = actual days in the month-type; Xnstar = standardized (Feb=28.25); Xlpyr =
// Xn-Xnstar; Daybar = 30.4375 (monthly) / 91.25 (quarterly).
void tdset_td(X13Context& ctx, const int* begdat, int lfda, int llda, int sp);

// xrgtrn.f (mult, Tdgrp>0, Kswv=0): transform the copied irregular in place over
// the 1-based absolute span [l1,l2] (repacked to x[1..]): x = Xnstar*x - Xn.
void xrgtrn_td(X13Context& ctx, double* x, int l1, int l2);

// tdxtrm.f: two-pass sigma test on the RAW irregular Sti over [irridx,irrend];
// flags extreme rows into ctx.xclude.rgxcld (1-based, i-irridx+1) and Nxcld.
// Kpart=2 uses per-month-type means; Kpart=3 uses the B-iteration Faccal.
void tdxtrm_td(X13Context& ctx, const double* sti, double sigm, int kpart,
               int irridx, int irrend);

// dlrgrw.f: compact the Nxcld excluded rows out of the row-major [X:y] matrix xy
// (ncxy columns, nrxy rows) using the 1-based rgxcld flags.
void dlrgrw(double* xy, int ncxy, int nrxy, const bool* rgxcld);

// regx11.f: OLS of the (transformed) irregular design in ctx.mdldat.xy on the TD
// columns, with the tdxtrm-excluded rows dropped. Fills ctx.mdldat.b (coeffs) +
// chlxpx; sets Var/Lnlkhd/Armaer. Reuses the ported olsreg/resid. Returns false
// on a singular column (Armaer=PSNGER). When aout is non-null, the OLS residuals
// (nrtxy = nspobs - nxcld of them) are copied out with *naout = *nefout = nrtxy
// less nintvl -- the residual/effective-obs count idotlr's x11reg path needs.
bool regx11(X13Context& ctx, double* aout = nullptr, int* naout = nullptr,
            int* nefout = nullptr);

// x11ref.f (mult, TD-only): build the TD factor series ftd (and combined fcal)
// from the fitted coeffs b x design xy over Nrxy rows, mean-normalized by Xnstar
// (mulref) then finished with Xn/Xnstar. xdev = Pos1bk. rtype[icol] is the
// per-column regressor type. Both ftd/fcal are 1-based length-Nrxy outputs.
void x11ref_td(X13Context& ctx, double* fcal, double* ftd, int xdev, int nrxy,
               int ncxy, const double* b, const double* xy, int nb,
               const int* rtype);

// pritd.f (Kswv=1 user-weight prior trading day): build the prior-TD factors from
// the seven tdprior weights (ctx.x11reg.dwt) via td6var + x11ref_td. begdat is the
// series-origin date (Begbk2), frstob = Pos1bk. Fills ptdfac at absolute positions
// [frstob, frstob+nrxy-1]. Requires xtdtyp populated (tdset_td) over that span.
void pritd(X13Context& ctx, double* ptdfac, int nrxy, int sp, const int* begdat,
           int frstob);

// x11mdl.f orchestration (TD-only mult path): regress the X-11 irregular Sti on
// the TD design at the B (kpart=2) or C (kpart=3) iteration, snapshot the TD
// factors into ctx.x11reg_b16/c16 (b16/c16), and divide the TD effect out of
// Sti so x11pt2 re-iterates without it. Called from x11pt2 when ctx.hiddn.ixreg
// == 1 (x11regression{} present, no prior).
void x11mdl_td(X13Context& ctx, int kpart);

}  // namespace x13

#endif  // X13_X11_X11REG_HPP
