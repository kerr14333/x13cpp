// forecast.cpp -- fcstxy.f: regARIMA forecasts + forecast standard errors.
// Faithful port; all array indexing kept 1-based (Fortran) with -1 offsets on
// the C++ pointers. Every dependency (armafl, polyml, eltlen, ratpos, resid,
// dppsl, yprmy, copy, setdp) is already oracle-verified.
#include "regarima/forecast.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

#include "regarima/armafl.hpp"      // armafl
#include "regarima/priadj.hpp"      // lpfac
#include "regarima/regvar.hpp"      // ratpos
#include "regarima/estimate.hpp"    // resid
#include "numeric/numeric.hpp"      // dppsl, yprmy, dinvnr, scrmlt, eltfcn, dpeq
#include "transform/transform.hpp"  // invfcn, lgnrmc
#include "specparse/specparse.hpp"  // polyml, eltlen, copy, setdp
#include "gen/model.hpp"            // prm::DIFF, AR, MA, PB, PORDER, PDIFOR, POPR
#include "gen/srslen.hpp"           // prm::PLEN, prm::PFCST
#include "gen/tbltab.hpp"           // prm::LFOROS, prm::LFORBC
#include "x13/fformat.hpp"          // fwrite_fmt

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

// fcstout -- prtfct.f's forecast-output (LFOROS) numeric path, minus printing,
// save-file I/O, and the user-prior/holiday/TD correction branches. Produces the
// original-scale point forecast and two-tailed confidence band from the
// estimated model, storing them on ctx.forecasts.
void fcstout(X13Context& ctx, int nfcst, int fctdrp, double ciprob, bool lognrm) {
    if (nfcst <= 0) return;
    const int fcntyp = ctx.arima.fcntyp;
    const double lam = ctx.arima.lam;
    // prtfct.f: fctori = Nspobs - Fctdrp (the undifferenced series length fed to
    // fcstxy). ltrns is true unless the transform is the identity (lam==1 and the
    // "no transform" function code 4).
    const int fctori = ctx.mdldat.nspobs - fctdrp;
    const bool ltrns = (!dpeq(lam, 1.0)) || fcntyp != 4;
    const bool lam0 = dpeq(lam, 0.0);

    std::vector<double> fcst(prm::PFCST), fcstse(prm::PFCST), rgvar(prm::PFCST);
    fcstxy(ctx, fctori, nfcst, fcst.data(), fcstse.data(), rgvar.data());
    if (ctx.error.lfatal) return;

    // Point forecast on the original scale (untfct): lognormal mean-correction
    // for the log transform when requested, else the plain inverse transform.
    std::vector<double> untfct(prm::PFCST);
    if (lognrm && lam0) {
        lgnrmc(nfcst, fcst.data(), fcstse.data(), untfct.data(), /*ltrans=*/true);
    } else {
        invfcn(ctx, fcst.data(), nfcst, fcntyp, lam, untfct.data());
    }
    if (ctx.error.lfatal) return;

    // prtfct.f:92-103 -- Fcstx, the forecast APPENDED TO THE SERIES before the
    // seasonal adjustment, gets its own lognormal correction, on the
    // TRANSFORMED scale (`Ltrans=F`, so lgnrmc adds se^2/2 rather than
    // exponentiating). It is a separate buffer from `fcst`, which stays
    // uncorrected because the confidence band below is built from it.
    //
    // This is the half of `forecast{lognormal=}` that was missing: the option
    // was parsed and the :411 correction to `untfct` was applied, so the fct
    // TABLE was right while the forecast X-11 actually extends the series with
    // was not -- d10-d13 off behind an OUTCOME: OK.
    std::vector<double> fcstx(prm::PFCST);
    if (lognrm && lam0)
        lgnrmc(nfcst, fcst.data(), fcstse.data(), fcstx.data(),
               /*ltrans=*/false);
    else
        std::copy(fcst.begin(), fcst.begin() + nfcst, fcstx.begin());

    // Stash the transformed-scale forecast + SE before scrmlt scales fcstse.
    auto& out = ctx.forecasts;
    out.trnfct.assign(fcstx.begin(), fcstx.begin() + nfcst);
    out.trnse.assign(fcstse.begin(), fcstse.begin() + nfcst);

    // Confidence band: cv = dinvnr((Ciprob+1)/2) (two-tailed), then
    // [fcst - cv*se, fcst + cv*se] on the transformed scale, mapped back through
    // invfcn when the series was transformed.
    const double pval = (ciprob + 1.0) / 2.0;
    const double cv = dinvnr(pval, 1.0 - pval);
    scrmlt(cv, nfcst, fcstse.data());   // fcstse := cv * se
    std::vector<double> lwrci(prm::PFCST), uprci(prm::PFCST);
    eltfcn(ELT_SUB, fcst.data(), fcstse.data(), nfcst, lwrci.data());
    eltfcn(ELT_ADD, fcst.data(), fcstse.data(), nfcst, uprci.data());
    if (ltrns) {
        invfcn(ctx, lwrci.data(), nfcst, fcntyp, lam, lwrci.data());
        invfcn(ctx, uprci.data(), nfcst, fcntyp, lam, uprci.data());
        if (ctx.error.lfatal) return;
    }

    // prtfct.f:453-497 -- the prior-adjustment arm of the LFOROS block. The
    // NUMERIC half (eltfcn'ing Adj/X11hol/Stptd back onto the band) is the
    // `lpria` branch and is not in this slice; what IS here is the ELSE, whose
    // whole content is a WARNING. `lpria` asks whether the user's prior factors
    // COVER the forecast window; when there are factors and they stop short,
    // the band is labelled "After Prior Adjustments" and the oracle says so.
    //
    // The guard is `Prttab(LFOROS).or.Savtab(LFOROS).or.Lgraf`, which is also
    // the enclosing IF at prtfct.f:428 -- the oracle re-tests it here for
    // nothing (verified by pairing IF/END IF, not by indentation: :489's guard
    // closes at :493, well inside :428..:606). deftab(LFOROS) is TRUE, so this
    // fires on a spec that never mentions `print=`. Lgraf is false throughout
    // this port (no graphics files are written).
    //
    // prtfct.f:73-76: bgfcst = endspn - Fctdrp + 1, endspn = Begspn + Nspobs-1.
    // Note the third clause tests `Priadj.gt.0` where the outer IF tests
    // `.gt.1` -- transcribed as written.
    {
        const int sp = ctx.model.sp;
        const auto& pu = ctx.priusr;
        if ((pu.nustad > 0 || pu.nuspad > 0 || ctx.prior.priadj > 1) &&
            (ctx.tbllog.prttab(prm::LFOROS) || ctx.tbllog.savtab(prm::LFOROS))) {
            int endspn[2], bgfcst[2];
            addate(ctx.mdldat.begspn.data(), sp, ctx.mdldat.nspobs - 1, endspn);
            addate(endspn, sp, -fctdrp + 1, bgfcst);
            const bool lpria =
                (pu.nustad > 0 &&
                 chkcvr(pu.bgutad.data(), pu.nustad, bgfcst, nfcst, sp)) ||
                (pu.nuspad > 0 &&
                 chkcvr(pu.bgupad.data(), pu.nuspad, bgfcst, nfcst, sp)) ||
                (ctx.prior.priadj > 0 &&
                 chkcvr(ctx.adj.begadj.data(), ctx.adj.nadj, bgfcst, nfcst, sp));
            if (!lpria) {
                // prtfct.f:1120. Written whole rather than through writln: the
                // FORMAT's leading and trailing `/` are EMPTY records, where
                // writln's own blank is `(' ',a)` with a blank -- two spaces.
                const std::string rec = fwrite_fmt(
                    "(/,"
                    "' WARNING:  User-defined prior adjustment factor not "
                    "provided',/,"
                    "'           for the forecast period.',/)") + "\n";
                ctx.channels_.unit(stdio::STDERR).put(rec);   // Lquiet not ported
                errhdr(ctx);
                ctx.channels_.unit(ctx.units.mt2).put(rec);
            }
        }
    }

    // prtfct.f reapplies predefined prior factors after mapping forecasts back
    // to the original scale. In the fixed-model path Adj only spans observed
    // data, but these factors are date-only (td7var/lpfac), so build the
    // forecast-window slice directly without extending X-11's Sprior state.
    if (ctx.prior.priadj > 1) {
        const bool lom = (ctx.prior.priadj == 2 || ctx.prior.priadj == 3);
        const int op = (ctx.adj.adjmod < 2) ? ELT_MULT : ELT_ADD;
        std::vector<double> prior(static_cast<std::size_t>(nfcst));
        for (int i = 0; i < nfcst; ++i) {
            int idate[2];
            addate(ctx.mdldat.begspn.data(), ctx.model.sp, fctori + i, idate);
            prior[static_cast<std::size_t>(i)] =
                lpfac(idate[0], idate[1], ctx.model.sp, lom);
        }
        eltfcn(op, untfct.data(), prior.data(), nfcst, untfct.data());
        eltfcn(op, lwrci.data(), prior.data(), nfcst, lwrci.data());
        eltfcn(op, uprci.data(), prior.data(), nfcst, uprci.data());
    }

    out.nfcst = nfcst;
    out.fcst.assign(untfct.begin(), untfct.begin() + nfcst);
    out.lwrci.assign(lwrci.begin(), lwrci.begin() + nfcst);
    out.uprci.assign(uprci.begin(), uprci.begin() + nfcst);
}

// mkback.f:57-80 -- the numeric core of the backcasts (the remaining ~310 lines
// of mkback.f are print/save surface this port defers by design).
void bcstout(X13Context& ctx, int nbcst, const double* trnsrs, bool lognrm) {
    if (nbcst <= 0) return;
    mdldat_cmn& d = ctx.mdldat;
    const int nfcst = ctx.extend.nfcst < 0 ? 0 : ctx.extend.nfcst;

    // mkback.f:58 -- rebuild the design over Nspobs rows (NOT Nobspf) with the
    // backcast rows included. This deliberately leaves Nrxy/Begxy/Xy rebuilt for
    // the rest of the run, exactly as the oracle does.
    int nrxy = 0, frstry = 0;
    regvar(ctx, const_cast<double*>(trnsrs), d.nspobs, ctx.arima.fctdrp, nfcst,
           nbcst, ctx.arima.userx.data(), ctx.arima.bgusrx.data(),
           ctx.arima.nrusrx, ctx.prior.priadj, ctx.arima.reglom, nrxy,
           ctx.arima.begxy.data(), frstry, true, ctx.arima.elong);
    if (ctx.error.lfatal) return;
    ctx.arima.nrxy = nrxy;

    // mkback.f:61-68 -- reverse the observed+backcast rows of [X:y] so the
    // forward forecast recursion runs backwards in time, forecast Nbcst steps,
    // then put Xy back. The forecast rows (the trailing Nfcst) are excluded from
    // the reversal: they are not part of the backward-looking sample.
    const int ncxy = ctx.model.ncxy;
    const std::size_t n = static_cast<std::size_t>(nrxy) * ncxy;
    std::vector<double> bkxy(n);
    copy(ctx.mdldat.xy.data(), static_cast<int>(n), 1, bkxy.data());
    revrse(bkxy.data(), nrxy - nfcst, ncxy, ctx.mdldat.xy.data());

    std::vector<double> bcst(prm::PFCST), bse(prm::PFCST), rgvar(prm::PFCST);
    fcstxy(ctx, d.nspobs, nbcst, bcst.data(), bse.data(), rgvar.data());

    copy(bkxy.data(), static_cast<int>(n), 1, ctx.mdldat.xy.data());
    if (ctx.error.lfatal) return;

    // mkback.f:80 -- the lognormal mean-correction, in place on the transformed
    // backcasts (Ltrans=F: mkback corrects Bcst itself, not a separate array).
    if (lognrm && dpeq(ctx.arima.lam, 0.0))
        lgnrmc(nbcst, bcst.data(), bse.data(), bcst.data(), /*ltrans=*/false);

    // mkback.f:267-293 -- the BACKCAST twin of the WARNING in fcstout above,
    // same shape and same redundant inner guard (:285..:289 sits inside
    // :248..:387, which tests the same three flags). Two differences, both
    // transcribed: the window is `Begbak` for `Nbcst` periods rather than
    // bgfcst for Nfcst, and the sentence ends "for the backcast period."
    //
    // deftab(LFORBC) is FALSE where deftab(LFOROS) is TRUE, so unlike the
    // forecast half this one needs the spec to ask for the `bct` table --
    // `extra/airline_prior-backcast` is that spec, and it reaches this block
    // (mutating `lpria` away here fails 7 gates). What it does NOT do is make
    // the WARNING fire, and that is not a corpus gap -- the WARNING is
    // UNREACHABLE IN THE ORACLE. The proof, in the sense entry 95 asks for
    // (routes tried, not assumed):
    //
    //   mkback runs only for Nbcst > 0, and adjsrs.f:39-40 sets
    //   Begadj = Begspn - Nbcst -- which IS Begbak (editor.f:207) -- and
    //   Nadj = Nspobs + Nbcst + max(Sp, Nfcst-Fctdrp) >= Nbcst. So
    //   chkcvr(Begadj,Nadj,Begbak,Nbcst) is true and the third clause fires
    //   for ANY Priadj > 0.
    //   With no predefined prior the guard needs a user set, and addadj.f:29
    //   has already REFUSED the run unless that set covers [Begspn,Nspobs];
    //   addadj.f:43-52 then re-anchors Bgusra at Begadj and grows Nusrad, so
    //   it covers [Begbak,Nbcst] too and clause 1 or 2 fires.
    //   `.not.lpria` therefore needs Priadj==0 AND Nustad==0 AND Nuspad==0,
    //   which fails the guard above.
    //
    // Kept transcribed rather than walled because it is not a gap: the oracle
    // has the lines and never executes them. The mechanism is measurable even
    // though the outcome is not -- drop addadj's re-anchor (addadj.f:52) and
    // this WARNING starts firing, one gate. Mutating the WARNING away costs 0,
    // and that zero is a theorem here, not a saturated precondition.
    {
        const int sp = ctx.model.sp;
        const auto& pu = ctx.priusr;
        if ((pu.nustad > 0 || pu.nuspad > 0 || ctx.prior.priadj > 1) &&
            (ctx.tbllog.prttab(prm::LFORBC) || ctx.tbllog.savtab(prm::LFORBC))) {
            const int* begbak = ctx.extend.begbak.data();
            const bool lpria =
                (pu.nustad > 0 &&
                 chkcvr(pu.bgutad.data(), pu.nustad, begbak, nbcst, sp)) ||
                (pu.nuspad > 0 &&
                 chkcvr(pu.bgupad.data(), pu.nuspad, begbak, nbcst, sp)) ||
                (ctx.prior.priadj > 0 &&
                 chkcvr(ctx.adj.begadj.data(), ctx.adj.nadj, begbak, nbcst, sp));
            if (!lpria) {
                const std::string rec = fwrite_fmt(
                    "(/,"
                    "' WARNING:  User-defined prior adjustment factor not "
                    "provided',/,"
                    "'           for the backcast period.',/)") + "\n";
                ctx.channels_.unit(stdio::STDERR).put(rec);   // Lquiet not ported
                errhdr(ctx);
                ctx.channels_.unit(ctx.units.mt2).put(rec);
            }
        }
    }

    auto& out = ctx.forecasts;
    out.nbcst = nbcst;
    out.trnbct.assign(bcst.begin(), bcst.begin() + nbcst);
    out.trnbse.assign(bse.begin(), bse.begin() + nbcst);
}

}  // namespace x13
