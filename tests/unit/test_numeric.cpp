// test_numeric.cpp -- parity unit tests for the M3 Tier-0 numeric leaves.
//
// Expected values are REFERENCE OUTPUTS captured from the vendored oracle
// Fortran (dpmpar.f/dpeq.f/enorm.f/ddot.f/daxpy.f/dcopy.f/scrmlt.f/maxvec.f/
// revrse.f) compiled with rtools44 gfortran -O2. See tools/ref_tier0.f for the
// driver that regenerates them. The point is bit-faithfulness to the oracle,
// including the truncated dpmpar literal and ddot's underflow-skip.
#include "microtest.hpp"
#include "numeric/numeric.hpp"

#include <cmath>

using namespace x13;

namespace {
// relative closeness for the irrational (sqrt-derived) norms; the exact-value
// cases below use CHECK_EQ.
bool rclose(double a, double b, double rtol) {
    double d = std::fabs(a - b);
    double s = std::fabs(b);
    return d <= rtol * (s > 1.0 ? s : 1.0);
}
}  // namespace

TEST("dpmpar: truncated oracle literals, not machine epsilon") {
    CHECK_EQ(dpmpar(1), 2.220446e-16);
    CHECK_EQ(dpmpar(2), 2.225074e-308);
    CHECK_EQ(dpmpar(3), 1.797693e308);
    // The oracle value is deliberately NOT DBL_EPSILON.
    CHECK(dpmpar(1) != 2.220446049250313e-16);
}

TEST("dpeq: |x-target| < 3.834e-20") {
    CHECK(dpeq(3.0e-20, 0.0));         // below DELTA
    CHECK(!dpeq(4.0e-20, 0.0));        // above DELTA
    CHECK(!dpeq(3.834e-20, 0.0));      // exactly DELTA is not < DELTA
    CHECK(dpeq(1.0, 1.0));
}

TEST("scrmlt: scale in place") {
    double c[4] = {1.0, 2.0, 3.0, 4.0};
    scrmlt(2.5, 4, c);
    CHECK_EQ(c[2], 7.5);  // oracle scrmlt_c3
    CHECK_EQ(c[0], 2.5);
}

TEST("maxvec: max magnitude") {
    double a[4] = {-7.0, 3.0, -9.5, 2.0};
    double mx = -1.0;
    maxvec(a, 4, mx);
    CHECK_EQ(mx, 9.5);  // oracle maxvec
    double z = 42.0;
    maxvec(a, 0, z);
    CHECK_EQ(z, 0.0);  // n<=0 -> 0
}

TEST("dcopy: strided copy incx=1 incy=2") {
    double a[3] = {11.0, 22.0, 33.0};
    double b[8];
    for (int i = 0; i < 8; ++i) b[i] = -1.0;
    dcopy(3, a, 1, b, 2);
    CHECK_EQ(b[0], 11.0);  // oracle dcopy_b1
    CHECK_EQ(b[2], 22.0);  // dcopy_b3
    CHECK_EQ(b[4], 33.0);  // dcopy_b5
    CHECK_EQ(b[1], -1.0);  // dcopy_b2 untouched
}

TEST("daxpy: dy <- da*dx + dy") {
    double a[5] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double b[5] = {10.0, 20.0, 30.0, 40.0, 50.0};
    daxpy(5, 2.0, a, 1, b, 1);
    double s = 0.0;
    for (int i = 0; i < 5; ++i) s += b[i];
    CHECK_EQ(s, 180.0);  // oracle daxpy_sum
    CHECK_EQ(b[0], 12.0);
    // da == 0 is a no-op (dpeq early-out)
    double c[3] = {1.0, 2.0, 3.0};
    daxpy(3, 0.0, a, 1, c, 1);
    CHECK_EQ(c[1], 2.0);
}

TEST("ddot: underflow-skipping dot product") {
    // first term (1e-160 * 1e-160 = 1e-320) underflows below dpmpar(2) and is
    // dropped: result = 2*5 + 3*7 = 31, NOT 31 + tiny.
    double a[3] = {1.0e-160, 2.0, 3.0};
    double b[3] = {1.0e-160, 5.0, 7.0};
    CHECK_EQ(ddot(3, a, 1, b, 1), 31.0);  // oracle ddot1
    // n=6 exercises the 5-way unroll cleanup.
    double p[6], q[6];
    for (int i = 0; i < 6; ++i) {
        p[i] = i + 1.0;
        q[i] = 6.0 - i;
    }
    CHECK_EQ(ddot(6, p, 1, q, 1), 56.0);  // oracle ddot2
    CHECK_EQ(ddot(0, p, 1, q, 1), 0.0);
}

TEST("revrse: reverse rows of a 4x2 matrix") {
    // rows: [41,42],[51,52],[61,62],[71,72] -> [71,72],[61,62],[51,52],[41,42]
    double a[8];
    for (int i = 1; i <= 4; ++i) {
        a[2 * (i - 1)] = 10.0 * i + 1.0;
        a[2 * (i - 1) + 1] = 10.0 * i + 2.0;
    }
    double b[8];
    revrse(a, 4, 2, b);
    CHECK_EQ(b[0], 41.0);  // oracle revrse_b1 (last row moved to front)
    CHECK_EQ(b[1], 42.0);  // revrse_b2
    CHECK_EQ(b[6], 11.0);  // revrse_b7 (first row moved to back)
    CHECK_EQ(b[7], 12.0);  // revrse_b8
}

TEST("enorm: MINPACK 3-bin scaled norm") {
    double m[2] = {3.0, 4.0};
    CHECK_EQ(enorm(2, m), 5.0);  // intermediate bin, oracle enorm1

    double small[3] = {1.0e-20, 2.0e-20, 3.0e-20};  // all <= RDWARF (small bin)
    CHECK(rclose(enorm(3, small), 3.7416573867739415e-20, 1e-15));  // enorm2

    double large[2] = {1.0e19, 2.0e19};  // > agiant (large bin)
    CHECK(rclose(enorm(2, large), 2.2360679774997897e19, 1e-15));  // enorm3

    double mixed[3] = {1.0e-20, 3.0, 1.0e19};
    CHECK(rclose(enorm(3, mixed), 1.0e19, 1e-15));  // enorm4 (large dominates)
}

int main() { return mt::run_all(); }
