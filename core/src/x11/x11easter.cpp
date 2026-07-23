// x11easter.cpp -- classic X-11 Easter holiday estimation.
// Faithful transcription of the vendored oracle Fortran: chkeas.f, easter.f,
// holidy.f, holday.f, with the kdate.prm Easter/Labor/Thanksgiving date table.
//
// Deterministic binned-average Easter estimator: bins the preliminary irregular
// (Yhol = Sti*100) by the Easter date offset (Xhol, from kdate), trims extremes,
// interpolates the pre/post-Easter effect, removes the March seasonal, and emits
// the holiday factor into X11hol. 1-based Fortran indices -> arr[i-1]. GO TO
// control flow in easter's extreme-trim loop is preserved verbatim.
#include "x11/x11easter.hpp"

#include <cmath>

#include "common/x13context.hpp"

namespace x13 {

namespace {

// kdate.prm: Easter = Mar 22 + KDATE[y][0], Labor Day = Aug 31 + KDATE[y][1],
// Thanksgiving = Nov 21 + KDATE[y][2], for year = 1901+y (y = 0..199).
const int KDATE[200][3] = {
    {16,2,7},{8,1,6},{21,7,5},{12,5,3},{32,4,2},{24,3,1},{9,2,7},{28,7,5},
    {20,6,4},{5,5,3},{25,4,2},{16,2,7},{1,1,6},{21,7,5},{13,6,4},{32,4,2},
    {17,3,1},{9,2,7},{29,1,6},{13,6,4},{5,5,3},{25,4,2},{10,3,1},{29,1,6},
    {21,7,5},{13,6,4},{26,5,3},{17,3,1},{9,2,7},{29,1,6},{14,7,5},{5,5,3},
    {25,4,2},{10,3,1},{30,2,7},{21,7,5},{6,6,4},{26,5,3},{18,4,2},{2,2,0},
    {22,1,-1},{14,7,5},{34,6,4},{18,4,2},{10,3,1},{30,2,7},{15,1,6},{6,6,4},
    {26,5,3},{18,4,2},{3,3,1},{22,1,6},{14,7,5},{27,6,4},{19,5,3},{10,3,1},
    {30,2,7},{15,1,6},{7,7,5},{26,5,3},{11,4,2},{31,3,1},{23,2,7},{7,7,5},
    {27,6,4},{19,5,3},{4,4,2},{23,2,7},{15,1,6},{7,7,5},{20,6,4},{11,4,2},
    {31,3,1},{23,2,7},{8,1,6},{27,6,4},{19,5,3},{4,4,2},{24,3,1},{15,1,6},
    {28,7,5},{20,6,4},{12,5,3},{31,3,1},{16,2,7},{8,1,6},{28,7,5},{12,5,3},
    {4,4,2},{24,3,1},{9,2,7},{28,7,5},{20,6,4},{12,5,3},{25,4,2},{16,2,7},
    {8,1,6},{21,7,5},{13,6,4},{32,4,2},{24,3,1},{9,2,7},{29,1,6},{20,6,4},
    {5,5,3},{25,4,2},{17,3,1},{1,1,6},{21,7,5},{13,6,4},{33,5,3},{17,3,1},
    {9,2,7},{29,1,6},{14,7,5},{5,5,3},{25,4,2},{10,3,1},{30,2,7},{21,7,5},
    {13,6,4},{26,5,3},{18,4,2},{9,2,7},{29,1,6},{14,7,5},{6,6,4},{25,4,2},
    {10,3,1},{30,2,7},{22,1,6},{6,6,4},{26,5,3},{18,4,2},{3,3,1},{22,1,6},
    {14,7,5},{34,6,4},{19,5,3},{10,3,1},{30,2,7},{15,1,6},{7,7,5},{26,5,3},
    {18,4,2},{3,3,1},{23,2,7},{14,7,5},{27,6,4},{19,5,3},{11,4,2},{30,2,7},
    {15,1,6},{7,7,5},{27,6,4},{11,4,2},{31,3,1},{23,2,7},{8,1,6},{27,6,4},
    {19,5,3},{4,4,2},{24,3,1},{15,1,6},{7,7,5},{20,6,4},{12,5,3},{31,3,1},
    {23,2,7},{8,1,6},{28,7,5},{19,5,3},{4,4,2},{24,3,1},{16,2,7},{28,7,5},
    {20,6,4},{12,5,3},{32,4,2},{16,2,7},{8,1,6},{28,7,5},{13,6,4},{4,4,2},
    {24,3,1},{9,2,7},{29,1,6},{20,6,4},{12,5,3},{25,4,2},{17,3,1},{8,1,6},
    {21,7,5},{13,6,4},{33,5,3},{24,3,1},{9,2,7},{29,1,6},{21,7,5},{6,5,3}};

// chkeas.f: count Easters in four date bins over the March column.
void chkeas(X13Context& ctx, int lmar, int llda) {
    int* ieast = ctx.xeastr.ieast.data();
    const double* xhol = ctx.xeastr.xhol.data();
    for (int i = 0; i < 4; ++i) ieast[i] = 0;
    for (int i = lmar; i <= llda; i += 12) {
        double x = xhol[i - 1];
        if (x <= 10.0)
            ieast[0]++;
        else if (x > 24.0)
            ieast[1]++;
        else if (x > 10.0 && x <= 17.0)
            ieast[2]++;
        else
            ieast[3]++;
    }
}

// easter.f: estimate the Easter effect into Yhat (== X11hol). khol=mar index,
// kkhol=marbk, kh2=apr, ihol=Keastr (set 0 if inadmissible).
void easter(X13Context& ctx, double* yhat, int khol, int kkhol, int kh2,
            int llda, int& ihol, int nfcst) {
    const double TWOHND = 200.0, TEN = 10.0, TWO = 2.0, ZERO = 0.0,
                 TWNTY4 = 24.0, ONE = 1.0, SVNTN = 17.0, ELEVEN = 11.0,
                 FOUR = 4.0, TWNTY1 = 21.0, FOURTN = 14.0, ONEHND = 100.0,
                 TWNTY5 = 25.0, SEVEN = 7.0;
    static const double mfreq[35] = {
        0.0100, 0.0150, 0.0050, 0.0175, 0.0300, 0.0325, 0.0250, 0.0300, 0.0300,
        0.0400, 0.0375, 0.0350, 0.0250, 0.0275, 0.0425, 0.0425, 0.0275, 0.0300,
        0.0225, 0.0400, 0.0425, 0.0325, 0.0300, 0.0350, 0.0300, 0.0425, 0.0375,
        0.0350, 0.0300, 0.0250, 0.0350, 0.0300, 0.0100, 0.0100, 0.0100};
    double mfac[35];
    double* yhol = ctx.xeastr.yhol.data();
    const double* xhol = ctx.xeastr.xhol.data();

    int ll = llda;
    if (ctx.xeastr.lgenx) {
        chkeas(ctx, khol, llda);
        const int* ie = ctx.xeastr.ieast.data();
        if (ie[0] * ie[1] * ie[2] * ie[3] == 0) {
            ihol = 0;
            return;
        }
    }
    int mm = ll + nfcst;
    int kk = kkhol;

    // Invert April.
    for (int i = khol; i <= ll - 1; i += 12) yhol[i + 1 - 1] = TWOHND - yhol[i + 1 - 1];

    // Average of irregulars before April 2.
    double summ = ZERO, sum0 = ZERO;
    for (int i = khol; i <= ll; i += 12) {
        if (xhol[i - 1] <= TEN) {
            summ += yhol[i - 1];
            sum0 += ONE;
            if (i < ll) {
                summ += yhol[i + 1 - 1];
                sum0 += ONE;
            }
        }
    }
    summ = summ / sum0;

    // Standard deviation of obs before April 2.
    double varm = ZERO;
    for (int i = khol; i <= ll; i += 12) {
        if (xhol[i - 1] <= TEN) {
            varm = (yhol[i - 1] - summ) * (yhol[i - 1] - summ) + varm;
            if (i < ll) varm = (yhol[i + 1 - 1] - summ) * (yhol[i + 1 - 1] - summ) + varm;
        }
    }
    double sdm = std::sqrt(varm) / std::sqrt(sum0) * TWO;

    // Trim extremes.
    double summe = ZERO;
    int nsum = 0;
    for (int i = khol; i <= ll; i += 12) {
        if (xhol[i - 1] <= TEN) {
            double yl1 = yhol[i - 1];
            double dif = std::fabs(yhol[i - 1] - summ);
            int nsum1 = 1;
            if (dif >= sdm) {
                nsum1 = 0;
                yl1 = ZERO;
            }
            double yl2 = ZERO;
            int nsum2 = 0;
            if (i < ll) {
                dif = std::fabs(yhol[i + 1 - 1] - summ);
                if (dif < sdm) {
                    yl2 = yhol[i + 1 - 1];
                    nsum2 = 1;
                }
            }
            summe = yl1 + yl2 + summe;
            nsum = nsum1 + nsum2 + nsum;
        }
    }
    summ = summe / nsum;

    // Average after April 16.
    double suma = ZERO;
    sum0 = ZERO;
    for (int i = khol; i <= ll; i += 12) {
        if (xhol[i - 1] > TWNTY4) {
            suma += yhol[i - 1];
            sum0 += ONE;
            if (i < ll) {
                suma += yhol[i + 1 - 1];
                sum0 += ONE;
            }
        }
    }
    suma = suma / sum0;

    // Standard deviation of obs after April 16.
    double vara = ZERO;
    for (int i = khol; i <= ll; i += 12) {
        if (xhol[i - 1] > TWNTY4) {
            vara = (yhol[i - 1] - suma) * (yhol[i - 1] - suma) + vara;
            if (i < ll) vara = (yhol[i + 1 - 1] - suma) * (yhol[i + 1 - 1] - suma) + vara;
        }
    }
    double sda = std::sqrt(vara) / std::sqrt(sum0) * TWO;

    // Trim extremes.
    double sumae = ZERO;
    nsum = 0;
    for (int i = khol; i <= ll; i += 12) {
        if (xhol[i - 1] > TWNTY4) {
            double yl1 = yhol[i - 1];
            double dif = std::fabs(yhol[i - 1] - suma);
            int nsum1 = 1;
            if (dif >= sda) {
                nsum1 = 0;
                yl1 = ZERO;
            }
            double yl2 = ZERO;
            int nsum2 = 0;
            if (i < ll) {
                dif = std::fabs(yhol[i + 1 - 1] - suma);
                if (dif < sda) {
                    yl2 = yhol[i + 1 - 1];
                    nsum2 = 1;
                }
            }
            sumae = yl1 + yl2 + sumae;
            nsum = nsum1 + nsum2 + nsum;
        }
    }
    suma = sumae / static_cast<double>(nsum);

    // Bin sums for April 2-8 (suma1) and April 9-15 (suma2).
    double sum1 = ZERO, sum2 = ZERO, suma1 = ZERO, suma2 = ZERO;
    for (int i = khol; i <= ll; i += 12) {
        if (xhol[i - 1] > TEN && xhol[i - 1] < TWNTY5) {
            if (xhol[i - 1] <= SVNTN) {
                suma1 += yhol[i - 1];
                sum1 += ONE;
                if (i < ll) {
                    suma1 += yhol[i + 1 - 1];
                    sum1 += ONE;
                }
            } else {
                suma2 += yhol[i - 1];
                sum2 += ONE;
                if (i < ll) {
                    suma2 += yhol[i + 1 - 1];
                    sum2 += ONE;
                }
            }
        }
    }
    suma1 = suma1 / sum1;
    suma2 = suma2 / sum2;

    // Preliminary fit over March/April.
    double sumd1, sumd2, sumd3;
    for (int i = khol; i <= mm; i += 12) {
        if (xhol[i - 1] < ELEVEN) {
            yhat[i - 1] = summ;
            yhat[i + 1 - 1] = TWOHND - yhat[i - 1];
        } else if (xhol[i - 1] > TWNTY4) {
            yhat[i - 1] = suma;
            yhat[i + 1 - 1] = TWOHND - yhat[i - 1];
        } else if (xhol[i - 1] <= FOURTN) {
            sumd1 = FOURTN - xhol[i - 1];
            yhat[i - 1] = suma1 + sumd1 * (summ - suma1) / FOUR;
        } else if (xhol[i - 1] <= TWNTY1) {
            sumd2 = TWNTY1 - xhol[i - 1];
            yhat[i - 1] = suma2 + sumd2 * (suma1 - suma2) / SEVEN;
        } else {
            sumd3 = TWNTY5 - xhol[i - 1];
            yhat[i - 1] = suma + sumd3 * (suma2 - suma) / FOUR;
        }
    }

    // Standard errors for April 2-8 / 9-15.
    double var28 = ZERO, var915 = ZERO;
    for (int i = khol; i <= ll; i += 12) {
        if (xhol[i - 1] > TEN && xhol[i - 1] < TWNTY5) {
            if (xhol[i - 1] > SVNTN) {
                var915 = (yhol[i - 1] - yhat[i - 1]) * (yhol[i - 1] - yhat[i - 1]) + var915;
                if (i < ll)
                    var915 = (yhol[i + 1 - 1] - yhat[i - 1]) * (yhol[i + 1 - 1] - yhat[i - 1]) + var915;
            } else {
                var28 = (yhol[i - 1] - yhat[i - 1]) * (yhol[i - 1] - yhat[i - 1]) + var28;
                if (i < ll)
                    var28 = (yhol[i + 1 - 1] - yhat[i - 1]) * (yhol[i + 1 - 1] - yhat[i - 1]) + var28;
            }
        }
    }
    double sd28 = std::sqrt(var28) / std::sqrt(sum1) * TWO;
    double sd915 = std::sqrt(var915) / std::sqrt(sum2) * TWO;

    // Trim extremes beyond 2 SE for April 2-8 and 9-15 (verbatim GO TO logic).
    int nsuma = 0, nsuaa = 0, nsuma1 = 0, nsuma2 = 0, nsuaa1 = 0, nsuaa2 = 0;
    double suma1e = ZERO, suma2e = ZERO;
    for (int i = khol; i <= ll; i += 12) {
        double yl1 = yhol[i - 1];
        double yl2 = 0.0;
        if (i < ll) yl2 = yhol[i + 1 - 1];
        if (xhol[i - 1] > TEN && xhol[i - 1] < TWNTY5) {
            if (xhol[i - 1] > SVNTN) {
                // April 9-15.
                double dif2 = std::fabs(yhol[i - 1] - yhat[i - 1]);
                if (dif2 < sd915) {
                    nsuaa1 = 1;
                } else {
                    yl1 = ZERO;
                    nsuaa1 = 0;
                }
                if (i >= ll) goto lbl20;
                dif2 = std::fabs(yhol[i + 1 - 1] - yhat[i - 1]);
                if (dif2 >= sd915) goto lbl20;
                nsuaa2 = 1;
                goto lbl30;
            } else {
                // April 2-8.
                double dif1 = std::fabs(yhol[i - 1] - yhat[i - 1]);
                if (dif1 < sd28) {
                    nsuma1 = 1;
                } else {
                    yl1 = ZERO;
                    nsuma1 = 0;
                }
                if (i < ll) {
                    dif1 = std::fabs(yhol[i + 1 - 1] - yhat[i - 1]);
                    if (dif1 < sd28) {
                        nsuma2 = 1;
                        goto lbl10;
                    }
                }
                yl2 = ZERO;
                nsuma2 = 0;
            }
        lbl10:
            suma1e = yl1 + yl2 + suma1e;
            nsuma = nsuma1 + nsuma2 + nsuma;
        }
        goto lbl40;
    lbl20:
        yl2 = ZERO;
        nsuaa2 = 0;
    lbl30:
        suma2e = yl1 + yl2 + suma2e;
        nsuaa = nsuaa1 + nsuaa2 + nsuaa;
    lbl40:;
    }
    if (nsuma != 0) suma1 = suma1e / static_cast<double>(nsuma);
    if (nsuaa != 0) suma2 = suma2e / static_cast<double>(nsuaa);

    // Recompute the April 2-8 / 9-15 fit with extremes removed.
    for (int i = kk; i <= mm; i += 12) {
        if (xhol[i - 1] > TEN && xhol[i - 1] < TWNTY5) {
            if (xhol[i - 1] <= FOURTN) {
                sumd1 = FOURTN - xhol[i - 1];
                yhat[i - 1] = suma1 + sumd1 * (summ - suma1) / FOUR;
                yhat[i + 1 - 1] = TWOHND - yhat[i - 1];
            } else if (xhol[i - 1] <= TWNTY1) {
                sumd2 = TWNTY1 - xhol[i - 1];
                yhat[i - 1] = suma2 + sumd2 * (suma1 - suma2) / SEVEN;
                yhat[i + 1 - 1] = TWOHND - yhat[i - 1];
            } else {
                sumd3 = TWNTY5 - xhol[i - 1];
                yhat[i - 1] = suma + sumd3 * (suma2 - suma) / FOUR;
                yhat[i + 1 - 1] = TWOHND - yhat[i - 1];
            }
        }
    }

    // Seasonal component of the Easter effect for March.
    double smfac = ZERO;
    for (int i = 0; i <= 34; ++i) {
        if (i <= 10) {
            mfac[i] = summ;
        } else if (i > 10 && i <= 14) {
            sumd1 = FOURTN - i;
            mfac[i] = suma1 + sumd1 * (summ - suma1) / FOUR;
        } else if (i > 14 && i < 21) {
            sumd2 = TWNTY1 - i;
            mfac[i] = suma2 + sumd2 * (suma1 - suma2) / SEVEN;
        } else if (i >= 21 && i <= 24) {
            sumd3 = TWNTY5 - i;
            mfac[i] = suma + sumd3 * (suma2 - suma) / FOUR;
        } else {
            mfac[i] = suma;
        }
        smfac = smfac + (mfreq[i] * mfac[i]);
    }

    // Divide the seasonal effect out of the March / April values.
    for (int i = kk; i <= mm; i += 12) {
        yhat[i - 1] = (yhat[i - 1] * ONEHND) / smfac;
        yhat[i + 1 - 1] = (yhat[i + 1 - 1] * ONEHND) / (TWOHND - smfac);
    }

    if (kh2 == 0) return;
    if (xhol[kh2 - 1] <= TEN) {
        yhat[kh2 + 1 - 1] = TWOHND - summ;
    } else if (xhol[kh2 - 1] > TEN && xhol[kh2 - 1] <= FOURTN) {
        sumd1 = FOURTN - xhol[kh2 - 1];
        yhat[kh2 + 1 - 1] = TWOHND - (suma1 + sumd1 * (summ - suma1) / FOUR);
    } else if (xhol[kh2 - 1] > FOURTN && xhol[kh2 - 1] <= TWNTY1) {
        sumd2 = TWNTY1 - xhol[kh2 - 1];
        yhat[kh2 + 1 - 1] = TWOHND - (suma2 + sumd2 * (suma1 - suma2) / SEVEN);
    } else if (xhol[kh2 - 1] > TWNTY1 && xhol[kh2 - 1] <= TWNTY4) {
        sumd3 = TWNTY5 - xhol[kh2 - 1];
        yhat[kh2 + 1 - 1] = TWOHND - (suma + sumd3 * (suma2 - suma) / FOUR);
    } else {
        yhat[kh2 + 1 - 1] = TWOHND - nsuma;
    }
    yhat[kh2 + 1 - 1] = (yhat[kh2 + 1 - 1] * ONEHND) / (TWOHND - smfac);
}

// holidy.f: generate the Xhol Easter-date indicator (from kdate) and call easter.
void holidy(X13Context& ctx, double* yhat, int nyear, int lfda, int lfbk,
            int lyr, int llda, int numfct, int& keastr, int& khol) {
    double* xhol = ctx.xeastr.xhol.data();
    int m = (lfbk / 12) * 12 + 1;
    if (lfbk % 12 == 0) m = m - 12;

    if (ctx.xeastr.lgenx) {
        int l = lyr - 1900;
        int l2 = l + nyear - 1;
        for (int i = l; i <= l2; ++i)
            for (int j = 1; j <= 3; ++j)
                for (int k = 1; k <= 4; ++k)
                    xhol[(m + (i - l) * 12) + 4 * j - 4 + k - 1] =
                        static_cast<double>(KDATE[i - 1][j - 1]);
    }

    int lfdam = lfda % 12;
    if (lfdam == 0) lfdam = 12;
    int mar, marbk;
    if (lfda == lfbk) {
        mar = 3 + m - 1;
        if (lfdam > 3) mar = mar + 12;
        marbk = mar;
    } else {
        int lfdam2 = lfbk % 12;
        if (lfdam2 == 0) lfdam2 = 12;
        marbk = 3 + m - 1;
        if (lfdam2 > 3) mar = mar + 12;   // verbatim (mar, not marbk)
        m = (lfda / 12) * 12 + 1;
        if (lfda % 12 == 0) m = m - 12;
        mar = 3 + m - 1;
        if (lfdam > 3) mar = mar + 12;
    }
    int apr = 0;
    if (lfdam == 4) apr = 3;

    if (keastr >= 1) {
        // holidy.f:71 passes Keastr by reference into easter; easter.f zeroes it
        // on an inadmissible (any-empty Ieast bin) decomposition, so the global
        // Keastr must clear too -- pass ctx.x11opt.keastr straight through.
        easter(ctx, yhat, mar, marbk, apr, llda, keastr, numfct);
    } else {
        int* ie = ctx.xeastr.ieast.data();
        for (int i = 0; i < 4; ++i) ie[i] = -99;
    }
    khol = 2;  // treat holiday factors as prior adjustments
}

}  // namespace

void holday(X13Context& ctx, const double* sti, int iforc, int xdsp) {
    const int pos1ob = ctx.x11ptr.pos1ob, pos1bk = ctx.x11ptr.pos1bk,
              posfob = ctx.x11ptr.posfob;
    double* yhol = ctx.xeastr.yhol.data();
    double* x11hol = ctx.x11fac.x11hol.data();

    int numfct = iforc;
    if (iforc == 0) numfct = 12;
    int iend = posfob + numfct + xdsp;
    for (int i = pos1bk; i <= iend; ++i) {
        if (i <= posfob) yhol[i - 1] = sti[i - 1] * 100.0;
        x11hol[i - 1] = 100.0;
    }
    int l3 = iend - pos1bk + 1;
    int nyear = l3 / 12;
    if (l3 % 12 != 0) nyear = nyear + 1;

    holidy(ctx, x11hol, nyear, pos1ob, pos1bk, ctx.extend.begbak(1), posfob,
           numfct + xdsp, ctx.x11opt.keastr, ctx.x11opt.khol);

    for (int i = 1; i <= iend; ++i) x11hol[i - 1] = x11hol[i - 1] / 100.0;
}

}  // namespace x13
