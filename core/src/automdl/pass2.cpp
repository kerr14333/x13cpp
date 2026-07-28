// pass2.cpp -- pass2.f. See pass2.hpp for what it is and when the oracle runs
// it (answer: on every automdl spec, not only when an outlier scan found
// something).
#include "automdl/pass2.hpp"

#include "automdl/adqtst.hpp"     // bkdfmd
#include "automdl/automd_finalize.hpp"  // clrotl
#include "automdl/iddiff.hpp"     // prterr
#include "automdl/mdlset.hpp"     // mdlint, mdlset, mkmdsn
#include "regarima/estimate.hpp"  // rgarma
#include "regarima/regvar.hpp"    // regvar
#include "specparse/specparse.hpp"  // abend, copy
#include "numeric/numeric.hpp"    // dpeq
#include "gen/model.hpp"          // prm::AO/LS/TC
#include "gen/srslen.hpp"         // prm::PLEN

#include <cmath>

namespace x13 {

void pass2(X13Context& ctx, double* trnsrs, int& frstry, int& ipr, int& idr,
           int& iqr, int& ips, int& ids, int& iqs, int& lpr, int& ldr,
           int& lqr, int& lps, int& lds, int& lqs, int& naut, int& naut0,
           double& plbox, double& plbox0, int& bldf, int bldf0, double& rvr,
           double& rvr0, bool& lmu, bool& lmu0, double* a, const double* a0,
           int& na, int na0, int aici0, bool pcktd0, int aicit0,
           const double* adj0, const double* trns0, double fct2, bool& ismd0,
           double* cvl0, int& nefobs, int& nloop, int& nround, int& igo) {
    using namespace prm;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& ar = ctx.arima;

    constexpr double TWO = 2.0;
    constexpr double PI = 3.14159265358979;
    constexpr double TWOPT8 = 2.8;

    // pass2.f:43-44. Arimap is laid out with the differencing operators first,
    // so ar1p is the slot of the FIRST nonseasonal AR coefficient -- which the
    // ichk=5/6 tests read directly.
    const int ardsp = m.nnsedf + m.nseadf;
    const int ar1p = ardsp + 1;

    igo = 0;
    int ichk = 0;
    bool lcv = false;
    bool inptok = true;

    // ---- part 1: is the DEFAULT model good enough to take back? ----
    if (naut0 <= naut &&
        (idr != ldr || ids != lds || ipr != lpr || ips != lps || iqr != lqr ||
         iqs != lqs || (lmu != lmu0))) {
        if (plbox < 0.95 && plbox0 < 0.75 && rvr0 < rvr) {
            ichk = 1;
        } else if (nloop == 1 && plbox >= 0.95 && plbox0 < 0.95) {
            ichk = 2;
        } else if (plbox < 0.95 && plbox0 < 0.75 && plbox0 < plbox &&
                   rvr0 < ar.fct * rvr) {
            ichk = 3;
        } else if (plbox >= 0.95 && plbox0 < 0.95 && rvr0 < fct2 * rvr) {
            ichk = 4;
        } else if (idr == 0 && ids == 1 && ipr == 1 &&
                   d.arimap(ar1p) >= 0.82 && ips == 0 && iqr <= 1 &&
                   iqs == 1) {
            ichk = 5;
        } else if (idr == 1 && ids == 0 && ipr == 0 &&
                   d.arimap(ar1p) >= 0.65 && ips == 1 && iqr == 1 &&
                   iqs <= 1) {
            ichk = 6;
        }

        if (ichk > 0) {
            mdlint(ctx);
            mdlset(ctx, lpr, ldr, lqr, lps, lds, lqs, inptok);
            if (ctx.error.lfatal) return;
            nefobs = ctx.mdldat.nspobs - m.nintvl;
            ctx.series.dnefob = static_cast<double>(nefobs);
            d.lnlkhd =
                -(d.lndtcv + ctx.series.dnefob *
                                 (std::log(TWO * PI * d.var) + 1.0)) / TWO;

            // pass2.f:97-113. Reverting the model also reverts the automatic
            // trading-day CHOICE, so when that choice differs from the
            // default's the prior-adjustment factors and the transformed
            // series have to go back with it. Unreachable on the plain
            // automdl path (Picktd is only ever set by the aictest td branch,
            // so pcktd0 and Picktd are both false and the test is false);
            // transcribed for the aictest path.
            if ((pcktd0 && !ctx.picktd.picktd) ||
                (!pcktd0 && ctx.picktd.picktd)) {
                copy(adj0, PLEN, 1, ctx.adj.adj.data());
                copy(trns0, PLEN, 1, trnsrs);
                copy(ctx.adj.adj.data(), ctx.adj.nadj, -1,
                     &ctx.inpt.sprior(ctx.adj.setpri));
                if (!(ar.fcntyp == 4 || dpeq(ar.lam, 1.0))) {
                    if (pcktd0) {
                        if (ctx.prior.kfmt == 0) ctx.prior.kfmt = 1;
                        if (!ctx.prior.lpradj) ctx.prior.lpradj = true;
                    } else if (ctx.priusr.nustad == 0 &&
                               ctx.priusr.nuspad == 0) {
                        ctx.prior.kfmt = 0;
                        if (ctx.prior.lpradj) ctx.prior.lpradj = false;
                    }
                }
            }

            // bkdfmd(false) restores the DEFAULT model's fitted state from the
            // backup automd.f:379 took at label 10 -- which is why label 10
            // has to run on every path, not just the aictest one.
            bkdfmd(ctx, false);
            regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst,
                   0, ar.userx.data(), ar.bgusrx.data(), ar.nrusrx,
                   ctx.prior.priadj, ar.reglom, ar.nrxy, ar.begxy.data(),
                   frstry, true, ar.elong);
            // NB the oracle passes the COMMON Lautom here as rgarma's argok
            // out-parameter, not a local -- so a non-converged fit on this
            // path silently clears the "automatic modelling" flag. Verbatim.
            if (!ctx.error.lfatal)
                rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs,
                       ar.lautom);
            if (!ctx.error.lfatal) prterr(ctx, nefobs, true);
            if (ctx.error.lfatal) return;
            // (the "Model changed to" print, pass2.f:121-131, is print surface)

            ismd0 = true;
            plbox = plbox0;
            rvr = rvr0;
            ipr = lpr;
            idr = ldr;
            iqr = lqr;
            ips = lps;
            ids = lds;
            iqs = lqs;
            lmu = lmu0;
            bldf = bldf0;
            ar.aicind = aici0;
            ar.aicint = aicit0;
            mkmdsn(ctx, ipr, idr, iqr, ips, ids, iqs);
            if (ctx.error.lfatal) return;
            na = na0;
            copy(a0, na, 1, a);
        }
    }

    plbox0 = plbox;
    rvr0 = rvr;
    naut0 = naut;

    // ---- part 2: raise the acceptance limit, and redo the identification if
    // the surviving model still fails it (pass2.f:160-330) ----
    if (nloop == 1)
        ar.pcr += 0.025;
    else
        ar.pcr += 0.015;

    if (plbox <= ar.pcr) return;

    if (nloop == 1 && !ar.lotmod) {
        lcv = (ar.ltstao && ar.critvl(AO) > TWOPT8) ||
              (ar.ltstls && ar.critvl(LS) > TWOPT8) ||
              (ar.ltsttc && ar.critvl(TC) > TWOPT8);
        if (lcv) {
            // Reduce each outlier critical value by Predcv percent, floored at
            // 2.8, keeping the old value in cvl0 so the tail below can put it
            // back. (The Ltstso arm is commented out in the Fortran.)
            const int types[3] = {AO, LS, TC};
            const bool on[3] = {ar.ltstao, ar.ltstls, ar.ltsttc};
            for (int k = 0; k < 3; ++k) {
                if (!on[k]) continue;
                const int t = types[k];
                cvl0[t - 1] = ar.critvl(t);
                ar.critvl(t) =
                    std::max(TWOPT8, ar.critvl(t) - ar.critvl(t) * ar.predcv);
            }
        }
    }

    ldr = idr;
    lds = ids;
    lpr = ipr;
    lps = ips;
    lqr = iqr;
    lqs = iqs;
    lmu0 = lmu;
    nloop = nloop + 1;
    nround = nround + 1;

    // Fortran precedence: `.and.` binds tighter than `.or.`, so this is
    // (nloop<=2 && !Lotmod) || (nloop==2 && lcv). With the DEFAULT Lotmod both
    // disjuncts are false (lcv can only be set under !Lotmod), so the default
    // path always takes the model-of-last-resort ELSE below.
    if ((nloop <= 2 && !ar.lotmod) || (nloop == 2 && lcv)) {
        if (ismd0) {
            ismd0 = false;
            naut = naut0;
            igo = 2;
        } else if (ar.lautod && ar.lautom) {
            igo = 1;
        } else {
            igo = 3;
        }
        return;
    }

    // ---- the model of last resort (pass2.f:245-330) ----
    ipr = 3;
    if (ids > 0) ips = 0;
    iqr = ar.lmixmd ? 1 : 0;
    if (m.sp > 1) {
        if (ar.lmixmd) {
            if (ar.maxord(2) > 0) iqs = 1;
        } else {
            if (ips == 0 && ar.maxord(2) > 0) iqs = 1;
        }
    }
    mkmdsn(ctx, ipr, idr, iqr, ips, ids, iqs);
    if (ctx.error.lfatal) return;
    mdlint(ctx);
    mdlset(ctx, ipr, idr, iqr, ips, ids, iqs, inptok);
    if (ctx.error.lfatal) return;
    regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
           ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
           ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
    if (!ctx.error.lfatal)
        rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs,
               ar.lautom);
    if (!ctx.error.lfatal && ctx.mdldat.convrg) prterr(ctx, nefobs, true);
    if (ctx.error.lfatal) return;

    // Walk the AR order down until the estimation converges (pass2.f:283-307).
    // Note the loop tests `Ipr.gt.0` but the failure test below is `Ipr.le.0`,
    // so a model that converges only at ipr==0 exits the loop and is KEPT --
    // the abend fires only when ipr reached 0 without converging.
    if (!ctx.mdldat.convrg) {
        while (!ctx.mdldat.convrg && ipr > 0) {
            ipr = ipr - 1;
            mdlint(ctx);
            mdlset(ctx, ipr, idr, iqr, ips, ids, iqs, inptok);
            if (!ctx.error.lfatal)
                regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp,
                       ctx.extend.nfcst, 0, ar.userx.data(), ar.bgusrx.data(),
                       ar.nrusrx, ctx.prior.priadj, ar.reglom, ar.nrxy,
                       ar.begxy.data(), frstry, true, ar.elong);
            if (!ctx.error.lfatal)
                rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs,
                       ar.lautom);
            if (ctx.error.lfatal) return;
        }
        if (ipr <= 0 && !ctx.mdldat.convrg) {
            abend(ctx);
            return;
        }
        if (ipr < 3) {
            mkmdsn(ctx, ipr, idr, iqr, ips, ids, iqs);
            if (ctx.error.lfatal) return;
        }
    }

    if (ar.lotmod || (nloop == 2 && !lcv)) {
        nloop = 3;
    } else {
        if (ar.ltstao) ar.critvl(AO) = cvl0[AO - 1];
        if (ar.ltstls) ar.critvl(LS) = cvl0[LS - 1];
        if (ar.ltsttc) ar.critvl(TC) = cvl0[TC - 1];
        if (m.natotl > 0) {
            clrotl(ctx, ar.nrxy);
            if (!ctx.error.lfatal)
                regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp,
                       ctx.extend.nfcst, 0, ar.userx.data(), ar.bgusrx.data(),
                       ar.nrusrx, ctx.prior.priadj, ar.reglom, ar.nrxy,
                       ar.begxy.data(), frstry, true, ar.elong);
            if (ctx.error.lfatal) return;
        }
    }
    igo = 2;
}

}  // namespace x13
