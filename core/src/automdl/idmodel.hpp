// idmodel.hpp -- automatic model identification (automdl / TRAMO) root checks.
// chkrt1.f / chkurt.f examine the roots of the AR and MA operators of the
// current model (built in ctx.model / ctx.mdldat) to drive the differencing and
// over-differencing decisions in iddiff / tstmd2.
#ifndef X13_AUTOMDL_IDMODEL_HPP
#define X13_AUTOMDL_IDMODEL_HPP

#include "common/x13context.hpp"

namespace x13 {

// chkrt1.f: count near-unit AR roots and track the largest inverse modulus of
// the AR operators, used by iddiff's unit-root screen. irunit/isunit receive
// the regular/seasonal unit-root counts (root modulus <= ublim, imag <= 0.05,
// real > 0). rmaxr/rmaxs receive the largest 1/modulus among near-real AR roots
// (imag <= 0.02, real > 0), or DNOTST when none. linv is AND-folded with the
// all-roots-invertible flag from every AR operator. Only the AR (factor) block
// is walked despite the header comment mentioning theta(B).
void chkrt1(X13Context& ctx, int& irunit, int& isunit, double& rmaxr,
            double& rmaxs, bool& linv, double ublim);

// chkurt.f: count AR and MA roots at or inside the near-unit modulus limit
// 1/0.95 across both the AR and MA operators, split into regular/seasonal by the
// operator's seasonal factor. urpr/urps = regular/seasonal AR unit roots,
// urqr/urqs = regular/seasonal MA unit roots. Drives tstmd2's over-fit check.
void chkurt(X13Context& ctx, int& urpr, int& urps, int& urqr, int& urqs);

}  // namespace x13
#endif  // X13_AUTOMDL_IDMODEL_HPP
