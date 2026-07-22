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
#include "regarima/regvar.hpp"
#include "regarima/estimate.hpp"
#include "regarima/forecast.hpp"
#include "regarima/outlier.hpp"     // idotlr, setcv
#include "automdl/automd.hpp"       // automd (automatic model selection)
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
            bool estimate) {
    if (!parse_spec(ctx, spec_text, base)) return false;
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
    std::vector<double> padj(static_cast<std::size_t>(nobspf));
    std::vector<double> fac(static_cast<std::size_t>(nobspf), 1.0);
    for (int tpnt = 1; tpnt <= nobspf; ++tpnt) {
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
        trnfcn(ctx, padj.data(), nobspf, ctx.arima.fcntyp, ctx.arima.lam,
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
            // Prior factors span-aligned into Adj (Adj1st=1), so the automatic-
            // model path's prlkhd sees Adj(Adj1st)==fac[0], matching arima.f.
            copy(fac.data(), ncp, 1, ctx.adj.adj.data());
            ctx.adj.adj1st = 1;
        }
    }
    if (wants_save(ctx, "trn")) {
        savtbl(ctx, LTRNDT, begspn, 1, nspobs, sp, trnsrs.data(), base, base, nser);
        if (ctx.error.lfatal) return false;
    }

    // The regression design matrix (arima.f:280): regvar builds [X:y] from the
    // parsed regression groups and the transformed series. Pre-model this is
    // fully calendar-determined (constant/seasonal/td/holiday columns), so the
    // rmx save table (savmtx.f, arima.f:1013) is reproducible here.
    if (ctx.captured.has_model && have_trn) {
        int nrxy, frstry;
        regvar(ctx, trnsrs.data(), nobspf, fctdrp, nfcst, 0,
               ctx.arima.userx.data(), ctx.arima.bgusrx.data(), ctx.arima.nrusrx,
               ctx.prior.priadj, ctx.arima.reglom, nrxy,
               ctx.arima.begxy.data(), frstry, true, ctx.arima.elong);
        if (ctx.error.lfatal) return false;
        ctx.arima.nrxy = nrxy;
        // (arima.f:282 Iregfx>=2 rmfix/regvar re-run: fixed regressors are not
        // reachable in this pre-model slice.)
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
            // parity for ismd0 series like airline). Still-deferred automd features
            // (auto-transform, outlier, non-default-model nloop/tstmd1 finalization)
            // are noted in tools/automdl_scouting.md; specs needing them stay xfailed.
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
            } else {
                rgarma(ctx, ctx.arima.lestim, ctx.arima.mxiter, ctx.arima.mxnlit,
                       /*lprtit=*/false, a.data(), na, nefobs, lauto);
                (void)na;
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
                    idotlr(ctx, ctx.arima.ltstao, ctx.arima.ltstls,
                           ctx.arima.ltsttc, ctx.arima.ladd1, critvl,
                           ctx.arima.cvrduc, begtst, endtst, nefobs,
                           ctx.arima.lestim, ctx.arima.mxiter, ctx.arima.mxnlit,
                           /*lauto=*/false, a.data());
                    if (ctx.error.lfatal) return false;
                }
            }
            (void)nefobs;  // nefobs == Nspobs-Nintvl; the estimates live in mdldat

            // Likelihood statistics (arima.f:742 prlkhd): the transform-Jacobian-
            // adjusted log likelihood + AIC/AICC/BIC/HQ into ctx.lkhd. Y is the
            // original untransformed series over the span (aptr == Y(Frstsy)); the
            // prior factors are `fac` (all 1 with no prior).
            prlkhd(ctx, aptr, fac.data(), ctx.adj.adjmod, ctx.arima.fcntyp,
                   ctx.arima.lam);
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
            if (nfcst > 0) {
                fcstout(ctx, nfcst, ctx.arima.fctdrp, ctx.arima.ciprob,
                        ctx.arima.lognrm);
                if (ctx.error.lfatal) return false;
            }
        }
    }

    return !ctx.error.lfatal;
}

}  // namespace x13
