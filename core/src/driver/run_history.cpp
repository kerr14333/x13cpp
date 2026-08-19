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
#include "driver/rev_outlier.hpp"  // rmotrv, chkorv (held-back outliers)
#include "x11/loadxr.hpp"          // loadxr (regARIMA <-> x11reg model swap)
#include "regarima/rvfixd.hpp"     // rvfixd (fixreg= group walk, shared with ssmdl/ssxmdl)
#include "regarima/outlier.hpp"    // wrtdat (errhdr's per-span date string)
#include "x13/fformat.hpp"        // fwrite_fmt (revchk.f:1131/1135)
#include "specparse/specparse.hpp"   // dfdate, addate
#include "gen/model.hpp"            // prm:: regression-type constants (PRG*), AR/MA
#include "gen/notset.hpp"           // prm::DNOTST (prtrev's "no such estimate yet")

#include <algorithm>
#include <cmath>
#include <vector>

namespace x13 {

namespace {

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

}  // namespace

bool run_history(X13Context& ctx, const std::vector<double>& trnsrs_full,
                 const int* begspn_full, int nspobs_full, int nfcst_full,
                 const int* endmdl_full) {
    ctx.hist_out = HistoryOutput{};
    if (!ctx.captured.has_history) return true;   // history{} not requested
    // revdrv.f:5 takes Lseats and hands it to x11ari, which substitutes the
    // SEATS chain for x11pt3 per span (seatdg.f:148-181 then feeds the same
    // getrev). Derived exactly as run_slidingspans derives it.
    const bool lseats = ctx.captured.has_seats && !ctx.captured.has_x11;
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
    // revchk.f:54 -- `usstrt` is "the user gave a start date", tested BEFORE
    // the default below fills rvstrt in. It picks FORMAT 1050 over 1060 for the
    // dropped-lag NOTEs: 1050 names the start date, 1060 says "from default
    // starting date".
    //
    // INCOMPLETE, and deliberately so: revchk.f:678 and :756 flip it back to F
    // after printing "the start of the history analysis has been advanced to",
    // and neither of those blocks is ported (no golden carries them). So this
    // flag is right only while they are missing. **Port the flip in the same
    // commit as either block** -- otherwise a run whose start was advanced
    // will name a date the oracle stopped naming.
    const bool usstrt = rev.rvstrt(1) > 0;
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
    // revchk.f files these in /revcmn/. They used to stay local here because
    // nothing downstream read them; getrev, which x11pt3 now calls inside every
    // span, reads Begrev/Endrev/Revptr from the COMMON.
    ctx.rev.begrev = begrev;
    ctx.rev.endrev = endrev;

    // --- the alternate revision TARGETS (history{sadjlags=/trendlags=}) -------
    // setrvp.f:26-40: with targets the span loop has to run mxrlag periods
    // further than the table, since the "N later" estimate for the LAST table
    // row is made by a span N periods past it. Endtbl/Revnum are already fixed
    // (setrvp sets them before this block), so only Endsa widens -- and Endsa is
    // what prtrev's DNOTST mask keys on. Note the order in revchk.f: setrvp runs
    // BEFORE the list is validated, so a lag that is about to be dropped still
    // widens Endsa here.
    revtrg_cmn& rt = ctx.revtrg;
    if (rt.ntarsa > 0 || rt.ntartr > 0) {
        int mxrlag = 0;
        for (int i = 1; i <= rt.ntarsa; ++i)
            if (mxrlag < rt.targsa(i)) mxrlag = rt.targsa(i);
        for (int i = 1; i <= rt.ntartr; ++i)
            if (mxrlag < rt.targtr(i)) mxrlag = rt.targtr(i);
        endsa += mxrlag;
        if (endsa > llda) endsa = llda;
    }
    // revchk.f:1053-1110 -- sort each list ascending (intsrt) and drop, from the
    // top, any lag that does not fit inside the revision span (the oracle prints
    // a NOTE and zeroes the entry). Lr1y2y is then "both a 1-year and a 2-year
    // lag survive", which adds the extra Fin(2yr)-Fin(1yr) column.
    bool r1y2y = false;
    if (rt.ntarsa > 0 || rt.ntartr > 0) {
        int nyrev = 0;
        dfdate(rvend, rvstrt, ny, nyrev);
        // CB-22: Lr1y2y is a single flag shared by BOTH tables' prtrev calls
        // (revdrv.f:825/851 pass the same one), and the trend block below
        // overwrites whatever the sadj block decided. So `sadjlags=(12 24)`
        // together with a `trendlags=` that is not also a 1yr/2yr pair silently
        // costs the SA table its (1yr-2yr) column -- and vice versa. Transcribed
        // as written; see tools/census_bugs.md.
        auto validate = [&](farray1<int, 5>& targ, int& ntarg,
                            const char* what) {
            std::sort(targ.data(), targ.data() + ntarg);
            int i2 = 0;
            for (int i = ntarg; i >= 1; --i) {
                if (nyrev <= targ(i)) {            // no room for this lag
                    // revchk.f:1058-1071 (and :1089-1102 for trends) -- and the
                    // oracle SAYS which lag it dropped. The drop was ported and
                    // the sentence was not, which is the fourth time in this
                    // inventory (editor.f:400, editor.f:2831's neighbour,
                    // idotlr.f:494, revchk.f:801). The Mt2 half only; the
                    // STDERR half is `IF(.not.Lquiet)` and Lquiet is not
                    // modelled here.
                    // The FORMAT is ONE write, not two: after the date there
                    // are TWO consecutive slashes, so an empty record sits
                    // between the "at lag" line and the "See" line. Splitting
                    // it into a body plus a tail lost exactly that record --
                    // which is the only thing the gate can see, since _trim
                    // rstrips content but never drops a line (entry 119).
                    auto& mt2 = ctx.channels_.unit(ctx.units.mt2);
                    const std::string sec = "Section 7";
                    const std::string prg = "X-13ARIMA-SEATS";
                    const std::string doc = "Reference Manual";
                    if (usstrt)
                        mt2.put(fwrite_fmt(
                            "(/,' NOTE: Not enough data to perform a history ',"
                            "'analysis for ',a,/,"
                            "'       at lag ',i2,' starting in ',a,'.',/,"
                            "/,'       See ',a,' of the ',a,' ',a,'.')",
                            std::string(what), targ(i), wrtdat(rvstrt, ny),
                            sec, prg, doc) + "\n");
                    else
                        mt2.put(fwrite_fmt(
                            "(/,' NOTE: Not enough data to perform a history ',"
                            "'analysis for ',a,/,"
                            "'       at lag ',i2,' from default starting date.',"
                            "/,'       See ',a,' of the ',a,' ',a,'.')",
                            std::string(what), targ(i), sec, prg, doc) +
                            "\n");
                    targ(i) = 0;
                    ntarg -= 1;
                } else if (targ(i) == ny || targ(i) == 2 * ny) {
                    i2 += 1;
                }
            }
            r1y2y = (i2 == 2);
        };
        if (rt.ntarsa > 0)
            validate(rt.targsa, rt.ntarsa, "seasonal adjustments");
        if (rt.ntartr > 0) validate(rt.targtr, rt.ntartr, "trends");
    }
    const int ntarsa = rt.ntarsa, ntartr = rt.ntartr;
    const bool cnctar = ctx.rev.cnctar;
    // prtrev.f:90-91 -- the revision tables carry one column per surviving lag,
    // plus the (1yr-2yr) one; the level tables carry only the lags. The TREND
    // and trend-change calls pass Lr1y2y as a literal F (revdrv.f:852/859), so
    // that extra column exists only on the SA / SA-change / indirect tables --
    // even though revchk derives the flag from BOTH lists (CB-22).
    //
    // The `ntarsa > 0` half of the guard below is a DELIBERATE, DOCUMENTED
    // deviation, not a transcription of prtrev.f:90-91 (which adds the column
    // on Lr1y2y alone). It only differs for `trendlags=(Ny 2Ny)` with NO
    // `sadjlags=`: revchk lets the trend list set the shared Lr1y2y (CB-22), so
    // Ntarsa==0 and Lr1y2y==T reach prtrev together. There the oracle sizes the
    // SA table to one column and then never writes it -- prtrev.f:202's
    // `IF(Lr1y2y.and.i2.gt.0)` sits inside `DO i2=0,Ntargt`, which with
    // Ntargt==0 runs the i2==0 pass only, so `rev(ncol,.)` is emitted straight
    // out of uninitialised storage. That is CB-22's second face: a column whose
    // VALUES are whatever memory held. Reproducing a specific garbage value is
    // not portable, so this port omits the column rather than fabricate one.
    // Nothing in the corpus reaches it (every target spec sets sadjlags).
    const int ncol_sa = ntarsa + ((r1y2y && ntarsa > 0) ? 1 : 0);
    const int ncol_tr = ntartr;
    // Fin(1:Ntargt, .) for each table family, filled during the span loop.
    const int nt_rows = revnum + 1;
    // (the INDIRECT target columns go straight into /revdta/ Finisa(1:,.), the
    // shared accumulator every component folds into -- same as Finisa(0,.).)
    std::vector<std::vector<double>> finsa_t, finch_t, fintrn_t, fintch_t;
    if (ntarsa > 0) {
        finsa_t.assign(ntarsa + 1, std::vector<double>(nt_rows, 0.0));
        finch_t.assign(ntarsa + 1, std::vector<double>(nt_rows, 0.0));
    }
    if (ntartr > 0) {
        fintrn_t.assign(ntartr + 1, std::vector<double>(nt_rows, 0.0));
        fintch_t.assign(ntartr + 1, std::vector<double>(nt_rows, 0.0));
    }

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
    // "re-estimate once a year" convention is switched off, and the oracle SAYS
    // so. The comment here has claimed that parenthetically since the rule was
    // ported; the two writln lines under it were not, which is the third time
    // in this file's inventory that a ported rule kept its diagnostic unported
    // (see also editor.f:400 and idotlr.f:494).
    //
    // `Fhnote` is `STDERR` (revdrv.f:85) -- `Lquiet` would make it 0, and this
    // port does not model Lquiet, so there is no second arm to write.
    //
    // The oracle zeroes the COMMON `Fixper`; this port zeroes a LOCAL and
    // threads it on. Checked rather than assumed: `ctx.rev.fixper` is written
    // once (series.cpp:343) and read nowhere else, and revdrv.f:481-487's
    // reader is fed the local here -- so the two are equivalent, unlike
    // prlkhd's third arm, where the same shape was a real gap.
    int fixper = ctx.rev.fixper;
    if (fixper > 0 && rev.revfix) {
        fixper = 0;
        writln(ctx,
               "NOTE: regARIMA model parameters will not be re-estimated once "
               "a year",
               stdio::STDERR, ctx.units.mt2, true);
        writln(ctx, "      during the history analysis.", stdio::STDERR,
               ctx.units.mt2, false);
    }
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
    // revdrv.f:116-126 -- `otlfix` outlives the fixreg block: chkorv re-adds a
    // held-back outlier with its coefficient FIXED when fixreg=(outlier).
    bool otlfix = false;
    if (rev.nrvfxr > 0 &&
        ((ctx.model.nb > 0 && ctx.model.iregfx < 3) || ctx.xrgmdl.nbx > 0)) {
        bool tdfix = false, holfix = false, usrfix = false;
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

    // revdrv.f:296-306 -- HOLD BACK the outliers dated after the first revision
    // date. Not behind any flag: a span ending at date T must not know about an
    // outlier dated after T, so every outlier-type regressor past Beglup comes
    // out of the design now and chkorv puts it back when a span's model span
    // reaches it. `outlier=` only decides whether they are SAVED for that
    // re-introduction (keep/auto) or simply dropped (remove).
    //   Placed after the Revfxx block rather than before it as in revdrv; the
    //   two touch disjoint state (the regARIMA design vs the x11reg store).
    RevOtlStore otr;
    if (has_model) {
        // revdrv.f:275-287 -- `outlier=` (OTLDIC = 'keepremoveauto', so the
        // DEFAULT is keep=0; remove=1, auto=2). Below 2 the oracle switches the
        // per-span automatic identification OFF, which is what this port does
        // unconditionally (run_x11_span never re-identifies), so keep/remove
        // need no Ltstao/Ltstls/Ltsttc handling here. At 2 it switches it ON --
        // and that, plus rmatot's save-and-re-enter arm and the per-span rmatot
        // at revdrv.f:723, is a whole unported sub-engine. Fatal rather than
        // silent: measured, `auto` moves sar 1.2e+0 against a 5e-3 tolerance.
        if (rev.otlrev >= 2) {
            errhdr(ctx);
            writln(ctx,
                   "ERROR: history{outlier=auto} (per-span automatic outlier "
                   "identification) is not yet ported.",
                   stdio::STDERR, ctx.units.mt2, true);
            abend(ctx);
            return false;
        }
        // revdrv.f:290-295 -- outlier=remove: drop the outliers the MAIN run's
        // outlier{} identified, so no span inherits a find it could not have
        // made itself.
        if (rev.otlrev == 1) {
            rmatot(ctx, rev.otlrev, ctx.arima.nrxy);
            if (ctx.error.lfatal) return false;
        }
        const bool lotlrv = (rev.otlrev == 0 || rev.otlrev == 2);
        rmotrv(ctx, begspn_full, beglup, ctx.arima.nrxy, otr, lotlrv);
        if (ctx.error.lfatal) return false;
        // revdrv.f:305 -- and note what it is keyed on: the STORE being
        // non-empty, not on anything having been deleted. With `outlier=remove`
        // and nothing dated after the start, rmatot's deletions are NOT
        // snapshotted (rmatot.f's own update sits inside its Otlrev>=2 arm) and
        // the first span's restor puts them straight back. Transcribed.
        if (!otr.empty()) rev_snapshot_design(ctx);
    }
    // revdrv.f:332-350 -- the same two routines again, on the X11REGRESSION
    // design. loadxr(false) swaps that store into the working model arrays, the
    // manipulation happens there, loadxr(true) saves it back, and restor puts
    // the regARIMA model back over the top. `x11outlier=` (Rvxotl, DEFAULT yes)
    // picks which of the two runs: yes deletes the automatically identified
    // x11reg outliers outright (each span re-identifies its own), no holds them
    // back by date the way the regARIMA side does.
    //
    // Reaching this at all needs the x11reg design to CARRY automatically
    // identified outliers, i.e. `x11regression{critical=}` -> Otlxrg -- and that
    // argument used to be accepted by the parser and dropped, so these blocks
    // were inert (ctx.xrgmdl held six trading-day columns and no outliers, and
    // the whole family measured the resulting MAIN-run gap instead). With
    // critical= honoured, the DEFAULT gates at the per-span floor (sar
    // 5.2e-1 -> 4.7e-4) and the MODEL-FREE case gates BIT-EXACT (5.3e-1 ->
    // 5.3e-15) -- nothing re-estimates there, so the only thing moving is the
    // per-span identification, which is exactly what this pins.
    //
    // `x11outlier=no` is CLOSED and gated bit-exact (entry 90). It had been
    // open at sar 7.5e-1, re-measured at sar 4.6e+0 / trr 1.3e+1, and the whole
    // divergence was `x11mdl.f:424`'s guard -- `Otlxrg .and. (Irev.lt.4.or.
    // (Irev.eq.4.and.Rvxotl)) .and. (Issap.lt.2.or.(Issap.eq.2.and.Ssxotl))`,
    // of which this port had ported `Otlxrg` alone, so every span re-identified
    // its own AO set on top of the ones rmotrv had held back. The clause was
    // restored in entry 87 and measured to change NOTHING, because
    // `ctx.hiddn.irev` was still 1 for the whole span loop; entry 89 advanced it
    // to 4, and the guard has been live -- and this arm bit-exact -- ever since.
    // Nothing here changed to close it. Gated by extra/airline_history-
    // x11outlier-no and -no-nomodel; the two arms are 708/714 oracle lines
    // apart, and 8 gates each fall to a mutation of either.
    RevOtlStore otx;
    // revdrv.f:246 -- `mdl2x`, the MAIN run's Endxrg, the x11reg counterpart of
    // mdl2. Each span's Endxrg is its own end unless the main run's x11reg span
    // ends inside it (revdrv.f:499-513).
    const int mdl2x[2] = {ctx.x11reg.endxrg(1), ctx.x11reg.endxrg(2)};
    if (ctx.hiddn.ixreg > 0) {
        loadxr(ctx, false);
        // This rmatot is REDUNDANT with the per-span one at :731-741 and
        // measures zero: that block's `else if (Rvxotl)` arm has no `i>Begrev`
        // guard, so span 1 deletes the same automatic columns again on a design
        // this call has already stripped. Mutating it out fails no gate, and
        // that is structural rather than a corpus gap -- transcribed because the
        // oracle makes the call, not because anything depends on it.
        if (rev.rvxotl) {
            rmatot(ctx, 1, ctx.arima.nrxy);
            if (ctx.error.lfatal) return false;
        }
        rmotrv(ctx, begspn_full, beglup, ctx.arima.nrxy, otx, !rev.rvxotl);
        if (ctx.error.lfatal) return false;
        loadxr(ctx, true);
        if (has_model) restor_span(ctx);
    }
    // revdrv.f:381 -- CALL ssprep(Lmodel,F,F), UNCONDITIONALLY, right before the
    // span loop. This is the one that actually makes the two blocks above stick:
    // :306's conditional ssprep only fires when rmotrv SAVED something, so with
    // `outlier=remove` (which deletes without saving) nothing would be
    // snapshotted and the first span's restor would put every deleted outlier
    // straight back -- measured, that costs sar 1.19e+0. Note the order it
    // implies: with x11regression{} present, :351's restor runs FIRST and
    // reinstates the regARIMA design from whatever the snapshot then held, so
    // a `remove` that :306 did not snapshot really is undone on that path. Both
    // are transcribed as written.
    if (has_model) rev_snapshot_design(ctx);
    // revdrv.f:181/484-488 -- `addreg`. True whenever a model is estimated,
    // EXCEPT under Fixper, where a span whose model span was pulled back to the
    // fixed period must not gain a regressor the estimation would not see.
    bool addreg = has_model;

    const int nspan = endrev - begrev + 1;         // includes the final full span
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

    // revdrv.f:387 -- the HIDDEN-RUN phase indicator for the span loop, the
    // history counterpart of sliding spans' Issap=2. It gates ~40 conditions
    // across the tree (the `Issap.lt.2.and.Irev.lt.4` gudrun family, getrev's
    // five x11pt3 sites, x11mdl.f:424's outlier-ID arm, x11reg.f's coefficient
    // seed, errhdr's one-header-per-mode rule); this port left it at 1 for
    // years, so every one of those took its MAIN-RUN branch during a replay.
    ctx.hiddn.irev = 4;
    ctx.hiddn.ierhdr = prm::NOTSET;                // revdrv.f:396

    for (int i = beglup; i <= endrev; ++i) {
        const int revptr = i - begrev + 1;         // <=0 for the pre-Begrev spans
        ctx.rev.revptr = revptr;                   // revdrv.f:524
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
        // revdrv.f:478 -- this span's end as text, for errhdr's banner.
        {
            int endspn_h[2];
            addate(begspn_full, ny, nlen - 1, endspn_h);
            const std::string ds = wrtdat(endspn_h, ctx.model.sp);
            ctx.hiddn.crvend = ds;
            ctx.hiddn.nrvend = static_cast<int>(ds.size());
        }
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
                // revdrv.f:484-488 -- addreg is ON only in the spans whose own
                // end IS the fixed period, i.e. the ones that actually
                // re-estimate; in between, the model span is pulled back and a
                // newly-defined outlier must wait.
                addreg = (endmdl_i[1] == fixper);
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
        // revdrv.f:587-593 -- put back every held-back outlier this span's MODEL
        // span now covers (`i - nend`, not `i`: with a modelspan/Fixper end the
        // estimation stops short of the span end, and an outlier past that
        // point is still undefined for the fit). chkorv re-snapshots the design
        // so the NEXT span's restor keeps it.
        if (addreg && i > begrev && !otr.empty()) {
            chkorv(ctx, begspn_full, i - nend_mdl, otr, otlfix, ctx.arima.nrxy,
                   /*lmdl=*/true);
            if (ctx.error.lfatal) return false;
        }
        // revdrv.f:731-741 -- the x11regression half, per span. With
        // `x11outlier=yes` (the DEFAULT) every span starts by deleting the
        // automatically identified x11reg outliers, so it re-identifies its own
        // on its own data; with `no` they are held back by date and chkorv adds
        // each one back as the spans reach it. Note `lmdl=false`: chkorv does
        // NOT re-snapshot on this branch -- the change is saved into the x11reg
        // store by loadxr(true) instead, and nothing restores that store.
        if (ctx.hiddn.ixreg > 0) {
            // revdrv.f:499-513 -- Endxrg: this span's own end, unless Fxprxr is
            // set or the main run's x11reg span ends inside it, in which case
            // that end. nend_xrg is how far short of the span end it stops.
            int nend_xrg = 0;
            if (ctx.x11reg.fxprxr <= 0) {
                int endspn_x[2];
                addate(begspn_full, ny, nlen - 1, endspn_x);
                int nendx = 0;
                dfdate(endspn_x, mdl2x, ny, nendx);
                if (nendx > 0) nend_xrg = nendx;
            }
            loadxr(ctx, false);
            if (i > begrev && !otx.empty() && !rev.rvxotl) {
                // The x11reg model span end (Endxrg), not the regARIMA one.
                chkorv(ctx, begspn_full, i - nend_xrg, otx, otlfix,
                       ctx.arima.nrxy, /*lmdl=*/false);
            } else if (rev.rvxotl) {
                rmatot(ctx, 1, ctx.arima.nrxy);
            }
            if (ctx.error.lfatal) return false;
            loadxr(ctx, true);
            if (has_model) restor_span(ctx);
        }
        // revdrv.f:416-427 -- past Endsa (and not the final span) no adjustment
        // is produced at all; the span is estimated for the model diagnostics
        // only, and therefore captures nothing.
        const bool lx11_span = (i <= endsa) || (i == endrev);
        if (!run_x11_span(ctx, trnsrs_full, has_model, nlen, nfcst_full,
                          /*nbcst=*/0, /*nbcst2=*/0, lsp, nend_mdl, lseats,
                          /*set_xrg_span=*/false, /*ss_outliers=*/false,
                          /*ss_otlfix=*/false, /*ssusr=*/nullptr, lx11_span))
            return false;
        if (ctx.error.lfatal) return false;
        const int posfob = ctx.x11ptr.posfob;      // = nlen = i
        // The SA / trend / seasonal captures (Cncsa, Cnctrn, Cncch, Cnctch,
        // Cncsf, Cncsfp and every alternate-target row) are made by getrev
        // INSIDE x11pt3, on the buffer the oracle hands it. They used to be
        // re-derived from ctx.x11srs here, which is the same value only while
        // nothing after the getrev site rewrites the buffer -- see entry 89.
        if (revptr > 0) {                          // getrev's IF(Revptr.gt.0)
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
        (void)posfob;
    }
    // revdrv.f:761-762 -- leave the span loop, and close off the error file's
    // hidden-run section if any span opened one.
    ctx.hiddn.irev = 5;
    if (ctx.hiddn.ierhdr != prm::NOTSET) errhdr(ctx);

    // Everything the tables below consume now sits in /revdta/, filed by getrev
    // from inside the span. Lift it into the local shapes the printing code has
    // always used; the FINAL column is getrev.f:118-146's, written by the last
    // span (the one with all the data) out of the same buffer it just captured
    // its concurrent value from.
    std::vector<double> cncsa(nspan + 1, 0.0), cnctrn(nspan + 1, 0.0);
    std::vector<double> cncch(nspan + 1, 0.0), cnctch(nspan + 1, 0.0);
    std::vector<double> cncsf(nspan + 1, 0.0);
    std::vector<double> projsf(revnum + 1, 0.0);
    std::vector<double> finsa(revnum + 1, 0.0), fintrn(revnum + 1, 0.0);
    std::vector<double> finch(revnum + 1, 0.0), finsf(revnum + 1, 0.0);
    std::vector<double> fintch(revnum + 1, 0.0);
    for (int r = 1; r <= nspan; ++r) {
        cncsa[r] = ctx.revsrs.cncsa(r);
        cnctrn[r] = ctx.revsrs.cnctrn(r);
        cncch[r] = ctx.revsrs.cncch(r);
        cnctch[r] = ctx.revsrs.cnctch(r);
        cncsf[r] = ctx.revsrs.cncsf(r);
    }
    for (int r = 1; r <= revnum; ++r) {
        projsf[r] = ctx.revsrs.cncsfp(r);
        finsa[r] = ctx.revsrs.finsa(0, r);
        fintrn[r] = ctx.revsrs.fintrn(0, r);
        finch[r] = ctx.revsrs.finch(0, r);
        fintch[r] = ctx.revsrs.fintch(0, r);
        finsf[r] = ctx.revsrs.finsf(r);
    }
    for (int t = 1; t <= ntarsa; ++t)
        for (int r = 1; r <= revnum; ++r) {
            finsa_t[t][r] = ctx.revsrs.finsa(t, r);
            finch_t[t][r] = ctx.revsrs.finch(t, r);
        }
    for (int t = 1; t <= ntartr; ++t)
        for (int r = 1; r <= revnum; ++r) {
            fintrn_t[t][r] = ctx.revsrs.fintrn(t, r);
            fintch_t[t][r] = ctx.revsrs.fintch(t, r);
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
    out.ntarsa = ntarsa;
    out.ntartr = ntartr;
    out.ncol_sa = ncol_sa;
    out.ncol_tr = ncol_tr;
    out.r1y2y = r1y2y;
    out.cnctar = cnctar;
    for (int t = 1; t <= ntarsa; ++t) out.targsa.push_back(rt.targsa(t));
    for (int t = 1; t <= ntartr; ++t) out.targtr.push_back(rt.targtr(t));

    // prtrev.f:174-226 -- one table's alternate-target columns for one row.
    // `rev_out` gets the ncol REVISION columns (the save file's own order: lag
    // ascending, the shared 1yr-2yr column last), `lvl_out` the ntarg "N later"
    // LEVELS the conc/final save table appends unmasked.
    //   default (target=final)      column t = Fin(0) - Fin(t)   [/Fin(t)]
    //   target=concurrent (Cnctar)  column t = Fin(t) - Conc     [/Conc]
    // A row past `lstrev` has no such estimate yet and is written DNOTST.
    auto targ_cols = [&](int revptr, int ntarg, int ncol,
                         const farray1<int, 5>& targ,
                         const std::vector<std::vector<double>>& fin_t,
                         double cnc, double fin0, bool percent, bool use_1y2y,
                         std::vector<double>& rev_out,
                         std::vector<double>& lvl_out) {
        const int i = begrev + revptr - 1;         // prtrev's absolute row index
        double fin1yr = 0.0;
        // NB when Lr1y2y came from the OTHER family (CB-22) this row's last
        // column is never assigned -- in the Fortran it is then whatever the
        // uninitialised `rev` scratch held. Left at 0 here.
        std::vector<double> col(static_cast<std::size_t>(ncol) + 1, 0.0);
        for (int i2 = 1; i2 <= ntarg; ++i2) {
            const int v = targ(i2);
            const double fint = fin_t[i2][revptr];
            int lstrev = endsa - v;
            if (cnctar) lstrev += 1;
            double r;
            if (i >= lstrev) {
                r = prm::DNOTST;
            } else if (cnctar) {
                r = fint - cnc;
                if (percent) r = (r / cnc) * 100.0;
            } else {
                r = fin0 - fint;
                if (percent) r = (r / fint) * 100.0;
            }
            col[static_cast<std::size_t>(i2)] = r;
            // prtrev.f:203-226 -- the extra (1yr-2yr) column, formed on the
            // 2-year lag's pass from the 1-year lag's value saved on the
            // previous one (the list is sorted, so 1yr comes first). Its mask
            // is one row LOOSER than the others' and ignores Cnctar.
            if (use_1y2y) {
                if (v == ny) fin1yr = fint;
                if (v == 2 * ny) {
                    if (i >= endsa - v) {
                        col[static_cast<std::size_t>(ncol)] = prm::DNOTST;
                    } else {
                        double q = fint - fin1yr;
                        if (percent) q = (q / fin1yr) * 100.0;
                        col[static_cast<std::size_t>(ncol)] = q;
                    }
                }
            }
        }
        for (int k = 1; k <= ncol; ++k)
            rev_out.push_back(col[static_cast<std::size_t>(k)]);
        for (int k = 1; k <= ntarg; ++k) lvl_out.push_back(fin_t[k][revptr]);
    };

    // The indirect target levels live in the shared /revdta/ accumulator (every
    // component folded into it above); lift them into the same shape.
    std::vector<std::vector<double>> finisa_t;
    if (out.have_ind && ntarsa > 0) {
        finisa_t.assign(ntarsa + 1, std::vector<double>(nt_rows, 0.0));
        for (int t = 1; t <= ntarsa; ++t)
            for (int r = 1; r <= revnum; ++r)
                finisa_t[t][r] = ctx.revsrs.finisa(t, r);
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
            if (ntarsa > 0)
                targ_cols(revptr, ntarsa, ncol_sa, rt.targsa, finsa_t, cnc, fin,
                          rvper, r1y2y, out.sar_t, out.sae_t);
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
            if (ntarsa > 0)
                targ_cols(revptr, ntarsa, ncol_sa, rt.targsa, finisa_t, cnc, fin,
                          rvper, r1y2y, out.iar_t, out.iae_t);
        }
        if (lrvtrn) {
            const double cnc = cnctrn[revptr], fin = fintrn[revptr];
            double r = fin - cnc;
            if (rvper) r = (r / cnc) * 100.0;
            out.tre_cnc.push_back(cnc);
            out.tre_fin.push_back(fin);
            out.trr.push_back(r);
            if (ntartr > 0)
                targ_cols(revptr, ntartr, ncol_tr, rt.targtr, fintrn_t, cnc, fin,
                          rvper, false, out.trr_t, out.tre_t);
        }
        if (lrvch) {
            // Change table (Tbltyp=2): the conc/final values are already the
            // month-to-month % change; the revision is a plain difference
            // (prtrev forces Rvper=F for Tbltyp 2), no second percenting.
            const double cnc = cncch[revptr], fin = finch[revptr];
            out.che_cnc.push_back(cnc);
            out.che_fin.push_back(fin);
            out.chr.push_back(fin - cnc);
            if (ntarsa > 0)                        // Tbltyp=2 also takes Targsa
                targ_cols(revptr, ntarsa, ncol_sa, rt.targsa, finch_t, cnc, fin,
                          /*percent=*/false, r1y2y, out.chr_t, out.che_t);
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
            if (ntartr > 0)                        // Tbltyp=5 also takes Targtr
                targ_cols(revptr, ntartr, ncol_tr, rt.targtr, fintch_t, cnc, fin,
                          /*percent=*/false, false, out.tcr_t, out.tce_t);
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
