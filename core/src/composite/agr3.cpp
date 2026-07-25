// agr3.cpp -- agrxpt.f + the indirect-adjustment core of agr3.f.
//
// Produces the four indirect tables isf/isa/itn/iir (indirect seasonal /
// seasonally adjusted / trend / irregular) from the buffers agr2 accumulated,
// plus the DIRECT series/trend pair (Orig2/Tem) that agr2's Iagr==4 branch
// compares them against.
//
// Also here: the indirect D8/D9 SI ratios and the seasonality test battery
// (agr3.f:288-350). Those are NOT print surface -- ftest/kwtest/mstest/combft
// write /tests/ Test1,Test2, which are the M7 inputs the x11pt4 pass that runs
// next (x11ari.f:341) reads for the indirect Q, and vsfa's Ratis is `if2.is`.
//
// Not ported here, and none of it composite-specific (tools/composite_scouting.md):
// the title page and component table (Prttab(LCMPAH)). The forced/rounded
// indirect series is likewise still open.
#include "composite/agr3.hpp"

#include "common/x13context.hpp"
#include "gen/notset.hpp"
#include "numeric/numeric.hpp"
#include "specparse/specparse.hpp"   // addate, copy, setdp
#include "x11/x11drv.hpp"            // vtc
#include "x11/x11filt.hpp"           // divsub, addmul
#include "x11/x11seas.hpp"           // vsfa (the I/S ratios behind if2.is)
#include "x11/x11tests.hpp"          // ftest, kwtest, mstest, combft

#include <cmath>
#include <vector>

namespace x13 {

namespace {
constexpr int PLEN = 1020;
}  // namespace

// agrxpt.f -- reconcile the direct and indirect buffer geometries. When the
// direct run starts later in the padded buffer than the aggregate did, the
// aggregate is slid forward; when it starts earlier, the DIRECT pointers move to
// the indirect ones instead.
void agrxpt(X13Context& ctx, const int* begspn, int sp) {
    agr_cmn& ag = ctx.agr;
    agrsrs_cmn& as = ctx.agrsrs;
    x11ptr_cmn& ptr = ctx.x11ptr;
    if (ptr.pos1ob > ag.ind1ob) {
        const int shift = ptr.pos1ob - ag.ind1ob;
        for (int i = ag.indffc; i >= 1; --i) {
            const int i2 = i + shift;
            as.o(i2) = as.o(i);
            as.o2(i2) = as.o2(i);
            as.o3(i2) = as.o3(i);
            as.o4(i2) = as.o4(i);
            as.o5(i2) = as.o5(i);
            as.ci(i2) = as.ci(i);
            as.omod(i2) = as.omod(i);
            as.tem(i2) = as.tem(i);
        }
        ag.ind1ob = ptr.pos1ob;
        ag.indfob = ptr.posfob;
        ag.ind1bk = ag.ind1ob - ag.indnbc;
        ag.indffc = ag.indfob + ag.indnfc;
        ag.ibgbk2(1) = ctx.extend.begbk2(1);
        ag.ibgbk2(2) = ctx.extend.begbk2(2);
    } else if (ptr.pos1ob < ag.ind1ob) {
        ptr.pos1ob = ag.ind1ob;
        ptr.posfob = ag.indfob;
        ptr.pos1bk = ptr.pos1ob - ctx.extend.nbcst;
        ptr.posffc = ptr.posfob + ctx.extend.nfcst;
        ctx.extend.begbk2(1) = ag.ibgbk2(1);
        ctx.extend.begbk2(2) = ag.ibgbk2(2);
        int nbcst2 = 0;
        dfdate(begspn, ctx.extend.begbk2.data(), sp, nbcst2);
        ctx.extend.nbcst2 = nbcst2;
    }
}

// agr3.f -- the indirect seasonal adjustment. Runs on the composite total's own
// run (Iagr==3) AFTER its direct adjustment, and rebuilds the X-11 D-tables from
// the aggregated component results instead of from the aggregate itself.
void agr3(X13Context& ctx, const int* begspn) {
    agr_cmn& ag = ctx.agr;
    agrsrs_cmn& as = ctx.agrsrs;
    x11ptr_cmn& ptr = ctx.x11ptr;
    extend_cmn& ext = ctx.extend;
    x11opt_cmn& opt = ctx.x11opt;
    const int ny = opt.ny;
    const int muladd = opt.muladd;
    const double rinit = (muladd == 1) ? 0.0 : 1.0;

    // agr3.f:57 EQUIVALENCE(Orig2,tempo) and (stexx,Omod): the two "scratch"
    // names below are the SAME storage as Orig2 and Omod. Reproduce the aliasing
    // -- later stages read Orig2 (the direct SA) and Omod through it.
    double* tempo = ctx.inpt.orig2.data();     // == Orig2
    double* stexx = as.omod.data();            // == Omod

    // agr3.f:100-108 -- stash the DIRECT seasonally adjusted series into Orig2,
    // and put x11pt3's pre-fold SA snapshot (/kcser/ Ckhs) back into Stci: the
    // comparison statistics filter the DIRECT trend out of THAT, not out of the
    // published D11.
    for (int i = ptr.pos1bk; i <= ptr.posffc; ++i) {
        tempo[i - 1] = ctx.x11srs.stci(i);
        ctx.x11srs.stci(i) = ctx.kcser_ckhs[i - 1];
    }
    ag.iagr = 4;

    // agr3.f:110-143 -- the DIRECT trend for the comparison statistics: a 13-term
    // (5 quarterly) Henderson, insisted on regardless of what the direct run
    // chose. Stashed in `Tem`; agr2's Iagr==4 branch pairs it with Orig2 above.
    // (Nterm is reported as `indtrendma` here, which is the direct filter -- but
    // Ktcopt pins it to 13/5, so the label is the only thing that is off.)
    opt.ktcopt = (ny == 4) ? 5 : 13;
    vtc(ctx, ctx.x11srs.stc.data(), ctx.x11srs.stci.data());
    ctx.agr_indtrendma = opt.nterm;
    for (int i = ptr.pos1bk; i <= ptr.posffc; ++i) as.tem(i) = ctx.x11srs.stc(i);

    // agr3.f:146-159 -- swap the run onto the INDIRECT geometry.
    ptr.pos1bk = ag.ind1bk;
    ptr.posffc = ag.indffc;
    ext.nofpob = ext.nofpob - ext.nfcst + ag.indnfc;
    ext.nbfpob = ext.nofpob - ext.nfcst + ag.indnfc - ext.nbcst + ag.indnbc;
    ag.dirnfc = ext.nfcst;
    ag.dirnbc = ext.nbcst;
    ext.nfcst = ag.indnfc;
    ext.nbcst = ag.indnbc;
    addate(begspn, ny, -ext.nbcst, ext.begbak.data());

    const int pos1ob = ptr.pos1ob, posfob = ptr.posfob;
    const int pos1bk = ptr.pos1bk, posffc = ptr.posffc;

    // agr3.f:161-166 -- the indirect SA and modified original ARE the sums.
    for (int i = pos1bk; i <= posffc; ++i) {
        ctx.x11srs.stci(i) = as.ci(i);
        ctx.adxser.stome(i) = as.omod(i);
    }
    opt.kpart = 1;
    if (opt.tmpma == 2) opt.tmpma = 0;
    opt.kpart = 4;

    // agr3.f:181-190 -- extremes, then the modified SA series (E2).
    divsub(stexx, ctx.inpt.series.data(), ctx.adxser.stome.data(), pos1ob, posfob, muladd);
    divsub(ctx.adxser.stcime.data(), ctx.x11srs.stci.data(), stexx, pos1ob, posfob, muladd);
    for (int i = pos1ob; i <= posfob; ++i)
        ctx.x11srs.stci(i) = ctx.adxser.stcime(i);

    // agr3.f:195-205 -- strip indirect level shifts before the trend filter.
    std::vector<double> flsind(PLEN, rinit), faoind(PLEN, rinit);
    if (ag.lindot) {
        if (ag.lindls) {
            divsub(flsind.data(), as.o.data(), as.o3.data(), pos1bk, posffc, muladd);
            divsub(ctx.x11srs.stci.data(), ctx.x11srs.stci.data(), flsind.data(),
                   pos1bk, posffc, muladd);
        }
        if (ag.lindao)
            divsub(faoind.data(), as.o.data(), as.o4.data(), pos1bk, posffc, muladd);
    }

    // agr3.f:210-215 -- the final indirect trend: a 13-term (5 quarterly)
    // Henderson, insisted on regardless of the direct run's choice.
    opt.ktcopt = (ny == 4) ? 5 : 13;
    vtc(ctx, ctx.x11srs.stc.data(), ctx.x11srs.stci.data());
    for (int i = pos1ob; i <= posfob; ++i) ctx.x11srs.stci(i) = as.ci(i);

    // agr3.f:220-224 -- fold the level shifts back into the published trend.
    std::vector<double> stc2in(PLEN, 0.0);
    copy(ctx.x11srs.stc.data(), PLEN, 1, stc2in.data());
    if (ag.lindot && ag.lindls)
        addmul(stc2in.data(), stc2in.data(), flsind.data(), pos1bk, posffc, muladd);

    // agr3.f:255-263 -- the indirect irregular, and its modified counterpart.
    divsub(ctx.x11srs.sti.data(), ctx.x11srs.stci.data(), stc2in.data(),
           pos1ob, posfob, muladd);
    divsub(ctx.mq5a_stime.data(), ctx.x11srs.sti.data(), stexx, pos1ob, posfob, muladd);

    // agr3.f:266-280 -- the indirect seasonal factors, plus the total/calendar
    // adjustment factors. (Psuadd -- pseudo-additive -- is increment 3.)
    divsub(ctx.x11srs.sts.data(), as.o5.data(), ctx.x11srs.stci.data(),
           pos1bk, posffc, muladd);
    ctx.agr_ststd.assign(PLEN, 0.0);
    divsub(ctx.agr_ststd.data(), as.o2.data(), ctx.x11srs.stci.data(),
           pos1bk, posffc, muladd);
    divsub(ctx.x11fac.faccal.data(), as.o2.data(), as.o5.data(), pos1bk, posffc, muladd);

    // --- agr3.f:288-334 -- the indirect D8 SI ratios and the seasonality test
    // battery. These are NOT print surface: ftest/kwtest/mstest write /tests/,
    // combft turns that into Test1/Test2, and Test1/Test2 ARE the M7 inputs that
    // x11pt4's f3cal reads for the indirect Q. vsfa's Ratis is `if2.is`.
    double* stsie = ctx.work3_stsie.data();
    divsub(stsie, ctx.inpt.series.data(), stc2in.data(), pos1ob, posfob, muladd);
    divsub(ctx.x11srs.stsi.data(), stsie, stexx, pos1ob, posfob, muladd);

    // agr3.f:298-302 -- the D8 tests run on the SI with any indirect AO outlier
    // taken back out. kwtest SORTS its argument in place, so the moving-
    // seasonality input is rebuilt from scratch afterwards (agr3.f:324), not
    // reused.
    std::vector<double> temp4(PLEN, 0.0);
    if (ag.lindot && ag.lindao)
        divsub(temp4.data(), stsie, faoind.data(), pos1bk, posffc, muladd);
    else
        copy(stsie, posffc, 1, temp4.data());
    ftest(ctx, temp4.data(), pos1ob, posfob, ny, 0);
    kwtest(ctx, temp4.data(), pos1ob, posfob, ny);
    divsub(temp4.data(), ctx.inpt.series.data(), stc2in.data(), pos1ob, posfob,
           muladd);
    mstest(ctx, temp4.data(), pos1ob, posfob, ny);
    combft(ctx);

    // agr3.f:334 -- the I/S ratios. NB the range is Pos1ob..Posfob here, where
    // x11pt3's own vsfa call uses Pos1bk..Posfob.
    vsfa(ctx.x11srs.stsi.data(), pos1ob, posfob, ny, muladd, ctx.x11msc.psuadd,
         opt.rati.data(), opt.ratis);

    // agr3.f:340-350 -- the D9 final replacement values: the modified SI, but
    // only where the extreme-value factor actually moved the unmodified SI by at
    // least 1e-4 in relative terms. Everywhere else the table is blank (DNOTST).
    // Nothing downstream reads this; it is the id8/id9 save pair.
    ctx.agr_id8.assign(stsie, stsie + PLEN);
    ctx.agr_id9.assign(PLEN, prm::DNOTST);
    const double ebar = 1.0 - static_cast<double>(muladd);
    for (int i = pos1ob; i <= posfob; ++i) {
        double tmpe = stexx[i - 1] - ebar;
        if (dpeq(tmpe, 0.0)) continue;
        tmpe = tmpe / stsie[i - 1];
        if (std::abs(tmpe) >= 0.0001)
            ctx.agr_id9[static_cast<std::size_t>(i - 1)] = ctx.x11srs.stsi(i);
    }

    // The published indirect trend lives in stc2in, not Stc (which still holds
    // the pre-level-shift filter output). Hand it to the caller.
    ctx.agr_stc2in.assign(stc2in.begin(), stc2in.end());
    // agr3.f:598 -- and publish it into Stc2 as well, because the x11pt4 pass
    // x11ari.f:341 runs next reads Stc2 for the indirect E7.
    copy(stc2in.data(), posffc, -1, ctx.x11srs.stc2.data());
}

}  // namespace x13
