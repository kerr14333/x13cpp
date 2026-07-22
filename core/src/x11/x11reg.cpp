// x11reg.cpp -- x11regression{} irregular-component regression (WIP scaffolding).
//
// TD-only, multiplicative path of x11mdl.f + tdset/xrgtrn/tdxtrm/dlrgrw/regx11/
// x11ref/mulref. Not yet wired into x11pt2 (the B/C-iteration call site + the
// b16/c16 emit + the divsub Sti fold land next). Reuses olsreg/resid/daxpy.
// See tools/x11regression_scope.md for the routine map.
#include "x11/x11reg.hpp"

#include <cmath>
#include <vector>

#include "common/x13context.hpp"
#include "regarima/estimate.hpp"   // olsreg, resid
#include "regarima/regvar.hpp"     // regvar
#include "numeric/numeric.hpp"     // daxpy
#include "x11/x11filt.hpp"         // divsub
#include "specparse/specparse.hpp" // addate
#include "gen/model.hpp"           // PRG* regressor types, PSNGER
#include "gen/notset.hpp"          // prm::DNOTST

namespace x13 {
namespace {

constexpr int PLEN = 1020;
constexpr int PXPX = 3403;  // Chlxpx packed size (mdldat.cmn)

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
bool regx11(X13Context& ctx) {
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
        if (md.sngcol > 0) { md.armaer = prm::PSNGER; return false; }
        md.nfev += ncxy + 1;
    }
    std::vector<double> a(nrtxy > 0 ? nrtxy : 1, 0.0);
    resid(ctx, txy.data(), nrtxy, ncxy, ncxy, 1, nb, -1.0, md.b.data(), a.data());
    if (ctx.error.lfatal) return false;
    double apa = 0.0;
    for (int i = 0; i < nrtxy; ++i) apa += a[i] * a[i];
    md.var = apa / dnefob;
    return true;
}

// ---- x11ref.f (mult, TD-only) --------------------------------------------
void x11ref_td(X13Context& ctx, double* fcal, double* ftd, int xdev, int nrxy,
               int ncxy, const double* b, const double* xy, int nb,
               const int* rtype) {
    for (int i = 0; i < nrxy; ++i) { fcal[i] = 0.0; ftd[i] = 0.0; }
    const double* xn = ctx.xtdtyp.xn.data();
    const double* xnstar = ctx.xtdtyp.xnstar.data();
    // Raw TD factor: Ftd += B(icol) * Xy(:,icol) (column icol, stride Ncxy).
    for (int icol = 1; icol <= nb; ++icol)
        if (is_td_type(rtype[icol - 1]))
            daxpy(nrxy, b[icol - 1], xy + (icol - 1), ncxy, ftd, 1);
    // Mean-normalize by Xnstar (mulref, DNOTST -> use the vector), then finish.
    mulref(nrxy, fcal, ftd, xdev, xnstar, prm::DNOTST, false);
    mulref(nrxy, ftd, ftd, xdev, xnstar, prm::DNOTST, true);
    for (int irow = 1; irow <= nrxy; ++irow) {
        const int ir2 = irow + xdev - 1;
        ftd[irow - 1] += xn[ir2 - 1] / xnstar[ir2 - 1];
        fcal[irow - 1] += xn[ir2 - 1] / xnstar[ir2 - 1];
    }
}

// ---- x11mdl.f orchestration (TD-only mult path) --------------------------
void x11mdl_td(X13Context& ctx, int kpart) {
    auto& md = ctx.mdldat;
    auto& m = ctx.model;
    auto& ar = ctx.arima;
    const int sp = m.sp;
    const int pos1ob = ctx.x11ptr.pos1ob, posfob = ctx.x11ptr.posfob;
    const int pos1bk = ctx.x11ptr.pos1bk, posffc = ctx.x11ptr.posffc;
    const int nspobs = md.nspobs;
    const int muladd = ctx.x11opt.muladd;
    double* sti = ctx.x11srs.sti.data();

    const double sigxrg = 2.5;   // editor.f:1733 TD-only default
    const int nfcst = ctx.extend.nfcst;
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

    // Extreme-value exclusion on the RAW irregular.
    tdxtrm_td(ctx, sti, sigxrg, kpart, irridx, irrend);

    // Build the design and solve the OLS.
    int nrxy = 0, frstry = 0;
    regvar(ctx, trnsrs.data(), nobspf, ar.fctdrp, nfcst, 0, ar.userx.data(),
           ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj, ar.reglom, nrxy,
           ar.begxy.data(), frstry, /*xmeans=*/true, ar.elong);
    if (ctx.error.lfatal) return;
    ar.nrxy = nrxy;
    if (!regx11(ctx)) return;

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
