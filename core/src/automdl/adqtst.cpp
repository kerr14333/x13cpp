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

}  // namespace x13
