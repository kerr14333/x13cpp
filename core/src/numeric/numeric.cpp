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

}  // namespace x13
