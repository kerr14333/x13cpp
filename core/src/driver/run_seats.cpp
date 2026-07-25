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
#include "gen/model.hpp"       // prm::PRGTCN (mean-regressor type), prm::DIFF
#include "regarima/regvar.hpp" // ratpos (rebuild the undifferenced Constant column)
#include "seats/canonical_denoms.hpp"
#include "seats/decompspectrum.hpp"
#include "seats/estbur.hpp"
#include "seats/model_decode.hpp"
#include "seats/seatopts.hpp"
#include "seats/spectru.hpp"

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

// Imean!=0: the model carries a Constant (mean) regressor. SEATS keeps the mean
// IN the decomposed series and folds it back via wm-centering of the CALCFX
// seeds + za in the FCAST extension + wmf/wmb in ESTBUR's general branch
// (analts.f:1969-1979, ansub1.f:2166-2181, ansub3.f zaf/zab). That path IS
// ported in estbur.cpp and is d-agnostic (gated bit-exact for BOTH d>=1 drift
// and d==0). A mean ALONGSIDE other regressors (TD/outliers) is also gated: the
// Constant's contribution is added back onto the fully-adjusted series while the
// other effects stay removed (below). See tools/seats_general_scope.md.
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
    // (ctx.seats_combined_orig -- the raw original series feeding the s16/s18
    // combined-adjustment factors -- is stashed in run_pre_model, where the raw
    // untransformed a1 and the lom/leap prior are directly available.)

    // decode -> canonical denoms -> SPECTRU -> DecompSpectrum -> ESTBUR
    // (historical span only; see estbur.hpp for exact scope/limits).
    try {
        // NOTE opts.finite (seats{finite=}, /setopt/ Lfinit) is deliberately NOT
        // read here. It gates getDiag (sigex.f:1502) -- the finite-sample
        // filter/gain/time-shift save tables, the SEATS savelog keys and the
        // out=0 error-analysis print tables -- but never the decomposition:
        // every table written in both modes is byte-identical across a 24-
        // configuration oracle probe, gated by *_finite-seats. See the
        // measurement note in seats/seatopts.hpp and tools/census_bugs.md CB-14.
        SeatsOptions opts = seats_resolve_options(ctx);
        SeatsModelOrders mo;
        if (seats_decode_model(ctx, opts.xl, mo)) {
            SeatsCanonicalDenoms cd;
            seats_canonical_denoms(mo, opts.rmod, opts.epsphi, cd);

            SpectruResult sr;
            spectru(cd.thstar, cd.qstar, cd.chi, cd.nchi, cd.cyc, cd.ncyc,
                    cd.psi, cd.npsi, cd.pstar, mo.mq, mo.bd, mo.d, /*out=*/1,
                    /*har=*/0, cd.root0c, cd.rootpic, cd.rootpis, sr);

            SeatsComponentModels comp;
            decomp_spectrum(sr, cd, cd.is_close_to_td, comp);

            EstburResult est;
            estbur_historical(ctx, mo, cd, comp, est);
            if (est.ok) return true;
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
