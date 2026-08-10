// x11reg.cpp -- x11regression{} irregular-component regression.
//
// Multiplicative path of x11mdl.f + tdset/xrgtrn/tdxtrm/dlrgrw/regx11/x11ref/
// mulref/x11aic, wired into x11pt2 at the B/C iterations (b16/c16 + xrm emit +
// the divsub Sti fold). Automatic AO outlier ID runs via the shared idotlr
// (lxreg path). Reuses olsreg/resid/daxpy/idotlr/xrlkhd/addeas.
// See tools/x11regression_scope.md + x11regression_aictest_scope.md.
#include "x11/x11reg.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "common/x13context.hpp"
#include "regarima/estimate.hpp"   // olsreg, resid, xrlkhd
#include "automdl/automd_finalize.hpp"  // rmfix, addfix (x11mdl.f:390/459)
#include "regarima/regvar.hpp"     // regvar
#include "regarima/outlier.hpp"    // idotlr, setcv
#include "automdl/aictst.hpp"      // addeas
#include "numeric/numeric.hpp"     // daxpy
#include "x11/x11filt.hpp"         // divsub
#include "x11/slidingspans.hpp"    // ssrit (x11mdl.f:874's per-span TD store)
#include "specparse/specparse.hpp" // addate
#include "gen/model.hpp"           // PRG* regressor types, PSNGER
#include "gen/notset.hpp"          // prm::DNOTST
#include "x13/fformat.hpp"         // fwrite_fmt (prterx's two-channel diagnostic)

namespace x13 {
namespace {

constexpr int PLEN = 1020;
constexpr int PXPX = 3403;  // Chlxpx packed size (mdldat.cmn)

// Local clean-fatal (x11parts.cpp's x11_not_ported is TU-local).
void x11reg_not_ported(X13Context& ctx, const char* what) {
    errhdr(ctx);
    writln(ctx, std::string("ERROR: ") + what + " not yet ported (M5 X-11 spine).",
           stdio::STDERR, ctx.units.mt2, true);
    abend(ctx);
}

// x11aic.f:115-123 / :260-268 -- the trading-day regressor families the AIC
// test strips out of the irregular-regression design (both lists are the same
// 24 types). Note this is is_td_rgvr()'s list from aictst.cpp with the
// length-of-month family taken UNCONDITIONALLY: tdaic.f gates those on
// `Lomtst.eq.0` because it runs its own separate LOM test, and x11aic has no
// LOM test to keep them for.
bool is_x11aic_td_type(int t) {
    return t == prm::PRGTTD || t == prm::PRGTST || t == prm::PRRTTD ||
           t == prm::PRRTST || t == prm::PRATTD || t == prm::PRATST ||
           t == prm::PRG1TD || t == prm::PRR1TD || t == prm::PRA1TD ||
           t == prm::PRG1ST || t == prm::PRR1ST || t == prm::PRA1ST ||
           t == prm::PRGTLM || t == prm::PRGTSL || t == prm::PRGTLQ ||
           t == prm::PRGTLY || t == prm::PRRTLM || t == prm::PRRTSL ||
           t == prm::PRRTLQ || t == prm::PRRTLY || t == prm::PRATLM ||
           t == prm::PRATSL || t == prm::PRATLQ || t == prm::PRATLY;
}

// x11aic.f:126-128 / :131-133 -- the user-defined regressor families the USER
// AICC test strips out (and restores through the seven adrgef arms at :496-521).
bool is_x11aic_user_type(int t) {
    return t == prm::PRGTUD || t == prm::PRGTUH || t == prm::PRGUAO ||
           t == prm::PRGUTD || t == prm::PRGULM || t == prm::PRGULQ ||
           t == prm::PRGULY;
}

// A holiday-family regressor column (x11ref.f:47-50 daxpy-into-Fhol predicate):
// Easter, labor day, Thanksgiving, StatCan easter, user holiday.
bool is_hol_type(int t) {
    return t == prm::PRGTEA || t == prm::PRGTLD || t == prm::PRGTTH ||
           t == prm::PRGTEC || t == prm::PRGTUH;
}

// A TD-family regressor column (x11ref.f:37-52 daxpy-into-Ftd predicate).
bool is_td_type(int t) {
    return t == prm::PRGTTD || t == prm::PRGTST || t == prm::PRRTTD ||
           t == prm::PRRTST || t == prm::PRATTD || t == prm::PRATST ||
           t == prm::PRGTLY || t == prm::PRRTLY || t == prm::PRATLY ||
           t == prm::PRG1TD || t == prm::PRR1TD || t == prm::PRA1TD ||
           t == prm::PRG1ST || t == prm::PRR1ST || t == prm::PRA1ST ||
           t == prm::PRGUTD || t == prm::PRGULM || t == prm::PRGULQ ||
           t == prm::PRGULY;
}

// mulref.f: Fac = Tmp/Xvec[irow+Xdev-1] (or /Xval scalar when Xval>0); Same
// overwrites Fac, else accumulates into it.
void mulref(int nrxy, double* fac, const double* tmp, int xdev,
            const double* xvec, double xval, bool same) {
    if (xval > 0.0) {
        for (int irow = 1; irow <= nrxy; ++irow)
            fac[irow - 1] = same ? tmp[irow - 1] / xval
                                 : fac[irow - 1] + tmp[irow - 1] / xval;
    } else {
        for (int irow = 1; irow <= nrxy; ++irow) {
            const int j = irow + xdev - 1;
            fac[irow - 1] = same ? tmp[irow - 1] / xvec[j - 1]
                                 : fac[irow - 1] + tmp[irow - 1] / xvec[j - 1];
        }
    }
}

}  // namespace

// ---- tdset.f -------------------------------------------------------------
void tdset_td(X13Context& ctx, const int* begdat, int lfda, int llda, int sp) {
    static const int lnomo[2][12] = {
        {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
        {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};
    static const int fdomo[2][12] = {
        {0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5},
        {0, 3, 4, 0, 2, 5, 0, 3, 6, 1, 4, 6}};
    static const int lnoqtr[2][4] = {{90, 91, 92, 92}, {91, 91, 92, 92}};
    static const int fdoqtr[2][4] = {{0, 6, 6, 0}, {0, 0, 0, 1}};
    static const int fouryr[4] = {0, 2, 3, 4};
    auto& t = ctx.tdtyp;
    auto& x = ctx.xtdtyp;
    int predat[2];
    addate(begdat, sp, -lfda, predat);
    for (int irow = lfda; irow <= llda; ++irow) {
        int idate[2];
        addate(predat, sp, irow, idate);
        const int year = idate[0], period = idate[1];
        int sdoyr = 5 * (year / 4 - 441) + fouryr[year % 4];
        int cendsp = (year - 1601) / 100;
        cendsp = cendsp - cendsp / 4 - 1;
        sdoyr = (sdoyr - cendsp) % 7;
        const int lpyr =
            (((year % 100 != 0 && year % 4 == 0) || year % 400 == 0) ? 2 : 1);
        int sd, nd;
        if (sp == 12) {
            sd = (sdoyr + fdomo[lpyr - 1][period - 1]) % 7;
            nd = lnomo[lpyr - 1][period - 1];
        } else {
            sd = (sdoyr + fdoqtr[lpyr - 1][period - 1]) % 7;
            nd = lnoqtr[lpyr - 1][period - 1];
        }
        if (sd == 0) sd = 7;
        t.tday(irow) = sd;
        x.xn(irow) = static_cast<double>(nd);
        x.xnstar(irow) = static_cast<double>(nd);
        if (sp == 12) {
            if (nd == 31) t.tday(irow) += 7;
            if (nd == 28) t.tday(irow) += 14;
            if (nd == 29) t.tday(irow) += 21;
            if (period == 2) x.xnstar(irow) = 28.25;
        } else {
            if (nd == 92) t.tday(irow) += 7;
            if (nd == 90) t.tday(irow) += 14;
            if (lpyr == 2 && period == 1) t.tday(irow) += 21;
            if (period == 1) x.xnstar(irow) = 90.25;
        }
        x.xlpyr(irow) = x.xn(irow) - x.xnstar(irow);
    }
    x.daybar = (sp == 12) ? 30.4375 : 91.25;
}

// ---- xrgtrn.f (mult arm) -------------------------------------------------
// The Tdgrp arm matters: with no trading-day group in the model the irregular
// is only centred (`X-1`), NOT rescaled by the day counts. Every fixed-TD
// x11regression spec takes the first arm, which is why this used to assume it;
// x11aic's no-TD candidate is what reaches the second.
//
// xrgtrn.f:36-40 -- inside the Tdgrp>0 arm, Kswv==3 (a tdprior prior TD has
// ALREADY divided the day-count effect out of the irregular, see x11pt1.f:235)
// subtracts Xnstar rather than Xn, i.e. the centring is against the STANDARD
// month length, not the actual one. Only the Kswv==3 route reaches it.
//
// The Haveum / Psuadd / log-additive arms of xrgtrn.f:22-49 are still unported;
// Haveum (x11regression umdata=) is never set on this port and the other two are
// walled upstream in x11parts/x11mdl.
void xrgtrn_td(X13Context& ctx, double* x, int l1, int l2, int tdgrp) {
    const auto& xt = ctx.xtdtyp;
    const bool kswv3 = (ctx.x11opt.kswv == 3);
    for (int i = l1; i <= l2; ++i) {
        const int i2 = i - l1 + 1;
        if (tdgrp > 0)
            x[i2 - 1] = xt.xnstar(i) * x[i2 - 1] -
                        (kswv3 ? xt.xnstar(i) : xt.xn(i));
        else
            x[i2 - 1] = x[i2 - 1] - 1.0;
    }
}

// ---- tdxtrm.f ------------------------------------------------------------
void tdxtrm_td(X13Context& ctx, const double* sti, double sigm, int kpart,
               int irridx, int irrend) {
    auto& xc = ctx.xclude;
    const int* tday = ctx.tdtyp.tday.data();
    const double* faccal = ctx.x11fac.faccal.data();
    const int posfob = ctx.x11ptr.posfob;
    const int muladd = ctx.x11opt.muladd;
    for (int i = 1; i <= PLEN; ++i) xc.rgxcld(i) = false;
    xc.nxcld = 0;
    std::vector<int> karray(PLEN + 1, 0);
    for (int i = 1; i <= posfob; ++i) karray[i] = tday[i - 1];
    double tmean[29], tcc[29];
    int kstd = 0;
    while (kstd < 2) {
        double tsd = 0.0, tk = 0.0;
        if (kpart == 2) {
            const double tkon = (muladd == 1) ? 0.0 : 1.0;
            for (int i = 1; i <= 28; ++i) {
                tcc[i] = 0.0;
                tmean[i] = (i <= 21) ? 0.0 : tkon;
            }
            for (int i = irridx; i <= irrend; ++i) {
                const int m = karray[i];
                if (m < 15) {
                    tmean[m] += sti[i - 1];
                    tcc[m] += 1.0;
                    tk += 1.0;
                } else if (m <= 21) {
                    for (int k = 15; k <= 21; ++k) {
                        tmean[k] += sti[i - 1];
                        tcc[k] += 1.0;
                    }
                    tk += 1.0;
                }
            }
            for (int i = 1; i <= 21; ++i)
                if (tcc[i] > 0.0) tmean[i] /= tcc[i];
            for (int i = irridx; i <= irrend; ++i) {
                const int m = karray[i];
                if (m <= 21) {
                    const double d = sti[i - 1] - tmean[m];
                    tsd += d * d;
                }
            }
        } else {
            for (int i = irridx; i <= irrend; ++i) {
                const int m = karray[i];
                if (m <= 28) {
                    const double d = sti[i - 1] - faccal[i - 1];
                    tsd += d * d;
                    tk += 1.0;
                }
            }
        }
        tsd = std::sqrt(tsd / tk) * sigm;
        ++kstd;
        for (int i = irridx; i <= irrend; ++i) {
            const int m = karray[i];
            if (m <= 28) {
                const double tirr = (kpart == 2) ? tmean[m] : faccal[i - 1];
                if (std::fabs(sti[i - 1] - tirr) > tsd) {
                    karray[i] += 28;
                    xc.rgxcld(i - irridx + 1) = true;
                    ++xc.nxcld;
                }
            }
        }
    }
}

// ---- dlrgrw.f ------------------------------------------------------------
void dlrgrw(double* xy, int ncxy, int nrxy, const bool* rgxcld) {
    int i2 = 1;
    for (int i = 1; i <= nrxy; ++i) {
        if (!rgxcld[i - 1]) {
            const int disp1 = (i - 1) * ncxy, disp2 = (i2 - 1) * ncxy;
            for (int j = 1; j <= ncxy; ++j) xy[disp2 + j - 1] = xy[disp1 + j - 1];
            ++i2;
        }
    }
}

// ---- prterx.f ------------------------------------------------------------
// The irregular regression's singular-design abend. Every `CALL regx11` in the
// oracle except x11mdl.f:416 is followed by
//   IF(.not.Lfatal.and.Armaer.eq.PSNGER)CALL prterx()
// and x11mdl.f:417 is that one too -- the guard is universal. Without it a
// singular x11regression design is a SILENT stop: regx11 sets Armaer and the
// caller unwinds, the run reaches its end, and the harness reports OUTCOME: OK
// on a seasonal adjustment whose calendar regression never fitted. That was the
// state of this port until now (M5_PORT_NOTES entry 73).
//
// prterx.f:28-34 names the offending column out of Colttl when Sngcol indexes a
// real column and falls back to the literal 'data' otherwise (Sngcol==Ncxy is
// the y column, which has no title). prterx.f:49-55's Prttab(LXRXMX) reprint of
// the design matrix goes to Mt1, the .out print engine this port does not
// implement at all, so it is deferred with the rest of it -- note that it prints
// Nrxy rows while regx11 fits Nspobs of them, so on the aictest=(user) path it
// renders zero rows over six live columns. Two different counts, not a
// contradiction; see entry 73.
void prterx(X13Context& ctx) {
    auto& m = ctx.model;
    std::string str;
    int nchr = 4;
    if (ctx.mdldat.sngcol < m.ncxy) {
        getstr(ctx, m.colttl.data(), m.colptr.data(), m.ncoltl, ctx.mdldat.sngcol,
               str, nchr);
        if (ctx.error.lfatal) return;
    } else {
        str = "data";
    }
    errhdr(ctx);
    const std::string col = str.substr(0, static_cast<std::size_t>(nchr));
    auto& err = ctx.channels_.unit(stdio::STDERR);
    auto& mt2 = ctx.channels_.unit(ctx.units.mt2);
    // prterx.f:1230 -- STDERR only, and it names the .err file rather than the
    // column. Serno is this port's Cursrs (both are the spec base name).
    err.put(fwrite_fmt("(' Error(s) found while estimating the irregular ',"
                       "'regression model.',/,"
                       "' For more details, check the error file (',a,'.err).')",
                       ctx.title.serno.str()) + "\n");
    // prterx.f:1270 -- written to Mt1 AND Mt2; only the Mt2 half exists here.
    mt2.put(fwrite_fmt("(/,' ERROR: Irregular regression matrix singular ',"
                       "'because of ',a,'.',/,"
                       "'        Check irregular regression model.',/)", col));
    abend(ctx);
}

void prterx_if_singular(X13Context& ctx) {
    if (!ctx.error.lfatal && ctx.mdldat.armaer == prm::PSNGER) prterx(ctx);
}

// ---- regx11.f (reuses olsreg/resid) --------------------------------------
bool regx11(X13Context& ctx, double* aout, int* naout, int* nefout) {
    auto& md = ctx.mdldat;
    auto& m = ctx.model;
    const int nspobs = md.nspobs, ncxy = m.ncxy, nb = m.nb;
    md.nfev = 0;
    md.armaer = 0;
    double dnefob = static_cast<double>(nspobs - m.nintvl);
    int nrtxy = nspobs;
    int neltxy = nspobs * ncxy;
    std::vector<double> txy(neltxy > 0 ? neltxy : 1, 0.0);
    for (int i = 0; i < neltxy; ++i) txy[i] = md.xy(i + 1);
    if (ctx.xclude.nxcld > 0) {
        dlrgrw(txy.data(), ncxy, nspobs, ctx.xclude.rgxcld.data());
        nrtxy -= ctx.xclude.nxcld;
        dnefob -= ctx.xclude.nxcld;
    }
    if (nb > 0) {
        olsreg(ctx, txy.data(), nrtxy, ncxy, ncxy, md.b.data(), md.chlxpx.data(),
               PXPX, md.sngcol);
        if (ctx.error.lfatal) return false;
        if (md.sngcol > 0) { md.convrg = false; md.armaer = prm::PSNGER; return false; }
        md.nfev += ncxy + 1;
    }
    md.convrg = true;
    std::vector<double> a(nrtxy > 0 ? nrtxy : 1, 0.0);
    resid(ctx, txy.data(), nrtxy, ncxy, ncxy, 1, nb, -1.0, md.b.data(), a.data());
    if (ctx.error.lfatal) return false;
    if (aout) {
        for (int i = 0; i < nrtxy; ++i) aout[i] = a[i];
        if (naout) *naout = nrtxy;
        if (nefout) *nefout = static_cast<int>(dnefob);
    }
    double apa = 0.0;
    for (int i = 0; i < nrtxy; ++i) apa += a[i] * a[i];
    // regx11.f:88-94 -- ML variance + Gaussian log-likelihood (xrlkhd reads these
    // for the AICC). Oracle PI is the 15-digit-truncated 3.14159265358979D0.
    md.var = apa / dnefob;
    if (md.var < 2.0 * std::numeric_limits<double>::epsilon()) md.var = 0.0;
    if (dpeq(md.var, 0.0)) {
        md.lnlkhd = 0.0;
    } else {
        constexpr double PI = 3.14159265358979;
        md.lnlkhd = -(dnefob * (std::log(2.0 * PI * md.var) + 1.0)) / 2.0;
    }
    return true;
}

// ---- x11ref.f (mult path, TD + holiday) ----------------------------------
// Builds the TD factor Ftd and the combined calendar factor Fcal from the fitted
// coefficients. Holiday (Easter etc.) columns accumulate into a local Fhol that
// is folded into Fcal via mulref (x11ref.f:87-95, the mult Tdgrp>0 branch); with
// no holiday column Fhol stays 0 and Fcal is TD-only (the pritd / bare-TD case).
// The Bell-Hilmer nonlinear-Easter Kvec path (Xhlnln) is not reached here (the
// x11regression easter regressor is linear, Xhlnln=F).
//
// Tdgrp/Stdgrp/Holgrp are PARAMETERS, not the COMMON: pritd.f:44 passes the
// literals 1/0/0 (it is building a prior-TD factor out of six day-of-week
// contrasts and has no regression groups at all), while x11mdl.f:694/814 pass
// the live values. Reading ctx here would give pritd whatever the irregular
// regression happened to leave behind.
void x11ref_td(X13Context& ctx, double* fcal, double* ftd, int xdev, int nrxy,
               int ncxy, const double* b, const double* xy, int nb,
               const int* rtype, int kswv, int tdgrp, int stdgrp, int holgrp) {
    for (int i = 0; i < nrxy; ++i) { fcal[i] = 0.0; ftd[i] = 0.0; }
    std::vector<double> fhol(nrxy > 0 ? nrxy : 1, 0.0);
    // x11ref.f:123-129 -- with a holiday group AND forcecal=yes the combined
    // factor is the PRODUCT Ftd*Fhol rather than the mulref accumulation below.
    // (The other arm, the Bell-Hilmer nonlinear-Easter Kvec divide, needs
    // Xhlnln, which no x11regression holiday regressor sets.) Reachable only
    // since forcecal= started being honoured.
    if (ctx.x11log.calfrc) {
        for (int icol = 1; icol <= nb; ++icol) {
            if (is_hol_type(rtype[icol - 1])) {
                x11reg_not_ported(ctx, "x11ref forcecal= combined calendar factor");
                return;
            }
        }
    }
    const double* xn = ctx.xtdtyp.xn.data();
    const double* xnstar = ctx.xtdtyp.xnstar.data();
    // Raw factors: Ftd/Fhol += B(icol) * Xy(:,icol) (column icol, stride Ncxy).
    for (int icol = 1; icol <= nb; ++icol) {
        if (is_td_type(rtype[icol - 1]))
            daxpy(nrxy, b[icol - 1], xy + (icol - 1), ncxy, ftd, 1);
        else if (is_hol_type(rtype[icol - 1]))
            daxpy(nrxy, b[icol - 1], xy + (icol - 1), ncxy, fhol.data(), 1);
    }
    // x11ref.f:76-86 -- mean-normalize the TD factor by Xnstar. GUARDED on
    // Tdgrp: with no trading-day group Ftd is all zero and the oracle skips the
    // pair entirely.
    if (tdgrp > 0) {
        mulref(nrxy, fcal, ftd, xdev, xnstar, prm::DNOTST, false);
        mulref(nrxy, ftd, ftd, xdev, xnstar, prm::DNOTST, true);
    }
    // x11ref.f:87-95 -- fold the holiday factor into Fcal. Two different arms,
    // and which one you get turns on Tdgrp:
    //
    //   IF((Muladd.eq.2.or.Trumlt).and.Tdgrp.gt.0)  -> normalize by Xnstar
    //   ELSE                                        -> divide by ONE (i.e. add
    //                                                  Fhol to Fcal unscaled)
    //
    // `Trumlt` there is a CENSUS DEFECT, now filed as CB-42: x11ref.f:19 declares
    // it LOGICAL, it is not a dummy argument and not in any COMMON, and nothing
    // ever assigns it -- line 88 reads an uninitialized local. MEASURED
    // DIRECTLY as of 2026-08-09, not inferred from gates: an instrumented build
    // of the vendored sources prints `Trumlt= T` at :87 on
    // extra/airline_x11regression-aictest-easter8, which is the arm this port
    // takes. One build's stack value, not a language guarantee; the gates pin
    // it. With Tdgrp==0 the condition is false whatever Trumlt holds, so the
    // defect cannot reach the no-TD path at all.
    // The `IF(Holgrp.gt.0)` OUTER guard is now reproduced. It was NOT, from
    // entry 76 until 2026-08-09, and the note here said adding it cost 54 gates
    // -- so "something restores Holgrp that is not visible in x11aic.f".
    //
    // It was visible, four lines below an `addeas` this port had ported:
    // `x11aic.f:318-322` re-locates the Easter group after every candidate past
    // the first and adopts it as Holgrp when no other holiday group survived.
    // With that writer in place, an instrumented build of the vendored sources
    // reports `Holgrp=2, Tdgrp=1, Trumlt=T` at x11ref.f:87 on
    // extra/airline_x11regression-aictest-easter8 -- the guard is TRUE there,
    // which is why folding unconditionally measured correct. The 54 gates were
    // the port's own missing writer, not a defect in the Fortran.
    if (holgrp > 0) {
        if (tdgrp > 0) {
            mulref(nrxy, fcal, fhol.data(), xdev, xnstar, prm::DNOTST, false);
            mulref(nrxy, fhol.data(), fhol.data(), xdev, xnstar, prm::DNOTST,
                   true);
        } else {
            mulref(nrxy, fcal, fhol.data(), xdev, xnstar, 1.0, false);
        }
    }
    // x11ref.f:99-135, the Muladd==0 arm (psuadd and log-additive are walled
    // upstream). x11ref.f:116-122 -- Kswv==3 (a tdprior prior TD has already
    // removed the day-count effect) adds ONE instead of the Xn/Xnstar
    // month-length ratio. Note the Kswv=4 recompute at x11mdl.f:813 deliberately
    // does NOT: it passes 4 precisely so the combined-weight factor is built the
    // ordinary way.
    //
    // The Tdgrp==0 arm (x11ref.f:133-135) adds ONE to Fcal and leaves Ftd alone
    // unless there is a STOCK trading-day group. Adding Xn/Xnstar there instead
    // -- which this port did, because it only had the Tdgrp>0 arm -- puts the
    // month-length ratio into a factor that is supposed to be holiday-only: a
    // holiday-only x11regression came back with February off by 28/28.25 in B1.
    for (int irow = 1; irow <= nrxy; ++irow) {
        const int ir2 = irow + xdev - 1;
        fhol[irow - 1] += 1.0;                       // x11ref.f:101
        if (tdgrp > 0) {
            const double add = (kswv == 3) ? 1.0 : xn[ir2 - 1] / xnstar[ir2 - 1];
            ftd[irow - 1] += add;
            fcal[irow - 1] += add;
            // (x11ref.f:124-131's Xhlnln / Calfrc sub-arms are unreachable here:
            // no x11regression holiday regressor sets Xhlnln, and Calfrc is
            // walled above.)
        } else {
            fcal[irow - 1] += 1.0;
            if (stdgrp > 0) ftd[irow - 1] += 1.0;
        }
    }
}

// ---- pritd.f (Kswv=1 user-weight prior trading day, mult/log-add path) ----
// Prior trading-day adjustment factors from the seven user weights (the
// x11regression tdprior argument). Reuses td6var (the six day-of-week contrast
// regressors) + x11ref_td (the multiplicative TD factor build). The caller must
// have populated xtdtyp (Xn/Xnstar) via tdset_td over the factor span first
// (editor.f:2240). Ptdfac is filled at absolute positions [Frstob, Frstob+Nrxy-1].
void pritd(X13Context& ctx, double* ptdfac, int nrxy, int sp, const int* begdat,
           int frstob) {
    const int ncxy = 6;
    // setlg(T,PLEN,begrgm): every row is in-regime (no change-of-regime).
    std::unique_ptr<bool[]> begrgm(new bool[PLEN]);
    std::fill(begrgm.get(), begrgm.get() + PLEN, true);
    const bool* begrgm_p = begrgm.get();
    // Pridat = the calendar date at the first factor row (absolute pos Frstob).
    int pridat[2];
    if (frstob > 1) addate(begdat, sp, frstob - 1, pridat);
    else { pridat[0] = begdat[0]; pridat[1] = begdat[1]; }
    // Six raw day-of-week trading-day contrasts.
    std::vector<double> tdxy(static_cast<std::size_t>(PLEN) * ncxy, 0.0);
    td6var(ctx, pridat, sp, nrxy, ncxy, 1, ncxy, 0, tdxy.data(), begrgm_p, false);
    if (ctx.error.lfatal) return;
    // X-11 style weights -> regression coefficients (Dwt-1), all trading-day type.
    double btd[6];
    int rtype[6];
    for (int i = 0; i < 6; ++i) {
        btd[i] = ctx.x11reg.dwt(i + 1) - 1.0;
        rtype[i] = prm::PRGTTD;
    }
    // Build the factor over rows [1,Nrxy] (Ftd is the prior-TD factor), reading
    // Xn/Xnstar at absolute [Frstob, Frstob+Nrxy-1] via Xdev=Frstob.
    std::vector<double> fcal(nrxy > 0 ? nrxy : 1, 0.0);
    std::vector<double> ftd(nrxy > 0 ? nrxy : 1, 0.0);
    x11ref_td(ctx, fcal.data(), ftd.data(), frstob, nrxy, ncxy, btd, tdxy.data(),
              6, rtype, ctx.x11opt.kswv,
              // pritd.f:44 passes Tdgrp=1, Stdgrp=0, Holgrp=0 as literals.
              /*tdgrp=*/1, /*stdgrp=*/0, /*holgrp=*/0);
    // pritd.f:47-51 -- shift the [1,Nrxy] factors up to absolute [Frstob, ...].
    for (int i = 0; i < nrxy; ++i) ptdfac[frstob - 1 + i] = ftd[i];
}

// ---- x11aic.f -------------------------------------------------------------
// Automatic AICC keep/drop tests on the X-11 irregular regression (x11mdl.f:253,
// B iteration only): trading day first (:148-295), then Easter (:299-458). Each
// test rebuilds the design (addtd / addeas -> regvar), refits the OLS (regx11),
// and scores the AICC (xrlkhd + the mult Jacobian -2*jadj when TD is present);
// the winner is LEFT IN THE MODEL so the downstream regvar/factor build picks it
// up, and x11mdl.f then reads the verdict back off the model rather than off any
// stored flag.
//
// The USER-defined branch (:462-591) is NOT ported -- the parser refuses
// `aictest=(user)` rather than accept the token and silently run the other two.
//
// Ordering note for the two tests together: when trading day is ACCEPTED,
// `estend` goes false and the Easter loop's first iteration (:327) skips its
// re-estimation entirely, reusing `aichol = aictd` from :244. So on a spec with
// both tests `aictest.xe.aicc.noeaster` is bit-identical to
// `aictest.xtd.aicc.td` -- that is the Fortran's arithmetic, not a coincidence.
void x11aic(X13Context& ctx, double* trnsrs, const double* sti, int nobspf,
            int nfcst, int irridx, int irrend, int muladd, bool trumlt) {
    auto& m = ctx.model;
    auto& ar = ctx.arima;
    auto& xr = ctx.x11reg;
    const auto& xc = ctx.xclude;
    const int easidx = m.easidx;
    const int sp = m.sp;
    const double xraicd = ctx.xrgmdl.xraicd;
    const int neasvx = xr.neasvx;
    const bool jac = trumlt || muladd == 2;
    const int xtdtst = xr.xtdtst;
    const bool xeastr = ctx.x11log.xeastr;

    // x11aic.f:56 `aicind=-1`. `aicind` is NOT in that routine's declaration
    // list, so Fortran case-insensitivity resolves it to the COMMON `Aicind` of
    // arima.cmn -- the same slot easaic.f writes for the regARIMA Easter test.
    // Entering x11aic therefore CLOBBERS the regARIMA window unconditionally,
    // before either test runs, and x11mdl.f:280 reads that same slot back for
    // `aictest.xe.window`. Both halves are reproduced: the -1 lands here (and
    // is what a TD-only call leaves behind), the winner is written at the end.
    ar.aicind = -1;

    // x11aic.f:57 -- false once either test has left its own design in place,
    // which is what tells the Easter loop and the epilogue not to re-estimate.
    bool estend = true;

    // x11aic.f:58-65 -- clear the group pointers the tests are about to
    // re-derive, before the columns themselves come out below.
    if (xtdtst > 0) {
        if (xr.tdgrp > 0) xr.tdgrp = 0;
        if (xr.stdgrp > 0) xr.stdgrp = 0;
    }
    if (xeastr) {
        if (xr.holgrp > 0) xr.holgrp = 0;
        if (xr.easgrp > 0) xr.easgrp = 0;
    }
    // x11aic.f:66-75 -- the user test only exists if there ARE user columns;
    // with none, Xuser is switched off for the rest of the routine. Ncusrx/
    // Haveum are dropped for the duration so the no-user regvar cannot rebuild
    // the columns, and put back at :493-494.
    bool xuser = ctx.x11log.xuser;
    int ncx2 = 0;
    bool lhum2 = false;
    if (xuser) {
        if (ctx.usrreg.ncusrx > 0) {
            ncx2 = ctx.usrreg.ncusrx;
            ctx.usrreg.ncusrx = 0;
            lhum2 = ctx.xrgum.haveum;
            ctx.xrgum.haveum = false;
        } else {
            xuser = false;
        }
    }

    // x11aic.f:79-82 -- Xtdtst is the XAICDC token index (td / tdstock /
    // td1coef / tdstock1coef = 1/2/3/4); addtd.f and mktdlb.f take the WIDER
    // regressor index, which also numbers the two `nolpyear` variants. The
    // three tests are sequential and each value passes through at most one.
    int tdindx = xtdtst;
    if (tdindx == 4) tdindx = 6;
    if (tdindx == 3) tdindx = 4;
    if (tdindx == 2) tdindx = 3;

    // AICC Jacobian adjustment (mult / log-add): sum log(Xnstar) over the rows
    // not excluded by tdxtrm (x11aic.f:87-93).
    double jadj = 0.0, jadj2 = 0.0;
    if (jac) {
        const double* xnstar = ctx.xtdtyp.xnstar.data();
        for (int i = irridx; i <= irrend; ++i) {
            bool ladj = true;
            if (xc.nxcld > 0) ladj = !xc.rgxcld(i - irridx + 1);
            if (ladj) jadj += std::log(xnstar[i - 1]);
        }
        // x11aic.f:98-105, log-additive only, and note the excluded/included
        // sense is INVERTED against jadj above: `ladj = Rgxcld(...)`, not its
        // negation. Transcribed as written. No corpus spec reaches it
        // (x11regression under mode=logadd), so it is unmeasured.
        if (muladd == 2) {
            for (int i = irridx; i <= irrend; ++i) {
                bool ladj = true;
                if (xc.nxcld > 0) ladj = xc.rgxcld(i - irridx + 1);
                if (ladj) jadj2 += sti[i - 1];
            }
        }
    }

    // Re-copy the irregular out of Sti and re-transform it (x11aic.f:157-164,
    // and again at :191-198 / :279-286). Every one of those sites is guarded by
    // `(Muladd.eq.0.or.Muladd.eq.2).or.Haveum`; Haveum is the user-mean flag,
    // which this port never sets.
    auto retransform = [&]() {
        if (muladd == 0 || muladd == 2) {
            for (int i = 0; i < nobspf; ++i) trnsrs[i] = sti[(irridx - 1) + i];
            xrgtrn_td(ctx, trnsrs, irridx, irrend, xr.tdgrp);
        }
    };
    auto rebuild_design = [&](bool xm = true) {
        int nrxy = 0, frstry = 0;
        regvar(ctx, trnsrs, nobspf, ar.fctdrp, nfcst, 0, ar.userx.data(),
               ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj, ar.reglom, nrxy,
               ar.begxy.data(), frstry, xm, ar.elong);
        if (ctx.error.lfatal) return;
        ar.nrxy = nrxy;
    };
    // x11aic.f:328-329 / :449-450 -- the Easter-branch regvar calls pass
    // `xm = Easidx.eq.0 .and. .not.(Trumlt.and.tdhol.and.Xhlnln)`. Xhlnln is
    // the Bell-Hilmer nonlinear Easter, which this port never sets, so the
    // second conjunct is always true and `xm` is just Easidx==0. The
    // trading-day branch's three regvar calls pass a literal T.
    const bool eas_xm = (easidx == 0);

    // x11aic.f:112-143 -- strip the regressors the requested tests are about to
    // re-derive. `icol` starts at the ENTRY Nb and counts down independently of
    // the deletions, so the bound is captured once.
    //
    // The Xuser arm stashes B/Regfx/Rgvrtp per stripped column for the restore
    // at :496-521. TWO transcription hazards here, both reproduced:
    //
    //  - THE READ IS AFTER THE DELETE, and it is UNREACHABLE -- measured, not
    //    assumed (docs/M5_PORT_NOTES.md entry 62). x11aic.f:129 calls dlrgef
    //    and only then reads `B(icol)`; dlrgef.f:75-77 would shift
    //    B/Rgvrtp/Regfx DOWN over the hole and hand back the FOLLOWING
    //    column's coefficient -- except that it copies `noldc-1-endcol`
    //    elements, i.e. ZERO when the deleted column is the last one, leaving
    //    the slot untouched. The user columns are always the TRAILING block
    //    (gtxreg.f:183 adds `variables=` inside the argument loop, :733-743
    //    appends the user columns after it, and :496-521 below re-appends them
    //    last), and this loop counts DOWN, so each one IS the last column when
    //    it is deleted. Keep the two statements in this order anyway.
    //  - INDEX ORDER. This loop counts DOWN, so bu2/fx2/typ2 fill in
    //    descending-column order, while the restore reads them 1..Ncusrx
    //    alongside `getstr(Usrttl, ..., i)` in ASCENDING title order. With two
    //    or more user columns the two disagree and coefficients are paired
    //    with the wrong titles. Real, and reproduced -- but the two-column
    //    spec that shows it also trips CB-36 one stage earlier, in editor, and
    //    that route is walled, so this arm is not gated on its own.
    const int nb0 = m.nb;
    int iuser = 0;
    double bu2[prm::PUREG] = {0.0};
    bool fx2[prm::PUREG] = {false};
    int typ2[prm::PUREG] = {0};
    for (int icol = nb0; icol >= 1; --icol) {
        const int rtype = m.rgvrtp(icol);
        const bool istd = xtdtst > 0 && is_x11aic_td_type(rtype);
        const bool iseas =
            (rtype == prm::PRGTEA || rtype == prm::PRGTEC) && xeastr;
        const bool isusr = xuser && is_x11aic_user_type(rtype);
        if (istd || iseas || isusr) {
            dlrgef(ctx, icol, ar.nrxy, 1);
            if (ctx.error.lfatal) return;
            if (isusr && iuser < prm::PUREG) {
                bu2[iuser] = ctx.mdldat.b(icol);
                fx2[iuser] = m.regfx(icol);
                typ2[iuser] = rtype;
                ++iuser;
            }
        } else if (xeastr &&
                   (rtype == prm::PRGTLD || rtype == prm::PRGTTH)) {
            // A labor-day / Thanksgiving column survives the Easter test and
            // IS the holiday group for the AICC's mult Jacobian.
            xr.holgrp = icol;
        }
    }

    // ---- trading day (x11aic.f:148-295) ------------------------------------
    double aichol = prm::DNOTST;
    if (xtdtst > 0) {
        const std::string tdstr = mktdlb(ctx, tdindx, xr.xaicst,
                                         xr.xaicrg.data(), ctx.xrgmdl.xtdzro,
                                         sp);
        if (ctx.error.lfatal) return;

        // AICC without trading day.
        retransform();
        rebuild_design();
        if (ctx.error.lfatal) return;
        if (!regx11(ctx)) { prterx_if_singular(ctx); return; }
        double aicntd = prm::DNOTST;
        xrlkhd(ctx, aicntd, xc.nxcld);
        if (ctx.error.lfatal) return;
        if (xr.holgrp > 0 && muladd == 2) aicntd += 2.0 * jadj2;

        // Add the trading-day group back and re-fit.
        addtd(ctx, xr.xaicst, xr.xaicrg.data(), ctx.xrgmdl.xtdzro, sp, tdindx);
        if (ctx.error.lfatal) return;
        xr.tdgrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                          "Trading Day");
        if (xr.tdgrp == 0 && xtdtst == 2)
            xr.stdgrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                               m.ngrptl, "Stock Trading Day");
        // x11aic.f:186-187 -- ADDITIVE only. On the mult/log-add paths the
        // leap-year variation rides in the Xnstar day-count normalization
        // (gtxreg.f:186-192), so no Leap Year column is wanted.
        if (muladd == 1 && xtdtst == 1)
            adrgef(ctx, prm::DNOTST, "Leap Year", "Leap Year", prm::PRGTLY,
                   false, false);
        if (ctx.error.lfatal) return;
        retransform();
        rebuild_design();
        if (ctx.error.lfatal) return;
        if (!regx11(ctx)) { prterx_if_singular(ctx); return; }
        // (x11aic.f:208's rgtdhl is a no-op here: it returns unless Xhlnln,
        // the Bell-Hilmer nonlinear Easter, which this port never sets.)
        double aictd = prm::DNOTST;
        xrlkhd(ctx, aictd, xc.nxcld);
        if (ctx.error.lfatal) return;
        if (jac) {
            aictd -= 2.0 * jadj;
            if (muladd == 2) aictd += 2.0 * jadj2;
        }

        ctx.x11reg_aicc_xtd_notd = aicntd;
        ctx.x11reg_aicc_xtd_td = aictd;
        ctx.x11reg_xtd_reg = tdstr;

        if (aictd + xraicd < aicntd) {
            if (!ctx.x11log.axrgtd && xr.ixrgtd > 0) ctx.x11log.axrgtd = true;
            estend = false;
            // x11aic.f:243-247. Both arms test Xeastr, so the second is dead
            // and `aicnus` -- the user branch's no-user AICC -- is never seeded
            // here; from the surrounding code the intent was plainly
            // `ELSE IF(Xuser)`. Not claimed as a CB entry: the user branch is
            // unported, so the port cannot measure the difference. Transcribed
            // as written, dead arm and all.
            if (xeastr) {
                aichol = aictd;
            } else if (xeastr) {
                /* aicnus = aictd -- UNREACHABLE. This is what leaves aicnus
                   uninitialized at :481 whenever TD is accepted with no
                   Easter; see the comment on `aicnus` below. */
            }
        } else {
            if (ctx.x11log.axrgtd) ctx.x11log.axrgtd = false;
            if (ctx.x11log.havxtd) ctx.x11log.havxtd = false;
            xr.tdgrp = 0;
            xr.stdgrp = 0;
            // x11aic.f:257-274 -- take the trading-day columns back out.
            const int nb1 = m.nb;
            for (int icol = nb1; icol >= 1; --icol) {
                if (is_x11aic_td_type(m.rgvrtp(icol))) {
                    dlrgef(ctx, icol, ar.nrxy, 1);
                    if (ctx.error.lfatal) return;
                }
            }
            // x11aic.f:278-293 -- only rebuild if something is left to test or
            // to carry. Note Ncusrx is 0 for the whole routine when Xuser is on
            // (:68), so the `.or.Ncusrx.gt.0` half only fires for user columns
            // present WITHOUT an aictest=(user) request.
            if (xeastr || xr.holgrp > 0 || xuser || ctx.usrreg.ncusrx > 0) {
                retransform();
                rebuild_design();
                if (ctx.error.lfatal) return;
            }
        }
        ctx.x11reg_xtd_ran = true;
    }

    auto find_easter = [&]() -> int {
        int eg = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                        "Easter");
        if (eg == 0)
            eg = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
                        "StatCanEaster");
        return eg;
    };
    auto del_easter = [&]() {
        // x11aic.f:304-311 and :427-433 both write the COMMON `Easgrp` here,
        // not a local -- the search result outlives the delete and x11mdl.f:141
        // reads it.
        xr.easgrp = find_easter();
        const int eg = xr.easgrp;
        if (eg > 0) {
            const int begcol = m.grp(eg - 1);
            const int ncol = m.grp(eg) - begcol;
            dlrgef(ctx, begcol, ar.nrxy, ncol);
        }
    };

    // ---- easter (x11aic.f:299-458) -----------------------------------------
    // aicnus -- the AICC of the model WITHOUT the user-defined regressors. The
    // Fortran declares it and never initializes it, and there are exactly three
    // ways it can hold a value at :481:
    //   :470  computed, when estend is still true at :463;
    //   :457  seeded from the winning Easter aichol, when the Easter test ran
    //         and left its own design fitted;
    //   never -- TD accepted with no Easter test. :245's `ELSE IF(Xeastr)` is
    //         the arm that was meant to seed it and cannot fire.
    // The third case is a live Census defect and it DECIDES the test: the
    // vendored -O2 oracle reads 0.0 out of the uninitialized slot, so
    // `aicusr + Xraicd < aicnus` holds for any negative AICC and the user
    // regressors are accepted unconditionally. Measured on
    // extra/airline_x11regression-aictest-tduser, and the 0.0 survives a
    // changed ARIMA model. Reproduced by initializing to 0.0 -- with the
    // caveat that this is one Fortran build's stack value, not a language
    // guarantee; the gate is what pins it.
    double aicnus = 0.0;

    if (xeastr) {

    double aicbst = prm::DNOTST;
    int aicind = ar.aicind;   // set to -1 at entry; see the note there
    // A span/history replay is a full x11pt1->x11pt3 pass and re-enters here,
    // so the table must be rebuilt, not appended to. (The oracle cannot hit
    // this: x11mdl.f:291 sets Xeastr=F after the first test. This port leaves
    // the flag live -- the C iteration is fenced off by the kpart==2 gate in
    // x11mdl_td instead -- so the rebuild guard belongs here.)
    ctx.x11reg_aicc_xe.clear();
    for (int i = 1; i <= neasvx; ++i) {
        if (i > 2) { del_easter(); if (ctx.error.lfatal) return; }
        if (i > 1) {
            addeas(ctx, ctx.x11reg.xeasvc(i) + easidx, easidx, 1);
            if (ctx.error.lfatal) return;
            // x11aic.f:318-322 -- THE MISSING WRITER. `:63` cleared Holgrp on
            // the way in, and this is what puts it back: every candidate after
            // the first re-locates its own Easter group and, if no other
            // holiday group survived, adopts it. Absent, Holgrp stayed 0 for
            // the rest of the run, which is what made x11ref.f:87's outer guard
            // look like a Census defect (entry 76's open question -- "something
            // restores Holgrp that is not visible in x11aic.f"; it is visible,
            // four lines below the addeas this port did port).
            xr.easgrp = find_easter();
            if (xr.holgrp == 0) xr.holgrp = xr.easgrp;
        }
        // x11aic.f:327 -- with `estend` false the trading-day test has already
        // left exactly this design fitted, and its `aictd` was copied into
        // aichol at :244. Skipping the refit here is what makes
        // `aictest.xe.aicc.noeaster == aictest.xtd.aicc.td` on a two-test spec.
        if (i > 1 || estend) {
            rebuild_design(eas_xm);
            if (ctx.error.lfatal) return;
            if (!regx11(ctx)) { prterx_if_singular(ctx); return; }
            aichol = prm::DNOTST;
            xrlkhd(ctx, aichol, xc.nxcld);
            // x11aic.f:345-350 -- the Jacobian is the TD one whenever a TD
            // group survived, and the holiday one otherwise.
            if (xr.tdgrp > 0 && jac) {
                aichol -= 2.0 * jadj;
                if (muladd == 2) aichol += 2.0 * jadj2;
            } else if (xr.holgrp > 0 && muladd == 2) {
                aichol += 2.0 * jadj2;
            }
        }
        ctx.x11reg_aicc_xe.push_back({ctx.x11reg.xeasvc(i), aichol});
        if (i == 1) {
            aicbst = aichol;
            aicind = ctx.x11reg.xeasvc(1);   // 0
        } else if ((aicind == 0 && aichol + xraicd < aicbst) ||
                   (aicind > 0 && aichol < aicbst)) {
            aicbst = aichol;
            aicind = ctx.x11reg.xeasvc(i);
        }
    }

    // Leave the winning model in place (x11aic.f:424-447). If the winner is the
    // last window tested it is already current; otherwise re-select it.
    estend = false;
    if (aicind < ctx.x11reg.xeasvc(neasvx)) {
        estend = true;
        del_easter();
        if (ctx.error.lfatal) return;
        if (aicind == 0) {
            // x11aic.f:438-441 -- with no Easter left, the holiday group is
            // whatever OTHER holiday column is still in the design.
            xr.easgrp = 0;
            xr.holgrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                               m.ngrptl, "Thanksgiving");
            if (xr.holgrp == 0)
                xr.holgrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                                   m.ngrptl, "Labor");
        } else {
            addeas(ctx, aicind + easidx, easidx, 1);
        }
        if (ctx.error.lfatal) return;
        rebuild_design(eas_xm);
        if (ctx.error.lfatal) return;
    }
    ar.aicind = aicind;
    ctx.x11reg_xe_window = aicind;
    ctx.x11reg_xe_easidx = easidx;
    // x11mdl.f:271-292 -- the accept/reject verdict is taken off the MODEL,
    // not off aicind: x11aic has already left the winning design in place, so
    // the caller just asks whether an Easter group survived. Same two-step
    // lookup as find_easter above.
    ctx.x11reg_xe_accepted = find_easter() > 0;
    ctx.x11reg_xe_ran = true;

    // x11aic.f:457 -- the LAST line of the Xeastr block, and the second of the
    // three aicnus provenances above.
    if (!estend && xuser) aicnus = aichol;

    }   // end of the Xeastr block (x11aic.f:298-458)

    // ---- user-defined (x11aic.f:462-591) -----------------------------------
    if (!xuser) return;

    // x11aic.f:463-479 -- the no-user AICC, only when nothing upstream has
    // already left a fitted design behind.
    if (estend) {
        if (!regx11(ctx)) { prterx_if_singular(ctx); return; }
        // (:466's rgtdhl is the same Xhlnln no-op as in the TD branch.)
        xrlkhd(ctx, aicnus, xc.nxcld);
        if (ctx.error.lfatal) return;
        if (xr.tdgrp > 0 && jac) {
            aicnus -= 2.0 * jadj;
            if (muladd == 2) aicnus += 2.0 * jadj2;
        } else if (xr.holgrp > 0 && muladd == 2) {
            aicnus += 2.0 * jadj2;
        }
    }
    ctx.x11reg_aicc_xu_nouser = aicnus;

    // x11aic.f:493-521 -- put the user columns back. Ncusrx/Haveum first (the
    // titles are read out of Usrttl, which needs the count), then one adrgef per
    // column dispatched on the SAVED Rgvrtp. Seven arms; the parse-side dispatch
    // in gt_x11regression is four, which is not a discrepancy -- the parser maps
    // four usertype= tokens, this restores whatever types ended up in the model.
    ctx.usrreg.ncusrx = ncx2;
    ctx.xrgum.haveum = lhum2;
    for (int i = 1; i <= ctx.usrreg.ncusrx; ++i) {
        std::string effttl;
        int nchr = 0;
        getstr(ctx, ctx.usrreg.usrttl.data(), ctx.usrreg.usrptr.data(),
               ctx.usrreg.ncusrx, i, effttl, nchr);
        if (ctx.error.lfatal) return;
        const std::string ttl = effttl.substr(0, static_cast<std::size_t>(nchr));
        const int t = typ2[i - 1];
        const char* grp = nullptr;
        if (t == prm::PRGTUD)      grp = "User-defined";
        else if (t == prm::PRGUTD) grp = "User-defined Trading Day";
        else if (t == prm::PRGULY) grp = "User-defined Leap Year";
        else if (t == prm::PRGULM) grp = "User-defined LOM";
        else if (t == prm::PRGULQ) grp = "User-defined LOQ";
        else if (t == prm::PRGUAO) grp = "User-defined AO";
        else if (t == prm::PRGTUH) grp = "User-defined Holiday";
        // No ELSE in the Fortran either: an unrecognised saved type restores
        // nothing, silently dropping the column.
        if (grp != nullptr)
            adrgef(ctx, bu2[i - 1], ttl, grp, t, fx2[i - 1], false);
        if (ctx.error.lfatal) return;
    }

    // x11aic.f:526-533 -- the Haveum retransform. Haveum is the user-MEAN flag
    // (x11regression umdata=), which this port never sets, so xrgtrn's Haveum
    // arm stays unported; guard it rather than pretend.
    if (ctx.xrgum.haveum) {
        x11reg_not_ported(ctx, "x11aic user branch with umdata= (Haveum)");
        return;
    }

    // x11aic.f:537-552 -- refit WITH the user columns and score.
    rebuild_design();
    if (ctx.error.lfatal) return;
    if (!regx11(ctx)) { prterx_if_singular(ctx); return; }
    double aicusr = prm::DNOTST;
    xrlkhd(ctx, aicusr, xc.nxcld);
    if (ctx.error.lfatal) return;
    if (xr.tdgrp > 0 && jac) {
        aicusr -= 2.0 * jadj;
        if (muladd == 2) aicusr += 2.0 * jadj2;
    } else if (xr.holgrp > 0 && muladd == 2) {
        aicusr += 2.0 * jadj2;
    }
    ctx.x11reg_aicc_xu_user = aicusr;

    // x11aic.f:557-591 -- the verdict.
    if (aicusr + xraicd < aicnus) {
        estend = false;
    } else {
        estend = true;
        // Haveum retransform (:566-572) -- walled above.
        // :573-579 deletes ONLY the literal group 'User-defined', though the
        // restore above can have created six other titles. Whether that strands
        // columns depends on adrgef's grouping, which no corpus spec exercises
        // (every user column here is the default PRGTUD). Transcribed as
        // written; flagged, not claimed.
        const int igrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                                m.ngrptl, "User-defined");
        // NB the Fortran indexes Grp(igrp-1) with NO igrp>0 guard, so a model
        // whose restored columns carry only the other six titles would read
        // Grp(-1). Guarded here: reproducing an out-of-bounds read is not
        // reproducing a behaviour.
        if (igrp > 0) {
            const int begcol = m.grp(igrp - 1);
            const int ncol = m.grp(igrp) - begcol;
            dlrgef(ctx, begcol, ar.nrxy, ncol);
            if (ctx.error.lfatal) return;
        }
        ctx.usrreg.ncusrx = 0;
        ctx.usrxrg.ncxusx = 0;
        ctx.usrxrg.nrxusx = 0;
        rebuild_design();
        if (ctx.error.lfatal) return;
    }
    // (x11aic.f:594-598's `IF(estend) regx11` epilogue is subsumed: x11mdl_td
    // rebuilds the design and re-fits unconditionally on return, exactly as
    // x11mdl.f:308-420 does.)
    (void)estend;

    // x11mdl.f:293-305 -- the verdict is read back off the MODEL, not off a
    // stored flag, the same way the Easter one is.
    ctx.x11reg_xu_accepted =
        strinx(true, m.grpttl.raw(), m.grpptr.data(), 1, m.ngrptl,
               "User-defined") > 0;
    ctx.x11reg_xu_ran = true;
}

// ---- x11mdl.f orchestration (TD-only mult path) --------------------------
void x11mdl_td(X13Context& ctx, int kpart) {
    auto& md = ctx.mdldat;
    auto& m = ctx.model;
    auto& ar = ctx.arima;
    const int sp = m.sp;
    const int pos1ob = ctx.x11ptr.pos1ob, posfob = ctx.x11ptr.posfob;
    int pos1bk = ctx.x11ptr.pos1bk, posffc = ctx.x11ptr.posffc;
    const int muladd = ctx.x11opt.muladd;
    double* sti = ctx.x11srs.sti.data();

    // x11mdl.f:210 `IF(Sigxrg.gt.ZERO)`. The CHOICE between the 2.5-sigma
    // tdxtrm clip and automatic AO identification is not made here: editor.f
    // :1727-1747 makes it once at spec-read, off the parsed x11reg model,
    // and leaves either Sigxrg=2.5 or Otlxrg=T behind (xrg_editor_setup in
    // readers_spec.cpp). Deciding it here instead was wrong -- it could not see
    // the user columns' contribution to Tdgrp/Holgrp, and the CB-36 stale-rtype
    // route in particular.
    const double sigxrg = dpeq(ctx.x11reg.sigxrg, prm::DNOTST)
                              ? 0.0
                              : ctx.x11reg.sigxrg;
    int nfcst = ctx.extend.nfcst;
    // x11mdl.f:120-134: on the final (C) iteration, restore the X-11-regression
    // forecast horizon (Nfcstx >= 1 seasonal year) so the design & TD factor span
    // the forecast region. The OLS estimate itself still uses only the Nspobs data
    // rows (regx11) and mulref normalizes per-row (own Xnstar), so the observed
    // factor is UNCHANGED -- this only appends the forecast-region factor (c16.A),
    // which the main run's x11pt3 D11/D16 fold needs over [Pos1bk,Posffc].
    if (kpart == 3 && (ctx.xrgfct.nfcstx > 0 || ctx.xrgfct.nbcstx > 0)) {
        nfcst = ctx.xrgfct.nfcstx;
        posffc = posfob + nfcst;
        pos1bk = pos1ob - ctx.xrgfct.nbcstx;
        // ...and x11mdl.f:126-137 writes the COMMONs too, not just this
        // routine's view of them: Nofpob/Nbfpob come off the CURRENT Nspobs,
        // which on the Xdsp route is still the NARROW one the B iteration left,
        // and xrgdrv.f:166's restore is conditional -- so that narrow Nofpob is
        // what the oracle's main run inherits.
        //
        // Scoped to Ixreg>=2, i.e. to the xrgdrv TRANSPARENT pass. The oracle's
        // write is unconditional, but with Ixreg==1 the ordinary irregular
        // regression runs x11mdl from the MAIN x11pt2, where this port already
        // carries the equivalent state by other means -- so the scope is about
        // which x11mdl call site each port reaches the write from, not about
        // the write itself.
        if (ctx.hiddn.ixreg >= 2) {
            ctx.extend.nfcst = nfcst;
            ctx.extend.nbcst = ctx.xrgfct.nbcstx;
            ctx.x11ptr.pos1bk = pos1bk;
            ctx.x11ptr.posffc = posffc;
            ctx.extend.nofpob = md.nspobs + nfcst;
            ctx.extend.nbfpob = md.nspobs + nfcst + ctx.xrgfct.nbcstx;
        }
    }
    // x11mdl.f:113-118 -- move Begspn/Endspn onto the IRREGULAR REGRESSION span
    // (`x11regression{span=}`, gtxreg.f:629-661). Everything from here to the
    // restore below fits on that span; the factor built afterwards spans the
    // full one, which is why the Fortran restores and rebuilds the design
    // rather than simply keeping the narrow fit.
    // ...except on the route where xrgdrv already did it: xrgdrv.f:151-158 moved
    // Endspn onto Endxrg (and pulled Posfob/Posffc back by Xdsp) before the
    // transparent pass started, leaving Nspobs alone. Endspn is not a field this
    // port maintains -- it is derived from Begspn+Nspobs-1 -- so the flag stands
    // in for it. `nend` then comes out 0, exactly as it does in the Fortran, and
    // the span arrives instead as the shortened pointers.
    int endspn_cur[2];
    if (ctx.xrg_endspn_narrow) {
        endspn_cur[0] = ctx.x11reg.endxrg(1);
        endspn_cur[1] = ctx.x11reg.endxrg(2);
    } else {
        addate(md.begspn.data(), sp, md.nspobs - 1, endspn_cur);
    }
    int nbeg = 0, nend = 0;
    dfdate(ctx.x11reg.begxrg.data(), md.begspn.data(), sp, nbeg);
    dfdate(endspn_cur, ctx.x11reg.endxrg.data(), sp, nend);
    // Everything the narrowing touches, saved for the restore. The guard makes
    // the restore unconditional on the way out: this is a full x11 COMMON, and
    // an early return leaving Begspn/Nspobs narrowed is the span-replay bug
    // this subsystem has already paid for four times (see core/src/x11/
    // CLAUDE.md). The explicit setspn below runs FIRST, before the factor
    // build, and DISARMS the guard: past that point the Fortran leaves these
    // values wherever :512-528 left them, and re-imposing the entry values would
    // be wrong. It matters on the Xdsp route -- there the B iteration legitimately
    // ends with Nspobs still narrow, and the C iteration's entry value IS that
    // narrow one, so an unconditional guard would undo the C restore.
    struct SpanGuard {
        X13Context& c;
        int begspn[2], nspobs, frstsy, nomnfy, adj1st;
        bool armed = true;
        ~SpanGuard() {
            if (!armed) return;
            c.mdldat.begspn(1) = begspn[0];
            c.mdldat.begspn(2) = begspn[1];
            c.mdldat.nspobs = nspobs;
            c.arima.frstsy = frstsy;
            c.arima.nomnfy = nomnfy;
            c.adj.adj1st = adj1st;
        }
    } span_guard{ctx, {md.begspn(1), md.begspn(2)}, md.nspobs, ar.frstsy,
                 ar.nomnfy, ctx.adj.adj1st};
    if (nbeg > 0) {
        md.begspn(1) = ctx.x11reg.begxrg(1);
        md.begspn(2) = ctx.x11reg.begxrg(2);
    }
    if (nend > 0) {
        endspn_cur[0] = ctx.x11reg.endxrg(1);
        endspn_cur[1] = ctx.x11reg.endxrg(2);
    }
    // x11mdl.f:168-175, inside the same `IF(Kpart.eq.2)` as the addtd/addeas
    // pre-build the AICC tests carry: in a sliding-spans or history replay the
    // irregular regression starts from the x11reg STORE's coefficients rather
    // than from scratch. With slidingspans{fixx11reg=} (default YES) or
    // history{fixx11reg=yes} that store holds the MAIN run's C-iteration daily
    // weights -- xrgdrv.f:206's loadxr(T) put them there -- and Irgxfx==3 then
    // has rmfix strip every column, so these seeded values ARE the applied
    // weights, not a starting guess. Without the seed a fixed design would
    // re-apply whatever B happened to hold.
    if (kpart == 2 && (ctx.hiddn.issap == 2 || ctx.hiddn.irev == 4)) {
        const sspinp_cmn& si = ctx.sspinp;
        if (si.nssfxx > 0 || ctx.rev.nrvfxr > 0 || si.ssxint || ctx.rev.revfxx) {
            copy(ctx.xrgmdl.bx.data(), prm::PB, 1, md.b.data());
        } else {
            for (int i = 1; i <= prm::PB; ++i) md.b(i) = prm::DNOTST;
        }
    }
    // x11mdl.f:186-195 -- Nspobs off the (possibly moved) endpoints, and the
    // series-relative pointers only when something actually moved.
    int nspobs = 0;
    dfdate(endspn_cur, md.begspn.data(), sp, nspobs);
    nspobs += 1;
    md.nspobs = nspobs;
    const int irridx = pos1ob + nbeg;
    // With no span the design/factors span the forecast-extended buffer
    // [pos1ob, posffc] so Factd/Faccal cover the whole [pos1bk,posffc] used by
    // the Stcsi feedback and the D-part; the OLS itself still uses only the
    // Nspobs data rows (regx11). Once the span narrows, x11mdl.f:193 is what
    // sizes the design instead, and the buffer-wide factor comes from the
    // rebuild after the restore.
    int nobspf = posffc - pos1ob + 1;
    if (nbeg > 0 || nend > 0) {
        dfdate(md.begspn.data(), ar.begsrs.data(), sp, ar.frstsy);
        ar.frstsy += 1;
        ar.nomnfy = ar.nobs - ar.frstsy + 1;
        nobspf = std::min(nspobs + std::max(nfcst - ar.fctdrp, 0), ar.nomnfy);
    }
    const int irrend = irridx + nspobs - 1;

    // Trading-day calendar quantities (tdset). The oracle calls tdset ONCE,
    // from editor.f:2240, with Begbak and the whole [Pos1bk,Posffc] buffer --
    // never per x11mdl call and never with a narrowed span. This port issues it
    // here, so it must use the span start the BUFFER is indexed from, i.e. the
    // one saved before the x11regression span narrowed Begspn. Feeding it the
    // narrowed date slides Xnstar/Xn by nbeg periods against the factor.
    // The END of that buffer has the same requirement, and it is the Xdsp route
    // that exposes it: xrgdrv pulled Posffc back by Xdsp, but the C iteration
    // rebuilds the design over the FULL span and x11ref indexes Xnstar by row.
    // Stopping tdset at the shortened Posffc left Xnstar zero over the last Xdsp
    // rows and the factor came out NaN there.
    int begd[2] = {span_guard.begspn[0], span_guard.begspn[1]};
    tdset_td(ctx, begd, pos1bk, posffc + std::max(ctx.x11reg.xdsp, 0), sp);

    // editor.f:1618-1627 -- the group pointers into the irregular-regression
    // model. The oracle derives these at spec-read time from Grpttx; this port
    // derives them here, off the model loadxr has already made live, which is
    // the same set. They were not needed while every x11regression spec carried
    // a fixed `variables=(td)` -- xrgtrn_td simply assumed Tdgrp>0 -- but
    // `aictest=(td)` fits a model with NO trading day, and xrgtrn.f:43 takes a
    // different arm for it (`X-1` instead of `Xnstar*X - Xn`).
    ctx.x11reg.tdgrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                              m.ngrptl, "Trading Day");
    if (ctx.x11reg.tdgrp == 0)
        ctx.x11reg.stdgrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                                   m.ngrptl, "Stock Trading Day");
    ctx.x11reg.holgrp = ctx.x11reg.easgrp;
    if (ctx.x11reg.holgrp == 0)
        ctx.x11reg.holgrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                                   m.ngrptl, "Thanksgiving");
    if (ctx.x11reg.holgrp == 0)
        ctx.x11reg.holgrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                                   m.ngrptl, "Labor");

    // Copy the irregular into trnsrs and transform it (mult/logadd).
    std::vector<double> trnsrs(nobspf > 0 ? nobspf : 1, 0.0);
    for (int i = 0; i < nobspf; ++i) trnsrs[i] = sti[(irridx - 1) + i];
    if (muladd == 0 || muladd == 2)
        xrgtrn_td(ctx, trnsrs.data(), irridx, irrend, ctx.x11reg.tdgrp);

    // Extreme-value exclusion on the RAW irregular (tdxtrm); skipped when the
    // AO-outlier method is active (Sigxrg==0), leaving no rows excluded until the
    // outlier ID step (a later increment) runs.
    if (sigxrg > 0.0) {
        tdxtrm_td(ctx, sti, sigxrg, kpart, irridx, irrend);
    } else {
        for (int i = 1; i <= PLEN; ++i) ctx.xclude.rgxcld(i) = false;
        ctx.xclude.nxcld = 0;
    }

    // Automatic AICC tests on the irregular (x11mdl.f:251-252, B iteration
    // only). Leaves the winning trading-day / Easter / user design in the model
    // so the build below (and the C iteration) carries it.
    if (kpart == 2 && (ctx.x11reg.xtdtst > 0 || ctx.x11log.xeastr ||
                       (ctx.x11log.xuser && ctx.usrreg.ncusrx > 0))) {
        const bool trumlt = (muladd == 0);   // !Psuadd && Muladd==0 (mult path)
        x11aic(ctx, trnsrs.data(), sti, nobspf, nfcst, irridx, irrend, muladd,
               trumlt);
        if (ctx.error.lfatal) return;
        // x11mdl.f:256-270 -- the verdict, then Xtdtst=0 so the C iteration
        // does not re-run the test. (Xeastr's matching :291 clear is NOT
        // reproduced; the kpart==2 gate above already fences the C iteration
        // off. See the note in x11aic.)
        if (ctx.x11reg.xtdtst > 0) {
            int iaic = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                              m.ngrptl, "Trading Day");
            if (iaic == 0)
                iaic = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                              m.ngrptl, "Stock Trading Day");
            ctx.x11reg_xtd_accepted = iaic > 0;
            ctx.x11reg.xtdtst = 0;
        }
    }

    // x11mdl.f:308-381 -- if the AIC tests left NO trading-day, stock-TD or
    // holiday group in the irregular-regression model there is nothing to
    // estimate: the oracle prints a NOTE, punches IDENTITY factors for whatever
    // factor tables were requested, clears the three "an x11regression <x> is
    // active" flags and RETURNS. Everything below is skipped, including the
    // divsub that would otherwise take a factor back out of the irregular.
    //
    // Only the TD reject arm can reach this, which is why it went unnoticed
    // until a spec with `aicdiff=` above the AICC gap existed: without this the
    // engine carried on, fitted the empty design and produced a c16 carrying
    // the length-of-month prior where the oracle writes 1.0 -- OUTCOME: OK,
    // wrong factor.
    if (!(ctx.x11reg.holgrp > 0 || ctx.x11reg.tdgrp > 0 ||
          ctx.x11reg.stdgrp > 0)) {
        errhdr(ctx);
        writln(ctx,
               " NOTE: Because of the AIC test result, X-13ARIMA-SEATS has "
               "removed any trading day,",
               ctx.units.mt2, ctx.units.mt2, false);
        writln(ctx,
               "       stock trading day, or holiday regressors from the "
               "irregular component",
               ctx.units.mt2, ctx.units.mt2, false);
        writln(ctx,
               "       regression model.  No further model estimation will be "
               "attempted.",
               ctx.units.mt2, ctx.units.mt2, false);
        // x11mdl.f:326-350 -- ZERO for an additive adjustment, ONE otherwise.
        // (The `icol.ge.4.and.Kswv.gt.0` arm punches the user prior TD Stptd
        // instead; Kswv is 0 on this path, so it is not reproduced.)
        const double idv = (muladd == 1) ? 0.0 : 1.0;
        for (int i = pos1bk; i <= posffc; ++i) {
            ctx.x11fac.faccal(i) = idv;
            ctx.x11fac.factd(i) = idv;
        }
        // The oracle punches only the C-iteration factor here -- the golden for
        // the reject spec carries an all-ones .c16 and NO .b16 and no .xrm, so
        // the B-iteration snapshot is deliberately left unwritten.
        if (kpart == 3)
            ctx.x11reg_c16.assign(posfob - pos1ob + 1, idv);
        ctx.x11reg_ran = true;
        // x11mdl.f:372-374.
        if (ctx.x11log.axrgtd) ctx.x11log.axrgtd = false;
        if (ctx.x11log.axrghl) ctx.x11log.axrghl = false;
        if (ctx.x11log.axruhl) ctx.x11log.axruhl = false;
        // NOT reproduced, and it is a pre-existing gap rather than one this
        // branch introduces: x11mdl.f:377-379's `nfinalxreg` / `finalxreg01`
        // savelog pair. The engine emits that family on NO path, accept arm
        // included, so it is ungated everywhere. Note for whoever ports it that
        // :378 and :883 write `'finalxreg01: none'` through FORMAT 1060, which
        // in that scope is the WEEKDAY-HEADER format ('  Mon  Tue ...') and
        // carries no data descriptor -- so the string is dropped and the .udg
        // gets a stray column header instead of the key. Census bug, measured
        // in this spec's own golden, not claimed as a CB entry until the
        // family is ported and the difference is reproducible.
        return;
    }

    // Build the design and solve the OLS.
    int nrxy = 0, frstry = 0;
    regvar(ctx, trnsrs.data(), nobspf, ar.fctdrp, nfcst, 0, ar.userx.data(),
           ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj, ar.reglom, nrxy,
           ar.begxy.data(), frstry, /*xmeans=*/true, ar.elong);
    if (ctx.error.lfatal) return;
    ar.nrxy = nrxy;
    // x11mdl.f:390-395 -- x11regression coefficients held FIXED (Iregfx>=2 on the
    // WORKING model, i.e. loadxr's copy of Irgxfx/Regfxx) are struck from the
    // design and their effect subtracted from the irregular before the OLS; the
    // matching addfix below puts both back before the factor is built. This is
    // what history{fixx11reg=yes} / slidingspans reach: with every column fixed
    // the OLS has nothing left to estimate and each span reuses the main run's
    // daily weights.
    const bool xrg_fixed = ctx.model.iregfx >= 2 && ctx.model.nb > 0;
    if (xrg_fixed) {
        rmfix(ctx, trnsrs.data(), ctx.extend.nbcst, nrxy, 1);
        if (ctx.error.lfatal) return;
        int nrxyf = 0, frstryf = 0;
        regvar(ctx, trnsrs.data(), nobspf, ar.fctdrp, nfcst, 0, ar.userx.data(),
               ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj, ar.reglom, nrxyf,
               ar.begxy.data(), frstryf, /*xmeans=*/true, ar.elong);
        if (ctx.error.lfatal) return;
        nrxy = nrxyf;
        ar.nrxy = nrxy;
    }
    // Initial OLS fit; capture the residuals + effective-obs count so the
    // outlier-ID robust mse has its starting values (x11mdl.f:416 regx11(a)).
    std::vector<double> aotl(PLEN, 0.0);
    int naotl = 0, nefotl = 0;
    if (!regx11(ctx, aotl.data(), &naotl, &nefotl)) { prterx_if_singular(ctx); return; }
    // x11mdl.f:424 -- Otlxrg is the automatic AO outlier identification arm of
    // the extreme-value method; editor.f:1727-1747 chose it at spec-read (see
    // xrg_editor_setup). AO-only, add-one, over the full model span, with the
    // critical value derived from the outlier-span length when none was given.
    //
    // The two span guards on x11mdl.f:424-425 are what stop the design growing
    // without bound during a replay. `Issap==2` is the sliding-spans loop and
    // `Irev==4` the history one; on either, a re-identification is done only
    // when the user asked for one per span (`slidingspans{x11outlier=}` /
    // `history{x11outlier=}`, both defaulting to yes). Without the Issap arm,
    // `x11outlier=no` still identified a fresh AO set for every span on top of
    // the ones ssxmdl had held back, and airline + x11regression{critical=3.5}
    // died at "Adding AO1958.Jan exceeds the number of regression effects
    // allowed in the model (80)" where the oracle finishes.
    const bool span_reid =
        (ctx.hiddn.irev < 4 || (ctx.hiddn.irev == 4 && ctx.rev.rvxotl)) &&
        (ctx.hiddn.issap < 2 || (ctx.hiddn.issap == 2 && ctx.sspinp.ssxotl));
    if (ctx.x11log.otlxrg && span_reid && ctx.xclude.nxcld == 0) {
        // x11mdl.f:441 hands idotlr the COMMON Begxot/Endxot. This used to
        // re-derive the pair from Begspn/Nspobs, which was the same window only
        // while `x11regression{outlierspan=}` was being dropped by the parser:
        // the option moves it, and even with no outlierspan= at all the
        // default's END is the series end (gtxreg.f:671, Begsrs+Nobs-1), not
        // the span end. Written by gtxreg for the main run and by
        // ssx11a.f:105-106 for each sliding-spans span.
        const int* begxot = ctx.x11reg.begxot.data();
        const int* endxot = ctx.x11reg.endxot.data();
        // Critxr is whatever editor.f:1749-1757 left: the user's `critical=`,
        // or the value derived there from the outlier-span length. Deriving it
        // here instead made it track the SPAN on a replay (see the note in
        // readers_spec.cpp's xrg_editor_setup).
        const double critxr = ctx.x11reg.critxr;
        double cvec[3] = {critxr, critxr, critxr};
        int nefobs = nefotl;
        idotlr(ctx, /*ltstao=*/true, /*ltstls=*/false, /*ltsttc=*/false,
               /*ladd1=*/true, cvec, /*cvrduc=*/0.5, begxot, endxot, nefobs,
               ctx.arima.lestim, ctx.arima.mxiter, ctx.arima.mxnlit,
               /*lauto=*/false, aotl.data(), /*lxreg=*/true);
        if (ctx.error.lfatal) return;
        // x11mdl.f:452 -- rebuild the Nobspf-row design so the inserted AO columns
        // carry their forecast-region values (idotlr fills only the Nspobs span).
        // The coefficients from idotlr's final regx11 stand (no re-fit here).
        int nrxy2 = 0, frstry2 = 0;
        regvar(ctx, trnsrs.data(), nobspf, ar.fctdrp, nfcst, 0, ar.userx.data(),
               ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj, ar.reglom, nrxy2,
               ar.begxy.data(), frstry2, /*xmeans=*/true, ar.elong);
        if (ctx.error.lfatal) return;
        nrxy = nrxy2;
        ar.nrxy = nrxy;
    }

    // x11mdl.f:459-467 -- restore the fixed columns (and their effect) now that
    // the OLS is done, so the factor below is built from the FULL design.
    if (xrg_fixed) {
        addfix(ctx, trnsrs.data(), ctx.extend.nbcst, /*rind=*/1, 1);
        if (ctx.error.lfatal) return;
        int nrxya = 0, frstrya = 0;
        regvar(ctx, trnsrs.data(), nobspf, ar.fctdrp, nfcst, ctx.extend.nbcst,
               ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
               ar.reglom, nrxya, ar.begxy.data(), frstrya, /*xmeans=*/true,
               ar.elong);
        if (ctx.error.lfatal) return;
        nrxy = nrxya;
        ar.nrxy = nrxy;
    }

    // x11mdl.f:499-509 -- the `.xrm` design-matrix save, and it sits HERE, before
    // the restore, so what the oracle writes is the NARROW design (the fit's own
    // rows), not the rebuilt full-span one. Kpart==3 only.
    if (kpart == 3) {
        const int ncxy_x = m.ncxy, nb_x = m.nb;
        ctx.x11reg_xrm_ncol = nb_x;
        ctx.x11reg_xrm_begxy[0] = ar.begxy(1);
        ctx.x11reg_xrm_begxy[1] = ar.begxy(2);
        ctx.x11reg_xrm.assign(static_cast<std::size_t>(nrxy) * nb_x, 0.0);
        for (int r = 0; r < nrxy; ++r)
            for (int c = 1; c <= nb_x; ++c)
                ctx.x11reg_xrm[static_cast<std::size_t>(r) * nb_x + (c - 1)] =
                    md.xy(r * ncxy_x + c);
    }

    // x11mdl.f:512-528 -- walk the span back out (setspn.f) and REBUILD the
    // design over it, so the factor below covers the full series span even
    // though the fit above used only the regression span. Xdsp is the OTHER
    // narrowing: xrgdrv shortened the pointers rather than Begspn/Nspobs, so
    // `nend` is 0 above and :515-517 is what puts the span back -- on the C
    // iteration only. The B iteration therefore ENDS narrow, deliberately.
    {
        int nend_r = nend;
        if (ctx.x11reg.xdsp > 0 && kpart == 3) nend_r = ctx.x11reg.xdsp;
        if (nbeg > 0 || nend_r > 0) {
            if (nend_r > 0)
                addate(ctx.x11reg.endxrg.data(), sp, nend_r, endspn_cur);
            if (nbeg > 0)
                addate(ctx.x11reg.begxrg.data(), sp, -nbeg, md.begspn.data());
            dfdate(endspn_cur, md.begspn.data(), sp, nspobs);
            nspobs += 1;
            md.nspobs = nspobs;
            dfdate(md.begspn.data(), ar.begsrs.data(), sp, ar.frstsy);
            ar.frstsy += 1;
            ar.nomnfy = ar.nobs - ar.frstsy + 1;
            nobspf = std::min(nspobs + std::max(nfcst - ar.fctdrp, 0),
                              ar.nomnfy);
            dfdate(md.begspn.data(), ctx.adj.begadj.data(), sp, ctx.adj.adj1st);
            ctx.adj.adj1st += 1;
            int nrxyr = 0, frstryr = 0;
            regvar(ctx, trnsrs.data(), nobspf, ar.fctdrp, nfcst,
                   ctx.extend.nbcst, ar.userx.data(), ar.bgusrx.data(),
                   ar.nrusrx, ctx.prior.priadj, ar.reglom, nrxyr,
                   ar.begxy.data(), frstryr, /*xmeans=*/true, ar.elong);
            if (ctx.error.lfatal) return;
            nrxy = nrxyr;
            ar.nrxy = nrxy;
            // (x11mdl.f:526's `IF(Xhlnln) kfcn` -- the nonlinear-holiday
            // rescale -- is not reached: Xhlnln needs an x11regression holiday
            // regressor, which this TD-only path does not carry.)
        }
        // Disarm only where the Fortran's own restore is what left the values:
        // when :518's `IF(nbeg>0.or.nend>0)` fired, and on the Xdsp route, where
        // the B iteration is SUPPOSED to end narrow. Everywhere else the guard
        // keeps its original restore-to-entry behaviour -- history{}'s per-span
        // xrgdrv replays depend on it (29 gates).
        if (nbeg > 0 || nend_r > 0 || ctx.x11reg.xdsp > 0)
            span_guard.armed = false;
    }

    // x11mdl.f:531-540 -- the EFFECTIVE regressor type x11ref classifies by is
    // not Rgvrtp. A column carrying the generic PRGTUD ('User-defined') takes
    // its type from the declared `usertype=` list instead, which loadxr.f:76
    // has copied Usxtyp -> Usrtyp. So `usertype=(td)` on a user column puts it
    // in the TRADING-DAY factor even though its Rgvrtp says User-defined, and
    // dropping this remap loses exactly that column's contribution to Ftd
    // (measured: b16 out by b_u2*u2/Xnstar, up to 7.6e-4 relative).
    //
    // `iusr` advances only on PRGTUD columns while Usrtyp is indexed by USER
    // column number, so a PRGUTD column ahead of a PRGTUD one shifts the read
    // -- the same off-by-one shape as CB-36 and reproduced for the same reason.
    std::vector<int> rtype(static_cast<std::size_t>(m.nb > 0 ? m.nb : 1), 0);
    {
        int iusr = 1;
        for (int icol = 1; icol <= m.nb; ++icol) {
            if (m.rgvrtp(icol) == prm::PRGTUD && ctx.usrreg.ncusrx > 0) {
                rtype[icol - 1] = (iusr <= prm::PUREG)
                                      ? ctx.usrreg.usrtyp(iusr) : 0;
                ++iusr;
            } else {
                rtype[icol - 1] = m.rgvrtp(icol);
            }
        }
    }

    // ---- x11mdl.f:541-660 -- the "X-11 style" daily weights Dx11 ------------
    // Built here, ABOVE x11ref (x11mdl.f:694), because the Lxrneg reweighting
    // at :603-612 writes back into B: every TD factor downstream comes off the
    // rewritten coefficients. This block used to sit below the x11ref_td call,
    // which was harmless only because its one consumer (the Kswv==3 combine)
    // re-runs x11ref_td for itself.
    //
    // `Haveum` is never set in this port, so :541's `.and.(.not.Haveum)` is
    // always true.
    double dx11[7];
    bool have_dx11 = false;
    int tdbegcol = 0, tdendcol = 0;
    // x11mdl.f:541 -- `IF(Havxtd.and.(.not.Haveum))`. The Haveum half had been
    // dropped; it is restored here rather than argued away, per the standing
    // rule about narrowed conditions waiting for their second call site.
    if (ctx.x11log.havxtd && !ctx.xrgum.haveum) {
        // x11mdl.f:545 -- an EXACT search for "Trading Day", which is what makes
        // the one-column arm below dead code. `addtd.f:75-77` titles every
        // single-column TD group `1-Coefficient <title>`, so `td1coef` builds
        // "1-Coefficient Trading Day" and this search returns 0. Measured on an
        // instrumented build of the vendored sources: `igrp=0, Grpttl=
        // "1-Coefficient Trading DayAutomatically Identified Outliers"`.
        const int igrp = strinx(true, m.grpttl.raw(), m.grpptr.data(), 1,
                                m.ngrptl, "Trading Day");
        if (igrp > 0) {
            have_dx11 = true;
            tdbegcol = m.grp(igrp - 1);
            tdendcol = m.grp(igrp) - 1;
            const int begcol = tdbegcol, endcol = tdendcol;
            setdp(prm::DNOTST, 7, dx11);
            dx11[6] = 0.0;
            if (begcol == endcol) {
                // td1coef: one contrast coefficient -> five weekday weights and
                // the -5/2 weekend split.
                for (int icol = 1; icol <= 5; ++icol) {
                    dx11[icol - 1] = md.b(begcol);
                    if (muladd != 1) dx11[icol - 1] += 1.0;
                }
                for (int icol = 6; icol <= 7; ++icol) {
                    dx11[icol - 1] = (-5.0 * md.b(begcol)) / 2.0;
                    if (muladd != 1) dx11[icol - 1] += 1.0;
                }
            } else {
                for (int icol = begcol; icol <= endcol; ++icol) {
                    dx11[icol - begcol] =
                        (muladd == 1) ? md.b(icol) : 1.0 + md.b(icol);
                    dx11[6] -= md.b(icol);
                }
                if (muladd != 1) dx11[6] += 1.0;
            }

            // x11mdl.f:577-624 -- x11regression{reweight=yes}. A negative daily
            // weight makes the B16/C16 trading-day factor go negative, which a
            // multiplicative adjustment cannot use. Reweighting clamps every
            // UNFIXED negative weight to zero and rescales the surviving
            // positive unfixed weights so the seven still total 7, leaving the
            // fixed ones alone; the rescaled weights are then written back into
            // B, so x11ref builds the factors from them.
            if (ctx.x11log.lxrneg) {
                const auto& xg = ctx.xrgmdl;
                double tdwsum = 0.0, tdwfix = 0.0;
                bool tdneg = false;
                const int ncol0 = endcol - begcol + 1;
                int icol = 1;
                for (icol = 1; icol <= ncol0; ++icol) {
                    const bool isfix = xg.regfxx(begcol + icol - 1);
                    if (dx11[icol - 1] >= 0.0) {
                        if (isfix) tdwfix += dx11[icol - 1];
                        else tdwsum += dx11[icol - 1];
                    } else if (!isfix) {
                        dx11[icol - 1] = 0.0;
                        tdneg = true;
                    }
                }
                // NOT a transcription slip. x11mdl.f:597-602 TESTS `Dx11(7)` but
                // ADDS/ZEROES `Dx11(icol)`, where `icol` is the DO variable the
                // loop above left at ncol0+1. For a six-column TD group ncol0==6
                // and icol==7, so the two agree by accident; for td1coef ncol0==1
                // and it would read element 7 while writing element 2.
                //
                // CLOSED 2026-08-09, and the answer is that the divergent case
                // is DEAD CODE in the oracle -- not for the reason this comment
                // used to give.
                //
                // It said the case needed an UNFIXED td1coef coefficient above
                // 0.4, and that fixing it to get there makes the group all-fixed
                // and clears Lxrneg at editor.f:1660. Both true, and both beside
                // the point: an instrumented build of the vendored sources on
                // `x11regression{ variables=(td1coef) reweight=yes }` never
                // reaches this block at all. `Havxtd=T, Lxrneg=T`, and then
                //   igrp=0  Grpttl="1-Coefficient Trading DayAutomatically…"
                // `addtd.f:75-77` titles EVERY single-column trading-day group
                // `1-Coefficient <title>`, and x11mdl.f:545's search for
                // "Trading Day" is exact -- so `begcol == endcol` cannot be
                // reached through a group this program builds with one column.
                // The only other route would be a six-column "Trading Day" group
                // reduced to one by deletion, and nothing deletes TD columns
                // singly (x11aic.f:257-274 takes them all).
                //
                // Kept transcribed verbatim anyway: the guard above it is what
                // makes it dead, and a future caller that finds the group by a
                // different name would resurrect it.
                const int stale = ncol0 + 1;
                if (dx11[6] >= 0.0) {
                    tdwsum += dx11[stale - 1];
                } else {
                    dx11[stale - 1] = 0.0;
                    tdneg = true;
                }
                if (tdneg) {
                    if (!(tdwsum > 0.0)) {
                        // x11mdl.f:613-623 -- every unfixed weight is zero, so
                        // there is no scale factor to build. The oracle abends.
                        writln(ctx, "ERROR: Cannot generate factor necessary to "
                               "reweight trading day", stdio::STDERR,
                               ctx.units.mt2, true);
                        writln(ctx, "       daily weights - none of the unfixed "
                               "daily weights are greater", stdio::STDERR,
                               ctx.units.mt2, false);
                        writln(ctx, "      than zero.", stdio::STDERR,
                               ctx.units.mt2, false);
                        abend(ctx);
                        return;
                    }
                    for (int i = 1; i <= 7; ++i) {
                        const int icol2 = begcol + i - 1;
                        // `Regfxx(icol2)` runs past endcol for a short TD group;
                        // the Fortran reads the same slots out of the same
                        // PB-length array, so the read is in bounds either way.
                        const bool fixed_here =
                            (icol2 >= 1 && icol2 <= prm::PB) && xg.regfxx(icol2);
                        if (fixed_here && endcol > begcol) continue;
                        if (dx11[i - 1] > 0.0)
                            dx11[i - 1] *= (7.0 - tdwfix) / tdwsum;
                        if (i <= ncol0) md.b(icol2) = dx11[i - 1] - 1.0;
                    }
                    // x11mdl.f:628-658's "NOTE: At least one of the parameter
                    // estimates above yields a negative daily weight" table is
                    // part of the deferred .out print engine: it is guarded by
                    // Prttab(fext) and writes only to Mt1/Mt2, never STDERR.
                }
            }
        } else {
            // x11mdl.f:661-690 -- and the PAIRING is the whole finding. This
            // `ELSE` belongs to `:546`'s `IF(igrp.gt.0)`, NOT to `:541`'s
            // `IF(Havxtd.and.(.not.Haveum))`: an x11regression trading day is
            // present but the WORKING model carries no "Trading Day" group,
            // which is what a STOCK design looks like. Decoded from the END IF
            // chain mechanically (`:660` closes `:578`, `:690` closes `:546`,
            // `:691` closes `:541`) after a first transcription guessed `:541`
            // and produced a branch that could never fire -- the same lesson as
            // last increment's ARGDIC decode. Count the block, do not read the
            // indentation.
            //
            // MEASURED BEFORE PORTING: on airline +
            // `x11regression{variables=(tdstock[15]) b=(-1.5f ...)}` the oracle
            // abends with the message below and this engine returned
            // `OUTCOME: OK` -- the silent-wrongness shape the standing rules
            // call worse than having no code at all.
            //
            // Two oracle oddities transcribed rather than tidied:
            //  * `:664` looks the group up in the x11regression STORE
            //    (`Grpttx/Gpxptr/Ngrptx`) and then indexes the WORKING model's
            //    `Grp` with the index it got back, where `:545` uses the working
            //    model for both. Identical whenever nothing has mutated the
            //    design since `loadxr(F)`; they diverge once x11aic has added or
            //    struck a column. Same family as CB-37's `Grpx(-1)` alias, and
            //    NOT filed as a Census bug -- no spec here reaches a state where
            //    the two disagree, and "measure before naming a Census bug".
            //  * The test is `B(icol) < -1 .or. dpeq(B(icol),-1)` -- a `<=`
            //    written as a disjunction with the equality half on `dpeq`.
            //    Transcribed as the same two comparisons.
            dx11[0] = prm::DNOTST;
            if (muladd == 0) {
                const int istk = strinx(true, ctx.xrgmdl.grpttx.raw(),
                                        ctx.xrgmdl.gpxptr.data(), 1,
                                        ctx.xrgmdl.ngrptx, "Stock Trading Day");
                if (istk > 0) {
                    const int begcol = m.grp(istk - 1);
                    const int endcol = m.grp(istk) - 1;
                    bool tdneg = false;
                    for (int icol = begcol; icol <= endcol; ++icol)
                        if (md.b(icol) < -1.0 || dpeq(md.b(icol), -1.0))
                            tdneg = true;
                    if (tdneg) {
                        // FORMAT 1070, written straight to the channels rather
                        // than through writln: the `//` pairs emit EMPTY
                        // records, and writln's `lblnk` blank is `(' ',a)` with
                        // a space -- two characters, not zero. Measured against
                        // the oracle's .err byte for byte.
                        errhdr(ctx);
                        auto& err = ctx.channels_.unit(stdio::STDERR);
                        auto& mt2c = ctx.channels_.unit(ctx.units.mt2);
                        static const char* const K[] = {
                            " ERROR: At least one of the stock trading day "
                            "regression coefficient",
                            "        estimates from the irregular regression "
                            "model produce",
                            "        nonpositive trading day factors for "
                            "multiplicative seasonal",
                            "        adjustments.",
                            "",
                            "        Use the regression spec to estimate the "
                            "stock trading day effect.",
                            "",
                            "",
                        };
                        for (const char* ln : K) {
                            err.put(std::string(ln) + "\n");
                            mt2c.put(std::string(ln) + "\n");
                        }
                        abend(ctx);
                        return;
                    }
                }
            }
        }
    }

    // Build the TD factor series and copy into Factd/Faccal.
    std::vector<double> fcal(nrxy > 0 ? nrxy : 1, 0.0);
    std::vector<double> ftd(nrxy > 0 ? nrxy : 1, 0.0);
    x11ref_td(ctx, fcal.data(), ftd.data(), pos1bk, nrxy, m.ncxy, md.b.data(),
              md.xy.data(), m.nb, rtype.data(), ctx.x11opt.kswv,
              ctx.x11reg.tdgrp, ctx.x11reg.stdgrp, ctx.x11reg.holgrp);
    // x11mdl.f:701-702 -- the C iteration writes Xdsp EXTRA points, covering the
    // stretch xrgdrv chopped off the pointers; the design was rebuilt over the
    // full span just above, so the rows exist.
    int nfac = posffc - pos1bk + 1;
    if (ctx.x11reg.xdsp > 0 && kpart == 3) nfac += ctx.x11reg.xdsp;
    for (int i = 0; i < nfac; ++i) {
        ctx.x11fac.faccal(pos1bk + i) = fcal[i];
        ctx.x11fac.factd(pos1bk + i) = ftd[i];
    }

    // Snapshot b16 (Kpart=2) / c16 (Kpart=3) over [Pos1ob, lastpr] -- x11mdl.f
    // :514-517, the same Xdsp extension the table/punch calls use.
    int lastpr = posfob;
    if (ctx.x11reg.xdsp > 0 && kpart == 3) lastpr = posfob + ctx.x11reg.xdsp;
    std::vector<double> snap(lastpr - pos1ob + 1);
    for (int i = pos1ob; i <= lastpr; ++i) snap[i - pos1ob] = ctx.x11fac.factd(i);
    if (kpart == 2) ctx.x11reg_b16 = snap;
    else ctx.x11reg_c16 = snap;

    ctx.x11reg_ran = true;

    // Divide the TD effect out of the irregular (x11pt2 re-iterates without it).
    divsub(sti, sti, ctx.x11fac.faccal.data(), pos1ob, posfob, muladd);

    // ---- x11mdl.f:786-830 -- the COMBINED daily weights ---------------------
    // Only on the Kswv==3 route (tdprior weights AND an x11regression TD model,
    // see x11pt1.f:235). Dx11 -- built above, and already reweighted if
    // reweight=yes -- is ADDED to the user's prior weights and the
    // calendar/TD factors rebuilt from the sum, so the published Faccal/Factd
    // carry BOTH effects, not just the estimated one.
    if (ctx.x11opt.kswv == 3 && ctx.x11log.havxtd) {
        if (!have_dx11) {
            // Kswv==3 requires Axrgtd, and Axrgtd without a Trading Day group in
            // the loaded x11reg model would leave Dx11 at DNOTST -- the oracle
            // would combine sentinels. Refuse rather than reproduce garbage.
            x11reg_not_ported(ctx, "x11mdl Kswv=3 with no Trading Day group");
            return;
        }
        const int begcol = tdbegcol;
        const int endcol = tdendcol;
        ctx.x11reg_tdwt.assign(dx11, dx11 + 7);

        // x11mdl.f:787-789 -- the combine itself.
        for (int icol = 1; icol <= 7; ++icol)
            dx11[icol - 1] += ctx.x11reg.dwt(icol) - 1.0;
        ctx.x11reg_combtdwt.assign(dx11, dx11 + 7);

        if (ctx.x11log.calfrc) {
            // x11mdl.f:798-800 -- forcecal=yes folds the prior-TD factor Stptd
            // straight into Faccal/Factd instead of rebuilding. Unported, and
            // x11regression{forcecal=} is not honoured by this parser either, so
            // Calfrc can only be true if that changes.
            x11reg_not_ported(ctx, "x11mdl Kswv=3 forcecal= combine");
            return;
        }
        // x11mdl.f:803-820 -- rebuild Faccal/Factd from the COMBINED weights.
        // bb2 replaces the TD block of B with Dx11-1 and keeps every other
        // coefficient; Kswv is passed as 4, NOT 3, so x11ref finishes with the
        // ordinary Xn/Xnstar ratio (see x11ref_td).
        std::vector<double> bb2(static_cast<std::size_t>(m.nb > 0 ? m.nb : 1),
                                0.0);
        for (int icol = 1; icol <= m.nb; ++icol)
            bb2[icol - 1] = (icol >= begcol && icol <= endcol)
                                ? dx11[icol - begcol] - 1.0
                                : md.b(icol);
        std::vector<double> fcal2(nrxy > 0 ? nrxy : 1, 0.0);
        std::vector<double> ftd2(nrxy > 0 ? nrxy : 1, 0.0);
        x11ref_td(ctx, fcal2.data(), ftd2.data(), pos1bk, nrxy, m.ncxy,
                  bb2.data(), md.xy.data(), m.nb, rtype.data(), /*kswv=*/4,
                  ctx.x11reg.tdgrp, ctx.x11reg.stdgrp, ctx.x11reg.holgrp);
        for (int i = 0; i < nfac; ++i) {
            ctx.x11fac.faccal(pos1bk + i) = fcal2[i];
            ctx.x11fac.factd(pos1bk + i) = ftd2[i];
        }
        // x11mdl.f:825-826 -- and Kswv itself moves on: back to 3 after the B
        // iteration, to 4 after the C one (which stops xrgtrn/x11ref taking the
        // Kswv==3 arm for the rest of the run).
        ctx.x11opt.kswv = (kpart == 3) ? 4 : 3;
    }

    // x11mdl.f:873-875, inside its `IF(Kpart.eq.3)` -- the sliding-spans store
    // of the x11regression calendar factor, and the ONLY producer of the `tds`
    // (trading-day spans) table on an x11regression run. x11pt2.f:132-136's
    // ssrit is keyed on `Adjtd.eq.1`, the regARIMA trading day, and takes its
    // `Itd=0; IF(Axrgtd)Itd=1` else-branch here instead. Without this the port
    // emitted no tds table at all and the parametrised gate skipped it as
    // "spec does not produce this tag" -- a parametrisation that shrank.
    if (kpart == 3 && ctx.hiddn.issap == 2 && ctx.ssap.itd == 1)
        ssrit(ctx, ctx.x11fac.factd.data(), pos1ob, lastpr, 1,
              ctx.inpt.series.data());
}

}  // namespace x13
