// run_x11.cpp -- M5 X-11 driver phase: assemble the classic X-11 decomposition
// spine (the x11ari base path) on top of the parsed spec.
//
// First target: the airline_x11-default *no-model* path. With only series{} +
// x11{}, Lmodel is false, so gtinpt leaves Nfcst=0 and Ldestm=false -- there is
// no regARIMA model, hence no forecast/backcast extension and no arima/extend/
// adjreg glue. The padded X-11 buffer is exactly the observed span, and the
// transformed input Series == the untransformed Orig == the raw series (mode=mult
// selects Muladd=0 with Fcntyp=4 "none"). The spine reduces to:
//
//     [editor.f 224-233 span pointers] -> setxpt -> x11int -> x11pt1 -> x11pt2
//
// producing the B/C/D decomposition (B1..D7) in the ctx x11srs arrays. The model
// path (estimate + forecast + extend + adjreg) and x11pt3/x11pt4 wire in here as
// they land; until then a spec carrying a model fatals cleanly.
#include "specparse/specparse.hpp"
#include "x11/x11parts.hpp"   // x11pt1, x11pt2
#include "x11/x11drv.hpp"     // setxpt, x11int, chkadj, regeff, extend, adjreg
#include "regarima/regvar.hpp"   // regvar (design rebuild for regression effects)
#include "notset.hpp"         // prm::NOTSET

#include <algorithm>
#include <string>

namespace x13 {

bool run_x11(X13Context& ctx, const std::string& spec_text, const std::string& base) {
    if (!parse_spec(ctx, spec_text, base)) return false;
    if (!ctx.captured.has_series) return false;
    if (!ctx.captured.has_x11) return false;

    // Model path: estimate the regARIMA model + forecast (arima.f front) via the
    // shared run_m2 body before the X-11 spine. The clean transformed series is
    // captured for the forecast-extension below; begxy/nrxy/forecasts land on ctx.
    const bool has_model = ctx.captured.has_model;
    std::vector<double> trnsrs;
    int nobspf_m = 0;
    if (has_model) {
        if (!run_m2_after_parse(ctx, base, /*estimate=*/true, &trnsrs, &nobspf_m))
            return false;
        if (ctx.error.lfatal) return false;
    }

    const int sp = ctx.model.sp;
    const int* begsrs = ctx.arima.begsrs.data();
    const int* begspn = ctx.mdldat.begspn.data();
    const int nspobs = ctx.mdldat.nspobs;

    // Span offset within the full input series (getsrs guarantees coverage).
    int offset = 0;
    dfdate(begspn, begsrs, sp, offset);
    const double* aptr = ctx.arima.y.data() + offset;   // 1-based over the span

    // editor.f 224-233: forecast/backcast pointer bookkeeping. No model here, so
    // Nfcst = Nbcst = 0 and the padded buffer collapses to the observed span.
    const int frstsy = offset + 1;
    ctx.arima.frstsy = frstsy;
    ctx.arima.nomnfy = ctx.arima.nobs - frstsy + 1;
    int nfcst = ctx.extend.nfcst;
    if (nfcst < 0) nfcst = 0;                            // NOTSET -> 0 defensive
    int nbcst = ctx.extend.nbcst;
    if (nbcst < 0) nbcst = 0;
    ctx.extend.nfcst = nfcst;
    ctx.extend.nbcst = nbcst;
    ctx.extend.nbcst2 = 0;
    const bool lsadj = true;                             // Lx11
    const int fctdrp = ctx.arima.fctdrp;
    int nfdrp = nfcst;
    if (!lsadj && fctdrp > 0) nfdrp = std::max(0, nfcst - fctdrp);
    ctx.extend.nfdrp = nfdrp;
    ctx.extend.nobspf = std::min(nspobs + nfdrp, ctx.arima.nomnfy);
    ctx.extend.nofpob = nspobs + nfdrp;
    ctx.extend.nbfpob = nspobs + nfdrp + nbcst;
    ctx.lzero.lsp = 1;

    // editor.f:150 Ny=Sp ; editor.f:1486 Kersa=0 (set under IF(Lx11)).
    ctx.x11opt.ny = sp;
    ctx.xtrm.kersa = 0;
    // gtinpt.f: Cnstnt defaults to DNOTST (no user constant). x11pt3 keys its
    // constant-removal branch on Cnstnt != DNOTST, so the zero-init default must
    // be corrected or the base path wrongly enters that (unported) branch.
    ctx.adj.cnstnt = prm::DNOTST;

    // editor.f 2042-2103: X-11 seasonal-filter default resolution (the post-parse
    // setup getx11 leaves to the editor). Base (type!=trend): an unset Lterm ->
    // 6 (auto MSR) with Lter(1..Ny)=6; the stable/3x5 flags follow Lterm; Lmsr=6
    // when Lterm==6. Only the base (no seasonalma) branch is reproduced.
    ctx.work2.lstabl = false;
    ctx.work2.l3x5 = false;
    if (ctx.x11opt.lterm == prm::NOTSET) {
        ctx.x11opt.lterm = 6;
        for (int i = 1; i <= sp; ++i) ctx.x11opt.lter(i) = 6;
    }
    if (ctx.x11opt.lterm == 5) ctx.work2.lstabl = true;
    if (ctx.x11opt.lterm == 2 || ctx.x11opt.lterm == 0) ctx.work2.l3x5 = true;
    ctx.x11opt.lmsr = (ctx.x11opt.lterm == 6) ? 6 : 0;

    // editor.f 2130-2135: initial trend-cycle I/C ratio (Tic) default. An unset
    // Tic (0) resolves from the preselected Henderson length Ktcopt; base
    // (Ktcopt=0, monthly) -> 3.5, the standard 13-term I/C. hndtrn's end filter
    // divides by Tic^2 (rbeta = 4/(Tic^2*pi)), so a 0 here would NaN the trend.
    if (ctx.x11opt.tic == 0.0) {
        ctx.x11opt.tic = 3.5;
        const int ktc = ctx.x11opt.ktcopt;
        if (ktc <= 9 && ktc > 0) ctx.x11opt.tic = 1.0;
        if (ktc > 13) ctx.x11opt.tic = 4.5;
        if (ktc <= 5 && sp == 4) ctx.x11opt.tic = 0.001;
        if (ktc >= 7 && sp == 4) ctx.x11opt.tic = 4.5;
    }

    // Span pointers (setxpt.f). Base no-model: Pos1bk = Pos1ob = 1,
    // Posfob = Posffc = Nspobs.
    setxpt(ctx, nfdrp, lsadj, fctdrp);
    const int pos1ob = ctx.x11ptr.pos1ob;

    // Populate the X-11 input buffers. x11pt1 reads Series (-> Stcsi/Stoap/Stopp/
    // Stocal) and Orig (-> Sto), both 1-based starting at Pos1ob. With no model
    // and no transform both are the raw series; Orig must carry Nomnfy points
    // (x11pt1 copies Orig over [Pos1ob, Pos1ob+Nomnfy-1] into Sto).
    const int norig = ctx.arima.nomnfy;
    for (int i = 0; i < nspobs; ++i) ctx.inpt.series(pos1ob + i) = aptr[i];
    for (int i = 0; i < norig; ++i)  ctx.inpt.orig(pos1ob + i) = aptr[i];

    // X-11 array initialization (x11int.f), then the parts spine.
    x11int(ctx);

    const bool lmodel = has_model, lgraf = false, lgrfxr = false, lseats = false;
    const bool lx11 = true;
    x11pt1(ctx, lmodel, lgraf, lgrfxr);
    if (ctx.error.lfatal) return false;

    // Model path only (arima.f tail 1225-1332): forecast-extend the transformed
    // series into orix, then adjreg subtracts the regression effects (regeff
    // fills them by type), inverse-transforms to the original scale, and fills
    // Stcsi (the B1 input), the Series forecast tail, and Stocal.
    if (has_model) {
        // chkadj (arima.f:1256): set the Adj*/count indicators from the estimated
        // model's regressor types, so regeff/adjreg remove the effects present.
        int ntd = 0;
        chkadj(ctx, ntd, ctx.x11opt.khol, lseats, ctx.arima.lam);
        constexpr int PLEN = 1020;
        std::vector<double> orix(PLEN, 0.0), orixmv(PLEN, 0.0), orixot(PLEN, 0.0);
        std::vector<double> ftd(PLEN, 0.0), fao(PLEN, 0.0), fls(PLEN, 0.0),
            ftc(PLEN, 0.0), fso(PLEN, 0.0), fsea(PLEN, 0.0), fcyc(PLEN, 0.0),
            fusr(PLEN, 0.0), fmv(PLEN, 0.0), fhol(PLEN, 0.0);
        const double lam = ctx.arima.lam;
        const int fcntyp = ctx.arima.fcntyp;
        bool extok = true;
        double bcstx = 0.0;   // Nbcst==0 on this path
        if ((nfcst > 0 && nfdrp > 0) || nbcst > 0) {
            extend(ctx, trnsrs.data(), ctx.arima.begxy.data(), orix.data(), extok,
                   lam, ctx.forecasts.trnfct.data(), &bcstx);
            if (ctx.error.lfatal) return false;
        } else {
            copy(trnsrs.data(), ctx.extend.nobspf, 1, orix.data() + (pos1ob - 1));
        }
        // arima.f:1283: rebuild the regression design over the forecast-extended
        // orix. rgarma differenced Xy in place during estimation; regeff needs the
        // un-differenced calendar columns. Only needed when regression effects are
        // present (some Adj*==1); harmless otherwise.
        const x11adj_cmn& adj = ctx.x11adj;
        const bool have_eff = adj.adjtd == 1 || adj.adjhol == 1 || adj.adjao == 1 ||
                              adj.adjls == 1 || adj.adjtc == 1 || adj.adjso == 1 ||
                              adj.adjsea == 1 || adj.adjcyc == 1 || adj.adjusr == 1 ||
                              adj.finhol || adj.finao || adj.finls || adj.fintc ||
                              adj.finusr;
        if (have_eff) {
            int nrxy2 = 0, frstry2 = 0;
            regvar(ctx, orix.data(), ctx.arima.nrxy, fctdrp, nfcst, nbcst,
                   ctx.arima.userx.data(), ctx.arima.bgusrx.data(),
                   ctx.arima.nrusrx, ctx.prior.priadj, ctx.arima.reglom, nrxy2,
                   ctx.arima.begxy.data(), frstry2, true, ctx.arima.elong);
            if (ctx.error.lfatal) return false;
            ctx.arima.nrxy = nrxy2;
            regeff(ctx, ctx.arima.nrxy, ftd.data(), fhol.data(), fao.data(),
                   fls.data(), ftc.data(), fso.data(), fsea.data(), fcyc.data(),
                   fusr.data(), fmv.data(), lseats);
        }
        int n = 0;
        adjreg(ctx, orix.data(), orixmv.data(), orixot.data(), ftd.data(),
               fao.data(), fls.data(), ftc.data(), fso.data(), fsea.data(),
               fcyc.data(), fusr.data(), fmv.data(), fhol.data(), fcntyp, lam,
               ctx.arima.nrxy, n);
        if (ctx.error.lfatal) return false;
    }

    x11pt2(ctx, lmodel, lx11, lseats, lgraf, lgrfxr);
    if (ctx.error.lfatal) return false;
    // x11pt3 (D8..D16 finals): D10=Sts, D11=Stci, D12=Stc, D13=Sti. It consumes
    // the D7-return state x11pt2 leaves. (D8/D9 read x11pt2's /mq10/ Stex, which
    // is function-local in this port and does not persist -- the D10..D13/D16
    // finals do not depend on it, so they gate correctly regardless.)
    x11pt3(ctx, lgraf, /*lttc=*/false);
    if (ctx.error.lfatal) return false;

    return !ctx.error.lfatal;
}

}  // namespace x13
