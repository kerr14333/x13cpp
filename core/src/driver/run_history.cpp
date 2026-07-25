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
                 const int* begspn_full, int nspobs_full, int nfcst_full) {
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
    const bool lrvtdrg = rev.lrvtdrg && ctx.captured.has_model;
    if (!(lrvsa || lrvtrn || lrvch || lrvsf || lrvtch || lrvfct ||
          lrvaic || lrvarma || lrvtdrg))
        return true;                               // nothing this driver emits

    const int ny = ctx.model.sp;
    const bool has_model = ctx.captured.has_model;

    // --- revchk.f / setrvp.f: loop bounds -------------------------------------
    int rvstrt[2] = {rev.rvstrt(1), rev.rvstrt(2)};
    int rvend[2] = {rev.rvend(1), rev.rvend(2)};
    if (rvend[1] == 0 && rvend[0] == 0)            // default = end of series
        addate(begspn_full, ny, nspobs_full - 1, rvend);

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
    // that projects the first table year's seasonal factors runs. (No Fixper
    // handling -- fixed-period model estimation is out of scope.)
    int beglup = begrev;
    if (lrvsf) {
        int lupbeg[2] = {rvstrt[0] - 1, ny};
        dfdate(lupbeg, begspn_full, ny, beglup);
        beglup += lfda;
        if (beglup < 1) beglup = 1;                // clamp (Frstsa floor)
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

    for (int i = beglup; i <= endrev; ++i) {
        const int revptr = i - begrev + 1;         // <=0 for the pre-Begrev spans
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
        const int nlen = i;                        // Length = Posfob - Pos1ob + 1
        const int lsp = 1;                         // Pos1ob = Nbcst2(0) + Lsp = 1
        if (!run_x11_span(ctx, trnsrs_full, has_model, nlen, nfcst_full,
                          /*nbcst=*/0, /*nbcst2=*/0, lsp))
            return false;
        if (ctx.error.lfatal) return false;
        const int posfob = ctx.x11ptr.posfob;      // = nlen = i
        if (revptr > 0) {                          // getrev's IF(Revptr.gt.0)
            cncsa[revptr] = ctx.x11srs.stci(posfob);
            cnctrn[revptr] = ctx.x11srs.stc(posfob);
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
    for (int revptr = 1; revptr <= revnum; ++revptr) {
        const int pos = begrev + revptr - 1;
        finsa[revptr] = ctx.x11srs.stci(pos);
        fintrn[revptr] = ctx.x11srs.stc(pos);
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
