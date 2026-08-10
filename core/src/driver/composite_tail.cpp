// composite_tail.cpp -- x11ari.f:329-374, lifted out of run_x11 so run_seats
// reaches it too. See composite_tail.hpp and tools/composite_scouting.md.
#include "driver/composite_tail.hpp"

#include "common/x13context.hpp"
#include "composite/agr2.hpp"
#include "composite/agr3.hpp"
#include "composite/agr3s.hpp"
#include "diag/genqs.hpp"            // gennpsa
#include "driver/run_spectrum.hpp"
#include "x11/x11summ.hpp"           // x11pt4_etables / x11pt4_partf

namespace x13 {

bool run_composite_tail(X13Context& ctx, const int* begspn_full, bool lx11) {
    // x11ari.f:372-373 -- a COMPONENT (Iag>=0 and Iagr not the total's 3) folds
    // itself into the aggregation buffers. No-op for every single-series run.
    if (ctx.agr.iagr > 0 && ctx.agr.iagr != 3 && ctx.agr.iag >= 0) {
        if (!agr2_component(ctx)) return false;   // it writes its own NOTE/ERROR
        return true;
    }
    if (ctx.agr.iagr != 3) return true;

    // x11ari.f:333-343 -- this run IS the composite total, so after its own
    // DIRECT adjustment rebuild the tables from the aggregated component results.
    // (Ixreg/Kswv are saved and zeroed around the block in the oracle; agr3s does
    // the zeroing itself and neither is set on the agr3 path.)
    //
    // agr3/agr3s REPLACE the D-table buffers, so snapshot the total's own DIRECT
    // d10-d13 first. Not an oracle step -- the oracle has already punched them by
    // this point, while this port hands all output to the caller afterwards.
    constexpr int PLEN = 1020;
    ctx.agr_direct_d10.assign(ctx.x11srs.sts.data(), ctx.x11srs.sts.data() + PLEN);
    ctx.agr_direct_d11.assign(ctx.x11srs.stci.data(), ctx.x11srs.stci.data() + PLEN);
    ctx.agr_direct_d12.assign(ctx.x11srs.stc.data(), ctx.x11srs.stc.data() + PLEN);
    ctx.agr_direct_d13.assign(ctx.x11srs.sti.data(), ctx.x11srs.sti.data() + PLEN);

    if (ctx.x11agr) {
        agr3(ctx, begspn_full);
        if (ctx.error.lfatal) return false;
        // x11ari.f:340 -- the SAME x11pt4 over the indirect buffers agr3 just
        // installed, producing the `if2.*`/`if3.*` block. Its inputs are the
        // indirect analogues: Sti is already the final indirect irregular and Stc
        // the pre-level-shift filter output (Stc2 the folded, published one), so
        // there is no internal-vs-published split to undo the way x11pt3 needs on
        // the direct side. The direct block was snapshotted above, so overwriting
        // /inpt2/, /work2/ and Mcd here is safe.
        x11pt4_etables(ctx, ctx.x11srs.stc.data(), ctx.x11srs.stc2.data(),
                       ctx.arima.lttc);
        if (x11pt4_partf(ctx, ctx.x11srs.sti.data(), ctx.x11srs.stc.data())) {
            ctx.agr_f2inpt2 = ctx.inpt2;
            ctx.agr_f2work2 = ctx.work2;
            ctx.agr_f2tests = ctx.tests;
            ctx.agr_f2mcd = ctx.x11opt.mcd;
            ctx.agr_f2ratic = ctx.x11opt.ratic;
            ctx.agr_f2ratis = ctx.x11opt.ratis;
            ctx.agr_f3_set = true;
        }
    } else {
        // x11ari.f:342 -- at least one component came from SEATS. No x11pt4 call
        // follows this branch in the oracle, which is why a SEATS composite has
        // no if2./if3. block and no ie1/ie2/ie3/ie7/ie8 tables.
        agr3s(ctx, begspn_full, lx11);
        if (ctx.error.lfatal) return false;
    }

    // --- the INDIRECT diagnostics (x11ari.f:344-370) -------------------------
    // agr3/agr3s have replaced the D-table buffers with the indirect adjustment,
    // so spcdrv and gennpsa run a SECOND time over them under Iagr==4.
    //
    // genqs comes first (x11ari.f:346-349) and is a NO-OP by CENSUS DEFECT --
    // CB-31. Its `Tblind` argument is `LSLIQS` (=69, a SAVELOG index from
    // spcsvl.i) where genqs.f:439 uses it as `Savtab(Tblind)`, a TABLE-log
    // subscript; the direct call one screen earlier correctly passes `LSPCQS`
    // (=113). `LSPQSI` (=114) exists in spctbl.i, is plainly the intended one,
    // and is passed nowhere. So the whole indirect QS savelog block is gated on
    // an unrelated table's save flag and the oracle emits no `qsind*` key at all
    // -- which is exactly what the composite total's golden shows, next to a full
    // set of `npind*` and `spcind*`. Reproduced by not calling it.
    if (!run_spectrum(ctx, /*iagr4=*/true)) return false;
    if (!gennpsa(ctx, /*lseats=*/!lx11, /*iagr4=*/true)) return false;

    // x11ari.f:372-373 -- agr3/agr3s leave Iagr==4, which routes the SAME agr2
    // call into its comparison-statistics branch: direct vs indirect roughness,
    // then the restore of the direct pointer geometry.
    agr2_compare(ctx, begspn_full);
    return true;
}

}  // namespace x13
