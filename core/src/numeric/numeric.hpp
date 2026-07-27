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

// eltfcn.f operation selectors (ADD/SUB/MULT/DIV in the Fortran).
enum EltOp { ELT_ADD = 1, ELT_SUB = 2, ELT_MULT = 3, ELT_DIV = 4 };

// eltfcn.f: elementwise cvec = avec <oprn> bvec over nelt elements (see EltOp).
// The Fortran Pc arg only sizes Cvec's declaration; dropped here. Arrays may
// alias (prtfct calls it in place).
void eltfcn(int oprn, const double* avec, const double* bvec, int nelt,
            double* cvec);

// devlpl.f: Horner evaluation of A(1)+A(2)X+...+A(N)X^(N-1); a is 0-based.
double devlpl(const double* a, int n, double x);

// stvaln.f: starting value for the Newton normal-inverse iteration (Kennedy &
// Gentle rational approximation).
double stvaln(double p);

// cumnor.f: cumulative normal (Cody ANORM). result = CDF(arg), ccum = 1-result,
// computed by three-interval rational fits. Machine constants eps=0.5*DBL_EPSILON
// and minx=DBL_MIN inline the oracle's spmpar(1)*0.5 / spmpar(2) exactly.
void cumnor(double arg, double& result, double& ccum);

// dinvnr.f: inverse normal CDF -- returns X with CUMNOR(X)=P (Q=1-P). Newton
// iteration seeded by stvaln; falls back to the starting value on non-
// convergence. Used by prtfct for the CI critical value dinvnr((Ciprob+1)/2).
double dinvnr(double p, double q);

// chsppf.f: chi-squared percent-point (inverse CDF) for probability p in [0,1)
// and integer degrees of freedom nu>=1 (DATAPAC/Filliben). Reduces to the gamma
// PPF, computed by bracketing then bisection to 1e-10; the gamma normalizer G
// uses the Stirling-series log-gamma of the oracle verbatim. Used by the automdl
// AIC-test family to turn a pvaictest probability + df into a critical value.
// Returns 0.0 on the oracle's error paths (p out of range, nu<1, no convergence).
double chsppf(double p, int nu);

// dscal.f: BLAS scale dx <- da*dx over n strided elements (no-op if n<=0).
// Scaling has no accumulation, so the mod-5 unrolling of the oracle is dropped;
// the result is bit-identical.
void dscal(int n, double da, double* dx, int incx);

// gauss.f: central normal probability P(-x < N(0,1) < x) via the Statistics
// Canada rational approximation (three regimes on |x|/2). Symmetric in x.
double gauss(double x);

// fvalue.f: F-distribution upper-tail probability P(F_{m,n} > x). Returns 0.0
// for x>90 (or x>40 with n>150), and 1.0 for x<=0.
//
// x is BY REFERENCE on purpose: see CB-17. On the two "probability is 1" exits
// the oracle assigns X=0D0, clobbering the caller's F-statistic. Faithful.
double fvalue(double& x, int m, int n);

// chisq.f: chi-squared upper-tail probability P(chi2_n > x) (Statistics Canada).
// Series in x for x<90 split by even/odd df; 1.0 for x<=0, 0.0 for x>=90. Used
// by chitst for the regressor-group chi-square test p-value.
double chisq(double x, int n);

// special.f -- the TRAMO/SEATS special-function trio, distinct from the Census
// routines above (different author, different conventions). Only the F CDF is
// reached: Tpeaks2 (specpeak.f) scores every Tukey spectral peak against it.
//
// special.f:10 log_gamma  -- Lanczos, 6 coefficients.
// special.f:35 beta_inc   -- the regularized incomplete beta I_x(a,b), via
//                            special.f:62's Lentz continued fraction (EPS 1e-7,
//                            MaxItera 1000, no convergence failure signalled --
//                            it writes to stdout and returns the last iterate).
//                            NOTE the exponent is floored at -500 before EXP,
//                            deliberately, to avoid an underflow trap.
// special.f:108 fcdf      -- P(F(x,y) <= f) = 1 - I_{y/(y+x*f)}(y/2, x/2).
double log_gamma(double a);
double beta_inc(double x, double a, double b);
double fcdf(double f, double x, double y);

// sumf.f: left-to-right sum of x over the 1-based inclusive range [n1,n2]
// (x[0]==X(1)). Accumulation order is preserved for bit-parity.
double sumf(const double* x, int n1, int n2);

// smeadl.f: mean-deletion. Subtracts the mean of x over [n1,n2] from every
// element in that range; the divisor is the caller-supplied count n (the oracle
// passes n separately, not necessarily n2-n1+1). Used by iddiff/amdid to center
// the differenced series before ACF/regression work. x is 1-based (x[0]==X(1)).
void smeadl(double* x, int n1, int n2, int n, double& xmean);

// totals.f: sum/average over the strided 1-based range [i,j] step k, skipping
// DNOTST obs. iopt 0=total, 1=average, 2=abs average, 3=good-obs count.
double totals(const double* x, int i, int j, int k, int iopt);

// sdev.f: standard deviation over the strided range [i,j] step k. iopt<1 uses
// the series mean, ==1 assumes mean 0, else assumes mean 1.
double sdev(const double* x, int i, int j, int k, int iopt);

// shlsrt.f: in-place ascending shell sort of vecx[0..nr-1] (gap = nr/2, halved).
void shlsrt(int nr, double* vecx);

// medabs.f: median of the absolute values of s[0..nr-1] (shell-sorted). Even nr
// averages the two central order statistics. The Fortran PA work-array guard is
// dropped (the C++ scratch grows to nr).
void medabs(const double* s, int nr, double& median);

// dppdi.f: LINPACK determinant/inverse from a packed Cholesky factor ap (from
// dppfa). job: 11 both, 01 inverse only, 10 determinant only. The upper triangle
// of ap is overwritten with the inverse; det = det[0]*10^det[1]. Uses dscal/daxpy
// on the packed columns.
void dppdi(double* ap, int n, double* det, int job);

// dpmpar.f: MINPACK machine parameters. i in {1,2,3} selects machine precision,
// smallest magnitude, largest magnitude. NOTE: the active oracle DATA statement
// uses TRUNCATED literals (dpmpar(1)=2.220446e-16, NOT DBL_EPSILON); ported
// verbatim so parity thresholds match the oracle binary bit for bit.
double dpmpar(int i);

// dpeq.f: tolerance equality, |x - dtargt| < 3.834e-20. Used throughout
// lmdif/qrfac/lmpar/enorm as the exact ==-style test; port before its callers.
bool dpeq(double x, double dtargt);

// gfortran real(8)**int(4): square-and-multiply (reciprocal for n<0). Matches
// the oracle's `x**n` bit for bit where std::pow would differ in the last ULP.
// (Defined in rpoly.cpp.)
double dpow_ri(double base, int n);

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

// yprmy.f: inner product y'y = sum y[i]^2 into ypy (sequential accumulation,
// order preserved). y is nr long.
void yprmy(const double* y, int nr, double& ypy);

// logdet.f: log-determinant of a packed triangular (Cholesky) factor,
// lgdt = sum_{i=1..n} 2*log(ap[diag_i]) where diag_i walks the packed
// diagonal (offsets 1,3,6,...). A zero diagonal legally yields -inf (dppfa may
// leave one); the log is NOT guarded.
void logdet(const double* ap, int n, double& lgdt);

// uconv.f: autocovariance of a moving-average polynomial, C(z)=A(z)*A(1/z),
// computed in place. fulma and c are 0-based, indices 0..mxmalg. The in-place
// read of c[i+k] above the write cursor is load-bearing (ascending i keeps
// those entries at their original values) -- do not reorder.
void uconv(const double* fulma, int mxmalg, double* c);

// xpand.f: power-series expansion of A(z)/B(z) = C(z) up to order nc, in place.
// On entry c[0..na] holds the numerator A; b is 0-based (b[0] unused, the
// denominator is 1 - b[1]z - ...). pc bounds the workspace/output order. The
// numerator is snapshotted before the recursion begins.
void xpand(const double* b, int mxarlg, int na, int nc, double* c, int pc);

// xprmx.f: packed upper triangle of [X:y]'[X:y] via the underflow-skipping
// ddot. xy is row-major (nspobs rows x pcxy leading columns); column c is the
// strided vector xy[c-1], xy[c-1+pcxy], ... When pcxy>ncxy the data vector y is
// in column pcxy: the routine appends X'y (ncxy elements) then y'y. Output
// xypxy is packed by (i=1..ncxy, j=1..i). Because it uses the vendored ddot,
// X'X low-order bits differ from a plain BLAS -- load-bearing for parity.
void xprmx(const double* xy, int nspobs, int ncxy, int pcxy, double* xypxy);

// dppfa.f: packed Cholesky, Census-modified LINPACK. Factors a packed SPD
// matrix ap (upper triangle, column by column) in place into R with A=R'R.
// info=0 on success, else info=j at the first non-PD leading minor. NOT stock
// dppfa: the tolerated-zero branch (s in [-dpmpar(1), 0]) sets that diagonal to
// 0.0 and STILL exits with info=j (not a success path); the inner products use
// the underflow-skipping ddot, so factors differ from textbook Cholesky in low
// bits.
void dppfa(double* ap, int n, int& info);

// dsolve.f: multi-RHS triangular solve against a packed Cholesky factor a
// (from dppfa), variation of LINPACK dposl. b is COLUMN-MAJOR nc x nr:
// element (col j, row i) at b[(j-1)+(i-1)*nc]. Solves R'w=b always; if lainvb
// then also R x=w (full A x=b). The ddot/daxpy calls use mixed strides 1/nc,
// exercising the BLAS unequal-increment paths.
void dsolve(const double* a, int nr, int nc, bool lainvb, double* b);

// dppsl.f: LINPACK single-RHS solve a*x=b against the packed Cholesky factor ap
// (from dppfa; a = L L'). With alt=true it stops after the forward solve,
// returning x from L*x=b only (the Census `alt` extension used by fcstxy). b is
// the length-n RHS, overwritten with the solution. Uses ddot/daxpy on the packed
// column storage (ap[kk-1] the k-th diagonal, ap+kk the column off-diagonals).
void dppsl(const double* ap, int n, double* b, bool alt);

// euclid.f: solves Fular(z)F(1/z)+F(z)Fular(1/z)=G(z) by the Euclid algorithm
// (AR-covariance step of armafl). fular/g are 0-based (fular[0..mxarlg],
// g[0..maxpq]); b and a are 1-based workspace of length mxarlg (b[i-1],a[i-1]),
// caller-provided. On success g[0..maxpq] holds F and err=0. If an AR coeff
// fails |r|>1 (non-stationary), err=1 and the routine returns early with g
// PARTIALLY modified -- callers must check err before using g. The even-i
// midpoint element is written twice with the same value in both the reduction
// and construction loops; preserved verbatim.
void euclid(const double* fular, double* b, double* a, int maxpq, int mxarlg,
            int mxmalg, double* g, int& err);

}  // namespace x13

#endif  // X13_NUMERIC_NUMERIC_HPP
