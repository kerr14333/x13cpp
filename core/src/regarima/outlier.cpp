// outlier.cpp -- automatic outlier identification leaves (see outlier.hpp).
// Faithful ports of makotl.f and ttest.f; 1-based Fortran index arithmetic is
// preserved with -1 offsets. Type-indexed arrays (ltest/ltstpt/propt/snglr/
// mxcol) follow the caller's contiguous [AO,LS,TC] slice convention: element
// (type) lives at index type-1 (prm::AO-1 = 0, prm::LS-1 = 1, prm::TC-1 = 2).
#include "regarima/outlier.hpp"

#include <cmath>
#include <vector>

#include "numeric/numeric.hpp"  // ddot, dpmpar
#include "gen/model.hpp"        // prm::PB, POTLR, AO, LS, TC

namespace x13 {

// makotl.f -- build the AO/LS/TC regressor(s) for time t0. The active types are
// packed interleaved into otlvar with stride notlr; dsp gives each type's offset
// back from the block end. TC decays geometrically by tcalfa after t0.
void makotl(int t0, int nr, const int* ltest, double* otlvar, int& notlr,
            double tcalfa, int /*sp*/) {
    constexpr double ONE = 1.0, MONE = -1.0, ZERO = 0.0;
    const int AO = prm::AO, LS = prm::LS, TC = prm::TC;
    // dsp(AO)=dsp[AO-1], dsp(LS)=dsp[LS-1); length POTLR-1 (no TC slot).
    int dsp[prm::POTLR - 1] = {0, 0};
    if (ltest[TC - 1] == 1) {
        dsp[AO - 1]++;
        dsp[LS - 1]++;
    }
    if (ltest[LS - 1] == 1) dsp[AO - 1]++;
    notlr = dsp[AO - 1];
    if (ltest[AO - 1] == 1) notlr++;

    // Observations before t0.
    for (int i = notlr; i <= notlr * (t0 - 1); i += notlr) {
        if (ltest[AO - 1] == 1) otlvar[i - dsp[AO - 1] - 1] = ZERO;
        if (ltest[LS - 1] == 1) otlvar[i - dsp[LS - 1] - 1] = MONE;
        if (ltest[TC - 1] == 1) otlvar[i - 1] = ZERO;
    }
    // Observation t0.
    int i = t0 * notlr;
    if (ltest[AO - 1] == 1) otlvar[i - dsp[AO - 1] - 1] = ONE;
    if (ltest[LS - 1] == 1) otlvar[i - dsp[LS - 1] - 1] = ZERO;
    if (ltest[TC - 1] == 1) otlvar[i - 1] = ONE;
    // Observations after t0.
    for (i = i + notlr; i <= nr * notlr; i += notlr) {
        if (ltest[AO - 1] == 1) otlvar[i - dsp[AO - 1] - 1] = ZERO;
        if (ltest[LS - 1] == 1) otlvar[i - dsp[LS - 1] - 1] = ZERO;
        if (ltest[TC - 1] == 1) otlvar[i - 1] = otlvar[i - notlr - 1] * tcalfa;
    }
}

// ttest.f -- proportional outlier t-statistics via an augmented Cholesky update
// of the estimated regression. For each active type it forms o'o, X'o and y'o
// against the filtered design, solves L*l = X'o, and returns b/se(b)*mse =
// (o'y - l'w)/sqrt(o'o - l'l).
void ttest(const double* xy, int nspobs, int ncxy, const double* chlxpx,
           const double* otlvar, const int* ltstpt, int* mxcol, double* propt,
           bool* snglr) {
    constexpr double ZERO = 0.0;
    const int POTLR = prm::POTLR;
    for (int t = 0; t < POTLR; ++t) snglr[t] = false;

    const int nb = ncxy - 1;
    const int neltxy = nspobs * ncxy;
    const int nxpx = nb * ncxy / 2;
    const double xl = std::sqrt(dpmpar(2));

    // Active types (otype[k] holds the 1-based type value).
    int otype[prm::POTLR];
    int notlr = 0;
    for (int i = 1; i <= POTLR; ++i)
        if (ltstpt[i - 1] == 1) otype[notlr++] = i;

    const int lstride = ncxy + 1;  // l[k][j], j in 1..ncxy
    std::vector<double> oomll(notlr, 0.0), oymlw(notlr, 0.0), tmp(notlr, 0.0);
    std::vector<double> l(static_cast<std::size_t>(notlr) * lstride, 0.0);
    std::vector<double> l1(lstride, 0.0);

    // o'o per type (underflow-skipping), interleaved cycle over notlr.
    int i2 = 0;
    for (int ielt = 1; ielt <= notlr * nspobs; ++ielt) {
        if (std::fabs(otlvar[ielt - 1]) > xl)
            oomll[i2] += otlvar[ielt - 1] * otlvar[ielt - 1];
        if (++i2 >= notlr) i2 = 0;
    }

    // [X:y]'o : column j dotted (strided by ncxy) against the interleaved otlvar.
    for (int j = 1; j <= ncxy; ++j) {
        for (int k = 0; k < notlr; ++k) tmp[k] = 0.0;
        int i = 1;
        for (int ielt = j; ielt <= neltxy; ielt += ncxy) {
            for (int k = 0; k < notlr; ++k) {
                tmp[k] += xy[ielt - 1] * otlvar[i - 1];
                ++i;
            }
        }
        for (int k = 0; k < notlr; ++k) l[k * lstride + j] = tmp[k];
    }

    // Solve L*l = X'o, accumulating o'y - l'w and o'o - l'l.
    for (int k = 0; k < notlr; ++k) oymlw[k] = l[k * lstride + ncxy];
    int ielt = 0;
    for (int i = 1; i <= nb; ++i) {
        for (int k = 0; k < notlr; ++k) {
            for (int j = 1; j <= ncxy; ++j) l1[j] = l[k * lstride + j];
            tmp[k] = l[k * lstride + i] - ddot(i - 1, &chlxpx[ielt], 1, &l1[1], 1);
        }
        ielt += i;
        for (int k = 0; k < notlr; ++k) {
            tmp[k] = tmp[k] / chlxpx[ielt - 1];
            l[k * lstride + i] = tmp[k];
            oymlw[k] -= chlxpx[nxpx + i - 1] * tmp[k];
            oomll[k] -= tmp[k] * tmp[k];
        }
    }

    // b/se(b)*mse per type; flag singular (o'o - l'l <= 0).
    for (int k = 0; k < notlr; ++k) {
        if (oomll[k] <= ZERO) {
            snglr[otype[k] - 1] = true;
            propt[otype[k] - 1] = ZERO;
        } else {
            propt[otype[k] - 1] = oymlw[k] / std::sqrt(oomll[k]);
        }
    }

    // Rank the types by |t|, largest first (insertion sort).
    mxcol[0] = otype[0];
    if (notlr > 1) {
        std::vector<double> tv(notlr);
        tv[0] = propt[otype[0] - 1];
        for (int i = 2; i <= notlr; ++i) {
            tv[i - 1] = propt[otype[i - 1] - 1];
            mxcol[i - 1] = otype[i - 1];
            int j = i - 1;
            bool lgo = true;
            while (lgo && j > 0) {
                if (std::fabs(tv[j - 1]) < std::fabs(tv[j])) {
                    tv[j] = tv[j - 1];
                    mxcol[j] = mxcol[j - 1];
                    tv[j - 1] = propt[otype[i - 1] - 1];
                    mxcol[j - 1] = otype[i - 1];
                } else {
                    lgo = false;
                }
                --j;
            }
        }
    }
}

// coladd.f -- make room for naddc columns at begcol by shifting the trailing
// columns right, walking rows from the last backward so the in-place moves do
// not clobber unread data. Row-major xy with leading dimension ncxy; 1-based
// Fortran index arithmetic preserved (-1 on the C++ accesses).
void coladd(int begcol, int endcol, int nrxy, int /*peltxy*/, double* xy,
            int& ncxy) {
    int naddc = endcol - begcol + 1;
    int nnewc = ncxy + naddc;
    int offset = nrxy * naddc;
    int iend = nrxy * ncxy;
    int ibeg = iend - ncxy + begcol;
    for (int j = iend; j >= ibeg; --j) xy[j + offset - 1] = xy[j - 1];
    for (int i = nrxy - 1; i >= 1; --i) {
        offset = i * naddc;
        iend = ibeg - 1;
        ibeg = iend - ncxy + 1;
        for (int j = iend; j >= ibeg; --j) xy[j + offset - 1] = xy[j - 1];
    }
    ncxy = nnewc;
}

}  // namespace x13
