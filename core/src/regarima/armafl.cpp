// armafl.cpp -- intgpg.f / exctma.f (exact ARMA filter helpers). Faithful ports
// of the vendored oracle Fortran; ex-COMMON state reached through ctx.
#include "regarima/armafl.hpp"

#include <algorithm>
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

// armafl.f -- exact ARMA filter. Faithful line-for-line port; the four Fortran
// GO TO 10 error exits collapse to early returns (all fire before any Arimal
// mutation, so no cleanup is skipped). nextma is the SAVEd local (ctx.saved).
void armafl(X13Context& ctx, int nr, int nc, bool linit, bool lckrts,
            double* mata, int& na, int nata, int& info) {
    constexpr double ONE = 1.0, ZERO = 0.0;
    constexpr int PMATD = (prm::PLEN + prm::PORDER) * prm::PORDER;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    int& nextma = ctx.saved.armafl_nextma;  // SAVE nextma

    info = 0;
    if (m.nopr == 0) {  // No ARMA model to filter
        na = nr;
        return;
    }
    int begopr = m.lar ? m.mdl(prm::AR - 1) : m.mdl(prm::MA - 1);
    int endopr = m.mdl(prm::MA) - 1;

    if (lckrts && chkrts(d.arimap.data(), m.arimal.data(), m.arimaf.data(),
                         m.opr.data(), m.oprfac.data(), begopr, endopr,
                         d.prbfac)) {
        info = prm::PINVER;
        return;
    }

    std::vector<double> acv(prm::PORDER + 1, 0.0), fular(prm::PORDER + 1, 0.0),
        fulma(prm::PORDER + 1, 0.0), psiwgt(prm::PORDER + 1, 0.0);

    if (linit && (m.lma || m.lar)) {
        nextma = nr - m.mxdflg - m.mxarlg + m.mxmalg;
        intgpg(ctx, nextma, info);
        if (info > 0) { info = prm::PGPGER; return; }

        if (m.lar) {
            // Expand the MA operator into fulma: theta(B)Theta(B)*1.
            fulma[0] = ONE;
            int nfulma = m.mxmalg + 1;
            mltpos(1, d.arimap.data(), m.arimal.data(), m.opr.data(),
                   m.mdl(prm::MA - 1), m.mdl(prm::MA) - 1, nfulma, fulma.data());
            copy(fulma.data(), nfulma, 1, psiwgt.data());
            ratpos(nfulma, d.arimap.data(), m.arimal.data(), m.opr.data(),
                   m.mdl(prm::AR - 1), m.mdl(prm::AR) - 1, nfulma, psiwgt.data());
            int maxpq = std::max(m.mxarlg, m.mxmalg);
            uconv(fulma.data(), m.mxmalg, acv.data());
            // Expand the AR operator into fular: phi(B)Phi(B)*1.
            fular[0] = ONE;
            int nfular = m.mxarlg + 1;
            mltpos(1, d.arimap.data(), m.arimal.data(), m.opr.data(),
                   m.mdl(prm::AR - 1), m.mdl(prm::AR) - 1, nfular, fular.data());
            euclid(fular.data(), d.matd.data(), d.matd.data() + m.mxarlg, maxpq,
                   m.mxarlg, m.mxmalg, acv.data(), info);
            if (info > 0) { info = prm::PACFER; return; }
            int nacv = m.mxarlg;
            xpand(fular.data(), m.mxarlg, maxpq, nacv, acv.data(), prm::PORDER);
            acv[0] = 2.0 * acv[0];
        }

        // Calculate D (only the last min(p,q) columns are nonzero).
        if (m.lar && m.lma) {
            int neltd = (nr - m.mxdflg - m.mxarlg) * m.mxarlg;
            setdp(ZERO, neltd, d.matd.data());
            for (int row = 1; row <= m.mxmalg; ++row) {
                int qprow = m.mxmalg + row;
                double tmp = ZERO;
                for (int k = row; k <= m.mxmalg; ++k)
                    tmp = tmp + fulma[qprow - k] * psiwgt[m.mxmalg - k];
                int ielt = m.mxarlg * row;
                for (int k = std::max(1, m.mxarlg - row + 1); k <= m.mxarlg;
                     ++k) {
                    d.matd(ielt) = tmp;
                    ielt = ielt - m.mxarlg - 1;
                }
            }
            // Multiply the lags by Mxarlg so a matrix filters like a vector.
            int lastlg = m.opr(endopr) - 1;
            for (int ilag = 1; ilag <= lastlg; ++ilag)
                m.arimal(ilag) = m.mxarlg * m.arimal(ilag);
            exctma(ctx, m.mxarlg, d.matd.data(), neltd, PMATD);
            nextma = neltd / m.mxarlg;
            for (int ilag = 1; ilag <= lastlg; ++ilag)
                m.arimal(ilag) = m.arimal(ilag) / m.mxarlg;
            xprmx(d.matd.data(), nextma, m.mxarlg, m.mxarlg, d.chlvwp.data());
            int ielt = 0;
            for (int j = 1; j <= m.mxarlg; ++j)
                for (int i = 1; i <= j; ++i) {
                    ielt = ielt + 1;
                    d.chlvwp(ielt) = acv[j - i] - d.chlvwp(ielt);
                }
        } else if (m.lar) {
            int ielt = 0;
            for (int j = 1; j <= m.mxarlg; ++j)
                for (int i = 1; i <= j; ++i) {
                    ielt = ielt + 1;
                    d.chlvwp(ielt) = acv[j - i];
                }
        }

        if (m.lar) {
            dppfa(d.chlvwp.data(), m.mxarlg, info);
            if (info > 0) { info = prm::PVWPER; return; }
            double ldtvwp;
            logdet(d.chlvwp.data(), m.mxarlg, ldtvwp);
            d.lndtcv = d.lndtcv + ldtvwp;
        }
    } else if (m.lma || m.lar) {
        nextma = nr - m.mxdflg - m.mxarlg + m.mxmalg;
    }

    // Multiply the series length and lags by Nc to filter a matrix like a vector.
    int nelta = nr * nc;
    endopr = m.mdl(prm::MA) - 1;
    int lastlg = m.opr(endopr) - 1;
    for (int ilag = 1; ilag <= lastlg; ++ilag)
        m.arimal(ilag) = nc * m.arimal(ilag);

    // Difference, then (conditionally) AR filter.
    arflt(nelta, d.arimap.data(), m.arimal.data(), m.opr.data(),
          m.mdl(prm::DIFF - 1), m.mdl(prm::DIFF) - 1, mata, nelta);

    int neltwp;
    if (m.lar) {  // Put w_p first, then \hat{a}.
        neltwp = m.mxarlg * nc;
        copy(mata, nelta, -1, mata + neltwp);
    } else {
        neltwp = 0;
    }

    arflt(nelta, d.arimap.data(), m.arimal.data(), m.opr.data(),
          m.mdl(prm::AR - 1), m.mdl(prm::AR) - 1, mata + neltwp, nelta);

    exctma(ctx, nc, mata + neltwp, nelta, nata - neltwp);

    // w_p - D' \hat{a}.
    if (m.lar && m.lma) {
        int ielt = 0;
        for (int i = 1; i <= m.mxarlg; ++i)
            for (int j = 1; j <= nc; ++j) {
                ielt = ielt + 1;
                mata[ielt - 1] =
                    mata[ielt - 1] - ddot(nextma, d.matd.data() + (i - 1),
                                          m.mxarlg, mata + neltwp + (j - 1), nc);
            }
    }

    // chol(var(w_p|z)) x = w_p - D' \hat{a}.
    if (m.lar) {
        dsolve(d.chlvwp.data(), m.mxarlg, nc, false, mata);
        nelta = nelta + neltwp;
    }

    na = nelta / nc;
    for (int ilag = 1; ilag <= lastlg; ++ilag)
        m.arimal(ilag) = m.arimal(ilag) / nc;
}

}  // namespace x13
