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
#include "specparse/specparse.hpp" // addate
#include "gen/model.hpp"           // PRG* regressor types, PSNGER
#include "gen/notset.hpp"          // prm::DNOTST

namespace x13 {
namespace {

constexpr int PLEN = 1020;
constexpr int PXPX = 3403;  // Chlxpx packed size (mdldat.cmn)

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

// ---- xrgtrn.f (mult, Tdgrp>0, Kswv=0) ------------------------------------
void xrgtrn_td(X13Context& ctx, double* x, int l1, int l2) {
    const auto& xt = ctx.xtdtyp;
    for (int i = l1; i <= l2; ++i) {
        const int i2 = i - l1 + 1;
        x[i2 - 1] = xt.xnstar(i) * x[i2 - 1] - xt.xn(i);
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
void x11ref_td(X13Context& ctx, double* fcal, double* ftd, int xdev, int nrxy,
               int ncxy, const double* b, const double* xy, int nb,
               const int* rtype) {
    for (int i = 0; i < nrxy; ++i) { fcal[i] = 0.0; ftd[i] = 0.0; }
    std::vector<double> fhol(nrxy > 0 ? nrxy : 1, 0.0);
    const double* xn = ctx.xtdtyp.xn.data();
    const double* xnstar = ctx.xtdtyp.xnstar.data();
    // Raw factors: Ftd/Fhol += B(icol) * Xy(:,icol) (column icol, stride Ncxy).
    for (int icol = 1; icol <= nb; ++icol) {
        if (is_td_type(rtype[icol - 1]))
            daxpy(nrxy, b[icol - 1], xy + (icol - 1), ncxy, ftd, 1);
        else if (is_hol_type(rtype[icol - 1]))
            daxpy(nrxy, b[icol - 1], xy + (icol - 1), ncxy, fhol.data(), 1);
    }
    // Mean-normalize the TD factor by Xnstar (mulref, DNOTST -> use the vector).
    mulref(nrxy, fcal, ftd, xdev, xnstar, prm::DNOTST, false);
    mulref(nrxy, ftd, ftd, xdev, xnstar, prm::DNOTST, true);
    // Fold the holiday factor into Fcal the same way (no-op when Fhol is all 0).
    mulref(nrxy, fcal, fhol.data(), xdev, xnstar, prm::DNOTST, false);
    mulref(nrxy, fhol.data(), fhol.data(), xdev, xnstar, prm::DNOTST, true);
    for (int irow = 1; irow <= nrxy; ++irow) {
        const int ir2 = irow + xdev - 1;
        ftd[irow - 1] += xn[ir2 - 1] / xnstar[ir2 - 1];
        fcal[irow - 1] += xn[ir2 - 1] / xnstar[ir2 - 1];
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
              6, rtype);
    // pritd.f:47-51 -- shift the [1,Nrxy] factors up to absolute [Frstob, ...].
    for (int i = 0; i < nrxy; ++i) ptdfac[frstob - 1 + i] = ftd[i];
}

// ---- x11aic.f (easter branch) --------------------------------------------
// Automatic AICC keep/drop test for the Easter holiday on the X-11 irregular
// regression (x11mdl.f:253, B iteration only). For each window in Xeasvc
// (Xeasvc(1)=0 == the no-Easter baseline, then 1/8/15 by default) it rebuilds
// the design (addeas -> regvar), refits the OLS (regx11), and scores the AICC
// (xrlkhd + the mult Jacobian -2*jadj when TD is present). The lowest-AICC
// window wins (aicdiff=Xraicd guards the switch away from no-Easter); the model
// is left carrying the winner so the downstream regvar/factor build picks it up.
// Easter goes into the holiday factor (not the TD b16/c16), so the factor build
// is unchanged here -- only the shared TD coefficients shift. Assumes TD is
// present (this spec's variables=(td)); td/user aictest are follow-on increments.
void x11aic_easter(X13Context& ctx, double* trnsrs, int nobspf, int nfcst,
                   int irridx, int irrend, int muladd, bool trumlt) {
    auto& m = ctx.model;
    auto& ar = ctx.arima;
    const auto& xc = ctx.xclude;
    const int easidx = m.easidx;
    const double xraicd = ctx.xrgmdl.xraicd;
    const int neasvx = ctx.x11reg.neasvx;
    const bool jac = trumlt || muladd == 2;

    // AICC Jacobian adjustment (mult / log-add): sum log(Xnstar) over the rows
    // not excluded by tdxtrm (x11aic.f:87-93). TD is present, so every easter
    // candidate gets the same -2*jadj shift.
    double jadj = 0.0;
    if (jac) {
        const double* xnstar = ctx.xtdtyp.xnstar.data();
        for (int i = irridx; i <= irrend; ++i) {
            bool ladj = true;
            if (xc.nxcld > 0) ladj = !xc.rgxcld(i - irridx + 1);
            if (ladj) jadj += std::log(xnstar[i - 1]);
        }
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
        int eg = find_easter();
        if (eg > 0) {
            const int begcol = m.grp(eg - 1);
            const int ncol = m.grp(eg) - begcol;
            dlrgef(ctx, begcol, ar.nrxy, ncol);
        }
    };

    double aicbst = prm::DNOTST;
    // x11aic.f:55 `aicind=-1`. `aicind` is NOT in that routine's declaration
    // list, so Fortran case-insensitivity resolves it to the COMMON `Aicind`
    // of arima.cmn -- the same slot easaic.f writes for the regARIMA Easter
    // test. Entering x11aic therefore CLOBBERS the regARIMA window, and
    // x11mdl.f:280 reads that same slot back for `aictest.xe.window`. Both
    // halves are reproduced: ar.aicind is written below, and the -1 is what an
    // x11aic call that skips the easter branch would leave behind.
    int aicind = 0;
    // A span/history replay is a full x11pt1->x11pt3 pass and re-enters here,
    // so the table must be rebuilt, not appended to. (The oracle cannot hit
    // this: x11mdl.f:291 sets Xeastr=F after the first test. The port keeps
    // that flag live because its editor.f:1734-1757 re-derivation of Otlxrg
    // below reads it, so the guard belongs here instead.)
    ctx.x11reg_aicc_xe.clear();
    for (int i = 1; i <= neasvx; ++i) {
        if (i > 2) { del_easter(); if (ctx.error.lfatal) return; }
        if (i > 1) {
            addeas(ctx, ctx.x11reg.xeasvc(i) + easidx, easidx, 1);
            if (ctx.error.lfatal) return;
        }
        int nrxy = 0, frstry = 0;
        regvar(ctx, trnsrs, nobspf, ar.fctdrp, nfcst, 0, ar.userx.data(),
               ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj, ar.reglom, nrxy,
               ar.begxy.data(), frstry, /*xmeans=*/true, ar.elong);
        if (ctx.error.lfatal) return;
        ar.nrxy = nrxy;
        if (!regx11(ctx)) return;
        double aichol = prm::DNOTST;
        xrlkhd(ctx, aichol, xc.nxcld);
        if (jac) aichol -= 2.0 * jadj;   // TD present -> mult Jacobian
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
    if (aicind < ctx.x11reg.xeasvc(neasvx)) {
        del_easter();
        if (ctx.error.lfatal) return;
        if (aicind > 0) { addeas(ctx, aicind + easidx, easidx, 1); }
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
}

// ---- x11mdl.f orchestration (TD-only mult path) --------------------------
void x11mdl_td(X13Context& ctx, int kpart) {
    auto& md = ctx.mdldat;
    auto& m = ctx.model;
    auto& ar = ctx.arima;
    const int sp = m.sp;
    const int pos1ob = ctx.x11ptr.pos1ob, posfob = ctx.x11ptr.posfob;
    int pos1bk = ctx.x11ptr.pos1bk, posffc = ctx.x11ptr.posffc;
    const int nspobs = md.nspobs;
    const int muladd = ctx.x11opt.muladd;
    double* sti = ctx.x11srs.sti.data();

    // editor.f:1729-1736: the 2.5-sigma tdxtrm exclusion is used only for a
    // TD-only irregular regression with no easter/holiday/AO group AND no
    // explicit critical value; when easter (etc.) or a critical= is present the
    // extreme values are handled by automatic AO outlier identification instead
    // (Otlxrg), so Sigxrg stays 0 and tdxtrm is skipped. An explicit
    // x11regression{sigma=} overrides the whole rule.
    double sigxrg;
    if (!dpeq(ctx.x11reg.sigxrg, prm::DNOTST))
        sigxrg = ctx.x11reg.sigxrg;
    else if (!ctx.x11log.xeastr && dpeq(ctx.x11reg.critxr, prm::DNOTST) &&
             !ctx.x11log.otlxrg)
        sigxrg = 2.5;
    else
        sigxrg = 0.0;
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
    }
    const int nbeg = 0, irridx = pos1ob + nbeg;
    // The design/factors span the forecast-extended buffer [pos1ob, posffc] so
    // Factd/Faccal cover the whole [pos1bk,posffc] used by the Stcsi feedback and
    // the D-part; the OLS itself still uses only the Nspobs data rows (regx11).
    const int nobspf = posffc - pos1ob + 1;
    const int irrend = irridx + nspobs - 1;

    // Trading-day calendar quantities (tdset).
    int begd[2] = {md.begspn(1), md.begspn(2)};
    tdset_td(ctx, begd, pos1bk, posffc, sp);

    // Copy the irregular into trnsrs and transform it (mult/logadd).
    std::vector<double> trnsrs(nobspf > 0 ? nobspf : 1, 0.0);
    for (int i = 0; i < nobspf; ++i) trnsrs[i] = sti[(irridx - 1) + i];
    if (muladd == 0 || muladd == 2)
        xrgtrn_td(ctx, trnsrs.data(), irridx, irrend);

    // Extreme-value exclusion on the RAW irregular (tdxtrm); skipped when the
    // AO-outlier method is active (Sigxrg==0), leaving no rows excluded until the
    // outlier ID step (a later increment) runs.
    if (sigxrg > 0.0) {
        tdxtrm_td(ctx, sti, sigxrg, kpart, irridx, irrend);
    } else {
        for (int i = 1; i <= PLEN; ++i) ctx.xclude.rgxcld(i) = false;
        ctx.xclude.nxcld = 0;
    }

    // Automatic Easter AICC test on the irregular (x11mdl.f:253, B iteration).
    // Leaves the winning Easter window in the model so the design build below
    // (and the C iteration) carries it. TD is present on this path.
    if (kpart == 2 && ctx.x11log.xeastr) {
        const bool trumlt = (muladd == 0);   // !Psuadd && Muladd==0 (mult path)
        x11aic_easter(ctx, trnsrs.data(), nobspf, nfcst, irridx, irrend, muladd,
                      trumlt);
        if (ctx.error.lfatal) return;
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
    if (!regx11(ctx, aotl.data(), &naotl, &nefotl)) return;

    // editor.f:1734-1757 -- with a holiday group present (here: the AIC-tested
    // Easter) the extreme-value method for the irregular regression is automatic
    // AO outlier identification (Otlxrg), with the AO critical value set from the
    // outlier-span length. AO-only, add-one, over the full model span.
    if (ctx.x11log.xeastr) ctx.x11log.otlxrg = true;
    if (ctx.x11log.otlxrg && ctx.xclude.nxcld == 0) {
        int begxot[2] = {md.begspn(1), md.begspn(2)};
        int endxot[2];
        addate(begxot, sp, nspobs - 1, endxot);
        int nobxot = 0;
        dfdate(endxot, begxot, sp, nobxot);
        nobxot += 1;
        // editor.f:1749-1757 -- an explicit x11regression{critical=} IS the
        // critical value; only when none was given is it derived from the
        // outlier-span length. (The Cvxtyp corrected variant, setcvl, is
        // deferred.) Reading the parsed value is the fix for a main-run
        // wrong-numbers bug: this used to be the derived value unconditionally,
        // so `critical=3.0` identified the default's outlier set instead of the
        // user's -- measured d11 1.7e-4 relative on airline.
        const double critxr = dpeq(ctx.x11reg.critxr, prm::DNOTST)
                                  ? setcv(nobxot, ctx.xrgmdl.cvxalf)
                                  : ctx.x11reg.critxr;
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

    // Build the TD factor series and copy into Factd/Faccal.
    std::vector<double> fcal(nrxy > 0 ? nrxy : 1, 0.0);
    std::vector<double> ftd(nrxy > 0 ? nrxy : 1, 0.0);
    x11ref_td(ctx, fcal.data(), ftd.data(), pos1bk, nrxy, m.ncxy, md.b.data(),
              md.xy.data(), m.nb, m.rgvrtp.data());
    const int nfac = posffc - pos1bk + 1;
    for (int i = 0; i < nfac; ++i) {
        ctx.x11fac.faccal(pos1bk + i) = fcal[i];
        ctx.x11fac.factd(pos1bk + i) = ftd[i];
    }

    // Snapshot b16 (Kpart=2) / c16 (Kpart=3) over [Pos1ob, Posfob].
    std::vector<double> snap(posfob - pos1ob + 1);
    for (int i = pos1ob; i <= posfob; ++i) snap[i - pos1ob] = ctx.x11fac.factd(i);
    if (kpart == 2) ctx.x11reg_b16 = snap;
    else ctx.x11reg_c16 = snap;

    // Snapshot the xrm design matrix: the Nb regressor columns over ALL Nrxy
    // rows (the oracle saves the forecast-extended span: Nspobs data + Nfcst
    // rows). md.xy is row-major, stride Ncxy; col c of row r at r*Ncxy+c.
    // Iteration-independent (the TD contrasts are date-based), so the last write
    // wins -- C iteration.
    const int ncxy = m.ncxy, nb = m.nb;
    ctx.x11reg_xrm_ncol = nb;
    ctx.x11reg_xrm.assign(static_cast<std::size_t>(nrxy) * nb, 0.0);
    for (int r = 0; r < nrxy; ++r)
        for (int c = 1; c <= nb; ++c)
            ctx.x11reg_xrm[static_cast<std::size_t>(r) * nb + (c - 1)] =
                md.xy(r * ncxy + c);
    ctx.x11reg_ran = true;

    // Divide the TD effect out of the irregular (x11pt2 re-iterates without it).
    divsub(sti, sti, ctx.x11fac.faccal.data(), pos1ob, posfob, muladd);
}

}  // namespace x13
