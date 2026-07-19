// gtinpt.cpp -- top-level spec dispatch (gtinpt.f).
//
// M1 scope: faithful reproduction of the spec dispatch loop (getfcn over the
// spec-name dictionary), the cross-spec mutual-exclusion checks, and the
// end-of-input validation that determine the fatal/ok outcome. The exhaustive
// default-initialization of the estimation / X-11 / SEATS option COMMON blocks
// (~500 lines) is deferred to the milestones that consume them; here only the
// subset the parser reads back is initialized.
#include "specparse/specparse.hpp"
#include "notset.hpp"
#include "gen/model.hpp"

#include <algorithm>

namespace x13 {

using namespace lexprm;

// dpeq helper.
static inline bool dpeq(double a, double b) { return a == b; }

void gtinpt(X13Context& ctx, bool& lx11, bool& lseats, bool& lmodel, bool& inptok) {
    LexState& L = ctx.lex;

    // --- spec-name dictionary (verbatim from gtinpt.f) ---
    constexpr int PSPC = 20;
    static const char SPCDIC[] =
        "seriestransformidentifyregressionarimaautomdlestimateoutliercheck"
        "forecastx11historyslidingspanscompositex11regressionseatspickmdl"
        "forcemetadataspectrum";
    static const int spcptr[PSPC + 1] = {1, 7, 16, 24, 34, 39, 46, 54, 61, 66, 74,
        77, 84, 96, 105, 118, 123, 130, 135, 143, 151};
    int spclog[2 * PSPC];

    // Initialize parser input.
    intinp(ctx, ctx.units.mt);
    if (ctx.error.lfatal) return;
    setint(prm::NOTSET, 2 * PSPC, spclog);

    // Parser-read default subset.
    ctx.arima.begsrs(1) = 1;
    ctx.arima.begsrs(2) = 1;
    ctx.arima.nobs = 0;
    ctx.model.sp = 12;
    ctx.savcmn.svprec = 15;
    ctx.x11msc.yr2000 = true;
    ctx.x11opt.divpwr = prm::NOTSET;
    ctx.model.isrflw = prm::NOTSET;
    ctx.missng.mvcode = -99999.0;
    ctx.missng.mvval = 1000000000.0;
    ctx.agr.iag = prm::NOTSET;
    ctx.agr.w = 1.0;
    ctx.arima.fcntyp = prm::NOTSET;
    ctx.arima.lam = 1.0;
    ctx.picktd.picktd = false;
    ctx.prior.priadj = 0;    // gtinpt.f: Priadj=0 (no predefined prior adjustment)
    ctx.prior.kfmt = 0;      // gtinpt.f: Kfmt=0
    ctx.arima.reglom = 0;    // gtinpt.f: Reglom=0

    // gtinpt.f 188-207 (+ mdlint.f): regression-group / ARIMA-operator state.
    intlst(prm::PB, ctx.model.colptr.data(), ctx.model.ncoltl);
    ctx.model.nb = ctx.model.ncoltl;
    ctx.model.ncxy = 1;
    intlst(prm::PGRP, ctx.model.grpptr.data(), ctx.model.ngrptl);
    intlst(prm::PGRP, ctx.model.grp.data(), ctx.model.ngrp);
    intlst(prm::POPR, ctx.model.opr.data(), ctx.model.nopr);
    intlst(prm::PMDL, ctx.model.mdl.data(), ctx.model.nmdl);
    intlst(prm::POPR, ctx.model.oprptr.data(), ctx.model.noprtl);   // mdlint.f
    ctx.model.mdl(2) = 1;    // Mdl(AR)=1
    ctx.model.mdl(3) = 1;    // Mdl(MA)=1
    ctx.arima.elong = true;  // gtinpt.f: Elong=T
    ctx.model.easidx = 0;    // gtinpt.f: Easidx=0
    ctx.extend.nfcst = prm::NOTSET;   // gtinpt.f: Nfcst=NOTSET
    ctx.extend.nbcst = prm::NOTSET;   // gtinpt.f: Nbcst=NOTSET
    ctx.arima.fctdrp = 0;             // gtinpt.f: Fctdrp=0
    ctx.picktd.tdzero = 0;
    ctx.picktd.lnzero = 0;
    setint(prm::NOTSET, 2, ctx.picktd.tddate.data());
    setint(prm::NOTSET, 2, ctx.picktd.lndate.data());

    // Control flags.
    bool havsrs = false, havesp = false, havotl = false, havreg = false, havtd = false;
    bool larma = false, hvfcst = false, hvspec = false, havmdl = false, havreq = false;
    bool lautom = false, lautox = false, hvmfil = false, lagr = false, l1stcomp = false;
    bool ldestm = false, x11reg = false;
    bool ldata = false;
    std::string dtafil;
    lx11 = false; lseats = false; lmodel = false;
    inptok = true;

    // Local echo of x11/transform state for the muladd/fcntyp end block.
    int muladd = prm::NOTSET;

    int spcidx;
    while (true) {
        if (getfcn(ctx, SPCDIC, spcptr, PSPC, spcidx, spclog, inptok)) {
            switch (spcidx) {
            case 1:  // series
                getsrs(ctx, havsrs, havesp, lagr, ldata, dtafil, inptok);
                if (ctx.error.lfatal) return;
                ctx.captured.spec_order.push_back("series");
                break;
            case 2:  // transform
                gt_transform(ctx, inptok);   // sets ctx.arima.fcntyp / lam (getadj.f)
                if (ctx.error.lfatal) return;
                ctx.captured.spec_order.push_back("transform");
                break;
            case 3:  // identify
                gt_identify(ctx, inptok);
                if (ctx.error.lfatal) return;
                if (!lmodel) lmodel = true;
                break;
            case 4:  // regression
                if (hvmfil) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify regression variables when a model file is given."); inptok = false; }
                gt_regression(ctx, havsrs, havesp, havtd, inptok);
                if (ctx.error.lfatal) return;
                if (!lmodel) lmodel = true;
                if (!havreg) havreg = true;
                if (!havreq) havreq = true;
                ctx.captured.spec_order.push_back("regression");
                break;
            case 5:  // arima
                if (lautom) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify arima and automdl spec in the same input file."); inptok = false; }
                else if (lautox) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify arima and pickmdl spec in the same input file."); inptok = false; }
                if (hvmfil) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify arima spec if model is read in from the file argument"); inptok = false; }
                gt_arima(ctx, inptok);
                if (ctx.error.lfatal) return;
                if (!lmodel) lmodel = true;
                if (!havmdl) havmdl = true;
                larma = true;
                ctx.captured.spec_order.push_back("arima");
                break;
            case 6:  // automdl
                if (larma) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify arima and automdl spec in the same input file."); inptok = false; }
                else if (lautox) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify automdl and pickmdl spec in the same input file."); inptok = false; }
                if (hvmfil) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify automdl spec if model is read in from the file argument"); inptok = false; }
                gt_automdl(ctx, inptok);
                if (ctx.error.lfatal) return;
                lautom = true;
                if (!lmodel) lmodel = true;
                if (!havmdl) havmdl = true;
                ldestm = true;
                ctx.captured.spec_order.push_back("automdl");
                break;
            case 7:  // estimate
                gt_estimate(ctx, inptok);
                if (ctx.error.lfatal) return;
                ldestm = true;
                if (!lmodel) lmodel = true;
                if (!havreq) havreq = true;
                ctx.captured.spec_order.push_back("estimate");
                break;
            case 8:  // outlier
                if (!havsrs) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Need to specify a series to identify outliers"); inptok = false; }
                gt_outlier(ctx, inptok);
                if (ctx.error.lfatal) return;
                if (!havreq) havreq = true;
                ctx.captured.spec_order.push_back("outlier");
                break;
            case 9:  // check
                gt_check(ctx, inptok);
                if (ctx.error.lfatal) return;
                if (!lmodel) lmodel = true;
                if (!havreq) havreq = true;
                break;
            case 10:  // forecast
                gt_forecast(ctx, inptok);
                if (ctx.error.lfatal) return;
                if (!lmodel) lmodel = true;
                if (!havreq) havreq = true;
                hvfcst = true;
                ctx.captured.spec_order.push_back("forecast");
                break;
            case 11:  // x11
                if (lseats) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify x11 and seats spec in the same input file."); inptok = false; }
                gt_x11(ctx, inptok);
                if (ctx.error.lfatal) return;
                if (!lx11) lx11 = true;
                if (!ctx.captured.x11_mode.empty()) muladd = 1;
                ctx.captured.spec_order.push_back("x11");
                break;
            case 14:  // composite
                if (ctx.agr.iagr <= 0) {
                    if (ctx.agr.iagr == 0)
                        inpter(ctx, PERROR, L.pos.data() + 1,
                               "No component series were specified for composite adjustment.");
                    else {
                        inpter(ctx, PERROR, L.errpos.data() + 1,
                               "Error(s) were found while executing the spec file(s) of component");
                        ctx.agr.iagr = prm::NOTSET;
                    }
                    inptok = false;
                }
                gt_composite(ctx, havsrs, lagr, inptok);
                if (ctx.error.lfatal) return;
                l1stcomp = true;
                ctx.captured.spec_order.push_back("composite");
                break;
            case 16:  // seats
                if (lx11) { inpter(ctx, PERROR, L.pos.data() + 1,
                        "Cannot specify x11 and seats spec in the same input file."); inptok = false; }
                gt_seats(ctx, inptok);
                if (ctx.error.lfatal) return;
                ldestm = true;
                if (!lmodel) lmodel = true;
                if (!havreq) havreq = true;
                lseats = true;
                ctx.captured.spec_order.push_back("seats");
                break;
            case 12:  // history
            case 13:  // slidingspans
            case 15:  // x11regression
            case 17:  // pickmdl
            case 18:  // force
            case 19:  // metadata
            case 20:  // spectrum
            default:
                inpter(ctx, PERROR, L.pos.data() + 1,
                       "This spec is recognized but not yet supported by the M1 parser port.");
                abend(ctx);
                return;
            }
            (void)havotl; (void)hvfcst; (void)hvspec; (void)havreg;
            (void)l1stcomp; (void)ldestm; (void)x11reg;
            continue;   // GO TO 210
        }
        if (!inptok) return;

        // --- End-of-input validation ---
        if (!havsrs) {
            writln(ctx, "ERROR: Series for analysis not specifed; a valid series or composite",
                   ctx.units.mt2, stdio::STDERR, true);
            writln(ctx, "       spec is required.", ctx.units.mt2, stdio::STDERR, false);
            inptok = false;
            return;
        }
        if (!havmdl && havreq) {
            writln(ctx, "ERROR: A spec that requires modeling was found ", ctx.units.mt2, stdio::STDERR, true);
            writln(ctx, "       (regression, check, estimate, forecast, outlier or seats),", ctx.units.mt2, stdio::STDERR, false);
            writln(ctx, "       but no provision for an ARIMA model was found.", ctx.units.mt2, stdio::STDERR, false);
            writln(ctx, "       If modeling was intended, please provide an ARIMA model ", ctx.units.mt2, stdio::STDERR, false);
            writln(ctx, "       using the arima spec or allow for automatic ARIMA", ctx.units.mt2, stdio::STDERR, false);
            writln(ctx, "       model selection using an automdl or pickmdl spec. ", ctx.units.mt2, stdio::STDERR, false);
            inptok = false;
            return;
        }

        // Muladd / Fcntyp resolution (only the Inptok-affecting branch matters here).
        int fcntyp = ctx.arima.fcntyp;
        double lam = ctx.arima.lam;
        if (muladd == prm::NOTSET) {
            if (fcntyp == 0 || fcntyp == 4 || (fcntyp == 5 && dpeq(lam, 1.0))) {
                muladd = 1;
            } else {
                muladd = 0;
                if (fcntyp == prm::NOTSET) fcntyp = 4;
            }
        } else {
            if (fcntyp == 0) {
                writln(ctx, "ERROR: Cannot set seasonal adjustment mode when automatic transformation",
                       ctx.units.mt2, stdio::STDERR, true);
                writln(ctx, "       selection is done.", ctx.units.mt2, stdio::STDERR, false);
                inptok = false;
                return;
            } else if (fcntyp == prm::NOTSET) {
                fcntyp = 4;
            }
        }
        ctx.arima.fcntyp = fcntyp;

        // --- gtinpt.f ~989-1067: td/lom prior-adjustment setup (Picktd) ---
        // With a log transform, the td (or td1coef) regressor's leap-year
        // component becomes a prior adjustment of the series: rmlnvr sets
        // Priadj (lpyear, or lom/loq for x11 type=trend) and removes any
        // Length-of-* / Leap Year regressor column.
        if (ctx.picktd.picktd) {
            if (dpeq(lam, 0.0)) {
                if (ctx.prior.priadj > 1) {
                    writln(ctx, "ERROR: A length of month, length of quarter, or "
                           "leap year prior adjustment", stdio::STDERR,
                           ctx.units.mt2, true);
                    writln(ctx, "       cannot be specified with the td or td1coef "
                           "regressor.", stdio::STDERR, ctx.units.mt2, false);
                    inptok = false;
                } else {
                    rmlnvr(ctx, ctx.prior.priadj, ctx.x11opt.kfulsm,
                           ctx.mdldat.nspobs);
                    if (ctx.error.lfatal) return;
                }
            } else {
                if (ctx.prior.priadj > 1) {
                    writln(ctx, "ERROR: A length of month, length of quarter, or "
                           "leap year prior adjustment", stdio::STDERR,
                           ctx.units.mt2, true);
                    writln(ctx, "       cannot be specified with the td or td1coef "
                           "regressor.", stdio::STDERR, ctx.units.mt2, false);
                    inptok = false;
                } else if (ctx.x11opt.kfulsm == 2) {
                    // gtinpt.f: CALL replyf() -- x11 type=trend path, not yet
                    // ported.
                    writln(ctx, "ERROR: x11 type=trend with td regressors "
                           "(replyf.f) not yet ported.", stdio::STDERR,
                           ctx.units.mt2, true);
                    abend(ctx);
                    return;
                }
            }
        }
        // gtinpt.f 1072-1084: check for lom in the regression and in the prior
        // adjustment.
        if (ctx.prior.priadj > 1 && ctx.model.nb > 0) {
            for (int icol = 1; icol <= ctx.model.nb; ++icol) {
                int t = ctx.model.rgvrtp(icol);
                if (t == 5 || t == 6 || t == 7 || t == 9 ||    // PRGTLM/LQ/LY/SL
                    t == 58 || t == 59 || t == 60) {           // PRGULM/ULQ/ULY
                    writln(ctx, "ERROR: Cannot have a length-of-period or leap "
                           "year regressor with a", stdio::STDERR, ctx.units.mt2,
                           true);
                    writln(ctx, "       length-of-period or leap year prior "
                           "adjustment.", stdio::STDERR, ctx.units.mt2, false);
                    inptok = false;
                    break;
                }
            }
        }

        // gtinpt.f 1110-1117: no forecasts excluded when seasonal adjustment done.
        if (lx11 && ctx.arima.fctdrp > 0) {
            writln(ctx, "WARNING: No observations should be excluded from "
                   "forecasting when a", ctx.units.mt2, stdio::STDERR, true);
            writln(ctx, "         seasonal adjustment is done.", ctx.units.mt2,
                   stdio::STDERR, false);
            ctx.arima.fctdrp = 0;
        }

        // gtinpt.f 1142-1167: default Nbcst / Nfcst.
        if (ctx.extend.nbcst == prm::NOTSET) ctx.extend.nbcst = 0;
        if (ctx.extend.nfcst == prm::NOTSET) {
            if (lmodel) {
                if (lx11 || lseats) {
                    if (lseats) ctx.extend.nfcst = std::max(12, 3 * ctx.model.sp);
                    else ctx.extend.nfcst = ctx.model.sp;
                } else if (hvfcst) {
                    ctx.extend.nfcst = ctx.model.sp;
                } else {
                    ctx.extend.nfcst = 0;
                }
            } else {
                ctx.extend.nfcst = 0;
            }
        }
        return;
    }
}

} // namespace x13
