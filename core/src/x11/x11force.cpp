// x11force.cpp -- X-11 force-yearly-totals benchmarking. See x11force.hpp.
//
// Faithful translation of oracle/fortran/qmap.f. The convolution weights are the
// fixed modified-Denton coefficients from qmap.f's DATA statements, concatenated
// exactly as the Fortran EQUIVALENCE lays them out in w(200):
//   w(1..50)   = wq   -- quarterly (ny==4)
//   w(51..125) = wm1  -- monthly (ny==12), first half
//   w(126..200)= wm2  -- monthly, second half
// so the monthly path indexes from offset jy=50 into wm1/wm2 and the quarterly
// path from 0 into wq, matching the Fortran. All indexing is kept 1-based (access
// kW[ij-1]) to track the source line-for-line.
#include "x11/x11force.hpp"

#include <cmath>
#include <vector>

#include "numeric/numeric.hpp"  // dpeq

namespace x13 {
namespace {

// qmap.f DATA wq / wm1 / wm2, in that order (50 + 75 + 75 = 200).
const double kW[200] = {
    // wq (quarterly)
    0.3101014, -0.0745478, 0.0179083, -0.0042489, 0.0007868,
    0.2860609, -0.0447286, 0.0107450, -0.0025492, 0.0004721,
    0.2379797, 0.0149095, -0.0035817, 0.0008498, -0.0001575,
    0.1658580, 0.1043667, -0.0250716, 0.0059485, -0.0011016,
    0.0696957, 0.2236430, -0.0537249, 0.0127467, -0.0023605,
    0.0033526, 0.2818963, -0.0436963, 0.0103673, -0.0019199,
    -0.0331716, 0.2791267, 0.0050143, -0.0011897, 0.0002203,
    -0.0398767, 0.2153340, 0.0924069, -0.0219243, 0.0040601,
    -0.0167627, 0.0905184, 0.2184814, -0.0518365, 0.0095994,
    -0.0008120, 0.0043849, 0.2815186, -0.0430668, 0.0079753,
    // wm1 (monthly, first half)
    0.1053950, -0.0279005, 0.0073776, -0.0019194, 0.0003807,
    0.1044693, -0.0267298, 0.0070680, -0.0018389, 0.0003647,
    0.1026180, -0.0243885, 0.0064489, -0.0016778, 0.0003327,
    0.0998410, -0.0208766, 0.0055203, -0.0014362, 0.0002849,
    0.0961383, -0.0161940, 0.0042821, -0.0011141, 0.0002210,
    0.0915100, -0.0103407, 0.0027344, -0.0007114, 0.0001410,
    0.0859560, -0.0033168, 0.0008771, -0.0002282, 0.0000453,
    0.0794764, 0.0048777, -0.0012898, 0.0003356, -0.0000666,
    0.0720711, 0.0142429, -0.0037662, 0.0009799, -0.0001943,
    0.0637402, 0.0247787, -0.0065521, 0.0017046, -0.0003381,
    0.0544835, 0.0364852, -0.0096476, 0.0025100, -0.0004978,
    0.0443012, 0.0493624, -0.0130527, 0.0033959, -0.0006735,
    0.0331933, 0.0634102, -0.0167673, 0.0043624, -0.0008652,
    0.0232560, 0.0750521, -0.0189211, 0.0049227, -0.0009764,
    0.0144893, 0.0842882, -0.0195142, 0.0050770, -0.0010070,
    // wm2 (monthly, second half)
    0.0068933, 0.0911184, -0.0185466, 0.0048253, -0.0009570,
    0.0004679, 0.0955427, -0.0160183, 0.0041675, -0.0008267,
    -0.0047868, 0.0975612, -0.0119292, 0.0031037, -0.0006156,
    -0.0088708, 0.0971739, -0.0062794, 0.0016337, -0.0003240,
    -0.0117842, 0.0943806, 0.0009312, -0.0002423, 0.0000480,
    -0.0135270, 0.0891815, 0.0097025, -0.0025243, 0.0005007,
    -0.0140991, 0.0815765, 0.0200345, -0.0052124, 0.0010338,
    -0.0135006, 0.0715657, 0.0319272, -0.0083065, 0.0016475,
    -0.0117315, 0.0591490, 0.0453807, -0.0118068, 0.0023417,
    -0.0087915, 0.0443265, 0.0603949, -0.0157130, 0.0031165,
    -0.0061612, 0.0310647, 0.0729068, -0.0180586, 0.0035816,
    -0.0038405, 0.0193636, 0.0829163, -0.0188434, 0.0037373,
    -0.0018293, 0.0092233, 0.0904234, -0.0180674, 0.0035834,
    -0.0001277, 0.0006437, 0.0954281, -0.0157308, 0.0031200,
    0.0012644, -0.0063752, 0.0979305, -0.0118334, 0.0023470};

}  // namespace

void qmap(const double* series, const double* stci, double* stci2, int lfda,
          int llda, int ny, int& ns, int& ne, int nyrt) {
    double r[187] = {0.0};  // r(186), 1-based; per-year target-vs-SA discrepancy.

    // Bracket the first/last full year contained in [lfda,llda] (qmap.f:57-66).
    ns = lfda;
    const int ntest = (lfda - 1) / ny * ny + nyrt;
    if (ntest > lfda) ns = ntest;
    if (ntest < lfda) ns = ntest + ny;
    int ny2 = nyrt - 1;
    if (ny2 == 0) ny2 = ny;
    ne = (llda / ny * ny) - (ny - ny2);
    if ((llda - ne) >= ny) ne = ne + ny;
    const int n1 = (ns - 1) / ny + 1;
    const int n2 = ne / ny;

    // Annual discrepancy r(i) = sum over year i of (target - SA).
    for (int i = n1; i <= n2; ++i) {
        const int n3 = (i - 1) * ny + nyrt;
        const int n4 = i * ny + (nyrt - 1);
        r[i] = 0.0;
        for (int j = n3; j <= n4; ++j) r[i] += series[j - 1] - stci[j - 1];
    }

    const int jy = (ny == 12) ? 50 : 0;
    const int k1 = 2 * ny;

    // Ends: the first/last 2*ny observations use the boundary weight rows.
    for (int i = 1; i <= k1; ++i) {
        const int i1 = ns + i - 1;
        const int i2 = ne - i + 1;
        double tmp1 = stci[i1 - 1];
        double tmp2 = stci[i2 - 1];
        const int ii = (i - 1) * 5 + jy;
        for (int j = 1; j <= 5; ++j) {
            const int j1 = n1 + j - 1;
            const int j2 = n2 - j + 1;
            const int ij = ii + j;
            tmp1 += r[j1] * kW[ij - 1];
            tmp2 += r[j2] * kW[ij - 1];
        }
        stci2[i1 - 1] = tmp1;
        stci2[i2 - 1] = tmp2;
    }

    // Interior: the symmetric 5-year (10-coefficient) benchmark filter.
    const int l1 = n1 + 2;
    const int l2 = n2 - 2;
    for (int l = l1; l <= l2; ++l) {
        const int k2 = (l - 1) * ny + nyrt;
        const int k3 = k2 + ny / 2 - 1;
        const int j1 = l - 2;
        const int j2 = l + 2;
        for (int i = k2; i <= k3; ++i) {
            const int i2 = 2 * k2 + ny - i - 1;
            double tmp1 = stci[i - 1];
            double tmp2 = stci[i2 - 1];
            const int ii = 5 * (i + k1 - k2) + 1 + jy;
            for (int j = j1; j <= j2; ++j) {
                const int ij = ii + j - j1;
                tmp1 += r[j] * kW[ij - 1];
                tmp2 += r[j2 + j1 - j] * kW[ij - 1];
            }
            stci2[i - 1] = tmp1;
            stci2[i2 - 1] = tmp2;
        }
    }
}

// ---------------------------------------------------------------------------
// qmap2 (Cholette-Dagum regression benchmarking) and its matrix helpers.
//
// Matrices are stored column-major with an explicit leading dimension `lda`,
// matching the Fortran A(Ia,*) convention: element (i,j) (1-based) lives at
// A[(j-1)*lda + (i-1)]. The oracle sizes every matrix at PLEN/PYRS; the result
// is independent of the leading dimension (it only sets the storage stride), so
// the port allocates each matrix at its actual working size and passes that as
// lda -- numerically identical, far less memory.
// ---------------------------------------------------------------------------
namespace {

inline int fidx(int lda, int i, int j) { return (j - 1) * lda + (i - 1); }

// gfortran integer power (x**n, n>=0): exponentiation by squaring, matching
// _gfortran_pow_r8_i4's multiply order bit-for-bit (used for Omec = rol**|i-j|,
// where pow() could differ in the last ulp).
double ipow_nonneg(double base, int n) {
    double result = 1.0, b = base;
    int e = n;
    for (;;) {
        if (e & 1) result *= b;
        e >>= 1;
        if (e == 0) break;
        b *= b;
    }
    return result;
}

// mult.f: C(m,q) = A(m,p) * B(p,q), column-major with leading dims ia/ib/ic.
void matmlt(const double* A, const double* B, double* C, int m, int ip, int iq,
            int ia, int ib, int ic) {
    for (int ir = 1; ir <= m; ++ir)
        for (int is = 1; is <= iq; ++is) {
            double sum = 0.0;
            for (int i = 1; i <= ip; ++i)
                sum += A[fidx(ia, ir, i)] * B[fidx(ib, i, is)];
            C[fidx(ic, ir, is)] = sum;
        }
}

// addsub.f: C = A +/- B over an n x m block (leading dim id). ind>0 adds.
void add_sub(const double* A, const double* B, double* C, int n, int m, int id,
             int ind) {
    for (int i = 1; i <= n; ++i)
        for (int j = 1; j <= m; ++j)
            C[fidx(id, i, j)] = (ind > 0) ? A[fidx(id, i, j)] + B[fidx(id, i, j)]
                                          : A[fidx(id, i, j)] - B[fidx(id, i, j)];
}

// meancra.f: spread the annual correction ratios A1x/Aty over Mq periods/year.
void meancra(const double* a1x, const double* aty, double* rtz, int modlid,
             int mq, int ny) {
    for (int i = 1; i <= ny; ++i) {
        const double tt = (modlid == 0) ? a1x[i - 1] / aty[i - 1] - 1.0
                                        : a1x[i - 1] - aty[i - 1];
        const int k = mq * (i - 1);
        const double val = (modlid == 0) ? tt : tt / mq;
        for (int j = 1; j <= mq; ++j) rtz[k + j - 1] = val;
    }
}

// simul.f: full-pivot Gauss-Jordan solve/inverse with determinant. `a` is n x n
// (plus column n+1 when indic>=0), leading dim ia; overwritten with the inverse
// (indic<=0) and/or the solution in x (indic>=0). Returns the determinant, or 0
// if singular (|pivot|<=eps). Faithful port including the row/col-order restore.
double simul(int n, double* a, double* x, double eps, int indic, int ia) {
    std::vector<double> irow(n + 1, 0.0), jcol(n + 1, 0.0), jord(n + 1, 0.0),
        y(n + 1, 0.0);
    const int imax = (indic >= 0) ? n + 1 : n;
    if (n > 396) return 0.0;

    double deter = 1.0;
    for (int k = 1; k <= n; ++k) {
        const int km1 = k - 1;
        double pivot = 0.0;
        for (int i = 1; i <= n; ++i) {
            for (int j = 1; j <= n; ++j) {
                bool skip = false;
                if (k != 1) {
                    for (int iscan = 1; iscan <= km1 && !skip; ++iscan)
                        for (int jscan = 1; jscan <= km1 && !skip; ++jscan) {
                            if (dpeq(static_cast<double>(i), irow[iscan]) ||
                                dpeq(static_cast<double>(j), jcol[jscan]))
                                skip = true;
                        }
                }
                if (skip) continue;
                if (std::fabs(a[fidx(ia, i, j)]) > std::fabs(pivot)) {
                    pivot = a[fidx(ia, i, j)];
                    irow[k] = static_cast<double>(i);
                    jcol[k] = static_cast<double>(j);
                }
            }
        }
        if (std::fabs(pivot) > eps) {
            const int irowk = static_cast<int>(irow[k]);
            const int jcolk = static_cast<int>(jcol[k]);
            deter *= pivot;
            for (int j = 1; j <= imax; ++j) a[fidx(ia, irowk, j)] /= pivot;
            a[fidx(ia, irowk, jcolk)] = 1.0 / pivot;
            for (int i = 1; i <= n; ++i) {
                const double aijck = a[fidx(ia, i, jcolk)];
                if (i != irowk) {
                    a[fidx(ia, i, jcolk)] = -aijck / pivot;
                    for (int j = 1; j <= imax; ++j)
                        if (j != jcolk)
                            a[fidx(ia, i, j)] -= aijck * a[fidx(ia, irowk, j)];
                }
            }
        } else {
            return 0.0;
        }
    }

    for (int i = 1; i <= n; ++i) {
        const int irowi = static_cast<int>(irow[i]);
        const int jcoli = static_cast<int>(jcol[i]);
        jord[irowi] = jcol[i];
        if (indic >= 0) x[jcoli - 1] = a[fidx(ia, irowi, imax)];
    }
    int intch = 0;
    for (int i = 1; i <= n - 1; ++i)
        for (int j = i + 1; j <= n; ++j)
            if (jord[j] < jord[i]) {
                const int jtemp = static_cast<int>(jord[j]);
                jord[j] = jord[i];
                jord[i] = static_cast<double>(jtemp);
                ++intch;
            }
    if (intch / 2 * 2 != intch) deter = -deter;

    if (indic <= 0) {
        for (int j = 1; j <= n; ++j) {
            for (int i = 1; i <= n; ++i) {
                const int irowi = static_cast<int>(irow[i]);
                const int jcoli = static_cast<int>(jcol[i]);
                y[jcoli] = a[fidx(ia, irowi, j)];
            }
            for (int i = 1; i <= n; ++i) a[fidx(ia, i, j)] = y[i];
        }
        for (int i = 1; i <= n; ++i) {
            for (int j = 1; j <= n; ++j) {
                const int irowj = static_cast<int>(irow[j]);
                const int jcolj = static_cast<int>(jcol[j]);
                y[irowj] = a[fidx(ia, i, jcolj)];
            }
            for (int j = 1; j <= n; ++j) a[fidx(ia, i, j)] = y[j];
        }
    }
    return deter;
}

// ceilng.f: smallest integer >= x, as a double (int64 truncation toward zero,
// then bump up when x has a positive fractional part).
double ceilng(double x) {
    double c = static_cast<double>(static_cast<long long>(x));
    if (x > c) c += 1.0;
    return c;
}

// round.f: round x to the nearest integer (ties away from zero), via ceilng.
int round_x13(double x) {
    const double c1 = ceilng(x - 0.5);
    const double c2 = ceilng(x);
    if (dpeq(c1, c2)) return static_cast<int>(c2);
    if (dpeq(c1, x - 0.5)) return static_cast<int>(c2);
    return static_cast<int>(c2) - 1;
}

// ssort.f (Kflag=2 case): Singleton quicksort of x[1..n] ascending, carrying y.
// NOT stable; the exact tie-ordering it leaves in y is load-bearing for rndsa
// (it selects which observation absorbs the rounding residual), so this is a
// line-for-line goto-preserving port of the carry-along path, not a std::sort.
// Arrays are 1-based (x[idx-1]).
void ssort2(double* x, double* y, int n) {
    auto X = [&](int idx) -> double& { return x[idx - 1]; };
    auto Y = [&](int idx) -> double& { return y[idx - 1]; };
    int il[22], iu[22];
    int m = 1, i = 1, j = n, k = 0, l = 0, ij = 0;
    double r = 0.375, tmp = 0.0, tt = 0.0, ty = 0.0, tty = 0.0;

L70:
    if (i == j) goto L100;
    if (r > 0.5898437) r -= 0.21875; else r += 3.90625e-2;
L80:
    k = i;
    ij = i + static_cast<int>(static_cast<double>(j - i) * r);
    tmp = X(ij);
    ty = Y(ij);
    if (X(i) > tmp) {
        X(ij) = X(i); X(i) = tmp; tmp = X(ij);
        Y(ij) = Y(i); Y(i) = ty;  ty = Y(ij);
    }
    l = j;
    if (X(j) < tmp) {
        X(ij) = X(j); X(j) = tmp; tmp = X(ij);
        Y(ij) = Y(j); Y(j) = ty;  ty = Y(ij);
        if (X(i) > tmp) {
            X(ij) = X(i); X(i) = tmp; tmp = X(ij);
            Y(ij) = Y(i); Y(i) = ty;  ty = Y(ij);
        }
    }
    for (;;) {
        l = l - 1;
        if (X(l) <= tmp) {
            for (;;) {
                k = k + 1;
                if (X(k) >= tmp) {
                    if (k <= l) {
                        tt = X(l); X(l) = X(k); X(k) = tt;
                        tty = Y(l); Y(l) = Y(k); Y(k) = tty;
                        goto L90;
                    }
                    if (l - i <= j - k) { il[m] = k; iu[m] = j; j = l; ++m; }
                    else                { il[m] = i; iu[m] = l; i = k; ++m; }
                    goto L110;
                }
            }
        }
    L90:;
    }
L100:
    m = m - 1;
    if (m == 0) return;
    i = il[m];
    j = iu[m];
L110:
    if (j - i >= 1) goto L80;
    if (i == 1) goto L70;
    i = i - 1;
    for (;;) {
        i = i + 1;
        if (i == j) goto L100;
        tmp = X(i + 1);
        ty = Y(i + 1);
        if (X(i) > tmp) {
            k = i;
            for (;;) {
                X(k + 1) = X(k);
                Y(k + 1) = Y(k);
                k = k - 1;
                if (tmp >= X(k)) { X(k + 1) = tmp; Y(k + 1) = ty; break; }
            }
        }
    }
}

}  // namespace

void qmap2(const double* series, const double* stci, double* stci2, int lfda,
           int llda, int ny, int /*iagr*/, double lamda, double rol, int mid,
           int begyrt, double* cratio, double* rratio) {
    const int np = llda - lfda + 1;

    int ns = begyrt - lfda + 1;
    while (ns <= 0) ns += ny;
    const int naly = (np - ns + 1) / ny;

    // J (naly x np, annual-sum selector) and its transpose Jpi (np x naly).
    std::vector<double> Jmat(static_cast<std::size_t>(naly) * np, 0.0);
    std::vector<double> Jmatpi(static_cast<std::size_t>(np) * naly, 0.0);
    {
        int k = ns;
        for (int i = 1; i <= naly; ++i) {
            const int kmq = k + ny - 1;
            for (int j = k; j <= kmq; ++j) Jmat[fidx(naly, i, j)] = 1.0;
            k = kmq + 1;
        }
    }
    for (int i = 1; i <= naly; ++i)
        for (int j = 1; j <= np; ++j)
            Jmatpi[fidx(np, j, i)] = Jmat[fidx(naly, i, j)];

    std::vector<double> xx(np, 0.0), xa(np, 0.0), xd(np, 0.0);
    for (int j = 1; j <= np; ++j) {
        xx[j - 1] = series[j + lfda - 2];
        xa[j - 1] = stci[j + lfda - 2];
    }

    // C = diag(|xa/ttf|^lambda) (ttf = mean |xa|); lambda default 0 => identity.
    double ttf = 0.0;
    for (int i = 1; i <= np; ++i) ttf += std::fabs(xa[i - 1]);
    ttf /= np;
    std::vector<double> Cmat(static_cast<std::size_t>(np) * np, 0.0);
    for (int i = 1; i <= np; ++i)
        Cmat[fidx(np, i, i)] = std::pow(std::fabs(xa[i - 1] / ttf), lamda);

    std::vector<double> mx1(naly, 0.0), mx2(naly, 0.0), mx3(naly, 0.0);

    if (rol <= 0.99999) {
        // Omega = AR(1) correlation matrix rol**|i-j| (identity if rol ~ 0).
        std::vector<double> Omec(static_cast<std::size_t>(np) * np, 0.0);
        if (rol < 1.0e-10) {
            for (int i = 1; i <= np; ++i) Omec[fidx(np, i, i)] = 1.0;
        } else {
            for (int i = 1; i <= np; ++i)
                for (int j = 1; j <= np; ++j)
                    Omec[fidx(np, i, j)] = ipow_nonneg(rol, std::abs(i - j));
        }
        std::vector<double> Ttmat(static_cast<std::size_t>(np) * np, 0.0);
        std::vector<double> Tmx1(static_cast<std::size_t>(np) * np, 0.0);
        std::vector<double> R1(static_cast<std::size_t>(np) * naly, 0.0);
        std::vector<double> Ttmat2(static_cast<std::size_t>(naly) * np, 0.0);
        std::vector<double> Invr(static_cast<std::size_t>(naly) * naly, 0.0);
        std::vector<double> r2(static_cast<std::size_t>(np) * naly, 0.0);
        std::vector<double> ansum(naly, 0.0);

        matmlt(Cmat.data(), Omec.data(), Ttmat.data(), np, np, np, np, np, np);
        matmlt(Ttmat.data(), Cmat.data(), Tmx1.data(), np, np, np, np, np, np);
        matmlt(Tmx1.data(), Jmatpi.data(), R1.data(), np, np, naly, np, np, np);
        matmlt(Jmat.data(), Tmx1.data(), Ttmat2.data(), naly, np, np, naly, np,
               naly);
        matmlt(Ttmat2.data(), Jmatpi.data(), Invr.data(), naly, np, naly, naly,
               np, naly);
        simul(naly, Invr.data(), ansum.data(), 1.0e-20, -1, naly);
        matmlt(R1.data(), Invr.data(), r2.data(), np, naly, naly, np, naly, np);
        matmlt(Jmat.data(), xx.data(), mx1.data(), naly, np, 1, naly, np, naly);
        matmlt(Jmat.data(), xa.data(), mx2.data(), naly, np, 1, naly, np, naly);
        add_sub(mx1.data(), mx2.data(), mx3.data(), naly, 1, naly, 0);
        matmlt(r2.data(), mx3.data(), xd.data(), np, naly, 1, np, naly, np);
        add_sub(xa.data(), xd.data(), xx.data(), np, 1, np, 1);
        for (int j = 1; j <= np; ++j) stci2[j + lfda - 2] = xx[j - 1];
    } else {
        // rol ~ 1: first-difference smoothness benchmark via the augmented KKT
        // system (qmap2.f:158-288). C is inverted in place here.
        for (int i = 1; i <= np; ++i)
            Cmat[fidx(np, i, i)] = 1.0 / Cmat[fidx(np, i, i)];
        const int npn1 = np - 1;
        std::vector<double> delta(static_cast<std::size_t>(np) * np, 0.0);
        std::vector<double> deltapi(static_cast<std::size_t>(np) * np, 0.0);
        for (int i = 1; i <= npn1; ++i) {
            delta[fidx(np, i, i)] = -1.0;
            delta[fidx(np, i, i + 1)] = 1.0;
        }
        for (int i = 1; i <= npn1; ++i)
            for (int j = 1; j <= np; ++j)
                deltapi[fidx(np, j, i)] = delta[fidx(np, i, j)];

        std::vector<double> Ttmat(static_cast<std::size_t>(np) * np, 0.0);
        std::vector<double> Omec(static_cast<std::size_t>(np) * np, 0.0);
        std::vector<double> Tmx1(static_cast<std::size_t>(np) * np, 0.0);
        matmlt(deltapi.data(), delta.data(), Ttmat.data(), np, npn1, np, np, np,
               np);
        matmlt(Cmat.data(), Ttmat.data(), Omec.data(), np, np, np, np, np, np);
        matmlt(Omec.data(), Cmat.data(), Tmx1.data(), np, np, np, np, np, np);

        const int knpn = np + naly;
        std::vector<double> wcomp1(static_cast<std::size_t>(knpn) * knpn, 0.0);
        std::vector<double> wcomp2(static_cast<std::size_t>(knpn) * knpn, 0.0);
        std::vector<double> wcomp3(static_cast<std::size_t>(knpn) * knpn, 0.0);
        for (int i = 1; i <= np; ++i)
            for (int j = 1; j <= np; ++j)
                wcomp1[fidx(knpn, i, j)] = Tmx1[fidx(np, i, j)];
        for (int i = 1; i <= np; ++i)
            for (int j = 1; j <= naly; ++j)
                wcomp1[fidx(knpn, i, np + j)] = Jmatpi[fidx(np, i, j)];
        for (int i = 1; i <= naly; ++i)
            for (int j = 1; j <= np; ++j)
                wcomp1[fidx(knpn, np + i, j)] = Jmat[fidx(naly, i, j)];
        for (int i = 1; i <= np; ++i)
            for (int j = 1; j <= np; ++j)
                wcomp2[fidx(knpn, i, j)] = Tmx1[fidx(np, i, j)];
        for (int i = 1; i <= naly; ++i)
            wcomp2[fidx(knpn, np + i, np + i)] = 1.0;
        for (int i = 1; i <= naly; ++i)
            for (int j = 1; j <= np; ++j)
                wcomp2[fidx(knpn, np + i, j)] = Jmat[fidx(naly, i, j)];

        std::vector<double> rr(knpn, 0.0);  // SIMUL X output (unused, indic=-1)
        std::vector<double> r2(static_cast<std::size_t>(np) * naly, 0.0);
        simul(knpn, wcomp1.data(), rr.data(), 1.0e-10, -1, knpn);
        matmlt(wcomp1.data(), wcomp2.data(), wcomp3.data(), knpn, knpn, knpn,
               knpn, knpn, knpn);
        for (int i = 1; i <= np; ++i)
            for (int j = 1; j <= naly; ++j)
                r2[fidx(np, i, j)] = wcomp3[fidx(knpn, i, j + np)];
        for (int j = 1; j <= np; ++j) {
            xx[j - 1] = series[j + lfda - 2];
            xa[j - 1] = stci[j + lfda - 2];
        }
        matmlt(Jmat.data(), xx.data(), mx1.data(), naly, np, 1, naly, np, naly);
        matmlt(Jmat.data(), xa.data(), mx2.data(), naly, np, 1, naly, np, naly);
        add_sub(mx1.data(), mx2.data(), mx3.data(), naly, 1, naly, 0);
        matmlt(r2.data(), mx3.data(), xd.data(), np, naly, 1, np, naly, np);
        add_sub(xa.data(), xd.data(), xx.data(), np, 1, np, 1);
        for (int j = 1; j <= np; ++j) stci2[j + lfda - 2] = xx[j - 1];
    }

    // Correction-ratio series (force{} cr/rr tables) -- only when requested;
    // does not affect the forced series stci2 (qmap2.f:290-315).
    if (cratio != nullptr || rratio != nullptr) {
        std::vector<double> and11(naly, 0.0), ansum(naly, 0.0),
            rtz(static_cast<std::size_t>(naly) * ny, 0.0);
        for (int j = 1; j <= naly; ++j) {
            ansum[j - 1] = mx1[j - 1];
            and11[j - 1] = mx2[j - 1];
        }
        if (cratio != nullptr) {
            for (int j = 1; j <= np; ++j)
                cratio[j + lfda - 2] =
                    (mid == 0) ? stci2[j + lfda - 2] / stci[j + lfda - 2] - 1.0
                               : stci2[j + lfda - 2] - stci[j + lfda - 2];
        }
        meancra(ansum.data(), and11.data(), rtz.data(), mid, ny, naly);
        if (rratio != nullptr) {
            const int npnp = naly * ny;
            for (int j = 1; j <= np; ++j) rratio[j + lfda - 2] = 0.0;
            for (int j = 1; j <= npnp; ++j)
                rratio[j + ns - 1 + lfda - 2] = rtz[j - 1];
        }
    }
}

void rndsa(const double* sa, double* sarnd, int l1, int l2, int ny, int kdec,
           bool& rndok) {
    rndok = true;
    const double scale = ipow_nonneg(10.0, kdec);  // 10**Kdec (Kdec >= 0)

    int ll1 = l1;
    // If L1 begins on a year boundary, round that lone leading obs by itself.
    if ((ll1 / ny) * ny == ll1) {
        const double x0 = sa[ll1 - 1] * scale;
        sarnd[ll1 - 1] = round_x13(x0) / scale;
        ll1 = ll1 + 1;
    }

    std::vector<double> x(ny + 2, 0.0), c(ny + 2, 0.0), cindx(ny + 2, 0.0),
        thisx(ny + 2, 0.0);
    std::vector<int> r(ny + 2, 0);

    while (ll1 <= l2) {
        double sumx = 0.0;
        int sumr = 0;
        int ll2 = ((ll1 / ny) + 1) * ny;
        if (ll2 > l2) ll2 = l2;

        for (int ij = ll1; ij <= ll2; ++ij) {
            const int jj = ij - ll1 + 1;
            cindx[jj] = static_cast<double>(jj);
            double xj = sa[ij - 1] * scale;
            if (xj > 1000.0) {
                // qmap.f zeroes the low three integer digits (f21.0 write, set
                // cols 18:20 to '000', read back) to keep the value in integer
                // range. Never triggered for the ported corpus (Kdec small,
                // |SA| < 1000); this is the faithful integer equivalent.
                const long long s = static_cast<long long>(round_x13(xj));
                thisx[jj] = static_cast<double>((s / 1000) * 1000);
                xj = xj - thisx[jj];
            } else {
                thisx[jj] = 0.0;
            }
            x[jj] = xj;
            r[jj] = round_x13(xj);
            c[jj] = static_cast<double>(r[jj]) - xj;
            sumx += xj;
            sumr += r[jj];
        }

        const int d = round_x13(sumx) - sumr;
        const int nn = ll2 - ll1 + 1;
        // Sort the rounding residuals ascending, carrying the period index.
        ssort2(c.data() + 1, cindx.data() + 1, nn);

        // Distribute the discrepancy d. NOTE: the oracle's loops use Fortran's
        // default DO step of +1, so `DO j=nn,nn-d+1` only executes for d==1 and
        // `DO j=1,d` never executes for d<0 -- i.e. only a d==1 discrepancy is
        // corrected (to the single largest-residual obs), any |d|>1 is left
        // uncorrected. Replicated exactly for bit parity (see tools/census_bugs.md).
        if (d > 0) {
            for (int jj = nn; jj <= nn - d + 1; ++jj) {
                const int ii = static_cast<int>(cindx[jj]);
                r[ii] = r[ii] + 1;
            }
        } else if (d < 0) {
            for (int jj = 1; jj <= d; ++jj) {
                const int ii = static_cast<int>(cindx[jj]);
                r[ii] = r[ii] - 1;
            }
        }

        for (int ij = ll1; ij <= ll2; ++ij) {
            const int jj = ij - ll1 + 1;
            sarnd[ij - 1] = (thisx[jj] + static_cast<double>(r[jj])) / scale;
        }
        ll1 = ll2 + 1;
    }
}

}  // namespace x13
