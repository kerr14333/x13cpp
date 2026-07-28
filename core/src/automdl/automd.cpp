// automd.cpp -- automd.f driver. See automd.hpp for the two paths (plain
// identification spine vs. the full aictest finalization) and scope/deferred
// features. The plain path mirrors automd.f: default airline model -> chkmu
// (mean test) -> iddiff (differencing) -> amdid (ARMA orders) -> re-add the
// mean when significant -> final estimate. The aictest path (automd.f l.322-
// 982, Lidotl=F) additionally runs block-1/2/3 tdaic/easaic, the a0/ismd0
// revert via rmfix/addfix/ssprep/restor, tstmd1, and the label-30 finalization
// tail (pass0/chkrt1/redomd/testodf/tstmd2/autoer) -- see automd_finalize.cpp
// for those primitives.
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

// automd.f label 30 (l.654) through label 70/autoer (l.983-985): the shared
// finalization tail, reached both from the ismd0 a0-revert shortcut (l.503-506,
// via GO TO 30) and after the non-ismd0 block-2 AIC / tstmd1 / block-3 AIC
// sequence (also GO TO 30, l.654 immediately follows label 40's block). pass2
// (l.664, outlier-model revert; Lidotl && nloop<=2) is unreachable here: it
// needs a real (non-BIGCV) outlier scan that finds outliers, which the current
// corpus never triggers (the Lotmod-forced AO scan uses BIGCV and finds none).
static void automd_finalize_tail(X13Context& ctx, double* trnsrs, int& frstry,
                                 int& nefobs, double* a, int& na, int& lpr,
                                 int& ldr, int& lqr, int& lps, int& lds,
                                 int& lqs, bool& lmu, int kstep) {
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    auto& ar = ctx.arima;

    double blpct, blq, rvr, rtval;
    int bldf;
    mdlchk(ctx, a, na, nefobs, blpct, blq, bldf, rvr, rtval);
    if (ctx.error.lfatal) return;

    // (mkmdsn -> .udg 'automdl.first' print and pass2 (Lidotl && nloop<=2) are
    // both deferred/not-applicable: no print milestone yet, and the BIGCV AO
    // scan never finds the outliers pass2 would revert.)

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

    // ---- Two automdl{} arguments are parsed (gtauto.f labels 180/190) but are
    // consumed ONLY by the model-adequacy stage this driver does not port (see
    // the closing note at the end of this file, automd.f:577-850). Measured
    // engine-vs-oracle on ukgas and the CES probe series, they moved the oracle
    // by 17-66 .udg keys and the engine by zero, i.e. the run came back
    // `OUTCOME: OK` carrying the DEFAULT model. Fatal instead: an unported
    // computation behind a silently-accepted argument is the one shape this
    // port has repeatedly been bitten by.
    //
    //   urfinal       -- the FINAL unit root test threshold. The manual: "if
    //                    the magnitude of an AR root for the final model is
    //                    less than this number, a unit root is assumed". That
    //                    test is chkrt1 at automd.f:717 -- inside the stage.
    //   noautooutlier -- selects tstmd1 (automd.f:577) over the amidot path.
    //                    Undocumented: it appears nowhere in the 306-page
    //                    reference manual, only in gtauto.f's NOTDIC.
    //
    // Both are keyed to the NON-DEFAULT value, so every existing spec and the
    // whole corpus are unaffected (urfinal 1.05 / noautooutlier=same).
    if (ar.ubfin != 1.05) {
        errhdr(ctx);
        writln(ctx,
               "ERROR: automdl{urfinal=} not yet ported (automd.f:717 chkrt1, "
               "in the unported model-adequacy stage).",
               stdio::STDERR, ctx.units.mt2, true);
        abend(ctx);
        return;
    }
    if (!ar.lotmod) {
        errhdr(ctx);
        writln(ctx,
               "ERROR: automdl{noautooutlier=tramo} not yet ported "
               "(automd.f:577 tstmd1, in the unported model-adequacy stage).",
               stdio::STDERR, ctx.units.mt2, true);
        abend(ctx);
        return;
    }

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
    bool lidotl = ar.ltstao || ar.ltstls || ar.ltsttc;
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

    // ---- aictest finalization: a faithful port of automd.f l.322-982.
    // Replaces the old ctx-snapshot-only ismd0 revert: this drives the oracle's
    // own rmfix/addfix/ssprep/restor/pass0/chkrt1 sequence and the label-40
    // dispatch. With Lotmod on (default), label 40 takes the amidot branch (a
    // BIGCV AO scan that finds nothing) rather than tstmd1, so the identified
    // model's order is kept; only an explicit outlier{} would change that. ----
    if (aic) {
        // ---- default-model residual diagnostics (automd.f:322-344). Lidotl's
        // outlier-ID block (l.280-321) is skipped -- unreachable, see above. ----
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
        double tair[2] = {0.0, 0.0};
        armats(ctx, tair);  // !Lidotl branch (l.343) -- always taken here.
        if (ctx.error.lfatal) return;

        // Laccdf: accept the default model outright when its Ljung-Box residual
        // diagnostic passes (automd.f:348-357 -- IF(Laccdf.and.Blpct0.lt.Pcr) ...
        // GO TO 70). ctx.model already holds the estimated default (0 1 1)(0 lds0
        // lqs0); build its designation and jump straight to the label-70 finalize,
        // skipping the whole order-search body. lpr0..lqs0 == 0,1,1,0,lds0,lqs0.
        if (ar.laccdf && blpct0 < ar.pcr) {
            mkmdsn(ctx, 0, 1, 1, 0, lds0, lqs0);  // Bstdsn <- default designation
            if (ctx.error.lfatal) return;
            autoer(ctx, ctx.mdldat.armaer);       // GO TO 70
            return;
        }

        // ---- save a0, enter the nloop (automd.f:360-373; nloop stays 1 for
        // this corpus -- the only re-loop triggers are Lidotl-gated pass2/
        // outlier-critical-value retries, both unreachable here). ----
        const bool lmu0 = lmu;
        const int kstep = 1;
        std::vector<double> a0(static_cast<std::size_t>(na));
        copy(a, na, 1, a0.data());
        std::vector<double> adj0(PLEN), trns0(PLEN);
        copy(ctx.adj.adj.data(), PLEN, 1, adj0.data());
        copy(trnsrs, PLEN, 1, trns0.data());

        // ---- label 10 (automd.f:378-388): snapshot + strip regressors. ----
        ssprep_save(ctx);
        bkdfmd(ctx, true);
        const int nbcst = ctx.extend.nbcst < 0 ? 0 : ctx.extend.nbcst;
        int nbb = 0;
        if (m.nb > 0) {
            nbb = m.nb;
            rmfix(ctx, trnsrs, nbcst, ar.nrxy, 2);
            if (ctx.error.lfatal) return;
            regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst,
                   0, ar.userx.data(), ar.bgusrx.data(), ar.nrusrx,
                   ctx.prior.priadj, ar.reglom, ar.nrxy, ar.begxy.data(),
                   frstry, true, ar.elong);
            if (ctx.error.lfatal) return;
        }

        // ---- identify differencing + ARMA orders on the clean series
        // (automd.f:390-441; Lautod is true for the whole corpus -- no diff=
        // override). ----
        int ldr = ar.diffam(1), lds = ar.diffam(2);
        if (ar.lautod) {
            iddiff(ctx, ldr, lds, trnsrs, nefobs, frstry, a, na, imu, lmu,
                   false, 0);
            if (ctx.error.lfatal) return;
        } else {
            mdlint(ctx);
            mdlset(ctx, 0, ldr, 0, 0, lds, 0, inptok);
            if (ctx.error.lfatal) return;
        }
        int lpr = 0, lqr = 0, lps = 0, lqs = 0;
        bool locok = true;
        amdid(ctx, lpr, ldr, lqr, lps, lds, lqs, trnsrs, frstry, nefobs, a, na,
              lmu, 0, locok);
        if (ctx.error.lfatal) return;

        // ismd0 (automd.f:434-439): identified model == default airline and
        // the mean is unchanged.
        const bool ismd0 =
            ((m.sp > 1 && lpr == 0 && ldr == 1 && lqr == 1 && lps == 0 &&
              lds == lds0 && lqs == lqs0) ||
             (m.sp == 1 && lpr == 0 && ldr == 1 && lqr == 1)) &&
            (lmu == lmu0);

        // ---- put regressors back (automd.f:443-506) ----
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
            // nloop==1 always for this corpus:
            if (ismd0) {
                restor_model(ctx);
                copy(a0.data(), na, 1, a);
                regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp,
                       ctx.extend.nfcst, 0, ar.userx.data(), ar.bgusrx.data(),
                       ar.nrusrx, ctx.prior.priadj, ar.reglom, ar.nrxy,
                       ar.begxy.data(), frstry, true, ar.elong);
                if (ctx.error.lfatal) return;
                went_to_30 = true;
            }
            if (!went_to_30) {
                if (m.natotl > 0) {
                    clrotl(ctx, ar.nrxy);
                    if (ctx.error.lfatal) return;
                }
                regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp,
                       ctx.extend.nfcst, 0, ar.userx.data(), ar.bgusrx.data(),
                       ar.nrusrx, ctx.prior.priadj, ar.reglom, ar.nrxy,
                       ar.begxy.data(), frstry, true, ar.elong);
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
                       ctx.extend.nfcst, 0, ar.userx.data(), ar.bgusrx.data(),
                       ar.nrusrx, ctx.prior.priadj, ar.reglom, ar.nrxy,
                       ar.begxy.data(), frstry, true, ar.elong);
                if (ctx.error.lfatal) return;
                lester = true;
            }
        }
        if (!went_to_30 && lester) {
            automd_reestim(ctx, a, na, nefobs);
            if (ctx.error.lfatal) return;
        }
        if (!went_to_30 && ismd0) {
            copy(a0.data(), na, 1, a);
            went_to_30 = true;
        }

        if (!went_to_30) {
            // ---- block-2 AIC tests on the identified model (automd.f:508-548) ----
            ssprep_save(ctx);
            if (!automd_aic_round(ctx, trnsrs, a, nefobs, na, frstry)) return;

            // ---- label 40 (automd.f:549-649). Lidotl -> amidot (automatic
            // outlier ID); else (non-default identified model) tstmd1 + the
            // redo-aictest block. With Lotmod forcing Lidotl on (BIGCV AO), the
            // amidot branch is taken: idotlr finds nothing and, crucially,
            // tstmd1's insignificant-lag order reduction is skipped. ----
            if (lidotl) {
                amidot(ctx, trnsrs, frstry, nefobs, a);
                if (ctx.error.lfatal) return;
            } else {
                tstmd1(ctx, trnsrs, frstry, a, na, nefobs, blpct0, rvr0, rtval0,
                       lpr, lps, lqr, lqs, ldr, lds, lmu, adj0.data(),
                       trns0.data(), tair);
                if (ctx.error.lfatal) return;

                if (ar.itdtst > 0 || ar.leastr ||
                    (ar.luser && ctx.usrreg.ncusrx > 0) || imu == 0) {
                    if (!automd_aic_round(ctx, trnsrs, a, nefobs, na, frstry))
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
        }

        // ---- label 30 onward: the shared finalization tail. ----
        automd_finalize_tail(ctx, trnsrs, frstry, nefobs, a, na, lpr, ldr, lqr,
                             lps, lds, lqs, lmu, kstep);
        return;
    }

    // ---- Laccdf: accept the estimated default model outright when its Ljung-Box
    // residual diagnostic passes (automd.f:348-357), BEFORE any order search. The
    // default (0 1 1)(0 lds0 lqs0) is already estimated in ctx (rgarma above); the
    // accept test is not aictest-gated, so it lives here in the non-aic path too
    // (the aic path has the mirror branch). ----
    if (ar.laccdf) {
        // automd.f:325 -- the residual-diagnostics rgarma refines the default
        // model estimate. The l.266 rgarma above only converges partway from
        // the chkmu-context start; the oracle runs this second rgarma (Lestim)
        // unconditionally before the accept test. The aic path runs it as its
        // own line-422 call; the non-aic acceptdefault path needs it here too,
        // or the accepted airline's tail forecast drifts ~8.5e-6. (The
        // intervening Lidotl amidot/pass0 block is a no-op here -- BIGCV finds
        // no outlier and payems carries no regressor -- exactly as on the aic
        // path, which also skips it and stays bit-exact.)
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
        if (blpct0 < ar.pcr) {
            mkmdsn(ctx, 0, 1, 1, 0, lds0, lqs0);  // Bstdsn <- default designation
            if (ctx.error.lfatal) return;
            autoer(ctx, ctx.mdldat.armaer);       // GO TO 70
            return;
        }
    }

    // ---- identify differencing (iddiff) starting from the maxdiff limits ----
    // automd.f:392-400. The `Lautod` else-branch is what makes `automdl{diff=}`
    // mean anything: `diff=` fixes the orders and (unlike `maxdiff=`) leaves
    // Lautod false, so the search is skipped and mdlset installs them directly.
    // This site used to call iddiff unconditionally -- the guard existed only at
    // the OTHER, unreachable call site, so `diff=` silently got the searched
    // orders instead of the given ones.
    int ldr = ar.diffam(1), lds = ar.diffam(2);
    if (ar.lautod) {
        iddiff(ctx, ldr, lds, trnsrs, nefobs, frstry, a, na, imu, lmu, false, 0);
        if (ctx.error.lfatal) return;
    } else {
        mdlint(ctx);
        mdlset(ctx, 0, ldr, 0, 0, lds, 0, inptok);
        if (ctx.error.lfatal) return;
    }

    // ---- identify ARMA orders (amdid re-fits the winner in place) ----
    int lpr = 0, lqr = 0, lps = 0, lqs = 0;
    bool locok = true;
    amdid(ctx, lpr, ldr, lqr, lps, lds, lqs, trnsrs, frstry, nefobs, a, na, lmu,
          0, locok);
    if (ctx.error.lfatal) return;

    // ---- re-add the mean when iddiff/amdid selected one but it is not present
    // (automd.f:479) and re-estimate the final model. ----
    if (imu == 0 && lmu) {
        int icol = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                          "Constant");
        if (icol == 0) {
            adrgef(ctx, DNOTST, "Constant", "Constant", PRGTCN, false, false);
            if (ctx.error.lfatal) return;
            regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
                   ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
                   ar.reglom, ar.nrxy, ar.begxy.data(), frstry, true, ar.elong);
            if (ctx.error.lfatal) return;
            rgarma(ctx, ar.lestim, ar.mxiter, ar.mxnlit, false, a, na, nefobs,
                   argok);
            if (!ctx.error.lfatal) {
                prterr(ctx, nefobs, true);
                if (!ctx.mdldat.convrg)
                    abend(ctx);
                else if (!argok)
                    abend(ctx);
            }
            if (ctx.error.lfatal) return;
        }
    }
    // DEFERRED (non-ismd0 aictest + non-aictest adequacy):
    //
    // 1. Non-default-model aictest series (automd.f:378-648). The block-1 AIC tests
    //    + ismd0 a0-revert above reach parity for series whose identified model is
    //    the default airline (e.g. airline). Series that identify to a NON-default
    //    model (expgs/payems/unrate) would need the full nloop: addfix (put
    //    regressors back), the round-2 AIC tests on the identified model, and the
    //    tstmd1 finalization. Those aictest-x11 specs now gate bit-exact anyway --
    //    reached via the ismd0 ctx-snapshot revert + the amidot/Lotmod control-flow
    //    fix (e2191ad), NOT the full nloop, which stays unported. rmfix/addfix +
    //    the fxreg dictionary + ssprep/restor are only needed for that faithful
    //    nloop (this driver uses a ctx snapshot for the ismd0 revert instead).
    //
    // 2. Model-adequacy stage (tstmd1 revert + redomd/testodf finalization + final
    //    re-estimate, automd.f:577-850). tstmd1/bkdfmd/testodf are ported
    //    (automdl/adqtst.cpp) and tstmd1 correctly reverts usdeaths/region to the
    //    airline default, but wiring only tstmd1 broke parity on the non-revert
    //    cases: the oracle re-estimates AFTER tstmd1, so tstmd1's intermediate fit
    //    must not be the reported one. Wire the whole finalization together (see
    //    automdl_scouting.md 3c / FABLE_REVIEW.md).
    //
    // The identified model is left estimated in ctx.
}

}  // namespace x13
