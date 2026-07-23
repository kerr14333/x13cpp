// shrink.f / glbshk.f / locshk.f / lkshnk.f -- global & local shrinkage of the
// X-11 seasonal factors (Miller & Williams 2003). Faithful transcription; the
// oracle keeps Pos1ob/Posfob/Pos1bk/Posffc in the x11ptr common, passed here as
// explicit 1-based window bounds. All arrays are indexed arr[i-1] to mirror the
// Fortran 1-based DIMENSIONs.
#include "x11/shrink.hpp"

#include <cmath>

#include "specparse/specparse.hpp"  // setdp, copy

namespace x13 {

namespace {

constexpr int PSP = 12;    // srslen.prm
constexpr int PLEN = 1020;
constexpr int PYRS = 85;

// lkshnk.f: Gaussian likelihood of S1 given true mean S2, variance Sig.
double lkshnk(double s1, double s2, double sig) {
    constexpr double PI = 3.14159265358979;
    return (1.0 / std::sqrt(sig * 2.0 * PI)) *
           std::exp((-1.0 / 2.0) * (((s1 - s2) * (s1 - s2)) / sig));
}

// glbshk.f: global shrinkage estimator -- one shrinkage weight per year.
void glbshk(double* sts, double v, int ny, int muladd, int pos1ob, int posfob,
            int pos1bk, int posffc) {
    double avec[PYRS], wvec[PYRS], temps[PLEN];
    setdp(0.0, PYRS, avec);
    setdp(0.0, PYRS, wvec);
    double sfmu = (muladd == 1) ? 0.0 : 1.0;

    // Copy seasonal factors into temps, aligned so temps(it1) == Sts(Pos1ob).
    int it1 = pos1ob % ny;
    if (it1 == 0) it1 = ny;
    int nt = posfob - pos1ob + 1;
    copy(sts + (pos1ob - 1), nt, 1, temps + (it1 - 1));

    // Fill the incomplete first year with backcasts (or the same period a year
    // on) so temps starts on a period-1 boundary.
    if (it1 > 1) {
        for (int i = 1; i <= it1 - 1; ++i) {
            int ipi = pos1ob - i;
            int imi = it1 - i;
            if (ipi >= pos1bk)
                temps[imi - 1] = sts[ipi - 1];
            else
                temps[imi - 1] = sts[ipi + ny - 1];
        }
    }
    int iend = it1 + (posfob - pos1ob);
    int it2 = posfob % ny;
    if (it2 > 0) {
        for (int i = 1; i <= ny - it2; ++i) {
            int ipi = posfob + i;
            int imi = iend + i;
            if (ipi <= posffc)
                temps[imi - 1] = sts[ipi - 1];
            else
                temps[imi - 1] = sts[ipi - ny - 1];
        }
        iend = iend + ny - it2;
    }

    int kyr = iend / ny;
    double dny = static_cast<double>(ny);

    for (int k = 1; k <= kyr; ++k) {
        int i1 = (k - 1) * ny + 1;
        int i2 = k * ny;
        for (int i = i1; i <= i2; ++i)
            avec[k - 1] += (sts[i - 1] - sfmu) * (sts[i - 1] - sfmu);
        avec[k - 1] = (avec[k - 1] / (dny - 1.0)) - v;
        if (avec[k - 1] < 0.0) avec[k - 1] = 0.0;
    }

    double corr = static_cast<double>(ny - 3) / static_cast<double>(ny - 1);
    for (int j = 1; j <= kyr; ++j)
        wvec[j - 1] = corr * v / (v + avec[j - 1]);

    if (it2 > 0) iend = iend - ny + it2;
    for (int k = 1; k <= kyr; ++k) {
        int i1 = (k - 1) * ny + 1;
        if (i1 < it1) i1 = it1;
        int i2 = k * ny;
        if (i2 > iend) i2 = iend;
        for (int i = i1; i <= i2; ++i) {
            int j = i + (pos1ob - it1);
            sts[j - 1] = temps[i - 1] * (1.0 - wvec[k - 1]) + wvec[k - 1];
        }
    }
}

// locshk.f: local shrinkage estimator -- moving-likelihood weights per period.
void locshk(double* sts, double v, int ny, int pos1ob, int posfob, int posffc) {
    double lmat[PLEN][PSP], temps[PLEN], w[PSP][PSP];
    int ny2 = ny / 2;
    int i1 = pos1ob + ny2;
    int iend = posfob;

    copy(sts, posffc, 1, temps);

    // Moving likelihoods for i1..iend, normalized within each window.
    for (int i = i1; i <= iend; ++i) {
        double lsum = 0.0;
        for (int j = 1; j <= ny; ++j) {
            int j2 = i + j - (ny2 + 1);
            lmat[i - 1][j - 1] = lkshnk(temps[i - 1], temps[j2 - 1], v);
            lsum += lmat[i - 1][j - 1];
        }
        for (int j = 1; j <= ny; ++j)
            lmat[i - 1][j - 1] = lmat[i - 1][j - 1] / lsum;
    }

    setdp(0.0, PSP * PSP, &w[0][0]);

    // Average the normalized likelihoods per calendar period.
    for (int i = i1; i <= i1 + ny - 1; ++i) {
        int it1 = i % ny;
        if (it1 == 0) it1 = ny;
        double dny = 0.0;
        for (int i2 = i; i2 <= iend; i2 += ny) {
            dny += 1.0;
            for (int k = 1; k <= ny; ++k)
                w[it1 - 1][k - 1] += lmat[i2 - 1][k - 1];
        }
        for (int k = 1; k <= ny; ++k)
            w[it1 - 1][k - 1] = w[it1 - 1][k - 1] / dny;
    }

    setdp(0.0, PLEN, temps);

    // Local seasonals: weighted sum over the surrounding window.
    int it1 = pos1ob % ny;
    if (it1 == 0) it1 = ny;
    for (int i = pos1ob; i <= posfob; ++i) {
        for (int j = -ny2; j <= ny2 - 1; ++j) {
            int ij = i + j;
            if (ij < pos1ob) ij = ny + ij;
            temps[i - 1] += sts[ij - 1] * w[it1 - 1][j + ny2 + 1 - 1];
        }
        it1 = it1 + 1;
        if (it1 > ny) it1 = it1 - ny;
    }
    for (int i = pos1ob; i <= posfob; ++i) sts[i - 1] = temps[i - 1];
}

}  // namespace

void shrink(const double* stsi, double* sts, int mtype, int ishrnk, int muladd,
            int ny, int pos1ob, int posfob, int pos1bk, int posffc) {
    double varsts[PSP];
    setdp(0.0, PSP, varsts);
    double wtx11 = 0.0, varmu = 0.0;

    // Variance of the SI ratios (per-period for global, pooled for local).
    if (ishrnk == 1) {
        for (int i = pos1ob; i <= pos1ob + ny - 1; ++i) {
            double dn = 0.0;
            int i2 = i % ny;
            if (i2 == 0) i2 = ny;
            for (int j = i; j <= posfob; j += ny) {
                dn += 1.0;
                double sssf = (stsi[j - 1] - sts[j - 1]) * (stsi[j - 1] - sts[j - 1]);
                varsts[i2 - 1] += sssf;
            }
            varsts[i2 - 1] = varsts[i2 - 1] / (dn - 1.0);
            varmu += varsts[i2 - 1];
        }
        varmu = varmu / static_cast<double>(ny);
    } else {
        for (int i = pos1ob; i <= posfob; ++i)
            varmu = (stsi[i - 1] - sts[i - 1]) * (stsi[i - 1] - sts[i - 1]) + varmu;
        varmu = varmu / static_cast<double>(posfob - pos1ob + 1 - ny);
    }

    // Weight from the seasonal-filter length. NB: lx11 has 5 entries; the oracle
    // indexes lx11(Mtype) for any Mtype!=7, so a stable filter (Mtype==6) reads
    // past the array -- a latent Census bug not reachable on the gated paths
    // (default MSR filters give Mtype in {2,3,4,5}).
    static const int lx11[5] = {1, 3, 5, 9, 15};
    if (mtype == 7) {
        wtx11 = 1.0 / 3.0;
    } else {
        double dlx11 = static_cast<double>(lx11[mtype - 1]);
        double den = (dlx11 * 3.0) * (dlx11 * 3.0);
        wtx11 = 10.0 / den;
        for (int j = 1; j <= lx11[mtype - 1] - 2; ++j) wtx11 += 9.0 / den;
    }
    double v = wtx11 * varmu;

    if (ishrnk == 1)
        glbshk(sts, v, ny, muladd, pos1ob, posfob, pos1bk, posffc);
    else
        locshk(sts, v, ny, pos1ob, posfob, posffc);
}

}  // namespace x13
