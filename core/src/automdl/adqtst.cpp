// adqtst.cpp -- mdlchk.f: the shared Ljung-Box / residual-mean statistic for the
// automatic model-selection adequacy tests. Reuses the ported acf (which fills
// ctx.autoq) and the current model's operator/fix state. Print branches (the
// too-few-obs error) are deferred; the vendored source substitutes bldf=nefobs/2
// there rather than aborting.
#include "automdl/adqtst.hpp"

#include <cmath>
#include <vector>

#include "automdl/amdest.hpp"   // acf
#include "gen/srslen.hpp"       // prm::PLEN

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

}  // namespace x13
