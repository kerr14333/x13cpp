// x11seas.cpp -- X-11 Tier 2 seasonal moving-average chain (see x11seas.hpp).
// Faithful ports of the vendored oracle Fortran: fis.f, vsfc.f, vsfa.f, vsfb.f.
//
// Index convention: 0-based C pointers, Fortran index i -> element [i-1]; ranges
// (lfda/llda, nyr, ...) stay Fortran 1-based. Loop order and float op order are
// preserved verbatim (the seasonal-MA weights are parity-sensitive; no algebraic
// simplification). COMMON-block state is passed explicitly. ALL WRITE deferred.
#include "x11/x11seas.hpp"

#include "x11/x11filt.hpp"       // averag, divsub, endsf
#include "numeric/numeric.hpp"   // totals, dpeq

#include <cmath>

namespace x13 {

namespace {
// srslen.prm: PYRS = PYR1(65) + 20 = 85. vsfa/vsfb per-period scratch is
// dimensioned PYRS+6 (simon/savg/stimon); +1 for the 1-based slack.
constexpr int PYRS = 85;
constexpr int PSCR = PYRS + 6 + 2;  // comfortable 1-based scratch bound
}  // namespace

// fis.f -- I/S-ratio "number of years" correction. c(N-1)/c(N+3) lookup for
// N<6, closed-form asymptotics otherwise. Cs returns the paired SBAR factor.
double fis(double& cs, int n) {
    static const double c[8] = {1.00000, 1.02584, 1.01779, 1.01383,
                                1.00000, 3.00000, 1.55291, 1.30095};
    if (n < 6) {
        cs = c[n + 3 - 1];  // Cs = c(N+3)
        return c[n - 1 - 1];  // fis = c(N-1)
    }
    cs = static_cast<double>(n) * 1.732051 /
         (8.485281 + static_cast<double>(n - 6) * 1.732051);
    return static_cast<double>(n) * 12.247449 /
           (73.239334 + static_cast<double>(n - 6) * 12.247449);
}

// vsfc.f -- center the seasonals via a 2xNyr MA, filling the k=Nyr/2 end terms
// by repeating the same-period (Lter==5) or nearest MA value, then divsub.
void vsfc(double* sts, int lfda, int llda, int nyr, const int* lter,
          double* temp, int muladd) {
    averag(sts, temp, lfda, llda, 2, nyr);
    int k = nyr / 2;
    int kfda = lfda + k;
    int klda = llda - k;
    int k1 = kfda % nyr;
    for (int i = 1; i <= k; ++i) {
        k1 = k1 - 1;
        if (k1 <= 0) k1 = nyr + k1;
        if (lter[k1 - 1] == 5)
            temp[(kfda - i) - 1] = temp[(kfda - i + nyr) - 1];
        else
            temp[(kfda - i) - 1] = temp[kfda - 1];
    }
    k1 = klda % nyr;
    for (int i = 1; i <= k; ++i) {
        k1 = k1 + 1;
        if (k1 > nyr) k1 = 1;
        if (lter[k1 - 1] == 5)
            temp[(klda + i) - 1] = temp[(klda + i - nyr) - 1];
        else
            temp[(klda + i) - 1] = temp[klda - 1];
    }
    divsub(sts, sts, temp, lfda, llda, muladd);
}

// vsfa.f -- preliminary seasonal MA + I/S ratios (global MSR ratis).
void vsfa(const double* stsi, int lfda, int llda, int nyr, int muladd,
          bool psuadd, double* rati, double& ratis) {
    const double ZERO = 0.0;
    double simon[PSCR], savg[PSCR], stimon[PSCR];
    double cs = 0.0;
    int kfda = lfda + nyr - 1;
    ratis = 999.99;
    double r1 = ZERO;
    double r2 = ZERO;
    int ki = 0;
    for (int j = lfda; j <= kfda; ++j) {
        int m = j - (j - 1) / nyr * nyr;
        int k = 3;
        for (int i = j; i <= llda; i += nyr) {
            ++k;
            simon[k - 1] = stsi[i - 1];
        }
        double tmp1 = (simon[4 - 1] + simon[5 - 1] + simon[6 - 1]) / 3.0;
        double tmp2 = (simon[k - 1] + simon[k - 1 - 1] + simon[k - 2 - 1]) / 3.0;
        for (int i = 1; i <= 3; ++i) {
            ki = k + i;
            simon[i - 1] = tmp1;
            simon[ki - 1] = tmp2;
        }
        averag(simon, savg, 1, ki, 1, 7);
        rati[m - 1] = ZERO;
        rati[m + nyr - 1] = ZERO;
        rati[m + 2 * nyr - 1] = 999.99;
        if (psuadd) {
            for (int i = 4; i <= k; ++i)
                stimon[i - 1] = simon[i - 1] - savg[i - 1] + 1.0;
        } else {
            divsub(stimon, simon, savg, 4, k, muladd);
        }
        int n = k - 4;
        if (muladd < 1) {
            for (int i = 5; i <= k; ++i) {
                rati[m - 1] += std::fabs(stimon[i - 1] - stimon[i - 2]) /
                               stimon[i - 2];
                rati[m + nyr - 1] +=
                    std::fabs(savg[i - 1] - savg[i - 2]) / savg[i - 2];
            }
            rati[m - 1] = rati[m - 1] * 100.0 * fis(cs, n);
            rati[m + nyr - 1] = rati[m + nyr - 1] * 100.0 * cs;
        } else {
            for (int i = 5; i <= k; ++i) {
                rati[m - 1] +=
                    std::fabs(stimon[i - 1] - stimon[i - 2]) * fis(cs, n);
                rati[m + nyr - 1] += std::fabs(savg[i - 1] - savg[i - 2]) * cs;
            }
        }
        r1 = r1 + rati[m - 1];
        r2 = r2 + rati[m + nyr - 1];
        if (!dpeq(rati[m + nyr - 1], ZERO)) {
            if (rati[m - 1] <= 999.0 * rati[m + nyr - 1])
                rati[m + nyr * 2 - 1] = rati[m - 1] / rati[m + nyr - 1];
        }
        double fk = static_cast<double>(n);
        rati[m - 1] = rati[m - 1] / fk;
        rati[m + nyr - 1] = rati[m + nyr - 1] / fk;
    }
    if (r1 <= 999.0 * r2 && !dpeq(r2, ZERO)) ratis = r1 / r2;
    if (muladd == 2) {
        for (int i = 1; i <= nyr; ++i) {
            rati[i - 1] = 100.0 * rati[i - 1];
            rati[i + nyr - 1] = 100.0 * rati[i + nyr - 1];
        }
    }
}

// vsfb.f -- seasonal MA selector (3x3 / 3x5 / 3x9 / 3x15 / stable / 3-term) then
// vsfc centering. w9 / w15 are the 3x9 / 3x15 end-weight tables (verbatim).
void vsfb(double* sts, const double* stsi, int lfda, int llda, int nyr,
          int lterm, const int* lter, int ksect, bool shrtsf, double* temp,
          int muladd, int* pmtype) {
    static const double w9[40] = {
        0.246, 0.221, 0.197, 0.173, 0.112, 0.051,
        0.208, 0.192, 0.176, 0.160, 0.144, 0.092, 0.028,
        0.173, 0.163, 0.154, 0.143, 0.133, 0.123, 0.079, 0.032,
        0.141, 0.137, 0.132, 0.128, 0.123, 0.117, 0.113, 0.075, 0.034,
        0.084, 0.120, 0.118, 0.117, 0.116, 0.114, 0.113, 0.111, 0.073, 0.034};
    static const double w15[100] = {
        .16000, .16000, .16000, .16000, .16000, .06667, .06667, .04444, .02222,
        .14667, .14667, .14667, .14667, .14667, .06667, .06667, .06667, .04444,
        .02220,
        .13333, .13333, .13333, .13333, .13333, .06667, .06667, .06667, .06667,
        .04444, .02223,
        .12000, .12000, .12000, .12000, .12000, .06667, .06667, .06667, .06667,
        .06667, .04444, .02221,
        .10667, .10667, .10667, .10667, .10667, .06667, .06667, .06667, .06667,
        .06667, .06667, .04444, .02219,
        .09333, .09333, .09333, .09333, .09333, .06667, .06667, .06667, .06667,
        .06667, .06667, .06667, .04444, .02222,
        .08000, .08000, .08000, .08000, .08000, .06667, .06667, .06667, .06667,
        .06667, .06667, .06667, .06667, .04444, .02220,
        .04889, .07111, .07111, .07111, .07111, .06667, .06667, .06667, .06667,
        .06667, .06667, .06667, .06667, .06667, .04444, .02220};
    double simon[PSCR], savg[PSCR];

    int kfda = lfda + nyr - 1;
    int mtype = lterm + 1;
    if (!shrtsf && (llda - lfda - 5 * nyr + 1) < 0) mtype = 6;
    for (int j = lfda; j <= kfda; ++j) {
        if (((llda - lfda + 1 - 5 * nyr) >= 0) || shrtsf) {
            int jjj = j % nyr;
            if (jjj == 0) jjj = nyr;
            mtype = lter[jjj - 1] + 1;
        }
        if (mtype == 7 || mtype == 1) {
            mtype = 3;
            if (ksect == 1) mtype = 2;
        } else if (mtype == 8) {
            mtype = 7;
        }
        int k = 0;
        for (int i = j; i <= llda; i += nyr) {
            ++k;
            simon[k - 1] = stsi[i - 1];
        }
        if (shrtsf && k == 3 && mtype == 3) mtype = 6;
        if (mtype == 2) {
            // 3x3 moving average.
            averag(simon, savg, 1, k, 3, 3);
            savg[1 - 1] =
                (11.0 * (simon[1 - 1] + simon[2 - 1]) + 5.0 * simon[3 - 1]) /
                27.0;
            savg[k - 1] = (11.0 * (simon[k - 1] + simon[k - 1 - 1]) +
                           5.0 * simon[k - 2 - 1]) /
                          27.0;
            if (k == 3) {
                savg[2 - 1] = (simon[1 - 1] + simon[2 - 1] + simon[3 - 1]) / 3.0;
            } else {
                savg[2 - 1] = (0.7 * (simon[1 - 1] + simon[3 - 1]) +
                               simon[2 - 1] + 0.3 * simon[4 - 1]) /
                              2.7;
                savg[k - 1 - 1] = (0.7 * (simon[k - 1] + simon[k - 2 - 1]) +
                                   simon[k - 1 - 1] + 0.3 * simon[k - 3 - 1]) /
                                  2.7;
            }
        } else if (mtype == 3) {
            // 3x5 moving average.
            averag(simon, savg, 1, k, 3, 5);
            savg[1 - 1] = (17.0 * (simon[1 - 1] + simon[2 - 1] + simon[3 - 1]) +
                           9.0 * simon[4 - 1]) /
                          60.0;
            savg[k - 1] =
                (17.0 * (simon[k - 1] + simon[k - 1 - 1] + simon[k - 2 - 1]) +
                 9.0 * simon[k - 3 - 1]) /
                60.0;
            if (k == 4) {
                savg[2 - 1] =
                    (simon[1 - 1] + simon[2 - 1] + simon[3 - 1] + simon[4 - 1]) /
                    4.0;
                savg[3 - 1] =
                    (simon[1 - 1] + simon[2 - 1] + simon[3 - 1] + simon[4 - 1]) /
                    4.0;
            } else {
                savg[2 - 1] =
                    (15.0 * (simon[1 - 1] + simon[2 - 1] + simon[3 - 1]) +
                     11.0 * simon[4 - 1] + 4.0 * simon[5 - 1]) /
                    60.0;
                savg[k - 1 - 1] =
                    (15.0 * (simon[k - 1] + simon[k - 1 - 1] + simon[k - 2 - 1]) +
                     11.0 * simon[k - 3 - 1] + 4.0 * simon[k - 4 - 1]) /
                    60.0;
            }
            if (k == 5) {
                savg[3 - 1] = (simon[1 - 1] + simon[2 - 1] + simon[3 - 1] +
                               simon[4 - 1] + simon[5 - 1]) /
                              5.0;
            } else if (k > 5) {
                savg[3 - 1] =
                    (9.0 * simon[1 - 1] +
                     13.0 * (simon[2 - 1] + simon[3 - 1] + simon[4 - 1]) +
                     8.0 * simon[5 - 1] + 4.0 * simon[6 - 1]) /
                    60.0;
                savg[k - 2 - 1] =
                    (9.0 * simon[k - 1] +
                     13.0 * (simon[k - 1 - 1] + simon[k - 2 - 1] +
                             simon[k - 3 - 1]) +
                     8.0 * simon[k - 4 - 1] + 4.0 * simon[k - 5 - 1]) /
                    60.0;
            }
        } else if (mtype == 4) {
            // 3x9 moving average + end weights.
            averag(simon, savg, 1, k, 3, 9);
            endsf(simon, savg, k, w9, 5);
        } else if (mtype == 5 && k >= 20) {
            // 3x15 moving average + end weights.
            averag(simon, savg, 1, k, 3, 15);
            endsf(simon, savg, k, w15, 8);
        } else if (mtype == 6 || mtype == 5) {
            // Stable seasonal: average of all SI ratios for this period.
            double tmp1 = totals(simon, 1, k, 1, 1);
            for (int i = 1; i <= k; ++i) savg[i - 1] = tmp1;
        } else if (mtype == 7) {
            // 3-term moving average.
            averag(simon, savg, 1, k, 1, 3);
            savg[1 - 1] = 0.61 * simon[1 - 1] + 0.39 * simon[2 - 1];
            savg[k - 1] = 0.61 * simon[k - 1] + 0.39 * simon[k - 1 - 1];
        }
        k = 0;
        for (int i = j; i <= llda; i += nyr) {
            ++k;
            sts[i - 1] = savg[k - 1];
        }
    }
    if (pmtype) *pmtype = mtype;  // export final Mtype for shrink (vsfb.f common)
    vsfc(sts, lfda, llda, nyr, lter, temp, muladd);
}

}  // namespace x13
