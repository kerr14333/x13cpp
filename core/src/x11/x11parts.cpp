// x11parts.cpp -- X-11 decomposition parts spine (see x11parts.hpp). Faithful
// port of the vendored oracle Fortran: x11pt1.f.
//
// Index convention: 0-based C pointers, Fortran index i -> element [i-1]; range
// args (Pos1ob/Posfob/Posffc/Pos1bk/lastpr) stay Fortran 1-based. ALL print/save
// (table/punch/prtshd/prttbl/x11plt/prtadj) is deferred and NOT ported -- each
// such call is dropped, its Lfatal guard collapsing to the no-fatal path. Loop
// and float op order preserved for parity.
#include "x11/x11parts.hpp"

#include "common/x13context.hpp"
#include "x11/x11filt.hpp"          // divsub, addmul, setmv, logar, averag
#include "x11/d8bd9a.hpp"       // prtd8b_savelog, prtd9a_savelog
#include "x11/x11seas.hpp"          // vsfa, vsfb, vsfc
#include "x11/x11reg.hpp"           // x11mdl_td (x11regression irregular regression)
#include "x11/loadxr.hpp"           // loadxr (regARIMA <-> x11reg model swap)
#include "x11/x11xtrm.hpp"          // xtrm, vtest, entsch
#include "x11/x11drv.hpp"           // forcst, vtc, si
#include "x11/x11tests.hpp"         // ftest, kwtest, mstest, combft (F2 tests)
#include "x11/x11force.hpp"         // qmap (force yearly totals)
#include "x11/slidingspans.hpp"     // ssrit
#include "x11/shrink.hpp"           // shrink (seasonal-factor shrinkage)
#include "specparse/specparse.hpp"  // copy, setlg, abend, errhdr, writln, stdio
#include "transform/transform.hpp"  // invfcn (makadj user-prior inverse transform)
#include "numeric/numeric.hpp"      // dpeq
#include "gen/notset.hpp"           // prm::NOTSET

#include <string>

namespace x13 {

namespace {
constexpr int PLEN = 1020;  // srslen.prm: POBS + 2*PFCST

// ispos.f: are all values of a 1-based array over [l1,l2] strictly positive?
bool ispos(const double* s, int l1, int l2) {
    for (int i = l1; i <= l2; ++i)
        if (s[i - 1] <= 0.0) return false;
    return true;
}

// Signal a feature branch that depends on a still-unported routine (mirrors
// regvar.cpp's local not_ported; the X-11 spine reaches this only for prior-TD /
// x11-regression trading-day, which needs pritd/ssrit).
void x11_not_ported(X13Context& ctx, const char* what) {
    errhdr(ctx);
    writln(ctx, std::string("ERROR: ") + what + " not yet ported (M5 X-11 spine).",
           stdio::STDERR, ctx.units.mt2, true);
    abend(ctx);
}
}  // namespace

// chktrn.f -- multiplicative-mode trend-positivity check/repair (called from
// x11pt2 when Muladd==0). Scans the trend-cycle Stc over the padded span; any
// non-positive value is replaced in place by the mean of its two nearest positive
// neighbours, or by the nearest positive value when it sits at a series end.
// Returns oktrn = "all positive over the [Pos1ob,last] core span" (the caller
// seeds oktrn=true, so the all-positive early return keeps it true). tstfct is
// in/out: reset to false when nothing needs repair. The Fortran Kpart/Ktabl/Trnchr
// args are print-only (a warning message + the '*' markers consumed by prttrn) and
// are dropped under the deferred-print convention.
bool chktrn(X13Context& ctx, double* stc, bool& tstfct) {
    const x11ptr_cmn& ptr = ctx.x11ptr;
    const extend_cmn& ext = ctx.extend;
    const int pos1bk = ptr.pos1bk;
    const int pos1ob = ptr.pos1ob;
    const int posfob = ptr.posfob;
    const int posffc = ptr.posffc;

    int last = posfob;
    if (tstfct && ext.nfcst > 0) last = posffc;

    // No non-positive value anywhere in the padded span: nothing to repair.
    const bool prtmsg = !ispos(stc, pos1bk, posffc);
    if (!prtmsg) {
        if (tstfct) tstfct = false;
        return true;
    }

    const bool oktrn = ispos(stc, pos1ob, last);

    for (int i = pos1bk; i <= posffc; ++i) {
        if (stc[i - 1] <= 0.0) {
            // Walk outward to the nearest positive neighbour on each side; NOTSET
            // marks "ran off that end of the observed span".
            int i2 = 1;
            int before = 0;
            int after = 0;
            while (before == 0 || after == 0) {
                if (before == 0) {
                    const int i3 = i - i2;
                    if (i3 < pos1ob) before = prm::NOTSET;
                    else if (stc[i3 - 1] > 0.0) before = i3;
                }
                if (after == 0) {
                    const int i3 = i + i2;
                    if (i3 > posfob) after = prm::NOTSET;
                    else if (stc[i3 - 1] > 0.0) after = i3;
                }
                ++i2;
            }
            if (before == prm::NOTSET)
                stc[i - 1] = stc[after - 1];
            else if (after == prm::NOTSET)
                stc[i - 1] = stc[before - 1];
            else
                stc[i - 1] = (stc[after - 1] + stc[before - 1]) / 2.0;
        }
    }
    return oktrn;
}

// x11pt1.f -- prior adjustments (holiday / prior factors / prior trading day) and
// setup of the X-11 working buffers before the B/C/D decomposition.
void x11pt1(X13Context& ctx, bool lmodel, bool /*lgraf*/, bool /*lgrfxr*/) {
    x11opt_cmn& opt = ctx.x11opt;
    x11ptr_cmn& ptr = ctx.x11ptr;
    inpt_cmn& in = ctx.inpt;
    orisrs_cmn& os = ctx.orisrs;
    x11fac_cmn& fac = ctx.x11fac;

    const int pos1bk = ptr.pos1bk;
    const int pos1ob = ptr.pos1ob;
    const int posfob = ptr.posfob;
    const int posffc = ptr.posffc;
    const int ny = opt.ny;

    // logadd is treated as multiplicative for the prior-adjustment stage.
    if (opt.muladd == 2) opt.muladd = 0;
    const int muladd = opt.muladd;

    // Part A.
    opt.kpart = 1;

    // Missing-value indicators over the observed span.
    bool mvind[PLEN];
    setlg(false, PLEN, mvind);
    if (ctx.missng.missng) {
        for (int i = pos1ob; i <= posfob; ++i)
            if (dpeq(in.series(i), ctx.missng.mvval)) mvind[i - 1] = true;
    }

    // Set Sto and Stcsi equal to the input series (with the forecast-drop span).
    ctx.mdldat.nspobs = ctx.extend.nofpob - ctx.extend.nfdrp;
    const int nspobs = ctx.mdldat.nspobs;
    copy(in.series.data() + (pos1ob - 1), nspobs, -1,
         os.stcsi.data() + (pos1ob - 1));
    copy(in.series.data(), posfob, -1, os.stoap.data());
    copy(in.series.data(), posfob, -1, os.stopp.data());
    copy(in.series.data(), posfob, -1, os.stocal.data());
    copy(in.orig.data() + (pos1ob - 1), ctx.arima.nomnfy, -1,
         os.sto.data() + (pos1ob - 1));

    int lastpr = ctx.extend.nofpob;
    if (pos1ob > 1) lastpr = lastpr + pos1ob - 1;

    // (deferred: A1 unadjusted-original print/save + the Cnstnt subtract/add
    // bracket, which is a net no-op on Series once its two prints are dropped.)

    // Test for prior adjustment: divide/subtract by the prior-adjustment series.
    if (ctx.prior.kfmt >= 1) {
        // (deferred: prtadj prior-factor print.)
        divsub(os.sto.data(), os.sto.data(), in.sprior.data(), pos1ob, lastpr,
               muladd);
        if (ctx.missng.missng) {
            setmv(os.sto.data(), mvind, ctx.missng.mvval, pos1ob, posfob);
            setmv(os.stoap.data(), mvind, ctx.missng.mvval, pos1ob, posfob);
            setmv(os.stopp.data(), mvind, ctx.missng.mvval, pos1ob, posfob);
        }
    }

    // Prior calendar adjustment (X-11 regression / X-11 Easter or user holiday).
    int phol = posffc;
    if (posfob == posffc) phol = posfob + ny;
    // Ixreg==3 main run: the OLS prior-TD factor was estimated by xrgdrv's
    // transparent pass and stashed over the forecast-extended span [Pos1ob,Posffc]
    // (this main run's x11int wiped /x11fac/), so restore Faccal before the divide
    // below folds the observed part out of Sto and x11pt3 folds the whole span
    // (incl. the forecast region c16.A) into D11/D16. Empty on every other path.
    if (ctx.hiddn.ixreg == 3 && ctx.x11log.axrgtd &&
        !ctx.x11_faccal_prior.empty()) {
        const int nfp = static_cast<int>(ctx.x11_faccal_prior.size());
        for (int k = 0; k < nfp; ++k)
            fac.faccal(pos1ob + k) = ctx.x11_faccal_prior[static_cast<std::size_t>(k)];
    }
    if (((ctx.x11log.axrghl || ctx.x11log.axrgtd) && ctx.hiddn.ixreg == 3) ||
        opt.khol > 1) {
        if (opt.khol > 1)
            addmul(fac.faccal.data(), fac.faccal.data(), fac.x11hol.data(),
                   pos1bk, phol, muladd);
        divsub(os.sto.data(), os.sto.data(), fac.faccal.data(), pos1ob, posfob,
               muladd);
        if (ctx.missng.missng)
            setmv(os.sto.data(), mvind, ctx.missng.mvval, pos1ob, posfob);
    }

    // Prior trading-day adjustment. Two ported producers:
    //   Kswv==1  -- user prior weights (x11regression tdprior): pritd builds the
    //               a4 factor here and folds it into Faccal.
    //   Ixreg>=2 & Axrgtd -- OLS-estimated prior TD (xrgdrv): the factor is built
    //               by xrgdrv's transparent x11pt2 into Faccal and folded via the
    //               Ixreg==3 divide above, so NOTHING is generated here for it.
    // NB `muladd` is already past x11pt1.f:52's Muladd 2->0 collapse, so
    // log-additive reaches here as 0 and takes the same divide as multiplicative
    // (gated by *_x11regression-tdprior-logadd; that fall-through used to be a
    // silent wrong-numbers bug -- see run_pre_model.cpp). Additive and
    // pseudo-additive weights never get this far: editor.f:1518-1530 rejects
    // them at parse, ported in gtinpt.cpp, so the muladd/psuadd arms below are
    // belt-and-braces.
    //
    // x11pt1.f:229-230's entry condition is
    //     Kswv==1 .and. (((Axrghl.or.Axrgtd).and.Ixreg==3) .or. Khol<2)
    // -- so with the classic X-11 Easter on (Khol>=2) and no x11-regression
    // prior calendar, the oracle SKIPS the whole prior-TD block and adjusts
    // without a prior TD. This guard used to FATAL there instead, on a spec the
    // oracle runs to completion: `x11{x11easter=yes}` + `x11regression{
    // tdprior=}` writes d10/d11 in the oracle and returned OUTCOME: FATAL here
    // (measured). Now the skip is reproduced and only the genuinely unported
    // arm -- entering WITH Khol>=2 via the x11-regression branch, which needs
    // the classic-Easter user-weight combine -- keeps the wall.
    const bool xrg_prior =
        (ctx.x11log.axrghl || ctx.x11log.axrgtd) && ctx.hiddn.ixreg == 3;
    if (opt.kswv == 1 && (xrg_prior || opt.khol < 2)) {
        if (muladd != 0 || ctx.x11msc.psuadd || opt.khol >= 2) {
            x11_not_ported(ctx, "x11pt1 prior trading-day adjustment (pritd/ssrit)");
            return;
        }
        // pritd.f + fold (x11pt1.f:229-273). (deferred: A3/A3P/A4 prints/punch.)
        int n2 = ctx.extend.nbfpob;
        if (ctx.extend.nfcst == 0) n2 += ny;
        double stptd[PLEN];
        setdp(0.0, PLEN, stptd);
        // editor.f:2240 tdset -> Xn/Xnstar over the factor span, keyed on the date
        // at Pos1bk (= Begspn; the oracle's Begbak/Begbk2 backcast-origin dates are
        // not populated on this port path -- for the ported no-backcast prior-TD
        // case Pos1bk==Pos1ob, so the span origin is Begspn). pritd's begdat is the
        // date at absolute position 1 (Begbk2), = Begspn shifted back Pos1bk-1.
        int begd1[2];
        addate(ctx.mdldat.begspn.data(), ny, -(pos1bk - 1), begd1);
        tdset_td(ctx, ctx.mdldat.begspn.data(), pos1bk, posffc, ny);
        pritd(ctx, stptd, n2, ny, begd1, pos1bk);
        if (ctx.error.lfatal) return;
        // Stash the prior-TD factor (A4) over the observed span for the harness
        // dump / result object (bit-exact vs the oracle a4 table).
        ctx.x11_a4_prior.assign(stptd + (pos1ob - 1),
                                stptd + (pos1ob - 1) + (posfob - pos1ob + 1));
        // Sliding-spans capture of the prior-TD factor (Issap==2, Ixreg!=2).
        if (ctx.hiddn.issap == 2 && ctx.hiddn.ixreg != 2)
            ssrit(ctx, stptd, pos1ob, posfob, 1, in.series.data());
        // Divide the prior-adjusted series by the prior-TD factors; fold into
        // the combined calendar factor Faccal (Kswv==1).
        double stdbuf[PLEN];
        copy(stptd + (pos1ob - 1), ctx.extend.nbfpob, -1, stdbuf + (pos1ob - 1));
        divsub(os.sto.data(), os.sto.data(), stdbuf, pos1ob, lastpr, muladd);
        addmul(fac.faccal.data(), fac.faccal.data(), stptd, pos1bk, phol, muladd);
        if (ctx.missng.missng)
            setmv(os.sto.data(), mvind, ctx.missng.mvval, pos1ob, lastpr);
    } else if ((opt.kswv != 0 || (ctx.hiddn.ixreg >= 2 && ctx.x11log.axrgtd)) &&
               (muladd != 0 || ctx.x11msc.psuadd)) {
        // OLS prior-TD (Ixreg>=2) additive / pseudo-additive weights unported.
        x11_not_ported(ctx, "x11pt1 additive/pseudo-additive prior trading-day");
        return;
    }

    // (deferred: lmodel pre-ARIMA prior-adjusted-series prints A3/A3P/A4D.)
    (void)lmodel;

    // Set Stcsi equal to the prior-adjusted series.
    copy(os.sto.data() + (pos1ob - 1), posfob - pos1ob + 1, -1,
         os.stcsi.data() + (pos1ob - 1));

    // No-model B1 snapshot: when an X-11 Easter prior is present, B1 is the
    // prior-adjusted Stcsi (not the raw series). x11pt2 overwrites Stcsi during
    // the C/D passes, so stash it in Stoap (dead after x11pt1 on the no-model
    // path) for the b1 save-table dump.
    if (!lmodel && opt.khol > 1)
        copy(os.stcsi.data() + (pos1ob - 1), posfob - pos1ob + 1, -1,
             os.stoap.data() + (pos1ob - 1));
}

// x11pt2.f -- X-11 PARTS B1->D7: the iterated B/C/D moving-average decomposition.
// Consumes the prior-adjusted / forecast-extended B1 input (Stcsi, from x11pt1)
// and runs three passes (Kpart = 2/B, 3/C, 4/D) of the classic X-11 kernel,
// returning at D7 (Kpart==4) -- x11pt3 then produces D8..D16. Each pass has a
// Section-1 half (centered Ny-term MA trend -> SI ratios -> si/vsfb seasonal ->
// preliminary SA) and a Section-2 half (variable Henderson trend vtc -> chktrn
// trend-positivity -> si/vsfa seasonal -> irregular -> xtrm extreme component ->
// SI reweight). Base-path only: the l.74-351 model-based prior-adjustment / factor
// preamble is deferred print/save plus Faccal combines gated by inactive flags;
// makadj/tdlom/ssrit and the x11-regression option stay not_ported. All table/
// punch/x11plt/ftest output is dropped (deferred-print convention).
void x11pt2(X13Context& ctx, bool lmodel, bool lx11, bool lseats,
            bool /*lgraf*/, bool /*lgrfxr*/) {
    x11opt_cmn& opt = ctx.x11opt;
    x11ptr_cmn& ptr = ctx.x11ptr;
    orisrs_cmn& os = ctx.orisrs;
    x11srs_cmn& srs = ctx.x11srs;
    xtrm_cmn& xt = ctx.xtrm;
    const extend_cmn& ext = ctx.extend;
    (void)lseats;

    const int pos1bk = ptr.pos1bk;
    const int pos1ob = ptr.pos1ob;
    const int posfob = ptr.posfob;
    const int posffc = ptr.posffc;
    const int ny = opt.ny;
    const int ny2 = ny / 2;
    const int nfcst = ext.nfcst;
    const int nbcst = ext.nbcst;
    const bool noxfct = ctx.x11msc.noxfct;
    const bool psuadd = ctx.x11msc.psuadd;

    opt.length = posfob - pos1ob + 1;
    const int kersa1 = xt.kersa;
    const int ksdev1 = xt.ksdev;
    const bool goodlm = dpeq(ctx.arima.lam, 0.0) || dpeq(ctx.arima.lam, 1.0);

    double* stcsi = os.stcsi.data();
    double* sto = os.sto.data();
    double* stc = srs.stc.data();
    double* stsi = srs.stsi.data();
    double* sts = srs.sts.data();
    double* stci = srs.stci.data();
    double* sti = srs.sti.data();
    double* stwt = xt.stwt.data();

    auto STCSI = [&](int i) -> double& { return stcsi[i - 1]; };
    auto STO = [&](int i) -> double& { return sto[i - 1]; };
    auto STC = [&](int i) -> double& { return stc[i - 1]; };
    auto STS = [&](int i) -> double& { return sts[i - 1]; };
    auto STCI = [&](int i) -> double& { return stci[i - 1]; };
    auto STI = [&](int i) -> double& { return sti[i - 1]; };
    auto STWT = [&](int i) -> double& { return stwt[i - 1]; };

    double temp[PLEN];  // COMMON /work/ Temp  -- vsfb scratch
    // COMMON /mq10/ Stex -- per-iteration extreme component. ctx-persistent
    // (ctx.mq10_stex): x11pt3's D8/D9 stage reads whatever x11pt2's LAST pass
    // (Part D) leaves here; see the ctx.mq10_stex declaration for why this
    // must be a real ctx member, not a function-local array (a single main
    // run happened to never exercise the consumer path, but a sliding-spans
    // sub-span replay does).
    double* stex = ctx.mq10_stex.data();
    auto STEX = [&](int i) -> double& { return stex[i - 1]; };

    // --- Model-based prior-adjustment / factor preamble (l.74-351) ---
    // (deferred: .xdg prioradj savelog.)
    //
    // makadj/tdlom (x11pt2.f:115-129): when a model trading-day effect coexists
    // with a length-of-month/leap-year PRIOR adjustment (Priadj>1, set by the
    // aictest td1coef selection), fold the prior factor (Sprior, populated from
    // ctx.adj by x11int) into the model TD factor (Factd) and remove it from the
    // calendar-adjusted series (Stocal); then neutralise Sprior and negate Priadj
    // so it is not removed again downstream. B1 (Stcsi) is untouched here -- the
    // leap-year effect was already divided out of the series pre-estimation.
    if (ctx.hiddn.ixreg != 2 && ctx.prior.priadj > 1 && goodlm) {
        x11adj_cmn& adj = ctx.x11adj;
        const adj_cmn& adjc = ctx.adj;
        const priusr_cmn& pu = ctx.priusr;
        const int muladd = opt.muladd;
        const int n2 = (nfcst < ny) ? posfob + ny : posffc;
        // makadj.f: build the temporary prior-adjustment Adjtmp. With user prior
        // factors, copy Usrtad (+ Usrpad) into Adjtmp(Setpri); otherwise it is the
        // mode identity. Pre-fill base so positions the user span does not cover
        // are deterministic (the oracle leaves them as stack garbage but only reads
        // the written [Setpri,+Nadj) range).
        double adjtmp[PLEN];
        setdp(adjc.adjmod == 2 ? 0.0 : 1.0, PLEN, adjtmp);
        if (pu.nustad > 0 || pu.nuspad > 0) {
            double* atmp = adjtmp + (adjc.setpri - 1);
            if (pu.nustad > 0) {
                copy(&ctx.priadj.usrtad.data()[pu.frstat - 1], adjc.nadj, 1, atmp);
                if (pu.nuspad > 0)
                    addmul(atmp, &ctx.priadj.usrpad.data()[pu.frstap - 1], atmp, 1,
                           adjc.nadj, muladd);
            } else {
                copy(&ctx.priadj.usrpad.data()[pu.frstap - 1], adjc.nadj, 1, atmp);
            }
            if (muladd != 1 && adjc.adjmod == 0)
                invfcn(ctx, atmp, adjc.nadj, 1, 0.0, atmp);
        }
        double* sprior = ctx.inpt.sprior.data();
        double* factd = ctx.x11fac.factd.data();
        double* stocal = os.stocal.data();
        if (adj.nflwtd > 0) {
            // tdlom.f (Adjtd==1): combine LOM/leap-year prior into the model TD
            // factor and strip it from Stocal; reset Sprior to Adjtmp.
            if (adj.adjtd == 1) {
                addmul(factd, factd, sprior, pos1bk, n2, muladd);
                divsub(factd, factd, adjtmp, pos1bk, n2, muladd);
                divsub(stocal, stocal, sprior, pos1bk, n2, muladd);
                addmul(stocal, stocal, adjtmp, pos1bk, n2, muladd);
                copy(adjtmp + (adjc.setpri - 1), adjc.nadj, 1,
                     sprior + (adjc.setpri - 1));
                ctx.prior.priadj = -ctx.prior.priadj;
            } else {
                // tdlom.f:44-59, the Adjtd != 1 branch. REACHED BY
                // `regression{noapply=(td)}`, which sets Adjtd = -1 and leaves
                // it there (chkadj.f:157-158 only move Adjtd between 0 and 1).
                //
                // This block carried a written-out proof that the branch was
                // unreachable, and the proof was sound for every route it
                // considered. It could not consider this one: `noapply=` was
                // in the ARGDIC with no case in the reader, so the argument was
                // consumed and thrown away and no spec could set Adjtd < 0.
                // GENERALIZABLE: a reachability argument is only valid over the
                // options the PARSER actually honours -- a dropped option
                // silently removes edges from the graph you are reasoning about,
                // and the analysis then proves something narrower than it looks.
                // The four routes it did rule out are kept below; they remain
                // correct, and none of them is this one.
                //
                // The arithmetic: put the prior back into BOTH the original and
                // the calendar-adjusted series, replace Sprior with the user
                // prior alone (Adjtmp, i.e. the LOM/leap part removed), then
                // take that back out of both. Net effect is to strip the
                // length-of-month factor while leaving any user prior applied.
                // Priadj goes to 0 here, NOT to -Priadj as on the Adjtd==1 arm
                // above -- the ssprep/restor pair keys on the sign, so the two
                // arms leave genuinely different state behind.
                addmul(stcsi, stcsi, sprior, pos1bk, n2, muladd);
                addmul(stocal, stocal, sprior, pos1bk, n2, muladd);
                copy(adjtmp + (adjc.setpri - 1), adjc.nadj, 1,
                     sprior + (adjc.setpri - 1));
                divsub(stcsi, stcsi, sprior, pos1bk, n2, muladd);
                divsub(stocal, stocal, sprior, pos1bk, n2, muladd);
                ctx.prior.priadj = 0;
            }
            // The pre-existing reachability analysis, retained as prose: these
            // four routes to Adjtd != 1 with Nflwtd>0 and Priadj>1 really are
            // closed by the oracle, and re-deriving them is not free.
            //   * chkadj.f:209 (a transform that is neither log nor identity)
            //     -- the automatic lom/leap prior that makes Priadj>1 exists
            //     only under `td`+log, and an EXPLICIT transform{adjust=lom|
            //     lpyear} alongside regression{variables=(td)} is rejected
            //     outright ("Cannot include a length-of-month type variable as
            //     both a regression variable and a prior adjustment");
            //   * editor.f:2277 (.not.Lmodel) -- regression{} with no arima{}/
            //     automdl{} is rejected outright;
            //   * x11ari.f:110 (a constant series) -- the oracle refuses the
            //     run and writes no tables, so there is nothing to gate;
            //   * xrgdrv.f:80 -- runs with Ixreg==2, which the tdlom call site
            //     above already excludes.
        } else {
            // Nflwtd==0 (x11pt2.f:125-128): strip LOM from Stocal directly.
            divsub(stocal, stocal, sprior, pos1bk, n2, muladd);
            addmul(stocal, stocal, adjtmp, pos1bk, n2, muladd);
        }
    }
    // Store regression trading-day factors for sliding-spans analysis
    // (x11pt2.f:131-146). No RETURN here in the oracle -- falls through to
    // the adjustment-factor combine below regardless.
    if (ctx.hiddn.issap == 2) {
        ssap_cmn& ssa = ctx.ssap;
        if (ssa.itd == 1) {
            if (ctx.x11adj.adjtd == 1) {
                ssrit(ctx, ctx.x11fac.factd.data(), pos1ob, posfob, 1,
                      ctx.inpt.series.data());
            } else {
                ssa.itd = 0;
                if (ctx.x11log.axrgtd) ssa.itd = 1;
            }
        }
        if (!(ctx.x11adj.adjhol == 1 || ctx.x11adj.finhol) &&
            (ssa.ihol == 1 && opt.khol == 0))
            ssa.ihol = 0;
    }
    // --- Model-based adjustment-factor combine (x11pt2.f:158-336). Fold the
    // regARIMA trading-day + holiday factors into the combined calendar factor
    // Faccal (which x11pt3 divides out of D11 and folds into the D16 total
    // factors). Factd/Fachol were inverse-transformed and stored in ctx.x11fac by
    // adjreg; Faccal was identity-initialized in x11int. The factor tables/emits
    // (D16/D18/A18 + the outlier/user/seasonal factor prints) are deferred output;
    // the only compute here is the Faccal combine. The still-unported activations
    // (outlier ao/ls/tc/so, user, regARIMA-seasonal, cycle, Khol>=2 x11-Easter,
    // x11regression Axrg* calendar) fatal cleanly. ---
    {
        x11adj_cmn& adj = ctx.x11adj;
        const x11log_cmn& xl = ctx.x11log;
        x11fac_cmn& fac = ctx.x11fac;
        const int muladd = opt.muladd;
        // n2: combine end pointer (x11pt2.f:110-114).
        const int n2 = (nfcst < ny) ? posfob + ny : posffc;

        // Outlier factors (Facao/Facls/Factc/Facso) do NOT fold into Faccal on
        // the base (non-x11reg) path: x11pt2.f:187-352 only builds the deferred
        // A8/A18/A19 factor tables from them. The AO/LS/TC removal from the SA
        // series happens upstream (adjreg -> B1) and is restored/finalized in
        // x11pt3 (the D11 Fin* / D13 Adj* folds). So adjao/adjls/adjtc/adjso/fin*
        // pass through here. Still-unported activations (user regression,
        // regARIMA-seasonal, transitory cycle, x11-Easter Khol>=2, x11regression
        // calendar) genuinely fatal.
        // x11regression TD (xl.axrgtd, Ixreg==1) is handled IN the B/C iteration
        // by x11mdl_td (below), not in this setup combine: at setup Factd is not
        // yet built and adjtd==0 (the TD is X-11-regression, not a model factor),
        // so the adjtd fold at :358 is skipped and Faccal passes through until
        // x11mdl_td overwrites it. Holiday (axrghl) is still unported.
        // Khol>=2 (x11-Easter prior) is NOT fatal here: the Easter factor was
        // already folded X11hol -> Faccal in Part A, and this block's khol==2
        // work (Fachol += X11hol at x11pt2.f:309, the Stocal A18 print) feeds
        // only deferred factor tables (A16/A18), not D10-D13.
        // Adjusr/Finusr (user-regression factor) are also NOT fatal here: like
        // the AO/LS/TC factors they only build deferred A8/A18/A19 tables in
        // x11pt2; the user effect is removed at adjreg (-> B1) and restored/
        // finalized by the x11pt3 D11 Finusr / D13 Adjusr folds.
        // Axrgtd is handled for Ixreg==1 (in-line x11mdl_td below), Ixreg==2
        // (xrgdrv transparent pass -- x11mdl_td builds Faccal here) and Ixreg==3
        // (main run -- the prior Faccal passes through, adjtd==0 so the :358 fold
        // is skipped), so none of those fatal. Ixreg==0 with Axrgtd cannot occur.
        // Adjso/Adjsea were in this fatal list unconditionally, but on the BASE
        // (non-x11regression) path they are print-only here, exactly like
        // Adjao/Adjls/Adjtc/Adjusr above: Facso reaches only the deferred A8
        // accumulator (x11pt2.f:204-207 dtemp -> LRGOTL table/punch) and Facsea
        // only the deferred A10 table (x11pt2.f:273). Their real arithmetic is
        // the x11pt3 combine, which is ported and gated. The ONE place they are
        // numeric in x11pt2 is the x11regression feedback rebuild at
        // x11pt2.f:851-859, which re-applies the outlier/user/seasonal priors to
        // a Stcsi rebuilt from the raw Series -- and the C++ takes the
        // `STCSI = STO` shortcut there, which is only bit-equivalent when no such
        // factor exists. So keep them fatal on that path alone, not everywhere.
        const bool xreg_feedback =
            (ctx.hiddn.ixreg == 1 || ctx.hiddn.ixreg == 2) && xl.axrgtd;
        if (adj.adjcyc == 1 || xl.axrghl ||
            (xreg_feedback && (adj.adjso == 1 || adj.adjsea == 1 ||
                               adj.adjusr == 1)) ||
            (xl.axrgtd && ctx.hiddn.ixreg != 1 && ctx.hiddn.ixreg != 2 &&
             ctx.hiddn.ixreg != 3)) {
            x11_not_ported(ctx, "x11pt2 user/seasonal/cycle/x11reg factor combine+emit");
            return;
        }
        // Trading day (x11pt2.f:158-171).
        if (adj.adjtd == 1 && goodlm)
            addmul(fac.faccal.data(), fac.faccal.data(), fac.factd.data(), pos1bk,
                   n2, muladd);
        // Holiday (x11pt2.f:173-186); Khol!=1, in a model, not x11-reg holiday.
        if ((adj.adjhol == 1 || (adj.finhol && adj.nhol > 0)) && opt.khol != 1 &&
            goodlm && lmodel && !xl.axrghl)
            addmul(fac.faccal.data(), fac.faccal.data(), fac.fachol.data(), pos1bk,
                   n2, muladd);
        // Combined holiday effect (x11pt2.f:294-311): fold the X-11 Easter factor
        // into Fachol so x11pt3's divsub(Faccal,Faccal,Fachol) removes it from the
        // combined calendar factor Faccal (affects D16/D18, not D10-D13). lsthol
        // per x11pt2.f:301-305. The Facxhl/Axrghl irregular-reg branch is unported.
        if (!ctx.xrgum.noxfac && opt.khol == 2 && goodlm) {
            const int lsthol = (nfcst == 0) ? posfob + ny : posffc;
            addmul(fac.fachol.data(), fac.fachol.data(), fac.x11hol.data(), pos1bk,
                   lsthol, muladd);
        }
    }

    // --- PART B ---
    opt.kpart = 2;
    opt.ksect = 1;

    // (deferred: B1 table/punch/x11plt.)
    if (!lx11) return;  // X-11 seasonal-adjustment options not requested.

    // Log-transform B1 for the logadd model, then set Sto = B1.
    opt.muladd = opt.tmpma;
    const int muladd = opt.muladd;
    if (muladd == 2) logar(stcsi, pos1bk, posffc);
    copy(stcsi + (pos1bk - 1), posffc - pos1bk + 1, 1, sto + (pos1bk - 1));
    // (Kswv==0 on the base path: the prior-TD Series*Stptd rescale is skipped.)

    // --- B/C/D iteration (Fortran DO WHILE(T); Kpart 2->3->4, RETURN at D7). ---
    for (;;) {
        const int kpart = opt.kpart;

        // Section 1: centered Ny-term MA -> trend; SI ratios.
        opt.ksect = 1;
        averag(stcsi, stc, pos1bk, posffc, 2, ny);
        const int mfda = pos1bk + ny2;
        const int mfd1 = pos1ob + ny2;
        const int mlda = posffc - ny2;
        const int klda = posfob - ny2;
        divsub(stsi, stcsi, stc, mfda, mlda, muladd);
        // x11pt2.f:436-437 -- the B1 stable-seasonality F-test (savelog
        // f2.fsb1). Only the B pass's section-1 SI ratios feed it.
        if (opt.ksect == 1 && kpart == 2 && ctx.hiddn.ixreg != 2 && opt.khol != 1)
            ftest(ctx, stsi, mfd1, klda, ny, 2);
        // (deferred: B2/C2/D2 trend table.)

        if (kpart == 2) {
            // Part B: replace extreme SI ratios (si drives vsfb/xtrm/replac).
            int pos1ex = mfda, posfex = mlda;
            if (noxfct) { pos1ex = mfd1; posfex = klda; }
            si(ctx, opt.ksect, mfda, mlda, ny, nfcst, nbcst, kersa1, ksdev1,
               pos1ob, posfob, opt.kfulsm, pos1ex, posfex);
        } else if (opt.kfulsm < 2) {
            vsfb(sts, stsi, mfda, mlda, ny, opt.lterm, opt.lter.data(), opt.ksect,
                 ctx.x11msc.shrtsf, temp, muladd, &opt.mtype);
            // (deferred: C4/D4 modified-SI table.)
        }

        // Fill the Ny/2 ends lost to the centered MA, then preliminary SA.
        forcst(sts, mfda, mlda, posffc, ny, 1, 0.0, 1.0);
        if (psuadd) {
            for (int i = pos1bk; i <= posffc; ++i) {
                if (i < mfda || i > mlda)
                    STCI(i) = STCSI(i) / STS(i);  // (pseudo-add zero-SF check dropped)
                else
                    STCI(i) = STCSI(i) - STC(i) * (STS(i) - 1.0);
            }
        } else {
            divsub(stci, stcsi, sts, pos1bk, posffc, muladd);
        }
        // (deferred: B5/C5/D5 seasonal, B6/C6/D6 SA tables.)

        // Section 2: variable trend-cycle; multiplicative trend-positivity check.
        opt.ksect = 2;
        vtc(ctx, stc, stci);
        if (muladd == 0) {
            bool chkfct = false;
            chktrn(ctx, stc, chkfct);  // oktrn only gated a deferred D7 print.
    }
    // x11pt2.f:610-614 -- the D7 Henderson length, reported only on the D pass
    // (Kpart==4) of a run that let the program choose it (Ktcopt==0).
    if (opt.ktcopt == 0 && kpart == 4) ctx.x11_d7trendma = opt.nterm;
    {
        }
        // (deferred: d7trendma savelog; B7/C7/D7 trend table.)

        divsub(stsi, stcsi, stc, pos1bk, posffc, muladd);
        if (kpart == 2) {
            int pos1ex = pos1bk, posfex = posffc;
            if (noxfct) { pos1ex = pos1ob; posfex = posfob; }
            si(ctx, opt.ksect, pos1bk, posffc, ny, nfcst, nbcst, kersa1, ksdev1,
               pos1ob, posfob, opt.kfulsm, pos1ex, posfex);
        } else if (kpart == 4) {
            return;  // D7 trend finalized; x11pt3 continues with D8..D16.
        } else if (opt.kfulsm < 2) {
            vsfa(stsi, pos1bk, posfob, ny, muladd, psuadd, opt.rati.data(),
                 opt.ratis);
            vsfb(sts, stsi, pos1bk, posffc, ny, opt.lterm, opt.lter.data(),
                 opt.ksect, ctx.x11msc.shrtsf, temp, muladd, &opt.mtype);
            // (deferred: C9 modified-SI table.)
        }

        // Preliminary seasonally adjusted series from the prior-adjusted Sto.
        if (opt.kfulsm == 2) {
            copy(sto, posffc, 1, stci);
        } else if (psuadd) {
            for (int i = pos1bk; i <= posffc; ++i)
                STCI(i) = STO(i) - STC(i) * (STS(i) - 1.0);
        } else {
            divsub(stci, sto, sts, pos1bk, posffc, muladd);
        }
        // Preliminary irregular.
        divsub(sti, stci, stc, pos1bk, posffc, muladd);
        // (deferred: B10/C10 seasonal, B11/C11 SA, B13/C13 irregular tables.)

        // X-11 regression on the irregular (x11pt2.f:711 -> x11mdl), Ixreg==1
        // (in-line) or Ixreg==2 (xrgdrv transparent pass): regress the TD design
        // on Sti (B13/C13), snapshot b16/c16, and divide the TD effect out of Sti
        // so the iteration continues without it. Ixreg==3 (main run after xrgdrv)
        // does NOT re-estimate -- the TD was already removed as a prior.
        if ((ctx.hiddn.ixreg == 1 || ctx.hiddn.ixreg == 2) &&
            (kpart == 2 || kpart == 3)) {
            // x11pt2.f:720/724: swap the x11reg regressors into the working model
            // for the irregular OLS, then save the estimated betas back.
            loadxr(ctx, /*toxreg=*/false);
            x11mdl_td(ctx, kpart);
            loadxr(ctx, /*toxreg=*/true);
            if (ctx.error.lfatal) return;
        }

        // Bundesbank outlier test (B iteration): auto-select the sigma limits.
        if (kpart == 2 && xt.ksdev < 4) {
            int iv = 0;
            vtest(sti, iv, pos1bk, posfob, ny, muladd);
            entsch(kersa1, ksdev1, xt.kersa, xt.ksdev, iv);
        }

        // Irregular-component extreme-value weights.
        {
            int pos1ex = pos1bk, posfex = posffc;
            if (noxfct) { pos1ex = pos1ob; posfex = posfob; }
            xtrm(sti, pos1bk, posffc, pos1ex, posfex, ny, muladd, xt.ksdev,
                 opt.imad, opt.sigmu, opt.sigml, ctx.lzero.lsp, stwt,
                 xt.stdper.data(), xt.stdev.data(), xt.csigvc.data());
        }
        // (deferred: B17/C17 weights table; calendarsigma savelog; combined TD/cal.)
        // (Ixreg==0: no x11-regression prior-adjustment early return.)

        // Extreme component (multiplicative on the base path).
        if (muladd > 0) {
            for (int i = pos1bk; i <= posffc; ++i)
                STEX(i) = STI(i) * (1.0 - STWT(i));
        } else {
            for (int i = pos1bk; i <= posffc; ++i)
                STEX(i) = STI(i) / (1.0 + STWT(i) * (STI(i) - 1.0));
        }
        // Stcsi for the next iteration. The oracle x11regression feedback
        // (x11pt2.f:846-894, Axrgtd/Ixreg==1) rebuilds Stcsi from the raw forecast-
        // extended Series, re-applies the outlier/user/Sprior priors, then divides
        // out the combined calendar factors. On the TD-only corpus path that is
        // exactly Sto/Faccal -- Sto is already Orig/Sprior (x11pt1) and there are
        // no outlier/user factors -- so the STCSI=STO shortcut is bit-equivalent
        // here (verified: rebuilding from Series gave identical results). Outlier/
        // user x11reg specs would need the full :851-859 prior divsubs.
        for (int i = pos1bk; i <= posffc; ++i) STCSI(i) = STO(i);
        if ((ctx.hiddn.ixreg == 1 || ctx.hiddn.ixreg == 2) && ctx.x11log.axrgtd)
            divsub(stcsi, stcsi, ctx.x11fac.faccal.data(), pos1bk, posffc, muladd);

        // Modify the (calendar-adjusted) original to remove the extremes.
        if (psuadd) {
            for (int i = pos1bk; i <= posffc; ++i) {
                if (opt.kfulsm == 2)
                    STCSI(i) = STC(i) * (STI(i) / STEX(i));
                else
                    STCSI(i) = STC(i) * (STS(i) + (STI(i) / STEX(i) - 1.0));
            }
        } else {
            divsub(stcsi, stcsi, stex, pos1bk, posffc, muladd);
        }
        // (deferred: B20/C20 extreme-value table.)

        // Advance to the next iteration (C, then D).
        opt.kpart = kpart + 1;
        opt.ksect = 1;
        // (deferred: C1/D1 modified-original table.)
    }
}

// x11pt3.f -- X-11 PARTS D8->D16: the finals. Consumes the D7 trend/seasonal
// left by x11pt2 and lands the final seasonal (D10=Sts), final SA (D11=Stci),
// final trend (D12=Stc), final irregular (D13=Sti), and combined factors
// (D16=ststd) in the ctx table arrays; also builds the unmodified/modified SI
// (D8=Stsie / D9=Temp) and the Part-E modified series (E1=Stome / E3=Stime /
// E2=Stcime). Base decomposition path only (see the header) -- gated-off feature
// branches fatal cleanly via x11_not_ported keyed to their activating flag, the
// D8 F/M diagnostics and the residual-seasonality ftest are deferred no-ops, and
// every table/punch/x11plt/prttrn/prtd8b/prtd9a/writln call is dropped.
//
// The transient COMMON scratch /work/ Temp, /work3/ Stsie, /mq10/ Stex, /mq5a/
// Stime, /kcser/ Ckhs are function-local PLEN buffers here (as in x11pt2). Stex
// in particular carries the last-iteration extreme component out of x11pt2's
// /mq10/; the driver wiring x11pt2 -> x11pt3 must persist it (D8/D9 depend on
// it). The finals D10-D13/D16 do not.
void x11pt3(X13Context& ctx, bool /*lgraf*/, bool lttc) {
    x11opt_cmn& opt = ctx.x11opt;
    x11ptr_cmn& ptr = ctx.x11ptr;
    orisrs_cmn& os = ctx.orisrs;
    x11srs_cmn& srs = ctx.x11srs;
    xtrm_cmn& xt = ctx.xtrm;
    extend_cmn& ext = ctx.extend;
    inpt_cmn& in = ctx.inpt;
    adxser_cmn& ax = ctx.adxser;
    const x11adj_cmn& adj = ctx.x11adj;
    const adj_cmn& adjc = ctx.adj;       // Cnstnt
    const prior_cmn& pri = ctx.prior;    // Kfmt, Priadj, Lprntr
    const priusr_cmn& pu = ctx.priusr;   // Nustad, Nuspad
    const hiddn_cmn& hid = ctx.hiddn;    // Lhiddn, Issap, Irev, Ixreg
    const force_cmn& frc = ctx.force;    // Iyrt, Lrndsa
    const x11log_cmn& xl = ctx.x11log;   // Axrgtd, Axrghl

    const int pos1bk = ptr.pos1bk;
    const int pos1ob = ptr.pos1ob;
    const int posfob = ptr.posfob;
    const int posffc = ptr.posffc;
    const int ny = opt.ny;
    int muladd = opt.muladd;   // 0 mult / 1 add / 2 logadd; toggles 2->0 at the
                               // D10/D12 antilog steps (kept in sync with opt.muladd)
    const bool psuadd = ctx.x11msc.psuadd;

    double* sts = srs.sts.data();
    double* stsi = srs.stsi.data();
    double* stc = srs.stc.data();
    double* stc2 = srs.stc2.data();
    double* stci = srs.stci.data();
    double* sti = srs.sti.data();
    double* stcsi = os.stcsi.data();
    double* stwt = xt.stwt.data();
    double* series = in.series.data();
    double* stome = ax.stome.data();
    double* stcime = ax.stcime.data();
    double* stci2 = ax.stci2.data();

    // Transient COMMON scratch (function-local PLEN, as in x11pt2). Stsie is
    // ctx-persistent (ctx.work3_stsie) -- see its declaration in
    // x13context.hpp for why: x11pt3.f's Ksdev>1 branch resets Stsi from
    // Stsie starting at index 1 (not Pos1bk), which only stays well-defined
    // for a replayed sub-span (Pos1bk>1, slidingspans{}/history{}) if Stsie
    // genuinely persists across calls like the oracle's COMMON does.
    double temp[PLEN];    // /work/  Temp   (D9 replacement buffer)
    double* stsie = ctx.work3_stsie.data();  // /work3/ Stsie (D8 unmodified SI)
    // /mq10/ Stex -- ctx-persistent (ctx.mq10_stex), written by x11pt2's B/C/D
    // loop (see its declaration there); this is the fix, not just a rename --
    // the prior function-local Stex here was uninitialized on every call.
    double* stex = ctx.mq10_stex.data();
    double* stime = ctx.mq5a_stime.data();  // /mq5a/ Stime (E3 modified irregular),
                          // ctx-persistent so run_spectrum can read it (spcdrv
                          // differences E2/E3, computed here in Part E below).
    // /kcser/ Ckhs -- ctx-persistent (ctx.kcser_ckhs): a COMMON in the oracle
    // because agr3.f reads this SA snapshot after x11pt3 returns, to filter the
    // DIRECT trend the composite comparison statistics use. Dead otherwise.
    double* ckhs = ctx.kcser_ckhs.data();
    double ststd[PLEN];   // ststd          (D16 combined factors)
    double biasfc[PLEN];  // biasfc         (logadd trend bias-correction factors)
    double sp2[PLEN];     // sp2            (Sprior snapshot; dead on the base path)

    // Snapshot the prior factors (used only by the off-base Adj* Sprior restore).
    copy(in.sprior.data(), PLEN, 1, sp2);
    ext.nfcst = posffc - posfob;
    const int nfcst = ext.nfcst;

    // --- D8: MSR seasonal-filter (re)selection + unmodified SI ---
    if (opt.kfulsm < 2 && xt.ksdev <= 1)
        sfmsr(ctx, sts, stsi, pos1bk, posfob, posffc);
    addmul(stsie, stsi, stex, pos1bk, posffc, muladd);
    // (deferred: D8 table/punch.)

    // D8 analysis of variance on the unmodified SI ratios (x11pt3.f:111-130).
    // ftest/mstest are read-only on the series; kwtest SORTS Stsie in place,
    // which is why the oracle rebuilds it from Stsi/Stex in between.
    if (!hid.lhiddn && opt.khol != 1) {
        ftest(ctx, stsie, pos1ob, posfob, ny, 0);
        kwtest(ctx, stsie, pos1bk, posfob, ny);
        addmul(stsie, stsi, stex, pos1bk, posffc, muladd);  // rebuild post-kwtest
        mstest(ctx, stsie, pos1bk, posfob, ny);
        combft(ctx);
    } else if (hid.issap == 2) {
        // Sliding-spans replay: same battery, print suppressed (x11pt3.f:131-135).
        ftest(ctx, stsie, pos1ob, posfob, ny, 0);
        kwtest(ctx, stsie, pos1bk, posfob, ny);
        addmul(stsie, stsi, stex, pos1bk, posffc, muladd);
        mstest(ctx, stsie, pos1bk, posfob, ny);
        combft(ctx);
    }

    double ebar = 0.0;
    if (muladd == 0) ebar = 1.0;

    // Ksdev>1 (calendarsigma / override) extreme re-replacement -- ported leaves,
    // off the single-sigma base path.
    if (xt.ksdev > 1) {
        copy(stsie, posfob, 1, stsi);
        replac(stsi, temp, stwt, pos1bk, posfob, ny);
        if (opt.kfulsm < 2) sfmsr(ctx, sts, stsi, pos1bk, posfob, posffc);
    }
    // x11pt3.f:149-151 -- D8B's savelog block. Print surface is still
    // deferred; this is the `.udg` half, which is NOT print surface (its gate
    // includes `Lsumm>0.and.gudrun`).
    if (!hid.lhiddn) prtd8b_savelog(ctx, stwt, pos1ob, posfob);
    if (ctx.error.lfatal) return;

    // D9: identify which SI ratios are modified (extreme); mark stc2 = ebar.
    for (int i = pos1bk; i <= posffc; ++i) {
        if (!dpeq(stex[i - 1], ebar)) temp[i - 1] = stsi[i - 1];
        else temp[i - 1] = prm::DNOTST;
        stc2[i - 1] = ebar;
    }
    // x11pt3.f:176-185 -- D9A's savelog block, under the same two gates the
    // oracle applies (a classic X-11 Easter run and the trend-only mode both
    // skip it). Print surface stays deferred.
    if (opt.khol != 1 && opt.kfulsm < 2) prtd9a_savelog(ctx);

    // Year-ahead seasonal factors.
    int klda = posffc + ny;
    if (opt.kfulsm < 2) forcst(sts, 0, posffc, klda, ny, 1, 0.5, 1.0);

    // --- D10: modified seasonally adjusted series ---
    if (opt.kfulsm == 2) {
        // Trend-only (x11pt3.f:207-209): no seasonal is removed -- the modified
        // SA is just D1 (Stcsi), and the seasonal factors collapse to the
        // constant ebar.
        copy(stcsi, posffc, 1, stci);
        setdp(ebar, klda, sts);
    } else if (psuadd) {
        // Pseudo-additive modified SA (x11pt3.f:245-249):
        // Stci = Stcsi - Stc*(Sts-1); flag any non-positive result as not-good.
        for (int i = pos1bk; i <= posffc; ++i) {
            stci[i - 1] = stcsi[i - 1] - stc[i - 1] * (sts[i - 1] - 1.0);
            if (ctx.goodob.gudval(i) && stci[i - 1] <= 0.0)
                ctx.goodob.gudval(i) = false;
        }
    } else {
    divsub(stci, stcsi, sts, pos1bk, posffc, muladd);  // modified SA
    if (muladd == 2) {
        // Log-additive: the components are on the log scale; antilog the seasonal
        // family back to the original scale and switch to multiplicative
        // arithmetic (x11pt3.f:252-266). klda == Posffc+Ny here.
        muladd = 0;
        opt.muladd = 0;
        antilg(sts, pos1bk, klda);
        antilg(stsi, pos1bk, posffc);
        antilg(stsie, pos1bk, posffc);
        divsub(stex, stsie, stsi, pos1bk, posffc, muladd);
        antilg(stcsi, pos1bk, posffc);
    }
    }  // end !psuadd modified-SA
    // The regARIMA-seasonal combine is skipped in the pseudo-additive case
    // (x11pt3.f:272 .and.(.not.Psuadd)).
    if ((adj.adjsea == 1 || adj.adjso == 1) && !psuadd) {
        // x11pt3.f:272-281: combine the regARIMA seasonal (Facsea) and seasonal-
        // outlier (Facso) factors into the final X-11 seasonal Sts (D10). D11/Stci
        // was already formed from the X-11-only Sts above; this fold makes D10 the
        // TOTAL seasonal. The stsx11 snapshot feeds only the deferred A18/LXEARS
        // table (output), so it is skipped.
        if (adj.adjsea == 1)
            addmul(sts, ctx.x11fac.facsea.data(), sts, pos1bk, posffc, muladd);
        if (adj.adjso == 1)
            addmul(sts, ctx.x11fac.facso.data(), sts, pos1bk, posffc, muladd);
        // x11pt3.f:280 -- x11{centerseasonal=yes}: re-center the COMBINED
        // seasonal (X-11 seasonal + the regARIMA seasonal/SO factors just folded
        // in) with a 2xNy moving average. vsfc itself was already ported (it is
        // vsfb's own tail); only this call site was missing, so the argument was
        // accepted and silently dropped. Reachable only from inside this
        // Adjsea/Adjso block, which is why it stayed invisible: without a
        // seasonal or seasonal-outlier regressor the oracle does not center
        // either, and on/off are identical.
        // NOT GATED, and currently UNREACHABLE. Adjsea/Adjso==1 needs a seasonal
        // or SO regressor (chkadj.f:165), and every such spec fatals earlier on
        // the separate "x11pt2 user/seasonal/cycle/x11reg factor combine+emit"
        // wall -- so there is no spec that both makes Lcentr live in the oracle
        // (measured: 2.06e-3 in d10/d11/d13/d16 with so1955.1) and runs here.
        // The transcription is a direct one of x11pt3.f:280 against a vsfc that
        // is itself already gated via vsfb; treat it as unverified until that
        // x11pt2 wall lifts, at which point it needs a real gate.
        if (ctx.x11msc.lcentr) {
            std::vector<double> temp(static_cast<std::size_t>(PLEN), 0.0);
            vsfc(sts, pos1bk, posffc, ny, opt.lter.data(), temp.data(), muladd);
        }
    }
    if (opt.ishrnk > 0) {
        // x11pt3.f:283-287: shrink the final seasonal factors (Miller &
        // Williams 2003). The Sts snapshot into stsx11 feeds only the deferred
        // SNS diagnostic table, so it is skipped here.
        shrink(stsi, sts, opt.mtype, opt.ishrnk, muladd, ny, pos1ob, posfob,
               pos1bk, posffc);
    }
    // (deferred: D10 table/punch/x11plt, D10b/EARS/SNS emits.)
    // Store the final seasonal factors for sliding-spans analysis
    // (x11pt3.f:311). No RETURN here in the oracle -- falls through to the
    // Irev check (and beyond, to D12/D11) regardless.
    if (hid.issap == 2) ssrit(ctx, sts, pos1ob, posfob, 2, series);
    // x11pt3.f:311-313 -- `IF(Irev.eq.4 .and. Lrvsf)`. The Lrvsf half was
    // missing, making the wall fire for any Irev==4 even when SEASONAL-factor
    // revisions were not among `estimates=`. Currently unobservable either way:
    // Irev only ever takes 0 or 1 in this port (readers_spec.cpp:2271 is the
    // only assignment; revdrv.f:387's Irev=4 has no counterpart, because
    // run_history inlines getrev's arithmetic instead of reaching it through
    // x11pt3). Tightened so the guard says what the Fortran says.
    if (hid.irev == 4 && ctx.rev.lrvsf) {
        x11_not_ported(ctx, "x11pt3 revisions seasonal store (getrev)");
        return;
    }
    opt.muladd = opt.tmpma;  // restore the model's adjustment mode (x11pt3.f:376)
    muladd = opt.tmpma;      // keep the local in sync (logadd: back to 2 for vtc)

    // x11pt3.f:379 -- snapshot the SA series here, before the D11 Fin*/Adj* folds
    // below rewrite Stci. Read back by the summary-only path and, for a composite
    // total, by agr3 (the DIRECT trend behind the R1/R2 comparison statistics).
    copy(stci + (pos1bk - 1), ext.nbfpob, 1, ckhs + (pos1bk - 1));

    klda = posfob + ny;
    if (nfcst > 0) klda = posfob + nfcst;
    const int k2 = klda - pos1bk + 1;

    // --- D12: final trend cycle via the variable trend-cycle filter ---
    // Summary measures (Kfulsm==1, x11pt3.f:473): the trend is fit to D1
    // (Stcsi), not to the modified SA.
    if (opt.kfulsm == 1)
        copy(stcsi + (pos1bk - 1), ext.nbfpob, 1, stci + (pos1bk - 1));
vtc(ctx, stc, stci);
    // x11pt3.f:400/407-410 -- the FINAL trend filter length. Gated on Ktcopt==0
    // (the program chose the length) AND on the FULL-ADJUSTMENT / trend-only
    // modes: x11pt3.f wraps the whole vtc block in `Kfulsm.eq.0.or.Kfulsm.eq.2`,
    // so a summary-measures run (Kfulsm==1) emits nothing even though this port
    // still runs vtc there over a different input.
    if (opt.ktcopt == 0 && (opt.kfulsm == 0 || opt.kfulsm == 2))
        ctx.x11_finaltrendma = opt.nterm;
    if (muladd == 2) {
        // Log-additive: antilog the trend, then bias-correct it (x11pt3.f:411-424).
        // biasfc is trbias's output, used only for a deferred print. tru7hn from
        // x11msc. After this the components are original-scale -> Muladd=0.
        antilg(stc, pos1bk, posffc);
        trbias(stc, sts, sti, pos1bk, posffc, biasfc, ny, ctx.x11msc.tru7hn);
        ebar = 1.0;
        muladd = 0;
        opt.muladd = 0;
    }
    if (muladd == 0) {
        bool chkfct = false;  // Nfcst>0 & Prttab(LXETRF): deferred print -> false.
        chktrn(ctx, stc, chkfct);  // oktrn/oktrf only gated deferred prints.
    }
    // x11pt3.f:439-442 (and the same four lines at :508 in the Kfulsm==1 branch)
    // accumulate the outlier / user-regression factors into sp2, the Sprior
    // snapshot taken at entry. Beyond the deferred A18/D18 total-factor tables
    // this DOES have a numeric consumer: the writeback at :1284-1288 replaces
    // Sprior with sp2, and x11pt4's Part F then reports Pbar/Psq/Vp off it.
    if (adj.adjls == 1)
        addmul(sp2, ctx.x11fac.facls.data(), sp2, pos1bk, posffc, muladd);
    if (adj.adjao == 1)
        addmul(sp2, ctx.x11fac.facao.data(), sp2, pos1bk, posffc, muladd);
    if (adj.adjtc == 1)
        addmul(sp2, ctx.x11fac.factc.data(), sp2, pos1bk, posffc, muladd);
    if (adj.adjusr == 1)
        addmul(sp2, ctx.x11fac.facusr.data(), sp2, pos1bk, posffc, muladd);

    // --- D11: final seasonally adjusted series (x11pt3.f:447-467, 516-522) ---
    if (opt.kfulsm == 2) {
        // Trend-only: the "final SA" is the original series unchanged.
        copy(series, posffc, 1, stci);
    } else if (opt.kfulsm == 1) {
        // Summary measures: replace both the final SA (D11) and its snapshot
        // (Ckhs) with A1 (the original series).
        copy(series + (pos1bk - 1), ext.nbfpob, 1, ckhs + (pos1bk - 1));
        copy(series + (pos1bk - 1), ext.nbfpob, 1, stci + (pos1bk - 1));
    } else if (psuadd) {
        // Pseudo-additive: Stci = Series - Stc*(Sts-1).
        for (int i = pos1bk; i <= posffc; ++i) {
            stci[i - 1] = series[i - 1] - stc[i - 1] * (sts[i - 1] - 1.0);
            if (ctx.goodob.gudval(i) && stci[i - 1] <= 0.0)
                ctx.goodob.gudval(i) = false;
        }
    } else {
        divsub(stci, series, sts, pos1bk, posffc, muladd);
    }
    // Combined factors = seasonal factors (no trading-day component on base);
    // for trend-only they collapse to the constant ebar (x11pt3.f:463-467).
    if (opt.kfulsm == 2)
        setdp(ebar, klda, ststd);
    else
        copy(sts + (pos1bk - 1), k2, 1, ststd + (pos1bk - 1));

    // Calendar / trading-day combine (x11pt3.f:525-550). Holiday combined-factor
    // rebuild: with no user holiday model (Haveum=F) this reduces to Faccal/Fachol.
    double* faccal = ctx.x11fac.faccal.data();
    if (!adj.finhol && (opt.khol == 2 || (hid.ixreg > 0 && xl.axrghl) ||
                        adj.adjhol == 1)) {
        if (ctx.xrgum.haveum) {
            setdp(muladd == 1 ? 0.0 : 1.0, PLEN, faccal);
            if (adj.adjtd == 1)
                addmul(faccal, faccal, ctx.x11fac.factd.data(), pos1bk, klda, muladd);
            else if (opt.kswv > 0)
                addmul(faccal, faccal, ctx.x11fac.stptd.data(), pos1bk, klda, muladd);
        } else {
            divsub(faccal, faccal, ctx.x11fac.fachol.data(), pos1bk, klda, muladd);
        }
    }
    // Combine the calendar factor into the final SA (D11) and total factors (D16).
    if ((adj.adjtd == 1 || (hid.ixreg > 0 && xl.axrgtd)) ||
        ((adj.finhol && (hid.ixreg > 0 && xl.axrghl)) || adj.adjhol == 1) ||
        opt.kswv > 0) {
        if (opt.kfulsm == 0 || opt.kfulsm == 2)
            divsub(stci, stci, faccal, pos1bk, posffc, muladd);
        addmul(ststd, ststd, faccal, pos1bk, klda, muladd);
    }
    if ((adj.adjtd == 0 || opt.kswv == 0) && pri.priadj > 1) {
        // x11pt3.f:556-566 -- a prior length-of-month/leap adjustment ran
        // without model trading day: fold those prior factors into the combined
        // adjustment factor (ststd/D16).
        // Begbak = Begspn shifted back Nbcst periods (editor.f:207); our driver
        // does not persist ctx.extend.begbak, so recompute it here.
        int begbak[2];
        const int nbc = ctx.extend.nbcst < 0 ? 0 : ctx.extend.nbcst;
        addate(ctx.mdldat.begspn.data(), ny, -nbc, begbak);
        int iadj1 = 0;
        dfdate(begbak, ctx.adj.begadj.data(), ny, iadj1);
        iadj1 += 1;
        for (int i = pos1bk; i <= klda; ++i) {
            if (muladd == 1)
                ststd[i - 1] += ctx.adj.adj(i + iadj1 - 1);
            else
                ststd[i - 1] *= ctx.adj.adj(i + iadj1 - 1);
        }
    }
    if (pu.nuspad > 0 || pri.priadj > 1) {
        // x11pt3.f:569 rmpadj.f -- remove the permanent prior adjustment from Stci
        // (D11). Three cases: no predefined prior => remove Usrpad; predefined prior
        // and no temporary user prior => remove Sprior; predefined + temporary user
        // prior => remove (Sprior -/ Usrtad). Frstap/Frstat+Lsp index the user
        // arrays to the span start.
        const int lsp = ctx.lzero.lsp;
        const double* sprior = ctx.inpt.sprior.data();
        for (int i = pos1bk; i <= posffc; ++i) {
            double d;
            if (pri.priadj <= 1) {
                d = ctx.priadj.usrpad(pu.frstap + i - pos1bk + lsp - 1);
            } else if (pu.nustad == 0) {
                d = sprior[i - 1];
            } else {
                const double t = ctx.priadj.usrtad(pu.frstat + i - pos1bk + lsp - 1);
                d = (muladd == 1) ? sprior[i - 1] - t : sprior[i - 1] / t;
            }
            if (muladd == 1) stci[i - 1] -= d;
            else stci[i - 1] /= d;
        }
    }

    // --- D13: final irregular ---
    divsub(sti, stci, stc, pos1bk, posffc, muladd);
    // x11pt3.f:575-587 -- final outlier/user re-adjustment. Sti was computed from
    // the pre-removal Stci above, so the order matters: Fin* remove the effect
    // from the final SA series (Stci/D11); Adj* remove it from the final irregular
    // (Sti/D13, which the D13 published table then restores via sti2). Facusr (the
    // user-regression factor) folds with no count guard, mirroring the oracle.
    double* facao = ctx.x11fac.facao.data();
    double* facls = ctx.x11fac.facls.data();
    double* factc = ctx.x11fac.factc.data();
    double* facusr = ctx.x11fac.facusr.data();
    if (adj.finao && adj.nao > 0) divsub(stci, stci, facao, pos1bk, posffc, muladd);
    if (adj.finls && adj.nls > 0) divsub(stci, stci, facls, pos1bk, posffc, muladd);
    if (adj.fintc && adj.ntc > 0) divsub(stci, stci, factc, pos1bk, posffc, muladd);
    if (adj.finusr) divsub(stci, stci, facusr, pos1bk, posffc, muladd);
    if (adj.adjls == 1 && adj.nls > 0) divsub(sti, sti, facls, pos1bk, posffc, muladd);
    if (adj.adjao == 1 && adj.nao > 0) divsub(sti, sti, facao, pos1bk, posffc, muladd);
    if (adj.adjtc == 1 && adj.ntc > 0) divsub(sti, sti, factc, pos1bk, posffc, muladd);
    if (adj.adjusr == 1) divsub(sti, sti, facusr, pos1bk, posffc, muladd);
    // x11pt3.f:591-604 -- remove the TEMPORARY prior from the final irregular.
    // (Unlike the permanent prior, which rmpadj takes out of D11 above, the
    // temporary one stays in the SA series -- that is what makes it temporary --
    // so it is stripped from D13 alone. Note the index base: Frstat+i-Pos1bk,
    // WITHOUT rmpadj's extra Lsp-1.)
    if (pu.nustad > 0) {
        for (int i = pos1bk; i <= posffc; ++i) {
            const double t = ctx.priadj.usrtad(pu.frstat + i - pos1bk);
            if (muladd == 1) sti[i - 1] -= t;
            else             sti[i - 1] /= t;
        }
    }

    // D11 write + residual-seasonality test.
    // x11pt3.f:621-635 -- transform{constant=} added the constant to the WHOLE
    // series back in editor.f, so the PUBLISHED seasonally adjusted series and
    // original have it taken back out here. The pre-removal D11 is kept as
    // Stcipc, which is the `sac` save table ("SA series with the constant").
    // The negative-value message is print surface, but its Iyrt>0 floor at zero
    // is not -- force reads Stci afterwards.
    if (!dpeq(adjc.cnstnt, prm::DNOTST)) {
        const double c = adjc.cnstnt;
        ctx.x11_stcipc.assign(stci, stci + PLEN);
        for (int i = pos1ob; i <= posffc; ++i) {
            stci[i - 1] -= c;
            series[i - 1] -= c;
            if (!(stci[i - 1] > 0.0) && muladd != 1 && frc.iyrt > 0)
                stci[i - 1] = 0.0;
        }
    } else {
        ctx.x11_stcipc.clear();
    }
    // x11pt3.f:620/661 -- the RESIDUAL-SEASONALITY F-test on D11. Print/punch
    // of the D11 table itself is still deferred, but this call also writes the
    // `d11.f` / `d11.3y.f` savelog rows, which are not print surface. Runs on
    // BOTH branches of the transform{constant=} split above, exactly as the
    // oracle does (:620 with a constant removed, :661 without).
    ftest(ctx, stci, pos1ob, posfob, ny, 1);
    if (ctx.error.lfatal) return;

    // Store SA for sliding-spans / revisions (off base). Oracle RETURNs
    // immediately after the ssrit call here (x11pt3.f:678-680).
    if (hid.issap == 2 && frc.iyrt == 0 && !frc.lrndsa) {
        ssrit(ctx, stci, pos1ob, posfob, 3, series);
        return;
    }
    // x11pt3.f:677-685 -- `IF(Irev.eq.4 .and. (Lrvsa.or.Lrvch) .and.
    // Iyrt.eq.0)`. See the seasonal-store note above for why the missing
    // disjunct is unobservable here.
    if (hid.irev == 4 && (ctx.rev.lrvsa || ctx.rev.lrvch) && frc.iyrt == 0) {
        x11_not_ported(ctx, "x11pt3 revisions SA store (getrev)");
        return;
    }
    // (deferred: D11 forecast-portion table/x11plt.)

    // Force yearly totals (Iyrt>0): revise D11 (Stci) into Stci2 so its
    // calendar-year totals match a target series' yearly totals (force{} spec).
    // x11pt3.f:702-784. Iyrt==0 -> ELSE just copies Stci into Stci2.
    if (frc.iyrt > 0) {
        const int lstfrc = frc.lfctfr ? posffc : posfob;  // last obs to force
        // Target (stbase), x11pt3.f:715-722. target=original takes the original
        // series; the other three take the buffers x11pt1/x11pt2 left holding the
        // partially-adjusted original:
        //   1 calendaradj    -> Stocal (original less the calendar effect)
        //   2 permprioradj   -> Stopp  (original less the permanent prior)
        //   3 both           -> Stopp, then also divide out Faccal
        // NOTE the faithful short copy: x11pt1 fills Stocal/Stopp only over
        // [Pos1ob, Posfob], so with Lfctfr the tail [Posfob+1, lstfrc] is whatever
        // those buffers already held (identity-initialized here, stack garbage in
        // the oracle) -- the oracle does the same thing, so it is reproduced.
        double stbase[PLEN];
        switch (frc.iftrgt) {
        case 0:
            copy(series, lstfrc, 1, stbase);
            break;
        case 1:
            copy(os.stocal.data(), lstfrc, 1, stbase);
            break;
        default:
            copy(os.stopp.data(), lstfrc, 1, stbase);
            if (frc.iftrgt == 3)
                divsub(stbase, stbase, ctx.x11fac.faccal.data(), pos1ob, lstfrc,
                       muladd);
            break;
        }

        if (frc.iyrt == 1) {
            // type=denton: modified-Denton benchmarking.
            int ib = 0, ie = 0;
            qmap(stbase, stci, stci2, pos1ob, lstfrc, ny, ib, ie, frc.begyrt);
            // Carry the boundary adjustment across a trailing/leading partial
            // year (X-11-ARIMA/88 behaviour, x11pt3.f:730-745).
            if (ie < lstfrc) {
                const double d = stci2[ie - 1] - stci[ie - 1];
                for (int i = ie + 1; i <= lstfrc; ++i) stci2[i - 1] = stci[i - 1] + d;
            }
            if (ib > pos1ob) {
                const double d = stci2[ib - 1] - stci[ib - 1];
                for (int i = pos1ob; i <= ib - 1; ++i) stci2[i - 1] = stci[i - 1] + d;
            }
        } else {
            // type=regress: Cholette-Dagum regression benchmarking. qmap2
            // computes the full forced span directly (no partial-year carry).
            qmap2(stbase, stci, stci2, pos1ob, lstfrc, ny, 0, frc.lamda,
                  frc.rol, frc.mid, frc.begyrt);
        }

        // x11pt3.f:750-782 -- the negative-value correction. Reachable only with
        // transform{constant=} in a non-additive mode: the constant removal above
        // subtracts Cnstnt from Stci (flooring it at zero when Iyrt>0), so the
        // forced Stci2 can land at or below zero too. Clamp those to zero and, if
        // any were found, prorate the CLAMPED series against the target with a
        // second pass. Three things are load-bearing here:
        //   * the fallback pass is qmap2 even when the primary pass was Denton;
        //   * its input series is Stci2 (the clamped, already-forced series), not
        //     Stci -- so the correction compounds on the first pass;
        //   * it runs with Rol=0 / Lamda=0.5, which the oracle achieves by saving
        //     and restoring the /force/ COMMON around the call (x11pt3.f:767-773).
        //     Here they are arguments, so the live frc.rol/frc.lamda are untouched.
        bool negmsg = false, negfin = false;
        if (muladd != 1 && !dpeq(adjc.cnstnt, prm::DNOTST)) {
            for (int i = pos1ob; i <= lstfrc; ++i) {
                if (!(stci2[i - 1] > 0.0)) {
                    negmsg = true;
                    stci2[i - 1] = 0.0;
                }
            }
            if (negmsg) {
                std::vector<double> temp2(PLEN, 0.0);
                qmap2(stbase, stci2, temp2.data(), pos1ob, lstfrc, ny, 0,
                      /*lamda=*/0.5, /*rol=*/0.0, frc.mid, frc.begyrt);
                // Faithful full-PLEN copy (x11pt3.f:774) -- everything outside
                // [Pos1ob,lstfrc] is overwritten with temp2's untouched tail, zero
                // here and uninitialised stack in the oracle. Nothing reads Stci2
                // outside that range, so the difference is unobservable.
                copy(temp2.data(), PLEN, 1, stci2);
                for (int i = pos1ob; i <= lstfrc; ++i)
                    if (!(stci2[i - 1] > 0.0)) negfin = true;
            }
        }
        // (deferred: D11A/rnd/e6*/p6*/cr/rr table + punch; the negmsg/negfin
        // NOTE messages.)

        // x11pt3.f:812-814 -- the residual-seasonality F-test AGAIN, now on the
        // FORCED series. Its print flags differ but its Lsav is the same, so it
        // OVERWRITES the `d11.f`/`d11.3y.f` savelog rows the :620 call wrote --
        // on a force{} run those canaries describe the forced SA, not D11.
        ftest(ctx, stci2, pos1ob, posfob, ny, 1);
        if (ctx.error.lfatal) return;

        // x11pt3.f:815-831 -- with force{} on, the sliding-spans / revisions SA
        // store takes the FORCED series and the oracle RETURNs right after the
        // ssrit call. (The Iyrt==0 counterpart is above, at x11pt3.f:678-680.)
        if (!frc.lrndsa) {
            if (hid.issap == 2) {
                ssrit(ctx, stci2, pos1ob, posfob, 3, series);
                return;
            }
            // x11pt3.f:815-829 -- `IF(Irev.eq.4 .and. (Lrvsa.or.Lrvch))`.
            if (hid.irev == 4 && (ctx.rev.lrvsa || ctx.rev.lrvch)) {
                x11_not_ported(ctx, "x11pt3 revisions forced-SA store (getrev)");
                return;
            }
        }

        // x11pt3.f:836-850 -- the per-observation forcing factor (the `ffc` save
        // table). With negfin (values STILL <= 0 after the correction) the ratio
        // is not formed at all: those observations get DNOTST, and the rest take
        // a hard quotient rather than going through divsub. That looks like an
        // inconsistency and is not one -- the log-additive mode has already
        // collapsed muladd 2->0 at the D12 antilog above, so muladd is 0 or 1
        // here, and the negfin branch is guarded on muladd!=1. Where it runs,
        // divsub divides too. (Measured: a logadd force spec's ffc is the
        // quotient of the published D11/D11A on both branches.)
        ctx.x11_frcfac.assign(PLEN, 0.0);
        double* frcfac = ctx.x11_frcfac.data();
        if (negfin) {
            for (int i = pos1ob; i <= lstfrc; ++i)
                frcfac[i - 1] = (stci2[i - 1] > 0.0)
                                    ? stci[i - 1] / stci2[i - 1]
                                    : prm::DNOTST;
        } else {
            divsub(frcfac, stci, stci2, pos1ob, lstfrc, muladd);
        }
    } else {
        copy(stci, posffc, 1, stci2);
        ctx.x11_frcfac.clear();
    }

    if (frc.lrndsa) {
        // Rounded seasonally adjusted series (UK X-11): round Stci2 so each
        // year's rounded values sum to the rounded annual total (x11pt3.f:881).
        bool rndok = false;
        rndsa(stci2, ctx.adxser.stcirn.data(), pos1ob, posfob, ny,
              ctx.x11opt.kdec, rndok);
        // x11pt3.f:897-899 -- and once more on the ROUNDED series, which
        // overwrites the forced run's rows in turn (same Lsav).
        ftest(ctx, ctx.adxser.stcirn.data(), pos1ob, posfob, ny, 1);
        if (ctx.error.lfatal) return;
        // (deferred: rnd table/punch; ssrit/getrev
        // stores. Rndok==false only on integer overflow -- unreachable for the
        // ported spans -- so no Lrndsa fallback is needed here.)
        (void)rndok;
    }

    // --- D12 published trend (x11pt3.f:926-932). If a level shift (or a TC, when
    // Lttc) was removed pre-x11, fold it back into the FINAL trend: an LS is a
    // permanent level change, so it belongs in the trend, not the irregular.
    // stc2 = Facls*Stc (*Factc). The internal Stc stays LS-free (Part-E's weight-
    // zero SA replacement then uses stc2); the published D12 is stc2, stored into
    // srs.stc as the final action. Base path leaves srs.stc as the published D12.
    const bool tad_in_trend = (pu.nustad > 0 && pri.lprntr);
    bool have_stc2 = false;
    if (((!adj.finls) && adj.adjls == 1) || tad_in_trend ||
        ((!adj.fintc) && lttc && adj.adjtc == 1)) {
        copy(stc + (pos1bk - 1), posffc - pos1bk + 1, 1, stc2 + (pos1bk - 1));
        if ((!adj.finls) && adj.adjls == 1)
            addmul(stc2, facls, stc, pos1bk, posffc, muladd);
        if ((!adj.fintc) && lttc && adj.adjtc == 1)
            addmul(stc2, factc, stc2, pos1bk, posffc, muladd);
        // x11pt3.f:937-946 -- transform{temppriortrend=yes}: the temporary prior
        // was divided out before the adjustment, so put it back into the
        // published trend as well as into the SA series.
        if (tad_in_trend) {
            for (int i = pos1bk; i <= posffc; ++i) {
                const double t = ctx.priadj.usrtad(pu.frstat + i - pos1bk);
                if (muladd == 1) stc2[i - 1] += t;
                else             stc2[i - 1] *= t;
            }
        }
        have_stc2 = true;
    }
    // x11pt3.f:950-953 (folded branch) / :1015-1018 (plain branch) -- take the
    // transform{constant=} back out of the PUBLISHED trend too, keeping the
    // pre-removal copy as stc2pc, the `tac` save table. Note the constant stays
    // in the INTERNAL Stc when the folded branch runs, exactly as in the oracle:
    // there it is `stc2` that is decremented, and Stc is only touched when there
    // is no stc2. Part F reads the internal one and adds the constant back.
    if (!dpeq(adjc.cnstnt, prm::DNOTST)) {
        const double c = adjc.cnstnt;
        double* pub = have_stc2 ? stc2 : stc;
        ctx.x11_stc2pc.assign(pub, pub + PLEN);
        for (int i = pos1ob; i <= posffc; ++i) pub[i - 1] -= c;
    } else {
        ctx.x11_stc2pc.clear();
    }
    // (deferred: D12 table/prttrn/punch/x11plt of stc2/Stc.)
    if (hid.irev == 4) {
        x11_not_ported(ctx, "x11pt3 revisions trend store (getrev)");
        return;
    }
    // (deferred: logadd trend bias-correction table -- Tmpma==2 only.)

    // --- D13 published table (x11pt3.f:1089-1122). The AO/TC outliers removed
    // from Sti above (Adj* -> irregular) are RESTORED for the published D13:
    // sti2 = Sti (*Facao)(*Factc). Built into a scratch buffer here and stored
    // into srs.sti as the final action (Part-E below still consumes the AO-removed
    // Sti). The base path (no outlier) leaves srs.sti as the published D13, so it
    // stays untouched. ---
    double sti2buf[PLEN];
    bool have_sti2 = false;
    if (adj.adjao == 1 || (adj.adjtc == 1 && !lttc)) {
        copy(sti + (pos1bk - 1), posffc - pos1bk + 1, 1, sti2buf + (pos1bk - 1));
        if (adj.adjao == 1)
            addmul(sti2buf, facao, sti2buf, pos1bk, posffc, muladd);
        if (adj.adjtc == 1 && !lttc)
            addmul(sti2buf, factc, sti2buf, pos1bk, posffc, muladd);
        have_sti2 = true;
    }
    // (deferred: D13 table/punch/x11plt of sti2/Sti.)

    if (opt.khol == 1) return;
    // x11pt3.f:1128-1146 -- D16, the combined seasonal + calendar adjustment
    // factors. The table/punch itself is deferred output, but the buffer is
    // snapshotted onto ctx here (ststd is a function-local in the oracle) so the
    // harness can emit d16 for the parity gate. Save range mirrors the punch:
    // frstsf=Pos1ob (Pos1bk when Savbct), lastsf=Posfob / Posffc (Savfct and
    // Nfcst>0) / Posfob+Ny (Savfct, no forecasts) -- resolved by the caller.
    // (deferred: D16b Psuadd; D18 TD table -- all off base or pure deferred print.)
    ctx.x11_ststd.assign(ststd, ststd + PLEN);

    // --- PART E: modified original / SA / irregular series ---
    // x11pt3.f:69-70 -- nadj2 is Nadj when a prior-adjustment series exists;
    // it gates the Sprior fold on the weight-zero replacement below (the SA
    // replacement is built from the trend, which carries no prior, so the prior
    // has to be put back). Feeds E2 = Stcime and hence x11pt4's Cimbar.
    const int nadj2 = (pri.kfmt > 0) ? ctx.adj.nadj : 0;
    opt.kpart = 5;
    // x11pt3.f:1204-1207 etc. -- Series/Stci had the constant taken out above, so
    // Part E puts it back for the duration: every measure it builds lives on the
    // shifted scale, and :1275-1279 removes it again at the end.
    const bool have_cnst = !dpeq(adjc.cnstnt, prm::DNOTST);
    const double cnst = have_cnst ? adjc.cnstnt : 0.0;
    for (int i = pos1bk; i <= posffc; ++i) {
        if (stwt[i - 1] > 0.0) {
            stome[i - 1] = series[i - 1] + cnst;
            stime[i - 1] = sti[i - 1];
            stcime[i - 1] = stci[i - 1] + cnst;
        } else {
            stcime[i - 1] = have_stc2 ? stc2[i - 1] : stc[i - 1];  // LS -> folded trend
            stcime[i - 1] += cnst;
            stime[i - 1] = ebar;          // expected irregular
            // x11pt3.f:1227-1231 -- rebuild the original from its components.
            const double s = series[i - 1] + cnst;
            stome[i - 1] = muladd == 1 ? s - sti[i - 1] : s / sti[i - 1];
            // x11pt3.f:1232-1238 -- put the prior back into the SA replacement.
            if (nadj2 > 0) {
                if (muladd == 1) stcime[i - 1] += in.sprior(i);
                else             stcime[i - 1] *= in.sprior(i);
            }
            // x11pt3.f:1243-1249 -- with final=ls the level shift is NOT part of
            // the published SA series, so take it back out of the replacement
            // value (which was built from the trend, and the trend carries it).
            if (adj.finls) {
                if (muladd == 1) stcime[i - 1] -= facls[i - 1];
                else             stcime[i - 1] /= facls[i - 1];
            }
        }
    }
    // x11pt3.f:1265-1273 -- if AO/TC outliers were removed pre-adjustment, remove
    // them from the modified original (E1=Stome) and, unless kept final, the
    // modified SA (E2=Stcime). The modified irregular (E3=Stime) is left untouched
    // (the oracle comments out its divide). LS/user E-folds stay unported.
    // Finls removes the LS from the modified SA -- done inside the loop above,
    // where the oracle does it. Adjusr does NOT fold into E1/E2 here; it only
    // triggers the sp2 -> Sprior writeback (x11pt3.f:1284-1288, Kfmt/A18), which
    // does not feed the E-tables, so it passes through.
    if (adj.adjao == 1) {
        divsub(stome, stome, facao, pos1bk, posffc, muladd);
        if (!adj.finao) divsub(stcime, stcime, facao, pos1bk, posffc, muladd);
    }
    if (adj.adjtc == 1) {
        divsub(stome, stome, factc, pos1bk, posffc, muladd);
        if (!adj.fintc) divsub(stcime, stcime, factc, pos1bk, posffc, muladd);
    }
    // x11pt3.f:1275-1281 -- and take the constant back out of E1/E2. NB this
    // runs AFTER the AO/TC divides above, so in multiplicative mode the constant
    // is divided by those factors first and then subtracted: the round trip is
    // NOT the identity. Verbatim.
    if (have_cnst) {
        for (int i = pos1bk; i <= posffc; ++i) {
            stome[i - 1] -= cnst;
            stcime[i - 1] -= cnst;
        }
    }
    // x11pt3.f:1284-1288 -- with any outlier / user-regression effect removed,
    // the prior-factor series becomes the TOTAL prior: Sprior <- sp2. Read by
    // the deferred A18/D18 tables and, numerically, by x11pt4's Pbar/Psq/Vp.
    // (Cnstnt==DNOTST base: no constant subtract above.)
    if (adj.adjls == 1 || adj.adjusr == 1 || adj.adjao == 1 || adj.adjtc == 1) {
        copy(sp2, PLEN, 1, in.sprior.data());
        // (nadj2 is a local the oracle also bumps here, but only Part E above
        // reads it -- the assignment is dead by this point.)
        if (ctx.prior.kfmt == 0) ctx.prior.kfmt = 1;
    }

    // Hand x11pt4 the INTERNAL D13/D12 before the publication folds below
    // overwrite them -- the oracle keeps Sti/Stc internal here and prints from
    // its own sti2/stc2 locals, so the Part-F summary measures are computed on
    // the AO/TC-removed irregular and the level-shift-free trend.
    ctx.x11_sti_int.assign(sti, sti + PLEN);
    ctx.x11_stc_int.assign(stc, stc + PLEN);

    // Store the AO/TC-restored published D13 (sti2, built above) so the harness
    // and D13 table read it as Sti. Base path (no outlier) leaves srs.sti as the
    // already-published D13, untouched.
    if (have_sti2)
        copy(sti2buf + (pos1bk - 1), posffc - pos1bk + 1, 1, sti + (pos1bk - 1));
    if (have_stc2)
        copy(stc2 + (pos1bk - 1), posffc - pos1bk + 1, 1, stc + (pos1bk - 1));
}

}  // namespace x13
