// run_x11_span.cpp -- see hpp for the design note. Body mirrors run_x11.cpp's
// model-path tail (arima.f's forecast-extend/adjreg glue + the x11int/x11pt1/
// x11pt2/x11pt3 spine), parameterized by an arbitrary sub-span window instead
// of the ctx-global main-run span, and replaying rgarma over that window with
// the model held fixed (ctx.model.arimaf) instead of skipping estimation.
#include "driver/run_x11_span.hpp"

#include "common/x13context.hpp"
#include "specparse/specparse.hpp"   // dfdate, addate, copy
#include "regarima/estimate.hpp"     // rgarma, prlkhd
#include "regarima/forecast.hpp"     // fcstout
#include "regarima/regvar.hpp"       // regvar
#include "x11/x11parts.hpp"          // x11pt1, x11pt2, x11pt3
#include "x11/x11drv.hpp"            // setxpt, x11int, chkadj, extend, adjreg, regeff
#include "gen/notset.hpp"            // prm::NOTSET, prm::DNOTST
#include "gen/srslen.hpp"            // prm::PLEN
#include "gen/model.hpp"             // prm::PORDER

#include <algorithm>

namespace x13 {

bool run_x11_span(X13Context& ctx, const std::vector<double>& trnsrs_full,
                   bool has_model, int nlen, int nfcst, int nbcst, int nbcst2,
                   int lsp) {
    const int sp = ctx.model.sp;
    const int* begsrs = ctx.arima.begsrs.data();

    ctx.mdldat.nspobs = nlen;
    ctx.extend.nfcst = nfcst;
    ctx.extend.nbcst = nbcst;
    ctx.extend.nbcst2 = nbcst2;
    const bool lsadj = true;   // Lx11
    const int fctdrp = ctx.arima.fctdrp;
    int nfdrp = nfcst;
    if (!lsadj && fctdrp > 0) nfdrp = std::max(0, nfcst - fctdrp);
    ctx.extend.nfdrp = nfdrp;
    ctx.extend.nofpob = nlen + nfdrp;
    ctx.extend.nbfpob = nlen + nfdrp + nbcst;
    ctx.lzero.lsp = lsp;

    ctx.x11opt.ny = sp;
    ctx.xtrm.kersa = 0;
    // Each sub-span is an independent adjustment: start xtrm.Ksdev from the
    // parsed spec/default (captured by ssprep_snapshot before the main run
    // mutated it), NOT the main run's evolved value. Leaking the latter flips
    // entsch's kersa/ksdev decode (k=Kersa1+Ksdev1+1), selecting a different
    // extreme-value sigma mode -> wrong extreme weights -> wrong seasonal MA
    // and Henderson-length choice (the ~3.45% span-1 sfs error).
    ctx.xtrm.ksdev = ctx.saved.ksdev0;
    // (Cnstnt is a COMMON in the oracle and survives every span replay, so it is
    // deliberately NOT reset here.)

    // Span pointers (setxpt.f) -- purely from Lsp/Nbcst2/Nbcst/Nfcst/Nofpob,
    // no calendar dependency yet.
    setxpt(ctx, nfdrp, lsadj, fctdrp);
    const int pos1ob = ctx.x11ptr.pos1ob;
    const int posfob = ctx.x11ptr.posfob;
    ctx.adj.setpri = ctx.x11ptr.pos1bk;

    // ssx11a.f: derive this span's calendar Begspn/Endspn from the JUST-SET
    // Pos1ob/Posfob (via the ctx-global Lyr/Ny calendar anchor) + the
    // setssp-computed constant Im (ctx.ssap.im). Begmdl/Endmdl mirror
    // Begspn/Endspn (no modelspan{} narrowing in this port's scope).
    const int ny = ctx.x11opt.ny;
    const int lyr = ctx.x11opt.lyr;
    const int im = ctx.ssap.im;
    int ly0 = lyr + pos1ob / ny;
    if (pos1ob % ny == 0) ly0 -= 1;
    int lstyr = lyr + posfob / ny;
    if (posfob % ny == 0) lstyr -= 1;
    int begspn[2] = {ly0, im};
    int endmo = posfob % ny;
    if (endmo == 0) endmo = ny;
    int endspn[2] = {lstyr, endmo};
    ctx.mdldat.begspn(1) = begspn[0];
    ctx.mdldat.begspn(2) = begspn[1];
    ctx.arima.endspn(1) = endspn[0];
    ctx.arima.endspn(2) = endspn[1];
    ctx.arima.begmdl(1) = begspn[0];
    ctx.arima.begmdl(2) = begspn[1];
    ctx.arima.endmdl(1) = endspn[0];
    ctx.arima.endmdl(2) = endspn[1];

    int offset = 0;
    dfdate(begspn, begsrs, sp, offset);   // 0-based offset into the raw series
    const int frstsy = offset + 1;
    ctx.arima.frstsy = frstsy;
    ctx.arima.nomnfy = ctx.arima.nobs - frstsy + 1;
    const double* aptr = ctx.arima.y.data() + offset;   // 1-based over the span
    ctx.extend.nobspf = std::min(nlen + nfdrp, ctx.arima.nomnfy);

    const int norig = ctx.arima.nomnfy;
    for (int i = 0; i < nlen; ++i) ctx.inpt.series(pos1ob + i) = aptr[i];
    for (int i = 0; i < norig; ++i) ctx.inpt.orig(pos1ob + i) = aptr[i];

    x11int(ctx);

    const bool lmodel = has_model, lgraf = false, lgrfxr = false, lseats = false;
    const bool lx11 = true;
    x11pt1(ctx, lmodel, lgraf, lgrfxr);
    if (ctx.error.lfatal) return false;

    if (has_model) {
        // Model-coefficient replay: rgarma with the model held fixed
        // (ctx.model.arimaf, set by the caller before the span loop -- see
        // slidingspans.cpp's ssmdl_fix_model) recomputes residuals/likelihood
        // over just this span's window (Nspobs==nlen); Nestpm==0 means it
        // takes exactly one pass and never perturbs ctx.mdldat.arimap.
        const double* trnsrs_span = trnsrs_full.data() + offset;
        int nrxy = 0, frstry = 0;
        regvar(ctx, trnsrs_span, ctx.extend.nobspf, fctdrp, nfcst, 0,
               ctx.arima.userx.data(), ctx.arima.bgusrx.data(),
               ctx.arima.nrusrx, ctx.prior.priadj, ctx.arima.reglom, nrxy,
               ctx.arima.begxy.data(), frstry, true, ctx.arima.elong);
        if (ctx.error.lfatal) return false;
        ctx.arima.nrxy = nrxy;

        constexpr int PA = prm::PLEN + 2 * prm::PORDER;
        std::vector<double> a(static_cast<std::size_t>(PA));
        int na = 0, nefobs = 0;
        bool lauto = false;
        rgarma(ctx, /*lestim=*/true, ctx.arima.mxiter, ctx.arima.mxnlit,
               /*lprtit=*/false, a.data(), na, nefobs, lauto);
        if (ctx.error.lfatal) return false;
        (void)na;
        (void)nefobs;

        // arima.f:742 -- the span's likelihood statistics (Olkhd/Aicc/...). The
        // oracle calls prlkhd on every estimation pass, and history{
        // estimates=(aic)} reads exactly this. It OVERWRITES the main run's
        // /lkhd/, which is why run_x11.cpp save/restores it around the span
        // drivers (the oracle writes its own .udg before revdrv runs).
        prlkhd(ctx, aptr, &ctx.adj.adj(ctx.adj.adj1st), ctx.adj.adjmod,
               ctx.arima.fcntyp, ctx.arima.lam);
        if (ctx.error.lfatal) return false;

        if (nfcst > 0) {
            fcstout(ctx, nfcst, ctx.arima.fctdrp, ctx.arima.ciprob,
                    ctx.arima.lognrm);
            if (ctx.error.lfatal) return false;
        }

        // arima.f tail (forecast-extend -> regression-effect removal ->
        // inverse-transform), identical structure to run_x11.cpp's model path.
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
        double bcstx = 0.0;
        // trnsrs_span is a const pointer into the caller-owned full buffer;
        // extend() wants a mutable double* (it only reads the observed part
        // and writes the padded orix output), so hand it a local copy of the
        // span's own nobspf-length window instead of a const_cast.
        std::vector<double> trnsrs_local(trnsrs_span,
                                          trnsrs_span + ctx.extend.nobspf);
        if ((nfcst > 0 && nfdrp > 0) || nbcst > 0) {
            extend(ctx, trnsrs_local.data(), ctx.arima.begxy.data(),
                   orix.data(), extok, lam, ctx.forecasts.trnfct.data(),
                   &bcstx);
            if (ctx.error.lfatal) return false;
        } else {
            copy(trnsrs_local.data(), ctx.extend.nobspf, 1,
                 orix.data() + (pos1ob - 1));
        }
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
        const int posfob_b1 = ctx.x11ptr.posfob;
        copy(ctx.orisrs.stcsi.data() + (pos1ob - 1), posfob_b1 - pos1ob + 1, 1,
             ctx.orisrs.stoap.data() + (pos1ob - 1));
    }

    x11pt2(ctx, lmodel, lx11, lseats, lgraf, lgrfxr);
    if (ctx.error.lfatal) return false;
    x11pt3(ctx, lgraf, /*lttc=*/false);
    if (ctx.error.lfatal) return false;

    return !ctx.error.lfatal;
}

}  // namespace x13
