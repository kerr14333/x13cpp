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
#include "regarima/forecast.hpp"
#include "regarima/outlier.hpp"
#include "gen/model.hpp"
#include "transform/transform.hpp"
#include "specparse/specparse.hpp"
#include "numeric/minpack.hpp"
#include "numeric/rpoly.hpp"

#include <cmath>
#include <memory>

namespace x13 {
// ratneg lives in core/src/regarima (declared in regvar.hpp, which pulls in the
// full X13Context); forward-declare it here to keep this leaf test lightweight.
void ratneg(int nelta, const double* arimap, const int* arimal, const int* opr,
            int begopr, int endopr, double* c);
void ratpos(int nelta, const double* arimap, const int* arimal, const int* opr,
            int begopr, int endopr, int neltc, double* c);
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

// dppsl: single-RHS packed-Cholesky solve (against ref driver drv_dppsl.f, which
// factors A=[[4,2,1],[2,5,3],[1,3,6]] with dppfa then solves for b=[1,2,3]).
// Covers both the full a*x=b solve (alt=false) and the Census forward-only
// L*x=b solve (alt=true).
TEST("dppsl: packed-Cholesky single-RHS solve, full + alt paths") {
    // Packed upper (col-major): a11,a12,a22,a13,a23,a33.
    double ap[6] = {4.0, 2.0, 5.0, 1.0, 3.0, 6.0};
    int info = 0;
    dppfa(ap, 3, info);
    CHECK_EQ(info, 0);
    double apf[6];
    for (int i = 0; i < 6; ++i) apf[i] = ap[i];

    double b[3] = {1.0, 2.0, 3.0};
    dppsl(apf, 3, b, /*alt=*/false);          // full A x = b
    CHECK(rclose(b[0], 0.8955223880597013e-01, 1e-14));   // oracle full1
    CHECK(rclose(b[1], 0.1044776119402986e+00, 1e-14));   // full2
    CHECK(rclose(b[2], 0.4328358208955223e+00, 1e-14));   // full3

    double b2[3] = {1.0, 2.0, 3.0};
    dppsl(apf, 3, b2, /*alt=*/true);          // forward-only L x = b
    CHECK(rclose(b2[0], 0.5000000000000000e+00, 1e-14));  // oracle alt1
    CHECK(rclose(b2[1], 0.7500000000000000e+00, 1e-14));  // alt2
    CHECK(rclose(b2[2], 0.8857284715832128e+00, 1e-14));  // alt3
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

// Case E (A2): multi-column Nc=3 ARMA(1,1). Exercises Arimal*=Nc scale/restore,
// overlapping backward copy, multi-RHS dsolve/exctma, strided ddot.
TEST("armafl: multi-column Nc=3 ARMA(1,1)") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    m.lar = true; m.lma = true; m.nopr = 2;
    m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 2; m.mdl(3) = 3;
    m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3;
    m.arimal(1) = 1; m.arimal(2) = 1;
    d.arimap(1) = 0.5; d.arimap(2) = 0.3;
    m.arimaf(1) = false; m.arimaf(2) = false;
    m.oprfac(1) = 1; m.oprfac(2) = 1;
    m.mxarlg = 1; m.mxmalg = 1; m.mxdflg = 0;
    d.lndtcv = 0.0;
    double mata[200] = {0.0};
    for (int i = 1; i <= 30; ++i)
        mata[i - 1] = double(7 * i % 13) - 6.0 + 0.25 * double(3 * i % 8);
    int na = -99, info = -99;
    armafl(ctx, 10, 3, true, true, mata, na, 200, info);
    CHECK_EQ(info, 0);   // ef_info
    CHECK_EQ(na, 11);    // ef_na
    CHECK(rclose(d.lndtcv, 5.6954892925541983e-02, 1e-12));   // ef_ldt
    CHECK(rclose(mata[0],   1.1306818363801443e+00, 1e-12));  // ef_a1
    CHECK(rclose(mata[4],  -1.7332616287235918e+00, 1e-12));  // ef_a5
    CHECK(rclose(mata[32], -4.1250034239322408e+00, 1e-12));  // ef_alast (na*3)
}

// Case F (A5): seasonal AR (0 1 0)(1 0 0)12 -- pure-AR exact path (ELSE IF Lar
// Chlvwp=acv fill), sparse fular, euclid Mxmalg=0 branch, Lndtcv accumulation.
TEST("armafl: seasonal AR (0 1 0)(1 0 0)12") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    m.lar = true; m.lma = false; m.nopr = 2;
    m.mdl(0) = 1; m.mdl(1) = 2; m.mdl(2) = 3; m.mdl(3) = 3;
    m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3;
    m.arimal(1) = 1; m.arimal(2) = 12;
    d.arimap(1) = 1.0; d.arimap(2) = 0.4;
    m.arimaf(1) = true; m.arimaf(2) = false;
    m.oprfac(1) = 1; m.oprfac(2) = 12;
    m.mxarlg = 12; m.mxmalg = 0; m.mxdflg = 1;
    d.lndtcv = 0.0;
    double mata[200] = {0.0};
    for (int i = 1; i <= 40; ++i)
        mata[i - 1] = double(7 * i % 13) - 6.0 + 0.25 * double(3 * i % 8);
    int na = -99, info = -99;
    armafl(ctx, 40, 1, true, true, mata, na, 200, info);
    CHECK_EQ(info, 0);   // ff_info
    CHECK_EQ(na, 39);    // ff_na
    CHECK(rclose(d.lndtcv, 2.0922406457373310e+00, 1e-12));   // ff_ldt
    CHECK(rclose(mata[0],  -4.8117044797036321e+00, 1e-12));  // ff_a1
    CHECK(rclose(mata[12],  7.8499999999999996e+00, 1e-12));  // ff_a13
    CHECK(rclose(mata[38],  7.8499999999999996e+00, 1e-12));  // ff_alast
}

// Case G (A6): mixed (1 0 1)(1 0 1)12 -- largest D machinery (13x13 Sigma_p-D'D),
// mltpos secpas on real data, regular-then-seasonal operator-order FP.
TEST("armafl: mixed (1 0 1)(1 0 1)12") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    m.lar = true; m.lma = true; m.nopr = 4;
    m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 3; m.mdl(3) = 5;
    m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3; m.opr(3) = 4; m.opr(4) = 5;
    m.arimal(1) = 1; m.arimal(2) = 12; m.arimal(3) = 1; m.arimal(4) = 12;
    d.arimap(1) = 0.3; d.arimap(2) = 0.2; d.arimap(3) = 0.4; d.arimap(4) = 0.3;
    m.arimaf(1) = false; m.arimaf(2) = false;
    m.arimaf(3) = false; m.arimaf(4) = false;
    m.oprfac(1) = 1; m.oprfac(2) = 12; m.oprfac(3) = 1; m.oprfac(4) = 12;
    m.mxarlg = 13; m.mxmalg = 13; m.mxdflg = 0;
    d.lndtcv = 0.0;
    double mata[200] = {0.0};
    for (int i = 1; i <= 45; ++i)
        mata[i - 1] = double(7 * i % 13) - 6.0 + 0.25 * double(3 * i % 8);
    int na = -99, info = -99;
    armafl(ctx, 45, 1, true, true, mata, na, 200, info);
    CHECK_EQ(info, 0);   // gf_info
    CHECK_EQ(na, 58);    // gf_na
    CHECK(rclose(d.lndtcv, 1.4955153694748469e-01, 1e-12));   // gf_ldt
    CHECK(rclose(mata[0],   1.6818026153991728e+00, 1e-12));  // gf_a1
    CHECK(rclose(mata[19],  7.3953276312852423e-01, 1e-12));  // gf_a20
    CHECK(rclose(mata[57], -6.1351674219679686e-01, 1e-12));  // gf_alast
}

// Case J (A12): pure differencing (0 1 0)(0 1 0)12 -- Lar=Lma=F, both Linit
// branches false, empty-range loops, Nopr global side-effect write.
TEST("armafl: pure differencing (0 1 0)(0 1 0)12") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    m.lar = false; m.lma = false; m.nopr = 2;
    m.mdl(0) = 1; m.mdl(1) = 3; m.mdl(2) = 3; m.mdl(3) = 3;
    m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3;
    m.arimal(1) = 1; m.arimal(2) = 12;
    d.arimap(1) = 1.0; d.arimap(2) = 1.0;
    m.arimaf(1) = true; m.arimaf(2) = true;
    m.oprfac(1) = 1; m.oprfac(2) = 12;
    m.mxarlg = 0; m.mxmalg = 0; m.mxdflg = 13;
    d.lndtcv = -42.0;
    double mata[200] = {0.0};
    for (int i = 1; i <= 40; ++i)
        mata[i - 1] = double(7 * i % 13) - 6.0 + 0.25 * double(3 * i % 8);
    int na = -99, info = -99;
    armafl(ctx, 40, 1, true, true, mata, na, 200, info);
    CHECK_EQ(info, 0);   // jf_info
    CHECK_EQ(na, 27);    // jf_na
    CHECK_EQ(m.nopr, 2); // jf_nopr (exctma side-effect write)
    CHECK(rclose(mata[0],  1.1000000000000000e+01, 1e-12));  // jf_a1
    CHECK(rclose(mata[26], 1.1000000000000000e+01, 1e-12));  // jf_alast
}

// Case (A7): Linit=F reuse. Init ARMA(1,1) on Nr=8, then re-filter a NEW Nr=12
// series with Linit=F/Lckrts=F -- nextma recomputed, Chlgpg/Chlvwp/Matd reused,
// ddot reads the zero Matd tail, Lndtcv NOT re-accumulated. A fresh ctx gives the
// zero Matd the Fortran driver zeroes explicitly. (forecasting/outlier pattern.)
TEST("armafl: Linit=F reuse across two calls") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    m.lar = true; m.lma = true; m.nopr = 2;
    m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 2; m.mdl(3) = 3;
    m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3;
    m.arimal(1) = 1; m.arimal(2) = 1;
    d.arimap(1) = 0.5; d.arimap(2) = 0.3;
    m.arimaf(1) = false; m.arimaf(2) = false;
    m.oprfac(1) = 1; m.oprfac(2) = 1;
    m.mxarlg = 1; m.mxmalg = 1; m.mxdflg = 0;
    d.lndtcv = 0.0;

    double mata[200] = {0.0};
    for (int i = 1; i <= 8; ++i)
        mata[i - 1] = double(7 * i % 13) - 6.0 + 0.25 * double(3 * i % 8);
    int na = -99, info = -99;
    armafl(ctx, 8, 1, true, true, mata, na, 200, info);  // call 1: Linit=T
    CHECK_EQ(na, 9);                                          // r1_na
    CHECK(rclose(d.lndtcv, 5.6954892689151181e-02, 1e-12));   // r1_ldt

    for (int i = 1; i <= 12; ++i)  // NEW series
        mata[i - 1] = double(5 * i % 11) - 5.0 + 0.5 * double(2 * i % 7);
    na = -99; info = -99;
    armafl(ctx, 12, 1, false, false, mata, na, 200, info);  // call 2: Linit=F
    CHECK_EQ(info, 0);   // r2_info
    CHECK_EQ(na, 13);    // r2_na
    CHECK(rclose(d.lndtcv, 5.6954892689151181e-02, 1e-12));   // r2_ldt (preserved)
    CHECK(rclose(mata[0],  2.9525617825546679e+00, 1e-12));   // r2_a1
    CHECK(rclose(mata[6],  5.2475382813583993e+00, 1e-12));   // r2_a7
    CHECK(rclose(mata[12], 2.1818029554071101e+00, 1e-12));   // r2_alast
}

// Case (A8): q>p ARMA(1,2) and p=q ARMA(2,2) -- opposite euclid branch mix from
// the covered AR(2)MA(1); D-loop max(1,Mxarlg-row+1) clamp.
TEST("armafl: ARMA(1,2) and ARMA(2,2)") {
    // A8a: ARMA(1,2) phi=0.5, theta=(0.3,-0.2).
    {
        auto ctxp = std::make_unique<X13Context>();
        X13Context& ctx = *ctxp;
        auto& m = ctx.model;
        auto& d = ctx.mdldat;
        m.lar = true; m.lma = true; m.nopr = 2;
        m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 2; m.mdl(3) = 3;
        m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 4;
        m.arimal(1) = 1; m.arimal(2) = 1; m.arimal(3) = 2;
        d.arimap(1) = 0.5; d.arimap(2) = 0.3; d.arimap(3) = -0.2;
        m.arimaf(1) = false; m.arimaf(2) = false; m.arimaf(3) = false;
        m.oprfac(1) = 1; m.oprfac(2) = 1; m.oprfac(3) = 1;
        m.mxarlg = 1; m.mxmalg = 2; m.mxdflg = 0;
        d.lndtcv = 0.0;
        double mata[200] = {0.0};
        for (int i = 1; i <= 10; ++i)
            mata[i - 1] = double(7 * i % 13) - 6.0 + 0.25 * double(3 * i % 8);
        int na = -99, info = -99;
        armafl(ctx, 10, 1, true, true, mata, na, 200, info);
        CHECK_EQ(info, 0); CHECK_EQ(na, 12);                     // kf
        CHECK(rclose(d.lndtcv, 2.2314353556278715e-01, 1e-12));  // kf_ldt
        CHECK(rclose(mata[0],  5.9555400043398588e-01, 1e-12));  // kf_a1
        CHECK(rclose(mata[11], -3.3654835780478798e-01, 1e-12)); // kf_alast
    }
    // A8b: ARMA(2,2) phi=(0.4,-0.2), theta=(0.3,-0.1).
    {
        auto ctxp = std::make_unique<X13Context>();
        X13Context& ctx = *ctxp;
        auto& m = ctx.model;
        auto& d = ctx.mdldat;
        m.lar = true; m.lma = true; m.nopr = 2;
        m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 2; m.mdl(3) = 3;
        m.opr(0) = 1; m.opr(1) = 3; m.opr(2) = 5;
        m.arimal(1) = 1; m.arimal(2) = 2; m.arimal(3) = 1; m.arimal(4) = 2;
        d.arimap(1) = 0.4; d.arimap(2) = -0.2; d.arimap(3) = 0.3;
        d.arimap(4) = -0.1;
        m.arimaf(1) = false; m.arimaf(2) = false;
        m.arimaf(3) = false; m.arimaf(4) = false;
        m.oprfac(1) = 1; m.oprfac(2) = 1; m.oprfac(3) = 1; m.oprfac(4) = 1;
        m.mxarlg = 2; m.mxmalg = 2; m.mxdflg = 0;
        d.lndtcv = 0.0;
        double mata[200] = {0.0};
        for (int i = 1; i <= 10; ++i)
            mata[i - 1] = double(7 * i % 13) - 6.0 + 0.25 * double(3 * i % 8);
        int na = -99, info = -99;
        armafl(ctx, 10, 1, true, true, mata, na, 200, info);
        CHECK_EQ(info, 0); CHECK_EQ(na, 12);                     // lf
        CHECK(rclose(d.lndtcv, 2.2887660549580469e-02, 1e-12));  // lf_ldt
        CHECK(rclose(mata[0],  1.4796812403305011e+00, 1e-12));  // lf_a1
        CHECK(rclose(mata[11], -6.6965817759949309e-02, 1e-12)); // lf_alast
    }
}

// Case (A13): partially-fixed ARMA(1,1) (MA lag fixed) -- filter output is BYTE-
// IDENTICAL to the free case A: Arimaf only gates chkrts, never the numerics.
TEST("armafl: partially-fixed = same numerics as free") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    m.lar = true; m.lma = true; m.nopr = 2;
    m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 2; m.mdl(3) = 3;
    m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3;
    m.arimal(1) = 1; m.arimal(2) = 1;
    d.arimap(1) = 0.5; d.arimap(2) = 0.3;
    m.arimaf(1) = false; m.arimaf(2) = true;  // MA fixed
    m.oprfac(1) = 1; m.oprfac(2) = 1;
    m.mxarlg = 1; m.mxmalg = 1; m.mxdflg = 0;
    d.lndtcv = 0.0;
    double mata[200] = {1.0, 2.0, -1.0, 0.5, 3.0, -2.0, 1.5, 0.25};
    int na = -99, info = -99;
    armafl(ctx, 8, 1, true, true, mata, na, 200, info);
    CHECK_EQ(info, 0); CHECK_EQ(na, 9);
    CHECK(rclose(mata[0], 1.3372276673516541e+00, 1e-12));  // == case A af_a1
    CHECK(rclose(mata[8], 1.3515159993273351e-02, 1e-12));  // == case A af_a9
}

// Case (A4): error-code paths. Non-stationary AR with Lckrts=F reaches euclid ->
// PACFER=12, and mata is left UNTOUCHED (error fires in the init block, before
// filtering). The two fixed-boundary cases return info=0 (no spurious error --
// the allfix chkrts-skip path runs clean; PVWPER/PGPGER need other triggers).
TEST("armafl: error-code paths (PACFER + no-spurious-error)") {
    // A4a: phi=1.05 non-stationary, Lckrts=F -> PACFER=12, mata untouched.
    {
        auto ctxp = std::make_unique<X13Context>();
        X13Context& ctx = *ctxp;
        auto& m = ctx.model;
        auto& d = ctx.mdldat;
        m.lar = true; m.lma = true; m.nopr = 2;
        m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 2; m.mdl(3) = 3;
        m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3;
        m.arimal(1) = 1; m.arimal(2) = 1;
        d.arimap(1) = 1.05; d.arimap(2) = 0.3;
        m.arimaf(1) = false; m.arimaf(2) = false;
        m.oprfac(1) = 1; m.oprfac(2) = 1;
        m.mxarlg = 1; m.mxmalg = 1; m.mxdflg = 0;
        d.lndtcv = 0.0;
        double mata[200] = {1.0, 2.0, -1.0};
        int na = -99, info = -99;
        armafl(ctx, 8, 1, true, false, mata, na, 200, info);
        CHECK_EQ(info, 12);         // nf_info = PACFER
        CHECK_EQ(mata[0], 1.0);     // nf_a1 untouched
    }
    // A4b/A4c: fixed boundary coeffs -> info=0 (allfix chkrts skip, no error).
    auto run_fixed = [](double ap1, double ap2, bool f1, bool f2) {
        auto ctxp = std::make_unique<X13Context>();
        X13Context& ctx = *ctxp;
        auto& m = ctx.model;
        auto& d = ctx.mdldat;
        m.lar = true; m.lma = true; m.nopr = 2;
        m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 2; m.mdl(3) = 3;
        m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3;
        m.arimal(1) = 1; m.arimal(2) = 1;
        d.arimap(1) = ap1; d.arimap(2) = ap2;
        m.arimaf(1) = f1; m.arimaf(2) = f2;
        m.oprfac(1) = 1; m.oprfac(2) = 1;
        m.mxarlg = 1; m.mxmalg = 1; m.mxdflg = 0;
        d.lndtcv = 0.0;
        double mata[200] = {1.0, 2.0};
        int na = -99, info = -99;
        armafl(ctx, 8, 1, true, true, mata, na, 200, info);
        return info;
    };
    CHECK_EQ(run_fixed(1.0, 0.3, true, false), 0);  // of_info (fixed AR=1.0)
    CHECK_EQ(run_fixed(0.3, 1.2, false, true), 0);  // pf_info (fixed MA=1.2)
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

// ---- fcnar (lmdif objective; against ref_fcnar.f). ARMA(1,1) via upespm, then
// armafl-filter tsrs. Success under exact ML scales residuals by
// exp(lndtcv/2/dnefob); a filter failure floods with lrgrsd + clears err. -------
TEST("fcnar: objective function, success + error paths") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    m.lar = true; m.lma = true; m.lextma = true; m.lprier = false;
    m.nopr = 2; m.nestpm = 2;
    d.nspobs = 8;
    ctx.series.dnefob = 8.0;
    ctx.series.lrgrsd = 1e6;
    m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 2; m.mdl(3) = 3;
    m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3;
    m.arimal(1) = 1; m.arimal(2) = 1;
    m.arimaf(1) = false; m.arimaf(2) = false;
    m.oprfac(1) = 1; m.oprfac(2) = 1;
    m.mxarlg = 1; m.mxmalg = 1; m.mxdflg = 0;
    const double series[8] = {1.0, 2.0, -1.0, 0.5, 3.0, -2.0, 1.5, 0.25};
    for (int i = 0; i < 8; ++i) ctx.series.tsrs(i + 1) = series[i];

    double a[1092] = {0.0};
    // success: exact-ML scaling.
    double est1[2] = {0.5, 0.3};
    d.lndtcv = 0.0;
    int na = 9, err = -99;
    fcnar(ctx, na, 2, est1, a, false, true, err, true);
    CHECK_EQ(na, 9);        // fc_na
    CHECK_EQ(err, -99);     // fc_err (untouched on success)
    CHECK(rclose(d.lndtcv, 5.6954892689151181e-02, 1e-12));  // fc_ldt
    CHECK(rclose(a[0],  1.3419962532816043e+00, 1e-12));     // fc_a1
    CHECK(rclose(a[1], -3.1359720543906200e-01, 1e-12));     // fc_a2
    CHECK(rclose(a[8],  1.3563355377917604e-02, 1e-12));     // fc_a9

    // error: phi=1.05 non-stationary, lckinv=F -> flood lrgrsd, err->0.
    double est2[2] = {1.05, 0.3};
    d.lndtcv = 0.0;
    na = 9; err = -99;
    fcnar(ctx, na, 2, est2, a, false, true, err, false);
    CHECK_EQ(na, 9);        // ec_na
    CHECK_EQ(err, 0);       // ec_err
    CHECK_EQ(a[0], 1e6);    // ec_a1
    CHECK_EQ(a[8], 1e6);    // ec_a9
}

// ---- strtvl (ARMA starting values; against ref_strtvl.f). AR(2)+MA(1): lag 1
// free & DNOTST -> 0.1; lag 2 already 0.5 -> kept; lag 3 MA fixed -> kept. ------
TEST("strtvl: seed free not-set ARMA lags to 0.1") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 2; m.mdl(3) = 3;  // DIFF empty, AR, MA
    m.opr(0) = 1; m.opr(1) = 3; m.opr(2) = 4;                // AR lags 1..2, MA lag 3
    d.arimap(1) = -999.0; m.arimaf(1) = false;  // DNOTST, free, not set -> seed
    d.arimap(2) = 0.5;    m.arimaf(2) = false;  // free, already valued -> keep
    d.arimap(3) = -999.0; m.arimaf(3) = true;   // DNOTST but fixed -> keep
    strtvl(ctx);
    CHECK(rclose(d.arimap(1), 1.0000000000000001e-01, 1e-12));  // sv1
    CHECK_EQ(d.arimap(2), 5.0000000000000000e-01);              // sv2
    CHECK_EQ(d.arimap(3), -9.9900000000000000e+02);             // sv3 (DNOTST kept)
}

// ---- stpitr (IGLS convergence test + SAVEd oldobj; against ref_stpitr.f). A
// six-call sequence on ONE ctx so the oldobj carry is exercised: call 3 only
// converges because oldobj=5 was carried from call 2 (without the SAVE, ratio
// would be -1 and it would keep iterating). Calls 4-6 hit the error stops. -----
TEST("stpitr: convergence test with carried oldobj (SAVE)") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    bool convrg;
    int armaer;
    // call 1: Iter=1 -> just record oldobj, keep going.
    armaer = 0; convrg = false;
    CHECK_EQ(stpitr(ctx, false, 10.0, 1e-5, 1, 1, 100, convrg, armaer, false), true);
    CHECK_EQ(convrg, true);  CHECK_EQ(armaer, 0);              // st1
    // call 2: Iter=2, 10->5, ratio uses carried oldobj=10.
    armaer = 0; convrg = false;
    CHECK_EQ(stpitr(ctx, false, 5.0, 1e-5, 2, 2, 100, convrg, armaer, false), true);
    CHECK_EQ(convrg, true);  CHECK_EQ(armaer, 0);              // st2
    // call 3: Iter=3, 5->5.00002, |ratio|<devtol -> converged (needs oldobj=5).
    armaer = 0; convrg = false;
    CHECK_EQ(stpitr(ctx, false, 5.00002, 1e-5, 3, 3, 100, convrg, armaer, false), false);
    CHECK_EQ(convrg, true);  CHECK_EQ(armaer, 0);              // st3
    // call 4: Nliter>=Mxiter -> PMXIER, hard stop.
    armaer = 0; convrg = false;
    CHECK_EQ(stpitr(ctx, false, 5.0, 1e-5, 4, 100, 100, convrg, armaer, false), false);
    CHECK_EQ(convrg, false); CHECK_EQ(armaer, 5);              // st4 (PMXIER)
    // call 5: devtol/2 < mprec -> PCNTER, hard stop.
    armaer = 0; convrg = false;
    CHECK_EQ(stpitr(ctx, false, 5.0, 1e-18, 5, 5, 100, convrg, armaer, false), false);
    CHECK_EQ(convrg, false); CHECK_EQ(armaer, 14);             // st5 (PCNTER)
    // call 6: objfcn<mprec but nonzero -> PDVTER (convrg stays true).
    armaer = 0; convrg = false;
    CHECK_EQ(stpitr(ctx, false, 1e-18, 1e-5, 6, 6, 100, convrg, armaer, false), false);
    CHECK_EQ(convrg, true);  CHECK_EQ(armaer, 15);             // st6 (PDVTER)
}

// ---- leaf-level "don't clean up the math" edge cases (against ref_leafedge.f).
// A9: ratneg leaves c(i) STALE when its sum lands exactly 0 (a naive port writes
// 0). A10: ratpos skips a term whose coefficient underflows below 1e-150 (a naive
// port propagates 1e-160). A11: chkrts with sparse / decreasing lag lists. ------
TEST("ratneg: exact-zero no-write staleness (A9)") {
    double ap[1] = {0.5};
    int al[1] = {1};
    int op[2] = {1, 2};
    double c[4] = {-1.0, 2.0, 0.0, 0.0};
    ratneg(4, ap, al, op, 1, 1, c);
    CHECK_EQ(c[0], -1.0);  // rn1: sum=-1+0.5*2=0 exactly -> c(1) KEPT, not zeroed
    CHECK_EQ(c[1], 2.0);   // rn2
    CHECK_EQ(c[2], 0.0);   // rn3
    CHECK_EQ(c[3], 0.0);   // rn4
}

TEST("ratpos: coefficient underflow-skip (A10)") {
    double ap[1] = {1e-160};  // below the 1e-150 skip threshold
    int al[1] = {1};
    int op[2] = {1, 2};
    double c[4] = {1.0, 0.0, 0.0, 0.0};
    ratpos(1, ap, al, op, 1, 1, 4, c);
    CHECK_EQ(c[0], 1.0);
    CHECK_EQ(c[1], 0.0);  // rp2: term skipped -> 0, NOT 1e-160
    CHECK_EQ(c[2], 0.0);
    CHECK_EQ(c[3], 0.0);
}

TEST("chkrts: sparse and decreasing lag lists (A11)") {
    int op[2] = {1, 3};
    int oprf[1] = {1};
    // A11a: sparse MA lags [2 4] -> degree 4 with coef(1)=coef(3)=0.
    {
        double ap[2] = {0.3, 0.2};
        int al[2] = {2, 4};
        bool af[2] = {false, false};
        int pf = -99;
        CHECK_EQ(chkrts(ap, al, af, op, oprf, 1, 1, pf), false);  // saa
        CHECK_EQ(pf, -99);
    }
    // A11b: decreasing lags [4 2] -> degree fixed from the highest (first) lag.
    {
        double ap[2] = {0.3, 0.2};
        int al[2] = {4, 2};
        bool af[2] = {false, false};
        int pf = -99;
        CHECK_EQ(chkrts(ap, al, af, op, oprf, 1, 1, pf), false);  // sab
        CHECK_EQ(pf, -99);
    }
}

// ---- qrfac + qrsolv (Census MINPACK; against ref_minpack.f). ----------------
TEST("qrfac: Householder QR with column pivoting") {
    // 4x3 matrix, column-major (lda=4).
    double a[12] = {1, 2, 3, 4, 1, 0, 1, 0, 2, 1, 0, 1};
    int ipvt[3] = {0, 0, 0};
    double rdiag[3] = {0}, acnorm[3] = {0}, wa[3] = {0};
    qrfac(4, 3, a, 4, true, ipvt, 3, rdiag, acnorm, wa);
    CHECK_EQ(ipvt[0], 1); CHECK_EQ(ipvt[1], 3); CHECK_EQ(ipvt[2], 2);  // qf_ipvt
    CHECK(rclose(rdiag[0], -5.4772255750516612e+00, 1e-12));  // qf_rd1
    CHECK(rclose(rdiag[1],  1.9663841605003496e+00, 1e-12));  // qf_rd2
    CHECK(rclose(rdiag[2], -1.1141720290623114e+00, 1e-12));  // qf_rd3
    CHECK(rclose(acnorm[0], 5.4772255750516612e+00, 1e-12));  // qf_ac1
    CHECK(rclose(acnorm[1], 1.4142135623730951e+00, 1e-12));  // qf_ac2
    CHECK(rclose(acnorm[2], 2.4494897427831779e+00, 1e-12));  // qf_ac3
    CHECK(rclose(a[0], 1.1825741858350554e+00, 1e-12));  // qf_a1
    CHECK(rclose(a[1], 3.6514837167011072e-01, 1e-12));  // qf_a2
    CHECK(rclose(a[5], 1.0348568246221159e+00, 1e-12));  // qf_a6
}

TEST("qrsolv: Givens elimination + back-substitution") {
    // 3x3 upper-tri R, column-major (ldr=3).
    double r[9] = {2, 0, 0, 1, 3, 0, 1, 1, 4};
    int ipvt[3] = {1, 2, 3};
    double diag[3] = {0.5, 0.5, 0.5};
    double qtb[3] = {1, 2, 3};
    double x[3] = {0}, sdiag[3] = {0}, wa[3] = {0};
    qrsolv(3, r, 3, ipvt, diag, qtb, x, sdiag, wa);
    CHECK(rclose(x[0], -6.9571958043652002e-02, 1e-12));  // qs_x1
    CHECK(rclose(x[1],  4.0764516363460185e-01, 1e-12));  // qs_x2
    CHECK(rclose(x[2],  7.4019524720815877e-01, 1e-12));  // qs_x3
    CHECK(rclose(sdiag[0], 2.0615528128088303e+00, 1e-12));  // qs_sd1
    CHECK(rclose(sdiag[1], 3.0510364680566773e+00, 1e-12));  // qs_sd2
    CHECK(rclose(sdiag[2], 4.0377855911690963e+00, 1e-12));  // qs_sd3
}

TEST("lmpar: LM parameter secant search") {
    double r[9] = {2, 0, 0, 1, 3, 0, 1, 1, 4};
    int ipvt[3] = {1, 2, 3};
    double diag[3] = {1, 1, 1};
    double qtb[3] = {1, 2, 3};
    double x[3] = {0}, sdiag[3] = {0}, wa1[3] = {0}, wa2[3] = {0};
    double par = 0.0;
    lmpar(3, r, 3, ipvt, diag, qtb, 0.3, par, x, sdiag, wa1, wa2);
    CHECK(rclose(par, 3.5560792823453269e+01, 1e-12));  // lp_par
    CHECK(rclose(x[0], 3.0435623515339374e-02, 1e-12)); // lp_x1
    CHECK(rclose(x[1], 1.2866076853658420e-01, 1e-12)); // lp_x2
    CHECK(rclose(x[2], 2.6931053329193400e-01, 1e-12)); // lp_x3
}

TEST("fdjac2: forward-difference Jacobian") {
    // Same analytic test fcn as ref_fdjac2.f: w1=x1+x2, w2=x1*x2, w3=x1^2+x2.
    MinpackFcn fcn = [](int&, int, const double* x, double* w, bool, bool, int&,
                        bool) {
        w[0] = x[0] + x[1];
        w[1] = x[0] * x[1];
        w[2] = x[0] * x[0] + x[1];
    };
    double x[2] = {1.5, -0.5};
    double fvec[3] = {0}, fjac[6] = {0}, wa[3] = {0};
    int iflag = 1;
    fcn(iflag, 2, x, fvec, false, false, iflag, false);  // fvec = fcn(x0)
    fdjac2(fcn, 3, 2, x, fvec, fjac, 3, iflag, 0.0, wa, false, false, false);
    // fjac column-major (ldfjac=3): [f11 f21 f31 f12 f22 f32].
    CHECK(rclose(fjac[0],  1.0000000011560772e+00, 1e-12));  // fj11
    CHECK(rclose(fjac[1], -5.0000000057803862e-01, 1e-12));  // fj21
    CHECK(rclose(fjac[2],  3.0000000332705543e+00, 1e-12));  // fj31
    CHECK(rclose(fjac[3],  1.0000000110901848e+00, 1e-12));  // fj12
    CHECK(rclose(fjac[4],  1.5000000017341157e+00, 1e-12));  // fj22
    CHECK(rclose(fjac[5],  1.0000000110901848e+00, 1e-12));  // fj32
}

TEST("covar: covariance matrix from QR output (permuted)") {
    // 3x3 nonsingular upper-tri R, column-major (ldr=3), NON-identity
    // ipvt=(2,3,1) to exercise the permutation. Goldens from tools/ref_covar.f.
    const double golden[9] = {
         6.2500000000000000e-02, -2.0833333333333336e-02, -2.0833333333333332e-02,
        -2.0833333333333336e-02,  2.8472222222222221e-01, -4.8611111111111105e-02,
        -2.0833333333333332e-02, -4.8611111111111105e-02,  1.1805555555555555e-01};
    // Case 1: Tol=0 (auto tolerance path, dpmpar(1)).
    {
        double r[9] = {2, 0, 0, 1, 3, 0, 1, 1, 4};
        int ipvt[3] = {2, 3, 1};
        double wa[3] = {0};
        int info = -99;
        covar(3, r, 3, ipvt, 0.0, info, wa);
        CHECK_EQ(info, 0);  // cv_info
        for (int i = 0; i < 9; ++i) CHECK(rclose(r[i], golden[i], 1e-12));
    }
    // Case 2: explicit Tol=1e-10 (ELSE tolerance path); still nonsingular.
    {
        double r[9] = {2, 0, 0, 1, 3, 0, 1, 1, 4};
        int ipvt[3] = {2, 3, 1};
        double wa[3] = {0};
        int info = -99;
        covar(3, r, 3, ipvt, 1.0e-10, info, wa);
        CHECK_EQ(info, 0);  // cw_info
        for (int i = 0; i < 9; ++i) CHECK(rclose(r[i], golden[i], 1e-12));
    }
}

// ---- lmdif (Census-modified MINPACK LM core; against ref_lmdif.f). ----------
// Analytic objectives matching the vendored fcn signature. Near-zero gradient
// components (qtf at convergence) get an absolute tolerance -- a relative rtol
// is meaningless on a quantity built from catastrophic cancellation.
TEST("lmdif: Rosenbrock, full trajectory to convergence") {
    MinpackFcn rosen = [](int&, int, const double* x, double* f, bool, bool,
                          int&, bool) {
        f[0] = 1.0e1 * (x[1] - x[0] * x[0]);
        f[1] = 1.0 - x[0];
    };
    double x[2] = {-1.2, 1.0};
    double fvec[2] = {0}, diag[2] = {0}, fjac[4] = {0}, qtf[2] = {0};
    double wa1[2] = {0}, wa2[2] = {0}, wa3[2] = {0}, wa4[2] = {0};
    int ipvt[2] = {0};
    int info = -99, nliter = 0, nfev = 0;
    lmdif(rosen, 2, 2, x, fvec, false, false, 1e-10, 1e-10, 0.0, 100, 0.0, diag,
          1, 100.0, 0, info, nliter, nfev, fjac, 2, ipvt, qtf, wa1, wa2, wa3, wa4);
    CHECK_EQ(info, 2);       // a_info (xtol convergence)
    CHECK_EQ(nliter, 16);    // a_nliter (trajectory canary)
    CHECK_EQ(nfev, 54);      // a_nfev
    CHECK(rclose(x[0], 1.0, 1e-12));  // a_x1
    CHECK(rclose(x[1], 1.0, 1e-12));  // a_x2
    CHECK(rclose(enorm(2, fvec), 0.0, 1e-12));  // a_fnorm (exact 0)
    CHECK(rclose(fjac[0], 2.0024984467011738e+01, 1e-12));  // a_fj11
    CHECK_EQ(ipvt[0], 1); CHECK_EQ(ipvt[1], 2);             // a_ipvt
    CHECK(rclose(diag[0], 2.4020824177816124e+01, 1e-12));  // a_diag1
    CHECK(rclose(diag[1], 1.0000000045103494e+01, 1e-12));  // a_diag2
    CHECK(std::fabs(qtf[0] - (-3.6941376259390850e-10)) <= 1e-12);  // a_qtf1
    CHECK(std::fabs(qtf[1] - 1.8470687992078067e-11) <= 1e-12);     // a_qtf2
}

TEST("lmdif: overdetermined exponential fit, M>N, Mode=2") {
    MinpackFcn expfit = [](int&, int, const double* x, double* f, bool, bool,
                           int&, bool) {
        const double t[5] = {0.5, 1.0, 1.5, 2.0, 2.5};
        const double y[5] = {1.8, 1.2, 0.9, 0.5, 0.3};
        for (int i = 0; i < 5; ++i) f[i] = x[0] * std::exp(x[1] * t[i]) - y[i];
    };
    double x[2] = {1.0, 0.0};
    double fvec[5] = {0}, diag[2] = {2.0, 0.5}, fjac[10] = {0}, qtf[2] = {0};
    double wa1[2] = {0}, wa2[2] = {0}, wa3[2] = {0}, wa4[5] = {0};
    int ipvt[2] = {0};
    int info = -99, nliter = 0, nfev = 0;
    lmdif(expfit, 5, 2, x, fvec, false, false, 1e-10, 1e-10, 1e-10, 100, 0.0,
          diag, 2, 100.0, 0, info, nliter, nfev, fjac, 5, ipvt, qtf, wa1, wa2,
          wa3, wa4);
    CHECK_EQ(info, 1);     // b_info (ftol convergence, nonzero residual)
    CHECK_EQ(nliter, 7);   // b_nliter
    CHECK_EQ(nfev, 22);    // b_nfev
    CHECK(rclose(x[0], 2.7220291273816937e+00, 1e-12));   // b_x1
    CHECK(rclose(x[1], -8.1179791426242320e-01, 1e-12));  // b_x2
    CHECK(rclose(enorm(5, fvec), 1.1782056122644691e-01, 1e-12));  // b_fnorm
    CHECK(rclose(fjac[0], -2.3865052096440240e+00, 1e-12));  // b_fj11
    CHECK(rclose(fjac[6], 4.4536936170477159e-01, 1e-12));   // b_fj22 (fjac(2,2))
    CHECK_EQ(ipvt[0], 2); CHECK_EQ(ipvt[1], 1);              // b_ipvt
    CHECK(std::fabs(qtf[0] - 3.7826624683240384e-08) <= 1e-12);  // b_qtf1
    CHECK(std::fabs(qtf[1] - 6.4333247720791548e-08) <= 1e-12);  // b_qtf2
}

TEST("lmdif: cumulative counters + Info=5 (re-entrant Rosenbrock)") {
    MinpackFcn rosen = [](int&, int, const double* x, double* f, bool, bool,
                          int&, bool) {
        f[0] = 1.0e1 * (x[1] - x[0] * x[0]);
        f[1] = 1.0 - x[0];
    };
    double x[2] = {-1.2, 1.0};
    double fvec[2] = {0}, diag[2] = {0}, fjac[4] = {0}, qtf[2] = {0};
    double wa1[2] = {0}, wa2[2] = {0}, wa3[2] = {0}, wa4[2] = {0};
    int ipvt[2] = {0};
    int info = -99, nliter = 7, nfev = 13;  // fake prior IGLS accumulation
    lmdif(rosen, 2, 2, x, fvec, false, false, 1e-10, 1e-10, 0.0, 10, 0.0, diag,
          1, 100.0, 0, info, nliter, nfev, fjac, 2, ipvt, qtf, wa1, wa2, wa3, wa4);
    CHECK_EQ(info, 5);      // c_info (mxiter cap via cumulative nliter>=10)
    CHECK_EQ(nliter, 10);   // c_nliter (7 + 3 accepted)
    CHECK_EQ(nfev, 25);     // c_nfev (13 + evals)
    CHECK(rclose(x[0], -5.7518863935252185e-01, 1e-12));  // c_x1
    CHECK(rclose(x[1], 2.9301052380426973e-01, 1e-12));   // c_x2
    CHECK(rclose(enorm(2, fvec), 1.6199818171907148e+00, 1e-12));  // c_fnorm
}

// ---- rpoly (Jenkins-Traub root finder; against ref_rpoly.f). ----------------
// The vendored oracle has a Census bug: hardcoded single-precision-ish machine
// constants (Eta=5e-15, smalno=1e-38) make lo=smalno/Eta~2e-24, and when any
// coefficient magnitude is >= 10 the scaling branch (guarded by xmax<10) runs
// and a dpeq(sc,0) absolute-threshold quirk rescales the whole polynomial by
// ~1e-37, collapsing all convergence tests -> fail=true, degree->0. The trigger
// is COEFFICIENT MAGNITUDE, not root location; X-13's own AR/MA polynomials
// have constant term 1 and small coefficients, so the branch is never hit and
// rpoly is reliable in practice. The port reproduces the bug bit-for-bit.
// Coefficients below are in order of DECREASING powers.
TEST("rpoly: deterministic failure when max|coeff| >= 10 (Census bug)") {
    double op[4] = {1.0, -6.0, 11.0, -6.0};  // (x-1)(x-2)(x-3), max|c|=11
    double zr[3] = {0}, zi[3] = {0};
    int deg = 3;
    bool fail = false;
    rpoly(op, deg, zr, zi, fail);
    CHECK(fail);        // r1_fail
    CHECK_EQ(deg, 0);   // r1_deg
    double op4[5] = {1.0, -10.0, 35.0, -50.0, 24.0};  // (x-1)..(x-4), max|c|=50
    double zr4[4] = {0}, zi4[4] = {0};
    int deg4 = 4;
    bool fail4 = false;
    rpoly(op4, deg4, zr4, zi4, fail4);
    CHECK(fail4);       // r3_fail
    CHECK_EQ(deg4, 0);  // r3_deg
    // Roots INSIDE the unit circle (0.5,0.3,-0.2) still fail when max|c|=20:
    // the trigger is coefficient magnitude, not root location.
    double op6[4] = {20.0, -12.0, -0.2, 0.6};
    double zr6[3] = {0}, zi6[3] = {0};
    int deg6 = 3;
    bool fail6 = false;
    rpoly(op6, deg6, zr6, zi6, fail6);
    CHECK(fail6);       // r6_fail
    CHECK_EQ(deg6, 0);  // r6_deg
}

TEST("rpoly: cubic with a complex pair") {
    double op[4] = {1.0, -2.0, 1.0, -2.0};  // (x^2+1)(x-2)
    double zr[3] = {0}, zi[3] = {0};
    int deg = 3;
    bool fail = true;
    rpoly(op, deg, zr, zi, fail);
    CHECK(!fail);
    CHECK_EQ(deg, 3);
    CHECK(std::fabs(zr[0] - (-6.4854874656406277e-18)) <= 1e-14);  // r2_zr1
    CHECK(rclose(zi[0], 1.0, 1e-12));                              // r2_zi1
    CHECK(std::fabs(zr[1] - (-6.4854874656406277e-18)) <= 1e-14);  // r2_zr2
    CHECK(rclose(zi[1], -1.0, 1e-12));                             // r2_zi2
    CHECK(rclose(zr[2], 2.0, 1e-12));                              // r2_zr3
    CHECK(rclose(zi[2], 0.0, 1e-12));                              // r2_zi3
}

TEST("rpoly: distinct real roots, small coefficients") {
    double op[4] = {1.0, -0.6, -0.01, 0.03};  // (x-0.5)(x-0.3)(x+0.2)
    double zr[3] = {0}, zi[3] = {0};
    int deg = 3;
    bool fail = true;
    rpoly(op, deg, zr, zi, fail);
    CHECK(!fail);
    CHECK_EQ(deg, 3);
    CHECK(rclose(zr[0], -1.9999999999999940e-01, 1e-12));  // r4_zr1
    CHECK(rclose(zr[1], 2.9999999999999777e-01, 1e-12));   // r4_zr2
    CHECK(rclose(zr[2], 5.0000000000000167e-01, 1e-12));   // r4_zr3
    CHECK(rclose(zi[0], 0.0, 1e-12));
    CHECK(rclose(zi[1], 0.0, 1e-12));
    CHECK(rclose(zi[2], 0.0, 1e-12));
}

TEST("rpoly: real root + complex pair, small coefficients") {
    double op[4] = {1.0, -0.4, 0.25, -0.1};  // (x-0.4)(x^2+0.25)
    double zr[3] = {0}, zi[3] = {0};
    int deg = 3;
    bool fail = true;
    rpoly(op, deg, zr, zi, fail);
    CHECK(!fail);
    CHECK_EQ(deg, 3);
    CHECK(std::fabs(zr[0] - 1.3466778305049108e-17) <= 1e-14);  // r5_zr1
    CHECK(rclose(zi[0], 4.9999999999999994e-01, 1e-12));        // r5_zi1
    CHECK(std::fabs(zr[1] - 1.3466778305049108e-17) <= 1e-14);  // r5_zr2
    CHECK(rclose(zi[1], -4.9999999999999994e-01, 1e-12));       // r5_zi2
    CHECK(rclose(zr[2], 4.0000000000000002e-01, 1e-12));        // r5_zr3
    CHECK(rclose(zi[2], 0.0, 1e-12));                           // r5_zi3
}

// ---- roots (polynomial root modulus/frequency; against ref_roots.f). --------
// Wraps rpoly with revrse + modulus/frequency; Allinv = all zeros invertible
// (every modulus >= 1). Reproduces CB-3: the frequency divisor is a typo'd 2pi
// (6.28318730707959), so a +-2i pair reports frequency 0.24999992... not 0.25.
TEST("roots: MA(2) invertible, MA(1) non-invertible, complex pair") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    {  // (1-0.2B)(1-0.3B) = 1 -0.5B +0.06B^2: roots 3.33 and 5 (invertible)
        double thetab[3] = {1.0, -0.5, 0.06};
        double zr[2] = {0}, zi[2] = {0}, zm[2] = {0}, zf[2] = {0};
        int deg = 2;
        bool allinv = false;
        roots(ctx, thetab, deg, allinv, zr, zi, zm, zf);
        CHECK_EQ(deg, 2);
        CHECK(allinv);                                    // ro1_ainv
        CHECK(rclose(zr[0], 3.3333333333333326, 1e-12));  // ro1_zr1
        CHECK(rclose(zr[1], 5.0000000000000009, 1e-12));  // ro1_zr2
        CHECK(rclose(zm[0], 3.3333333333333326, 1e-12));  // ro1_zm1
        CHECK(rclose(zm[1], 5.0000000000000009, 1e-12));  // ro1_zm2
        CHECK(rclose(zi[0], 0.0, 1e-12));
        CHECK(rclose(zf[0], 0.0, 1e-12));
    }
    {  // 1 -2B: root 0.5 -> modulus < 1 -> NOT invertible
        double thetab[2] = {1.0, -2.0};
        double zr[1] = {0}, zi[1] = {0}, zm[1] = {0}, zf[1] = {0};
        int deg = 1;
        bool allinv = false;
        roots(ctx, thetab, deg, allinv, zr, zi, zm, zf);
        CHECK_EQ(deg, 1);
        CHECK(!allinv);                    // ro2_ainv
        CHECK(rclose(zr[0], 0.5, 1e-12));  // ro2_zr1
        CHECK(rclose(zm[0], 0.5, 1e-12));  // ro2_zm1
        CHECK(rclose(zf[0], 0.0, 1e-12));  // ro2_zf1
    }
    {  // 1 +0.25B^2: roots +-2i (invertible); conjugate fill + CB-3 frequency
        double thetab[3] = {1.0, 0.0, 0.25};
        double zr[2] = {0}, zi[2] = {0}, zm[2] = {0}, zf[2] = {0};
        int deg = 2;
        bool allinv = false;
        roots(ctx, thetab, deg, allinv, zr, zi, zm, zf);
        CHECK_EQ(deg, 2);
        CHECK(allinv);                                        // ro3_ainv
        CHECK(rclose(zi[0], 2.0, 1e-12));                     // ro3_zi1
        CHECK(rclose(zi[1], -2.0, 1e-12));                    // ro3_zi2
        CHECK(rclose(zm[0], 2.0, 1e-12));                     // ro3_zm1
        CHECK(rclose(zm[1], 2.0, 1e-12));                     // ro3_zm2
        CHECK(rclose(zf[0], 2.4999992042653249e-01, 1e-12));  // ro3_zf1 (CB-3)
        CHECK(rclose(zf[1], -2.4999992042653249e-01, 1e-12)); // ro3_zf2
    }
}

// ---- setmdl (pack estprm + starting-value root check; against ref_setmdl.f). -
// MA(1) theta=0.95 (root modulus ~1.0526). Three calls on one context exercise
// the SAVEd `first`: call 1 packs + validates (no shrink); call 2 shrinks the
// near-unit-circle operator by 0.9**lag -> 0.855; call 3 leaves it (root now
// ~1.169). All invertible, so no abend and Laumts stays false.
TEST("setmdl: estprm packing, root check, and operator shrinkage") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    // MA(1) shell: DIFF empty, AR empty, MA = operator 1, lag 1.
    m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 1; m.mdl(3) = 2;
    m.opr(0) = 1; m.opr(1) = 2;
    m.arimal(1) = 1;
    m.oprfac(1) = 1;
    d.arimap(1) = 0.95;
    m.arimaf(1) = false;
    m.lar = false;

    double estprm[133] = {0};  // PARIMA
    const double c_ep[3] = {0.95, 0.855, 0.855};  // c1/c2/c3_ep1
    for (int call = 1; call <= 3; ++call) {
        bool laumts = false;
        setmdl(ctx, estprm, laumts);
        CHECK_EQ(m.nestpm, 1);                            // c_nestpm
        CHECK(!laumts);                                   // c_laum
        CHECK(rclose(estprm[0], c_ep[call - 1], 1e-12));  // c_ep1
        CHECK(rclose(d.arimap(1), c_ep[call - 1], 1e-12));  // c_ap1
    }
}

// ---- rgarma (THE regARIMA IGLS estimation engine; against ref_rgarma.f). ------
// No-regression (Nb=0) pure ARMA(1,1) -- phi & theta both free, exact ML -- on a
// 24-point stationary series held in Xy (Ncxy=1). Lprier/Lprtit both false and
// Savtab clear, so all deferred prints/saves are inert. Estimation runs one IGLS
// pass driving lmdif over the two ARMA parameters (14 nonlinear iterations, 47
// fcn evals), then builds the ARMA covariance (fdjac2/qrfac/covar). The exact
// integer counters Nliter/Nfev matching is the strong canary that the whole
// optimizer trajectory is bit-identical to the oracle (scouting parity risk 1/4).
TEST("rgarma: ARMA(1,1) no-regression IGLS estimation") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    // 24-point stationary working series (the y column of Xy, Ncxy=1).
    d.nspobs = 24;
    m.ncxy = 1;
    const double series[24] = {0.5, -0.3, 0.8, -0.6, 0.2,  0.9, -0.7, 0.4,
                               0.1, -0.5, 0.6, -0.2, 0.7,  -0.8, 0.3, 0.5,
                               -0.4, 0.9, -0.1, 0.6, -0.7, 0.2, 0.4, -0.5};
    for (int i = 1; i <= 24; ++i) d.xy(i) = series[i - 1];

    // ARMA(1,1) model shell: one AR operator (lag 1), one MA operator (lag 1).
    m.lar = true; m.lma = true; m.lextma = true; m.lextar = false;
    m.lprier = false; ctx.hiddn.lhiddn = false;
    m.nopr = 2;
    m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 2; m.mdl(3) = 3;
    m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3;
    m.arimal(1) = 1; m.arimal(2) = 1;
    m.arimaf(1) = false; m.arimaf(2) = false;
    m.oprfac(1) = 1; m.oprfac(2) = 1;
    m.mxarlg = 1; m.mxmalg = 1; m.mxdflg = 0;
    d.arimap(1) = 0.3; d.arimap(2) = 0.3;  // starting values

    // No regression, no differencing intervals.
    m.nb = 0; m.nintvl = 0; m.nextvl = 0;
    // Tolerances / step (overall Tol used since Nb=0).
    m.tol = 1e-5; m.nltol0 = 1e-5; m.nltol = 1e-5; m.stepln = 0.0;
    // Run controls: gudrun=T, no fatal, table saves off (zero-init already).
    ctx.hiddn.issap = 0; ctx.hiddn.irev = 0;
    ctx.error.lfatal = false;
    ctx.units.mt1 = 6; ctx.units.mt2 = 6;

    auto a = std::make_unique<double[]>(1092);  // PA residual work vector
    int na = 0, nefobs = 0;
    bool lauto = false;
    rgarma(ctx, /*lestim=*/true, /*mxiter=*/200, /*mxnlit=*/60, /*lprtit=*/false,
           a.get(), na, nefobs, lauto);

    CHECK_EQ(d.armaer, 0);   // rg_armaer
    CHECK(d.convrg);         // rg_convrg
    CHECK_EQ(d.nliter, 14);  // rg_nliter
    CHECK_EQ(d.nfev, 47);    // rg_nfev
    CHECK_EQ(na, 25);        // rg_na
    CHECK_EQ(nefobs, 24);    // rg_nefobs
    CHECK(!lauto);           // rg_lauto
    CHECK(m.lcalcm);         // rg_lcalcm
    CHECK(rclose(d.arimap(1), -4.9893724335763090e-01, 1e-12));  // rg_phi
    CHECK(rclose(d.arimap(2),  2.0807091229579630e-01, 1e-12));  // rg_theta
    CHECK(rclose(d.var,        1.7987081616384901e-01, 1e-12));  // rg_var
    CHECK(rclose(d.lnlkhd,    -1.3732363162787594e+01, 1e-12));  // rg_lnlkhd
    CHECK(rclose(d.lndtcv,     5.2806970546683596e-01, 1e-12));  // rg_ldtcv
    CHECK(rclose(d.armacm(1, 1), 3.8913728603066050e-01, 1e-12));  // rg_cm11
    CHECK(rclose(d.armacm(2, 2), 5.1567691371183755e-01, 1e-12));  // rg_cm22

    // Post-estimation stats off this converged state (against ref_armastat.f).
    double aicc = 0.0;
    xrlkhd(ctx, aicc, /*nxcld=*/0);
    CHECK(rclose(aicc, 2.9646544507393372e+01, 1e-12));  // as_aicc
    double tval[2] = {0.0, 0.0};
    armats(ctx, tval);
    CHECK(rclose(tval[0], -1.8858804037050478e+00, 1e-12));  // as_tphi
    CHECK(rclose(tval[1],  6.8319105929467638e-01, 1e-12));  // as_ttheta
}

// ---- rgarma with a FIXED ARMA coefficient (against ref_rgarma_fixed.f). --------
// ARMA(1,1) with theta HELD at 0.4 (arimaf(2)=true), so only phi is estimated:
// Nestpm=1, lmdif optimizes one parameter, and upespm/setmdl exercise the
// fixed-lag skip that every all-free rgarma case bypasses (test-plan C2/B3). The
// held coefficient must come back exactly 0.4. Oracle at rtol 1e-12.
TEST("rgarma: fixed ARMA coefficient (theta held, only phi estimated)") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    d.nspobs = 24;
    m.ncxy = 1;
    const double series[24] = {0.5, -0.3, 0.8, -0.6, 0.2,  0.9, -0.7, 0.4,
                               0.1, -0.5, 0.6, -0.2, 0.7,  -0.8, 0.3, 0.5,
                               -0.4, 0.9, -0.1, 0.6, -0.7, 0.2, 0.4, -0.5};
    for (int i = 1; i <= 24; ++i) d.xy(i) = series[i - 1];

    m.lar = true; m.lma = true; m.lextma = true; m.lextar = false;
    m.lprier = false; ctx.hiddn.lhiddn = false;
    m.nopr = 2;
    m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 2; m.mdl(3) = 3;
    m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3;
    m.arimal(1) = 1; m.arimal(2) = 1;
    m.arimaf(1) = false; m.arimaf(2) = true;   // phi free, theta FIXED
    m.oprfac(1) = 1; m.oprfac(2) = 1;
    m.mxarlg = 1; m.mxmalg = 1; m.mxdflg = 0;
    d.arimap(1) = 0.3; d.arimap(2) = 0.4;       // phi start, theta held at 0.4
    m.nb = 0; m.nintvl = 0; m.nextvl = 0; m.iregfx = 0;
    m.tol = 1e-5; m.nltol0 = 1e-5; m.nltol = 1e-5; m.stepln = 0.0;
    ctx.hiddn.issap = 0; ctx.hiddn.irev = 0;
    ctx.error.lfatal = false;
    ctx.units.mt1 = 6; ctx.units.mt2 = 6;

    auto a = std::make_unique<double[]>(1092);
    int na = 0, nefobs = 0;
    bool lauto = false;
    rgarma(ctx, true, 200, 60, false, a.get(), na, nefobs, lauto);

    CHECK(d.convrg);
    CHECK_EQ(d.nliter, 3);
    CHECK_EQ(d.nfev, 7);
    CHECK_EQ(m.nestpm, 1);   // only phi is a free parameter
    CHECK(rclose(d.arimap(1), -3.3700310088310953e-01, 1e-12));  // estimated phi
    CHECK(rclose(d.arimap(2), 0.4, 1e-15));                      // theta held exact
    CHECK(rclose(d.var, 1.8611106696857027e-01, 1e-12));
    CHECK(rclose(d.lnlkhd, -1.4151496390762206e+01, 1e-12));
}

// ---- fcstxy (forecasts + forecast SEs; against ref_fcstxy.f). ------------------
// Estimates the same no-regression ARMA(1,1) as the rgarma test, then forecasts
// 6 steps ahead. Nb=0 so the design-uncertainty term is skipped (Rgvar all 0);
// this validates the forecast recursion (full AR*diff and MA operators via
// polyml, seeded by the exact ARMA-filtered residuals) and the psi(B)-weight
// standard errors. Fctori=Nspobs=24 (no differencing). Oracle at rtol 1e-12.
TEST("fcstxy: ARMA(1,1) no-regression forecasts + standard errors") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    d.nspobs = 24;
    m.ncxy = 1;
    const double series[24] = {0.5, -0.3, 0.8, -0.6, 0.2,  0.9, -0.7, 0.4,
                               0.1, -0.5, 0.6, -0.2, 0.7,  -0.8, 0.3, 0.5,
                               -0.4, 0.9, -0.1, 0.6, -0.7, 0.2, 0.4, -0.5};
    for (int i = 1; i <= 24; ++i) d.xy(i) = series[i - 1];

    m.lar = true; m.lma = true; m.lextma = true; m.lextar = false;
    m.lprier = false; ctx.hiddn.lhiddn = false;
    m.nopr = 2;
    m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 2; m.mdl(3) = 3;
    m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3;
    m.arimal(1) = 1; m.arimal(2) = 1;
    m.arimaf(1) = false; m.arimaf(2) = false;
    m.oprfac(1) = 1; m.oprfac(2) = 1;
    m.mxarlg = 1; m.mxmalg = 1; m.mxdflg = 0;
    d.arimap(1) = 0.3; d.arimap(2) = 0.3;
    m.nb = 0; m.nintvl = 0; m.nextvl = 0; m.iregfx = 0;
    m.tol = 1e-5; m.nltol0 = 1e-5; m.nltol = 1e-5; m.stepln = 0.0;
    ctx.hiddn.issap = 0; ctx.hiddn.irev = 0;
    ctx.error.lfatal = false;
    ctx.units.mt1 = 6; ctx.units.mt2 = 6;

    auto a = std::make_unique<double[]>(1092);
    int na = 0, nefobs = 0;
    bool lauto = false;
    rgarma(ctx, true, 200, 60, false, a.get(), na, nefobs, lauto);
    CHECK(d.convrg);

    double fcst[6] = {0}, se[6] = {0}, rgvar[6] = {0};
    fcstxy(ctx, /*fctori=*/24, /*nfcst=*/6, fcst, se, rgvar);

    const double ofcst[6] = {
        2.9218163115413764e-01, -1.4578029760778152e-01, 7.2735219824281550e-02,
        -3.6290310074138341e-02, 1.8106587268984245e-02, -9.0340507386013728e-03};
    const double ose[6] = {
        4.2411179677515337e-01, 5.1940459956728047e-01, 5.4052129216262779e-01,
        5.4565102227665507e-01, 5.4692052957970538e-01, 5.4723610083821750e-01};
    for (int i = 0; i < 6; ++i) {
        CHECK(rclose(fcst[i], ofcst[i], 1e-12));
        CHECK(rclose(se[i], ose[i], 1e-12));
        CHECK_EQ(rgvar[i], 0.0);
    }
}

// ---- fcstxy WITH regression (Nb>0; against ref_fcstxy2.f). ---------------------
// Estimates the ARMA(1,1)+intercept of the Nb>0 rgarma case, extends the
// intercept design into the 6 forecast rows (Xy rows 25..30 = [1, 0]), then
// forecasts. Nb=1, Iregfx=0 -> nb2=1, so the design-uncertainty term
// X_f(X'X)^-1 X_f' fires (dppsl/yprmy) and Rgvar is nonzero -- the branch the
// no-regression case skips. Oracle at rtol 1e-11.
TEST("fcstxy: ARMA(1,1) + intercept forecasts (Nb>0, dppsl design term)") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    const double ser[24] = {0.5, -0.3, 0.8, -0.6, 0.2,  0.9, -0.7, 0.4,
                            0.1, -0.5, 0.6, -0.2, 0.7,  -0.8, 0.3, 0.5,
                            -0.4, 0.9, -0.1, 0.6, -0.7, 0.2, 0.4, -0.5};
    d.nspobs = 24;
    m.ncxy = 2;
    for (int i = 1; i <= 24; ++i) {
        d.xy(2 * i - 1) = 1.0;
        d.xy(2 * i) = ser[i - 1] + 2.0;
    }
    m.lar = true; m.lma = true; m.lextma = true; m.lextar = false;
    m.lprier = false; ctx.hiddn.lhiddn = false;
    m.nopr = 2;
    m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 2; m.mdl(3) = 3;
    m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3;
    m.arimal(1) = 1; m.arimal(2) = 1;
    m.arimaf(1) = false; m.arimaf(2) = false;
    m.oprfac(1) = 1; m.oprfac(2) = 1;
    m.mxarlg = 1; m.mxmalg = 1; m.mxdflg = 0;
    d.arimap(1) = 0.3; d.arimap(2) = 0.3;
    m.nb = 1; m.nintvl = 0; m.nextvl = 0; m.iregfx = 0;
    m.tol = 1e-5; m.nltol0 = 1e-3; m.nltol = 1e-5; m.stepln = 0.0;
    ctx.hiddn.issap = 0; ctx.hiddn.irev = 0;
    ctx.error.lfatal = false;
    ctx.units.mt1 = 6; ctx.units.mt2 = 6;

    auto a = std::make_unique<double[]>(1092);
    int na = 0, nefobs = 0;
    bool lauto = false;
    rgarma(ctx, true, 200, 60, false, a.get(), na, nefobs, lauto);
    CHECK(d.convrg);

    // Extend the intercept design into the forecast rows (y slot zeroed by fcstxy).
    for (int i = 25; i <= 30; ++i) { d.xy(2 * i - 1) = 1.0; d.xy(2 * i) = 0.0; }

    double fcst[6] = {0}, se[6] = {0}, rgvar[6] = {0};
    fcstxy(ctx, /*fctori=*/24, /*nfcst=*/6, fcst, se, rgvar);

    const double ofcst[6] = {
        2.5716559843740114e+00, 1.9567341331768837e+00, 2.1449022022421316e+00,
        2.0873221664265023e+00, 2.1049418430326297e+00, 2.0995501651827131e+00};
    const double ose[6] = {
        3.3687688811033284e-01, 5.2254376660852997e-01, 5.3736600702490056e-01,
        5.3851466405151294e-01, 5.3868891974149136e-01, 5.3868478718934021e-01};
    const double orgv[6] = {
        1.2807928800118933e-02, 6.5419658213969225e-04, 2.8495104099106040e-04,
        1.5118733177552387e-05, 6.1841553817841690e-05, 4.4187604400179843e-05};
    for (int i = 0; i < 6; ++i) {
        CHECK(rclose(fcst[i], ofcst[i], 1e-11));
        CHECK(rclose(se[i], ose[i], 1e-11));
        CHECK(rclose(rgvar[i], orgv[i], 1e-10));
    }
}

// ---- fcstxy on a DIFFERENCED model (mxdfar>0; against ref_fcstxy3.f). ----------
// (0 1 1): nonseasonal diff (1-B, coef 1 fixed) + MA(1), so Mxdflg=1, Mxarlg=0,
// mxdfar=1 -- the tfcst-seeding-from-the-last-row + ndltar-offset recursion the
// no-diff cases never touch (test-coverage C4). I(1) cumulative-sum series so the
// differenced fit is well-behaved; the I(1) forecast is flat (last level) and the
// SEs grow with horizon, both as expected. Oracle at rtol 1e-11.
TEST("fcstxy: differenced (0 1 1) forecasts, mxdfar>0 seeding") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    const double ser[24] = {0.5, -0.3, 0.8, -0.6, 0.2,  0.9, -0.7, 0.4,
                            0.1, -0.5, 0.6, -0.2, 0.7,  -0.8, 0.3, 0.5,
                            -0.4, 0.9, -0.1, 0.6, -0.7, 0.2, 0.4, -0.5};
    d.nspobs = 24;
    m.ncxy = 1;
    double acc = 10.0;
    for (int i = 1; i <= 24; ++i) { acc += ser[i - 1]; d.xy(i) = acc; }  // I(1)

    m.lar = false; m.lma = true; m.lextma = true; m.lextar = true;
    m.lprier = false; ctx.hiddn.lhiddn = false;
    m.nopr = 2;
    m.mdl(0) = 1; m.mdl(1) = 2; m.mdl(2) = 2; m.mdl(3) = 3;  // DIFF op, no AR, MA op
    m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3;
    m.arimal(1) = 1; m.arimal(2) = 1;
    m.arimaf(1) = true; m.arimaf(2) = false;   // diff coef fixed, MA free
    m.oprfac(1) = 1; m.oprfac(2) = 1;
    m.mxarlg = 0; m.mxmalg = 1; m.mxdflg = 1;
    d.arimap(1) = 1.0; d.arimap(2) = 0.3;      // diff coef 1, MA start 0.3
    m.nb = 0; m.nintvl = 1; m.nextvl = 1; m.iregfx = 0;
    m.tol = 1e-5; m.nltol0 = 1e-5; m.nltol = 1e-5; m.stepln = 0.0;
    ctx.hiddn.issap = 0; ctx.hiddn.irev = 0;
    ctx.error.lfatal = false;
    ctx.units.mt1 = 6; ctx.units.mt2 = 6;

    auto a = std::make_unique<double[]>(1092);
    int na = 0, nefobs = 0;
    bool lauto = false;
    rgarma(ctx, true, 200, 60, false, a.get(), na, nefobs, lauto);
    CHECK(d.convrg);
    CHECK(rclose(d.arimap(2), 5.1399452122580702e-01, 1e-11));  // MA coef

    double fcst[6] = {0}, se[6] = {0}, rgvar[6] = {0};
    fcstxy(ctx, /*fctori=*/24, /*nfcst=*/6, fcst, se, rgvar);

    const double ose[6] = {
        4.5218674809544068e-01, 5.0276198598175748e-01, 5.4869515574999583e-01,
        5.9106948266570447e-01, 6.3060280108313416e-01, 6.6779985933592878e-01};
    for (int i = 0; i < 6; ++i) {
        // I(1) MMSE forecast is flat at the last estimated level.
        CHECK(rclose(fcst[i], 1.2445409112913696e+01, 1e-11));
        CHECK(rclose(se[i], ose[i], 1e-11));
        CHECK_EQ(rgvar[i], 0.0);
    }
}

// ---- rgarma WITH regression (Nb>0; against ref_rgarma2.f). ---------------------
// Same 24-point series shifted to mean 2.0 with an intercept column, so Xy is
// 24x2 = [1, y] (Ncxy=2, Nb=1). Drives the branches the Nb=0 case skips: the
// olsreg GLS solve (Nfev+=Ncxy+1 per pass), the multi-pass IGLS outer loop
// (locest stays true while Nb>0; tnltol switches from 2/n*Nltol0 to 2/n*Nltol
// after iter 2), and resid over a real regression column. The oracle converges
// to a NEAR-UNIT MA root (theta~0.99999) -- a penalty-wall stress case (parity
// risk 1); the exact Nliter=12/Nfev=56 match confirms the trajectory is
// bit-identical even at the invertibility boundary.
TEST("rgarma: ARMA(1,1) + intercept, multi-pass IGLS (Nb>0)") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    const double ser[24] = {0.5, -0.3, 0.8, -0.6, 0.2,  0.9, -0.7, 0.4,
                            0.1, -0.5, 0.6, -0.2, 0.7,  -0.8, 0.3, 0.5,
                            -0.4, 0.9, -0.1, 0.6, -0.7, 0.2, 0.4, -0.5};
    d.nspobs = 24;
    m.ncxy = 2;
    for (int i = 1; i <= 24; ++i) {
        d.xy(2 * i - 1) = 1.0;             // intercept column
        d.xy(2 * i) = ser[i - 1] + 2.0;    // y column (mean 2.0)
    }

    m.lar = true; m.lma = true; m.lextma = true; m.lextar = false;
    m.lprier = false; ctx.hiddn.lhiddn = false;
    m.nopr = 2;
    m.mdl(0) = 1; m.mdl(1) = 1; m.mdl(2) = 2; m.mdl(3) = 3;
    m.opr(0) = 1; m.opr(1) = 2; m.opr(2) = 3;
    m.arimal(1) = 1; m.arimal(2) = 1;
    m.arimaf(1) = false; m.arimaf(2) = false;
    m.oprfac(1) = 1; m.oprfac(2) = 1;
    m.mxarlg = 1; m.mxmalg = 1; m.mxdflg = 0;
    d.arimap(1) = 0.3; d.arimap(2) = 0.3;

    m.nb = 1; m.nintvl = 0; m.nextvl = 0;
    m.tol = 1e-5; m.nltol0 = 1e-3; m.nltol = 1e-5; m.stepln = 0.0;
    ctx.hiddn.issap = 0; ctx.hiddn.irev = 0;
    ctx.error.lfatal = false;
    ctx.units.mt1 = 6; ctx.units.mt2 = 6;

    auto a = std::make_unique<double[]>(1092);
    int na = 0, nefobs = 0;
    bool lauto = false;
    rgarma(ctx, true, 200, 60, false, a.get(), na, nefobs, lauto);

    CHECK_EQ(d.armaer, 0);   // rg_armaer
    CHECK(d.convrg);         // rg_convrg
    CHECK_EQ(d.nliter, 12);  // rg_nliter
    CHECK_EQ(d.nfev, 56);    // rg_nfev
    CHECK_EQ(na, 25);        // rg_na
    CHECK_EQ(nefobs, 24);    // rg_nefobs
    CHECK(!lauto);           // rg_lauto
    CHECK(m.lcalcm);         // rg_lcalcm
    CHECK(rclose(d.b(1),       2.1008134628340014e+00, 1e-12));  // rg_b1
    CHECK(rclose(d.arimap(1), -3.0600322414778652e-01, 1e-12));  // rg_phi
    CHECK(rclose(d.arimap(2),  9.9999478122852692e-01, 1e-12));  // rg_theta
    CHECK(rclose(d.var,        1.0067810894278280e-01, 1e-12));  // rg_var
    CHECK(rclose(d.lnlkhd,    -8.4206459605326067e+00, 1e-12));  // rg_lnlkhd
    CHECK(rclose(d.lndtcv,     3.8320880866185578e+00, 1e-12));  // rg_ldtcv
    CHECK(rclose(d.armacm(1, 1), 3.4961993382451767e-01, 1e-12));  // rg_cm11
    CHECK(rclose(d.armacm(2, 2), 1.8308075964429438e-01, 1e-12));  // rg_cm22
}

// ---- mdlfin (gtinpt.f:220 model-finalize; the spec->estimate bridge). ----------
// Derives Lar/Lma/Nintvl/Nextvl from the operator max-lags + exact-ARMA switches.
// Pure int/bool, so the formula is its own oracle; four configs cover both
// branches and the flag gates. (Closes the cheapest slice of the m3_scouting.md
// section 7 spec->estimate gap.)
TEST("mdlfin: ARMA model-dimension finalize for estimation") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    auto& m = ctx.model;

    // (a) Airline (0 1 1)(0 1 1)12, exact ARMA default: no AR, diff=1+12,
    //     MA=1+12. Lar off (Mxarlg=0), Lma on; Lextar branch.
    m.lextar = true; m.lextma = true;
    m.mxarlg = 0; m.mxdflg = 13; m.mxmalg = 13;
    mdlfin(ctx);
    CHECK(!m.lar); CHECK(m.lma);
    CHECK_EQ(m.nintvl, 13); CHECK_EQ(m.nextvl, 13);

    // (b) Pure AR(2), exact: Lar on, Lma off; Lextar branch, no differencing.
    m.lextar = true; m.lextma = true;
    m.mxarlg = 2; m.mxdflg = 0; m.mxmalg = 0;
    mdlfin(ctx);
    CHECK(m.lar); CHECK(!m.lma);
    CHECK_EQ(m.nintvl, 0); CHECK_EQ(m.nextvl, 2);

    // (c) Conditional AR (Lextar=F) with AR(1), diff 1, MA(1): AR folded into
    //     the differencing count; Lma on via Lextma. else branch.
    m.lextar = false; m.lextma = true;
    m.mxarlg = 1; m.mxdflg = 1; m.mxmalg = 1;
    mdlfin(ctx);
    CHECK(!m.lar); CHECK(m.lma);
    CHECK_EQ(m.nintvl, 2); CHECK_EQ(m.nextvl, 1);

    // (d) Conditional throughout (Lextar=F, Lextma=F): no exact paths, Nextvl 0.
    m.lextar = false; m.lextma = false;
    m.mxarlg = 0; m.mxdflg = 0; m.mxmalg = 1;
    mdlfin(ctx);
    CHECK(!m.lar); CHECK(!m.lma);
    CHECK_EQ(m.nintvl, 0); CHECK_EQ(m.nextvl, 0);
}

// ---- estimate{} spec reader end-to-end (gt_estimate + mdlfin, via parse_spec).
// Parses a real spec (inline data + arima(0 1 1) + estimate{}) through the whole
// M1/M2 front end and asserts the estimation-control state rgarma consumes.
// Verifies the spec->estimate wiring: knob application, tol reconciliation, and
// the mdlfin finalize -- the pieces that let run_m2 feed a real model to rgarma.
TEST("estimate{}: spec reader applies knobs + mdlfin finalize") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    const char* spec =
        "series{\n"
        "  start = 1990.1\n"
        "  period = 12\n"
        "  data = (\n"
        "   112 118 132 129 121 135 148 148 136 119 104 118\n"
        "   115 126 141 135 125 149 170 170 158 133 114 140\n"
        "   145 150 178 163 172 178 199 199 184 162 146 166 )\n"
        "}\n"
        "arima{ model = (0 1 1) }\n"
        "estimate{ maxiter=77 maxnliter=33 tol=1e-4 nltol=1e-6 exact=ma "
        "step=0.001 }\n";
    bool ok = parse_spec(ctx, spec, "estimate-test.spc");
    CHECK(ok);
    auto& m = ctx.model;

    // Applied estimate{} knobs.
    CHECK_EQ(ctx.arima.mxiter, 77);
    CHECK_EQ(ctx.arima.mxnlit, 33);
    CHECK(rclose(m.tol, 1e-4, 1e-15));
    CHECK(rclose(m.stepln, 1e-3, 1e-15));
    CHECK(ctx.arima.lestim);          // default (no parms arg)
    // exact = ma  ->  Lextar off, Lextma on.
    CHECK(!m.lextar);
    CHECK(m.lextma);
    // nltol supplied -> hvnltl: Nltol0 = Nltol (gtestm.f tail).
    CHECK(rclose(m.nltol, 1e-6, 1e-15));
    CHECK(rclose(m.nltol0, 1e-6, 1e-15));

    // mdlfin on (0 1 1): Mxarlg=0, Mxdflg=1, Mxmalg=1; exact=ma (Lextar=F).
    CHECK(!m.lar);
    CHECK(m.lma);
    CHECK_EQ(m.nintvl, 1);   // Mxdflg + Mxarlg
    CHECK_EQ(m.nextvl, 1);   // Lextma ? Mxmalg : 0
}

// ---- run_m2 -> rgarma end-to-end REAL-DATA estimation parity (the M3 headline).
// Drives the full front end -- parse_spec (getsrs/transform/getmdl/regvar) then
// rgarma -- on the classic airline series (144 obs, log transform, ARMA
// (0 1 1)(0 1 1), no regression) exactly as the airline_check corpus spec, and
// compares the estimated ARMA coefficients / variance / iteration counters
// against the ORACLE .udg golden (tests/golden/extra/airline_check). This is the
// first estimation parity from a real spec through the real pipeline (not
// hand-set common state): rgarma differences Xy internally (Nintvl=13 -> Nefobs=
// 131) and writes Tsrs from Xy itself. udg targets: MA nonseasonal coef
// 0.40180794878596 (t 5.0945818522955), MA seasonal 0.55694564337114 (t
// 7.3036848055248), variance$mle 0.13480973219978E-02, niter 6, nfev 19.
TEST("run_m2->rgarma: airline (0 1 1)(0 1 1) real-data estimation vs oracle udg") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    const char* spec =
        "series{\n"
        "  title = \"International Airline Passengers\"\n"
        "  start = 1949.01\n"
        "  period = 12\n"
        "  data = (\n"
        "   112 118 132 129 121 135 148 148 136 119 104 118\n"
        "   115 126 141 135 125 149 170 170 158 133 114 140\n"
        "   145 150 178 163 172 178 199 199 184 162 146 166\n"
        "   171 180 193 181 183 218 230 242 209 191 172 194\n"
        "   196 196 236 235 229 243 264 272 237 211 180 201\n"
        "   204 188 235 227 234 264 302 293 259 229 203 229\n"
        "   242 233 267 269 270 315 364 347 312 274 237 278\n"
        "   284 277 317 313 318 374 413 405 355 306 271 306\n"
        "   315 301 356 348 355 422 465 467 404 347 305 336\n"
        "   340 318 362 348 363 435 491 505 404 359 310 337\n"
        "   360 342 406 396 420 472 548 559 463 407 362 405\n"
        "   417 391 419 461 472 535 622 606 508 461 390 432 )\n"
        "}\n"
        "transform{ function = log }\n"
        "arima{ model = (0 1 1)(0 1 1) }\n"
        "estimate{ }\n";

    bool ok = run_m2(ctx, spec, "airline_check", /*estimate=*/true);
    CHECK(ok);
    CHECK(!ctx.error.lfatal);

    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    // Model dimensions the front end derived (mdlfin): (0 1 1)(0 1 1) -> Mxarlg=0,
    // Mxmalg=12, Mxdflg=13 (1 + 12). Nintvl=Mxdflg=13, Nefobs=144-13=131.
    CHECK_EQ(d.nspobs, 144);
    CHECK_EQ(m.nb, 0);
    CHECK_EQ(m.nintvl, 13);
    CHECK_EQ(d.nspobs - m.nintvl, 131);   // nefobs

    // Estimation converged; the integer optimizer counters are the exact-
    // trajectory canary against the oracle udg (niter 6, nfev 19).
    CHECK(d.convrg);
    CHECK_EQ(d.armaer, 0);
    CHECK_EQ(d.nliter, 6);
    CHECK_EQ(d.nfev, 19);

    // ARMA coefficients. arimap is packed by operator-coefficient position over
    // DIFF..AR..MA (not by literal lag): the two differencing operators (1-B),
    // (1-B^12) take fixed slots 1,2 (coef 1), so the free MA coefficients land at
    // slots 3 (nonseasonal lag 1) and 4 (seasonal lag 12).
    CHECK(rclose(d.arimap(3), 0.40180794878596, 1e-10));   // MA nonseasonal
    CHECK(rclose(d.arimap(4), 0.55694564337114, 1e-10));   // MA seasonal
    // MLE innovation variance.
    CHECK(rclose(d.var, 0.13480973219978e-02, 1e-12));

    // ARMA t-statistics (armats): coef / sqrt(Var * Armacm_kk), oracle udg
    // reports 5.0945818522955 and 7.3036848055248 as the third field.
    double tval[2] = {0.0, 0.0};
    armats(ctx, tval);
    CHECK(rclose(tval[0], 5.0945818522955, 1e-9));   // MA nonseasonal t
    CHECK(rclose(tval[1], 7.3036848055248, 1e-9));   // MA seasonal t

    // Likelihood statistics. The .udg "loglikelihood" key is the RAW Lnlkhd
    // (arima.f:973 writes Lnlkhd directly), while AIC/AICC/BIC/HQ come from
    // prlkhd and use the transform-Jacobian-adjusted Olkhd=Lnlkhd+jacadj
    // internally. run_m2 ran prlkhd into ctx.lkhd. The .udg rounds to 4
    // decimals, so compare at rtol 1e-6.
    CHECK(rclose(d.lnlkhd, 244.6965, 1e-6));         // loglikelihood (raw Lnlkhd)
    CHECK(rclose(ctx.lkhd.aic, 987.1956, 1e-6));     // AIC
    CHECK(rclose(ctx.lkhd.aicc, 987.3845, 1e-6));    // AICC
    CHECK(rclose(ctx.lkhd.bic, 995.8211, 1e-6));     // BIC
    CHECK(rclose(ctx.lkhd.hnquin, 990.7005, 1e-6));  // Hannan-Quinn
}

// ---- run_m2 -> rgarma REAL-DATA estimation WITH regression (Nb>0). -------------
// Same airline series, but regression{ variables = (td easter[8]) } -- 6 free
// trading-day columns + 1 Easter column (Nb=7, Ncxy=8), the derived Sunday
// column reconstructed post-hoc. Drives the GLS olsreg path (not the Nb=0 yprmy
// path) through the real regvar-built design matrix (td6var/td7var + Easter),
// matching the oracle .udg golden (02-airline-log-td-easter). The forecast{}/
// x11{} of the corpus spec are dropped -- they run after estimation and don't
// change the fit -- so the .udg estimation values are the oracle for this spec.
// Targets: niter 9, nfev 76, MA 0.21534460625351 / 0.55174518432520,
// Easter beta 0.0219499756200038, TD-Mon beta -0.00547059178976284,
// variance$mle 0.10803917843822E-02, loglikelihood 259.3105.
TEST("run_m2->rgarma: airline + TD + Easter regression (Nb>0) vs oracle udg") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    const char* spec =
        "series{\n"
        "  title = \"International Airline Passengers\"\n"
        "  start = 1949.01\n"
        "  period = 12\n"
        "  data = (\n"
        "   112 118 132 129 121 135 148 148 136 119 104 118\n"
        "   115 126 141 135 125 149 170 170 158 133 114 140\n"
        "   145 150 178 163 172 178 199 199 184 162 146 166\n"
        "   171 180 193 181 183 218 230 242 209 191 172 194\n"
        "   196 196 236 235 229 243 264 272 237 211 180 201\n"
        "   204 188 235 227 234 264 302 293 259 229 203 229\n"
        "   242 233 267 269 270 315 364 347 312 274 237 278\n"
        "   284 277 317 313 318 374 413 405 355 306 271 306\n"
        "   315 301 356 348 355 422 465 467 404 347 305 336\n"
        "   340 318 362 348 363 435 491 505 404 359 310 337\n"
        "   360 342 406 396 420 472 548 559 463 407 362 405\n"
        "   417 391 419 461 472 535 622 606 508 461 390 432 )\n"
        "}\n"
        "transform{ function = log }\n"
        "regression{ variables = (td easter[8]) }\n"
        "arima{ model = (0 1 1)(0 1 1) }\n"
        "estimate{ }\n";

    bool ok = run_m2(ctx, spec, "02-airline-log-td-easter", /*estimate=*/true);
    CHECK(ok);
    CHECK(!ctx.error.lfatal);

    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    // 6 TD + 1 Easter free regressors -> Nb=7, Ncxy=8.
    CHECK_EQ(m.nb, 7);
    CHECK_EQ(m.ncxy, 8);
    CHECK(d.convrg);
    CHECK_EQ(d.armaer, 0);
    CHECK_EQ(d.nliter, 9);   // niter
    CHECK_EQ(d.nfev, 76);    // nfev

    // ARMA coefficients (slots 3/4 behind the two differencing slots).
    CHECK(rclose(d.arimap(3), 0.21534460625351, 1e-10));   // MA nonseasonal
    CHECK(rclose(d.arimap(4), 0.55174518432520, 1e-10));   // MA seasonal
    CHECK(rclose(d.var, 0.10803917843822e-02, 1e-11));     // variance$mle

    // Regression betas (b packs the design columns in spec order: 6 TD then
    // Easter). Check the first TD contrast and the Easter coefficient.
    CHECK(rclose(d.b(1), -0.547059178976284e-02, 1e-9));   // Trading Day Mon
    CHECK(rclose(d.b(7),  0.219499756200038e-01, 1e-9));   // Easter[8]

    // Raw log likelihood (the .udg key), transform-Jacobian AIC via prlkhd.
    CHECK(rclose(d.lnlkhd, 259.3105, 1e-6));
}

// ---- invfcn / lgnrmc (inverse transform + lognormal correction). --------------
// The forecast-output numeric leaves: invfcn maps transformed forecasts back to
// the original scale, lgnrmc applies the lognormal mean-correction. Oracle values
// from drv_inv.f (invfcn.f/lgnrmc.f driven directly). rtol 1e-14.
TEST("invfcn: inverse Box-Cox / logit transform (all branches)") {
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;
    const double t[4] = {0.5, -0.3, 1.2, 2.0};
    double y[4] = {0, 0, 0, 0};

    invfcn(ctx, t, 4, /*fcntyp=*/1, /*lam=*/0.0, y);   // exp (log inverse)
    CHECK(rclose(y[0], 0.1648721270700128e+01, 1e-14));
    CHECK(rclose(y[1], 0.7408182206817179e+00, 1e-14));
    CHECK(rclose(y[2], 0.3320116922736547e+01, 1e-14));
    CHECK(rclose(y[3], 0.7389056098930650e+01, 1e-14));

    invfcn(ctx, t, 4, /*fcntyp=*/0, /*lam=*/0.5, y);   // inverse box-cox
    CHECK(rclose(y[0], 1.265625, 1e-14));
    CHECK(rclose(y[1], 0.525625, 1e-14));
    CHECK(rclose(y[2], 2.175625, 1e-14));
    CHECK(rclose(y[3], 3.515625, 1e-14));

    invfcn(ctx, t, 4, /*fcntyp=*/3, /*lam=*/0.0, y);   // inverse logit
    CHECK(rclose(y[0], 0.6224593312018546e+00, 1e-14));
    CHECK(rclose(y[3], 0.8807970779778824e+00, 1e-14));

    invfcn(ctx, t, 4, /*fcntyp=*/0, /*lam=*/1.0, y);   // identity
    CHECK(rclose(y[0], 0.5, 1e-15));
    CHECK(rclose(y[1], -0.3, 1e-15));
}

TEST("lgnrmc: lognormal mean-correction of forecasts") {
    const double unc[3] = {0.5, 1.0, -0.2};
    const double se[3] = {0.3, 0.4, 0.25};
    double cor[3] = {0, 0, 0};

    lgnrmc(3, unc, se, cor, /*ltrans=*/true);   // exp(0.5*se^2 + unc)
    CHECK(rclose(cor[0], 0.1724608382376436e+01, 1e-14));
    CHECK(rclose(cor[1], 0.2944679551065524e+01, 1e-14));
    CHECK(rclose(cor[2], 0.8447200570049834e+00, 1e-14));

    lgnrmc(3, unc, se, cor, /*ltrans=*/false);  // 0.5*se^2 + unc (still transformed)
    CHECK(rclose(cor[0], 0.545, 1e-14));
    CHECK(rclose(cor[1], 1.080, 1e-14));
    CHECK(rclose(cor[2], -0.16875, 1e-14));
}

// ---- eltfcn / devlpl / cumnor / dinvnr (forecast-output numeric leaves). -----
// eltfcn: elementwise vector op. devlpl: Horner polynomial. cumnor: normal CDF.
// dinvnr: inverse normal CDF (CI critical value for prtfct). Golden values from
// ref_dinvnr.f (dinvnr/cumnor/stvaln/devlpl/spmpar/ipmpar driven directly).
TEST("eltfcn: elementwise add/sub/mult/div") {
    const double av[4] = {6.0, 8.0, 3.0, 10.0};
    const double bv[4] = {2.0, 4.0, 3.0, 5.0};
    double cv[4];
    eltfcn(ELT_ADD, av, bv, 4, cv);
    CHECK(cv[0] == 8.0 && cv[3] == 15.0);
    eltfcn(ELT_SUB, av, bv, 4, cv);
    CHECK(cv[0] == 4.0 && cv[1] == 4.0);
    eltfcn(ELT_MULT, av, bv, 4, cv);
    CHECK(cv[2] == 9.0 && cv[3] == 50.0);
    eltfcn(ELT_DIV, av, bv, 4, cv);
    CHECK(cv[0] == 3.0 && cv[1] == 2.0 && cv[3] == 2.0);
    // In-place aliasing (as prtfct uses it).
    double x[3] = {1.0, 2.0, 3.0};
    const double y[3] = {10.0, 20.0, 30.0};
    eltfcn(ELT_ADD, x, y, 3, x);
    CHECK(x[0] == 11.0 && x[2] == 33.0);
}

TEST("devlpl: Horner polynomial A(1)+A(2)X+...+A(N)X^(N-1)") {
    const double a[3] = {1.0, 2.0, 3.0};   // 1 + 2x + 3x^2
    CHECK(rclose(devlpl(a, 3, 2.0), 17.0, 1e-15));   // 1+4+12
    CHECK(rclose(devlpl(a, 3, 0.0), 1.0, 1e-15));
    CHECK(rclose(devlpl(a, 1, 5.0), 1.0, 1e-15));    // constant term only
}

TEST("cumnor: cumulative normal CDF + complement") {
    double cum, ccum;
    cumnor(-0.5, cum, ccum);
    CHECK(rclose(cum, 3.0853753872598694e-01, 1e-14));
    CHECK(rclose(ccum, 6.9146246127401301e-01, 1e-14));
    cumnor(3.0, cum, ccum);
    CHECK(rclose(cum, 9.9865010196836990e-01, 1e-14));
    CHECK(rclose(ccum, 1.3498980316300946e-03, 1e-14));
    cumnor(1.959963984540054, cum, ccum);   // z_0.975
    CHECK(rclose(cum, 0.975, 1e-13));
    CHECK(rclose(ccum, 0.025, 1e-12));
}

TEST("dinvnr: inverse normal CDF (CI critical values)") {
    CHECK(rclose(dinvnr(0.75, 0.25), 6.7448975019608159e-01, 1e-13));
    CHECK(rclose(dinvnr(0.90, 0.10), 1.2815515655446006e+00, 1e-13));
    CHECK(rclose(dinvnr(0.95, 0.05), 1.6448536269514722e+00, 1e-13));
    CHECK(rclose(dinvnr(0.975, 0.025), 1.9599639845400538e+00, 1e-13));
    CHECK(rclose(dinvnr(0.99, 0.01), 2.3263478740408408e+00, 1e-13));
    CHECK(rclose(dinvnr(0.995, 0.005), 2.5758293035489004e+00, 1e-13));
    // Lower-tail sign symmetry.
    CHECK(rclose(dinvnr(0.025, 0.975), -1.9599639845400543e+00, 1e-13));
    CHECK(rclose(dinvnr(0.001, 0.999), -3.0902323061678136e+00, 1e-13));
    // P=0.5 -> 0 (Newton noise ~6.6e-17).
    CHECK(std::fabs(dinvnr(0.5, 0.5)) < 1e-14);
}

// ---- chsppf (chi-squared PPF for the automdl AIC-test family). ---------------
// Golden values from ref_chsppf.f (oracle chsppf.f driven directly). Bisection
// stops at 1e-10, so a 1e-9 tolerance is the meaningful budget.
TEST("chsppf: chi-squared percent-point function") {
    // P=0.90 across df 1/2/6/7.
    CHECK(rclose(chsppf(0.90, 1), 2.7055434551940660e+00, 1e-9));
    CHECK(rclose(chsppf(0.90, 2), 4.6051701871329049e+00, 1e-9));
    CHECK(rclose(chsppf(0.90, 6), 1.0644640676292067e+01, 1e-9));
    CHECK(rclose(chsppf(0.90, 7), 1.2017036624602387e+01, 1e-9));
    // P=0.95 (the classic table column).
    CHECK(rclose(chsppf(0.95, 1), 3.8414588210417624e+00, 1e-9));
    CHECK(rclose(chsppf(0.95, 2), 5.9914645503216963e+00, 1e-9));
    CHECK(rclose(chsppf(0.95, 6), 1.2591587245946918e+01, 1e-9));
    CHECK(rclose(chsppf(0.95, 7), 1.4067140452336819e+01, 1e-9));
    // P=0.99.
    CHECK(rclose(chsppf(0.99, 1), 6.6348966058495371e+00, 1e-9));
    CHECK(rclose(chsppf(0.99, 2), 9.2103403780271869e+00, 1e-9));
    CHECK(rclose(chsppf(0.99, 6), 1.6811893836705174e+01, 1e-9));
    CHECK(rclose(chsppf(0.99, 7), 1.8475306917046829e+01, 1e-9));
    // Error paths return 0.0 (p out of range, nu<1).
    CHECK(chsppf(1.0, 3) == 0.0);
    CHECK(chsppf(-0.1, 3) == 0.0);
    CHECK(chsppf(0.95, 0) == 0.0);
}

// ---- sumf / smeadl (series sum + mean-deletion for iddiff/amdid). ------------
// Exact by construction (integer-valued inputs; left-to-right accumulation).
TEST("sumf: 1-based inclusive range sum") {
    const double x[5] = {2.0, 4.0, 6.0, 8.0, 10.0};
    CHECK(sumf(x, 1, 5) == 30.0);
    CHECK(sumf(x, 2, 4) == 18.0);
    CHECK(sumf(x, 3, 3) == 6.0);
}

TEST("smeadl: mean-deletion over a range") {
    double x[5] = {2.0, 4.0, 6.0, 8.0, 10.0};
    double xmean = 0.0;
    smeadl(x, 1, 5, 5, xmean);
    CHECK(xmean == 6.0);
    CHECK(x[0] == -4.0 && x[1] == -2.0 && x[2] == 0.0 && x[3] == 2.0 &&
          x[4] == 4.0);
    // Sub-range with the oracle's separately-supplied divisor (n != count).
    double y[5] = {1.0, 3.0, 5.0, 7.0, 9.0};
    double ym = 0.0;
    smeadl(y, 2, 4, 3, ym);          // sum 3+5+7=15, /3 = 5
    CHECK(ym == 5.0);
    CHECK(y[1] == -2.0 && y[2] == 0.0 && y[3] == 2.0);
    CHECK(y[0] == 1.0 && y[4] == 9.0);   // outside range untouched
}

// ---- outlier-identification leaves (shlsrt/medabs/makotl/dppdi/ttest). -------
// Numeric leaves of idotlr.f's automatic outlier scan. Golden values from
// ref_outlier.f (leaves driven directly on small fixed inputs).
TEST("shlsrt: ascending shell sort") {
    double v[8] = {3, -1, 4, -1, 5, -9, 2, -6};
    shlsrt(8, v);
    for (int i = 1; i < 8; ++i) CHECK(v[i - 1] <= v[i]);
    CHECK(v[0] == -9 && v[7] == 5);
    double one[1] = {42};
    shlsrt(1, one);
    CHECK(one[0] == 42);
}

TEST("medabs: median of absolute values") {
    const double s[8] = {3, -1, 4, -1, 5, -9, 2, -6};
    double m = 0;
    medabs(s, 8, m);
    CHECK(rclose(m, 3.5, 1e-14));          // even n: mean of two central
    medabs(s, 5, m);
    CHECK(rclose(m, 3.0, 1e-14));          // odd n: central order statistic
}

TEST("makotl: AO/LS/TC outlier regressor construction") {
    // AO+LS+TC at t0=3, nr=5, tcalfa=0.7 -> interleaved [AO,LS,TC] per time.
    const int all3[3] = {1, 1, 1};
    double ov[15] = {0};
    int notlr = 0;
    makotl(3, 5, all3, ov, notlr, 0.7, 12);
    CHECK(notlr == 3);
    const double exp[15] = {0, -1, 0,  0, -1, 0,  1, 0, 1,
                            0, 0, 0.7, 0, 0, 0.49};
    for (int i = 0; i < 15; ++i) CHECK(rclose(ov[i], exp[i], 1e-14));

    // AO only.
    const int aoonly[3] = {1, 0, 0};
    double ao[5] = {9, 9, 9, 9, 9};
    makotl(3, 5, aoonly, ao, notlr, 0.7, 12);
    CHECK(notlr == 1);
    CHECK(ao[0] == 0 && ao[1] == 0 && ao[2] == 1 && ao[3] == 0 && ao[4] == 0);

    // LS only: -1 before t0, 0 from t0 on.
    const int lsonly[3] = {0, 1, 0};
    double ls[5] = {9, 9, 9, 9, 9};
    makotl(3, 5, lsonly, ls, notlr, 0.7, 12);
    CHECK(ls[0] == -1 && ls[1] == -1 && ls[2] == 0 && ls[3] == 0 && ls[4] == 0);
}

TEST("dppdi: packed determinant + inverse of a Cholesky factor") {
    // A = [[4,2],[2,3]] packed upper = {4,2,3}.
    double ap[3] = {4, 2, 3};
    int info = 0;
    dppfa(ap, 2, info);
    CHECK(info == 0);
    double det[2] = {0, 0};
    dppdi(ap, 2, det, 11);
    CHECK(rclose(ap[0], 0.375, 1e-14));    // inverse packed upper
    CHECK(rclose(ap[1], -0.25, 1e-14));
    CHECK(rclose(ap[2], 0.5, 1e-14));
    CHECK(rclose(det[0], 8.0, 1e-14));     // det = 8 * 10^0
    CHECK(rclose(det[1], 0.0, 1e-14));
}

TEST("ttest: proportional outlier t-statistics (augmented Cholesky)") {
    // X = const(6), y = [2,3,1,8,4,5]; [X:y] row-major, ncxy=2.
    const int nspobs = 6, ncxy = 2;
    double xy[12] = {1, 2, 1, 3, 1, 1, 1, 8, 1, 4, 1, 5};
    double chlxpx[3] = {0};
    xprmx(xy, nspobs, ncxy, ncxy, chlxpx);
    int info = 0;
    dppfa(chlxpx, ncxy, info);
    CHECK(info == 0);

    double ov[3 * 6] = {0};
    int notlr = 0, mxcol[3] = {0};
    double propt[3] = {0};
    bool snglr[3] = {false};

    // AO only at t0=3.
    const int aoonly[3] = {1, 0, 0};
    makotl(3, nspobs, aoonly, ov, notlr, 0.7, 12);
    ttest(xy, nspobs, ncxy, chlxpx, ov, aoonly, mxcol, propt, snglr);
    CHECK(rclose(propt[prm::AO - 1], -3.10376115919594, 1e-12));

    // AO+LS+TC at t0=4; check values and the |t| ranking (TC > AO > LS).
    const int all3[3] = {1, 1, 1};
    makotl(4, nspobs, all3, ov, notlr, 0.7, 12);
    ttest(xy, nspobs, ncxy, chlxpx, ov, all3, mxcol, propt, snglr);
    CHECK(rclose(propt[prm::AO - 1], 4.56435464587638, 1e-12));
    CHECK(rclose(propt[prm::LS - 1], 4.49073119510250, 1e-12));
    CHECK(rclose(propt[prm::TC - 1], 5.03237170472164, 1e-12));
    CHECK(mxcol[0] == prm::TC && mxcol[1] == prm::AO && mxcol[2] == prm::LS);
}

TEST("coladd: open regression columns, shifting existing data right") {
    // 2 rows x 2 cols row-major: row1=[1,2], row2=[3,4]. Insert 1 col at col 2.
    double xy[6] = {1, 2, 3, 4, 0, 0};
    int ncxy = 2;
    coladd(2, 2, 2, 6, xy, ncxy);
    CHECK(ncxy == 3);
    // Existing columns: col1 stays, col2 shifts to col3. New col2 is a gap.
    CHECK(xy[0] == 1);   // row1 col1
    CHECK(xy[2] == 2);   // row1 col3 (was col2)
    CHECK(xy[3] == 3);   // row2 col1
    CHECK(xy[5] == 4);   // row2 col3 (was col2)

    // Insert 2 cols at the front of a 3x1 matrix: col1 -> col3.
    double xy2[9] = {7, 8, 9, 0, 0, 0, 0, 0, 0};
    int ncxy2 = 1;
    coladd(1, 2, 3, 9, xy2, ncxy2);
    CHECK(ncxy2 == 3);
    CHECK(xy2[2] == 7 && xy2[5] == 8 && xy2[8] == 9);  // each row's datum at col3
}

TEST("setcv: default outlier critical value from span length") {
    // Golden from ref_setcv.f (setcv/setcvl/ppnd/lassol driven directly).
    CHECK(rclose(setcv(132, 0.5), 3.1278891070172814, 1e-12));
    CHECK(rclose(setcv(60, 0.5), 2.8814980658151446, 1e-12));
}

int main() { return mt::run_all(); }
