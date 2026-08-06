// slidingspans.hpp -- the slidingspans{} driver: setssp.f (span bookkeeping) +
// sspdrv.f (replay loop, built on driver/run_x11_span.hpp) + ssrit.f (per-span
// capture) + ssap.f's xchng/mflag/rplus (cross-span max-%-diff) for the sfs
// (seasonal-factor spans) and chs (month-to-month SA-change spans) tables --
// the two tags with committed corpus goldens (see tools/slidingspans_scope.md).
//
// Scope: single-series only (no Iagr==5/6 composite/indirect). SEATS,
// x11regression, all three `fixmdl=` arms and the outlier hold-back are all
// live and gated; what is NOT ported is inventoried in docs/WALLS.md and
// refuses rather than adjusts. ads (the SA-series spans) is produced only
// under ssap.f:209-210's gating -- a trading-day, holiday, round or force
// option must be live -- so most specs legitimately emit no ads table at all,
// and ycs is still not produced. Do not restate the open list here: it goes
// stale, and `walls.py` derives the real one from the refusals themselves.
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
// `capture_saved` covers the three fields that are NOT ssprep.cmn members --
// ctx.saved.ksdev0/lterm0/nterm0, which this port stashes here because it has
// no editor block to read them from. They are parse-time values and must be
// taken ONCE; a per-span re-snapshot would capture that span's evolved Ksdev
// and re-open the extreme-value-mode gap it was added to close. Pass false
// from inside a span replay (arima.f:1430's ssprep), true from the main run.
// `lx11` is ssprep.f's own Lx11 argument, gating the Lter/Ktcopt/Tic third of
// the snapshot. Only sspdrv.f:218's call passes false, and it matters there:
// that one runs after x11pt2 has resolved the auto-select filter lengths.
void ssprep_snapshot(X13Context& ctx, bool capture_saved = true,
                     bool lx11 = true);

// restor.f, same scope as ssprep_snapshot: reset Lter(1..Ny)/Ktcopt/Tic and
// (Lmodel) Arimap/Var/Nintvl/.../Lma/Lar from the ctx.ssprep snapshot. Called
// at the top of each span's replay (mirrors ssx11a.f's CALL restor(...) call
// position, before that span's own run_x11_span).
void restor_span(X13Context& ctx);

// ssmdl.f, scoped to exclude the change-of-regime block (walled):
//
//  * ssmdl.f:50-121 -- the regressor-fixing block. `tdfix`/`holfix` are IN/OUT:
//    they arrive carrying slidingspans{fixreg=} and leave carrying this
//    routine's own verdict on an already-fixed design, which setssp then hands
//    to ssxmdl. Demotes Itd/Ihol to -1 ("requested but not analysed") when the
//    component cannot be re-estimated per span.
//  * ssmdl.f:341-352 -- when Ssinit==1 (fixmdl=yes, the default), fix every
//    ARIMA parameter (ctx.model.arimaf all true) so run_x11_span's rgarma
//    replay recomputes residuals/likelihood without re-optimizing.
//  * ssmdl.f:124-280 -- the group walk: hold back (rmotss) every outlier
//    regressor the span intersection does not cover, and REFUSE on a
//    change-of-regime regressor (see the wall's own comment).
//  * ssmdl.f:358-373 -- re-snapshot the design when the walk changed it.
//
// Returns false on the change-of-regime wall; true otherwise.
bool ssmdl_fix_model(X13Context& ctx, bool& tdfix, bool& holfix, bool otlfix,
                     bool usrfix);

// ssx11a.f:220-270 -- the per-span half of the outlier hold-back, called from
// run_x11_span once this span's Begspn/Endspn are set and BEFORE its regvar:
//
//   1. delete every outlier column the span's own window does not cover
//      (:229-263), and
//   2. adotss (:268) -- re-add every entry of the ctx.otlrev store that it
//      DOES cover, at the main run's coefficient.
//
// The store is not consumed: each span re-tests the whole of it, and
// run_slidingspans strips the re-added columns afterwards (sspdrv.f:208-219).
// `lastsy` is the span end as a 1-based index into the full series (ssx11a.f:
// 86-87); `otlfix` is `Otlfix.or.Ssinit.eq.1`.
void ssx11a_span_outliers(X13Context& ctx, int lastsy, bool otlfix);

// ssx11a.f:99-154 -- the same job for the X11REGRESSION design, and it runs
// EARLIER in the span (before restor, before the Orig copy) and on the other
// model store. Called from run_x11_span's set_xrg_span arm once Begxrg/Endxrg
// are set. With `slidingspans{x11outlier=yes}` (the default) it strikes the
// previous span's automatically identified AO columns so this span's x11mdl
// re-identifies from a clean design; the `x11outlier=no` arm needs the
// ssxmdl store; `otlfix` is setssp's RAW fixreg=(outlier) flag, which this
// routine combines with `Ssxint` (ssx11a.f:150) exactly as run_x11_span
// combines it with `Ssinit==1` for the regARIMA store (ssx11a.f:269).
void ssx11a_span_xrg_outliers(X13Context& ctx, bool otlfix);

// sspdrv.f:208-219 -- after a span, take the columns adotss added back out and
// re-snapshot, so the next span's restor starts from the held-back design
// again. No-op when the store is empty.
void ssp_strip_span_outliers(X13Context& ctx);

// setssp.f, scoped per the file header. Resolves Ncol/Nlen defaults from the
// main run's Length (ctx.x11opt.length) + Ltmax when the user didn't set
// numspans=/length=, computes Sslen/Iyr/Im/Ic/Icyr/Icm/Itot (ctx.ssap) and
// Nbcst2/L0 (ctx.extend.nbcst2/ctx.lzero.l0), and (Lmodel) calls
// ssmdl_fix_model. Returns false (ctx.hiddn.issap left at 0) when there is
// not enough data for >=2 spans -- a clean skip (matches the oracle's
// NOTE-and-RETURN), not a FATAL; the "NOTE:" diagnostic text itself is not
// ported (deferred print, as elsewhere in this codebase).
// `otlfix` is setssp.f's OUT argument (setssp.f:323-334, `Ssfxrg(i).eq.4`):
// sspdrv.f:66-67 declares it as a local, setssp writes it, and sspdrv then
// passes it on to ssx11a ONCE PER SPAN (:121). It therefore outlives this call
// and must be threaded out -- collapsing it into the fixmdl arm is only correct
// while `slidingspans{fixreg=(outlier)}` is refused.
bool setssp_span(X13Context& ctx, int ltmax, bool lmodel, bool lseats,
                  bool lncset, bool lnlset, bool& otlfix);

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
    // ads (the SA-series spans). Unlike sfs/chs this one is CONDITIONAL --
    // ssap.f:209-210 only flags Sa when a trading-day, holiday, round or force
    // option is on -- so `have_ads` says whether the oracle produced the table
    // at all, and an absent one is not the same as an all-DNOTST one.
    bool have_ads = false;
    std::vector<double> dmax_ads;     // MXLEN
    // tds (the trading-day-factor spans), conditional the same way: ssap.f:208
    // flags Td only when Itd==1, i.e. a trading-day regressor survived into the
    // span analysis. The store behind it is filled by x11pt2.f:136 (regARIMA TD)
    // or x11mdl.f:874 (x11regression TD).
    bool have_tds = false;
    std::vector<double> dmax_tds;     // MXLEN
};

}  // namespace x13

#endif  // X13_X11_SLIDINGSPANS_HPP
