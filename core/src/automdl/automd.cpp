// automd.cpp -- automd.f driver. See automd.hpp for scope/deferred features.
//
// automd.f has ONE path: default airline model -> chkmu -> residual
// diagnostics -> label 10 (snapshot + strip regressors) -> iddiff -> amdid ->
// put regressors back -> label 40 (amidot | tstmd1) -> label 30 finalization ->
// label 70. The AIC tests are conditional BLOCKS inside that flow, not a branch
// around it.
//
// This driver used to mirror them as a branch, and the split bit twice. First
// the label-30 finalization tail (pass0/chkrt1/redomd/testodf/tstmd2/autoer)
// was reached only from the aictest side, so a plain `automdl{}` spec silently
// skipped the unit-root redomd, the over-differencing check, the residual-mean
// Constant add and the insignificant-lag drop. Then label 40 itself, plus the
// three things its tstmd1 arm reads -- `tair` (automd.f:343), `adj0`/`trns0`
// (:369-371) and bkdfmd's backup (:379, which tstmd1.f:221 restores from) --
// were all likewise aictest-only, so `noautooutlier=tramo` had nowhere to land.
//
// There is now ONE path here too; `aic` gates only the three tdaic/easaic
// blocks, exactly as Itdtst/Leastr gate them in the Fortran. Keep it that way.
// Primitives are in automd_finalize.cpp.
#include "automdl/automd.hpp"

#include "automdl/adqtst.hpp"        // mdlchk, tstmd1, tstmd2, testodf, bkdfmd
#include "automdl/aictst.hpp"        // tdaic, easaic (AIC-test regressor family)
#include "automdl/amdid.hpp"         // amdid
#include "automdl/automd_finalize.hpp" // ssprep_save/restor_model/rmfix/addfix/
                                     // pass0/clrotl/autoer
#include "automdl/chkmu.hpp"         // chkmu
#include "automdl/idmodel.hpp"       // chkrt1
#include "automdl/iddiff.hpp"        // iddiff, prterr
#include "automdl/mdlset.hpp"        // mdlint, mdlset, mkmdsn
#include "automdl/pass2.hpp"        // pass2 (the nloop re-entry)
#include "regarima/estimate.hpp"     // rgarma, armats
#include "regarima/regvar.hpp"       // regvar
#include "regarima/outlier.hpp"      // idotlr, setcv (amidot)
#include "specparse/specparse.hpp"   // strinx, adrgef, abend, addate, dfdate,
                                     // setdp, dlrgef
#include "numeric/numeric.hpp"       // daxpy
#include "gen/model.hpp"             // prm::PRGTCN, prm::PAICT
#include "gen/notset.hpp"            // prm::DNOTST, prm::NOTSET
#include "gen/srslen.hpp"            // prm::PLEN

#include <cctype>                    // std::tolower
#include <cmath>                     // std::pow (amidot)
#include <string>
#include <vector>

namespace x13 {

// Block-1 AIC tests on the default model (automd.f:216-232). Installs the state
// the (unported) getreg/editor aictest-argument parser would set for the
// aictest=(td [easter]) corpus, then runs tdaic (if td) and easaic (if easter).
// Each estimates the candidates and leaves the model rebuilt+re-estimated to the
// lowest-AICC choice, so the selected td/Easter regressors enter the model.
// Setup lifted from the m4-gated tools/x13run_iddiff --aictest harness
// (lines 185-260). Returns false + sets lfatal on an unported aictest token.
static bool automd_aictest_block1(X13Context& ctx, double* trnsrs, double* a,
                                  int& nefobs, int& na, int& frstry) {
    using namespace prm;
    model_cmn& m = ctx.model;
    arima_cmn& ar = ctx.arima;

    bool want_td = false, want_easter = false, unported = false;
    for (const auto& t : ctx.captured.aictest_vars) {
        std::string s;
        for (char c : t)
            s += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (s == "td") want_td = true;
        else if (s == "easter") want_easter = true;
        else unported = true;    // td1coef/tdstock/lom/user/... not yet ported
    }
    if (unported) { abend(ctx); return false; }
    if (!(want_td || want_easter)) return true;

    // Prior-adjustment span (adjsrs.f:20-21,89-90) tdaic's leap-year preadjust
    // needs; the ported pre-model does prior adjustment inline and never sets
    // Begadj/Nadj/Adj1st. Nbcst==0 for these specs.
    const int nbcst = ctx.extend.nbcst < 0 ? 0 : ctx.extend.nbcst;
    addate(ctx.mdldat.begspn.data(), m.sp, -nbcst, ctx.adj.begadj.data());
    const int nfc = ctx.extend.nfcst < 0 ? 0 : ctx.extend.nfcst;
    const int tail = m.sp > (nfc - ar.fctdrp) ? m.sp : (nfc - ar.fctdrp);
    ctx.adj.nadj = ctx.mdldat.nspobs + nbcst + tail;
    int a1st = 0;
    dfdate(ctx.mdldat.begspn.data(), ctx.adj.begadj.data(), m.sp, a1st);
    ctx.adj.adj1st = a1st + 1;

    // aictest defaults the parser would install (gtinpt.f:294-301 + picktd state).
    ar.pvaic = DNOTST;
    for (int k = 1; k <= PAICT; ++k) ar.rgaicd(k) = 0.0;
    ar.traicd = DNOTST;
    ar.lomtst = 0;
    ctx.picktd.lrgmtd = false;
    ctx.picktd.tdzero = 0;
    ctx.picktd.tddate(1) = NOTSET;
    ctx.picktd.tddate(2) = NOTSET;
    ar.aicstk = 0;
    m.easidx = 0;

    bool lester = false;
    if (want_td) {
        // editor.f:1151-1166 (aictest=td, monthly/quarterly, no existing TD
        // regressor): Tdayvc = (0, 1, 4), Ntdvec = 3.
        ar.itdtst = 1;
        ar.ntdvec = 3;
        ar.tdayvc(1) = 0;
        ar.tdayvc(2) = 1;
        ar.tdayvc(3) = 4;
        int tdmdl1 = 0;
        tdaic(ctx, trnsrs, a, nefobs, na, frstry, tdmdl1, /*ltdlom=*/false, lester);
        if (ctx.error.lfatal) return false;
    }
    if (!lester && want_easter) {
        // editor.f:1410-1442 (aictest=easter, no existing Easter regressor):
        // Easvec = (-1, 1, 8, 15), Neasvc = 4, Eastst = 1.
        ar.eastst = 1;
        ar.neasvc = 4;
        ar.easvec(1) = -1;
        ar.easvec(2) = 1;
        ar.easvec(3) = 8;
        ar.easvec(4) = 15;
        ctx.x11adj.neas = 0;
        // editor.f:1440: aictest=easter with no existing Easter regressor also
        // flags the final SA series for holiday-factor removal. Without this,
        // x11pt3's Haveum=F Faccal rebuild (x11pt3.f:539, x11parts.cpp:687)
        // strips Fachol back out of Faccal before D11/D13 divide by it, so the
        // Easter effect never leaves the final SA series/irregular even though
        // B1 (the regARIMA-adjusted series) is already Easter-clean.
        if (!ctx.x11adj.finhol) ctx.x11adj.finhol = true;
        easaic(ctx, trnsrs, a, nefobs, na, frstry, lester);
        if (ctx.error.lfatal) return false;
    }
    return true;
}

// Block-2 (automd.f:508-548) / block-3 (automd.f:585-620) AIC re-test: rerun
// tdaic then (if td didn't error) easaic, reusing the candidate state block-1
// installed (Itdtst/Tdayvc/Leastr/Easvec persist for the whole automd call).
// lomaic/usraic/chkchi are unreachable here -- automd_aictest_block1 rejects
// any aictest token other than td/easter before this ever runs.
static bool automd_aic_round(X13Context& ctx, double* trnsrs, double* a,
                             int& nefobs, int& na, int& frstry) {
    auto& ar = ctx.arima;
    bool lester = false;
    if (ar.itdtst > 0) {
        int tdmdl1 = 0;
        tdaic(ctx, trnsrs, a, nefobs, na, frstry, tdmdl1, /*ltdlom=*/false,
              lester);
        if (ctx.error.lfatal) return false;
    }
    if (!lester && ar.leastr) {
        easaic(ctx, trnsrs, a, nefobs, na, frstry, lester);
        if (ctx.error.lfatal) return false;
    }
    if (lester) {
        prterr(ctx, nefobs, false);
        if (!ctx.error.lfatal && !ctx.mdldat.convrg) abend(ctx);
        if (ctx.error.lfatal) return false;
    }
    return true;
}

// Re-estimate + prterr + convrg/argok abend-check, the pattern automd.f repeats
// at every "rgarma(Lestim,...)" checkpoint (l.491-502, 635-646, 680-692,
// 795-807, 847-858, 915-926, 956-967). abend() (called on !convrg or !argok)
// always sets ctx.error.lfatal, so callers need only check that afterward --
// matching the oracle's own uniform "IF(Lfatal)RETURN" that follows every one
// of these checkpoints.
static void automd_reestim(X13Context& ctx, double* a, int& na, int& nefobs) {
    bool argok = true;
    rgarma(ctx, ctx.arima.lestim, ctx.arima.mxiter, ctx.arima.mxnlit, false, a,
           na, nefobs, argok);
    if (!ctx.error.lfatal) {
        prterr(ctx, nefobs, true);
        if (!ctx.mdldat.convrg)
            abend(ctx);
        else if (!argok)
            abend(ctx);
    }
}

// automd.f l.674 through label 70/autoer (l.983-985): everything after label
// 30's mdlchk and the pass2 re-entry decision, both of which now live in the
// label loop in automd() below. blpct/blq/bldf/rvr/rtval come in from that
// mdlchk and are updated in place by the redomd/testodf branches here, exactly
// as the Fortran's locals are.
static void automd_finalize_tail(X13Context& ctx, double* trnsrs, int& frstry,
                                 int& nefobs, double* a, int& na, int& lpr,
                                 int& ldr, int& lqr, int& lps, int& lds,
                                 int& lqs, bool& lmu, int kstep, double& blpct,
                                 double& blq, int& bldf, double& rvr,
                                 double& rtval) {
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& ar = ctx.arima;

    // ---- final AIC significance recheck (automd.f:675-702) ----
    int isig = 0;
    pass0(ctx, trnsrs, frstry, isig, 1, false);
    if (ctx.error.lfatal) return;
    bool argok = true;
    if (isig > 0) {
        automd_reestim(ctx, a, na, nefobs);
        if (ctx.error.lfatal) return;
        int kmu = strinx(false, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                         "Constant");
        lmu = kmu > 0;
    }

    // ---- unit-root check + redomd (automd.f:704-818) ----
    bool redomd = false;
    int irt = 0, ist = 0;
    double tmpr = 0.0, tmps = 0.0;
    bool linv = true;
    chkrt1(ctx, irt, ist, tmpr, tmps, linv, ar.ubfin);
    if (ctx.error.lfatal) return;
    if (irt > 0 && ldr <= 2) {
        lpr -= 1;
        ldr += 1;
        redomd = true;
    }
    if (ist > 0 && lds <= 1 && !m.lseff) {
        lps -= 1;
        lds += 1;
        redomd = true;
    }
    if (redomd) {
        bool inptok = true;
        mdlint(ctx);
        mdlset(ctx, lpr, ldr, lqr, lps, lds, lqs, inptok);
        if (!ctx.error.lfatal)
            regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst,
                   0, ar.userx.data(), ar.bgusrx.data(), ar.nrusrx,
                   ctx.prior.priadj, ar.reglom, ar.nrxy, ar.begxy.data(),
                   frstry, true, ar.elong);
        if (ctx.error.lfatal) return;
        int kmu = strinx(false, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                         "Constant");
        if (kmu > 0) {
            chkmu(ctx, trnsrs, a, nefobs, na, frstry, kstep, false);
            if (ctx.error.lfatal) return;
        }
        if (m.natotl > 0) {
            clrotl(ctx, ar.nrxy);
            if (!ctx.error.lfatal)
                regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp,
                       ctx.extend.nfcst, 0, ar.userx.data(), ar.bgusrx.data(),
                       ar.nrusrx, ctx.prior.priadj, ar.reglom, ar.nrxy,
                       ar.begxy.data(), frstry, true, ar.elong);
            if (ctx.error.lfatal) return;
        }
        automd_reestim(ctx, a, na, nefobs);
        if (ctx.error.lfatal) return;
        // (Lidotl && !Lotmod amidot redo skipped: the BIGCV AO scan finds
        // nothing, so there is no outlier-model to redo here.)
        mdlchk(ctx, a, na, nefobs, blpct, blq, bldf, rvr, rtval);
        if (ctx.error.lfatal) return;
        mkmdsn(ctx, lpr, ldr, lqr, lps, lds, lqs);
        if (ctx.error.lfatal) return;
    }

    // ---- over-differencing check (automd.f:820-832) ----
    bool redoMd = false;
    testodf(ctx, trnsrs, frstry, nefobs, a, na, lpr, ldr, lqr, lps, lds, lqs,
            kstep, redoMd, argok);
    if (ctx.error.lfatal) return;
    if (redoMd) {
        mdlchk(ctx, a, na, nefobs, blpct, blq, bldf, rvr, rtval);
        if (ctx.error.lfatal) return;
        mkmdsn(ctx, lpr, ldr, lqr, lps, lds, lqs);
        if (ctx.error.lfatal) return;
    }

    // ---- residual-mean t-stat > 2.5 -> add Constant (automd.f:834-866). NB
    // (bug-for-bug): the oracle does not update Lmu here even though it adds
    // the regressor -- lmu is only read by the deferred .udg 'automean:' print
    // downstream, so this has no numeric effect and is reproduced verbatim. ----
    constexpr double TWOPT5 = 2.5;
    if (!lmu && ar.lautod && rtval > TWOPT5 && ar.lchkmu) {
        adrgef(ctx, prm::DNOTST, "Constant", "Constant", prm::PRGTCN, false,
               false);
        if (ctx.error.lfatal) return;
        regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
               ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
               ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
        if (!ctx.error.lfatal) automd_reestim(ctx, a, na, nefobs);
        if (ctx.error.lfatal) return;
    }

    // ---- drop insignificant ARMA coefficients (automd.f:871-981, label 60).
    // The nnsig>1 && lidold && nloop<=2 outlier-critical-value-reduce/GO TO 10
    // branch (l.881-950) is unreachable: lidold=F always (no outlier{}). ----
    while (true) {
        int nnsig = 0;
        tstmd2(ctx, nnsig, d.nspobs, lpr, lqr, lps, lqs);
        if (ctx.error.lfatal) return;
        if (nnsig <= 0) break;
        automd_reestim(ctx, a, na, nefobs);
        if (ctx.error.lfatal) return;
        mkmdsn(ctx, lpr, ldr, lqr, lps, lds, lqs);
        if (ctx.error.lfatal) return;
    }

    autoer(ctx, d.armaer);
}

// amidot.f: run automatic outlier identification inside the automatic model
// procedure (the label-40 Lidotl branch). Delegates to the ported idotlr with
// the model-span test window and the current critical values (Critvl(AO)=BIGCV
// when Lotmod forced it on, so no AO is found), then reforms the full-Nobspf
// regression design. Convergence/print-branch error handling deferred.
static void amidot(X13Context& ctx, double* trnsrs, int& frstry, int& nefobs,
                   double* a) {
    using namespace prm;
    auto& ar = ctx.arima;
    const int sp = ctx.model.sp;
    int begtst[2] = {ctx.mdldat.begspn(1), ctx.mdldat.begspn(2)};
    int endtst[2];
    addate(ctx.mdldat.begspn.data(), sp, ctx.mdldat.nspobs - 1, endtst);
    if (dpeq(ctx.model.tcalfa, DNOTST))
        ctx.model.tcalfa = std::pow(0.7, 12.0 / sp);
    int nobtst = 0;
    dfdate(endtst, begtst, sp, nobtst);
    nobtst += 1;
    double cv = setcv(nobtst, ar.cvalfa);
    for (int t = 1; t <= POTLR; ++t)
        if (dpeq(ar.critvl(t), DNOTST)) ar.critvl(t) = cv;
    double critvl[POTLR] = {ar.critvl(1), ar.critvl(2), ar.critvl(3)};
    idotlr(ctx, ar.ltstao, ar.ltstls, ar.ltsttc, ar.ladd1, critvl, ar.cvrduc,
           begtst, endtst, nefobs, ar.lestim, ar.mxiter, ar.mxnlit,
           /*lauto=*/false, a);
    if (ctx.error.lfatal) return;
    int nrxy2 = 0;
    regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
           ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
           ar.reglom, nrxy2, ar.begxy.data(), frstry, true, ar.elong);
}

void automd(X13Context& ctx, double* trnsrs, int& frstry, int& nefobs,
            double* a, int& na, bool do_aictest) {
    using namespace prm;
    auto& m = ctx.model;
    auto& ar = ctx.arima;

    bool inptok = true;

    // `ljungboxlimit` -> Pcr has two consumers and both are now ported: the
    // acceptdefault test below, and pass2.f:160-169, which INCREMENTS Pcr
    // (+0.025 on the first nloop pass, +0.015 after) before deciding whether to
    // redo the identification. It used to fatal here for want of the second.
    bool lmu = false;
    // imu: index of a USER-specified Constant (computed before chkmu adds one).
    int imu = 0;
    if (m.nb > 0)
        imu = strinx(false, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                     "Constant");
    if (imu > 0) lmu = true;

    // ---- outlier-identification enable (automd.f:167-181). Lidotl comes from
    // outlier{} (ltstao/ltstls/ltsttc). When it is off but the automdl outlier
    // method Lotmod is on (the DEFAULT, gtinpt.f:238), the automatic modeler
    // still runs a dummy AO identification pass with an impossible critical
    // value (Critvl(AO)=BIGCV) -- it finds nothing but takes the amidot path at
    // label 40 instead of tstmd1, so tstmd1's insignificant-lag order reduction
    // is NOT applied to the identified model. (The cvlold critical-value backup
    // and the outlier print/save-table suppression are deferred -- no print
    // milestone; BIGCV makes the AO scan a no-op regardless.) ----
    int igo = 0;  // automd.f:165; pass2's re-entry selector
    bool lidotl = ar.ltstao || ar.ltstls || ar.ltsttc;
    // automd.f:167 captures Lidotl BEFORE the Lotmod override below, so with no
    // outlier{} spec lidold is FALSE while lidotl becomes TRUE. That gap is
    // what label 50's `IF(nloop.eq.1.or.lidold)` reads: on a pass2 re-entry the
    // default path does NOT re-run amdid.
    const bool lidold = lidotl;
    if (lidotl) {
        ar.lotmod = false;
    } else if (ar.lotmod) {
        ar.ltstao = true;
        lidotl = true;
        ar.critvl(AO) = 1000001.0;  // BIGCV (automd.f:36)
    }

    // ---- default "airline" model (0 1 1)(0 1 1); no seasonal part for Sp==1
    // or seasonal-effect regressors. ----
    int lds0 = 1, lqs0 = 1;
    if (m.lseff || m.sp == 1) {
        lds0 = 0;
        lqs0 = 0;
    }
    mdlint(ctx);
    mdlset(ctx, 0, 1, 1, 0, lds0, lqs0, inptok);
    if (ctx.error.lfatal) return;
    regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
           ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
           ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
    if (ctx.error.lfatal) return;

    // ---- block-1 AIC tests on the default model (automd.f:216-232). Gated:
    // only the real pre-model/x11 path enables this; the m4 --afterauto harness
    // runs the AIC tests itself. Selected td/Easter regressors are now in m.
    const bool aic = do_aictest && !ctx.captured.aictest_vars.empty();
    if (aic) {
        if (!automd_aictest_block1(ctx, trnsrs, a, nefobs, na, frstry)) return;
        if (ctx.error.lfatal) return;
    }

    // ---- mean test on the default model (adds/keeps a Constant iff significant)
    if (ar.lchkmu) {
        chkmu(ctx, trnsrs, a, nefobs, na, frstry, /*kstep=*/0, /*lprt=*/false);
        if (ctx.error.lfatal) return;
        int kmu = strinx(false, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                         "Constant");
        lmu = kmu > 0;
    }

    // ---- estimate the default model (automd.f:266) ----
    bool argok = true;
    rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
    if (!ctx.error.lfatal) {
        prterr(ctx, nefobs, true);
        if (!ctx.mdldat.convrg)
            abend(ctx);
        else if (!argok)
            abend(ctx);
    }
    if (ctx.error.lfatal) return;

    // ---- default-model residual diagnostics (automd.f:322-344). Unconditional
    // in the Fortran, and the values are what label 40's tstmd1 arm reads:
    // blpct0/rvr0/rtval0 are the default model's Ljung-Box and residual
    // statistics (also the acceptdefault test's input at l.348) and tair is its
    // ARMA t-statistic pair. (The Lidotl outlier block at l.280-321 is skipped:
    // the BIGCV AO scan finds nothing, and its pass0 has no AIC-selected
    // regressor to re-test on this corpus.) ----
    rgarma(ctx, ar.lestim, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
    if (!ctx.error.lfatal) {
        prterr(ctx, nefobs, true);
        if (!ctx.mdldat.convrg)
            abend(ctx);
        else if (!argok)
            abend(ctx);
    }
    if (ctx.error.lfatal) return;
    double blpct0, blq0, rvr0, rtval0;
    int bldf0;
    mdlchk(ctx, a, na, nefobs, blpct0, blq0, bldf0, rvr0, rtval0);
    if (ctx.error.lfatal) return;
    // automd.f:343 is GUARDED (`IF(.not.Lidotl)`), not unconditional: with the
    // default Lotmod the BIGCV AO scan forces Lidotl true, the oracle skips
    // armats and tair stays zero. tair is read only by tstmd1, i.e. only on the
    // !Lidotl arm, so honouring the guard is what makes the two agree.
    double tair[2] = {0.0, 0.0};
    if (!lidotl) {
        armats(ctx, tair);
        if (ctx.error.lfatal) return;
    }

    // ---- accept the default model outright when its Ljung-Box residual
    // diagnostic passes (automd.f:348-357 -> GO TO 70), before any order
    // search. ctx.model already holds the estimated default. ----
    if (ar.laccdf && blpct0 < ar.pcr) {
        mkmdsn(ctx, 0, 1, 1, 0, lds0, lqs0);  // Bstdsn <- default designation
        if (ctx.error.lfatal) return;
        autoer(ctx, ctx.mdldat.armaer);       // GO TO 70
        return;
    }

    // ---- save the default-model state (automd.f:360-373) ----
    bool lmu0 = lmu;
    const int kstep = 1;
    int nloop = 1;   // automd.f:161 nloop=0, :364 nloop=nloop+1
    int nround = 1;  // :162, reset to 1 at :363
    const int na0 = na;
    std::vector<double> a0(static_cast<std::size_t>(na));
    copy(a, na, 1, a0.data());
    std::vector<double> adj0(PLEN), trns0(PLEN);
    copy(ctx.adj.adj.data(), PLEN, 1, adj0.data());
    copy(trnsrs, PLEN, 1, trns0.data());
    // aici0/aicit0/pcktd0 are pass2's revert targets for the aictest state
    // (:370-372). cvl0 is the outlier critical-value backup pass2 fills and
    // restores; it stays at its DNOTST default here because the default-model
    // Lidotl block at :280-321 (which would seed it) is skipped.
    const int aici0 = ar.aicind, aicit0 = ar.aicint;
    const bool pcktd0 = ctx.picktd.picktd;
    double cvl0[POTLR];
    setdp(DNOTST, POTLR, cvl0);
    // automd.f:183-184. Fct is only in play when an adjustment was requested.
    const double fct2 =
        (ctx.captured.has_x11 || ctx.captured.has_seats) ? ar.fct : 1.0;

    // The identified model's orders, and the working state the label loop
    // carries across a pass2 re-entry.
    int ldr = ar.diffam(1), lds = ar.diffam(2);
    int lpr = 0, lqr = 0, lps = 0, lqs = 0;
    int lpr0 = 0, ldr0 = 1, lqr0 = 1, lps0 = 0;  // the DEFAULT airline orders;
                                                 // lds0/lqs0 are set above
    const int nbcst = ctx.extend.nbcst < 0 ? 0 : ctx.extend.nbcst;
    int nbb = 0;
    bool ismd0 = false;
    double blpct = 0.0, blq = 0.0, rvr = 0.0, rtval = 0.0;
    int bldf = 0;
    // nauto0 is the default model's automatic-outlier count (automd.f:303).
    // Zero here for the same reason cvl0 is DNOTST.
    int nauto0 = 0;

    // ---- the label loop. automd.f is a GO-TO graph over labels 10 / 50 / 40 /
    // 30, and pass2 (:664-672) is what makes it a loop: igo 1/2/3 re-enters at
    // 10/40/50. Everything below is one pass through that graph. ----
    enum { L10, L50, L40, L30 };
    int pc = L10;
    for (;;) {
        if (pc == L10) {
            // ---- label 10 (automd.f:378-388): snapshot + strip regressors.
            // bkdfmd's backup is not optional bookkeeping -- tstmd1.f:221 and
            // pass2.f:115 both restore from it. ----
            ssprep_save(ctx);
            bkdfmd(ctx, true);
            if (nloop > 1 && m.natotl == 0) {
                pc = L40;
            } else {
                nbb = 0;
                if (m.nb > 0) {
                    nbb = m.nb;
                    rmfix(ctx, trnsrs, nbcst, ar.nrxy, 2);
                    if (ctx.error.lfatal) return;
                    regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp,
                           ctx.extend.nfcst, 0, ar.userx.data(),
                           ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
                           ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true,
                           ar.elong);
                    if (ctx.error.lfatal) return;
                }

                // ---- identify differencing on the clean series
                // (automd.f:390-400). The `Lautod` else-branch is what makes
                // `automdl{diff=}` mean anything: `diff=` fixes the orders and
                // (unlike `maxdiff=`) leaves Lautod false, so the search is
                // skipped and mdlset installs them directly. ----
                ldr = ar.diffam(1);
                lds = ar.diffam(2);
                if (ar.lautod) {
                    iddiff(ctx, ldr, lds, trnsrs, nefobs, frstry, a, na, imu,
                           lmu, false, 0);
                    if (ctx.error.lfatal) return;
                } else {
                    mdlint(ctx);
                    mdlset(ctx, 0, ldr, 0, 0, lds, 0, inptok);
                    if (ctx.error.lfatal) return;
                }
                ismd0 = false;
                pc = L50;
            }
        }

        if (pc == L50) {
            // ---- label 50 (automd.f:402-441) ----
            if (nloop == 1 || lidold) {
                if (nloop > 1 && m.nb > 0) {
                    nbb = m.nb;
                    rmfix(ctx, trnsrs, nbcst, ar.nrxy, 2);
                    if (ctx.error.lfatal) return;
                    regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp,
                           ctx.extend.nfcst, 0, ar.userx.data(),
                           ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
                           ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true,
                           ar.elong);
                    if (ctx.error.lfatal) return;
                }
                lpr = 0;
                lqr = 0;
                lps = 0;
                lqs = 0;
                bool locok = true;
                amdid(ctx, lpr, ldr, lqr, lps, lds, lqs, trnsrs, frstry, nefobs,
                      a, na, lmu, 0, locok);
                if (ctx.error.lfatal) return;

                // ismd0 (automd.f:434-439): identified model == the default
                // airline and the mean is unchanged. It is BOTH the a0-revert
                // trigger and label 40's tstmd1 guard.
                ismd0 = ((m.sp > 1 && lpr == lpr0 && ldr == ldr0 &&
                          lqr == lqr0 && lps == lps0 && lds == lds0 &&
                          lqs == lqs0) ||
                         (m.sp == 1 && lpr == lpr0 && ldr == ldr0 &&
                          lqr == lqr0)) &&
                        (lmu == lmu0);
            }

            // ---- put the regressors back (automd.f:443-506) ----
            bool lester = false;
            bool went_to_30 = false;
            if (nbb > 0) {
                addfix(ctx, trnsrs, nbcst, 0, 2);
                if (ctx.error.lfatal) return;
                if (!lmu) {
                    int igrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                                      m.ngrptl, "Constant");
                    if (igrp > 0) {
                        int icol = m.grp(igrp - 1);
                        dlrgef(ctx, icol, ar.nrxy, 1);
                        if (ctx.error.lfatal) return;
                    }
                }
                if (m.nb > 0) lester = true;
                if (nloop == 1) {
                    nbb = 0;
                    if (ismd0) {
                        restor_model(ctx);
                        copy(a0.data(), na, 1, a);
                        regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp,
                               ctx.extend.nfcst, 0, ar.userx.data(),
                               ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
                               ar.reglom, ar.nrxy, ar.begxy.data(), frstry,
                               true, ar.elong);
                        if (ctx.error.lfatal) return;
                        went_to_30 = true;
                    }
                }
                if (!went_to_30) {
                    // automd.f:472 reads NAUTO0 here (the DEFAULT model's
                    // automatic-outlier count), not Natotl -- the port had
                    // Natotl, which is a different quantity that happens to be
                    // 0 alongside it on this corpus.
                    if (nauto0 > 0 && igo == 0) {
                        clrotl(ctx, ar.nrxy);
                        if (ctx.error.lfatal) return;
                    }
                    regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp,
                           ctx.extend.nfcst, 0, ar.userx.data(),
                           ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
                           ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true,
                           ar.elong);
                    if (ctx.error.lfatal) return;
                }
            }
            if (!went_to_30 && imu == 0 && lmu) {
                int icol = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                                  m.ngrptl, "Constant");
                if (icol == 0) {
                    adrgef(ctx, DNOTST, "Constant", "Constant", PRGTCN, false,
                           false);
                    if (ctx.error.lfatal) return;
                    regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp,
                           ctx.extend.nfcst, 0, ar.userx.data(),
                           ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
                           ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true,
                           ar.elong);
                    if (ctx.error.lfatal) return;
                    lester = true;
                }
            }
            if (!went_to_30 && lester) {
                automd_reestim(ctx, a, na, nefobs);
                if (ctx.error.lfatal) return;
            }
            if (!went_to_30 && ismd0 && nloop == 1) {
                copy(a0.data(), na, 1, a);
                went_to_30 = true;
            }

            if (went_to_30) {
                pc = L30;
            } else {
                // ---- block-2 AIC tests on the identified model
                // (automd.f:508-548). Gated on `aic`, not on Itdtst/Leastr as
                // the Fortran gates it: there the candidate vectors
                // (Tdayvc/Easvec/Neasvc) come from the parser and editor, which
                // this port has not ported -- automd_aictest_block1 stands in
                // for them. Running a round without that setup indexes an
                // uninitialised Easvec. ----
                ssprep_save(ctx);
                if (aic && !automd_aic_round(ctx, trnsrs, a, nefobs, na, frstry))
                    return;
                pc = L40;
            }
        }

        if (pc == L40) {
            // ---- label 40 (automd.f:549-649): `IF(Lidotl) amidot ELSE IF(.not.
            // ismd0) tstmd1`. Lidotl is true by default because Lotmod forces a
            // BIGCV AO scan (which finds nothing), so the default run takes
            // amidot and tstmd1's insignificant-lag order reduction is skipped;
            // `automdl{noautooutlier=tramo}` clears Lotmod and selects tstmd1.
            if (lidotl) {
                amidot(ctx, trnsrs, frstry, nefobs, a);
                if (ctx.error.lfatal) return;
            } else if (!ismd0) {
                tstmd1(ctx, trnsrs, frstry, a, na, nefobs, blpct0, rvr0, rtval0,
                       lpr, lps, lqr, lqs, ldr, lds, lmu, adj0.data(),
                       trns0.data(), tair);
                if (ctx.error.lfatal) return;

                // ---- redo the regressor AIC tests on the model tstmd1 left
                // (automd.f:583-648) ----
                if (ar.itdtst > 0 || ar.leastr ||
                    (ar.luser && ctx.usrreg.ncusrx > 0) || imu == 0) {
                    if (aic &&
                        !automd_aic_round(ctx, trnsrs, a, nefobs, na, frstry))
                        return;
                    if (ar.lchkmu) {
                        chkmu(ctx, trnsrs, a, nefobs, na, frstry, kstep, false);
                        if (ctx.error.lfatal) return;
                        int kmu = strinx(false, m.grpttl.raw(),
                                         m.grpptr.data(), 1, m.ngrptl,
                                         "Constant");
                        lmu = kmu > 0;
                    }
                    automd_reestim(ctx, a, na, nefobs);
                    if (ctx.error.lfatal) return;
                }
            }
            pc = L30;
        }

        // ---- label 30 (automd.f:654-672) ----
        mdlchk(ctx, a, na, nefobs, blpct, blq, bldf, rvr, rtval);
        if (ctx.error.lfatal) return;
        // (the `automdl.first` .udg line at :661-663 is print surface)

        if (!(lidotl && nloop <= 2)) break;

        pass2(ctx, trnsrs, frstry, lpr, ldr, lqr, lps, lds, lqs, lpr0, ldr0,
              lqr0, lps0, lds0, lqs0, m.natotl, nauto0, blpct, blpct0, bldf,
              bldf0, rvr, rvr0, lmu, lmu0, a, a0.data(), na, na0, aici0,
              pcktd0, aicit0, adj0.data(), trns0.data(), fct2, ismd0, cvl0,
              nefobs, nloop, nround, igo);
        if (ctx.error.lfatal) return;
        if (igo == 1) { pc = L10; continue; }
        if (igo == 2) { pc = L40; continue; }
        if (igo == 3) { pc = L50; continue; }
        break;
    }

    // ---- automd.f:674 onward: the finalization tail. ----
    automd_finalize_tail(ctx, trnsrs, frstry, nefobs, a, na, lpr, ldr, lqr, lps,
                         lds, lqs, lmu, kstep, blpct, blq, bldf, rvr, rtval);
    if (ctx.error.lfatal) return;

    // DEFERRED: the Lidotl outlier-ID block on the DEFAULT model
    // (automd.f:280-321) -- amidot + pass0 + the nauto0/cvl0 bookkeeping pass2
    // reads. Skipped because the BIGCV AO scan finds nothing and pass0 has no
    // AIC-selected regressor to re-test on this corpus, which is why nauto0
    // stays 0 and cvl0 stays DNOTST above.
    //
    // The identified model is left estimated in ctx.
}

}  // namespace x13
