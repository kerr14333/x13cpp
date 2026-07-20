// chkmu.cpp -- chkmu.f / genrtt.f: the automatic Constant-term test on the
// default model (Gomez-Maravall). Reuses adrgef/dlrgef/regvar/rgarma and the
// packed-inverse t-stat machinery (dppdi). Error-message prints deferred.
#include "automdl/chkmu.hpp"

#include <cmath>

#include "numeric/numeric.hpp"       // dpmpar, dppdi, dpeq
#include "regarima/estimate.hpp"     // rgarma
#include "regarima/regvar.hpp"       // regvar
#include "automdl/iddiff.hpp"        // prterr
#include "specparse/specparse.hpp"   // strinx, adrgef, dlrgef, copy, abend
#include "gen/model.hpp"             // prm::PB, PRGTCN
#include "gen/notset.hpp"            // prm::DNOTST

namespace x13 {

void genrtt(X13Context& ctx, double* tval) {
    using namespace prm;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    int nb2 = m.nb;
    if (m.iregfx >= 2)
        for (int j = 1; j <= m.nb; ++j)
            if (m.regfx(j)) --nb2;

    double xpxinv[PB * (PB + 1) / 2], tmp[2];
    double rmse = 0.0;
    if (nb2 > 0) {
        if (d.var > 2.0 * dpmpar(1)) {
            int nelt = (nb2 + 1) * (nb2 + 2) / 2;
            rmse = std::sqrt(d.var);
            copy(d.chlxpx.data(), nelt, 1, xpxinv);
            dppdi(xpxinv, nb2, tmp, 1);
        }
    }
    if (dpeq(rmse, 0.0)) return;

    int nfix = 0;
    for (int igrp = 1; igrp <= m.ngrp; ++igrp) {
        int begcol = m.grp(igrp - 1);
        int endcol = m.grp(igrp) - 1;
        for (int icol = begcol; icol <= endcol; ++icol) {
            double seb;
            if (m.regfx(icol)) {
                seb = 0.0;
                ++nfix;
            } else {
                int regidx = icol - nfix;
                seb = std::sqrt(xpxinv[regidx * (regidx + 1) / 2 - 1]) * rmse;
            }
            tval[icol - 1] = (seb > 0.0) ? d.b(icol) / seb : 0.0;
        }
    }
}

void chkmu(X13Context& ctx, double* trnsrs, double* a, int& nefobs, int& na,
           int& frstry, int kstep, bool lprt) {
    using namespace prm;
    (void)lprt;  // NOTE/print output deferred
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& ar = ctx.arima;

    // Add a Constant regressor if the model does not already carry a mean.
    int kmu = strinx(false, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                     "Constant");
    if (kmu == 0) {
        adrgef(ctx, DNOTST, "Constant", "Constant", PRGTCN, false, false);
        if (ctx.error.lfatal) return;
        kmu = strinx(false, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                     "Constant");
    }

    regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
           ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
           ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
    if (ctx.error.lfatal) return;

    bool argok = true;
    rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
    if (!argok) {
        // Estimation-error message deferred.
        prterr(ctx, nefobs, false);
        if (ctx.error.lfatal) return;
        abend(ctx);
    }
    if (ctx.error.lfatal) return;

    if (d.convrg) {
        double tval[PB];
        genrtt(ctx, tval);
        double cval = (kstep == 0) ? 1.96 : 1.6;
        int icol = m.grp(kmu) - 1;   // last column of group kmu == the constant
        if (std::abs(tval[icol - 1]) < cval) kmu = -1;
    } else {
        // Non-convergence NOTE deferred; drop the constant.
        kmu = -1;
    }

    // Remove the constant when not significant and rebuild the matrix.
    if (kmu < 0) {
        int igrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                          "Constant");
        int begcol = m.grp(igrp - 1);
        dlrgef(ctx, begcol, ar.nrxy, 1);
        if (ctx.error.lfatal) return;
        regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
               ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
               ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
        if (ctx.error.lfatal) return;
    }
}

}  // namespace x13
