// x11filt.cpp -- X-11 Tier 0 + Tier 1 leaf routines (see x11filt.hpp). Faithful
// ports of the vendored oracle Fortran: divsub.f, addmul.f, logar.f, antilg.f,
// setmv.f, change.f, divgud.f, chkzro.f, averag.f, hender.f, apply.f, hndend.f,
// ends.f, endsf.f, hndtrn.f.
//
// Index convention: 0-based C pointers, Fortran index i -> element [i-1] (as in
// core/src/numeric). Ranges (jfda/jlda, ib/ie, ...) stay Fortran 1-based.
#include "x11/x11filt.hpp"

#include "numeric/numeric.hpp"  // totals (endsf)
#include "gen/notset.hpp"       // prm::DNOTST

#include <cmath>

namespace x13 {

// hender.prm: maximum Henderson filter length and derived work-array sizes.
namespace {
constexpr int PMXHND = 101;             // max Henderson length
constexpr int PMXHN2 = (PMXHND + 1) / 2;  // half-weight buffer (== 51)
constexpr int PMXHN1 = PMXHND - 1;        // end-weight buffer (== 100)
}  // namespace

// ---- Tier 0: mode-arithmetic recombine primitives --------------------------

void divsub(double* result, const double* a1, const double* a2, int jfda,
            int jlda, int muladd) {
    if (muladd == 0) {
        for (int i = jfda; i <= jlda; ++i) result[i - 1] = a1[i - 1] / a2[i - 1];
        return;
    }
    for (int i = jfda; i <= jlda; ++i) result[i - 1] = a1[i - 1] - a2[i - 1];
}

void addmul(double* z, const double* x, const double* y, int ib, int ie,
            int muladd) {
    if (muladd != 0) {
        for (int i = ib; i <= ie; ++i) z[i - 1] = x[i - 1] + y[i - 1];
        return;
    }
    for (int i = ib; i <= ie; ++i) z[i - 1] = x[i - 1] * y[i - 1];
}

void logar(double* x, int i, int j) {
    for (int k = i; k <= j; ++k) x[k - 1] = std::log(x[k - 1]);
}

void antilg(double* x, int i, int j) {
    for (int k = i; k <= j; ++k) x[k - 1] = std::exp(x[k - 1]);
}

void setmv(double* srs, const bool* mvind, double mvval, int pos1ob,
           int posfob) {
    for (int i = pos1ob; i <= posfob; ++i)
        if (mvind[i - 1]) srs[i - 1] = mvval;
}

void change(const double* x, double* y, int ib, int ie, int muladd,
            const bool* gudval) {
    if (muladd != 1) {
        for (int i = ib; i <= ie; ++i) {
            if (gudval[i - 2])  // Gudval(i-1)
                y[i - 1] = (x[i - 1] - x[i - 2]) / x[i - 2];
            else
                y[i - 1] = prm::DNOTST;
        }
        return;
    }
    for (int i = ib; i <= ie; ++i) y[i - 1] = x[i - 1] - x[i - 2];
}

void divgud(double* result, const double* a1, const double* a2, int jfda,
            int jlda, const bool* gudval) {
    for (int i = jfda; i <= jlda; ++i) {
        if (gudval[i - 1])
            result[i - 1] = a1[i - 1] / a2[i - 1];
        else
            result[i - 1] = prm::DNOTST;
    }
}

void chkzro(const double* ori, const double* sa, const double* sa2,
            const double* sarnd, const double* ocal, int pos1, int pos2,
            int kfulsm, int iyrt, bool lrndsa, bool* gudval) {
    for (int i = pos1; i <= pos2; ++i) {
        int k = i - 1;
        if (!gudval[k]) continue;
        // .not.(Ori>0 .and. (Kfulsm==0 .and. Sa>0) .and. Ocal>0)
        if (!(ori[k] > 0.0 && (kfulsm == 0 && sa[k] > 0.0) && ocal[k] > 0.0)) {
            gudval[k] = false;
        } else if ((iyrt > 0 || lrndsa) && kfulsm == 0) {
            if (iyrt > 0) {
                if (!(sa2[k] > 0.0)) gudval[k] = false;
            }
            if (lrndsa) {
                if (!(sarnd[k] > 0.0)) gudval[k] = false;
            }
        }
    }
}

// ---- Tier 0: moving-average / Henderson weight kernels ---------------------

void averag(const double* x, double* y, int ib, int ie, int m, int n) {
    int ki = (m + n) / 2 - 1;
    int kb = ib + ki;
    int ke = ie - ki;
    if (ke >= kb) {
        double fmn = static_cast<double>(m * n);
        for (int k = kb; k <= ke; ++k) {
            double tmp = 0.0;
            int i1 = k - ki;
            int i2 = i1 + m - 1;
            for (int i = i1; i <= i2; ++i) {
                int ji = i + n - 1;
                for (int j = i; j <= ji; ++j) tmp += x[j - 1];
            }
            y[k - 1] = tmp / fmn;
        }
    }
}

void hender(double* w, int n) {
    const double ONE = 1.0, THREE = 3.0, FOUR = 4.0, NINE = 9.0;
    double y = static_cast<double>((n + 3) / 2);
    int m = (n + 1) / 2;
    double y1 = (y - ONE) * (y - ONE);
    double y2 = y * y;
    double y3 = (y + ONE) * (y + ONE);
    double y4 = THREE * y2 - 16.0;
    double y5 = FOUR * y2;
    double denomi =
        8.0 * y * (y2 - ONE) * (y5 - ONE) * (y5 - NINE) * (y5 - 25.0) / 315.0;
    for (int i = 1; i <= m; ++i) {
        double x = static_cast<double>((i - 1) * (i - 1));
        w[i - 1] = (y1 - x) * (y2 - x) * (y3 - x) * (y4 - 11.0 * x) / denomi;
    }
}

double apply(const double* x, int k, const double* w, int n) {
    int m = (n + 1) / 2;
    double r = w[0] * x[k - 1];
    for (int i = 2; i <= m; ++i) {
        int j = k - i + 1;
        int l = k + i - 1;
        r += w[i - 1] * (x[j - 1] + x[l - 1]);
    }
    return r;
}

// ---- Tier 1: Henderson end filters -----------------------------------------

void hndend(int m, int nterm, const double* w, double* endwt, double r) {
    const double TWO = 2.0;
    double cw[PMXHND];
    int n = (nterm + 1) / 2;
    int j = 1;
    for (int i = n; i >= 2; --i) {  // cw(1..n-1) = W(n),W(n-1),...,W(2)
        cw[j - 1] = w[i - 1];
        ++j;
    }
    j = 1;
    for (int i = n; i <= nterm; ++i) {  // cw(n..nterm) = W(1..)
        cw[i - 1] = w[j - 1];
        ++j;
    }
    double u1 = 0.0, u2 = 0.0;
    for (int jj = m + 1; jj <= nterm; ++jj) {
        u1 += cw[jj - 1];
        u2 += (jj - static_cast<double>(m + 1) / TWO) * cw[jj - 1];
    }
    for (int i = 1; i <= m; ++i) {
        double c1 = (i - (static_cast<double>(m + 1) / TWO)) * r;
        double c2 =
            1.0 + (static_cast<double>(m * (m - 1) * (m + 1)) / 12.0) * r;
        endwt[i - 1] = cw[i - 1] + (u1 / static_cast<double>(m)) + (u2 * (c1 / c2));
    }
}

void ends(double* stc, const double* stci, int ib, int ie, int k,
          double rbeta) {
    double wtcntr[PMXHN2];
    double wtend[PMXHN1];
    hender(wtcntr, k);
    int m = k - 1;
    int l = m / 2;
    int lm = (k + 1) / 2;
    for (int i = 1; i <= l; ++i) {
        stc[(ib + i - 1) - 1] = 0.0;
        stc[(ie - i + 1) - 1] = 0.0;
        int n = lm + i - 1;
        hndend(n, k, wtcntr, wtend, rbeta);
        for (int j = 1; j <= n; ++j) {
            stc[(ib + i - 1) - 1] += wtend[(n - j + 1) - 1] * stci[(ib + j - 1) - 1];
            stc[(ie - i + 1) - 1] += wtend[(n - j + 1) - 1] * stci[(ie - j + 1) - 1];
        }
    }
}

void endsf(const double* simon, double* savg, int k, const double* w,
           int nend) {
    int kk = 0;
    int jj = 1;
    int j1 = jj;
    int j2 = k;
    while (jj <= nend && j1 <= j2) {
        int jk = jj + nend;
        if (jk > k) {
            savg[j1 - 1] = totals(simon, 1, k, 1, 1);
            if (j1 != j2) savg[j2 - 1] = savg[j1 - 1];
        } else {
            savg[j1 - 1] = 0.0;
            savg[j2 - 1] = 0.0;
            double sumwt = 0.0;
            for (int l = 1; l <= jk; ++l) {
                savg[j1 - 1] += w[(kk + l) - 1] * simon[l - 1];
                if (j1 != j2)
                    savg[j2 - 1] += w[(kk + l) - 1] * simon[(k - l + 1) - 1];
                sumwt += w[(kk + l) - 1];
            }
            // Faithful to the oracle: both divisions are unconditional, so when
            // j1==j2 (odd K, centre point) the single element is divided twice.
            savg[j1 - 1] /= sumwt;
            savg[j2 - 1] /= sumwt;
        }
        kk += jk;
        ++jj;
        j1 = jj;
        j2 = k - jj + 1;
    }
}

void hndtrn(double* stc, const double* stci, int lfda, int lldaf, int nterm,
            double& tic, bool lend, bool lsame, bool tru7hn) {
    const double PI = 3.14159265358979;
    double w[PMXHN2];
    int ib = lfda + nterm / 2;
    int ie = lldaf - nterm / 2;
    if (!lsame) {
        hender(w, nterm);
        for (int i = ib; i <= ie; ++i) stc[i - 1] = apply(stci, i, w, nterm);
        if (!lend) return;
    }
    int i = nterm;
    while (i == 7 && !tru7hn) {
        i -= 2;
        hender(w, i);
        --ib;
        ++ie;
        stc[ib - 1] = apply(stci, ib, w, i);
        stc[ie - 1] = apply(stci, ie, w, i);
        tic = 0.001;
    }
    double rbeta = 4.0 / (tic * tic * PI);
    ends(stc, stci, lfda, lldaf, i, rbeta);
}

}  // namespace x13
