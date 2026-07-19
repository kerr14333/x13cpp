// minpack.hpp -- the (Census-modified) MINPACK Levenberg-Marquardt optimizer
// used by regARIMA estimation: lmdif and its supporting linear-algebra routines
// qrfac / qrsolv / lmpar / fdjac2 / covar. These are faithful ports of the
// VENDORED oracle Fortran, which differs from stock MINPACK (notably the use of
// Census's dpeq() dp-equality in place of exact .eq.0 comparisons); port the
// repo version, never textbook MINPACK. All matrices are column-major with a
// leading dimension, Fortran-base indexing preserved. This module is independent
// of the model state: lmdif takes the objective as a callback so the numeric
// optimizer does not depend on regarima.
#ifndef X13_NUMERIC_MINPACK_HPP
#define X13_NUMERIC_MINPACK_HPP

namespace x13 {

// qrfac.f: Householder QR with optional column pivoting. a is m-by-n column-
// major (leading dim lda); on output its upper trapezoid holds R and the lower
// part the factored Q. rdiag = diagonal of R, acnorm = original column norms,
// wa = work (n). ipvt (length lipvt>=n if pivot) gives the permutation A*P=Q*R.
void qrfac(int m, int n, double* a, int lda, bool pivot, int* ipvt, int lipvt,
           double* rdiag, double* acnorm, double* wa);

// qrsolv.f: solve (R*P'*x = Q'b, D*x = 0) in the least-squares sense given the
// QR factor r (n-by-n column-major, leading dim ldr; its lower triangle is used
// as scratch and restored), the pivot ipvt, the diagonal diag, and qtb. Outputs
// x and the Cholesky diagonal sdiag; wa is work (n).
void qrsolv(int n, double* r, int ldr, const int* ipvt, const double* diag,
            const double* qtb, double* x, double* sdiag, double* wa);

// lmpar.f: determine the Levenberg-Marquardt parameter par such that the scaled
// step x solving (R'R + par*D*D) x = R'*qtb has ||D*x|| ~ delta (the trust-
// region radius). Iterates the secant search (>=10 steps max) calling qrsolv per
// step. par is in/out (starting guess -> chosen value). x, sdiag are outputs;
// wa1, wa2 are work (n). r is the n-by-n QR factor (column-major, leading dim
// ldr); its lower triangle is scratch (restored by qrsolv).
void lmpar(int n, double* r, int ldr, const int* ipvt, const double* diag,
           const double* qtb, double delta, double& par, double* x,
           double* sdiag, double* wa1, double* wa2);

}  // namespace x13

#endif  // X13_NUMERIC_MINPACK_HPP
