// xrgdrv.cpp -- see xrgdrv.hpp. Self-contained transparent-SA prior-TD estimator
// for x11regression{} with a regARIMA model (Ixreg>=2). Mirrors xrgdrv.f, reusing
// run_x11.cpp's no-model span/pointer/buffer/x11int setup (Nfcst==Nbcst==0) and
// the ported x11pt1/x11pt2/x11mdl_td + loadxr swap.
#include "x11/xrgdrv.hpp"

#include "common/x13context.hpp"
#include "specparse/specparse.hpp"   // dfdate, addate
#include "x11/x11parts.hpp"          // x11pt1, x11pt2
#include "x11/x11drv.hpp"            // setxpt, x11int
#include "x11/loadxr.hpp"            // loadxr
#include "x11/slidingspans.hpp"      // ssprep_snapshot, restor_span
#include "gen/notset.hpp"            // prm::NOTSET, prm::DNOTST
#include "gen/srslen.hpp"            // prm::PLEN

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace x13 {

// Local clean-fatal (x11parts.cpp's x11_not_ported is TU-local).
static void xrg_not_ported(X13Context& ctx, const char* what) {
    errhdr(ctx);
    writln(ctx, std::string("ERROR: ") + what + " not yet ported (M5 X-11 spine).",
           stdio::STDERR, ctx.units.mt2, true);
    abend(ctx);
}

// Guard: only the TD-only, multiplicative, no-user-regressor, no-x11reg-span,
// no-classic-Easter path is ported. Anything else fatals cleanly (matching the
// old x11pt1 not_ported behaviour, just relocated to the estimation front).
static bool xrgdrv_supported(X13Context& ctx) {
    return ctx.hiddn.ixreg >= 2 && ctx.x11log.axrgtd &&
           ctx.x11opt.muladd == 0 && !ctx.x11msc.psuadd &&
           ctx.x11opt.khol != 1 && ctx.usrreg.ncusrx == 0 &&
           ctx.x11reg.xdsp == 0;
}

bool xrgdrv(X13Context& ctx, bool span_mode) {
    if (!(ctx.hiddn.ixreg >= 2 && ctx.x11log.axrgtd)) return true;  // no-op
    if (!xrgdrv_supported(ctx)) {
        xrg_not_ported(ctx, "xrgdrv OLS prior trading-day (Ixreg>=2): only the "
                            "TD-only multiplicative path is ported");
        return false;
    }

    const int sp = ctx.model.sp;
    const int* begsrs = ctx.arima.begsrs.data();
    const int* begspn = ctx.mdldat.begspn.data();
    const int nspobs = ctx.mdldat.nspobs;

    int offset = 0;
    dfdate(begspn, begsrs, sp, offset);
    const double* aptr = ctx.arima.y.data() + offset;   // 1-based over the span

    // --- xrgdrv.f:57-87: save SA/model state; zero the model Adj* indicators.
    // The oracle's ssprep here is harmless because revdrv/ssx11a call `restor`
    // immediately before x11ari, so this snapshot rewrites what it just read.
    // In this port ctx.saved additionally carries ksdev0/lterm0/nterm0, which
    // the span loop reads back to give each span a fresh filter/spread start --
    // so inside a span the whole snapshot is preserved rather than rewritten.
    const ssprep_cmn sv_ssprep = ctx.ssprep;
    const auto sv_saved = ctx.saved;   // anonymous struct type
    ssprep_snapshot(ctx);
    x11adj_cmn& adj = ctx.x11adj;
    const int sv_adjtd = adj.adjtd, sv_adjhol = adj.adjhol, sv_adjao = adj.adjao,
              sv_adjls = adj.adjls, sv_adjtc = adj.adjtc, sv_adjso = adj.adjso,
              sv_adjusr = adj.adjusr, sv_adjsea = adj.adjsea;
    if (adj.adjtd == 1) adj.adjtd = 0;
    if (adj.adjhol == 1) adj.adjhol = 0;
    if (adj.adjao == 1) adj.adjao = 0;
    if (adj.adjls == 1) adj.adjls = 0;
    if (adj.adjtc == 1) adj.adjtc = 0;
    if (adj.adjso == 1) adj.adjso = 0;
    if (adj.adjusr == 1) adj.adjusr = 0;
    if (adj.adjsea == 1) adj.adjsea = 0;

    // --- xrgdrv.f:93-125: suppress the length-of-month prior when TD is chosen in
    // the regression spec (reset Sprior to the base, no user prior on this path).
    const bool sv_picktd = ctx.picktd.picktd;
    const int sv_priadj = ctx.prior.priadj;
    const int sv_kfmt = ctx.prior.kfmt;
    std::vector<double> sv_sprior;
    bool pktd = false;
    if (ctx.picktd.picktd && ctx.prior.priadj > 1) {
        pktd = true;
        ctx.picktd.picktd = false;
        ctx.prior.priadj = 0;
        sv_sprior.assign(ctx.inpt.sprior.data(),
                         ctx.inpt.sprior.data() + prm::PLEN);
        // Muladd==0 (mult) base = 1.0; Nprtyp==0 here (no user prior on this path)
        // -> setdp(base, Sprior), Kfmt=0.
        for (int i = 0; i < prm::PLEN; ++i) ctx.inpt.sprior(i + 1) = 1.0;
        ctx.prior.kfmt = 0;
    }

    // Save the X-11 seasonal-filter selector (Lterm) and the Bundesbank extreme-
    // value spread control (Ksdev) so the main run re-derives them fresh. The
    // transparent pass's setup sets Lterm NOTSET->6 and its x11pt2 MSR selection
    // overwrites Lter + the Bundesbank vtest/entsch sets Ksdev; without restoring,
    // the main run inherits both (run_x11's editor re-resolves Lter only when
    // Lterm==NOTSET, and x11pt2's Bundesbank test re-derives Ksdev only when
    // Ksdev<4), leaking the transparent decomposition's filter/spread into the
    // final seasonal factors (~2% off, worst at the low-amplitude series start).
    // Mirrors restor.f restoring Lter (via Lterm re-resolution here) and the
    // slidingspans/history per-span Ksdev reset. Ktcopt/Tic are unchanged (same
    // default value in both passes), so they need no explicit save.
    const int sv_lterm = ctx.x11opt.lterm;
    const int sv_ksdev = ctx.xtrm.ksdev;

    // --- xrgdrv.f:129: load the x11regression TD design into the working model.
    loadxr(ctx, /*toxreg=*/false);

    // --- xrgdrv.f:131-159. Two shapes, because this stands in for two call
    // sites (see hpp). The oracle only ever has the span-mode shape: it zeroes
    // Nfcst/Nbcst in place and NUDGES the four derived pointers, keeping the
    // Pos1ob/Posfob its caller already set. The hoisted main-run call has no
    // caller to have set them, so it builds the span from scratch instead.
    extend_cmn& ext = ctx.extend;
    const int sv_nfcst = ext.nfcst, sv_nbcst = ext.nbcst;
    const int sv_nbcst2 = ext.nbcst2, sv_nfdrp = ext.nfdrp;
    const int sv_nobspf = ext.nobspf;
    const int sv_nofpob = ext.nofpob, sv_nbfpob = ext.nbfpob;
    const int sv_lsp = ctx.lzero.lsp;
    const int sv_pos1bk = ctx.x11ptr.pos1bk, sv_posffc = ctx.x11ptr.posffc;
    const int sv_setpri = ctx.adj.setpri;
    const int sv_nterm = ctx.x11opt.nterm;
    const int sv_lmsr = ctx.x11opt.lmsr;
    const int sv_kersa = ctx.xtrm.kersa;
    const bool sv_lstabl = ctx.work2.lstabl, sv_l3x5 = ctx.work2.l3x5;
    if (!span_mode) {
        ctx.arima.frstsy = offset + 1;
        ctx.arima.nomnfy = ctx.arima.nobs - ctx.arima.frstsy + 1;
    }
    ext.nfcst = 0;
    ext.nbcst = 0;
    ext.nbcst2 = 0;
    ext.nfdrp = 0;
    ext.nobspf = std::min(nspobs, ctx.arima.nomnfy);
    ext.nofpob = nspobs;
    ext.nbfpob = nspobs;
    if (!span_mode) ctx.lzero.lsp = 1;

    ctx.x11opt.ny = sp;
    if (!span_mode) ctx.x11opt.lyr = begspn[0];
    ctx.xtrm.kersa = 0;
    // (Cnstnt is a COMMON and survives the transparent pass; do not clear it.)

    // editor.f 2042-2103 / 2130-2135 -- the seasonal-filter and trend I/C-ratio
    // defaults. Only the hoisted call needs them: editor runs once, at parse,
    // and in a span replay `restor` (restor_span) has already reinstated Lter/
    // Ktcopt/Tic from the main run's snapshot and the caller has reset Lterm.
    if (!span_mode) {
        ctx.work2.lstabl = false;
        ctx.work2.l3x5 = false;
        if (ctx.x11opt.lterm == prm::NOTSET) {
            ctx.x11opt.lterm = 6;
            for (int i = 1; i <= sp; ++i) ctx.x11opt.lter(i) = 6;
        }
        if (ctx.x11opt.lterm == 5) ctx.work2.lstabl = true;
        if (ctx.x11opt.lterm == 2 || ctx.x11opt.lterm == 0) ctx.work2.l3x5 = true;
        ctx.x11opt.lmsr = (ctx.x11opt.lterm == 6) ? 6 : 0;

        if (ctx.x11opt.tic == 0.0) {
            ctx.x11opt.tic = 3.5;
            const int ktc = ctx.x11opt.ktcopt;
            if (ktc <= 9 && ktc > 0) ctx.x11opt.tic = 1.0;
            if (ktc > 13) ctx.x11opt.tic = 4.5;
            if (ktc <= 5 && sp == 4) ctx.x11opt.tic = 0.001;
            if (ktc >= 7 && sp == 4) ctx.x11opt.tic = 4.5;
        }
    }

    const bool lsadj = true;
    const int fctdrp = ctx.arima.fctdrp;
    if (span_mode) {
        // xrgdrv.f:143-146 -- with Nfcst/Nbcst now zero the derived pointers
        // collapse onto the observed span; Pos1ob/Posfob are the caller's and
        // are deliberately NOT recomputed (the oracle never calls setxpt here).
        ctx.x11ptr.pos1bk = ctx.x11ptr.pos1ob;
        ctx.x11ptr.posffc = ctx.x11ptr.posfob;
    } else {
        setxpt(ctx, /*nfdrp=*/0, lsadj, fctdrp);
    }
    const int pos1ob = ctx.x11ptr.pos1ob;
    const int posfob = ctx.x11ptr.posfob;
    ctx.adj.setpri = ctx.x11ptr.pos1bk;

    // Populate the X-11 input buffers from the raw span (no model, no transform-
    // extension: the transparent pass runs on the observed series only). In span
    // mode the caller has already done exactly this, as ssx11a/revdrv do before
    // x11ari -- refilling would be a no-op at best and would use a different
    // Orig length at worst.
    if (!span_mode) {
        const int norig = std::min(nspobs, ctx.arima.nomnfy);
        for (int i = 0; i < nspobs; ++i) ctx.inpt.series(pos1ob + i) = aptr[i];
        for (int i = 0; i < norig; ++i)  ctx.inpt.orig(pos1ob + i) = aptr[i];
    }
    (void)aptr;

    // x11int initializes /x11fac/ (Faccal=base) and the working arrays for this
    // transparent decomposition. xrgdrv.f:163-165: x11pt1 + x11pt2 (Lx11=T); the
    // x11pt2 x11mdl at Kpart=2/3 estimates the TD and folds it into Faccal.
    x11int(ctx);
    x11pt1(ctx, /*lmodel=*/false, false, false);
    if (!ctx.error.lfatal) x11pt2(ctx, false, /*lx11=*/true, false, false, false);

    // --- xrgdrv.f:206-233: restore model/SA state; Ixreg=3 for the main run.
    // loadxr(T) saves the working model (with the swapped-in TD) back into
    // ctx.xrgmdl; restor (xrgdrv.f:207) then restores the BARE regARIMA model so
    // the subsequent ML estimate does NOT fit the x11regression TD columns. The
    // C++ restor_span only resets the x11 filter state, so clear the working
    // regressors explicitly (mirrors the parse-time dlrgef path) -- otherwise the
    // 6 TD columns leak into the ML design (ncxy 1->7, wrong ARMA estimate).
    loadxr(ctx, /*toxreg=*/true);
    xrg_clear_working(ctx);
    restor_span(ctx);
    if (pktd) {
        ctx.picktd.picktd = true;
        ctx.prior.priadj = sv_priadj;
        ctx.prior.kfmt = sv_kfmt;
        std::copy(sv_sprior.begin(), sv_sprior.end(), ctx.inpt.sprior.data());
    }
    adj.adjtd = sv_adjtd; adj.adjhol = sv_adjhol; adj.adjao = sv_adjao;
    adj.adjls = sv_adjls; adj.adjtc = sv_adjtc; adj.adjso = sv_adjso;
    adj.adjusr = sv_adjusr; adj.adjsea = sv_adjsea;
    ctx.picktd.picktd = sv_picktd;   // xrgdrv.f:211-217 (pktd branch already set)
    ext.nfcst = sv_nfcst;
    ext.nbcst = sv_nbcst;

    // Restore Lterm (-> main run re-resolves Lter/Lmsr/Lstabl/L3x5 in editor.f's
    // 2042-2103 block) and Ksdev (-> main run re-runs the Bundesbank spread test).
    ctx.x11opt.lterm = sv_lterm;
    ctx.xtrm.ksdev = sv_ksdev;

    if (span_mode) {
        // xrgdrv.f:167-178 puts Nfcst/Nbcst/Nfdrp and the four derived pointers
        // back so the caller's x11pt1/arima/x11pt2 see the span it set up. The
        // Fortran's other state is restored by COMMONs it never wrote; this port
        // writes some of them, so they are undone here.
        ext.nbcst2 = sv_nbcst2;
        ext.nfdrp = sv_nfdrp;
        ext.nobspf = sv_nobspf;
        ext.nofpob = sv_nofpob;
        ext.nbfpob = sv_nbfpob;
        ctx.lzero.lsp = sv_lsp;
        ctx.x11ptr.pos1bk = sv_pos1bk;
        ctx.x11ptr.posffc = sv_posffc;
        ctx.adj.setpri = sv_setpri;
        // The transparent pass's x11pt2/vtc resolves the Henderson length in
        // place. Every span must re-select it from its own I/C ratio, exactly as
        // Lterm above, so restore what the caller had set.
        ctx.x11opt.nterm = sv_nterm;
        // Same class: the transparent pass's x11pt2 resolves the MSR selector,
        // the extreme-value mode and the stable/3x5 filter flags in place. On the
        // main path run_x11's editor block re-derives all of them AFTER this
        // returns; a span has no editor block, so hand them back.
        ctx.x11opt.lmsr = sv_lmsr;
        ctx.xtrm.kersa = sv_kersa;
        ctx.work2.lstabl = sv_lstabl;
        ctx.work2.l3x5 = sv_l3x5;
        ctx.ssprep = sv_ssprep;
        ctx.saved = sv_saved;
    }

    if (ctx.error.lfatal) return false;

    // Stash the estimated Faccal over the FORECAST-EXTENDED span for (a) the
    // pre-model estimation-input divide (run_pre_model, observed only) and (b) the
    // main x11pt1 Ixreg==3 restore + x11pt3 D11/D16 fold, which run over
    // [Pos1ob,Posffc] (main x11int wipes /x11fac/, so it is restored from here).
    // x11mdl_td (Kpart==3) filled Faccal to Posfob+Nfcstx; carry that whole span.
    const int nfcstx = ctx.xrgfct.nfcstx > 0 ? ctx.xrgfct.nfcstx : 0;
    const int stash_end = posfob + nfcstx;
    ctx.x11_faccal_prior.assign(ctx.x11fac.faccal.data() + (pos1ob - 1),
                                ctx.x11fac.faccal.data() + (pos1ob - 1) + (stash_end - pos1ob + 1));

    ctx.hiddn.ixreg = 3;   // xrgdrv.f:233
    return true;
}

}  // namespace x13
