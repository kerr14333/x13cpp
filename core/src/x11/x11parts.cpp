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
#include "x11/x11filt.hpp"          // divsub, addmul, setmv
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

}  // namespace x13
