// run_x11_span.hpp -- the re-entrant span-window driver: THE architectural
// piece slidingspans{} was blocked on (see tools/slidingspans_scope.md S4).
//
// run_x11() only knows how to parse-and-run a spec text end-to-end once. This
// function factors out "replay the model+X11 pipeline over an in-memory
// sub-span of the already-parsed/estimated ctx" as its own reviewable, reusable
// entry point -- shared by slidingspans{} (ssx11a.f + x11ari, called once per
// sliding span) and, later, history{} (revdrv.f's per-cutoff re-run: same
// underlying need).
//
// Scope: the regARIMA model is replayed with its coefficients HELD FIXED at
// the main run's already-converged values (ctx.model.arimaf all true --
// mirrors ssmdl.f's Ssinit==1 "fixmdl=yes" default, which fixes the whole
// model). rgarma still runs (recomputing residuals/likelihood over the span's
// shorter window) but with zero free parameters (Nestpm==0) it takes exactly
// one pass and never perturbs ctx.mdldat.arimap -- this is the well-tested
// "all-fixed" branch already exercised by fixed-coefficient arima{} specs.
// Regression columns (Nb>0) are threaded through generically (mirroring
// run_x11.cpp's model-path tail) but only the Nb==0 path (the slidingspans
// gate corpus: no regression{}, no outlier{}, no TD) is validated here.
#ifndef X13_DRIVER_RUN_X11_SPAN_HPP
#define X13_DRIVER_RUN_X11_SPAN_HPP

#include <vector>

namespace x13 {

struct X13Context;

// trnsrs_full: the FULL clean transformed series as produced once by the main
// run's run_m2_after_parse (indexed so trnsrs_full[k] holds the transform of
// Y(1+k), the same 0-based absolute offset convention as ctx.arima.y) -- the
// caller (run_x11, which already has this as a local) passes it straight
// through; nothing here recomputes the transform or re-touches prior
// adjustment.
//
// nlen/nfcst/nbcst/nbcst2/lsp: the padded-buffer window (ssx11a.f's Nspobs/
// Nfcst/Nbcst/Nbcst2/Lsp -- ssap.cmn/extend.cmn/lzero.cmn). Mirroring
// ssx11a.f's own dependency order, the span's calendar Begspn/Endspn are NOT
// a caller input: they are DERIVED here from setxpt's resulting Pos1ob/Posfob
// (via ctx.x11opt.lyr/ny, the ctx-global calendar anchor) plus ctx.ssap.im
// (the setssp-computed constant first-month-of-span-1, unchanged across
// spans). This lets the caller describe a span purely as "how many
// observations, staggered how far via Lsp" without duplicating date
// arithmetic. Pass lsp=1, nbcst=nbcst2=0, nlen=<the main run's own Nspobs> to
// reproduce the main/full-span run exactly -- the step-2 sanity check in
// tools/slidingspans_scope.md S5.
//
// has_model selects the regARIMA replay; when false the padded buffer is just
// the observed sub-span (mirrors run_x11's no-model path).
//
// Leaves results on ctx.x11srs/ctx.orisrs exactly like run_x11 does for the
// main run (b1/d10-d13) -- the caller reads them back (or lets the ssrit
// capture sites in x11parts.cpp, gated on ctx.hiddn.issap==2, do it) before
// the next call overwrites them.
// nend_mdl (default 0): history{}'s FIXED-PERIOD model estimation (Fixper, from
// series{modelspan=(,0.per)} -- revdrv.f:481-489). The span's regARIMA model is
// estimated only through Endmdl = the last occurrence of period Fixper at or
// before the span end, i.e. `nend_mdl` periods short of Endspn, and the span END
// is then put back (setspn.f) before the forecasts and X-11 run. This is
// arima.f:134-157 + arima.f:1145's narrowing, restricted to the nbeg==0 case
// (revdrv never moves Begmdl). Pass 0 for the ordinary full-span replay.
//
// lseats (default false): run the span's SEATS decomposition in place of
// x11pt3, mirroring x11ari.f -- sspdrv and revdrv both pass Lseats straight
// through to x11ari, so a SEATS spec's spans differ from an X-11 spec's only in
// which adjustment routine follows x11pt2. With lseats the span's store is
// seatdg.f's (ssrit on Seatsf/Seatsa) rather than x11pt3's.
bool run_x11_span(X13Context& ctx, const std::vector<double>& trnsrs_full,
                   bool has_model, int nlen, int nfcst, int nbcst, int nbcst2,
                   int lsp, int nend_mdl = 0, bool lseats = false);

}  // namespace x13

#endif  // X13_DRIVER_RUN_X11_SPAN_HPP
