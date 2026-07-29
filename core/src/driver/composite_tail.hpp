// composite_tail.hpp -- x11ari.f:329-374, the composite{} tail.
#ifndef X13_DRIVER_COMPOSITE_TAIL_HPP
#define X13_DRIVER_COMPOSITE_TAIL_HPP

namespace x13 {

struct X13Context;

// x11ari.f:329-374 -- everything a metafile run does after the adjustment: fold
// a COMPONENT into the aggregation buffers (Iagr 1/2), or, on the composite
// TOTAL (Iagr==3), build the INDIRECT adjustment from those buffers, run the
// indirect diagnostics over it and compare it against the direct one.
//
// Reached identically from the X-11 and the SEATS driver -- the oracle has one
// x11ari, and this block sits after its Lseats/Lx11 branch has rejoined. `lx11`
// is this run's own adjustment; it selects agr3 vs agr3s only via X11agr (the
// COMPONENTS' adjustment), but does select which buffers the direct series is
// taken from and whether the total's own D-tables need snapshotting.
//
// Returns false when the aggregation was refused (Iagr=-1); the NOTE/ERROR is
// already written.
bool run_composite_tail(X13Context& ctx, const int* begspn_full, bool lx11);

}  // namespace x13
#endif  // X13_DRIVER_COMPOSITE_TAIL_HPP
