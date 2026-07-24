// agr3.hpp -- indirect (aggregate-of-components) seasonal adjustment.
#ifndef X13_COMPOSITE_AGR3_HPP
#define X13_COMPOSITE_AGR3_HPP

namespace x13 {

struct X13Context;

// agrxpt.f -- reconcile the direct and indirect buffer geometries before the
// composite total's own adjustment starts (editor.f:234, under Iagr==3).
void agrxpt(X13Context& ctx, const int* begspn, int sp);

// agr3.f -- rebuild the X-11 D-tables from the AGGREGATED component results:
// D10 (Sts) = indirect seasonal, D11 (Stci) = indirect SA, D12 = the indirect
// trend (left in ctx.agr_stc2in), D13 (Sti) = indirect irregular. Sets Iagr=4
// and switches the run onto the indirect geometry. Increment 2 scope -- see
// tools/composite_scouting.md.
void agr3(X13Context& ctx, const int* begspn);

}  // namespace x13
#endif  // X13_COMPOSITE_AGR3_HPP
