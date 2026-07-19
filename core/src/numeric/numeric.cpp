// numeric.cpp -- Tier-0 numeric leaves (see numeric.hpp). Faithful ports of the
// vendored oracle Fortran: dpmpar.f, dpeq.f, scrmlt.f, maxvec.f, dcopy.f,
// daxpy.f, ddot.f (+ its UNDERFLOW helper), revrse.f, enorm.f.
#include "numeric/numeric.hpp"

#include <cmath>
#include <limits>
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

// dppsl.f -- LINPACK single-RHS packed-Cholesky solve (see hpp). Packed
// upper-triangular column storage: kk accumulates 1,3,6,... so ap[kk-1] is the
// k-th diagonal and ap+kk (Fortran Ap(kk+1)) starts column k's off-diagonal.
void dppsl(const double* ap, int n, double* b, bool alt) {
    int kk = 0;
    // Forward: solve L w = b (L from a = L L').
    for (int k = 1; k <= n; ++k) {
        double t = ddot(k - 1, ap + kk, 1, b, 1);
        kk = kk + k;
        b[k - 1] = (b[k - 1] - t) / ap[kk - 1];
    }
    // The Census `alt` option stops after the forward solve (L x = b).
    if (alt) return;
    // Back: solve L' x = w, completing a x = b.
    for (int kb = 1; kb <= n; ++kb) {
        int k = n + 1 - kb;
        b[k - 1] = b[k - 1] / ap[kk - 1];
        kk = kk - k;
        double t = -b[k - 1];
        daxpy(k - 1, t, ap + kk, 1, b, 1);
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

// dscal.f -- BLAS scale dx <- da*dx (strided). Pure scaling: no FP-order
// dependence, so the oracle's mod-5 unrolling is elided losslessly.
void dscal(int n, double da, double* dx, int incx) {
    if (n <= 0) return;
    int idx = 0;
    for (int i = 0; i < n; ++i) {
        dx[idx] = da * dx[idx];
        idx += incx;
    }
}

// shlsrt.f -- ascending shell sort, in place (1-based logic preserved via the
// -1 offsets). gap starts at nr, is halved each outer pass; within a pass,
// gap-separated pairs are bubbled down until ordered.
void shlsrt(int nr, double* vecx) {
    int gap = nr;
    while (true) {
        gap = gap / 2;
        if (gap > 0) {
            int nsrt = nr - gap;
            int bot = 0;
            bool exit_bot = false;
            while (!exit_bot) {           // bot loop
                bot = bot + 1;
                if (bot <= nsrt) {
                    while (true) {         // exchange loop
                        int top = bot + gap;
                        if (vecx[bot - 1] <= vecx[top - 1]) break;  // -> label 10
                        double tmp = vecx[top - 1];
                        vecx[top - 1] = vecx[bot - 1];
                        vecx[bot - 1] = tmp;
                        if (bot <= gap) break;                      // -> label 10
                        bot = bot - gap;
                    }
                    // label 10: fall through to next bot iteration.
                } else {
                    exit_bot = true;       // GO TO 20: halve gap again
                }
            }
        } else {
            return;                        // gap==0: done
        }
    }
}

// medabs.f -- median of |s[i]|. Sorts the absolute values (shlsrt), then takes
// the central order statistic (average of the two middle for even nr).
void medabs(const double* s, int nr, double& median) {
    std::vector<double> abss(nr > 0 ? nr : 1);
    for (int i = 0; i < nr; ++i) abss[i] = std::fabs(s[i]);
    shlsrt(nr, abss.data());
    int midpt = nr / 2;   // 1-based midpt; 0-based central index is midpt-1/midpt
    if (nr % 2 == 0) {
        median = (abss[midpt - 1] + abss[midpt]) / 2.0;
    } else {
        median = abss[midpt];   // abss(midpt+1) 1-based == abss[midpt] 0-based
    }
}

// dppdi.f -- determinant/inverse of an SPD matrix from its packed Cholesky
// factor (LINPACK). Packed upper-triangle indexing preserved 1-based via kk/j1/
// kj/etc. det = det[0]*10^det[1] with 1<=det[0]<10 (or 0). ap overwritten with
// the packed inverse when the inverse is requested.
void dppdi(double* ap, int n, double* det, int job) {
    constexpr double ZERO = 0.0, ONE = 1.0;
    // Determinant.
    if (job / 10 != 0) {
        det[0] = ONE;
        det[1] = ZERO;
        const double s = 10.0;
        int ii = 0;
        for (int i = 1; i <= n; ++i) {
            ii += i;
            det[0] = ap[ii - 1] * ap[ii - 1] * det[0];
            if (!dpeq(det[0], ZERO)) {
                while (det[0] < ONE) {
                    det[0] = s * det[0];
                    det[1] = det[1] - ONE;
                }
                while (det[0] >= s) {
                    det[0] = det[0] / s;
                    det[1] = det[1] + ONE;
                }
            }
        }
    }
    // Inverse.
    if (job % 10 != 0) {
        // inverse(r)
        int kk = 0;
        for (int k = 1; k <= n; ++k) {
            int k1 = kk + 1;
            kk += k;
            ap[kk - 1] = ONE / ap[kk - 1];
            double t = -ap[kk - 1];
            dscal(k - 1, t, &ap[k1 - 1], 1);
            int kp1 = k + 1;
            int j1 = kk + 1;
            int kj = kk + k;
            if (n >= kp1) {
                for (int j = kp1; j <= n; ++j) {
                    t = ap[kj - 1];
                    ap[kj - 1] = ZERO;
                    daxpy(k, t, &ap[k1 - 1], 1, &ap[j1 - 1], 1);
                    j1 += j;
                    kj += j;
                }
            }
        }
        // inverse(r) * trans(inverse(r))
        int jj = 0;
        for (int j = 1; j <= n; ++j) {
            int j1 = jj + 1;
            jj += j;
            int jm1 = j - 1;
            int k1 = 1;
            int kj = j1;
            if (jm1 >= 1) {
                for (int k = 1; k <= jm1; ++k) {
                    double t = ap[kj - 1];
                    daxpy(k, t, &ap[j1 - 1], 1, &ap[k1 - 1], 1);
                    k1 += k;
                    kj += 1;
                }
            }
            double t = ap[jj - 1];
            dscal(j, t, &ap[j1 - 1], 1);
        }
    }
}

// eltfcn.f -- elementwise cvec = avec <op> bvec over nelt elements. Oprn:
// ADD=1, SUB=2, MULT=3, DIV=4. The Fortran Pc argument only sizes Cvec's
// declaration (writes are 1..Nelt); C++ callers guarantee cvec length, so it is
// dropped. avec/bvec/cvec may alias (prtfct calls it in place).
void eltfcn(int oprn, const double* avec, const double* bvec, int nelt,
            double* cvec) {
    for (int i = 0; i < nelt; ++i) {
        switch (oprn) {
            case ELT_ADD:  cvec[i] = avec[i] + bvec[i]; break;
            case ELT_SUB:  cvec[i] = avec[i] - bvec[i]; break;
            case ELT_MULT: cvec[i] = avec[i] * bvec[i]; break;
            case ELT_DIV:  cvec[i] = avec[i] / bvec[i]; break;
        }
    }
}

// devlpl.f -- Horner evaluation of A(1)+A(2)X+...+A(N)X^(N-1). a is 0-based
// (a[0..n-1]); walks from the high-order term down.
double devlpl(const double* a, int n, double x) {
    double term = a[n - 1];
    for (int i = n - 1; i >= 1; --i) term = a[i - 1] + term * x;
    return term;
}

// stvaln.f -- STarting VALue for the Newton-Raphson normal-inverse iteration
// (Kennedy & Gentle rational approximation). Returns X with CUMNOR(X)~=P.
double stvaln(double p) {
    // DATA arrays kept 1-based (index [0] a dummy); devlpl reads &x_[1].
    static const double xnum[6] = {0.0, -0.322232431088, -1.000000000000,
                                   -0.342242088547, -0.204231210245e-1,
                                   -0.453642210148e-4};
    static const double xden[6] = {0.0, 0.993484626060e-1, 0.588581570495,
                                   0.531103462366, 0.103537752850,
                                   0.38560700634e-2};
    double xsign, z;
    if (p > 0.5) {
        xsign = 1.0;
        z = 1.0 - p;
    } else {
        xsign = -1.0;
        z = p;
    }
    double y = std::sqrt(-2.0 * std::log(z));
    double s = y + devlpl(&xnum[1], 5, y) / devlpl(&xden[1], 5, y);
    return xsign * s;
}

// cumnor.f -- cumulative normal (Cody ALGORITHM 715 / ANORM), returning both the
// CDF (result) and its complement (ccum) via near-minimax rational fits over
// three |x| intervals. The two machine constants come from spmpar: eps =
// spmpar(1)*0.5 = 2^-53 = 0.5*DBL_EPSILON, minx = spmpar(2) = 2^-1022 = DBL_MIN
// (exact under the oracle's active ipmpar DATA block), inlined rather than
// porting ipmpar/spmpar for two constants.
void cumnor(double arg, double& result, double& ccum) {
    // DATA arrays 1-based ([0] dummy) to mirror the Fortran indexing directly.
    static const double a[6] = {0.0, 2.2352520354606839287e00,
                                1.6102823106855587881e02,
                                1.0676894854603709582e03,
                                1.8154981253343561249e04,
                                6.5682337918207449113e-2};
    static const double b[5] = {0.0, 4.7202581904688241870e01,
                                9.7609855173777669322e02,
                                1.0260932208618978205e04,
                                4.5507789335026729956e04};
    static const double c[10] = {0.0, 3.9894151208813466764e-1,
                                 8.8831497943883759412e00,
                                 9.3506656132177855979e01,
                                 5.9727027639480026226e02,
                                 2.4945375852903726711e03,
                                 6.8481904505362823326e03,
                                 1.1602651437647350124e04,
                                 9.8427148383839780218e03,
                                 1.0765576773720192317e-8};
    static const double d[9] = {0.0, 2.2266688044328115691e01,
                                2.3538790178262499861e02,
                                1.5193775994075548050e03,
                                6.4855582982667607550e03,
                                1.8615571640885098091e04,
                                3.4900952721145977266e04,
                                3.8912003286093271411e04,
                                1.9685429676859990727e04};
    static const double p[7] = {0.0, 2.1589853405795699e-1,
                                1.274011611602473639e-1,
                                2.2235277870649807e-2,
                                1.421619193227893466e-3,
                                2.9112874951168792e-5,
                                2.307344176494017303e-2};
    static const double q[6] = {0.0, 1.28426009614491121e00,
                                4.68238212480865118e-1,
                                6.59881378689285515e-2,
                                3.78239633202758244e-3,
                                7.29751555083966205e-5};
    const double one = 1.0, half = 0.5, zero = 0.0, sixten = 1.60;
    const double sqrpi = 3.9894228040143267794e-1, thrsh = 0.66291,
                 root32 = 5.656854248;
    const double eps = 0.5 * std::numeric_limits<double>::epsilon();  // spmpar(1)*0.5
    const double minx = std::numeric_limits<double>::min();           // spmpar(2)

    double x = arg;
    double y = std::fabs(x);
    double xnum, xden, xsq, del, temp;
    if (y <= thrsh) {
        // |x| <= 0.66291
        xsq = zero;
        if (y > eps) xsq = x * x;
        xnum = a[5] * xsq;
        xden = xsq;
        for (int i = 1; i <= 3; ++i) {
            xnum = (xnum + a[i]) * xsq;
            xden = (xden + b[i]) * xsq;
        }
        result = x * (xnum + a[4]) / (xden + b[4]);
        temp = result;
        result = half + temp;
        ccum = half - temp;
    } else if (y <= root32) {
        // 0.66291 <= |x| <= sqrt(32)
        xnum = c[9] * y;
        xden = y;
        for (int i = 1; i <= 7; ++i) {
            xnum = (xnum + c[i]) * y;
            xden = (xden + d[i]) * y;
        }
        result = (xnum + c[8]) / (xden + d[8]);
        xsq = std::trunc(y * sixten) / sixten;  // aint
        del = (y - xsq) * (y + xsq);
        result = std::exp(-xsq * xsq * half) * std::exp(-del * half) * result;
        ccum = one - result;
        if (x > zero) {
            temp = result;
            result = ccum;
            ccum = temp;
        }
    } else {
        // |x| > sqrt(32)
        result = zero;
        xsq = one / (x * x);
        xnum = p[6] * xsq;
        xden = xsq;
        for (int i = 1; i <= 4; ++i) {
            xnum = (xnum + p[i]) * xsq;
            xden = (xden + q[i]) * xsq;
        }
        result = xsq * (xnum + p[5]) / (xden + q[5]);
        result = (sqrpi - result) / y;
        xsq = std::trunc(x * sixten) / sixten;
        del = (x - xsq) * (x + xsq);
        result = std::exp(-xsq * xsq * half) * std::exp(-del * half) * result;
        ccum = one - result;
        if (x > zero) {
            temp = result;
            result = ccum;
            ccum = temp;
        }
    }
    if (result < minx) result = 0.0;
    if (ccum < minx) ccum = 0.0;
}

// dinvnr.f -- inverse normal CDF: returns X with CUMNOR(X)=P, via a Newton
// iteration seeded by stvaln. qporq picks min(P,Q) to iterate on the closer
// tail; the sign is restored on the not-qporq branch. On non-convergence
// (MAXIT=100) it returns the (signed) starting value, per the Fortran.
double dinvnr(double p, double q) {
    const int MAXIT = 100;
    const double EPS = 1.0e-13;
    const double R2PI = 0.3989422804014326;
    const double NHALF = -0.5;
    bool qporq = p <= q;
    double pp = qporq ? p : q;
    double strtx = stvaln(pp);
    double xcur = strtx;
    for (int i = 1; i <= MAXIT; ++i) {
        double cum, ccum;
        cumnor(xcur, cum, ccum);
        double dennor = R2PI * std::exp(NHALF * xcur * xcur);
        double dx = (cum - pp) / dennor;
        xcur = xcur - dx;
        if (std::fabs(dx / xcur) < EPS) {  // converged
            return qporq ? xcur : -xcur;
        }
    }
    // Newton failed.
    return qporq ? strtx : -strtx;
}

}  // namespace x13
