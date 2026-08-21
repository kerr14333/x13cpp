// estimate.cpp -- olsreg.f / resid.f (regression solve + residuals). Faithful
// ports of the vendored oracle Fortran.
#include "regarima/estimate.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "regarima/armafl.hpp"      // armafl
#include "regarima/armafilt.hpp"    // arflt
#include "numeric/numeric.hpp"      // xprmx, dppfa, dcopy, daxpy, scrmlt, revrse,
                                    // yprmy, maxvec, dpmpar, dpeq
#include "numeric/minpack.hpp"      // lmdif, fdjac2, qrfac, covar (+ hook types)
#include "numeric/rpoly.hpp"        // rpoly
#include "specparse/specparse.hpp"  // copy, setdp, abend, errhdr, writln
#include "gen/model.hpp"            // prm::DIFF, prm::MA, prm::PORDER, error codes
#include "gen/srslen.hpp"           // prm::PLEN (PA sizing)
#include "gen/notset.hpp"           // prm::DNOTST (not-set sentinel)
#include "x13/fformat.hpp"          // fwrite_fmt (fcnar's Lprier message formats)

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

// fcnar.f -- optimizer objective. The info!=0 Lprier block writes the run's
// most numerous `.err` diagnostic (374 lines on one pickmdl spec): the objective
// is called once per lmdif function evaluation, so a non-invertible operator is
// reported on EVERY evaluation that hits one, not once per estimation.
//
// Only the Mt2 half is emitted. `fh2` is Mt1, the main printout, which this port
// defers wholesale -- it is written here for shape, and `prtitr`'s parameter
// dump under the non-auto arm stays deferred with the rest of the print engine.
//
// DORMANT, and deliberately so: `Lprier` is `Prttab(LESTIE)` (gtestm.f:208) and
// this port defers the print-table STORE, so m.lprier is false on every run and
// nothing below fires. It is transcribed rather than walled because it was
// verified against the goldens first: forcing the flag on reproduces
// `extra/airline_pickmdl`'s block byte for byte, trailing space included. The
// flag is NOT saturated -- forcing it on emits the warning on 55 specs whose
// goldens are silent (they carry a bare `estimate{ }`; every spec that carries
// the warning carries `estimate{print=all}`), so this cannot be closed by taking
// Prttab true the way run_spectrum.cpp does for LSPCRS. It needs the store.
void fcnar(X13Context& ctx, int& na, int testpm, const double* estprm, double* a,
           bool lauto, bool gudrun, int& err, bool lckinv) {
    (void)testpm;   // dummy that should equal Nestpm (Estprm's declared length)
    constexpr double TWO = 2.0;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    constexpr int PA = prm::PLEN + 2 * prm::PORDER;

    // fcnar.f:69-75. cdot ends the 1010 line: a period on the explicit-model
    // path, a BLANK under Lauto, because there the sentence continues into
    // 1049's "for model ...". The trailing space is in the goldens.
    const char* cdot = ".";
    if (lauto) cdot = " ";
    int fh2 = 0;
    if (!lauto) {
        fh2 = ctx.units.mt1;
        if (!gudrun) fh2 = 0;
    }

    // Insert the estimated parameters into the ARIMA filter structures.
    upespm(ctx, estprm);
    // Filter the working series into the deviance vector a.
    copy(ctx.series.tsrs.data(), d.nspobs, 1, a);
    int info = 0;
    armafl(ctx, d.nspobs, 1, true, lckinv, a, na, PA, info);

    if (info != 0) {
        if (m.lprier) {
            const int Mt2 = ctx.units.mt2;
            auto& mt2 = ctx.channels_.unit(Mt2);
            if (info == prm::PINVER) {
                std::string str;
                int ntmpcr = 0;
                getstr(ctx, m.oprttl.data(), m.oprptr.data(), m.noprtl, d.prbfac,
                       str, ntmpcr);
                if (ctx.error.lfatal) return;
                const std::string opr =
                    str.substr(0, static_cast<std::size_t>(ntmpcr));
                const std::string rec =
                    fwrite_fmt("(/,' WARNING: ',a,' roots inside the unit circle',a)",
                               opr, std::string(cdot)) + "\n";
                if (fh2 > 0) ctx.channels_.unit(fh2).put(rec);
                errhdr(ctx);
                mt2.put(rec);
            } else if (info == prm::PGPGER) {
                const std::string rec =
                    fwrite_fmt("(/,' WARNING: Problem with MA parameter estimation.  ',a,"
                               "' can''t',/,'          invert the G''G matrix. Try a "
                               "simpler ARIMA ','model without',/,'          parameter "
                               "constraints. Please send us the ','data and spec file',"
                               "/,'          that produced this message ',"
                               "'(x12@census.gov)',a)",
                               std::string(stdio::PRGNAM), std::string(cdot)) + "\n";
                if (fh2 > 0) ctx.channels_.unit(fh2).put(rec);
                errhdr(ctx);
                mt2.put(rec);
            } else if (info == prm::PACFER) {
                const std::string rec =
                    fwrite_fmt("(/,' WARNING: Problem calculating the theoretical ARMA "
                               "ACF',a)", std::string(cdot)) + "\n";
                if (fh2 > 0) ctx.channels_.unit(fh2).put(rec);
                errhdr(ctx);
                mt2.put(rec);
            } else if (info == prm::PVWPER) {
                const std::string rec =
                    fwrite_fmt("(/,' WARNING: Problem calculating var(w_p|z)',a)",
                               std::string(cdot)) + "\n";
                if (fh2 > 0) ctx.channels_.unit(fh2).put(rec);
                errhdr(ctx);
                mt2.put(rec);
            }
            // fcnar.f:114-130 -- the tail. Note Lckinv, not the info code,
            // picks it, so an unrecognized info writes a bare tail and no head.
            if (lckinv) {
                errhdr(ctx);
                if (lauto) {
                    mt2.put(fwrite_fmt("('          for model ',a,'.  Will',/,"
                                       "'          attempt to fix the problem, and "
                                       "continue.')",
                                       m.mdldsn.raw().substr(
                                           0, static_cast<std::size_t>(m.nmddcr))) +
                            "\n");
                } else {
                    const std::string rec =
                        fwrite_fmt("('          Will print out the parameters,',/,"
                                   "'          attempt to fix the problem, and "
                                   "continue.')") + "\n";
                    // (fh2's prtitr parameter dump is deferred with the print engine.)
                    if (fh2 > 0) ctx.channels_.unit(fh2).put(rec);
                    mt2.put(rec);
                }
            } else {
                const std::string rec = fwrite_fmt("(/)") + "\n";
                errhdr(ctx);
                if (fh2 > 0) ctx.channels_.unit(fh2).put(rec);
                mt2.put(rec);
            }
        }
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

// gfortran real(8)**int(4): square-and-multiply (matches PT9**lag exactly).
static double dpowri(double base, int n) {
    if (n == 0) return 1.0;
    bool neg = n < 0;
    unsigned u = neg ? static_cast<unsigned>(-n) : static_cast<unsigned>(n);
    double pow = 1.0, x = base;
    for (;;) {
        if (u & 1u) pow *= x;
        u >>= 1;
        if (u)
            x *= x;
        else
            break;
    }
    return neg ? 1.0 / pow : pow;
}

// setmdl.f -- pack estprm + root-check the starting values (see the hpp).
void setmdl(X13Context& ctx, double* estprm, bool& laumts) {
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    constexpr double ZERO = 0.0, PT9 = 0.9;
    bool& first = ctx.saved.setmdl_first;
    bool mainsa = false, maonua = false, arona = false;
    // Fortran sizes lagind(PORDER); ilag indexes the PARIMA lag space, so size
    // to PARIMA here to avoid the latent Fortran undersizing (census_bugs.md
    // CB-4). lagind[ilag] = the estprm slot of that free lag (non-first calls).
    int lagind[prm::PARIMA];

    // Pack the free (non-fixed) AR/MA coefficients into estprm.
    m.nestpm = 0;
    for (int iflt = prm::DIFF; iflt <= prm::MA; ++iflt) {
        int begopr = m.mdl(iflt - 1);
        int endopr = m.mdl(iflt) - 1;
        for (int iopr = begopr; iopr <= endopr; ++iopr) {
            int beglag = m.opr(iopr - 1);
            int endlag = m.opr(iopr) - 1;
            for (int ilag = beglag; ilag <= endlag; ++ilag) {
                if (!m.arimaf(ilag)) {
                    m.nestpm = m.nestpm + 1;
                    estprm[m.nestpm - 1] = d.arimap(ilag);
                    if (!first) lagind[ilag - 1] = m.nestpm;
                }
            }
        }
    }

    double coef[prm::PORDER + 1], zeror[prm::PORDER], zeroi[prm::PORDER],
        zerom[prm::PORDER], zerof[prm::PORDER];

    // ---- roots of the initial theta(B): invertibility ----
    {
        int begopr = m.mdl(prm::MA - 1);
        int endopr = m.mdl(prm::MA) - 1;
        if (endopr > 0) {
            for (int iopr = begopr; iopr <= endopr; ++iopr) {
                int beglag = m.opr(iopr - 1);
                int endlag = m.opr(iopr) - 1;
                int factor = m.oprfac(iopr);
                int degree = m.arimal(endlag) / factor;
                coef[0] = -1.0;
                setdp(ZERO, degree, coef + 1);
                setdp(ZERO, prm::PORDER, zeror);
                setdp(ZERO, prm::PORDER, zeroi);
                setdp(ZERO, prm::PORDER, zerom);
                for (int ilag = beglag; ilag <= endlag; ++ilag)
                    coef[m.arimal(ilag) / factor] = d.arimap(ilag);
                bool allinv = false;
                roots(ctx, coef, degree, allinv, zeror, zeroi, zerom, zerof);
                if (!allinv) mainsa = true;  // noninvertible (error deferred)
                bool onunit = false, shrnkp = false;
                int i = 0;
                while (i < degree) {
                    i = i + 1;
                    if (first) {
                        if (dpeq(zerom[i - 1], 1.0) && !onunit) onunit = true;
                    } else {
                        if (zerom[i - 1] <= 1.06 && !shrnkp) shrnkp = true;
                    }
                }
                bool allfix = true;
                for (int ilag = beglag; ilag <= endlag; ++ilag)
                    if (!m.arimaf(ilag) && allfix) allfix = false;
                if (onunit && !allfix) maonua = true;  // MA root on unit circle
                // Shrink a near-unit-circle operator (IGLS re-entries only).
                if (shrnkp && !allfix) {
                    for (int ilag = beglag; ilag <= endlag; ++ilag) {
                        if (!m.arimaf(ilag)) {
                            d.arimap(ilag) =
                                d.arimap(ilag) * dpowri(PT9, m.arimal(ilag));
                            estprm[lagind[ilag - 1] - 1] = d.arimap(ilag);
                        }
                    }
                }
            }
        }
    }

    // ---- roots of the initial phi(B): stationarity ----
    {
        int begopr = m.mdl(prm::AR - 1);
        int endopr = m.mdl(prm::AR) - 1;
        if (endopr > 0) {
            for (int iopr = begopr; iopr <= endopr; ++iopr) {
                int beglag = m.opr(iopr - 1);
                int endlag = m.opr(iopr) - 1;
                int factor = m.oprfac(iopr);
                int degree = m.arimal(endlag) / factor;
                coef[0] = -1.0;
                setdp(ZERO, prm::PORDER, zeror);
                setdp(ZERO, prm::PORDER, zeroi);
                setdp(ZERO, prm::PORDER, zerom);
                setdp(ZERO, degree, coef + 1);
                for (int ilag = beglag; ilag <= endlag; ++ilag)
                    coef[m.arimal(ilag) / factor] = d.arimap(ilag);
                bool allinv = false;
                roots(ctx, coef, degree, allinv, zeror, zeroi, zerom, zerof);
                bool onunit = false;
                int i = 0;
                while (i < degree) {
                    i = i + 1;
                    if (dpeq(zerom[i - 1], 1.0) && !onunit) onunit = true;
                }
                if (!allinv || onunit) {
                    // Nonstationary start: error (+arona) if exact AR, else a
                    // deferred warning. The root-table print is deferred.
                    if (m.lar) arona = true;
                }
            }
        }
    }

    first = false;
    if (mainsa || maonua || arona) {
        if (laumts)
            laumts = false;  // signal failed HR initial values to the caller
        else
            abend(ctx);
    }
}

// chkrt2.f -- re-check theta(B) (and phi(B) when exact AR) after a filter
// failure. In the vendored version this DOES NOT invert anything, despite the
// header comment saying it does: `Inverr` is set to 0 and never changed, and
// `roots` is called with `allinv=F` and its rewritten `coef` thrown away. The
// whole effect is diagnostic -- which is exactly why the previous stub, a body
// of three `(void)param;` lines, was the shape this project's standing rules
// name as a defect waiting for its first caller who cares.
//
// TWO channels, and only one of them is what the old comment claimed:
//   * the `Lprmsg` writln goes to (Mt2, STDERR) -- the GATED `.err` channel,
//     not the deferred printout. Only rgarma.f:277 passes Lprmsg=T; prterr.f's
//     two calls pass F, which is what let iddiff's PNIFER/PNIMER arm say the
//     stub was "faithful on the gated channel" there;
//   * the root TABLE goes to `imt`, which is Mt1 -- EXCEPT under `Lhiddn`,
//     where it is Mt2. Lhiddn is true for every aictest sub-run (easaic.f:49,
//     lomaic.f:46, tdaic.f:59, trnaic.f:163, usraic.f:48) and for a
//     `slidingspans{}`/`history{}` replay carrying a transform test
//     (sspdrv.f:73, revdrv.f:356). So "Mt1-only" is false on precisely the
//     runs that reach prterr's forced-Lprier call.
//
// DORMANT for the same reason fcnar's block above is: `Lprier` is
// `Prttab(LESTIE)` (gtestm.f:208), false unless the spec asks for the
// estimation iterations table. prterr.f:171-175 and :193-197 force it TRUE
// around their two calls, so that path does not need the store -- it needs a
// non-invertible operator with fixed or missing lags (PNIFER/PNIMER), which no
// corpus spec carries yet. No golden contains this routine's sentence.
void chkrt2(X13Context& ctx, bool lprmsg, int& inverr, bool lhiddn) {
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    // dotln is CHARACTER*(POPRCR+1) = 73; the DATA literal is 61 characters
    // (two spaces + 59 dashes) and Fortran blank-pads the rest.
    static const std::string DOTLN =
        std::string(2, ' ') + std::string(59, '-') + std::string(12, ' ');

    inverr = 0;

    const int begopr = m.lextar ? m.mdl(prm::AR - 1) : m.mdl(prm::MA - 1);
    // chkrt2.f:52's `beglag=Opr(begopr-1)` is a dead store -- the loop below
    // re-derives beglag from iopr on its first pass. Not transcribed.
    const int endopr = m.mdl(prm::MA) - 1;
    if (endopr <= 0) return;

    double coef[prm::PORDER + 1], zeror[prm::PORDER], zeroi[prm::PORDER],
        zerom[prm::PORDER], zerof[prm::PORDER];

    for (int iopr = begopr; iopr <= endopr; ++iopr) {
        const int beglag = m.opr(iopr - 1);
        const int endlag = m.opr(iopr) - 1;
        const int factor = m.oprfac(iopr);
        int degree = m.arimal(endlag) / factor;   // roots() may reduce it
        coef[0] = -1.0;
        setdp(0.0, degree, coef + 1);
        // The Fortran leaves zeror/zeroi/zerom/zerof undefined when rpoly
        // fails; zero them so the C++ read is defined. roots() fills 1..degree
        // on every non-failing call, which is the only path a golden can see.
        setdp(0.0, prm::PORDER, zeror);
        setdp(0.0, prm::PORDER, zeroi);
        setdp(0.0, prm::PORDER, zerom);
        setdp(0.0, prm::PORDER, zerof);
        for (int ilag = beglag; ilag <= endlag; ++ilag)
            coef[m.arimal(ilag) / factor] = d.arimap(ilag);

        bool allinv = false;
        roots(ctx, coef, degree, allinv, zeror, zeroi, zerom, zerof);
        if (ctx.error.lfatal) return;
        if (allinv || !m.lprier) continue;

        std::string tmpttl;
        int ntmpcr = 0;
        getstr(ctx, m.oprttl.data(), m.oprptr.data(), m.noprtl, iopr, tmpttl,
               ntmpcr);
        if (ctx.error.lfatal) return;
        const std::string ttl =
            tmpttl.substr(0, static_cast<std::size_t>(ntmpcr));

        if (lprmsg)
            writln(ctx,
                   ttl + " roots inside the unit circle.  Will attempt to "
                         "invert them.",
                   ctx.units.mt2, stdio::STDERR, true);

        // imt: Mt1 normally, Mt2 when the run is hidden (see the header note).
        auto& out = ctx.channels_.unit(lhiddn ? ctx.units.mt2 : ctx.units.mt1);
        out.put(fwrite_fmt("(' ',a,' Roots',/,'  Root',t25,'Real',t31,"
                           "'Imaginary',t44,'Modulus',t53,'Frequency',/,a)",
                           ttl, DOTLN) + "\n");
        for (int i = 1; i <= degree; ++i)
            out.put(fwrite_fmt("('   Root',i3,t18,4F11.4)", i, zeror[i - 1],
                               zeroi[i - 1], zerom[i - 1], zerof[i - 1]) +
                    "\n");
    }
}

// rgarma.f -- the regARIMA IGLS estimation engine (see hpp for the overview).
void rgarma(X13Context& ctx, bool lestim, int mxiter, int mxnlit, bool lprtit,
            double* a, int& na, int& nefobs, bool& lauto) {
    constexpr double ONE = 1.0, PI = 3.14159265358979, TWO = 2.0, ZERO = 0.0,
                     MONE = -1.0;
    constexpr int PA = prm::PLEN + 2 * prm::PORDER;         // 1092
    constexpr int PXY = prm::PLEN * (prm::PB + 1);          // 82620
    constexpr int PXA = PA * (prm::PB + 1);                 // 88452
    constexpr int LESTIT = 59;  // mdltbl.i iteration-save table index

    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& s = ctx.series;
    auto& h = ctx.hiddn;

    // The EQUIVALENCE overlay of diag/qtf/wa1..wa4/tmpa onto txy (rgarma.f:100-
    // 104) has no live overlap, so use disjoint arrays (scouting parity risk 11).
    std::vector<double> txy(PXA);
    double estprm[prm::PARIMA];
    int ipvt[prm::PARIMA];
    double diag[prm::PARIMA], qtf[prm::PARIMA], wa1[prm::PARIMA], wa2[prm::PARIMA],
        wa3[prm::PARIMA], wa4[PA], tmpa[PA], wacov[prm::PARIMA];

    // (Lprtit sets /lgiter/ Frstcl/Scndcl for prtitr's formatting -- deferred
    // with the prtitr print hook; it has no numeric effect here.)
    bool gudrun = h.issap < 2 && h.irev < 4;

    // Check the work array size.
    if (d.nspobs * m.ncxy > PXY) {
        errhdr(ctx);
        writln(ctx,
               " ERROR: Work array too small, " + std::to_string(d.nspobs) +
                   "*" + std::to_string(m.ncxy) + ">" + std::to_string(PXY) + ".",
               stdio::STDERR, ctx.units.mt2, true);
        if (lauto)
            lauto = false;
        else
            abend(ctx);
        return;
    }

    int info = 1;
    d.armaer = 0;
    constexpr bool intflt = true;  // always re-initialize |G'G| in armafl
    bool locest = lestim;
    s.lrgrsd = 1e6;
    int nprint = lprtit ? 1 : 0;
    (void)nprint;  // fed only to the deferred lmdif prtitr hook

    // Copy the model specs to the common variables and set the starting values.
    strtvl(ctx);
    bool la = false;
    setmdl(ctx, estprm, la);
    if (ctx.error.lfatal) return;

    nefobs = d.nspobs - m.nintvl;
    if (nefobs < m.nextvl) {
        writln(ctx,
               " ERROR: Number of observations after differencing (" +
                   std::to_string(nefobs) + ") < minimum series length (" +
                   std::to_string(m.nextvl) + ").",
               stdio::STDERR, ctx.units.mt2, true);
        if (lauto)
            lauto = false;
        else
            abend(ctx);
        return;
    }

    s.dnefob = static_cast<double>(nefobs);
    int neltxy = d.nspobs * m.ncxy;

    // Check whether the objective function is identically zero: if the series is
    // annihilated by the Difference/AR operators there is nothing to estimate.
    if (m.mdl(prm::DIFF) - m.mdl(prm::DIFF - 1) > 0) {
        int nelta = d.nspobs;
        int i2 = 0;
        for (int i = m.ncxy; i <= neltxy; i += m.ncxy) {
            i2 = i2 + 1;
            txy[i2 - 1] = d.xy(i);
        }
        arflt(nelta, d.arimap.data(), m.arimal.data(), m.opr.data(),
              m.mdl(prm::DIFF - 1), m.mdl(prm::DIFF) - 1, txy.data(), nelta);
        i2 = 1;
        bool xyzero = dpeq(txy[0], ZERO);
        while (i2 < nelta && xyzero) {
            i2 = i2 + 1;
            xyzero = dpeq(txy[i2 - 1], ZERO);
        }
        if (xyzero) {
            d.armaer = prm::POBFN0;
            if (lauto) lauto = false;
            return;
        }
    }

    // Input tolerances are on the log likelihood; convert to the deviance the
    // program actually checks. (scouting parity risk 6.)
    double devtol = TWO / s.dnefob * m.tol;
    if (m.lar || m.lma)
        na = nefobs + m.mxmalg;
    else
        na = nefobs;

    double eps = m.stepln;
    double tnltol, nltolf = 0.0;
    int tnlitr;
    if (m.nb > 0) {
        tnltol = TWO / s.dnefob * m.nltol0;
        nltolf = TWO / s.dnefob * m.nltol;
        tnlitr = mxnlit;
    } else {
        tnltol = devtol;
        tnlitr = mxiter;
    }

    // Check the nonlinear work arrays are big enough.
    if (m.nestpm > 0) {
        d.nlwrk = std::max(na, d.nspobs) * (m.nestpm + 1) + 5 * m.nestpm;
        if (d.nlwrk > PXA) {
            writln(ctx,
                   " ERROR: Non linear work array too small " +
                       std::to_string(d.nlwrk) + ">" + std::to_string(PXA) + ".",
                   stdio::STDERR, ctx.units.mt2, true);
            if (lauto)
                lauto = false;
            else
                abend(ctx);
            return;
        } else {
            d.nlwrk = std::max(na, d.nspobs);
        }
    }

    int iter = 0;
    d.nliter = 0;
    d.nfev = 0;
    double apa = 0.0, objfcn = 0.0;

    // The fcn callback (fcnar) and the model-sync hook (upespm) threaded into
    // lmdif/fdjac2.
    MinpackFcn fcn = [&ctx](int& mm, int nn, const double* x, double* fvec,
                            bool lau, bool gr, int& iflag, bool lck) {
        fcnar(ctx, mm, nn, x, fvec, lau, gr, iflag, lck);
    };
    MinpackSync sync = [&ctx](const double* x) { upespm(ctx, x); };

    // IGLS iteration loop: GLS regression given the current ARMA parameters,
    // then a nonlinear ARMA re-estimation (lmdif). Fortran label 10 == RETURN;
    // label 20 == continue the loop.
    int lstnit = 0;
    while (true) {
        copy(d.xy.data(), neltxy, 1, txy.data());
        int nrtxy = 0, flterr = 0;
        armafl(ctx, d.nspobs, m.ncxy, intflt, false, txy.data(), nrtxy, PXA,
               flterr);

        // Filter failure: check invertibility/stationarity, record the error.
        if (flterr > 0) {
            if (m.mdl(prm::MA) > m.mdl(prm::DIFF)) {
                int inverr = 0;
                chkrt2(ctx, true, inverr, h.lhiddn);
                if (ctx.error.lfatal) return;
                if (inverr > 0) {
                    d.armaer = inverr;
                } else {
                    d.armaer = flterr;
                    d.var = ZERO;
                }
            } else {
                d.armaer = flterr;
                d.var = ZERO;
            }
            return;  // GO TO 10
        }

        // Regression parameters given the current ARMA parameters.
        apa = 0.0;
        if (m.nb <= 0) {
            yprmy(txy.data(), nrtxy, apa);
            d.chlxpx(1) = std::sqrt(apa);
        } else {
            olsreg(ctx, txy.data(), nrtxy, m.ncxy, m.ncxy, d.b.data(),
                   d.chlxpx.data(), prm::PXPX, d.sngcol);
            if (ctx.error.lfatal) return;
            if (d.sngcol > 0) {
                d.convrg = false;
                d.armaer = prm::PSNGER;
                if (lauto) lauto = false;
                return;  // GO TO 10
            }
            d.nfev = d.nfev + m.ncxy + 1;
        }

        // Objective function and convergence test.
        resid(ctx, txy.data(), nrtxy, m.ncxy, m.ncxy, 1, m.nb, MONE, d.b.data(),
              a);
        if (ctx.error.lfatal) return;
        yprmy(a, nrtxy, apa);
        objfcn = apa * std::exp(d.lndtcv / s.dnefob);

        // Largest residual magnitude for the constrained Minpack estimation.
        if (iter == 0) {
            maxvec(a, nrtxy, s.lrgrsd);
            s.lrgrsd = s.lrgrsd * std::exp(d.lndtcv / TWO / s.dnefob);
        }
        bool lnxstp = locest && m.nestpm > 0 &&
                      stpitr(ctx, m.lprier, objfcn, devtol, iter, d.nliter,
                             mxiter, d.convrg, d.armaer, h.lhiddn);
        iter = iter + 1;

        // (Lprtit ARMA/IGLS prtitr iteration prints deferred to the .out
        // milestone; they have no numeric effect.)

        // Re-estimate the ARMA parameters.
        bool goto20 = false;
        if (lnxstp) {
            if (iter > 2) tnltol = nltolf;
            lstnit = d.nliter;
            resid(ctx, d.xy.data(), d.nspobs, m.ncxy, m.ncxy, 1, m.nb, MONE,
                  d.b.data(), s.tsrs.data());
            int lm_mxiter = d.nliter + tnlitr;  // snapshot before the ref counter
            lmdif(fcn, na, m.nestpm, estprm, a, lauto, gudrun, tnltol, ZERO, ZERO,
                  lm_mxiter, eps, diag, 1, 100.0, nprint, info, d.nliter, d.nfev,
                  d.armacm.data(), PA, ipvt, qtf, wa1, wa2, wa3, wa4, sync);
            if (ctx.error.lfatal) return;
            locest = m.nb > 0;
            if (info >= 1 && info <= 8 && d.nliter > lstnit) goto20 = true;
        }
        if (goto20) continue;  // GO TO 20

        // ---- convergence classification (only when the model is estimated) ----
        if (lestim && m.nestpm > 0) {
            if (m.nb == 0) {
                // .AND. binds tighter than .OR. (rgarma.f:389-390).
                d.convrg = (d.convrg && (info >= 1 && info <= 4)) ||
                           (info >= 6 && info <= 8);
            } else {
                d.convrg = d.convrg && info >= 1 && info <= 8;
            }
            if (info < 0) {
                d.armaer = prm::PUNKER;
                d.convrg = false;
                d.var = ZERO;
                return;  // GO TO 10
            } else if (info == 0) {
                d.armaer = prm::PINPER;
            } else if (info == 5 || (d.nliter >= mxiter && !d.convrg)) {
                if (d.nliter >= mxiter)
                    d.armaer = prm::PMXIER;
                else
                    d.armaer = prm::PMXFER;
            } else if (info >= 1 && info <= 4) {
                d.armaer = 0;
            } else {
                d.armaer = info;
            }
        }

        // ---- ML variance and log likelihood ----
        d.var = apa / s.dnefob;
        if (d.var < TWO * dpmpar(1)) d.var = ZERO;
        if (dpeq(d.var, ZERO)) {
            d.lnlkhd = ZERO;
        } else {
            d.lnlkhd =
                -(d.lndtcv + s.dnefob * (std::log(TWO * PI * d.var) + ONE)) / TWO;
        }

        // ---- ARMA parameter covariance from the optimizer QR (only at the MLE)
        if (lestim && m.nestpm > 0 && d.convrg) {
            resid(ctx, d.xy.data(), d.nspobs, m.ncxy, m.ncxy, 1, m.nb, MONE,
                  d.b.data(), s.tsrs.data());
            fcnar(ctx, na, m.nestpm, estprm, tmpa, lauto, gudrun, info, false);
            if (ctx.error.lfatal) return;
            int iflag = 2;
            fdjac2(fcn, na, m.nestpm, estprm, tmpa, d.armacm.data(), PA, iflag,
                   0.0, wa4, lauto, gudrun, false);
            upespm(ctx, estprm);
            if (iflag >= 0) {
                qrfac(na, m.nestpm, d.armacm.data(), PA, true, ipvt, m.nestpm,
                      wa1, wa2, wa3);
                for (int i = 1; i <= m.nestpm; ++i) d.armacm(i, i) = wa1[i - 1];
                covar(m.nestpm, d.armacm.data(), PA, ipvt, tnltol, info, wacov);
                m.lcalcm = info == 0;
                if (!m.lcalcm) d.armaer = prm::PACSER;
            } else {
                m.lcalcm = false;
            }
        }

        // (LESTIT iteration-save via savitr deferred to the .out/save milestone.)
        if (ctx.tbllog.savtab(LESTIT)) {
            // dvec(1)=0; savitr(LCLOSE, iter, iter, 0, dvec, 1);  -- deferred
        }
        return;  // GO TO 10
    }
}

// xrlkhd.f -- corrected AIC (AICC) for the estimated model (see hpp).
void xrlkhd(X13Context& ctx, double& aicc, int nxcld) {
    constexpr double ONE = 1.0, TWO = 2.0, ZERO = 0.0;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    int nefobs = d.nspobs - nxcld;
    double dnefob = static_cast<double>(nefobs);
    // Estimated-parameter count: Ncxy less any held-fixed regression betas.
    double dnp = static_cast<double>(m.ncxy);
    if (m.nb > 0) {
        for (int ilag = 1; ilag <= m.nb; ++ilag)
            if (m.regfx(ilag)) dnp = dnp - ONE;
    }
    double dnp1 = dnp + ONE;
    aicc = prm::DNOTST;
    if (d.var > ZERO && d.convrg && dnefob > dnp1)
        aicc = -TWO * (d.lnlkhd - dnefob * dnp / (dnefob - dnp1));
}

// prlkhd.f -- likelihood statistics for the estimated model (see hpp). Computes
// the transform-Jacobian-adjusted log likelihood Olkhd=Lnlkhd+jacadj and the
// information criteria (Aic/Aicc/Hnquin/Bic/Bic2/Eic) into ctx.lkhd. All the
// WRITE output (the "Likelihood Statistics" block, the savelog rows) and the
// x11-holiday/x11reg penalty notes are deferred to the .out/save milestone; the
// numerics and the exact-ML / convergence gates are reproduced. y is the
// original untransformed, undifferenced series over the span; adj the prior-
// adjustment factors (1 when there is no prior). Reachable pre-x11: Adjmod<2 with
// Khol/Ixreg absent, so the Jacobian is the pure Box-Cox/logit term.
void prlkhd(X13Context& ctx, const double* y, const double* adj, int adjmod,
            int fcntyp, double lam) {
    constexpr double ONE = 1.0, TWO = 2.0, ZERO = 0.0;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& lk = ctx.lkhd;

    // Fixmdl==3 with automatically-identified outliers suppresses the stats
    // (prlkhd.f:68). That path needs the fixed-model + auto-outlier flow (not
    // reachable here); the group-title search is deferred with the print.
    int nefobs = d.nspobs - m.nintvl;
    double dnefob = static_cast<double>(nefobs);

    // np: estimated parameters = Ncxy, less held-fixed regression betas, plus the
    // free ARMA lags (Opr(Nopr)-1 lags, Arimaf false).
    int np = m.ncxy;
    if (m.nb > 0)
        for (int ilag = 1; ilag <= m.nb; ++ilag)
            if (m.regfx(ilag)) np = np - 1;
    bool lclaic = (m.mdl(prm::AR) - m.mdl(prm::DIFF) == 0 || m.lar) &&
                  (m.mdl(prm::MA) - m.mdl(prm::AR) == 0 || m.lma);
    if (m.nopr > 0) {
        int endlag = m.opr(m.nopr) - 1;
        for (int ilag = 1; ilag <= endlag; ++ilag)
            if (!m.arimaf(ilag)) np = np + 1;
    }

    if (d.var <= ZERO) return;  // no variance -> no likelihood (prlkhd.f:138)

    // Jacobian of the transformation, summed over the effective span
    // (i = Nintvl+1..Nspobs). f'(y/a)*a^-1 by the chain rule.
    double jacadj = ZERO;
    if (adjmod < 2) {
        for (int i = m.nintvl + 1; i <= d.nspobs; ++i) {
            double yi_full = y[i - 1], adji = adj[i - 1];
            if (dpeq(yi_full, ZERO) || dpeq(adji, ZERO)) continue;
            double jaci = adji;   // Khol/Ixreg x11 factors deferred (absent here)
            if (fcntyp != 4) {
                double yi = yi_full / jaci;
                if (fcntyp == 3)  // logit
                    jaci = jaci / (jaci * yi_full - yi_full * yi_full);
                else if (!dpeq(lam, ONE))  // Box-Cox (incl. log, sqrt)
                    jaci = std::pow(std::abs(yi), lam - ONE) / jaci;
            }
            jacadj = jacadj + std::log(jaci);
        }
    } else if (!dpeq(lam, ONE) || fcntyp == 3) {
        for (int i = m.nintvl + 1; i <= d.nspobs; ++i) {
            double yi = y[i - 1];
            double jaci = ONE;
            if (fcntyp == 3)
                jaci = jaci / (jaci * yi - yi * yi);
            else if (!dpeq(lam, ONE))
                jaci = std::pow(std::abs(yi), lam - ONE) / jaci;
            jacadj = jacadj + std::log(jaci);
        }
    }
    lk.olkhd = d.lnlkhd + jacadj;  // dpeq(jacadj,0) -> Olkhd=Lnlkhd (same value)

    // prlkhd.f:248-355 is a THREE-armed chain and this port had fused it into
    // one early return, which dropped the first and third arms entirely.
    //
    //   IF(.not.lclaic)        -> the NOTE below
    //   ELSE IF(Convrg)        -> the AIC block
    //   ELSE                   -> every statistic reset to DNOTST
    //
    // `IF(Irev.eq.4)RETURN` closes the first and third arms; in this port
    // nothing follows the chain but `IF(Lprtfm)`'s Mt1 legend, which is the
    // deferred print engine, so the early return has no counterpart to write.
    if (!lclaic) {
        // :250-254 -- the Mt1 half is deferred with the print engine; the Mt2
        // half is the `.err`, and the guard on it is `gudrun` (arima.f:123's
        // `Issap.lt.2 .and. Irev.lt.4`), not `lprt`. The FORMAT opens with a
        // `/`, so the record above the NOTE is EMPTY -- not writln's two-space
        // blank. The goldens show the difference in the byte column.
        if (ctx.hiddn.issap < 2 && ctx.hiddn.irev < 4)
            ctx.channels_.unit(ctx.units.mt2).put(
                fwrite_fmt("(/,' NOTE:  AIC and related statistics are printed"
                           " only ','for exact',/,"
                           "'  maximum likelihood estimation.')") +
                "\n");
        return;
    }
    if (!d.convrg) {
        // :346-353 -- an exact-ML fit that did NOT converge publishes nothing.
        // Note what is absent: `Bic2` is not reset, and `Olkhd` above keeps the
        // value it was just given. Transcribed, including the asymmetry.
        lk.aic = prm::DNOTST;
        lk.aicc = prm::DNOTST;
        lk.bic = prm::DNOTST;
        lk.hnquin = prm::DNOTST;
        d.lnlkhd = prm::DNOTST;
        lk.eic = prm::DNOTST;
        return;
    }
    double dnp = static_cast<double>(np);
    lk.aic = -TWO * (d.lnlkhd + jacadj - dnp);
    if (nefobs > np + 1)
        lk.aicc =
            -TWO * (d.lnlkhd + jacadj - dnefob * dnp / (dnefob - (dnp + ONE)));
    lk.hnquin = -TWO * (d.lnlkhd + jacadj - std::log(std::log(dnefob)) * dnp);
    lk.bic = -TWO * (d.lnlkhd + jacadj) + dnp * std::log(dnefob);
    lk.bic2 = (-TWO * d.lnlkhd + dnp * std::log(dnefob)) / dnefob;
    if (d.eick > ZERO)
        lk.eic = -TWO * (d.lnlkhd + jacadj) + dnp * d.eick;
    else
        lk.eic = prm::DNOTST;
}

// armats.f -- t-statistics for the ARMA parameter estimates (see hpp).
void armats(X13Context& ctx, double* tval) {
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    if (d.armaer == prm::PACSER) {
        writln(ctx,
               "ERROR: The covariance matrix of the ARMA parameters is "
               "singular;",
               stdio::STDERR, ctx.units.mt2, true);
        writln(ctx,
               "       cannot compute t-statistics for the ARMA parameters.",
               stdio::STDERR, ctx.units.mt2, false);
        abend(ctx);
        return;
    }
    int itv = 0;
    for (int iflt = prm::AR; iflt <= prm::MA; ++iflt) {
        int begopr = m.mdl(iflt - 1);
        int endopr = m.mdl(iflt) - 1;
        for (int iopr = begopr; iopr <= endopr; ++iopr) {
            int beglag = m.opr(iopr - 1);
            int endlag = m.opr(iopr) - 1;
            for (int ilag = beglag; ilag <= endlag; ++ilag) {
                // CB-6: itv counts every lag (not just free ones) while Armacm
                // is packed by the free params -- misindexes on fixed ARMA lags.
                itv = itv + 1;
                tval[itv - 1] =
                    d.arimap(ilag) / std::sqrt(d.var * d.armacm(itv, itv));
            }
        }
    }
}

}  // namespace x13
