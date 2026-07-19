// numeric.hpp -- Tier-0 scalar/vector numeric leaves for the regARIMA engine
// (M3). Direct ports of the vendored MINPACK / BLAS-style Fortran leaves; one
// free function per Fortran routine, faithful to the oracle down to loop order,
// unrolling, and the DELTA/underflow/3-bin edge cases that the optimizer's step
// acceptance depends on. All arrays are 0-based C pointers; the Fortran 1-based
// index arithmetic is preserved where it affects results (strided BLAS paths).
//
// copy/maxlag/insort/setdp are ported elsewhere (specparse); not duplicated here.
#ifndef X13_NUMERIC_NUMERIC_HPP
#define X13_NUMERIC_NUMERIC_HPP

namespace x13 {

// dpmpar.f: MINPACK machine parameters. i in {1,2,3} selects machine precision,
// smallest magnitude, largest magnitude. NOTE: the active oracle DATA statement
// uses TRUNCATED literals (dpmpar(1)=2.220446e-16, NOT DBL_EPSILON); ported
// verbatim so parity thresholds match the oracle binary bit for bit.
double dpmpar(int i);

// dpeq.f: tolerance equality, |x - dtargt| < 3.834e-20. Used throughout
// lmdif/qrfac/lmpar/enorm as the exact ==-style test; port before its callers.
bool dpeq(double x, double dtargt);

// scrmlt.f: scale x[0..n-1] by scalar c in place.
void scrmlt(double c, int n, double* x);

// maxvec.f: maximum magnitude of dx[0..n-1] into mxabvl (0 if n<=0).
void maxvec(const double* dx, int n, double& mxabvl);

// dcopy.f: BLAS copy of n elements dx->dy with strides incx/incy.
void dcopy(int n, const double* dx, int incx, double* dy, int incy);

// daxpy.f: BLAS dy <- da*dx + dy (no-op if n<=0 or dpeq(da,0)).
void daxpy(int n, double da, const double* dx, int incx, double* dy, int incy);

// ddot.f: BLAS dot product, but each term is skipped when its product would
// underflow (log10|x|+log10|y| <= log10(dpmpar(2))) or either factor is zero --
// the vendored ddot is NOT a plain sum, and the skip changes low-order bits.
double ddot(int n, const double* dx, int incx, const double* dy, int incy);

// revrse.f: reverse the Nr rows of an (Nc x Nr Fortran-order) matrix frwd into
// bkwd. Element (row i, col j) lives at frwd[(i-1)*nc + (j-1)]; may alias.
void revrse(const double* frwd, int nr, int nc, double* bkwd);

// enorm.f: MINPACK Euclidean norm via 3-bin (small/mid/large) scaled summation
// -- NOT sqrt(sum(x^2)). Ported verbatim so step norms (hence step acceptance
// in lmdif) match the oracle.
double enorm(int n, const double* x);

}  // namespace x13

#endif  // X13_NUMERIC_NUMERIC_HPP
