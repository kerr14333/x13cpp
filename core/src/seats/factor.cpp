// factor.cpp -- SEATS canonical-decomposition leaves PARFRA and MAK1, plus the
// SEATS-private helpers they need (all ported verbatim from ansub2.f):
//   CONVM/CONJM (ansub2.f:1567,2211) : build the harmonic-product coefficient
//                                      matrix for the partial-fraction solve
//   MLTSOL      (ansub2.f:2565)      : sparse Gaussian-elimination linear solve
//   SYMPOLY     (ansub2.f:2309)      : reduce a symmetric polynomial to n/2 coeffs
//   SQROOTC/ROOTC (ansub2.f:2391,2522): complex sqrt / roots of x^2+(a+bi)x+1
//   MPBC        (ansub2.f:2258)      : complex polynomial multiply
//   grRoots + getRoot/getRootc/closestRoot/JoinRoot (ansub2.f:1896..2101),
//   halfRoots   (ansub2.f:2111)      : group roots of the symmetric spectrum and
//                                      keep the invertible half (modulus <= 1)
// RPQ (roots.cpp) and CONJ (poly.cpp) are the already-ported dependencies.
//
// Fortran arrays are 1-based; internal buffers keep index 0 unused so every
// subscript matches the .f verbatim. The public 0-based ports rpq()/conj() are
// bridged with &buf[1]. Char period tags (per/gPer) are print-only in the
// oracle and are omitted here (deferred output, no numeric effect).
#include "seats/seatsfact.hpp"
#include "seats/seatspoly.hpp"

#include <cmath>
#include <complex>

namespace x13 {

namespace {

// Leading dimension of the PARFRA coefficient matrix cc(60,66) (column-major).
constexpr int LD = 60;
inline double& CC(double* a, int i, int j) { return a[(j - 1) * LD + (i - 1)]; }

// CONVM(a,mplus1,b,nplus1,c,ncol) -- ansub2.f:1567. Column array a times row
// array b, accumulated into columns (ncol+1..ncol+nplus1) of c. c is (60,66).
void convm(const double* a, int mplus1, const double* b, int nplus1, double* c,
           int ncol) {
    for (int i = 1; i <= mplus1; ++i) {
        for (int j = 1; j <= nplus1; ++j) {
            int num = i + j - 1;
            CC(c, num, j + ncol) += a[i] * b[j];
        }
    }
}

// CONJM(a,mplus1,b,nplus1,c,ncol) -- ansub2.f:2211. Correlation (folded) product
// c(|i-j|+1, j+ncol) += a(i)*b(j).
void conjm(const double* a, int mplus1, const double* b, int nplus1, double* c,
           int ncol) {
    for (int i = 1; i <= mplus1; ++i) {
        for (int j = 1; j <= nplus1; ++j) {
            int k = i - j;
            int num = std::abs(k) + 1;
            CC(c, num, j + ncol) += a[i] * b[j];
        }
    }
}

}  // namespace

// MLTSOL(a,n,l,pr,pc) -- ansub2.f:2565. Solve l right-hand sides (packed in
// columns n+1..n+l of a) against the n-by-n system in columns 1..n, by sparse
// Gauss-Jordan with implicit pivoting; the solution overwrites those columns.
// a is (pr=60, pc=66) column-major -- the SAME physical layout ESTBUR's own
// am(60,66) uses (estbur.cpp reuses this directly rather than re-porting a
// second copy). Exposed (moved out of the anonymous namespace above) so
// estbur.cpp can link it; declared in seatsfact.hpp.
void mltsol(double* a, int n, int l) {
    int m[67];
    double b[61];
    // min1 = 10.d0**(-15.d0): keep the Fortran ** (see FABLE_REVIEW ULP note).
    double min1 = std::pow(10.0, -15.0);
    double min2 = 0.0 - min1;
    int nl = n + l;
    for (int k = 1; k <= nl; ++k) m[k] = 0;
    int i1 = 0;
    for (int irev = 1; irev <= n; ++irev) {
        int i = n - irev + 1;
        double u = 10.0e-30;
        for (int k = 1; k <= n; ++k) {
            if (std::abs(CC(a, i, k)) > u && m[k] == 0) {
                u = std::abs(CC(a, i, k));
                i1 = k;
            }
        }
        m[i1] = i;
        double pivot = 1.0 / CC(a, i, i1);
        if (pivot >= min2 && pivot <= min1) pivot = 0.0;
        for (int j = 1; j <= n; ++j) {
            if (j != i) {
                if (std::abs(CC(a, j, i1)) >= 1.0e-13) {
                    double fac = pivot * CC(a, j, i1);
                    for (int k = 1; k <= nl; ++k) {
                        if (CC(a, i, k) >= min2 && CC(a, i, k) <= min1) {
                            CC(a, i, k) = 0.0;
                        } else {
                            if (m[k] == 0) {
                                CC(a, j, k) = CC(a, j, k) - fac * CC(a, i, k);
                            }
                        }
                    }
                }
            }
        }
        for (int k = 1; k <= nl; ++k) {
            if (m[k] == 0) CC(a, i, k) = pivot * CC(a, i, k);
        }
    }
    int n1 = n + 1;
    for (int k = n1; k <= nl; ++k) {
        for (int r = 1; r <= n; ++r) {
            if (m[r] != 0) {
                b[r] = CC(a, m[r], k);
            } else {
                b[r] = 0.0;
            }
        }
        for (int r = 1; r <= n; ++r) CC(a, r, k) = b[r];
    }
}

namespace {

// SYMPOLY(poly,npoly,rdpoly,nrpoly) -- ansub2.f:2309. Reduce a symmetric
// polynomial (n coeffs) to the Chebyshev-basis polynomial of n/2 coeffs.
void sympoly(const double* poly, int npoly, double* rdpoly, int& nrpoly) {
    double poly1[65];
    static thread_local double s[65 * 65];  // s(64,64) column-major, big scratch
    double s0[65], s1[65], s2[65];
    auto S = [&](int i, int j) -> double& { return s[(j - 1) * 65 + (i - 1)]; };

    s0[1] = 2.0;
    int ns0 = 1;
    s1[1] = 0.0;
    s1[2] = 1.0;
    int ns1 = 2;
    for (int i = 1; i <= npoly; ++i)
        for (int j = 1; j <= npoly; ++j) S(i, j) = 0.0;
    S(1, 1) = 1.0;
    S(2, 2) = 1.0;
    int ns2 = 0;
    for (int i = 3; i <= npoly; ++i) {
        s2[1] = 0.0;
        for (int j = 2; j <= ns1 + 1; ++j) s2[j] = s1[j - 1];
        ns2 = ns1 + 1;
        s0[ns0 + 1] = 0.0;
        s0[ns0 + 2] = 0.0;
        ns0 = ns0 + 2;
        for (int j = 1; j <= ns2; ++j) s2[j] = s2[j] - s0[j];
        for (int j = 1; j <= ns2; ++j) S(j, i) = s2[j];
        for (int j = 1; j <= ns1; ++j) s0[j] = s1[j];
        for (int j = 1; j <= ns2; ++j) s1[j] = s2[j];
        ns1 = ns2;
    }
    for (int i = 1; i <= npoly; ++i) poly1[npoly + 1 - i] = poly[i];
    for (int i = 1; i <= npoly; ++i) {
        double temp = 0.0;
        for (int j = 1; j <= npoly; ++j) temp = temp + S(i, j) * poly1[j];
        rdpoly[npoly + 1 - i] = temp;
    }
    nrpoly = npoly;
}

// SQROOTC(rez,imz,rez1,imz1) -- ansub2.f:2391. Principal complex square root.
void sqrootc(double rez, double imz, double& rez1, double& imz1) {
    double temp;
    if (rez >= 0.0) {
        temp = std::sqrt(rez * rez + imz * imz);
        rez1 = std::sqrt((rez + temp) / 2.0);
        if (std::abs(rez1) < 1.0e-8) {
            imz1 = 0.0;
        } else {
            imz1 = imz / (2.0 * rez1);
        }
    } else {
        temp = std::sqrt(rez * rez + imz * imz);
        if (imz > 0.0) {
            imz1 = std::sqrt((std::abs(rez) + temp) / 2.0);
        } else {
            imz1 = -std::sqrt((std::abs(rez) + temp) / 2.0);
        }
        if (std::abs(imz1) < 1.0e-8) {
            rez1 = 0.0;
        } else {
            rez1 = imz / (2.0 * imz1);
        }
    }
}

// ROOTC(rez,imz,r1,r2) -- ansub2.f:2522. Roots of x^2 + (rez+i*imz) x + 1 = 0.
// r1,r2 are 2-vectors (real, imag).
void rootc(double rez, double imz, double* r1, double* r2) {
    double delta = rez * rez - imz * imz - 4.0;
    double deltai = 2.0 * rez * imz;
    double a, b;
    sqrootc(delta, deltai, a, b);
    r1[1] = (-rez + a) / 2.0;
    r1[2] = (-imz + b) / 2.0;
    r2[1] = (-rez - a) / 2.0;
    r2[2] = (-imz - b) / 2.0;
}

// MPBC(a,b,n,m,e) -- ansub2.f:2258. Product of two complex polynomials in B.
// a(0:n), b(0:m), e(0:n+m). e may alias a or b (inputs copied first).
void mpbc(const std::complex<double>* a, const std::complex<double>* b, int n,
          int m, std::complex<double>* e) {
    std::complex<double> aa[101], bb[101];
    for (int i = 0; i <= m; ++i) bb[i] = b[i];
    for (int i = 0; i <= n; ++i) aa[i] = a[i];
    for (int i = 0; i <= n + m; ++i) e[i] = std::complex<double>(0.0, 0.0);
    for (int i = 0; i <= n; ++i)
        for (int j = 0; j <= m; ++j) e[i + j] = e[i + j] + aa[i] * bb[j];
}

// getRoot -- ansub2.f:1975. Index in rez/imz[1..nr] matching (realr,imagr), else 0.
int get_root(const double* rez, const double* imz, int nr, double realr,
             double imagr, double xeps) {
    int i = 1;
    while (i <= nr) {
        if (std::abs(rez[i] - realr) <= xeps && std::abs(imz[i] - imagr) <= xeps)
            return i;
        ++i;
    }
    return 0;
}

// getRootc -- ansub2.f:2001. As getRoot but searching backward from nr to ni.
int get_rootc(const double* rez, const double* imz, int ni, int nr, double realr,
              double imagr, double xeps) {
    int i = nr;
    while (i >= ni) {
        if (std::abs(rez[i] - realr) <= xeps && std::abs(imz[i] - imagr) <= xeps)
            return i;
        --i;
    }
    return 0;
}

// closestRoot -- ansub2.f:2024. Index in [ni,ng] nearest (realz,imagz).
int closest_root(const double* grez, const double* gimz, const double* /*gmod*/,
                 int ni, int ng, double realz, double imagz) {
    int i2 = 0;
    double mindist = 1.0e10;
    for (int i = ni; i <= ng; ++i) {
        double dist = (grez[i] - realz) * (grez[i] - realz) +
                      (gimz[i] - imagz) * (gimz[i] - imagz);
        if (dist < mindist) {
            i2 = i;
            mindist = dist;
        }
    }
    return i2;
}

// JoinRoot -- ansub2.f:2050. Merge grouped roots ni and ni2 into ni (weighted by
// multiplicity), recompute modulus/arg/period, then compact the arrays.
void join_root(double* grez, double* gimz, double* gmodul, double* gar,
               double* gpr, int* gcont, int& ng, int ni, int ni2) {
    const double pi = 3.14159265358979;
    double sumg = static_cast<double>(gcont[ni]) + static_cast<double>(gcont[ni2]);
    gmodul[ni] = gmodul[ni] * gmodul[ni2];
    grez[ni] = (grez[ni] * gcont[ni] + grez[ni2] * gcont[ni2]) / sumg;
    gimz[ni] = (gimz[ni] * gcont[ni] + gimz[ni2] * gcont[ni2]) / sumg;
    gcont[ni] = static_cast<int>(sumg);
    double mm = grez[ni] * grez[ni] + gimz[ni] * gimz[ni];
    mm = std::sqrt(gmodul[ni] / mm);
    grez[ni] = grez[ni] * mm;
    gimz[ni] = gimz[ni] * mm;
    gmodul[ni] = std::sqrt(gmodul[ni]);
    if (grez[ni] > 0.0) {
        gar[ni] = (std::atan(gimz[ni] / grez[ni]) * 180.0) / pi;
    } else if (grez[ni] < 0.0) {
        gar[ni] = 180.0 + (std::atan(gimz[ni] / grez[ni]) * 180.0) / pi;
        if (gar[ni] > 180.0) gar[ni] = 180.0 - gar[ni];
    } else if (gimz[ni] > 0.0) {
        gar[ni] = 90.0;
    } else {
        gar[ni] = -90.0;
    }
    if (gar[ni] != 0.0) {
        gpr[ni] = 360.0 / gar[ni];
    } else {
        gpr[ni] = 999.99;
    }
    ng = ng - 1;
    for (int i = ni2; i <= ng; ++i) {
        grez[i] = grez[i + 1];
        gimz[i] = gimz[i + 1];
        gmodul[i] = gmodul[i + 1];
        gar[i] = gar[i + 1];
        gpr[i] = gpr[i + 1];
        gcont[i] = gcont[i + 1];
    }
}

// grRoots -- ansub2.f:1896. Group equal roots (with multiplicity), keep complex
// conjugates consecutive, and merge lone unit roots with their nearest neighbour.
void gr_roots(const double* rez, const double* imz, const double* modul,
              const double* ar, const double* pr, int nr, double* grez,
              double* gimz, double* gmodul, double* gar, double* gpr, int* gcont,
              int& ng) {
    const double xeps = 1.0e-13;
    ng = 0;
    int i = 1;
    while (i <= nr) {
        double xeps2;
        if (std::abs(modul[i] - 1.0) < xeps) {
            xeps2 = 1.0e-30;
        } else {
            xeps2 = 1.0e-30;
        }
        int ni = get_root(grez, gimz, ng, rez[i], imz[i], xeps2);
        if (ni > 0) {
            gcont[ni] = gcont[ni] + 1;
        } else {
            ng = ng + 1;
            grez[ng] = rez[i];
            gimz[ng] = imz[i];
            gmodul[ng] = modul[i];
            gar[ng] = ar[i];
            gpr[ng] = pr[i];
            gcont[ng] = 1;
            if (std::abs(imz[i]) > xeps2) {
                int ic = get_rootc(rez, imz, i + 1, nr, rez[i], -imz[i], xeps2);
                if (ic == 0) {
                    // ERROR: conjugate not found (oracle no-op).
                } else {
                    ng = ng + 1;
                    grez[ng] = rez[ic];
                    gimz[ng] = imz[ic];
                    gmodul[ng] = modul[ic];
                    gar[ng] = ar[ic];
                    gpr[ng] = pr[ic];
                    gcont[ng] = 0;
                }
            } else {
                gimz[ng] = 0.0;
            }
        }
        i = i + 1;
    }
    i = 1;
    while (i < ng) {
        if (gcont[i] == 1 && std::abs(gmodul[i] - 1.0) < xeps) {
            int i2 = closest_root(grez, gimz, gmodul, i + 1, ng, grez[i], gimz[i]);
            join_root(grez, gimz, gmodul, gar, gpr, gcont, ng, i, i2);
        }
        i = i + 1;
    }
}

// halfRoots -- ansub2.f:2111. From the grouped roots of the symmetric spectrum
// S(B,F), return the roots of the invertible factor P(B) with P(B)P(F)=S(B,F):
// keep roots with modulus < 1 (their full multiplicity) and, for unit-modulus
// roots, one copy per conjugate pair.
void half_roots(const double* grez, const double* gimz, const double* gmodul,
                const double* gar, const double* gpr, const int* gcont, int ng,
                double* rez, double* imz, double* modul, double* ar, double* pr,
                int& nr) {
    const double xeps = 1.0e-13;
    double xeps2 = 1.0e-10;
    int i = 1;
    int j = 1;
    while (i <= ng) {
        int nrep = 0;
        if (std::abs(gmodul[i] - 1.0) < xeps) {
            nrep = gcont[i] / 2;
            if (gcont[i] == 1) {
                if (i >= ng) {
                    // ERROR (oracle no-op).
                } else if (gcont[i + 1] == 1 &&
                           std::abs(gmodul[i + 1] - 1.0) < xeps) {
                    i = i + 1;
                    if (grez[i] > 0.0) {
                        rez[j] = gmodul[i];
                        imz[j] = 0.0;
                        modul[j] = gmodul[i];
                        pr[j] = 999.0;
                        ar[j] = 180.0;
                    } else {
                        rez[j] = -gmodul[i];
                        imz[j] = 0.0;
                        modul[j] = gmodul[i];
                        pr[j] = 2.0;
                        ar[j] = 0.0;
                    }
                    j = j + 1;
                }
            }
        } else if (gmodul[i] < 1.0) {
            nrep = gcont[i];
        } else {
            nrep = 0;
        }
        for (int k = 1; k <= nrep; ++k) {
            rez[j] = grez[i];
            imz[j] = gimz[i];
            modul[j] = gmodul[i];
            ar[j] = gar[i];
            pr[j] = gpr[i];
            j = j + 1;
            if (std::abs(gimz[i]) > xeps2 &&
                std::abs(grez[i] - grez[i + 1]) < xeps2) {
                rez[j] = grez[i + 1];
                imz[j] = gimz[i + 1];
                modul[j] = gmodul[i + 1];
                ar[j] = gar[i + 1];
                pr[j] = gpr[i + 1];
                j = j + 1;
            }
        }
        if (std::abs(gimz[i]) > xeps2 &&
            std::abs(grez[i] - grez[i + 1]) < xeps2 && nrep > 0) {
            i = i + 2;  // skip the conjugate complex root
        } else {
            i = i + 1;
        }
    }
    nr = j - 1;
}

}  // namespace

// ==========================================================================
// PARFRA -- ansub2.f:1495
// ==========================================================================
void parfra(const double* rt, int nrt, const double* t, int nt, const double* s,
            int ns, double* u, int& nu, double* v, int& nv) {
    (void)nrt;
    double cc[LD * 66];
    double a[61];  // a(60) 1-based scratch of ones
    for (int idx = 0; idx < LD * 66; ++idx) cc[idx] = 0.0;

    int m = nt - 1;
    int n = ns - 1;
    int p = m + n;
    // s(1..ns) and t(1..nt) are 0-based inputs; use 1-based views.
    const double* s1 = s - 1;
    const double* t1 = t - 1;
    const double* rt1 = rt - 1;

    for (int i = 1; i <= m; ++i) a[i] = 1.0;
    int ncol = 0;
    convm(s1, ns, a, m, cc, ncol);
    conjm(s1, ns, a, m, cc, ncol);
    for (int i = 1; i <= n; ++i) a[i] = 1.0;
    ncol = m;
    convm(t1, nt, a, n, cc, ncol);
    conjm(t1, nt, a, n, cc, ncol);
    for (int i = 1; i <= p; ++i)
        for (int j = 1; j <= p; ++j) CC(cc, i, j) = CC(cc, i, j) / 2.0;
    for (int i = 1; i <= p; ++i) CC(cc, i, p + 1) = rt1[i];
    mltsol(cc, p, 1);
    for (int i = 1; i <= m; ++i) u[i - 1] = CC(cc, i, p + 1);
    nu = m;
    for (int i = m + 1; i <= p; ++i) v[i - m - 1] = CC(cc, i, p + 1);
    nv = n;
}

// ==========================================================================
// MAK1 -- ansub2.f:1615
// ==========================================================================
void mak1(const double* ufin, int nufin, double* theta, int& ntheta,
          double& var, int nnio, double xl, double& toterr) {
    const double pi = 3.14159265358979;
    const double tol = 1.0e-5;

    // 1-based local buffers (index 0 unused), sizes matching the Fortran.
    double ar[65], ar1[65], imz[65], imz1[65], modul[65], modul1[65];
    double poly[35], pr[65], pr1[65], r1[3], r2[3], rez[65], rez1[65];
    double rdpoly[35];
    std::complex<double> az[65], bz[65];
    double grez[65], gimz[65], gmodul[65], gar[65], gpr[65];
    int gcont[65];
    double vn[65];
    int nvn;
    const double* ufin1 = ufin - 1;  // 1-based view of the input

    for (int i = 1; i <= 64; ++i) grez[i] = 0.0;  // MAK1 zeros only gRez

    double gamzer = ufin1[1];
    int n = nufin;
    for (int i = 1; i <= nufin - 1; ++i) poly[i] = ufin1[nufin + 1 - i];
    poly[nufin] = ufin1[1] * 2.0;

    int nrpoly;
    if (n <= 2) {
        rez1[1] = -poly[2] / poly[1];
        imz1[1] = 0.0;
        nrpoly = 2;
    } else {
        sympoly(poly, n, rdpoly, nrpoly);
        // rpq() is 0-based; &buf[1] maps rpq's out[0] -> Fortran buf(1).
        rpq(rdpoly + 1, nrpoly, rez1 + 1, imz1 + 1, modul1 + 1, ar1 + 1, pr1 + 1,
            1, 1);
    }

    // Roots of the original symmetric polynomial via ROOTC.
    int k = 1;
    for (int i = 1; i <= nrpoly - 1; ++i) {
        double a = -rez1[i];
        double b = -imz1[i];
        rootc(a, b, r1, r2);
        double temp = std::sqrt(r1[1] * r1[1] + r1[2] * r1[2]);
        double temp1 = std::sqrt(r2[1] * r2[1] + r2[2] * r2[2]);
        rez[k] = r1[1];
        imz[k] = r1[2];
        modul[k] = temp;
        if (modul[k] < 1.0e-8) {
            ar[k] = 0.0;
        } else {
            ar[k] = rez[k] / modul[k];
        }
        if (std::abs(ar[k]) <= 1.0) {
            ar[k] = std::acos(ar[k]);
            if (imz[k] < 0.0) ar[k] = -ar[k];
        } else {
            ar[k] = 0.0;
            if (rez[k] < 0.0) ar[k] = pi;
        }
        k = k + 1;
        rez[k] = r2[1];
        imz[k] = r2[2];
        modul[k] = temp1;
        if (modul[k] < 1.0e-8) {
            ar[k] = 0.0;
        } else {
            ar[k] = rez[k] / modul[k];
        }
        if (std::abs(ar[k]) <= 1.0) {
            ar[k] = std::acos(ar[k]);
            if (imz[k] < 0.0) ar[k] = -ar[k];
        } else {
            ar[k] = 0.0;
            if (rez[k] < 0.0) ar[k] = pi;
        }
        k = k + 1;
    }
    int nroots = k - 1;
    for (int i = 1; i <= nroots; ++i) {
        if (imz[i] > tol || imz[i] < -tol) {
            pr[i] = 2.0 * pi / ar[i];
        } else {
            pr[i] = 999.99;
        }
        ar[i] = 180.0 * ar[i] / pi;
    }
    n = k;

    int ia = 0;
    int ng = 0;
    gr_roots(rez, imz, modul, ar, pr, n - 1, grez, gimz, gmodul, gar, gpr, gcont,
             ng);
    half_roots(grez, gimz, gmodul, gar, gpr, gcont, ng, rez1, imz1, modul1, ar1,
               pr1, ia);

    if (nnio == 1) {
        for (int i = 1; i <= ia; ++i) {
            if (std::abs(modul1[i] - 1.0) < 1.0e-8) {
                if (imz1[i] > tol || imz1[i] < -tol) {
                    double temp = rez1[i] / imz1[i];
                    rez1[i] = (temp * xl) / std::sqrt(temp * temp + 1.0);
                    imz1[i] = xl / std::sqrt(temp * temp + 1.0);
                    modul1[i] = xl;
                } else {
                    rez1[i] = xl;
                    modul1[i] = xl;
                }
            }
        }
    }

    // Build theta(B) from the invertible roots (MPBC), then variance / toterr.
    for (int i = 1; i <= 64; ++i) {
        az[i] = std::complex<double>(0.0, 0.0);
        bz[i] = std::complex<double>(0.0, 0.0);
    }
    ntheta = ia + 1;
    if (ia > 1) {
        az[1] = std::complex<double>(1.0, 0.0);
        az[2] = -std::complex<double>(rez1[1], imz1[1]);
        bz[1] = std::complex<double>(1.0, 0.0);
        bz[2] = -std::complex<double>(rez1[2], imz1[2]);
        mpbc(az + 1, bz + 1, 1, 1, bz + 1);
        if (ia > 2) {
            for (int i = 2; i <= ia - 1; ++i) {
                az[1] = std::complex<double>(1.0, 0.0);
                az[2] = -std::complex<double>(rez1[i + 1], imz1[i + 1]);
                mpbc(bz + 1, az + 1, i, 1, bz + 1);
            }
        }
        for (int i = 1; i <= ntheta; ++i) theta[i - 1] = bz[i].real();
    }
    if (ia == 1) {
        theta[0] = 1.0;
        theta[1] = -rez1[1];
        ntheta = 2;
    }

    var = 0.0;
    for (int i = 1; i <= ia + 1; ++i) var = var + theta[i - 1] * theta[i - 1];
    var = gamzer / var;

    // toterr: CONJ(theta,theta) is the factored ACF; compare to ufin.
    conj(theta, ntheta, theta, ntheta, vn + 1, nvn);
    toterr = 0.0;
    for (int i = 1; i <= nvn; ++i)
        toterr = toterr + (vn[i] * var - ufin1[i]) * (vn[i] * var - ufin1[i]);
}

}  // namespace x13
