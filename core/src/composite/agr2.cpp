// agr2.cpp -- setapt.f + the per-component accumulation branch of agr2.f.
//
// INCREMENT 1 (see tools/composite_scouting.md): only the DIRECT composite total
// `O` is accumulated. The indirect buffers (O1..O5 from the Fin*/prior/LS/AO/TC/
// calendar-stripped originals, Ci/Ci2 from the component SA, Omod from Stome) and
// the Iagr==4 direct-vs-indirect comparison statistics are increments 2 and 3.
#include "composite/agr2.hpp"

#include "common/x13context.hpp"
#include "composite/agr.hpp"
#include "gen/notset.hpp"
#include "numeric/numeric.hpp"
#include "specparse/specparse.hpp"   // addate, copy
#include "x11/x11filt.hpp"          // divsub, addmul

#include <vector>

namespace x13 {

namespace {
constexpr int YR = 1, MO = 2;   // 1-based date components (Fortran convention)

// issame.f -- are all the GOOD observations in [l1,l2] equal?
bool issame(const X13Context& ctx, const double* lsrs, int l1, int l2) {
    const double base = lsrs[l1 - 1];
    for (int i = l1 + 1; i <= l2; ++i) {
        if (ctx.goodob.gudval(i) && !dpeq(lsrs[i - 1], base)) return false;
    }
    return true;
}
}  // namespace

// setapt.f -- set the indirect-adjustment pointers from this component's
// backcast/forecast counts. The FIRST component fixes them; later ones can only
// shrink the common extension region.
void setapt(X13Context& ctx, int nb, int nf, const int* begspn, int sp) {
    agr_cmn& ag = ctx.agr;
    const x11ptr_cmn& ptr = ctx.x11ptr;
    if (ag.indnfc == prm::NOTSET) {
        ag.indnfc = nf;
        ag.indnbc = nb;
        ag.ind1bk = ptr.pos1ob - nb;
        ag.ind1ob = ptr.pos1ob;
        ag.indfob = ptr.posfob;
        ag.indffc = ptr.posfob + nf;
        int ibgbk2[2];
        addate(begspn, sp, -nb, ibgbk2);
        if (ibgbk2[MO - 1] > 1) ibgbk2[MO - 1] = 1;
        ag.ibgbk2(YR) = ibgbk2[YR - 1];
        ag.ibgbk2(MO) = ibgbk2[MO - 1];
    } else {
        if (ag.indnbc > ctx.extend.nbcst) {
            ag.indnbc = ctx.extend.nbcst;
            ag.ind1bk = ag.ind1ob - ctx.extend.nbcst;
            ag.ibgbk(YR) = ctx.extend.begbak(YR);
            ag.ibgbk(MO) = ctx.extend.begbak(MO);
        }
        if (ag.indnfc > ctx.extend.nfcst) {
            ag.indnfc = ctx.extend.nfcst;
            ag.indffc = ag.indfob + ctx.extend.nfcst;
        }
    }
}

// agr2.f, the component-accumulation path (agr2.f:57-64 + 195-303).
bool agr2_component(X13Context& ctx) {
    agr_cmn& ag = ctx.agr;
    const x11ptr_cmn& ptr = ctx.x11ptr;
    const int ny = ctx.model.sp;
    const int* begspn = ctx.mdldat.begspn.data();

    // agr2.f:57-64 -- the first component to reach here stamps the span every
    // later component must match. (editor.f:2245-2255 does this instead when the
    // run is -c/check-input or -w/composite-only; a plain metafile run lands
    // here.)
    if (ag.iagr < 2) {
        ag.iagr = 2;
        ag.itest(1) = ny;
        ag.itest(2) = begspn[MO - 1];
        ag.itest(3) = ctx.x11opt.lstmo;
        ag.itest(4) = ctx.lzero.ly0;
        ag.itest(5) = ctx.x11opt.lstyr;
    }

    // agr2.f:195-199 -- every component must cover exactly the same span.
    if (!(ag.itest(1) == ny && ag.itest(2) == begspn[MO - 1] &&
          ag.itest(3) == ctx.x11opt.lstmo && ag.itest(4) == ctx.lzero.ly0 &&
          ag.itest(5) == ctx.x11opt.lstyr)) {
        // agr2.f:311-315: "Series <name> has non-overlapping time span."
        ag.iagr = -1;
        return false;
    }

    setapt(ctx, ctx.extend.nbcst, ctx.extend.nfcst, begspn, ny);

    // agr2.f:200-261 -- build the five per-component originals that the indirect
    // adjustment aggregates, each a copy of a whole-buffer series with a
    // different set of effects stripped.
    constexpr int PLEN = 1020;
    const int pos1bk = ptr.pos1bk, posffc = ptr.posffc;
    const int muladd = ctx.x11opt.muladd;
    const x11adj_cmn& adj = ctx.x11adj;
    std::vector<double> srs1(PLEN, 0.0), srs2(PLEN, 0.0), srs3(PLEN, 0.0),
        srs4(PLEN, 0.0), srs5(PLEN, 0.0);

    // srs1 = the prior-adjusted original (agr2.f:204).
    copy(ctx.orisrs.stoap.data(), posffc, 1, srs1.data());
    // srs2 = the original less everything removed from the SA series
    // (agr2.f:209-217). The Fin*/prior removals are no-ops unless those options
    // are on; the port has no Nuspad/Priadj>1 path yet (rmpadj is unported --
    // it is the "user PRIOR factors" stub), so guard it loudly.
    copy(ctx.inpt.orig2.data(), posffc, 1, srs2.data());
    if (adj.finao && adj.nao > 0)
        divsub(srs2.data(), srs2.data(), ctx.x11fac.facao.data(), pos1bk, posffc, muladd);
    if (adj.finls && adj.nls > 0)
        divsub(srs2.data(), srs2.data(), ctx.x11fac.facls.data(), pos1bk, posffc, muladd);
    if (adj.fintc && adj.ntc > 0)
        divsub(srs2.data(), srs2.data(), ctx.x11fac.factc.data(), pos1bk, posffc, muladd);
    if (adj.finusr)
        divsub(srs2.data(), srs2.data(), ctx.x11fac.facusr.data(), pos1bk, posffc, muladd);
    // srs3 = the original less LS effects (agr2.f:223-249).
    copy(ctx.inpt.orig2.data(), posffc, 1, srs3.data());
    if (adj.adjls == 1 && adj.nls > 0 && !adj.finls) {
        divsub(srs3.data(), srs3.data(), ctx.x11fac.facls.data(), pos1bk, posffc, muladd);
        ag.lindls = true;
    }
    // srs4 = the original less AO/TC effects (agr2.f:243-252).
    copy(ctx.inpt.orig2.data(), posffc, 1, srs4.data());
    if (adj.adjao == 1 && adj.nao > 0 && !adj.finao) {
        divsub(srs4.data(), srs4.data(), ctx.x11fac.facao.data(), pos1bk, posffc, muladd);
        ag.lindao = true;
    }
    if (adj.adjtc == 1 && adj.ntc > 0 && !adj.fintc) {
        divsub(srs4.data(), srs4.data(), ctx.x11fac.factc.data(), pos1bk, posffc, muladd);
        ag.lindao = true;
    }
    // srs5 = srs2 less the calendar factor (agr2.f:255-261).
    if (ctx.x11opt.kfulsm == 1) {
        copy(srs2.data(), posffc, 1, srs5.data());
    } else {
        divsub(srs5.data(), srs2.data(), ctx.x11fac.faccal.data(), pos1bk, posffc, muladd);
        if (!ag.lindcl)
            ag.lindcl = !issame(ctx, ctx.x11fac.faccal.data(), pos1bk, posffc);
    }

    // agr2.f:266-281 -- accumulate. `w` is by-reference in the Fortran too, so
    // the first agr() call resolves an unset weight to 1 for all of them.
    const int ind1 = ptr.pos1ob - ag.indnbc;
    agrsrs_cmn& as = ctx.agrsrs;
    agr(ctx.inpt.orig2.data(), as.o.data(),  ag.iag, ind1, posffc, ag.ind1bk, ag.w);
    agr(srs1.data(),           as.o1.data(), ag.iag, ind1, posffc, ag.ind1bk, ag.w);
    agr(srs2.data(),           as.o2.data(), ag.iag, ind1, posffc, ag.ind1bk, ag.w);
    agr(srs3.data(),           as.o3.data(), ag.iag, ind1, posffc, ag.ind1bk, ag.w);
    agr(srs4.data(),           as.o4.data(), ag.iag, ind1, posffc, ag.ind1bk, ag.w);
    agr(srs5.data(),           as.o5.data(), ag.iag, ind1, posffc, ag.ind1bk, ag.w);
    // Lx11 && X11agr: the X-11 modified original (agr2.f:273-274).
    agr(ctx.adxser.stome.data(), as.omod.data(), ag.iag, ind1, posffc, ag.ind1bk, ag.w);
    // The component SA (Stci) and its forced counterpart (Stci2) -- the indirect
    // adjustment IS this sum (agr2.f:276-281). The SEATS branch (Seatsa/Setsa2)
    // is increment 2b.
    agr(ctx.x11srs.stci.data(),  as.ci.data(),  ag.iag, ind1, posffc, ag.ind1bk, ag.w);
    agr(ctx.adxser.stci2.data(), as.ci2.data(), ag.iag, ind1, posffc, ag.ind1bk, ag.w);

    ag.ncomp = ag.ncomp + 1;   // agr2.f:296
    return true;
}

}  // namespace x13
