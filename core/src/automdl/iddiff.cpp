// iddiff.cpp -- iddiff.f (unit-root / differencing-order identification) and
// prterr.f / itrerr.f / prarma.f (the estimation-error report). iddiff's control
// flow follows the Fortran DO WHILE with the GO TO 10 (continue) / GO TO 20
// (exit) / GO TO 30 (finish) targets modeled by a continue-flag and a break.
// Farray state stays 1-based; raw scratch (txy, a) is 0-based. iddiff's
// Prttab table output is deferred (see the print-engine milestone); its
// decisions are the load-bearing effect. prterr, by contrast, emits: its Mt2
// half is the run's `===ERR===` block and gates read it.
#include "automdl/iddiff.hpp"

#include <cmath>
#include <vector>

#include "automdl/amdest.hpp"        // amdest
#include "automdl/idmodel.hpp"       // chkrt1
#include "automdl/mdlset.hpp"        // mdlint, mdlset
#include "numeric/numeric.hpp"       // totals, sdev, dppdi, dpmpar, dpeq
#include "regarima/armafilt.hpp"     // arflt
#include "regarima/estimate.hpp"     // rgarma
#include "regarima/regvar.hpp"       // regvar
#include "specparse/specparse.hpp"   // copy, adrgef, dlrgef, abend, setdp
#include "gen/model.hpp"             // prm::AR, DIFF, MA, PB, PRGTCN, PSNGER, PUNKER
#include "gen/srslen.hpp"            // prm::PLEN
#include "gen/notset.hpp"            // prm::DNOTST
#include "x13/fformat.hpp"           // fwrite_fmt (prterr/itrerr message formats)

namespace x13 {

// prarma.f -- the ARMA start values, dumped under itrerr's option (2) so the
// user can paste them back into the spec. Fh is a unit number; itrerr's only
// caller passes Mt2 (the Mt1 half is the deferred .out engine).
static void prarma(X13Context& ctx, int fh) {
    using namespace prm;
    const auto& m = ctx.model;
    const auto& d = ctx.mdldat;
    static const char* armopr[4] = {"", "", "ar  ", "ma  "};
    auto& u = ctx.channels_.unit(fh);
    for (int iflt = AR; iflt <= MA; ++iflt) {
        const int begopr = m.mdl(iflt - 1);
        const int endopr = m.mdl(iflt) - 1;
        if (endopr < begopr) continue;
        u.put(fwrite_fmt("('   ',a,'=(')", std::string(armopr[iflt])) + "\n");
        for (int iopr = begopr; iopr <= endopr; ++iopr) {
            const int beglag = m.opr(iopr - 1);
            const int endlag = m.opr(iopr) - 1;
            for (int ilag = beglag; ilag <= endlag; ++ilag) {
                if (m.arimaf(ilag))
                    u.put(fwrite_fmt("('    ',e24.10,a)", d.arimap(ilag),
                                     std::string("f")) + "\n");
                else
                    u.put(fwrite_fmt("('    ',e24.10)", d.arimap(ilag)) + "\n");
            }
        }
        u.put(fwrite_fmt("('   )')") + "\n");
    }
}

// itrerr.f -- the iteration/function-evaluation limit report. Only the Mt2 half
// is emitted (Mt1 is the deferred .out engine, STDERR is not gated), but note
// that `Lauto` changes the Mt2 TEXT and not merely which channels are written:
// the automatic-model arm offers two remedies and the explicit arm offers three
// with the ARMA start values under them.
static void itrerr(X13Context& ctx, const char* errstr, bool lauto, int issap,
                   int irev) {
    const int Mt2 = ctx.units.mt2;
    auto& mt2 = ctx.channels_.unit(Mt2);
    static const std::string RULE =
        "(/,' "
        "***********************************************************************')";
    const std::string es(errstr);

    mt2.put(fwrite_fmt(RULE) + "\n");
    if (issap == 2) {
        mt2.put(fwrite_fmt("(/,' ERROR: Estimation failed to converge -- maximum ',a,"
                           "' reached',/,'        during sliding spans analysis.')",
                           es) + "\n");
    } else if (irev == 4) {
        mt2.put(fwrite_fmt("(/,' ERROR: Estimation failed to converge -- maximum ',a,"
                           "' reached',/,'        during history analysis.')",
                           es) + "\n");
    } else {
        mt2.put(fwrite_fmt("(/,' ERROR: Estimation failed to converge -- maximum ',a,"
                           "' reached.')", es) + "\n");
    }
    mt2.put(fwrite_fmt("('        Rerun program trying one of the following:',/,"
                       "10x,'(1) Allow more iterations (set a larger value of ',"
                       "'maxiter).')") + "\n");
    if (lauto) {
        mt2.put(fwrite_fmt("(10x,'(2) Try a different model.',//,1x,'See ',a,"
                           "' of the ',a,' ',a,' for more discussion.')",
                           std::string(stdio::MDLSEC), std::string(stdio::PRGNAM),
                           std::string(stdio::DOCNAM)) + "\n");
        mt2.put(fwrite_fmt(RULE) + "\n");
        return;
    }
    bool lparma = false;
    if (issap == 2 || irev == 4) {
        mt2.put(fwrite_fmt("(10x,'(2) Fix the values of the ARMA coefficients to ',"
                           "'those obtained',/,14x,"
                           "'while estimating the full series (set fixmdl=yes)')") + "\n");
    } else {
        mt2.put(fwrite_fmt("(10x,'(2) Use initial values for ARMA parameters as ',"
                           "'given ',a,'.')", std::string("below")) + "\n");
        lparma = true;
    }
    mt2.put(fwrite_fmt("(10x,'(3) Try a different model.',//,1x,'See ',a,"
                       "' of the ',a,' ',a,' for more discussion.')",
                       std::string(stdio::MDLSEC), std::string(stdio::PRGNAM),
                       std::string(stdio::DOCNAM)) + "\n");
    if (lparma) {
        // itrerr.f:82-84 -- `WRITE(Mt2,*)' '` is list-directed, so it emits a
        // leading blank of its own plus the blank datum.
        mt2.put("  \n");
        prarma(ctx, Mt2);
        mt2.put("  \n");
    }
    mt2.put(fwrite_fmt(RULE) + "\n");
}

// prterr.f -- report whatever /mdldat/'s Armaer says went wrong in the last
// estimation. Ported here as the Mt2 half plus the control flow: the Mt1
// listing and the STDERR courtesy line (FORMAT 1230) belong to the deferred
// .out engine and to a channel no gate reads.
//
// This routine used to be a four-line stub that zeroed Convrg on PUNKER and
// dropped both its arguments -- the shape CLAUDE.md calls "an argument the
// Fortran passes and this port dropped". `Lauto` selects the message TEXT and
// gates five `CALL abend()`s; `Nefobs` is a number inside FORMAT 1180. With it
// stubbed, a run whose estimation hit the iteration limit came back with an
// EMPTY `===ERR===` block and, until arima.f:1216 was ported beside it,
// `OUTCOME: OK`.
void prterr(X13Context& ctx, int nefobs, bool lauto) {
    using namespace prm;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    const int Mt2 = ctx.units.mt2;
    auto& mt2 = ctx.channels_.unit(Mt2);
    const int issap = ctx.hiddn.issap;
    const int irev = ctx.hiddn.irev;
    const int ae = d.armaer;

    if (ae == PUNKER || ae < 0) {
        // Unknown error.
        d.convrg = false;
        d.var = 0.0;
        if (issap == 2) {
            errhdr(ctx);
            mt2.put(fwrite_fmt("(/,' ERROR: Nonlinear estimation error with unknown "
                               "cause ','during ',/,'        sliding spans analysis.')") + "\n");
        } else if (irev == 4) {
            errhdr(ctx);
            mt2.put(fwrite_fmt("(/,' ERROR: Nonlinear estimation error with unknown "
                               "cause ','during ',/,'        revisions analysis.')") + "\n");
        } else {
            // No errhdr on this arm -- prterr.f:51-55 has none.
            mt2.put(fwrite_fmt("(/,' ERROR: Nonlinear estimation error with unknown "
                               "cause.',/)") + "\n");
        }
    } else if (ae == PSNGER || ae == PISNER) {
        // Xy is singular.
        std::string str;
        int nchr = 4;
        if (d.sngcol < m.ncxy) {
            getstr(ctx, m.colttl.data(), m.colptr.data(), m.ncoltl, d.sngcol, str,
                   nchr);
            if (ctx.error.lfatal) return;
        } else {
            str = "data";
        }
        const std::string col = str.substr(0, static_cast<std::size_t>(nchr));
        errhdr(ctx);
        if (ae == PISNER) {
            mt2.put(fwrite_fmt("(/,' ERROR: Regression matrix singular because of ',a,"
                               "'.',/,'        Remove variable(s) from regression spec "
                               "and ','try again.',/)", col) + "\n");
        } else {
            mt2.put(fwrite_fmt("(/,' ERROR: Regression matrix singular because of ',a,"
                               "'.',/,'        Check regression model or change "
                               "automatic ','outlier options',/,'        i.e. method to "
                               "addone or types to identify AO ','only.',/)", col) + "\n");
        }
        if (!lauto) abend(ctx);
        return;
    } else if (ae == PINPER) {
        errhdr(ctx);
        mt2.put(fwrite_fmt("(/,' WARNING: Improper input parameters to the likelihood',"
                           "'minimization routine.',/,'          Please send us the data "
                           "and spec file that ','produced this',/,'          message "
                           "(x12@census.gov).')") + "\n");
    } else if (ae == PMXIER) {
        errhdr(ctx);
        itrerr(ctx, "iterations", lauto, issap, irev);
    } else if (ae == PMXFER) {
        errhdr(ctx);
        itrerr(ctx, "function evaluations", lauto, issap, irev);
    } else if (ae == PSCTER || ae == PSPMER || ae == PCOSER) {
        errhdr(ctx);
        mt2.put(fwrite_fmt("(/,' WARNING: Estimation was terminated because no ',"
                           "'further improvement in',/,'          the likelihood was "
                           "possible.  Check ','iteration output to ',/,'          "
                           "confirm that model estimation really ','converged.')") + "\n");
        if (!m.lprier) {
            mt2.put(fwrite_fmt("(/)") + "\n");
        } else if (ae == PSCTER) {
            mt2.put(fwrite_fmt("('          Convergence tolerance on the likelihood is ',"
                               "'too strict.',/)") + "\n");
        } else if (ae == PSPMER) {
            errhdr(ctx);
            mt2.put(fwrite_fmt("(/,' WARNING: Convergence tolerance for the relative ',"
                               "'difference in the',/,'          parameter estimates is "
                               "too strict.')") + "\n");
        } else {
            errhdr(ctx);
            mt2.put(fwrite_fmt("(/,'          Cosine of the angle between the vector of ',"
                               "'expected values and ',/,'          any column of the "
                               "jacobian is too small.',/)") + "\n");
        }
    } else if (ae == PNIFER || ae == PNIMER) {
        // Invertibility errors. Print the estimates, and stop.
        std::string str;
        int nchr = 0;
        getstr(ctx, m.oprttl.data(), m.oprptr.data(), m.noprtl, d.prbfac, str, nchr);
        if (ctx.error.lfatal) return;
        const std::string opr = str.substr(0, static_cast<std::size_t>(nchr));
        errhdr(ctx);
        if (ae == PNIFER) {
            mt2.put(fwrite_fmt("(/,' ERROR: ',a,' has roots inside the unit circle but ',"
                               "/,'some',/,'         parameters are fixed so cannot "
                               "invert the ','operator.',/)", opr) + "\n");
        } else {
            mt2.put(fwrite_fmt("(/,' ERROR: ',a,' has roots inside the unit circle but ',"
                               "'some are missing',/,'        so cannot invert the "
                               "operator.  Try ','including all lags.',/)", opr) + "\n");
        }
        // prterr.f:170-175 -- Lprier is forced on for the root listing, which is
        // Mt1-only: chkrt2's one Mt2 write is under `IF(Lprmsg)` and this call
        // passes Lprmsg=F. So the stub is faithful on the gated channel.
        const bool ltmper = m.lprier;
        m.lprier = true;
        int itmp = 0;
        chkrt2(ctx, false, itmp, ctx.hiddn.lhiddn);
        if (ctx.error.lfatal) return;
        m.lprier = ltmper;
        if (!lauto) abend(ctx);
        return;
    } else if (ae == PCNTER) {
        // Stpitr convergence errors.
        errhdr(ctx);
        mt2.put(fwrite_fmt("(/,' ERROR: Convergence tolerance must be set larger than ',"
                           "'machine',/,'precision',e25.14,'.',/)",
                           2.0 / static_cast<double>(nefobs) * dpmpar(1)) + "\n");
        if (!lauto) abend(ctx);
        return;
    } else if (ae == PDVTER) {
        errhdr(ctx);
        mt2.put(fwrite_fmt("(/,' WARNING: Deviance was less than machine precision ',"
                           "'so could not',/,'          calculate the relative "
                           "deviance.',/)") + "\n");
    } else if (ae == PACSER) {
        // Singular ARMA covariance matrix.
        errhdr(ctx);
        mt2.put(fwrite_fmt("(/,' WARNING: The covariance matrix of the ARMA ',"
                           "'parameters is singular,',/,'          so the standard "
                           "errors and the correlation ','matrix of the ARMA',/,"
                           "'          parameters will not be printed out.',/)") + "\n");
    } else if (ae == POBFN0) {
        // Objective function equal to zero.
        errhdr(ctx);
        mt2.put(fwrite_fmt("(/,' ERROR: Differencing has annihilated the series.',/,"
                           "'        Check the model specified in the arima spec,',"
                           "' set or change',/,'        the possible differencing orders "
                           "(if using the ','automdl spec), or',/,'        change the "
                           "models specified in the automatic ','model file',/,"
                           "'        (if using the pickmdl spec).')") + "\n");
        if (!lauto) abend(ctx);
    }
}

void iddiff(X13Context& ctx, int& idr, int& ids, double* trnsrs, int& nefobs,
            int& frstry, double* a, int& na, int imu, bool& lmu, bool svldif,
            int lsumm) {
    using namespace prm;
    (void)svldif;
    (void)lsumm;  // gate only deferred summary prints
    constexpr double ONE = 1.0, PONE = 1.0e-1, PTWO = 2.0e-1, PT9 = 9.0e-1,
                     PTOL = 1.0e-3;
    constexpr int TWOHND = 200;

    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& ar = ctx.arima;

    std::vector<double> txy(PLEN, 0.0), xpxinv(PB * (PB + 1) / 2, 0.0);
    double tmp2[2] = {0.0, 0.0};

    int irm1 = 0;
    bool inptok = true;
    int mxitbk = ar.mxiter;
    int iround = 1;
    int Id = 0;
    int idrf = 0, idsf = 0;
    int limrd = idr, limsd = ids;
    double rmaxr = 0.0, rmaxs = 0.0;
    int ardsp = 0, ar1r = 0, ar1s = 0, ma1r = 0, ma1s = 0;
    int info = 0;
    double tolbak = 0.0, nltbak = 0.0;

    while (true) {
        // ---- set up the trial (p 0 0)(P 0 0) model for this round ----
        mdlint(ctx);
        if (Id > 0 || iround > 1) {
            if (m.lseff || m.sp == 1)
                mdlset(ctx, 1, idrf, 1, 0, 0, 0, inptok);
            else
                mdlset(ctx, 1, idrf, 1, 1, idsf, 1, inptok);
            if (ctx.error.lfatal) return;
            ardsp = m.nnsedf + m.nseadf;
            nefobs = d.nspobs - m.nintvl;
            int i3 = (m.sp == 1) ? 2 : 3;
            ar1r = ardsp + 1;
            ma1r = ardsp + i3;
            ar1s = 0;
            ma1s = 0;
            if (m.sp > 1) {
                ar1s = ar1r + 1;
                ma1s = ma1r + 1;
            }
        } else {
            if (m.lseff || m.sp == 1) {
                ar1r = ar.frstar;
                ar1s = 0;
            } else if (m.sp == 2) {
                ar1r = 1;
                ar1s = 1;
            } else if (m.sp == 3 && ar.frstar >= 3) {
                ar1r = 2;
                ar1s = 1;
            } else if (m.sp == 4 && ar.frstar == 4) {
                ar1r = 3;
                ar1s = 1;
            } else {
                ar1r = ar.frstar;
                ar1s = 1;
            }
            mdlset(ctx, ar1r, 0, 0, ar1s, 0, 0, inptok);
            if (ctx.error.lfatal) return;
            ardsp = 0;
            nefobs = d.nspobs - m.nintvl;
        }

        if (!inptok || ctx.error.lfatal) {
            if (!ctx.error.lfatal) abend(ctx);
            return;
        }

        // ---- difference the data and mean-delete ----
        int nelta = d.nspobs;
        copy(trnsrs, nelta, 1, txy.data());
        if (Id > 0)
            arflt(nelta, d.arimap.data(), m.arimal.data(), m.opr.data(),
                  m.mdl(DIFF - 1), m.mdl(DIFF) - 1, txy.data(), nelta);
        double xmu;
        smeadl(txy.data(), 1, nelta, nelta, xmu);

        // ---- Hannan-Rissen estimate of this trial model ----
        amdest(ctx, txy.data(), nelta, nefobs, ardsp, true, false, info);
        if (ctx.error.lfatal) return;

        if (d.armaer == PSNGER) {
            abend(ctx);
            return;
        } else if (d.armaer != 0) {
            d.armaer = 0;
        }

        // ---- first round: check whether the AR roots are inside the circle ----
        if (iround == 1 && info == 0) {
            bool linv = true;
            chkrt1(ctx, idr, ids, rmaxr, rmaxs, linv, ar.ub1lim);
            if (ctx.error.lfatal) return;
            if (!linv) {
                if (ar1r == 1 && std::abs(d.arimap(1)) > 1.02)
                    info = 1;
                else if (ar1r > 1)
                    info = 1;
                if (ar1s == 1 && std::abs(d.arimap(ar1r + 1)) > 1.02) info = 1;
            }
            if (info > 0)
                for (int i = 1; i <= ar1r + ar1s; ++i) d.arimap(i) = PONE;
        } else if (info != 0) {
            if (iround == 1) ma1r = ar1r + ar1s + 1;
            for (int i = ar1r; i <= ma1r - 1; ++i) d.arimap(i) = PONE;
            if (iround > 1)
                for (int i = ma1r; i <= ma1s; ++i) d.arimap(i) = PTWO;
        }
        if (irm1 == 1) {
            info = 1;
            for (int i = ar1r; i <= ma1s; ++i) d.arimap(i) = PONE;
        }
        if (idrf == 0 && idsf == 0 && iround > 2) info = 1;

        // ---- re-estimate by exact/conditional MLE when a model was flagged ----
        if (info != 0) {
            bool lxar = m.lextar;
            m.lextar = ar.exdiff > 0;
            m.lar = m.lextar && m.mxarlg > 0;
            if (m.lextar) {
                m.nintvl = m.mxdflg;
                m.nextvl = m.mxarlg + m.mxmalg;
                if (ar.exdiff == 2) ar.mxiter = TWOHND;
            } else {
                m.nintvl = m.mxdflg + m.mxarlg;
                m.nextvl = 0;
            }
            // Loosen tolerance for the exact-likelihood pass.
            if (m.tol < PTOL) {
                nltbak = m.nltol;
                tolbak = m.tol;
                m.tol = PTOL;
                m.nltol = PTOL;
                m.nltol0 = 100.0 * m.tol;
            }
            regvar(ctx, trnsrs, d.nspobs, ar.fctdrp, ctx.extend.nfcst, 0,
                   ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
                   ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
            rgarma(ctx, ar.lestim, ar.mxiter, ar.mxnlit, false, a, na, nefobs,
                   inptok);
            if (!ctx.error.lfatal) {
                if (!d.convrg && ar.exdiff == 2) {
                    m.nintvl = m.mxdflg + m.mxarlg;
                    m.nextvl = 0;
                    m.lextar = false;
                    m.lar = false;
                    ar.mxiter = mxitbk;
                    regvar(ctx, trnsrs, d.nspobs, ar.fctdrp, ctx.extend.nfcst, 0,
                           ar.userx.data(), ar.bgusrx.data(), ar.nrusrx,
                           ctx.prior.priadj, ar.reglom, ar.nrxy, ar.begxy.data(),
                           frstry, true, ar.elong);
                    rgarma(ctx, ar.lestim, ar.mxiter, ar.mxnlit, false, a, na,
                           nefobs, inptok);
                    if (ctx.error.lfatal) return;
                }
                prterr(ctx, nefobs, true);
                if (!d.convrg)
                    abend(ctx);
                else if (!inptok)
                    abend(ctx);
                if (dpeq(m.tol, PTOL)) {
                    m.tol = tolbak;
                    m.nltol = nltbak;
                    m.nltol0 = m.tol * 100.0;
                }
            }
            if (ctx.error.lfatal) return;

            // Restore the estimation-variable state.
            m.lextar = lxar;
            m.lar = m.lextar && m.mxarlg > 0;
            if (m.lextar) {
                m.nintvl = m.mxdflg;
                m.nextvl = m.mxarlg + m.mxmalg;
            } else {
                m.nintvl = m.mxdflg + m.mxarlg;
                m.nextvl = 0;
            }
            if (iround == 1) {
                bool linv = true;
                chkrt1(ctx, idr, ids, rmaxr, rmaxs, linv, ar.ub1lim);
                if (ctx.error.lfatal) return;
            }
        }

        // ---- check roots and update the difference counters ----
        int icon = 0;
        irm1 = 0;
        if (iround == 1) {
            idrf = idrf + idr;
            idsf = idsf + ids;
        } else {
            double din = 1.005 - ar.ub2lim;
            ar.cancel = ar.cancel - 0.002;
            if (d.armaer == 0 && ar.ub2lim >= 0.869) din = din + ar.ub2lim - 0.869;
            if (std::abs(ONE - d.arimap(ar1r)) <= din) {
                if (d.arimap(ar1r) > 1.02) {
                    irm1 = 1;
                    icon = 1;
                } else if (std::abs(d.arimap(ar1r) - d.arimap(ma1r)) > ar.cancel) {
                    icon = icon + 1;
                    idrf = idrf + 1;
                    Id = idrf + idsf * m.sp;
                }
            } else if (std::abs(d.arimap(ar1r)) > 1.12) {
                irm1 = 1;
                icon = 1;
            }
            if (m.sp > 1) {
                if (std::abs(ONE - d.arimap(ar1s)) <= 0.19) {
                    if (d.arimap(ar1s) > 1.02) {
                        irm1 = 1;
                        icon = 1;
                    } else if (std::abs(d.arimap(ar1s) - d.arimap(ma1s)) >
                                   ar.cancel &&
                               idsf == 0) {
                        icon = icon + 1;
                        idsf = idsf + 1;
                        Id = idrf + idsf * m.sp;
                        if (irm1 == 1) {
                            irm1 = 0;
                            icon = icon - 1;
                        }
                    }
                } else if (std::abs(d.arimap(ar1s)) > 1.12) {
                    irm1 = 1;
                    icon = 1;
                }
            }
            bool lchks = ar1s > 0;
            if (lchks)
                lchks = std::abs(d.arimap(ar1s) - d.arimap(ma1s)) <= ar.cancel;
            if (((std::abs(d.arimap(ar1r) - d.arimap(ma1r)) <= ar.cancel) ||
                 lchks) &&
                iround == 2) {
                if (idrf == 0 && idsf == 0 && (rmaxr >= PT9 || rmaxs >= PT9)) {
                    if (irm1 == 1 && icon == 1) {
                        irm1 = 0;
                        icon = 0;
                    }
                    icon = icon + 1;
                    if (rmaxr > rmaxs)
                        idrf = idrf + 1;
                    else
                        idsf = idsf + 1;
                }
            }
            lchks = ar1s > 0;
            if (lchks) lchks = std::abs(ONE - d.arimap(ar1s)) <= 0.16;
            bool lchks2 = ar1s > 0;
            if (lchks) lchks2 = std::abs(ONE - d.arimap(ar1s)) <= 0.17;
            if ((iround == 2 && idrf == 0 && idsf == 0 &&
                 (std::abs(ONE - d.arimap(ar1r)) <= 0.15 || lchks) &&
                 (rmaxr >= PT9 || rmaxs >= 0.88)) ||
                (iround == 2 && idrf == 0 && idsf == 0 &&
                 (std::abs(ONE - d.arimap(ar1r)) <= 0.16 || lchks2) &&
                 (rmaxr >= 0.91 || rmaxs >= 0.89))) {
                if (irm1 == 1 && icon == 1) {
                    irm1 = 0;
                    icon = 0;
                }
                icon = icon + 1;
                if (rmaxr > rmaxs)
                    idrf = idrf + 1;
                else
                    idsf = idsf + 1;
            }
            lchks = ar1s > 0;
            if (lchks) lchks = std::abs(ONE - d.arimap(ar1s)) <= 0.25;
            if (iround >= 2 && idrf == 0 && idsf == 0 &&
                (std::abs(ONE - d.arimap(ar1r)) <= 0.15 || lchks)) {
                if (m.sp == 1) {
                    idrf = idrf + 1;
                } else {
                    rmaxr = d.arimap(ar1r);
                    rmaxs = d.arimap(ar1s);
                    if (rmaxr > rmaxs)
                        idrf = idrf + 1;
                    else
                        idsf = idsf + 1;
                }
            }
            // Increase one at a time -- possible over-differencing.
            if (icon == 2) {
                idrf = idrf - 1;
                idsf = idsf - 1;
                if (iround >= 2 && d.armaer > 0) {
                    if (std::abs(d.arimap(ar1r)) > std::abs(d.arimap(ar1s)))
                        idrf = idrf + 1;
                    else
                        idsf = idsf + 1;
                } else if (rmaxr > rmaxs) {
                    if (rmaxr > -9999.0) idrf = idrf + 1;
                } else if (rmaxs > -9999.0) {
                    idsf = idsf + 1;
                }
            }
        }

        // ---- iround increment, over-differencing caps, exit conditions ----
        bool continue_loop = false;
        if (iround >= 1) {
            iround = iround + 1;
            if (idrf == 3) {
                idrf = 2;
                icon = 0;
            }
            if (idsf == 2) {
                idsf = 1;
                icon = 0;
            }
            Id = idrf + idsf * m.sp;
            bool goto30 = false;
            if (idrf > limrd) {
                idrf = limrd;  // Regular-diff-reset print deferred
                goto30 = true;
            }
            if (!goto30 && idsf > limsd) {
                idsf = limsd;  // Seasonal-diff-reset print deferred
                goto30 = true;
            }
            if (!goto30 && ((icon >= 1 && iround <= 7) || iround == 2))
                continue_loop = true;
        }
        if (continue_loop) continue;  // GO TO 10

        // ---- label 30: results (prints deferred) + mean-significance test ----
        if (imu == 0 && ar.lchkmu) {
            lmu = true;
            if (Id == 0) {
                double wm = totals(trnsrs, 1, d.nspobs, 1, 1);
                double wd = sdev(trnsrs, 1, d.nspobs, 1, 1);
                double tval = std::sqrt(static_cast<double>(d.nspobs)) * wm / wd;
                double vct;
                if (d.nspobs <= 80)
                    vct = 1.96;
                else if (d.nspobs > 80 && d.nspobs <= 200)
                    vct = 2.0;
                else
                    vct = 2.55;
                if (std::abs(tval) < vct) lmu = false;
            } else {
                adrgef(ctx, DNOTST, "Constant", "Constant", PRGTCN, false, false);
                if (ctx.error.lfatal) return;
                int nelta = d.nspobs;
                copy(trnsrs, nelta, 1, txy.data());
                regvar(ctx, txy.data(), nelta, ar.fctdrp, ctx.extend.nfcst, 0,
                       ar.userx.data(), ar.bgusrx.data(), ar.nrusrx,
                       ctx.prior.priadj, ar.reglom, ar.nrxy, ar.begxy.data(),
                       frstry, true, ar.elong);
                if (ctx.error.lfatal) return;
                // Seed the free ARMA coefficients to 0.1.
                int nparma = m.mdl(MA);
                for (int i = 1; i <= nparma; ++i)
                    if (!m.arimaf(i)) d.arimap(i) = 0.1;
                if (m.tol < PTOL) {
                    nltbak = m.nltol;
                    tolbak = m.tol;
                    m.tol = PTOL;
                    m.nltol = PTOL;
                    m.nltol0 = 100.0 * m.tol;
                }
                rgarma(ctx, ar.lestim, ar.mxiter, ar.mxnlit, false, a, na, nefobs,
                       inptok);
                if (!ctx.error.lfatal) {
                    prterr(ctx, nefobs, true);
                    if (!d.convrg)
                        abend(ctx);
                    else if (!inptok)
                        abend(ctx);
                }
                if (ctx.error.lfatal) return;
                if (dpeq(m.tol, PTOL)) {
                    m.tol = tolbak;
                    m.nltol = nltbak;
                    m.nltol0 = m.tol * 100.0;
                }
                int nelt = m.ncxy * (m.ncxy + 1) / 2;
                double rmse;
                if (d.var > 2.0 * dpmpar(1)) {
                    rmse = std::sqrt(d.var);
                    copy(d.chlxpx.data(), nelt, 1, xpxinv.data());
                    dppdi(xpxinv.data(), m.nb, tmp2, 1);
                } else {
                    rmse = 0.0;
                }
                double seb = std::sqrt(xpxinv[0]) * rmse;
                double tval = 0.0;
                if (seb > 0.0) tval = d.b(1) / seb;
                double vct;
                if (d.nspobs <= 80)
                    vct = 1.96;
                else if (d.nspobs > 80 && d.nspobs <= 155)
                    vct = 1.98;
                else if (d.nspobs > 155 && d.nspobs <= 230)
                    vct = 2.1;
                else if (d.nspobs > 230 && d.nspobs <= 350)
                    vct = 2.3;
                else
                    vct = 2.5;
                if (std::abs(tval) < vct) lmu = false;  // significance print deferred
                dlrgef(ctx, 1, ar.nrxy, 1);
                if (ctx.error.lfatal) return;
            }
        }
        break;  // GO TO 20
    }

    idr = idrf;
    ids = idsf;
    ar.mxiter = mxitbk;
}

}  // namespace x13
