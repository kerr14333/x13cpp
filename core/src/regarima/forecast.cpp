// forecast.cpp -- fcstxy.f: regARIMA forecasts + forecast standard errors.
// Faithful port; all array indexing kept 1-based (Fortran) with -1 offsets on
// the C++ pointers. Every dependency (armafl, polyml, eltlen, ratpos, resid,
// dppsl, yprmy, copy, setdp) is already oracle-verified.
#include "regarima/forecast.hpp"

#include <cmath>
#include <vector>

#include "regarima/armafl.hpp"      // armafl
#include "regarima/regvar.hpp"      // ratpos
#include "regarima/estimate.hpp"    // resid
#include "numeric/numeric.hpp"      // dppsl, yprmy
#include "specparse/specparse.hpp"  // polyml, eltlen, copy, setdp
#include "gen/model.hpp"            // prm::DIFF, AR, MA, PB, PORDER, PDIFOR, POPR
#include "gen/srslen.hpp"           // prm::PLEN, prm::PFCST

namespace x13 {

void fcstxy(X13Context& ctx, int fctori, int nfcst, double* fcst, double* se,
            double* rgvar) {
    constexpr double ZERO = 0.0, ONE = 1.0, MONE = -1.0;
    constexpr int PCXY = prm::PB + 1;
    constexpr int PAF = (2 * prm::PORDER + prm::PLEN) * PCXY;
    constexpr int PARDOR = prm::PORDER + prm::PDIFOR;

    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    // Filter the [X:y] matrix to residuals a (exact ARMA filter, no re-init of
    // |G'G|, no invertibility check -- linit=F, lckrts=F, as in the oracle).
    std::vector<double> a(PAF);
    int nrsd = 0, info = 0;
    copy(d.xy.data(), fctori * m.ncxy, 1, a.data());
    armafl(ctx, fctori, m.ncxy, /*linit=*/false, /*lckrts=*/false, a.data(),
           nrsd, PAF, info);

    // Full AR*differencing operator (fularp/fularl) via polyml over DIFF..AR.
    std::vector<double> fularp(PARDOR, 0.0), fulmap(prm::PORDER, 0.0),
        piwght(PARDOR + prm::PFCST, 0.0);
    std::vector<int> fularl(PARDOR, 0), fulmal(prm::PORDER, 0);
    int nfular = 0;
    {
        int begopr = m.mdl(prm::DIFF - 1);
        int endopr = m.mdl(prm::AR) - 1;
        for (int iopr = begopr; iopr <= endopr; ++iopr) {
            int beglag = m.opr(iopr - 1);
            int nlag = 0;
            eltlen(ctx, iopr, m.opr.data(), m.nopr, nlag);
            if (ctx.error.lfatal) return;
            polyml(d.arimap.data() + beglag - 1, m.arimal.data() + beglag - 1,
                   nlag, fularp.data(), fularl.data(), nfular, PARDOR,
                   fularp.data(), fularl.data(), nfular);
        }
    }
    int mxdfar = m.mxdflg + m.mxarlg;

    // Full MA operator (fulmap/fulmal) via polyml over the MA operators.
    int nfulma = 0;
    {
        int begopr = m.mdl(prm::MA - 1);
        int endopr = m.mdl(prm::MA) - 1;
        for (int iopr = begopr; iopr <= endopr; ++iopr) {
            int beglag = m.opr(iopr - 1);
            int nlag = 0;
            eltlen(ctx, iopr, m.opr.data(), m.nopr, nlag);
            if (ctx.error.lfatal) return;
            polyml(d.arimap.data() + beglag - 1, m.arimal.data() + beglag - 1,
                   nlag, fulmap.data(), fulmal.data(), nfulma, prm::PORDER,
                   fulmap.data(), fulmal.data(), nfulma);
        }
    }

    // psi(B) weights for the standard errors: psi(B) = MA(B)/(AR*diff)(B).
    setdp(ZERO, PARDOR + prm::PFCST, piwght.data());
    piwght[0] = ONE;
    for (int i = 1; i <= nfulma; ++i) piwght[fulmal[i - 1]] = -fulmap[i - 1];
    // aropr indexes the single expanded AR*diff operator for ratpos (the +1 is
    // because the pointers address the first element of the *next* operator).
    int aropr[prm::POPR + 1] = {0};
    aropr[0] = 1;
    aropr[1] = nfular + 1;
    ratpos(m.mxmalg + 1, fularp.data(), fularl.data(), aropr, 1, 1, nfcst,
           piwght.data());

    // Scale the operator lags by Ncxy so a column-major matrix filters like a
    // vector.
    for (int ilag = 1; ilag <= nfular; ++ilag) fularl[ilag - 1] *= m.ncxy;
    for (int ilag = 1; ilag <= nfulma; ++ilag) fulmal[ilag - 1] *= m.ncxy;

    int ndltar = mxdfar * m.ncxy;
    int ndltma = nrsd * m.ncxy;
    int neltxy = fctori * m.ncxy;
    int neltf = nfcst * m.ncxy;

    // tfcst: last mxdfar rows of Xy, followed by zeroed forecast slots (only the
    // y column of each forecast row is zeroed; the X columns carry the design).
    std::vector<double> tfcst(static_cast<std::size_t>((prm::PFCST + PARDOR) * PCXY),
                              0.0);
    copy(d.xy.data() + (neltxy - ndltar), ndltar + neltf, 1, tfcst.data());
    for (int ielt = ndltar + m.ncxy; ielt <= ndltar + neltf; ielt += m.ncxy)
        tfcst[ielt - 1] = ZERO;

    // Forecast recursion.
    for (int ielt = 1; ielt <= neltf; ++ielt) {
        int ishft1 = ielt + ndltar;
        double tmp = ZERO;
        for (int j = 1; j <= nfular; ++j)
            tmp += fularp[j - 1] * tfcst[ishft1 - fularl[j - 1] - 1];
        int ishft2 = ielt + ndltma;
        for (int j = 1; j <= nfulma; ++j) {
            int ia = ishft2 - fulmal[j - 1];
            if (ia <= ndltma && ia > 0) tmp -= fulmap[j - 1] * a[ia - 1];
        }
        tfcst[ielt - 1] = tmp - tfcst[ishft1 - 1];
        if (ielt < ishft1) tfcst[ishft1 - 1] = tmp;
    }

    // Regression adjustment: Fcst = Ay - (AX - X_f) b.
    resid(ctx, tfcst.data(), nfcst, m.ncxy, m.ncxy, 1, m.nb, MONE, d.b.data(),
          fcst);
    if (ctx.error.lfatal) return;

    // Forecast standard errors. se(lead) = sqrt(Var*(regvar + sum psi^2)).
    double fctssq = ZERO;
    int ielt = 1;
    int nb2 = m.nb;
    if (m.iregfx >= 2)
        for (int j = 1; j <= m.nb; ++j)
            if (m.regfx(j)) nb2 = nb2 - 1;
    for (int i = 1; i <= nfcst; ++i) {
        double tmp;
        if (nb2 > 0) {
            // Design-uncertainty term X_f (X'X)^-1 X_f' via the packed Cholesky
            // (dppsl forward-solve, then yprmy sum of squares).
            dppsl(d.chlxpx.data(), nb2, tfcst.data() + (ielt - 1), true);
            yprmy(tfcst.data() + (ielt - 1), nb2, tmp);
        } else {
            tmp = ZERO;
        }
        rgvar[i - 1] = tmp * d.var;
        fctssq += piwght[i - 1] * piwght[i - 1];
        tmp += fctssq;
        se[i - 1] = std::sqrt(tmp * d.var);
        ielt += m.ncxy;
    }
}

}  // namespace x13
