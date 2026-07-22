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
};

// Resolves ctx.seatop (populated by gt_seats, readers_spec.cpp -- untouched
// here) against the defaults above: an explicitly-parsed value (not
// prm::DNOTST / prm::NOTSET) wins, otherwise the default applies.
SeatsOptions seats_resolve_options(const X13Context& ctx);

}  // namespace x13

#endif  // X13_SEATS_SEATOPTS_HPP
