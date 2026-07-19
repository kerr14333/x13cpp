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

#include <functional>

namespace x13 {

// The objective callback, matching the vendored fcn signature
// fcn(M,N,X,Fvec,Lauto,Gudrun,Iflag,Lckinv): m (residual count, in/out -- the
// objective may reset it), n params, x params (in), fvec residuals (out), the
// two run-mode flags, iflag (in/out, <0 aborts), lckinv (root-check gate).
using MinpackFcn = std::function<void(int& m, int n, const double* x,
                                      double* fvec, bool lauto, bool gudrun,
                                      int& iflag, bool lckinv)>;

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

// fdjac2.f: forward-difference approximation to the m-by-n Jacobian fjac
// (column-major, leading dim ldfjac) of fcn at x, given the residuals fvec=fcn(x)
// and a relative step epsfcn. wa is work (m). Aborts (returns early) if fcn sets
// iflag<0. x is restored after each perturbation.
void fdjac2(const MinpackFcn& fcn, int m, int n, double* x, const double* fvec,
            double* fjac, int ldfjac, int& iflag, double epsfcn, double* wa,
            bool lauto, bool gudrun, bool lckinv);

// covar.f: covariance matrix (R'R)^-1 from the QR factorization output of lmdif.
// r is the n-by-n matrix (column-major, leading dim ldr) whose upper triangle
// holds R; on output its full symmetric content is the covariance matrix with
// the ipvt column pivot applied. ipvt is qrfac's permutation, tol the relative
// singularity tolerance (tol<=0 selects dpmpar(1)). wa is work (n). info returns
// 0 when R was nonsingular, otherwise the count of nonsingular leading columns
// found before the first singular pivot (the NOTSET sentinel if column 1 is
// already singular). Census specific: the singularity test is the literal
// |R(k,k)|<=tolr, not the dpeq() equality used by the sibling leaves.
void covar(int n, double* r, int ldr, const int* ipvt, double tol, int& info,
           double* wa);

// Model-state sync hook. lmdif calls it with the current accepted parameter
// vector x after every Jacobian (fdjac2 leaves the model synced to a perturbed
// x) and after every rejected trial step, so that on exit the model state
// matches the returned x. It never feeds back into lmdif's numerics (the
// objective re-syncs on every call); an empty hook is a bit-for-bit no-op.
// regarima passes [&ctx](const double* x){ upespm(ctx, x); }.
using MinpackSync = std::function<void(const double* x)>;

// Iteration-print hook standing in for prtitr.f. Args mirror
// prtitr(A,Na,Parms,Nparms,Itrlbl,Iter,Nfev). Returns true iff a fatal error
// occurred (error.cmn Lfatal), which makes lmdif return immediately. An empty
// hook = no printing (the nprint<=0 behavior); the .out print engine is
// deferred, so production callers pass an empty hook for now.
using MinpackPrtitr = std::function<bool(const double* fvec, int m,
                                         const double* x, int n,
                                         const char* itrlbl, int nliter,
                                         int nfev)>;

// lmdif.f: the Census-modified MINPACK Levenberg-Marquardt core (forward-
// difference Jacobian). Minimizes ||fcn(x)||^2. NOT stock MINPACK: MAXFEV is
// derived internally from mxiter (maxfev = max(mxiter,200)*(n+1)), fcn carries
// lauto/gudrun/lckinv, nliter/nfev are CUMULATIVE in/out counters (not zeroed
// on entry, so repeated IGLS/AIC-test calls accumulate), and dpeq() replaces
// every ==0 test. x is start (in) / final estimate (out); fvec the residuals at
// the final x; diag the scale factors (in/out for mode!=2, in for mode==2);
// info the termination code 0..8 (or the negative iflag on a user abort). fjac
// (m-by-n, leading dim ldfjac), ipvt, qtf return the final QR of the Jacobian.
// wa1/wa2/wa3 are work (n), wa4 work (m). sync/prtitr default to empty; see
// their hook docs above. m is aliased through fcn like fdjac2.
void lmdif(const MinpackFcn& fcn, int m, int n, double* x, double* fvec,
           bool lauto, bool gudrun, double ftol, double xtol, double gtol,
           int mxiter, double epsfcn, double* diag, int mode, double factor,
           int nprint, int& info, int& nliter, int& nfev, double* fjac,
           int ldfjac, int* ipvt, double* qtf, double* wa1, double* wa2,
           double* wa3, double* wa4, const MinpackSync& sync = {},
           const MinpackPrtitr& prtitr = {});

}  // namespace x13

#endif  // X13_NUMERIC_MINPACK_HPP
