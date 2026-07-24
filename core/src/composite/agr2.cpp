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
#include "specparse/specparse.hpp"   // addate

namespace x13 {

namespace {
constexpr int YR = 1, MO = 2;   // 1-based date components (Fortran convention)
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

    // agr2.f:267 -- the DIRECT original total. (The remaining agr() calls of
    // agr2.f:268-281 are increment 2.)
    const int ind1 = ptr.pos1ob - ag.indnbc;
    agr(ctx.inpt.orig2.data(), ctx.agrsrs.o.data(), ag.iag, ind1, ptr.posffc,
        ag.ind1bk, ag.w);

    ag.ncomp = ag.ncomp + 1;   // agr2.f:296
    return true;
}

}  // namespace x13
