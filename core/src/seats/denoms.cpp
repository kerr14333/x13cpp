// denoms.cpp -- seats_init_denoms: nonstationary/stationary denominator setup
// from differencing orders + near-unit seasonal-AR root. Faithful port of
// oracle/fortran/sigex.f:476-540. See seatsdenoms.hpp for the contract.
#include "seats/seatsdenoms.hpp"
#include "seats/seatspoly.hpp"

#include <cmath>

namespace x13 {

void seats_init_denoms(int d, int bd, int bp, const double* bphi, int mq,
                        double* chins, int& nchins, double* chis, int& nchis,
                        double* psins, int& npsins, double* psis, int& npsis,
                        double* cycns, int& ncycns, double* cycs, int& ncycs) {
    // ------------------------------------------------------------------
    // Chins = (1-B)^dplusd, dplusd = d + bd -- a seasonal difference
    // contributes ONE extra (1-B) to the trend denominator (the rest of
    // (1-B^mq) = (1-B)(1+B+...+B^(mq-1)) goes to Psins below).
    // ------------------------------------------------------------------
    chins[0] = 1.0;
    nchins = 1;
    int dplusd = d + bd;
    if (dplusd != 0) {
        for (int i = 1; i <= dplusd; ++i) {
            chins[i] = 0.0;                    // Chins(i+1), 0-based index i
            for (int j = 1; j <= i; ++j) {
                int k = i - j + 2;
                chins[k - 1] -= chins[k - 2];  // Chins(k) -= Chins(k-1)
            }
        }
    }
    nchins = dplusd + 1;

    chis[0] = 1.0;
    nchis = 1;

    // Near-unit seasonal-AR root heuristic: bphi(mq+1) (0-based bphi[mq])
    // holds -Phi for a length-1 seasonal AR factor (1 - Phi*B^mq); cmu is
    // its mq-th root (the per-lag modulus). Reused by the Psins/Psis fold
    // below, so both `cmu` and the branch it took must survive past this
    // block -- matches the oracle, which never recomputes them.
    double cmu = 0.0;
    bool near_unit = false;
    const bool have_near_unit_sar = (bp != 0 && bphi[mq] < 0.0);
    if (have_near_unit_sar) {
        cmu = std::pow(-bphi[mq], 1.0 / mq);
        near_unit = std::fabs(1.0 - cmu) < 1.0e-13;
        double dum[2] = {1.0, -cmu};
        if (near_unit) {
            conv(dum, 2, chins, nchins, chins, nchins);
        } else {
            conv(dum, 2, chis, nchis, chis, nchis);
        }
    }

    // ------------------------------------------------------------------
    // Psins/Psis: the seasonal-summation part of (1-B^mq) (bd folds) plus
    // the near-unit seasonal-AR geometric factor.
    // ------------------------------------------------------------------
    psins[0] = 1.0;
    for (int i = 1; i < 27; ++i) psins[i] = 0.0;
    npsins = 1;
    psis[0] = 1.0;
    npsis = 1;
    if (bd != 0) {
        double dum[64];
        for (int i = 0; i < mq; ++i) dum[i] = 1.0;   // 1+B+...+B^(mq-1)
        conv(dum, mq, psins, npsins, psins, npsins);
        if (bd != 1) {
            conv(dum, mq, psins, npsins, psins, npsins);
        }
    }
    if (have_near_unit_sar) {
        double dum[64];
        dum[0] = 1.0;
        for (int i = 1; i < mq; ++i) dum[i] = cmu * dum[i - 1];  // 1+cmu*B+...
        if (near_unit) {
            conv(dum, mq, psins, npsins, psins, npsins);
        } else {
            conv(dum, mq, psis, npsis, psis, npsis);
        }
    }

    // ------------------------------------------------------------------
    // Cycs/Cycns: a positive (not near-unit) seasonal-AR root goes straight
    // into the (stationary) cycle denominator as the full seasonal AR
    // polynomial; otherwise cycle starts trivial (folded by F1RST later).
    // ------------------------------------------------------------------
    if (bp > 0 && bphi[mq] > 0.0) {
        for (int i = 1; i <= mq + 1; ++i) cycs[i - 1] = bphi[i - 1];
        ncycs = mq + 1;
    } else {
        cycs[0] = 1.0;
        ncycs = 1;
    }
    cycns[0] = 1.0;
    ncycns = 1;
}

}  // namespace x13
