// amdid.cpp -- amdid.f / amdid2.f / bestmd.f / mdlmch.f: automatic ARMA-order
// identification. amdid2 estimates one candidate (reusing mdlint/mdlset/amdest/
// setmdl/rgarma/prlkhd), bestmd/mdlmch maintain the best-five BIC ranking, and
// amdid drives the seasonal-then-regular-then-seasonal grid search + the balance
// tie-break. Deferred: all Prttab/WRITE table + best-five prints.
#include "automdl/amdid.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "automdl/amdest.hpp"        // amdest
#include "automdl/mdlset.hpp"        // mdlint, mdlset
#include "numeric/numeric.hpp"       // dpeq, smeadl
#include "regarima/armafilt.hpp"     // arflt
#include "regarima/estimate.hpp"     // setmdl, rgarma, prlkhd
#include "regarima/regvar.hpp"       // regvar
#include "specparse/specparse.hpp"   // copy, setdp/setint (inline), abend
#include "gen/model.hpp"             // prm::DIFF, PARIMA, PSNGER
#include "gen/srslen.hpp"            // prm::PLEN
#include "gen/notset.hpp"            // prm::DNOTST, NOTSET

namespace x13 {

bool mdlmch(int nrar, int nrdiff, int nrma, int nsar, int nsdiff, int nsma,
            const int* bstrar, const int* bstrdf, const int* bstrma,
            const int* bstsar, const int* bstsdf, const int* bstsma,
            const double* bstbic) {
    for (int i = 0; i < 5; ++i) {
        if (dpeq(bstbic[i], prm::DNOTST)) return false;
        if (nrar == bstrar[i] && nrdiff == bstrdf[i] && nrma == bstrma[i] &&
            nsar == bstsar[i] && nsdiff == bstsdf[i] && nsma == bstsma[i])
            return true;
    }
    return false;
}

void bestmd(X13Context& ctx, int irar, int irdf, int irma, int isar, int isdf,
            int isma, int* bstrar, int* bstrdf, int* bstrma, int* bstsar,
            int* bstsdf, int* bstsma, double* bstbic) {
    double bic2 = ctx.lkhd.bic2;
    int i = 1;
    while (i <= 5) {
        if (dpeq(bstbic[i - 1], prm::DNOTST) || bstbic[i - 1] > bic2) {
            if (i < 5 && !dpeq(bstbic[i - 1], prm::DNOTST)) {
                for (int j = 4; j >= i; --j) {
                    if (!dpeq(bstbic[j - 1], prm::DNOTST)) {
                        bstbic[j] = bstbic[j - 1];
                        bstrdf[j] = bstrdf[j - 1];
                        bstrar[j] = bstrar[j - 1];
                        bstrma[j] = bstrma[j - 1];
                        bstsdf[j] = bstsdf[j - 1];
                        bstsar[j] = bstsar[j - 1];
                        bstsma[j] = bstsma[j - 1];
                    }
                }
            }
            bstbic[i - 1] = bic2;
            bstrdf[i - 1] = irdf;
            bstrar[i - 1] = irar;
            bstrma[i - 1] = irma;
            bstsdf[i - 1] = isdf;
            bstsar[i - 1] = isar;
            bstsma[i - 1] = isma;
            i = 6;
        } else {
            i = i + 1;
        }
    }
}

void amdid2(X13Context& ctx, int irar, int irdf, int irma, int isar, int isdf,
            int isma, double* txy, int nelta, bool lmu, bool& lgo) {
    using namespace prm;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& ar = ctx.arima;

    bool inptok = true;
    mdlint(ctx);
    mdlset(ctx, irar, irdf, irma, isar, isdf, isma, inptok);
    if (!inptok || ctx.error.lfatal) {
        lgo = false;
        if (!ctx.error.lfatal) abend(ctx);
        return;
    }
    int ardsp = m.nnsedf + m.nseadf;
    int nefobs = d.nspobs - m.nintvl;
    d.armaer = 0;
    if (!m.lcalcm) m.lcalcm = true;

    int info = 0;
    lgo = true;
    if (ar.hrinit) {
        double estprm[PARIMA];
        amdest(ctx, txy, nelta, nefobs, ardsp, lmu, false, info);
        if (ctx.error.lfatal) return;
        if (d.armaer == PSNGER || info < 0) {
            lgo = false;  // HR-init note deferred
            return;
        } else if (d.armaer != 0) {
            d.armaer = 0;
        }
        setmdl(ctx, estprm, lgo);
    }

    constexpr int PA = PLEN + 2 * PORDER;
    std::vector<double> a(static_cast<std::size_t>(PA), 0.0);
    int na = 0;
    if (lgo) {
        rgarma(ctx, ar.lestim, ar.mxiter, ar.mxnlit, false, a.data(), na, nefobs,
               ar.lautom);
        if (!ar.lautom) abend(ctx);
        if (ctx.error.lfatal) return;
        if (d.armaer == 0)
            prlkhd(ctx, ar.y.data() + ar.frstsy - 1,
                   ctx.adj.adj.data() + ctx.adj.adj1st - 1, ctx.adj.adjmod,
                   ar.fcntyp, ar.lam);
    }
    if (lgo) lgo = d.convrg && d.armaer == 0;
}

void amdid(X13Context& ctx, int& irar, int irdf, int& irma, int& isar, int isdf,
           int& isma, double* trnsrs, int& frstry, int& nefobs, double* a,
           int& na, bool lmu, int lsumm, bool& locok) {
    using namespace prm;
    (void)nefobs;
    (void)a;
    (void)na;
    (void)lsumm;  // best-five .udg summary print deferred
    constexpr double THREE2 = 0.03, THREE3 = 0.003, PTOL = 1.0e-3;
    constexpr int NMOD = 5;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& ar = ctx.arima;

    int bstrar[5], bstrdf[5], bstrma[5], bstsar[5], bstsdf[5], bstsma[5];
    double bstbic[5];

    irar = 3;
    irma = 0;
    int id = ar.diffam(1) + ar.diffam(2);
    setdp(DNOTST, 5, bstbic);
    setint(0, 5, bstrar);
    setint(0, 5, bstrdf);
    setint(0, 5, bstrma);
    setint(0, 5, bstsar);
    setint(0, 5, bstsdf);
    setint(0, 5, bstsma);

    // Loosen tolerance for exact-likelihood estimation.
    double nltbak = DNOTST, tolbak = m.tol;
    if (m.tol < PTOL) {
        nltbak = m.nltol;
        m.tol = PTOL;
        m.nltol = PTOL;
        m.nltol0 = 100.0 * m.tol;
    }

    bool lxar = m.lextar, lxma = m.lextma;
    m.lextar = true;
    m.lextma = true;
    m.lar = m.lextar && m.mxarlg > 0;
    m.lma = m.lextma && m.mxmalg > 0;
    if (m.lextar) {
        m.nintvl = m.mxdflg;
        m.nextvl = m.mxarlg + m.mxmalg;
    } else {
        m.nintvl = m.mxdflg + m.mxarlg;
        m.nextvl = 0;
        if (m.lextma) m.nextvl = m.mxmalg;
    }

    // Difference the data (the trial models here are ARMA on the differenced y).
    int nelta = d.nspobs;
    std::vector<double> txy(PLEN, 0.0);
    copy(trnsrs, nelta, 1, txy.data());
    if (id > 0)
        arflt(nelta, d.arimap.data(), m.arimal.data(), m.opr.data(),
              m.mdl(DIFF - 1), m.mdl(DIFF) - 1, txy.data(), nelta);
    double xmu;
    if (lmu) smeadl(txy.data(), 1, nelta, nelta, xmu);

    regvar(ctx, trnsrs, d.nspobs, ar.fctdrp, ctx.extend.nfcst, 0, ar.userx.data(),
           ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj, ar.reglom, ar.nrxy,
           ar.begxy.data(), frstry, true, false);

    bool inptok;
    int psar;

    // ---- seasonal orders at a fixed regular AR(3) ----
    if (m.sp == 1) {
        isar = 0;
        isma = 0;
        psar = 0;
    } else {
        int ngood = 0;
        for (isar = 0; isar <= ar.maxord(2); ++isar) {
            for (isma = 0; isma <= ar.maxord(2); ++isma) {
                if (ar.lmixmd || isar == 0 || isma == 0) {
                    amdid2(ctx, 3, irdf, 0, isar, isdf, isma, txy.data(), nelta,
                           lmu, inptok);
                    if (ctx.error.lfatal) return;
                    if (inptok) {
                        bestmd(ctx, 3, irdf, 0, isar, isdf, isma, bstrar, bstrdf,
                               bstrma, bstsar, bstsdf, bstsma, bstbic);
                        ++ngood;
                    }
                }
            }
        }
        if (ngood > 0) {
            isar = bstsar[0];
            isma = bstsma[0];
        } else {
            abend(ctx);  // "Cannot make a choice of seasonal ARMA order" deferred
            return;
        }
        if (ar.maxord(1) < 3) {
            setdp(DNOTST, 5, bstbic);
            setint(0, 5, bstrar);
            setint(0, 5, bstrdf);
            setint(0, 5, bstrma);
            setint(0, 5, bstsar);
            setint(0, 5, bstsdf);
            setint(0, 5, bstsma);
        }
    }

    // ---- regular orders at the chosen seasonal orders ----
    int ngood = 0;
    for (irar = 0; irar <= ar.maxord(1); ++irar) {
        for (irma = 0; irma <= ar.maxord(1); ++irma) {
            if ((ar.lmixmd || irar == 0 || irma == 0) &&
                !mdlmch(irar, irdf, irma, isar, isdf, isma, bstrar, bstrdf, bstrma,
                        bstsar, bstsdf, bstsma, bstbic)) {
                amdid2(ctx, irar, irdf, irma, isar, isdf, isma, txy.data(), nelta,
                       lmu, inptok);
                if (ctx.error.lfatal) return;
                if (inptok) {
                    bestmd(ctx, irar, irdf, irma, isar, isdf, isma, bstrar, bstrdf,
                           bstrma, bstsar, bstsdf, bstsma, bstbic);
                    ++ngood;
                }
            }
        }
    }
    if (ngood > 0) {
        irar = bstrar[0];
        irma = bstrma[0];
    } else {
        abend(ctx);  // "Cannot make a choice of nonseasonal ARMA order" deferred
        return;
    }
    // If nothing identified, take the next model for the re-identification stage.
    if (isar + isma + irar + irma == 0) {
        irar = bstrar[1];
        irma = bstrma[1];
    }

    // ---- re-identify seasonal orders at the chosen regular orders ----
    psar = ar.maxord(2);
    if (psar < 2 && isdf == 1) psar = 0;
    if (m.sp > 1) {
        ngood = 0;
        for (isar = 0; isar <= psar; ++isar) {
            for (isma = 0; isma <= ar.maxord(2); ++isma) {
                if ((ar.lmixmd || isar == 0 || isma == 0) &&
                    !mdlmch(irar, irdf, irma, isar, isdf, isma, bstrar, bstrdf,
                            bstrma, bstsar, bstsdf, bstsma, bstbic)) {
                    amdid2(ctx, irar, irdf, irma, isar, isdf, isma, txy.data(),
                           nelta, lmu, inptok);
                    if (ctx.error.lfatal) return;
                    if (inptok) {
                        bestmd(ctx, irar, irdf, irma, isar, isdf, isma, bstrar,
                               bstrdf, bstrma, bstsar, bstsdf, bstsma, bstbic);
                        ++ngood;
                    }
                }
            }
        }
        if (ngood > 0) {
            isar = bstsar[0];
            isma = bstsma[0];
        }
    }

    // ---- BIC-closeness / model-balance tie-break over the best five ----
    double bic1 = bstbic[0];
    irar = bstrar[0];
    isar = bstsar[0];
    irma = bstrma[0];
    isma = bstsma[0];
    int icon = 1;
    int ir1 = bstrar[0] + bstrma[0];
    int is1 = bstsar[0] + bstsma[0];
    int irr1 = std::abs(bstrar[0] + irdf - bstrma[0]);
    int iss1 = std::abs(bstsar[0] + isdf - bstsma[0]);
    double bmax = std::abs(bic1 - bstbic[NMOD - 1]);
    if (bmax < THREE3)
        bmax = 0.0625;
    else if (bmax < THREE2)
        bmax = 0.25;
    else
        bmax = 1.0;
    double vc11 = 0.01 * bmax, vc2 = 0.0025 * bmax, vc22 = 0.0075 * bmax;

    for (int i = 2; i <= NMOD; ++i) {
        int ir2 = bstrar[i - 1] + bstrma[i - 1];
        int is2 = bstsar[i - 1] + bstsma[i - 1];
        int irr2 = std::abs(bstrar[i - 1] + irdf - bstrma[i - 1]);
        int iss2 = std::abs(bstsar[i - 1] + isdf - bstsma[i - 1]);
        double dbic = std::abs(bstbic[i - 1] - bstbic[icon - 1]);
        int ichk = 0;
        if ((irr2 < irr1 || iss2 < iss1) && ir1 == ir2 && is1 == is2 &&
            dbic <= vc11 && ar.lbalmd)
            ichk = 1;
        else if (irr2 < irr1 && ir2 <= ir1 && is2 == is1 && bstrar[i - 1] > 0 &&
                 bstrma[i - 1] > 0 && dbic <= vc2 && ar.lbalmd)
            ichk = 2;
        else if ((((irr2 == 0 && irr2 < irr1 && irdf > 0) ||
                   (iss2 == 0 && iss2 < iss1 && isdf > 0))) &&
                 ir1 == ir2 && is1 == is2 && dbic <= vc11 && ar.lbalmd)
            ichk = 3;
        else if (irr2 == 0 && iss2 == 0 && dbic < vc2 && ar.lbalmd)
            ichk = 4;
        else if (ir2 > ir1 && irr2 == 0 && is2 == is1 && dbic < vc2 && ar.lbalmd)
            ichk = 5;
        else if (is2 > is1 && iss2 == 0 && ir2 == ir1 && dbic < vc2 && ar.lbalmd)
            ichk = 6;
        else if (is2 < is1 && is2 > 0 && ir2 == ir1 && iss2 == 0 && dbic <= vc2 &&
                 ar.lbalmd)
            ichk = 7;
        else if (i == 2 && ir1 == 0 && ir2 == 1 && is2 == is1 && dbic < vc2)
            ichk = 8;
        else if (ir2 < ir1 && ir2 > 0 && is2 == is1 && dbic < vc2)
            ichk = 9;
        else if (is2 < is1 && is2 > 0 && ir2 == ir1 && dbic < vc2)
            ichk = 10;
        else if (bstrar[i - 1] < irar && bstrma[i - 1] == irma && ir2 > 0 &&
                 is2 == is1 && dbic < vc22)
            ichk = 11;
        if (ichk > 0) {
            vc11 = vc11 - std::abs(bstbic[0] - bstbic[i - 1]);
            vc2 = vc2 - std::abs(bstbic[0] - bstbic[i - 1]);
            vc22 = vc22 - std::abs(bstbic[0] - bstbic[i - 1]);
            ir1 = ir2;
            is1 = is2;
            irr1 = irr2;
            iss1 = iss2;
            icon = i;
            bic1 = bstbic[i - 1];
            irar = bstrar[i - 1];
            isar = bstsar[i - 1];
            irma = bstrma[i - 1];
            isma = bstsma[i - 1];
        }
    }
    (void)bic1;
    // A no-ARMA-parameter winner falls through to the next-ranked model.
    if (isar + isma + irar + irma == 0 && icon < NMOD) {
        irar = bstrar[icon];
        irma = bstrma[icon];
        isar = bstsar[icon];
        isma = bstsma[icon];
    }

    if (std::getenv("X13_AMDID_DEBUG")) {
        for (int k = 0; k < NMOD; ++k)
            std::fprintf(stderr,
                         "best5 %d: (%d %d %d)(%d %d %d) bic=%.4f\n", k + 1,
                         bstrar[k], bstrdf[k], bstrma[k], bstsar[k], bstsdf[k],
                         bstsma[k], bstbic[k]);
    }

    m.lextar = lxar;
    m.lextma = lxma;
    if (nltbak != NOTSET) {
        m.tol = tolbak;
        m.nltol = nltbak;
        m.nltol0 = m.tol * 100.0;
    }

    // ---- final: build + estimate the chosen model ----
    mdlint(ctx);
    mdlset(ctx, irar, irdf, irma, isar, isdf, isma, locok);
    if (!ctx.error.lfatal)
        rgarma(ctx, ar.lestim, ar.mxiter, ar.mxnlit, false, a, na, nefobs,
               ar.lautom);
    if (!ar.lautom) abend(ctx);
    if (ctx.error.lfatal) return;
    if (!d.convrg) {
        abend(ctx);  // non-convergence message deferred
        return;
    }

    ar.bstdsn = m.mdldsn.raw();   // Bstdsn = Mdldsn
    ar.nbstds = m.nmddcr;
    // Best-five .udg/table emission deferred (automdl.best5.* prints).
}

}  // namespace x13
