// slidingspans.cpp -- see hpp for the design note and per-function scope.
#include "x11/slidingspans.hpp"

#include "common/x13context.hpp"
#include "driver/run_x11_span.hpp"
#include "specparse/specparse.hpp"   // dfdate, addate, copy, copylg
#include "numeric/numeric.hpp"       // dpeq
#include "gen/notset.hpp"            // prm::NOTSET, prm::DNOTST
#include "gen/model.hpp"             // prm::PARIMA
#include "x13/farray.hpp"            // farray2

#include <algorithm>
#include <cmath>

namespace x13 {

namespace {
constexpr int MXCOL = 4;
constexpr int MXLEN = 276;
}  // namespace

// sfmax.f
int sfmax_span(int lterm, const int* lter, int ny) {
    int sfmax = lterm;
    if (sfmax == 6 || sfmax == 5)
        sfmax = 0;
    else if (sfmax == 7)
        sfmax = -1;
    bool lstabl = true;
    for (int i = 2; i <= ny; ++i) {
        if (lter[i - 1] == 0 && sfmax < 1)
            sfmax = 2;
        else if (lter[i - 1] > sfmax && lter[i - 1] < 5)
            sfmax = lter[i - 1];
        lstabl = lstabl && (lter[i - 1] == 5);
    }
    if (lstabl) sfmax = 5;
    return sfmax;
}

// getsma.f -- the first-order seasonal MA parameter of the fitted model, or 0
// when the model has none. Ported asymmetry, verbatim: the lag scan finds the
// lag equal to Sp but then returns `Arimap(beglag)`, the parameter of the
// operator's FIRST lag, not of the lag it matched. The two coincide whenever
// the seasonal MA operator leads with lag Sp -- which is every model this
// corpus fits -- so the difference is unobservable here; it is transcribed as
// written rather than "fixed".
double getsma(X13Context& ctx) {
    const model_cmn& m = ctx.model;
    const mdldat_cmn& d = ctx.mdldat;
    const int begopr = m.mdl(prm::MA - 1);
    const int endopr = m.mdl(prm::MA) - 1;
    if (begopr > endopr) return 0.0;
    std::string ttl;
    int ntmp = 0;
    for (int iopr = begopr; iopr <= endopr; ++iopr) {
        getstr(ctx, m.oprttl.data(), m.oprptr.data(), m.noprtl, iopr, ttl, ntmp);
        if (ctx.error.lfatal) return 0.0;
        if (ttl != "Seasonal MA") continue;
        const int beglag = m.opr(iopr - 1);
        const int endlag = m.opr(iopr) - 1;
        for (int ilag = beglag; ilag <= endlag; ++ilag)
            if (m.arimal(ilag) == m.sp) return d.arimap(beglag);
    }
    return 0.0;
}

// mdssln.f -- the SEATS sliding-span length, from Findley (2003): the closer
// the seasonal MA is to non-invertibility the longer a span has to be to say
// anything about the seasonal factors. Falls through to 19 years.
int mdssln(X13Context& ctx, int sp) {
    static const double smalim[15] = {0.16,  0.325, 0.49, 0.535, 0.62,
                                       0.64,  0.695, 0.71, 0.75,  0.76,
                                       0.795, 0.805, 0.84, 0.85,  0.91};
    const double sma = getsma(ctx);
    for (int i = 1; i <= 15; ++i)
        if (sma < smalim[i - 1]) return (i + 3) * sp;
    return 19 * sp;
}

// ssprep.f, scoped to Lx11=true always and Lx11rg=false (see hpp). The
// Nb>0/Ngrp>0 regression-snapshot fields (Ngr2/Ncxy2/Nbb/Colttl/.../Regfx2/
// Rgv2) are NOT copied -- out of scope (no regression{} in the gate corpus;
// a future TD/regression slidingspans spec needs this extended).
void ssprep_snapshot(X13Context& ctx) {
    ssprep_cmn& p = ctx.ssprep;
    const x11opt_cmn& opt = ctx.x11opt;

    for (int i = 1; i <= 12; ++i) p.lt2(i) = opt.lter(i);
    p.ktc2 = opt.ktcopt;
    p.tc2 = opt.tic;
    // Runs before the main run's x11int/x11pt2 (run_x11.cpp), so xtrm.ksdev is
    // still the parsed spec/default value -- stash it for each span's fresh
    // start (run_x11_span). Not an oracle ssprep.cmn field (kept in ctx.saved).
    ctx.saved.ksdev0 = ctx.xtrm.ksdev;
    // Same rationale for the seasonal-filter selector (Lterm) and Henderson
    // trend-filter length (Nterm): captured here as parsed, before x11pt2/vtc
    // resolve them, so history{}'s expanding-window replay can re-select per
    // span. (slidingspans uses fixed-length spans and does not read these back,
    // so capturing them is harmless there.)
    ctx.saved.lterm0 = ctx.x11opt.lterm;
    ctx.saved.nterm0 = ctx.x11opt.nterm;

    if (!ctx.captured.has_model) return;
    const model_cmn& m = ctx.model;
    const mdldat_cmn& d = ctx.mdldat;
    // ssprep.f:56-62 -- Priadj is NOT a plain snapshot. x11pt2's tdlom NEGATES
    // it after folding the length-of-month/leap-year prior into the model TD
    // factor, so that nothing downstream removes the prior a second time; the
    // negated value must never become the snapshot, or every span replay would
    // restore a Priadj<=0 and skip the fold. When tdlom has already run
    // (Priadj<=0 on the aictest-td log path) the flow is the other way round --
    // the LIVE value is restored FROM the snapshot.
    if (ctx.picktd.picktd && ctx.arima.fcntyp == 1 && ctx.prior.priadj <= 0)
        ctx.prior.priadj = p.pri2;
    else
        p.pri2 = ctx.prior.priadj;
    copy(d.arimap.data(), prm::PARIMA, 1, p.ap2.data());
    copylg(m.arimaf.data(), prm::PARIMA, 1, p.fxa.data());
    // ssprep.f:81-95 -- the REGRESSION half. Previously skipped as "Nb==0", true
    // of every span-replay spec in the corpus until one carried a regression{}
    // group: without it each span starts its regression from whatever the
    // PREVIOUS span converged to instead of from the main run, and the spans
    // drift (measured 8.4e-3 in sfs on airline_slidingspans-td).
    copy(d.b.data(), prm::PB, 1, p.bb.data());
    copylg(m.regfx.data(), prm::PB, 1, p.regfx2.data());
    p.irfx2 = m.iregfx;
    // ssprep.f:64-76/80 -- the design DICTIONARY. Inert on every path that
    // leaves the regression structure alone (restor puts back exactly what was
    // there), and required by the two that do not: rmotrv deletes the outliers
    // dated after the first revision date before the span loop and chkorv adds
    // them back as the spans grow (driver/rev_outlier.cpp). Without it every
    // span's restor would reinstate the main run's Nb/Colttl and undo both.
    p.ngr2 = m.ngrp;
    p.ngrt2 = m.ngrptl;
    p.ncxy2 = m.ncxy;
    p.nct2 = m.ncoltl;
    p.cttl = m.colttl.raw();
    p.gttl = m.grpttl.raw();
    cpyint(m.colptr.data(), prm::PB + 1, 1, p.clptr.data());
    cpyint(m.grp.data(), prm::PGRP + 1, 1, p.g2.data());
    cpyint(m.grpptr.data(), prm::PGRP + 1, 1, p.gptr.data());
    cpyint(m.rgvrtp.data(), prm::PB, 1, p.rgv2.data());
    p.nr2 = ctx.arima.nrxy;
    p.nbb = m.nb;
    p.v2 = d.var;
    p.nintv2 = m.nintvl;
    p.nextv2 = m.nextvl;
    p.mxdfl2 = m.mxdflg;
    p.mxarl2 = m.mxarlg;
    p.mxmal2 = m.mxmalg;
    p.lma2 = m.lma;
    p.lar2 = m.lar;
    p.dtcv2 = d.lndtcv;
}

// restor.f, same scope as ssprep_snapshot.
void restor_span(X13Context& ctx) {
    const ssprep_cmn& p = ctx.ssprep;
    x11opt_cmn& opt = ctx.x11opt;

    for (int i = 1; i <= 12; ++i) opt.lter(i) = p.lt2(i);
    opt.ktcopt = p.ktc2;
    opt.tic = p.tc2;

    if (!ctx.captured.has_model) return;
    model_cmn& m = ctx.model;
    mdldat_cmn& d = ctx.mdldat;
    // restor.f:55 -- put Priadj back to its PRE-tdlom value, so this span's
    // x11pt2 folds the length-of-month/leap-year prior into Factd again. The
    // main run's x11pt2 left it negated (see ssprep_snapshot); without this the
    // replay's Factd carries no prior at all and every February of the span's
    // D11/D16 is off by exactly the leap-year factor (0.9912 / 1.0265).
    ctx.prior.priadj = p.pri2;
    copy(p.ap2.data(), prm::PARIMA, 1, d.arimap.data());
    copylg(p.fxa.data(), prm::PARIMA, 1, m.arimaf.data());
    // restor.f:66-70 -- the regression half, the counterpart of the ssprep
    // block above. Every span must start from the MAIN run's regression state,
    // not the previous span's.
    copy(p.bb.data(), prm::PB, 1, d.b.data());
    m.iregfx = p.irfx2;
    copylg(p.regfx2.data(), prm::PB, 1, m.regfx.data());
    // restor.f:50-64 -- the design dictionary half of the same pair. See the
    // note in ssprep_snapshot: byte-identical on every path that does not
    // change the regression structure between spans.
    m.ngrp = p.ngr2;
    m.ngrptl = p.ngrt2;
    m.ncxy = p.ncxy2;
    m.nb = p.nbb;
    m.ncoltl = p.nct2;
    m.colttl = p.cttl.raw();
    m.grpttl = p.gttl.raw();
    cpyint(p.clptr.data(), prm::PB + 1, 1, m.colptr.data());
    cpyint(p.g2.data(), prm::PGRP + 1, 1, m.grp.data());
    cpyint(p.gptr.data(), prm::PGRP + 1, 1, m.grpptr.data());
    cpyint(p.rgv2.data(), prm::PB, 1, m.rgvrtp.data());
    ctx.arima.nrxy = p.nr2;
    d.var = p.v2;
    m.nintvl = p.nintv2;
    m.nextvl = p.nextv2;
    m.mxdflg = p.mxdfl2;
    m.mxarlg = p.mxarl2;
    m.mxmalg = p.mxmal2;
    m.lma = p.lma2;
    m.lar = p.lar2;
    d.lndtcv = p.dtcv2;
}

// ssmdl.f: the Ssinit==1 "fix all model parameters" tail, both halves (ARMA and
// regression). The earlier Nb==0 scoping was only ever true because no
// span-replay spec carried a regression{} group.
void ssmdl_fix_model(X13Context& ctx) {
    sspinp_cmn& si = ctx.sspinp;
    if (ctx.model.nb <= 0) si.nssfxr = 0;
    if (si.ssotl <= 1) {
        ctx.arima.ltstao = false;
        ctx.arima.ltstls = false;
        ctx.arima.ltsttc = false;
    }
    if (si.ssinit == 1) {
        // ssmdl.f:343-344 `CALL copy(Arimap,PARIMA,1,Ap2); CALL setlg(T,PARIMA,
        // Fxa)` -- Ap2/Fxa here are ssprep.cmn's SNAPSHOT fields, not the live
        // model.cmn Arimap/Arimaf. This is the mechanism that makes the fix
        // stick across every span: restor_span() (ssx11a.f's CALL restor(...),
        // run before EACH span) resets the LIVE Arimap/Arimaf FROM this
        // snapshot every time, so fixing only the live copy here would be
        // silently undone by the very next restor_span() call. Also fix the
        // live copy so it is already correct before the first restor_span().
        copy(ctx.mdldat.arimap.data(), prm::PARIMA, 1, ctx.ssprep.ap2.data());
        for (int i = 1; i <= prm::PARIMA; ++i) {
            ctx.ssprep.fxa(i) = true;
            ctx.model.arimaf(i) = true;
        }
        // ssmdl.f:345-350 -- the REGRESSION half of the same fix. `regchg` (the
        // outlier-regressor-changed path) is not reachable here, so the B
        // snapshot is unconditional. Snapshot AND live copy, for the same reason
        // as Arimap/Arimaf above.
        copy(ctx.mdldat.b.data(), prm::PB, 1, ctx.ssprep.bb.data());
        for (int i = 1; i <= prm::PB; ++i) ctx.ssprep.regfx2(i) = true;
        if (ctx.model.iregfx < 3) ctx.model.iregfx = 3;
        for (int i = 1; i <= ctx.model.nb; ++i) ctx.model.regfx(i) = true;
        ctx.ssprep.irfx2 = 3;
        // (bakusr/Userfx needs user-defined regressors, which this driver's
        // scope excludes.)
        if (!ctx.model.userfx) ctx.model.userfx = ctx.usrreg.ncusrx > 0;
    }
}

// setssp.f, scoped per the hpp header.
bool setssp_span(X13Context& ctx, int ltmax, bool lmodel, bool lseats,
                  bool lncset, bool lnlset) {
    sspinp_cmn& si = ctx.sspinp;
    ssap_cmn& sa = ctx.ssap;
    hiddn_cmn& hid = ctx.hiddn;
    x11opt_cmn& opt = ctx.x11opt;

    if (sa.itd != 1 && (ctx.x11log.axrgtd || ctx.x11adj.adjtd > 0)) sa.itd = 1;
    if (sa.ihol != 1 && (ctx.x11log.axrghl || ctx.x11adj.adjhol > 0)) sa.ihol = 1;
    if (sa.itd == 1 && ctx.x11adj.adjtd > 0 && si.ssinit == 1) sa.itd = -1;
    if (sa.ihol == 1 && ctx.x11adj.adjhol > 0 && si.ssinit == 1) sa.ihol = -1;
    if (sa.ihol == 1 && !ctx.x11adj.finhol) sa.ihol = 0;
    if (opt.muladd != 1 && si.ssdiff) si.ssdiff = false;

    const int length = opt.length;
    const int ny = opt.ny;
    const int pos1 = ctx.x11ptr.pos1ob;
    const int pos2 = ctx.x11ptr.posfob;

    if (lncset && lnlset) {
        if (length < (si.nlen + (si.ncol - 1) * ny)) {
            hid.issap = 0;
        }
    } else if (ltmax == 5) {
        // Stable-seasonal branch -- not reachable by the gate corpus (Lterm
        // resolves to an MSR-selected 1/2/3 for airline-like monthly data,
        // never 5); not ported (would need faithful porting + a golden with
        // seasonalma=s3x1/stable to validate against).
        hid.issap = 0;
    } else {
        static const int nl[6] = {6, 6, 7, 8, 11, 17};   // nl(-1:4)
        if (!lnlset) {
            if (lseats) {
                si.nlen = mdssln(ctx, ny);   // setssp.f:151-152
            } else {
                si.nlen = nl[ltmax + 1] * ny;
            }
        }
        if (hid.issap != 0) {
            int ncmax = ((length - si.nlen) / ny) + 1;
            if (lncset) {
                if (ncmax < si.ncol) hid.issap = 0;
            } else {
                if (ncmax > 4)
                    si.ncol = 4;
                else if (ncmax < 2)
                    hid.issap = 0;
                else
                    si.ncol = ncmax;
            }
        }
    }
    if (hid.issap == 0) {
        si.ncol = 0;
        si.nlen = 0;
        return false;
    }

    sa.sslen = si.nlen + (si.ncol - 1) * ny;

    int im, iyr;
    if (length == sa.sslen) {
        im = pos1 % ny;
        iyr = opt.lyr + (pos1 / ny);
        if (im == 0) { im = ny; iyr -= 1; }
    } else {
        int l2 = length - (si.nlen + (si.ncol - 1) * ny);
        iyr = opt.lyr + (pos1 + l2) / ny;
        im = (pos1 + l2) % ny;
        if (im == 0) { im = ny; iyr -= 1; }
        if (!lnlset && ltmax < 4) {
            int lyr0 = opt.lyr + (pos1 / ny);
            int pos0 = pos1 % ny;
            if (pos0 == 0) { pos0 = ny; lyr0 -= 1; }
            if ((pos0 == 1 && im != 1) || (pos0 < im && iyr == lyr0)) {
                si.nlen = (im - pos0) + si.nlen;
                im = pos0;
            } else if (im > 1 && iyr > lyr0) {
                si.nlen = (im - 1) + si.nlen;
                im = 1;
            }
        }
        sa.sslen = si.nlen + (si.ncol - 1) * ny;
    }
    sa.iyr = iyr;
    sa.im = im;

    // First month/year of the sliding-spans comparisons (Ic/Icm/Icyr).
    if (si.strtss(1) == prm::NOTSET) {
        si.strtss(1) = iyr + 1;
        si.strtss(2) = im;
        sa.icm = im;
        sa.icyr = iyr + 1;
        sa.ic = im + ny;
    } else {
        int frstsp[2] = {iyr, im};
        int i = 0;
        dfdate(si.strtss.data(), frstsp, ny, i);
        if (i >= ny) {
            sa.icm = si.strtss(2);
            sa.icyr = si.strtss(1);
            sa.ic = (sa.icyr - iyr) * ny + sa.icm;
        } else {
            sa.icm = im;
            sa.icyr = iyr + 1;
            sa.ic = im + ny;
            si.strtss(2) = sa.icm;
            si.strtss(1) = sa.icyr;
        }
    }

    // Number of sliding-spans comparisons per estimate kind (Itot).
    int begss[2] = {iyr, im};
    int endss[2];
    addate(begss, ny, sa.sslen - ny, endss);
    int nmcomp = 0;
    dfdate(endss, si.strtss.data(), ny, nmcomp);
    for (int i = 1; i <= 5; ++i) {
        if (i <= 3)
            sa.itot(i) = nmcomp;
        else if (i == 4)
            sa.itot(i) = nmcomp - 1;
        else
            sa.itot(i) = nmcomp - ny;
    }

    // Backcast-start alignment (Nbcst2).
    const int nbcst = ctx.extend.nbcst;
    int ssbak[2];
    addate(begss, ny, -nbcst, ssbak);
    ctx.extend.nbcst2 = (ssbak[1] > 1) ? (nbcst + ssbak[1] - 1) : nbcst;

    // TD/holiday downgrade for short spans.
    if (si.nlen < 5 * ny) {
        if (sa.itd == 1) sa.itd = -2;
        if (sa.ihol == 1) sa.ihol = -2;
    }

    sa.nsea = ny;
    ctx.lzero.l0 = pos2 - (si.nlen + im - 2 + (si.ncol - 1) * ny);

    // (Nssfxr>0 fixreg=/Ssfxxr bookkeeping: not reachable, the gate corpus
    // has no fixreg= argument, Nssfxr==0.)

    if (lmodel) ssmdl_fix_model(ctx);
    // setssp.f:353-356's `IF(Nbx.gt.0) CALL ssxmdl(...)` -- NOT ported.
    //
    // This used to read "out of scope, Nbx==0 always in this port", which was
    // true when x11regression{} was unported and false ever since. Nothing
    // caught it because NO corpus spec combined slidingspans{} with
    // x11regression{} -- checked 2026-08-04, and that is the whole reason this
    // stood. extra/airline_slidingspans-x11regression is that spec now.
    //
    // What ssxmdl decides, in the order it decides it: whether an
    // x11regression{span=} forces Ssxint (every coefficient held fixed for the
    // spans, plus a NOTE); rvfixd's Tdfix/Holfix; whether Itd/Ihol are demoted
    // to -1 so the TD/holiday span analyses do not run; and ssxmdl.f:153's
    // Lxrneg reset. On the gated spec every one of those is inert -- no span=,
    // nothing fixed, Irgxfx==1 -- which is why the spec gates bit-exact on
    // sfs and every D-table. It is NOT why `chs` is wrong; see below.

    return true;
}

// ssrit.f, scoped to non-composite (Iagr!=2 -- the Indssp/Saind/Sfind/Sfinda
// composite bookkeeping is dead code for a single-series run).
void ssrit(X13Context& ctx, const double* x, int l1, int l2, int isec,
           const double* series) {
    (void)series;   // Isfadd numerator: Muladd==1 only, never for this port's
                     // log/mult-mode gate corpus.
    ssap_cmn& sa = ctx.ssap;
    sspdat_cmn& d = ctx.sspdat;
    const int muladd = ctx.x11opt.muladd;
    const int icol = ctx.ssft.icol;
    constexpr double ONEHND = 100.0;
    const double DNOTST = prm::DNOTST;

    int l0 = ctx.lzero.l0;
    if (isec == 0) l0 += sa.nsea;

    int l10 = l1 - l0 + 1;
    if (l10 != 1) {
        for (int i = 1; i <= l10 - 1; ++i) {
            if (isec <= 1) d.td(i, icol) = DNOTST;
            if (isec == 2) d.s(i, icol) = DNOTST;
            if (isec == 3) d.sa(i, icol) = DNOTST;
        }
    }
    for (int i = l1; i <= l2; ++i) {
        int i0 = i - l0 + 1;
        if (isec <= 1)
            d.td(i0, icol) = (muladd == 1) ? x[i - 1] : x[i - 1] * ONEHND;
        if (isec == 2)
            d.s(i0, icol) = (muladd == 1) ? x[i - 1] : x[i - 1] * ONEHND;
        if (isec == 3) {
            d.sa(i0, icol) = x[i - 1];
            // (Muladd==1 Isfadd / Iagr==2 indirect bookkeeping: out of scope.)
        }
    }
    int ll0 = sa.sslen + sa.im + l0 - 2;
    int l20 = ll0;
    if (ll0 % sa.nsea != 0) l20 = ((ll0 / sa.nsea) + 1) * sa.nsea;
    if (l20 == l2 && ctx.x11opt.lstmo < sa.nsea) l20 = (sa.nsea - ctx.x11opt.lstmo) + l20;
    if (l20 != l2) {
        for (int i = l2 + 1; i <= l20; ++i) {
            int i0 = i - l0 + 1;
            if (isec <= 1) d.td(i0, icol) = DNOTST;
            if (isec == 2) d.s(i0, icol) = DNOTST;
            if (isec == 3) d.sa(i0, icol) = DNOTST;
        }
    }
    if (isec == 0) l0 -= sa.nsea;
    (void)l0;
}

namespace {

// xchng.f: change of the Sa series over Nchng periods (1 = month-to-month),
// absolute (ldiff) or percent, into c (MXLEN x MXCOL, column-major flat).
void xchng(const x13::farray2<double, MXLEN, MXCOL>& x, std::vector<double>& c,
           int ncol, int im, int sslen, int nchng, bool ldiff) {
    c.assign(static_cast<std::size_t>(MXLEN) * MXCOL, prm::DNOTST);
    auto C = [&](int row, int col) -> double& {
        return c[static_cast<std::size_t>((row - 1) + (col - 1) * MXLEN)];
    };
    for (int i = 1; i <= ncol; ++i) {
        for (int iyy = 1; iyy <= sslen + im - 1; ++iyy) {
            C(iyy, i) = prm::DNOTST;
            int iyy2 = iyy - nchng;
            if (!dpeq(x(iyy, i), prm::DNOTST) && iyy2 > 0) {
                if (!dpeq(x(iyy2, i), prm::DNOTST)) {
                    double v = x(iyy, i) - x(iyy2, i);
                    if (!ldiff) v = (v / std::fabs(x(iyy2, i))) * 100.0;
                    C(iyy, i) = v;
                }
            }
        }
    }
}

// rplus.f, scoped to the sfs/chs gate (no Lsaneg/turning-point/sign-change
// bookkeeping -- those feed the histogram/breakdown tables, out of scope; see
// hpp). Computes the cross-span max-%-difference (or max-difference, when
// nop2!=0) for row i.
double rplus_mpd(const double* row /*1-based, ncol entries*/, int ncol, int nop2,
                  bool ssdiff) {
    int j = 0;
    while (j < ncol && dpeq(row[j], prm::DNOTST)) ++j;
    if (j >= ncol) return prm::DNOTST;   // no span covers this row at all
    double xmx = row[j], xmn = row[j];
    for (int j2 = j; j2 < ncol; ++j2) {
        if (!dpeq(row[j2], prm::DNOTST)) {
            if (xmx < row[j2]) xmx = row[j2];
            if (xmn > row[j2]) xmn = row[j2];
        }
    }
    double mpd = xmx - xmn;
    if (!ssdiff && nop2 == 0) {
        // (Lsaneg mixed-sign additive-SA branch: Muladd==1 only, out of scope
        // for this port's log/mult-mode gate corpus.)
        if (xmn > 0.0)
            mpd = (mpd / xmn) * 100.0;
        else
            mpd = (mpd / std::fabs(xmx)) * 100.0;
    }
    return mpd;
}

// mflag.f, scoped to the sfs/chs gate: just the per-row Dmax (rplus_mpd),
// gated by the Km/Sslen2 "observation common to <2 spans" trim -- the
// histogram/breakdown (Kount/Ayr/Aobs/Chsgn/Iturn) side-tables are not
// ported (see hpp).
void mflag(const x13::farray2<double, MXLEN, MXCOL>& x, int nop2, int km,
           int sslen2, int ncol, bool ssdiff, std::vector<double>& dmax) {
    dmax.assign(MXLEN, prm::DNOTST);
    double row[MXCOL];
    for (int i = 1; i <= MXLEN; ++i) {
        if (i <= km || i >= sslen2) {
            dmax[static_cast<std::size_t>(i - 1)] = prm::DNOTST;
            continue;
        }
        for (int j = 1; j <= ncol; ++j) row[j - 1] = x(i, j);
        dmax[static_cast<std::size_t>(i - 1)] = rplus_mpd(row, ncol, nop2, ssdiff);
    }
}

void mflag_c(const std::vector<double>& c, int nop2, int km, int sslen2,
             int ncol, bool ssdiff, std::vector<double>& dmax) {
    dmax.assign(MXLEN, prm::DNOTST);
    double row[MXCOL];
    for (int i = 1; i <= MXLEN; ++i) {
        if (i <= km || i >= sslen2) {
            dmax[static_cast<std::size_t>(i - 1)] = prm::DNOTST;
            continue;
        }
        for (int j = 1; j <= ncol; ++j)
            row[j - 1] = c[static_cast<std::size_t>((i - 1) + (j - 1) * MXLEN)];
        dmax[static_cast<std::size_t>(i - 1)] = rplus_mpd(row, ncol, nop2, ssdiff);
    }
}

}  // namespace

bool run_slidingspans(X13Context& ctx, const std::vector<double>& trnsrs_full) {
    ctx.ssout = SlidingSpansOutput{};
    hiddn_cmn& hid = ctx.hiddn;
    if (hid.issap != 1) return true;   // slidingspans{} not requested/parsed

    sspinp_cmn& si = ctx.sspinp;
    ssap_cmn& sa = ctx.ssap;

    const bool lncset = si.ncol > 0;
    const bool lnlset = si.nlen > 0;
    const int ltmax = sfmax_span(ctx.x11opt.lterm, ctx.x11opt.lter.data(),
                                  ctx.x11opt.ny);

    // sspdrv.f:66 passes Lseats through to setssp, and :121/:180 pass it on to
    // ssx11a/x11ari -- a SEATS spec's spans differ from an X-11 spec's only in
    // which adjustment routine runs after x11pt2 (seatdg's ssrit store instead
    // of x11pt3's). Nothing else here is X-11-specific.
    const bool lseats = ctx.captured.has_seats && !ctx.captured.has_x11;
    if (!setssp_span(ctx, ltmax, ctx.captured.has_model, lseats,
                      lncset, lnlset) || hid.issap == 0) {
        hid.issap = 0;
        return true;   // "not enough data" -- clean skip, not FATAL
    }
    hid.issap = 2;

    // Replay loop (sspdrv.f). The gate corpus has no fixreg{}/regression{}/
    // outlier{}, so the user-regressor deletion/restore (chusrg/bakusr) and
    // automatic-outlier removal (dlrgef/ssprep) blocks sspdrv.f runs between
    // spans are all dead code here (Ncusrx==Nusxrg==Notrtl==0) -- not ported.
    const bool has_model = ctx.captured.has_model;
    const int nfcst = ctx.extend.nfcst;
    const int nbcst = ctx.extend.nbcst;
    const int nbcst2 = ctx.extend.nbcst2;
    const int ny = sa.nsea;
    const int l0 = ctx.lzero.l0;
    for (int j = 1; j <= si.ncol; ++j) {
        ctx.ssft.icol = j;
        restor_span(ctx);
        if (si.ssinit == 2) {
            for (int i = 1; i <= prm::PARIMA; ++i)
                if (!ctx.model.arimaf(i)) ctx.mdldat.arimap(i) = prm::DNOTST;
            // (Nb==0: the B/Bx DNOTST reset lines are no-ops.)
        }
        // NOT PORTED, and MEASURED: ssx11a.f:93-95's `Ixreg=1; IF(Lmodel)Ixreg=2`
        // demote -- the slidingspans twin of revdrv.f:530-532, which run_history
        // now does. Adding it here makes results WORSE, so the oracle must reach
        // the same numbers by another route: on airline + x11regression{
        // variables=(td)} + a regARIMA model, `sfs` is currently BIT-EXACT
        // (4.7e-15) against the oracle with Ixreg left at 3, and demoting takes it
        // to 4.1e+0. Whatever ssx11a's demote costs is evidently paid back inside
        // sspdrv (Ssinit/Ssxint hold the irregular regression across spans), which
        // is not ported. Do not "fix" this by copying the history change.
        // Still open and separately wrong on this family: `chs` (5.0e+0) and the
        // whole MODEL-FREE case (sfs 2.0e+2). Neither moves with the demote, so
        // neither is this seam.
        //
        // The `chs` half was attributed HERE to "the same per-span prior-phase
        // problem airline_slidingspans-td already records". That attribution is
        // WRONG, and cheap-spec-vs-expensive-spec is what showed it: delete
        // `x11regression{}` and chs goes 0/600 different -- BIT-EXACT -- while
        // deleting `transform{function=log}`, which is what creates the lom /
        // leap-year prior the other spec's gap is about, leaves all 408 of 600
        // wrong cells exactly where they were. The two gaps share a symptom and
        // not an owner.
        //
        // What IS measured about this one: `sfs` is bit-exact and `chs` is not,
        // so the per-span SEASONAL factors are right and the per-span
        // SEASONALLY ADJUSTED series is not -- i.e. the per-span calendar
        // factor. The engine's chs does respond to x11regression (427 of 600
        // cells move when the spec drops it), so the irregular regression is
        // running per span; it lands nearer the no-TD answer than the oracle's,
        // so it is running on the wrong inputs. ssx11a.f:96-97's per-span
        // Begxrg/Endxrg was the obvious candidate and has been ported (see
        // run_x11_span's set_xrg_span) -- it is measurably inert, so that is not
        // it either. Recorded as a KNOWN GAP in test_slidingspans_tables.py with
        // the golden committed.
        const int lsp = l0 + (j - 1) * ny + sa.im - nbcst2 - 1;
        if (!run_x11_span(ctx, trnsrs_full, has_model, si.nlen, nfcst, nbcst,
                           nbcst2, lsp, /*nend_mdl=*/0, lseats,
                           /*set_xrg_span=*/true))
            return false;
        if (ctx.error.lfatal) return false;
    }
    hid.issap = 3;

    // ssap.f's cross-span diagnostics, scoped to S (sfs) and c=Sa month-to-
    // month change (chs) -- see hpp for exactly what is/isn't ported.
    const int im = sa.im, sslen = sa.sslen;
    sa.sslen2 = sslen - ny + im;
    sa.ns1 = si.ncol + 1;

    xchng(ctx.sspdat.sa, ctx.ssout.c_flat, si.ncol, im, sslen, 1, si.ssdiff);

    const int iobs_s = im + ny - 1;         // Km for S (mflag.f's iobs before +1)
    const bool ssdiff = si.ssdiff;
    const int muladd = ctx.x11opt.muladd;
    if (muladd == 0 && ctx.x11opt.kfulsm == 0)
        mflag(ctx.sspdat.s, /*nop2=*/0, iobs_s, sa.sslen2, si.ncol, ssdiff,
              ctx.ssout.dmax_sfs);
    else
        ctx.ssout.dmax_sfs.assign(MXLEN, prm::DNOTST);

    // ssap.f:209-210 / :219-222 -- the Sa (ads) table. It is NOT unconditional:
    // the seasonal factors alone move every span, but the SA series only picks
    // up a separately-flagged difference when something outside the seasonal
    // factor is being re-estimated per span, so the oracle emits ads only with a
    // trading-day / holiday / rounding / force option live. The Sa call shares
    // `iobs` with the S call above -- ssap.f does not bump iobs until after it.
    const bool ads_on =
        (ctx.x11opt.kfulsm == 0 &&
         (ctx.force.lrndsa || ctx.force.iyrt > 0 || sa.itd == 1)) ||
        sa.ihol == 1;
    // (The Muladd!=0 branch's own `IF(Ssdiff) ... ELSE mflag(Sa)` fallback --
    // which flags Sa unconditionally when Ssdiff is off -- is not reached by
    // this port's mult/log gate corpus and is deliberately not wired.)
    if (muladd == 0 && ctx.agr.iagr < 6 && ads_on) {
        mflag(ctx.sspdat.sa, /*nop2=*/0, iobs_s, sa.sslen2, si.ncol, ssdiff,
              ctx.ssout.dmax_ads);
        ctx.ssout.have_ads = true;
    } else {
        ctx.ssout.dmax_ads.assign(MXLEN, prm::DNOTST);
        ctx.ssout.have_ads = false;
    }

    const int io1 = (ny == 4) ? 2 : 1;
    const int iobs_c = iobs_s + 1;           // mflag.f: iobs=iobs+1 before the c call
    mflag_c(ctx.ssout.c_flat, io1, iobs_c, sa.sslen2, si.ncol, ssdiff,
            ctx.ssout.dmax_chs);

    ctx.ssout.ran = true;
    ctx.ssout.ncol = si.ncol;
    ctx.ssout.sslen = sslen;
    ctx.ssout.im = im;
    ctx.ssout.iyr = sa.iyr;
    ctx.ssout.nsea = ny;
    return true;
}

}  // namespace x13
