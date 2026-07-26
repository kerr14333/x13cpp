// run_history.cpp -- see run_history.hpp for the design note and scope.
//
// The mechanism, condensed from revchk.f/setrvp.f/revdrv.f/getrev.f/putrev.f/
// prtrev.f for the airline_history gate:
//
//   revchk/setrvp: Rvstrt (parsed) + Rvend (=end of series) -> the loop bounds
//     Begrev = (Rvstrt - Begspn) + Lfda, Endrev = Llda, Endtbl = Endsa,
//     Revnum = Endtbl - Begrev, with Lfda=1/Llda=Nspobs the main run's
//     Pos1ob/Posfob (no backcasts here -> the padded buffer position equals the
//     absolute series position 1..Nspobs).
//
//   revdrv loop: for the last-observation position i = Begrev..Endrev, replay
//     the model+X11 pipeline over the expanding window [1, i] (run_x11_span with
//     Nlen=i, Lsp=1, Nbcst2=Nbcst=0). getrev(Stci,Posfob,...,Itype=1) stores the
//     concurrent SA = Stci(i); getrev(Stc,...,Itype=2) the concurrent trend =
//     Stc(i). The final iteration (i=Endrev, full data) also fills the final
//     column Fin(0,revptr) = Stci_full(Begrev+revptr-1) (getrev.f:118-146).
//
//   prtrev: rev = (Fin - Cnc); percent (Rvper) whenever Muladd!=1 and the table
//     is a level (sadj=1 / trend=4), so rev = (Fin-Cnc)/Cnc*100. Emitted for
//     rows i=Begrev..Endtbl-1 (the last concurrent, i=Endrev, seeds Fin but has
//     no revision row).
#include "driver/run_history.hpp"

#include "common/x13context.hpp"
#include "driver/run_x11_span.hpp"
#include "x11/slidingspans.hpp"      // restor_span
#include "specparse/specparse.hpp"   // dfdate, addate
#include "gen/model.hpp"            // prm:: regression-type constants (PRG*), AR/MA

#include <algorithm>
#include <cmath>
#include <vector>

namespace x13 {

namespace {

// putrev.f:25-30 -- fold one component's value into the shared INDIRECT
// accumulator, by that component's series{comptype=} (Iag) and series{compwt=}
// (W). Iag: 0 add, 1 subtract, 2 multiply, 3 divide.
//
// agr1.f:62-63 initializes the accumulator to ZERO for every comptype, so a
// composite whose FIRST component is mult/div would multiply into 0 and stay
// there. Measured, that is unreachable rather than a defect: the same zero start
// applies to agr's aggregation of the ORIGINAL series, so a mult-first metafile
// hands the total an all-zero series and the oracle refuses the run outright
// ("All data values read into X-13ARIMA-SEATS are equal to zero"). A mult
// component AFTER an additive one accumulates from a nonzero base and is
// reproduced exactly (verified on the oracle). Left verbatim either way.
inline void ind_fold(double& acc, double v, int iag, double w) {
    const double x = v * w;
    switch (iag) {
    case 0: acc += x; break;
    case 1: acc -= x; break;
    case 2: acc *= x; break;
    case 3: acc /= x; break;
    default: break;
    }
}

// rvtdrg.f -- this span's FREE trading-day / length-of-period / user-TD
// regression coefficients, in group order, with each TRADING-DAY group followed
// by its implied contrast column -sum(b) (the Sunday, or Sat/Sun for the
// 1-coefficient form, that the group's columns leave out). Only the numbers and
// their order are ported: the column TITLES rvtdrg also builds are print
// surface, and so is the .tdh save file's own separator layout (see CB-20).
std::vector<double> rvtdrg(X13Context& ctx) {
    using namespace prm;
    const model_cmn& m = ctx.model;
    std::vector<double> out;
    int iusr = 0;
    for (int igrp = 1; igrp <= m.ngrp; ++igrp) {
        const int begcol = m.grp(igrp - 1);
        const int endcol = m.grp(igrp) - 1;
        int rtype = m.rgvrtp(begcol);
        const bool is_td =
            rtype == PRGTTD || rtype == PRGTST || rtype == PRRTTD ||
            rtype == PRRTST || rtype == PRATTD || rtype == PRATST ||
            rtype == PRG1TD || rtype == PRR1TD || rtype == PRA1TD ||
            rtype == PRG1ST || rtype == PRR1ST || rtype == PRA1ST ||
            rtype == PRGUTD;
        const bool is_len =
            rtype == PRGTLM || rtype == PRGTSL || rtype == PRGTLQ ||
            rtype == PRGTLY || rtype == PRRTLM || rtype == PRRTSL ||
            rtype == PRRTLQ || rtype == PRRTLY || rtype == PRATLM ||
            rtype == PRATSL || rtype == PRATLQ || rtype == PRATLY ||
            rtype == PRGULM || rtype == PRGULQ || rtype == PRGULY;
        const bool is_usr =
            rtype == PRGTUD || (rtype >= PRGTUH && rtype <= PRGUH5) ||
            rtype == PRGTUS || rtype == PRGUTD || rtype == PRGULM ||
            rtype == PRGULQ || rtype == PRGULY || rtype == PRGUAO ||
            rtype == PRGULS || rtype == PRGUCN || rtype == PRGUCY ||
            rtype == PRGUSO;
        if (!(is_td || is_len || is_usr)) continue;
        double sumtd = 0.0;
        bool any = false;
        for (int icol = begcol; icol <= endcol; ++icol) {
            bool good = false;
            if (is_usr) {
                iusr += 1;
                // A generic user column takes the usertype= it was declared
                // with; only the TD/length-of-period ones are collected.
                if (rtype == PRGTUD) rtype = ctx.usrreg.usrtyp(iusr);
                if (rtype == PRGUTD || rtype == PRGULM || rtype == PRGULQ ||
                    rtype == PRGULY)
                    good = !m.regfx(icol);
            } else {
                good = !m.regfx(icol);
            }
            if (!good) continue;
            out.push_back(ctx.mdldat.b(icol));
            sumtd += ctx.mdldat.b(icol);
            any = true;
        }
        // The contrast column is appended for a TD group whether or not any of
        // its own columns were free (rvtdrg.f:96 is outside the itd>oldtd test);
        // with everything fixed it is a plain 0 - 0.
        (void)any;
        if (is_td) out.push_back(0.0 - sumtd);
    }
    return out;
}

// rvfixd.f -- history{fixreg=}: hold every regressor belonging to one of the
// named GROUPS fixed for the whole analysis. Called twice by revdrv.f:131-134,
// once for the regARIMA design and once for the x11regression design, which is
// why it takes its arrays rather than reading model.cmn directly.
//
// A user-defined column carries the group it was DECLARED with (usertype=), not
// the generic PRGTUD it occupies in Rgvrtp, so the walk has to advance the
// user-column cursor `iusr` in lockstep -- and, faithfully, it advances that
// cursor for the user HOLIDAY and SEASONAL types too without substituting them
// (only PRGTUD is re-typed). Note that `usrfix` and `tdfix` both claim the
// user-TD/length-of-period types, so fixreg=(td) fixes a usertype=td column.
void rvfixd(bool tdfix, bool holfix, bool otlfix, bool usrfix, int& iregfx,
            x13::farray1<bool, 80>& regfx, int nb,
            const x13::farray1<int, 80>& rgvrtp, int nusrrg,
            const x13::farray1<int, 52>& usrtyp, int ncusrx, bool& userfx) {
    using namespace prm;
    int iusr = 1;
    bool allfix = true;
    for (int i = 1; i <= nb; ++i) {
        int rtype = rgvrtp(i);
        if (nusrrg > 0) {
            if (rtype == PRGTUD) {
                rtype = usrtyp(iusr);
                iusr += 1;
            } else if ((rtype >= PRGTUH && rtype <= PRGUH5) ||
                       rtype == PRGTUS) {
                iusr += 1;
            }
        }
        const bool istd =
            (rtype == PRGTTD || rtype == PRGTST || rtype == PRRTTD ||
             rtype == PRRTST || rtype == PRATTD || rtype == PRATST ||
             rtype == PRG1TD || rtype == PRR1TD || rtype == PRA1TD ||
             rtype == PRG1ST || rtype == PRR1ST || rtype == PRA1ST) ||
            (rtype == PRGTLM || rtype == PRGTSL || rtype == PRGTLQ ||
             rtype == PRGTLY || rtype == PRRTLQ || rtype == PRRTLM ||
             rtype == PRRTSL || rtype == PRATSL || rtype == PRRTLY ||
             rtype == PRATLM || rtype == PRATLQ || rtype == PRATLY) ||
            rtype == PRGUTD || rtype == PRGULY || rtype == PRGULM ||
            rtype == PRGULQ;
        const bool ishol =
            rtype == PRGTEA || rtype == PRGTEC || rtype == PRGTES ||
            rtype == PRGTLD || rtype == PRGTTH ||
            (rtype >= PRGTUH && rtype <= PRGUH5);
        const bool isusr =
            rtype == PRGTUD || rtype == PRGTUS ||
            (rtype >= PRGTUH && rtype <= PRGUH5) || rtype == PRGUTD ||
            rtype == PRGULY || rtype == PRGULM || rtype == PRGULQ ||
            rtype == PRGUAO || rtype == PRGULS || rtype == PRGUCN ||
            rtype == PRGUCY || rtype == PRGUSO;
        const bool isotl =
            rtype == PRGTAO || rtype == PRGTLS || rtype == PRGTRP ||
            rtype == PRGTTC || rtype == PRGTSO || rtype == PRGTAL ||
            rtype == PRGTAA || rtype == PRGTAT || rtype == PRGTQD ||
            rtype == PRGTQI || rtype == PRGTTL || rtype == PRGUAO ||
            rtype == PRGULS || rtype == PRGUSO;
        if ((tdfix && istd) || (holfix && ishol) || (usrfix && isusr) ||
            (otlfix && isotl)) {
            regfx(i) = true;
            if (iregfx <= 1) iregfx = 2;
        }
        allfix = allfix && regfx(i);
    }
    if (allfix && iregfx == 2) iregfx = 3;
    if (!userfx) userfx = (usrfix && ncusrx > 0);
}

}  // namespace

bool run_history(X13Context& ctx, const std::vector<double>& trnsrs_full,
                 const int* begspn_full, int nspobs_full, int nfcst_full,
                 const int* endmdl_full) {
    ctx.hist_out = HistoryOutput{};
    if (!ctx.captured.has_history) return true;   // history{} not requested
    const rev_cmn& rev = ctx.rev;
    const bool lrvsa = rev.lrvsa;
    const bool lrvtrn = rev.lrvtrn;
    const bool lrvch = rev.lrvch;                  // sadjchng (chr/che)
    const bool lrvsf = rev.lrvsf;                  // seasonal (sfr/sfe)
    const bool lrvtch = rev.lrvtch;                // trendchng (tcr/tce)
    // revchk.f:424-467 -- the forecast-error history needs forecasts to exist,
    // and its lag list has its own defaults/validation.
    bool lrvfct = rev.lrvfct;
    int nfctlg = rev.nfctlg;
    int rfctlg[4] = {rev.rfctlg(1), rev.rfctlg(2), rev.rfctlg(3), rev.rfctlg(4)};
    if (lrvfct) {
        if (nfcst_full == 0) {
            lrvfct = false;                        // "no forecasts specified"
            for (int& v : rfctlg) v = 0;
            nfctlg = 0;
        } else if (nfctlg == 0) {
            nfctlg = 2;
            rfctlg[0] = 1;
            if (nfcst_full >= ctx.model.sp)   rfctlg[1] = ctx.model.sp;
            else if (nfcst_full > 1)          rfctlg[1] = nfcst_full;
            else                              nfctlg = 1;
        } else {
            for (int i = 0; i < nfctlg; ++i)
                if (rfctlg[i] > nfcst_full) lrvfct = false;  // lag past maxlead
            if (lrvfct) std::sort(rfctlg, rfctlg + nfctlg);
        }
    }
    // revchk.f:161-192 -- the model histories need a regARIMA model to exist.
    const bool lrvaic = rev.lrvaic && ctx.captured.has_model;
    const bool lrvarma = rev.lrvarma && ctx.captured.has_model;
    bool lrvtdrg = rev.lrvtdrg && ctx.captured.has_model;
    // revchk.f:230-287 -- the trading-day COEFFICIENT history has nothing to
    // report unless a free TD regressor exists to re-estimate: the oracle drops
    // it (with an ERROR) when there is no TD regressor at all, when fixreg=(td)
    // is holding them, or when they are all individually fixed (fixmdl=yes being
    // one way to get there). The same user-column cursor walk as rvfixd.
    if (lrvtdrg) {
        using namespace prm;
        const model_cmn& m = ctx.model;
        int ntd = 0, iusr = 1;
        bool isfixed = true;
        for (int icol = 1; icol <= m.nb; ++icol) {
            int rtype = m.rgvrtp(icol);
            if (ctx.x11adj.nusrrg > 0) {
                if (rtype == PRGTUD) {
                    rtype = ctx.usrreg.usrtyp(iusr);
                    iusr += 1;
                } else if ((rtype >= PRGTUH && rtype <= PRGUH5) ||
                           rtype == PRGTUS) {
                    iusr += 1;
                }
            }
            const bool istd =
                (rtype == PRGTTD || rtype == PRGTST || rtype == PRRTTD ||
                 rtype == PRRTST || rtype == PRATTD || rtype == PRATST ||
                 rtype == PRG1TD || rtype == PRR1TD || rtype == PRA1TD ||
                 rtype == PRG1ST || rtype == PRR1ST || rtype == PRA1ST) ||
                (rtype == PRGTLM || rtype == PRGTSL || rtype == PRGTLQ ||
                 rtype == PRGTLY || rtype == PRRTLM || rtype == PRRTSL ||
                 rtype == PRRTLQ || rtype == PRRTLY || rtype == PRATLM ||
                 rtype == PRATSL || rtype == PRATLQ || rtype == PRATLY) ||
                (rtype == PRGUTD || rtype == PRGULM || rtype == PRGULQ ||
                 rtype == PRGULY);
            if (istd) {
                ntd += 1;
                if (!rev.revfix) isfixed = isfixed && m.regfx(icol);
            }
        }
        bool tdfix = false;
        if (ntd > 0 && rev.nrvfxr > 0)
            for (int i = 1; i <= rev.nrvfxr; ++i)
                if (rev.rvfxrg(i) == 1) tdfix = true;
        if (ntd == 0 || tdfix || isfixed) lrvtdrg = false;
    }
    if (!(lrvsa || lrvtrn || lrvch || lrvsf || lrvtch || lrvfct ||
          lrvaic || lrvarma || lrvtdrg))
        return true;                               // nothing this driver emits

    const int ny = ctx.model.sp;
    const bool has_model = ctx.captured.has_model;

    // --- composite{}: the INDIRECT seasonally-adjusted revision history -------
    // Iagr==2 is a COMPONENT of a composite run (agr2.f:58); Iagr>=5 is the
    // aggregate TOTAL after agr2's indirect tail (agr2.f:73). putrev.f:25-30
    // makes each component fold its own concurrent/final SA into the shared
    // /revdta/ Cncisa/Finisa accumulator by the component's own comptype (Iag:
    // 0 add, 1 sub, 2 mult, 3 div) and compwt (W); the total then prints the
    // aggregate against itself (revdrv.f:838-846). The accumulator, Nrcomp and
    // Indrev/Indrvs are COMMONs that outlive a spec, so the metafile driver
    // carries them (tools/x13run_composite.cpp) exactly like /mq11/ and /agreg/.
    const int iagr = ctx.agr.iagr;
    const int iag = ctx.agr.iag;
    const bool ind_comp = (iagr == 2 && iag >= 0);
    const bool ind_acc = ind_comp && ctx.rev.indrev > 0 && lrvsa;
    // revchk.f:547-551 -- a component that did not reach getrev (no seasonal
    // adjustment revision history of its own) leaves Nrcomp short of Ncomp, and
    // the indirect analysis is dropped. Ported with its guard: the test is the
    // ELSE of `IF(Kfulsm.ge.1)` at revchk.f:475, so on a summary/trend-only run
    // (x11{type=}) the counts are never compared at all.
    if (ctx.x11opt.kfulsm < 1 && iagr >= 5 &&
        ctx.agr.ncomp != ctx.rev.nrcomp && ctx.rev.indrev > 0)
        ctx.rev.indrev = 0;

    // --- revchk.f / setrvp.f: loop bounds -------------------------------------
    int rvstrt[2] = {rev.rvstrt(1), rev.rvstrt(2)};
    int rvend[2] = {rev.rvend(1), rev.rvend(2)};
    // revchk.f:569-608 -- history{start=} is optional; with none given the
    // analysis begins a fixed number of years into the span. strtyr(-1:5) is
    // indexed by Ltmax, the LONGEST seasonal moving average in use (sfmax.f),
    // because a longer filter needs more startup data; a model-only history
    // (fcst/aic/arma/td) instead wants 8 years (10 for quarterly). NOTE the
    // parse-order dependency: gtrvst's Indrev check reads Rvstrt BEFORE this
    // default is applied, so on a composite an unspecified start really does
    // disable the indirect analysis (gtrvst.f:419-427).
    if (rvstrt[1] == 0 && rvstrt[0] == 0) {
        static const int strtyr[7] = {6, 5, 6, 8, 12, 18, 6};   // strtyr(-1:5)
        const int ltmax =
            sfmax_span(ctx.x11opt.lterm, ctx.x11opt.lter.data(), ny);
        int ilt = ltmax + 1;
        if (ilt < 0) ilt = 0;
        if (ilt > 6) ilt = 6;
        const bool revsa = lrvsa || lrvsf || lrvch || lrvtrn || lrvtch;
        const bool revmdl = lrvfct || lrvaic || lrvarma || lrvtdrg;
        int nstart = strtyr[ilt] * ny;
        if (revmdl) {
            const int imdl = (ny == 4) ? 10 * ny : 8 * ny;
            nstart = (!revsa || imdl >= strtyr[ilt] * ny) ? imdl
                                                          : strtyr[ilt] * ny;
        }
        addate(begspn_full, ny, nstart, rvstrt);
    } else if (rvstrt[0] < 1900) {
        rvstrt[0] += 1900;                          // two-digit year
    }
    if (rvend[1] == 0 && rvend[0] == 0)            // default = end of series
        addate(begspn_full, ny, nspobs_full - 1, rvend);
    else if (rvend[0] < 1900)
        rvend[0] += 1900;

    // revchk.f:1025-1045 -- a forecast lead longer than the revision span has no
    // row to land in; drop it from the TOP of the (sorted) list.
    if (lrvfct && nfctlg > 0) {
        int nyrev = 0;
        dfdate(rvend, rvstrt, ny, nyrev);
        for (int i = nfctlg; i >= 1; --i)
            if (nyrev < rfctlg[i - 1]) { rfctlg[i - 1] = 0; nfctlg -= 1; }
        if (nfctlg == 0) return true;              // revchk sets Irev=0
    }

    const int lfda = 1;                            // main run Pos1ob (no backcast)
    const int llda = nspobs_full;                  // main run Posfob
    int begrev = 0;
    dfdate(rvstrt, begspn_full, ny, begrev);
    begrev += lfda;
    int endsa = 0;
    dfdate(rvend, begspn_full, ny, endsa);
    endsa += lfda;
    int endrev = llda;
    if (endsa != endrev) endsa += 1;
    const int endtbl = endsa;
    const int revnum = endtbl - begrev;            // # of revision-table rows
    if (revnum <= 0) return true;                  // not enough data (clean skip)

    // --- revdrv.f expanding-span loop -----------------------------------------
    // Each span starts at the fixed series calendar origin (Im = Begspn month,
    // Lyr = Begspn year); only the endpoint grows. run_x11_span reads Im from
    // ctx.ssap.im and the year anchor from ctx.x11opt.lyr/ny -- set them for the
    // history geometry (slidingspans left them at its last span's values).
    ctx.ssap.im = begspn_full[1];
    ctx.x11opt.lyr = begspn_full[0];
    ctx.x11opt.ny = ny;

    // setrvp.f: with Lrvsf the loop starts a year earlier, at Beglup = the
    // December (month=Ny) of the year before Rvstrt, so the year-boundary span
    // that projects the first table year's seasonal factors runs.
    int beglup = begrev;
    int frstsa = begrev;
    int lupbeg[2] = {rvstrt[0], rvstrt[1]};
    if (lrvsf) {
        lupbeg[0] = rvstrt[0] - 1;
        lupbeg[1] = ny;
        dfdate(lupbeg, begspn_full, ny, beglup);
        beglup += lfda;
        if (beglup < 1) beglup = 1;                // clamp (Frstsa floor)
        frstsa = beglup;
    }

    // revchk.f:801-805 -- fixmdl=yes already holds every parameter fixed, so the
    // "re-estimate once a year" convention is switched off (and the oracle prints
    // a NOTE saying so).
    int fixper = ctx.rev.fixper;
    if (fixper > 0 && rev.revfix) fixper = 0;
    // (revchk.f:812-816 also clears Lrfrsh here; refresh is not read by this
    // port, so there is nothing to clear.)

    // setrvp.f:64-71 -- Fixper (series{modelspan=(,0.per)}): start the loop at
    // the first occurrence of period Fixper at or before Lupbeg, so the model
    // estimation that the pre-Begrev spans share has already happened.
    if (fixper > 0 && has_model) {
        if (lupbeg[1] > fixper) {
            beglup -= (lupbeg[1] - fixper);
            lupbeg[1] = fixper;
        } else if (lupbeg[1] < fixper) {
            beglup -= (ny - (fixper - lupbeg[1]));
            lupbeg[1] = fixper;
            lupbeg[0] -= 1;
        }
        if (beglup < 1) beglup = 1;
    }

    // revdrv.f:112-134 -- history{fixreg=} (Rvfxrg): hold the named regression
    // GROUPS at the main run's converged values for every span. Unlike fixmdl
    // the ARMA parameters still re-estimate, so this rides the ordinary per-span
    // re-estimation floor rather than being bit-exact.
    //
    // Like fixmdl below, the fix only STICKS if the ssprep SNAPSHOT is fixed as
    // well: restor_span (restor.f) now restores B/Regfx/Iregfx from it before
    // every span, so writing only the live model here would be undone by the
    // very first span. (That restore is itself new -- ssprep.f:81-95's
    // regression half had been skipped as "Nb==0", which was true until a
    // span-replay spec carried a regression{} group.)
    if (rev.nrvfxr > 0 &&
        ((ctx.model.nb > 0 && ctx.model.iregfx < 3) || ctx.xrgmdl.nbx > 0)) {
        bool tdfix = false, holfix = false, usrfix = false, otlfix = false;
        for (int i = 1; i <= rev.nrvfxr; ++i) {
            switch (rev.rvfxrg(i)) {
            case 1: tdfix = true; break;
            case 2: holfix = true; break;
            case 3: usrfix = true; break;
            case 4: otlfix = true; break;
            default: break;
            }
        }
        // revdrv.f:131-134: the flags are decoded under the guard above but the
        // two rvfixd calls sit OUTSIDE it -- with Nrvfxr==0 every flag is false
        // and both calls are no-ops apart from the allfix/Iregfx bookkeeping,
        // which is why the oracle can afford to make them unconditional. This
        // port keeps them inside, where they are observationally identical.
        if (ctx.model.nb > 0)
            rvfixd(tdfix, holfix, otlfix, usrfix, ctx.model.iregfx,
                   ctx.model.regfx, ctx.model.nb, ctx.model.rgvrtp,
                   ctx.x11adj.nusrrg, ctx.usrreg.usrtyp, ctx.usrreg.ncusrx,
                   ctx.model.userfx);
        if (ctx.xrgmdl.nbx > 0)
            rvfixd(tdfix, holfix, otlfix, usrfix, ctx.xrgmdl.irgxfx,
                   ctx.xrgmdl.regfxx, ctx.xrgmdl.nbx, ctx.xrgmdl.rgxvtp,
                   ctx.xrgmdl.nusxrg, ctx.usrxrg.usxtyp, ctx.xrgmdl.nusxrg,
                   ctx.xrgmdl.usrxfx);
        // Mirror the result into the ssprep snapshot restor_span restores from.
        copylg(ctx.model.regfx.data(), prm::PB, 1, ctx.ssprep.regfx2.data());
        ctx.ssprep.irfx2 = ctx.model.iregfx;
    }

    // revdrv.f:250-262 -- history{fixmdl=yes} (Revfix): hold the WHOLE model at
    // the main run's converged values for every span, so each span re-FILTERS
    // rather than re-estimates. The oracle then re-snapshots at revdrv.f:381
    // (ssprep), which is what makes the fix survive the per-span restor -- so the
    // ssprep copy (ctx.ssprep.fxa) has to be fixed here too or restor_span would
    // undo it on the very first span. Same mechanism, same trap, as
    // slidingspans' ssmdl_fix_model. (The Userfx/bakusr arm needs user
    // regressors, which this driver does not carry.)
    // WITH x11regression{} the flag is INERT, and faithfully so: revdrv.f:309-350's
    // Ixreg block ends with `CALL loadxr(T); IF(Lmodel)CALL restor(Lmodel,F,F)`,
    // which reinstates Arimaf/Regfx/Iregfx FROM the ssprep snapshot before the
    // span loop ever starts -- and Revfix only ever set the LIVE copy (the ssprep
    // at revdrv.f:380 is commented out in the Fortran). Measured: the oracle's
    // `fixmdl=yes` and default runs are byte-identical on airline +
    // x11regression{variables=(td)} + history{}. So the ssprep mirror below,
    // which is what makes the fix survive restor_span at all, is applied only
    // when there is no x11regression -- with one, the live fix is undone by the
    // first span's restor_span exactly as the oracle's restor undoes it.
    if (rev.revfix && has_model) {
        const bool sticks = ctx.hiddn.ixreg == 0;
        for (int i = 1; i <= prm::PARIMA; ++i) {
            ctx.model.arimaf(i) = true;
            if (sticks) ctx.ssprep.fxa(i) = true;
        }
        for (int i = 1; i <= ctx.model.nb; ++i) {
            ctx.model.regfx(i) = true;
            if (sticks) ctx.ssprep.regfx2(i) = true;  // restor_span restores here
        }
        ctx.model.iregfx = 3;
        if (sticks) ctx.ssprep.irfx2 = 3;
    }

    // revdrv.f:309-330 -- history{fixx11reg=yes} (Revfxx): hold the
    // x11regression{} daily weights at the main run's values for every span. The
    // fix goes on the x11reg STORE (Irgxfx/Regfxx), not the working model, and
    // each span's loadxr(F) copies it in; x11mdl's Iregfx>=2 rmfix/addfix then
    // strikes every fixed column, so the per-span OLS has nothing to estimate.
    // Unlike the regARIMA fixes above nothing restores the store between spans,
    // so no ssprep mirror is needed. (The Usrxfx/bakusr arm needs user x11reg
    // regressors, which this driver does not carry.)
    if (rev.revfxx && ctx.hiddn.ixreg > 0 && ctx.xrgmdl.nbx > 0) {
        for (int i = 1; i <= prm::PB; ++i) ctx.xrgmdl.regfxx(i) = true;
        if (ctx.xrgmdl.irgxfx < 3) ctx.xrgmdl.irgxfx = 3;
        // revdrv.f:323-330 -- with the trading-day group held fixed there is
        // nothing for the negative-weight reweighting to act on.
        if (ctx.x11log.lxrneg) {
            const int igrp = strinx(true, ctx.xrgmdl.grpttx.raw(),
                                    ctx.xrgmdl.gpxptr.data(), 1,
                                    ctx.xrgmdl.ngrptx, "Trading Day");
            if (igrp > 0) {
                bool all = true;
                for (int c = ctx.xrgmdl.grpx(igrp - 1);
                     c <= ctx.xrgmdl.grpx(igrp) - 1; ++c)
                    all = all && ctx.xrgmdl.regfxx(c);
                ctx.x11log.lxrneg = !all;
            }
        }
    }

    const int nspan = endrev - begrev + 1;         // includes the final full span
    std::vector<double> cncsa(nspan + 1, 0.0), cnctrn(nspan + 1, 0.0);
    std::vector<double> cncch(nspan + 1, 0.0);     // concurrent SA % change (putrev)
    std::vector<double> cnctch(nspan + 1, 0.0);    // concurrent trend % change (putrev)
    std::vector<double> cncsf(nspan + 1, 0.0);     // concurrent seasonal factor (x100)
    std::vector<double> projsf(revnum + 1, 0.0);   // projected SF, by output revptr
    const double sfsc = (ctx.x11opt.muladd != 1) ? 100.0 : 1.0;  // putrev Itype=0
    // prtfct.f:613 -- Cncfct(k,Revptr+Rfctlg(k)): the ORIGINAL-scale forecast a
    // span made for the date it lands on. Indexed [k][output revptr].
    std::vector<std::vector<double>> cncfct;
    if (lrvfct)
        cncfct.assign(static_cast<std::size_t>(nfctlg),
                      std::vector<double>(static_cast<std::size_t>(nspan) + 1, 0.0));
    // The model histories: one row per span (revptr 1..nspan), variable width.
    const bool have_mdl = lrvaic || lrvarma || lrvtdrg;
    std::vector<double> revaic(nspan + 1, 0.0), rvlkhd(nspan + 1, 0.0);
    std::vector<std::vector<double>> cncarma(static_cast<std::size_t>(nspan) + 1);
    std::vector<std::vector<double>> cnctdrg(static_cast<std::size_t>(nspan) + 1);

    // revdrv.f:246 -- mdl2, the MAIN run's Endmdl: with a series{modelspan=} end
    // inside the span, every history span's model span is capped at it.
    const int mdl2[2] = {endmdl_full[0], endmdl_full[1]};
    const bool have_mdl2 = has_model && ctx.arima.ldestm && fixper <= 0;

    for (int i = beglup; i <= endrev; ++i) {
        const int revptr = i - begrev + 1;         // <=0 for the pre-Begrev spans
        // revdrv.f:432-453 -- the pre-Begrev part of the loop is NOT a plain
        // run: only i==Beglup and i==Frstsa are processed, everything between
        // them falls through the ELSE to the loop increment. With Beglup<Frstsa
        // (which only Fixper produces) the i==Beglup pass additionally runs with
        // Lx11/Lseats FALSE -- a model-only estimation whose sole purpose in the
        // oracle is to leave converged parameters behind for the spans that
        // follow. This port restores every span from the main run's snapshot
        // (restor_span) and re-estimates it, exactly as the oracle's own per-span
        // `restor` does, so that pass leaves no trace and is skipped outright.
        if (i < begrev && i != frstsa) continue;
        // restor.f: reset Lter/Ktcopt/Tic and (model) Arimap/Arimaf/... to the
        // main run's converged snapshot as this span's fresh starting state.
        // Model NOT fixed (no ssmdl_fix_model) -> rgarma re-estimates each span.
        restor_span(ctx);
        // restor_span resets Lter(1..Ny)/Ktcopt/Tic but NOT the scalar seasonal-
        // filter selector Lterm nor the Henderson length Nterm; reset those to
        // their parsed values too, so each (differently-lengthed) span re-selects
        // via MSR / the I/C ratio instead of inheriting the first span's choice.
        ctx.x11opt.lterm = ctx.saved.lterm0;
        ctx.x11opt.nterm = ctx.saved.nterm0;
        // revdrv.f:530-532 -- demote Ixreg 3->1/2 at the span head, so this span
        // re-estimates the x11regression irregular OLS on its own data instead of
        // reusing the main run's coefficients. Ixreg==2 is what makes
        // run_x11_span run the transparent xrgdrv pass (x11ari.f:88-95); at 1 the
        // inline x11mdl_td in x11pt2 does the work, which is also what the oracle
        // does there. Left at 3 (what this port used to do) every span silently
        // behaved as history{fixx11reg=yes} -- measured sar 8.2e-1 on airline +
        // x11regression{variables=(td)}, against a 5e-3 tolerance.
        if (ctx.hiddn.ixreg == 3) {
            ctx.hiddn.ixreg = 1;
            if (has_model || ctx.x11reg.fxprxr > 0 || ctx.x11opt.khol > 0)
                ctx.hiddn.ixreg = 2;
        }
        const int nlen = i;                        // Length = Posfob - Pos1ob + 1
        const int lsp = 1;                         // Pos1ob = Nbcst2(0) + Lsp = 1
        // revdrv.f:479-497 -- this span's model span end. Endspn = the span's
        // last observation; Endmdl is either the last occurrence of period
        // Fixper at or before it (the "0.per" convention), or mdl2 when the main
        // run's model span ends inside this span, or Endspn itself. nend is how
        // many periods short of Endspn the model span stops.
        int nend_mdl = 0;
        if (has_model && (fixper > 0 || have_mdl2)) {
            int endspn_i[2];
            addate(begspn_full, ny, nlen - 1, endspn_i);
            int endmdl_i[2] = {endspn_i[0], endspn_i[1]};
            if (fixper > 0) {
                if (endmdl_i[1] != fixper) {
                    if (endmdl_i[1] < fixper) endmdl_i[0] -= 1;
                    endmdl_i[1] = fixper;
                }
                // (revdrv's `addreg` flag rides along here; it only gates the
                // chkorv outlier re-introduction, which needs otlrev=remove --
                // not in this driver's scope.)
            } else {
                int nend2 = 0;
                dfdate(endspn_i, mdl2, ny, nend2);
                if (nend2 > 0) { endmdl_i[0] = mdl2[0]; endmdl_i[1] = mdl2[1]; }
            }
            dfdate(endspn_i, endmdl_i, ny, nend_mdl);
            if (nend_mdl < 0) nend_mdl = 0;
            if (nend_mdl >= nlen) nend_mdl = 0;    // defensive: keep >=1 obs
        }
        if (!run_x11_span(ctx, trnsrs_full, has_model, nlen, nfcst_full,
                          /*nbcst=*/0, /*nbcst2=*/0, lsp, nend_mdl))
            return false;
        if (ctx.error.lfatal) return false;
        const int posfob = ctx.x11ptr.posfob;      // = nlen = i
        if (revptr > 0) {                          // getrev's IF(Revptr.gt.0)
            cncsa[revptr] = ctx.x11srs.stci(posfob);
            cnctrn[revptr] = ctx.x11srs.stc(posfob);
            // putrev.f:25-30 -- this component's contribution to the INDIRECT
            // concurrent SA.
            if (ind_acc)
                ind_fold(ctx.revsrs.cncisa(revptr), ctx.x11srs.stci(posfob),
                         iag, ctx.agr.w);
            if (lrvch) {                           // putrev Outch on this span's Stci
                const double a = ctx.x11srs.stci(posfob);
                const double b = ctx.x11srs.stci(posfob - 1);
                cncch[revptr] = ((a - b) / b) * 100.0;  // Muladd!=1 -> percent change
            }
            if (lrvsf) cncsf[revptr] = ctx.x11srs.sts(posfob) * sfsc;
            if (lrvtch) {                          // putrev Outch on this span's Stc
                const double a = ctx.x11srs.stc(posfob);
                const double b = ctx.x11srs.stc(posfob - 1);
                cnctch[revptr] = ((a - b) / b) * 100.0;
            }
            if (lrvfct) {
                // prtfct.f:628-641. ctx.forecasts.fcst IS prtfct's untfct: the
                // original-scale point forecast with the length-of-period /
                // leap-year prior reapplied (fcstout mirrors the LFOROS path,
                // which is the branch prtfct takes whenever the fct table is
                // printed or saved -- and the eltfcn folds its no-table branch
                // adds are already inside that same value).
                const auto& fc = ctx.forecasts.fcst;
                for (int k = 1; k <= nfctlg; ++k) {
                    const int lag = rfctlg[k - 1];
                    const int outr = revptr + lag;
                    if (outr <= nspan && lag <= static_cast<int>(fc.size()))
                        cncfct[static_cast<std::size_t>(k - 1)][outr] = fc[lag - 1];
                }
            }
            if (lrvaic) {                          // revdrv.f:671-673
                revaic[revptr] = ctx.lkhd.aicc;
                rvlkhd[revptr] = ctx.lkhd.olkhd;
            }
            if (lrvarma) {                         // rvarma.f
                auto& row = cncarma[static_cast<std::size_t>(revptr)];
                const model_cmn& m = ctx.model;
                for (int iflt = prm::AR; iflt <= prm::MA; ++iflt)
                    for (int iopr = m.mdl(iflt - 1); iopr <= m.mdl(iflt) - 1; ++iopr)
                        for (int ilag = m.opr(iopr - 1); ilag <= m.opr(iopr) - 1; ++ilag)
                            if (!m.arimaf(ilag))
                                row.push_back(ctx.mdldat.arimap(ilag));
            }
            if (lrvtdrg) cnctdrg[static_cast<std::size_t>(revptr)] = rvtdrg(ctx);
        }
        // getrev Itype=0: at a year-boundary span (Posfob a multiple of Ny =
        // December for this Jan-start series) store the Ny projected factors
        // Sts(Posfob+k) -- the forecast-region seasonal factors -- into the
        // output row (revptr+k) they concurrently project. One December span
        // covers the next 12 months, so rows never collide.
        if (lrvsf && posfob % ny == 0) {
            for (int k = 1; k <= ny; ++k) {
                const int outr = revptr + k;
                if (outr >= 1 && outr <= revnum)
                    projsf[outr] = ctx.x11srs.sts(posfob + k) * sfsc;
            }
        }
    }

    // Final column: the loop's last iteration was i=Endrev (full data), so
    // ctx.x11srs.stci/stc now hold the final adjustment. Fin(0,revptr) is the
    // final estimate at the revision date Begrev+revptr-1 (getrev.f:118-124).
    std::vector<double> finsa(revnum + 1, 0.0), fintrn(revnum + 1, 0.0);
    std::vector<double> finch(revnum + 1, 0.0), finsf(revnum + 1, 0.0);
    std::vector<double> fintch(revnum + 1, 0.0);
    // getrev.f:117 -- this component has now contributed a full SA revision
    // history; the total compares Nrcomp against Ncomp. Counted whether or not
    // Indrev survived (the Fortran test is Itype/Iagr/Iag only).
    if (ind_comp && lrvsa) ctx.rev.nrcomp += 1;
    for (int revptr = 1; revptr <= revnum; ++revptr) {
        const int pos = begrev + revptr - 1;
        finsa[revptr] = ctx.x11srs.stci(pos);
        fintrn[revptr] = ctx.x11srs.stc(pos);
        // putrev.f:25-30 via getrev.f:124 -- the INDIRECT FINAL SA.
        if (ind_acc)
            ind_fold(ctx.revsrs.finisa(0, revptr), ctx.x11srs.stci(pos), iag,
                     ctx.agr.w);
        if (lrvch) {                               // final change from full-data Stci
            const double a = ctx.x11srs.stci(pos);
            const double b = ctx.x11srs.stci(pos - 1);
            finch[revptr] = ((a - b) / b) * 100.0;
        }
        if (lrvsf) finsf[revptr] = ctx.x11srs.sts(pos) * sfsc;
        if (lrvtch) {                              // final trend change from full-data Stc
            const double a = ctx.x11srs.stc(pos);
            const double b = ctx.x11srs.stc(pos - 1);
            fintch[revptr] = ((a - b) / b) * 100.0;
        }
    }

    // --- prtrev.f: revision arithmetic ----------------------------------------
    // Rvper: percent revision for a level table (sadj/trend) whenever Muladd!=1.
    const bool rvper = ctx.x11opt.muladd != 1;
    HistoryOutput& out = ctx.hist_out;
    out.ran = true;
    out.have_sa = lrvsa;
    out.have_tr = lrvtrn;
    out.have_ch = lrvch;
    out.have_sf = lrvsf;
    out.have_tch = lrvtch;
    out.nsea = ny;
    out.revspn[0] = rvstrt[0];
    out.revspn[1] = rvstrt[1];
    // revdrv.f:838-846 -- only the aggregate TOTAL prints the indirect table,
    // and only when every component contributed and Indrev survived. The
    // `historyindsa: yes|no` savelog line is written either way (:1199).
    if (iagr >= 5 && lrvsa) {
        out.ind_reported = true;
        out.ind_yes = (ctx.rev.nrcomp == ctx.agr.ncomp && ctx.rev.indrev > 0);
        out.have_ind = out.ind_yes;
    }
    for (int revptr = 1; revptr <= revnum; ++revptr) {
        int idate[2];
        addate(rvstrt, ny, revptr - 1, idate);
        out.dates.push_back(idate[0] * 100 + idate[1]);
        if (lrvsa) {
            const double cnc = cncsa[revptr], fin = finsa[revptr];
            double r = fin - cnc;
            if (rvper) r = (r / cnc) * 100.0;
            out.sae_cnc.push_back(cnc);
            out.sae_fin.push_back(fin);
            out.sar.push_back(r);
        }
        if (out.have_ind) {
            // prtrev Tbltyp=3: the same level-table arithmetic as Tbltyp=1, run
            // on the aggregated Cncisa/Finisa instead of this run's own SA.
            const double cnc = ctx.revsrs.cncisa(revptr);
            const double fin = ctx.revsrs.finisa(0, revptr);
            double r = fin - cnc;
            if (rvper) r = (r / cnc) * 100.0;
            out.iae_cnc.push_back(cnc);
            out.iae_fin.push_back(fin);
            out.iar.push_back(r);
        }
        if (lrvtrn) {
            const double cnc = cnctrn[revptr], fin = fintrn[revptr];
            double r = fin - cnc;
            if (rvper) r = (r / cnc) * 100.0;
            out.tre_cnc.push_back(cnc);
            out.tre_fin.push_back(fin);
            out.trr.push_back(r);
        }
        if (lrvch) {
            // Change table (Tbltyp=2): the conc/final values are already the
            // month-to-month % change; the revision is a plain difference
            // (prtrev forces Rvper=F for Tbltyp 2), no second percenting.
            const double cnc = cncch[revptr], fin = finch[revptr];
            out.che_cnc.push_back(cnc);
            out.che_fin.push_back(fin);
            out.chr.push_back(fin - cnc);
        }
        if (lrvsf) {
            // prtrv2: two revisions, Final-Conc and Final-Proj; Rvper (Muladd!=1)
            // divides each by its own base. The stored levels are already x100.
            const double cnc = cncsf[revptr], proj = projsf[revptr];
            const double fin = finsf[revptr];
            double rc = fin - cnc, rp = fin - proj;
            if (rvper) { rc = (rc / cnc) * 100.0; rp = (rp / proj) * 100.0; }
            out.sfe_cnc.push_back(cnc);
            out.sfe_proj.push_back(proj);
            out.sfe_fin.push_back(fin);
            out.sfr_cnc.push_back(rc);
            out.sfr_proj.push_back(rp);
        }
        if (lrvtch) {                              // trend-change table (Tbltyp=5, Rvper=F)
            const double cnc = cnctch[revptr], fin = fintch[revptr];
            out.tce_cnc.push_back(cnc);
            out.tce_fin.push_back(fin);
            out.tcr.push_back(fin - cnc);
        }
    }

    // --- the three model histories (revdrv.f:873-1190) ----------------------
    // Row range i=Begrev..Endrev, labelled from Rvstrt -- one row more than the
    // revision tables above.
    if (have_mdl) {
        for (int revptr = 1; revptr <= nspan; ++revptr) {
            int idate[2];
            addate(rvstrt, ny, revptr - 1, idate);
            out.mdates.push_back(idate[0] * 100 + idate[1]);
        }
        if (lrvaic) {
            out.have_aic = true;
            for (int revptr = 1; revptr <= nspan; ++revptr) {
                out.lkh_lkhd.push_back(rvlkhd[revptr]);
                out.lkh_aicc.push_back(revaic[revptr]);
            }
        }
        if (lrvarma) {
            // Nrvarma is fixed at Revptr==1 in rvarma.f, so every row is that
            // wide; a span that somehow freed a different parameter count would
            // be a model change the history loop does not allow.
            out.have_arma = true;
            out.nrvarma = static_cast<int>(cncarma[1].size());
            for (int revptr = 1; revptr <= nspan; ++revptr)
                for (int k = 0; k < out.nrvarma; ++k) {
                    const auto& row = cncarma[static_cast<std::size_t>(revptr)];
                    out.amh.push_back(k < static_cast<int>(row.size()) ? row[k] : 0.0);
                }
        }
        if (lrvtdrg) {
            out.nrvtdrg = static_cast<int>(cnctdrg[1].size());
            // revdrv.f:689 -- no free TD group at all turns the analysis off.
            out.have_tdrg = out.nrvtdrg > 0;
            if (out.have_tdrg) {
                for (int revptr = 1; revptr <= nspan; ++revptr)
                    for (int k = 0; k < out.nrvtdrg; ++k) {
                        const auto& row = cnctdrg[static_cast<std::size_t>(revptr)];
                        out.tdh.push_back(
                            k < static_cast<int>(row.size()) ? row[k] : 0.0);
                    }
            }
        }
    }

    // --- prfcrv.f: the forecast-error history -------------------------------
    // Its own row range: i = Begrev+Rfctlg(1)..Endrev, labelled from
    // begfct = Rvstrt + Rfctlg(1). fctss is the EVOLVING (running) sum of
    // squares, so it carries across rows.
    if (lrvfct) {
        out.have_fct = true;
        out.nfctlg = nfctlg;
        for (int k = 1; k <= nfctlg; ++k) out.fctlag.push_back(rfctlg[k - 1]);
        int begfct[2];
        addate(rvstrt, ny, rfctlg[0], begfct);
        // Rvtrfc (history{transformfcst=yes}): difference on the TRANSFORMED
        // scale instead of the original one.
        const double lam = ctx.arima.lam;
        const int fcntyp = ctx.arima.fcntyp;
        const bool trnfct = rev.rvtrfc && lam != 1.0;
        std::vector<double> fctss(static_cast<std::size_t>(nfctlg), 0.0);
        int j = 0, lastptr = 0;
        for (int i = begrev + rfctlg[0]; i <= endrev; ++i) {
            const int revptr = i - begrev + 1;
            j += 1;
            lastptr = revptr;
            int idate[2];
            addate(begfct, ny, j - 1, idate);
            out.fdates.push_back(idate[0] * 100 + idate[1]);
            for (int k = 1; k <= nfctlg; ++k) {
                // prfcrv's ndef cut: with more than one lag, lag k only has a
                // stored forecast once j has reached it. Undefined lags are
                // written as an exact 0 in the save file.
                const bool defined = (nfctlg == 1) || (rfctlg[k - 1] <= j);
                if (!defined) {
                    out.fce.push_back(0.0);
                    out.fch_fcst.push_back(0.0);
                    out.fch_err.push_back(0.0);
                    continue;
                }
                const double obs = ctx.inpt.orig(i);
                const double cnc = cncfct[static_cast<std::size_t>(k - 1)][revptr];
                double err, shown;
                if (trnfct) {
                    if (lam == 0.0) {
                        err = std::log(obs) - std::log(cnc);
                        shown = std::log(cnc);
                    } else if (fcntyp == 3) {      // logistic
                        const double t1 = std::log(obs / (1.0 - obs));
                        const double t2 = std::log(cnc / (1.0 - cnc));
                        err = t1 - t2;
                        shown = t2;
                    } else {
                        const double t1 = lam * lam + (std::pow(obs, lam) - 1.0) / lam;
                        const double t2 = lam * lam + (std::pow(cnc, lam) - 1.0) / lam;
                        err = t1 - t2;
                        shown = t2;
                    }
                } else {
                    err = obs - cnc;
                    shown = cnc;
                }
                fctss[static_cast<std::size_t>(k - 1)] += err * err;
                out.fce.push_back(fctss[static_cast<std::size_t>(k - 1)]);
                out.fch_fcst.push_back(shown);
                out.fch_err.push_back(err);
            }
        }
        // meanssfe: fctss(k)/(Revptr-Rfctlg(k)) at the last row (prfcrv.f:213).
        for (int k = 1; k <= nfctlg; ++k)
            out.meanssfe.push_back(fctss[static_cast<std::size_t>(k - 1)] /
                                   static_cast<double>(lastptr - rfctlg[k - 1]));
    }
    return true;
}

}  // namespace x13
