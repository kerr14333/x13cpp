// roots.cpp -- SEATS complex-polynomial root finder and its driver.
//   C02AEF / C02AEZ : NAG-derived Grant & Hitchins (1971) search root finder
//                     (oracle/fortran/ansub2.f:2900-3298)
//   RPQ            : root driver + classification (ansub2.f:16-175)
//
// PARITY HINGE. SEATS does NOT use the ported Jenkins-Traub rpoly here; it uses
// this bespoke C02AEF. Deflation order, the Adams-test acceptance in C02AEZ, and
// the search start/step are reproduced verbatim -- a last-ULP root difference
// re-classifies a root into a different decomposition component downstream. The
// Fortran control flow (computed GOTO, DO-WHILE with escape GOTOs) is preserved
// via C++ goto rather than restructured, to keep the operation order identical.
//
// The vendored X-13 build rewires the NAG machine-constant helpers:
//   X02AJF() -> dpmpar(1),  X02ALF() -> dpmpar(3)   (see ansub2.f:3460,3572)
// while RPQ's own tolerance X02AAF() is the hard literal 2.225073858507201d-14.
// All printing (OutRPQ) and the P01ABF hard-fail path are deferred, matching the
// SEATS "do not abort" edits.
//
// Fortran arrays are 1-based; the ports use local 1-based buffers (index 0 is a
// valid, unused slot) so every subscript matches the Fortran verbatim. Public
// pointers are ordinary 0-based C arrays; copy-in/out bridges the two.
#include "seats/seatspoly.hpp"
#include "numeric/numeric.hpp"

#include <cmath>

namespace x13 {

namespace {

// /ac02ae/ common block shared between C02AEF and C02AEZ.
struct Ac02ae {
    double X, Y0, R, Rx, J, Jx;
    bool Sat;
};

// C02AEZ(a,n,tol) -- ansub2.f:3239. Evaluates R,Rx,J,Jx at the point X+iY0 and
// applies the Adams test; sets Sat true when satisfied. a is 1-based (a[1..n]).
void c02aez(const double* a, int n, double tol, Ac02ae& cx) {
    const double two = 2.0, zero = 0.0, p8 = 0.8, ten = 1.0e1, a8 = 8.0;
    double a1, a2, a3, b1, b2, b3, c, p, q, t;

    p = -two * cx.X;
    q = cx.X * cx.X + cx.Y0 * cx.Y0;
    t = std::sqrt(q);
    a2 = zero;
    b2 = zero;
    b1 = a[1];
    a1 = a[1];
    c = std::abs(a1) * p8;
    n = n - 2;
    for (int k = 2; k <= n; ++k) {
        a3 = a2;
        a2 = a1;
        a1 = a[k] - p * a2 - q * a3;
        c = t * c + std::abs(a1);
        b3 = b2;
        b2 = b1;
        b1 = a1 - p * b2 - q * b3;
    }
    n = n + 2;
    a3 = a2;
    a2 = a1;
    a1 = a[n - 1] - p * a2 - q * a3;
    cx.R = a[n] + cx.X * a1 - q * a2;
    cx.J = a1 * cx.Y0;
    cx.Rx = a1 - two * b2 * cx.Y0 * cx.Y0;
    cx.Jx = two * cx.Y0 * (b1 - cx.X * b2);
    c = t * (t * c + std::abs(a1)) + std::abs(cx.R);
    cx.Sat = (std::sqrt(cx.R * cx.R + cx.J * cx.J)) <
             ((ten * c - a8 * (std::abs(cx.R) + std::abs(a1) * t) +
               two * std::abs(cx.X * a1)) *
              tol);
}

}  // namespace

// C02AEF(a,n,rez,imz,tol,ifail) -- ansub2.f:2900.
void c02aef(double* a0, int& n, double* rez0, double* imz0, double& tol,
            int& ifail) {
    // Data statements (ansub2.f:2980-2982).
    const double one = 1.0, a1p5 = 1.5, zero = 0.0, p4z1 = 1.0e-5;
    const double two = 2.0, p5 = 0.5, p2z1 = 1.0e-3, p1 = 0.1;
    const double p3z2 = 2.0e-4, four = 4.0;

    // Local 1-based buffers (index 0 unused). Max n is 100 per the guard.
    double a[102], rez[102], imz[102], b[102], c[102];
    int i, i2, ii, ind, jtemp, k, jj, norig = 0;
    bool cbig, flag;
    double cmax, fac, fun, g, nfun, s, s1, s2, scale, sig, t, tol2, xxx, zzz;
    Ac02ae cx;

    xxx = dpmpar(1);          // X02AJF()
    if (tol < xxx) tol = xxx;
    zzz = dpmpar(3);          // X02ALF()
    cmax = std::sqrt(zzz);
    fac = one;
    flag = (ifail == 2);
    if (flag) ifail = 1;
    ind = 0;
    tol2 = std::pow(tol, a1p5);

    if ((n >= 2) && (n <= 100)) {
        norig = n;
        for (i = 1; i <= n; ++i) a[i] = a0[i - 1];
        for (i = 1; i <= n; ++i) {
            rez[i] = rez0[i - 1];
            imz[i] = imz0[i - 1];
        }
        while (dpeq(a[n], 0.0) && n >= 2) {
            rez[n - 1] = zero;
            imz[n - 1] = zero;
            n = n - 1;
        }

        while (true) {  // MAIN outer loop (ansub2.f:3012)
            scale = zero;
            for (i = 1; i <= n; ++i) {
                if (std::abs(a[i]) >= p4z1) scale = scale + std::log(std::abs(a[i]));
            }
            k = (int)(scale / ((double)n * std::log(two)) + p5);
            scale = std::ldexp(1.0, -k);  // two**(-k), exact power of two
            for (i = 1; i <= n; ++i) {
                a[i] = a[i] * scale;
                b[i] = a[i];
            }
            if (n <= 3) {
                if (n == 1) goto L5009;
                if (n == 2) goto L5005;
                if (n == 3) goto L5006;
                goto L5000;
            L5005:
                rez[1] = -a[2] / a[1] * fac;
                imz[1] = zero;
                goto L5007;
            }
        L5000:
            while (true) {  // DO 10 WHILE (ansub2.f:3036)
                for (i = 2; i <= n; ++i) {
                    ii = n - i + 2;
                    if (dpeq(b[ii], 0.0)) goto L5001;
                    t = b[1] / b[ii];
                    if (std::abs(t) >= one) goto L5001;
                    for (k = 2; k <= ii; ++k) {
                        i2 = ii - k + 1;
                        c[k - 1] = b[k] - t * b[i2];
                    }
                    jtemp = ii - 1;
                    for (k = 1; k <= jtemp; ++k) b[k] = c[k];
                }
                fac = fac * two;
                scale = one;
                jj = n;
                while (true) {
                    jj = jj - 1;
                    if (jj < 1) goto L10;
                    scale = scale * two;
                    a[jj] = a[jj] * scale;
                    b[jj] = a[jj];
                }
            L10:;
            }
        L5001:
            if (!flag) {
                cx.X = p2z1;
                cx.Y0 = p1;
            } else {
                cx.X = rez[1];
                cx.Y0 = imz[1] + tol;
                flag = false;
            }
            c02aez(a, n, tol, cx);
            fun = cx.R * cx.R + cx.J * cx.J;
            while (true) {  // Newton iteration (ansub2.f:3076)
                g = cx.Rx * cx.Rx + cx.Jx * cx.Jx;
                if (g < fun * tol2) goto L5008;
                s1 = -(cx.R * cx.Rx + cx.J * cx.Jx) / g;
                s2 = (cx.R * cx.Jx - cx.J * cx.Rx) / g;
                sig = p3z2;
                s = std::sqrt(s1 * s1 + s2 * s2);
                if (s > one) {
                    s1 = s1 / s;
                    s2 = s2 / s;
                    sig = sig / s;
                }
                cx.X = cx.X + s1;
                cx.Y0 = cx.Y0 + s2;
                while (true) {  // step-halving (ansub2.f:3093)
                    c02aez(a, n, tol, cx);
                    if (cx.Sat) goto L5003;
                    nfun = cx.R * cx.R + cx.J * cx.J;
                    if (fun - nfun >= sig * fun) goto L5002;
                    s1 = p5 * s1;
                    s2 = p5 * s2;
                    if (std::abs(s1) <= xxx * std::abs(cx.X) &&
                        std::abs(s2) <= xxx * std::abs(cx.Y0))
                        goto L5008;
                    s = p5 * s;
                    sig = p5 * sig;
                    cx.X = cx.X - s1;
                    cx.Y0 = cx.Y0 - s2;
                }
            L5002:
                fun = nfun;
            }
        L5003:
            fun = one / tol2;
            k = 0;
            imz[n - 1] = cx.Y0 * fac;
            if (std::abs(cx.Y0) <= p1) {
                // Check possibility of a real root.
                s1 = cx.Y0;
                cx.Y0 = zero;
                c02aez(a, n, tol, cx);
                cx.Y0 = s1;
                if (cx.Sat) {
                    // Real root: linear-factor backward+forward deflation.
                    rez[n - 1] = cx.X * fac;
                    imz[n - 1] = zero;
                    n = n - 1;
                    b[1] = a[1];
                    c[n] = -a[n + 1] / cx.X;
                    cbig = false;
                    for (i = 2; i <= n; ++i) {  // DO 15
                        b[i] = a[i] + cx.X * b[i - 1];
                        ii = n - i + 1;
                        if (!cbig) {
                            c[ii] = (c[ii + 1] - a[ii + 1]) / cx.X;
                            if (std::abs(c[ii]) <= cmax) continue;
                            cbig = true;
                        }
                        c[ii] = cmax;
                    }
                    goto L5004;
                }
            }
            // Complex root: quadratic-factor backward+forward deflation.
            rez[n - 1] = cx.X * fac;
            rez[n - 2] = cx.X * fac;
            imz[n - 2] = -imz[n - 1];
            n = n - 2;
            cx.R = two * cx.X;
            cx.J = -(cx.X * cx.X + cx.Y0 * cx.Y0);
            b[1] = a[1];
            b[2] = a[2] + cx.R * b[1];
            c[n] = -a[n + 2] / cx.J;
            c[n - 1] = -(a[n + 1] + cx.R * c[n]) / cx.J;
            if (n != 2) {
                cbig = false;
                for (i = 3; i <= n; ++i) {  // DO 20
                    b[i] = a[i] + cx.R * b[i - 1] + cx.J * b[i - 2];
                    ii = n - i + 1;
                    if (!cbig) {
                        c[ii] = -(a[ii + 2] - c[ii + 2] + cx.R * c[ii + 1]) / cx.J;
                        if (std::abs(c[ii]) <= cmax) continue;
                        cbig = true;
                    }
                    c[ii] = cmax;
                }
            }
        L5004:
            for (i = 1; i <= n; ++i) {
                nfun = std::abs(b[i]) + std::abs(c[i]);
                if (nfun > tol) {
                    nfun = std::abs(b[i] - c[i]) / nfun;
                    if (nfun < fun) {
                        fun = nfun;
                        k = i;
                    }
                }
            }
            if (k != 1) {
                jtemp = k - 1;
                for (i = 1; i <= jtemp; ++i) a[i] = b[i];
            }
            if (k != 0) {
                a[k] = p5 * (b[k] + c[k]);
            }
            if (k != n) {
                jtemp = k + 1;
                for (i = jtemp; i <= n; ++i) a[i] = c[i];
            }
        }  // end MAIN outer loop

    L5006:
        cx.R = a[2] * a[2] - four * a[1] * a[3];
        if (cx.R > zero) {
            imz[1] = zero;
            imz[2] = zero;
            if (a[2] < 0.0) {
                rez[1] = p5 * (-a[2] + std::sqrt(cx.R)) / a[1] * fac;
            } else if (dpeq(a[2], 0.0)) {
                rez[1] = -p5 * std::sqrt(cx.R) / a[1] * fac;
            } else {
                rez[1] = p5 * (-a[2] - std::sqrt(cx.R)) / a[1] * fac;
            }
            rez[2] = a[3] / (rez[1] * a[1]) * fac * fac;
        } else {
            rez[2] = -p5 * a[2] / a[1] * fac;
            rez[1] = rez[2];
            imz[2] = p5 * std::sqrt(-cx.R) / a[1] * fac;
            imz[1] = -imz[2];
        }
    L5007:
        n = 1;
        goto L5009;
    L5008:
        ifail = 1;
        goto finish;
    L5009:
        ifail = ind;
        goto finish;
    finish:
        for (i = 1; i <= norig - 1; ++i) {
            rez0[i - 1] = rez[i];
            imz0[i - 1] = imz[i];
        }
        return;
    } else {
        ifail = 1;
        return;
    }
}

// RPQ(b,n,rez,imz,m,ar,p,noprint,out) -- ansub2.f:16. Root-finds b(1..n) then
// classifies each root by modulus / argument (degrees) / period. Outputs are
// 0-based C arrays; every Fortran subscript X(i) is written X[i-1] here.
void rpq(const double* b, int n, double* rez, double* imz, double* m,
         double* ar, double* p, int noprint, int out) {
    (void)noprint;
    (void)out;  // OutRPQ printing is deferred.
    const double ZERO = 0.0, ONE = 1.0;

    double a[66];  // RPQ's local poly copy a(65).
    int nroots, n1, ifail, i, j, k;
    double pi, tol, v, w;

    tol = 2.225073858507201e-14;  // X02AAF()
    pi = 3.14159265358979e0;
    nroots = n - 1;
    if (n > 1) {
        rez[0] = -b[1];  // rez(1) = -b(2)
    } else {
        rez[0] = ZERO;
    }
    imz[0] = 0.0;
    if (n > 2) {
        rez[0] = ZERO;
        imz[0] = ZERO;
        for (i = 1; i <= n; ++i) a[i - 1] = b[i - 1];
        n1 = n;
        ifail = 0;
        c02aef(a, n1, rez, imz, tol, ifail);
        if (ifail == 2) {
            c02aef(a, n1, rez, imz, tol, ifail);
        }
        // Reorder: list complex roots first.
        while (true) {
            k = 0;
            j = 0;
            for (i = 1; i <= nroots; ++i) {
                if (imz[i - 1] < tol && imz[i - 1] > -tol) {
                    k = i;
                } else {
                    j = i;
                }
                if (j > k && k > 0) goto L5000;
            }
            goto L5001;
        L5000:
            v = rez[j - 1];
            w = imz[j - 1];
            rez[j - 1] = rez[k - 1];
            imz[j - 1] = imz[k - 1];
            rez[k - 1] = v;
            imz[k - 1] = w;
        }
    }
L5001:
    // Modulus and argument.
    for (i = 1; i <= nroots; ++i) {
        m[i - 1] = std::sqrt(rez[i - 1] * rez[i - 1] + imz[i - 1] * imz[i - 1]);
        if (m[i - 1] < 1.0e-8) {
            ar[i - 1] = 0.0;
        } else {
            ar[i - 1] = rez[i - 1] / m[i - 1];
        }
        if (std::abs(ar[i - 1]) <= ONE) {
            ar[i - 1] = std::acos(ar[i - 1]);
            if (imz[i - 1] < ZERO) {
                ar[i - 1] = -ar[i - 1];
            }
        } else {
            ar[i - 1] = ZERO;
            if (rez[i - 1] < ZERO) {
                ar[i - 1] = pi;
            }
        }
    }
    // Period of each root.
    for (i = 1; i <= nroots; ++i) {
        if ((std::abs(ar[i - 1]) > 1.0e-8) &&
            (imz[i - 1] > tol || imz[i - 1] < -tol)) {
            p[i - 1] = 2.0e0 * pi / ar[i - 1];
        } else {
            p[i - 1] = 999.99;
        }
    }
    // Arguments in degrees.
    for (i = 1; i <= nroots; ++i) {
        ar[i - 1] = 180.0e0 * ar[i - 1] / pi;
    }
}

}  // namespace x13
