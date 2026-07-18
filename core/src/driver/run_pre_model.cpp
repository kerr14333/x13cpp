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

#include <algorithm>
#include <string>
#include <vector>

namespace x13 {

namespace {
constexpr int LSRSSP = 2;    // mdltbl.i: original series for span; ext 'a1'
constexpr int LTRNDT = 20;   // mdltbl.i: prior-adjusted+transformed data; ext 'trn'

bool wants_save(const X13Context& ctx, const std::string& ext) {
    const auto& v = ctx.captured.save_tables;
    return std::find(v.begin(), v.end(), ext) != v.end();
}
}  // namespace

bool run_m2(X13Context& ctx, const std::string& spec_text, const std::string& base) {
    if (!parse_spec(ctx, spec_text, base)) return false;
    if (!ctx.captured.has_series) return false;

    // Save-format setup (gtinpt.f): default precision 15 -> field width 22,
    // format (sp,e22.15).
    ctx.savcmn.svprec = 15;
    ctx.savcmn.svsize = 22;
    ctx.savcmn.svfmt = "(sp,e22.15)";

    int sp = ctx.model.sp;
    const int* begsrs = ctx.arima.begsrs.data();
    const int* begspn = ctx.mdldat.begspn.data();
    int nspobs = ctx.mdldat.nspobs;

    // Span offset within the full series (getsrs guarantees coverage).
    int offset = 0;
    dfdate(begspn, begsrs, sp, offset);

    // avec must be 1-based over the span: avec[tpnt-1] == y(offset+tpnt).
    const double* aptr = ctx.arima.y.data() + offset;

    // x12run.f: Serno (the header label) is the base name capped at 16 chars;
    // the on-disk filename (Cursrs) keeps the full base name.
    int nser = static_cast<int>(base.size());
    if (nser > 16) nser = 16;
    savtbl(ctx, LSRSSP, begspn, 1, nspobs, sp, aptr, base, base, nser);
    if (ctx.error.lfatal) return false;

    // Table trn (LTRNDT): the transformed (prior-adjusted) series that feeds
    // regARIMA modeling. arima.f copies the prior-adjusted series (Sto) over the
    // span into trnsrs, applies the Box-Cox/logit transform (trnfcn), and saves
    // it. With no prior-adjustment factors the pre-model series is the original
    // series over the span (== a1), so trnsrs = trnfcn(a1). Prior factors are
    // applied by the prior-adjustment phase (handled where present).
    if (wants_save(ctx, "trn")) {
        std::vector<double> trn(static_cast<std::size_t>(nspobs));
        trnfcn(ctx, aptr, nspobs, ctx.arima.fcntyp, ctx.arima.lam, trn.data());
        if (ctx.error.lfatal) return false;
        savtbl(ctx, LTRNDT, begspn, 1, nspobs, sp, trn.data(), base, base, nser);
        if (ctx.error.lfatal) return false;
    }

    return !ctx.error.lfatal;
}

}  // namespace x13
