// rpoly.hpp -- the Jenkins-Traub real-polynomial root finder (RPOLY, CACM
// algorithm 493), a faithful port of the vendored Census Fortran suite
// rpoly.f/fxshfr.f/quadit.f/realit.f/calcsc.f/nextk.f/newest.f/quadsd.f/quad.f.
// The whole suite shares one scratch COMMON block (global.cmn) that is used by
// no other Census routine; here it is a module-private RpolyState threaded
// through the internal helpers, so the public surface is just rpoly(). Used by
// roots.f (via setmdl start-value root checks and chkrt2 root reporting).
#ifndef X13_NUMERIC_RPOLY_HPP
#define X13_NUMERIC_RPOLY_HPP

namespace x13 {

// rpoly.f: find the zeros of a real polynomial. op holds the degree+1
// coefficients in order of DECREASING powers (op[0] is the leading coefficient).
// On return zeror/zeroi hold the real/imaginary parts of the zeros (length
// degree). fail is true iff the leading coefficient is zero or fewer than
// `degree` zeros were found; in the latter case degree is reset to the number
// of zeros actually found. degree is in/out. zeror/zeroi must have room for the
// input degree. Matches the oracle bit-for-bit (same rtools44 libm for the
// transcendental steps); the RPOLY machine constant Eta = 0.5*(1/1e14) is
// reproduced exactly as the Fortran base**(1-15) evaluates.
void rpoly(const double* op, int& degree, double* zeror, double* zeroi,
           bool& fail);

}  // namespace x13

#endif  // X13_NUMERIC_RPOLY_HPP
