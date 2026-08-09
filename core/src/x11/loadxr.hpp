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

#include <string>
#include <vector>

namespace x13 {

struct X13Context;

// Clear the working regression model's regressors (gtinpt.f:805-816 dlrgef path;
// a no-op when Nb==0, as on the bare-ARIMA path).
void xrg_clear_working(X13Context& ctx);

// The DESIGN half of restor.f, which this port has no general stand-in for.
//
// Both places that build the x11regression model do it in the WORKING arrays and
// then hand them to loadxr(T) -- gtinpt.f:804-832 at parse and xrgdrv.f:129-207
// per run -- and in both the oracle brackets the block with ssprep/restor, so the
// regARIMA design that was there before comes back. `ssprep.f:64-83` is the field
// list (Ngrp/Ngrptl/Ncxy/Nb/Ncoltl/Colttl/Grpttl and the pointer, type,
// coefficient and fix arrays under them, plus Nrxy/Iregfx/Ncusrx/Nrusrx/Picktd's
// Fulltd); `restor_span` in this port carries only the x11 filter state.
//
// Until 2026-08-09 both sites CLEARED the design instead (`xrg_clear_working`),
// which is the same thing when it was empty -- and it is empty on every spec that
// has an x11regression{} and no regression{}, which was the whole corpus. With
// both, the clear deleted the regARIMA regressors outright: `nreg: 0` where the
// oracle reports 2.
struct model_design_backup {
    int ngrp, ngrptl, ncxy, nb, ncoltl, iregfx, nrxy, ncusrx, nrusrx;
    bool fulltd;
    std::string colttl, grpttl, usrttl;
    std::vector<int> colptr, grp, grpptr, rgvrtp, usrtyp, usrptr;
    std::vector<char> regfx;      // vector<bool> is not a container
    std::vector<double> b;
};

model_design_backup capture_model_design(const X13Context& ctx);
void restore_model_design(X13Context& ctx, const model_design_backup& bak);

// loadxr.f: Toxreg=true saves the working model into ctx.xrgmdl; false loads
// ctx.xrgmdl into the working model (and applies the ARIMA-operator resets).
void loadxr(X13Context& ctx, bool toxreg);

}  // namespace x13
#endif  // X13_X11_LOADXR_HPP
