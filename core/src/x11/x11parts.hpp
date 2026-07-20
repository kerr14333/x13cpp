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

}  // namespace x13

#endif  // X13_X11_X11PARTS_HPP
