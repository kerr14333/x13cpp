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
#include "regarima/priadj.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace x13 {

namespace {
constexpr int LSRSSP = 2;    // mdltbl.i: original series for span; ext 'a1'
constexpr int LTRNPA = 13;   // mdltbl.i: prior-adjustment factors; ext 'a2'
constexpr int LTRNA3 = 16;   // mdltbl.i: prior-adjusted data; ext 'a3'
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

    // Prior adjustment (adjsrs.f / prtadj.f, predefined lom/loq/lpyear priors).
    // adjsrs builds the multiplicative prior-factor series Adj via td7var; the
    // prior-adjusted series is the original series divided by those factors
    // (divsub). The factors depend only on the calendar date, so a2 (factors),
    // a3 (prior-adjusted data), and the prior-adjusted trn are pre-model
    // reproducible. User prior-factor files and calendar (holiday/TD) priors are
    // out of this pre-model slice.
    int priadj = ctx.prior.priadj;
    bool has_prior = (priadj > 1);   // 2 lom / 3 loq / 4 lpyear
    bool lom = (priadj == 2 || priadj == 3);   // adjsrs.f: lom for lom/loq

    // The prior-adjusted series over the span (== a1 with no prior).
    std::vector<double> padj(static_cast<std::size_t>(nspobs));
    std::vector<double> fac(static_cast<std::size_t>(nspobs), 1.0);
    for (int tpnt = 1; tpnt <= nspobs; ++tpnt) {
        double a1 = aptr[tpnt - 1];
        if (has_prior) {
            int idate[2];
            addate(begspn, sp, tpnt - 1, idate);
            double f = lpfac(idate[0], idate[1], sp, lom);   // td7var factor
            fac[static_cast<std::size_t>(tpnt - 1)] = f;
            padj[static_cast<std::size_t>(tpnt - 1)] = a1 / f;   // divsub (mult mode)
        } else {
            padj[static_cast<std::size_t>(tpnt - 1)] = a1;
        }
    }

    // Table a2 (LTRNPA): the combined prior-adjustment factors (Sprior).
    if (has_prior && wants_save(ctx, "a2")) {
        savtbl(ctx, LTRNPA, begspn, 1, nspobs, sp, fac.data(), base, base, nser);
        if (ctx.error.lfatal) return false;
    }
    // Table a3 (LTRNA3): the prior-adjusted data (Sto after divsub).
    if (has_prior && wants_save(ctx, "a3")) {
        savtbl(ctx, LTRNA3, begspn, 1, nspobs, sp, padj.data(), base, base, nser);
        if (ctx.error.lfatal) return false;
    }

    // Table trn (LTRNDT): the transformed prior-adjusted series that feeds
    // regARIMA modeling. arima.f applies the Box-Cox/logit transform (trnfcn) to
    // the prior-adjusted series (== a3, or a1 when there is no prior).
    if (wants_save(ctx, "trn")) {
        std::vector<double> trn(static_cast<std::size_t>(nspobs));
        trnfcn(ctx, padj.data(), nspobs, ctx.arima.fcntyp, ctx.arima.lam, trn.data());
        if (ctx.error.lfatal) return false;
        savtbl(ctx, LTRNDT, begspn, 1, nspobs, sp, trn.data(), base, base, nser);
        if (ctx.error.lfatal) return false;
    }

    return !ctx.error.lfatal;
}

}  // namespace x13
