// test_numeric.cpp -- parity unit tests for the M3 Tier-0 numeric leaves.
//
// Expected values are REFERENCE OUTPUTS captured from the vendored oracle
// Fortran (dpmpar.f/dpeq.f/enorm.f/ddot.f/daxpy.f/dcopy.f/scrmlt.f/maxvec.f/
// revrse.f) compiled with rtools44 gfortran -O2. See tools/ref_tier0.f for the
// driver that regenerates them. The point is bit-faithfulness to the oracle,
// including the truncated dpmpar literal and ddot's underflow-skip.
#include "microtest.hpp"
#include "numeric/numeric.hpp"
#include "regarima/armafilt.hpp"

#include <cmath>

namespace x13 {
// ratneg lives in core/src/regarima (declared in regvar.hpp, which pulls in the
// full X13Context); forward-declare it here to keep this leaf test lightweight.
void ratneg(int nelta, const double* arimap, const int* arimal, const int* opr,
            int begopr, int endopr, double* c);
}  // namespace x13

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

// ---- M3 Tier-1 numeric/ARMA leaves. Goldens from tools/ref_tier1.f driving
// the oracle yprmy/logdet/uconv/xpand/ratneg (gfortran -O2, exact match). ----

TEST("yprmy: y'y sum of squares") {
    double y[4] = {1.0, 2.0, 3.0, 4.0};
    double ypy = -1.0;
    yprmy(y, 4, ypy);
    CHECK_EQ(ypy, 30.0);  // oracle yprmy
}

TEST("logdet: 2*sum(log(packed diag))") {
    double ap[6] = {2.0, 9.0, 3.0, 9.0, 9.0, 4.0};  // diag at 1,3,6 = 2,3,4
    double lgdt = -1.0;
    logdet(ap, 3, lgdt);
    // oracle 6.3561076606958906 == 2*ln(24)
    CHECK(rclose(lgdt, 6.3561076606958906, 1e-15));
}

TEST("uconv: MA autocovariance in place") {
    double fulma[3] = {1.0, 0.5, 0.25};
    double c[3] = {0.0, 0.0, 0.0};
    uconv(fulma, 2, c);
    CHECK_EQ(c[0], 1.3125);  // oracle uconv0
    CHECK_EQ(c[1], 0.625);   // uconv1
    CHECK_EQ(c[2], 0.25);    // uconv2
}

TEST("xpand: A(z)/B(z) expansion in place") {
    double b[2] = {0.0, 0.5};             // b[1]=0.5
    double c[4] = {1.0, 0.0, 0.0, 0.0};   // numerator A=[1], na=0
    xpand(b, 1, 0, 3, c, 3);
    CHECK_EQ(c[0], 1.0);      // oracle xpand0
    CHECK_EQ(c[1], -0.5);     // xpand1
    CHECK_EQ(c[2], 0.25);     // xpand2
    CHECK_EQ(c[3], -0.125);   // xpand3
}

TEST("ratneg: negative-power expansion, backward recursion") {
    // single AR operator, lag 1, phi=0.5, nelta=4, c=[1,2,3,4].
    double arimap[1] = {0.5};
    int arimal[1] = {1};
    int opr[2] = {1, 2};  // opr[0]=beglag=1, opr[1]=2 -> endlag=1
    double c[4] = {1.0, 2.0, 3.0, 4.0};
    ratneg(4, arimap, arimal, opr, 1, 1, c);
    CHECK_EQ(c[0], 3.25);  // oracle ratneg1
    CHECK_EQ(c[1], 4.5);   // ratneg2
    CHECK_EQ(c[2], 5.0);   // ratneg3
    CHECK_EQ(c[3], 4.0);   // ratneg4 (never updated; loop starts at i=3)
}

TEST("arflt: conditional AR filter in place") {
    // single AR operator, lag 1, phi=0.5, nelta=5, c=[1,2,3,4,5].
    double arimap[1] = {0.5};
    int arimal[1] = {1};
    int opr[2] = {1, 2};
    double c[5] = {1.0, 2.0, 3.0, 4.0, 5.0};
    int neltc = -1;
    arflt(5, arimap, arimal, opr, 1, 1, c, neltc);
    CHECK_EQ(neltc, 4);    // oracle arflt_neltc
    CHECK_EQ(c[0], 1.5);   // arflt1
    CHECK_EQ(c[1], 2.0);   // arflt2
    CHECK_EQ(c[2], 2.5);   // arflt3
    CHECK_EQ(c[3], 3.0);   // arflt4
    CHECK_EQ(c[4], 5.0);   // arflt5 (beyond neltc, untouched)
}

TEST("euclid: AR-covariance solve + non-stationary early-out") {
    // Fular=[1,0.5], mxarlg=1, maxpq=1, mxmalg=1, G=[2,1].
    double fular[2] = {1.0, 0.5};
    double b[1], a[1];
    double g[2] = {2.0, 1.0};
    int err = -1;
    euclid(fular, b, a, 1, 1, 1, g, err);
    CHECK_EQ(err, 0);  // oracle euclid_err
    CHECK(rclose(g[0], 6.6666666666666663e-01, 1e-15));  // euclid_g0
    CHECK(rclose(g[1], 6.6666666666666663e-01, 1e-15));  // euclid_g1

    // |r|>1 -> non-stationary -> err=1.
    double fular2[2] = {1.0, 1.5};
    double g2[2] = {2.0, 1.0};
    err = -1;
    euclid(fular2, b, a, 1, 1, 1, g2, err);
    CHECK_EQ(err, 1);  // oracle euclid_err2
}

// ---- Packed-Cholesky trio. Goldens from tools/ref_tier2.f (oracle
// xprmx/dppfa/dsolve, gfortran -O2, exact match). ----

TEST("xprmx: packed [X:y]'[X:y] via strided ddot") {
    // 3 obs, 2 X cols + y in col 3. row-major [x1 x2 y] per row.
    double xy[9] = {1, 1, 2, 1, 2, 3, 1, 3, 5};
    double xpx[6] = {0, 0, 0, 0, 0, 0};
    xprmx(xy, 3, 2, 3, xpx);
    CHECK_EQ(xpx[0], 3.0);   // X'X (1,1)
    CHECK_EQ(xpx[1], 6.0);   // (2,1)
    CHECK_EQ(xpx[2], 14.0);  // (2,2)
    CHECK_EQ(xpx[3], 10.0);  // X'y (1)
    CHECK_EQ(xpx[4], 23.0);  // X'y (2)
    CHECK_EQ(xpx[5], 38.0);  // y'y
}

TEST("dppfa: Census packed Cholesky") {
    double ap[3] = {4.0, 2.0, 10.0};  // packed [[4,2],[2,10]]
    int info = -1;
    dppfa(ap, 2, info);
    CHECK_EQ(info, 0);    // oracle dppfa_info
    CHECK_EQ(ap[0], 2.0); // R packed [2,1,3]
    CHECK_EQ(ap[1], 1.0);
    CHECK_EQ(ap[2], 3.0);
}

TEST("dsolve: multi-RHS solve against packed factor") {
    // A x = b with the factor of [[4,2],[2,10]], b=[6,28], nr=2 nc=1.
    double a[3] = {2.0, 1.0, 3.0};  // R from dppfa above
    double b[2] = {6.0, 28.0};
    dsolve(a, 2, 1, true, b);
    CHECK(rclose(b[0], 1.1111111111111094e-01, 1e-14));  // oracle dsolve1
    CHECK(rclose(b[1], 2.7777777777777781e+00, 1e-14));  // dsolve2
}

TEST("mltpos: operator multiply with secpas zero-padding") {
    int opr[3] = {1, 2, 3};
    // Case A: single AR op lag1 phi=0.5, nelta=3, neltc=5, c=[1,2,3,0,0].
    {
        double arimap[1] = {0.5};
        int arimal[1] = {1};
        double c[5] = {1.0, 2.0, 3.0, 0.0, 0.0};
        mltpos(3, arimap, arimal, opr, 1, 1, 5, c);
        CHECK_EQ(c[0], 1.0);    // oracle mltA
        CHECK_EQ(c[1], 1.5);
        CHECK_EQ(c[2], 2.0);
        CHECK_EQ(c[3], -1.5);
        CHECK_EQ(c[4], 0.0);
    }
    // Case B: two AR ops (0.5 then 0.3); second pass runs with secpas=true.
    {
        double arimap[2] = {0.5, 0.3};
        int arimal[2] = {1, 1};
        double c[5] = {1.0, 2.0, 3.0, 0.0, 0.0};
        mltpos(3, arimap, arimal, opr, 1, 2, 5, c);
        CHECK_EQ(c[0], 1.0);    // oracle mltB
        CHECK_EQ(c[1], 1.2);
        CHECK(rclose(c[2], 1.55, 1e-15));
        CHECK(rclose(c[3], -2.1000000000000001, 1e-15));
        CHECK(rclose(c[4], 4.4999999999999996e-01, 1e-15));
    }
}

int main() { return mt::run_all(); }
