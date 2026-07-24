// xrgdrv.hpp -- xrgdrv.f: the OLS-estimated prior trading-day / holiday driver
// for x11regression{} when a regARIMA model is present (Ixreg>=2).
//
// When a model is estimated AND an irregular-component x11regression was
// requested, the oracle promotes Ixreg 1->2 (gtinpt.f:1201) and, BEFORE the
// model + the main X-11 run (x11ari.f:93), runs a transparent (model-free,
// no-forecast/backcast) X-11 decomposition whose x11pt2 x11mdl OLS estimates the
// TD daily weights on the irregular and builds the prior TD factor Faccal. The
// model is then fit on the prior-TD-adjusted series (x11pt1.f:180 divides Sto by
// Faccal), and the main X-11 run proceeds with Ixreg==3.
//
// The C++ estimates the regARIMA model in an earlier phase (run_pre_model) than
// the main X-11 run, so this transparent estimation is hoisted ahead of the model
// estimate: it is self-contained (its own model-free span/pointer/buffer/x11int
// setup, mirroring run_x11.cpp's no-model path with Nfcst==Nbcst==0), leaves the
// estimated Faccal in ctx.x11_faccal_prior (observed span) for the pre-model
// divide and the main x11pt1 Ixreg==3 fold, and sets Ixreg=3 on return.
//
// TD-only path (Kswv==0, Khol!=1, multiplicative): the Picktd/Priadj LOM
// suppression + loadxr TD swap + Adj* indicator reset are ported; the classic
// X-11 Easter (Khol==1) holday branch and the x11regression span (Xdsp>0) stay
// deferred and fatal cleanly.
#ifndef X13_X11_XRGDRV_HPP
#define X13_X11_XRGDRV_HPP

namespace x13 {

struct X13Context;

// Run the transparent-SA prior-TD estimation. On success leaves Faccal in
// ctx.x11_faccal_prior over the observed span and sets ctx.hiddn.ixreg=3.
// Returns false (ctx.error.lfatal set) on any unported sub-branch or numeric
// fatal. A no-op returning true when Ixreg<2 / Axrgtd is false.
bool xrgdrv(X13Context& ctx);

}  // namespace x13
#endif  // X13_X11_XRGDRV_HPP
