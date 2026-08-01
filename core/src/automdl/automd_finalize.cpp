// automd_finalize.cpp -- see automd_finalize.hpp. Faithful ports of
// oracle/fortran/ssprep.f, restor.f, rmfix.f, addfix.f, tstdrv.f, pass0.f,
// clrotl.f, autoer.f (the Lmodel-only / non-outlier call shapes automd.f's
// finalization uses).
#include "automdl/automd_finalize.hpp"

#include <cmath>
#include <string>

#include "automdl/chkmu.hpp"          // genrtt
#include "numeric/numeric.hpp"        // dpeq, dpmpar, dppdi, daxpy, eltfcn
#include "regarima/regvar.hpp"        // regvar, td7var, gtrgpt
#include "transform/transform.hpp"    // trnfcn
#include "specparse/specparse.hpp"    // strinx, adrgef, dlrgef, getstr, insstr,
                                       // insptr, delstr, intlst, copy, copylg,
                                       // cpyint, setdp, abend, writln
#include "gen/model.hpp"              // prm PRG* type codes
#include "gen/notset.hpp"             // prm::DNOTST, prm::NOTSET
#include "gen/srslen.hpp"             // prm::PLEN

namespace x13 {
using namespace prm;

namespace {
constexpr int PACM = (PLEN + 2 * PORDER) * PARIMA;

// True when Rgvrtp(begcol) is one of the "user regressor" family codes that
// rmfix/addfix special-case via dlusrg/addusr. Not reachable by the
// td/easter/Constant aictest corpus.
bool is_user_rgvr(int rt) {
    return (rt >= PRGTUH && rt <= PRGUH5) || rt == PRGTUS || rt == PRGTUD ||
           rt == PRGUAO || rt == PRGULS || rt == PRGUSO || rt == PRGUTD ||
           rt == PRGULM || rt == PRGULQ || rt == PRGULY || rt == PRGUCN ||
           rt == PRGUCY;
}

// The trading-day / length-of-month family pass0 manages when deciding whether
// to strip the TD group entirely (pass0.f:63-76, 105-118).
bool is_td_rgvr_pass0(int rt) {
    return rt == PRGTST || rt == PRGTTD || rt == PRRTST || rt == PRRTTD ||
           rt == PRATST || rt == PRATTD || rt == PRGTLM || rt == PRGTLQ ||
           rt == PRGTLY || rt == PRGTSL || rt == PRRTLM || rt == PRRTLQ ||
           rt == PRRTLY || rt == PRRTSL || rt == PRATLM || rt == PRATLQ ||
           rt == PRATLY || rt == PRATSL || rt == PRG1TD || rt == PRR1TD ||
           rt == PRA1TD || rt == PRG1ST || rt == PRR1ST || rt == PRA1ST ||
           rt == PRGUTD || rt == PRGULM || rt == PRGULQ || rt == PRGULY;
}
}  // namespace

// ---------------------------------------------------------------------------
// ssprep.f / restor.f (Lmodel branch)
// ---------------------------------------------------------------------------
void ssprep_save(X13Context& ctx) {
    auto& sp = ctx.ssprep;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& ar = ctx.arima;
    auto& pk = ctx.picktd;
    auto& pr = ctx.prior;
    auto& x11 = ctx.x11adj;
    auto& ur = ctx.usrreg;

    sp.kfm2 = pr.kfmt;
    if (pr.nprtyp == 0 && sp.kfm2 > 0 && !pr.lpradj) sp.kfm2 = 0;

    if (pk.picktd && ar.fcntyp == 1 && pr.priadj <= 0) {
        pr.priadj = sp.pri2;
    } else {
        sp.pri2 = pr.priadj;
    }
    sp.ngr2 = m.ngrp;
    sp.ngrt2 = m.ngrptl;
    sp.ncxy2 = m.ncxy;
    sp.nbb = m.nb;
    sp.nct2 = m.ncoltl;
    sp.cttl = m.colttl.raw();
    sp.gttl = m.grpttl.raw();
    cpyint(m.colptr.data(), PB + 1, 1, sp.clptr.data());
    cpyint(m.grp.data(), PGRP + 1, 1, sp.g2.data());
    cpyint(m.grpptr.data(), PGRP + 1, 1, sp.gptr.data());
    cpyint(m.rgvrtp.data(), PB, 1, sp.rgv2.data());
    copy(d.arimap.data(), PARIMA, 1, sp.ap2.data());
    copy(d.b.data(), PB, 1, sp.bb.data());
    copylg(m.arimaf.data(), PARIMA, 1, sp.fxa.data());
    sp.nr2 = ar.nrxy;
    sp.ncusr2 = ur.ncusrx;
    sp.nrusrx2 = ar.nrusrx;
    sp.irfx2 = m.iregfx;
    copylg(m.regfx.data(), PB, 1, sp.regfx2.data());
    sp.pktd2 = pk.picktd;
    sp.atd = x11.adjtd;
    sp.ahol = x11.adjhol;
    sp.aao = x11.adjao;
    sp.als = x11.adjls;
    sp.atc = x11.adjtc;
    sp.aso = x11.adjso;
    sp.asea = x11.adjsea;
    sp.acyc = x11.adjcyc;
    sp.ausr = x11.adjusr;
    sp.fnhol = x11.finhol;
    sp.fnao = x11.finao;
    sp.fnls = x11.finls;
    sp.fntc = x11.fintc;
    sp.fnusr = x11.finusr;
    sp.flltd = pk.fulltd;
    sp.lma2 = m.lma;
    sp.lar2 = m.lar;
    sp.nintv2 = m.nintvl;
    sp.nextv2 = m.nextvl;
    sp.mxdfl2 = m.mxdflg;
    sp.mxarl2 = m.mxarlg;
    sp.mxmal2 = m.mxmalg;
    sp.v2 = d.var;
    copy(d.chlxpx.data(), PXPX, 1, sp.chx2.data());
    copy(d.chlgpg.data(), PGPG, 1, sp.chg2.data());
    copy(d.armacm.data(), PACM, 1, sp.acm2.data());
    sp.dtcv2 = d.lndtcv;
}

void restor_model(X13Context& ctx) {
    auto& sp = ctx.ssprep;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& ar = ctx.arima;
    auto& pk = ctx.picktd;
    auto& pr = ctx.prior;
    auto& x11 = ctx.x11adj;
    auto& ur = ctx.usrreg;

    pr.kfmt = sp.kfm2;

    m.ngrp = sp.ngr2;
    m.ngrptl = sp.ngrt2;
    m.ncxy = sp.ncxy2;
    m.nb = sp.nbb;
    pr.priadj = sp.pri2;
    m.ncoltl = sp.nct2;
    m.colttl = sp.cttl.raw();
    m.grpttl = sp.gttl.raw();
    cpyint(sp.clptr.data(), PB + 1, 1, m.colptr.data());
    cpyint(sp.g2.data(), PGRP + 1, 1, m.grp.data());
    cpyint(sp.gptr.data(), PGRP + 1, 1, m.grpptr.data());
    cpyint(sp.rgv2.data(), PB, 1, m.rgvrtp.data());
    copy(sp.ap2.data(), PARIMA, 1, d.arimap.data());
    copy(sp.bb.data(), PB, 1, d.b.data());
    // NB (bug-for-bug): only PB elements restored, though ssprep saved PARIMA.
    copylg(sp.fxa.data(), PB, 1, m.arimaf.data());
    ar.nrxy = sp.nr2;
    ur.ncusrx = sp.ncusr2;
    ar.nrusrx = sp.nrusrx2;
    m.iregfx = sp.irfx2;
    copylg(sp.regfx2.data(), PB, 1, m.regfx.data());
    pk.picktd = sp.pktd2;
    x11.adjtd = sp.atd;
    x11.adjhol = sp.ahol;
    x11.adjao = sp.aao;
    x11.adjls = sp.als;
    x11.adjtc = sp.atc;
    x11.adjso = sp.aso;
    x11.adjsea = sp.asea;
    x11.adjusr = sp.ausr;
    x11.finhol = sp.fnhol;
    x11.finao = sp.fnao;
    x11.finls = sp.fnls;
    x11.fintc = sp.fntc;
    x11.finusr = sp.fnusr;
    pk.fulltd = sp.flltd;
    m.lma = sp.lma2;
    m.lar = sp.lar2;
    m.nintvl = sp.nintv2;
    m.nextvl = sp.nextv2;
    m.mxdflg = sp.mxdfl2;
    m.mxarlg = sp.mxarl2;
    m.mxmalg = sp.mxmal2;
    d.var = sp.v2;
    copy(sp.chx2.data(), PXPX, 1, d.chlxpx.data());
    copy(sp.chg2.data(), PGPG, 1, d.chlgpg.data());
    copy(sp.acm2.data(), PACM, 1, d.armacm.data());
    d.lndtcv = sp.dtcv2;
}

// ---------------------------------------------------------------------------
// rmfix.f / addfix.f
// ---------------------------------------------------------------------------
void rmfix(X13Context& ctx, double* trnsrs, int nbcst, int nrxy, int fxindx) {
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& fx = ctx.fxreg;

    if (fxindx < 2 || fx.nfxttl == 0) {
        setdp(0.0, PLEN, fx.fixfac.data());
        intlst(PB, fx.cfxptr.data(), fx.nfxttl);
        intlst(PGRP, fx.gfxptr.data(), fx.ngfxtl);
        intlst(PGRP, fx.grpfix.data(), fx.ngrpfx);
    }
    if (fxindx == 2) setdp(0.0, PLEN, fx.fixfc2.data());

    int oldfix = fx.nfxttl;
    int nreg = fx.nfxttl + 1;
    int numgrp = fx.ngfxtl + 1;
    if (m.ngrp == 0) return;

    for (int igrp = m.ngrp; igrp >= 1; --igrp) {
        std::string strgrp;
        int nchgrp = 0;
        getstr(ctx, m.grpttl.data(), m.grpptr.data(), m.ngrptl, igrp, strgrp,
               nchgrp);
        if (ctx.error.lfatal) return;
        int endcol = m.grp(igrp) - 1;
        int begcol = m.grp(igrp - 1);
        int icol = endcol;
        while (icol >= begcol) {
            if (m.regfx(icol) || fxindx == 2) {
                std::string str;
                int nchr = 0;
                getstr(ctx, m.colttl.data(), m.colptr.data(), m.ncoltl, icol,
                       str, nchr);
                if (!ctx.error.lfatal)
                    insstr(ctx, str, nreg, PB, fx.cfxttl.data(),
                           static_cast<int>(fx.cfxttl.size()), fx.cfxptr.data(),
                           fx.nfxttl);
                if (ctx.error.lfatal) return;
                fx.bfx(fx.nfxttl) = d.b(icol);
                fx.fxtype(fx.nfxttl) = m.rgvrtp(icol);
                fx.fixind(fx.nfxttl) = fxindx;
                ++nreg;

                if (fxindx == 2) {
                    if (m.rgvrtp(icol) != PRGTCN)
                        daxpy(nrxy, d.b(icol), &d.xy(icol), m.ncxy,
                              fx.fixfc2.data(), 1);
                } else {
                    daxpy(nrxy, d.b(icol), &d.xy(icol), m.ncxy, fx.fixfac.data(),
                          1);
                }

                if (is_user_rgvr(m.rgvrtp(icol))) {
                    // dlusrg not ported: unreachable for the td/easter/Constant
                    // aictest corpus (no user-defined regressor types).
                    abend(ctx);
                    return;
                }
                dlrgef(ctx, icol, nrxy, 1);
                if (ctx.error.lfatal) return;
            }
            --icol;
        }
        if (oldfix < fx.nfxttl) {
            insstr(ctx, strgrp, numgrp, PGRP, fx.gfxttl.data(),
                   static_cast<int>(fx.gfxttl.size()), fx.gfxptr.data(),
                   fx.ngfxtl);
            if (!ctx.error.lfatal)
                insptr(ctx, true, fx.nfxttl - oldfix, numgrp, PGRP, PB,
                       fx.grpfix.data(), fx.ngrpfx);
            if (ctx.error.lfatal) return;
            oldfix = fx.nfxttl;
            ++numgrp;
        }
    }

    if (fx.nfxttl > 0) {
        if (fxindx == 2) {
            for (int i = 1; i <= d.nspobs; ++i) trnsrs[i - 1] -= fx.fixfc2(i + nbcst);
        } else {
            for (int i = 1; i <= d.nspobs; ++i) trnsrs[i - 1] -= fx.fixfac(i + nbcst);
        }
    }
}

void addfix(X13Context& ctx, double* trnsrs, int nbcst, int rind, int fxindx) {
    (void)rind;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& fx = ctx.fxreg;

    if (fx.ngrpfx == 0) return;

    int nu = 0;
    for (int igrp = fx.ngrpfx; igrp >= 1; --igrp) {
        int begcol = fx.grpfix(igrp - 1);
        int endcol = fx.grpfix(igrp) - 1;
        if (is_user_rgvr(fx.fxtype(begcol))) {
            ++nu;
            for (int icol = endcol; icol >= begcol; --icol) {
                if (fx.fixind(icol) == fxindx) {
                    delstr(ctx, icol, fx.cfxttl.data(), fx.cfxptr.data(),
                           fx.nfxttl, PB);
                    if (ctx.error.lfatal) return;
                }
            }
        } else {
            std::string strgrp;
            int nchgrp = 0;
            getstr(ctx, fx.gfxttl.data(), fx.gfxptr.data(), fx.ngfxtl, igrp,
                   strgrp, nchgrp);
            if (ctx.error.lfatal) return;
            for (int icol = endcol; icol >= begcol; --icol) {
                if (fx.fixind(icol) == fxindx) {
                    std::string str;
                    int nchr = 0;
                    getstr(ctx, fx.cfxttl.data(), fx.cfxptr.data(), fx.nfxttl,
                           icol, str, nchr);
                    if (!ctx.error.lfatal)
                        adrgef(ctx, fx.bfx(icol), str, strgrp, fx.fxtype(icol),
                               fxindx == 1, false);
                    if (ctx.error.lfatal) return;
                    delstr(ctx, icol, fx.cfxttl.data(), fx.cfxptr.data(),
                           fx.nfxttl, PB);
                    if (ctx.error.lfatal) return;
                }
            }
        }
    }

    if (m.userfx || (fxindx == 2 && nu > 0)) {
        // addusr not ported: unreachable for the td/easter/Constant corpus.
        abend(ctx);
        return;
    }

    if (fxindx == 2) {
        for (int i = 1; i <= d.nspobs; ++i) trnsrs[i - 1] += fx.fixfc2(i + nbcst);
    } else {
        for (int i = 1; i <= d.nspobs; ++i) trnsrs[i - 1] += fx.fixfac(i + nbcst);
    }
}

// ---------------------------------------------------------------------------
// tstdrv.f
// ---------------------------------------------------------------------------
double tstdrv(X13Context& ctx, int igrp) {
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    int regidx[PB];
    int nfix = 0;
    for (int j = 1; j <= m.nb; ++j) {
        if (m.regfx(j)) {
            ++nfix;
            regidx[j - 1] = NOTSET;
        } else {
            regidx[j - 1] = j - nfix;
        }
    }
    int nb2 = m.nb - nfix;

    double rmse = 0.0;
    double xpxinv[PB * (PB + 1) / 2];
    if (nb2 > 0 && d.var > 2.0 * dpmpar(1)) {
        int nelt = (nb2 + 1) * (nb2 + 2) / 2;
        rmse = std::sqrt(d.var);
        copy(d.chlxpx.data(), nelt, 1, xpxinv);
        double tmp[2];
        dppdi(xpxinv, nb2, tmp, 1);
    }
    if (dpeq(rmse, 0.0)) return 0.0;

    int begcol = m.grp(igrp - 1);
    int endcol = m.grp(igrp) - 1;
    double sumb = -d.b(begcol);
    int baselt = 0;
    double sumvar = 0.0;
    if (!m.regfx(begcol)) {
        baselt = regidx[begcol - 1] * (regidx[begcol - 1] + 1) / 2;
        sumvar = xpxinv[baselt - 1];
    }

    double seb = 0.0;
    if (begcol == endcol) {
        sumb *= 2.5;
        if (baselt > 0) seb = std::sqrt(sumvar) * rmse * 2.5;
    } else {
        for (int icol = begcol + 1; icol <= endcol; ++icol) {
            sumb -= d.b(icol);
            if (!m.regfx(icol)) {
                baselt = (regidx[icol - 1] - 1) * regidx[icol - 1] / 2;
                sumvar += xpxinv[baselt + regidx[icol - 1] - 1];
                for (int jcol = begcol; jcol <= icol - 1; ++jcol)
                    if (regidx[jcol - 1] != NOTSET)
                        sumvar += 2.0 * xpxinv[baselt + regidx[jcol - 1] - 1];
            }
        }
        if (baselt > 0) seb = std::sqrt(sumvar) * rmse;
    }
    return (baselt > 0) ? sumb / seb : 0.0;
}

// ---------------------------------------------------------------------------
// rmlpyr.f
// ---------------------------------------------------------------------------
void rmlpyr(X13Context& ctx, double* trnsrs, int nobspf) {
    constexpr int PLOM = 2, PLOQ = 3;  // prior.prm Priadj codes (aictst.cpp
                                        // convention -- not in gen/model.hpp).
    auto& ar = ctx.arima;
    auto& pk = ctx.picktd;
    auto& pr = ctx.prior;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& aj = ctx.adj;
    auto& pu = ctx.priusr;
    auto& ip = ctx.inpt;

    bool lom = (pr.priadj == PLOM || pr.priadj == PLOQ);
    bool begrgm[PLEN];
    if (pk.lrgmtd && (pk.tdzero % 2) != 0)
        gtrgpt(ctx, aj.begadj.data(), pk.tddate.data(), pk.tdzero, begrgm,
               aj.nadj);
    else
        for (int i = 0; i < PLEN; ++i) begrgm[i] = true;

    double lomeff[PLEN];
    for (int i = 0; i < PLEN; ++i) lomeff[i] = 1.0;
    td7var(aj.begadj.data(), m.sp, aj.nadj, 1, 1, lom, false, true, lomeff,
           begrgm);

    eltfcn(ELT_DIV, &ar.y(ar.frstsy), &aj.adj(aj.adj1st), nobspf, trnsrs);
    eltfcn(ELT_MULT, trnsrs, lomeff + (aj.adj1st - 1), nobspf, trnsrs);
    if (m.lmvaft || m.ln0aft)
        trnfcn(ctx, trnsrs, d.nspobs, ar.fcntyp, ar.lam, trnsrs);
    else
        trnfcn(ctx, trnsrs, nobspf, ar.fcntyp, ar.lam, trnsrs);
    if (ctx.error.lfatal) return;
    eltfcn(ELT_DIV, &aj.adj(aj.adj1st), lomeff + (aj.adj1st - 1), nobspf,
           &aj.adj(aj.adj1st));

    // rmlpyr.f:59 -- Sprior follows Adj when the leap-year prior is removed.
    // One of the three Sprior writes the MODEL stage makes (with tdaic.f:603/611
    // and pass2.f:101); all three were inert until Setpri moved ahead of the
    // model stage, and the post-model `Adj -> Sprior` copy in x11int stood in
    // for them. See M5_PORT_NOTES entry 57. The guard is a bounds check on
    // sprior(0), not a deferral -- the oracle has none.
    if (aj.setpri >= 1) copy(aj.adj.data(), aj.nadj, -1, &ip.sprior(aj.setpri));

    pr.priadj = 0;
    if (pu.nustad == 0 && pu.nuspad == 0) {
        pr.kfmt = 0;
        if (pr.lpradj) pr.lpradj = false;
    }
}

// ---------------------------------------------------------------------------
// pass0.f
// ---------------------------------------------------------------------------
void pass0(X13Context& ctx, double* trnsrs, int& frstry, int& isig, int istep,
           bool lprt) {
    (void)lprt;  // print output deferred
    auto& m = ctx.model;
    auto& ar = ctx.arima;
    auto& pk = ctx.picktd;
    auto& pr = ctx.prior;

    int ktd = strinx(false, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                     "Trading Day");
    if (ktd == 0 && (ar.itdtst == 1 || ar.itdtst == 4 || ar.itdtst == 5))
        ktd = strinx(false, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                     "1-Coefficient Trading Day");
    if (ktd == 0 && ar.itdtst == 3)
        ktd = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                     "Stock Trading Day");
    if (ktd == 0 && ar.itdtst == 6)
        ktd = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                     "1-Coefficient Stock Trading Day");

    int keastr = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                        "Easter");
    if (keastr == 0)
        keastr = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                        "StatCanEaster");
    if (keastr == 0)
        keastr = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                        "StockEaster");

    int kmu = strinx(false, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                     "Constant");
    if (ktd == 0 && keastr == 0 && kmu == 0) return;

    double tval[PB];
    genrtt(ctx, tval);
    double cval = 1.96;

    if (ktd > 0 && ar.itdtst > 0) {
        int nsig = 0;
        for (int igrp = m.ngrp; igrp >= 1; --igrp) {
            int begcol = m.grp(igrp - 1);
            int endcol = m.grp(igrp) - 1;
            if (is_td_rgvr_pass0(m.rgvrtp(begcol)))
                for (int icol = begcol; icol <= endcol; ++icol)
                    if (std::abs(tval[icol - 1]) >= cval) ++nsig;
        }
        if (nsig < 1) {
            double tderiv = tstdrv(ctx, ktd);
            if (std::abs(tderiv) < cval) ktd = -ktd;
        }
    }
    if (keastr > 0 && ar.leastr) {
        int nsig = 0;
        int begcol = m.grp(keastr - 1);
        int endcol = m.grp(keastr) - 1;
        for (int icol = begcol; icol <= endcol; ++icol)
            if (std::abs(tval[icol - 1]) >= cval) ++nsig;
        if (nsig < 1) keastr = -keastr;
    }
    if (istep == 1) cval = ar.tsig;
    if (kmu > 0 && ar.lchkmu) {
        int begcol = m.grp(kmu - 1);
        if (std::abs(tval[begcol - 1]) < cval) kmu = -kmu;
    }

    if (ktd < 0) {
        for (int igrp = m.ngrp; igrp >= 1; --igrp) {
            int begcol = m.grp(igrp - 1);
            int endcol = m.grp(igrp) - 1;
            if (is_td_rgvr_pass0(m.rgvrtp(begcol))) {
                dlrgef(ctx, begcol, ar.nrxy, endcol - begcol + 1);
                if (ctx.error.lfatal) return;
            }
        }
        ++isig;
        ar.aicint = 0;
        if (pk.picktd) {
            pk.picktd = false;
            if (!(ar.fcntyp == 4 || dpeq(ar.lam, 1.0))) {
                rmlpyr(ctx, trnsrs, ctx.extend.nobspf);
                if (ctx.error.lfatal) return;
            }
        }
    }
    if (keastr < 0) {
        int igrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                          "Easter");
        if (igrp == 0)
            igrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                          "StatCanEaster");
        if (igrp == 0)
            igrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                          "StockEaster");
        int begcol = m.grp(igrp - 1);
        int endcol = m.grp(igrp) - 1;
        dlrgef(ctx, begcol, ar.nrxy, endcol - begcol + 1);
        if (ctx.error.lfatal) return;
        ++isig;
        ar.aicind = 0;
    }
    if (kmu < 0) {
        int igrp = strinx(false, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                          "Constant");
        int begcol = m.grp(igrp - 1);
        dlrgef(ctx, begcol, ar.nrxy, 1);
        if (ctx.error.lfatal) return;
        ++isig;
    }
    if (isig > 0)
        regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
               ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, pr.priadj,
               ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
}

// ---------------------------------------------------------------------------
// clrotl.f
// ---------------------------------------------------------------------------
void clrotl(X13Context& ctx, int nrxy) {
    auto& m = ctx.model;
    int icol = m.nb;
    while (icol >= 1) {
        int rt = m.rgvrtp(icol);
        if (rt == PRGTAA || rt == PRGTAL || rt == PRGTAT) {
            dlrgef(ctx, icol, nrxy, 1);
            if (ctx.error.lfatal) return;
        }
        --icol;
    }
    m.natotl = 0;
}

// ---------------------------------------------------------------------------
// autoer.f
// ---------------------------------------------------------------------------
void autoer(X13Context& ctx, int info) {
    if (info == PINVER || info == PGPGER || info == PACFER || info == PVWPER)
        abend(ctx);
}

}  // namespace x13
