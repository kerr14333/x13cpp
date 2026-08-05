// slidingspans.cpp -- see hpp for the design note and per-function scope.
#include "x11/slidingspans.hpp"

#include "common/x13context.hpp"
#include "driver/run_x11_span.hpp"
#include "regarima/outlier.hpp"      // rdotlr (rmotss/adotss)
#include "regarima/rvfixd.hpp"       // rvfixd (fixreg= group walk)
#include "specparse/specparse.hpp"   // dfdate, addate, copy, copylg
#include "numeric/numeric.hpp"       // dpeq
#include "gen/notset.hpp"            // prm::NOTSET, prm::DNOTST
#include "gen/model.hpp"             // prm::PARIMA, PXPX, PGPG, PORDER
#include "gen/srslen.hpp"            // prm::PLEN (PACM below)
#include "x13/farray.hpp"            // farray2

#include <algorithm>
#include <cmath>
#include <string>

namespace x13 {

namespace {
constexpr int MXCOL = 4;
constexpr int MXLEN = 276;

// Local clean-fatal, shaped so tools/walls.py inventories it (x11parts.cpp's
// x11_not_ported is TU-local).
void ssp_not_ported(X13Context& ctx, const char* what) {
    errhdr(ctx);
    writln(ctx, std::string("ERROR: ") + what + " not yet ported (M5 X-11 spine).",
           stdio::STDERR, ctx.units.mt2, true);
    abend(ctx);
}

// The group-level regressor-type tests ssmdl.f:76-93 and ssxmdl.f:88-104 walk
// with. The two lists are NOT the same and the difference is load-bearing, so
// they are transcribed separately rather than shared:
//
//   * ssmdl's GROUP test additionally admits the user-holiday range
//     (PRGTUH..PRGUH5) and the user-seasonal PRGTUS; ssxmdl's does not.
//   * ssmdl's INNER holiday test uses the same PRGTUH..PRGUH5 range;
//     ssxmdl.f:100 spells the single value PRGTUH instead -- and since PRGTUH
//     cannot pass ssxmdl's group test at all, that arm of it is DEAD. Kept
//     verbatim; do not "fix" it into the range.
//
// Both tests are on the type of the group's FIRST column (Rgvrtp(begcol)).
bool ss_is_calendar_group(int rtype, bool with_user_hol) {
    using namespace prm;
    const bool istd =
        rtype == PRGTTD || rtype == PRGTST || rtype == PRRTTD ||
        rtype == PRA1TD || rtype == PRRTST || rtype == PRATTD ||
        rtype == PRATST || rtype == PRG1TD || rtype == PRR1TD ||
        rtype == PRA1ST || rtype == PRG1ST || rtype == PRR1ST;
    const bool islom =
        rtype == PRGTLM || rtype == PRGTSL || rtype == PRGTLQ ||
        rtype == PRGTLY || rtype == PRATLQ || rtype == PRRTLM ||
        rtype == PRRTSL || rtype == PRRTLQ || rtype == PRRTLY ||
        rtype == PRATLM || rtype == PRATSL || rtype == PRATLY;
    const bool isusrtd =
        rtype == PRGUTD || rtype == PRGULM || rtype == PRGULQ ||
        rtype == PRGULY;
    const bool ishol =
        rtype == PRGTEA || rtype == PRGTEC || rtype == PRGTES ||
        rtype == PRGTLD || rtype == PRGTTH;
    const bool isusrhol =
        with_user_hol &&
        ((rtype >= PRGTUH && rtype <= PRGUH5) || rtype == PRGTUS);
    return istd || islom || isusrtd || ishol || isusrhol;
}

// adotss.f's `otypvc`: the Rgvrtp type an outlier of each rdotlr type is re-added
// as. Indexed 1..9 by otltyp (AO LS TC RP MV TLS SO QI QD). Index 5 (MV) maps to
// PRGTAO, verbatim -- same DATA statement as chkorv.f's (driver/rev_outlier.cpp).
int ss_otypvc(int otltyp) {
    using namespace prm;
    switch (otltyp) {
    case 1: return PRGTAO;   // AO
    case 2: return PRGTLS;   // LS
    case 3: return PRGTTC;   // TC
    case 4: return PRGTRP;   // RP
    case 5: return PRGTAO;   // MV  (as written)
    case 6: return PRGTTL;   // TLS
    case 7: return PRGTSO;   // SO
    case 8: return PRGTQI;   // QI
    case 9: return PRGTQD;   // QD
    default: return 0;
    }
}

// adotss.f's `opref`: which outlier type WINS when several land on the last
// observation of a span. DATA opref/1,4,2,0,0,0,3/, indexed 1..7 by otltyp;
// HIGHER wins and the loser's column is deleted.
int ss_opref(int otltyp) {
    static const int pref[8] = {0, 1, 4, 2, 0, 0, 0, 3};
    return (otltyp >= 1 && otltyp <= 7) ? pref[otltyp] : 0;
}

// The design-dictionary half of ssprep.f, re-taken after a structural change so
// the NEXT span's restor keeps it. ssmdl.f:358-373's `IF(regchg)` block and
// sspdrv.f:218's `CALL ssprep(T,F,F)` are the two callers; both follow a
// dlrgef/adrgef that moved columns, and without the re-snapshot the very next
// restor_span would reinstate the main run's Nb/Colttl and undo it.
//
// ssmdl's version writes Bb/Irfx2/Regfx2 as well (:370-372); sspdrv.f:218 goes
// through the full ssprep, which writes those too. So unlike rev_outlier.cpp's
// pair -- where chkorv.f:189-202 pointedly omits the fix flags -- both callers
// here want the same set, and it is written in one place.
void ss_snapshot_design(X13Context& ctx) {
    ssprep_cmn& p = ctx.ssprep;
    const model_cmn& m = ctx.model;
    p.ngr2 = m.ngrp;
    p.ngrt2 = m.ngrptl;
    p.ncxy2 = m.ncxy;
    p.nbb = m.nb;
    p.nct2 = m.ncoltl;
    p.cttl = m.colttl.raw();
    p.gttl = m.grpttl.raw();
    cpyint(m.colptr.data(), prm::PB + 1, 1, p.clptr.data());
    cpyint(m.grp.data(), prm::PGRP + 1, 1, p.g2.data());
    cpyint(m.grpptr.data(), prm::PGRP + 1, 1, p.gptr.data());
    cpyint(m.rgvrtp.data(), prm::PB, 1, p.rgv2.data());
    copy(ctx.mdldat.b.data(), prm::PB, 1, p.bb.data());
    p.irfx2 = m.iregfx;
    copylg(m.regfx.data(), prm::PB, 1, p.regfx2.data());
}

// rmotss.f -- decide what happens to ONE outlier column of the design before the
// sliding-spans loop, and it is a three-way verdict, not a two-way one:
//
//   * dated before the FIRST span starts   -> deleted outright, NOT stored. No
//     span can estimate it and no span will ever want it back.
//   * undefined in one or more spans       -> its title, coefficient and fix
//     flag go into the store (ctx.otlrev) and the column is deleted; each span's
//     adotss re-adds the ones its own window covers.
//   * defined in every span                -> left exactly where it is.
//
// The middle test is per outlier TYPE and the endpoint strictness differs by
// type (AO/TC use `<`/`>` against starta/enda, LS/SO use `<=`/`>=`, RP wants
// BOTH endpoints strictly inside) -- transcribed one clause per line.
//
// `revchg` is the caller's regchg: set whenever the design changed, so the
// caller re-snapshots.
void rmotss(X13Context& ctx, int icol, const int* begxy, int nrxy,
            const int* strtss, const int* starta, const int* enda, bool otlfix,
            bool& revchg) {
    using namespace prm;
    model_cmn& m = ctx.model;
    otlrev_cmn& st = ctx.otlrev;
    const int sp = m.sp;
    const int nreg = st.notrtl + 1;

    std::string str;
    int nchr = 0;
    getstr(ctx, m.colttl.data(), m.colptr.data(), m.ncoltl, icol, str, nchr);
    if (ctx.error.lfatal) return;
    int otltyp = 0, begotl = 0, endotl = 0;
    bool locok = true;
    rdotlr(ctx, str, begxy, sp, otltyp, begotl, endotl, locok);
    if (!locok) { abend(ctx); return; }

    // rmotss.f:34-40 -- before the first span, so gone for good.
    int sspos = 0;
    dfdate(strtss, begxy, sp, sspos);
    sspos += 1;
    if (begotl < sspos) {
        revchg = true;
        dlrgef(ctx, icol, nrxy, 1);
        return;
    }

    // rmotss.f:46-65 -- starta is the LAST span's start, enda the FIRST span's
    // end, so [starta,enda] is the intersection of every span. An outlier
    // outside it is undefined for at least one span.
    int sspos1 = 0, sspos2 = 0;
    dfdate(starta, begxy, sp, sspos1);
    sspos1 += 1;
    dfdate(enda, begxy, sp, sspos2);
    sspos2 += 1;
    const bool undef =
        (otltyp == AO && (begotl < sspos1 || begotl > sspos2)) ||
        (otltyp == LS && (begotl <= sspos1 || begotl >= sspos2)) ||
        (otltyp == SO && (begotl <= sspos1 || begotl >= sspos2)) ||
        (otltyp == TC && (begotl < sspos1 || begotl > sspos2)) ||
        (otltyp == RP && !(begotl > sspos1 && begotl < sspos2));
    if (!undef) return;
    revchg = true;
    insstr(ctx, str.substr(0, static_cast<std::size_t>(nchr)), nreg, PB,
           st.otrttl.data(), static_cast<int>(st.otrttl.size()),
           st.otrptr.data(), st.notrtl);
    if (ctx.error.lfatal) return;
    st.botr(st.notrtl) = ctx.mdldat.b(icol);
    st.fixotr(st.notrtl) = m.regfx(icol) || otlfix;
    dlrgef(ctx, icol, nrxy, 1);
}

// adotss.f -- the per-span counterpart: re-introduce every stored outlier this
// span's window [Frstsy,Lastsy] can estimate. Unlike chkorv (its history{}
// twin) it does NOT consume the store -- the same entries are re-tested for
// every span, and sspdrv.f:208-219 takes the added columns back out afterwards.
//
// `otlfix` is the caller's `Otlfix.or.Ssinit.eq.1` / `Otlfix.or.Ssxint`: an
// outlier re-added into a span whose whole model is held fixed comes back
// FIXED, at the coefficient the main run estimated for it.
void adotss(X13Context& ctx, int lastsy, bool otlfix, int nrxy) {
    using namespace prm;
    model_cmn& m = ctx.model;
    otlrev_cmn& st = ctx.otlrev;
    const int sp = m.sp;
    const int frstsy = ctx.arima.frstsy;
    const int endcol = st.notrtl;
    int nlast = 0;
    bool lastls = false;

    for (int icol = 1; icol <= endcol; ++icol) {
        std::string str;
        int nch = 0;
        getstr(ctx, st.otrttl.data(), st.otrptr.data(), st.notrtl, icol, str,
               nch);
        if (ctx.error.lfatal) return;
        // adotss.f:35-36 -- skip an entry that is ALREADY a group in the design.
        // (strinx over Grpttl, not Colttl: a one-column outlier group's title
        // and its column title are the same string.)
        if (strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl, str) != 0)
            continue;
        int otltyp = 0, begotl = 0, endotl = 0;
        bool locok = true;
        rdotlr(ctx, str, ctx.arima.begsrs.data(), sp, otltyp, begotl, endotl,
               locok);
        // adotss.f:41-49 -- note the bounds are against the SERIES-relative
        // Frstsy/Lastsy (rdotlr was handed Begsrs above), not the design's
        // Begxy, and the strictness differs by type exactly as in rmotss.
        const bool defined =
            ((otltyp == RP || otltyp == TLS || otltyp == QI || otltyp == QD) &&
             begotl >= frstsy && endotl <= lastsy) ||
            ((otltyp == SO || otltyp == LS) && begotl > frstsy &&
             begotl <= lastsy) ||
            ((otltyp == AO || otltyp == TC) && begotl >= frstsy &&
             begotl <= lastsy);
        if (!defined) continue;
        const bool fx = st.fixotr(icol) || otlfix;
        adrgef(ctx, st.botr(icol), str, str, ss_otypvc(otltyp), fx, false);
        if (ctx.error.lfatal) return;
        if (m.iregfx == 3 && !fx) m.iregfx = 2;
        // adotss.f:53-54, transcribed as written -- the same CB-23 shape as
        // chkorv.f:54-58. Census meant `.not.(RP.or.TLS.or.QI.or.QD)`, but
        // `.and.` binds tighter than `.or.`, so
        //   (t/=RP .and. t/=TLS) .or. t/=QI .or. t/=QD
        // is TRUE for every type and the guard collapses to `begotl==Lastsy`.
        // Reproduced; writing the intended test would be improving the Fortran.
        if (begotl == lastsy) {
            nlast += 1;
            if (!lastls) lastls = (otltyp == LS);
        }
    }
    (void)lastls;   // adotss.f computes it and never reads it
    if (nlast <= 1) return;

    // adotss.f:66-101 -- more than one outlier on the final observation is a
    // singular design, so keep the highest-ranked and delete the rest. Walk the
    // DESIGN backwards; `opref` decides which of a pair survives.
    int ltype = 0, ilast = 0, lcol = 0;
    for (int icol = m.nb; icol >= 1; --icol) {
        const int rtype = m.rgvrtp(icol);
        if (!(rtype == PRGTAO || rtype == PRGTAA || rtype == PRGTLS ||
              rtype == PRGTAL || rtype == PRGTTC || rtype == PRGTAT ||
              rtype == PRGTSO))
            continue;
        std::string str;
        int nch = 0;
        getstr(ctx, m.colttl.data(), m.colptr.data(), m.nb, icol, str, nch);
        if (ctx.error.lfatal) return;
        int otltyp = 0, begotl = 0, endotl = 0;
        bool locok = true;
        rdotlr(ctx, str, ctx.arima.begsrs.data(), sp, otltyp, begotl, endotl,
               locok);
        if (ctx.error.lfatal) return;
        if (begotl != lastsy) continue;   // same collapsed guard as above
        ilast += 1;
        if (ilast == 1) {
            ltype = otltyp;
            lcol = icol;
        } else if (ss_opref(ltype) < ss_opref(otltyp)) {
            dlrgef(ctx, icol, nrxy, 1);            // this one loses
            if (ctx.error.lfatal) return;
        } else {
            dlrgef(ctx, lcol, nrxy, 1);            // the incumbent loses
            if (ctx.error.lfatal) return;
            ltype = otltyp;
            lcol = icol;
        }
    }
}
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

// restor.f:24 -- PACM=(PLEN+2*PORDER)*PARIMA, the flat length of Armacm/Acm2.
constexpr int PACM = (prm::PLEN + 2 * prm::PORDER) * prm::PARIMA;

// ssprep.f, scoped to Lx11=true always and Lx11rg=false (see hpp).
void ssprep_snapshot(X13Context& ctx, bool capture_saved, bool lx11) {
    ssprep_cmn& p = ctx.ssprep;
    const x11opt_cmn& opt = ctx.x11opt;

    // ssprep.f:36-42's `IF(Lx11)`. Every caller in this port passes Lx11=true
    // except sspdrv.f:218's, and there the distinction is load-bearing rather
    // than cosmetic: that call fires AFTER the span's x11pt2 has RESOLVED the
    // auto-select Lter sentinels, so snapshotting them would hand the next
    // span's restor a filter length already chosen for its predecessor. (The
    // other post-estimation call, arima.f:1430's, is safe only because it sits
    // before x11pt2 -- which is an accident of placement, not of the flag.)
    if (lx11) {
        for (int i = 1; i <= 12; ++i) p.lt2(i) = opt.lter(i);
        p.ktc2 = opt.ktcopt;
        p.tc2 = opt.tic;
    }
    if (capture_saved) {
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
    }

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
    // ssprep.f -- Chx2/Chg2/Acm2, the estimation workspace (the Cholesky
    // factors of X'X and G'G, and the ARMA covariance matrix). Deleting this
    // pair fails ZERO gates: every span's rgarma rebuilds all three from its
    // own design before reading them, so the inherited values are dead on this
    // corpus. Kept anyway, and the zero is recorded rather than treated as
    // licence to drop it -- an incomplete stand-in for `restor` has produced
    // three separate defects in this port already (CLAUDE.md), and each was
    // invisible until a later phase in a different file happened to read one
    // of the omitted fields.
    copy(d.chlxpx.data(), prm::PXPX, 1, p.chx2.data());
    copy(d.chlgpg.data(), prm::PGPG, 1, p.chg2.data());
    copy(d.armacm.data(), PACM, 1, p.acm2.data());
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
    // restor.f:96-99 -- the counterpart of the Chx2/Chg2/Acm2 snapshot above.
    copy(p.chx2.data(), prm::PXPX, 1, d.chlxpx.data());
    copy(p.chg2.data(), prm::PGPG, 1, d.chlgpg.data());
    copy(p.acm2.data(), PACM, 1, d.armacm.data());
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
bool ssmdl_fix_model(X13Context& ctx, bool& tdfix, bool& holfix, bool otlfix,
                     bool usrfix) {
    sspinp_cmn& si = ctx.sspinp;
    ssap_cmn& sa = ctx.ssap;
    model_cmn& m = ctx.model;

    // ssmdl.f:50-121 -- decide whether the sliding spans may analyse trading
    // day and holiday AT ALL. Three mutually exclusive arms, and only the
    // first needs `slidingspans{fixreg=}`:
    //
    //   Nssfxr>0   the user named the groups: rvfixd fixes them, and Tdfix /
    //              Holfix (which the caller then hands to ssxmdl) say so.
    //   Iregfx==3  every regARIMA coefficient is already fixed -- nothing can
    //              be re-estimated per span, so the analysis is pointless.
    //   Iregfx==2  SOME are fixed: walk the groups and demote only the
    //              component whose columns are fixed to a man.
    //
    // `Itd`/`Ihol` == -1 means "requested but not analysed", and its whole
    // observable is that ssap.f:206-211 then writes no `tds` (and, for Itd, no
    // `ads`) -- plus the ssphdr NOTE. See docs/M5_PORT_NOTES.md entry 81.
    if (m.nb > 0) {
        if (si.nssfxr > 0) {
            rvfixd(tdfix, holfix, otlfix, usrfix, m.iregfx, m.regfx, m.nb,
                   m.rgvrtp, ctx.x11adj.nusrrg, ctx.usrreg.usrtyp,
                   ctx.usrreg.ncusrx, m.userfx);
            if (sa.itd == 1 && tdfix) sa.itd = -1;
            if (sa.ihol == 1 && holfix) sa.ihol = -1;
            // NOTE what is deliberately NOT done here, because the obvious
            // reading of this port's own rules says to do it. rvfixd writes
            // only the LIVE Iregfx/Regfx, and ssmdl.f:53 does not mirror them
            // into ssprep.cmn's Regfx2/Irfx2 the way ssmdl.f:342-352 mirrors
            // Arimap/Arimaf for fixmdl=yes -- so the restor inside ssx11a.f:160
            // puts the main run's all-free flags straight back and NO span ever
            // sees a fixed coefficient. `slidingspans{fixreg=}` therefore does
            // not fix anything in the oracle: its entire effect is the Itd/Ihol
            // demote two lines up (no tds/ads table, plus the ssphdr NOTE).
            //
            // Measured, not deduced (docs/M5_PORT_NOTES.md entry 83): an
            // instrumented oracle dumping Arimap and B(1..7) per span gives
            // airline_slidingspans-fixreg-td values byte-identical to
            // airline_slidingspans-fixmdl-no, which is the same spec without
            // the fixreg= line. Mirroring the flags into the snapshot -- which
            // this port did, by analogy with fixmdl and history{fixreg=} --
            // made every span hold the TD coefficients fixed and put sfs 8.4e-03
            // out on span 1 alone.
        } else if (m.iregfx == 3) {
            // ssmdl.f:57-70. The Nssfxr/Ssfxrg write-back records the verdict
            // for the caller, and is skipped when Ssinit==1 because fixmdl=yes
            // is about to fix everything anyway.
            if (sa.itd == 1) {
                sa.itd = -1;
                if (si.ssinit != 1) {
                    si.nssfxr = 1;
                    si.ssfxrg(1) = 1;
                }
            }
            if (sa.ihol == 1) {
                sa.ihol = -1;
                if (si.ssinit != 1) {
                    si.nssfxr += 1;
                    si.ssfxrg(si.nssfxr) = 2;
                }
            }
        } else if (m.iregfx == 2) {
            // ssmdl.f:71-119. Tdfix/Holfix start TRUE and are ANDed down over
            // every column of every calendar group -- so a component is
            // demoted only when it is fixed in its entirety, and a design with
            // no group of that kind at all stays TRUE vacuously.
            tdfix = true;
            holfix = true;
            for (int igrp = 1; igrp <= m.ngrp; ++igrp) {
                const int begcol = m.grp(igrp - 1);
                const int endcol = m.grp(igrp) - 1;
                const int rtype = m.rgvrtp(begcol);
                if (!ss_is_calendar_group(rtype, /*with_user_hol=*/true))
                    continue;
                const bool ishol =
                    rtype == prm::PRGTEA || rtype == prm::PRGTEC ||
                    rtype == prm::PRGTES || rtype == prm::PRGTLD ||
                    rtype == prm::PRGTTH ||
                    (rtype >= prm::PRGTUH && rtype <= prm::PRGUH5);
                for (int icol = begcol; icol <= endcol; ++icol) {
                    if (ishol)
                        holfix = holfix && m.regfx(icol);
                    else
                        tdfix = tdfix && m.regfx(icol);
                }
            }
            if (tdfix && sa.itd > 0) {
                sa.itd = -1;
                if (si.ssinit != 1) {
                    si.nssfxr = 1;
                    si.ssfxrg(1) = 1;
                }
            }
            // ssmdl.f:112 tests `Ihol.eq.1` where its Itd twin tests `.gt.0`.
            // Transcribed as written -- the two are not interchangeable once
            // setssp.f:314's short-span demote has put Ihol at -2.
            if (holfix && sa.ihol == 1) {
                sa.ihol = -1;
                if (si.ssinit != 1) {
                    si.nssfxr += 1;
                    si.ssfxrg(si.nssfxr) = 2;
                }
            }
        }
    } else {
        si.nssfxr = 0;
    }

    // ssmdl.f:124-130.
    intlst(prm::PB, ctx.otlrev.otrptr.data(), ctx.otlrev.notrtl);
    if (si.ssotl <= 1) {
        ctx.arima.ltstao = false;
        ctx.arima.ltstls = false;
        ctx.arima.ltsttc = false;
    }

    // ssmdl.f:134-280 -- the group walk that decides which regressors can
    // survive into a span. `starta` is the LAST span's start and `enda` the
    // FIRST span's end, so [starta,enda] is the intersection of all Ncol spans;
    // a regressor undefined anywhere in it cannot be estimated by every span.
    //
    // MEASURED before porting (docs/M5_PORT_NOTES.md entry 85): on airline +
    // slidingspans{} + regression{variables=(ao1959.nov td)} the oracle holds
    // the AO back and re-adds it per span, this port did neither, and the
    // divergence was 4.3e-03 in sfs / 1.2e+01 in chs on spans 3-4 at
    // `OUTCOME: OK`. Deleting the ao= line reproduces neither (bit-exact), and
    // an AO inside the intersection (ao1950.feb) is bit-exact too -- so the
    // owner is the hold-back, not the presence of a regression{} group.
    bool regchg = false;
    {
        int begss[2] = {sa.iyr, sa.im};
        int starta[2] = {0, 0}, enda[2] = {0, 0};
        addate(begss, m.sp, (si.ncol - 1) * m.sp, starta);
        addate(ctx.arima.endspn.data(), m.sp, (1 - si.ncol) * m.sp, enda);
        for (int igrp = m.ngrp; igrp >= 1; --igrp) {
            const int begcol = m.grp(igrp - 1);
            const int endcol = m.grp(igrp) - 1;
            const int rtype = m.rgvrtp(begcol);
            using namespace prm;
            // ssmdl.f:150-241 -- the CHANGE-OF-REGIME arm, walled rather than
            // ported. The oracle does not survive it either: its date search at
            // :159 spells `'(change from before '` while EVERY title producer in
            // the tree (addlom.f:63, addtd.f:88, adrgim.f:72/178) writes
            // `'(change for before '`, so the second search can never match, the
            // fall-through hands `ctodat` position 20 of the title, the date
            // parse fails and the run halts with "Program error(s) halt
            // execution". Measured on airline + slidingspans{} +
            // regression{variables=(td/1955.jan/)}: oracle writes no sfs/chs at
            // all, this port wrote both in full. See CB-39. Refusing here is the
            // honest floor: it is not the oracle's message, but it is a refusal
            // where the oracle refuses, instead of numbers where it has none.
            if (rtype == PRRTST || rtype == PRRTTD || rtype == PRRTSE ||
                rtype == PRRTTS || rtype == PRRTLM || rtype == PRRTLQ ||
                rtype == PRRTLY || rtype == PRRTSL || rtype == PRR1TD ||
                rtype == PRR1ST || rtype == PRATTD || rtype == PRATST ||
                rtype == PRATSE || rtype == PRATTS || rtype == PRATLM ||
                rtype == PRATLQ || rtype == PRATLY || rtype == PRATSL) {
                ssp_not_ported(ctx,
                               "slidingspans{} with a change-of-regime "
                               "regression variable (ssmdl.f:150-241, which the "
                               "oracle itself halts on -- see CB-39) is");
                return false;
            }
            // ssmdl.f:246-253 -- user-specified outliers.
            if (rtype == PRGTAO || rtype == PRGTLS || rtype == PRGTRP ||
                rtype == PRGTTC || rtype == PRGTQD || rtype == PRGTQI ||
                rtype == PRGTSO || rtype == PRGTTL) {
                for (int icol = endcol; icol >= begcol; --icol) {
                    rmotss(ctx, icol, ctx.arima.begxy.data(), ctx.arima.nrxy,
                           begss, starta, enda, otlfix || si.ssinit == 1,
                           regchg);
                    if (ctx.error.lfatal) return false;
                }
            } else if (rtype == PRGTAA || rtype == PRGTAL ||
                       rtype == PRGTAT) {
                // ssmdl.f:259-278 -- AUTOMATICALLY identified outliers. With
                // Ssotl==1 (`slidingspans{outlier=remove}`, the default) each is
                // re-typed to its ordinary equivalent and then handed to rmotss
                // like a user's own; otherwise the column is simply deleted, so
                // no span carries it. Note `regchg` is set unconditionally by
                // this loop -- the re-type alone is a change.
                for (int icol = endcol; icol >= begcol; --icol) {
                    if (si.ssotl == 1) {
                        if (m.rgvrtp(icol) == PRGTAA) m.rgvrtp(icol) = PRGTAO;
                        if (m.rgvrtp(icol) == PRGTAL) m.rgvrtp(icol) = PRGTLS;
                        if (m.rgvrtp(icol) == PRGTAT) m.rgvrtp(icol) = PRGTTC;
                        rmotss(ctx, icol, ctx.arima.begxy.data(),
                               ctx.arima.nrxy, begss, starta, enda,
                               otlfix || si.ssinit == 1, regchg);
                        if (ctx.error.lfatal) return false;
                    } else {
                        dlrgef(ctx, icol, ctx.arima.nrxy, 1);
                        if (ctx.error.lfatal) return false;
                    }
                    regchg = true;
                }
            }
        }
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
        // ssmdl.f:345-350 -- the REGRESSION half of the same fix. `IF(.not.
        // regchg) CALL copy(B,PB,1,Bb)`: when the group walk above CHANGED the
        // design (an outlier held back), Bb is left alone here and rewritten by
        // the regchg store below instead -- the same array, but taken after
        // dlrgef has shifted every coefficient down past the deleted column.
        // Snapshot AND live copy, for the same reason as Arimap/Arimaf above.
        if (!regchg) copy(ctx.mdldat.b.data(), prm::PB, 1, ctx.ssprep.bb.data());
        for (int i = 1; i <= prm::PB; ++i) ctx.ssprep.regfx2(i) = true;
        if (ctx.model.iregfx < 3) ctx.model.iregfx = 3;
        for (int i = 1; i <= ctx.model.nb; ++i) ctx.model.regfx(i) = true;
        ctx.ssprep.irfx2 = 3;
        // (bakusr/Userfx needs user-defined regressors, which this driver's
        // scope excludes.)
        if (!ctx.model.userfx) ctx.model.userfx = ctx.usrreg.ncusrx > 0;
    }

    // ssmdl.f:358-373 -- make the structural change survive. Every span begins
    // with restor_span(), which reinstates Nb/Colttl/Grp/B/Regfx FROM this
    // snapshot; without the re-take the first span would put every held-back
    // outlier column straight back and the hold-back would be a no-op. Note it
    // runs AFTER the Ssinit==1 block, so on the fixmdl=yes path it overwrites
    // the all-true Regfx2/Irfx2=3 that block just wrote with the LIVE flags --
    // which by then are also all-true for columns 1..Nb (:348), but Iregfx is
    // whatever :347 left, not literal 3. Transcribed in the Fortran's order.
    if (regchg) ss_snapshot_design(ctx);
    return true;
}

// ssx11a.f:220-270, scoped per the hpp header.
void ssx11a_span_outliers(X13Context& ctx, int lastsy, bool otlfix) {
    using namespace prm;
    model_cmn& m = ctx.model;
    const int sp = m.sp;
    const int nrxy = ctx.arima.nrxy;

    // ssx11a.f:222-228 -- Begtst/Endtst follow the span when a per-span outlier
    // re-identification is live. `Ltstso` is commented out in the Fortran's own
    // `lidotl`; kept that way.
    const bool lidotl =
        ctx.arima.ltstao || ctx.arima.ltstls || ctx.arima.ltsttc;
    if (lidotl) {
        cpyint(ctx.mdldat.begspn.data(), 2, 1, ctx.arima.begtst.data());
        cpyint(ctx.arima.endspn.data(), 2, 1, ctx.arima.endtst.data());
    }

    // ssx11a.f:229-263 -- strike every outlier column this span cannot
    // estimate. Backwards over the groups so a delete cannot disturb the
    // columns still to check. RP/TLS are tested on BOTH endpoints and with the
    // inequalities reversed relative to the point types -- transcribed as
    // written (`begotl.ge.obeg .or. endotl.le.oend`, which is not the negation
    // of the other arm and is what the Fortran says).
    if (m.ngrp > 0) {
        int obeg = 0, oend = 0;
        dfdate(ctx.mdldat.begspn.data(), ctx.arima.begxy.data(), sp, obeg);
        obeg += 1;
        dfdate(ctx.arima.endspn.data(), ctx.arima.begxy.data(), sp, oend);
        oend += 1;
        for (int igrp = m.ngrp; igrp >= 1; --igrp) {
            const int begcol = m.grp(igrp - 1);
            const int endcol = m.grp(igrp) - 1;
            const int rtype = m.rgvrtp(begcol);
            if (!(rtype == PRGTAA || rtype == PRGTAO || rtype == PRGTAL ||
                  rtype == PRGTLS || rtype == PRGTAT || rtype == PRGTTC ||
                  rtype == PRGTQD || rtype == PRGTQI || rtype == PRGTSO ||
                  rtype == PRGTRP || rtype == PRGTTL))
                continue;
            for (int icol = endcol; icol >= begcol; --icol) {
                std::string str;
                int nchr = 0;
                getstr(ctx, m.colttl.data(), m.colptr.data(), m.ncoltl, icol,
                       str, nchr);
                if (ctx.error.lfatal) return;
                int otltyp = 0, begotl = 0, endotl = 0;
                bool locok = true;
                rdotlr(ctx, str, ctx.arima.begxy.data(), sp, otltyp, begotl,
                       endotl, locok);
                if (!locok) { abend(ctx); return; }
                const bool span_type = (otltyp == RP || otltyp == TLS);
                const bool gone =
                    (span_type && (begotl >= obeg || endotl <= oend)) ||
                    (!span_type && (begotl < obeg || begotl > oend));
                if (!gone) continue;
                dlrgef(ctx, icol, nrxy, 1);
                if (ctx.error.lfatal) return;
            }
        }
    }
    // ssx11a.f:268-269.
    adotss(ctx, lastsy, otlfix, nrxy);
}

// sspdrv.f:208-219, scoped per the hpp header.
void ssp_strip_span_outliers(X13Context& ctx) {
    model_cmn& m = ctx.model;
    otlrev_cmn& st = ctx.otlrev;
    if (st.notrtl <= 0) return;
    for (int i = 1; i <= st.notrtl; ++i) {
        std::string str;
        int nchr = 0;
        getstr(ctx, st.otrttl.data(), st.otrptr.data(), st.notrtl, i, str, nchr);
        if (ctx.error.lfatal) return;
        const int otl =
            strinx(true, m.colttl.raw(), m.colptr.data(), 1, m.nb, str);
        if (otl > 0) {
            dlrgef(ctx, otl, ctx.arima.nrxy, 1);
            if (ctx.error.lfatal) return;
        }
    }
    // sspdrv.f:218's `CALL ssprep(T,F,F)` is the FULL snapshot, not the design
    // half -- and it has to be, because dlrgef above shifted B and Regfx down
    // past each deleted column: a design-only re-take would leave Bb indexed
    // for the WIDER design and every following span would restore coefficients
    // one column out. Same routine arima.f:1430 calls (entry 83), same
    // capture_saved=false for the same reason, and lx11=false because
    // sspdrv.f:218 passes `ssprep(T,F,F)` -- see the note on that parameter.
    ssprep_snapshot(ctx, /*capture_saved=*/false, /*lx11=*/false);
}

// ssxmdl.f -- the x11regression half of ssmdl, called from setssp.f:353 under
// `IF(Nbx.gt.0)`.
//
// THE DEFAULT PATH IS LOAD-BEARING, which is what made skipping this routine
// expensive. `Ssxint` (slidingspans{fixx11reg=}) DEFAULTS TO YES at
// gtinpt.f:531 -- getssp.f:257 only ever overrides it -- so the tail below
// fixes every irregular-regression coefficient before the first span runs. Each
// span's xrgdrv then loads Bx (the MAIN run's C-iteration weights, saved there
// by xrgdrv.f:206's loadxr(T)) and x11mdl's Iregfx>=2 rmfix/addfix strikes every
// fixed column, so the span's OLS has nothing left to estimate and re-applies
// the main run's daily weights verbatim.
//
// Measured on the oracle (airline + slidingspans{} + x11regression{
// variables=(td)}): with the default the per-span TD factors are byte-identical
// across all four spans AND equal to the main run's c16; `fixx11reg=no` moves
// them (99.144951 -> 98.847324 at 1951.Jan). See docs/M5_PORT_NOTES.md entry 79.
static bool ssxmdl_span(X13Context& ctx, bool tdfix, bool holfix, bool otlfix,
                        bool usrfix) {
    xrgmdl_cmn& xg = ctx.xrgmdl;
    sspinp_cmn& si = ctx.sspinp;
    ssap_cmn& sa = ctx.ssap;
    const int sp = ctx.model.sp;

    // ssxmdl.f:27-39 -- an `x11regression{span=}` narrower than the series span
    // forces the fix on whatever fixx11reg= said, says so, and takes the TD and
    // holiday span analyses down with it (Itd/Ihol -1 = "requested but not
    // done").
    int nbeg = 0, nend = 0;
    dfdate(ctx.x11reg.begxrg.data(), ctx.mdldat.begspn.data(), sp, nbeg);
    dfdate(ctx.arima.endspn.data(), ctx.x11reg.endxrg.data(), sp, nend);
    if (nbeg > 0 || nend > 0) {
        si.ssxint = true;
        if (sa.itd > 0) sa.itd = -1;
        if (sa.ihol > 0) sa.ihol = -1;
        errhdr(ctx);
        writln(ctx,
               "NOTE: Since a span is used in the x11regression spec, the "
               "irregular ",
               stdio::STDERR, ctx.units.mt2, true);
        writln(ctx,
               "      regression coefficient estimates will be held fixed "
               "during the ",
               stdio::STDERR, ctx.units.mt2, false);
        writln(ctx, "      sliding spans analysis.", stdio::STDERR,
               ctx.units.mt2, false);
    }

    // ssxmdl.f:41 -- clear the held-back x11regression outlier list.
    ctx.otxrev.notxtl = 0;
    for (int i = 0; i <= prm::PB; ++i) ctx.otxrev.otxptr(i) = 1;

    // ssxmdl.f:44-76 -- the per-span outlier re-check (rmotss). Reached only
    // with slidingspans{x11outlier=no}, and Ssxotl DEFAULTS TO TRUE
    // (gtinpt.f:530), so the whole block is off unless the user asks for it.
    // Refuse rather than skip: this decides which x11regression AO columns a
    // span may keep, and getting it silently wrong is a wrong-numbers OK.
    if (!si.ssxotl && ctx.x11log.otlxrg) {
        ssp_not_ported(ctx,
                       "slidingspans{x11outlier=no} with automatic "
                       "x11regression outlier identification "
                       "(ssxmdl.f:44-76's rmotss block) is");
        return false;
    }

    // ssxmdl.f:78-83 -- rvfixd on the x11regression design (unconditional here:
    // this routine is only entered under setssp.f:353's `IF(Nbx.gt.0)`), then
    // the fixreg= demotes.
    //
    // THE STORE WRITES rvfixd MAKES HERE DO NOT SURVIVE THE ROUTINE. ssxmdl
    // brackets this block with `loadxr(F)` at :42 and `loadxr(T)` at :137: the
    // first copies Irgxfx/Regfxx/Usrxfx into the regARIMA working model, the
    // second copies them straight back out of it -- i.e. back to the values
    // rvfixd was handed, undoing every fix it just made. Only the group walk
    // immediately below ever reads the modified values. This port makes
    // neither loadxr call (nor setssp.f:356's compensating `restor` for the
    // working model those calls trample), so the save/restore below stands in
    // for that pair; see docs/M5_PORT_NOTES.md entry 82 for what the omitted
    // half leaves behind.
    const int irgxfx0 = xg.irgxfx;
    const auto regfxx0 = xg.regfxx;
    const bool usrxfx0 = xg.usrxfx;

    bool tdfx = false, holfx = false;
    rvfixd(tdfix, holfix, otlfix, usrfix, xg.irgxfx, xg.regfxx, xg.nbx,
           xg.rgxvtp, xg.nusxrg, ctx.usrxrg.usxtyp, xg.nusxrg, xg.usrxfx);
    if (tdfix && sa.itd > 0) sa.itd = -1;
    if (holfix && sa.ihol > 0) sa.ihol = -1;

    // ssxmdl.f:85-136 -- the ALREADY-fixed case: an x11regression design whose
    // calendar coefficients the user fixed with `b=(... f)`. Nothing is left
    // for a span to re-estimate, so the span analysis of that component is
    // suppressed. Note the guard runs on the component the x11regression
    // design owns (Axrgtd/Axrghl) and skips a component fixreg= already
    // claimed (`.not.Tdfix`), because that arm demoted it four lines up.
    if (((sa.itd == 1 && ctx.x11log.axrgtd && !tdfix) ||
         (sa.ihol == 1 && ctx.x11log.axrghl && !holfix)) &&
        xg.irgxfx >= 2) {
        tdfx = true;
        holfx = true;
        // Irgxfx==3 is "all fixed": the walk is skipped outright and both
        // flags stay true. Only Irgxfx==2 ("some fixed") has to look.
        if (xg.irgxfx == 2) {
            for (int igrp = 1; igrp <= xg.nxgrp; ++igrp) {
                const int begcol = xg.grpx(igrp - 1);
                const int endcol = xg.grpx(igrp) - 1;
                const int rtype = xg.rgxvtp(begcol);
                if (!ss_is_calendar_group(rtype, /*with_user_hol=*/false))
                    continue;
                // ssxmdl.f:100-102's inner test names PRGTUH, which its own
                // group test above cannot admit -- a dead arm, kept verbatim.
                const bool ishol =
                    rtype == prm::PRGTEA || rtype == prm::PRGTEC ||
                    rtype == prm::PRGTES || rtype == prm::PRGTLD ||
                    rtype == prm::PRGTTH || rtype == prm::PRGTUH;
                for (int icol = begcol; icol <= endcol; ++icol) {
                    if (ishol)
                        holfx = holfx && xg.regfxx(icol);
                    else
                        tdfx = tdfx && xg.regfxx(icol);
                }
            }
        }
        // ssxmdl.f:120-133. Ssfxxr is DIMENSION(4) and the Fortran increments
        // Nssfxx without checking it; the store is clamped here because it is
        // WRITE-ONLY in the whole oracle (Nssfxx is read at x11mdl.f:170, the
        // array itself nowhere), so the clamp cannot change any result.
        if (tdfx && sa.itd > 0) {
            sa.itd = -1;
            if (!tdfix) {
                si.nssfxx += 1;
                if (si.nssfxx <= 4) si.ssfxxr(si.nssfxx) = 1;
            }
        }
        if (holfx && sa.ihol > 0) {
            sa.ihol = -1;
            if (!holfix) {
                si.nssfxx += 1;
                if (si.nssfxx <= 4) si.ssfxxr(si.nssfxx) = 2;
            }
        }
    }

    // ssxmdl.f:137 -- loadxr(T); see the note above the save.
    xg.irgxfx = irgxfx0;
    xg.regfxx = regfxx0;
    xg.usrxfx = usrxfx0;

    // ssxmdl.f:138-150 -- the tail, and the reason this routine matters.
    // (The bakusr arm needs x11regression user regressors; Nusxrg==0 in the
    // corpus and the refusal above already fences the fixed-design cases.)
    if (si.ssxint) {
        for (int i = 1; i <= prm::PB; ++i) xg.regfxx(i) = true;
        if (xg.irgxfx < 3) xg.irgxfx = 3;
        if (!xg.usrxfx && xg.nusxrg > 0) {
            ssp_not_ported(ctx,
                           "slidingspans{} with x11regression{user=} "
                           "(ssxmdl.f:142-148's bakusr) is");
            return false;
        }
    }

    // ssxmdl.f:152-153 -- with every trading-day weight held fixed there is
    // nothing left for reweight= to renormalize. Same line as revdrv.f:327,
    // which run_history already carries. `tdfx` is the second trigger: a
    // b=(... f) design reaches it with Irgxfx==2.
    if (ctx.x11log.lxrneg && (xg.irgxfx == 3 || tdfx)) ctx.x11log.lxrneg = false;
    return true;
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

    // setssp.f:320-341 -- `slidingspans{fixreg=}` (Ssfxrg): which regressor
    // GROUPS are held fixed for the whole analysis. The four flags are OUTPUTS
    // of this decode and INPUTS to both ssmdl and ssxmdl -- and ssmdl writes
    // tdfix/holfix BACK, which is how its Iregfx==2 verdict reaches ssxmdl's
    // `.not.Tdfix` guard. They are one shared quartet, not two local pairs.
    //
    // The predecessor comment here read "not reachable, the gate corpus has no
    // fixreg= argument" -- true of the corpus, and beside the point: fixreg=
    // was PARSED into si.ssfxrg and then read by nobody, so a spec that used it
    // returned OUTCOME: OK with the trading-day span statistics the oracle
    // suppresses. Measured on airline+slidingspans{fixreg=(td)}: oracle writes
    // neither tds nor ads, this engine wrote 120 rows of each.
    bool tdfix = false, holfix = false, otlfix = false, usrfix = false;
    if (si.nssfxr > 0) {
        for (int i = 1; i <= si.nssfxr; ++i) {
            switch (si.ssfxrg(i)) {
            case 1: tdfix = true; break;
            case 2: holfix = true; break;
            case 3: usrfix = true; break;
            case 4: otlfix = true; break;
            default: break;
            }
        }
        // setssp.f:338-341.
        cpyint(si.ssfxrg.data(), 4, 1, si.ssfxxr.data());
        si.nssfxx = si.nssfxr;
    }
    // fixreg=(outlier) alone goes further than the rvfixd walk: `otlfix`
    // outlives setssp and reaches ssx11a per span (sspdrv.f:121), where it
    // decides whether a held-back outlier is re-added with its coefficient
    // fixed. That consumer is unported, so refuse rather than honour half of
    // the option.
    if (otlfix) {
        ssp_not_ported(ctx,
                       "slidingspans{fixreg=(outlier)} (the per-span otlfix "
                       "that reaches ssx11a, sspdrv.f:121) is");
        return false;
    }

    if (lmodel && !ssmdl_fix_model(ctx, tdfix, holfix, otlfix, usrfix))
        return false;
    // setssp.f:353-356 -- `IF(Nbx.gt.0) CALL ssxmdl(...)`.
    if (ctx.xrgmdl.nbx > 0 &&
        !ssxmdl_span(ctx, tdfix, holfix, otlfix, usrfix))
        return false;

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
        // sspdrv.f:130-143, "reset model parameters to original values". It sits
        // between ssx11a (whose tail is the restor above) and x11ari at :180,
        // so it runs BEFORE this span's estimation, not after the previous
        // one's -- the two readings differ and only the call order settles it.
        // Ssinit is `Intidx = ivec(1)-1` over INTDIC='no','yes','clear'
        // (getssp.f:50/156), so 2 is `slidingspans{fixmdl=clear}` and no corpus
        // spec sets it; the B/Bx arms are additionally inert while Iregfx or
        // Irgxfx is nonzero.
        if (si.ssinit == 2) {
            for (int i = 1; i <= prm::PARIMA; ++i)
                if (!ctx.model.arimaf(i)) ctx.mdldat.arimap(i) = prm::DNOTST;
            if (ctx.model.iregfx == 0)
                for (int i = 1; i <= prm::PB; ++i) ctx.mdldat.b(i) = prm::DNOTST;
            if (ctx.hiddn.ixreg > 0 && ctx.xrgmdl.irgxfx == 0)
                for (int i = 1; i <= prm::PB; ++i) ctx.xrgmdl.bx(i) = prm::DNOTST;
        }
        // sspdrv.f:127's `IF(Ixreg.eq.3)Ixreg=2` and ssx11a.f:93-94's demote are
        // both done inside run_x11_span (its set_xrg_span arm), so the
        // irregular regression re-runs per span. That demote was once measured
        // ALONE, found to take `sfs` from bit-exact to 4.1e+0, and rejected --
        // correct measurement, wrong conclusion. On its own it makes each span
        // REFIT the daily weights; the oracle refits nothing, because
        // slidingspans{fixx11reg=} defaults to YES and ssxmdl has already fixed
        // every x11regression coefficient. The demote and the fix are ONE
        // change. See docs/M5_PORT_NOTES.md entry 79.
        //
        // The `chs` gap this family carried (408 of 600 cells, attributed here
        // to a per-span prior phase and then, correctly, to the per-span
        // calendar factor) is CLOSED by that pair: sfs / chs / ads / tds all
        // gate bit-exact. The MODEL-FREE case (sfs 2.0e+2) is a separate and
        // still-open thing; it has no x11regression in it.
        const int lsp = l0 + (j - 1) * ny + sa.im - nbcst2 - 1;
        // ssx11a.f:268's `Otlfix.or.Ssinit.eq.1`. `Otlfix` is setssp's
        // fixreg=(outlier) flag, which cannot be true here: setssp_span walls
        // that option and returns false before this loop is reached, so the
        // disjunction collapses to the fixmdl arm. When that wall lifts, thread
        // the flag through instead of collapsing it.
        const bool ss_otlfix = (si.ssinit == 1);
        if (!run_x11_span(ctx, trnsrs_full, has_model, si.nlen, nfcst, nbcst,
                           nbcst2, lsp, /*nend_mdl=*/0, lseats,
                           /*set_xrg_span=*/true, /*ss_outliers=*/true,
                           ss_otlfix))
            return false;
        if (ctx.error.lfatal) return false;
        // sspdrv.f:208-219 -- take the re-added outlier columns back out before
        // the next span, so each span starts from the held-back design.
        ssp_strip_span_outliers(ctx);
        if (ctx.error.lfatal) return false;
    }
    hid.issap = 3;

    // sspdrv.f:264 -- the summary-measures early return, which sits BEFORE the
    // header below and before ssap. Reproduced only as a guard on the header:
    // ssap itself is entered either way here and reads Kfulsm per-branch.
    const bool kfulsm_ret =
        ctx.x11opt.kfulsm == 1 ||
        (ctx.x11opt.kfulsm == 2 && sa.itd == 0 && sa.ihol == 0);

    // ssphdr.f:145-152 -- the Mt2 half of the sliding-spans header. Almost all
    // of ssphdr is Mt1 (the deferred `.out` print engine), but these two NOTEs
    // go to BOTH channels, and they are the only observable of an Itd/Ihol
    // demote to -1: the whole effect of the demote is that the tds (and, for
    // Itd, the ads) table is NOT produced, which is an absence, not a value.
    //
    // The Fortran gates the whole routine on `Prttab(LSSSHD).or.Savtab(LSSSHD)`
    // and then on Lprt. This port has no print-table dictionary (Prttab/Savtab
    // are parsed-and-dropped by design, readers_val.cpp:874), so the NOTE is
    // emitted whenever the header stage is reached. That is exact for
    // `print=all` / the default, and over-emits for a `slidingspans{print=}`
    // list that excludes the header -- recorded here rather than faked, since
    // faking it would need a dictionary this port deliberately does not have.
    if (!kfulsm_ret) {
        auto& mt2c = ctx.channels_.unit(ctx.units.mt2);
        // FORMAT 2000/2001 carry `/` and `//` separators, which emit EMPTY
        // records -- writln's lblnk blank is `(' ',a)`, two characters, and
        // would be wrong here (see docs/M5_PORT_NOTES.md entry 80).
        static const char* const K2000[] = {
            "",
            " NOTE: Since the trading day coefficients are fixed in the sliding spans",
            "       analysis, the trading day statistics of the sliding spans analysis",
            "       are not printed.",
            "",
            "       In addition, the spans statistics for the seasonally adjusted",
            "       series have the same values as the corresponding statistics",
            "       for the seasonal factors.  In this case, the statistics for the",
            "       seasonally adjusted series are not printed.",
            "",
        };
        static const char* const K2001[] = {
            "",
            " NOTE: Since the holiday coefficients are fixed in the sliding spans analysis,",
            "       the spans statistics for the seasonally adjusted series have",
            "       the same values as the corresponding statistics for the seasonal",
            "       factors.  In this case, the statistics for the seasonally adjusted",
            "       series are not printed.",
            "",
        };
        if (sa.itd == -1 && sa.ihol <= 0) {
            for (const char* ln : K2000) mt2c.put(std::string(ln) + "\n");
        } else if (sa.ihol == -1 && sa.itd <= 0) {
            for (const char* ln : K2001) mt2c.put(std::string(ln) + "\n");
        }
        // (The Itd/Ihol == -2 arm at ssphdr.f:154-160 is Mt1-only -- the
        // deferred print engine -- and setssp.f:314 only reaches it on a span
        // shorter than five years, which no gated spec builds.)
    }

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

    // ssap.f:208 / :218 -- the Td (tds) table, conditional on Itd==1 and sharing
    // `iobs` with the S call above (ssap.f does not bump iobs until after the Sa
    // call). The store behind it comes from x11pt2.f:136 on a regARIMA trading
    // day and from x11mdl.f:874 on an x11regression one.
    if (muladd == 0 && ctx.agr.iagr < 6 && sa.itd == 1) {
        mflag(ctx.sspdat.td, /*nop2=*/0, iobs_s, sa.sslen2, si.ncol, ssdiff,
              ctx.ssout.dmax_tds);
        ctx.ssout.have_tds = true;
    } else {
        ctx.ssout.dmax_tds.assign(MXLEN, prm::DNOTST);
        ctx.ssout.have_tds = false;
    }

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
