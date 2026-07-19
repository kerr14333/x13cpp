// numeric.cpp -- Tier-0 numeric leaves (see numeric.hpp). Faithful ports of the
// vendored oracle Fortran: dpmpar.f, dpeq.f, scrmlt.f, maxvec.f, dcopy.f,
// daxpy.f, ddot.f (+ its UNDERFLOW helper), revrse.f, enorm.f.
#include "numeric/numeric.hpp"

#include <cmath>
#include <vector>

namespace x13 {

// dpmpar.f -- active DATA dmach/2.220446d-16,2.225074d-308,1.797693d308/.
// Literals reproduced exactly (truncated, per the oracle's Solaris DATA block).
double dpmpar(int i) {
    static const double dmach[3] = {2.220446e-16, 2.225074e-308, 1.797693e308};
    return dmach[i - 1];
}

// dpeq.f -- PARAMETER(DELTA=3.834D-20); dpeq = |x-dtargt| < DELTA.
bool dpeq(double x, double dtargt) {
    const double DELTA = 3.834e-20;
    return std::fabs(x - dtargt) < DELTA;
}

// scrmlt.f
void scrmlt(double c, int n, double* x) {
    for (int i = 0; i < n; ++i) x[i] = c * x[i];
}

// maxvec.f
void maxvec(const double* dx, int n, double& mxabvl) {
    mxabvl = 0.0;
    if (n <= 0) return;
    for (int i = 0; i < n; ++i) {
        double xmag = std::fabs(dx[i]);
        if (xmag > mxabvl) mxabvl = xmag;
    }
}

// dcopy.f -- unit-stride path is 7-way unrolled in the oracle; the unrolling is
// value-neutral for a copy but preserved for faithfulness.
void dcopy(int n, const double* dx, int incx, double* dy, int incy) {
    if (n <= 0) return;
    if (incx != 1 || incy != 1) {
        int ix = 1, iy = 1;
        if (incx < 0) ix = (1 - n) * incx + 1;
        if (incy < 0) iy = (1 - n) * incy + 1;
        for (int i = 0; i < n; ++i) {
            dy[iy - 1] = dx[ix - 1];
            ix += incx;
            iy += incy;
        }
        return;
    }
    int m = n % 7;
    for (int i = 0; i < m; ++i) dy[i] = dx[i];
    for (int i = m; i < n; i += 7) {
        dy[i] = dx[i];
        dy[i + 1] = dx[i + 1];
        dy[i + 2] = dx[i + 2];
        dy[i + 3] = dx[i + 3];
        dy[i + 4] = dx[i + 4];
        dy[i + 5] = dx[i + 5];
        dy[i + 6] = dx[i + 6];
    }
}

// daxpy.f -- early-out on dpeq(da,0); unit-stride path 4-way unrolled.
void daxpy(int n, double da, const double* dx, int incx, double* dy, int incy) {
    if (n <= 0 || dpeq(da, 0.0)) return;
    if (incx == incy && incx == 1) {
        int m = n % 4;
        if (m != 0) {
            for (int i = 0; i < m; ++i) dy[i] = dy[i] + da * dx[i];
            if (n < 4) return;
        }
        for (int i = m; i < n; i += 4) {
            dy[i] = dy[i] + da * dx[i];
            dy[i + 1] = dy[i + 1] + da * dx[i + 1];
            dy[i + 2] = dy[i + 2] + da * dx[i + 2];
            dy[i + 3] = dy[i + 3] + da * dx[i + 3];
        }
        return;
    }
    if (incx == incy && incx > 1) {  // equal positive nonunit increments
        int ns = n * incx;
        for (int i = 0; i < ns; i += incx) dy[i] = da * dx[i] + dy[i];
        return;
    }
    // nonequal or nonpositive increments
    int ix = 1, iy = 1;
    if (incx < 0) ix = (-n + 1) * incx + 1;
    if (incy < 0) iy = (-n + 1) * incy + 1;
    for (int i = 0; i < n; ++i) {
        dy[iy - 1] = dy[iy - 1] + da * dx[ix - 1];
        ix += incx;
        iy += incy;
    }
}

// ddot.f UNDERFLOW helper: TRUE if x or y is zero, or the product would underflow
// below dpmpar(2). Terms flagged TRUE are omitted from the accumulation.
namespace {
bool underflow(double x, double y) {
    if (x == 0.0 || y == 0.0) return true;
    double dmin = std::log10(dpmpar(2));
    double xp = std::log10(std::fabs(x)) + std::log10(std::fabs(y));
    if (xp <= dmin) return true;
    return false;
}
}  // namespace

// ddot.f
double ddot(int n, const double* dx, int incx, const double* dy, int incy) {
    double d = 0.0;
    if (n <= 0) return d;
    if (incx == incy) {
        if (incx == 1) {
            int m = n % 5;
            if (m != 0) {
                for (int i = 0; i < m; ++i)
                    if (!underflow(dx[i], dy[i])) d += dx[i] * dy[i];
                if (n < 5) return d;
            }
            for (int i = m; i < n; i += 5) {
                if (!underflow(dx[i], dy[i])) d += dx[i] * dy[i];
                if (!underflow(dx[i + 1], dy[i + 1])) d += dx[i + 1] * dy[i + 1];
                if (!underflow(dx[i + 2], dy[i + 2])) d += dx[i + 2] * dy[i + 2];
                if (!underflow(dx[i + 3], dy[i + 3])) d += dx[i + 3] * dy[i + 3];
                if (!underflow(dx[i + 4], dy[i + 4])) d += dx[i + 4] * dy[i + 4];
            }
            return d;
        }
        if (incx > 1) {
            int ns = n * incx;
            for (int i = 0; i < ns; i += incx)
                if (!underflow(dx[i], dy[i])) d += dx[i] * dy[i];
            return d;
        }
        // incx == incy <= 0: oracle computes nothing (returns 0).
        return d;
    }
    // unequal or nonpositive increments
    int ix = 1, iy = 1;
    if (incx < 0) ix = (-n + 1) * incx + 1;
    if (incy < 0) iy = (-n + 1) * incy + 1;
    for (int i = 0; i < n; ++i) {
        if (!underflow(dx[ix - 1], dy[iy - 1])) d += dx[ix - 1] * dy[iy - 1];
        ix += incx;
        iy += incy;
    }
    return d;
}

// revrse.f -- Frwd(Nc,Nr): element (row i, col j) at (i-1)*Nc + (j-1).
void revrse(const double* frwd, int nr, int nc, double* bkwd) {
    int hlf = (nr + 1) / 2;
    for (int i = 1; i <= hlf; ++i) {
        int backi = nr - i + 1;
        for (int j = 1; j <= nc; ++j) {
            double tmp = frwd[(i - 1) * nc + (j - 1)];
            bkwd[(i - 1) * nc + (j - 1)] = frwd[(backi - 1) * nc + (j - 1)];
            bkwd[(backi - 1) * nc + (j - 1)] = tmp;
        }
    }
}

// enorm.f -- MINPACK 3-bin scaled Euclidean norm.
double enorm(int n, const double* x) {
    const double ZERO = 0.0, ONE = 1.0, RDWARF = 3.834e-20, RGIANT = 1.304e19;
    double s1 = ZERO, s2 = ZERO, s3 = ZERO, x1max = ZERO, x3max = ZERO;
    double floatn = n;
    double agiant = RGIANT / floatn;
    for (int i = 0; i < n; ++i) {
        double xabs = std::fabs(x[i]);
        if (xabs > RDWARF && xabs < agiant) {
            // intermediate components
            s2 = s2 + xabs * xabs;
        } else if (xabs <= RDWARF) {
            // small components
            if (xabs <= x3max) {
                if (!dpeq(xabs, ZERO)) s3 = s3 + (xabs / x3max) * (xabs / x3max);
            } else {
                s3 = ONE + s3 * (x3max / xabs) * (x3max / xabs);
                x3max = xabs;
            }
        } else if (xabs <= x1max) {
            // large components
            s1 = s1 + (xabs / x1max) * (xabs / x1max);
        } else {
            s1 = ONE + s1 * (x1max / xabs) * (x1max / xabs);
            x1max = xabs;
        }
    }
    double result;
    if (!dpeq(s1, ZERO)) {
        result = x1max * std::sqrt(s1 + (s2 / x1max) / x1max);
    } else if (dpeq(s2, ZERO) && s3 > ZERO) {
        result = x3max * std::sqrt(s3);
    } else {
        if (dpeq(x3max, ZERO) && dpeq(std::sqrt(s2), ZERO)) {
            result = ZERO;
        } else if (s2 >= x3max) {
            result = std::sqrt(s2 * (ONE + (x3max / s2) * (x3max * s3)));
        } else {
            result = std::sqrt(x3max * ((s2 / x3max) + (x3max * s3)));
        }
    }
    return result;
}

// yprmy.f -- sum of squares, sequential accumulation (Y(i)**2 -> y*y).
void yprmy(const double* y, int nr, double& ypy) {
    ypy = 0.0;
    for (int i = 1; i <= nr; ++i) ypy = ypy + y[i - 1] * y[i - 1];
}

// logdet.f -- packed diagonal walk ielt += i (1,3,6,...); 2*log of each diag.
void logdet(const double* ap, int n, double& lgdt) {
    lgdt = 0.0;
    int ielt = 0;
    for (int i = 1; i <= n; ++i) {
        ielt = ielt + i;
        lgdt = lgdt + 2.0 * std::log(ap[ielt - 1]);
    }
}

// uconv.f -- MA autocovariance in place; both arrays 0-based (0..mxmalg).
void uconv(const double* fulma, int mxmalg, double* c) {
    for (int i = 0; i <= mxmalg; ++i) c[i] = fulma[i];
    for (int i = 0; i <= mxmalg; ++i) {
        double sum = fulma[i];
        int qmi = mxmalg - i;
        for (int k = 1; k <= qmi; ++k) sum = sum + fulma[k] * c[i + k];
        c[i] = sum;
    }
}

// xpand.f -- expansion of A(z)/B(z) to order nc, in place. Local workspace `a`
// snapshots the numerator c[0..na] before the recursion overwrites c.
void xpand(const double* b, int mxarlg, int na, int nc, double* c, int pc) {
    constexpr double ZERO = 0.0;
    std::vector<double> a(pc + 1, ZERO);
    for (int i = 0; i <= na; ++i) a[i] = c[i];
    for (int i = 0; i <= nc; ++i) {
        int nlag = (mxarlg < i) ? mxarlg : i;
        double sum = (i <= na) ? a[i] : ZERO;
        for (int w = 1; w <= nlag; ++w) sum = sum - b[w] * c[i - w];
        c[i] = sum;
    }
}

// xprmx.f -- packed [X:y]'[X:y] via strided (underflow-skipping) ddot; column c
// is the base pointer xy+(c-1) with stride pcxy.
void xprmx(const double* xy, int nspobs, int ncxy, int pcxy, double* xypxy) {
    int ielt = 0;
    for (int i = 1; i <= ncxy; ++i) {
        for (int j = 1; j <= i; ++j) {
            ielt = ielt + 1;
            xypxy[ielt - 1] = ddot(nspobs, xy + (i - 1), pcxy, xy + (j - 1), pcxy);
        }
    }
    if (pcxy > ncxy) {
        for (int j = 1; j <= ncxy; ++j) {
            ielt = ielt + 1;
            xypxy[ielt - 1] =
                ddot(nspobs, xy + (j - 1), pcxy, xy + (pcxy - 1), pcxy);
        }
        xypxy[ielt] =
            ddot(nspobs, xy + (pcxy - 1), pcxy, xy + (pcxy - 1), pcxy);
    }
}

// dppfa.f -- Census-modified packed Cholesky. kj/kk/jj walk the packed columns;
// the tolerated-zero branch exits via `return` with info still = j.
void dppfa(double* ap, int n, int& info) {
    double mprec = dpmpar(1);
    int jj = 0;
    for (int j = 1; j <= n; ++j) {
        info = j;
        double s = 0.0;
        int jm1 = j - 1;
        int kj = jj;
        int kk = 0;
        if (jm1 >= 1) {
            for (int k = 1; k <= jm1; ++k) {
                kj = kj + 1;
                double t = ap[kj - 1] - ddot(k - 1, ap + kk, 1, ap + jj, 1);
                kk = kk + k;
                t = t / ap[kk - 1];
                ap[kj - 1] = t;
                s = s + t * t;
            }
        }
        jj = jj + j;
        s = ap[jj - 1] - s;
        if (s > 0.0) {
            ap[jj - 1] = std::sqrt(s);
        } else {
            if (s >= -mprec) ap[jj - 1] = 0.0;
            return;  // GO TO 10: exit with info = j (non-PD, incl. tolerated 0).
        }
    }
    info = 0;
}

// dsolve.f -- b is column-major nc x nr: b[(j-1)+(i-1)*nc]. Mixed-stride
// ddot/daxpy (1 and nc) exercise the BLAS unequal-increment paths.
void dsolve(const double* a, int nr, int nc, bool lainvb, double* b) {
    int ielt = 0;
    // Solve R'w = b.
    for (int i = 1; i <= nr; ++i) {
        double diag = a[ielt + i - 1];
        for (int j = 1; j <= nc; ++j) {
            double sum = ddot(i - 1, a + ielt, 1, b + (j - 1), nc);
            b[(j - 1) + (i - 1) * nc] = (b[(j - 1) + (i - 1) * nc] - sum) / diag;
        }
        ielt = ielt + i;
    }
    // Solve R x = w.
    if (lainvb) {
        for (int ib = 1; ib <= nr; ++ib) {
            int i = nr + 1 - ib;
            double diag = a[ielt - 1];
            ielt = ielt - i;
            for (int j = 1; j <= nc; ++j) {
                b[(j - 1) + (i - 1) * nc] = b[(j - 1) + (i - 1) * nc] / diag;
                daxpy(i - 1, -b[(j - 1) + (i - 1) * nc], a + ielt, 1,
                      b + (j - 1), nc);
            }
        }
    }
}

// euclid.f -- fular/g 0-based; b/a 1-based workspace (b[i-1],a[i-1]).
void euclid(const double* fular, double* b, double* a, int maxpq, int mxarlg,
            int mxmalg, double* g, int& err) {
    constexpr double ONE = 1.0, TWO = 2.0, ZERO = 0.0;
    for (int i = 1; i <= mxarlg; ++i) b[i - 1] = fular[i];
    err = 0;
    // Order-reduction loop.
    for (int i = maxpq; i >= 1; --i) {
        if (i <= mxarlg) {
            double r = b[i - 1];
            if (std::fabs(r) > ONE) {
                err = 1;
                return;  // GO TO 10: early exit, g left partially modified.
            }
            double s = ONE / (ONE - r * r);
            a[i - 1] = s;
            int midpt = i / 2;
            for (int fsthlf = 1; fsthlf <= midpt; ++fsthlf) {
                int lsthlf = i - fsthlf;
                double bs = b[fsthlf - 1];
                double br = b[lsthlf - 1];
                b[fsthlf - 1] = (bs - br * r) * s;
                b[lsthlf - 1] = (br - bs * r) * s;
            }
        }
        int lim = i - 1;
        if (i > mxarlg) lim = mxarlg;
        if (i > mxmalg) {
            lim = 0;
            g[i] = ZERO;
        }
        for (int fsthlf = 1; fsthlf <= lim; ++fsthlf) {
            int lsthlf = i - fsthlf;
            g[lsthlf] = g[lsthlf] - b[fsthlf - 1] * g[i];
        }
    }
    // Construction loop.
    g[0] = g[0] / TWO;
    for (int i = 1; i <= mxarlg; ++i) {
        int midpt = i / 2;
        for (int fsthlf = 0; fsthlf <= midpt; ++fsthlf) {
            int lsthlf = i - fsthlf;
            double gs = g[fsthlf];
            double gr = g[lsthlf];
            g[fsthlf] = (gs - b[i - 1] * gr) * a[i - 1];
            g[lsthlf] = (gr - b[i - 1] * gs) * a[i - 1];
        }
    }
}

}  // namespace x13
