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

// lmpar.f -- LM parameter secant search. The GO TO 10 termination collapses to a
// break; Fortran precedence makes the condition (|fp|<=p1*delta) OR
// (parl==0 AND fp<=temp AND temp<0) OR (iter==10), with temp holding the PREVIOUS
// fp. Only dpeq() differs from stock MINPACK.
void lmpar(int n, double* r, int ldr, const int* ipvt, const double* diag,
           const double* qtb, double delta, double& par, double* x,
           double* sdiag, double* wa1, double* wa2) {
    constexpr double P1 = 1.0e-1, P001 = 1.0e-3, ZERO = 0.0;
    double dwarf = dpmpar(2);
    auto R = [&](int i, int j) -> double& { return r[(j - 1) * ldr + (i - 1)]; };

    // Gauss-Newton direction (least-squares if rank-deficient).
    int nsing = n;
    for (int j = 1; j <= n; ++j) {
        wa1[j - 1] = qtb[j - 1];
        if (dpeq(R(j, j), ZERO) && nsing == n) nsing = j - 1;
        if (nsing < n) wa1[j - 1] = ZERO;
    }
    if (nsing >= 1) {
        for (int k = 1; k <= nsing; ++k) {
            int j = nsing - k + 1;
            wa1[j - 1] = wa1[j - 1] / R(j, j);
            double temp = wa1[j - 1];
            int jm1 = j - 1;
            if (jm1 >= 1)
                for (int i = 1; i <= jm1; ++i) wa1[i - 1] = wa1[i - 1] - R(i, j) * temp;
        }
    }
    for (int j = 1; j <= n; ++j) {
        int l = ipvt[j - 1];
        x[l - 1] = wa1[j - 1];
    }
    int iter = 0;
    for (int j = 1; j <= n; ++j) wa2[j - 1] = diag[j - 1] * x[j - 1];
    double dxnorm = enorm(n, wa2);
    double fp = dxnorm - delta;
    if (fp > P1 * delta) {
        // Lower bound parl (zero if rank-deficient).
        double parl = ZERO;
        if (nsing >= n) {
            for (int j = 1; j <= n; ++j) {
                int l = ipvt[j - 1];
                wa1[j - 1] = diag[l - 1] * (wa2[l - 1] / dxnorm);
            }
            for (int j = 1; j <= n; ++j) {
                double sum = ZERO;
                int jm1 = j - 1;
                if (jm1 >= 1)
                    for (int i = 1; i <= jm1; ++i) sum = sum + R(i, j) * wa1[i - 1];
                wa1[j - 1] = (wa1[j - 1] - sum) / R(j, j);
            }
            double temp = enorm(n, wa1);
            parl = ((fp / delta) / temp) / temp;
        }
        // Upper bound paru.
        for (int j = 1; j <= n; ++j) {
            double sum = ZERO;
            for (int i = 1; i <= j; ++i) sum = sum + R(i, j) * qtb[i - 1];
            int l = ipvt[j - 1];
            wa1[j - 1] = sum / diag[l - 1];
        }
        double gnorm = enorm(n, wa1);
        double paru = gnorm / delta;
        if (dpeq(paru, ZERO)) paru = dwarf / std::min(delta, P1);
        // Clamp the input par into (parl,paru).
        par = std::max(par, parl);
        par = std::min(par, paru);
        if (dpeq(par, ZERO)) par = gnorm / dxnorm;
        while (true) {
            iter = iter + 1;
            if (dpeq(par, ZERO)) par = std::max(dwarf, P001 * paru);
            double temp = std::sqrt(par);
            for (int j = 1; j <= n; ++j) wa1[j - 1] = temp * diag[j - 1];
            qrsolv(n, r, ldr, ipvt, wa1, qtb, x, sdiag, wa2);
            for (int j = 1; j <= n; ++j) wa2[j - 1] = diag[j - 1] * x[j - 1];
            dxnorm = enorm(n, wa2);
            temp = fp;
            fp = dxnorm - delta;
            if (std::fabs(fp) <= P1 * delta ||
                (dpeq(parl, ZERO) && fp <= temp && temp < ZERO) || iter == 10)
                break;
            // Newton correction.
            for (int j = 1; j <= n; ++j) {
                int l = ipvt[j - 1];
                wa1[j - 1] = diag[l - 1] * (wa2[l - 1] / dxnorm);
            }
            for (int j = 1; j <= n; ++j) {
                wa1[j - 1] = wa1[j - 1] / sdiag[j - 1];
                double t = wa1[j - 1];
                int jp1 = j + 1;
                if (n >= jp1)
                    for (int i = jp1; i <= n; ++i) wa1[i - 1] = wa1[i - 1] - R(i, j) * t;
            }
            temp = enorm(n, wa1);
            double parc = ((fp / delta) / temp) / temp;
            if (fp > ZERO) parl = std::max(parl, par);
            if (fp < ZERO) paru = std::min(paru, par);
            par = std::max(parl, par + parc);
        }
    }
    if (iter == 0) par = ZERO;
}

}  // namespace x13
