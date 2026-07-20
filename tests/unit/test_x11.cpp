// test_x11.cpp -- parity unit tests for the X-11 Tier 0 + Tier 1 leaf routines
// (core/src/x11/x11filt.*): mode-arithmetic recombine primitives, the M-of-N
// moving-average kernel, and the Henderson trend-filter chain (symmetric weights
// + Doherty 1993 asymmetric end filters).
//
// Golden values are either (a) hand-computed from the exact rational arithmetic
// of the Fortran formula, or (b) the published Henderson filter weight tables.
// Where the golden is an INVARIANT of the algorithm (Henderson weights sum to 1;
// symmetric filters reproduce linear trends; Doherty end filters reproduce a
// constant), the invariant is asserted directly -- these are not tautologies:
// they fail on any indexing / coefficient error in the port.
#include "microtest.hpp"
#include "x11/x11filt.hpp"
#include "gen/notset.hpp"  // prm::DNOTST

#include <cmath>

using namespace x13;

namespace {
bool close(double a, double b, double tol) { return std::fabs(a - b) <= tol; }
}  // namespace

// ===========================================================================
// Tier 0 -- mode-arithmetic recombine primitives
// ===========================================================================

TEST("divsub: divide (mult) vs subtract (add)") {
    const double a1[3] = {6.0, 8.0, 10.0};
    const double a2[3] = {2.0, 4.0, 5.0};
    double r[3];
    divsub(r, a1, a2, 1, 3, MODE_MULT);
    CHECK_EQ(r[0], 3.0);
    CHECK_EQ(r[1], 2.0);
    CHECK_EQ(r[2], 2.0);
    divsub(r, a1, a2, 1, 3, MODE_ADD);
    CHECK_EQ(r[0], 4.0);
    CHECK_EQ(r[1], 4.0);
    CHECK_EQ(r[2], 5.0);
    // logadd takes the subtract branch (muladd != 0), like additive.
    divsub(r, a1, a2, 2, 2, MODE_LOGADD);
    CHECK_EQ(r[1], 4.0);
}

TEST("addmul: multiply (mult) vs add (add)") {
    const double x[3] = {3.0, 2.0, 2.0};
    const double y[3] = {2.0, 4.0, 5.0};
    double z[3];
    addmul(z, x, y, 1, 3, MODE_MULT);
    CHECK_EQ(z[0], 6.0);
    CHECK_EQ(z[1], 8.0);
    CHECK_EQ(z[2], 10.0);
    addmul(z, x, y, 1, 3, MODE_ADD);
    CHECK_EQ(z[0], 5.0);
    CHECK_EQ(z[1], 6.0);
    CHECK_EQ(z[2], 7.0);
    // divsub then addmul is the inverse round-trip in each mode.
    const double a1[3] = {6.0, 8.0, 10.0};
    const double a2[3] = {2.0, 4.0, 5.0};
    double q[3], back[3];
    divsub(q, a1, a2, 1, 3, MODE_MULT);
    addmul(back, q, a2, 1, 3, MODE_MULT);
    CHECK_EQ(back[0], 6.0);
    CHECK_EQ(back[2], 10.0);
}

TEST("logar / antilg: in-place log and exp over a range") {
    double x[3] = {1.0, std::exp(1.0), std::exp(2.0)};
    logar(x, 1, 3);
    CHECK(close(x[0], 0.0, 1e-12));
    CHECK(close(x[1], 1.0, 1e-12));
    CHECK(close(x[2], 2.0, 1e-12));
    antilg(x, 1, 3);
    CHECK(close(x[0], 1.0, 1e-12));
    CHECK(close(x[1], std::exp(1.0), 1e-12));
    CHECK(close(x[2], std::exp(2.0), 1e-12));
    // Partial range: only [2,2] touched.
    double y[3] = {5.0, std::exp(3.0), 7.0};
    logar(y, 2, 2);
    CHECK_EQ(y[0], 5.0);
    CHECK(close(y[1], 3.0, 1e-12));
    CHECK_EQ(y[2], 7.0);
}

TEST("setmv: reset missing-value code where flagged") {
    double srs[4] = {1.0, 2.0, 3.0, 4.0};
    const bool mv[4] = {false, true, false, true};
    setmv(srs, mv, -999.0, 1, 4);
    CHECK_EQ(srs[0], 1.0);
    CHECK_EQ(srs[1], -999.0);
    CHECK_EQ(srs[2], 3.0);
    CHECK_EQ(srs[3], -999.0);
}

TEST("change: additive difference vs multiplicative percent change") {
    // Additive (muladd==1): plain first difference, gudval ignored.
    const double xa[4] = {10.0, 13.0, 12.0, 20.0};
    const bool g_all[4] = {true, true, true, true};
    double ya[4] = {0, 0, 0, 0};
    change(xa, ya, 2, 4, MODE_ADD, g_all);
    CHECK_EQ(ya[1], 3.0);
    CHECK_EQ(ya[2], -1.0);
    CHECK_EQ(ya[3], 8.0);
    // Multiplicative: percent change, guarded by gudval(i-1); non-good -> DNOTST.
    const double xm[3] = {10.0, 15.0, 12.0};
    const bool gm[3] = {true, false, true};  // gudval(2)==false blanks y(3)
    double ym[3] = {0, 0, 0};
    change(xm, ym, 2, 3, MODE_MULT, gm);
    CHECK(close(ym[1], 0.5, 1e-12));         // (15-10)/10
    CHECK_EQ(ym[2], prm::DNOTST);            // gudval(2) false
}

TEST("divgud: good-obs divide, DNOTST elsewhere") {
    const double a1[3] = {6.0, 8.0, 10.0};
    const double a2[3] = {2.0, 4.0, 5.0};
    const bool g[3] = {true, false, true};
    double r[3];
    divgud(r, a1, a2, 1, 3, g);
    CHECK_EQ(r[0], 3.0);
    CHECK_EQ(r[1], prm::DNOTST);
    CHECK_EQ(r[2], 2.0);
}

TEST("chkzro: clear good-obs flag on non-positive levels") {
    const double ori[3] = {1.0, -1.0, 2.0};
    const double one[3] = {1.0, 1.0, 1.0};
    // iyrt=0, lrndsa=false so sa2/sarnd unused; kfulsm=0.
    bool g[3] = {true, true, true};
    chkzro(ori, one, one, one, one, 1, 3, 0, 0, false, g);
    CHECK(g[0]);
    CHECK(!g[1]);  // ori(2) <= 0
    CHECK(g[2]);
    // kfulsm != 0 makes the (kfulsm==0 && sa>0) clause false -> clears all.
    bool g2[3] = {true, true, true};
    chkzro(one, one, one, one, one, 1, 3, 1, 0, false, g2);
    CHECK(!g2[0]);
    CHECK(!g2[1]);
    CHECK(!g2[2]);
}

// ===========================================================================
// Tier 0 -- moving-average / Henderson weight kernels
// ===========================================================================

TEST("averag: 1x3 and 3x3 moving averages") {
    // 1-of-3 == simple centered 3-term mean.
    const double x[5] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double y[5] = {-1, -1, -1, -1, -1};
    averag(x, y, 1, 5, 1, 3);
    CHECK_EQ(y[0], -1.0);  // boundary untouched
    CHECK_EQ(y[1], 2.0);   // (1+2+3)/3
    CHECK_EQ(y[2], 3.0);   // (2+3+4)/3
    CHECK_EQ(y[3], 4.0);   // (3+4+5)/3
    CHECK_EQ(y[4], -1.0);
    // 3-of-3 on a ramp: centered value equals the centre sample.
    const double x2[7] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
    double y2[7] = {0, 0, 0, 0, 0, 0, 0};
    averag(x2, y2, 1, 7, 3, 3);  // ki=2, k in [3,5]
    CHECK_EQ(y2[3], 4.0);        // 36/9
    CHECK_EQ(y2[2], 3.0);
    CHECK_EQ(y2[4], 5.0);
}

TEST("hender: 13-term and 9-term weights vs published tables") {
    // Published 13-term Henderson half-weights (Henderson, 1916; standard table
    // reproduced e.g. in Ladiray & Quenneville 2001, "Seasonal Adjustment with
    // the X-11 Method"): central..edge = 0.240 0.214 0.147 0.066 0.000 -0.028
    // -0.019. Publication rounds to 3 decimals -> compare at 2e-3.
    double w13[7];
    hender(w13, 13);
    const double pub13[7] = {0.240, 0.214, 0.147, 0.066, 0.000, -0.028, -0.019};
    for (int i = 0; i < 7; ++i) CHECK(close(w13[i], pub13[i], 2e-3));
    // Henderson weights sum to 1 (level preservation) -- exact invariant.
    double s13 = w13[0];
    for (int i = 1; i < 7; ++i) s13 += 2.0 * w13[i];
    CHECK(close(s13, 1.0, 1e-12));

    // Published 9-term half-weights: 0.330 0.267 0.119 -0.010 -0.041.
    double w9[5];
    hender(w9, 9);
    const double pub9[5] = {0.330, 0.267, 0.119, -0.010, -0.041};
    for (int i = 0; i < 5; ++i) CHECK(close(w9[i], pub9[i], 2e-3));
    double s9 = w9[0];
    for (int i = 1; i < 5; ++i) s9 += 2.0 * w9[i];
    CHECK(close(s9, 1.0, 1e-12));
}

TEST("apply: symmetric Henderson reproduces a linear trend") {
    // A symmetric Henderson filter reproduces polynomials up to cubic; assert
    // exact linear reproduction at an interior point. x(i)=a+b*i, N=13, K=7.
    const double a = 4.0, b = 0.5;
    double x[13];
    for (int i = 1; i <= 13; ++i) x[i - 1] = a + b * i;
    double w[7];
    hender(w, 13);
    double r = apply(x, 7, w, 13);
    CHECK(close(r, a + b * 7.0, 1e-9));
    // Constant series -> constant out (weights sum to 1).
    double c[13];
    for (int i = 0; i < 13; ++i) c[i] = 3.25;
    CHECK(close(apply(c, 7, w, 13), 3.25, 1e-12));
}

// ===========================================================================
// Tier 1 -- Henderson end filters (the classic X-11 divergence point)
// ===========================================================================

TEST("hndend: Doherty end weights vs exact hand computation") {
    // 5-term Henderson half-weights (exact rationals over 205920):
    //   w(1)=115200/205920, w(2)=60480/205920, w(3)=-15120/205920.
    double w[3];
    hender(w, 5);
    // End filter with M=3 available points, Nterm=5, R=0 (no linear term).
    //   Endwt = [0, 45360/123552? ...]; exact: [0, 0.3671328671, 0.6328671329].
    double e0[3];
    hndend(3, 5, w, e0, 0.0);
    CHECK(close(e0[0], 0.0, 1e-12));
    CHECK(close(e0[1], 0.36713286713287, 1e-9));
    CHECK(close(e0[2], 0.63286713286713, 1e-9));
    CHECK(close(e0[0] + e0[1] + e0[2], 1.0, 1e-12));
    // R=1 (c2 = 1 + 2R = 3): exact rationals over 205920:
    //   [-25200, 75600, 155520]/205920 = [-0.12237762, 0.36713287, 0.75524476].
    double e1[3];
    hndend(3, 5, w, e1, 1.0);
    CHECK(close(e1[0], -0.12237762237762, 1e-9));
    CHECK(close(e1[1], 0.36713286713287, 1e-9));
    CHECK(close(e1[2], 0.75524475524476, 1e-9));
    CHECK(close(e1[0] + e1[1] + e1[2], 1.0, 1e-12));
    // Constant reproduction is exact for every end-filter length of a 13-term
    // filter (M = 7..12), independent of R.
    double w13[7];
    hender(w13, 13);
    for (int m = 7; m <= 12; ++m) {
        double e[13];
        hndend(m, 13, w13, e, 4.0 / 3.14159265358979);
        double s = 0.0;
        for (int i = 0; i < m; ++i) s += e[i];
        CHECK(close(s, 1.0, 1e-12));
    }
}

TEST("ends: end filters reproduce a constant series") {
    // ends fills l=(K-1)/2 points at each end; each is a convex combination of
    // end weights that sum to 1, so a constant series is reproduced exactly.
    const int N = 15;
    double stci[N], stc[N];
    for (int i = 0; i < N; ++i) { stci[i] = 7.5; stc[i] = -1.0; }
    ends(stc, stci, 1, N, 5, 4.0 / 3.14159265358979);  // K=5, l=2
    CHECK(close(stc[0], 7.5, 1e-9));
    CHECK(close(stc[1], 7.5, 1e-9));
    CHECK(close(stc[N - 2], 7.5, 1e-9));
    CHECK(close(stc[N - 1], 7.5, 1e-9));
    CHECK_EQ(stc[7], -1.0);  // interior untouched by ends
}

TEST("endsf: seasonal-MA end weights (hand-computed)") {
    // Simon = 1..7, Nend=2, all-ones weight table (length >=7). The first pair
    // (j1=1,j2=7) uses W(1..3); the second (j1=2,j2=6) uses W(4..7):
    //   Savg(1)=(1+2+3)/3=2, Savg(7)=(7+6+5)/3=6,
    //   Savg(2)=(1+2+3+4)/4=2.5, Savg(6)=(7+6+5+4)/4=5.5.
    const double simon[7] = {1, 2, 3, 4, 5, 6, 7};
    const double w[7] = {1, 1, 1, 1, 1, 1, 1};
    double savg[7] = {0, 0, 0, 0, 0, 0, 0};
    endsf(simon, savg, 7, w, 2);
    CHECK_EQ(savg[0], 2.0);
    CHECK_EQ(savg[6], 6.0);
    CHECK_EQ(savg[1], 2.5);
    CHECK_EQ(savg[5], 5.5);
    CHECK_EQ(savg[2], 0.0);  // interior untouched (filled by symmetric MA)
    // jk > K branch: Nend large -> first point is the overall average.
    double savg2[7] = {0, 0, 0, 0, 0, 0, 0};
    endsf(simon, savg2, 7, w, 7);
    CHECK(close(savg2[0], 4.0, 1e-12));  // mean of 1..7
    CHECK(close(savg2[6], 4.0, 1e-12));
}

TEST("hndtrn: Henderson trend driver -- constant + linear reproduction") {
    const int N = 25;  // lfda=1, lldaf=25
    // Constant series: symmetric filter AND Doherty end filters reproduce it
    // exactly at every point.
    double stci[N], stc[N];
    for (int i = 0; i < N; ++i) { stci[i] = 5.0; stc[i] = -99.0; }
    double tic = 1.0;
    hndtrn(stc, stci, 1, N, 13, tic, /*lend=*/true, /*lsame=*/false,
           /*tru7hn=*/false);
    for (int i = 0; i < N; ++i) CHECK(close(stc[i], 5.0, 1e-9));

    // Linear series: the symmetric 13-term interior [7,19] reproduces it exactly
    // (end points are minimum-revision compromises and are not asserted).
    for (int i = 0; i < N; ++i) { stci[i] = 3.0 + 0.5 * (i + 1); stc[i] = -99.0; }
    tic = 1.0;
    hndtrn(stc, stci, 1, N, 13, tic, true, false, false);
    for (int i = 7; i <= 19; ++i)  // Fortran 1-based -> C index i-1
        CHECK(close(stc[i - 1], 3.0 + 0.5 * i, 1e-9));
}

int main() { return mt::run_all(); }
