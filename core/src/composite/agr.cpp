// agr.cpp -- agr.f + agr1.f (composite adjustment primitives).
#include "composite/agr.hpp"

#include "common/x13context.hpp"
#include "gen/notset.hpp"
#include "numeric/numeric.hpp"
#include "specparse/specparse.hpp"   // setdp

namespace x13 {

namespace {
constexpr int PLEN = 1020;
constexpr int YR = 1, MO = 2;   // 1-based, as in the Fortran (agr1.f uses Indrvs(YR/MO))
}  // namespace

// agr.f
void agr(const double* a, double* b, int iag, int j1, int j2, int j0, double& wt) {
    // agr.f:16 -- an unset weight becomes 1, and the Fortran writes it back
    // through the by-reference Wt (so the caller's W is 1 from then on).
    if (dpeq(wt, 0.0)) wt = 1.0;
    for (int i = j1; i <= j2; ++i) {
        const int j = i + j0 - j1;
        if (iag == 0) b[j - 1] = b[j - 1] + (a[i - 1] * wt);
        if (iag == 1) b[j - 1] = b[j - 1] - (a[i - 1] * wt);
        if (iag == 2) b[j - 1] = b[j - 1] * (a[i - 1] * wt);
        if (iag == 3) b[j - 1] = b[j - 1] / (a[i - 1] * wt);
    }
}

// agr1.f
void agr1(X13Context& ctx, double* y, int& nobs) {
    agr_cmn& ag = ctx.agr;
    agrsrs_cmn& as = ctx.agrsrs;

    if (ag.iagr == 0) {
        // --- INITIALIZATION (agr1.f:31-70) ---
        ag.ncomp = 0;
        ctx.rev.nrcomp = 0;
        ctx.ssap.nscomp = 0;
        ctx.ssap.indssp = prm::NOTSET;
        ag.indnfc = prm::NOTSET;
        ag.indnbc = prm::NOTSET;
        ag.iagr = 1;
        for (int i = 1; i <= PLEN; ++i) {
            as.o(i) = 0.0;
            as.o2(i) = 0.0;
            as.o3(i) = 0.0;
            as.o4(i) = 0.0;
            as.o5(i) = 0.0;
            as.omod(i) = 0.0;
            as.ci(i) = 0.0;
        }
        ag.lindao = false;
        ag.lindls = false;
        ag.lindcl = false;
        // Sliding-spans + revisions-history indirect buffers.
        setdp(0.0, 276 * 4, ctx.sspdat.saind.data());
        setdp(0.0, 276 * 4, ctx.sspdat.sfind.data());
        setdp(0.0, 276 * 4, ctx.sspdat.sfinda.data());
        setdp(0.0, 1000, ctx.revsrs.cncisa.data());
        setdp(0.0, 7 * 1000, ctx.revsrs.finisa.data());
        ctx.rev.indrev = prm::NOTSET;
        ctx.rev.indrvs(YR) = 0;
        ctx.rev.indrvs(MO) = 0;
        return;
    }
    if (ag.iagr > 0) {
        // --- HAND THE DIRECT COMPOSITE TOTAL TO THE COMPOSITE SPEC ---
        // agr1.f:75-80. Itest = (Sp, BegMo, EndMo, BegYr, EndYr) stamped by the
        // first component (editor.f:2251-2255 / agr2.f:59-63), so this is the
        // observation count of the common span.
        ag.iagr = 3;
        nobs = (ag.itest(5) - ag.itest(4)) * ag.itest(1) +
               (ag.itest(3) - ag.itest(2)) + 1;
        for (int i = 1; i <= nobs; ++i) y[i - 1] = as.o(i + ag.ind1ob - 1);
        return;
    }
    // Iagr < 0: an aggregation error was flagged; reset (agr1.f:82-84).
    ag.iagr = 0;
    ag.ncomp = 0;
}

}  // namespace x13
