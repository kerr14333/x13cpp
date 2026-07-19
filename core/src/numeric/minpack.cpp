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

// fdjac2.f -- forward-difference Jacobian. m is shared with the fcn call (the
// Fortran M is aliased through fcn, which may reset it) and re-read as the inner
// loop bound, matching the oracle.
void fdjac2(const MinpackFcn& fcn, int m, int n, double* x, const double* fvec,
            double* fjac, int ldfjac, int& iflag, double epsfcn, double* wa,
            bool lauto, bool gudrun, bool lckinv) {
    constexpr double ZERO = 0.0;
    double epsmch = dpmpar(1);
    double eps = std::sqrt(std::max(epsfcn, epsmch));
    auto FJ = [&](int i, int j) -> double& {
        return fjac[(j - 1) * ldfjac + (i - 1)];
    };
    int mm = m;  // aliased through fcn (may be reset), re-read as loop bound
    for (int j = 1; j <= n; ++j) {
        double temp = x[j - 1];
        double h = eps * std::fabs(temp);
        if (dpeq(h, ZERO)) h = eps;
        x[j - 1] = temp + h;
        fcn(mm, n, x, wa, lauto, gudrun, iflag, lckinv);
        if (iflag < 0) return;
        x[j - 1] = temp;
        for (int i = 1; i <= mm; ++i) FJ(i, j) = (wa[i - 1] - fvec[i - 1]) / h;
    }
}

// covar.f -- covariance matrix (R'R)^-1 from lmdif's QR output. Census
// specifics vs stock MINPACK: a literal |R(k,k)|<=tolr singularity test (NOT
// dpeq), Info seeded with the notset.prm sentinel NOTSET=-32767 (so the
// downstream DO k=1,Info loop is empty when column 1 is singular), and the wa
// scratch supplied by the caller (length n) instead of a PARIMA-sized local.
void covar(int n, double* r, int ldr, const int* ipvt, double tol, int& info,
           double* wa) {
    constexpr double ONE = 1.0, ZERO = 0.0;
    constexpr int NOTSET = -32767;  // notset.prm
    auto R = [&](int i, int j) -> double& { return r[(j - 1) * ldr + (i - 1)]; };

    // Form the inverse of R in the full upper triangle of R.
    double tolr;
    if (tol <= ZERO)
        tolr = dpmpar(1) * std::fabs(R(1, 1));
    else
        tolr = tol * std::fabs(R(1, 1));
    info = NOTSET;
    for (int k = 1; k <= n; ++k) {
        if (std::fabs(R(k, k)) <= tolr) break;  // GO TO 10
        R(k, k) = ONE / R(k, k);
        int km1 = k - 1;
        for (int j = 1; j <= km1; ++j) {
            double temp = R(k, k) * R(j, k);
            R(j, k) = ZERO;
            for (int i = 1; i <= j; ++i) R(i, k) = R(i, k) - temp * R(i, j);
        }
        info = k;
    }
    // Form the full upper triangle of the inverse of R'R in R.
    for (int k = 1; k <= info; ++k) {
        int km1 = k - 1;
        for (int j = 1; j <= km1; ++j) {
            double temp = R(j, k);
            for (int i = 1; i <= j; ++i) R(i, j) = R(i, j) + temp * R(i, k);
        }
        double temp = R(k, k);
        for (int i = 1; i <= k; ++i) R(i, k) = temp * R(i, k);
    }
    // Form the full lower triangle of the covariance matrix in the strict lower
    // triangle of R and in wa.
    for (int j = 1; j <= n; ++j) {
        int jj = ipvt[j - 1];
        bool sing = j > info;
        for (int i = 1; i <= j; ++i) {
            if (sing) R(i, j) = ZERO;
            int ii = ipvt[i - 1];
            if (ii > jj)
                R(ii, jj) = R(i, j);
            else if (ii < jj)
                R(jj, ii) = R(i, j);
        }
        wa[jj - 1] = R(j, j);
    }
    // Make R a symmetric matrix.
    for (int j = 1; j <= n; ++j) {
        for (int i = 1; i <= j; ++i) R(i, j) = R(j, i);
        R(j, j) = wa[j - 1];
    }
    // Nonsingular -> info=0; else info holds the nonsingular-column count.
    if (info == n) info = 0;
}

// lmdif.f -- the Census-modified MINPACK Levenberg-Marquardt core (the "LM
// core"; see the hpp for the model-independence design of the sync/prtitr
// hooks). Differences from stock MINPACK lmdif (Argonne 1980), all preserved
// here for bit-parity:
//   1. Signature: stock MAXFEV is dropped; the ARMA-iteration cap mxiter is
//      passed instead and maxfev = max(mxiter,200)*(n+1) is derived internally.
//      The run-mode flags lauto/gudrun and the counters nliter/nfev are threaded
//      through, and nliter/nfev are CUMULATIVE in/out (not zeroed on entry) so
//      repeated IGLS / AIC-test lmdif calls accumulate.
//   2. info=5 fires on nliter>=mxiter (cumulative) OR nfev-oldfev>=maxfev
//      (per-call) -- the asymmetry is intentional (rgarma passes nliter+tnlitr
//      as mxiter, rgarma.f:374). Do not "fix" it.
//   3. fcn carries three extra args (lauto,gudrun,lckinv); lckinv is false for
//      the initial eval and inside fdjac2, true only for trial steps -- that is
//      where the objective (fcnar) enforces ARMA invertibility.
//   4. upespm(x) is called after every Jacobian (:265) and after every rejected
//      step (:444) to re-sync the shared model state to x. It never feeds back
//      into lmdif's own numerics (fcnar re-syncs itself first), so it is exposed
//      as an optional sync hook; an empty hook is a bit-for-bit no-op.
//   5. Iteration printing goes through prtitr (with an error.cmn Lfatal early
//      return), not stock MINPACK's fcn(iflag=0) print convention; dpeq()
//      replaces every ==0 test.
void lmdif(const MinpackFcn& fcn, int m, int n, double* x, double* fvec,
           bool lauto, bool gudrun, double ftol, double xtol, double gtol,
           int mxiter, double epsfcn, double* diag, int mode, double factor,
           int nprint, int& info, int& nliter, int& nfev, double* fjac,
           int ldfjac, int* ipvt, double* qtf, double* wa1, double* wa2,
           double* wa3, double* wa4, const MinpackSync& sync,
           const MinpackPrtitr& prtitr) {
    constexpr double ONE = 1.0, P1 = 0.1, P5 = 0.5, P25 = 0.25, P75 = 0.75,
                     P0001 = 1.0e-4, MONE = -1.0, ZERO = 0.0;
    double epsmch = dpmpar(1);
    auto FJ = [&](int i, int j) -> double& {
        return fjac[(j - 1) * ldfjac + (i - 1)];
    };

    info = 0;
    int iflag = 0;
    int oldfev = nfev;
    int mm = m;              // Fortran M aliased through fcn (may be reset)
    int begitr = nliter;     // safe pre-init; re-set at the :247 site
    double ratio = ZERO;     // read at the :497 final-print guard
    double actred, delta = ZERO, dirder, fnorm, fnorm1, gnorm = ZERO, par,
           pnorm, prered, xnorm = ZERO;

    // Check the input parameters for errors (fall through with info=0 on abuse).
    if (n > 0 && m >= n && ldfjac >= m && ftol >= ZERO && xtol >= ZERO &&
        gtol >= ZERO && mxiter >= 0 && factor > ZERO) {
        int maxfev = std::max(mxiter, 200) * (n + 1);
        if (mode == 2) {
            for (int j = 1; j <= n; ++j)
                if (diag[j - 1] <= ZERO) goto termination;
        }
        // Evaluate the function at the starting point and calculate its norm.
        iflag = 1;
        fcn(mm, n, x, fvec, lauto, gudrun, iflag, false);
        nfev = nfev + 1;
        if (iflag >= 0) {
            fnorm = enorm(mm, fvec);
            begitr = nliter;
            par = ZERO;
            while (true) {  // ---- outer loop ----
                // Calculate the Jacobian matrix, then re-sync the model to the
                // original (not perturbed) parameters.
                iflag = 2;
                fdjac2(fcn, mm, n, x, fvec, fjac, ldfjac, iflag, epsfcn, wa4,
                       lauto, gudrun, false);
                nfev = nfev + n;
                if (sync) sync(x);
                if (iflag < 0) goto termination;
                // If requested, call prtitr to print iterates.
                if (nprint > 0 && nliter > begitr) {
                    if (prtitr &&
                        prtitr(fvec, mm, x, n, "ARMA      ", nliter, nfev))
                        return;  // Lfatal -> bare return (skips the final print)
                }
                // Compute the QR factorization of the Jacobian.
                qrfac(mm, n, fjac, ldfjac, true, ipvt, n, wa1, wa2, wa3);
                if (nliter == begitr) {
                    // First iteration: scale from the column norms (mode 1),
                    // then form the scaled xnorm and initialize the step bound.
                    if (mode != 2) {
                        for (int j = 1; j <= n; ++j) {
                            diag[j - 1] = wa2[j - 1];
                            if (dpeq(wa2[j - 1], ZERO)) diag[j - 1] = ONE;
                        }
                    }
                    for (int j = 1; j <= n; ++j) wa3[j - 1] = diag[j - 1] * x[j - 1];
                    xnorm = enorm(n, wa3);
                    delta = factor * xnorm;
                    if (dpeq(delta, ZERO)) delta = factor;
                }
                // Form Q'*fvec, storing the first n components in qtf.
                for (int i = 1; i <= mm; ++i) wa4[i - 1] = fvec[i - 1];
                for (int j = 1; j <= n; ++j) {
                    if (!dpeq(FJ(j, j), ZERO)) {
                        double sum = ZERO;
                        for (int i = j; i <= mm; ++i) sum = sum + FJ(i, j) * wa4[i - 1];
                        double temp = -sum / FJ(j, j);
                        for (int i = j; i <= mm; ++i) wa4[i - 1] = wa4[i - 1] + FJ(i, j) * temp;
                    }
                    FJ(j, j) = wa1[j - 1];
                    qtf[j - 1] = wa4[j - 1];
                }
                // Compute the norm of the scaled gradient.
                gnorm = ZERO;
                if (!dpeq(fnorm, ZERO)) {
                    for (int j = 1; j <= n; ++j) {
                        int l = ipvt[j - 1];
                        if (!dpeq(wa2[l - 1], ZERO)) {
                            double sum = ZERO;
                            for (int i = 1; i <= j; ++i)
                                sum = sum + FJ(i, j) * (qtf[i - 1] / fnorm);
                            gnorm = std::max(gnorm, std::fabs(sum / wa2[l - 1]));
                        }
                    }
                }
                // Test for convergence of the gradient norm.
                if (gnorm <= gtol) info = 4;
                if (info != 0) goto termination;
                // Rescale if necessary.
                if (mode != 2) {
                    for (int j = 1; j <= n; ++j)
                        diag[j - 1] = std::max(diag[j - 1], wa2[j - 1]);
                }
                while (true) {  // ---- inner loop ----
                    // Determine the Levenberg-Marquardt parameter.
                    lmpar(n, fjac, ldfjac, ipvt, diag, qtf, delta, par, wa1,
                          wa2, wa3, wa4);
                    // Store the direction p and x+p; compute the norm of p.
                    for (int j = 1; j <= n; ++j) {
                        wa1[j - 1] = -wa1[j - 1];
                        wa2[j - 1] = x[j - 1] + wa1[j - 1];
                        wa3[j - 1] = diag[j - 1] * wa1[j - 1];
                    }
                    pnorm = enorm(n, wa3);
                    // On the first iteration, adjust the initial step bound.
                    if (nliter == begitr) delta = std::min(delta, pnorm);
                    // Evaluate the function at x+p and calculate its norm.
                    iflag = 1;
                    fcn(mm, n, wa2, wa4, lauto, gudrun, iflag, true);
                    nfev = nfev + 1;
                    if (iflag < 0) goto termination;
                    fnorm1 = enorm(mm, wa4);
                    // Compute the scaled actual reduction.
                    actred = MONE;
                    if (P1 * fnorm1 < fnorm)
                        actred = ONE - (fnorm1 / fnorm) * (fnorm1 / fnorm);
                    // Compute the scaled predicted reduction and directional
                    // derivative.
                    for (int j = 1; j <= n; ++j) {
                        wa3[j - 1] = ZERO;
                        int l = ipvt[j - 1];
                        double temp = wa1[l - 1];
                        for (int i = 1; i <= j; ++i) wa3[i - 1] = wa3[i - 1] + FJ(i, j) * temp;
                    }
                    double temp1 = enorm(n, wa3) / fnorm;
                    double temp2 = (std::sqrt(par) * pnorm) / fnorm;
                    prered = temp1 * temp1 + temp2 * temp2 / P5;
                    dirder = -(temp1 * temp1 + temp2 * temp2);
                    // Compute the ratio of the actual to the predicted reduction.
                    ratio = ZERO;
                    if (!dpeq(prered, ZERO)) ratio = actred / prered;
                    // Update the step bound.
                    if (ratio <= P25) {
                        double temp;
                        if (actred >= ZERO) temp = P5;
                        if (actred < ZERO)
                            temp = P5 * dirder / (dirder + P5 * actred);
                        if (P1 * fnorm1 >= fnorm || temp < P1) temp = P1;
                        delta = temp * std::min(delta, pnorm / P1);
                        par = par / temp;
                    } else if (dpeq(par, ZERO) || ratio >= P75) {
                        delta = pnorm / P5;
                        par = P5 * par;
                    }
                    // Test for successful iteration.
                    if (ratio >= P0001) {
                        // Successful: update x, fvec, and their norms.
                        for (int j = 1; j <= n; ++j) {
                            x[j - 1] = wa2[j - 1];
                            wa2[j - 1] = diag[j - 1] * x[j - 1];
                        }
                        for (int i = 1; i <= mm; ++i) fvec[i - 1] = wa4[i - 1];
                        xnorm = enorm(n, wa2);
                        fnorm = fnorm1;
                        nliter = nliter + 1;
                    } else {
                        // Rejected: re-sync the model to the last accepted x.
                        if (sync) sync(x);
                    }
                    // Tests for convergence (run even on a rejected step).
                    if (std::fabs(actred) <= ftol && prered <= ftol &&
                        P5 * ratio <= ONE)
                        info = 1;
                    if (delta <= xtol * xnorm) info = 2;
                    if (std::fabs(actred) <= ftol && prered <= ftol &&
                        P5 * ratio <= ONE && info == 2)
                        info = 3;
                    if (info != 0) goto termination;
                    // Tests for termination and stringent tolerances.
                    if (mxiter > 0 && nliter >= mxiter) info = 5;  // cumulative
                    if (nfev - oldfev >= maxfev) info = 5;         // per-call
                    if (std::fabs(actred) <= epsmch && prered <= epsmch &&
                        P5 * ratio <= ONE)
                        info = 6;
                    if (delta <= epsmch * xnorm) info = 7;
                    if (gnorm <= epsmch) info = 8;
                    if (info != 0) goto termination;
                    // End of the inner loop. Repeat if iteration unsuccessful;
                    // a successful step breaks out to recompute the Jacobian.
                    if (ratio >= P0001) break;
                }
                // End of the outer loop.
            }
        }
    }

termination:
    if (iflag < 0) info = iflag;
    iflag = 0;
    // Print the final estimates if we ran at least one successful iteration.
    if (nprint > 0 && nliter > begitr && ratio >= P0001) {
        if (prtitr) prtitr(fvec, mm, x, n, "ARMA", nliter, nfev);
    }
}

}  // namespace x13
