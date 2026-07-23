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

#include <vector>

namespace x13 {

bool run_history(X13Context& ctx, const std::vector<double>& trnsrs_full,
                 const int* begspn_full, int nspobs_full, int nfcst_full) {
    ctx.hist_out = HistoryOutput{};
    if (!ctx.captured.has_history) return true;   // history{} not requested
    const rev_cmn& rev = ctx.rev;
    const bool lrvsa = rev.lrvsa;
    const bool lrvtrn = rev.lrvtrn;
    const bool lrvch = rev.lrvch;                  // sadjchng (chr/che)
    if (!(lrvsa || lrvtrn || lrvch)) return true;  // nothing this driver emits

    const int ny = ctx.model.sp;
    const bool has_model = ctx.captured.has_model;

    // --- revchk.f / setrvp.f: loop bounds -------------------------------------
    int rvstrt[2] = {rev.rvstrt(1), rev.rvstrt(2)};
    int rvend[2] = {rev.rvend(1), rev.rvend(2)};
    if (rvend[1] == 0 && rvend[0] == 0)            // default = end of series
        addate(begspn_full, ny, nspobs_full - 1, rvend);

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

    const int nspan = endrev - begrev + 1;         // includes the final full span
    std::vector<double> cncsa(nspan + 1, 0.0), cnctrn(nspan + 1, 0.0);
    std::vector<double> cncch(nspan + 1, 0.0);     // concurrent SA % change (putrev)

    for (int i = begrev; i <= endrev; ++i) {
        const int revptr = i - begrev + 1;
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
        cncsa[revptr] = ctx.x11srs.stci(posfob);
        cnctrn[revptr] = ctx.x11srs.stc(posfob);
        if (lrvch) {                               // putrev Outch on this span's Stci
            const double a = ctx.x11srs.stci(posfob);
            const double b = ctx.x11srs.stci(posfob - 1);
            cncch[revptr] = ((a - b) / b) * 100.0;  // Muladd!=1 -> percent change
        }
    }

    // Final column: the loop's last iteration was i=Endrev (full data), so
    // ctx.x11srs.stci/stc now hold the final adjustment. Fin(0,revptr) is the
    // final estimate at the revision date Begrev+revptr-1 (getrev.f:118-124).
    std::vector<double> finsa(revnum + 1, 0.0), fintrn(revnum + 1, 0.0);
    std::vector<double> finch(revnum + 1, 0.0);
    for (int revptr = 1; revptr <= revnum; ++revptr) {
        const int pos = begrev + revptr - 1;
        finsa[revptr] = ctx.x11srs.stci(pos);
        fintrn[revptr] = ctx.x11srs.stc(pos);
        if (lrvch) {                               // final change from full-data Stci
            const double a = ctx.x11srs.stci(pos);
            const double b = ctx.x11srs.stci(pos - 1);
            finch[revptr] = ((a - b) / b) * 100.0;
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
    }
    return true;
}

}  // namespace x13
