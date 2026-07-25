// seatopts.hpp -- resolve ctx.seatop's raw parsed seats{} option values
// (NOTSET/DNOTST sentinels when the user left them unspecified) against
// SEATS's real numeric defaults.
//
// PARITY NOTE: those defaults do NOT live where a naive port would look --
// not in gtinpt.f's NOTSET/DNOTST table (that governs *parsing*, not the
// values F1RST/SPECTRU actually consume), but in ansub9.f:1560-1650, the
// icode==0 ("set the namelist to defaults") branch of SETDEFAULT, called
// from NMLSTS. See tools/seats_scope.md's GTSEAT note (session 2) for how
// this was found. The options->internal bridge that maps the /setopt/ COMMON
// fields onto SEATS's own L_* variables is ansub9.f:1032-1122; the six
// numeric fields plus the four flags below are what this port resolves.
#ifndef X13_SEATS_SEATOPTS_HPP
#define X13_SEATS_SEATOPTS_HPP

#include "common/x13context.hpp"
#include "seats/model_decode.hpp"

namespace x13 {

struct SeatsOptions {
    double rmod = 0.5;      // ansub9.f:1587 l_rmod = .50d0
    double epsphi = 2.0;    // ansub9.f:1614 l_epsphi = 2.0d0
    double xl = 0.99;       // ansub9.f:1617 l_xl = 0.99d0
    double epsiv = 0.001;   // ansub9.f:1621 l_epsiv = 0.001d0
    int maxit = 20;         // ansub9.f:1620 l_maxit = 20
    int qmax = 50;          // ansub9.f:1591 l_qmax = 50

    // seats{noadmiss=} -> Lnoadm -> L_NOADMISS (ansub9.f:1064-1068). The
    // bridge sets it UNCONDITIONALLY from Lnoadm, whose gtinpt.f:533 default
    // is .false. -- so SETDEFAULT's l_noadmiss=1 (ansub9.f:1636) never
    // survives into an X-13 run and the effective default is 0. Only live
    // when the canonical decomposition turns out INADMISSIBLE; see
    // seats_decomp_unported_reason.
    int noadmiss = 0;
    // seats{imean=} -> Kmean -> L_IMEAN (ansub9.f:1072-1080). NOT simply
    // "off" by default: with Kmean==NOTSET the bridge derives it from whether
    // the fitted regARIMA model carries a 'Constant' regressor group
    // (strinx(Grpttl,...,'Constant')); an explicit seats{imean=yes/no}
    // OVERRIDES that derivation (gtseat.f:124 Kmean=2-ivec(1)).
    int imean = 0;
    // seats{statseas=} -> Lstsea -> L_statseas (ansub9.f:1090-1094; gtinpt.f:
    // 535 default .false.). Consumed ONLY by CHANGEMODEL (ansub1.f:3634-3747);
    // see seats_changemodel_cambiado.
    int statseas = 0;
    // seats{bias=} -> Bias2 -> L_Bias (ansub9.f:1118-1122): SETDEFAULT seeds
    // l_bias=-2 (ansub9.f:1600) and the bridge turns that sentinel into 1, so
    // the effective default is 1. THEN analts.f:1647-1653 silently promotes a
    // user-supplied bias=0 back to 1 ("BIAS SET EQUAL TO 1" in the .out) --
    // so of the three values gtseat.f:281-291 accepts, only -1 is live.
    // Reproduced verbatim in seats_resolve_options; do NOT "fix" it.
    int bias = 1;
};

// Resolves ctx.seatop (populated by gt_seats, readers_spec.cpp -- untouched
// here) against the defaults above: an explicitly-parsed value (not
// prm::DNOTST / prm::NOTSET) wins, otherwise the default applies.
SeatsOptions seats_resolve_options(const X13Context& ctx);

// CHANGEMODEL's `cambiado` return (ansub1.f:3634-3747), evaluated as a
// PREDICATE only. The oracle calls it from analts.f:1715/2260 on the decoded
// SEATS model; a nonzero result means SEATS REWRITES the ARIMA orders and
// sets init=0, i.e. re-estimates the changed model with its own ML machinery
// -- a subsystem this port does not have (the C++ SEATS consumes the fitted
// regARIMA model directly through nmlmdl/seats_decode_model). So we only need
// to know WHETHER it fires, in order to fatal instead of silently decomposing
// the wrong model. `posbphi` is L_posbphi, which SETDEFAULT seeds 0
// (ansub9.f:1578) and the X-13 bridge never overrides -- the bd!=0 branch is
// therefore dead in this program, but is ported for faithfulness.
int seats_changemodel_cambiado(const SeatsModelOrders& mo, double rmod,
                               int statseas, int posbphi);

// Model-stage scope guard: returns nullptr when the resolved options + decoded
// model stay inside the ported SEATS scope, else a description of the unported
// path (used to fatal rather than return OUTCOME: OK with wrong numbers).
// `is_log` == the lam==0 (log transform) decomposition domain.
const char* seats_model_unported_reason(const SeatsOptions& opts,
                                        const SeatsModelOrders& mo,
                                        bool is_log);

// Decomposition-stage scope guard: the two "decomposition invalid" tests the
// oracle applies once the canonical spectrum is known -- SPECTRU's
// admissibility number qt1 (spectrum.f:389/446-547) and DecompSpectrum's
// cycle white-noise variance (spectrum.f:1709-1737). With noadmiss=0 the
// oracle ABORTS SEATS outright (no s-tables at all); with noadmiss=1 it hands
// the model to APPROXIMATE (sigex.f:824) and re-estimates. Neither is ported,
// so both fatal here.
const char* seats_decomp_unported_reason(const SeatsOptions& opts, double qt1,
                                         int ncycth, int ncyc, double varwnc);

}  // namespace x13

#endif  // X13_SEATS_SEATOPTS_HPP
