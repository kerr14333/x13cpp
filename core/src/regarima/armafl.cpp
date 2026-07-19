// armafl.cpp -- intgpg.f / exctma.f (exact ARMA filter helpers). Faithful ports
// of the vendored oracle Fortran; ex-COMMON state reached through ctx.
#include "regarima/armafl.hpp"

#include <vector>

#include "regarima/armafilt.hpp"  // mltpos, arflt
#include "regarima/regvar.hpp"    // ratpos, ratneg
#include "numeric/numeric.hpp"    // scrmlt, dppfa, logdet, dsolve, uconv, ...
#include "specparse/specparse.hpp"  // copy, setdp
#include "gen/model.hpp"          // prm::MA, prm::PORDER, prm::PB
#include "gen/srslen.hpp"         // prm::PLEN

namespace x13 {

// intgpg.f -- pi weights (piwght) via ratpos on 1/theta, their negative-power
// cross products (ssqpwt) via ratneg; the first row of G'G is ssqpwt, the rest
// built by the Toeplitz-like recursion Chlgpg(ielt)=Chlgpg(ielt-j)-... . coef
// buffers are sized PA=PLEN+2*PORDER exactly as the fixed Fortran locals.
void intgpg(X13Context& ctx, int nextma, int& info) {
    constexpr double ONE = 1.0;
    constexpr int PA = prm::PLEN + 2 * prm::PORDER;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    if (m.lma) {
        std::vector<double> piwght(PA, 0.0), ssqpwt(PA, 0.0);
        piwght[0] = ONE;  // piwght(1)=ONE
        int ntmp = 1;
        ratpos(ntmp, d.arimap.data(), m.arimal.data(), m.opr.data(),
               m.mdl(prm::MA - 1), m.mdl(prm::MA) - 1, nextma, piwght.data());
        copy(piwght.data(), nextma, 1, ssqpwt.data());
        ratneg(nextma, d.arimap.data(), m.arimal.data(), m.opr.data(),
               m.mdl(prm::MA - 1), m.mdl(prm::MA) - 1, ssqpwt.data());
        int nap2 = nextma + 2;
        int ielt = 1;
        d.chlgpg(ielt) = ssqpwt[ielt - 1];  // Chlgpg(1)=ssqpwt(1)
        for (int j = 2; j <= m.mxmalg; ++j) {
            ielt = ielt + 1;
            d.chlgpg(ielt) = ssqpwt[j - 1];
            for (int i = 2; i <= j; ++i) {
                ielt = ielt + 1;
                d.chlgpg(ielt) = d.chlgpg(ielt - j) -
                                 piwght[nap2 - i - 1] * piwght[nap2 - j - 1];
            }
        }
        dppfa(d.chlgpg.data(), m.mxmalg, info);
        if (info <= 0) logdet(d.chlgpg.data(), m.mxmalg, d.lndtcv);
    } else {
        d.lndtcv = 0.0;
    }
}

// exctma.f -- form Hw (pad the differenced series with neltq zeros in front),
// apply 1/theta both directions to get G'Hw, solve against chlgpg for the
// initial values w*, negate and prepend, then ratpos-filter the whole vector.
// work is sized PXA=(PB+1)*(PLEN+2*PORDER) as in the Fortran. nelta is in/out.
void exctma(X13Context& ctx, int nc, double* a, int& nelta, int nata) {
    constexpr double ZERO = 0.0, MONE = -1.0;
    constexpr int PXA = (prm::PB + 1) * (prm::PLEN + 2 * prm::PORDER);
    (void)nata;  // Fortran uses it only as A's DIMENSION bound.
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    m.nopr = m.mdl(prm::MA) - 1;  // Nopr=Mdl(MA)-1 (writes common)
    if (m.nopr > 0) {
        if (m.lma) {
            std::vector<double> work(PXA, 0.0);
            int neltq = m.mxmalg * nc;
            copy(a, nelta, -1, a + neltq);                  // A -> A(neltq+1)
            copy(a + neltq, nelta, 1, work.data() + neltq);  // -> work(neltq+1)
            setdp(ZERO, neltq, work.data());
            nelta = neltq + nelta;
            ratpos(nelta, d.arimap.data(), m.arimal.data(), m.opr.data(),
                   m.mdl(prm::MA - 1), m.mdl(prm::MA) - 1, nelta, work.data());
            ratneg(nelta, d.arimap.data(), m.arimal.data(), m.opr.data(),
                   m.mdl(prm::MA - 1), m.mdl(prm::MA) - 1, work.data());
            dsolve(d.chlgpg.data(), m.mxmalg, nc, true, work.data());
            scrmlt(MONE, neltq, work.data());
            copy(work.data(), neltq, 1, a);
        }
        ratpos(nelta, d.arimap.data(), m.arimal.data(), m.opr.data(),
               m.mdl(prm::MA - 1), m.mdl(prm::MA) - 1, nelta, a);
    }
}

}  // namespace x13
