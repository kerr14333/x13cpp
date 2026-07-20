// seatspoly.hpp -- SEATS's own polynomial arithmetic and complex-polynomial
// root finder (foundational pure-numeric leaves of the ARIMA-model-based
// signal-extraction engine). Direct ports of the vendored Fortran in
// oracle/fortran/ansub2.f -- CONV/CONJ/MULTFN/DIVFCN (poly.cpp) and
// C02AEF/C02AEZ/RPQ (roots.cpp).
//
// PARITY NOTE: SEATS carries its OWN polynomial multiply (CONV) and its OWN
// root finder (the NAG-derived C02AEF, Grant & Hitchins 1971), NOT the ported
// Jenkins-Traub rpoly / uconv used by the regARIMA engine. These must be used
// inside SEATS to stay bit-exact: CONV's accumulation order and C02AEF's
// deflation/refinement order both feed the canonical decomposition, where a
// last-ULP root difference re-classifies a root into a different component.
//
// All array arguments follow the Fortran 1-based coefficient convention but are
// passed as ordinary 0-based C pointers: a coefficient vector A(1..k) in the
// Fortran is A[0..k-1] here. Where the port needs the 1-based index arithmetic
// verbatim it uses an internal offset view (see the .cpp files); it never
// mutates outside the documented [0, len) ranges.
#ifndef X13_SEATS_SEATSPOLY_HPP
#define X13_SEATS_SEATSPOLY_HPP

namespace x13 {

// ---- polynomial arithmetic (ansub2.f) -------------------------------------

// CONV: c = a * b, ordinary polynomial convolution (true signs). a has mplus1
// coefficients, b has nplus1; c must differ from a and b. On return
// lplus1 = mplus1 + nplus1 - 1 is the length of c.
//   a(1) + a(2)B + ... + a(mplus1)B^(mplus1-1)
void conv(const double* a, int mplus1, const double* b, int nplus1,
          double* c, int& lplus1);

// CONJ: c = A(z) * B(z^-1) (the "correlation" product used for cosine-power
// harmonic functions). lplus1 = max(mplus1, nplus1). c(|i-j|+1) += a(i)*b(j).
void conj(const double* a, int mplus1, const double* b, int nplus1,
          double* c, int& lplus1);

// MULTFN: product of two harmonic functions a,b in powers of cos(w). Equals
// (CONV(a,b) + CONJ(a,b)) / 2. lplus1 = mplus1 + nplus1 - 1.
void multfn(const double* a, int mplus1, const double* b, int nplus1,
            double* c, int& lplus1);

// DIVFCN: quotient q and remainder r of harmonic functions a / b. q,r must
// differ from a,b. nq = mplus1 - nplus1 + 1 (quotient length), nr = nplus1 - 1
// (remainder length; nr==0 when b is a constant).
void divfcn(const double* a, int mplus1, const double* b, int nplus1,
            double* q, int& nq, double* r, int& nr);

// ---- complex-polynomial root finder (ansub2.f) ----------------------------

// C02AEF: solves the real polynomial a(1)*x^(n-1) + ... + a(n) = 0 (a(1) is the
// coefficient of the HIGHEST power) by the Grant & Hitchins (1971) search to
// limiting machine precision. Roots returned as rez(k)+i*imz(k), k=1..(n-1) in
// approximately decreasing modulus order. tol is raised to dpmpar(1) if smaller.
// On exit ifail=0 on success, 1 on failure; a and n are overwritten (deflated).
// rez/imz must be sized to at least the incoming n. C02AEZ is the internal
// evaluator (Adams test) and is not exposed.
void c02aef(double* a, int& n, double* rez, double* imz, double& tol,
            int& ifail);

// RPQ: root driver over C02AEF. Given polynomial b(1..n) (b(1) highest power),
// fills the classification arrays consumed by the decomposition:
//   rez,imz : real/imaginary parts of the nroots = n-1 roots
//   m       : modulus of each root
//   ar      : argument in DEGREES
//   p       : period (2*pi/arg), or 999.99 for a (near-)real root
// noprint/out suppress the (deferred) tabular output; pass 1 / any nonzero.
void rpq(const double* b, int n, double* rez, double* imz, double* m,
         double* ar, double* p, int noprint, int out);

}  // namespace x13

#endif  // X13_SEATS_SEATSPOLY_HPP
