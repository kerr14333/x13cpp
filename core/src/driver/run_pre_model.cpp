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

#include <string>

namespace x13 {

namespace {
constexpr int LSRSSP = 2;   // mdltbl.i: original series for span; ext 'a1'
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

    return !ctx.error.lfatal;
}

}  // namespace x13
