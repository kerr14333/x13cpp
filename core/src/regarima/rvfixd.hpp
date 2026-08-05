// rvfixd.hpp -- rvfixd.f: hold every regressor belonging to one of the named
// GROUPS fixed for the rest of a run.
//
// Shared, not duplicated: the oracle calls this from THREE places with three
// different meanings, and all three take the arrays as arguments rather than
// reading model.cmn, precisely so the same walk can be pointed at either the
// regARIMA design or the x11regression one.
//
//   revdrv.f:131-134  history{fixreg=}          -- both designs
//   ssmdl.f:53        slidingspans{fixreg=}     -- the regARIMA design
//   ssxmdl.f:80       slidingspans{fixreg=}     -- the x11regression design
//
// A user-defined column carries the group it was DECLARED with (usertype=), not
// the generic PRGTUD it occupies in Rgvrtp, so the walk has to advance the
// user-column cursor `iusr` in lockstep -- and, faithfully, it advances that
// cursor for the user HOLIDAY and SEASONAL types too without substituting them
// (only PRGTUD is re-typed). Note that `usrfix` and `tdfix` both claim the
// user-TD/length-of-period types, so fixreg=(td) fixes a usertype=td column.
#ifndef X13_REGARIMA_RVFIXD_HPP
#define X13_REGARIMA_RVFIXD_HPP

#include "gen/model.hpp"   // prm:: regression-type constants (PRG*)
#include "x13/farray.hpp"

namespace x13 {

inline void rvfixd(bool tdfix, bool holfix, bool otlfix, bool usrfix,
                   int& iregfx, x13::farray1<bool, 80>& regfx, int nb,
                   const x13::farray1<int, 80>& rgvrtp, int nusrrg,
                   const x13::farray1<int, 52>& usrtyp, int ncusrx,
                   bool& userfx) {
    using namespace prm;
    int iusr = 1;
    bool allfix = true;
    for (int i = 1; i <= nb; ++i) {
        int rtype = rgvrtp(i);
        if (nusrrg > 0) {
            if (rtype == PRGTUD) {
                rtype = usrtyp(iusr);
                iusr += 1;
            } else if ((rtype >= PRGTUH && rtype <= PRGUH5) ||
                       rtype == PRGTUS) {
                iusr += 1;
            }
        }
        const bool istd =
            (rtype == PRGTTD || rtype == PRGTST || rtype == PRRTTD ||
             rtype == PRRTST || rtype == PRATTD || rtype == PRATST ||
             rtype == PRG1TD || rtype == PRR1TD || rtype == PRA1TD ||
             rtype == PRG1ST || rtype == PRR1ST || rtype == PRA1ST) ||
            (rtype == PRGTLM || rtype == PRGTSL || rtype == PRGTLQ ||
             rtype == PRGTLY || rtype == PRRTLQ || rtype == PRRTLM ||
             rtype == PRRTSL || rtype == PRATSL || rtype == PRRTLY ||
             rtype == PRATLM || rtype == PRATLQ || rtype == PRATLY) ||
            rtype == PRGUTD || rtype == PRGULY || rtype == PRGULM ||
            rtype == PRGULQ;
        const bool ishol =
            rtype == PRGTEA || rtype == PRGTEC || rtype == PRGTES ||
            rtype == PRGTLD || rtype == PRGTTH ||
            (rtype >= PRGTUH && rtype <= PRGUH5);
        const bool isusr =
            rtype == PRGTUD || rtype == PRGTUS ||
            (rtype >= PRGTUH && rtype <= PRGUH5) || rtype == PRGUTD ||
            rtype == PRGULY || rtype == PRGULM || rtype == PRGULQ ||
            rtype == PRGUAO || rtype == PRGULS || rtype == PRGUCN ||
            rtype == PRGUCY || rtype == PRGUSO;
        const bool isotl =
            rtype == PRGTAO || rtype == PRGTLS || rtype == PRGTRP ||
            rtype == PRGTTC || rtype == PRGTSO || rtype == PRGTAL ||
            rtype == PRGTAA || rtype == PRGTAT || rtype == PRGTQD ||
            rtype == PRGTQI || rtype == PRGTTL || rtype == PRGUAO ||
            rtype == PRGULS || rtype == PRGUSO;
        if ((tdfix && istd) || (holfix && ishol) || (usrfix && isusr) ||
            (otlfix && isotl)) {
            regfx(i) = true;
            if (iregfx <= 1) iregfx = 2;
        }
        allfix = allfix && regfx(i);
    }
    if (allfix && iregfx == 2) iregfx = 3;
    if (!userfx) userfx = (usrfix && ncusrx > 0);
}

}  // namespace x13

#endif  // X13_REGARIMA_RVFIXD_HPP
