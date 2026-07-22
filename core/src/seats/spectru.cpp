// spectru.cpp -- see spectru.hpp. FUNC0/Fbis/MINIM/MINIMbis/GlobalMinim/
// MinimGrid transcribed from oracle/fortran/ansub2.f; ADDJ and SPECTRU
// itself from oracle/fortran/spectrum.f.
#include "seats/spectru.hpp"
#include "seats/seatspoly.hpp"  // conv, conj, multfn, divfcn
#include "seats/seatsfact.hpp"  // parfra

#include <algorithm>
#include <cmath>

namespace x13 {

// ---------------------------------------------------------------------
// FUNC0 -- ansub2.f:595.
// ---------------------------------------------------------------------
double func0(const SpectruHarmonics& h, int ifunc, double x) {
    double w = 0.0, numer = 0.0, denom = 0.0;
    double c[32] = {};
    int l;
    if (ifunc == 5) {
        l = std::max(h.ndum, h.ndum1);
        for (int i = 1; i <= l; ++i) { c[i - 1] = std::cos(w); w += x; }
        for (int i = 1; i <= h.ndum; ++i) numer += h.dum[i - 1] * c[i - 1];
        for (int i = 1; i <= h.ndum1; ++i) denom += h.dum1[i - 1] * c[i - 1];
        if (std::fabs(denom) < 1.0e-13) denom = std::copysign(1.0e-13, denom);
        return numer / denom;
    } else if (ifunc != 1) {
        if (ifunc == 3) {
            l = std::max(h.nuc, h.nc);
            for (int i = 1; i <= l; ++i) { c[i - 1] = std::cos(w); w += x; }
            for (int i = 1; i <= h.nuc; ++i) numer += h.uc[i - 1] * c[i - 1];
            for (int i = 1; i <= h.nc; ++i) denom += h.fc[i - 1] * c[i - 1];
        } else if (ifunc == 4) {
            l = std::max(h.nf, h.nh);
            for (int i = 1; i <= l; ++i) { c[i - 1] = std::cos(w); w += x; }
            for (int i = 1; i <= h.nf; ++i) numer += h.ff[i - 1] * c[i - 1];
            for (int i = 1; i <= h.nh; ++i) denom += h.fh[i - 1] * c[i - 1];
        } else {
            l = std::max(h.nut, h.nt);
            for (int i = 1; i <= l; ++i) { c[i - 1] = std::cos(w); w += x; }
            for (int i = 1; i <= h.nut; ++i) numer += h.ut[i - 1] * c[i - 1];
            for (int i = 1; i <= h.nt; ++i) denom += h.ft[i - 1] * c[i - 1];
        }
        if (std::fabs(denom) < 1.0e-13) denom = std::copysign(1.0e-13, denom);
        return numer / denom;
    } else {
        l = std::max(h.nv, h.ns);
        for (int i = 1; i <= l; ++i) { c[i - 1] = std::cos(w); w += x; }
        for (int i = 1; i <= h.nv; ++i) numer += h.v[i - 1] * c[i - 1];
        for (int i = 1; i <= h.ns; ++i) denom += h.fs[i - 1] * c[i - 1];
        if (std::fabs(denom) < 1.0e-13) denom = std::copysign(1.0e-13, denom);
        return numer / denom;
    }
}

// ---------------------------------------------------------------------
// Fbis -- ansub2.f:1380.
// ---------------------------------------------------------------------
double fbis(const SpectruHarmonics& h, int ifunc, double w, int haydif, int mq,
            int tol) {
    (void)mq;
    constexpr double pi = 3.14159265358979;
    double epsilon = 2.0 * pi * static_cast<double>(tol) / 360.0;
    if (ifunc == 2) {
        if (w < epsilon && haydif != 0) return func0(h, ifunc, epsilon);
        return func0(h, ifunc, w);
    }
    return func0(h, ifunc, w);
}

// ---------------------------------------------------------------------
// MINIM -- ansub2.f:729. goto/labels preserved verbatim (see spectru.hpp).
// ---------------------------------------------------------------------
void minim(const SpectruHarmonics& h, int ifunc, double start, double step,
           double dstop, double lb, double ub, double& fmin, double& xmin,
           int& iconv) {
    int icount = 0;
    const int ilim = 50;
    double d = step;
    double x2 = start, x1 = start - d, x3 = start + d, x4 = lb;
    double f1 = func0(h, ifunc, x1);
    double f2 = func0(h, ifunc, x2);
    double f3 = func0(h, ifunc, x3);
    double f4 = 0.0, hh1, hh3;
    iconv = 0;

L10:
    if (!(icount <= ilim)) goto L5003;
    if (std::fabs(x1 - x2) < 1.0e-12 || std::fabs(x3 - x2) < 1.0e-12) goto L5003;
    hh1 = (f2 - f1) / (x2 - x1);
    hh3 = (f3 - f2) / (x3 - x2);
    if (hh3 <= hh1) {
        if (x3 <= ub) {
            if (x1 > lb) {
                if (f1 < f3) goto L5001;
            }
            x4 = x3 + 25.0 * d;
            if (x4 > ub) x4 = 0.5 * (x3 + ub);
            for (;;) {
                f4 = func0(h, ifunc, x4);
                ++icount;
                if (f4 <= f2) goto L5000;
                x4 = 0.5 * (x4 + x3);
                if (icount > 50) goto L5003;
            }
        L5000:
            x1 = x2; f1 = f2; x2 = x3; f2 = f3; x3 = x4; f3 = f4;
            goto L10;
        }
    L5001:
        x4 = x1 - 25.0 * d;
        if (x4 < lb) x4 = 0.5 * (x1 + lb);
        for (;;) {
            f4 = func0(h, ifunc, x4);
            ++icount;
            if (f4 <= f2) goto L5002;
            x4 = 0.5 * (x4 + x1);
            if (icount > 50) goto L5003;
        }
    L5002:
        x3 = x2; f3 = f2; x2 = x1; f2 = f1; x1 = x4; f1 = f4;
    } else {
        x4 = x2 - (hh3 * (x2 - x1) + hh1 * (x3 - x2)) / (hh3 - hh1) / 2.0;
        if (x4 < (x1 - 10.0 * d)) {
            if (x4 < lb) x4 = 0.5 * (x1 + lb);
            f4 = func0(h, ifunc, x4);
            ++icount;
            x3 = x1; f3 = f1;
            if (f4 >= f1) {
                x1 = x4; f1 = f4;
            } else {
                x2 = x4; f2 = f4; x1 = x2 - 10.0 * d;
                f1 = func0(h, ifunc, x1);
                goto L10;
            }
        } else if (x4 > (x3 + 10.0 * d)) {
            if (x4 > ub) x4 = 0.5 * (x3 + ub);
            f4 = func0(h, ifunc, x4);
            ++icount;
            x1 = x3; f1 = f3;
            if (f4 >= f3) {
                x3 = x4; f3 = f4;
            } else {
                x2 = x4; f2 = f4; x3 = x2 + 10.0 * d;
                f3 = func0(h, ifunc, x3);
                goto L10;
            }
        } else {
            if (std::fabs(x4 - ub) < 0.0001) x4 = ub;
            f4 = func0(h, ifunc, x4);
            ++icount;
            if (std::fabs(x4 - x2) < dstop) goto L5004;
            if (std::fabs(x4 - x1) < dstop || std::fabs(x4 - x3) < dstop) goto L5004;
            if (x4 >= x2) {
                x1 = x2; f1 = f2;
                if (x4 > x3) {
                    x2 = x3; f2 = f3; x3 = x4; f3 = f4;
                    goto L10;
                }
            } else {
                x3 = x2; f3 = f2;
                if (x4 < x1) {
                    x2 = x1; f2 = f1; x1 = x4; f1 = f4;
                    goto L10;
                }
            }
            x2 = x4; f2 = f4;
            goto L10;
        }
        x2 = 0.5 * (x1 + x3);
        f2 = func0(h, ifunc, x2);
    }
    goto L10;

L5003:
    iconv = 1;
L5004:
    xmin = x4;
    fmin = f4;
}

// ---------------------------------------------------------------------
// MINIMbis -- ansub2.f:1007. Same shape as MINIM (goto/labels preserved
// verbatim) but over Fbis, with a 500-iteration outer cap (MINIM's is 50);
// the two inner retry loops keep the 50-iteration cap in both routines.
// ---------------------------------------------------------------------
void minimbis(const SpectruHarmonics& h, int ifunc, double start, double step,
              double dstop, double lb, double ub, int haydif, int mq, int tol,
              double& fmin, double& xmin, int& iconv) {
    int icount = 0;
    double d = step;
    double x2 = start, x1 = start - d, x3 = start + d, x4 = lb;
    double f1 = fbis(h, ifunc, x1, haydif, mq, tol);
    double f2 = fbis(h, ifunc, x2, haydif, mq, tol);
    double f3 = fbis(h, ifunc, x3, haydif, mq, tol);
    double f4 = 0.0, hh1, hh3;
    iconv = 0;

L10:
    if (!(icount <= 500)) goto L5003;
    if (std::fabs(x1 - x2) < 1.0e-12 || std::fabs(x3 - x2) < 1.0e-12) goto L5003;
    hh1 = (f2 - f1) / (x2 - x1);
    hh3 = (f3 - f2) / (x3 - x2);
    if (hh3 <= hh1) {
        if (x3 <= ub) {
            if (x1 > lb) {
                if (f1 < f3) goto L5001;
            }
            x4 = x3 + 25.0 * d;
            if (x4 > ub) x4 = 0.5 * (x3 + ub);
            for (;;) {
                f4 = fbis(h, ifunc, x4, haydif, mq, tol);
                ++icount;
                if (f4 <= f2) goto L5000;
                x4 = 0.5 * (x4 + x3);
                if (icount > 50) goto L5003;
            }
        L5000:
            x1 = x2; f1 = f2; x2 = x3; f2 = f3; x3 = x4; f3 = f4;
            goto L10;
        }
    L5001:
        x4 = x1 - 25.0 * d;
        if (x4 < lb) x4 = 0.5 * (x1 + lb);
        for (;;) {
            f4 = fbis(h, ifunc, x4, haydif, mq, tol);
            ++icount;
            if (f4 <= f2) goto L5002;
            x4 = 0.5 * (x4 + x1);
            if (icount > 50) goto L5003;
        }
    L5002:
        x3 = x2; f3 = f2; x2 = x1; f2 = f1; x1 = x4; f1 = f4;
    } else {
        x4 = x2 - (hh3 * (x2 - x1) + hh1 * (x3 - x2)) / (hh3 - hh1) / 2.0;
        if (x4 < (x1 - 10.0 * d)) {
            if (x4 < lb) x4 = 0.5 * (x1 + lb);
            f4 = fbis(h, ifunc, x4, haydif, mq, tol);
            ++icount;
            x3 = x1; f3 = f1;
            if (f4 >= f1) {
                x1 = x4; f1 = f4;
            } else {
                x2 = x4; f2 = f4; x1 = x2 - 10.0 * d;
                f1 = fbis(h, ifunc, x1, haydif, mq, tol);
                goto L10;
            }
        } else if (x4 > (x3 + 10.0 * d)) {
            if (x4 > ub) x4 = 0.5 * (x3 + ub);
            f4 = fbis(h, ifunc, x4, haydif, mq, tol);
            ++icount;
            x1 = x3; f1 = f3;
            if (f4 >= f3) {
                x3 = x4; f3 = f4;
            } else {
                x2 = x4; f2 = f4; x3 = x2 + 10.0 * d;
                f3 = fbis(h, ifunc, x3, haydif, mq, tol);
                goto L10;
            }
        } else {
            if (std::fabs(x4 - ub) < 0.0001) x4 = ub;
            f4 = fbis(h, ifunc, x4, haydif, mq, tol);
            ++icount;
            if (std::fabs(x4 - x2) < dstop) goto L5004;
            if (std::fabs(x4 - x1) < dstop || std::fabs(x4 - x3) < dstop) goto L5004;
            if (x4 >= x2) {
                x1 = x2; f1 = f2;
                if (x4 > x3) {
                    x2 = x3; f2 = f3; x3 = x4; f3 = f4;
                    goto L10;
                }
            } else {
                x3 = x2; f3 = f2;
                if (x4 < x1) {
                    x2 = x1; f2 = f1; x1 = x4; f1 = f4;
                    goto L10;
                }
            }
            x2 = x4; f2 = f4;
            goto L10;
        }
        x2 = 0.5 * (x1 + x3);
        f2 = fbis(h, ifunc, x2, haydif, mq, tol);
    }
    goto L10;

L5003:
    iconv = 1;
L5004:
    xmin = x4;
    fmin = f4;
}

// ---------------------------------------------------------------------
// GlobalMinim -- ansub2.f:1447.
// ---------------------------------------------------------------------
void global_minim(const SpectruHarmonics& h, int ifunc, double lb, double ub,
                   int n_step, int haydif, int mq, int tol, double step,
                   double dstop, double& fmin, double& xmin, int& iconv) {
    fmin = 10.0e20;
    xmin = 10.0e20;
    double e_step = (ub - lb) / static_cast<double>(n_step);
    double start = lb;
    iconv = 0;
    while (start <= ub) {
        double fmintmp, xmintmp;
        minimbis(h, ifunc, start, step, dstop, lb, ub, haydif, mq, tol,
                 fmintmp, xmintmp, iconv);
        if (fmintmp < fmin) {
            fmin = fmintmp;
            xmin = xmintmp;
        }
        start = start + e_step;
    }
}

// ---------------------------------------------------------------------
// MinimGrid -- ansub2.f:1257.
// ---------------------------------------------------------------------
void minim_grid(const SpectruHarmonics& h, int ifunc, int mq, int epsphi,
                 int wcomp, double& fmin, double& xmin) {
    constexpr double pi = 3.14159265358979;
    double epsilon = 2.0 * pi * static_cast<double>(epsphi) / 360.0;
    double paso = pi / 100000.0;

    if (wcomp == 1) {
        xmin = 0.0;
        fmin = func0(h, ifunc, xmin);
        double inf = 0.0;
        double sup = 2.0 * pi / mq - epsilon;
        double x = paso;
        while (x <= sup) {
            double fx = func0(h, ifunc, x);
            if (fmin > fx) { fmin = fx; xmin = x; }
            x += paso;
        }
        for (int i = 1; i <= mq / 2 - 1; ++i) {
            inf = sup + 2 * epsilon;
            sup = inf + 2.0 * pi / mq - 2 * epsilon;
            x = inf;
            while (x <= sup) {
                double fx = func0(h, ifunc, x);
                if (fmin > fx) { fmin = fx; xmin = x; }
                x += paso;
            }
        }
    } else if (wcomp == 2) {
        xmin = 0.0 + epsilon;
        fmin = func0(h, ifunc, xmin);
        double inf = xmin + paso;
        double x = inf;
        while (x <= pi) {
            double fx = func0(h, ifunc, x);
            if (fmin > fx) { fmin = fx; xmin = x; }
            x += paso;
        }
    } else {
        xmin = 0.0;
        fmin = func0(h, ifunc, 0.0);
        double x = paso;
        while (x <= pi) {
            double fx = func0(h, ifunc, x);
            if (fmin > fx) { fmin = fx; xmin = x; }
            x += paso;
        }
    }
}

// ---------------------------------------------------------------------
// ADDJ -- spectrum.f:3138.
// ---------------------------------------------------------------------
void addj(const double* a, int mplus1, double d1, const double* b, int nplus1,
          double d2, double* c, int& lplus1) {
    int lo = std::min(mplus1, nplus1);
    if (lo > 0) {
        for (int i = 0; i < lo; ++i) c[i] = d1 * a[i] + d2 * b[i];
    }
    if (mplus1 > nplus1) {
        for (int i = nplus1; i < mplus1; ++i) c[i] = d1 * a[i];
        lplus1 = mplus1;
    } else if (mplus1 < nplus1) {
        for (int i = mplus1; i < nplus1; ++i) c[i] = d2 * b[i];
        lplus1 = nplus1;
    } else {
        lplus1 = mplus1;
    }
}

// ---------------------------------------------------------------------
// SPECTRU -- spectrum.f:558-1190. See spectru.hpp for the parity/state
// notes (the tabular (out==0 .and. har==1) print branches -- lines 657-685,
// 747-757, 820-1004, 1141-1173 -- are not ported; they have no effect on any
// value this function returns). The `har` parameter is accepted for
// signature parity but unused (matches an oracle run with printing off).
// ---------------------------------------------------------------------
void spectru(const double* thstar, int qstar, const double* chi, int nchi,
             const double* cyc, int ncyc, const double* psi, int npsi,
             int pstar, int mq, int bd, int d, int out, int har, bool root0c,
             bool rootpic, bool rootpis, SpectruResult& r) {
    (void)out;
    (void)har;
    constexpr double pi = 3.14159265358979;
    SpectruHarmonics& H = r.h;
    r = SpectruResult{};

    int jsfix = 0;

    // spectrum.f:648-653: harmonic functions F=T(z)T(1/z) etc, then the
    // trend-cycle and total-model products.
    conj(thstar, qstar, thstar, qstar, H.ff, H.nf);
    conj(chi, nchi, chi, nchi, H.ft, H.nt);
    conj(cyc, ncyc, cyc, ncyc, H.fc, H.nc);
    conj(psi, npsi, psi, npsi, H.fs, H.ns);
    double fn[66] = {};
    int nn = 0;
    multfn(H.ft, H.nt, H.fc, H.nc, fn, nn);
    multfn(fn, nn, H.fs, H.ns, H.fh, H.nh);

    // spectrum.f:731-746: F(x)/H(x) = QT(x) + RT(x)/H(x).
    double qt[32] = {}, rt[66] = {};
    int nqt = 0, nrt = 0;
    if (qstar < pstar) {
        nqt = 1;
        qt[0] = 0.0;
        for (int i = 0; i < qstar; ++i) rt[i] = H.ff[i];
        for (int i = qstar; i < pstar; ++i) rt[i] = 0.0;
        nrt = pstar;
    } else {
        divfcn(H.ff, H.nf, H.fh, H.nh, qt, nqt, rt, nrt);
    }

    if (npsi == 1) jsfix = 1;
    if (mq == 1) jsfix = 1;

    // spectrum.f:769-964: distribute RT(x) among Ut(trend)/V(seasonal)/
    // Uc(cycle), by direct assignment when only one component is nontrivial,
    // else via PARFRA partial fractions (already-ported factor.cpp).
    if (jsfix == 1 && ncyc == 1 && r.ncycth == 0 && nchi > 1) {
        for (int i = 0; i < nrt; ++i) H.ut[i] = rt[i];
        H.nut = nrt;
        r.estar = 0.0;
        r.enoc = 0.0;
    } else if (jsfix != 1 && ncyc == 1 && r.ncycth == 0 && nchi == 1) {
        for (int i = 0; i < nrt; ++i) H.v[i] = rt[i];
        H.nv = nrt;
        r.enot = 0.0;
        r.enoc = 0.0;
    } else if (jsfix == 1 && r.ncycth == 0 && ncyc > 1 && nchi == 1) {
        for (int i = 0; i < nrt; ++i) H.uc[i] = rt[i];
        H.nuc = nrt;
        r.estar = 0.0;
        r.enot = 0.0;
    } else if (jsfix != 1 && r.ncycth == 0 && ncyc > 1 && nchi > 1) {
        // Trend + seasonal + cycle all nontrivial: split twice.
        double uu[66] = {};
        int nuu = 0;
        parfra(rt, nrt, fn, nn, H.fs, H.ns, uu, nuu, H.v, H.nv);
        int ipipp = ncyc + nchi - 1;
        for (int i = nuu; i < ipipp; ++i) uu[i] = 0.0;
        nuu = ipipp;
        parfra(uu, nuu, H.fc, H.nc, H.ft, H.nt, H.uc, H.nuc, H.ut, H.nut);
        if (H.nuc == 1 && std::fabs(H.uc[0]) < 1.0e-15) H.uc[0] = 1.0e-15;
    } else if (jsfix == 1 && r.ncycth == 0 && ncyc > 1 && nchi > 1) {
        parfra(rt, nrt, H.ft, H.nt, H.fc, H.nc, H.ut, H.nut, H.uc, H.nuc);
        r.estar = 0.0;
    } else if (jsfix != 1 && r.ncycth == 0 && ncyc > 1 && nchi == 1) {
        parfra(rt, nrt, H.fc, H.nc, H.fs, H.ns, H.uc, H.nuc, H.v, H.nv);
        r.enot = 0.0;
    } else if (jsfix != 1 && r.ncycth == 0 && ncyc == 1 && nchi > 1) {
        parfra(rt, nrt, H.ft, H.nt, H.fs, H.ns, H.ut, H.nut, H.v, H.nv);
        r.enoc = 0.0;
    }
    // (else: nchi==ncyc==1 && jsfix==1 -- a pure-irregular model with no
    // trend/seasonal/cycle structure at all. The oracle leaves Ut/V/Uc
    // untouched in this case too; not exercised by any corpus spec.)

    // spectrum.f:967-973: fold the quotient QT(x) into Uc when the numerator
    // outranks the denominator (qstar>pstar) -- ADDJ is alias-safe (see
    // addj()'s doc), matching the oracle's in-place `ADDJ(Uc,...,Uc,Nuc)`.
    if (qstar > pstar) {
        double dumv[66] = {};
        int ndum = 0;
        multfn(qt, nqt, H.fc, H.nc, dumv, ndum);
        addj(H.uc, H.nuc, 1.0, dumv, ndum, 1.0, H.uc, H.nuc);
        r.ncycth = 1;
    }

    // spectrum.f:1007-1047: trend spectrum minimum (enot).
    if (nchi != 1) {
        double lb = 0.5 * pi, ub = pi;
        double e1, exmin1, e2, exmin2;
        int jc1 = 0, jc2 = 0;
        global_minim(H, 2, lb, ub, 12, d + bd, mq, 2, 0.01, 0.000005, e1,
                     exmin1, jc1);
        lb = 0.0;
        ub = 0.5 * pi;
        global_minim(H, 2, lb, ub, 12, d + bd, mq, 2, 0.01, 0.000005, e2,
                     exmin2, jc2);
        if (std::fabs(exmin2) < 1.0e-3) r.is_ugly = true;
        r.enot = std::min(e1, e2);
        if (H.ut[0] - r.enot * H.ft[0] < 0.0) {
            double e3, exmin3;
            minim_grid(H, 2, mq, 2, 2, e3, exmin3);
            if (e3 < r.enot) r.enot = e3;
        }
    }

    // spectrum.f:1049-1093: cycle spectrum minimum (enoc).
    if (r.ncycth != 0 || ncyc != 1) {
        double lb = 0.5 * pi, ub = pi;
        double ce1, cexmin1, ce2, cexmin2;
        int jc3 = 0, jc4 = 0;
        global_minim(H, 3, lb, ub, 12, d + bd, mq, 2, 0.01, 0.000005, ce1,
                     cexmin1, jc3);
        if (std::fabs(cexmin1 - pi) < 1.0e-3 && rootpic) r.is_ugly = true;
        lb = 0.0;
        ub = 0.5 * pi;
        global_minim(H, 3, lb, ub, 12, d + bd, mq, 2, 0.01, 0.000005, ce2,
                     cexmin2, jc4);
        if (std::fabs(cexmin2) < 1.0e-3 && root0c) r.is_ugly = true;
        r.enoc = std::min(ce1, ce2);
        if (H.uc[0] - r.enoc * H.fc[0] < 0.0) {
            double ce3, cexmin3;
            minim_grid(H, 3, mq, 2, 3, ce3, cexmin3);
            if (ce3 < r.enoc) r.enoc = ce3;
        }
    }

    // spectrum.f:1095-1139: seasonal spectrum minimum (estar), jmq local
    // minima (mq/2 windows) plus a global-grid check.
    if (jsfix != 1) {
        int jmq2 = mq / 2;
        double efmin[9] = {}, exminv[9] = {};
        int iconvv[9] = {};
        double lb = 0.0, ub = pi / static_cast<double>(jmq2);
        minim(H, 1, 0.0, 0.01, 0.000005, lb, ub, efmin[0], exminv[0],
              iconvv[0]);
        if (std::fabs(efmin[0] - pi) < 1.0e-3 && rootpis) r.is_ugly = true;
        for (int i = 2; i <= jmq2; ++i) {
            double start =
                (static_cast<double>(i) - 0.5) * pi / static_cast<double>(jmq2);
            lb = static_cast<double>(i - 1) * pi / static_cast<double>(jmq2);
            ub = static_cast<double>(i) * pi / static_cast<double>(jmq2);
            minim(H, 1, start, 0.01, 0.000005, lb, ub, efmin[i - 1],
                  exminv[i - 1], iconvv[i - 1]);
            if (std::fabs(efmin[i - 1] - pi) < 1.0e-3 && rootpis)
                r.is_ugly = true;
        }
        r.estar = 10.0;
        for (int i = 0; i < jmq2; ++i)
            if (efmin[i] < r.estar) r.estar = efmin[i];
        if (H.v[0] - r.estar * H.fs[0] < 0.0) {
            double efm, exm;
            minim_grid(H, 1, mq, 2, 1, efm, exm);
            if (efm < r.estar) r.estar = efm;
        }
    }

    // spectrum.f:1178-1187: the admissibility number.
    r.qt1 = qt[0] + r.enot + r.estar + r.enoc;
    if (qstar > pstar) r.qt1 = r.enot + r.estar + r.enoc;
    if (r.qt1 < 0.0) r.is_ugly = true;
}

}  // namespace x13
