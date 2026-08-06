// getrev.cpp -- faithful port of getrev.f + putrev.f (see getrev.hpp).
#include "x11/getrev.hpp"

#include "common/x13context.hpp"
#include "specparse/specparse.hpp"  // writln, stdio::STDERR

namespace x13 {

namespace {
constexpr double PCT = 100.0;
constexpr int MONE = -1;

// getrev.f:73-79 / :127-134 -- the SA percent-change warning, and :105-109 /
// :139-144 -- the trend one. Both are emitted from two places apiece with the
// same wording, so they are helpers here; the Fortran repeats the literals.
void warn_sa(X13Context& ctx) {
    writln(ctx,
           "WARNING: Revisions history analysis of the percent changes of the",
           stdio::STDERR, ctx.units.mt2, true);
    writln(ctx,
           "         seasonally adjusted series has ceased due to negative values",
           stdio::STDERR, ctx.units.mt2, false);
    writln(ctx, "         in the seasonally adjusted series.", stdio::STDERR,
           ctx.units.mt2, false);
}

void warn_trn(X13Context& ctx) {
    writln(ctx,
           "WARNING: Revisions history analysis of the percent changes of the",
           stdio::STDERR, ctx.units.mt2, true);
    writln(ctx, "         trend has ceased due to negative values in the trend.",
           stdio::STDERR, ctx.units.mt2, false);
}
}  // namespace

void putrev(const X13Context& ctx, const double* inrev, double& outrev,
            double& outch, double& outind, int iptr, bool lrv, bool& lrvch,
            int muladd, int itype, int& rvdiff, int indrev) {
    if (lrv) {
        outrev = inrev[iptr - 1];
        if (itype == 0) {
            if (muladd != 1) outrev = outrev * PCT;
        } else if (itype == 1) {
            // putrev.f:25-30 -- a composite COMPONENT folds its concurrent SA
            // into the shared indirect accumulator, by its own series{comptype=}
            // (Iag: 0 add, 1 sub, 2 mult, 3 div) and series{compwt=} (W). All
            // three come from /agr/, NOT from getrev's arguments.
            //
            // agr1.f:62-63 initializes the accumulator to ZERO for every
            // comptype, so a composite whose FIRST component is mult/div would
            // multiply into 0 and stay there. Measured, that is unreachable
            // rather than a defect: the same zero start applies to agr's
            // aggregation of the ORIGINAL series, so a mult-first metafile hands
            // the total an all-zero series and the oracle refuses the run
            // outright ("All data values read into X-13ARIMA-SEATS are equal to
            // zero"). A mult component AFTER an additive one accumulates from a
            // nonzero base and is reproduced exactly (verified on the oracle).
            const agr_cmn& ag = ctx.agr;
            if (ag.iagr == 2 && ag.iag >= 0 && indrev > 0) {
                if (ag.iag == 0) outind = outind + (inrev[iptr - 1] * ag.w);
                if (ag.iag == 1) outind = outind - (inrev[iptr - 1] * ag.w);
                if (ag.iag == 2) outind = outind * (inrev[iptr - 1] * ag.w);
                if (ag.iag == 3) outind = outind / (inrev[iptr - 1] * ag.w);
            }
        }
    }
    if (lrvch) {
        outch = inrev[iptr - 1] - inrev[iptr - 2];
        // putrev.f:36-41 -- additive mode with additivesa=percent (Rvdiff==2):
        // a non-positive previous value makes the percent change meaningless,
        // so the CHANGE history is switched off for the rest of the run. Note
        // `Lrvch` is the live COMMON flag and this write persists; `Rvdiff` is
        // getrev's local copy, so only the warning it raises is visible.
        if (muladd == 1 && rvdiff == 2) {
            if (inrev[iptr - 2] <= 0.0) {
                lrvch = false;
                rvdiff = -1;
            }
        }
        if (muladd != 1 || rvdiff == 2) outch = (outch / inrev[iptr - 2]) * PCT;
    }
}

void getrev(X13Context& ctx, const double* srs, int lstobs, int muladd,
            int itype, int ny, int iag, int iagr) {
    rev_cmn& rev = ctx.rev;
    revtrg_cmn& rt = ctx.revtrg;
    revsrs_cmn& rs = ctx.revsrs;

    bool ltemp = false;
    int rvd = rev.rvdiff;
    double tmp = 0.0;   // getrev.f's `tmp`: the scratch every unused output takes

    if (itype == 0) {
        if (rev.revptr > 0)
            putrev(ctx, srs, rs.cncsf(rev.revptr), tmp, tmp, lstobs, rev.lrvsf,
                   ltemp, muladd, itype, rvd, rev.indrev);
        // getrev.f:40-46 -- at a year boundary the Ny forecast-region factors
        // are the concurrent PROJECTED factors for the Ny rows that follow.
        if (lstobs % ny == 0) {
            for (int i = 1; i <= ny; ++i) {
                const int pptr = rev.revptr + i;
                if (pptr > 0)
                    putrev(ctx, srs, rs.cncsfp(pptr), tmp, tmp, lstobs + i,
                           rev.lrvsf, ltemp, muladd, itype, rvd, rev.indrev);
            }
        }
    } else if (rev.revptr > 0) {
        if (itype == 1) {
            putrev(ctx, srs, rs.cncsa(rev.revptr), rs.cncch(rev.revptr),
                   rs.cncisa(rev.revptr), lstobs, rev.lrvsa, rev.lrvch, muladd,
                   itype, rvd, rev.indrev);
            // getrev.f:58-71 -- the alternate revision targets. This span is the
            // "Targsa(i) periods later" estimate for the row that far back; the
            // list is sorted ascending and the DO WHILE stops at the first lag
            // this span cannot serve.
            if (rt.ntarsa > 0) {
                int i = 1;
                while (i <= rt.ntarsa) {
                    if (rev.revptr > rt.targsa(i)) {
                        const int i1 = rev.revptr - rt.targsa(i);
                        const int i2 = lstobs - rev.revptr + i1;
                        putrev(ctx, srs, rs.finsa(i, i1), rs.finch(i, i1),
                               rs.finisa(i, i1), i2, rev.lrvsa, rev.lrvch,
                               muladd, itype, rvd, rev.indrev);
                        i = i + 1;
                    } else {
                        i = rt.ntarsa + 1;
                    }
                }
            }
            if (rvd < 0) warn_sa(ctx);
        } else {
            putrev(ctx, srs, rs.cnctrn(rev.revptr), rs.cnctch(rev.revptr), tmp,
                   lstobs, rev.lrvtrn, rev.lrvtch, muladd, itype, rvd,
                   rev.indrev);
            if (rt.ntartr > 0) {
                int i = 1;
                while (i <= rt.ntartr) {
                    if (rev.revptr > rt.targtr(i)) {
                        const int i1 = rev.revptr - rt.targtr(i);
                        const int i2 = lstobs - rev.revptr + i1;
                        putrev(ctx, srs, rs.fintrn(i, i1), rs.fintch(i, i1), tmp,
                               i2, rev.lrvtrn, rev.lrvtch, muladd, itype, rvd,
                               rev.indrev);
                        i = i + 1;
                    } else {
                        i = rt.ntartr + 1;
                    }
                }
            }
            if (rvd < 0) warn_trn(ctx);
        }
    }
    // getrev.f:116-146 -- on the LAST span (the one with all the data) the same
    // buffer additionally yields the whole FINAL column: row i is read at
    // Lstobs-Revptr+i.
    if (rev.revptr < (rev.endrev - rev.begrev + 1) || rvd < 0) return;
    if (itype == 1 && iagr == 2 && iag >= 0) rev.nrcomp = rev.nrcomp + 1;
    for (int i = 1; i <= rev.revptr; ++i) {
        const int lli = lstobs - rev.revptr + i;
        if (itype == 0) {
            putrev(ctx, srs, rs.finsf(i), tmp, tmp, lli, rev.lrvsf, ltemp,
                   muladd, itype, rvd, rev.indrev);
        } else if (itype == 1) {
            putrev(ctx, srs, rs.finsa(0, i), rs.finch(0, i), rs.finisa(0, i),
                   lli, rev.lrvsa, rev.lrvch, muladd, itype, rvd, rev.indrev);
            if (rvd == MONE) {
                warn_sa(ctx);
                rvd = rvd - 1;
            }
        } else {
            putrev(ctx, srs, rs.fintrn(0, i), rs.fintch(0, i), tmp, lli,
                   rev.lrvtrn, rev.lrvtch, muladd, itype, rvd, rev.indrev);
            if (rvd == MONE) {
                warn_trn(ctx);
                rvd = rvd - 1;
            }
        }
    }
}

}  // namespace x13
