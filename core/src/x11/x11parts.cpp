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
#include "x11/x11seas.hpp"          // vsfa, vsfb
#include "x11/x11xtrm.hpp"          // xtrm, vtest, entsch
#include "x11/x11drv.hpp"           // forcst, vtc, si
#include "specparse/specparse.hpp"  // copy, setlg, abend, errhdr, writln, stdio
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

    // Prior trading-day adjustment (user-specified or via X-11 regression). The
    // factor generation (pritd) and sliding-spans capture (ssrit) are unported;
    // this branch is inactive for the base decomposition (Kswv=0, Ixreg=0).
    if (opt.kswv != 0 || (ctx.hiddn.ixreg >= 2 && ctx.x11log.axrgtd)) {
        x11_not_ported(ctx, "x11pt1 prior trading-day adjustment (pritd/ssrit)");
        return;
    }

    // (deferred: lmodel pre-ARIMA prior-adjusted-series prints A3/A3P/A4D.)
    (void)lmodel;

    // Set Stcsi equal to the prior-adjusted series.
    copy(os.sto.data() + (pos1ob - 1), posfob - pos1ob + 1, -1,
         os.stcsi.data() + (pos1ob - 1));
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
void x11pt2(X13Context& ctx, bool /*lmodel*/, bool lx11, bool lseats,
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
    double stex[PLEN];  // COMMON /mq10/ Stex  -- per-iteration extreme component
    auto STEX = [&](int i) -> double& { return stex[i - 1]; };

    // --- Model-based prior-adjustment / factor preamble (l.74-351) ---
    // (deferred: .xdg prioradj savelog.) None of these run on the base path:
    // makadj/tdlom (model-TD + length-of-month) need Priadj>1; ssrit needs sliding
    // spans; the Faccal/Fachol factor combines + calendar/outlier-adjusted-series
    // emits are print/save gated by the Adj*/Khol/Axrg* flags. Guard the compute-
    // affecting activations so a spec that needs them fatals cleanly.
    if (ctx.hiddn.ixreg != 2 && ctx.prior.priadj > 1 && goodlm) {
        x11_not_ported(
            ctx, "x11pt2 model-TD/length-of-month prior adjustment (makadj/tdlom)");
        return;
    }
    if (ctx.hiddn.issap == 2) {
        x11_not_ported(ctx, "x11pt2 sliding-spans trading-day capture (ssrit)");
        return;
    }
    {
        const x11adj_cmn& adj = ctx.x11adj;
        const x11log_cmn& xl = ctx.x11log;
        if (adj.adjtd == 1 || adj.adjhol == 1 || adj.finhol || adj.adjao == 1 ||
            adj.finao || adj.adjls == 1 || adj.finls || adj.adjtc == 1 ||
            adj.fintc || adj.adjso == 1 || adj.adjsea == 1 || adj.adjcyc == 1 ||
            adj.adjusr == 1 || adj.finusr || opt.khol >= 2 || xl.axrgtd ||
            xl.axrghl) {
            x11_not_ported(ctx, "x11pt2 model-based adjustment-factor combine/emit");
            return;
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
        // (deferred: ftest seasonality F-test; B2/C2/D2 trend table.)

        if (kpart == 2) {
            // Part B: replace extreme SI ratios (si drives vsfb/xtrm/replac).
            int pos1ex = mfda, posfex = mlda;
            if (noxfct) { pos1ex = mfd1; posfex = klda; }
            si(ctx, opt.ksect, mfda, mlda, ny, nfcst, nbcst, kersa1, ksdev1,
               pos1ob, posfob, opt.kfulsm, pos1ex, posfex);
        } else if (opt.kfulsm < 2) {
            vsfb(sts, stsi, mfda, mlda, ny, opt.lterm, opt.lter.data(), opt.ksect,
                 ctx.x11msc.shrtsf, temp, muladd);
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
                 opt.ksect, ctx.x11msc.shrtsf, temp, muladd);
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
        // (Ixreg==0: the x11-regression option is skipped.)

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
        // (Axrg* off: Stcsi = Sto; Ixreg==1 Stocal calendar-adjust also skipped.)
        for (int i = pos1bk; i <= posffc; ++i) STCSI(i) = STO(i);

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

    // Transient COMMON scratch (function-local PLEN, as in x11pt2).
    double temp[PLEN];    // /work/  Temp   (D9 replacement buffer)
    double stsie[PLEN];   // /work3/ Stsie  (D8 unmodified SI)
    double stex[PLEN];    // /mq10/  Stex   (extreme component; carries from x11pt2)
    double stime[PLEN];   // /mq5a/  Stime  (E3 modified irregular)
    double ckhs[PLEN];    // /kcser/ Ckhs   (SA snapshot; dead on the base path)
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

    // D8 analysis of variance on the unmodified SI ratios.
    if (!hid.lhiddn && opt.khol != 1) {
        // deferred diagnostics: ftest/kwtest/mstest/COMBFT (D8 F/moving-seasonality
        // tests). ftest/mstest are read-only on the series; kwtest sorts Stsie in
        // place but Stsie is dead downstream on the base path (only deferred prints
        // read it) and is rebuilt by the addmul below; COMBFT takes no series arg.
        // All stubbed no-ops.
        addmul(stsie, stsi, stex, pos1bk, posffc, muladd);  // rebuild Stsie post-kwtest
    }
    // (Issap==2 sliding-spans alt diagnostic branch: off base.)

    double ebar = 0.0;
    if (muladd == 0) ebar = 1.0;

    // Ksdev>1 (calendarsigma / override) extreme re-replacement -- ported leaves,
    // off the single-sigma base path.
    if (xt.ksdev > 1) {
        copy(stsie, posfob, 1, stsi);
        replac(stsi, temp, stwt, pos1bk, posfob, ny);
        if (opt.kfulsm < 2) sfmsr(ctx, sts, stsi, pos1bk, posfob, posffc);
    }
    // (deferred: D8B prtd8b.)

    // D9: identify which SI ratios are modified (extreme); mark stc2 = ebar.
    for (int i = pos1bk; i <= posffc; ++i) {
        if (!dpeq(stex[i - 1], ebar)) temp[i - 1] = stsi[i - 1];
        else temp[i - 1] = prm::DNOTST;
        stc2[i - 1] = ebar;
    }
    // (deferred: D9 table/punch/prtd9a.)

    // Year-ahead seasonal factors.
    int klda = posffc + ny;
    if (opt.kfulsm < 2) forcst(sts, 0, posffc, klda, ny, 1, 0.5, 1.0);

    // --- D10: modified seasonally adjusted series (Kfulsm==0 base path) ---
    if (opt.kfulsm == 2) {
        x11_not_ported(ctx, "x11pt3 Kfulsm==2 full-seasonal D10 branch");
        return;
    }
    if (psuadd) {
        x11_not_ported(ctx, "x11pt3 pseudo-additive D10/D11 (Psuadd)");
        return;
    }
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
    if (adj.adjsea == 1 || adj.adjso == 1) {
        x11_not_ported(ctx, "x11pt3 regARIMA-seasonal combine (Adjsea/Adjso)");
        return;
    }
    if (opt.ishrnk > 0) {
        x11_not_ported(ctx, "x11pt3 seasonal shrinkage (Ishrnk)");
        return;
    }
    // (deferred: D10 table/punch/x11plt, D10b/EARS/SNS emits.)
    if (hid.issap == 2) {
        x11_not_ported(ctx, "x11pt3 sliding-spans seasonal store (ssrit)");
        return;
    }
    if (hid.irev == 4) {
        x11_not_ported(ctx, "x11pt3 revisions seasonal store (getrev)");
        return;
    }
    opt.muladd = opt.tmpma;  // restore the model's adjustment mode (x11pt3.f:376)
    muladd = opt.tmpma;      // keep the local in sync (logadd: back to 2 for vtc)

    // Snapshot the modified SA for the summary-only path (dead on the base path).
    copy(stci + (pos1bk - 1), ext.nbfpob, 1, ckhs + (pos1bk - 1));

    klda = posfob + ny;
    if (nfcst > 0) klda = posfob + nfcst;
    const int k2 = klda - pos1bk + 1;

    if (opt.kfulsm != 0) {
        // Kfulsm==1 (summary measures) replaces D11 with D1 and re-runs vtc; the
        // full body is unported (base gate is Kfulsm==0).
        x11_not_ported(ctx, "x11pt3 Kfulsm==1 summary-measures branch");
        return;
    }

    // --- D12: final trend cycle via the variable trend-cycle filter ---
    vtc(ctx, stc, stci);
    // (deferred: finaltrendma savelog.)
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
    if (adj.adjls == 1 || adj.adjao == 1 || adj.adjtc == 1 || adj.adjusr == 1) {
        x11_not_ported(ctx, "x11pt3 outlier/user factor fold-in (Adj*)");
        return;
    }

    // --- D11: final seasonally adjusted series ---
    divsub(stci, series, sts, pos1bk, posffc, muladd);
    // Combined factors = seasonal factors (no trading-day component on base).
    copy(sts + (pos1bk - 1), k2, 1, ststd + (pos1bk - 1));

    // Calendar / trading-day / prior-length-of-month combine (all off base).
    if (!adj.finhol && (opt.khol == 2 || (hid.ixreg > 0 && xl.axrghl) ||
                        adj.adjhol == 1)) {
        x11_not_ported(ctx, "x11pt3 holiday factor rebuild (Faccal)");
        return;
    }
    if ((adj.adjtd == 1 || (hid.ixreg > 0 && xl.axrgtd)) ||
        ((adj.finhol && (hid.ixreg > 0 && xl.axrghl)) || adj.adjhol == 1) ||
        opt.kswv > 0) {
        x11_not_ported(ctx, "x11pt3 trading-day/holiday combine into D11/D16");
        return;
    }
    if ((adj.adjtd == 0 || opt.kswv == 0) && pri.priadj > 1) {
        x11_not_ported(ctx, "x11pt3 prior length-of-month fold-in (Priadj>1)");
        return;
    }
    if (pu.nuspad > 0 || pri.priadj > 1) {
        x11_not_ported(ctx, "x11pt3 prior-adjustment removal (rmpadj)");
        return;
    }

    // --- D13: final irregular ---
    divsub(sti, stci, stc, pos1bk, posffc, muladd);
    if ((adj.finao && adj.nao > 0) || (adj.finls && adj.nls > 0) ||
        (adj.fintc && adj.ntc > 0) || adj.finusr ||
        (adj.adjls == 1 && adj.nls > 0) || (adj.adjao == 1 && adj.nao > 0) ||
        (adj.adjtc == 1 && adj.ntc > 0) || adj.adjusr == 1) {
        x11_not_ported(ctx, "x11pt3 final outlier/user re-adjustment of Stci/Sti");
        return;
    }
    if (pu.nustad > 0) {
        x11_not_ported(ctx, "x11pt3 user temporary-adjustment removal (Nustad)");
        return;
    }

    // D11 write + residual-seasonality test. Base path: no temporary constant.
    if (!dpeq(adjc.cnstnt, prm::DNOTST)) {
        x11_not_ported(ctx, "x11pt3 constant removal from D11/original");
        return;
    }
    // (deferred: D11 table/punch; residual-seasonality ftest(Stci) -- read-only.)

    // Store SA for sliding-spans / revisions (off base).
    if (hid.issap == 2 && frc.iyrt == 0 && !frc.lrndsa) {
        x11_not_ported(ctx, "x11pt3 sliding-spans SA store (ssrit)");
        return;
    }
    if (hid.irev == 4 && frc.iyrt == 0) {
        x11_not_ported(ctx, "x11pt3 revisions SA store (getrev)");
        return;
    }
    // (deferred: D11 forecast-portion table/x11plt.)

    // Force yearly totals (Iyrt>0) is off base -> ELSE copies Stci into Stci2.
    if (frc.iyrt > 0) {
        x11_not_ported(ctx, "x11pt3 force yearly totals (qmap/qmap2)");
        return;
    }
    copy(stci, posffc, 1, stci2);

    if (frc.lrndsa) {
        x11_not_ported(ctx, "x11pt3 rounded seasonally adjusted series (rndsa)");
        return;
    }

    // --- D12 write: level-shift/temporary-change fold-in is off base; the ELSE
    // is all deferred print (Cnstnt==DNOTST here). ---
    if (((!adj.finls) && adj.adjls == 1) || (pu.nustad > 0 && pri.lprntr) ||
        ((!adj.fintc) && lttc && adj.adjtc == 1)) {
        x11_not_ported(ctx, "x11pt3 final-trend LS/TC fold-in (D12)");
        return;
    }
    // (deferred: D12 table/prttrn/punch/x11plt of Stc.)
    if (hid.irev == 4) {
        x11_not_ported(ctx, "x11pt3 revisions trend store (getrev)");
        return;
    }
    // (deferred: logadd trend bias-correction table -- Tmpma==2 only.)

    // --- D13 write (all deferred): AO/TC restore branch is off base. ---
    if (adj.adjao == 1 || (adj.adjtc == 1 && !lttc)) {
        x11_not_ported(ctx, "x11pt3 D13 AO/TC restore (sti2)");
        return;
    }
    // (deferred: D13 table/punch/x11plt of Sti.)

    if (opt.khol == 1) return;
    // (deferred: D16 table/punch of ststd; D16b Psuadd; D18 TD table -- all off
    // base or pure deferred print.)

    // --- PART E: modified original / SA / irregular series ---
    opt.kpart = 5;
    for (int i = pos1bk; i <= posffc; ++i) {
        if (stwt[i - 1] > 0.0) {
            stome[i - 1] = series[i - 1];
            stime[i - 1] = sti[i - 1];
            stcime[i - 1] = stci[i - 1];
        } else {
            stcime[i - 1] = stc[i - 1];   // base: not LS/Nustad -> final trend
            stime[i - 1] = ebar;          // expected irregular
            stome[i - 1] = series[i - 1] / sti[i - 1];  // muladd==0
            // (nadj2==0 base: no Sprior fold; Finls==F: no Facls divide.)
        }
    }
    if (adj.adjao == 1 || adj.adjtc == 1) {
        x11_not_ported(ctx, "x11pt3 Part-E AO/TC removal from modified series");
        return;
    }
    // (Cnstnt==DNOTST base: no constant subtract.)
    if (adj.adjls == 1 || adj.adjusr == 1 || adj.adjao == 1 || adj.adjtc == 1) {
        x11_not_ported(ctx, "x11pt3 Part-E Sprior restore (Adj*)");
        return;
    }
}

}  // namespace x13
