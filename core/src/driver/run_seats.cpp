// run_seats.cpp -- SEATS driver phase: parse a spec, estimate the regARIMA
// model (SEATS has no direct/no-model path in the oracle -- see
// tools/seats_scope.md section 1), then dispatch to the SEATS canonical
// decomposition.
//
// STATUS (session 12): the historical-span decomposition (decode -> canonical
// denoms -> SPECTRU -> DecompSpectrum -> ESTBUR's general-branch solve, see
// core/src/seats/estbur.cpp) is now wired for the additive, no-real-
// seasonal/no-cycle case (unrate_seats). x13context.hpp is out of scope for
// this driver (owned by the concurrent span agent), so there is nowhere on
// ctx to STORE the resulting trend/sa/ir arrays -- this function only
// signals success/failure; tools/x13run_seats.cpp re-runs the same
// (idempotent, side-effect-free) chain itself to actually dump the tables.
// Falls back to the existing seats_not_ported() fatal for anything this
// pass doesn't cover (p>0/bp>0, imean!=0, or the chain simply failing).
#include "specparse/specparse.hpp"
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
// Signal that SEATS decomposition itself is not yet ported (mirrors
// x11parts.cpp's local x11_not_ported).
void seats_not_ported(X13Context& ctx, const char* what) {
    errhdr(ctx);
    writln(ctx, std::string("ERROR: ") + what + " not yet ported (SEATS decomposition).",
           stdio::STDERR, ctx.units.mt2, true);
    abend(ctx);
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
