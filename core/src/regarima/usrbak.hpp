// usrbak.hpp -- the USER-DEFINED REGRESSOR backup/restore family:
// oracle/fortran/bakusr.f, addusr.f, dlusrg.f and chusrg.f.
//
// Why these four are one file: `rmfix` strips every FIXED regression column
// before an estimation pass and `addfix` puts them back afterwards, but a
// user-defined column cannot go through that path -- its DATA lives in
// `Userx`, not in the Xy matrix, so `rmfix` calls `dlusrg` to compress the
// user matrix and `addfix` calls `addusr` to rebuild the columns from a
// backup that `bakusr` took earlier. `chusrg` is the sliding-spans/history
// entry point that decides, per span, whether a user regressor is still
// well-defined over the shortened span and fixes it if not.
//
// Nothing here was reachable until `slidingspans{}` met `regression{user=}`:
// `ssmdl.f:350` is what sets `Userfx`, and `Userfx` is what turns on
// `addfix.f:73`'s addusr branch. Both `rmfix` and `addfix` used to abend
// BARE (no message) on the user-regressor arm -- invisible to `walls.py`.
#ifndef X13_REGARIMA_USRBAK_HPP
#define X13_REGARIMA_USRBAK_HPP

#include "common/x13context.hpp"

namespace x13 {

// The design half of `bakusr`'s argument list. The Fortran passes fourteen
// scalars/arrays because the routine runs against EITHER the regARIMA design
// (`Userx`/`Usrtyp`/`Usrptr`/`Ncusrx`/`Usrttl`/`Regfx`/`B`/`Rgvrtp`/`Ngrp`/
// `Grp`, rind 0) or the x11regression one (`Xuserx`/`Usxtyp`/`Usrxpt`/
// `Nusxrg`/`Usrxtt`/`Regfxx`/`Bx`/`Rgxvtp`/`Nxgrp`/`Grpx`, rind 1).
//
// NOTE the fourth member: at every rind-1 call site the Fortran passes
// `Nusxrg` -- the number of user regression TYPES from
// `x11regression{usertype=}` -- into a dummy the routine treats as the number
// of user COLUMNS (`Ncxusx`). They coincide only when each column got its own
// `usertype=` entry.
struct usr_design {
    const double* userx;   // Userx(PUSERX) base
    const int* usrtyp;     // Usrtyp(PUREG) base
    const int* usrptr;     // Usrptr(0:PUREG) -- pass &v(0)
    int ncusrx;
    const char* usrttl;    // PCOLCR*PUREG raw chars
    const bool* regfx;     // Regfx(PB) base
    const double* b;       // B(PB) base
    const int* rgvrtp;     // Rgvrtp(PB) base
    int ngrp;
    const int* grp;        // Grp(0:PGRP) -- pass &v(0)
};

usr_design usr_design_reg(X13Context& ctx);   // the regARIMA design (rind 0)
usr_design usr_design_xrg(X13Context& ctx);   // the x11regression design (rind 1)

// bakusr.f -- snapshot the user-regressor coefficients (always) and the user
// matrix itself (only when `is1st`) into /usrbak/, so `addusr` can rebuild the
// columns after `rmfix`/`dlusrg` has deleted them.
//
// CARRIES CB-40 (see the .cpp): for rind 1 the Fortran displaces the SOURCE of
// the `Userx2`/`Usrty2` copies instead of the destination, reading past the end
// of `Xuserx`/`Usxtyp` and writing slot 0. Slot 1 -- the one `addusr(1)` reads
// -- is never written by anything.
void bakusr(X13Context& ctx, const usr_design& d, int rind, bool is1st);

// addusr.f -- the inverse: delete whatever user columns are still in the
// design, restore `Userx`/`Usrtyp`/`Usrptr`/`Ncusrx`/`Usrttl` from slot `rind`,
// and re-add one `adrgef` column per user regressor with its saved coefficient
// and fix flag. Always operates on the LIVE model arrays (usrreg.cmn +
// arima.cmn), whichever design `loadxr` last swapped in.
void addusr(X13Context& ctx, int rind, int fxindx);

// dlusrg.f -- delete column `begcol` of the user matrix and compress `Userx`.
// Called from `rmfix` for each fixed user column it strips.
void dlusrg(X13Context& ctx, int begcol);

// chusrg.f -- per-span well-definedness check. For every non-fixed user column
// whose DIFFERENCED values are identically zero over the span, set `Regfx` and
// record the column title in the caller's `usfxtl` list (which sspdrv prints
// once at the end). Sets `upuser` if it fixed anything, and promotes `Iregfx`
// to 2 or 3 accordingly. An `upuser` that is already true on entry short-
// circuits the whole routine (chusrg.f:31).
void chusrg(X13Context& ctx, bool& upuser, char* usfxtl, int usfxtl_len,
            int& nusfx, int& nusftl, int* usfptr);

}  // namespace x13

#endif  // X13_REGARIMA_USRBAK_HPP
