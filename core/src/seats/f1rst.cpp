// f1rst.cpp -- F1RST + isCloseTD (AR-root allocation to canonical
// components). Faithful port of oracle/fortran/sigsub.f:29-289. See
// seatsalloc.hpp for the routine-level contract.
//
// Fortran arrays here are 1-based (imz(1..p) etc.); the incoming pointers
// are ordinary 0-based C arrays, so every Fortran X(i) becomes X[i-1] --
// same convention as poly.cpp/factor.cpp.
#include "seats/seatsalloc.hpp"
#include "seats/seatspoly.hpp"

#include <cmath>

namespace x13 {

// isCloseTD(w,mq) -- sigsub.f:257.
bool is_close_td(double w, int mq) {
    if (mq == 12) {
        // The oracle also computes Wtd1/Dist1/Dist2 here but the live test
        // (sigsub.f:274-275) uses the literal bounds below instead -- those
        // locals are dead in the oracle too; omitted (no numeric effect).
        if (std::fabs(w) < 0.732 * 180.0 && std::fabs(w) > 0.680556 * 180.0) {
            return true;
        }
        return false;
    } else if (mq == 4) {
        double dist = 0.02 * 180.0;
        double wtd1 = 0.0892 * 180.0;
        if (std::fabs(std::fabs(w) - wtd1) < dist) return true;
        return false;
    }
    return false;
}

// F1RST -- sigsub.f:29.
void f1rst(int p, const double* imz, const double* rez, const double* ar,
           double epsphi, int mq, double* cycns, int& ncycns, double* psins,
           int& npsins, double* cycs, int& ncycs, double* chins, int& nchins,
           double* chis, int& nchis, const double* modul, double* psis,
           int& npsis, double rmod, bool& root0c, bool& rootpic,
           bool& rootpis, bool& is_close_to_td) {
    (void)rootpis;  // never assigned inside F1RST (see sigsub.f); accepted
                     // for signature parity only.
    if (p == 0) return;

    const int ny = 2;
    const double rmods = (npsins > 1 || npsis > 1) ? rmod : 0.9;
    double dum[3];

    // ------------------------------------------------------------------
    // Real-root branch: imz(1) ~ 0 means ALL p roots are real -- the
    // oracle's root finder orders complex pairs first, so a real leading
    // root implies an all-real set (sigsub.f's "TALE CHECK" comment).
    // ------------------------------------------------------------------
    if (imz[0] > -1.0e-13 && imz[0] < 1.0e-13) {
        for (int i = 1; i <= p; ++i) {
            dum[0] = 1.0;
            dum[1] = -rez[i - 1];
            if (rez[i - 1] <= 0.0) {
                if (std::fabs(1.0 + rez[i - 1]) < 1.0e-6) {
                    if (mq == 1) {
                        rootpic = true;
                        conv(dum, 2, cycns, ncycns, cycns, ncycns);
                    } else {
                        rootpic = true;
                        conv(dum, 2, psins, npsins, psins, npsins);
                    }
                } else if (mq == 1) {
                    rootpic = true;
                    conv(dum, 2, cycs, ncycs, cycs, ncycs);
                } else if (std::fabs(rez[i - 1]) >= rmods) {
                    rootpic = true;
                    conv(dum, 2, psis, npsis, psis, npsis);
                } else {
                    rootpic = true;
                    conv(dum, 2, cycs, ncycs, cycs, ncycs);
                }
            } else if (std::fabs(1.0 - rez[i - 1]) < 1.0e-6) {
                conv(dum, 2, chins, nchins, chins, nchins);
            } else if (std::fabs(rez[i - 1]) >= rmod) {
                conv(dum, 2, chis, nchis, chis, nchis);
            } else {
                rootpic = true;
                conv(dum, 2, cycs, ncycs, cycs, ncycs);
            }
        }
        return;
    }

    // ------------------------------------------------------------------
    // Complex branch: classify the leading conjugate pair rez(1)+-i*imz(1).
    // ------------------------------------------------------------------
    dum[0] = 1.0;
    dum[1] = -2.0 * rez[0];
    dum[2] = rez[0] * rez[0] + imz[0] * imz[0];

    if (mq == 1) {
        if (modul[0] > rmod && std::fabs(ar[0]) < 360.0 / (ny * mq)) {
            if (std::fabs(modul[0] - 1.0) < 1.0e-6) {
                conv(dum, 3, chins, nchins, chins, nchins);
            } else {
                conv(dum, 3, chis, nchis, chis, nchis);
            }
        } else {
            if (std::fabs(modul[0] - 1.0) < 1.0e-6) {
                conv(dum, 3, cycns, ncycns, cycns, ncycns);
            } else {
                conv(dum, 3, cycs, ncycs, cycs, ncycs);
            }
        }
    } else {
        // k = DBLE(360/mq): Fortran integer division truncates FIRST, then
        // widens -- must NOT be 360.0/mq (floating divide), or e.g. mq==7
        // would silently diverge from the oracle.
        double k = static_cast<double>(360 / mq);
        if (mq == 12 || mq == 6 || mq == 4 || mq == 2) {
            if (modul[0] > rmod && std::fabs(ar[0]) < 360.0 / (ny * mq)) {
                if (std::fabs(modul[0] - 1.0) < 1.0e-6) {
                    conv(dum, 3, chins, nchins, chins, nchins);
                } else {
                    conv(dum, 3, chis, nchis, chis, nchis);
                }
            } else {
                // neps is computed then unconditionally overwritten to 1 in
                // the oracle (sigsub.f:145-160 -- the guard can never be
                // true here since mq is already one of {12,6,4,2}); kept as
                // a dead store for line-for-line faithfulness.
                int neps = 1;
                if (mq != 12 && mq != 6 && mq != 4 && mq != 3 && mq != 2 &&
                    mq != 1) {
                    neps = 3;
                }
                neps = 1;
                int intocycle = 1;
                for (int i = 1; i <= mq / 2; ++i) {
                    if (mq != 12 || i != 4 || epsphi <= 2.5) {
                        if (std::fabs(ar[0]) > (k * i - neps * epsphi) &&
                            std::fabs(ar[0]) < (k * i + neps * epsphi) &&
                            modul[0] >= rmods) {
                            intocycle = 0;
                        }
                    } else {
                        if (std::fabs(ar[0]) > (k * i - neps * 2.5) &&
                            std::fabs(ar[0]) < (k * i + neps * 2.5) &&
                            modul[0] >= rmods) {
                            intocycle = 0;
                        }
                    }
                }
                if (intocycle == 0) {
                    if (std::fabs(modul[0] - 1.0) < 1.0e-6) {
                        conv(dum, 3, psins, npsins, psins, npsins);
                    } else {
                        conv(dum, 3, psis, npsis, psis, npsis);
                    }
                } else if (std::fabs(modul[0] - 1.0) < 1.0e-6) {
                    is_close_to_td = is_close_td(ar[0], mq);
                    conv(dum, 3, cycns, ncycns, cycns, ncycns);
                } else {
                    is_close_to_td = is_close_td(ar[0], mq);
                    conv(dum, 3, cycs, ncycs, cycs, ncycs);
                }
            }
        } else if (std::fabs(ar[0]) <= k - epsphi ||
                   std::fabs(ar[0]) >= k + epsphi || modul[0] < rmods) {
            if (std::fabs(modul[0] - 1.0) < 1.0e-6) {
                conv(dum, 3, cycns, ncycns, cycns, ncycns);
            } else {
                conv(dum, 3, cycs, ncycs, cycs, ncycs);
            }
        } else if (std::fabs(modul[0] - 1.0) < 1.0e-6) {
            conv(dum, 3, psins, npsins, psins, npsins);
        } else {
            conv(dum, 3, psis, npsis, psis, npsis);
        }
    }

    if (p <= 2) return;

    // ------------------------------------------------------------------
    // Trailing real root at index 3 (after the leading conjugate pair).
    // ------------------------------------------------------------------
    dum[0] = 1.0;
    dum[1] = -rez[2];
    if (rez[2] > 0.0) {
        if (std::fabs(1.0 - rez[2]) < 1.0e-6) {
            conv(dum, 2, chins, nchins, chins, nchins);
            return;
        }
        if (std::fabs(rez[2]) >= rmods) {
            conv(dum, 2, chis, nchis, chis, nchis);
        } else {
            root0c = true;
            conv(dum, 2, cycs, ncycs, cycs, ncycs);
        }
    } else if (std::fabs(1.0 + rez[2]) < 1.0e-6) {
        if (mq == 1) {
            root0c = true;
            conv(dum, 2, cycns, ncycns, cycns, ncycns);
        } else {
            root0c = true;
            conv(dum, 2, psins, npsins, psins, npsins);
        }
    } else {
        if (mq == 1) {
            root0c = true;
            conv(dum, 2, cycs, ncycs, cycs, ncycs);
            return;
        }
        if (std::fabs(rez[2]) >= rmod) {
            root0c = true;
            conv(dum, 2, psis, npsis, psis, npsis);
        } else if (std::fabs(rez[2]) < rmod) {
            root0c = true;
            conv(dum, 2, cycs, ncycs, cycs, ncycs);
        }
    }
}

}  // namespace x13
