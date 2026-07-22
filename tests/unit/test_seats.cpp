// test_seats.cpp -- parity unit tests for the SEATS foundational leaves:
// polynomial arithmetic (CONV/CONJ/MULTFN/DIVFCN) and the complex-polynomial
// root finder (C02AEF/RPQ) ported from oracle/fortran/ansub2.f.
//
// Golden values are HAND-COMPUTED polynomial products/quotients and polynomials
// with KNOWN roots (rather than oracle captures) -- cleaner and independently
// checkable. The exactly-representable cases use CHECK_EQ; the iterative
// root-finder path is checked to full double precision via a tight tolerance.
#include "microtest.hpp"
#include "seats/seatsalloc.hpp"
#include "seats/seatsdenoms.hpp"
#include "seats/seatsfact.hpp"
#include "seats/seatspoly.hpp"
#include "seats/model_decode.hpp"
#include "seats/canonical_denoms.hpp"
#include "seats/spectru.hpp"
#include "seats/decompspectrum.hpp"
#include "automdl/mdlset.hpp"
#include "common/x13context.hpp"

#include <cmath>
#include <cstdio>
#include <memory>

using namespace x13;

int main() { return mt::run_all(); }

// --------------------------------------------------------------------------
// Polynomial arithmetic
// --------------------------------------------------------------------------

TEST("conv: (1+2B)(1+3B) = 1+5B+6B^2") {
    double a[2] = {1.0, 2.0};
    double b[2] = {1.0, 3.0};
    double c[8] = {0};
    int l = 0;
    conv(a, 2, b, 2, c, l);
    CHECK_EQ(l, 3);
    CHECK_EQ(c[0], 1.0);
    CHECK_EQ(c[1], 5.0);
    CHECK_EQ(c[2], 6.0);
}

TEST("conj: A(z)B(1/z) of [1,2],[1,3] = [7,5]") {
    // c1 = a1 b1 + a2 b2 = 1 + 6 = 7 ; c2 = a1 b2 + a2 b1 = 3 + 2 = 5.
    double a[2] = {1.0, 2.0};
    double b[2] = {1.0, 3.0};
    double c[8] = {0};
    int l = 0;
    conj(a, 2, b, 2, c, l);
    CHECK_EQ(l, 2);
    CHECK_EQ(c[0], 7.0);
    CHECK_EQ(c[1], 5.0);
}

TEST("multfn: (1+2cos)(1+3cos) = 4 + 5cos + 3cos2w") {
    // (1+2cos)(1+3cos) = 1+5cos+6cos^2 ; cos^2 = (1+cos2w)/2 -> [4,5,3].
    double a[2] = {1.0, 2.0};
    double b[2] = {1.0, 3.0};
    double c[8] = {0};
    int l = 0;
    multfn(a, 2, b, 2, c, l);
    CHECK_EQ(l, 3);
    CHECK_EQ(c[0], 4.0);
    CHECK_EQ(c[1], 5.0);
    CHECK_EQ(c[2], 3.0);
}

TEST("divfcn: [4,5,3] / [1,3] = q [1,2], r 0") {
    // Inverse of the multfn case: dividing the harmonic product by [1,3].
    double a[3] = {4.0, 5.0, 3.0};
    double b[2] = {1.0, 3.0};
    double q[8] = {0};
    double r[8] = {-9.0, -9.0, -9.0};
    int nq = 0, nr = 0;
    divfcn(a, 3, b, 2, q, nq, r, nr);
    CHECK_EQ(nq, 2);
    CHECK_EQ(q[0], 1.0);
    CHECK_EQ(q[1], 2.0);
    CHECK_EQ(nr, 1);
    CHECK_EQ(r[0], 0.0);
}

TEST("divfcn: constant divisor path (nplus1 == 1)") {
    // [2,4,6] / [2] -> q [1,2,3], remainder empty.
    double a[3] = {2.0, 4.0, 6.0};
    double b[1] = {2.0};
    double q[8] = {0};
    double r[8] = {-9.0};
    int nq = 0, nr = -5;
    divfcn(a, 3, b, 1, q, nq, r, nr);
    CHECK_EQ(nq, 3);
    CHECK_EQ(q[0], 1.0);
    CHECK_EQ(q[1], 2.0);
    CHECK_EQ(q[2], 3.0);
    CHECK_EQ(nr, 0);
    CHECK_EQ(r[0], 0.0);
}

// --------------------------------------------------------------------------
// C02AEF root finder
// --------------------------------------------------------------------------

TEST("c02aef: (z-2)(z-3) = z^2-5z+6 -> roots 3, 2 (exact)") {
    // Quadratic explicit branch (n==3); all arithmetic exact in binary FP.
    double a[3] = {1.0, -5.0, 6.0};
    double rez[3] = {0}, imz[3] = {0};
    int n = 3, ifail = -1;
    double tol = 0.0;
    c02aef(a, n, rez, imz, tol, ifail);
    CHECK_EQ(ifail, 0);
    CHECK_EQ(rez[0], 3.0);
    CHECK_EQ(rez[1], 2.0);
    CHECK_EQ(imz[0], 0.0);
    CHECK_EQ(imz[1], 0.0);
}

TEST("c02aef: z^2+1 -> roots +/- i (exact)") {
    double a[3] = {1.0, 0.0, 1.0};
    double rez[3] = {0}, imz[3] = {0};
    int n = 3, ifail = -1;
    double tol = 0.0;
    c02aef(a, n, rez, imz, tol, ifail);
    CHECK_EQ(ifail, 0);
    CHECK_EQ(rez[0], 0.0);
    CHECK_EQ(rez[1], 0.0);
    CHECK_EQ(imz[0], -1.0);
    CHECK_EQ(imz[1], 1.0);
}

TEST("c02aef: (z-2)(z-3)(z-4) -> BIT-EXACT vs oracle (iterative path)") {
    // n==4 exercises the full Grant-Hitchins search + deflation (C02AEZ). The
    // literals below are the EXACT bits emitted by the vendored oracle C02AEF
    // (gfortran -O2 -ffp-contract=off; see tools note in commit). Bit-identity
    // here is the #1 SEATS parity gate: a last-ULP root difference re-classifies
    // a root into a different decomposition component downstream.
    double a[4] = {1.0, -9.0, 26.0, -24.0};
    double rez[4] = {0}, imz[4] = {0};
    int n = 4, ifail = -1;
    double tol = 0.0;
    c02aef(a, n, rez, imz, tol, ifail);
    CHECK_EQ(ifail, 0);
    CHECK_EQ(rez[0], 4.0000000000000053e0);
    CHECK_EQ(rez[1], 2.9999999999999947e0);
    CHECK_EQ(rez[2], 2.0000000000000009e0);
    CHECK_EQ(imz[0], 0.0);
    CHECK_EQ(imz[1], 0.0);
    CHECK_EQ(imz[2], 0.0);
}

TEST("c02aef: (z^2+4)(z-1) -> BIT-EXACT vs oracle (real + complex pair)") {
    // z^3 - z^2 + 4z - 4: a real root 1 and the conjugate pair +/-2i. Exact
    // oracle bits (note rez of the complex pair is -1.11e-16, not a clean 0).
    double a[4] = {1.0, -1.0, 4.0, -4.0};
    double rez[4] = {0}, imz[4] = {0};
    int n = 4, ifail = -1;
    double tol = 0.0;
    c02aef(a, n, rez, imz, tol, ifail);
    CHECK_EQ(ifail, 0);
    CHECK_EQ(rez[0], -1.1102230246251565e-16);
    CHECK_EQ(imz[0], -1.9999999999999998e0);
    CHECK_EQ(rez[1], -1.1102230246251565e-16);
    CHECK_EQ(imz[1], 1.9999999999999998e0);
    CHECK_EQ(rez[2], 1.0000000000000002e0);
    CHECK_EQ(imz[2], 0.0);
}

TEST("c02aef: (z-2)..(z-6) quintic -> BIT-EXACT vs oracle (deep deflation)") {
    // z^5 - 20z^4 + 155z^3 - 580z^2 + 1044z - 720. Five real roots; exercises
    // repeated deflation. Exact oracle bits.
    double a[6] = {1.0, -20.0, 155.0, -580.0, 1044.0, -720.0};
    double rez[6] = {0}, imz[6] = {0};
    int n = 6, ifail = -1;
    double tol = 0.0;
    c02aef(a, n, rez, imz, tol, ifail);
    CHECK_EQ(ifail, 0);
    CHECK_EQ(rez[0], 6.0000000000001057e0);
    CHECK_EQ(rez[1], 4.9999999999997362e0);
    CHECK_EQ(rez[2], 4.0000000000002052e0);
    CHECK_EQ(rez[3], 2.9999999999999516e0);
    CHECK_EQ(rez[4], 2.0000000000000000e0);
    for (int i = 0; i < 5; ++i) CHECK_EQ(imz[i], 0.0);
}

// --------------------------------------------------------------------------
// RPQ driver + classification
// --------------------------------------------------------------------------

TEST("rpq: real roots 0.5, 0.8 -> modulus/arg/period") {
    // b = z^2 - 1.3z + 0.4 = (z-0.5)(z-0.8). Both roots real & positive:
    // argument 0 deg, period sentinel 999.99.
    double b[3] = {1.0, -1.3, 0.4};
    double rez[2] = {0}, imz[2] = {0}, m[2] = {0}, ar[2] = {0}, p[2] = {0};
    rpq(b, 3, rez, imz, m, ar, p, 1, 1);
    // Roots come back largest-modulus first: 0.8 then 0.5.
    CHECK(std::fabs(rez[0] - 0.8) <= 1.0e-12);
    CHECK(std::fabs(rez[1] - 0.5) <= 1.0e-12);
    CHECK(std::fabs(m[0] - 0.8) <= 1.0e-12);
    CHECK(std::fabs(m[1] - 0.5) <= 1.0e-12);
    CHECK_EQ(ar[0], 0.0);
    CHECK_EQ(ar[1], 0.0);
    CHECK_EQ(p[0], 999.99);
    CHECK_EQ(p[1], 999.99);
}

TEST("rpq: complex pair (1 +/- i*sqrt3)/2 -> modulus 1, arg +/-60 deg") {
    // b = z^2 - z + 1: roots on the unit circle at +/-60 degrees, period +/-6.
    double b[3] = {1.0, -1.0, 1.0};
    double rez[2] = {0}, imz[2] = {0}, m[2] = {0}, ar[2] = {0}, p[2] = {0};
    rpq(b, 3, rez, imz, m, ar, p, 1, 1);
    CHECK(std::fabs(rez[0] - 0.5) <= 1.0e-12);
    CHECK(std::fabs(rez[1] - 0.5) <= 1.0e-12);
    CHECK(std::fabs(m[0] - 1.0) <= 1.0e-12);
    CHECK(std::fabs(m[1] - 1.0) <= 1.0e-12);
    // One root has argument -60, the other +60 (order: imz<0 first here).
    CHECK(std::fabs(std::fabs(ar[0]) - 60.0) <= 1.0e-9);
    CHECK(std::fabs(std::fabs(ar[1]) - 60.0) <= 1.0e-9);
    CHECK(std::fabs(ar[0] + ar[1]) <= 1.0e-9);        // opposite signs
    CHECK(std::fabs(std::fabs(p[0]) - 6.0) <= 1.0e-9);
    CHECK(std::fabs(std::fabs(p[1]) - 6.0) <= 1.0e-9);
}

// --------------------------------------------------------------------------
// PARFRA + MAK1 (canonical-decomposition leaves)
//
// Golden bits below are the EXACT doubles emitted by the vendored oracle
// Fortran, captured via tools/ref_seatsfact.f (built by tools/extract_seatsfact.py,
// linked against the oracle PARFRA/MAK1 + helpers extracted verbatim from
// ansub2.f, plus dpeq.f/dpmpar.f; gfortran -O2 -ffp-contract=off, ES24.16).
// Bit-identity is the SEATS parity contract: MAK1's factorization is driven by
// C02AEF roots + modulus comparisons, so a last-ULP root difference would move
// theta/var. MAK1 has no explicit convergence loop -- the tolerance gating lives
// in RPQ's Newton iteration and the root classification, and it reproduces
// bit-exact here.
// --------------------------------------------------------------------------

TEST("parfra: RT/(T*S)=U/T+V/S -> BIT-EXACT vs oracle") {
    // t,s harmonic funcs (powers of cos w); rt arbitrary. See ref_seatsfact.f.
    double t[3] = {2.0, -1.0, 0.5};
    double s[3] = {3.0, 0.4, -0.2};
    double rt[4] = {1.0, 2.0, 3.0, 4.0};
    double u[8] = {0}, v[8] = {0};
    int nu = 0, nv = 0;
    parfra(rt, 4, t, 3, s, 3, u, nu, v, nv);
    CHECK_EQ(nu, 2);
    CHECK_EQ(nv, 2);
    CHECK_EQ(u[0], -9.2105263157894726e0);
    CHECK_EQ(u[1], -3.1578947368421053e0);
    CHECK_EQ(v[0], 1.8315789473684212e1);
    CHECK_EQ(v[1], 1.4736842105263158e1);
}

TEST("mak1 A: ACF of MA(1) 1-0.5B -> theta exact (nufin<=2 path)") {
    // gam0=1.25, 2*gam1=-1.0. Degenerate n<=2 branch (no SYMPOLY/RPQ).
    double ufin[2] = {1.25, -1.0};
    double theta[8] = {0}, var = 0.0, toterr = -1.0;
    int nt = 0;
    mak1(ufin, 2, theta, nt, var, /*nnio=*/0, /*xl=*/0.99, toterr);
    CHECK_EQ(nt, 2);
    CHECK_EQ(theta[0], 1.0);
    CHECK_EQ(theta[1], -5.0000000000000000e-1);
    CHECK_EQ(var, 1.0);
    CHECK_EQ(toterr, 0.0);
}

TEST("mak1 B: ACF of MA(2) (1,-0.5,0.2) -> BIT-EXACT (real-root SYMPOLY/RPQ path)") {
    // gam0=1.29, 2*gam1=-1.2, 2*gam2=0.4. Exercises SYMPOLY -> RPQ -> C02AEF ->
    // grRoots -> halfRoots -> MPBC. toterr is a tiny FP residual (not 0).
    double ufin[3] = {1.29, -1.2, 0.4};
    double theta[8] = {0}, var = 0.0, toterr = -1.0;
    int nt = 0;
    mak1(ufin, 3, theta, nt, var, /*nnio=*/0, /*xl=*/0.99, toterr);
    CHECK_EQ(nt, 3);
    CHECK_EQ(theta[0], 1.0);
    CHECK_EQ(theta[1], -5.0000000000000000e-1);
    CHECK_EQ(theta[2], 2.0000000000000004e-1);
    CHECK_EQ(var, 1.0);
    CHECK_EQ(toterr, 5.2385294487332815e-32);
}

TEST("mak1 C: ACF of MA(2) (1,0,0.25) -> BIT-EXACT (complex-root path)") {
    // gam0=1.0625, 2*gam1=0, 2*gam2=0.5. Roots +-2i -> exercises the ROOTC
    // complex branch + conjugate grouping in grRoots/halfRoots.
    double ufin[3] = {1.0625, 0.0, 0.5};
    double theta[8] = {0}, var = 0.0, toterr = -1.0;
    int nt = 0;
    mak1(ufin, 3, theta, nt, var, /*nnio=*/0, /*xl=*/0.99, toterr);
    CHECK_EQ(nt, 3);
    CHECK_EQ(theta[0], 1.0);
    CHECK_EQ(theta[1], 0.0);
    CHECK_EQ(theta[2], 2.5000000000000000e-1);
    CHECK_EQ(var, 1.0);
    CHECK_EQ(toterr, 0.0);
}

// --------------------------------------------------------------------------
// F1RST + isCloseTD (AR-root allocation to canonical components)
//
// Every convolution below folds a fresh root factor against a length-1
// {1.0} accumulator (a fresh component's starting denominator, per
// sigex.f:540-541), so CONV's output is exactly the root-factor coefficients
// themselves (a(i)*1.0 is exact in IEEE754) -- no oracle capture needed, the
// expected values are computed with the SAME expression the port evaluates,
// so equality is bit-exact by construction rather than by matching decimals.
// --------------------------------------------------------------------------

TEST("f1rst: p=0 is a no-op") {
    double cycns[4] = {1.0, 0, 0, 0}, psins[4] = {1.0, 0, 0, 0};
    double cycs[4] = {1.0, 0, 0, 0}, chins[4] = {1.0, 0, 0, 0};
    double chis[4] = {1.0, 0, 0, 0}, psis[4] = {1.0, 0, 0, 0};
    int ncycns = 1, npsins = 1, ncycs = 1, nchins = 1, nchis = 1, npsis = 1;
    double imz[1] = {0}, rez[1] = {0}, ar[1] = {0}, modul[1] = {0};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(0, imz, rez, ar, 1.0, 12, cycns, ncycns, psins, npsins, cycs,
          ncycs, chins, nchins, chis, nchis, modul, psis, npsis, 0.5, root0c,
          rootpic, rootpis, close_td);
    CHECK_EQ(ncycns, 1);
    CHECK_EQ(npsins, 1);
    CHECK_EQ(ncycs, 1);
    CHECK_EQ(nchins, 1);
    CHECK_EQ(nchis, 1);
    CHECK_EQ(npsis, 1);
    CHECK(!root0c);
    CHECK(!rootpic);
}

TEST("f1rst: real root near +1 -> nonstationary trend (chins)") {
    double chins[4] = {1.0, 0, 0, 0};
    int nchins = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    double rez[1] = {0.9999995}, imz[1] = {0.0};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(1, imz, rez, /*ar*/ nullptr, 1.0, 12, dummy, n1, dummy, n1, dummy,
          n1, chins, nchins, dummy, n1, /*modul*/ nullptr, dummy, n1, 0.5,
          root0c, rootpic, rootpis, close_td);
    CHECK_EQ(nchins, 2);
    CHECK_EQ(chins[0], 1.0);
    CHECK_EQ(chins[1], -rez[0]);
    CHECK(!rootpic);
    CHECK(!root0c);
}

TEST("f1rst: real root, not near +-1, |rez|>=rmod -> stationary trend (chis)") {
    double chis[4] = {1.0, 0, 0, 0};
    int nchis = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    double rez[1] = {0.8}, imz[1] = {0.0};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(1, imz, rez, nullptr, 1.0, 12, dummy, n1, dummy, n1, dummy, n1,
          dummy, n1, chis, nchis, nullptr, dummy, n1, /*rmod=*/0.5, root0c,
          rootpic, rootpis, close_td);
    CHECK_EQ(nchis, 2);
    CHECK_EQ(chis[1], -rez[0]);
    CHECK(!rootpic);
}

TEST("f1rst: real root, not near +-1, |rez|<rmod -> cycle (cycs), rootpic set") {
    double cycs[4] = {1.0, 0, 0, 0};
    int ncycs = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    double rez[1] = {0.3}, imz[1] = {0.0};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(1, imz, rez, nullptr, 1.0, 12, dummy, n1, dummy, n1, cycs, ncycs,
          dummy, n1, dummy, n1, nullptr, dummy, n1, /*rmod=*/0.5, root0c,
          rootpic, rootpis, close_td);
    CHECK_EQ(ncycs, 2);
    CHECK_EQ(cycs[1], -rez[0]);
    CHECK(rootpic);
}

TEST("f1rst: real root near -1, mq!=1 -> nonstationary seasonal (psins)") {
    double psins[4] = {1.0, 0, 0, 0};
    int npsins = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    double rez[1] = {-0.9999995}, imz[1] = {0.0};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(1, imz, rez, nullptr, 1.0, 12, dummy, n1, psins, npsins, dummy, n1,
          dummy, n1, dummy, n1, nullptr, dummy, n1, /*rmod=*/0.5, root0c,
          rootpic, rootpis, close_td);
    CHECK_EQ(npsins, 2);
    CHECK_EQ(psins[1], -rez[0]);
    CHECK(rootpic);
}

TEST("f1rst: real root near -1, mq==1 -> nonstationary cycle (cycns)") {
    double cycns[4] = {1.0, 0, 0, 0};
    int ncycns = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    double rez[1] = {-0.9999995}, imz[1] = {0.0};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(1, imz, rez, nullptr, 1.0, /*mq=*/1, cycns, ncycns, dummy, n1,
          dummy, n1, dummy, n1, dummy, n1, nullptr, dummy, n1, /*rmod=*/0.5,
          root0c, rootpic, rootpis, close_td);
    CHECK_EQ(ncycns, 2);
    CHECK_EQ(cycns[1], -rez[0]);
    CHECK(rootpic);
}

TEST("f1rst: real root negative, not near -1, |rez|>=rmods(0.9) -> psis") {
    double psis[4] = {1.0, 0, 0, 0};
    int npsis = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    double rez[1] = {-0.95}, imz[1] = {0.0};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(1, imz, rez, nullptr, 1.0, 12, dummy, n1, dummy, n1, dummy, n1,
          dummy, n1, dummy, n1, nullptr, psis, npsis, /*rmod=*/0.98, root0c,
          rootpic, rootpis, close_td);
    CHECK_EQ(npsis, 2);
    CHECK_EQ(psis[1], -rez[0]);
    CHECK(rootpic);
}

TEST("f1rst: real root negative, not near -1, |rez|<rmods(0.9) -> cycs") {
    double cycs[4] = {1.0, 0, 0, 0};
    int ncycs = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    double rez[1] = {-0.3}, imz[1] = {0.0};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(1, imz, rez, nullptr, 1.0, 12, dummy, n1, dummy, n1, cycs, ncycs,
          dummy, n1, dummy, n1, nullptr, dummy, n1, /*rmod=*/0.98, root0c,
          rootpic, rootpis, close_td);
    CHECK_EQ(ncycs, 2);
    CHECK_EQ(cycs[1], -rez[0]);
    CHECK(rootpic);
}

TEST("f1rst: complex pair, mq==1, modul>rmod & near unit circle -> chins") {
    double chins[4] = {1.0, 0, 0, 0};
    int nchins = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    double rez[1] = {0.5}, imz[1] = {0.25}, ar[1] = {10.0}, modul[1] = {1.0000001};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(2, imz, rez, ar, 1.0, /*mq=*/1, dummy, n1, dummy, n1, dummy, n1,
          chins, nchins, dummy, n1, modul, dummy, n1, /*rmod=*/0.5, root0c,
          rootpic, rootpis, close_td);
    CHECK_EQ(nchins, 3);
    CHECK_EQ(chins[0], 1.0);
    CHECK_EQ(chins[1], -2.0 * rez[0]);
    CHECK_EQ(chins[2], rez[0] * rez[0] + imz[0] * imz[0]);
}

TEST("f1rst: complex pair, mq==1, modul>rmod, off unit circle -> chis") {
    double chis[4] = {1.0, 0, 0, 0};
    int nchis = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    double rez[1] = {0.5}, imz[1] = {0.25}, ar[1] = {10.0}, modul[1] = {0.95};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(2, imz, rez, ar, 1.0, /*mq=*/1, dummy, n1, dummy, n1, dummy, n1,
          dummy, n1, chis, nchis, modul, dummy, n1, /*rmod=*/0.5, root0c,
          rootpic, rootpis, close_td);
    CHECK_EQ(nchis, 3);
    CHECK_EQ(chis[1], -2.0 * rez[0]);
    CHECK_EQ(chis[2], rez[0] * rez[0] + imz[0] * imz[0]);
}

TEST("f1rst: complex pair, mq==1, modul<=rmod -> cycs") {
    double cycs[4] = {1.0, 0, 0, 0};
    int ncycs = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    double rez[1] = {0.5}, imz[1] = {0.25}, ar[1] = {10.0}, modul[1] = {0.3};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(2, imz, rez, ar, 1.0, /*mq=*/1, dummy, n1, dummy, n1, cycs, ncycs,
          dummy, n1, dummy, n1, modul, dummy, n1, /*rmod=*/0.5, root0c,
          rootpic, rootpis, close_td);
    CHECK_EQ(ncycs, 3);
    CHECK_EQ(cycs[1], -2.0 * rez[0]);
    CHECK_EQ(cycs[2], rez[0] * rez[0] + imz[0] * imz[0]);
}

TEST("f1rst: complex pair, mq=12 fast path (modul>rmod,|ar|<15) -> chis") {
    double chis[4] = {1.0, 0, 0, 0};
    int nchis = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    double rez[1] = {0.5}, imz[1] = {0.25}, ar[1] = {5.0}, modul[1] = {0.95};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(2, imz, rez, ar, /*epsphi=*/1.0, 12, dummy, n1, dummy, n1, dummy,
          n1, dummy, n1, chis, nchis, modul, dummy, n1, /*rmod=*/0.5, root0c,
          rootpic, rootpis, close_td);
    CHECK_EQ(nchis, 3);
    CHECK_EQ(chis[1], -2.0 * rez[0]);
    CHECK_EQ(chis[2], rez[0] * rez[0] + imz[0] * imz[0]);
    CHECK(!close_td);  // fast path never touches is_close_to_td
}

TEST("f1rst: complex pair, mq=12, arg near a seasonal harmonic -> psis") {
    double psis[4] = {1.0, 0, 0, 0};
    int npsis = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    // ar=30 is exactly the mq=12 first harmonic (k=360/12=30); epsphi=1.0
    // puts it inside the +-1deg admission window with modul>=rmods(0.9).
    double rez[1] = {0.5}, imz[1] = {0.25}, ar[1] = {30.0}, modul[1] = {0.95};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(2, imz, rez, ar, /*epsphi=*/1.0, 12, dummy, n1, dummy, n1, dummy,
          n1, dummy, n1, dummy, n1, modul, psis, npsis, /*rmod=*/0.5, root0c,
          rootpic, rootpis, close_td);
    CHECK_EQ(npsis, 3);
    CHECK_EQ(psis[1], -2.0 * rez[0]);
    CHECK_EQ(psis[2], rez[0] * rez[0] + imz[0] * imz[0]);
}

TEST("f1rst: complex pair, mq=12, arg off harmonics -> cycs, is_close_to_td=false") {
    double cycs[4] = {1.0, 0, 0, 0};
    int ncycs = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    double rez[1] = {0.5}, imz[1] = {0.25}, ar[1] = {15.0}, modul[1] = {0.4};
    bool root0c = false, rootpic = false, rootpis = false, close_td = true;
    f1rst(2, imz, rez, ar, /*epsphi=*/1.0, 12, dummy, n1, dummy, n1, cycs,
          ncycs, dummy, n1, dummy, n1, modul, dummy, n1, /*rmod=*/0.5, root0c,
          rootpic, rootpis, close_td);
    CHECK_EQ(ncycs, 3);
    CHECK_EQ(cycs[1], -2.0 * rez[0]);
    // IsCloseToTD assigns unconditionally on this path -- must flip from the
    // true seed above to the oracle's is_close_td(15,12)==false.
    CHECK(!close_td);
}

TEST("f1rst: complex pair, mq=12, arg off harmonics near TD frequency -> is_close_to_td=true") {
    double cycs[4] = {1.0, 0, 0, 0};
    int ncycs = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    double rez[1] = {0.5}, imz[1] = {0.25}, ar[1] = {125.0}, modul[1] = {0.3};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(2, imz, rez, ar, /*epsphi=*/1.0, 12, dummy, n1, dummy, n1, cycs,
          ncycs, dummy, n1, dummy, n1, modul, dummy, n1, /*rmod=*/0.5, root0c,
          rootpic, rootpis, close_td);
    CHECK_EQ(ncycs, 3);
    CHECK(close_td);
}

TEST("f1rst: p=3, trailing real root near -1 (mq==1) -> cycns + root0c") {
    // Leading pair falls into cycs (mq==1, modul<=rmod); the p<=2 return is
    // skipped and index-3's rez is classified separately.
    double cycs[4] = {1.0, 0, 0, 0}, cycns[4] = {1.0, 0, 0, 0};
    int ncycs = 1, ncycns = 1;
    double dummy[4] = {1.0, 0, 0, 0};
    int n1 = 1;
    double rez[3] = {0.5, 0.0, -0.9999995};
    double imz[3] = {0.25, 0.0, 0.0};
    double ar[3] = {10.0, 0, 0}, modul[3] = {0.3, 0, 0};
    bool root0c = false, rootpic = false, rootpis = false, close_td = false;
    f1rst(3, imz, rez, ar, 1.0, /*mq=*/1, cycns, ncycns, dummy, n1, cycs,
          ncycs, dummy, n1, dummy, n1, modul, dummy, n1, /*rmod=*/0.5,
          root0c, rootpic, rootpis, close_td);
    CHECK_EQ(ncycs, 3);  // leading complex pair folded into cycs
    CHECK_EQ(cycs[1], -2.0 * rez[0]);
    CHECK_EQ(ncycns, 2);  // trailing real root folded into cycns
    CHECK_EQ(cycns[1], -rez[2]);
    CHECK(root0c);
}

TEST("isCloseTD: mq=12, arg inside the TD window -> true") {
    CHECK(is_close_td(125.0, 12));
}

TEST("isCloseTD: mq=12, arg outside the TD window -> false") {
    CHECK(!is_close_td(15.0, 12));
}

TEST("isCloseTD: mq=4, arg at the TD center -> true") {
    CHECK(is_close_td(0.0892 * 180.0, 4));
}

TEST("isCloseTD: mq=4, arg far from the TD center -> false") {
    CHECK(!is_close_td(0.0, 4));
}

TEST("isCloseTD: unhandled mq (e.g. 6) is always false") {
    CHECK(!is_close_td(125.0, 6));
    CHECK(!is_close_td(30.0, 6));
}

// --------------------------------------------------------------------------
// seats_init_denoms (sigex.f:476-540) -- nonstationary/stationary denominator
// setup from the differencing orders, run before F1RST allocates the
// stationary AR roots on top. bp=0 (no seasonal AR) in most cases below, so
// the near-unit-seasonal-AR branch is exercised separately.
// --------------------------------------------------------------------------

TEST("seats_init_denoms: d=1,bd=0 (payems-style pure regular diff) -> chins=(1,-1)") {
    double chins[8] = {0}, chis[8] = {0}, psins[32] = {0}, psis[20] = {0},
           cycns[4] = {0}, cycs[16] = {0};
    int nchins = 0, nchis = 0, npsins = 0, npsis = 0, ncycns = 0, ncycs = 0;
    double bphi[13] = {0};
    seats_init_denoms(/*d=*/1, /*bd=*/0, /*bp=*/0, bphi, /*mq=*/12, chins,
                       nchins, chis, nchis, psins, npsins, psis, npsis, cycns,
                       ncycns, cycs, ncycs);
    CHECK_EQ(nchins, 2);
    CHECK_EQ(chins[0], 1.0);
    CHECK_EQ(chins[1], -1.0);
    CHECK_EQ(nchis, 1);
    CHECK_EQ(chis[0], 1.0);
    CHECK_EQ(npsins, 1);
    CHECK_EQ(psins[0], 1.0);
    CHECK_EQ(npsis, 1);
    CHECK_EQ(ncycns, 1);
    CHECK_EQ(ncycs, 1);
}

TEST("seats_init_denoms: d=1,bd=1,mq=12 -> chins=(1,-2,1), psins=12-term summation") {
    double chins[8] = {0}, chis[8] = {0}, psins[32] = {0}, psis[20] = {0},
           cycns[4] = {0}, cycs[16] = {0};
    int nchins = 0, nchis = 0, npsins = 0, npsis = 0, ncycns = 0, ncycs = 0;
    double bphi[13] = {0};
    seats_init_denoms(/*d=*/1, /*bd=*/1, /*bp=*/0, bphi, /*mq=*/12, chins,
                       nchins, chis, nchis, psins, npsins, psis, npsis, cycns,
                       ncycns, cycs, ncycs);
    // dplusd = d+bd = 2 -> (1-B)^2 = (1,-2,1).
    CHECK_EQ(nchins, 3);
    CHECK_EQ(chins[0], 1.0);
    CHECK_EQ(chins[1], -2.0);
    CHECK_EQ(chins[2], 1.0);
    // bd==1 -> ONE multiply by (1+B+...+B^11): psins is all-ones, length 12.
    CHECK_EQ(npsins, 12);
    for (int i = 0; i < 12; ++i) CHECK_EQ(psins[i], 1.0);
}

TEST("seats_init_denoms: d=0,bd=2,mq=4 -> psins is (1+B+B^2+B^3)^2") {
    double chins[8] = {0}, chis[8] = {0}, psins[32] = {0}, psis[20] = {0},
           cycns[4] = {0}, cycs[16] = {0};
    int nchins = 0, nchis = 0, npsins = 0, npsis = 0, ncycns = 0, ncycs = 0;
    double bphi[5] = {0};
    seats_init_denoms(/*d=*/0, /*bd=*/2, /*bp=*/0, bphi, /*mq=*/4, chins,
                       nchins, chis, nchis, psins, npsins, psis, npsis, cycns,
                       ncycns, cycs, ncycs);
    CHECK_EQ(nchins, 3);  // dplusd = 0+2 = 2 -> (1-B)^2 = (1,-2,1)
    CHECK_EQ(chins[1], -2.0);
    // bd==2 (!=1) -> TWO multiplies by (1+B+B^2+B^3):
    // (1+B+B^2+B^3)^2 = 1+2B+3B^2+4B^3+3B^4+2B^5+B^6.
    CHECK_EQ(npsins, 7);
    double expect[7] = {1, 2, 3, 4, 3, 2, 1};
    for (int i = 0; i < 7; ++i) CHECK_EQ(psins[i], expect[i]);
}

TEST("seats_init_denoms: near-unit seasonal AR root (bphi(mq+1)<0, not exactly unit) -> chis/psis fold") {
    double chins[8] = {0}, chis[8] = {0}, psins[32] = {0}, psis[20] = {0},
           cycns[4] = {0}, cycs[16] = {0};
    int nchins = 0, nchis = 0, npsins = 0, npsis = 0, ncycns = 0, ncycs = 0;
    // bphi(mq+1) = bphi[mq] (0-based) = -0.81 -> cmu = 0.81^(1/12), not
    // within 1e-13 of 1 -> folds into Chis/Psis, not Chins/Psins.
    double bphi[13] = {0};
    bphi[12] = -0.81;
    seats_init_denoms(/*d=*/0, /*bd=*/0, /*bp=*/1, bphi, /*mq=*/12, chins,
                       nchins, chis, nchis, psins, npsins, psis, npsis, cycns,
                       ncycns, cycs, ncycs);
    const double cmu = std::pow(0.81, 1.0 / 12.0);
    CHECK_EQ(nchins, 1);  // dplusd=0 and NOT near-unit -> chins untouched
    CHECK_EQ(nchis, 2);
    CHECK_EQ(chis[0], 1.0);
    CHECK_EQ(chis[1], -cmu);
    CHECK_EQ(npsins, 1);  // bd==0 -> no summation fold into psins
    CHECK_EQ(npsis, 12);  // near-unit-SAR geometric factor folds into psis
    double dum_i = 1.0;
    for (int i = 0; i < 12; ++i) {
        CHECK_EQ(psis[i], dum_i);
        dum_i *= cmu;
    }
    CHECK_EQ(ncycs, 1);  // bphi(mq+1)<0 -- the >0 Cycs branch does NOT fire
}

TEST("seats_init_denoms: positive seasonal-AR coefficient -> full poly into Cycs") {
    double chins[8] = {0}, chis[8] = {0}, psins[32] = {0}, psis[20] = {0},
           cycns[4] = {0}, cycs[16] = {0};
    int nchins = 0, nchis = 0, npsins = 0, npsis = 0, ncycns = 0, ncycs = 0;
    double bphi[13] = {0};
    bphi[0] = 1.0;
    bphi[12] = 0.5;  // bphi(mq+1) > 0 -> Cycs = bphi verbatim, length mq+1
    seats_init_denoms(/*d=*/0, /*bd=*/0, /*bp=*/1, bphi, /*mq=*/12, chins,
                       nchins, chis, nchis, psins, npsins, psis, npsis, cycns,
                       ncycns, cycs, ncycs);
    CHECK_EQ(ncycs, 13);
    for (int i = 0; i < 13; ++i) CHECK_EQ(cycs[i], bphi[i]);
    CHECK_EQ(ncycns, 1);
    CHECK_EQ(nchins, 1);  // no near-unit fold (bphi(mq+1) is not < 0)
    CHECK_EQ(nchis, 1);
    CHECK_EQ(npsins, 1);
    CHECK_EQ(npsis, 1);
}

// --------------------------------------------------------------------------
// TRANS0/TRANS2 (transc.f) -- bounded reparametrization round-trip used by
// nmlmdl.f (model_decode.cpp). Values are hand-derived, not oracle captures:
// for order-1 groups the round trip is a pure double-negation identity
// (bit-exact); for order-2 groups the second coefficient is also an exact
// passthrough, while the first involves one division then one multiplication
// (X(M+1)/(1-X(N)) then *(1-X(N))) that is NOT generally bit-exact in IEEE754
// but is exact to within a handful of ULPs. See model_decode.hpp for the full
// derivation, including why the order>=3 cubic-root (Alph) branch is provably
// dead code (never feeds the P/C output) and is omitted from the port.
// --------------------------------------------------------------------------

TEST("trans0/trans2: order-1 round trip is bit-exact") {
    double p_raw[1] = {0.3};
    double x[3] = {0, 0, 0};
    trans0(p_raw, 3, x, 1, 1, 1, 1.0, 0.99);
    double p2[3] = {0, 0, 0};
    trans2(p2, 3, x, 0, 1);
    CHECK_EQ(p2[0], p_raw[0]);
}

TEST("trans0/trans2: order-1 round trip, negative coefficient, bit-exact") {
    double p_raw[1] = {-0.55};
    double x[3] = {0, 0, 0};
    trans0(p_raw, 3, x, 1, 1, 1, 1.0, 0.99);
    double p2[3] = {0, 0, 0};
    trans2(p2, 3, x, 0, 1);
    CHECK_EQ(p2[0], p_raw[0]);
}

TEST("trans0/trans2: order-2 round trip -- 2nd coeff exact, 1st within a few ULPs") {
    double p_raw[2] = {0.3, -0.1};
    double x[3] = {0, 0, 0};
    trans0(p_raw, 3, x, 1, 2, 2, 1.0, 0.99);
    double p2[3] = {0, 0, 0};
    trans2(p2, 3, x, 0, 2);
    CHECK_EQ(p2[1], p_raw[1]);  // pure passthrough: exact
    CHECK(std::fabs(p2[0] - p_raw[0]) <= 1.0e-14);
}

// --------------------------------------------------------------------------
// seats_decode_model (nmlmdl.f) -- decode ctx.model's Mdl/Opr/Arimal/Arimap
// into plain (p,d,q)(P,D,Q) orders + phi/bphi/th/bth. Models are built with
// the already-ported mdlint/mdlset (automdl/mdlset.hpp) -- the same
// insopr/iscrfn/mkoprt machinery the spec-driven getmdl.cpp path uses -- then
// "fitted" coefficients are written directly into ctx.mdldat.arimap at the
// slots insopr placed them, exactly mirroring where a converged regARIMA
// estimation would leave them.
// --------------------------------------------------------------------------

TEST("seats_decode_model: payems-style (0,1,2), no seasonal structure") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    ctx.model.sp = 12;
    mdlint(ctx);
    bool inptok = false;
    mdlset(ctx, /*nrar=*/0, /*nrdiff=*/1, /*nrma=*/2, /*nsar=*/0,
           /*nsdiff=*/0, /*nsma=*/0, inptok);
    CHECK(inptok);
    // ardsp = Nnsedf+Nseadf = 1 -> the single diff coefficient occupies
    // Arimap(1); the two nonseasonal MA coefficients land at Arimap(2),(3).
    ctx.mdldat.arimap(2) = 0.0186;   // matches payems_seats.out's printed
    ctx.mdldat.arimap(3) = -0.1687;  // MA(1)/MA(2) estimates (4-decimal spot check)

    SeatsModelOrders mo;
    bool ok = seats_decode_model(ctx, /*xl=*/0.99, mo);
    CHECK(ok);
    CHECK(!ctx.error.lfatal);
    CHECK_EQ(mo.p, 0);
    CHECK_EQ(mo.d, 1);
    CHECK_EQ(mo.q, 2);
    CHECK_EQ(mo.bp, 0);
    CHECK_EQ(mo.bd, 0);
    CHECK_EQ(mo.bq, 0);
    CHECK_EQ(mo.mq, 12);
    // th(1) = -Arimap(2), 2nd-of-group passthrough is exact.
    CHECK_EQ(mo.th[1], -ctx.mdldat.arimap(3));
    CHECK(std::fabs(mo.th[0] - (-ctx.mdldat.arimap(2))) <= 1.0e-14);
}

TEST("seats_decode_model: airline-style (0,1,1)(0,1,1), mq=12") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    ctx.model.sp = 12;
    mdlint(ctx);
    bool inptok = false;
    mdlset(ctx, /*nrar=*/0, /*nrdiff=*/1, /*nrma=*/1, /*nsar=*/0,
           /*nsdiff=*/1, /*nsma=*/1, inptok);
    CHECK(inptok);
    // ardsp = Nnsedf+Nseadf = 1+1 = 2 -> Arimap(1),(2) are the two diff
    // coefficients; Arimap(3) is the nonseasonal MA(1) coefficient,
    // Arimap(4) the seasonal MA(1) (at lag Sp=12) coefficient.
    ctx.mdldat.arimap(3) = -0.35;
    ctx.mdldat.arimap(4) = -0.60;

    SeatsModelOrders mo;
    bool ok = seats_decode_model(ctx, /*xl=*/0.99, mo);
    CHECK(ok);
    CHECK_EQ(mo.p, 0);
    CHECK_EQ(mo.d, 1);
    CHECK_EQ(mo.q, 1);
    CHECK_EQ(mo.bp, 0);
    CHECK_EQ(mo.bd, 1);
    CHECK_EQ(mo.bq, 1);
    CHECK_EQ(mo.mq, 12);
    // Order-1 groups: the whole TRANS0/TRANS2 round trip is a pure
    // double-negation identity -- bit-exact.
    CHECK_EQ(mo.th[0], -ctx.mdldat.arimap(3));
    CHECK_EQ(mo.bth[0], -ctx.mdldat.arimap(4));
}

TEST("seats_decode_model: operator group with >3 lags abends (SEATS's own limit)") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    ctx.model.sp = 12;
    mdlint(ctx);
    bool inptok = false;
    // A 4-lag nonseasonal MA operator exceeds nmlmdl.f's Nn=3 cap.
    mdlset(ctx, /*nrar=*/0, /*nrdiff=*/0, /*nrma=*/4, /*nsar=*/0,
           /*nsdiff=*/0, /*nsma=*/0, inptok);
    CHECK(inptok);
    for (int i = 1; i <= 4; ++i) ctx.mdldat.arimap(i) = 0.1 * i;

    SeatsModelOrders mo;
    bool ok = seats_decode_model(ctx, /*xl=*/0.99, mo);
    CHECK(!ok);
    CHECK(ctx.error.lfatal);
}

// --------------------------------------------------------------------------
// seats_canonical_denoms -- wires RPQ -> seats_init_denoms -> F1RST on a real
// DECODED model (tools/seats_scope.md next-increment #2). No oracle capture
// exists at this layer yet (SPECTRU, the next real consumer, isn't ported),
// so this is verified via the internal consistency identity the scope doc
// itself suggests: Chi*Psi*Cyc (the product of all six running denominators
// F1RST distributes the AR roots across) must reproduce
// conv((1-B)^(d+bd), phis) -- the differencing factor seats_init_denoms seeds
// directly, times the full stationary AR polynomial F1RST's root-by-root
// classification is only ever REARRANGING, never altering the product of.
// --------------------------------------------------------------------------

TEST("seats_canonical_denoms: expgs-style AR(2), d=1, bp=0 -- Chi*Psi*Cyc reproduces (1-B)^d * phis") {
    SeatsModelOrders mo;
    mo.p = 2;
    mo.phi[0] = 0.3;
    mo.phi[1] = -0.2;
    mo.d = 1;
    mo.mq = 12;
    SeatsCanonicalDenoms out;
    seats_canonical_denoms(mo, /*rmod=*/0.5, /*epsphi=*/2.0, out);

    // RPQ found mo.p roots (2 real or a conjugate pair).
    // Expected: conv((1-B)^d, phis).
    double phis[3] = {1.0, -mo.phi[0], -mo.phi[1]};
    double onemb[2] = {1.0, -1.0};
    double expect[8] = {0};
    int nexpect = 0;
    conv(onemb, 2, phis, 3, expect, nexpect);
    CHECK_EQ(nexpect, 4);

    // Produced: Chi*Psi*Cyc = conv(conv(chins,chis), conv(conv(psins,psis), conv(cycns,cycs))).
    double chi[16] = {0}, psi[16] = {0}, cyc[16] = {0};
    int nchi = 0, npsi = 0, ncyc = 0;
    conv(out.chins, out.nchins, out.chis, out.nchis, chi, nchi);
    conv(out.psins, out.npsins, out.psis, out.npsis, psi, npsi);
    conv(out.cycns, out.ncycns, out.cycs, out.ncycs, cyc, ncyc);
    double chipsi[32] = {0};
    int nchipsi = 0;
    conv(chi, nchi, psi, npsi, chipsi, nchipsi);
    double totden[64] = {0};
    int ntotden = 0;
    conv(chipsi, nchipsi, cyc, ncyc, totden, ntotden);

    CHECK_EQ(ntotden, nexpect);
    for (int i = 0; i < nexpect; ++i)
        CHECK(std::fabs(totden[i] - expect[i]) <= 1.0e-12);
}

// --------------------------------------------------------------------------
// SPECTRU (spectrum.f:558-1190) -- the real canonical-decomposition driver.
//
// SESSION-5 FIX: session 4 left qt1 NOT matching the oracle's irrvar .mdc
// key (14.9% high on unrate_seats, wrong-signed on payems_seats), with MINIM
// and CONV/CONJ/MULTFN/DIVFCN independently verified NOT at fault. This
// session traced it to canonical_denoms.cpp's `thstar` (MA numerator
// polynomial) sign: built as `ths(i+1)=-Th(i)`, literally matching
// analts.f:2854-2859's text -- but empirically wrong. The fix
// (`ths(i+1)=+Th(i)`, no negation -- see canonical_denoms.cpp) makes qt1
// match golden irrvar BIT-EXACT on unrate_seats AND payems_seats (including
// the qstar>pstar ADDJ-fold path), and on all 7 corpus specs with a golden
// .mdc via tests/parity/test_seats_tables.py::test_seats_qt1. WHY the
// literal Fortran text disagrees with the empirically-correct sign is not
// resolved (flagged in canonical_denoms.cpp and tools/seats_scope.md for a
// future session); the direction itself is now confirmed on real data, not
// guesswork.
//
// These two tests reproduce the golden .mdc irrvar values directly (not
// hand-derived): payems_seats (0,1,2) exercises the qstar>pstar ADDJ fold;
// unrate_seats (0,1,1) is the clean case (qstar==pstar, no ADDJ).
// th1/th2/saden here are copied verbatim from the golden .mdc's
// sanum/saden keys (see tests/golden/generated/<base>/<base>.mdc).
// --------------------------------------------------------------------------

TEST("spectru: payems-style (0,1,2) qt1 BIT-EXACT vs oracle irrvar (ADDJ fold path)") {
    double chi[2] = {1.0, -1.0};
    double cyc[1] = {1.0};
    double psi[1] = {1.0};
    // sanum.001/.002 from payems_seats.mdc == Th(1)/Th(2) directly (no
    // negation -- see the session-5 note above).
    const double th1 = 0.0185932634268113, th2 = -0.168650371825201;
    double thstar[3] = {1.0, th1, th2};
    SpectruResult r;
    spectru(thstar, /*qstar=*/3, chi, /*nchi=*/2, cyc, /*ncyc=*/1, psi,
            /*npsi=*/1, /*pstar=*/2, /*mq=*/12, /*bd=*/0, /*d=*/1, /*out=*/1,
            /*har=*/0, /*root0c=*/false, /*rootpic=*/false,
            /*rootpis=*/false, r);
    // Not CHECK_EQ: th1/th2 above are hand-typed from the .mdc's printed
    // 15-sig-fig decimal, which does not necessarily round-trip to the exact
    // same double as the pipeline's internal (higher-precision) value: the
    // REAL pipeline (x13run_seats on payems_seats.spc) prints DECODE_QT1
    // matching golden irrvar as an EXACT STRING at 15 significant digits
    // (verified this session); this hand-literal version is checked to a
    // tight relative tolerance instead to avoid a spurious last-ULP failure.
    const double gold_irrvar = 0.165143227109591;
    CHECK(std::fabs(r.qt1 - gold_irrvar) / gold_irrvar <= 1.0e-13);
    CHECK(!r.is_ugly);
    CHECK_EQ(r.h.nut, 1);  // trend branch fired (nchi=2>1, npsi==1, ncyc==1)
}

TEST("spectru: unrate-style (0,1,1) qt1 BIT-EXACT vs oracle irrvar (no ADDJ fold)") {
    double chi[2] = {1.0, -1.0};
    double cyc[1] = {1.0};
    double psi[1] = {1.0};
    // sanum.001 from unrate_seats.mdc == Th(1) directly.
    const double th1 = 0.034645248012532;
    double thstar[2] = {1.0, th1};
    SpectruResult r;
    spectru(thstar, /*qstar=*/2, chi, /*nchi=*/2, cyc, /*ncyc=*/1, psi,
            /*npsi=*/1, /*pstar=*/2, /*mq=*/12, /*bd=*/0, /*d=*/1, /*out=*/1,
            /*har=*/0, /*root0c=*/false, /*rootpic=*/false,
            /*rootpis=*/false, r);
    // See the payems test above for why this is a tolerance, not CHECK_EQ.
    const double gold_irrvar = 0.232977449296196;
    CHECK(std::fabs(r.qt1 - gold_irrvar) / gold_irrvar <= 1.0e-13);
    CHECK(!r.is_ugly);
}

// --------------------------------------------------------------------------
// decomp_spectrum (spectrum.f:1380-2513 DecompSpectrum + spectrum.f:2710-
// 2890 MAspectrum) -- turns SPECTRU's Ut/V/Uc into the final per-component
// MA numerators (THETP/THETS/THETC/THADJ) + variances via the already-
// ported MAK1. These two tests build a REAL model (via seats_decode_model's
// sibling seats_canonical_denoms, not hand-typed chi/cyc/psi/thstar) and
// diff the result against the golden .mdc's sanum/saden/savar/trnum/trden/
// trvar keys -- confirmed bit-exact via the real x13run_seats pipeline
// (session 6); the tolerance here (not CHECK_EQ) is only because th1/th2
// are hand-typed literals, same caveat as the spectru tests above.
// --------------------------------------------------------------------------

TEST("decomp_spectrum: payems-style (0,1,2) sanum/saden/savar/trnum/trden/trvar vs oracle .mdc") {
    SeatsModelOrders mo;
    mo.p = 0;
    mo.d = 1;
    mo.q = 2;
    mo.mq = 12;
    mo.th[0] = 0.0185932634268113;
    mo.th[1] = -0.168650371825201;
    SeatsCanonicalDenoms cd;
    seats_canonical_denoms(mo, /*rmod=*/0.5, /*epsphi=*/2.0, cd);

    SpectruResult sr;
    spectru(cd.thstar, cd.qstar, cd.chi, cd.nchi, cd.cyc, cd.ncyc, cd.psi,
            cd.npsi, cd.pstar, mo.mq, mo.bd, mo.d, /*out=*/1, /*har=*/0,
            cd.root0c, cd.rootpic, cd.rootpis, sr);

    SeatsComponentModels comp;
    decomp_spectrum(sr, cd, cd.is_close_to_td, comp);

    // sanum/saden/savar <- THADJ/CHCYC/VARWNA (npsi==1 trivial-copy branch).
    CHECK_EQ(comp.nthadj, 3);
    CHECK_EQ(comp.thadj[0], 1.0);
    CHECK(std::fabs(comp.thadj[1] - 0.0185932634268113) <= 1.0e-13);
    CHECK(std::fabs(comp.thadj[2] - (-0.168650371825201)) <= 1.0e-13);
    CHECK_EQ(cd.nchcyc, 2);
    CHECK_EQ(cd.chcyc[0], 1.0);
    CHECK_EQ(cd.chcyc[1], -1.0);
    CHECK_EQ(comp.varwna, 1.0);

    // trnum/trden/trvar <- THETC/CYC/VARWNC -- the qstar>pstar ADDJ fold
    // sets ncycth=1, so this branch (absent for unrate below) fires.
    CHECK_EQ(sr.ncycth, 1);
    CHECK_EQ(comp.nthetc, 2);
    CHECK(std::fabs(comp.thetc[0] - 1.0) <= 1.0e-12);
    CHECK(std::fabs(comp.thetc[1] - 1.0) <= 1.0e-12);
    CHECK_EQ(cd.ncyc, 1);
    CHECK_EQ(cd.cyc[0], 1.0);
    CHECK(std::fabs(comp.varwnc - 0.168650371825201) <= 1.0e-12);
}

TEST("decomp_spectrum: unrate-style (0,1,1) sanum/saden/savar vs oracle .mdc (ncycth=0 -> no trnum)") {
    SeatsModelOrders mo;
    mo.p = 0;
    mo.d = 1;
    mo.q = 1;
    mo.mq = 12;
    mo.th[0] = 0.034645248012532;
    SeatsCanonicalDenoms cd;
    seats_canonical_denoms(mo, /*rmod=*/0.5, /*epsphi=*/2.0, cd);

    SpectruResult sr;
    spectru(cd.thstar, cd.qstar, cd.chi, cd.nchi, cd.cyc, cd.ncyc, cd.psi,
            cd.npsi, cd.pstar, mo.mq, mo.bd, mo.d, /*out=*/1, /*har=*/0,
            cd.root0c, cd.rootpic, cd.rootpis, sr);

    SeatsComponentModels comp;
    decomp_spectrum(sr, cd, cd.is_close_to_td, comp);

    CHECK_EQ(comp.nthadj, 2);
    CHECK_EQ(comp.thadj[0], 1.0);
    CHECK(std::fabs(comp.thadj[1] - 0.034645248012532) <= 1.0e-13);
    CHECK_EQ(cd.nchcyc, 2);
    CHECK_EQ(cd.chcyc[0], 1.0);
    CHECK_EQ(cd.chcyc[1], -1.0);
    CHECK_EQ(comp.varwna, 1.0);
    // No ADDJ fold (qstar==pstar) and no genuine cycle -> trnum/trden/trvar
    // are NOT emitted by the oracle either (unrate_seats.mdc has none).
    CHECK_EQ(sr.ncycth, 0);
    CHECK_EQ(cd.ncyc, 1);
}

TEST("seats_canonical_denoms: p=0 (payems-style, pure MA) -- denominators reduce to (1-B)^d") {
    SeatsModelOrders mo;
    mo.p = 0;
    mo.d = 1;
    mo.mq = 12;
    SeatsCanonicalDenoms out;
    seats_canonical_denoms(mo, /*rmod=*/0.5, /*epsphi=*/2.0, out);
    // F1RST never ran (p==0) -- chis/psins/psis/cycns/cycs stay at their
    // seats_init_denoms-seeded {1.0} identity, chins carries the whole (1-B)^d.
    CHECK_EQ(out.nchins, 2);
    CHECK_EQ(out.chins[0], 1.0);
    CHECK_EQ(out.chins[1], -1.0);
    CHECK_EQ(out.nchis, 1);
    CHECK_EQ(out.npsins, 1);
    CHECK_EQ(out.npsis, 1);
    CHECK_EQ(out.ncycns, 1);
    CHECK_EQ(out.ncycs, 1);
}
