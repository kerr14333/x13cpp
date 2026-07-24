// agr2.cpp -- setapt.f + the per-component accumulation branch of agr2.f.
//
// setapt.f + BOTH agr2.f branches: the per-component accumulation (the direct
// total `O`, the indirect buffers O1..O5/Ci/Ci2/Omod) and, once agr3 has built
// the indirect adjustment, the Iagr==4 direct-vs-indirect comparison statistics
// (aggmea.f). See tools/composite_scouting.md.
#include "composite/agr2.hpp"

#include "common/x13context.hpp"
#include "composite/agr.hpp"
#include "gen/notset.hpp"
#include "numeric/numeric.hpp"
#include "specparse/specparse.hpp"   // addate, copy
#include "x11/x11filt.hpp"          // divsub, addmul

#include <cmath>
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

namespace {

// aggmea.f -- the measures of roughness of one seasonally adjusted series `a`
// against its trend `b`, over [i1,i2].
//   R1 = mean square (and root mean square) of the FIRST DIFFERENCES of `a`;
//   R2 = the variance (and standard deviation) of the SA/trend ratio (muladd==0)
//        or difference, about its own mean.
// Note the divisors: R1 divides by i2-i1 (the number of differences), R2 by
// i2-i1+1 (the number of observations) -- aggmea.f reuses `div`, adding one.
void aggmea(const double* a, const double* b, double& w, double& x, double& y,
            double& z, int i1, int i2, int muladd, bool x11agr) {
    w = 0.0; x = 0.0; y = 0.0; z = 0.0;
    for (int i = i1; i <= i2 - 1; ++i) {
        const double d = a[i] - a[i - 1];   // a(i+1)-a(i), 1-based
        w += d * d;
    }
    double div = static_cast<double>(i2 - i1);
    w /= div;
    x = std::sqrt(w);
    if (!x11agr) return;
    double cbar = 0.0;
    for (int i = i1; i <= i2; ++i)
        cbar += (muladd == 0) ? (a[i - 1] / b[i - 1]) : (a[i - 1] - b[i - 1]);
    div += 1.0;
    cbar /= div;
    for (int i = i1; i <= i2; ++i) {
        const double c =
            ((muladd == 0) ? (a[i - 1] / b[i - 1]) : (a[i - 1] - b[i - 1])) - cbar;
        y += c * c;
    }
    y /= div;
    z = std::sqrt(y);
}

}  // namespace

// agr2.f:66-192 -- the end of the indirect adjustment.
void agr2_compare(X13Context& ctx, const int* begspn) {
    agr_cmn& ag = ctx.agr;
    x11ptr_cmn& ptr = ctx.x11ptr;
    extend_cmn& ext = ctx.extend;
    const int ny = ctx.x11opt.ny;
    const int muladd = ctx.x11opt.muladd;
    // X11agr: the composite is adjusted by X-11 (the SEATS composite runs agr3s
    // and is not ported), so the R2 half of the statistics is always computed.
    const bool x11agr = true;

    ag.iagr = 0;
    if (ctx.hiddn.issap > 0 || ctx.hiddn.irev > 0) ag.iagr = 5;
    if (ctx.hiddn.issap == 0 && ctx.hiddn.irev == 0) ag.ncomp = 0;

    // agr2.f:88-119 -- four aggmea calls: DIRECT (Orig2 = the SA agr3 stashed,
    // Tem = its forced-Henderson trend) and INDIRECT (Stci/Stc), each over the
    // full series and over the last three years.
    const int pos1ob = ptr.pos1ob, posfob = ptr.posfob;
    const int kfda = posfob - ny * 3 + 1;
    const double* dirsa = ctx.inpt.orig2.data();
    const double* dirtr = ctx.agrsrs.tem.data();
    const double* indsa = ctx.x11srs.stci.data();
    const double* indtr = ctx.x11srs.stc.data();

    std::vector<double> di(25, 0.0);   // 1-based di(1..24)
    double a1, a2, a3, a4;
    aggmea(dirsa, dirtr, a1, a2, a3, a4, pos1ob, posfob, muladd, x11agr);
    di[1] = a1; di[7] = a2; if (x11agr) { di[13] = a3; di[19] = a4; }
    aggmea(dirsa, dirtr, a1, a2, a3, a4, kfda, posfob, muladd, x11agr);
    di[2] = a1; di[8] = a2; if (x11agr) { di[14] = a3; di[20] = a4; }
    aggmea(indsa, indtr, a1, a2, a3, a4, pos1ob, posfob, muladd, x11agr);
    di[3] = a1; di[9] = a2; if (x11agr) { di[15] = a3; di[21] = a4; }
    aggmea(indsa, indtr, a1, a2, a3, a4, kfda, posfob, muladd, x11agr);
    di[4] = a1; di[10] = a2; if (x11agr) { di[16] = a3; di[22] = a4; }

    // agr2.f:120-125 -- di(i+4)/di(i+5) are the direct->indirect PERCENTAGE
    // changes; positive means the indirect adjustment is the smoother one.
    const int i2 = x11agr ? 19 : 7;
    for (int i = 1; i <= i2; i += 6) {
        di[i + 4] = (di[i] - di[i + 2]) * 100.0 / di[i];
        di[i + 5] = (di[i + 1] - di[i + 3]) * 100.0 / di[i + 1];
    }
    ctx.agr_cmpstat.assign(di.begin(), di.end());

    // agr2.f:183-190 -- put the pointers back on the DIRECT geometry that agr3
    // swapped away from.
    ptr.pos1bk = ptr.pos1ob - ag.dirnbc;
    ptr.posffc = ptr.posfob + ag.dirnfc;
    ext.nofpob = ext.nofpob - ext.nfcst + ag.dirnfc;
    ext.nbfpob = ext.nbfpob - ext.nfcst + ag.dirnfc - ext.nbcst + ag.dirnbc;
    ext.nfcst = ag.dirnfc;
    ext.nbcst = ag.dirnbc;
    addate(begspn, ny, -ext.nbcst, ext.begbak.data());
}

}  // namespace x13
