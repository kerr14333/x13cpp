// test_seats.cpp -- parity unit tests for the SEATS foundational leaves:
// polynomial arithmetic (CONV/CONJ/MULTFN/DIVFCN) and the complex-polynomial
// root finder (C02AEF/RPQ) ported from oracle/fortran/ansub2.f.
//
// Golden values are HAND-COMPUTED polynomial products/quotients and polynomials
// with KNOWN roots (rather than oracle captures) -- cleaner and independently
// checkable. The exactly-representable cases use CHECK_EQ; the iterative
// root-finder path is checked to full double precision via a tight tolerance.
#include "microtest.hpp"
#include "seats/seatspoly.hpp"

#include <cmath>

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
