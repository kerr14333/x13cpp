// x11parts.hpp -- the X-11 decomposition "parts" spine (Tier 6): x11pt1 (prior
// adjustments -> table B1), and (to come) x11pt2 (B1->D7), x11pt3 (D8->D16),
// x11pt4 (E1->F4), and the x11ari partition driver. These consume the modeled +
// forecast-extended series from the regARIMA stage and drive the Tier 0-5 leaves/
// drivers. ctx-first; all COMMON state lives on X13Context. Printing/save is
// deferred (dropped); feature branches that need still-unported routines fatal
// cleanly via the shared error path.
#ifndef X13_X11_X11PARTS_HPP
#define X13_X11_X11PARTS_HPP

namespace x13 {

struct X13Context;

// x11pt1.f: pre-adjustments (Part A -> B1 input). Copies the observed/modeled
// series into the X-11 working buffers (Stcsi/Sto/Stoap/Stopp/Stocal), applies
// any user prior-adjustment (Sprior), prior calendar/holiday (Faccal/X11hol), and
// prior trading-day factors, then sets Stcsi to the prior-adjusted series. lmodel
// gates only deferred pre-ARIMA prints; lgraf/lgrfxr are deferred graph/save
// flags. The prior-TD / x11-regression-TD branch (Kswv!=0 or Ixreg>=2 & Axrgtd)
// needs the unported pritd/ssrit and fatals if reached.
void x11pt1(X13Context& ctx, bool lmodel, bool lgraf, bool lgrfxr);

// x11pt2.f: X-11 PARTS B1->D7 -- the iterated B/C/D moving-average decomposition.
// Consumes the B1 input Stcsi (from x11pt1) and runs Kpart = 2/B, 3/C, 4/D of the
// classic X-11 kernel, returning at D7. Base decomposition path only: the model-
// based prior-adjustment / factor preamble (makadj/tdlom/ssrit + the x11-
// regression option) stays not_ported and fatals cleanly if a spec activates it;
// all table/punch/x11plt/ftest output is deferred. lx11 gates the early return
// when no X-11 options are requested; lmodel/lseats/lgraf/lgrfxr feed only
// deferred branches.
void x11pt2(X13Context& ctx, bool lmodel, bool lx11, bool lseats, bool lgraf,
            bool lgrfxr);

// x11pt3.f: X-11 PARTS D8->D16 -- the finals. Consumes the D7 trend/seasonal
// left by x11pt2 and produces the final seasonal (D10=Sts), final SA (D11=Stci),
// final trend (D12=Stc), final irregular (D13=Sti), the combined factors
// (D16=ststd), the unmodified/modified SI (D8/D9), and the Part-E modified series
// (E1/E2/E3). Base decomposition path only (Muladd==0 mult, Kfulsm==0, Ksdev==1,
// Psuadd=F, no priors/TD/holiday/outliers/forcing/constant): every gated-off
// feature branch (Kfulsm==2/1, Psuadd, Adjsea/Adjso/Adj*, Ishrnk, holiday/TD
// combine, rmpadj, Iyrt force, ssrit, getrev, constant removal, logadd) fatals
// cleanly via not_ported; the D8 F/M diagnostics (ftest/kwtest/mstest/COMBFT)
// and the residual-seasonality ftest are deferred no-ops. lgraf gates only
// deferred graph saves; lttc feeds only gated-off (temporary-change) branches.
void x11pt3(X13Context& ctx, bool lgraf, bool lttc);

// chktrn.f: multiplicative-mode trend-positivity check/repair, called from x11pt2
// (Muladd==0). Replaces any non-positive value in the trend-cycle stc in place
// (mean of nearest positive neighbours, or the nearest positive value at a series
// end) and returns oktrn (all values positive over the [Pos1ob,last] core span).
// tstfct is in/out (reset to false when no repair is needed). The Fortran
// Kpart/Ktabl/Trnchr args were print-only and are dropped (deferred-print).
bool chktrn(X13Context& ctx, double* stc, bool& tstfct);

}  // namespace x13

#endif  // X13_X11_X11PARTS_HPP
