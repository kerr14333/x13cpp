// run_history.hpp -- the history{} revisions-history driver: revchk.f (index
// setup) + setrvp.f (loop bounds) + revdrv.f's expanding-span re-run loop
// (DO i=Beglup,Endrev -> Posfob=i -> x11ari) + getrev.f/putrev.f (concurrent
// vs final capture) + prtrev.f's revision arithmetic, all built on the
// re-entrant sub-span replay driver (driver/run_x11_span.hpp), the same one
// slidingspans{} uses.
//
// Scope (airline_history gate -- see tools/x11_regeff_handoff.md / the
// x13cpp-diagnostics-ungated second-brain note):
//   * Single series (Iagr not composite), monthly or quarterly.
//   * Multiplicative/log mode (Muladd!=1); the additive percent-vs-difference
//     and negative-value ceasing branches (putrev.f/getrev.f) are out of scope.
//   * sadj + trend revisions (Lrvsa/Lrvtrn) -> the sar/sae/trr/tre save tables,
//     plus sadjchng (Lrvch: chr/che) -- the month-to-month SA %-change history:
//     putrev's Outch = (Stci(i)-Stci(i-1))/Stci(i-1)*100 on each span, revision
//     = Final - Conc (Tbltyp=2 forces Rvper=F, no second percenting).
//   * seasonal (Lrvsf: sfr/sfe) -- the seasonal-factor history. sfe emits the
//     concurrent Sts(Posfob), the projected factor (forecast-region Sts from the
//     prior year-boundary/December span, getrev Itype=0's Cncsfp), and the final
//     Sts; sfr the two revisions Final-Conc and Final-Proj (prtrv2, %-form when
//     Muladd!=1). setrvp.f's Beglup: with Lrvsf the loop starts a year earlier,
//     at the December before Rvstrt, so the first table year's projections run.
//   * trendchng (Lrvtch: tcr/tce) -- the month-to-month trend %-change history,
//     the chr/che pattern on Stc (Itype=2): conc/final = (Stc(i)-Stc(i-1))/
//     Stc(i-1)*100, revision = Final - Conc (Tbltyp=5 forces Rvper=F).
//   * fcst (Lrvfct: fce/fch + the meanssfe savelog canaries) -- the out-of-
//     sample forecast-error history. prtfct.f:613 stores each span's
//     original-scale forecast at the requested leads into Cncfct(k,Revptr+lag);
//     prfcrv.f then differences it against the raw series at that date and
//     accumulates the sum of squares. Needs forecast{maxlead=} (revchk.f:424
//     switches Lrvfct off with no forecasts). transformfcst=yes (Rvtrfc, errors
//     on the transformed scale) is ported; the eltfcn Facxhl/X11hol/Stptd folds
//     prtfct applies to untfct when the fct table is NOT printed are not -- the
//     port always has the full fcstout result, which is that same value.
//   * aic / arma / td (Lrvaic/Lrvarma/Lrvtdrg: lkh, amh, tdh) -- the three MODEL
//     histories, all captured at revdrv.f:670-690 right after each span's
//     rgarma: the span's (Olkhd, Aicc), its FREE ARMA coefficients in Mdl/Opr
//     order (rvarma.f), and its FREE trading-day / length-of-period / user-TD
//     regression coefficients plus each TD group's implied contrast column
//     -sum(b) (rvtdrg.f). Their row range is i=Begrev..Endrev -- one row longer
//     than the revision tables, which stop at Endtbl-1.
//     The additive-mode negative-value ceasing branch of putrev (Muladd==1) is
//     out of scope (this spec is multiplicative).
//   * The MODEL SPAN of each span (revdrv.f:479-497). Two branches, both ported:
//     `series{modelspan=(,0.per)}` sets Fixper, and each span's model then stops
//     at the last occurrence of that period -- so the estimation window advances
//     only ONCE A YEAR, which is how the oracle implements "parameters fixed to
//     what they were at the last value of Fixper"; setrvp.f:64-71 additionally
//     backs Beglup up to the first such period (a pre-Begrev, X-11-less span in
//     the oracle, which this port skips -- see the loop note in the .cpp). With
//     an ordinary modelspan END instead, every span's model span is capped at
//     `mdl2`, the main run's Endmdl. Both are applied through run_x11_span's
//     nend_mdl -> arima.f:142-152 narrow + arima.f:1145 (setspn.f) restore.
//   * history{fixmdl=yes} (Revfix, revdrv.f:250-262): every ARMA and regression
//     parameter held at the main run's converged values, so each span re-FILTERS
//     rather than re-estimates. This is the one history configuration that is
//     bit-exact rather than at the per-span re-estimation floor. revchk.f:801-805
//     then switches Fixper off (nothing left to re-estimate once a year).
//   * composite{}: the INDIRECT SA revision history (Indrev) -- see the
//     HistoryOutput fields below and the run_history.cpp comment block.
//   * No revision targets (Ntarsa==Ntartr==0 -> only the Fin(0,.) concurrent-
//     vs-final column), no regression{}/outlier{}/x11regression{}. Without
//     fixmdl the model is re-estimated each span (restor_span resets Arimap to
//     the main run's converged snapshot as the per-span starting values, then
//     rgarma re-optimizes).
//
// Still NOT ported from history{}'s option surface, all parsed and currently
// SILENT: outlier= / outlierwin= (Otlrev/Otlwin -- the per-span outlier
// re-identification), refresh= (Lrfrsh), fixreg= (Rvfxrg/Nrvfxr),
// fixx11reg= (Revfxx), sadjlags=/trendlags=/target= (Targsa/Targtr/Cnctar --
// the alternate revision targets), endtable= (Irev==2), transparent= (Rvtran,
// print surface) and additivesa= (Rvdiff, additive-mode only).
#ifndef X13_DRIVER_RUN_HISTORY_HPP
#define X13_DRIVER_RUN_HISTORY_HPP

#include <vector>

namespace x13 {

struct X13Context;

// history{} sar/sae/trr/tre result, one entry per revision-table row
// (i=Begrev..Endtbl-1). Not an oracle COMMON mirror; the underlying Cncsa/
// Finsa/Cnctrn/Fintrn live transiently during the loop, this keeps only what
// the save tables emit. Consumed by tools/x13run_x11.cpp's dump_history_table.
struct HistoryOutput {
    bool ran = false;
    bool have_sa = false;
    bool have_tr = false;
    bool have_ch = false;            // sadjchng requested (chr/che)
    bool have_sf = false;            // seasonal requested (sfr/sfe)
    bool have_tch = false;           // trendchng requested (tcr/tce)
    int nsea = 0;
    int revspn[2] = {0, 0};          // Rvstrt (first revision date)
    std::vector<int> dates;          // YYYYMM per row
    std::vector<double> sar;         // SA_revision (percent)
    std::vector<double> sae_cnc;     // Conc_SA
    std::vector<double> sae_fin;     // Final_SA
    std::vector<double> trr;         // TRND_revision (percent)
    std::vector<double> tre_cnc;     // Conc_TRND
    std::vector<double> tre_fin;     // Final_TRND
    // sadjchng: month-to-month SA percent change (putrev Outch), one per row.
    // che_cnc/che_fin are the concurrent/final change; chr = Final - Conc (a
    // plain difference -- Tbltyp=2 forces Rvper=F, no second percenting).
    std::vector<double> chr;         // SA-change revision
    std::vector<double> che_cnc;     // Conc_SA_change
    std::vector<double> che_fin;     // Final_SA_change
    // seasonal: concurrent + projected + final seasonal factor (all x100 when
    // Muladd!=1, matching putrev Itype=0). sfe emits the three levels; sfr the
    // two revisions Final-Conc and Final-Proj (prtrv2, %-form when Muladd!=1).
    std::vector<double> sfe_cnc;     // Conc_SF
    std::vector<double> sfe_proj;    // Proj_SF (from the prior-December span)
    std::vector<double> sfe_fin;     // Final_SF
    std::vector<double> sfr_cnc;     // SF revision vs concurrent
    std::vector<double> sfr_proj;    // SF revision vs projected
    // trendchng: month-to-month trend %-change (putrev Outch on Stc, Itype=2).
    // tcr = Final - Conc (Tbltyp=5 -> Rvper=F, plain difference), tce the two levels.
    std::vector<double> tcr;         // trend-change revision
    std::vector<double> tce_cnc;     // Conc_TRND_change
    std::vector<double> tce_fin;     // Final_TRND_change
    // fcst (Lrvfct): the out-of-sample forecast-error history (prfcrv.f). The
    // forecast-history rows are their OWN date range -- i = Begrev+Rfctlg(1) ..
    // Endrev, labelled from begfct = Rvstrt+Rfctlg(1) -- not the revision-table
    // range above, so they carry a separate `fdates`. Each row holds nfctlg
    // values per table, row-major: `fce` the EVOLVING sum of squared forecast
    // errors, `fch_fcst`/`fch_err` the concurrent forecast and its error. A lag
    // not yet defined on a row (prfcrv's ndef cut) is written as an exact 0,
    // which is what the oracle's save file puts there.
    bool have_fct = false;
    int nfctlg = 0;
    std::vector<int> fctlag;         // Rfctlg(1..Nfctlg), sorted
    std::vector<int> fdates;         // YYYYMM per forecast-history row
    std::vector<double> fce;         // nfctlg per row
    std::vector<double> fch_fcst;    // nfctlg per row
    std::vector<double> fch_err;     // nfctlg per row
    std::vector<double> meanssfe;    // nfctlg savelog canaries (final row)
    // The three MODEL histories (revdrv.f:670-690 capture, :873-1190 output).
    // They share one row range -- i = Begrev..Endrev, one row per span, labelled
    // from Rvstrt -- which is one row LONGER than the revision tables above
    // (those stop at Endtbl-1), so they carry their own `mdates`.
    std::vector<int> mdates;         // YYYYMM per model-history row
    bool have_aic = false;           // aic  -> lkh
    std::vector<double> lkh_lkhd;    // Olkhd (log likelihood)
    std::vector<double> lkh_aicc;    // Aicc
    bool have_arma = false;          // arma -> amh
    int nrvarma = 0;
    std::vector<double> amh;         // nrvarma per row, Mdl/Opr order
    bool have_tdrg = false;          // td   -> tdh
    int nrvtdrg = 0;
    std::vector<double> tdh;         // nrvtdrg per row
    // composite{}: the INDIRECT seasonally-adjusted revision history (Indrev,
    // revdrv.f:838-846 -> prtrev Tbltyp=3 -> the iar/iae save tables). Only the
    // aggregate TOTAL of a metafile emits it; every component instead folds its
    // own concurrent/final SA into the shared /revdta/ Cncisa/Finisa (putrev.f:
    // 25-30). Same row range and same percent arithmetic as sar/sae.
    bool have_ind = false;           // the total emitted iar/iae
    bool ind_reported = false;       // the total reached the historyindsa line
    bool ind_yes = false;            // .udg `historyindsa: yes|no`
    std::vector<double> iar;         // Ind_SA_revision (percent)
    std::vector<double> iae_cnc;     // Conc_Ind_SA
    std::vector<double> iae_fin;     // Final_Ind_SA
};

// Run the revisions-history analysis. No-op (returns true, leaves
// ctx.hist_out.ran false) when history{} was not requested
// (!ctx.captured.has_history). Called from the tail of run_x11() -- AFTER the
// main run's own x11pt3 tables and run_slidingspans (both of which, like this,
// clobber ctx.x11srs/x11ptr/mdldat with per-span content; the harness only
// dumps the main d-tables for specs whose goldens ship them, which the
// diagnostics specs do not). begspn_full/nspobs_full/nfcst_full are the main
// run's own geometry, captured by the caller before the span drivers overwrite
// the ctx copies. trnsrs_full is the main run's clean transformed series (see
// run_x11_span.hpp).
// endmdl_full is the MAIN run's Endmdl (revdrv.f:246's `mdl2`): every span's own
// model span is capped at it (revdrv.f:490-496), and when series{modelspan=} gave
// a "0.per" end it also set Fixper, which caps the model span at the last
// occurrence of that period instead (revdrv.f:481-489). Captured by the caller
// before the span drivers overwrite ctx.arima.endmdl with their own span end.
bool run_history(X13Context& ctx, const std::vector<double>& trnsrs_full,
                 const int* begspn_full, int nspobs_full, int nfcst_full,
                 const int* endmdl_full);

}  // namespace x13

#endif  // X13_DRIVER_RUN_HISTORY_HPP
