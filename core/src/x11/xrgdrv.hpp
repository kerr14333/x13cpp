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
//
// `span_mode` selects the CALL SITE this is standing in for:
//   false -- the hoisted main-run call from run_pre_model. Nothing has set up
//            the X-11 span yet, so this builds its own (setxpt, Frstsy/Nomnfy,
//            Lsp, the Series/Orig buffer fill).
//   true  -- the per-span call from run_x11_span, which IS x11ari.f:88-95's
//            call site: the caller has already set the pointers, the calendar
//            span and the input buffers, so this reproduces xrgdrv.f:134-150's
//            in-place Nfcst/Nbcst zeroing + pointer nudge instead and puts them
//            back at :167-178. It also save/restores the ssprep snapshot, which
//            the oracle does not need (its revdrv `restor` immediately precedes
//            this ssprep, so the snapshot is rewritten with the state it was
//            just restored from) but this port does, because ctx.saved carries
//            three non-oracle fields (ksdev0/lterm0/nterm0) that the span loop
//            reads back.
//
// `at_x11ari` says this call sits at x11ari.f:88-95's own point in time rather
// than being hoisted ahead of the model stage. Only the NO-MODEL main-run call
// (x11_prestage) sets it: with no model there is no estimation input to divide,
// so nothing has to run early, and x11_prestage issues it exactly where x11ari
// does -- after x11int, before x11pt1. The flag suppresses the Ksdev restore,
// which exists solely to compensate for the hoist (see xrgdrv.cpp).
bool xrgdrv(X13Context& ctx, bool span_mode = false, bool at_x11ari = false);

}  // namespace x13
#endif  // X13_X11_XRGDRV_HPP
