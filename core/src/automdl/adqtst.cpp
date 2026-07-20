// adqtst.cpp -- mdlchk.f: the shared Ljung-Box / residual-mean statistic for the
// automatic model-selection adequacy tests. Reuses the ported acf (which fills
// ctx.autoq) and the current model's operator/fix state. Print branches (the
// too-few-obs error) are deferred; the vendored source substitutes bldf=nefobs/2
// there rather than aborting.
#include "automdl/adqtst.hpp"

#include <cmath>
#include <vector>

#include "automdl/amdest.hpp"     // acf, cnvmdl
#include "automdl/idmodel.hpp"    // chkurt
#include "automdl/mdlset.hpp"     // mdlint, mdlset
#include "automdl/chkmu.hpp"      // chkmu
#include "automdl/iddiff.hpp"     // prterr
#include "regarima/estimate.hpp"  // armats, rgarma
#include "regarima/regvar.hpp"    // regvar
#include "specparse/specparse.hpp"// strinx, adrgef, abend
#include "gen/srslen.hpp"         // prm::PLEN
#include "gen/model.hpp"          // prm::PARIMA, PRGTCN
#include "gen/notset.hpp"         // prm::DNOTST

namespace x13 {

void mdlchk(X13Context& ctx, const double* a, int na, int nefobs, double& blpct,
            double& blq, int& bldf, double& rvr, double& rtval) {
    constexpr int PR = prm::PLEN / 4;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    // Ljung-Box lag count from the seasonal period.
    if (m.sp == 12)
        bldf = 24;
    else if (m.sp == 1)
        bldf = 8;
    else {
        bldf = 4 * m.sp;
        if (m.sp == 4 && nefobs <= 22 && nefobs >= 18) bldf = 6;
    }
    // Too few observations to compute the LB statistic: use nefobs/2 (the error
    // print/abort is commented out in the vendored source).
    if (bldf >= nefobs) bldf = nefobs / 2;

    int i1 = na - nefobs + 1;
    int np = 0;
    int endlag = m.opr(m.nopr) - 1;
    for (int ilag = 1; ilag <= endlag; ++ilag)
        if (!m.arimaf(ilag)) ++np;

    std::vector<double> smpac(PR, 0.0), seacf(PR, 0.0);
    acf(ctx, a + i1 - 1, nefobs, nefobs, smpac.data(), seacf.data(), bldf, np,
        m.sp, 0, true, false);
    blpct = 1.0 - ctx.autoq.qpv(bldf);
    blq = ctx.autoq.qs(bldf);

    // Residual-mean t-value.
    double rm = 0.0, rv = 0.0;
    for (int i = i1; i <= na; ++i) {
        rm += a[i - 1];
        rv += a[i - 1] * a[i - 1];
    }
    double an = static_cast<double>(na - i1 + 1);
    rm = rm / an;
    rv = rv / an - rm * rm;
    double rstd = std::sqrt(rv / an);
    rtval = rm / rstd;

    rvr = std::sqrt(d.var);
}

void tstmd2(X13Context& ctx, int& nnsig, int nz, int& ipr, int& iqr, int& ips,
            int& iqs) {
    using namespace prm;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    nnsig = 0;
    double cval = ctx.arima.tsig;
    double cmin = (nz <= 150) ? 0.15 : 0.1;
    int ardsp = m.nnsedf + m.nseadf;

    int idr, ids, id, ip, iq, iprs, iqrs, n;
    cnvmdl(ctx, ipr, ips, idr, ids, iqr, iqs, id, ip, iq, iprs, iqrs, n);
    if (ctx.error.lfatal) return;

    int iurpr, iurps, iurqr, iurqs;
    chkurt(ctx, iurpr, iurps, iurqr, iurqs);

    double tval[PARIMA];
    armats(ctx, tval);
    if (ctx.error.lfatal) return;

    int icpr = 0, icps = 0, icqr = 0, icqs = 0;
    double bmin = -DNOTST;  // 999
    if (iurpr == 0 && ipr > icpr) {
        int i = ipr - icpr;
        double cv = std::abs(tval[i - 1]);
        if (cv < cval && std::abs(d.arimap(i + ardsp)) < cmin) {
            ++icpr;
            if (bmin > cv) bmin = cv;
        }
    }
    if (ips > icps && iurps == 0) {
        int i = iprs - icps;
        double cv = std::abs(tval[i - 1]);
        if (cv < cval && std::abs(d.arimap(i + ardsp)) < cmin) {
            ++icps;
            if (bmin > cv) {
                bmin = cv;
                icpr = 0;
            }
        }
    }
    if (iqr > icqr && iurqr == 0) {
        int i = iprs + iqr - icqr;
        double cv = std::abs(tval[i - 1]);
        if (cv < cval && std::abs(d.arimap(i + ardsp)) < cmin) {
            ++icqr;
            if (bmin > cv) {
                bmin = cv;
                icpr = 0;
                icps = 0;
            }
        }
    }
    if (iqs > icqs && iurqs == 0) {
        int i = iprs + iqrs - icqs;
        double cv = std::abs(tval[i - 1]);
        if (cv < cval && std::abs(d.arimap(i + ardsp)) < cmin) {
            ++icqs;
            if (bmin > cv) {
                bmin = cv;
                icpr = 0;
                icps = 0;
                icqr = 0;
            }
        }
    }
    (void)bmin;
    nnsig = nnsig + icpr + icps + icqr + icqs;
    if ((iprs + iqrs) == 1 || (iurpr + iurps + iurqr + iurqs) > 0) nnsig = 0;

    if (nnsig >= 1) {
        if (icpr >= 1) {
            while (true) {
                for (int i = ipr; i <= n - 1; ++i)
                    d.arimap(i + ardsp) = d.arimap(i + 1 + ardsp);
                --ipr;
                --icpr;
                if (icpr <= 0) break;
            }
        } else if (icps >= 1) {
            while (true) {
                for (int i = iprs; i <= n - 1; ++i)
                    d.arimap(i + ardsp) = d.arimap(i + 1 + ardsp);
                --ips;
                --icps;
                if (icps <= 0) break;
            }
        } else if (icqr >= 1) {
            while (true) {
                for (int i = iprs + iqr; i <= n - 1; ++i)
                    d.arimap(i + ardsp) = d.arimap(i + 1 + ardsp);
                --iqr;
                --icqr;
                if (icqr <= 0) break;
            }
        } else {
            while (true) {
                for (int i = iprs + iqrs; i <= n - 1; ++i)
                    d.arimap(i + ardsp) = d.arimap(i + 1 + ardsp);
                --iqs;
                --icqs;
                if (icqs <= 0) break;
            }
        }
        bool inptok = true;
        mdlint(ctx);
        mdlset(ctx, ipr, idr, iqr, ips, ids, iqs, inptok);
    }
}

void testodf(X13Context& ctx, double* trnsrs, int& frstry, int& nefobs,
             double* a, int& na, int& lpr, int& ldr, int& lqr, int& lps,
             int& lds, int& lqs, int kstep, bool& redomd, bool& argok) {
    using namespace prm;
    constexpr double MALIM = 0.001;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& ar = ctx.arima;

    redomd = false;
    bool reredomd = false;

    // ---- nonseasonal over-differencing: sum of regular MA coeffs ~ 1 ----
    if (ldr > 0 && lqr > 0) {
        int disp = lpr + ldr + lps + lds;
        double summa = 0.0;
        for (int i = disp + 1; i <= disp + lqr; ++i) summa += d.arimap(i);
        if (std::abs(summa - 1.0) < MALIM) {
            redomd = true;
            ldr = ldr - 1;
            lqr = lqr - 1;
            int icol = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                              m.ngrptl, "Constant");
            if (icol == 0 && ar.lchkmu) {
                // Reduced differencing needs a mean; add a Constant (the non-
                // Lchkmu NOTE-to-add-a-constant path is a deferred print).
                adrgef(ctx, DNOTST, "Constant", "Constant", PRGTCN, false, false);
                if (ctx.error.lfatal) return;
            }
        }
    }

    // ---- seasonal over-differencing (Lsovdf; adds seasonal regressors via
    // sftest) -- DEFERRED: Lsovdf is off by default and sftest is not ported.
    if (lds > 0 && lqs > 0 && ar.lsovdf) {
        // Not reachable with the default Lsovdf=false; port sftest + the
        // seasonal-regressor construction when Lsovdf support lands.
    }

    if (redomd) {
        bool inptok = true;
        mdlint(ctx);
        mdlset(ctx, lpr, ldr, lqr, lps, lds, lqs, inptok);
        if (!ctx.error.lfatal)
            regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
                   ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
                   ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
        if (ctx.error.lfatal) return;
        // Automatic-outlier removal (Natotl>0 -> clrotl) deferred with the
        // outlier-in-automd path.
        rgarma(ctx, ar.lestim, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
        if (!ctx.error.lfatal) {
            prterr(ctx, nefobs, true);
            if (!d.convrg)
                abend(ctx);
            else if (!argok)
                abend(ctx);
        }
        if (ctx.error.lfatal) return;
        // Redo automatic outlier ID (Lidotl) deferred.

        // Recheck the added mean; if the constant fell out, re-estimate.
        if (ar.lchkmu) {
            chkmu(ctx, trnsrs, a, nefobs, na, frstry, kstep, false);
            if (ctx.error.lfatal) return;
            int icol1 = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                               m.ngrptl, "Constant");
            if (icol1 == 0) reredomd = true;
        }
        // Lsovdf seasonal-regressor significance (sftest) deferred.

        if (reredomd) {
            rgarma(ctx, ar.lestim, ar.mxiter, ar.mxnlit, false, a, na, nefobs,
                   argok);
            if (!ctx.error.lfatal) {
                prterr(ctx, nefobs, true);
                if (!d.convrg)
                    abend(ctx);
                else if (!argok)
                    abend(ctx);
            }
            if (ctx.error.lfatal) return;
        }
    }
}

void bkdfmd(X13Context& ctx, bool backup) {
    using namespace prm;
    auto& s = ctx.ss2rv;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& ar = ctx.arima;
    constexpr int PACM = (PLEN + 2 * PORDER) * PARIMA;  // armacm extent

    if (backup) {
        s.pri2rv = ctx.prior.priadj;
        s.ngr2rv = m.ngrp;
        s.ngrt2r = m.ngrptl;
        s.ncxy2r = m.ncxy;
        s.nbbrv = m.nb;
        s.nct2rv = m.ncoltl;
        s.cttlrv = m.colttl.raw();
        s.gttlrv = m.grpttl.raw();
        cpyint(m.colptr.data(), PB + 1, 1, s.clptrv.data());
        cpyint(m.grp.data(), PGRP + 1, 1, s.g2rv.data());
        cpyint(m.grpptr.data(), PGRP + 1, 1, s.gptrrv.data());
        cpyint(m.rgvrtp.data(), PB, 1, s.rgv2rv.data());
        copy(d.arimap.data(), PARIMA, 1, s.ap2rv.data());
        copy(d.b.data(), PB, 1, s.bbrv.data());
        copylg(m.arimaf.data(), PARIMA, 1, s.fxarv.data());
        s.nr2rv = ar.nrxy;
        s.irfx2r = m.iregfx;
        copylg(m.regfx.data(), PB, 1, s.rgfx2r.data());
        s.lma2r = m.lma;
        s.lar2r = m.lar;
        s.nint2r = m.nintvl;
        s.next2r = m.nextvl;
        s.mxdf2r = m.mxdflg;
        s.mxar2r = m.mxarlg;
        s.mxma2r = m.mxmalg;
        s.v2r = d.var;
        copy(d.chlxpx.data(), PXPX, 1, s.chx2r.data());
        copy(d.chlgpg.data(), PGPG, 1, s.chg2r.data());
        copy(d.armacm.data(), PACM, 1, s.acm2r.data());
        s.dtcv2r = d.lndtcv;
        // Deferred (no-op during no-holiday/no-outlier model-ID): the
        // holiday/TD/outlier-adjustment + picktd + user-regressor fields.
    } else {
        ctx.prior.priadj = s.pri2rv;
        m.ngrp = s.ngr2rv;
        m.ngrptl = s.ngrt2r;
        m.ncxy = s.ncxy2r;
        m.nb = s.nbbrv;
        m.ncoltl = s.nct2rv;
        m.colttl = s.cttlrv.raw();
        m.grpttl = s.gttlrv.raw();
        cpyint(s.clptrv.data(), PB + 1, 1, m.colptr.data());
        cpyint(s.g2rv.data(), PGRP + 1, 1, m.grp.data());
        cpyint(s.gptrrv.data(), PGRP + 1, 1, m.grpptr.data());
        cpyint(s.rgv2rv.data(), PB, 1, m.rgvrtp.data());
        copy(s.ap2rv.data(), PARIMA, 1, d.arimap.data());
        copy(s.bbrv.data(), PB, 1, d.b.data());
        copylg(s.fxarv.data(), PARIMA, 1, m.arimaf.data());
        ar.nrxy = s.nr2rv;
        m.iregfx = s.irfx2r;
        copylg(s.rgfx2r.data(), PB, 1, m.regfx.data());
        m.lma = s.lma2r;
        m.lar = s.lar2r;
        m.nintvl = s.nint2r;
        m.nextvl = s.next2r;
        m.mxdflg = s.mxdf2r;
        m.mxarlg = s.mxar2r;
        m.mxmalg = s.mxma2r;
        d.var = s.v2r;
        copy(s.chx2r.data(), PXPX, 1, d.chlxpx.data());
        copy(s.chg2r.data(), PGPG, 1, d.chlgpg.data());
        copy(s.acm2r.data(), PACM, 1, d.armacm.data());
        d.lndtcv = s.dtcv2r;
    }
}

void tstmd1(X13Context& ctx, double* trnsrs, int& frstry, double* a, int& na,
            int& nefobs, double pdfm, double rsddfm, double rtval, int& lpr,
            int& lps, int& lqr, int& lqs, int& ldr, int& lds, bool& lmu,
            const double* adj0, const double* trns0, const double* tair) {
    using namespace prm;
    (void)adj0;
    (void)trns0;  // used only by the deferred picktd/prior-restore branch
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& ar = ctx.arima;

    int ipr, ips, idr, ids, iqr, iqs, id, ip, iq, iprs, iqrs, n;
    cnvmdl(ctx, ipr, ips, idr, ids, iqr, iqs, id, ip, iq, iprs, iqrs, n);
    if (ctx.error.lfatal) return;

    // Default-model significance / already-airline short-circuits.
    int i1dfm = 1, i2dfm = (m.sp > 1) ? 1 : 0;
    if (std::abs(tair[0]) < 1.96) i1dfm = 0;
    if (m.sp > 1 && std::abs(tair[1]) < 1.96) i2dfm = 0;
    if ((idr == 1 && ids == 1 && m.sp > 1 && iqr == 1 && iqs == 1 && ipr == 0 &&
         ips == 0) ||
        (idr == 1 && m.sp == 1 && iqr == 1 && ipr == 0) || (i1dfm + i2dfm == 0))
        return;

    bool inptok = true;
    int iround = 1;
    double tval[PARIMA];
    while (true) {
        rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, inptok);
        if (ctx.error.lfatal) return;
        // (ssprep on iround>1 deferred -- rgarma self-preps.)
        armats(ctx, tval);
        if (ctx.error.lfatal) return;

        // Drop the least-significant ARMA lags (cval=1.8), one operator per pass.
        double cval = 1.8;
        int dipr = 0, dips = 0, diqr = 0, diqs = 0;
        while (true) {
            int ilag = 0;
            if (ipr > dipr && std::abs(tval[ipr - dipr - 1]) < cval) {
                ++dipr;
                ++ilag;
            }
            if (ips > dips && std::abs(tval[iprs - dips - 1]) < cval) {
                ++dips;
                ++ilag;
            }
            if (iqr > diqr && std::abs(tval[iprs + iqr - diqr - 1]) < cval) {
                ++diqr;
                ++ilag;
            }
            // NB: guard is `ips>dips` in the vendored source (bug-for-bug).
            if (ips > dips && std::abs(tval[iprs + iqrs - diqs - 1]) < cval) {
                ++diqs;
                ++ilag;
            }
            if (ilag < 1) break;
        }
        int iopr = dipr + dips + diqr + diqs;
        if (iopr == 1 && iprs + iqrs > 0 && iround == 1) {
            iround = iround + 1;
            if (dipr == 1)
                ipr = ipr - 1;
            else if (dips == 1)
                ips = ips - 1;
            else if (diqr == 1)
                iqr = iqr - 1;
            else
                iqs = iqs - 1;
            inptok = true;
            mdlint(ctx);
            mdlset(ctx, ipr, idr, iqr, ips, ids, iqs, inptok);
            if (!ctx.error.lfatal && inptok) continue;  // GO TO 10
            return;
        }
        break;
    }

    if (iround > 1) {
        mkmdsn(ctx, ipr, idr, iqr, ips, ids, iqs);
        if (ctx.error.lfatal) return;
        lpr = ipr;
        lps = ips;
        lqr = iqr;
        lqs = iqs;
        ldr = idr;
        lds = ids;
    }

    // Compare the identified model's residual adequacy to the default's.
    double pami, blq, rsdami, rtv;
    int bldf;
    mdlchk(ctx, a, na, nefobs, pami, blq, bldf, rsdami, rtv);
    const double fct2 = 1.0, fct0 = 1.025;
    int ichk = 0;
    if (pami < 0.95 && pdfm < 0.75 && rsddfm < rsdami)
        ichk = 1;
    else if (pami < 0.95 && pdfm < 0.75 && pdfm < pami && rsddfm < fct0 * rsdami)
        ichk = 2;
    else if (pami >= 0.95 && pdfm < 0.95 && rsddfm < fct2 * rsdami)
        ichk = 3;
    else if (idr == 0 && ids == 1 && ipr == 1 && d.arimap(2) >= 0.82 &&
             ips == 0 && iqr <= 1 && iqs == 1)
        ichk = 4;
    else if (idr == 1 && ids == 0 && ipr == 0 && d.arimap(2) >= 0.65 &&
             ips == 1 && iqr == 1 && iqs <= 1)
        ichk = 5;

    if (ichk > 0) {
        // Revert to the default airline model (0 1 1)(0 1 1); (0 1 1) for Sp==1.
        idr = 1;
        ids = 1;
        ipr = 0;
        ips = 0;
        iqr = 1;
        iqs = 1;
        if (m.sp == 1) {
            ids = 0;
            iqs = 0;
        }
        lpr = ipr;
        lps = ips;
        lqr = iqr;
        lqs = iqs;
        ldr = idr;
        lds = ids;
        inptok = true;
        mdlint(ctx);
        mdlset(ctx, ipr, idr, iqr, ips, ids, iqs, inptok);
        if (ctx.error.lfatal) return;
        // (picktd/prior-restore branch deferred: Pcktd0==Picktd here.)
        // (Aicind = Aici0 AIC-bookkeeping deferred.)
        bkdfmd(ctx, false);  // restore the backed-up default-model estimate
        regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
               ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
               ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, false);
        if (ctx.error.lfatal) return;
        mkmdsn(ctx, ipr, idr, iqr, ips, ids, iqs);
        if (ctx.error.lfatal) return;
        // Add the mean if it is significant and not already present.
        if (!lmu && rtval > 1.96 && ar.lchkmu) {
            lmu = true;
            adrgef(ctx, DNOTST, "Constant", "Constant", PRGTCN, false, false);
            if (!ctx.error.lfatal)
                regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst,
                       0, ar.userx.data(), ar.bgusrx.data(), ar.nrusrx,
                       ctx.prior.priadj, ar.reglom, ar.nrxy, ar.begxy.data(),
                       frstry, true, false);
            if (!ctx.error.lfatal)
                rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs,
                       inptok);
            if (!ctx.error.lfatal) prterr(ctx, nefobs, true);
            if (ctx.error.lfatal) return;
        }
    }
}

}  // namespace x13
