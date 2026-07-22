// slidingspans.hpp -- the slidingspans{} driver: setssp.f (span bookkeeping) +
// sspdrv.f (replay loop, built on driver/run_x11_span.hpp) + ssrit.f (per-span
// capture) + ssap.f's xchng/mflag/rplus (cross-span max-%-diff) for the sfs
// (seasonal-factor spans) and chs (month-to-month SA-change spans) tables --
// the two tags with committed corpus goldens (see tools/slidingspans_scope.md).
//
// Scope (see tools/slidingspans_scope.md S5 for the phased plan this
// implements): single-series only (no Iagr==5/6 composite/indirect), no
// SEATS, no x11regression (Nbx==0), and Ssinit==1 (fixmdl=yes, the default --
// ssmdl.f's tail then FIXES the whole regARIMA model at the main run's
// converged values for every span, which is what run_x11_span's fixed-
// coefficient rgarma replay assumes). ads/tds/ycs (needs TD/holiday/round/
// force or an explicit save=(ycs) with >=5 years of spans) are not produced;
// see setssp_span/run_slidingspans doc comments for exactly what is and is
// not ported.
#ifndef X13_X11_SLIDINGSPANS_HPP
#define X13_X11_SLIDINGSPANS_HPP

#include <string>
#include <vector>

namespace x13 {

struct X13Context;

// sfmax.f: the longest seasonal filter length actually used across the 12/4
// per-period Lter(i) choices (Lterm==5/6/7 sentinels collapse to 0-ish first,
// then the per-period max/stable-check). Feeds setssp_span's Ltmax.
int sfmax_span(int lterm, const int* lter, int ny);

// ssprep.f, scoped to Lmodel/Lx11=true, Lx11rg=false (no x11regression in this
// port's slidingspans scope): snapshot the model's converged
// Arimap/Var/Nintvl/.../Lma/Lar and the x11 Lter(1..Ny)/Ktcopt/Tic seasonal-
// filter settings into ctx.ssprep, BEFORE the main run's own x11int/x11ari
// call (Lter is still the unresolved auto-select sentinel at that point).
// Called once from run_x11.cpp, unconditionally (cheap; harmless when
// slidingspans{} was not requested) -- calling it any later would snapshot
// the MAIN run's own resolved/mutated Lter, which is NOT what restor_span
// needs to reproduce per-span.
void ssprep_snapshot(X13Context& ctx);

// restor.f, same scope as ssprep_snapshot: reset Lter(1..Ny)/Ktcopt/Tic and
// (Lmodel) Arimap/Var/Nintvl/.../Lma/Lar from the ctx.ssprep snapshot. Called
// at the top of each span's replay (mirrors ssx11a.f's CALL restor(...) call
// position, before that span's own run_x11_span).
void restor_span(X13Context& ctx);

// ssmdl.f, scoped to Nb==0 (no regression{}, no outlier{} -- the whole
// regressor-fixing/change-of-regime/outlier-in-span block is then dead code):
// when Ssinit==1 (fixmdl=yes, the default), fix every ARIMA parameter
// (ctx.model.arimaf all true) so run_x11_span's rgarma replay recomputes
// residuals/likelihood without re-optimizing.
void ssmdl_fix_model(X13Context& ctx);

// setssp.f, scoped per the file header. Resolves Ncol/Nlen defaults from the
// main run's Length (ctx.x11opt.length) + Ltmax when the user didn't set
// numspans=/length=, computes Sslen/Iyr/Im/Ic/Icyr/Icm/Itot (ctx.ssap) and
// Nbcst2/L0 (ctx.extend.nbcst2/ctx.lzero.l0), and (Lmodel) calls
// ssmdl_fix_model. Returns false (ctx.hiddn.issap left at 0) when there is
// not enough data for >=2 spans -- a clean skip (matches the oracle's
// NOTE-and-RETURN), not a FATAL; the "NOTE:" diagnostic text itself is not
// ported (deferred print, as elsewhere in this codebase).
bool setssp_span(X13Context& ctx, int ltmax, bool lmodel, bool lseats,
                  bool lncset, bool lnlset);

// ssrit.f, scoped to non-composite (Iagr!=2): stores this span's per-period
// values (X, over [l1,l2]) into the ctx.sspdat MXLEN x MXCOL accumulator
// (S/Td/Sa depending on isec: 1=Td, 2=S, 3=Sa), DNOTST-filling the rows
// outside [l1,l2] but inside this span's Nsea-aligned display window. series
// is Isec==3's Series buffer (Isfadd numerator; unused for isec 1/2). Called
// from the 3 x11parts.cpp sites gated on ctx.hiddn.issap==2 (mirrors
// x11pt2.f:136 / x11pt3.f:311 / x11pt3.f:678).
void ssrit(X13Context& ctx, const double* x, int l1, int l2, int isec,
           const double* series);

// sspdrv.f + ssap.f's xchng/mflag/rplus, scoped per the file header. Runs the
// setssp_span setup, then replays run_x11_span once per span (Ncol times),
// each landing its S/Sa capture in ctx.sspdat via the ssrit call sites; then
// computes the month-to-month SA change (xchng) and the per-row cross-span
// max-%-difference (mflag/rplus) for the sfs (S) and chs (c) tables, leaving
// the result on ctx.ssout for tools/x13run_x11.cpp to print. trnsrs_full is
// the main run's clean transformed series (see run_x11_span.hpp); called from
// the tail of run_x11() (has it in scope as a local), after the main run's
// own x11pt3 has already produced its tables. No-op (returns true, leaves
// ctx.ssout.ran false) when slidingspans{} was not requested
// (ctx.hiddn.issap != 1) or setssp_span finds insufficient data.
bool run_slidingspans(X13Context& ctx, const std::vector<double>& trnsrs_full);

// Non-oracle-mirrored result struct (the MXLEN x MXCOL S/Sa arrays already
// live on ctx.sspdat; this just adds what sfs/chs's Max_%_DIFF column and the
// chs table body (the derived c = month-to-month change array) need).
struct SlidingSpansOutput {
    bool ran = false;
    int ncol = 0;
    int sslen = 0;
    int im = 0;
    int iyr = 0;
    int nsea = 0;
    // Row r (1-based, r=1..sslen+im-1, matching svspan.f's DO l0=Im,Sslen+Im-1)
    // corresponds to calendar date (iyr,im) + (r-im) months/quarters.
    std::vector<double> c_flat;       // MXLEN*MXCOL, column-major (row-1)+(col-1)*MXLEN
    std::vector<double> dmax_sfs;     // MXLEN, 1-based via [row-1]
    std::vector<double> dmax_chs;     // MXLEN
};

}  // namespace x13

#endif  // X13_X11_SLIDINGSPANS_HPP
