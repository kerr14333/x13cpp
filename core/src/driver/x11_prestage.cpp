// x11_prestage.cpp -- see hpp. Body moved verbatim out of run_x11.cpp (the
// editor.f span/filter setup -> setxpt -> x11int -> x11pt1 -> x11pt2 block) so
// run_seats can take the same path the oracle's single x11ari entry does.
#include "driver/x11_prestage.hpp"

#include "common/x13context.hpp"
#include "specparse/specparse.hpp"
#include "x11/x11parts.hpp"       // x11pt1, x11pt2, x11pt3
#include "x11/x11drv.hpp"         // setxpt, x11int, chkadj, regeff, extend, adjreg
#include "x11/slidingspans.hpp"   // ssprep_snapshot, restor_span
#include "x11/x11easter.hpp"      // holday (classic X-11 Easter estimation)
#include "regarima/regvar.hpp"    // regvar (design rebuild for regression effects)
#include "regarima/priadj.hpp"    // adjsrs_factors (prior-adjustment factor series)
#include "composite/agr3.hpp"     // agrxpt (composite total buffer geometry)
#include "notset.hpp"             // prm::NOTSET

#include <algorithm>
#include <string>
#include <vector>

namespace x13 {

// xrgdrv.f (Khol==1 branch): estimate the classic X-11 Easter holiday factor.
// Runs a preliminary, model-free, no-forecast/backcast transparent X-11
// decomposition to get the irregular Sti, then holday() fits the Easter effect
// into X11hol and flips Khol to 2 so the main pass folds it as a prior. The
// polluted extension/pointer/filter state is restored from the ssprep snapshot
// (taken just before this call), so the main run starts clean.
static void x11_easter_prepass(X13Context& ctx) {
    extend_cmn& ext = ctx.extend;
    x11ptr_cmn& ptr = ctx.x11ptr;
    const int nfcst0 = ext.nfcst, nbcst0 = ext.nbcst;
    const int nspobs = ctx.mdldat.nspobs;

    // xrgdrv.f:134-150 -- zero forecasts/backcasts for the transparent pass, but
    // only when there are any (Xdsp stays 0: the Ixreg x11regression prior-span
    // path that moves it is unported, so the classic x11easter route never does).
    const bool haveext = (nfcst0 > 0 || nbcst0 > 0);
    if (haveext) {
        ext.nfcst = 0;
        ext.nbcst = 0;
        ext.nfdrp = 0;
        ext.nofpob = nspobs;
        ext.nbfpob = nspobs;
        ptr.pos1bk = ptr.pos1ob;
        ptr.posffc = ptr.posfob;
    }

    // No x11int here: the arrays were initialized once before this call, and
    // x11int would wipe X11hol/Faccal. x11pt1 re-populates the working buffers
    // from Series each pass, so the transparent decomposition is clean.
    x11pt1(ctx, /*lmodel=*/false, false, false);
    if (!ctx.error.lfatal) x11pt2(ctx, false, /*lx11=*/true, false, false, false);
    if (!ctx.error.lfatal) x11pt3(ctx, false, /*lttc=*/false);
    if (ctx.error.lfatal) return;

    // xrgdrv.f:168-178 -- restore the extension + X-11 pointers BEFORE holday, so
    // the Easter span covers the full extended range (Pos1bk..Posfob+Nfcst) and
    // backcast factors start at Pos1bk, not Pos1ob. Verbatim oracle formulas.
    if (haveext) {
        ext.nfcst = nfcst0;
        ext.nbcst = nbcst0;
        ext.nfdrp = nfcst0;
        ext.nofpob = nspobs + nfcst0;
        ext.nbfpob = nspobs + nfcst0 + nbcst0;
        ptr.pos1bk = ptr.pos1ob - nbcst0;
        ptr.posffc = ptr.posfob + nfcst0;
    }

    // Begbak = Begspn shifted back Nbcst periods (editor.f:207) -- holidy uses
    // its year to index the Easter-date table. Not persisted in ctx, so derive
    // it here (mirrors the x11pt3 LOM-prior begbak recompute).
    addate(ctx.mdldat.begspn.data(), ctx.x11opt.ny, -nbcst0,
           ctx.extend.begbak.data());
    // xrgdrv.f:184 passes the restored Nfcst as Iforc; Xdsp stays 0 (no x11reg).
    holday(ctx, ctx.x11srs.sti.data(), /*iforc=*/nfcst0, /*xdsp=*/0);

    // restor.f (Lx11=T): reset only Lter/Ktcopt/Tic (+ model params). Ksdev/
    // Lterm/Nterm are deliberately NOT reset -- the oracle's main pass reuses
    // whatever the transparent pass left them, so matching that is required.
    restor_span(ctx);
}

bool x11_prestage(X13Context& ctx, bool has_model, std::vector<double>& trnsrs,
                  bool lseats) {
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
    // editor.f:206-219 -- Begbak is the backcast start (Begspn shifted back
    // Nbcst); when it does not land on period 1 the backcast count is PADDED to
    // the start of that year (Nbcst2 >= Nbcst) so the X-11 buffer always begins
    // on a full year. setxpt then places the span at Pos1ob = Nbcst2 + Lsp, so
    // Nbcst2 is exactly the left padding of the padded buffer. This was pinned
    // to 0, which put Pos1bk = -Nbcst + 1 -- a NEGATIVE buffer index. Every
    // backcast run (forecast{maxback=}) segfaulted on it.
    addate(begspn, sp, -nbcst, ctx.extend.begbak.data());
    if (ctx.extend.begbak(2) > 1) {
        ctx.extend.nbcst2 = nbcst + ctx.extend.begbak(2) - 1;
        ctx.extend.begbk2(2) = 1;
    } else {
        ctx.extend.nbcst2 = nbcst;
        ctx.extend.begbk2(2) = ctx.extend.begbak(2);
    }
    ctx.extend.begbk2(1) = ctx.extend.begbak(1);
    const bool lsadj = true;                             // x11ari.f:76 Lx11.or.Lseats
    const int fctdrp = ctx.arima.fctdrp;
    int nfdrp = nfcst;
    if (!lsadj && fctdrp > 0) nfdrp = std::max(0, nfcst - fctdrp);
    ctx.extend.nfdrp = nfdrp;
    ctx.extend.nobspf = std::min(nspobs + nfdrp, ctx.arima.nomnfy);
    ctx.extend.nofpob = nspobs + nfdrp;
    ctx.extend.nbfpob = nspobs + nfdrp + nbcst;
    ctx.lzero.lsp = 1;

    // editor.f:150 Ny=Sp ; editor.f:235 Lyr=Begspn(1) ; editor.f:1486 Kersa=0
    // (set under IF(Lx11)). Lyr (the calendar year of the analyzed span's
    // first observation) has no other writer in this port; slidingspans{}'s
    // setssp_span/run_x11_span (core/src/x11/slidingspans.hpp) need it to
    // convert padded-buffer positions back to calendar dates per span.
    ctx.x11opt.ny = sp;
    ctx.x11opt.lyr = begspn[0];
    // editor.f:236-237 Lstyr/Lstmo = Endspn(1)/Endspn(2), and editor.f:423-424
    // L0=1 / Ly0=Lyr. Together with Ny and Begspn(MO) these five are exactly the
    // Itest(1..5) span signature composite adjustment matches components on
    // (composite/agr2.cpp); slidingspans' replay driver computes its own
    // per-span values, so this is the main run's.
    {
        int endspn[2];
        addate(begspn, sp, nspobs - 1, endspn);
        ctx.x11opt.lstyr = endspn[0];
        ctx.x11opt.lstmo = endspn[1];
    }
    ctx.lzero.l0 = 1;
    ctx.lzero.ly0 = ctx.x11opt.lyr;
    ctx.xtrm.kersa = 0;
    // (Cnstnt is set by gtinpt/gt_transform now -- transform{constant=} -- and
    // must NOT be cleared here or the user constant would never reach x11pt3.)

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
    // editor.f:234 -- on the composite total's run, reconcile the direct and
    // indirect buffer geometries before anything reads the pointers.
    if (ctx.agr.iagr == 3) agrxpt(ctx, begspn, sp);

    // editor.f:2071-2097 -- the 3x15 seasonal filter (Lterm/Lter == 4) is NOT
    // used on a series shorter than twenty years: it silently becomes a STABLE
    // filter (5), and that sets Lstabl. Placed here (not with the rest of the
    // 2042-2103 block above) because it needs Posffc, which setxpt has only just
    // resolved -- editor.f computes its own `nyr` from Posffc at :688, long
    // before reaching :2071. Lstabl is what f3cal reads to decide whether M8-M11
    // exist at all, so without this the engine reported four quality statistics
    // the oracle suppresses.
    {
        int nyr = ctx.x11ptr.posffc / sp;
        if (ctx.x11ptr.posffc % sp > 0) nyr += 1;
        if (ctx.x11opt.lterm == 4 && nyr < 20) {
            ctx.x11opt.lterm = 5;
            ctx.work2.lstabl = true;
        }
        for (int i = 1; i <= sp; ++i) {
            if (ctx.x11opt.lter(i) == 4 && nyr < 20) {
                ctx.x11opt.lter(i) = 5;
                if (!ctx.work2.lstabl) ctx.work2.lstabl = true;
            } else if (!ctx.work2.lstabl && ctx.x11opt.lter(i) == 5) {
                ctx.work2.lstabl = true;
            }
            if (ctx.work2.l3x5 && ctx.x11opt.lter(i) != 2 && ctx.x11opt.lter(i) != 0)
                ctx.work2.l3x5 = false;
        }
    }

    const int pos1ob = ctx.x11ptr.pos1ob;
    // editor.f:851 Setpri=Pos1bk -- the 1-based start of the prior-adjustment span
    // in the padded buffer. Never set in the base port (no gated prior-adj spec);
    // required now that the aictest leap-year prior gives Nadj>0, so x11int and the
    // x11pt2 makadj/tdlom Sprior copies index correctly.
    ctx.adj.setpri = ctx.x11ptr.pos1bk;

    // (tdprior weight resolution / Kswv now happens at parse -- gtinpt.f:1484-1536
    // editor slice -- so both the pre-model estimation input and x11pt1 see the
    // standardized weights.)

    // Populate the X-11 input buffers. x11pt1 reads Series (-> Stcsi/Stoap/Stopp/
    // Stocal) and Orig (-> Sto), both 1-based starting at Pos1ob. With no model
    // and no transform both are the raw series; Orig must carry Nomnfy points
    // (x11pt1 copies Orig over [Pos1ob, Pos1ob+Nomnfy-1] into Sto).
    const int norig = ctx.arima.nomnfy;
    for (int i = 0; i < nspobs; ++i) ctx.inpt.series(pos1ob + i) = aptr[i];
    for (int i = 0; i < norig; ++i)  ctx.inpt.orig(pos1ob + i) = aptr[i];
    // editor.f:2492 -- Orig2 gets the same copy. It is the buffer composite
    // adjustment aggregates (agr2.f:267).
    for (int i = 0; i < norig; ++i)  ctx.inpt.orig2(pos1ob + i) = aptr[i];
    // editor.f:2500-2502 -- the "good observation" flags. Every observation in
    // the span is good unless pseudo-additive adjustment is on and the value is
    // non-positive. chkzro clears more of them later; sumry/varlog/divgud/issame
    // (x11pt4's Part-F measures) and change() are the consumers. Only [Pos1ob,
    // Posfob] is written, exactly as the oracle does.
    for (int i = pos1ob; i <= ctx.x11ptr.posfob; ++i) {
        ctx.goodob.gudval(i) = true;
        if (ctx.x11msc.psuadd && ctx.inpt.series(i) <= 0.0)
            ctx.goodob.gudval(i) = false;
    }

    // arima.f:1433-1441 -- overlay the UNTRANSFORMED forecasts (and backcasts)
    // onto Orig2's extension region. Only the composite path reads this far out:
    // it is what makes the aggregated O2/O5 (hence the indirect seasonal factors)
    // defined over the appended forecast span. Without it those rows aggregate
    // zeros. ctx.forecasts.fcst IS untfct (the original-scale point forecast).
    if (nfcst > 0 && !ctx.forecasts.fcst.empty()) {
        const int n = static_cast<int>(ctx.forecasts.fcst.size());
        for (int i = ctx.x11ptr.posfob + 1; i <= ctx.x11ptr.posffc; ++i) {
            const int k = i - ctx.x11ptr.posfob;      // 1-based into untfct
            if (k <= n) ctx.inpt.orig2(i) = ctx.forecasts.fcst[k - 1];
        }
    }

    // ssprep.f: snapshot the model's converged parameters + the (still
    // unresolved, sentinel==6) x11 seasonal-filter settings BEFORE the main
    // run's own x11int/x11pt2/x11pt3 potentially mutate/resolve Lter -- this
    // is what slidingspans{}'s restor_span() resets to before each span, so
    // it must run here (mirrors x12run.f's CALL ssprep(...) placement, right
    // before x11int), not later from run_slidingspans(). Cheap; harmless
    // when slidingspans{} was not requested.
    ssprep_snapshot(ctx);

    // adjsrs.f on the NO-MODEL path. With a model, run_pre_model builds the prior
    // factors and arima.f's chain (extend/adjreg -> Stcsi) carries the
    // prior-adjusted series into X-11; with no model nothing upstream runs, so
    // the /adjcmn/ record is built here and x11int (Adj -> Sprior) + x11pt1
    // (Sto /= Sprior, gated on Kfmt>=1) do the removal, exactly as the oracle
    // does on both paths. Without this the no-model run decomposed the RAW series
    // while x11pt3 still tried to remove a prior that had never been applied --
    // which indexed Usrpad at Frstap==0 and threw.
    // The /adjcmn/ RECORD itself is unconditional in adjsrs.f (:39-40, :79-80,
    // :108-109): with no prior, Adj is the mode identity and Sprior comes out
    // all-1 rather than the all-zero COMMON -- which is what x11pt3's sp2
    // writeback then multiplies the outlier/user factors into. Only Kfmt is
    // keyed to a prior actually existing.
    if (!has_model) {
        const bool has_pri = (ctx.prior.priadj > 1 || ctx.priusr.nuspad > 0 ||
                              ctx.priusr.nustad > 0);
        // adjsrs.f:39-40 -- Nadj spans the backcasts, the series, and at least a
        // full year of forecasts; Begadj is the span start shifted back Nbcst.
        const int nadj = std::min(
            nspobs + nbcst + std::max(sp, nfcst - fctdrp), 1020 - ctx.adj.setpri);
        std::vector<double> fac(static_cast<std::size_t>(nadj), 1.0);
        addate(begspn, sp, -nbcst, ctx.adj.begadj.data());
        if (!adjsrs_factors(ctx, ctx.adj.begadj.data(), sp, nadj,
                            /*suppress_predef=*/false, fac.data()))
            return false;
        for (int t = 0; t < nadj; ++t) ctx.adj.adj(t + 1) = fac[t];
        ctx.adj.nadj = nadj;
        dfdate(begspn, ctx.adj.begadj.data(), sp, ctx.adj.adj1st);  // adjsrs.f:108
        ctx.adj.adj1st += 1;
        if (has_pri)
            ctx.prior.kfmt = 1;  // adjsrs.f:62,101 -- there IS a prior to remove
    }

    // X-11 array initialization (x11int.f) -- once, before both the (optional)
    // Easter transparent pre-pass and the main spine, so X11hol/Faccal set by
    // the Easter estimation survive into the main pass's prior fold.
    x11int(ctx);

    // Editor step (editor.f:1909-1920 / gtinpt.f:1239): x11easter=yes (Keastr>=1)
    // with no user-mean irregular regression enables the classic X-11 Easter
    // adjustment -- Khol=1, Lgenx=T. Monthly only.
    if (ctx.x11opt.keastr >= 1 && !ctx.xrgum.haveum) {
        // editor.f:1920-1969 -- validate the classic X-11 Easter request; any
        // failure aborts (Readok=F). Quarterly, auto-transform, non-mult mode,
        // conflicting regARIMA/irregular-reg Easter, and pre-1901 start all fatal.
        bool ok = true;
        if (ctx.model.sp == 4) {
            writln(ctx,
                   "ERROR: Cannot calculate X-11 holiday adjustment for a "
                   "quarterly series.",
                   ctx.units.mt2, stdio::STDERR, true);
            ok = false;
        }
        if (ctx.arima.fcntyp == 0) {
            writln(ctx,
                   "ERROR: An X-11 holiday adjustment cannot be performed when "
                   "the automatic transformation selection option is chosen.",
                   ctx.units.mt2, stdio::STDERR, true);
            ok = false;
        } else if (ctx.x11opt.muladd > 0) {
            writln(ctx,
                   "ERROR: An X-11 holiday adjustment cannot be performed unless "
                   "the multiplicative seasonal adjustment option is chosen.",
                   ctx.units.mt2, stdio::STDERR, true);
            ok = false;
        }
        // editor.f:1940-1952 -- regARIMA model-based Easter (Easter/StatCanEaster/
        // StockEaster group) cannot coexist with X-11 Easter.
        if (ctx.x11adj.adjhol == 1) {
            const model_cmn& m = ctx.model;
            int rhol = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                              "Easter");
            if (rhol == 0)
                rhol = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                              "StatCanEaster");
            if (rhol == 0)
                rhol = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                              "StockEaster");
            if (rhol > 0) {
                writln(ctx,
                       "ERROR: X-11 and regARIMA model-based Easter adjustment "
                       "cannot be specified in the same run.",
                       ctx.units.mt2, stdio::STDERR, true);
                ok = false;
            }
        }
        // editor.f:1954-1960 -- irregular-component regression Easter conflict.
        if (ctx.x11reg.easgrp > 0 && ctx.x11log.axrghl) {
            writln(ctx,
                   "ERROR: X-11 and irregular component regression-based Easter "
                   "adjustment cannot be specified in the same run.",
                   ctx.units.mt2, stdio::STDERR, true);
            ok = false;
        }
        // editor.f:1961-1968 -- Easter date table starts in 1901.
        if (ctx.mdldat.begspn(1) < 1901) {
            writln(ctx, "ERROR: No X-11 holiday effect before 1901.",
                   ctx.units.mt2, stdio::STDERR, true);
            ok = false;
        }
        if (!ok) {
            ctx.error.lfatal = true;
            return false;
        }
        ctx.x11opt.khol = 1;
        ctx.xeastr.lgenx = true;
        x11_easter_prepass(ctx);
        if (ctx.error.lfatal) return false;
    }

    // (x11int already ran above.) The parts spine follows.
    const bool lmodel = has_model, lgraf = false, lgrfxr = false;
    const bool lx11 = !lseats;
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
        // arima.f:1227 passes bcstx, mkback's transformed backcasts (empty when
        // Nbcst==0; extend only reads it when Nbcst>0). Sized to PFCST so the
        // guard loop inside extend never reads past the end.
        constexpr int PFCST = 120;   // srslen.prm: 10*PSP
        std::vector<double> bcstx(static_cast<std::size_t>(PFCST), 0.0);
        for (std::size_t i = 0; i < ctx.forecasts.trnbct.size() && i < bcstx.size();
             ++i)
            bcstx[i] = ctx.forecasts.trnbct[i];
        if ((nfcst > 0 && nfdrp > 0) || nbcst > 0) {
            extend(ctx, trnsrs.data(), ctx.arima.begxy.data(), orix.data(), extok,
                   lam, ctx.forecasts.trnfct.data(), bcstx.data());
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
        // Snapshot the adjreg-adjusted B1 (Stcsi) for the b1 table: x11pt2
        // overwrites Stcsi in place during the C/D passes. Use Stoap -- a x11pt1
        // scratch buffer (original*prior) that is dead after x11pt1 and untouched by
        // x11pt2/pt3. (NOT Series: x11pt3 reads Series as the ORIGINAL for D11 =
        // Series/seasonal/faccal; overwriting it double-removes the calendar.)
        // Snapshot the FULL padded span [Pos1bk, Posffc] (not just the observed
        // [Pos1ob, Posfob]): Stcsi here is the forecast/backcast-extended B1, so
        // this captures the appended forecast (and backcast) rows the b1 table
        // emits when series{appendfcst/appendbcst=yes} (getsrs.f Savfct/Savbct,
        // agr3.f:158-160). The extra region is harmless scratch when not saved.
        const int pos1bk_b1 = ctx.x11ptr.pos1bk;
        const int posffc_b1 = ctx.x11ptr.posffc;
        copy(ctx.orisrs.stcsi.data() + (pos1bk_b1 - 1), posffc_b1 - pos1bk_b1 + 1,
             1, ctx.orisrs.stoap.data() + (pos1bk_b1 - 1));
    }

    // x11ari.f:199 -- x11pt2 runs on BOTH the X-11 and the SEATS path (gated
    // `(.not.Lcmpaq).or.Lx11`, true for any non-composite-aggregate run). It is
    // what sets Length, which setssp reads to size the sliding spans.
    x11pt2(ctx, lmodel, lx11, lseats, lgraf, lgrfxr);
    if (ctx.error.lfatal) return false;

    return true;
}

}  // namespace x13
