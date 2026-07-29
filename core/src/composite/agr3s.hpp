// agr3s.hpp -- the SEATS branch of the composite indirect adjustment.
#ifndef X13_COMPOSITE_AGR3S_HPP
#define X13_COMPOSITE_AGR3S_HPP

namespace x13 {

struct X13Context;

// agr3s.f -- the indirect adjustment when at least one component was adjusted by
// SEATS rather than X-11 (X11agr false at x11ari.f:341). The indirect SA series
// IS the aggregate of the components' own SA series; the seasonal factor is
// recovered from it by division, and there is no indirect trend, irregular, D8/D9
// pair or x11pt4 pass. Sets Iagr=4 and switches the run onto the indirect
// geometry, exactly as agr3 does.
//
// `lx11` is the TOTAL's own adjustment (x11ari.f passes Lx11 straight through);
// it selects Stc/Ci vs Seattr/Seatsa for the direct-series stash and gates the
// residual-seasonality test on the rounded series.
void agr3s(X13Context& ctx, const int* begspn, bool lx11);

}  // namespace x13
#endif  // X13_COMPOSITE_AGR3S_HPP
