// estimate.cpp -- olsreg.f / resid.f (regression solve + residuals). Faithful
// ports of the vendored oracle Fortran.
#include "regarima/estimate.hpp"

#include <cmath>
#include <string>

#include "regarima/armafl.hpp"      // armafl
#include "numeric/numeric.hpp"      // xprmx, dppfa, dcopy, daxpy, scrmlt, revrse
#include "numeric/rpoly.hpp"        // rpoly
#include "specparse/specparse.hpp"  // copy, setdp, abend, errhdr, writln
#include "gen/model.hpp"            // prm::DIFF, prm::MA, prm::PORDER, error codes
#include "gen/srslen.hpp"           // prm::PLEN (PA sizing)
#include "gen/notset.hpp"           // prm::DNOTST (not-set sentinel)

namespace x13 {

// strtvl.f -- seed free, not-yet-set ARMA lags to 0.1. Walks operators DIFF..MA
// in lag order exactly like upespm; the seed fires only where the parameter is
// still DNOTST and the lag is not fixed.
void strtvl(X13Context& ctx) {
    constexpr double PNT1 = 0.1;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    for (int iflt = prm::DIFF; iflt <= prm::MA; ++iflt) {
        int begopr = m.mdl(iflt - 1);
        int endopr = m.mdl(iflt) - 1;
        for (int iopr = begopr; iopr <= endopr; ++iopr) {
            int beglag = m.opr(iopr - 1);
            int endlag = m.opr(iopr) - 1;
            for (int lag = beglag; lag <= endlag; ++lag) {
                if (dpeq(d.arimap(lag), prm::DNOTST) && !m.arimaf(lag))
                    d.arimap(lag) = PNT1;
            }
        }
    }
}

// stpitr.f -- IGLS convergence test (LOGICAL FUNCTION). Returns keep-iterating.
// oldobj is the vendored SAVEd local, now ctx.saved.stpitr_oldobj. The Lprier
// deviance-increase warning print is deferred (like fcnar's diagnostics); it
// gates on Lprier && ratio<0 and writes no output value, so control flow is
// unaffected. Every branch falls through to storing objfcn into oldobj.
bool stpitr(X13Context& ctx, bool lprier, double objfcn, double devtol, int iter,
            int nliter, int mxiter, bool& convrg, int& armaer, bool lhiddn) {
    (void)lprier;   // used only by the deferred deviance-increase warning print
    (void)lhiddn;
    constexpr double ZERO = 0.0, ONE = 1.0, TWO = 2.0;
    double& oldobj = ctx.saved.stpitr_oldobj;
    double mprec = dpmpar(1);

    bool stpitr_r = true;
    convrg = true;
    if (nliter >= mxiter) {
        // Stop: maximum number of overall iterations exceeded.
        armaer = prm::PMXIER;
        stpitr_r = false;
        convrg = false;
    } else if (iter > 1 && !dpeq(objfcn, ZERO)) {
        // Only check the deviance differences after the second iteration.
        double ratio = oldobj / objfcn - ONE;
        if (devtol / TWO < mprec) {
            // Convergence tolerance tighter than double precision.
            armaer = prm::PCNTER;
            stpitr_r = false;
            convrg = false;
        } else {
            // (Deferred: Lprier && ratio<0 deviance-increase warning print.)
            if (std::abs(ratio) < devtol) {
                stpitr_r = false;  // converged
            } else if (objfcn < mprec) {
                // Deviance below machine precision: can't refine further.
                armaer = prm::PDVTER;
                stpitr_r = false;
            }
        }
    } else {
        oldobj = ZERO;
    }
    oldobj = objfcn;
    return stpitr_r;
}

// olsreg.f -- normal equations then Cholesky back-substitution. The packed
// factor chlxpx after dppfa holds [chol(X'X); z; sqrt(RSS)]; the betas solve
// L'b = z walking the packed columns from the bottom (xelt) upward.
void olsreg(X13Context& ctx, const double* xy, int nrxy, int ncxy, int pcxy,
            double* b, double* chlxpx, int pxpx, int& info) {
    if (ncxy * (ncxy + 1) / 2 > pxpx) {
        errhdr(ctx);
        writln(ctx,
               " Elements needed for [X:y]'[X:y] exceed PXPX (" +
                   std::to_string(ncxy) + "*(" + std::to_string(ncxy) +
                   "+1)/2 > " + std::to_string(pxpx) + ")",
               stdio::STDERR, ctx.units.mt2, true);
        abend(ctx);
        return;
    }
    // Form X'X and X'y as the packed [X:y]'[X:y].
    xprmx(xy, nrxy, ncxy, pcxy, chlxpx);
    dppfa(chlxpx, ncxy, info);
    if (info <= 0 || info == ncxy) {
        int nb = ncxy - 1;
        int xelt = nb * ncxy / 2;
        copy(chlxpx + xelt, nb, 1, b);  // Chlxpx(xelt+1) -> b
        for (int i = nb; i >= 1; --i) {
            b[i - 1] = b[i - 1] / chlxpx[xelt - 1];
            xelt = xelt - i;
            daxpy(i - 1, -b[i - 1], chlxpx + xelt, 1, b, 1);
        }
        info = 0;  // reset when y is linearly dependent on X
    }
}

// resid.f -- rsd = y (+/-) X*b. addsub = sign(1,fac); the y column is xy(pc)
// with stride pc, each regression column icol added with stride pc.
void resid(X13Context& ctx, const double* xy, int nr, int nc, int pc, int begcol,
           int endcol, double fac, const double* b, double* rsd) {
    if (nc == 0 || endcol + 1 == begcol) {
        dcopy(nr, xy + (pc - 1), pc, rsd, 1);
    } else if (begcol < 1 || endcol > nc || endcol < begcol) {
        errhdr(ctx);
        writln(ctx, " Column error, 1<=begcol<=endcol<=nc", stdio::STDERR,
               ctx.units.mt2, true);
        abend(ctx);
        return;
    } else {
        double addsub = (fac < 0.0) ? -1.0 : 1.0;  // sign(ONE,Fac)
        dcopy(nr, xy + (pc - 1), pc, rsd, 1);
        for (int icol = begcol; icol <= endcol; ++icol)
            daxpy(nr, addsub * b[icol - 1], xy + (icol - 1), pc, rsd, 1);
    }
}

// upespm.f -- scatter estprm into arimap, skipping fixed lags. estptr advances
// only for non-fixed lags, so estprm is a dense vector of just the free params.
void upespm(X13Context& ctx, const double* estprm) {
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    int estptr = 0;
    for (int iflt = prm::DIFF; iflt <= prm::MA; ++iflt) {
        int begopr = m.mdl(iflt - 1);
        int endopr = m.mdl(iflt) - 1;
        for (int iopr = begopr; iopr <= endopr; ++iopr) {
            int beglag = m.opr(iopr - 1);
            int endlag = m.opr(iopr) - 1;
            for (int ilag = beglag; ilag <= endlag; ++ilag) {
                if (!m.arimaf(ilag)) {
                    estptr = estptr + 1;
                    d.arimap(ilag) = estprm[estptr - 1];
                }
            }
        }
    }
}

// fcnar.f -- optimizer objective. The info!=0 warning prints (Lprier block) are
// deferred to the print milestone; the sentinel-fill / info / err resets below
// them run unconditionally and ARE reproduced, as is the exact-ML scaling.
void fcnar(X13Context& ctx, int& na, int testpm, const double* estprm, double* a,
           bool lauto, bool gudrun, int& err, bool lckinv) {
    (void)testpm;   // dummy that should equal Nestpm (Estprm's declared length)
    (void)lauto;    // used only by the deferred diagnostic prints
    (void)gudrun;
    constexpr double TWO = 2.0;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    constexpr int PA = prm::PLEN + 2 * prm::PORDER;

    // Insert the estimated parameters into the ARIMA filter structures.
    upespm(ctx, estprm);
    // Filter the working series into the deviance vector a.
    copy(ctx.series.tsrs.data(), d.nspobs, 1, a);
    int info = 0;
    armafl(ctx, d.nspobs, 1, true, lckinv, a, na, PA, info);

    if (info != 0) {
        // (fcnar.f Lprier diagnostics deferred -- do not affect a/na/info/err.)
        // Flood the residuals so a bad jump is brought back in bounds.
        setdp(ctx.series.lrgrsd, na, a);
        info = 0;
        err = -info;
    } else if (m.lextma) {
        double fac = std::exp(d.lndtcv / TWO / ctx.series.dnefob);
        scrmlt(fac, na, a);
    }
}

// roots.f -- modulus/frequency of a polynomial's roots via rpoly (see the hpp).
void roots(X13Context& ctx, const double* thetab, int& degree, bool& allinv,
           double* zeror, double* zeroi, double* zerom, double* zerof) {
    (void)ctx;  // only needed for the deferred rpoly-failure warning
    constexpr double ZERO = 0.0, ONE = 1.0;
    // The Census 2pi divisor is a digit-transposition typo (true 2pi is
    // 6.283185307179586); reproduced verbatim for parity -- see census_bugs.md
    // CB-3. It perturbs the reported frequency in ~its 7th significant digit.
    constexpr double CENSUS_TWOPI = 6.28318730707959;
    int degp1 = degree + 1;
    double op[prm::PORDER + 1];
    // Reverse thetab (increasing powers) to op (decreasing powers).
    revrse(thetab, degp1, 1, op);
    // Strip leading (highest-degree) coefficients that are ~0.
    while (dpeq(op[0], ZERO)) {
        if (degree == 1) {
            allinv = true;
            degree = degree - 1;
            return;  // GO TO 10: nothing to check
        }
        for (int i = 1; i <= degree; ++i) op[i - 1] = op[i];
        degree = degree - 1;
    }
    bool fail;
    rpoly(op, degree, zeror, zeroi, fail);
    if (!fail) {
        allinv = true;
        int i = 0;
        while (i < degree) {
            i = i + 1;
            zerom[i - 1] = std::sqrt(zeror[i - 1] * zeror[i - 1] +
                                     zeroi[i - 1] * zeroi[i - 1]);
            zerof[i - 1] = std::atan2(zeroi[i - 1], zeror[i - 1]) / CENSUS_TWOPI;
            if (zerom[i - 1] < ONE && allinv) allinv = false;
            // Complex root: fill its conjugate's modulus/frequency and skip it.
            if (!dpeq(zeroi[i - 1], ZERO)) {
                i = i + 1;
                zerom[i - 1] = zerom[i - 2];
                zerof[i - 1] = ZERO - zerof[i - 2];
            }
        }
    }
    // else: rpoly failed -> warning deferred to the .out milestone; allinv is
    // left as the caller passed it (matching the oracle).
}

}  // namespace x13
