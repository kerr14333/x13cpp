// seatopts.hpp -- resolve ctx.seatop's raw parsed seats{} option values
// (NOTSET/DNOTST sentinels when the user left them unspecified) against
// SEATS's real numeric defaults.
//
// PARITY NOTE: those defaults do NOT live where a naive port would look --
// not in gtinpt.f's NOTSET/DNOTST table (that governs *parsing*, not the
// values F1RST/SPECTRU actually consume), but in ansub9.f:1560-1650, the
// icode==0 ("set the namelist to defaults") branch of SETDEFAULT, called
// from NMLSTS. See tools/seats_scope.md's GTSEAT note (session 2) for how
// this was found. Only the six fields the canonical-decomposition path
// (seats_canonical_denoms / SPECTRU) actually reads are resolved here.
#ifndef X13_SEATS_SEATOPTS_HPP
#define X13_SEATS_SEATOPTS_HPP

#include "common/x13context.hpp"

namespace x13 {

struct SeatsOptions {
    double rmod = 0.5;      // ansub9.f:1587 l_rmod = .50d0
    double epsphi = 2.0;    // ansub9.f:1614 l_epsphi = 2.0d0
    double xl = 0.99;       // ansub9.f:1617 l_xl = 0.99d0
    double epsiv = 0.001;   // ansub9.f:1621 l_epsiv = 0.001d0
    int maxit = 20;         // ansub9.f:1620 l_maxit = 20
    int qmax = 50;          // ansub9.f:1591 l_qmax = 50

    // seats{finite=} -> /setopt/ Lfinit (gtseat.f:303; gtinpt.f:537 defaults F):
    // the finite-sample signal-extraction filters. Note Lfinit is read straight
    // out of the COMMON at each branch site; unlike the six numeric options
    // above it has NO `L_*` alias in ansub9.f's icode==0 bridge, which is why it
    // does not appear in the ansub9.f:1040-1130 options block.
    //
    // Lfinit does NOT touch the decomposition. MEASURED against the oracle
    // (x13as_ascii_O2) over 24 configurations -- 4 series x {(0 1 1)(0 1 1),
    // (2 1 0)(0 1 1)} x {default, seats out=0, span < 120 obs, forecasts on} --
    // every table the run writes in BOTH modes is byte-identical, s10/s11/s12/
    // s13/s16/s18 included. That invariance is gated: tests/corpus/generated/
    // *_finite-seats.spc.
    //
    // What Lfinit DOES gate is a whole output subsystem, all of it downstream of
    // the one call at sigex.f:1502, `IF (Lfinit) CALL getDiag(...)`:
    //   1. TEN save tables. getDiag -> procFlts (procflts.f:111-116) is the ONLY
    //      writer of the lSAFlt/lSAGain/lSATmShf/lTreFlt/lTreGain/ltreTmShf
    //      flags that seatdg.f:187-275 requires before it will emit
    //      faf/fac (SA_Filter_Symetric/_Conc), ftf/ftc (Trn_Filter_*),
    //      gaf/gac/gtf/gtc (squared gains) and tac/ttc (time shifts). Without
    //      finite=yes the oracle accepts those save tokens and writes NO file;
    //      with it, ten files appear (146 or 1203 rows each). These are real
    //      numeric outputs, not print surface.
    //   2. 44 .udg savelog keys: 39 `oustat*` (the alternative over/under-
    //      adjustment diagnostics) plus `pctreductionyr1..5`, which finite=yes
    //      drives to zero -- a Census defect, tools/census_bugs.md CB-14.
    //   3. The SEATS text tables at out=0 (finite asymmetric-filter weights and
    //      squared gain, FINITE SAMPLE SE-of-revision in FINALSE, the
    //      estimation-error-variance summary, the over/under + crosscorrelation
    //      tests, finite growth-rate SEs in RATESGROWTH).
    //
    // NONE of that is ported: getDiag and its ~7.6 kloc closure (getdiag/bldcov/
    // blddif/extsgnl/compmse/complagdiag/compcrodiag/comprevs/getgr/getrevdec/
    // procflts, plus altundovrtst and ansub4.f UnderOverTest) has no C++ at all,
    // and neither does any SEATS save table beyond s10-s18 or any SEATS savelog
    // key -- so (1) and (2) have no surface here to be wrong ON. The field is
    // therefore resolved and carried but deliberately not read: the
    // decomposition ignoring it IS the faithful behaviour. Port getDiag first if
    // the filter/gain tables or the SEATS savelog are ever wanted.
    bool finite = false;
};

// Resolves ctx.seatop (populated by gt_seats, readers_spec.cpp -- untouched
// here) against the defaults above: an explicitly-parsed value (not
// prm::DNOTST / prm::NOTSET) wins, otherwise the default applies.
SeatsOptions seats_resolve_options(const X13Context& ctx);

}  // namespace x13

#endif  // X13_SEATS_SEATOPTS_HPP
