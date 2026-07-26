// run_seats.hpp -- the SEATS decomposition, factored out of the top-level
// run_seats() driver so a SPAN can run it too.
//
// x11ari.f:204-243 is the single place the oracle enters SEATS, and it is
// reached identically by the main run, by sspdrv's per-span replay
// (slidingspans{}) and by revdrv's per-cutoff replay (history{}) -- all three
// call x11ari with Lseats=T. This port's equivalent of x11ari's SEATS block is
// the pair below; run_seats() (the main run) and run_x11_span() (both span
// drivers) call them at the same point in their own pipelines.
//
// Neither function parses or estimates: both assume the regARIMA model is
// already fitted for whatever window is current and that ctx.series.tsrs holds
// that window's regression-adjusted transformed series.
#ifndef X13_DRIVER_RUN_SEATS_HPP
#define X13_DRIVER_RUN_SEATS_HPP

namespace x13 {

struct X13Context;

// The imean!=0 add-back: SEATS decomposes the series with the mean (drift) kept
// IN but every other regression effect removed. Idempotent per window -- call
// once after the window's estimation, before seats_decompose(). No-op when the
// model carries no Constant regressor.
void seats_restore_mean(X13Context& ctx);

// decode -> canonical denoms -> SPECTRU -> DecompSpectrum -> ESTBUR, publishing
// the components onto ctx (ctx.seats_ran / seats_sa / seats_trend / ...).
// Returns false having already raised the clean seats_not_ported() fatal when
// the model shape or the decomposition itself is unported.
bool seats_decompose(X13Context& ctx);

}  // namespace x13

#endif  // X13_DRIVER_RUN_SEATS_HPP
