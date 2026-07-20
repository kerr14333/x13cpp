// x11xtrm.cpp -- X-11 Tier 3 extreme-value adjustment leaves (see x11xtrm.hpp).
// Faithful ports of the vendored oracle Fortran: rho2.f, wtxtrm.f, sdxtrm.f,
// xtrm.f, replac.f, weight.f, vtest.f, entsch.f, trbias.f, tdxtrm.f (istrue.f/
// setdp.f inlined at their call sites).
//
// Index convention: 0-based C pointers, Fortran index i -> element [i-1]; span
// args stay Fortran 1-based. Loop/float op order preserved verbatim (sigma
// limits and MAD medians are parity-sensitive). COMMON state passed explicitly.
#include "x11/x11xtrm.hpp"

#include "common/x13context.hpp"  // X13Context (x11ptr / xclude)
#include "specparse/specparse.hpp"  // setlg, cpyint, setdp
#include "x11/x11filt.hpp"       // hndtrn
#include "numeric/numeric.hpp"   // totals, dpeq, shlsrt
#include "gen/notset.hpp"        // prm::DNOTST

#include <cmath>
#include <vector>

namespace x13 {

namespace {
constexpr int PYRS = 85;    // srslen.prm: PYR1(65)+20
constexpr int PY1 = PYRS + 1;
}  // namespace

// rho2.f -- Tukey biweight rho, saturating past |u|>2.798.
double rho2(double u) {
    if (std::fabs(u) > 2.798) return 6.502;
    double u2 = u * u;
    return (0.9249 * u2) + (0.0812 * u2 * u2) - (0.0119 * u2 * u2 * u2);
}

// wtxtrm.f -- graduated extreme weight.
double wtxtrm(double x, double xbar, double stddev, double sigmu, double sigml,
              int istep, double lstwt) {
    const double MONE = -1.0, ZERO = 0.0;
    double wt = lstwt;
    double temp = std::fabs(x - xbar) / stddev;
    if (temp <= sigmu) {
        if (temp > sigml && istep != 1) wt = (sigmu - temp) / (sigmu - sigml);
    } else if (istep == 1) {
        wt = ZERO;
    } else if (lstwt > ZERO) {
        wt = MONE;
    }
    return wt;
}

// sdxtrm.f -- five-year sigma / MAD standard error of the irregulars.
double sdxtrm(const double* xi, double xbar, int l, int m, int nsp, int imad,
              int istep, int ny, bool lgrp, const double* stwt,
              const bool* csigvc, int ksdev) {
    const double ZERO = 0.0;
    double sdx = ZERO;
    double xn = ZERO;
    int ixn = 0;
    // lsig = istrue(Csigvc,1,Ny) .and. Ksdev.eq.4
    bool lsig = false;
    for (int i = 1; i <= ny; ++i) {
        if (csigvc[i - 1]) { lsig = true; break; }
    }
    lsig = lsig && (ksdev == 4);
    // abdev scratch, 1-based abdev[ixn-1].
    std::vector<double> abdev(static_cast<std::size_t>((m - l) / nsp + 2), 0.0);
    int n;
    for (n = l; n <= m; n += nsp) {
        bool lselec = true;
        if (lsig) {
            int nper = n % ny;
            if (nper == 0) nper = ny;
            lselec = csigvc[nper - 1] && lgrp;
            if (!lselec) lselec = (!csigvc[nper - 1]) && (!lgrp);
        }
        if ((!(istep == 2 && dpeq(stwt[n - 1], ZERO))) && lselec) {
            xn = xn + 1.0;
            ixn = ixn + 1;
            if (imad == 0) sdx = sdx + (xi[n - 1] - xbar) * (xi[n - 1] - xbar);
            if (imad == 1 || imad == 3) abdev[ixn - 1] = std::fabs(xi[n - 1] - xbar);
            if (imad == 2 || imad == 4) abdev[ixn - 1] = std::fabs(std::log(xi[n - 1]));
        }
    }
    if (imad == 0) sdx = std::sqrt(sdx / xn);
    if (imad >= 1) {
        ixn = static_cast<int>(xn);
        shlsrt(ixn, abdev.data());
        double median;
        if (ixn % 2 == 0)
            median = (abdev[ixn / 2 - 1] + abdev[ixn / 2 + 1 - 1]) / 2.0;
        else
            median = abdev[(ixn + 1) / 2 - 1];
        sdx = median / 0.6745;
        if (imad == 2 || imad == 4)
            sdx = std::sqrt(std::exp(sdx * sdx) * (std::exp(sdx * sdx) - 1));
        if (imad >= 3) {
            double stau = ZERO;
            for (int i2 = 1; i2 <= ixn; ++i2) stau = stau + rho2(abdev[i2 - 1] / sdx);
            // NOTE: the oracle divides by `n`, the DO-loop variable AFTER the
            // range loop (its post-loop value l+ceil((m-l+1)/nsp)*nsp), NOT ixn.
            sdx = std::sqrt(sdx * sdx * stau / n);
        }
    }
    return sdx;
}

// xtrm.f -- extreme-value weight/detection driver (two passes).
void xtrm(const double* xi, int kfda, int klda, int kfdax, int kldax, int ny,
          int muladd, int ksdev, int imad, double sigmu, double sigml, int lsp,
          double* stwt, double* stdper, double* stdev, const bool* csigvc) {
    const double ZERO = 0.0, ONE = 1.0;
    int n1 = 2 * ny;
    int n2 = n1 + ny - 1;
    int n3 = n2 + n1;
    int istep = 1;
    int jfda = (kfdax + ny - 2) / ny * ny + 1;
    int jlda = kldax / ny * ny - n3;
    int nfda = (kfdax - 1) / ny * ny + 1;
    int nlda = (((kldax - 1) / ny) + 1) * ny - n3;
    if (jlda < jfda) nlda = nfda;
    for (int t = 1; t <= klda; ++t) stwt[t - 1] = ONE;   // setdp(ONE,Klda,Stwt)
    for (int t = 1; t <= ny; ++t) stdper[t - 1] = ZERO;  // setdp(ZERO,Ny,Stdper)
    double xbar = ONE;
    if (muladd != 0) xbar = ZERO;
    while (istep <= 2) {
        if (ksdev == 4) {
            double sdev1 = sdxtrm(xi, xbar, kfdax, kldax, 1, imad, istep, ny,
                                  true, stwt, csigvc, ksdev);
            double sdev2 = sdxtrm(xi, xbar, kfdax, kldax, 1, imad, istep, ny,
                                  false, stwt, csigvc, ksdev);
            for (int i = 1; i <= ny; ++i) {
                stdper[i - 1] = sdev2;
                if (csigvc[i - 1]) stdper[i - 1] = sdev1;
            }
            for (int k = kfda; k <= klda; ++k) {
                int i = k % ny;
                if (i == 0) i = ny;
                if (stdper[i - 1] > ZERO)
                    stwt[k - 1] = wtxtrm(xi[k - 1], xbar, stdper[i - 1], sigmu,
                                         sigml, istep, stwt[k - 1]);
            }
        } else if (ksdev > 0) {
            for (int l = kfda; l <= kfda + ny - 1; ++l) {
                int i = l % ny;
                if (i == 0) i = ny;
                int lx = l;
                if (l < kfdax) {
                    int jj = lx % ny;
                    if (jj == 0) jj = ny;
                    if (i >= jj)
                        lx = kfdax + (jj - i);
                    else
                        lx = kfdax + ny + (i - jj);
                }
                int m = ((klda - l) / ny) * ny + l;
                int mx = ((kldax - l) / ny) * ny + l;
                double sdev1 = sdxtrm(xi, xbar, lx, mx, ny, imad, istep, ny,
                                      true, stwt, csigvc, ksdev);
                stdper[i - 1] = sdev1;
                if (!dpeq(sdev1, ZERO)) {
                    for (int k = l; k <= m; k += ny)
                        stwt[k - 1] = wtxtrm(xi[k - 1], xbar, sdev1, sigmu,
                                             sigml, istep, stwt[k - 1]);
                }
            }
            if (istep == 2)
                for (int t = 1; t <= PY1; ++t) stdev[t - 1] = prm::DNOTST;
        } else {
            int inx = 3 + ((lsp - 1) / ny);
            double sdev1 = ZERO;
            for (int i = nfda; i <= nlda; i += ny) {
                int j, k, l, m;
                if (nlda <= nfda) {
                    j = kfda; k = klda; l = kfdax; m = kldax;
                } else if (i <= nfda) {
                    j = kfda; k = nfda + n2; l = kfdax; m = jfda + n3;
                } else if (i < nlda) {
                    j = i + n1; k = i + n2; l = i; m = n3 + i;
                } else {
                    j = nlda + n1; k = klda; l = jlda; m = kldax;
                }
                sdev1 = sdxtrm(xi, xbar, l, m, 1, imad, istep, ny, true, stwt,
                               csigvc, ksdev);
                stdev[inx - 1] = sdev1;
                ++inx;
                if (!dpeq(sdev1, ZERO)) {
                    for (int nn = j; nn <= k; ++nn)
                        stwt[nn - 1] = wtxtrm(xi[nn - 1], xbar, sdev1, sigmu,
                                              sigml, istep, stwt[nn - 1]);
                }
            }
            if (istep == 2) {
                for (int i = 1; i <= 3; ++i) {
                    stdev[i - 1 + inx - 1] = sdev1;
                    stdev[i - 1] = stdev[3 + ((lsp - 1) / ny) - 1];
                }
            }
        }
        istep = istep + 1;
    }
    for (int i = kfdax; i <= kldax; ++i)
        if ((stwt[i - 1] + ONE) <= ZERO) stwt[i - 1] = ZERO;
}

// replac.f -- reweighted-MA replacement of extreme SI values. GOTO structure
// ported faithfully (labels 10..80 -> L10..L80).
void replac(double* x, double* y, const double* stwt, int lfda, int llda,
            int nm) {
    const double BIG = 10e16, ONE = 1.0;
    if (nm != 1) {
        for (int i = 1; i <= llda; ++i) y[i - 1] = BIG;
    }
    for (int i = 1; i <= nm; ++i) {
        int kfda = lfda + i - 1;
        int klda = (llda - kfda) / nm * nm + kfda;
        double ave = 0.0;
        if (nm != 1) ave = totals(x, kfda, klda, nm, 1);
        for (int j = kfda; j <= llda; j += nm) {
            if (dpeq(stwt[j - 1], ONE)) continue;  // GO TO 80
            int n = 0;
            double sumx = stwt[j - 1] * x[j - 1];
            int ihee = 0, ihle = 0;
            int m = 0, l = 0, inc = 0, m2 = 0;
            bool goto20 = false;
            if (j - nm <= kfda) {
                m = kfda; l = klda; inc = nm;
            } else {
                if (klda - nm > j)
                    goto20 = true;
                else {
                    m = klda; l = kfda; inc = -nm;
                }
            }
            if (!goto20) {
            L10:
                if (stwt[m - 1] >= ONE) {
                    sumx = sumx + x[m - 1];
                    ++n;
                    if (n >= 4) goto L60;
                }
                if (m == l) goto L50;
                m = m + inc;
                goto L10;
            }
        L20:
            m = j;
            if (ihle == 0 && ihee == 1) m = m2;
            l = klda;
            inc = nm;
        L30:
            while (m != l) {
                m = m + inc;
                if (dpeq(stwt[m - 1], ONE)) {
                    sumx = sumx + x[m - 1];
                    ++n;
                    switch (n) {
                        case 1: goto L30;
                        case 2: goto L40;
                        case 3: goto L30;
                        case 4: goto L60;
                    }
                    goto L40;
                }
            }
            if (inc == nm) ihle = 1;
            if (inc == -nm) ihee = 1;
            if (ihle == 0) goto L20;
            if (ihee != 0) goto L50;
        L40:
            if (ihle <= 0 || n != 2) {
                if (ihee > 0)
                    m = m2;
                else {
                    m2 = m;
                    m = j;
                }
                l = kfda;
                inc = -nm;
            }
            goto L30;
        L50:
            if (nm <= 1) goto L80;
            x[j - 1] = ave;
            goto L70;
        L60:
            x[j - 1] = sumx / (n + stwt[j - 1]);
            if (nm <= 1) goto L80;
        L70:
            y[j - 1] = x[j - 1];
        L80:;
        }
    }
}

// weight.f -- preliminary trend-cycle centered MA (24-term monthly / 8-term
// quarterly) with the published Census end-weight tables.
void weight(const double* a, double* b, int i1, int i2, int mq) {
    static const double cent[25] = {
        -0.0112773, -0.0273401, -0.0195570, -0.0053389, 0.0113162,
        0.0274075,  0.0416667,  0.0559258,  0.0720171,  0.0886723,
        0.1028903,  0.1106735,  0.1058879,  0.1106735,  0.1028903,
        0.0886723,  0.0720171,  0.0559258,  0.0416667,  0.0274075,
        0.0113162,  -0.0053389, -0.0195570, -0.0273401, -0.0112773};
    static const double end12[24] = {
        -0.0225546, -0.0234459, -0.0160151, -0.0026792, 0.0121628,
        0.0279138,  0.0413240,  0.0564321,  0.0728637,  0.0913320,
        0.1064322,  0.1145676,  0.1058879,  0.1067793,  0.0993484,
        0.0860125,  0.0711706,  0.0554195,  0.0420093,  0.0269012,
        0.0104697,  -0.0079986, -0.0230988, -0.0312343};
    static const double end11[24] = {
        -0.0106354, -0.0195678, -0.0208123, -0.0144215, -0.0026399,
        0.0121064,  0.0272793,  0.0432593,  0.0595187,  0.0811315,
        0.1018984,  0.1178834,  0.0939688,  0.1029011,  0.1041457,
        0.0977549,  0.0859732,  0.0712269,  0.0560541,  0.0400741,
        0.0238147,  0.0022018,  -0.0185651, -0.0345500};
    static const double end10[24] = {
        0.0019024,  -0.0124004, -0.0214279, -0.0229499, -0.0159235,
        -0.0036926, 0.0116272,  0.0297519,  0.0471940,  0.0692546,
        0.0931847,  0.1151461,  0.0814309,  0.0957337,  0.1047612,
        0.1062832,  0.0992569,  0.0870260,  0.0717061,  0.0535814,
        0.0361393,  0.0140787,  -0.0098514, -0.0318128};
    static const double end9[24] = {
        0.0121814,  -0.0035929, -0.0177541, -0.0263876, -0.0255157,
        -0.0176449, -0.0039735, 0.0141749,  0.0338072,  0.0566876,
        0.0805875,  0.1057635,  0.0711520,  0.0869263,  0.1010874,
        0.1097209,  0.1088491,  0.1009782,  0.0873068,  0.0691584,
        0.0495261,  0.0266457,  0.0027458,  -0.0224301};
    static const double end8[24] = {
        0.0181990,  0.0047570,  -0.0107207, -0.0239906, -0.0292988,
        -0.0274358, -0.0176746, -0.0019316, 0.0172443,  0.0407996,
        0.0654582,  0.0895942,  0.0651343,  0.0785764,  0.0940541,
        0.1073239,  0.1126322,  0.1107692,  0.1010079,  0.0852650,
        0.0660891,  0.0425338,  0.0178751,  -0.0062608};
    static const double end7[24] = {
        0.0192206,  0.0109225,  -0.0021151, -0.0163613, -0.0263849,
        -0.0309178, -0.0270820, -0.0170193, -0.0009008, 0.0188178,
        0.0438290,  0.0696580,  0.0641127,  0.0724108,  0.0854485,
        0.0996946,  0.1097182,  0.1142511,  0.1104153,  0.1003527,
        0.0842342,  0.0645156,  0.0395043,  0.0136754};
    static const double qcent[9] = {-0.0258462, -0.0208718, 0.1250000,
                                    0.2708718,  0.3016923,  0.2708718,
                                    0.1250000,  -0.0208718, -0.0258462};
    static const double qend4[8] = {-0.0516923, 0.0012821, 0.1323846,
                                    0.2930256,  0.3016923, 0.2487179,
                                    0.1176154,  -0.0430256};
    static const double qend3[8] = {-0.0036410, -0.0579487, 0.0079487,
                                    0.1786410,  0.2536410,  0.3079487,
                                    0.2420513,  0.0713590};
    for (int i = i1; i <= i2; ++i) b[i - 1] = 0.0;
    if (mq != 2) {
        int j1 = i1 + 12;
        int j2 = i2 - 12;
        for (int i = j1; i <= j2; ++i) {
            b[i - 1] = cent[13 - 1] * a[i - 1];
            for (int j = 1; j <= 12; ++j)
                b[i - 1] = b[i - 1] + cent[13 - j - 1] * a[i - j - 1] +
                           cent[13 + j - 1] * a[i + j - 1];
        }
        double endw[6][24];
        for (int j = 1; j <= 24; ++j) {
            endw[0][j - 1] = end12[j - 1];
            endw[1][j - 1] = end11[j - 1];
            endw[2][j - 1] = end10[j - 1];
            endw[3][j - 1] = end9[j - 1];
            endw[4][j - 1] = end8[j - 1];
            endw[5][j - 1] = end7[j - 1];
        }
        for (int i = 1; i <= 6; ++i) {
            int l1 = j1 - i;
            int l2 = j2 + i;
            for (int k = 1; k <= 24; ++k) {
                int m1 = l1 + i + 12 - k;
                int m2 = l2 - i - 12 + k;
                b[l1 - 1] = b[l1 - 1] + endw[i - 1][k - 1] * a[m1 - 1];
                b[l2 - 1] = b[l2 - 1] + endw[i - 1][k - 1] * a[m2 - 1];
            }
        }
        return;
    }
    int j1 = i1 + 4;
    int j2 = i2 - 4;
    for (int i = j1; i <= j2; ++i) {
        b[i - 1] = qcent[5 - 1] * a[i - 1];
        for (int j = 1; j <= 4; ++j)
            b[i - 1] = b[i - 1] + qcent[5 - j - 1] * a[i - j - 1] +
                       qcent[5 + j - 1] * a[i + j - 1];
    }
    double qend[2][8];
    for (int j = 1; j <= 8; ++j) {
        qend[0][j - 1] = qend4[j - 1];
        qend[1][j - 1] = qend3[j - 1];
    }
    for (int i = 1; i <= 2; ++i) {
        int l1 = j1 - i;
        int l2 = j2 + i;
        for (int k = 1; k <= 8; ++k) {
            int m1 = l1 + i + 4 - k;
            int m2 = l2 - i - 4 + k;
            b[l1 - 1] = b[l1 - 1] + qend[i - 1][k - 1] * a[m1 - 1];
            b[l2 - 1] = b[l2 - 1] + qend[i - 1][k - 1] * a[m2 - 1];
        }
    }
}

// vtest.f -- Cochran's heteroskedasticity test.
void vtest(const double* x, int& i1, int ib, int ie, int ny, int muladd) {
    static const double t[40] = {
        .5410, .3934, .3264, .2880, .2624, .2439, .2299, .2187, .2098, .2020,
        .1980, .194,  .186,  .182,  .178,  .174,  .17,   .166,  .162,  .158,
        .15,   .15,   .15,   .15,   .15,   .15,   .15,   .15,   .15,   .15,
        .15,   .15,   .15,   .15,   .15,   .1403, .14,   .14,   .14,   .14};
    static const double t4[40] = {
        .9065, .7679, .6841, .6287, .5895, .5598, .5365, .5175, .5017, .4884,
        .480,  .471,  .463,  .454,  .445,  .4366, .433,  .430,  .427,  .424,
        .421,  .417,  .414,  .411,  .408,  .404,  .401,  .398,  .395,  .391,
        .388,  .385,  .382,  .379,  .375,  .3720, .369,  .366,  .362,  .359};
    double s[12];  // PSP
    double tw = 0.0;
    i1 = 0;
    double smax = -10.0;
    int nmin = 100;
    double st = 1.0;
    if (muladd == 1) st = 0.0;
    for (int i = 1; i <= ny; ++i) {
        int n1 = 1;
        int j = ib + i - 1;
        s[i - 1] = 0.0;
        while (true) {
            s[i - 1] = s[i - 1] + std::pow(x[j - 1] - st, 2.0);
            j = j + ny;
            n1 = n1 + 1;
            if (j > ie) {
                if (n1 - 2 < nmin) nmin = n1 - 2;
                s[i - 1] = s[i - 1] / static_cast<double>(n1 - 1);
                if (s[i - 1] > smax) smax = s[i - 1];
                tw = tw + s[i - 1];
                break;
            }
        }
    }
    if (!dpeq(tw, 0.0)) tw = smax / tw;
    if (nmin > 40) nmin = 40;
    double tt = t[nmin - 1];
    if (ny == 4) tt = t4[nmin - 1];
    if (tw >= tt) i1 = 1;
}

// entsch.f -- Ksdev auto-selection helper (5-way branch on ken+ker+1).
void entsch(int ken, int ker, int& ken1, int& ker1, int iv) {
    int k = ken + ker + 1;
    ken1 = 0;
    ker1 = 0;
    switch (k) {
        case 1: ken1 = iv; ker1 = 2; break;
        case 2: ken1 = 1; break;
        case 3: ken1 = iv; ker1 = iv; break;
        case 4: ken1 = 1; ker1 = 2; break;
        default: break;  // k==5 -> label 50 (no-op)
    }
}

// trbias.f -- log-additive trend bias correction (Thompson & Ozaki 1992).
void trbias(double* stc, const double* sts, const double* sti, int l1, int l2,
            double* biasfc, int ny, bool tru7hn) {
    double sig = 0.0;
    for (int i = l1; i <= l2; ++i) sig = sig + sti[i - 1] * sti[i - 1];
    sig = std::exp(sig / (2.0 * (l2 - l1 + 1)));
    std::vector<double> hs(static_cast<std::size_t>(l2), 0.0);
    double tic23 = 4.5;
    hndtrn(hs.data(), sts, l1, l2, (2 * ny) - 1, tic23, /*lend=*/true,
           /*lsame=*/false, tru7hn);
    for (int i = l1; i <= l2; ++i) {
        biasfc[i - 1] = sig * hs[i - 1];
        stc[i - 1] = stc[i - 1] * biasfc[i - 1];
    }
}

// tdxtrm.f -- flag extreme irregulars for the calendar/trading-day pass.
void tdxtrm(X13Context& ctx, double* sti, double* faccal, const int* tday,
            double sigm, int kpart, int muladd, int fext, int irridx,
            int irrend) {
    (void)fext;  // deferred: only used by the dropped table/punch tail.
    const double ONE = 1.0, ZERO = 0.0;
    const int PLEN = 1020;  // srslen.prm

    const int posfob = ctx.x11ptr.posfob;

    double ex[1020];
    int karray[1020];
    double tmean[28], tcc[28], dvec[1];
    double tsd, tdiff, tk, tirr, tkon;
    int i, m, kstd, k;

    // 1-based views to keep the index arithmetic diffable against the Fortran.
    auto STI = [&](int j) -> double& { return sti[j - 1]; };
    auto FACCAL = [&](int j) -> double& { return faccal[j - 1]; };
    auto KARR = [&](int j) -> int& { return karray[j - 1]; };
    auto EX = [&](int j) -> double& { return ex[j - 1]; };
    auto TMEAN = [&](int j) -> double& { return tmean[j - 1]; };
    auto TCC = [&](int j) -> double& { return tcc[j - 1]; };
    auto RGXCLD = [&](int j) -> bool& { return ctx.xclude.rgxcld(j); };

    // Set up logical vector of observations to exclude from regression.
    dvec[0] = ZERO;
    setlg(false, PLEN, ctx.xclude.rgxcld.data());
    ctx.xclude.nxcld = 0;

    cpyint(tday, posfob, 1, karray);
    setdp(prm::DNOTST, PLEN, ex);

    kstd = 0;
    while (kstd < 2) {
        tsd = 0;
        tk = 0;
        if (kpart == 2) {
            // Initialize mean variables.
            tkon = ONE;
            if (muladd == 1) tkon = ZERO;
            for (i = 1; i <= 28; ++i) {
                TCC(i) = ZERO;
                if (i <= 21) {
                    TMEAN(i) = ZERO;
                } else {
                    TMEAN(i) = tkon;
                }
            }
            for (i = irridx; i <= irrend; ++i) {
                m = KARR(i);
                if (m < 15) {
                    TMEAN(m) = TMEAN(m) + STI(i);
                    TCC(m) = TCC(m) + ONE;
                    tk = tk + ONE;
                } else if (m <= 21) {
                    for (k = 15; k <= 21; ++k) {
                        TMEAN(k) = TMEAN(k) + STI(i);
                        TCC(k) = TCC(k) + ONE;
                    }
                    tk = tk + ONE;
                }
            }
            for (i = 1; i <= 21; ++i) {
                if (TCC(i) > ZERO) TMEAN(i) = TMEAN(i) / TCC(i);
            }
            for (i = irridx; i <= irrend; ++i) {
                m = KARR(i);
                if (m <= 21) {
                    tdiff = STI(i) - TMEAN(m);
                    tsd = tsd + (tdiff * tdiff);
                }
            }
        } else {
            // Compute sq.dev. of Irregular from Calendar effects.
            for (i = irridx; i <= irrend; ++i) {
                m = KARR(i);
                if (m <= 28) {
                    tdiff = STI(i) - FACCAL(i);
                    tsd = tsd + (tdiff * tdiff);
                    tk = tk + ONE;
                }
            }
        }
        tsd = std::sqrt(tsd / tk) * sigm;
        kstd = kstd + 1;
        // Identify extreme irregulars by adding 28 to type code.
        for (i = irridx; i <= irrend; ++i) {
            m = KARR(i);
            if (m <= 28) {
                if (kpart == 2) {
                    tirr = TMEAN(m);
                } else {
                    tirr = FACCAL(i);
                }
                if (std::fabs(STI(i) - tirr) > tsd) {
                    KARR(i) = KARR(i) + 28;
                    EX(i) = STI(i);
                    RGXCLD(i - irridx + 1) = true;
                    ctx.xclude.nxcld = ctx.xclude.nxcld + 1;
                }
            }
        }
    }
    // deferred: table/punch (extreme-value print/save)
    (void)dvec;
}

}  // namespace x13
