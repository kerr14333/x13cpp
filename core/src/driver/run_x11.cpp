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
#include "driver/x11_prestage.hpp"  // x11_prestage (x11ari.f:60-199, shared with run_seats)
#include "x11/x11parts.hpp"   // x11pt3
#include "x11/x11summ.hpp"    // x11pt4_partf (Part-F summary measures + f3cal)
#include "x11/x11drv.hpp"     // setxpt, x11int, chkadj, regeff, extend, adjreg
#include "regarima/regvar.hpp"   // regvar (design rebuild for regression effects)
#include "regarima/priadj.hpp"   // adjsrs_factors (prior-adjustment factor series)
#include "numeric/numeric.hpp"   // dpeq (tdprior weight resolution)
#include "notset.hpp"         // prm::NOTSET
#include "x11/slidingspans.hpp"  // ssprep_snapshot, restor_span, run_slidingspans
#include "x11/x11easter.hpp"      // holday (classic X-11 Easter estimation)
#include "driver/run_history.hpp"  // run_history
#include "driver/run_spectrum.hpp"  // run_spectrum
#include "composite/agr2.hpp"       // agr2_component / agr2_compare (composite)
#include "composite/agr3.hpp"       // agr3, agrxpt (indirect adjustment)

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

    // x11ari.f:60-199 -- the shared X-11 PRE-STAGE (editor span/filter setup ->
    // setxpt -> x11int -> x11pt1 -> the model extend/adjreg glue -> x11pt2).
    // Lives in driver/x11_prestage.cpp because run_seats takes the identical
    // path: the oracle has ONE adjustment entry and only branches after x11pt2.
    if (!x11_prestage(ctx, has_model, trnsrs, /*lseats=*/false)) return false;

    const int* begspn = ctx.mdldat.begspn.data();
    const int nspobs = ctx.mdldat.nspobs;
    const int nfcst = ctx.extend.nfcst;
    const bool lgraf = false;
    // x11pt3 (D8..D16 finals): D10=Sts, D11=Stci, D12=Stc, D13=Sti. It consumes
    // the D7-return state x11pt2 leaves. (D8/D9 read x11pt2's /mq10/ Stex, which
    // is function-local in this port and does not persist -- the D10..D13/D16
    // finals do not depend on it, so they gate correctly regardless.)
    x11pt3(ctx, lgraf, /*lttc=*/false);
    if (ctx.error.lfatal) return false;

    // x11pt4.f's savelog point: freeze the D8/B1 seasonality-test battery here,
    // before the sliding-spans / history replays below re-run x11pt3 and
    // overwrite /tests/ with a sub-span's statistics.
    ctx.x11_f2tests = ctx.tests;
    ctx.x11_f2tests_set = true;

    // x11pt4.f's PART F: the summary measures (f2.a*/b*/c*/d/e/f/g, MCD) and the
    // f3cal quality statistics (M1-M11, Q, Q2). Same snapshot discipline -- and
    // the same placement, before the span replays. x11_sti_int is empty only
    // when x11pt3 took its Khol==1 early return, where the oracle emits no F
    // block either.
    // Part E first (x11pt4.f:162-319): the E5-E8 change tables, E11, E18 and the
    // total adjustment factors, all read off the LIVE buffers before Part F makes
    // its working copies. ctx.x11srs.stc is the PUBLISHED (LS/TC-folded) trend,
    // i.e. the oracle's Stc2, which is what E7 wants when the shift belongs in
    // the trend; ctx.x11_stc_int is the internal one.
    if (!ctx.x11_sti_int.empty())
        x11pt4_etables(ctx, ctx.x11_stc_int.data(), ctx.x11srs.stc.data(),
                       /*lttc=*/false);
    if (!ctx.x11_sti_int.empty() &&
        x11pt4_partf(ctx, ctx.x11_sti_int.data(), ctx.x11_stc_int.data())) {
        ctx.x11_f2inpt2 = ctx.inpt2;
        ctx.x11_f2work2 = ctx.work2;
        ctx.x11_f2mcd = ctx.x11opt.mcd;
        ctx.x11_f2ratic = ctx.x11opt.ratic;
        ctx.x11_f2ratis = ctx.x11opt.ratis;
        ctx.x11_f3_set = true;
    }

    // slidingspans{} (ssap.f/sspdrv.f/ssrit.f): replay the model+X11 pipeline
    // over each sub-span (driver/run_x11_span.hpp -- the re-entrant driver),
    // producing the sfs/chs cross-span stability tables on ctx.ssout. No-op
    // when slidingspans{} was not requested (ctx.hiddn.issap != 1). trnsrs is
    // empty on the no-model path; run_x11_span only reads it when has_model.
    // Capture the main run's own geometry BEFORE the span drivers overwrite the
    // ctx copies (run_slidingspans / run_x11_span reset ctx.mdldat.begspn to
    // their last span's value; `nspobs`/`nfcst` are already const locals holding
    // the main-run values, but `begspn` aliases the live ctx.mdldat buffer).
    const int begspn_full[2] = {begspn[0], begspn[1]};
    // ... and the main run's Endmdl (revdrv.f:246's mdl2). run_x11_span
    // overwrites ctx.arima.endmdl with each span's own end, so history{} cannot
    // read it back after run_slidingspans has run.
    const int endmdl_full[2] = {ctx.arima.endmdl(1), ctx.arima.endmdl(2)};

    // spectrum{} (spcdrv.f): the data-based periodogram diagnostic (sp0/sp1/sp2).
    // Runs on the pristine main-run X-11 state -- BEFORE the sliding-spans/history
    // span replays below, which mutate ctx.mdldat.begspn/nspobs and the x11ptr
    // span pointers that run_spectrum reads for Bgspec/begbk2. This mirrors the
    // oracle flow: spcdrv is part of the main x11ari pass (after x11pt4), while
    // slidingspans{}/history{} are separate re-runs. No-op when spectrum{} was
    // absent (ctx.spcout.requested false).
    if (!run_spectrum(ctx)) return false;

    // composite{} (x11ari.f:372-373): if this run is a COMPONENT of a composite
    // adjustment (series{comptype=...} set Iag>=0 and the metafile driver carried
    // Iagr>0 in), accumulate it into the aggregation buffers. Iagr==3 means this
    // run IS the composite total, which accumulates nothing. No-op for every
    // single-series run (Iag<0). See tools/composite_scouting.md.
    if (ctx.agr.iagr > 0 && ctx.agr.iagr != 3 && ctx.agr.iag >= 0) {
        if (!agr2_component(ctx)) {
            writln(ctx, "ERROR: Component series has a non-overlapping time "
                        "span.  Aggregation not computed.",
                   ctx.units.mt2, ctx.units.mt2, true);
            return false;
        }
    }
    // x11ari.f:333-341: this run IS the composite total (Iagr==3), so after its
    // own DIRECT adjustment above, rebuild the D-tables from the aggregated
    // component results -- the INDIRECT adjustment. (Ixreg/Kswv are zeroed
    // around it in the oracle; neither is set on this path.)
    if (ctx.agr.iagr == 3) {
        // agr3 REPLACES the D-table buffers with the indirect adjustment, so
        // snapshot the composite total's own DIRECT d10-d13 first. Not an oracle
        // step (the oracle has already printed/punched them by this point); this
        // port defers all output to the caller, so the caller needs both sets.
        constexpr int PLEN_D = 1020;
        ctx.agr_direct_d10.assign(ctx.x11srs.sts.data(), ctx.x11srs.sts.data() + PLEN_D);
        ctx.agr_direct_d11.assign(ctx.x11srs.stci.data(), ctx.x11srs.stci.data() + PLEN_D);
        ctx.agr_direct_d12.assign(ctx.x11srs.stc.data(), ctx.x11srs.stc.data() + PLEN_D);
        ctx.agr_direct_d13.assign(ctx.x11srs.sti.data(), ctx.x11srs.sti.data() + PLEN_D);
        agr3(ctx, begspn_full);
        if (ctx.error.lfatal) return false;
        // x11ari.f:341 -- the SAME x11pt4 over the indirect buffers agr3 just
        // installed, producing the `if2.*`/`if3.*` diagnostics block. Its inputs
        // are the indirect analogues: Sti is already the final indirect irregular
        // and Stc the pre-level-shift trend filter output (Stc2 the folded,
        // published one), so there is no internal-vs-published split to undo the
        // way x11pt3 needs on the direct side. The direct block was snapshotted
        // above, so overwriting /inpt2/, /work2/ and Mcd here is safe.
        x11pt4_etables(ctx, ctx.x11srs.stc.data(), ctx.x11srs.stc2.data(),
                       /*lttc=*/false);
        if (x11pt4_partf(ctx, ctx.x11srs.sti.data(), ctx.x11srs.stc.data())) {
            ctx.agr_f2inpt2 = ctx.inpt2;
            ctx.agr_f2work2 = ctx.work2;
            ctx.agr_f2tests = ctx.tests;
            ctx.agr_f2mcd = ctx.x11opt.mcd;
            ctx.agr_f2ratic = ctx.x11opt.ratic;
            ctx.agr_f2ratis = ctx.x11opt.ratis;
            ctx.agr_f3_set = true;
        }
        // x11ari.f:372-373: agr3 leaves Iagr==4, which routes the SAME agr2 call
        // into its comparison-statistics branch -- direct vs indirect roughness,
        // and the restore of the direct pointer geometry.
        agr2_compare(ctx, begspn_full);
    }

    // --- the span-replay diagnostics (x12run.f:225/257) ----------------------
    // sspdrv and revdrv are separate re-runs that x12run.f calls AFTER x11ari
    // has finished -- which includes x11ari's own composite tail above. Order
    // matters for a composite run in both directions: agr2_component reads the
    // main run's D-table buffers (a span replay overwrites them), and revdrv's
    // indirect revision table is printed by the TOTAL, whose Iagr the oracle has
    // already moved 4 -> 5 in agr2 by the time revdrv runs.
    //
    // Both span drivers re-estimate the model per span, so each span's prlkhd
    // overwrites /lkhd/ (the reported log likelihood + AIC/AICC/BIC/HQ). The
    // oracle has the same overwrite but writes its .udg before revdrv runs,
    // whereas this harness dumps at exit -- so keep the main run's values.
    const lkhd_cmn lkhd_main = ctx.lkhd;
    // The same hazard, one level up: a span replay is a full x11pt1->x11pt3 pass,
    // so it rewrites the published D-tables (/x11srs/), the Part-E / forced
    // series (/adxser/, including Stci2 and Stcirn), the factor buffers
    // (/x11fac/) and the span geometry (/x11ptr/ and Begspn) IN PLACE. The oracle
    // punches every one of those during the main pass, before sspdrv/revdrv ever
    // run; this harness dumps at exit, so without the restore below every table
    // it prints for a slidingspans{}/history{} spec is the LAST SPAN's -- wrong
    // values under wrong dates. (Nothing gated this until a spec carried force{}
    // and slidingspans{} together: saa/ffc came back as 84 rows starting seven
    // years late.) Same discipline as the ctx.x11_f2tests snapshot above.
    const auto x11srs_main = ctx.x11srs;
    const auto adxser_main = ctx.adxser;
    const auto x11fac_main = ctx.x11fac;
    const auto x11ptr_main = ctx.x11ptr;
    const auto mdlbegspn_main = ctx.mdldat.begspn;
    const std::vector<double> frcfac_main = ctx.x11_frcfac;
    // x11regression{}: each span demotes Ixreg and re-estimates its own prior-TD
    // factor into ctx.x11_faccal_prior (run_x11_span -> xrgdrv). Both are main-run
    // state here -- Ixreg==3 is what x11pt1 keys its Faccal restore on -- so put
    // them back for anything that reads the main adjustment afterwards.
    const int ixreg_main = ctx.hiddn.ixreg;
    const std::vector<double> faccal_main = ctx.x11_faccal_prior;
    // prtd8b/prtd9a write straight onto ctx from inside x11pt3, so a span
    // replay overwrites them with that span's own extremes and moving-
    // seasonality ratios -- measured: airline_slidingspans reported the LAST
    // span's d9a on every row. Same class as /x11srs/ and ctx.x11_f2tests
    // above; the oracle punches its .udg before sspdrv/revdrv run.
    const auto d8bd9a_main = ctx.d8bd9a;
    const double d11f_main[4] = {ctx.x11_d11f, ctx.x11_d11f_prob,
                                 ctx.x11_d11f3y, ctx.x11_d11f3y_prob};
    const bool d11f_set_main[2] = {ctx.x11_d11f_set, ctx.x11_d11f3y_set};
    const auto autosf_main = ctx.x11_autosf_msr;
    const int sfmsr_main = ctx.x11_sfmsr_filter;
    const int d7trend_main = ctx.x11_d7trendma;
    const int fintrend_main = ctx.x11_finaltrendma;

    if (!run_slidingspans(ctx, trnsrs)) return false;

    // history{} (revchk.f/setrvp.f/revdrv.f/getrev.f/prtrev.f): the expanding-
    // span concurrent-vs-final revisions analysis, built on the same re-entrant
    // driver (driver/run_history.hpp). No-op when history{} was absent.
    if (!run_history(ctx, trnsrs, begspn_full, nspobs, nfcst, endmdl_full))
        return false;

    ctx.lkhd = lkhd_main;
    ctx.x11srs = x11srs_main;
    ctx.adxser = adxser_main;
    ctx.x11fac = x11fac_main;
    ctx.x11ptr = x11ptr_main;
    ctx.mdldat.begspn = mdlbegspn_main;
    ctx.x11_frcfac = frcfac_main;
    ctx.hiddn.ixreg = ixreg_main;
    ctx.x11_faccal_prior = faccal_main;
    ctx.d8bd9a = d8bd9a_main;
    ctx.x11_d11f = d11f_main[0];
    ctx.x11_d11f_prob = d11f_main[1];
    ctx.x11_d11f3y = d11f_main[2];
    ctx.x11_d11f3y_prob = d11f_main[3];
    ctx.x11_d11f_set = d11f_set_main[0];
    ctx.x11_d11f3y_set = d11f_set_main[1];
    ctx.x11_autosf_msr = autosf_main;
    ctx.x11_sfmsr_filter = sfmsr_main;
    ctx.x11_d7trendma = d7trend_main;
    ctx.x11_finaltrendma = fintrend_main;

    return !ctx.error.lfatal;
}

}  // namespace x13
