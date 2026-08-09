// usrbak.cpp -- see usrbak.hpp. Faithful ports of oracle/fortran/bakusr.f,
// addusr.f, dlusrg.f and chusrg.f.
#include "regarima/usrbak.hpp"

#include <string>
#include <string_view>
#include <vector>

#include "regarima/armafilt.hpp"    // arflt
#include "numeric/numeric.hpp"      // daxpy, dpeq
#include "specparse/specparse.hpp"  // strinx, getstr, insstr, delstr, adrgef,
                                     // dlrgef, copy, cpyint, setdp, dfdate,
                                     // abend, errhdr, writln, stdio::STDERR
#include "gen/model.hpp"            // prm PRG* type codes, PUREG/PUSERX/PB
#include "gen/srslen.hpp"           // prm::PLEN

namespace x13 {
using namespace prm;

namespace {
// The same "is this a user-defined regressor type" list bakusr.f:31-37,
// addusr.f:34-38, addfix.f:36-42 and rmfix.f:82-88 all spell out inline.
bool is_user_type(int rt) {
    return (rt >= PRGTUH && rt <= PRGUH5) || rt == PRGTUS || rt == PRGUTD ||
           rt == PRGTUD || rt == PRGULM || rt == PRGULQ || rt == PRGULY ||
           rt == PRGUAO || rt == PRGULS || rt == PRGUSO || rt == PRGUCN ||
           rt == PRGUCY;
}
}  // namespace

usr_design usr_design_reg(X13Context& ctx) {
    return {ctx.arima.userx.data(), ctx.usrreg.usrtyp.data(),
            &ctx.usrreg.usrptr(0),   ctx.usrreg.ncusrx,
            ctx.usrreg.usrttl.data(), ctx.model.regfx.data(),
            ctx.mdldat.b.data(),     ctx.model.rgvrtp.data(),
            ctx.model.ngrp,          &ctx.model.grp(0)};
}

usr_design usr_design_xrg(X13Context& ctx) {
    return {ctx.xrgmdl.xuserx.data(), ctx.usrxrg.usxtyp.data(),
            &ctx.usrxrg.usrxpt(0),    ctx.xrgmdl.nusxrg,
            ctx.usrxrg.usrxtt.data(), ctx.xrgmdl.regfxx.data(),
            ctx.xrgmdl.bx.data(),     ctx.xrgmdl.rgxvtp.data(),
            ctx.xrgmdl.nxgrp,         &ctx.xrgmdl.grpx(0)};
}

// ---------------------------------------------------------------------------
// bakusr.f
// ---------------------------------------------------------------------------
void bakusr(X13Context& ctx, const usr_design& d, int rind, bool is1st) {
    urgbak_cmn& u = ctx.urgbak;

    // bakusr.f:27-44 -- walk the design and copy every user column's
    // coefficient and fix flag into slot `rind` of Buser/Fxuser. This half is
    // displaced CORRECTLY (destination side), unlike the block below.
    int iuser = PUREG * rind;
    for (int igrp = 1; igrp <= d.ngrp; ++igrp) {
        const int begcol = d.grp[igrp - 1];
        const int endcol = d.grp[igrp] - 1;
        if (!is_user_type(d.rgvrtp[begcol - 1])) continue;
        for (int i = begcol; i <= endcol; ++i) {
            ++iuser;
            u.buser(iuser) = d.b[i - 1];
            u.fxuser(iuser) = d.regfx[i - 1];
        }
    }

    if (!is1st) return;

    // CB-40 -- bakusr.f:49-52 DISPLACE THE SOURCE, NOT THE DESTINATION.
    //
    //     disp=(PUSERX*Rind)+1
    //     CALL copy(Userx(disp),PUSERX,1,Userx2)
    //     disp=(PUREG*Rind)+1
    //     CALL cpyint(Usrtyp(disp),PUREG,1,Usrty2)
    //
    // `Userx2` and `Usrty2` are the two-slot backups; :54's Usrpt2 write and
    // :55-56's Ncusx2/Usrtt2 writes displace the DESTINATION and are right.
    // For rind 0 the bug is invisible (disp==1 either way). For rind 1 --
    // reached from `ssxmdl.f:146`, `sspdrv.f:159`, `revdrv.f:317`/`:626` and
    // `editor.f:1543`, i.e. `x11regression{usertype=}` under slidingspans or
    // history -- it reads `Xuserx(PUSERX+1 …)` and `Usxtyp(PUREG+1 …)`, past
    // the end of both, and writes what it finds into SLOT 0. Confirmed with a
    // bounds-checked build of the vendored sources:
    //   "At line 50 of file bakusr.f -- Index '53041' of dimension 1 of array
    //    'userx' above upper bound of 53040".
    //
    // The OBSERVABLE is deterministic even so, which is why the rind-1 arm is
    // ported rather than walled: slot 1 is never written by ANY call, so
    // `addusr(1)` restores `Userx` and `Usrtyp` as all-zero from a /usrbak/
    // that was never touched -- the x11regression user column comes back with
    // an identically zero data matrix and type 0 ("User-defined"). Only slot 0
    // holds the garbage. This port reproduces the rind-1 effect (leave slot 1
    // alone) and does NOT reproduce the clobber of slot 0 -- see the block in
    // `addusr` below, which is where that decision is measured.
    if (rind == 0) {
        copy(&d.userx[0], PUSERX, 1, u.userx2.data());
        cpyint(&d.usrtyp[0], PUREG, 1, u.usrty2.data());
    }
    cpyint(&d.usrptr[0], PUREG + 1, 1, &u.usrpt2((PUREG + 1) * rind + 1));
    u.ncusx2(rind) = d.ncusrx;
    u.usrtt2(rind) = std::string_view(d.usrttl, PCOLCR * PUREG);
}

// ---------------------------------------------------------------------------
// addusr.f
// ---------------------------------------------------------------------------
void addusr(X13Context& ctx, int rind, int fxindx) {
    (void)fxindx;   // addusr.f:68/128 -- the Fxindx test is COMMENTED OUT
    model_cmn& m = ctx.model;
    mdldat_cmn& dd = ctx.mdldat;
    arima_cmn& ar = ctx.arima;
    usrreg_cmn& ur = ctx.usrreg;
    urgbak_cmn& u = ctx.urgbak;

    // ---- CB-40, and the one place its garbage would be read ----------------
    //
    // Slot 0 is read here and nowhere else. In the oracle it has been clobbered
    // by any preceding `bakusr(rind=1)` with storage past the end of `Xuserx` in
    // COMMON /cx11rd/. This port does NOT reproduce that -- it cannot; the bytes
    // are whatever the link map put after the COMMON -- and restores the backup
    // it actually took. Until 2026-08-09 this arm was a WALL instead, which
    // refused a run the oracle completes; that is a gap, and the measurements
    // below are why it is now a documented deviation rather than a refusal.
    //
    // MEASURED against an instrumented build of the vendored sources (a
    // scratchpad copy -- oracle/fortran is never edited):
    //   * after `editor.f:1543`'s bakusr(1), slot 0 holds 0.05, 0.5, then zeros
    //     -- `Cvxalf` and `Cvxrdc`, the two doubles that follow `Xuserx` in
    //     /cx11rd/, and then storage that reads as zero;
    //   * `addusr(0)` restores that into `Userx` and the oracle DOES read it
    //     afterwards: poisoning it with 1e30 moves d10/d11/d12/d13/b16/c16;
    //   * a `bakusr` patched to displace the DESTINATION -- i.e. made to behave
    //     the way this port behaves -- moves those same six tables AWAY from the
    //     stock oracle;
    //   * the oracle's within-sample aape is computed BEFORE the restore: it
    //     does not move under the poison, under the destination fix, or under a
    //     full CB-40 repair.
    //
    // And yet the engine, restoring the correct backup, is bit-exact against the
    // STOCK oracle on both shapes that reach here (`extra/airline_x11regression-
    // reg-user-bothfixed`, 23 gates, and `extra/airline_slidingspans-reg-
    // x11regression-user-bothfixed`, 26). So this port agrees by CANCELLATION,
    // not by faithfulness, and both halves are written down because either one
    // changing alone breaks the other:
    //   * it captures aape AFTER this restore where the oracle captures it
    //     before -- mutation: zeroing the matrix restored below moves aape;
    //   * its x11 factors do not re-derive from `Userx` after this point at all
    //     -- the same mutation moves NOTHING else, while in the oracle the six
    //     tables above depend on it.
    // Anyone extending user-regressor work here must re-measure that pair.

    // addusr.f:28-51 -- delete whatever user columns survive in the design.
    if (ur.ncusrx > 0) {
        int igrp = m.ngrp;
        while (igrp >= 1) {
            const int begcol = m.grp(igrp - 1);
            const int ncol = m.grp(igrp) - begcol;
            if (is_user_type(m.rgvrtp(begcol))) {
                for (int icol = begcol; icol <= begcol + ncol - 1; ++icol) {
                    std::string thisu;
                    int nusr = 0;
                    // addusr.f:40 passes Nb, not Ncoltl, as the element count.
                    getstr(ctx, m.colttl.data(), m.colptr.data(), m.nb, icol,
                           thisu, nusr);
                    if (ctx.error.lfatal) return;
                    // addusr.f:42 searches slot `Rind`'s TITLES with slot 0's
                    // POINTER table (`Usrpt2` is passed undisplaced).
                    const int ucol =
                        strinx(false, u.usrtt2(rind).raw(), &u.usrpt2(1), 1,
                               u.ncusx2(rind), thisu);
                    u.buser(ucol) = dd.b(icol);
                }
                dlrgef(ctx, begcol, ar.nrxy, ncol);
                if (ctx.error.lfatal) return;
            }
            --igrp;
        }
    }

    // addusr.f:55-62 -- restore the user matrix and its descriptors.
    copy(&u.userx2(PUSERX * rind + 1), PUSERX, 1, ar.userx.data());
    cpyint(&u.usrpt2((PUREG + 1) * rind + 1), PUREG + 1, 1, &ur.usrptr(0));
    cpyint(&u.usrty2(PUREG * rind + 1), PUREG, 1, ur.usrtyp.data());
    ur.ncusrx = u.ncusx2(rind);
    ur.usrttl = u.usrtt2(rind).raw();

    // addusr.f:66-129 -- re-add one column per user regressor. The group title
    // is chosen from the SAVED Usrtyp, so a zeroed slot (CB-40 above) lands
    // every column in the generic "User-defined" group.
    const int disp = PUREG * rind;
    for (int i = 1; i <= ur.ncusrx; ++i) {
        std::string effttl;
        int nchr = 0;
        getstr(ctx, ur.usrttl.data(), &ur.usrptr(0), ur.ncusrx, i, effttl, nchr);
        if (ctx.error.lfatal) return;
        const int ut = ur.usrtyp(i);
        const char* grp = "User-defined";
        int vartyp = ut;
        if (ut == PRGTUS)       grp = "User-defined Seasonal";
        else if (ut == PRGTUH)  grp = "User-defined Holiday";
        else if (ut == PRGUH2)  grp = "User-defined Holiday Group 2";
        else if (ut == PRGUH3)  grp = "User-defined Holiday Group 3";
        else if (ut == PRGUH4)  grp = "User-defined Holiday Group 4";
        else if (ut == PRGUH5)  grp = "User-defined Holiday Group 5";
        else if (ut == PRGUTD)  grp = "User-defined Trading Day";
        else if (ut == PRGULY)  grp = "User-defined Leap Year";
        else if (ut == PRGULM)  grp = "User-defined LOM";
        else if (ut == PRGULQ)  grp = "User-defined LOQ";
        else if (ut == PRGUAO)  grp = "User-defined AO";
        else if (ut == PRGULS)  grp = "User-defined LS";
        else if (ut == PRGUSO)  grp = "User-defined SO";
        else if (ut == PRGUCN)  grp = "User-defined Constant";
        else if (ut == PRGUCY)  grp = "User-defined Cycle";
        else                    vartyp = PRGTUD;   // addusr.f:125-126
        adrgef(ctx, u.buser(disp + i), effttl, grp, vartyp,
               u.fxuser(disp + i), false);
        if (ctx.error.lfatal) return;
    }
}

// ---------------------------------------------------------------------------
// dlusrg.f
// ---------------------------------------------------------------------------
void dlusrg(X13Context& ctx, int begcol) {
    arima_cmn& ar = ctx.arima;
    usrreg_cmn& ur = ctx.usrreg;

    const int noldc = ur.ncusrx;
    if (begcol < 1 || begcol > ur.ncusrx) {
        errhdr(ctx);
        writln(ctx,
               " ERROR: Deleted column not within the user-regression matrix.",
               stdio::STDERR, ctx.units.mt2, true);
        abend(ctx);
        return;
    }
    // delstr decrements Ncusrx.
    delstr(ctx, begcol, ur.usrttl.data(), &ur.usrptr(0), ur.ncusrx, PUREG);
    if (ctx.error.lfatal) return;
    // dlusrg.f:59 -- `noldc-1-Begcol` is ONE SHORT of the noldc-Begcol entries
    // that need shifting down, so the LAST user type is left stale. Not a CB
    // entry: unobservable while `Usrtyp` is re-read only up to the new Ncusrx,
    // and the copy is a no-op for the single-column case that reaches here.
    cpyint(&ur.usrtyp(begcol + 1), noldc - 1 - begcol, 1, &ur.usrtyp(begcol));

    if (noldc == 1) return;
    int iend = begcol - 1;
    for (int i = 1; i <= ar.nrusrx - 1; ++i) {
        const int offset = i;
        const int ibeg = iend + 1;
        iend = iend + ur.ncusrx;
        for (int j = ibeg; j <= iend; ++j) ar.userx(j) = ar.userx(j + offset);
    }
    {
        const int offset = ar.nrusrx;
        const int ibeg = iend + 1;
        for (int j = ibeg; j <= ar.nrusrx * ur.ncusrx; ++j)
            ar.userx(j) = ar.userx(j + offset);
    }
}

// ---------------------------------------------------------------------------
// chusrg.f
// ---------------------------------------------------------------------------
void chusrg(X13Context& ctx, bool& upuser, char* usfxtl, int usfxtl_len,
            int& nusfx, int& nusftl, int* usfptr) {
    model_cmn& m = ctx.model;
    mdldat_cmn& dd = ctx.mdldat;
    arima_cmn& ar = ctx.arima;
    usrreg_cmn& ur = ctx.usrreg;

    // chusrg.f:31 -- an already-true Upuser short-circuits the whole routine.
    if (upuser) return;
    int iuser = ur.ncusrx + 1;

    int disp = 0;
    dfdate(ar.begmdl.data(), ar.bgusrx.data(), m.sp, disp);

    bool allfix = true;
    std::vector<double> fvec(PLEN, 0.0);
    for (int i = m.nb; i >= 1; --i) {
        const int rtype = m.rgvrtp(i);
        if ((rtype == PRGTUD || (rtype >= PRGTUH && rtype <= PRGUH5) ||
             rtype == PRGTUS) && !m.regfx(i)) {
            --iuser;
            const int nuser = dd.nspobs;
            // The differenced user column, over the model span. Userx is
            // stored row-major with Ncusrx columns, so the stride is Ncusrx.
            setdp(0.0, PLEN, fvec.data());
            const int uptr = (ur.ncusrx * disp) + iuser;
            daxpy(nuser, 1.0, &ar.userx(uptr), ur.ncusrx, fvec.data(), 1);
            int neltc = nuser;
            arflt(nuser, dd.arimap.data(), m.arimal.data(), m.opr.data(),
                  m.mdl(DIFF - 1), m.mdl(DIFF) - 1, fvec.data(), neltc);
            int j = 1;
            while (dpeq(fvec[j - 1], 0.0) && j <= nuser) ++j;
            if (j > nuser) {
                // Identically zero over this span: fix it and record it.
                m.regfx(i) = true;
                upuser = true;
                std::string str;
                int nchr = 0;
                getstr(ctx, m.colttl.data(), m.colptr.data(), m.ncoltl, i, str,
                       nchr);
                if (ctx.error.lfatal) return;
                int k = 0;
                if (nusftl > 0)
                    k = strinx(false, std::string_view(usfxtl, usfxtl_len),
                               usfptr, 1, nusftl, str);
                if (k == 0) {
                    insstr(ctx, str, nusfx, PUREG, usfxtl, usfxtl_len, usfptr,
                           nusftl);
                    if (ctx.error.lfatal) return;
                    ++nusfx;
                }
            }
        }
        if (!m.regfx(i) && allfix) allfix = false;
    }

    if (upuser) m.iregfx = allfix ? 3 : 2;
}

}  // namespace x13
