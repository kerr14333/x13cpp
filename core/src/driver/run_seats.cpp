// run_seats.cpp -- SEATS driver phase: parse a spec, estimate the regARIMA
// model (SEATS has no direct/no-model path in the oracle -- see
// tools/seats_scope.md section 1), then dispatch to the SEATS canonical
// decomposition.
//
// STATUS: the historical-span decomposition (decode -> canonical denoms ->
// SPECTRU -> DecompSpectrum -> ESTBUR's general-branch solve, see
// core/src/seats/estbur.cpp) is wired and gates bit-exact for the whole SEATS
// corpus, including general-shape p>0/bp>0 AND the imean!=0 drift case (a
// Constant/mean regressor with d>=1; see the mean handling below). This
// function only signals success/failure; tools/x13run_seats.cpp re-runs the
// same (idempotent, side-effect-free) chain to dump the tables. Falls back to
// seats_not_ported() for the remaining gaps (imean alongside other regressors,
// or the chain simply failing).
#include "specparse/specparse.hpp"
#include "gen/model.hpp"  // prm::PRGTCN (mean-regressor type)
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
// Signal that a specific SEATS path is not yet ported (imean!=0, or the
// decomposition chain failing) -- most of SEATS is ported and gates bit-exact
// (mirrors x11parts.cpp's local x11_not_ported).
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
// and d==0). Only a mean ALONGSIDE other regressors remains open (needs a
// selective add-back; the caller fatals there). See tools/seats_general_scope.md.
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

    // SEATS decomposes the series WITH the mean present (verified: the oracle's
    // z reconstructs to raw log(airline) exactly, wm=mean of its differenced
    // form). But estimation left ctx.series.tsrs = the regression-ADJUSTED
    // series (y - X*b), which for a Constant regressor has the mean's drift
    // removed. Restore the mean-inclusive series for the decomposition. For a
    // Constant-ONLY model that is exactly the clean transformed series trnsrs
    // (raw log); a mean alongside OTHER regressors (TD/outliers, which SEATS
    // *does* remove) needs a selective add-back not yet ported -- fatal there.
    if (seats_has_mean(ctx)) {
        bool only_mean = true;
        for (int i = 1; i <= ctx.model.nb; ++i)
            if (ctx.model.rgvrtp(i) != prm::PRGTCN) { only_mean = false; break; }
        if (!only_mean) {
            seats_not_ported(ctx,
                "SEATS with a mean/constant regressor alongside other "
                "regressors (mean add-back onto the regression-adjusted "
                "series not yet ported)");
            return false;
        }
        int ncp = ctx.mdldat.nspobs;
        if (ncp > static_cast<int>(trnsrs.size())) ncp = static_cast<int>(trnsrs.size());
        for (int i = 0; i < ncp; ++i) ctx.series.tsrs(i + 1) = trnsrs[i];
    }

    // decode -> canonical denoms -> SPECTRU -> DecompSpectrum -> ESTBUR
    // (historical span only; see estbur.hpp for exact scope/limits).
    try {
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
