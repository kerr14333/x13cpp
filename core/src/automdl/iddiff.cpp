// iddiff.cpp -- iddiff.f (unit-root / differencing-order identification) and the
// non-print core of prterr.f. Control flow follows the Fortran DO WHILE with the
// GO TO 10 (continue) / GO TO 20 (exit) / GO TO 30 (finish) targets modeled by a
// continue-flag and a break. Farray state stays 1-based; raw scratch (txy, a) is
// 0-based. All WRITE/Prttab table output is deferred (see the print-engine
// milestone); iddiff's decisions are the load-bearing effect.
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

namespace x13 {

void prterr(X13Context& ctx, int nefobs, bool lauto) {
    (void)nefobs;
    (void)lauto;
    // Error-message printing deferred; only the unknown-cause branch has a
    // non-print effect -- zero the fit so the caller's !Convrg check triggers.
    int ae = ctx.mdldat.armaer;
    if (ae == prm::PUNKER || ae < 0) {
        ctx.mdldat.convrg = false;
        ctx.mdldat.var = 0.0;
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
