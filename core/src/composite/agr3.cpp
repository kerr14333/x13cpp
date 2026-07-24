// agr3.cpp -- agrxpt.f + the indirect-adjustment core of agr3.f.
//
// INCREMENT 2 (tools/composite_scouting.md): produce the four indirect tables
// isf/isa/itn/iir (indirect seasonal / seasonally adjusted / trend / irregular)
// from the buffers agr2 accumulated. Deferred to increment 3: the title page and
// component table (Prttab(LCMPAH)), the D8/D9 indirect SI diagnostics, the E-table
// family (x11pt4 on the indirect side), the forced/rounded indirect series, and
// the Iagr==4 direct-vs-indirect comparison statistics (aggmea/cmpchi).
#include "composite/agr3.hpp"

#include "common/x13context.hpp"
#include "gen/notset.hpp"
#include "numeric/numeric.hpp"
#include "specparse/specparse.hpp"   // addate, copy, setdp
#include "x11/x11drv.hpp"            // vtc
#include "x11/x11filt.hpp"           // divsub, addmul

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

    // agr3.f:100-108 -- stash the DIRECT seasonally adjusted series into Orig2.
    // (The companion `Stci(i)=Ckhs(i)` + its Henderson pass feed only `Tem`, the
    // direct trend used by the increment-3 comparison statistics; /kcser/ Ckhs is
    // not carried by this port yet, so that pair is deferred with Tem.)
    for (int i = ptr.pos1bk; i <= ptr.posffc; ++i) tempo[i - 1] = ctx.x11srs.stci(i);
    ag.iagr = 4;

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

    // The published indirect trend lives in stc2in, not Stc (which still holds
    // the pre-level-shift filter output). Hand it to the caller.
    ctx.agr_stc2in.assign(stc2in.begin(), stc2in.end());
}

}  // namespace x13
