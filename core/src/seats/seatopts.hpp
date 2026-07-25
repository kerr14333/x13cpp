// seatopts.hpp -- resolve ctx.seatop's raw parsed seats{} option values
// (NOTSET/DNOTST sentinels when the user left them unspecified) against
// SEATS's real numeric defaults.
//
// PARITY NOTE: those defaults do NOT live where a naive port would look --
// not in gtinpt.f's NOTSET/DNOTST table (that governs *parsing*, not the
// values F1RST/SPECTRU actually consume), but in ansub9.f:1560-1650, the
// icode==0 ("set the namelist to defaults") branch of SETDEFAULT, called
// from NMLSTS. See tools/seats_scope.md's GTSEAT note (session 2) for how
// this was found. Only the fields the ported paths actually read are
// resolved here.
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

    // ---- Hodrick-Prescott trend/cycle family --------------------------------
    // The options->internal bridge is ansub9.f:1080-1090 (Lhp) and
    // ansub9.f:1109-1117 (Hplan2); the SEATS defaults are ansub9.f:1615
    // (l_out = 0), :1641 (l_hpcycle = -1) and :1646 (l_hplan = -1.0d0).
    //
    // hpcycle: -1 = "auto" (resolve on the series length at decomposition
    // time, sigex.f:2370-2387 -- see seats_resolve_hpcycle), 0 = off,
    // 1 = split the TREND, 2 = split the SA series, 3 = split the ORIGINAL
    // series (sigex.f:2413/2434/2466).
    int hpcycle = -1;
    double hplan = -1.0;    // l_hplan; HPPARAM treats < 0.0625 as "unset"
    int hptarget = 0;       // Hptrgt as parsed (0 == NOTSET -> unset), 1/2/3
    bool hprmls = false;    // Lhprmls -- read straight out of /setopt/ by
                            // sigex.f:2421/2448/2480/2506 (no l_ mirror).
    // l_out (ansub9.f:1615 default 0, :1049 the `out=` override, :1050 the
    // "any seats PRINT table selected -> 3" override). HPOUTPUT -- and hence
    // the `cyc`/`ltt` save tables -- fire ONLY at out == 0 (ansub10.f:1157).
    int out = 0;
};

// Resolves ctx.seatop (populated by gt_seats, readers_spec.cpp -- untouched
// here) against the defaults above: an explicitly-parsed value (not
// prm::DNOTST / prm::NOTSET) wins, otherwise the default applies.
SeatsOptions seats_resolve_options(const X13Context& ctx);

// sigex.f:2370-2387 -- the hpcycle == -1 "auto" sentinel resolves at
// decomposition time against the series length: HP runs only when the span is
// long enough (10 years monthly, etc.), and then targets Hptrgt if the user
// gave one, else the trend (1). Returns 0/1/2/3. A pass-through for any
// already-resolved (>= 0) value.
int seats_resolve_hpcycle(const SeatsOptions& o, int mq, int nz);

}  // namespace x13

#endif  // X13_SEATS_SEATOPTS_HPP
