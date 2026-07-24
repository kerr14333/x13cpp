// agr2.hpp -- per-component accumulation for composite adjustment.
#ifndef X13_COMPOSITE_AGR2_HPP
#define X13_COMPOSITE_AGR2_HPP

namespace x13 {

struct X13Context;

// setapt.f -- set/narrow the indirect-adjustment pointers (Ind1ob/Ind1bk/
// Indfob/Indffc/Indnbc/Indnfc) from this component's backcast/forecast counts.
void setapt(X13Context& ctx, int nb, int nf, const int* begspn, int sp);

// agr2.f (component path) -- stamp/verify the common span (Itest) and accumulate
// this component into the composite buffers. Returns false (and sets Iagr=-1) if
// the component's span does not match the first component's.
// Increment 1 accumulates the direct original `O` only.
bool agr2_component(X13Context& ctx);

}  // namespace x13
#endif  // X13_COMPOSITE_AGR2_HPP
