// minpack.cpp -- Census-modified MINPACK optimizer leaves (see minpack.hpp).
// Faithful ports of the vendored oracle Fortran; column-major matrices via a
// leading-dimension index lambda, Fortran-base loop bounds preserved.
#include "numeric/minpack.hpp"

#include <algorithm>
#include <cmath>

#include "numeric/numeric.hpp"  // dpmpar, dpeq, enorm

namespace x13 {

// qrfac.f -- Householder QR with column pivoting. The only Census change from
// stock MINPACK is dpeq(.,0) in place of .eq.0. A(i,j) is column-major.
void qrfac(int m, int n, double* a, int lda, bool pivot, int* ipvt, int lipvt,
           double* rdiag, double* acnorm, double* wa) {
    (void)lipvt;
    constexpr double ONE = 1.0, P05 = 5.0e-2, ZERO = 0.0;
    double epsmch = dpmpar(1);
    auto A = [&](int i, int j) -> double& { return a[(j - 1) * lda + (i - 1)]; };

    for (int j = 1; j <= n; ++j) {
        acnorm[j - 1] = enorm(m, a + (j - 1) * lda);  // enorm(M,A(1,j))
        rdiag[j - 1] = acnorm[j - 1];
        wa[j - 1] = rdiag[j - 1];
        if (pivot) ipvt[j - 1] = j;
    }
    int minmn = std::min(m, n);
    for (int j = 1; j <= minmn; ++j) {
        if (pivot) {
            int kmax = j;
            for (int k = j; k <= n; ++k)
                if (rdiag[k - 1] > rdiag[kmax - 1]) kmax = k;
            if (kmax != j) {
                for (int i = 1; i <= m; ++i) {
                    double temp = A(i, j);
                    A(i, j) = A(i, kmax);
                    A(i, kmax) = temp;
                }
                rdiag[kmax - 1] = rdiag[j - 1];
                wa[kmax - 1] = wa[j - 1];
                int k = ipvt[j - 1];
                ipvt[j - 1] = ipvt[kmax - 1];
                ipvt[kmax - 1] = k;
            }
        }
        // Householder transform reducing column j to a multiple of e_j.
        double ajnorm = enorm(m - j + 1, a + (j - 1) * lda + (j - 1));  // A(j,j)
        if (!dpeq(ajnorm, ZERO)) {
            if (A(j, j) < ZERO) ajnorm = -ajnorm;
            for (int i = j; i <= m; ++i) A(i, j) = A(i, j) / ajnorm;
            A(j, j) = A(j, j) + ONE;
            int jp1 = j + 1;
            if (n >= jp1) {
                for (int k = jp1; k <= n; ++k) {
                    double sum = ZERO;
                    for (int i = j; i <= m; ++i) sum = sum + A(i, j) * A(i, k);
                    double temp = sum / A(j, j);
                    for (int i = j; i <= m; ++i)
                        A(i, k) = A(i, k) - temp * A(i, j);
                    if (!(!pivot || dpeq(rdiag[k - 1], ZERO))) {
                        temp = A(j, k) / rdiag[k - 1];
                        rdiag[k - 1] =
                            rdiag[k - 1] *
                            std::sqrt(std::max(ZERO, ONE - temp * temp));
                        double rw = rdiag[k - 1] / wa[k - 1];
                        if (P05 * rw * rw <= epsmch) {
                            rdiag[k - 1] =
                                enorm(m - j, a + (k - 1) * lda + j);  // A(jp1,k)
                            wa[k - 1] = rdiag[k - 1];
                        }
                    }
                }
            }
        }
        rdiag[j - 1] = -ajnorm;
    }
}

// qrsolv.f -- eliminate D with Givens rotations then back-substitute; nsing
// least-squares fallback. dpeq(.,0) is the Census change from stock MINPACK.
void qrsolv(int n, double* r, int ldr, const int* ipvt, const double* diag,
            const double* qtb, double* x, double* sdiag, double* wa) {
    constexpr double P5 = 5.0e-1, P25 = 2.5e-1, ZERO = 0.0;
    auto R = [&](int i, int j) -> double& { return r[(j - 1) * ldr + (i - 1)]; };

    // Copy R and Q'b; save R's diagonal in x.
    for (int j = 1; j <= n; ++j) {
        for (int i = j; i <= n; ++i) R(i, j) = R(j, i);
        x[j - 1] = R(j, j);
        wa[j - 1] = qtb[j - 1];
    }
    for (int j = 1; j <= n; ++j) {
        int l = ipvt[j - 1];
        if (!dpeq(diag[l - 1], ZERO)) {
            for (int k = j; k <= n; ++k) sdiag[k - 1] = ZERO;
            sdiag[j - 1] = diag[l - 1];
            double qtbpj = ZERO;
            for (int k = j; k <= n; ++k) {
                if (!dpeq(sdiag[k - 1], ZERO)) {
                    double cosine, sine;
                    if (std::fabs(R(k, k)) >= std::fabs(sdiag[k - 1])) {
                        double tangnt = sdiag[k - 1] / R(k, k);
                        cosine = P5 / std::sqrt(P25 + P25 * tangnt * tangnt);
                        sine = cosine * tangnt;
                    } else {
                        double cotan = R(k, k) / sdiag[k - 1];
                        sine = P5 / std::sqrt(P25 + P25 * cotan * cotan);
                        cosine = sine * cotan;
                    }
                    R(k, k) = cosine * R(k, k) + sine * sdiag[k - 1];
                    double temp = cosine * wa[k - 1] + sine * qtbpj;
                    qtbpj = -sine * wa[k - 1] + cosine * qtbpj;
                    wa[k - 1] = temp;
                    int kp1 = k + 1;
                    if (n >= kp1) {
                        for (int i = kp1; i <= n; ++i) {
                            double t2 = cosine * R(i, k) + sine * sdiag[i - 1];
                            sdiag[i - 1] = -sine * R(i, k) + cosine * sdiag[i - 1];
                            R(i, k) = t2;
                        }
                    }
                }
            }
        }
        sdiag[j - 1] = R(j, j);
        R(j, j) = x[j - 1];
    }
    // Solve the triangular system (least-squares if singular).
    int nsing = n;
    for (int j = 1; j <= n; ++j) {
        if (dpeq(sdiag[j - 1], ZERO) && nsing == n) nsing = j - 1;
        if (nsing < n) wa[j - 1] = ZERO;
    }
    if (nsing >= 1) {
        for (int k = 1; k <= nsing; ++k) {
            int j = nsing - k + 1;
            double summ = ZERO;
            int jp1 = j + 1;
            if (nsing >= jp1)
                for (int i = jp1; i <= nsing; ++i) summ = summ + R(i, j) * wa[i - 1];
            wa[j - 1] = (wa[j - 1] - summ) / sdiag[j - 1];
        }
    }
    // Permute z back to x.
    for (int j = 1; j <= n; ++j) {
        int l = ipvt[j - 1];
        x[l - 1] = wa[j - 1];
    }
}

}  // namespace x13
