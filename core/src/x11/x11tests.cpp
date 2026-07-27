// x11tests.cpp -- the X-11 seasonality test battery (see x11tests.hpp).
// Faithful ports of ftest.f, kwtest.f, mstest.f, combft.f.
//
// All four routines are mostly WRITE in the oracle; only the COMMON-block
// assignments are ported. Every early RETURN that PRECEDES an assignment is
// kept (it is what decides whether /tests/ keeps a stale value from an earlier
// pass), while the ones that only skip printing collapse into the tail.
#include "x11/x11tests.hpp"

#include "common/x13context.hpp"
#include "numeric/numeric.hpp"  // fvalue, chisq, dpeq

#include <cmath>
#include <vector>

namespace x13 {

// ftest.f -- one-way ANOVA. The oracle's Ind==1 branch loops twice (whole
// series, then the last three years) purely to print/save two more F values;
// neither is stored in a COMMON, so this port computes the first pass only.
void ftest(X13Context& ctx, const double* x, int ib, int ie, int nyr, int ind) {
    const int muladd = ctx.x11opt.muladd;

    if (ctx.hiddn.issap == 2 && ind > 0) return;
    double c = 1.0;
    if (muladd == 0) c = 10000.0;

    std::vector<double> temp(static_cast<std::size_t>(ie) + 1, 0.0);
    int kb;
    int l = 0;
    if (ind == 0 || ind == 2) {
        for (int i = ib; i <= ie; ++i) temp[i] = x[i - 1];
        kb = ib;
    } else {
        // "Cannot perform F-test on first differenced data" -- print-only in
        // the oracle, but it RETURNS, so nothing downstream is written.
        if (ctx.x11msc.same) return;
        l = nyr / 4;
        kb = ib + l;
        for (int i = kb; i <= ie; ++i) temp[i] = x[i - 1] - x[i - l - 1];
    }

    // ftest.f:65 -- a DO WHILE the Ind==1 (D11) path goes round TWICE: once
    // over the whole span, then again over just the last three years, which is
    // what separates `d11.f` from `d11.3y.f`. Every other Ind takes one pass.
    int itype = 0;
    while (true) {
        int nt = 0;
        double sumt = 0.0, ssqt = 0.0, ssm = 0.0;
        for (int i = 1; i <= nyr; ++i) {
            int nm = 0;
            double summ = 0.0;
            const int ji = i + kb - 1;
            for (int j = ji; j <= ie; j += nyr) {
                nm += 1;
                summ += temp[j];
                ssqt += temp[j] * temp[j];
            }
            if (nm == 0) continue;
            nt += nm;
            sumt += summ;
            ssm += summ * summ / nm;
        }
        const double st = nt;
        ssqt = (ssqt - sumt * sumt / st) * c;
        ssm = (ssm - sumt * sumt / st) * c;
        const double ssr = ssqt - ssm;
        const int kdfr = nt - nyr;
        const int kdfb = nyr - 1;
        const double fmsm = ssm / static_cast<double>(kdfb);
        const double fmsr = ssr / static_cast<double>(kdfr);
        // Residual MSE of exactly zero: the oracle warns and RETURNS, leaving
        // /tests/ untouched.
        if (dpeq(fmsr, 0.0)) return;

        double f = fmsm / fmsr;
        // NOTE fvalue takes f BY REFERENCE and may zero it (CB-17) -- and the
        // oracle stores f AFTER this call, so the clobbered value is the one
        // that reaches Fstabl/Fpres. Faithful.
        const double prob = fvalue(f, kdfb, kdfr) * 100.0;
        if (ind == 0) {
            ctx.tests.fstabl = f;
            ctx.tests.p1 = prob;
        } else if (ind != 1) {
            ctx.tests.fpres = f;
            ctx.tests.p3 = prob;
        }
        // ftest.f:113 -- this RETURN sits between the /tests/ store above and
        // the sliding-spans store below, so it must be kept even though every
        // other thing it guards is print.
        if ((ctx.hiddn.lhiddn && ctx.hiddn.issap < 2) ||
            ((ctx.hiddn.ixreg == 2 || ctx.x11opt.khol == 1) &&
             !ctx.title.prt1ps))
            return;
        // Sliding-spans per-column stable-F record (ftest.f:120).
        if (ind == 0 && ctx.hiddn.issap == 2) ctx.ssft.ssfts(ctx.ssft.icol) = f;

        // ftest.f:163-195 -- the D11 savelog rows. `d11.f` on the first pass,
        // `d11.3y.f` on the second; the `i` prefix is the INDIRECT composite
        // adjustment's (Iagr>=4), which agr3 reaches with the same routine.
        if (ind != 0 && ind != 2) {
            const bool indirect = ctx.agr.iagr >= 4;
            if (itype == 0) {
                if (indirect) {
                    ctx.x11_id11f = f;
                    ctx.x11_id11f_prob = prob;
                    ctx.x11_id11f_set = true;
                } else {
                    ctx.x11_d11f = f;
                    ctx.x11_d11f_prob = prob;
                    ctx.x11_d11f_set = true;
                }
                // ftest.f:192-196 -- go round again over the last three years,
                // but only if that window still starts inside the series.
                const int kb2 = ie - 3 * nyr + 1;
                if (kb2 < (ib + l)) return;
                kb = kb2;
                itype = 1;
                continue;
            }
            if (indirect) {
                ctx.x11_id11f3y = f;
                ctx.x11_id11f3y_prob = prob;
                ctx.x11_id11f3y_set = true;
            } else {
                ctx.x11_d11f3y = f;
                ctx.x11_d11f3y_prob = prob;
                ctx.x11_d11f3y_set = true;
            }
        }
        return;
    }
}

// kwtest.f -- Kruskal-Wallis. The rank sort is the oracle's own selection sort
// (an exchange whenever xval > X(j)), NOT a library sort: it is not stable and
// its tie handling is what the column rank sums see, so it is transcribed
// literally rather than replaced.
void kwtest(X13Context& ctx, double* x, int ib, int ie, int nyr) {
    std::vector<int> ns(static_cast<std::size_t>(nyr) + 1, 0);
    std::vector<int> kolr(static_cast<std::size_t>(nyr) + 1, 0);
    std::vector<int> k(static_cast<std::size_t>(ie) + 1, 0);

    for (int i = ib; i <= ie; ++i) k[i] = i;

    for (int i = ib; i <= ie; ++i) {
        double xval = x[i - 1];
        int kval = k[i];
        for (int j = i; j <= ie; ++j) {
            if (xval > x[j - 1]) {
                x[i - 1] = x[j - 1];
                k[i] = k[j];
                x[j - 1] = xval;
                k[j] = kval;
                xval = x[i - 1];
                kval = k[i];
            }
        }
    }

    for (int i = ib; i <= ie; ++i) {
        const int l = k[i] - (k[i] - 1) / nyr * nyr;
        ns[l] += 1;
        kolr[l] = i - ib + 1 + kolr[l];
    }

    double ck = 0.0;
    for (int i = 1; i <= nyr; ++i) {
        // kolr is INTEGER in the oracle, so kolr*kolr is 32-bit integer
        // arithmetic that can overflow on a long series (a monthly span of
        // ~1000 obs puts kolr near 41500, squared ~1.7e9 against a 2.1e9
        // ceiling). Kept as int -- widening it would diverge from the oracle
        // exactly where the oracle goes wrong.
        ck += static_cast<double>(kolr[i] * kolr[i]) / static_cast<double>(ns[i]);
    }
    const int n = ie - ib + 1;
    ctx.tests.chikw = 12.0 * ck / (n * (n + 1)) - 3 * (n + 1);
    const int ndf = nyr - 1;
    ctx.tests.p5 = chisq(ctx.tests.chikw, ndf) * 100.0;
}

// mstest.f -- two-way ANOVA (years x seasons) on the absolute deviation of the
// SI ratios from the mode identity (1 multiplicative, 0 additive).
void mstest(X13Context& ctx, const double* array, int jfda, int jlda, int nyr) {
    const double ONE = 1.0, ZERO = 0.0;
    const int muladd = ctx.x11opt.muladd;

    double c = ONE;
    const int ifmo = (jfda + nyr - 2) / nyr * nyr + 1;
    const int lmo = jlda / nyr * nyr;
    const int noyrs = (lmo - ifmo) / nyr + 1;
    const double fnoyrs = noyrs;

    std::vector<double> temp(static_cast<std::size_t>(lmo) + 1, 0.0);
    if (muladd == 0) {
        c = 10000.0;
        for (int j = ifmo; j <= lmo; ++j) temp[j] = std::fabs(array[j - 1] - ONE);
    } else {
        for (int i = ifmo; i <= lmo; ++i) temp[i] = std::fabs(array[i - 1]);
    }

    double suma1 = ZERO;
    for (int kk = ifmo; kk <= lmo; ++kk) suma1 += temp[kk];
    const double fnyr = nyr;
    const double xbar = suma1 / (fnyr * fnoyrs);

    double colss = ZERO;
    for (int l = 1; l <= nyr; ++l) {
        double colmn = ZERO;
        const int k1 = ifmo + l - 1;
        for (int m = k1; m <= lmo; m += nyr) colmn += temp[m];
        colmn = colmn / fnoyrs;
        colss += (colmn - xbar) * (colmn - xbar);
    }
    colss = colss * fnoyrs * c;

    double rowss = ZERO;
    for (int n = ifmo; n <= lmo; n += nyr) {
        double rowmn = ZERO;
        const int l1 = n + nyr - 1;
        for (int i1 = n; i1 <= l1; ++i1) rowmn += temp[i1];
        rowmn = rowmn / fnyr;
        rowss += (rowmn - xbar) * (rowmn - xbar);
    }
    rowss = rowss * fnyr * c;

    double totss = ZERO;
    for (int j1 = ifmo; j1 <= lmo; ++j1) totss += (temp[j1] - xbar) * (temp[j1] - xbar);
    const double errss = totss * c - colss - rowss;
    const double degfre = fnoyrs - ONE;
    const double rowssn = rowss / degfre;
    const double errssn = errss / ((fnyr - ONE) * degfre);
    const int ndgfre = (nyr - 1) * (noyrs - 1);
    // Zero residual MSE: warn-and-RETURN in the oracle, /tests/ untouched.
    if (dpeq(errssn, ZERO)) return;

    ctx.tests.fmove = rowssn / errssn;
    const int n1 = noyrs - 1;
    // By reference: CB-17 can zero Fmove in place here, and the oracle both
    // stores and prints the clobbered value.
    ctx.tests.p2 = fvalue(ctx.tests.fmove, n1, ndgfre) * 100.0;
    if (ctx.hiddn.issap == 2) ctx.ssft.ssmf(ctx.ssft.icol) = ctx.tests.fmove;
}

// combft.f -- combined identifiable-seasonality verdict. Iqfail indexes the
// oracle's yn/'yes','no '/ DATA, so 1 == present, 2 == not present.
void combft(X13Context& ctx) {
    tests_cmn& t = ctx.tests;

    t.iqfail = 1;
    t.test1 = 9.0;
    if (t.fstabl * 9.0 >= 7.0) t.test1 = 7.0 / t.fstabl;
    if (t.fstabl > 0.0) t.test2 = (3.0 * t.fmove) / t.fstabl;
    // NOTE the read of Test2 when Fstabl<=0: the oracle's .OR. is not
    // short-circuit-guaranteed, and Test2 is then whatever the previous run
    // left in the COMMON. The assignment that follows makes it 9 either way.
    if (t.test2 > 9.0 || t.fstabl <= 0.0) t.test2 = 9.0;

    if (t.p1 < 0.1) {
        if (t.p2 <= 5.0) {
            const double test = (t.test1 + t.test2) / 2.0;
            if (test >= 1.0) {  // GO TO 10 -- "NOT PRESENT"
                t.iqfail = 2;
                if (ctx.hiddn.issap == 2) ctx.ssft.issqf(ctx.ssft.icol) = 2;
                return;
            }
        }
        if (t.test1 < 1.0 && t.p5 <= 0.1 && t.test2 < 1.0) {
            // "IDENTIFIABLE SEASONALITY PRESENT"
            if (ctx.hiddn.issap == 2) ctx.ssft.issqf(ctx.ssft.icol) = 0;
            return;
        }
        // "PROBABLY NOT PRESENT" -- note Iqfail stays 1 ("yes") here; only the
        // printed wording softens.
        if (ctx.hiddn.issap == 2) ctx.ssft.issqf(ctx.ssft.icol) = 1;
        return;
    }
    t.iqfail = 2;
    if (ctx.hiddn.issap == 2) ctx.ssft.issqf(ctx.ssft.icol) = 2;
}

}  // namespace x13
