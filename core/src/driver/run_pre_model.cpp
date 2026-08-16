// run_pre_model.cpp -- M2 pre-model driver phase.
//
// Extends the M1 parse path (parse_spec -> gtinpt/getsrs) with the reachable
// pre-model table/save output. This first slice produces table a1 -- the
// original series over the analyzed span -- captured numerically (ctx.saves)
// and as byte-identical /rdb save-file text via savtbl.f.
//
// Table pointer LSRSSP=2 (mdltbl.i) carries the 'a1' save extension.
#include "specparse/specparse.hpp"
#include "tables/tables.hpp"
#include "transform/transform.hpp"
#include "transform/trnaic.hpp"     // trnaic (automatic transform selection)
#include "regarima/priadj.hpp"
#include "diag/checkres.hpp"   // check_residuals (arima.f:1044-1102)
#include "diag/estdgn.hpp"     // est_diagnostics (savotl.f counts + prtrts.f roots)
#include "diag/amdfct.hpp"     // aape_diagnostics (amdfct.f forecast error)
#include "diag/genqs.hpp"      // calcqs (arima.f:1107's residual QS statistic)
#include "regarima/regvar.hpp"
#include "x11/x11reg.hpp"            // pritd, tdset_td (x11regression tdprior)
#include "x11/xrgdrv.hpp"            // xrgdrv (x11regression OLS prior TD, Ixreg>=2)
#include "driver/x11_prestage.hpp"   // x11_editor_geometry (editor.f:206-233 + :851)
#include "driver/run_spectrum.hpp"   // run_residual_spectrum (arima.f:1126 spcrsd)
#include "regarima/estimate.hpp"
#include "regarima/forecast.hpp"
#include "regarima/outlier.hpp"     // idotlr, setcv
#include "automdl/automd.hpp"       // automd (automatic model selection)
#include "automdl/iddiff.hpp"       // prterr (arima.f:711/:772 estimation-error report)
#include "automdl/automx.hpp"       // automx (pickmdl candidate search)
#include "automdl/aictst.hpp"       // explicit_aictest (arima.f:569 aictest)
#include "automdl/svaict.hpp"       // svaict (arima.f:465 aictest.* savelog)
#include "automdl/automd_finalize.hpp"  // rmfix, addfix (arima.f:283/910)
#include "regarima/usrbak.hpp"      // bakusr (editor.f:1348-1350's backup)
#include "numeric/numeric.hpp"      // dpeq
#include "gen/srslen.hpp"           // prm::PLEN (residual work-vector sizing)
#include "gen/model.hpp"            // prm::PORDER
#include "gen/notset.hpp"           // prm::DNOTST

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace x13 {

namespace {
constexpr int LSRSSP = 2;    // mdltbl.i: original series for span; ext 'a1'
constexpr int LTRNPA = 13;   // mdltbl.i: prior-adjustment factors; ext 'a2'
constexpr int LTRNA3 = 16;   // mdltbl.i: prior-adjusted data; ext 'a3'
constexpr int LTRNDT = 20;   // mdltbl.i: prior-adjusted+transformed data; ext 'trn'
constexpr int LREGDT = 22;   // mdltbl.i: regression matrix; ext 'rmx'

bool wants_save(const X13Context& ctx, const std::string& ext) {
    const auto& v = ctx.captured.save_tables;
    return std::find(v.begin(), v.end(), ext) != v.end();
}
}  // namespace

bool run_m2(X13Context& ctx, const std::string& spec_text, const std::string& base,
            bool estimate, const std::string& spcname) {
    if (!parse_spec(ctx, spec_text,
                    spcname.empty() ? base + ".spc" : spcname))
        return false;
    return run_m2_after_parse(ctx, base, estimate, nullptr, nullptr);
}

// Post-parse pre-model + (optional) estimate/forecast body. Split out of run_m2
// so the X-11 driver (run_x11) can reuse the estimate/forecast machinery on an
// already-parsed context without re-parsing. When out_trnsrs is non-null it
// receives the clean transformed series (before estimation clobbers Tsrs with
// residuals) and *out_nobspf its length -- the inputs the X-11 extend stage needs.
bool run_m2_after_parse(X13Context& ctx, const std::string& base, bool estimate,
                        std::vector<double>* out_trnsrs, int* out_nobspf) {
    if (!ctx.captured.has_series) return false;

    // Save-format setup (gtinpt.f:1185-1187). svprec is already set by the parser
    // (default 15, or series{saveprecision=} in [1,14]); derive the field width and
    // format from it instead of clobbering back to the default.
    ctx.savcmn.svsize = (ctx.savcmn.svprec < 15) ? ctx.savcmn.svprec + 7 : 22;
    {
        auto z2 = [](int n) {
            std::string s = std::to_string(n);
            return s.size() < 2 ? "0" + s : s;
        };
        ctx.savcmn.svfmt = "(sp,e" + z2(ctx.savcmn.svsize) + "." +
                           z2(ctx.savcmn.svprec) + ")";
    }

    int sp = ctx.model.sp;
    const int* begsrs = ctx.arima.begsrs.data();
    const int* begspn = ctx.mdldat.begspn.data();
    int nspobs = ctx.mdldat.nspobs;

    // series{modelspan=} (arima.f:133-141): the regARIMA stage estimates over
    // Begmdl..Endmdl, not over the series span. nbeg/nend measure how far inside
    // the series span the model span sits. The narrowing itself is applied much
    // further down, just before the transform: the prior-factor record (adjsrs,
    // editor.f:849), a1/a2/a3, and the automatic transform test (trnaic,
    // x11ari.f:84) all run on the FULL span in the oracle, ahead of arima.f. It
    // is walked back out (setspn.f) before X-11 sees the series, so B1 and the
    // D-tables always cover the whole span.
    int nbeg = 0, nend = 0;
    if (ctx.arima.ldestm) {
        int endspn0[2];
        addate(begspn, sp, nspobs - 1, endspn0);
        dfdate(ctx.arima.begmdl.data(), begspn, sp, nbeg);
        dfdate(endspn0, ctx.arima.endmdl.data(), sp, nend);
        if (nbeg < 0) nbeg = 0;   // chkcvr already rejects a model span that
        if (nend < 0) nend = 0;   // is not inside the series span
    }
    // editor.f:176-187 -- a model span starting after the series span turns
    // backcasting off; mkback would otherwise be "backcasting" over observed
    // data. (The oracle also emits a WARNING here; print surface is deferred.)
    if (nbeg > 0 && ctx.extend.nbcst > 0) ctx.extend.nbcst = 0;

    // Span offset within the full series (getsrs guarantees coverage).
    int offset = 0;
    dfdate(begspn, begsrs, sp, offset);

    // avec must be 1-based over the span: avec[tpnt-1] == y(offset+tpnt).
    const double* aptr = ctx.arima.y.data() + offset;

    // x12run.f: Serno (the header label) is the series{ name=... } field when
    // present (the oracle's Serlbl), else the run base name; capped at 16 chars.
    // The on-disk filename (Cursrs) keeps the full base name regardless.
    const std::string& serlbl =
        ctx.captured.series_name.empty() ? base : ctx.captured.series_name;
    int nser = static_cast<int>(serlbl.size());
    if (nser > 16) nser = 16;
    savtbl(ctx, LSRSSP, begspn, 1, nspobs, sp, aptr, base, serlbl, nser);
    if (ctx.error.lfatal) return false;

    // Prior adjustment (adjsrs.f / prtadj.f, predefined lom/loq/lpyear priors).
    // adjsrs builds the multiplicative prior-factor series Adj via td7var; the
    // prior-adjusted series is the original series divided by those factors
    // (divsub). The factors depend only on the calendar date, so a2 (factors),
    // a3 (prior-adjusted data), and the prior-adjusted trn are pre-model
    // reproducible. User prior-factor files and calendar (holiday/TD) priors are
    // out of this pre-model slice.
    int priadj = ctx.prior.priadj;
    bool has_predef = (priadj > 1);   // 2 lom / 3 loq / 4 lpyear
    bool lom = (priadj == 2 || priadj == 3);   // adjsrs.f: lom for lom/loq

    // User permanent prior-adjustment factors (getadj.f/addadj.f, Usrpad). Minimal
    // slice: factors aligned 1:1 with the span (Frstad==0); adjsrs_factors fatals
    // on the general addadj span shift.
    const bool has_user = (ctx.priusr.nuspad > 0 || ctx.priusr.nustad > 0);
    const bool has_prior = has_predef || has_user;

    // Forecast-extension bookkeeping (editor.f 224-230). regvar builds
    // regressor rows for the forecast period from the calendar alone, so the
    // pre-model design matrix covers Nspobs + Nfdrp rows.
    int frstsy = offset + 1;                     // dfdate(Begspn,Begsrs)+1
    ctx.arima.frstsy = frstsy;
    ctx.arima.nomnfy = ctx.arima.nobs - frstsy + 1;
    int nfcst = ctx.extend.nfcst;
    if (nfcst < 0) nfcst = 0;                    // defensive (NOTSET)
    int fctdrp = ctx.arima.fctdrp;
    bool lsadj = ctx.captured.has_x11 || ctx.captured.has_seats;
    int nfdrp = nfcst;
    if (!lsadj && fctdrp > 0) nfdrp = std::max(0, nfcst - fctdrp);
    ctx.extend.nfdrp = nfdrp;
    int nobspf = std::min(nspobs + nfdrp, ctx.arima.nomnfy);
    ctx.extend.nobspf = nobspf;

    // The prior-adjusted series over the span + forecast-period data
    // (== a1 with no prior). a2/a3 save the span rows only.
    // x11regression OLS prior-TD (Ixreg>=2) with picktd: suppress the length-of-
    // month / leap-year prior in the estimation input -- the prior-TD factor
    // (Faccal = c16) already carries the day-count normalization (Xn/Xnstar in
    // x11ref), so applying the LOM prior too double-adjusts February. Mirrors
    // xrgdrv.f:95-97 (Picktd & Priadj>1 -> Priadj=0, Sprior=base).
    const bool suppress_lom_td =
        (ctx.hiddn.ixreg >= 2 && ctx.x11log.axrgtd && ctx.picktd.picktd &&
         priadj > 1);
    // adjsrs.f:39-40 -- Nadj, the length of the prior-factor series, is NOT the
    // estimation length: it spans the backcasts, the observed span, and at least
    // a full year of forecasts. The distinction matters because x11pt2's tdlom
    // multiplies the model TD factor by Sprior over [Pos1bk, Posffc] -- a factor
    // series that stopped at Nobspf left Sprior==0 across the forecast tail and
    // so zeroed Factd/Faccal there, which made D11 infinite over the forecast
    // span. Invisible in d10-d13 (printed over the observed span only), but
    // force{}'s qmap sums the target-vs-SA discrepancy over the FORECAST year
    // too, so saa/ffc came out +/-Inf.
    int nbcst_p = ctx.extend.nbcst;
    if (nbcst_p < 0) nbcst_p = 0;
    const int nadj = std::min(nspobs + nbcst_p + std::max(sp, nfcst - fctdrp),
                              prm::PLEN);
    // adjsrs.f:39 -- and the factor series starts at BEGADJ (Begspn shifted back
    // Nbcst), not Begspn: Adj(1) is the first BACKCAST period, which is what
    // x11int's Adj -> Sprior copy at Setpri = Pos1bk assumes. Anchoring it at
    // Begspn shifted every prior factor forward by Nbcst, which showed up as a
    // ~3.6e-2 error in D11/D13/D16 on any backcast run carrying trading day.
    // The estimation input therefore reads the factors from Adj1st, not from 1.
    int begadj[2];
    addate(begspn, sp, -nbcst_p, begadj);
    const int adj1st = nbcst_p + 1;          // dfdate(Begspn,Begadj)+1
    std::vector<double> padj(static_cast<std::size_t>(nobspf));
    std::vector<double> fac(static_cast<std::size_t>(nadj), 1.0);
    if (!adjsrs_factors(ctx, begadj, sp, nadj, suppress_lom_td, fac.data()))
        return false;
    const double* facspn = fac.data() + (adj1st - 1);   // factors from Begspn
    for (int tpnt = 1; tpnt <= nobspf; ++tpnt)
        padj[static_cast<std::size_t>(tpnt - 1)] =
            aptr[tpnt - 1] / facspn[tpnt - 1];   // divsub

    // SEATS s16/s18 combined-adjustment source: stash the RAW original series
    // (a1, original units) whenever a prior or a non-mean regressor will be
    // stripped from the SEATS decomposition input. The decomposition runs on the
    // linearized/prior-adjusted series, so s10 = linearized/sa but s16/s18 =
    // a1/sa refold the removed regression effects AND the lom/leap prior (see
    // estbur.cpp / x13context.hpp seats_combined_orig). Left empty when only a
    // mean / no regressor is present, so s10==s16==s18 and the no-regressor /
    // Constant-only SEATS corpus stays bit-identical.
    if (ctx.captured.has_seats) {
        bool nonmean_reg = false;
        for (int i = 1; i <= ctx.model.nb; ++i)
            if (ctx.model.rgvrtp(i) != prm::PRGTCN) { nonmean_reg = true; break; }
        if (has_prior || nonmean_reg) {
            ctx.seats_combined_orig.assign(static_cast<std::size_t>(nspobs), 0.0);
            for (int i = 0; i < nspobs; ++i) ctx.seats_combined_orig[i] = aptr[i];
        }
    }

    // x11regression tdprior (Kswv=1): prior trading-day pre-adjustment of the
    // estimation input. In the oracle x11pt1 runs BEFORE arima (x11ari.f:99-133),
    // so the regARIMA likelihood sees the prior-TD-adjusted Sto (x11pt1.f:265-266
    // divsub -> arima.f:157 trnsrs). The C++ runs X-11 as a later phase, so mirror
    // that division onto the pre-model padj series here; x11pt1 independently
    // rebuilds the same pritd factor for its own X-11 buffer + the D16 fold. Kept
    // separate from `fac`/Adj (the LOM/leap prior) -- prior TD is a distinct factor
    // (the a4 table), not the tdlom prior. Applies to log-additive (muladd==2)
    // exactly as to multiplicative: x11pt1.f:52 forces Muladd=0 for the whole
    // prior-adjustment stage, so the oracle divides in both modes and does not
    // reach the logadd exp-domain at all here. (This used to be gated on
    // muladd==0 as "deferred with the x11pt1 guard" -- but that guard tests
    // muladd AFTER the 2->0 collapse, so it never fired for logadd either, and a
    // tdprior+mode=logadd spec came back OUTCOME: OK with the prior TD missing
    // from B1 and d10-d13 entirely, ~2e-2 to 3.8e-2. Additive and pseudo-additive
    // are rejected at parse instead -- pritd.f:34-40.)
    // x11regression OLS-estimated prior trading day (Ixreg>=2, xrgdrv): estimate
    // the TD via a transparent pre-model seasonal adjustment, then divide the
    // estimation input by the resulting Faccal -- the oracle fits arima on the
    // prior-TD-adjusted series (x11ari.f runs xrgdrv + x11pt1 BEFORE arima). Same
    // inversion as the tdprior divide above; the main x11pt1 restores the same
    // Faccal for its Ixreg==3 fold. The stashed Faccal is forecast-extended
    // (spans [Pos1ob, Posfob+Nfcstx] per x11mdl.f:124-134); here we consume only
    // the leading nobspf rows the estimation input needs (min-guarded on nfac).
    // The guard is x11ari.f:91's `Ixreg.eq.2.or.Khol.eq.1` -- NOT `Axrgtd`. It
    // used to test Axrgtd, which is a PROXY for "the irregular regression has a
    // prior to estimate" and is narrower than the thing it stands for: a
    // HOLIDAY-only x11regression clears Axrgtd at editor.f:1722 and sets Axrghl
    // instead, so the whole transparent pass was skipped and B1 came back as the
    // raw series where the oracle had already divided the holiday factor out
    // (measured 8.8e-3 on `x11regression{variables=(easter[8])}`, and 1.4e-2 by
    // D10 once the filter choice flipped with it). Khol==1 is the classic X-11
    // Easter arm, which this port routes through x11_easter_prepass instead.
    if (ctx.hiddn.ixreg >= 2 && ctx.x11opt.muladd == 0) {
        if (!xrgdrv(ctx)) return false;
        const std::vector<double>& xrgfac = ctx.x11_faccal_prior;
        const int nfac = static_cast<int>(xrgfac.size());
        for (int t = 0; t < nobspf && t < nfac; ++t)
            padj[static_cast<std::size_t>(t)] /= xrgfac[static_cast<std::size_t>(t)];
    }

    // ...and THEN the tdprior divide, because x11ari.f runs xrgdrv (:99) before
    // x11pt1 (:133) and xrgdrv's own x11pt1 has already bumped Kswv past 1
    // (x11pt1.f:235) whenever an x11regression TD model is present -- so with
    // BOTH a tdprior and x11regression{variables=(td)} the oracle divides by the
    // combined Faccal only, never by the bare prior-TD factor as well. This block
    // used to sit ahead of the xrgdrv one and removed the prior TD twice.
    if (ctx.x11opt.kswv == 1) {
        std::vector<double> stptd(static_cast<std::size_t>(prm::PLEN), 0.0);
        tdset_td(ctx, begspn, 1, nobspf, sp);
        pritd(ctx, stptd.data(), nobspf, sp, begspn, 1);
        if (ctx.error.lfatal) return false;
        for (int t = 0; t < nobspf; ++t)
            padj[static_cast<std::size_t>(t)] /= stptd[static_cast<std::size_t>(t)];
    }

    // adjsrs.f also records the prior-factor series in the /adjcmn/ Adj array and
    // its span (Nadj/Adj1st/Begadj) so the X-11 makadj/tdlom step (x11pt2.f) can
    // fold the length-of-month/leap-year prior back into the model trading-day
    // factor (Factd) and strip it from the calendar-adjusted series. The aictest
    // path rebuilds this inside automd (tdaic); the FIXED-model X-11 path never
    // runs automd, so without this population Sprior stays identity (0) and tdlom
    // multiplies Factd by it -> Factd==0 -> Faccal==0 -> D11/D13 divide-by-zero.
    // (Adjmod is left at its default: tdlom only special-cases Adjmod==2, and the
    // prior here is the multiplicative LOM/leap ratio.)
    //
    // The RECORD is unconditional (adjsrs.f:39-40 + :108-109 sit outside the
    // prior branch, and its ELSE at :79-80 fills Adj with the mode identity):
    // with no prior, Adj is all-1 and x11int still copies it into Sprior, so
    // Sprior is the IDENTITY rather than the all-zero COMMON. That distinction
    // is invisible until x11pt3's sp2 writeback replaces Sprior with
    // Sprior*Facusr (etc.) -- with a zero Sprior the product stays zero and
    // x11pt4's Pbar/Vp come out NaN instead of measuring the outlier/user
    // factors. Only Kfmt stays keyed to whether a prior really exists.
    {
        for (int t = 0; t < nadj; ++t)
            ctx.adj.adj(t + 1) = fac[static_cast<std::size_t>(t)];
        ctx.adj.begadj(1) = begadj[0];
        ctx.adj.begadj(2) = begadj[1];
        ctx.adj.nadj = nadj;
        ctx.adj.adj1st = adj1st;
    }
    if (has_prior) {
        // adjsrs.f:62,101 -- a prior series exists, so Kfmt says so. adjreg.f:98
        // is gated on it: without Kfmt>0 the forecast tail of Series never gets
        // the prior folded back, so the X-11 "original" over the forecast span
        // was the prior-ADJUSTED forecast. Only force{} reads that far out, so
        // the D-tables never saw it.
        if (!suppress_lom_td) ctx.prior.kfmt = 1;
    }

    // Table a2 (LTRNPA): the combined prior-adjustment factors (Sprior).
    if (has_prior && wants_save(ctx, "a2")) {
        savtbl(ctx, LTRNPA, begspn, 1, nspobs, sp, facspn, base, serlbl, nser);
        if (ctx.error.lfatal) return false;
    }
    // Table a3 (LTRNA3): the prior-adjusted data (Sto after divsub).
    if (has_prior && wants_save(ctx, "a3")) {
        savtbl(ctx, LTRNA3, begspn, 1, nspobs, sp, padj.data(), base, serlbl, nser);
        if (ctx.error.lfatal) return false;
    }

    // Automatic transform selection (x11ari.f:81 trnaic), when
    // transform{function=auto} left Fcntyp==0. trnaic estimates the default
    // airline model untransformed and log-transformed, compares AICC, and leaves
    // the chosen Fcntyp/Lam in ctx before the series is transformed below. It
    // reads the untransformed span series (aptr == Y(Frstsy)); rgarma inside it
    // writes residuals into ctx.series.tsrs, which the trn block below refills.
    if (ctx.arima.fcntyp == 0) {
        bool lmodel = ctx.captured.has_model &&
                      !(ctx.arima.lautom || ctx.arima.lautox);
        double aicno = 0.0, aiclog = 0.0;
        trnaic(ctx, aptr, frstsy, nspobs, nobspf, lmodel, aicno, aiclog);
        if (ctx.error.lfatal) return false;
        ctx.trnaic_result.ran = true;
        ctx.trnaic_result.aicno = aicno;
        ctx.trnaic_result.aiclog = aiclog;
        ctx.trnaic_result.selected_log = (ctx.arima.fcntyp == 1);
    }

    // --- series{modelspan=}: move onto the model span (arima.f:139-153) ------
    // Endspn is not carried on the main path (it is derivable from Begspn +
    // Nspobs), so track it locally; setspn.f rebuilds both endpoints
    // arithmetically from Begmdl/Endmdl anyway.
    int endspn_cur[2];
    addate(begspn, sp, nspobs - 1, endspn_cur);
    int adj1st_cur = adj1st;
    // setspn.f -- recompute every span-derived pointer after Begspn/Endspn move.
    // Called twice on the way back out: once for the span END before forecasting
    // (arima.f:1145) and once for the span START afterwards (arima.f:1181).
    auto setspn = [&](int nend_r, int nbeg_r) {
        if (nend_r > 0)
            addate(ctx.arima.endmdl.data(), sp, nend_r, endspn_cur);
        if (nbeg_r > 0)
            addate(ctx.arima.begmdl.data(), sp, -nbeg_r, ctx.mdldat.begspn.data());
        dfdate(endspn_cur, begspn, sp, nspobs);
        nspobs += 1;
        ctx.mdldat.nspobs = nspobs;
        dfdate(begspn, begsrs, sp, frstsy);
        frstsy += 1;
        ctx.arima.frstsy = frstsy;
        ctx.arima.nomnfy = ctx.arima.nobs - frstsy + 1;
        // NOTE the asymmetry, ported as written: setspn.f:135 uses
        // max(Nfcst-Fctdrp,0) where the narrowing at arima.f:151 uses Nfdrp.
        // The two differ only when a seasonal adjustment is requested together
        // with forecast{fctdrp}, where editor.f leaves Nfdrp == Nfcst.
        nobspf = std::min(nspobs + std::max(nfcst - fctdrp, 0), ctx.arima.nomnfy);
        ctx.extend.nobspf = nobspf;
        dfdate(begspn, begadj, sp, adj1st_cur);
        adj1st_cur += 1;
        ctx.adj.adj1st = adj1st_cur;
        aptr = ctx.arima.y.data() + (frstsy - 1);
        facspn = fac.data() + (adj1st_cur - 1);
    };
    bool nend_done = false;   // has the arima.f:1145 span-END restore run yet?
    if (nbeg > 0 || nend > 0) {
        if (nbeg > 0) {
            ctx.mdldat.begspn(1) = ctx.arima.begmdl(1);
            ctx.mdldat.begspn(2) = ctx.arima.begmdl(2);
        }
        if (nend > 0) {
            endspn_cur[0] = ctx.arima.endmdl(1);
            endspn_cur[1] = ctx.arima.endmdl(2);
        }
        dfdate(endspn_cur, begspn, sp, nspobs);
        nspobs += 1;
        ctx.mdldat.nspobs = nspobs;
        dfdate(begspn, begsrs, sp, frstsy);
        frstsy += 1;
        ctx.arima.frstsy = frstsy;
        ctx.arima.nomnfy = ctx.arima.nobs - frstsy + 1;
        nobspf = std::min(nspobs + nfdrp, ctx.arima.nomnfy);
        ctx.extend.nobspf = nobspf;
        dfdate(begspn, begadj, sp, adj1st_cur);
        adj1st_cur += 1;
        ctx.adj.adj1st = adj1st_cur;
        aptr = ctx.arima.y.data() + (frstsy - 1);
        facspn = fac.data() + (adj1st_cur - 1);
    }

    // Table trn (LTRNDT): the transformed prior-adjusted series that feeds
    // regARIMA modeling. arima.f applies the Box-Cox/logit transform (trnfcn) to
    // the prior-adjusted series (== a3, or a1 when there is no prior), over
    // Nobspf points (span + retained forecast-period data); the trn table
    // itself covers the span.
    // NOTE: sized prm::PLEN (the Fortran COMMON Trnsrs(PLEN) bound), not nobspf.
    // automd/tdaic/easaic (automdl/automd.cpp, automdl/aictst.cpp) are ported
    // faithfully against the oracle's fixed-size Trnsrs COMMON and do blanket
    // `copy(trnsrs, PLEN, ...)` snapshot/restore calls on this same buffer
    // (tdaic.f:66-72 `tsrs`/`a2` save, tdaic.f:283 restore). A buffer sized only
    // to nobspf (~144 for a 12-year monthly series) is a heap allocation of ~156
    // doubles; a PLEN=1020-element copy against it is a heap buffer overflow
    // (over-read building the snapshot, over-WRITE on restore) that corrupts
    // adjacent heap memory and crashes later, nondeterministically, wherever the
    // corruption is next touched. tools/x13run_iddiff.cpp's tdaic/easaic harness
    // already allocates its `trn` buffer PLEN-sized for exactly this reason --
    // match it here so the real automd()-driven X-11 path is safe too.
    std::vector<double> trnsrs(static_cast<std::size_t>(prm::PLEN));
    bool have_trn = false;
    if (wants_save(ctx, "trn") || ctx.captured.has_model) {
        // arima.f:157 reads the estimation input from Sto(Pos1ob+nbeg): padj is
        // the prior-adjusted series over the FULL span, so the model span starts
        // nbeg periods in.
        trnfcn(ctx, padj.data() + nbeg, nobspf, ctx.arima.fcntyp, ctx.arima.lam,
               trnsrs.data());
        if (ctx.error.lfatal) return false;
        have_trn = true;
        // Hand the clean transformed series (+ its length) back to the caller
        // before rgarma overwrites Tsrs with residuals; run_x11's extend stage
        // consumes exactly this as the observed span to forecast-extend.
        if (out_trnsrs) *out_trnsrs = trnsrs;
        if (out_nobspf) *out_nobspf = nobspf;
        // Retain the transformed series on the context (Tsrs semantics).
        // Estimation later overwrites Tsrs from Xy with the same content; keeping
        // it here lets the automatic-model-ID path (iddiff/automd) read the series
        // without re-transforming. Bounded by the Tsrs (PLEN) extent.
        {
            int ncp = nobspf < prm::PLEN ? nobspf : prm::PLEN;
            copy(trnsrs.data(), ncp, 1, ctx.series.tsrs.data());
            // Prior factors into Adj at Adj1st, so the automatic-model path's
            // prlkhd sees Adj(Adj1st) == the factor at Begspn, matching arima.f.
            // Adj stays BEGADJ-anchored (Adj1st = Nbcst+1): writing the span
            // slice at index 1 instead would clobber the backcast rows the
            // /adjcmn/ record above put there, and x11int copies Adj into Sprior
            // from Setpri = Pos1bk -- the first BACKCAST period, not Begspn. With
            // no backcasts Adj1st == 1 and this is the identical write.
            copy(facspn, ncp, 1, ctx.adj.adj.data() + (adj1st_cur - 1));
            ctx.adj.adj1st = adj1st_cur;
        }
    }
    if (wants_save(ctx, "trn")) {
        savtbl(ctx, LTRNDT, begspn, 1, nspobs, sp, trnsrs.data(), base, serlbl, nser);
        if (ctx.error.lfatal) return false;
    }

    // ---- the editor's span pointers + Setpri, and x11int.f:53's Sprior copy --
    // Both belong BEFORE the model stage, which is where the oracle puts them:
    // editor.f:233/851 set the pointers and Setpri, and x12run.f:174 calls
    // x11int -- all of it ahead of x11ari, hence ahead of arima/automd/automx.
    //
    // This port used to assign Setpri only in x11_prestage, i.e. AFTER model
    // selection, which left it 0 for the whole model stage and so silently
    // disabled the three Sprior writes that stage makes (tdaic.f:600-623,
    // rmlpyr.f:59, pass2.f:101) -- each guarded on `Setpri >= 1` and therefore
    // indistinguishable from ported-and-inert. The post-model x11int copy
    // compensated, and agreed with the oracle exactly while Adj == Sprior at
    // that point, which is every spec whose model stage does not move the prior.
    // A pickmdl{} run whose candidates DISAGREE about trading day does move it:
    // the Picktd restore puts Adj back to its entry value while Sprior must keep
    // what tdaic wrote, and the compensation overwrote it -- Februaries out by
    // the leap ratio behind an OUTCOME: OK. See M5_PORT_NOTES entry 55.
    //
    // Placement here rather than beside the /adjcmn/ record above is load-
    // bearing: trnaic (the automatic transform selection, x11ari.f:81) rewrites
    // Adj wholesale, and the oracle re-issues the copy after it at
    // trnaic.f:278. Copying here covers both -- the geometry is a pure function
    // of the span and the forecast/backcast counts, so it does not care.
    // ...but NOT when xrgdrv has just left the geometry deliberately narrowed.
    // The oracle's editor runs once, at parse, BEFORE x11ari calls xrgdrv; this
    // port stands in for it here, i.e. after. That inversion is invisible while
    // the derivation is idempotent, and it stops being idempotent exactly when
    // an x11regression span ends early (xrgdrv.f:151-158 + x11mdl.f:126-137
    // leave Nofpob narrow on purpose). Same guard in x11_prestage; see there.
    const bool xrgdrv_left_geometry =
        ctx.hiddn.ixreg == 3 && ctx.x11reg.xdsp > 0;
    const xrg_geometry sv_geom = xrg_geometry::capture(ctx);
    x11_editor_geometry(ctx, lsadj, /*set_setpri=*/true);
    if (xrgdrv_left_geometry) sv_geom.restore(ctx);
    if (ctx.adj.nadj > 0)
        copy(ctx.adj.adj.data(), prm::PLEN - ctx.adj.setpri + 1, -1,
             ctx.inpt.sprior.data() + (ctx.adj.setpri - 1));

    // ---- editor.f:1344-1350 -- the EDITOR's user-regressor backup -----------
    // `addusr` rebuilds the user columns from /urgbak/ after `rmfix` has struck
    // them, and /urgbak/ has exactly ONE writer: `bakusr`. This port had only
    // the sliding-spans/history callers (ssmdl.f:351, sspdrv.f:170,
    // revdrv.f:257), so on the MAIN run the backup was never taken and
    // `addusr(0)` restored `Ncusrx=0` -- the user regressors simply vanished.
    // Measured on `extra/airline_reg-user-fixed`: the oracle reports `nreg: 2`
    // with both columns `(fixed)`, the engine reported `nreg: 0`, at OUTCOME: OK.
    //
    // The guard is transcribed, including the part that reads oddly: `Userfx` is
    // the flag `gtxreg.f:864` leaves behind when an `x11regression{}` spec is
    // present, because `gtinpt.f:831`'s `restor(T,F,F)` restores `Iregfx`/`Regfx`
    // and NOT `Userfx`. So on such a run this tests the x11regression design's
    // fixings against the regARIMA design's columns. Faithful; not a CB entry,
    // because both readings agree wherever a spec fixes user columns in one
    // design only.
    if (ctx.captured.has_model && ctx.model.nb > 0 &&
        (ctx.model.userfx || (ctx.usrreg.ncusrx > 0 && ctx.arima.lautom))) {
        bakusr(ctx, usr_design_reg(ctx), /*rind=*/0, /*is1st=*/true);
        if (ctx.error.lfatal) return false;
    }

    // The regression design matrix (arima.f:280): regvar builds [X:y] from the
    // parsed regression groups and the transformed series. Pre-model this is
    // fully calendar-determined (constant/seasonal/td/holiday columns), so the
    // rmx save table (savmtx.f, arima.f:1013) is reproducible here.
    // arima.f:129-130 -- the ONLY place QsRsd/QsRsd2 are initialised, and they
    // are initialised at the top of `arima`, which x11ari.f:132 calls only when
    // Lmodel. See CB-25 (core/src/diag/genqs.hpp): a model-free run reports the
    // COMMON's static zero as the residual QS statistic, and this reset is what
    // separates that from an identify-only run, where arima DOES run but the
    // :1105 block does not fire.
    if (ctx.captured.has_model) {
        ctx.qs.qsrsd = prm::DNOTST;
        ctx.qs.qsrsd2 = prm::DNOTST;
    }

    if (ctx.captured.has_model && have_trn) {
        int nrxy, frstry;
        regvar(ctx, trnsrs.data(), nobspf, fctdrp, nfcst, 0,
               ctx.arima.userx.data(), ctx.arima.bgusrx.data(), ctx.arima.nrusrx,
               ctx.prior.priadj, ctx.arima.reglom, nrxy,
               ctx.arima.begxy.data(), frstry, true, ctx.arima.elong);
        if (ctx.error.lfatal) return false;
        ctx.arima.nrxy = nrxy;
        // arima.f:281-287 -- with any regression coefficient held FIXED
        // (Iregfx>=2, i.e. regression{b=(.. f)} or estimate{fix=}), the fixed
        // columns are STRUCK from the design and their contribution b*X is
        // subtracted from the series, so what gets estimated is the residual
        // model; arima.f:907-914 puts both back before the final prlkhd and the
        // forecasts. This has to be here, ahead of automd/aictest/rgarma alike,
        // because the Fortran does it before the branch.
        // PORTED ASYMMETRY: regvar above is called with a backcast count of 0,
        // so Fixfac is built over a design with no backcast rows -- yet
        // arima.f:283 still hands rmfix the real Nbcst, shifting the
        // subtraction by that many periods. Transcribed as written.
        const bool fixed_reg = ctx.model.iregfx >= 2 && ctx.model.nb > 0;
        if (fixed_reg) {
            rmfix(ctx, trnsrs.data(), ctx.extend.nbcst, ctx.arima.nrxy, 1);
            if (ctx.error.lfatal) return false;
            int nrxyf = 0, frstryf = 0;
            regvar(ctx, trnsrs.data(), nobspf, fctdrp, nfcst, 0,
                   ctx.arima.userx.data(), ctx.arima.bgusrx.data(),
                   ctx.arima.nrusrx, ctx.prior.priadj, ctx.arima.reglom, nrxyf,
                   ctx.arima.begxy.data(), frstryf, true, ctx.arima.elong);
            if (ctx.error.lfatal) return false;
            ctx.arima.nrxy = nrxyf;
            frstry = frstryf;
        }
        if (wants_save(ctx, "rmx")) {
            savmtx(ctx, LREGDT, ctx.arima.begxy.data(), sp, ctx.mdldat.xy.data(),
                   nrxy, ctx.model.ncxy, ctx.model.colttl.data(),
                   ctx.model.colptr.data(), ctx.model.ncoltl, base);
            if (ctx.error.lfatal) return false;
        }

        // regARIMA estimation (arima.f:705). The M2 save path stops before this;
        // the M3 driver estimates the model in place. rgarma differences Xy
        // internally (Nintvl), so the un-differenced [X:y] regvar just built is
        // exactly what it wants; it writes Tsrs from Xy itself (no external seed).
        // Estimation outputs land in ctx.mdldat (arimap/var/lnlkhd/lndtcv/armacm/
        // nliter/nfev/convrg/armaer). Deferred prints/saves keep it side-effect
        // free, so the M2 save comparisons above are untouched.
        if (estimate && ctx.arima.lestim) {
            constexpr int PA = prm::PLEN + 2 * prm::PORDER;
            std::vector<double> a(static_cast<std::size_t>(PA));
            int na = 0, nefobs = 0;
            bool lauto = false;

            // Automatic model selection (arima.f:344 automd), when an automdl{}
            // spec is present. The reduced driver (default -> chkmu -> iddiff ->
            // amdid -> mean -> final) identifies and estimates the model in place.
            // trnsrs is the clean transformed series (rgarma writes residuals into
            // ctx.series.tsrs, a separate buffer). do_aictest=true enables the
            // block-1 tdaic/easaic regressor selection + ismd0 a0-revert (reaches
            // parity for ismd0 series like airline). Auto-transform (trnaic) and
            // outlier ID are ported; the faithful non-default-model nloop/tstmd1
            // finalization stays unported (tools/automdl_scouting.md), but the specs
            // that would need it now gate bit-exact via the ismd0 snapshot revert.
            // The suite has 0 xfails.
            // arima.f:459-471 -- svaict reports which AIC-tested regressor
            // group survived, and it runs AFTER the model is selected but
            // BEFORE :469-471 clear the flags. Snapshot them here because the
            // selection routines below consume them: automd zeroes Itdtst on
            // its way out, and tdaic rewrites Aicint to the winning form.
            const bool svaic_td = ctx.arima.itdtst > 0;
            const bool svaic_lom = ctx.arima.lomtst > 0;
            const bool svaic_eas = ctx.arima.leastr;
            const bool svaic_usr = ctx.arima.luser;

            if (ctx.arima.lautom) {
                automd(ctx, trnsrs.data(), frstry, nefobs, a.data(), na,
                       /*do_aictest=*/true);
                if (ctx.error.lfatal) return false;
                (void)na;
                // Re-capture the transformed series AFTER automd: for aictest the
                // block-1 tdaic applies the leap-year prior adjustment (priadj=4)
                // to trnsrs, which the X-11 stage (adjreg -> B1) needs. For the
                // non-aictest automdl path automd does not modify trnsrs, so this
                // is a no-op there.
                if (out_trnsrs) *out_trnsrs = trnsrs;
            } else if (ctx.arima.lautox) {
                // arima.f:416 -- pickmdl{}: the classic X-11-ARIMA candidate
                // search. See automx.hpp. It estimates the winner in place, so
                // nothing below re-estimates.
                bool hvmdl = false;
                int hvstar = 0;
                bool lidotl = ctx.captured.has_outlier &&
                              (ctx.arima.ltstao || ctx.arima.ltstls ||
                               ctx.arima.ltsttc);
                automx(ctx, trnsrs.data(), frstry, nefobs, a.data(), na, hvmdl,
                       hvstar, /*lsadj=*/true, lidotl);
                if (ctx.error.lfatal) return false;
                (void)na;
                if (!hvmdl) {
                    // arima.f:476-527's "no model selected" cleanup (turn every
                    // regARIMA prior adjustment back off, drop the forecasts,
                    // restore the model span) is unported. Fatal rather than
                    // continue with adjustment flags the oracle would have
                    // cleared.
                    errhdr(ctx);
                    writln(ctx,
                           "ERROR: pickmdl{}: no candidate model was accepted, "
                           "and arima.f:476-527's no-model cleanup is unported.",
                           stdio::STDERR, ctx.units.mt2, true);
                    abend(ctx);
                    return false;
                }
                if (out_trnsrs) *out_trnsrs = trnsrs;
            } else if (ctx.arima.itdtst > 0 || ctx.arima.lomtst > 0 ||
                       ctx.arima.leastr ||
                       (ctx.arima.luser && ctx.usrreg.ncusrx > 0) ||
                       (ctx.arima.ch2tst && ctx.usrreg.nguhl > 0)) {
                // Explicit-model AIC regressor test (arima.f:569-700). The td/
                // lom/easter AIC tests estimate the model internally (with and
                // without the regressor), keeping the lower-AICC form, so this
                // REPLACES the plain rgarma. user/chi-square branches deferred.
                explicit_aictest(ctx, trnsrs.data(), a.data(), nefobs, na, frstry);
                if (ctx.error.lfatal) return false;
                (void)na;
                // Same handoff as automd above: tdaic may apply a leap-year prior
                // in place, and the later X-11/diagnostic stage consumes this buffer.
                if (out_trnsrs) *out_trnsrs = trnsrs;
            } else {
                rgarma(ctx, ctx.arima.lestim, ctx.arima.mxiter, ctx.arima.mxnlit,
                       /*lprtit=*/false, a.data(), na, nefobs, lauto);
                (void)na;
                if (ctx.error.lfatal) return false;

                // arima.f:711 -- report whatever /mdldat/'s Armaer says about
                // the fit just made. The automatic paths call prterr from
                // inside automd/automx; the EXPLICIT-model path had no call at
                // all, so an explicit model that ran out of iterations reached
                // arima.f:1216's halt with an empty `===ERR===` block. `lauto`
                // is arima.f's own local (Lautom.or.Lautox), false by
                // construction on this arm -- which is what selects itrerr's
                // three-remedy text with the ARMA start values under it.
                prterr(ctx, nefobs,
                       ctx.arima.lautom || ctx.arima.lautox);
                if (ctx.error.lfatal) return false;

                // Automatic outlier identification (arima.f:756 idotlr), when an
                // outlier{} spec is present. Re-estimates the model in place with
                // the identified AO/LS/TC regressors. Default the test span (model
                // span), the TC decay, and the per-type critical value (setcv) as
                // arima.f does before the call.
                if (ctx.captured.has_outlier &&
                    (ctx.arima.ltstao || ctx.arima.ltstls || ctx.arima.ltsttc)) {
                    int begtst[2] = {begspn[0], begspn[1]};
                    int endtst[2];
                    addate(begspn, sp, nspobs - 1, endtst);
                    if (dpeq(ctx.model.tcalfa, prm::DNOTST))
                        ctx.model.tcalfa = std::pow(0.7, 12.0 / sp);
                    int nobtst = 0;
                    dfdate(endtst, begtst, sp, nobtst);
                    nobtst += 1;
                    // Default (Ljung) critical value; the corrected (Cvtype)
                    // variant is deferred.
                    double cv = setcv(nobtst, ctx.arima.cvalfa);
                    for (int t = 1; t <= prm::POTLR; ++t)
                        if (dpeq(ctx.arima.critvl(t), prm::DNOTST))
                            ctx.arima.critvl(t) = cv;
                    double critvl[prm::POTLR] = {ctx.arima.critvl(1),
                                                 ctx.arima.critvl(2),
                                                 ctx.arima.critvl(3)};
                    // arima.f:120-124 -- `lautid = lauto .and. gudrun`, where
                    // lauto is `Lautom.or.Lautox` and gudrun is the main-run
                    // test. This port hardcoded false, which is right only
                    // while no automatic model selection reaches here.
                    bool lautid = (ctx.arima.lautom || ctx.arima.lautox) &&
                                  (ctx.hiddn.issap < 2 && ctx.hiddn.irev < 4);
                    idotlr(ctx, ctx.arima.ltstao, ctx.arima.ltstls,
                           ctx.arima.ltsttc, ctx.arima.ladd1, critvl,
                           ctx.arima.cvrduc, begtst, endtst, nefobs,
                           ctx.arima.lestim, ctx.arima.mxiter, ctx.arima.mxnlit,
                           lautid, a.data());
                    if (ctx.error.lfatal) return false;
                    // arima.f:772 -- and again after outlier identification,
                    // which re-estimates. Gated on !Convrg here, where :711 is
                    // unconditional.
                    if (!ctx.mdldat.convrg) {
                        prterr(ctx, nefobs,
                               ctx.arima.lautom || ctx.arima.lautox);
                        if (ctx.error.lfatal) return false;
                    }
                    // arima.f:773 -- after idotlr, rebuild the FULL Nobspf-row
                    // regression design. idotlr's coladd/addotl fill only the
                    // Nspobs span rows of any inserted outlier column (idotlr.f:
                    // 869-873 pass Nspobs), leaving the forecast-period rows stale;
                    // regvar re-runs addotl over Nobspf (regvar.f:247), so fcstxy's
                    // design carries the correct outlier values in the forecast
                    // region. Without it the point forecast (and hence the X-11
                    // forecast-extended seasonal-filter tail) drifts ~1%.
                    int nrxy2 = 0, frstry2 = 0;
                    regvar(ctx, trnsrs.data(), nobspf, fctdrp, nfcst, 0,
                           ctx.arima.userx.data(), ctx.arima.bgusrx.data(),
                           ctx.arima.nrusrx, ctx.prior.priadj, ctx.arima.reglom,
                           nrxy2, ctx.arima.begxy.data(), frstry2, true,
                           ctx.arima.elong);
                    if (ctx.error.lfatal) return false;
                    ctx.arima.nrxy = nrxy2;
                }
            }
            // arima.f:907-914 -- restore the fixed regressors to the design (and
            // their effect to the series) now that estimation is done, so the
            // final prlkhd (arima.f:968), the forecasts and the whole X-11 stage
            // see the FULL model. Must precede prlkhd below, as in the Fortran.
            if (fixed_reg) {
                addfix(ctx, trnsrs.data(), ctx.extend.nbcst, /*rind=*/0, 1);
                if (ctx.error.lfatal) return false;
                int nrxya = 0, frstrya = 0;
                regvar(ctx, trnsrs.data(), nobspf, fctdrp, nfcst, 0,
                       ctx.arima.userx.data(), ctx.arima.bgusrx.data(),
                       ctx.arima.nrusrx, ctx.prior.priadj, ctx.arima.reglom,
                       nrxya, ctx.arima.begxy.data(), frstrya, true,
                       ctx.arima.elong);
                if (ctx.error.lfatal) return false;
                ctx.arima.nrxy = nrxya;
                if (out_trnsrs) *out_trnsrs = trnsrs;
            }

            // arima.f:465 / :583 / :607 / :630 / :649. The oracle calls svaict
            // once per group inside the explicit-aictest block and once for
            // the whole set on the model-selection path; either way each key is
            // written exactly once, and no corpus golden carries a duplicated
            // `aictest.*` line. One call with the snapshot above covers both.
            // `Hvmdl` is true here because every branch that reaches this point
            // has a fitted model -- pickmdl's no-model case returns above.
            if (svaic_td || svaic_lom || svaic_eas || svaic_usr) {
                svaict(ctx, svaic_td, svaic_lom, svaic_eas, svaic_usr,
                       /*hvmdl=*/true);
                if (ctx.error.lfatal) return false;
            }

            // nefobs == Nspobs-Nintvl; the estimates live in mdldat. Record it
            // while Nspobs is still the ESTIMATION span (a model span narrows it
            // and setspn.f widens it back before X-11).
            ctx.est_nefobs = nefobs;

            // Capture the final regARIMA residuals for the residual-spectrum
            // diagnostic (spr, spcrsd.f, run from arima.f:1126 after the final
            // fit). Their start date is Begspn + (Nspobs - na); run_spectrum
            // reads these off ctx.
            // arima.f:1105-1130 -- both the residual QS statistic and spcrsd
            // live inside `IF(Ldestm)` (arima.f:321), so an identify-only spec
            // (Lmodel set, Ldestm not) produces neither. This port estimates
            // whenever a model spec is present, so the enclosing condition has
            // to be spelled out; without it `extra/airline_identify` emits a
            // full spcrsd block the oracle does not.
            // `Convrg` is the oracle's own gate: arima.f:1046 wraps the whole
            // residual-diagnostic / spectrum / forecast / span-restore tail in
            // `IF(Convrg)THEN`, and its ELSE at :1216 is a bare `CALL abend`
            // (see the check below). This port runs the residual capture and
            // the residual QS ahead of prlkhd rather than after it, so the
            // condition has to be spelled out at each moved site.
            if (na > 0 && ctx.arima.ldestm && ctx.mdldat.convrg) {
                ctx.resid_a.assign(a.begin(), a.begin() + na);
                ctx.resid_na = na;
                // arima.f:1125 computes `idate` HERE, from the estimation-time
                // Begspn/Nspobs -- which a series{modelspan=} has narrowed and
                // setspn.f widens back at arima.f:1145/:1181, i.e. AFTER this
                // point. Deriving it later from the restored span slides the
                // residual spectrum's start (measured: spcrsd.median -32.32 vs
                // -32.59 on generated/airline_modelspan-both-x11). Same class
                // as ctx.est_nefobs just above.
                addate(ctx.mdldat.begspn.data(), ctx.model.sp,
                       ctx.mdldat.nspobs - na, ctx.resid_begdate.data());

                // arima.f:1105-1118 -- the RESIDUAL half of the QS seasonality
                // block (`qsrsd`/`qssrsd`). genqs.f computes the other six
                // series but not this one: it lives here because it needs the
                // residuals and the estimation-time span, both of which are
                // gone by the time x11ari runs. `Var > 0` and `Sp > 1` are the
                // oracle's own gates on the block.
                //
                // `Ldestm` is the ENCLOSING one (arima.f:321 wraps the entire
                // estimation half of arima in `IF(Ldestm)`), and it has to be
                // spelled out here because this port estimates whenever a model
                // spec is present. On an identify-only spec the oracle sets
                // Lmodel but not Ldestm, does no estimation at all, and leaves
                // Var at zero -- so it emits no qsrsd row, where the engine
                // would report 255.92 off residuals that are essentially the
                // series. NOTE the neighbouring diagnostics below (check{},
                // the estimation savelog, aape) sit inside the same
                // `IF(Ldestm)` and do NOT yet carry it; `extra/airline_identify`
                // is the only corpus spec with a golden that reaches the
                // difference, and it ships none of those blocks either.
                if (ctx.model.sp > 1 && ctx.mdldat.var > 0.0) {
                    ctx.qs.qsrsd = calcqs(a.data(), na - nefobs, na, ctx.model.sp);
                    int qpos = 0;
                    const int bg[2] = {ctx.rho.bgspec(1), ctx.rho.bgspec(2)};
                    dfdate(bg, ctx.resid_begdate.data(), ctx.model.sp, qpos);
                    if (qpos > 0)
                        ctx.qs.qsrsd2 = calcqs(a.data(), qpos, na, ctx.model.sp);
                }
            }

            // Likelihood statistics (arima.f:742 prlkhd): the transform-Jacobian-
            // adjusted log likelihood + AIC/AICC/BIC/HQ into ctx.lkhd. Y is the
            // original untransformed series over the span (aptr == Y(Frstsy)); the
            // prior factors are `fac` (all 1 with no prior).
            prlkhd(ctx, aptr, facspn, ctx.adj.adjmod, ctx.arima.fcntyp,
                   ctx.arima.lam);
            if (ctx.error.lfatal) return false;

            // arima.f:1044-1102 -- check{}'s residual diagnostics, on the final
            // converged residuals: the ACF/PACF significance lists, the
            // Ljung-Box and Box-Pierce Q sequences, the normality battery
            // (skewness / Geary's a / kurtosis), Durbin-Watson and Friedman.
            // Diagnostic only -- nothing downstream reads ctx.check -- but the
            // oracle writes every one of them to the .udg on ANY model run with
            // Lsumm>0 (editor.f:909 defaults Mxcklg to 2*Sp there), which is why
            // 287 of the 331 .udg goldens carry the block and none of it was
            // ported. See core/src/diag/checkres.hpp.
            //
            // editor.f:909-910 -- the LAST of the three Mxcklg defaults, applied
            // here rather than in the parser because it needs Lmodel, and this
            // is the model path. gtinpt.f:1169 (Lseats && still 0 -> 3*Sp) runs
            // first, and getchk's own default first of all.
            if (ctx.chkopt.mxcklg == 0) {
                if (ctx.captured.has_seats) ctx.chkopt.mxcklg = 3 * sp;
                else ctx.chkopt.mxcklg = 2 * sp;
            }
            // arima.f:1046's `IF(Convrg)` again -- prtacf and everything under
            // it are inside it. See the residual-capture note above.
            if (ctx.mdldat.convrg) {
                check_residuals(ctx, a.data(), na, nefobs);
                if (ctx.error.lfatal) return false;
            }

            // savotl.f (the outlier counts) + prtrts.f (the ARMA operator
            // roots) -- the other half of the estimation savelog block, and
            // like check{} above it had no C++ at all. `lidotl` gates only
            // whether the `autoout` line is emitted (savotl.f:152).
            est_diagnostics(ctx, ctx.captured.has_outlier);
            if (ctx.error.lfatal) return false;

            // arima.f:870-905 (amdfct.f) -- the average absolute percentage
            // forecast error over the last three years. Sits here, between the
            // estimation savelog block and the forecasts, exactly as in the
            // Fortran: it forecasts from origins INSIDE the span using the
            // design regvar has already built, so it must run before anything
            // rebuilds Xy for the real forecast. Gated on Var>0 (arima.f:872).
            if (ctx.mdldat.var > 0.0) {
                // gtinpt.f:1203-1216 -- `Outfct` (resolved in gtinpt from
                // estimate{outofsample=}) selects the OUT-OF-SAMPLE variant;
                // amdfct reads it itself. This is the non-automatic caller, so
                // no `lauto`.
                aape_diagnostics(ctx, trnsrs.data());
                if (ctx.error.lfatal) return false;
            }

            // arima.f:1216-1218 -- "If estimation did not converge, exit with
            // an error". A BARE `CALL abend`: the oracle writes nothing here,
            // because whichever prterr already fired owns the message. So this
            // is deliberately not routed through a `*_not_ported` helper and
            // will never appear in walls.py -- it is not a gap, it is the
            // oracle's own halt, and the run's `===ERR===` block carries only
            // what prterr/itrerr put there.
            //
            // Everything from here to the end of the estimation block (the
            // forecasts, the backcasts and arima.f:1180-1213's span restore)
            // lives inside the `IF(Convrg)` this closes, so the early return
            // IS the guard for all of it.
            if (!ctx.mdldat.convrg) {
                abend(ctx);
                return false;
            }

            // arima.f:1121-1129 -- spcrsd, the residual spectrum and its peak
            // WARNING. The numbers were derived in run_spectrum with the other
            // three tables and were bit-exact there; what moved here is the
            // POSITION of the warning in the Mt2 stream, which the oracle
            // writes before x11pt2 runs at all. See run_spectrum.hpp.
            run_residual_spectrum(ctx);
            if (ctx.error.lfatal) return false;

            // NOTE (deferred): forecasting on a post-outlier model with other
            // regressors (TD) is not yet exact. idotlr's coladd/addotl fill only
            // the span rows, so the forecast-period design rows of the inserted
            // outlier column (and the columns coladd shifts) are stale. arima.f
            // rebuilds the full Nobspf-row design via regvar before prtfct
            // (arima.f:1148); a first attempt at that here got close but not
            // bit-exact (and perturbed the previously-exact airline case), so the
            // exact rebuild is left for the outlier-forecast follow-up. Forecasts
            // without identified outliers (e.g. fixed-airline-seats) are exact.

            // Forecasting (arima.f:1164 prtfct, when Nfcst>0). Produces the
            // original-scale point forecast + confidence band on ctx.forecasts.
            // The prior-adjustment/holiday branches of prtfct are out of this
            // slice, so it is exact only without user prior factors (Priadj<=1).
            // arima.f:1144-1158 -- model span: put the span END back BEFORE
            // forecasting, so the forecasts start after the last observation of
            // the series span rather than after the end of the model span. The
            // estimated coefficients are kept; only the series and its design
            // matrix are rebuilt (Begspn is still Begmdl here, hence the +nbeg).
            if (nend > 0) {
                nend_done = true;
                setspn(nend, 0);
                trnfcn(ctx, padj.data() + nbeg, nspobs, ctx.arima.fcntyp,
                       ctx.arima.lam, trnsrs.data());
                if (ctx.error.lfatal) return false;
                int nrxy3 = 0, frstry3 = 0;
                regvar(ctx, trnsrs.data(), nobspf, fctdrp, nfcst, 0,
                       ctx.arima.userx.data(), ctx.arima.bgusrx.data(),
                       ctx.arima.nrusrx, ctx.prior.priadj, ctx.arima.reglom,
                       nrxy3, ctx.arima.begxy.data(), frstry3, true,
                       ctx.arima.elong);
                if (ctx.error.lfatal) return false;
                ctx.arima.nrxy = nrxy3;
            }
            if (nfcst > 0) {
                fcstout(ctx, nfcst, ctx.arima.fctdrp, ctx.arima.ciprob,
                        ctx.arima.lognrm);
                if (ctx.error.lfatal) return false;
            }
            // Backcasting (arima.f:1172-1176 mkback, when Nbcst>0). Must run
            // AFTER the forecasts: it rebuilds Xy/Nrxy over Nspobs rows with the
            // backcast rows included, which is also what adjreg then needs.
            if (ctx.extend.nbcst > 0) {
                bcstout(ctx, ctx.extend.nbcst, trnsrs.data(), ctx.arima.lognrm);
                if (ctx.error.lfatal) return false;
            }
        }
    }

    // arima.f:1180-1213 -- model span: put the span START back, then recopy and
    // re-transform the series over the FULL span and rebuild the design matrix.
    // Everything downstream (extend -> adjreg -> B1 -> X-11) decomposes the whole
    // series span; only the regARIMA fit and the forecasts used the model span.
    // arima.f:1204 re-transforms Nspobs points, not Nobspf, so any retained
    // post-span observations keep the values the narrowed pass left there --
    // ported as written.
    // (The `!nend_done` arm also covers the no-estimation harnesses, which skip
    // the block above: the span must still be restored before X-11 runs.)
    if (have_trn && (nbeg > 0 || (nend > 0 && !nend_done))) {
        setspn(nend, nbeg);
        trnfcn(ctx, padj.data(), nspobs, ctx.arima.fcntyp, ctx.arima.lam,
               trnsrs.data());
        if (ctx.error.lfatal) return false;
        int nrxy4 = 0, frstry4 = 0;
        regvar(ctx, trnsrs.data(), nobspf, fctdrp, nfcst, ctx.extend.nbcst,
               ctx.arima.userx.data(), ctx.arima.bgusrx.data(),
               ctx.arima.nrusrx, ctx.prior.priadj, ctx.arima.reglom, nrxy4,
               ctx.arima.begxy.data(), frstry4, true, ctx.arima.elong);
        if (ctx.error.lfatal) return false;
        ctx.arima.nrxy = nrxy4;
        // Refresh the Tsrs/Adj snapshots the later phases read (SEATS takes its
        // decomposition input straight off ctx.series.tsrs), or they keep the
        // model-span series while every span pointer says full span -- which
        // silently slides the whole decomposition by nbeg periods.
        {
            int ncp = nobspf < prm::PLEN ? nobspf : prm::PLEN;
            copy(trnsrs.data(), ncp, 1, ctx.series.tsrs.data());
            copy(facspn, ncp, 1, ctx.adj.adj.data() + (adj1st_cur - 1));
            ctx.adj.adj1st = adj1st_cur;
        }
    }
    if ((nbeg > 0 || nend > 0) && have_trn) {
        if (out_trnsrs) *out_trnsrs = trnsrs;
        if (out_nobspf) *out_nobspf = nobspf;
    }

    return !ctx.error.lfatal;
}

}  // namespace x13
