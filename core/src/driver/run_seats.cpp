// run_seats.cpp -- SEATS driver phase: parse a spec, estimate the regARIMA
// model (SEATS has no direct/no-model path in the oracle -- see
// tools/seats_scope.md section 1), then dispatch to the SEATS canonical
// decomposition.
//
// STATUS: the historical-span decomposition (decode -> canonical denoms ->
// SPECTRU -> DecompSpectrum -> ESTBUR's general-branch solve, see
// core/src/seats/estbur.cpp) is wired and gates bit-exact for the whole SEATS
// corpus, including general-shape p>0/bp>0, the imean!=0 drift case (a
// Constant/mean regressor, d-agnostic), AND a mean ALONGSIDE other regressors
// (TD/outliers): the mean is added back onto the regression-adjusted series
// while the other effects stay removed (see the mean handling below), and the
// s16/s18 combined-adjustment factors refold the removed effects + lom/leap
// prior (run_pre_model seats_combined_orig -> estbur combined_*). This function
// only signals success/failure; tools/x13run_seats.cpp re-runs the same
// (idempotent, side-effect-free) chain to dump the tables. Falls back to
// seats_not_ported() only when the decomposition chain itself fails.
#include "specparse/specparse.hpp"
#include "driver/run_seats.hpp"  // seats_restore_mean, seats_decompose
#include "driver/run_history.hpp"     // run_history (revdrv.f, Lseats)
#include "driver/x11_prestage.hpp"  // x11_prestage (x11ari.f:60-199, shared with run_x11)
#include "diag/genqs.hpp"      // genqs / gennpsa (QS + NP seasonality, x11ari.f:277/322)
#include "gen/model.hpp"       // prm::PRGTCN (mean-regressor type), prm::DIFF
#include "regarima/regvar.hpp" // ratpos (rebuild the undifferenced Constant column)
#include "seats/canonical_denoms.hpp"
#include "seats/decompspectrum.hpp"
#include "seats/estbur.hpp"
#include "seats/model_decode.hpp"
#include "seats/seatopts.hpp"
#include "seats/spectru.hpp"
#include "x11/x11filt.hpp"     // addmul (invert seatad's Adjsea divide)
#include "gen/notset.hpp"      // prm::DNOTST
#include "numeric/numeric.hpp" // dpeq

#include <cmath>
#include <string>
#include <vector>

namespace x13 {

namespace {
// Signal that a specific SEATS path is not yet ported (the decomposition chain
// failing) -- most of SEATS is ported and gates bit-exact (mirrors
// x11parts.cpp's local x11_not_ported).
void seats_not_ported(X13Context& ctx, const char* what) {
    errhdr(ctx);
    writln(ctx, std::string("ERROR: ") + what + " not yet ported (SEATS decomposition).",
           stdio::STDERR, ctx.units.mt2, true);
    abend(ctx);
}

// Publish the decomposition into the four /seatcm/ buffers the QS and NP
// diagnostics read, plus their /seatlg/ presence flags.
//
// The oracle fills these through the ansub9.f USRENTRY bridge from inside
// seats(): `sa` arrives twice, as 1309 -> Seatsa (1..Nz) and 1203 -> Stocsa
// (1..Nz+lfor), and `ir` twice, as 1312 -> Seatir and 1204 -> Stocir. They are
// the SAME arrays at that point; what separates them is seatad.f, which
// post-processes only the Seat* pair -- :27's `/100` on Seatir under Muladd!=1,
// and on Seatsa the forecast append (Posfob+1.. only) and the Adjsea==1 Facsea
// divide. This port has no pre-seatad buffer to copy, so the Stoc* pair is
// reconstructed by inverting exactly those two steps over [Pos1ob,Posfob].
//
// Anchored at Pos1ob because ansub9 stores at `i+Pos1ob-1`, and taken from
// ctx.seats_* (which the span drivers save/restore) rather than from inside
// seats_decompose, so a slidingspans/history replay cannot leave the LAST
// span's components here -- the same snapshot discipline /x11srs/, /lkhd/ and
// ctx.d8bd9a already need. It matches the oracle's own ansub9.f:110 guard
// (`IF(Issap.eq.2.or.Irev.eq.4)RETURN`), which skips the Stoc* store on a
// replay outright.
void publish_seats_commons(X13Context& ctx) {
    const int pos1ob = ctx.x11ptr.pos1ob;
    const int posfob = ctx.x11ptr.posfob;
    const int muladd = ctx.x11opt.muladd;
    const int adjsea = ctx.x11adj.adjsea;

    auto store = [&](const std::vector<double>& src, farray1<double, 1020>& seat,
                     farray1<double, 1020>& stoc, bool ir, bool& have) {
        have = false;
        const int n = static_cast<int>(src.size());
        for (int k = 0; k < n; ++k) {
            const int i = pos1ob + k;
            if (i < 1 || i > 1020) continue;
            const double v = src[static_cast<std::size_t>(k)];
            seat(i) = v;
            // Invert seatad. The irregular: undo the `/100`, which seatad only
            // applied under Muladd!=1. The SA series: undo the Facsea divide,
            // which only happened with a regARIMA seasonal regressor -- no
            // corpus spec pairs one with seats{}, so this arm is transcribed
            // from seatad.f:56 rather than measured.
            stoc(i) = ir ? (muladd != 1 ? v * 100.0 : v) : v;
            if (i >= pos1ob && i <= posfob && !dpeq(v, 0.0)) have = true;
        }
        if (!ir && adjsea == 1)
            addmul(stoc.data(), ctx.x11fac.facsea.data(), stoc.data(), pos1ob,
                   posfob, muladd);
    };

    store(ctx.seats_sa, ctx.seatcm.seatsa, ctx.seatcm.stocsa, /*ir=*/false,
          ctx.seatlg.hvstsa);
    store(ctx.seats_ir, ctx.seatcm.seatir, ctx.seatcm.stocir, /*ir=*/true,
          ctx.seatlg.hvstir);
}

// Imean!=0: the model carries a Constant (mean) regressor. SEATS keeps the mean
// IN the decomposed series and folds it back via wm-centering of the CALCFX
// seeds + za in the FCAST extension + wmf/wmb in ESTBUR's general branch
// (analts.f:1969-1979, ansub1.f:2166-2181, ansub3.f zaf/zab). That path IS
// ported in estbur.cpp and is d-agnostic (gated bit-exact for BOTH d>=1 drift
// and d==0). A mean ALONGSIDE other regressors (TD/outliers) is also gated: the
// Constant's contribution is added back onto the fully-adjusted series while the
// other effects stay removed (below). See tools/seats_general_scope.md.
//
// NOTE this is deliberately NOT L_IMEAN (SeatsOptions::imean). L_IMEAN decides
// whether SEATS MODELS a mean (the wm centering / za seed inside estbur), and
// seats{imean=yes/no} can override it in either direction; what SERIES gets
// decomposed is settled upstream in regARIMA (chkadj/regeff/adjreg never strip
// the Constant), so the add-back below stays keyed on the model actually
// carrying a Constant regressor. Both mismatched combinations gate bit-exact
// (*_imean-yes-seats: no Constant, L_IMEAN=1; *_imean-no-seats: Constant
// present, L_IMEAN=0).
bool seats_has_mean(const X13Context& ctx) {
    const auto& M = ctx.model;
    for (int i = 1; i <= M.nb; ++i)
        if (M.rgvrtp(i) == prm::PRGTCN) return true;
    return false;
}
}  // namespace

bool run_seats(X13Context& ctx, const std::string& spec_text, const std::string& base) {
    if (!parse_spec(ctx, spec_text, base)) return false;
    if (!ctx.captured.has_series) return false;
    if (!ctx.captured.has_seats) return false;
    // transform{constant=} is now parsed and applied (the whole series is
    // shifted up in parse_spec). SEATS takes it back out of its own published
    // components at seatpr.f:211-390, which is unported -- so fatal rather than
    // publish a decomposition that is uniformly `constant` too high.
    if (!dpeq(ctx.adj.cnstnt, prm::DNOTST)) {
        seats_not_ported(ctx, "transform{constant=} through SEATS "
                              "(seatpr.f:211-390 constant removal)");
        return false;
    }

    // SEATS is always model-based (analts.f: the fitted regARIMA model feeds
    // SEATS through NMLSTS/ss2rv -- see tools/seats_scope.md section 1). Reuse
    // the shared pre-model + estimate/forecast phase run_x11's model path
    // uses; a spec that fails estimation fatals here exactly as the oracle
    // would before ever reaching SEATS.
    std::vector<double> trnsrs;
    int nobspf_m = 0;
    if (!run_m2_after_parse(ctx, base, /*estimate=*/true, &trnsrs, &nobspf_m))
        return false;
    if (ctx.error.lfatal) return false;

    // Imean!=0: SEATS decomposes the series with the mean (drift) kept IN, but
    // with every OTHER regression effect (TD / holiday / outliers) removed --
    // the linearized series arima.f:1337 hands to SEATS (Orixs = orixmv), where
    // chkadj/regeff/adjreg strip the calendar/outlier effects and the Constant
    // regressor is excluded by construction (regeff has no PRGTCN branch). After
    // estimation ctx.series.tsrs = trnsrs - X*b has ALL effects removed,
    // including the Constant's drift. Add the Constant's fitted contribution
    // b_const * Xconst back -- Xconst is the regvar.cpp case-10 column, a column
    // of ones filtered by 1/Diff(B) (i.e. the drift ramp) -- to restore exactly
    // the mean while leaving the other regressors removed. For a Constant-only
    // model this reproduces the clean transformed series trnsrs (raw log);
    // alongside TD/outliers it keeps those removed. d-agnostic: the drift ramp +
    // the wm centering in estbur carry the mean regardless of d.
    seats_restore_mean(ctx);

    // x11ari.f:60-199 -- the oracle reaches SEATS through the SAME adjustment
    // entry X-11 uses: the editor's span/filter setup, setxpt, x11int, x11pt1 and
    // (at :199, gated `(.not.Lcmpaq).or.Lx11`, true for a non-composite run
    // either way) x11pt2 all run, and only THEN does :204-243 substitute the
    // SEATS chain for x11pt3. This port had two separate drivers and run_seats
    // skipped the whole pre-stage -- invisible for the decomposition (SEATS reads
    // ctx.series.tsrs and the fitted model, not the X-11 buffers), but it left
    // the span GEOMETRY unset (Length from x11pt2, Pos1ob from setxpt, Lyr from
    // editor.f:235, the ssprep snapshot), which is exactly what setssp/
    // run_x11_span read -- so slidingspans{}/history{} under seats{} were
    // silently dropped. Runs AFTER seats_restore_mean so the mean add-back still
    // happens on the pristine post-estimation tsrs.
    if (!x11_prestage(ctx, /*has_model=*/true, trnsrs, /*lseats=*/true,
                      /*lx11=*/false)) return false;

    if (!seats_decompose(ctx)) return false;

    // x11ari.f returns to x12run.f, which then calls sspdrv/revdrv -- the span
    // drivers are NOT part of the adjustment, they replay it. They are reached
    // identically from the SEATS path (both take Lseats and hand it to x11ari),
    // so run_seats needs the same tail run_x11 has.
    //
    // Both drivers rewrite the X-11 buffers, the span pointers AND -- new here
    // -- ctx.seats_* in place, because each span republishes its own
    // decomposition. The oracle punches the main run's tables before sspdrv
    // runs; this port dumps at exit, so the main decomposition has to be put
    // back afterwards. Same snapshot discipline run_x11.cpp already applies to
    // /x11srs/, /adxser/, /x11fac/, /x11ptr/ and /lkhd/.
    if (ctx.captured.has_slidingspans || ctx.captured.has_history) {
        // The restore set is run_x11.cpp's, plus the three things only the
        // SEATS path has: the published components (ctx.seats_*), the
        // decomposition INPUT (ctx.series.tsrs -- each span's rgarma leaves its
        // own residuals there and seats_restore_mean then adds that span's mean
        // back), and Nspobs (the window length every consumer derives its row
        // count from). tools/x13run_seats.cpp re-derives the whole ESTBUR chain
        // from ctx AFTER this returns, so a miss here shows up as the LAST
        // SPAN's decomposition under the main run's dates.
        const lkhd_cmn lkhd_main = ctx.lkhd;
        const auto x11srs_main = ctx.x11srs;
        const auto adxser_main = ctx.adxser;
        const auto x11fac_main = ctx.x11fac;
        const auto x11ptr_main = ctx.x11ptr;
        const auto mdlbegspn_main = ctx.mdldat.begspn;
        const int nspobs_main = ctx.mdldat.nspobs;
        const auto tsrs_main = ctx.series.tsrs;
        const bool seats_ran_main = ctx.seats_ran;
        const auto sa_main = ctx.seats_sa;
        const auto trend_main = ctx.seats_trend;
        const auto ir_main = ctx.seats_ir;
        const auto cycle_main = ctx.seats_cycle;
        const auto seasadd_main = ctx.seats_seasonal_add;
        const auto cmbadd_main = ctx.seats_combined_add;
        const auto cmbfac_main = ctx.seats_combined_factor;
        const auto d8bd9a_main = ctx.d8bd9a;

        // x12run.f:225/257 order: sspdrv then revdrv, both after x11ari.
        const int begspn_full[2] = {ctx.mdldat.begspn(1), ctx.mdldat.begspn(2)};
        const int endmdl_full[2] = {ctx.arima.endmdl(1), ctx.arima.endmdl(2)};
        bool ok = run_slidingspans(ctx, trnsrs);
        if (ok && !ctx.error.lfatal)
            ok = run_history(ctx, trnsrs, begspn_full, nspobs_main, ctx.extend.nfcst,
                             endmdl_full);

        ctx.lkhd = lkhd_main;
        ctx.x11srs = x11srs_main;
        ctx.adxser = adxser_main;
        ctx.x11fac = x11fac_main;
        ctx.x11ptr = x11ptr_main;
        ctx.mdldat.begspn = mdlbegspn_main;
        ctx.mdldat.nspobs = nspobs_main;
        ctx.series.tsrs = tsrs_main;
        ctx.seats_ran = seats_ran_main;
        ctx.seats_sa = sa_main;
        ctx.seats_trend = trend_main;
        ctx.seats_ir = ir_main;
        ctx.seats_cycle = cycle_main;
        ctx.seats_seasonal_add = seasadd_main;
        ctx.seats_combined_add = cmbadd_main;
        ctx.seats_combined_factor = cmbfac_main;
        ctx.d8bd9a = d8bd9a_main;

        if (!ok || ctx.error.lfatal) return false;
    }

    // x11ari.f:272-326 -- the diagnostics block sits AFTER the Lseats/Lx11
    // branch and is common to both, so a SEATS run reaches genqs and gennpsa
    // exactly as an X-11 run does. Neither is gated on any spec (see genqs.cpp),
    // and the publish has to come first because the SEATS arms read /seatcm/.
    //
    // spcdrv sits BETWEEN them in the oracle and is deliberately not called
    // here: its SEATS branch reads Hvstsa/Hvstir over a different construction
    // again (spcdrv.f:299/436) and is the separate open front tracked in
    // tools/spectrum_peaks_scouting.md. gennpsa reads none of spcdrv's state, so
    // the order between the two that ARE ported is preserved.
    publish_seats_commons(ctx);
    if (!genqs(ctx, /*lseats=*/true)) return false;
    if (!gennpsa(ctx, /*lseats=*/true)) return false;
    return true;
}

void seats_restore_mean(X13Context& ctx) {
    if (seats_has_mean(ctx)) {
        auto& M = ctx.model;
        auto& D = ctx.mdldat;
        int icon = 0;
        for (int i = 1; i <= M.nb; ++i)
            if (M.rgvrtp(i) == prm::PRGTCN) { icon = i; break; }
        const int nsp = D.nspobs;
        // Xconst = 1/Diff(B) applied to a column of ones (regvar.cpp:159-162).
        std::vector<double> xc(static_cast<std::size_t>(nsp), 1.0);
        ratpos(nsp, D.arimap.data(), M.arimal.data(), M.opr.data(),
               M.mdl(prm::DIFF - 1), M.mdl(prm::DIFF) - 1, nsp, xc.data());
        const double bcon = (icon > 0) ? D.b.data()[icon - 1] : 0.0;
        for (int i = 0; i < nsp; ++i)
            ctx.series.tsrs(i + 1) += bcon * xc[i];
    }
}

// (ctx.seats_combined_orig -- the raw original series feeding the s16/s18
// combined-adjustment factors -- is stashed in run_pre_model, where the raw
// untransformed a1 and the lom/leap prior are directly available.)

// decode -> canonical denoms -> SPECTRU -> DecompSpectrum -> ESTBUR
// (historical span only; see estbur.hpp for exact scope/limits).
bool seats_decompose(X13Context& ctx) {
    try {
        // NOTE opts.finite (seats{finite=}, /setopt/ Lfinit) is deliberately NOT
        // read here. It gates getDiag (sigex.f:1502) -- the finite-sample
        // filter/gain/time-shift save tables, the SEATS savelog keys and the
        // out=0 error-analysis print tables -- but never the decomposition:
        // every table written in both modes is byte-identical across a 24-
        // configuration oracle probe, gated by *_finite-seats. See the
        // measurement note in seats/seatopts.hpp and tools/census_bugs.md CB-16.
        SeatsOptions opts = seats_resolve_options(ctx);
        SeatsModelOrders mo;
        if (seats_decode_model(ctx, opts.xl, mo)) {
            // seats{} option scope guards (seatopts.cpp): CHANGEMODEL's model
            // rewrite and seats{bias=-1}'s BIASCORR are unported, so fatal
            // rather than decompose the wrong thing behind an OUTCOME: OK.
            const bool is_log = std::fabs(ctx.arima.lam) < 1e-9;
            if (const char* why =
                    seats_model_unported_reason(opts, mo, is_log)) {
                seats_not_ported(ctx, why);
                return false;
            }

            SeatsCanonicalDenoms cd;
            seats_canonical_denoms(mo, opts.rmod, opts.epsphi, cd);

            SpectruResult sr;
            spectru(cd.thstar, cd.qstar, cd.chi, cd.nchi, cd.cyc, cd.ncyc,
                    cd.psi, cd.npsi, cd.pstar, mo.mq, mo.bd, mo.d, /*out=*/1,
                    /*har=*/0, cd.root0c, cd.rootpic, cd.rootpis, sr);

            SeatsComponentModels comp;
            decomp_spectrum(sr, cd, cd.is_close_to_td, comp);

            // The admissibility verdict (seats{noadmiss=}): with an
            // inadmissible decomposition the oracle either aborts SEATS
            // outright (noadmiss=no) or APPROXIMATEs + re-estimates the model
            // (noadmiss=yes). Both fatal here.
            if (const char* why = seats_decomp_unported_reason(
                    opts, sr.qt1, sr.ncycth, cd.ncyc, comp.varwnc)) {
                seats_not_ported(ctx, why);
                return false;
            }

            EstburResult est;
            estbur_historical(ctx, mo, cd, comp, opts, est);
            if (est.ok) {
                // Keep the decomposition on the context. The CLI harness re-runs
                // this whole (idempotent, side-effect-free) chain to dump its
                // tables, but the C ABI has no business re-deriving it, so the
                // driver publishes what it computed. Pure bookkeeping -- no
                // numeric path reads these back.
                ctx.seats_ran = true;
                ctx.seats_sa = est.sa;
                ctx.seats_trend = est.trend;
                ctx.seats_ir = est.ir;
                ctx.seats_cycle = est.cycle;
                ctx.seats_seasonal_add = est.seasonal_add;
                ctx.seats_combined_add = est.combined_add;
                ctx.seats_combined_factor = est.combined_factor;
                return true;
            }
        }
    } catch (const std::exception&) {
        // Fall through to seats_not_ported below.
    }

    seats_not_ported(ctx,
        "SEATS historical-span decomposition (ESTBUR general-branch solve) -- "
        "either an unsupported model shape (p>0/bp>0/imean!=0) or the chain "
        "failed; see tools/seats_scope.md");
    return false;
}

}  // namespace x13
