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
//   * sadj + trend revisions only (Lrvsa/Lrvtrn) -> the sar/sae/trr/tre save
//     tables. Projected seasonal factors (Lrvsf: sfr/sfe) are NOT ported -- no
//     golden ships them for this spec, and they only add the year-early
//     Beglup..Begrev-1 iterations (which never touch Cncsa/Cnctrn), so the loop
//     starts at Begrev. Changes-in-adjustment (Lrvch: chr) / changes-in-trend
//     (Lrvtch) / AICC (Lrvaic) / forecast (Lrvfct) / ARMA-coeff / TD-coeff
//     histories are likewise out of scope.
//   * No revision targets (Ntarsa==Ntartr==0 -> only the Fin(0,.) concurrent-
//     vs-final column), no regression{}/outlier{}/x11regression{}, model
//     re-estimated each span (Revfix=F -- restor_span resets Arimap to the main
//     run's converged snapshot as the per-span starting values, then rgarma
//     re-optimizes; the fixmdl=yes path is slidingspans' job).
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
    int nsea = 0;
    int revspn[2] = {0, 0};          // Rvstrt (first revision date)
    std::vector<int> dates;          // YYYYMM per row
    std::vector<double> sar;         // SA_revision (percent)
    std::vector<double> sae_cnc;     // Conc_SA
    std::vector<double> sae_fin;     // Final_SA
    std::vector<double> trr;         // TRND_revision (percent)
    std::vector<double> tre_cnc;     // Conc_TRND
    std::vector<double> tre_fin;     // Final_TRND
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
bool run_history(X13Context& ctx, const std::vector<double>& trnsrs_full,
                 const int* begspn_full, int nspobs_full, int nfcst_full);

}  // namespace x13

#endif  // X13_DRIVER_RUN_HISTORY_HPP
