// test_x11b.cpp -- parity unit tests for the X-11 Tier 2 seasonal moving-average
// chain (core/src/x11/x11seas.*: fis / vsfc / vsfa / vsfb) and the Tier 3
// extreme-value adjustment leaves (core/src/x11/x11xtrm.*: rho2 / wtxtrm /
// sdxtrm / xtrm / replac / weight / vtest / entsch / trbias).
//
// Golden values are either (a) hand-computed from the exact Fortran formula, (b)
// an independent numeric reference computed off-line (marked "ref"), or (c) an
// INVARIANT of the algorithm (seasonal/trend MAs reproduce a constant because
// their weights sum to 1; a lone outlier is zeroed; centered seasonals average
// to 1). None are tautologies: each fails on any coefficient / indexing error.
//
// Seasonal-MA weight-table provenance: the 3x3 / 3x5 end formulas, the 3x9 (w9)
// and 3x15 (w15) end-weight tables, and the preliminary 2x12 / 2x4 trend weights
// are the standard X-11 tables (Ladiray & Quenneville 2001, "Seasonal Adjustment
// with the X-11 Method"; Shiskin-Young-Musgrave 1967, Census Technical Paper 15)
// transcribed verbatim from the vendored Fortran DATA blocks.
#include "microtest.hpp"
#include "x11/x11seas.hpp"
#include "x11/x11xtrm.hpp"
#include "x11/x11drv.hpp"     // forcst (Tier-4/5 driver, pure-numeric leaf)
#include "x11/x11parts.hpp"   // chktrn (Tier-6 spine leaf)
#include "common/x13context.hpp"
#include "gen/notset.hpp"  // prm::DNOTST

#include <cmath>
#include <memory>

using namespace x13;

namespace {
bool close(double a, double b, double tol) { return std::fabs(a - b) <= tol; }
}  // namespace

// ===========================================================================
// Tier 2 -- fis / vsfc / vsfa / vsfb
// ===========================================================================

TEST("fis: number-of-years I/S correction (lookup + closed form)") {
    double cs = -1.0;
    // n<6: the published 8-entry lookup c(N-1) / c(N+3).
    CHECK_EQ(fis(cs, 2), 1.0);
    CHECK_EQ(cs, 1.0);
    CHECK_EQ(fis(cs, 5), 1.01383);
    CHECK_EQ(cs, 1.30095);
    // n>=6: closed-form asymptotics (ref values).
    CHECK(close(fis(cs, 6), 1.0033501123863304, 1e-15));
    CHECK(close(cs, 1.2247450614776338, 1e-15));
    CHECK(close(fis(cs, 12), 1.0016722550719506, 1e-15));
    CHECK(close(cs, 1.1010205912439974, 1e-15));
}

TEST("vsfc: 2xNyr centering -- constant->1 invariant + reference window") {
    // Constant seasonals center to 1.0 (mult): the 2xNyr MA of a constant is the
    // same constant, so sts <- sts / MA == 1 everywhere.
    const int N = 12;  // ny=4, 3 years
    double sts[N], temp[N];
    const int lter[4] = {1, 1, 1, 1};
    for (int i = 0; i < N; ++i) sts[i] = 7.0;
    vsfc(sts, 1, N, 4, lter, temp, /*muladd=*/0);
    for (int i = 0; i < N; ++i) CHECK(close(sts[i], 1.0, 1e-12));

    // Reference window (ny=4, lter all !=5, sts=[1,2,3,4,2,3,4,5]): the 2x4 MA
    // is filled over the centered range and the k=Nyr/2=2 end terms are copied
    // from the nearest MA value; sts <- sts / MA.
    double s2[8] = {1, 2, 3, 4, 2, 3, 4, 5};
    double t2[8];
    vsfc(s2, 1, 8, 4, lter, t2, 0);
    const double ref[8] = {0.38095238095238093, 0.7619047619047619,
                           1.1428571428571428,  1.391304347826087,
                           0.64,                0.8888888888888888,
                           1.1851851851851851,  1.4814814814814814};
    for (int i = 0; i < 8; ++i) CHECK(close(s2[i], ref[i], 1e-12));

    // Additive centering: constant -> 0.0 (subtract branch).
    for (int i = 0; i < N; ++i) sts[i] = 7.0;
    vsfc(sts, 1, N, 4, lter, temp, /*muladd=*/1);
    for (int i = 0; i < N; ++i) CHECK(close(sts[i], 0.0, 1e-12));
}

TEST("vsfa: constant SI ratios -> zero I/S ratios, global MSR sentinel") {
    // Constant stsi: the 7-term seasonal estimate equals the constant, so the
    // irregular estimate is 1 everywhere and every accumulated |diff| is 0.
    // Rati[m], Rati[m+ny] collapse to 0; Rati[m+2ny] keeps its 999.99 sentinel
    // (S-bar==0 short-circuits the division); the global Ratis stays 999.99.
    const int ny = 4;
    const int N = 24;  // 6 years
    double stsi[N];
    for (int i = 0; i < N; ++i) stsi[i] = 2.0;
    double rati[3 * ny];
    for (int i = 0; i < 3 * ny; ++i) rati[i] = -1.0;
    double ratis = -1.0;
    vsfa(stsi, 1, N, ny, /*muladd=*/0, /*psuadd=*/false, rati, ratis);
    for (int m = 0; m < ny; ++m) {
        CHECK_EQ(rati[m], 0.0);              // I ratios
        CHECK_EQ(rati[m + ny], 0.0);         // S ratios
        CHECK_EQ(rati[m + 2 * ny], 999.99);  // I/S sentinel
    }
    CHECK_EQ(ratis, 999.99);
}

TEST("vsfb: every seasonal filter reproduces a constant (centered to 1)") {
    // ny=4, 20 years (k=20 per period) so each Mtype takes its real averag path
    // (3x15 needs k>=20). A constant SI series must map to seasonal factors of
    // exactly 1 for every filter -- this pins the weight normalization (sum==1)
    // of the 3x3/3x5 inline end formulas, the 3x9/3x15 endsf tables, the stable
    // average, the 3-term filter, and the vsfc centering, all at once.
    const int ny = 4;
    const int N = 80;
    // lter value -> seasonal filter: 1=3x3, 2=3x5, 3=3x9, 4=3x15, 5=stable,
    // 7=3-term.
    const int lters[6] = {1, 2, 3, 4, 5, 7};
    for (int f = 0; f < 6; ++f) {
        double sts[N], stsi[N], temp[N];
        int lter[4];
        for (int i = 0; i < 4; ++i) lter[i] = lters[f];
        for (int i = 0; i < N; ++i) stsi[i] = 3.0;
        vsfb(sts, stsi, 1, N, ny, /*lterm=*/6, lter, /*ksect=*/0,
             /*shrtsf=*/false, temp, /*muladd=*/0);
        for (int i = 0; i < N; ++i) CHECK(close(sts[i], 1.0, 1e-9));
    }
}

// ===========================================================================
// Tier 3 -- rho2 / wtxtrm / sdxtrm / xtrm / replac / weight / vtest / entsch /
//           trbias
// ===========================================================================

TEST("rho2: Tukey biweight rho with saturation") {
    CHECK(close(rho2(1.0), 0.9942, 1e-12));   // 0.9249+0.0812-0.0119
    CHECK(close(rho2(2.0), 4.2372, 1e-12));   // 0.9249*4+0.0812*16-0.0119*64
    CHECK_EQ(rho2(3.0), 6.502);               // |u|>2.798 saturates
    CHECK_EQ(rho2(-3.0), 6.502);              // even in u
}

TEST("wtxtrm: graduated extreme weight across the sigma band") {
    const double su = 2.5, sl = 1.5, std = 1.0, xbar = 0.0;
    // |dev| <= sigml: weight unchanged (returns lstwt).
    CHECK_EQ(wtxtrm(1.0, xbar, std, su, sl, 2, 1.0), 1.0);
    // sigml < |dev| <= sigmu, second pass: linear grade (su-t)/(su-sl).
    CHECK(close(wtxtrm(2.0, xbar, std, su, sl, 2, 1.0), 0.5, 1e-12));
    // In the band on the FIRST pass: no grading -> unchanged.
    CHECK_EQ(wtxtrm(2.0, xbar, std, su, sl, 1, 1.0), 1.0);
    // |dev| > sigmu, first pass: hard zero.
    CHECK_EQ(wtxtrm(3.0, xbar, std, su, sl, 1, 1.0), 0.0);
    // |dev| > sigmu, second pass, prior weight > 0: temporary -1.
    CHECK_EQ(wtxtrm(3.0, xbar, std, su, sl, 2, 1.0), -1.0);
    // |dev| > sigmu, second pass, prior weight already 0: stays 0.
    CHECK_EQ(wtxtrm(3.0, xbar, std, su, sl, 2, 0.0), 0.0);
}

TEST("sdxtrm: five-year RMS (imad=0) and MAD std error (imad=1)") {
    // xi = [1,2,3,4,5] (1-based), xbar=3, nsp=1, first pass (no omission).
    const double xi[5] = {1, 2, 3, 4, 5};
    const bool csig[1] = {false};
    const double stwt[5] = {1, 1, 1, 1, 1};
    // imad=0: sqrt(sum((xi-3)^2)/5) = sqrt(10/5) = sqrt(2).
    double s0 = sdxtrm(xi, 3.0, 1, 5, 1, 0, 1, 1, true, stwt, csig, 0);
    CHECK(close(s0, std::sqrt(2.0), 1e-12));
    // imad=1: MAD = median(|xi-3|) / 0.6745; |dev|=[2,1,0,1,2] -> median 1.
    double s1 = sdxtrm(xi, 3.0, 1, 5, 1, 1, 1, 1, true, stwt, csig, 0);
    CHECK(close(s1, 1.0 / 0.6745, 1e-12));
}

TEST("xtrm: standard five-year pass zeroes a lone outlier, keeps the rest") {
    // ny=12, 2-year span [1,24] (the <5-year single-window branch). An otherwise
    // constant irregular (xi==1, xbar==1) with one spike xi(11)=5: pass 1 gives
    // it |dev|/sigma = 4/0.8165 = 4.9 > sigmu -> weight 0; pass 2's sigma over
    // the non-extreme values is 0 -> no further change. Every other weight is 1.
    const int N = 24;
    double xi[N];
    for (int i = 0; i < N; ++i) xi[i] = 1.0;
    xi[10] = 5.0;  // Fortran Xi(11)
    double stwt[N];
    double stdper[12];
    double stdev[86];  // PYRS+1
    const bool csig[12] = {false, false, false, false, false, false,
                           false, false, false, false, false, false};
    xtrm(xi, 1, N, 1, N, /*ny=*/12, /*muladd=*/0, /*ksdev=*/0, /*imad=*/0,
         /*sigmu=*/2.5, /*sigml=*/1.5, /*lsp=*/1, stwt, stdper, stdev, csig);
    for (int i = 0; i < N; ++i) {
        if (i == 10)
            CHECK_EQ(stwt[i], 0.0);
        else
            CHECK_EQ(stwt[i], 1.0);
    }
}

TEST("replac: reweighted-MA replacement of an extreme (nm=1)") {
    // Whole-series irregular (nm=1). A single zero-weight spike at index 4 is
    // replaced by the mean of its four nearest full-weight neighbours (each 10):
    //   X(4) = (0*50 + 10 + 10 + 10 + 10) / (4 + 0) = 10.
    double x[7] = {10, 10, 10, 50, 10, 10, 10};
    double y[7] = {0, 0, 0, 0, 0, 0, 0};
    const double w[7] = {1, 1, 1, 0, 1, 1, 1};
    replac(x, y, w, 1, 7, 1);
    CHECK_EQ(x[3], 10.0);
    // Untouched full-weight values stay put.
    CHECK_EQ(x[0], 10.0);
    CHECK_EQ(x[6], 10.0);

    // Fewer than four neighbours AND nm==1: no replacement (the average-of-month
    // fallback only applies for nm>1), so the extreme survives unchanged.
    double x2[3] = {50, 10, 10};
    double y2[3] = {0, 0, 0};
    const double w2[3] = {0, 1, 1};
    replac(x2, y2, w2, 1, 3, 1);
    CHECK_EQ(x2[0], 50.0);
}

TEST("weight: preliminary trend MA reproduces constant and linear") {
    // Monthly (mq=1): the centered 2x12 MA fills [i1+6, i2-6]; the six Musgrave
    // end weights on each side extend it. Both the 25-term centre weights and
    // the end tables sum to 1 and reproduce a linear trend.
    const int N = 40;
    double a[N], b[N];
    for (int i = 0; i < N; ++i) a[i] = 5.0;
    weight(a, b, 1, N, 1);
    for (int i = 7; i <= 34; ++i) CHECK(close(b[i - 1], 5.0, 1e-5));  // filled
    CHECK_EQ(b[0], 0.0);   // outside the filled span
    CHECK_EQ(b[39], 0.0);

    for (int i = 1; i <= N; ++i) a[i - 1] = 2.0 + 0.3 * i;
    weight(a, b, 1, N, 1);
    for (int i = 7; i <= 34; ++i) CHECK(close(b[i - 1], 2.0 + 0.3 * i, 1e-5));

    // Quarterly (mq=2): the 2x4 MA fills [i1+2, i2-2].
    const int Nq = 20;
    double aq[Nq], bq[Nq];
    for (int i = 0; i < Nq; ++i) aq[i] = 4.0;
    weight(aq, bq, 1, Nq, 2);
    for (int i = 3; i <= 18; ++i) CHECK(close(bq[i - 1], 4.0, 1e-5));
}

TEST("vtest: Cochran heteroskedasticity flag") {
    const int ny = 4;
    const int N = 24;  // 6 obs per period, nmin=5 -> t4(5)=0.5895
    // Homoskedastic: every value 2.0 -> each period variance equal, share 0.25.
    double xh[N];
    for (int i = 0; i < N; ++i) xh[i] = 2.0;
    int flag = -1;
    vtest(xh, flag, 1, N, ny, /*muladd=*/0);
    CHECK_EQ(flag, 0);
    // Heteroskedastic: period 1 has 3x the deviation (variance 9 vs 1), share
    // 9/12 = 0.75 >= 0.5895 -> flagged.
    double xe[N];
    for (int i = 0; i < N; ++i) xe[i] = 2.0;
    for (int p = 1; p <= N; p += ny) xe[p - 1] = 4.0;  // period-1 positions
    vtest(xe, flag, 1, N, ny, 0);
    CHECK_EQ(flag, 1);
}

TEST("entsch: Ksdev (ken,ker) -> (ken1,ker1) branch table") {
    int ken1 = -1, ker1 = -1;
    entsch(0, 0, ken1, ker1, /*iv=*/5);  // k=1
    CHECK_EQ(ken1, 5);
    CHECK_EQ(ker1, 2);
    entsch(1, 0, ken1, ker1, 5);  // k=2
    CHECK_EQ(ken1, 1);
    CHECK_EQ(ker1, 0);
    entsch(1, 1, ken1, ker1, 3);  // k=3
    CHECK_EQ(ken1, 3);
    CHECK_EQ(ker1, 3);
    entsch(2, 1, ken1, ker1, 5);  // k=4
    CHECK_EQ(ken1, 1);
    CHECK_EQ(ker1, 2);
    entsch(2, 2, ken1, ker1, 5);  // k=5 -> no-op
    CHECK_EQ(ken1, 0);
    CHECK_EQ(ker1, 0);
}

TEST("trbias: log-additive trend bias correction") {
    // Constant seasonals (Henderson-smoothed to themselves) and a zero irregular
    // give sig=exp(0)=1, so stc <- stc * sts. With sts==2 and stc==1: stc->2.
    const int N = 30;  // ny=12 -> 23-term filter, span >= 23
    double stc[N], sts[N], sti[N], bias[N];
    for (int i = 0; i < N; ++i) { stc[i] = 1.0; sts[i] = 2.0; sti[i] = 0.0; }
    trbias(stc, sts, sti, 1, N, bias, /*ny=*/12, /*tru7hn=*/false);
    for (int i = 0; i < N; ++i) {
        CHECK(close(bias[i], 2.0, 1e-9));
        CHECK(close(stc[i], 2.0, 1e-9));
    }
    // Non-zero irregular: sig = exp(sum(sti^2)/(2*(L2-L1+1))). sti==0.1 over 30
    // points -> sig=exp(0.3/60), bias = sig*2 (ref value).
    for (int i = 0; i < N; ++i) { stc[i] = 1.0; sts[i] = 2.0; sti[i] = 0.1; }
    trbias(stc, sts, sti, 1, N, bias, 12, false);
    for (int i = 0; i < N; ++i) {
        CHECK(close(bias[i], 2.010025041718802, 1e-9));
        CHECK(close(stc[i], 2.010025041718802, 1e-9));
    }
}

// ===========================================================================
// Tier 4/5 driver -- forcst (pure-numeric seasonal forecast/backcast)
// ===========================================================================

TEST("forcst: Iorder=1, R=1 -- linear per-season extrapolation (hand values)") {
    // nyr=2, observed sts[5..8] (1-based) = 1,2,3,5. p1=idx5,7 ; p2=idx6,8.
    // Iorder=1, Wt=1, R=1 (dpeq -> w=Wt=1). Forecast [9,10], backcast [4,3].
    double s[16] = {0};
    s[5 - 1] = 1.0; s[6 - 1] = 2.0; s[7 - 1] = 3.0; s[8 - 1] = 5.0;
    forcst(s, /*ib=*/5, /*ie=*/8, /*ke=*/10, /*nyr=*/2, /*iorder=*/1,
           /*wt=*/1.0, /*r=*/1.0);
    // fc: s9 = s7+(s7-s5)=5 ; s10 = s8+(s8-s6)=8
    CHECK(close(s[9 - 1], 5.0, 1e-12));
    CHECK(close(s[10 - 1], 8.0, 1e-12));
    // bc: s4 = s6+(s6-s8)=-1 ; s3 = s5+(s5-s7)=-1
    CHECK(close(s[4 - 1], -1.0, 1e-12));
    CHECK(close(s[3 - 1], -1.0, 1e-12));
}

TEST("forcst: Iorder=1 collapses w to Wt regardless of R") {
    // Iorder=1 -> w = Wt*(R-1)/(R^1-1) = Wt. With Wt=0.5, R=3:
    // s9 = s7 + 0.5*(s7-s5) = 3 + 0.5*(3-1) = 4.
    double s[12] = {0};
    s[5 - 1] = 1.0; s[6 - 1] = 2.0; s[7 - 1] = 3.0; s[8 - 1] = 5.0;
    forcst(s, 5, 8, 9, /*nyr=*/2, /*iorder=*/1, /*wt=*/0.5, /*r=*/3.0);
    CHECK(close(s[9 - 1], 4.0, 1e-12));
}

TEST("forcst: Iorder=2, R=2 -- exercises dpow_ri + the difference k-loop") {
    // nyr=1 (annual), observed s[4..7]=0,1,4,9. Iorder=2, Wt=1, R=2 ->
    // w = 1*(2-1)/(2^2-1) = 1/3. dpow_ri(2,1)=2, dpow_ri(2,0)=1.
    // fc s8 = s7 + (1/3)[2*(s7-s6) + (s6-s5)] = 9 + (1/3)(10+3) = 40/3.
    // bc s3 = s4 + (1/3)[2*(s4-s5) + (s5-s6)] = 0 + (1/3)(-2-3) = -5/3.
    double s[12] = {0};
    s[4 - 1] = 0.0; s[5 - 1] = 1.0; s[6 - 1] = 4.0; s[7 - 1] = 9.0;
    forcst(s, /*ib=*/4, /*ie=*/7, /*ke=*/8, /*nyr=*/1, /*iorder=*/2,
           /*wt=*/1.0, /*r=*/2.0);
    CHECK(close(s[8 - 1], 40.0 / 3.0, 1e-12));
    CHECK(close(s[3 - 1], -5.0 / 3.0, 1e-12));
}

// ===========================================================================
// Tier 6 spine leaf -- chktrn (multiplicative trend-positivity check/repair)
// ===========================================================================

namespace {
// Minimal context for chktrn: it reads only x11ptr (span pointers) and
// extend.nfcst. Layout used below: backcasts 1..2, observed 3..8, forecasts 9..10.
// Heap-allocated -- X13Context carries large farray members (stack would overflow).
std::unique_ptr<X13Context> make_chktrn_ctx(int nfcst) {
    auto ctx = std::make_unique<X13Context>();
    ctx->x11ptr.pos1bk = 1;
    ctx->x11ptr.pos1ob = 3;
    ctx->x11ptr.posfob = 8;
    ctx->x11ptr.posffc = 10;
    ctx->extend.nfcst = nfcst;
    return ctx;
}
}  // namespace

TEST("chktrn: negatives replaced by neighbour mean / nearest end value; oktrn false") {
    auto ctx = make_chktrn_ctx(2);
    // 1-based; negatives at 3 (first obs -> nearest after), 5 (interior -> mean),
    // 8 (last obs -> nearest before).
    double stc[10] = {10, 10, -1, 20, -5, 40, 30, -2, 10, 10};
    bool tstfct = false;
    bool oktrn = chktrn(*ctx, stc, tstfct);
    CHECK_EQ(oktrn, false);          // negatives over [pos1ob,last=posfob]
    CHECK_EQ(stc[3 - 1], 20.0);      // first obs: before ran off -> Stc(after)=20
    CHECK_EQ(stc[5 - 1], 30.0);      // interior: (Stc(4)+Stc(6))/2=(20+40)/2
    CHECK_EQ(stc[8 - 1], 30.0);      // last obs: after ran off -> Stc(before)=30
    // Positive values untouched.
    CHECK_EQ(stc[4 - 1], 20.0);
    CHECK_EQ(stc[6 - 1], 40.0);
}

TEST("chktrn: all-positive trend is a no-op that resets tstfct; oktrn true") {
    auto ctx = make_chktrn_ctx(2);
    double stc[10] = {10, 10, 20, 20, 30, 40, 30, 25, 10, 10};
    double before[10];
    for (int i = 0; i < 10; ++i) before[i] = stc[i];
    bool tstfct = true;
    bool oktrn = chktrn(*ctx, stc, tstfct);
    CHECK_EQ(oktrn, true);
    CHECK_EQ(tstfct, false);         // reset on the all-positive early return
    for (int i = 0; i < 10; ++i) CHECK_EQ(stc[i], before[i]);  // unchanged
}

TEST("chktrn: tstfct+nfcst extends the oktrn span to the forecasts") {
    // Observed span clean, one negative in the forecast tail (index 9). With
    // tstfct=false the span is [pos1ob,posfob] -> oktrn stays true; with tstfct
    // and nfcst>0 the span reaches posffc so the forecast negative flips it false.
    // Either way the negative is repaired (nearest positive = last obs, index 8).
    double base[10] = {10, 10, 20, 20, 30, 40, 30, 30, -1, 10};

    auto c0 = make_chktrn_ctx(2);
    double s0[10];
    for (int i = 0; i < 10; ++i) s0[i] = base[i];
    bool t0 = false;
    CHECK_EQ(chktrn(*c0, s0, t0), true);   // span excludes forecasts
    CHECK_EQ(s0[9 - 1], 30.0);             // repaired from Stc(before)=Stc(8)

    auto c1 = make_chktrn_ctx(2);
    double s1[10];
    for (int i = 0; i < 10; ++i) s1[i] = base[i];
    bool t1 = true;
    CHECK_EQ(chktrn(*c1, s1, t1), false);  // span reaches posffc -> negative seen
    CHECK_EQ(s1[9 - 1], 30.0);
}

TEST("chktrn: adjacent negatives -- repair is sequential/in-place, not snapshot") {
    // Negatives at 5 and 6. Processing ascends and mutates Stc in place:
    //   i=5: before=Stc(4)=20; after skips the still-negative Stc(6) to Stc(7)=80
    //        -> Stc(5)=(80+20)/2=50.
    //   i=6: before finds the JUST-REPAIRED Stc(5)=50; after=Stc(7)=80
    //        -> Stc(6)=(80+50)/2=65.
    // A snapshot-then-fill implementation would instead skip the original negative
    // Stc(5) and use Stc(4)=20 -> Stc(6)=50. The 65 pins the in-place semantics.
    auto ctx = make_chktrn_ctx(2);
    double stc[10] = {10, 10, 10, 20, -5, -9, 80, 30, 10, 10};
    bool tstfct = false;
    CHECK_EQ(chktrn(*ctx, stc, tstfct), false);
    CHECK_EQ(stc[5 - 1], 50.0);
    CHECK_EQ(stc[6 - 1], 65.0);
}

TEST("chktrn: exact 0.0 counts as non-positive (<=0 boundary)") {
    auto ctx = make_chktrn_ctx(2);
    double stc[10] = {10, 10, 20, 0.0, 40, 30, 25, 30, 10, 10};
    bool tstfct = false;
    CHECK_EQ(chktrn(*ctx, stc, tstfct), false);  // a zero flips oktrn
    CHECK_EQ(stc[4 - 1], 30.0);                   // (Stc(3)+Stc(5))/2=(20+40)/2
}

TEST("chktrn: negative outside the core span repairs but leaves oktrn true") {
    // Negative in the backcast region (index 2, < pos1ob). The observed core span
    // is clean, so oktrn stays true even though a repair happens: the before-search
    // runs off pos1ob (NOTSET) and the value is taken from the nearest obs, Stc(3).
    auto ctx = make_chktrn_ctx(2);
    double stc[10] = {10, -7, 20, 30, 40, 30, 25, 30, 10, 10};
    bool tstfct = false;
    CHECK_EQ(chktrn(*ctx, stc, tstfct), true);
    CHECK_EQ(stc[2 - 1], 20.0);  // Stc(after)=Stc(3)
}

int main() { return mt::run_all(); }
