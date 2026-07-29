// agr2.hpp -- per-component accumulation for composite adjustment.
#ifndef X13_COMPOSITE_AGR2_HPP
#define X13_COMPOSITE_AGR2_HPP

namespace x13 {

struct X13Context;

// setapt.f -- set/narrow the indirect-adjustment pointers (Ind1ob/Ind1bk/
// Indfob/Indffc/Indnbc/Indnfc) from this component's backcast/forecast counts.
void setapt(X13Context& ctx, int nb, int nf, const int* begspn, int sp);

// agr2.f (component path) -- stamp/verify the common span (Itest) and accumulate
// this component into the composite buffers. Returns false (and sets Iagr=-1)
// when the component's span does not match the first component's, or when a
// SEATS component produced no signal extraction; the matching NOTE is written to
// Mt2 here, so the caller only has to stop.
bool agr2_component(X13Context& ctx);

// agr2.f, the Iagr==4 path (agr2.f:66-192): once agr3 has produced the indirect
// adjustment, compare it against the direct one -- the R1/R2 measures of
// roughness (aggmea.f) over the full series and the last three years -- then put
// the run's pointers back on the DIRECT geometry and clear Iagr. Fills
// ctx.agr_cmpstat with the oracle's di(1..24).
void agr2_compare(X13Context& ctx, const int* begspn);

}  // namespace x13
#endif  // X13_COMPOSITE_AGR2_HPP
