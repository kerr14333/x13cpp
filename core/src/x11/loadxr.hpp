// loadxr.hpp -- loadxr.f: swap the regression model between the regARIMA store
// (ctx.model/mdldat/arima/prior/...) and the x11-regression store (ctx.xrgmdl).
//
// x11regression{} variables are parsed into their OWN model store, kept out of
// the regARIMA ML estimate (which must stay the bare ARIMA model -- see the
// oracle's "these statistics do not contain a penalty for parameters estimated
// by x11regression" note). loadxr(false) loads the x11reg regressors into the
// working arrays for x11mdl; loadxr(true) saves the working arrays back. See
// gtinpt.f:804-834 for the parse-time ssprep/dlrgef/gtxreg/loadxr(T)/restor
// sequence and x11pt2.f:720/724 for the per-iteration swap.
#ifndef X13_X11_LOADXR_HPP
#define X13_X11_LOADXR_HPP

namespace x13 {

struct X13Context;

// Clear the working regression model's regressors (gtinpt.f:805-816 dlrgef path;
// a no-op when Nb==0, as on the bare-ARIMA path).
void xrg_clear_working(X13Context& ctx);

// loadxr.f: Toxreg=true saves the working model into ctx.xrgmdl; false loads
// ctx.xrgmdl into the working model (and applies the ARIMA-operator resets).
void loadxr(X13Context& ctx, bool toxreg);

}  // namespace x13
#endif  // X13_X11_LOADXR_HPP
