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
#include "regarima/armafl.hpp"
#include "regarima/estimate.hpp"

#include <cmath>
#include <memory>

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

TEST("chkrts: operator invertibility detector") {
    // Case A: AR lag1 phi=0.5, degree 1 -> invertible.
    {
        double arimap[1] = {0.5};
        int arimal[1] = {1};
        bool arimaf[1] = {false};
        int opr[2] = {1, 2};
        int oprfac[1] = {1};
        int prbfac = -99;
        CHECK_EQ(chkrts(arimap, arimal, arimaf, opr, oprfac, 1, 1, prbfac),
                 false);       // oracle chkA
        CHECK_EQ(prbfac, -99);  // untouched
    }
    // Case B: phi=1.5, root inside unit circle -> non-invertible, prbfac=1.
    {
        double arimap[1] = {1.5};
        int arimal[1] = {1};
        bool arimaf[1] = {false};
        int opr[2] = {1, 2};
        int oprfac[1] = {1};
        int prbfac = -99;
        CHECK_EQ(chkrts(arimap, arimal, arimaf, opr, oprfac, 1, 1, prbfac),
                 true);        // oracle chkB
        CHECK_EQ(prbfac, 1);
    }
    // Case C: degree-2 op phi=(0.3,0.4), exercises reflection update -> invertible.
    {
        double arimap[2] = {0.3, 0.4};
        int arimal[2] = {1, 2};
        bool arimaf[2] = {false, false};
        int opr[2] = {1, 3};
        int oprfac[1] = {1};
        int prbfac = -99;
        CHECK_EQ(chkrts(arimap, arimal, arimaf, opr, oprfac, 1, 1, prbfac),
                 false);       // oracle chkC
        CHECK_EQ(prbfac, -99);
    }
    // Case D: single lag FIXED (arimaf true) -> operator skipped even though
    // phi=1.5 would be non-invertible.
    {
        double arimap[1] = {1.5};
        int arimal[1] = {1};
        bool arimaf[1] = {true};
        int opr[2] = {1, 2};
        int oprfac[1] = {1};
        int prbfac = -99;
        CHECK_EQ(chkrts(arimap, arimal, arimaf, opr, oprfac, 1, 1, prbfac),
                 false);       // oracle chkD
        CHECK_EQ(prbfac, -99);
    }
}

// ---- intgpg + exctma (stateful; against ref_armafl.f). A minimal pure-MA(2)
// model theta = 0.3 B - 0.2 B^2 is built in the ex-COMMON structs, then intgpg
// factors G'G and exctma exact-MA-filters a length-5 series. rtol 1e-12. ------
TEST("intgpg/exctma: exact MA filter helpers, MA(2) model") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    m.lma = true;
    m.lar = false;
    m.mdl(0) = 1;
    m.mdl(1) = 1;
    m.mdl(2) = 1;
    m.mdl(3) = 2;
    m.opr(0) = 1;
    m.opr(1) = 3;
    m.arimal(1) = 1;
    m.arimal(2) = 2;
    d.arimap(1) = 0.3;
    d.arimap(2) = -0.2;
    m.oprfac(1) = 1;
    m.mxmalg = 2;
    d.lndtcv = -99.0;

    int info = -99;
    intgpg(ctx, 6, info);
    CHECK_EQ(info, 0);                                    // ref gpg_info
    CHECK(rclose(d.chlgpg(1), 1.0540716573838800e+00, 1e-12));   // gpg_c1
    CHECK(rclose(d.chlgpg(2), 2.6343503409358116e-01, 1e-12));   // gpg_c2
    CHECK(rclose(d.chlgpg(3), 1.0204831173577118e+00, 1e-12));   // gpg_c3
    CHECK(rclose(d.lndtcv, 1.4587318714264297e-01, 1e-12));      // gpg_ldt

    double a[200] = {0.0};
    a[0] = 1.0;
    a[1] = 2.0;
    a[2] = -1.0;
    a[3] = 0.5;
    a[4] = 3.0;
    int nelta = 5;
    exctma(ctx, 1, a, nelta, 200);
    CHECK_EQ(nelta, 7);         // ref exc_nelta
    CHECK_EQ(m.nopr, 1);        // ref exc_nopr (side-effect write)
    CHECK(rclose(a[0],  3.2675291615513941e-01, 1e-12));   // exc_a1
    CHECK(rclose(a[1], -1.1575593899396436e-01, 1e-12));   // exc_a2
    CHECK(rclose(a[2],  8.9992263507078274e-01, 1e-12));   // exc_a3
    CHECK(rclose(a[3],  2.2931279783200278e+00, 1e-12));   // exc_a4
    CHECK(rclose(a[4], -4.9204613351814830e-01, 1e-12));   // exc_a5
    CHECK(rclose(a[5], -1.0623943571945005e-01, 1e-12));   // exc_a6
    CHECK(rclose(a[6],  3.0665373959877948e+00, 1e-12));   // exc_a7
}

// ---- armafl (full exact ARMA filter; against ref_armaflx.f). ARMA(1,1) model
// phi=0.5, theta=0.3, no differencing; a length-8 series is filtered to
// residuals with Linit=T, Lckrts=T. Exercises the whole path: chkrts gate,
// intgpg, uconv/euclid/xpand ACVs, the D matrix, chol(var(w_p|z)), the ddot
// correction, and dsolve. rtol 1e-12. ------------------------------------------
TEST("armafl: full exact ARMA filter, ARMA(1,1)") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    m.lar = true;
    m.lma = true;
    m.nopr = 2;
    m.mdl(0) = 1;
    m.mdl(1) = 1;
    m.mdl(2) = 2;
    m.mdl(3) = 3;
    m.opr(0) = 1;
    m.opr(1) = 2;
    m.opr(2) = 3;
    m.arimal(1) = 1;
    m.arimal(2) = 1;
    d.arimap(1) = 0.5;
    d.arimap(2) = 0.3;
    m.arimaf(1) = false;
    m.arimaf(2) = false;
    m.oprfac(1) = 1;
    m.oprfac(2) = 1;
    m.mxarlg = 1;
    m.mxmalg = 1;
    m.mxdflg = 0;
    d.lndtcv = 0.0;

    double mata[200] = {0.0};
    const double series[8] = {1.0, 2.0, -1.0, 0.5, 3.0, -2.0, 1.5, 0.25};
    for (int i = 0; i < 8; ++i) mata[i] = series[i];
    int na = -99, info = -99;
    armafl(ctx, 8, 1, true, true, mata, na, 200, info);

    CHECK_EQ(info, 0);   // ref af_info
    CHECK_EQ(na, 9);     // ref af_na
    CHECK(rclose(d.lndtcv, 5.6954892689151181e-02, 1e-12));  // af_ldt
    const double golden[9] = {
        1.3372276673516541e+00, -3.1248288398110585e-01, 1.4062551348056682e+00,
        -1.5781234595582996e+00, 5.2656296213251008e-01, 2.9079688886397532e+00,
        -2.6276093334080741e+00, 1.7117171999775778e+00, 1.3515159993273351e-02};
    for (int i = 0; i < 9; ++i) CHECK(rclose(mata[i], golden[i], 1e-12));
}

// Case B: AR(2)MA(1) phi=(0.4,-0.2), theta=0.3 -- 2x2 D/Chlvwp, multi-lag ddot.
TEST("armafl: full exact ARMA filter, AR(2)MA(1)") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    m.lar = true;
    m.lma = true;
    m.nopr = 2;
    m.mdl(0) = 1;
    m.mdl(1) = 1;
    m.mdl(2) = 2;
    m.mdl(3) = 3;
    m.opr(0) = 1;
    m.opr(1) = 3;
    m.opr(2) = 4;
    m.arimal(1) = 1;
    m.arimal(2) = 2;
    m.arimal(3) = 1;
    d.arimap(1) = 0.4;
    d.arimap(2) = -0.2;
    d.arimap(3) = 0.3;
    m.arimaf(1) = false;
    m.arimaf(2) = false;
    m.arimaf(3) = false;
    m.oprfac(1) = 1;
    m.oprfac(2) = 1;
    m.mxarlg = 2;
    m.mxmalg = 1;
    m.mxdflg = 0;
    d.lndtcv = 0.0;

    double mata[200] = {0.0};
    const double series[8] = {1.0, 2.0, -1.0, 0.5, 3.0, -2.0, 1.5, 0.25};
    for (int i = 0; i < 8; ++i) mata[i] = series[i];
    int na = -99, info = -99;
    armafl(ctx, 8, 1, true, true, mata, na, 200, info);

    CHECK_EQ(info, 0);   // ref bf_info
    CHECK_EQ(na, 9);     // ref bf_na
    CHECK(rclose(d.lndtcv, 7.8567281094134883e-02, 1e-12));  // bf_ldt
    const double golden[9] = {
        9.7918409817936714e-01, 1.6417415706932461e+00,  3.1140437450799474e-01,
        -1.5065786876476017e+00, 8.4802639370571953e-01, 2.8544079181117157e+00,
        -2.2436776245664856e+00, 2.2268967126300545e+00, -8.1930986210983825e-02};
    for (int i = 0; i < 9; ++i) CHECK(rclose(mata[i], golden[i], 1e-12));
}

// Case C: AIRLINE (0 1 1)(0 1 1)12 -- the production model. DIFF block in arflt
// (Mxdflg=13), seasonal operators (Oprfac=12), 13x13 Chlgpg, exctma neltq=13.
TEST("armafl: airline (0 1 1)(0 1 1)12") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    m.lar = false;
    m.lma = true;
    m.nopr = 4;
    m.mdl(0) = 1; m.mdl(1) = 3; m.mdl(2) = 3; m.mdl(3) = 5;
    m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3; m.opr(3) = 4; m.opr(4) = 5;
    m.arimal(1) = 1; m.arimal(2) = 12; m.arimal(3) = 1; m.arimal(4) = 12;
    d.arimap(1) = 1.0; d.arimap(2) = 1.0; d.arimap(3) = 0.6; d.arimap(4) = 0.5;
    m.arimaf(1) = true; m.arimaf(2) = true;
    m.arimaf(3) = false; m.arimaf(4) = false;
    m.oprfac(1) = 1; m.oprfac(2) = 12; m.oprfac(3) = 1; m.oprfac(4) = 12;
    m.mxarlg = 0; m.mxmalg = 13; m.mxdflg = 13;
    d.lndtcv = 0.0;

    double mata[200] = {0.0};
    for (int i = 1; i <= 40; ++i)  // same integer formula as ref driver
        mata[i - 1] = double(7 * i % 13) - 6.0 + 0.25 * double(3 * i % 8);
    int na = -99, info = -99;
    armafl(ctx, 40, 1, true, true, mata, na, 200, info);

    CHECK_EQ(info, 0);   // cf_info
    CHECK_EQ(na, 40);    // cf_na (27 differenced + 13 exctma init values)
    CHECK(rclose(d.lndtcv, 3.7464539386530329e+00, 1e-12));  // cf_ldt
    CHECK(rclose(mata[0],   7.4454852918611214e-01, 1e-12)); // cf_a1
    CHECK(rclose(mata[1],  -2.2120944119000976e+00, 1e-12)); // cf_a2
    CHECK(rclose(mata[13],  6.8062918685969880e+00, 1e-12)); // cf_a14
    CHECK(rclose(mata[39],  3.3861287209896620e+00, 1e-12)); // cf_alast
}

// Case D: Nopr=0 no-op guard -> Na=Nr, Mata untouched, Info=0.
TEST("armafl: Nopr=0 no-op") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    ctx.model.nopr = 0;
    double mata[8] = {3.14, -2.71, 0, 0, 0, 0, 0, 0};
    int na = -99, info = -99;
    armafl(ctx, 5, 1, true, true, mata, na, 200, info);
    CHECK_EQ(info, 0);           // df_info
    CHECK_EQ(na, 5);             // df_na = Nr
    CHECK_EQ(mata[0], 3.14);     // df_a1 (untouched)
}

// ---- olsreg + resid (regression solve / residuals; against ref_estimate.f).
// 4-obs OLS: intercept + one regressor, y in the 3rd column. rtol 1e-12. -------
TEST("olsreg/resid: OLS normal-equations solve + residuals") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    // [X:y] row-major per obs: [1, x2, y], pcxy=3, nr=4.
    double xy[12] = {1.0, 1.0, 2.1, 1.0, 2.0, 3.9,
                     1.0, 3.0, 6.2, 1.0, 4.0, 7.8};
    double b[2] = {0.0, 0.0};
    double chlxpx[10] = {0.0};
    int info = -99;
    olsreg(ctx, xy, 4, 3, 3, b, chlxpx, 10, info);
    CHECK_EQ(info, 0);                                    // ref ols_info
    CHECK(rclose(b[0], 1.4999999999999858e-01, 1e-12));   // ols_b1
    CHECK(rclose(b[1], 1.9400000000000006e+00, 1e-12));   // ols_b2

    double rsd[4] = {0.0};
    resid(ctx, xy, 4, 3, 3, 1, 2, -1.0, b, rsd);
    CHECK(rclose(rsd[0],  1.0000000000000897e-02, 1e-12));  // rsd1
    CHECK(rclose(rsd[1], -1.2999999999999989e-01, 1e-12));  // rsd2
    CHECK(rclose(rsd[2],  2.2999999999999954e-01, 1e-12));  // rsd3
    CHECK(rclose(rsd[3], -1.1000000000000121e-01, 1e-12));  // rsd4
}

// ---- upespm (scatter estprm -> arimap, skip fixed lags; against ref_upespm.f).
TEST("upespm: parameter-vector scatter with fixed-lag skip") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    m.nestpm = 2;
    m.mdl(0) = 1;
    m.mdl(1) = 1;
    m.mdl(2) = 2;
    m.mdl(3) = 3;
    m.opr(0) = 1;
    m.opr(1) = 3;
    m.opr(2) = 4;
    m.arimaf(1) = false;
    m.arimaf(2) = false;
    m.arimaf(3) = true;  // MA lag fixed
    d.arimap(1) = -99.0;
    d.arimap(2) = -99.0;
    d.arimap(3) = 0.99;
    double estprm[2] = {0.4, -0.2};
    upespm(ctx, estprm);
    CHECK(rclose(d.arimap(1), 0.4, 1e-15));   // up1
    CHECK(rclose(d.arimap(2), -0.2, 1e-15));  // up2
    CHECK_EQ(d.arimap(3), 0.99);              // up3 (fixed, untouched)
}

// ---- chkrts invertibility BOUNDARY (FMA canary; against ref_chkrts.f E-H).
// The 1-c*c test at |theta|~1 flips if the build contracts to an FMA. Both sides
// are -ffp-contract=off, so theta=1 gives cfncsq=0 (non-inv) and theta=1-1e-16
// stays >0 (invertible). ------------------------------------------------------
TEST("chkrts: invertibility boundary (1-c*c FMA canary)") {
    int opr1[2] = {1, 2};
    int oprf1[1] = {1};
    // E: theta=1.0 exactly -> non-invertible.
    {
        double ap[1] = {1.0};
        int al[1] = {1};
        bool af[1] = {false};
        int pf = -99;
        CHECK_EQ(chkrts(ap, al, af, opr1, oprf1, 1, 1, pf), true);  // chkE
        CHECK_EQ(pf, 1);
    }
    // F: theta=1-1e-16 -> invertible (cfncsq ~2.2e-16 > 0).
    {
        double ap[1] = {0.9999999999999999};
        int al[1] = {1};
        bool af[1] = {false};
        int pf = -99;
        CHECK_EQ(chkrts(ap, al, af, opr1, oprf1, 1, 1, pf), false);  // chkF
    }
    // G: degree-2 (0.5,0.5) -> reflection drives coef(1) to 1.0 -> non-invertible.
    {
        double ap[2] = {0.5, 0.5};
        int al[2] = {1, 2};
        bool af[2] = {false, false};
        int opr2[2] = {1, 3};
        int pf = -99;
        CHECK_EQ(chkrts(ap, al, af, opr2, oprf1, 1, 1, pf), true);  // chkG
        CHECK_EQ(pf, 1);
    }
    // H: seasonal Theta=1.0 lag12 factor12 -> degree=12/12=1 -> non-invertible.
    {
        double ap[1] = {1.0};
        int al[1] = {12};
        bool af[1] = {false};
        int oprf12[1] = {12};
        int pf = -99;
        CHECK_EQ(chkrts(ap, al, af, opr1, oprf12, 1, 1, pf), true);  // chkH
        CHECK_EQ(pf, 1);
    }
}

int main() { return mt::run_all(); }
