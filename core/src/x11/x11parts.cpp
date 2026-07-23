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
#include "x11/x11reg.hpp"           // x11mdl_td (x11regression irregular regression)
#include "x11/loadxr.hpp"           // loadxr (regARIMA <-> x11reg model swap)
#include "x11/x11xtrm.hpp"          // xtrm, vtest, entsch
#include "x11/x11drv.hpp"           // forcst, vtc, si
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
                // Adjtd==0 branch (tdlom.f:44-59): unported (no gated spec).
                x11_not_ported(ctx, "x11pt2 tdlom Adjtd==0 LOM removal");
                return;
            }
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
        if (adj.adjso == 1 || adj.adjsea == 1 ||
            adj.adjcyc == 1 || xl.axrghl ||
            (xl.axrgtd && ctx.hiddn.ixreg != 1)) {
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
        // (deferred: ftest seasonality F-test; B2/C2/D2 trend table.)

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

        // X-11 regression on the irregular (x11pt2.f:711 -> x11mdl), Ixreg==1:
        // regress the TD design on Sti (B13/C13), snapshot b16/c16, and divide
        // the TD effect out of Sti so the iteration continues without it.
        if (ctx.hiddn.ixreg == 1 && (kpart == 2 || kpart == 3)) {
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
        if (ctx.hiddn.ixreg == 1 && ctx.x11log.axrgtd)
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
        x11_not_ported(ctx, "x11pt3 regARIMA-seasonal combine (Adjsea/Adjso)");
        return;
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

    // --- D12: final trend cycle via the variable trend-cycle filter ---
    // Summary measures (Kfulsm==1, x11pt3.f:473): the trend is fit to D1
    // (Stcsi), not to the modified SA.
    if (opt.kfulsm == 1)
        copy(stcsi + (pos1bk - 1), ext.nbfpob, 1, stci + (pos1bk - 1));
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
    // x11pt3.f:439-442 accumulates the outlier/user factors into sp2 (a Sprior
    // snapshot) that feeds only the deferred A18/D18 total-factor tables and the
    // Sprior writeback (l.1285) -- none of which feed D10-D13. Skipped on the
    // outlier base path; the user-regression factor (Adjusr) is likewise
    // deferred-only here.

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

    // Store SA for sliding-spans / revisions (off base). Oracle RETURNs
    // immediately after the ssrit call here (x11pt3.f:678-680).
    if (hid.issap == 2 && frc.iyrt == 0 && !frc.lrndsa) {
        ssrit(ctx, stci, pos1ob, posfob, 3, series);
        return;
    }
    if (hid.irev == 4 && frc.iyrt == 0) {
        x11_not_ported(ctx, "x11pt3 revisions SA store (getrev)");
        return;
    }
    // (deferred: D11 forecast-portion table/x11plt.)

    // Force yearly totals (Iyrt>0): revise D11 (Stci) into Stci2 so its
    // calendar-year totals match a target series' yearly totals (force{} spec).
    // x11pt3.f:702-784. Iyrt==0 -> ELSE just copies Stci into Stci2.
    if (frc.iyrt > 0) {
        const int lstfrc = frc.lfctfr ? posffc : posfob;  // last obs to force
        // Target (stbase): only Iftrgt==0 (target=original) is ported. The
        // calendaradj/permprioradj/both targets (x11pt3.f:717-721 -> Stocal /
        // Stopp) are transcribed but stay walled: force interacts with a prior
        // adjustment (Priadj>1) through a still-unported path (the LOM-prior
        // force case fails at ~2.5e-3 even for target=original), so there is no
        // clean Stocal!=Series gate yet.
        if (frc.iftrgt != 0) {
            x11_not_ported(ctx, "x11pt3 force non-original target (Iftrgt>0)");
            return;
        }
        double stbase[PLEN];
        copy(series, lstfrc, 1, stbase);

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

        // Negative-value correction re-runs qmap2; unreachable on the base path
        // (Cnstnt==DNOTST is guaranteed by the constant-removal guard above).
        if (muladd != 1 && !dpeq(adjc.cnstnt, prm::DNOTST)) {
            x11_not_ported(ctx, "x11pt3 force negative-value correction (qmap2)");
            return;
        }
        // (deferred: D11A/rnd/e6*/p6*/cr/rr/ffc table + punch.)
    } else {
        copy(stci, posffc, 1, stci2);
    }

    if (frc.lrndsa) {
        // Rounded seasonally adjusted series (UK X-11): round Stci2 so each
        // year's rounded values sum to the rounded annual total (x11pt3.f:881).
        bool rndok = false;
        rndsa(stci2, ctx.adxser.stcirn.data(), pos1ob, posfob, ny,
              ctx.x11opt.kdec, rndok);
        // (deferred: rnd table/punch; residual ftest on Stcirn; ssrit/getrev
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
    if (pu.nustad > 0 && pri.lprntr) {
        x11_not_ported(ctx, "x11pt3 D12 user temporary-adjustment fold-in (Nustad)");
        return;
    }
    bool have_stc2 = false;
    if (((!adj.finls) && adj.adjls == 1) ||
        ((!adj.fintc) && lttc && adj.adjtc == 1)) {
        copy(stc + (pos1bk - 1), posffc - pos1bk + 1, 1, stc2 + (pos1bk - 1));
        if ((!adj.finls) && adj.adjls == 1)
            addmul(stc2, facls, stc, pos1bk, posffc, muladd);
        if ((!adj.fintc) && lttc && adj.adjtc == 1)
            addmul(stc2, factc, stc2, pos1bk, posffc, muladd);
        have_stc2 = true;
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
            stcime[i - 1] = have_stc2 ? stc2[i - 1] : stc[i - 1];  // LS -> folded trend
            stime[i - 1] = ebar;          // expected irregular
            stome[i - 1] = series[i - 1] / sti[i - 1];  // muladd==0
            // (nadj2==0 base: no Sprior fold; Finls==F: no Facls divide.)
        }
    }
    // x11pt3.f:1265-1273 -- if AO/TC outliers were removed pre-adjustment, remove
    // them from the modified original (E1=Stome) and, unless kept final, the
    // modified SA (E2=Stcime). The modified irregular (E3=Stime) is left untouched
    // (the oracle comments out its divide). LS/user E-folds stay unported.
    // Finls removes the LS from the modified SA (x11pt3.f:1247-1253); the adjls
    // (non-final) LS lives in the folded trend stc2 (used above), so it needs no
    // Part-E fold. Adjusr does NOT fold into E1/E2 here -- it only triggers the
    // deferred sp2 -> Sprior writeback (x11pt3.f:1284-1288, Kfmt/A18), which does
    // not feed the E-tables; so it passes through.
    if (adj.finls) {
        x11_not_ported(ctx, "x11pt3 Part-E Finls re-adjustment (Facls)");
        return;
    }
    if (adj.adjao == 1) {
        divsub(stome, stome, facao, pos1bk, posffc, muladd);
        if (!adj.finao) divsub(stcime, stcime, facao, pos1bk, posffc, muladd);
    }
    if (adj.adjtc == 1) {
        divsub(stome, stome, factc, pos1bk, posffc, muladd);
        if (!adj.fintc) divsub(stcime, stcime, factc, pos1bk, posffc, muladd);
    }
    // (Cnstnt==DNOTST base: no constant subtract; the sp2/Sprior outlier writeback
    // x11pt3.f:1284-1288 feeds only deferred downstream tables -- skipped.)

    // Store the AO/TC-restored published D13 (sti2, built above) so the harness
    // and D13 table read it as Sti. Base path (no outlier) leaves srs.sti as the
    // already-published D13, untouched.
    if (have_sti2)
        copy(sti2buf + (pos1bk - 1), posffc - pos1bk + 1, 1, sti + (pos1bk - 1));
    if (have_stc2)
        copy(stc2 + (pos1bk - 1), posffc - pos1bk + 1, 1, stc + (pos1bk - 1));
}

}  // namespace x13
