// checkres.cpp -- see hpp.
#include "diag/checkres.hpp"

#include "common/x13context.hpp"
#include "numeric/numeric.hpp"   // chisq, dpeq
#include "specparse/specparse.hpp"  // writln
#include "gen/notset.hpp"          // prm::DNOTST
#include "gen/model.hpp"         // prm::PORDER
#include "gen/tbltab.hpp"        // prm::LCKAC2 (pracf2.f:37)
#include "x13/fformat.hpp"       // fwrite_fmt (pracf2.f:1010/1011)

#include <algorithm>
#include <cmath>
#include <vector>

namespace x13 {

namespace {

// nrmtst.var -- the one-percent points of the three normality statistics, with
// the observation counts they are tabulated at. Verbatim.
const double APP1U[9] = {0.9359, 0.9137, 0.9001, 0.8901, 0.8827, 0.8769,
                         0.8722, 0.8682, 0.8648};
const double APP1L[9] = {0.6675, 0.6829, 0.6950, 0.7040, 0.7110, 0.7167,
                         0.7216, 0.7256, 0.7291};
const int NA1[9] = {11, 16, 21, 26, 31, 36, 41, 46, 51};

const double APP2U[7] = {0.8722, 0.8648, 0.8592, 0.8549, 0.8515, 0.8484, 0.8460};
const double APP2L[7] = {0.7216, 0.7291, 0.7347, 0.7393, 0.7430, 0.7460, 0.7487};
const int NA2[7] = {41, 51, 61, 71, 81, 91, 101};

const double APP3U[10] = {0.8460, 0.8322, 0.8260, 0.8223, 0.8198,
                          0.8179, 0.8164, 0.8152, 0.8142, 0.8134};
const double APP3L[10] = {0.7487, 0.7629, 0.7693, 0.7731, 0.7757,
                          0.7776, 0.7791, 0.7803, 0.7814, 0.7822};
const int NA3[10] = {101, 201, 301, 401, 501, 601, 701, 801, 901, 1001};

const double KPP1U[5] = {4.88, 4.59, 4.39, 4.24, 4.13};
const double KPP1L[5] = {1.95, 2.08, 2.18, 2.24, 2.29};
const int NK1[5] = {50, 75, 100, 125, 150};

const double KPP2U[19] = {4.39, 4.13, 3.98, 3.87, 3.79, 3.72, 3.67, 3.63, 3.60,
                          3.57, 3.54, 3.52, 3.50, 3.48, 3.46, 3.45, 3.43, 3.42,
                          3.41};
const double KPP2L[19] = {2.18, 2.29, 2.37, 2.42, 2.46, 2.50, 2.52, 2.55, 2.57,
                          2.58, 2.60, 2.61, 2.62, 2.64, 2.65, 2.66, 2.66, 2.67,
                          2.68};
const int NK2[19] = {100, 150, 200, 250, 300, 350, 400, 450, 500, 550, 600,
                     650, 700, 750, 800, 850, 900, 950, 1000};

const double SPP1[6] = {1.061, 0.986, 0.923, 0.870, 0.825, 0.787};
const int NS1[6] = {25, 30, 35, 40, 45, 50};
const double SPP2[6] = {0.787, 0.723, 0.673, 0.631, 0.596, 0.567};
const int NS2[6] = {50, 60, 70, 80, 90, 100};
const double SPP3[5] = {0.567, 0.508, 0.464, 0.430, 0.403};
const int NS3[5] = {100, 125, 150, 175, 200};
const double SPP4[7] = {0.403, 0.360, 0.329, 0.305, 0.285, 0.269, 0.255};
const int NS4[7] = {200, 250, 300, 350, 400, 450, 500};

// intrpp.f -- the tabulated point at `ppi` (1-based), interpolated to `nobs` on
// first differences and optionally refined on second differences. NOTE the
// step it divides by is `Ppnum(2)-Ppnum(1)`, the FIRST gap of the table, not
// the gap at ppi -- correct only because every table is evenly spaced, and
// transcribed as written either way.
double intrpp(const double* ppvec, const int* ppnum, int nobs, int ppi,
              bool dif2nd) {
    const int k = ppi - 1;   // 0-based
    double v = ppvec[k];
    if (ppnum[k] == nobs) return v;
    const double theta = static_cast<double>(nobs - ppnum[k]) /
                         static_cast<double>(ppnum[1] - ppnum[0]);
    v += theta * (ppvec[k + 1] - ppvec[k]);
    if (dif2nd)
        v += ((theta * (theta - 1.0)) / 2.0) *
             (ppvec[k + 2] - 2.0 * ppvec[k + 1] + ppvec[k]);
    return v;
}

}  // namespace

// ansub11.f:1303 -- Kendall's statistic behind the Friedman seasonality test.
// Ranks each year's `mq` observations (average ranks on ties), sums the ranks
// by period, and forms 12*SS/((mq+1)*mq*ny). The leading `res` partial year is
// DROPPED, so the ranking runs over whole years counted back from the END.
double kendalls(const double* x, int nz, int mq) {
    constexpr double DBL_MAX_F = 1.0e307;
    constexpr int LOOPMAX = 1000;
    if (mq <= 1) return 0.0;
    const int ny = nz / mq;
    const int res = nz - ny * mq;
    if (ny <= 0) return 0.0;

    std::vector<double> r(static_cast<std::size_t>(ny) * mq, 0.0);
    std::vector<double> obs(static_cast<std::size_t>(mq));
    std::vector<int> found(static_cast<std::size_t>(mq));

    for (int i = 1; i <= ny; ++i) {
        for (int j = 1; j <= mq; ++j)
            obs[j - 1] = x[res + (i - 1) * mq + j - 1];
        int ind = 1, maxloop = 0;
        while (ind <= mq && maxloop < LOOPMAX) {
            ++maxloop;
            double min_val = obs[0];
            for (int j = 1; j < mq; ++j) if (obs[j] < min_val) min_val = obs[j];
            int k = 0;
            for (int j = 0; j < mq; ++j) {
                found[j] = 0;
                if (std::fabs(obs[j] - min_val) < 1.0e-20) { ++k; found[j] = 1; }
            }
            const double value = ind + (k - 1) / 2.0;
            for (int j = 0; j < mq; ++j) {
                if (found[j] == 1) {
                    obs[j] = DBL_MAX_F;
                    r[static_cast<std::size_t>(i - 1) * mq + j] = value;
                }
            }
            ind += k;
        }
        if (maxloop > LOOPMAX) return 0.0;
    }

    double tmp = 0.0;
    for (int i = 0; i < mq; ++i) {
        double sum = 0.0;
        for (int j = 0; j < ny; ++j)
            sum += r[static_cast<std::size_t>(j) * mq + i];
        const double d = sum - ny * (mq + 1) / 2.0;
        tmp += d * d;
    }
    return 12.0 * tmp / ((mq + 1.0) * mq * ny);
}

namespace {

// nrmtst.f -- skewness / Geary's a / kurtosis, each against its one-percent
// point. The Fortran RETURNS EARLY out of the middle of the routine when a
// statistic's table cannot cover `nobs`, which suppresses every statistic
// AFTER it as well; the have_* flags reproduce that.
//
// Each of those five early returns writes a NOTE first, to Mt1 AND Mt2, and
// unconditionally -- `Lprt` guards the tables above and below but not these.
// The port had the returns and none of the NOTEs, which is why two of the texts
// sat in test_err_block's `_UNPORTED_BLOCKS`. Note the port had also FUSED the
// low and high bound of Geary's a into one `return` and likewise for kurtosis;
// the Fortran's two arms write DIFFERENT sentences, so they are split again
// here. Nothing about the numbers changes.
void nrmtst(X13Context& ctx, CheckDiagnostics& ck, const double* y, int nobs) {
    const int mt1 = ctx.units.mt1;
    const int mt2 = ctx.units.mt2;
    auto note = [&](const char* l1, const char* l2) {
        writln(ctx, l1, mt1, mt2, true);
        writln(ctx, l2, mt1, mt2, false);
    };
    if (nobs <= 0) return;
    const double dnobs = static_cast<double>(nobs);
    // totals(Y,1,Nobs,1,1) -- Iopt==1 is the AVERAGE, and it SKIPS DNOTST
    // values in both the sum and the count (totals.f:30-37). The moments below
    // then divide by the full Nobs regardless, which is the Fortran's own
    // asymmetry; residuals never carry DNOTST, so the two counts agree here.
    double ymu = 0.0;
    {
        double fn = 0.0;
        for (int i = 0; i < nobs; ++i) {
            if (dpeq(y[i], prm::DNOTST)) continue;
            ymu += y[i];
            fn += 1.0;
        }
        ymu = (fn > 0.0) ? ymu / fn : 0.0;
    }

    double ga = 0.0, m2 = 0.0, m3 = 0.0, m4 = 0.0;
    for (int i = 0; i < nobs; ++i) {
        const double d = y[i] - ymu;
        ga += std::fabs(d) / dnobs;
        m2 += (d * d) / dnobs;
        m3 += (d * d * d) / dnobs;
        m4 += (d * d * d * d) / dnobs;
    }
    if (m2 <= 0.0) return;
    ga /= std::sqrt(m2);
    const double ykurt = m4 / (m2 * m2);
    const double yskew = m3 / (m2 * std::sqrt(m2));

    // ---- skewness -------------------------------------------------------
    double ppu = 0.0, ppl = 0.0;
    if (nobs < 25) {                             // nrmtst.f:53-57 early RETURN
        note("NOTE: The program cannot compute the significance of skewness "
             "statistic",
             "      on less than 25 observations.");
        return;
    }
    if (nobs < 50)        ppu = intrpp(SPP1, NS1, nobs, ((nobs - 25) / 5) + 1, false);
    else if (nobs < 100)  ppu = intrpp(SPP2, NS2, nobs, ((nobs - 50) / 10) + 1, false);
    else if (nobs < 200)  ppu = intrpp(SPP3, NS3, nobs, ((nobs - 100) / 25) + 1, false);
    else if (nobs < 500)  ppu = intrpp(SPP4, NS4, nobs, ((nobs - 200) / 50) + 1, false);
    else                  ppu = 2.326 * std::sqrt(6.0 / dnobs);
    ppl = -ppu;
    ck.have_skew = true;
    ck.skewness = yskew;
    ck.skew_mark = (yskew < ppl) ? '-' : (yskew > ppu ? '+' : ' ');

    // ---- Geary's a ------------------------------------------------------
    if (nobs < 11) {                             // nrmtst.f:101-105
        note("NOTE: The program cannot compute the significance of Geary's a "
             "statistic",
             "      on less than 11 observations.");
        return;
    }
    if (nobs > 1001) {                           // nrmtst.f:123-127
        note("NOTE: The program cannot compute the significance of Geary's a "
             "statistic",
             "      on more than 1001 observations.");
        return;
    }
    if (nobs < 41) {
        const int ppi = ((nobs - 11) / 5) + 1;
        ppu = intrpp(APP1U, NA1, nobs, ppi, true);
        ppl = intrpp(APP1L, NA1, nobs, ppi, true);
    } else if (nobs < 101) {
        if (nobs == 46) { ppu = APP1U[7]; ppl = APP1L[7]; }   // nrmtst.f:110-112
        else {
            const int ppi = ((nobs - 41) / 10) + 1;
            ppu = intrpp(APP2U, NA2, nobs, ppi, nobs < 81);
            ppl = intrpp(APP2L, NA2, nobs, ppi, nobs < 81);
        }
    } else {
        const int ppi = ((nobs - 101) / 100) + 1;
        ppu = intrpp(APP3U, NA3, nobs, ppi, nobs < 801);
        ppl = intrpp(APP3L, NA3, nobs, ppi, nobs < 801);
    }
    ck.have_geary = true;
    ck.geary = ga;
    ck.geary_mark = (ga < ppl || ga > ppu) ? '*' : ' ';

    // ---- kurtosis -------------------------------------------------------
    if (nobs < 50) {                             // nrmtst.f:147-151
        note("NOTE: The program cannot perform hypothesis tests for kurtosis on",
             "      less than 50 observations.");
        return;
    }
    if (nobs >= 1001) {                          // nrmtst.f:165-169
        // The bound and the sentence disagree in the ORACLE: the guard is
        // `Nobs.ge.1001` and the text says "more than 1000". Transcribed.
        note("NOTE: The program cannot perform hypothesis tests for kurtosis on "
             "more",
             "      than 1000 observations.");
        return;
    }
    if (nobs < 100) {
        const int ppi = ((nobs - 50) / 25) + 1;
        ppu = intrpp(KPP1U, NK1, nobs, ppi, true);
        ppl = intrpp(KPP1L, NK1, nobs, ppi, true);
    } else {
        if (nobs == 125) { ppu = KPP1U[4]; ppl = KPP1L[4]; }  // nrmtst.f:158-160
        else {
            const int ppi = ((nobs - 100) / 50) + 1;
            ppu = intrpp(KPP2U, NK2, nobs, ppi, nobs < 900);
            ppl = intrpp(KPP2L, NK2, nobs, ppi, nobs < 900);
        }
    }
    ck.have_kurt = true;
    ck.kurtosis = ykurt;
    ck.kurt_mark = (ykurt < ppl || ykurt > ppu) ? '*' : ' ';
}

}  // namespace

bool check_acf(const double* z, int nz, int nefobs, double* r, double* se,
               int nr, int np, int /*sp*/, int iqtype, bool lmu,
               double* qs, int* dgf, double* qpv) {
    double mu = 0.0;
    if (lmu) for (int k = 0; k < nz; ++k) mu += z[k];
    // acf.f:52 divides OUTSIDE the Lmu block, so with Lmu false mu stays 0.
    mu /= static_cast<double>(nz);

    double c0 = 0.0;
    for (int k = 0; k < nz; ++k) c0 += (z[k] - mu) * (z[k] - mu);
    if (c0 <= 0.0) return false;
    c0 /= static_cast<double>(nz);

    const double dnefobs = static_cast<double>(nefobs);
    double sq = 0.0;
    for (int i = 1; i <= nr; ++i) {
        double c = 0.0;
        for (int j = i; j < nz; ++j) c += (z[j] - mu) * (z[j - i] - mu);
        c /= static_cast<double>(nz);
        r[i - 1] = c / c0;
        if (iqtype == 0) {
            sq += r[i - 1] * r[i - 1] / (dnefobs - i);
            qs[i - 1] = sq * dnefobs * (dnefobs + 2.0);
        } else {
            sq += r[i - 1] * r[i - 1];
            qs[i - 1] = sq * dnefobs;
        }
        dgf[i - 1] = (i - np > 0) ? (i - np) : 0;
        qpv[i - 1] = (dgf[i - 1] > 0) ? chisq(qs[i - 1], dgf[i - 1]) : 0.0;
    }

    // Bartlett's formula: se(1) = 1/sqrt(n), se(q+1) = sqrt((1+2*sum r_i^2)/n).
    se[0] = 1.0 / std::sqrt(static_cast<double>(nz));
    double sr = 0.0;
    for (int i = 1; i <= nr - 1; ++i) {
        sr += r[i - 1] * r[i - 1];
        se[i] = std::sqrt((1.0 + 2.0 * sr) / static_cast<double>(nz));
    }
    return true;
}

void check_pacf(int nefobs, double* r, double* se, int nr) {
    if (nr <= 0) return;
    const double sep = 1.0 / std::sqrt(static_cast<double>(nefobs));
    // p is (nr x nr); the Yule-Walker recursion only ever reads row l-1.
    std::vector<double> p(static_cast<std::size_t>(nr) * nr, 0.0);
    auto P = [&](int i, int j) -> double& {
        return p[static_cast<std::size_t>(i - 1) * nr + (j - 1)];
    };
    std::vector<double> fkk(static_cast<std::size_t>(nr), 0.0);

    P(1, 1) = r[0];
    fkk[0] = P(1, 1);
    se[0] = sep;
    for (int l = 2; l <= nr; ++l) {
        se[l - 1] = sep;
        double dtop = 0.0, dbot = 0.0;
        for (int j = 1; j <= l - 1; ++j) {
            dtop += P(l - 1, j) * r[l - j - 1];
            dbot += P(l - 1, j) * r[j - 1];
        }
        P(l, l) = (r[l - 1] - dtop) / (1.0 - dbot);
        fkk[l - 1] = P(l, l);
        for (int j = 1; j <= l - 1; ++j)
            P(l, j) = P(l - 1, j) - P(l, l) * P(l - 1, l - j);
    }
    for (int i = 0; i < nr; ++i) r[i] = fkk[i];   // pacf.f's copy(fkk,Nr,1,R)
}

void check_residuals(X13Context& ctx, const double* a, int na, int nefobs) {
    // arima.f:1046 -- the whole block is behind IF(Convrg), and every statistic
    // inside is additionally behind Var > 0 (a zero-variance fit has no
    // residual structure to test).
    if (!ctx.mdldat.convrg) return;
    if (!(ctx.mdldat.var > 0.0)) return;
    if (na <= 0 || nefobs <= 0 || nefobs > na) return;

    const int sp = ctx.model.sp;
    CheckDiagnostics& ck = ctx.check;
    ck = CheckDiagnostics{};
    ck.qlimit = ctx.chkopt.qcheck;
    ck.acflimit = ctx.chkopt.acflim;

    // acfdgn.f:33-42 -- resolve Mxcklg. Unset (0) means "derive it": 10 for a
    // nonseasonal series, else two years, capped at a quarter of the effective
    // observations. An explicit check{maxlag=} is only capped at nefobs-1.
    int mxlag = ctx.chkopt.mxcklg;
    if (mxlag == 0) {
        mxlag = (sp == 1) ? 10 : 2 * sp;
        if (mxlag > nefobs / 4) mxlag = nefobs / 4;
    } else {
        if (mxlag > nefobs - 1) mxlag = nefobs - 1;
    }
    if (mxlag <= 0) return;
    constexpr int PR = 1020 / 4;
    if (mxlag > PR) mxlag = PR;
    ck.mxlag = mxlag;
    // arima.f:1053 -- the Q counts run over the first two years only.
    const int nlagbl = std::min(mxlag, sp * 2);
    ck.nlagbl = nlagbl;

    // acfdgn.f:45-49 -- np is the number of FREE ARMA parameters, counted over
    // the lag slots up to the last operator's start (Opr(Nopr)-1).
    int np = 0;
    {
        const model_cmn& m = ctx.model;
        const int endlag = m.opr(m.nopr) - 1;
        for (int ilag = 1; ilag <= endlag && ilag <= prm::PARIMA; ++ilag)
            if (!m.arimaf(ilag)) ++np;
    }

    const double* z = a + (na - nefobs);   // A(Na-Nefobs+1)
    std::vector<double> r(PR, 0.0), se(PR, 0.0), qs(PR, 0.0), qpv(PR, 0.0);
    std::vector<int> dgf(PR, 0);

    // ---- Ljung-Box (Iqtype 0) + the significant-ACF list -------------------
    if (!check_acf(z, nefobs, nefobs, r.data(), se.data(), mxlag, np, sp,
                   /*iqtype=*/0, /*lmu=*/true, qs.data(), dgf.data(),
                   qpv.data()))
        return;
    for (int i = 1; i <= nlagbl; ++i) {
        if (dgf[i - 1] > 0 && qpv[i - 1] < ck.qlimit)
            ck.lbq.push_back({i, qs[i - 1], qpv[i - 1], dgf[i - 1], 0.0});
    }
    // acfdgn.f:186-200 -- the significant-ACF list is built from the SAME r/se
    // the Ljung-Box pass left, which is why it is collected here and not after
    // the Box-Pierce recompute below.
    for (int i = 1; i <= nlagbl; ++i) {
        const double t = r[i - 1] / se[i - 1];
        if (std::fabs(t) > ck.acflimit)
            ck.sigacf.push_back({i, r[i - 1], se[i - 1], 0, t});
    }
    // Keep the ACF for the pacf recursion below: acfdgn calls pacf AFTER the
    // Box-Pierce acf() call, so pacf actually reads the Box-Pierce pass's r --
    // which is the same sequence (Iqtype only changes the Q weighting, never
    // r or se). Recomputed rather than assumed, to stay faithful either way.

    // ---- Box-Pierce (Iqtype 1) --------------------------------------------
    check_acf(z, nefobs, nefobs, r.data(), se.data(), mxlag, np, sp,
              /*iqtype=*/1, /*lmu=*/true, qs.data(), dgf.data(), qpv.data());
    for (int i = 1; i <= nlagbl; ++i) {
        if (dgf[i - 1] > 0 && qpv[i - 1] < ck.qlimit)
            ck.bpq.push_back({i, qs[i - 1], qpv[i - 1], dgf[i - 1], 0.0});
    }

    // ---- partial ACF ------------------------------------------------------
    // acfdgn.f:208 -- pacf() overwrites r in place with the pacf and se with
    // the flat 1/sqrt(nefobs).
    check_pacf(nefobs, r.data(), se.data(), mxlag);
    for (int i = 1; i <= nlagbl; ++i) {
        const double t = r[i - 1] / se[i - 1];
        if (std::fabs(t) > ck.acflimit)
            ck.sigpacf.push_back({i, r[i - 1], se[i - 1], 0, t});
    }

    // ---- normality (nrmtst.f) --------------------------------------------
    nrmtst(ctx, ck, z, nefobs);

    // ---- Durbin-Watson (arima.f:1075-1090) --------------------------------
    {
        double rvar = 0.0;
        for (int i = na - nefobs + 1; i <= na; ++i) rvar += a[i - 1] * a[i - 1];
        double dw = 0.0;
        for (int i = na - nefobs + 2; i <= na; ++i) {
            const double d = a[i - 1] - a[i - 2];
            dw += d * d;
        }
        if (rvar > 0.0) {
            ck.have_dw = true;
            ck.dw = dw / rvar;
        }
    }

    // ---- Friedman / Kendall (arima.f:1092-1101) ---------------------------
    {
        const double ken = kendalls(z, nefobs, sp);
        ck.have_friedman = true;
        ck.friedman = ken;
        ck.friedman_df = sp - 1;
        ck.friedman_pv = chisq(ken, sp - 1);
    }

    ck.ran = true;
}

// pracf2.f:37-60 -- the ENTRY GUARD and the two early-return NOTEs in front of
// the squared-residual ACF. The routine BODY (the ACF/PACF of a^2, its Ljung-Box
// Q and the histogram) is a print/save surface and is deferred with the rest of
// the `.out` engine, exactly like every other table producer here -- so this is
// NOT a wall: nothing is refused that the port otherwise computes.
//
// What was missing is the pair of refusals the oracle writes INSTEAD of that
// table, and they are not print surface -- they go to Mt2. `DEFTAB[LCKAC2]` is
// true, so the entry guard passes on a spec that never mentions `check{}`, and
// the second arm fires on any residual set shorter than ten years.
//
// `Lgraf` is in the entry guard and NOT in either NOTE guard: a graphics-only
// run enters the routine and writes no message. Transcribed as such; this port
// has no Lgraf, so the term is dropped and the difference is invisible until
// graphics exist.
void pracf2_notes(X13Context& ctx, int nefobs) {
    const int iacf = prm::LCKAC2, iacp = prm::LCKAC2 + 1;
    const bool want = ctx.tbllog.prttab(iacf) || ctx.tbllog.savtab(iacf) ||
                      ctx.tbllog.prttab(iacp);
    if (!want) return;                       // (|| Lgraf -- see above)
    auto& mt2 = ctx.channels_.unit(ctx.units.mt2);
    if (ctx.mdldat.var <= 0.0) {
        mt2.put(fwrite_fmt(
                    "(/,' NOTE: Can''t calculate an ACF of the squared ',"
                    "'residuals for a model with no variance.')") +
                "\n");
        return;
    }
    if (nefobs <= 10 * ctx.model.sp)
        mt2.put(fwrite_fmt(
                    "(/,' NOTE: ',a,' will not compute the ACF of the',"
                    "' squared residuals for',/,"
                    "'       a set of residuals that is less than ten ',"
                    "'years long.')",
                    std::string(stdio::PRGNAM)) +
                "\n");
}

}  // namespace x13
