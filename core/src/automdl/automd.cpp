// automd.cpp -- reduced automd.f driver (identification spine only). See
// automd.hpp for scope and the deferred features. Sequence mirrors automd.f:
// default airline model -> chkmu (mean test) -> iddiff (differencing) -> amdid
// (ARMA orders) -> re-add the mean when significant -> final estimate.
#include "automdl/automd.hpp"

#include "automdl/amdid.hpp"         // amdid
#include "automdl/chkmu.hpp"         // chkmu
#include "automdl/iddiff.hpp"        // iddiff, prterr
#include "automdl/mdlset.hpp"        // mdlint, mdlset
#include "regarima/estimate.hpp"     // rgarma
#include "regarima/regvar.hpp"       // regvar
#include "specparse/specparse.hpp"   // strinx, adrgef, abend
#include "gen/model.hpp"             // prm::PRGTCN
#include "gen/notset.hpp"            // prm::DNOTST

namespace x13 {

void automd(X13Context& ctx, double* trnsrs, int& frstry, int& nefobs,
            double* a, int& na) {
    using namespace prm;
    auto& m = ctx.model;
    auto& ar = ctx.arima;

    bool inptok = true;
    bool lmu = false;
    // imu: index of a USER-specified Constant (computed before chkmu adds one).
    int imu = 0;
    if (m.nb > 0)
        imu = strinx(false, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                     "Constant");
    if (imu > 0) lmu = true;

    // ---- default "airline" model (0 1 1)(0 1 1); no seasonal part for Sp==1
    // or seasonal-effect regressors. ----
    int lds0 = 1, lqs0 = 1;
    if (m.lseff || m.sp == 1) {
        lds0 = 0;
        lqs0 = 0;
    }
    mdlint(ctx);
    mdlset(ctx, 0, 1, 1, 0, lds0, lqs0, inptok);
    if (ctx.error.lfatal) return;
    regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
           ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
           ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
    if (ctx.error.lfatal) return;

    // ---- mean test on the default model (adds/keeps a Constant iff significant)
    if (ar.lchkmu) {
        chkmu(ctx, trnsrs, a, nefobs, na, frstry, /*kstep=*/0, /*lprt=*/false);
        if (ctx.error.lfatal) return;
        int kmu = strinx(false, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                         "Constant");
        lmu = kmu > 0;
    }

    // ---- estimate the default model ----
    bool argok = true;
    rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
    if (!ctx.error.lfatal) {
        prterr(ctx, nefobs, true);
        if (!ctx.mdldat.convrg)
            abend(ctx);
        else if (!argok)
            abend(ctx);
    }
    if (ctx.error.lfatal) return;

    // ---- identify differencing (iddiff) starting from the maxdiff limits ----
    int ldr = ar.diffam(1), lds = ar.diffam(2);
    iddiff(ctx, ldr, lds, trnsrs, nefobs, frstry, a, na, imu, lmu, false, 0);
    if (ctx.error.lfatal) return;

    // ---- identify ARMA orders (amdid re-fits the winner in place) ----
    int lpr = 0, lqr = 0, lps = 0, lqs = 0;
    bool locok = true;
    amdid(ctx, lpr, ldr, lqr, lps, lds, lqs, trnsrs, frstry, nefobs, a, na, lmu,
          0, locok);
    if (ctx.error.lfatal) return;

    // ---- re-add the mean when iddiff/amdid selected one but it is not present
    // (automd.f:479) and re-estimate the final model. ----
    if (imu == 0 && lmu) {
        int icol = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                          "Constant");
        if (icol == 0) {
            adrgef(ctx, DNOTST, "Constant", "Constant", PRGTCN, false, false);
            if (ctx.error.lfatal) return;
            regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
                   ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
                   ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
            if (ctx.error.lfatal) return;
            rgarma(ctx, ar.lestim, ar.mxiter, ar.mxnlit, false, a, na, nefobs,
                   argok);
            if (!ctx.error.lfatal) {
                prterr(ctx, nefobs, true);
                if (!ctx.mdldat.convrg)
                    abend(ctx);
                else if (!argok)
                    abend(ctx);
            }
            if (ctx.error.lfatal) return;
        }
    }
    // Model-adequacy retry (tstmd1/tstmd2/tstodf) deferred; the identified model
    // is left estimated in ctx.
}

}  // namespace x13
