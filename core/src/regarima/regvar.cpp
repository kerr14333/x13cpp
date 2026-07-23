// regvar.cpp -- regvar.f: build the [X:y] regression design matrix (mdldat.cmn
// Xy) from the parsed regression groups and the possibly transformed data.
//
// Ported branches (reachable pre-model): Constant (ratpos over the model
// differencing), Seasonal (addsef), Trading Day / 1-coef TD (td6var), LOM/LOQ
// (td7var lom), Leap Year (td7var), Stock TD (td6var smpday), Stock LOM
// (td7var stock), Easter/StatCanEaster/StockEaster (estrmu+adestr), and the
// Reglom length-of-period adjustment of the TD columns. Outlier, user-defined,
// trigonometric-seasonal, Labor/Thanksgiving, and change-of-regime branches
// abend loudly -- their group builders are not yet ported.
#include "regarima/regvar.hpp"
#include "regarima/outlier.hpp"   // addotl (outlier column reconstruction)
#include "specparse/specparse.hpp"
#include "notset.hpp"
#include "srslen.hpp"
#include "gen/model.hpp"

#include <memory>
#include <string>
#include <vector>

namespace x13 {

namespace {
void not_ported(X13Context& ctx, const std::string& what) {
    errhdr(ctx);
    writln(ctx, "ERROR: " + what +
           " not yet ported (M2 regression-matrix slice).",
           stdio::STDERR, ctx.units.mt2, true);
    abend(ctx);
}

// Fortran INDEX(str, pat): 1-based position of the first occurrence, 0 if none.
int idxof(std::string_view str, std::string_view pat) {
    std::size_t p = str.find(pat);
    return p == std::string_view::npos ? 0 : static_cast<int>(p) + 1;
}
}  // namespace

void regvar(X13Context& ctx, const double* y, int nobpf, int fctdrp, int nfcst,
            int nbcst, const double* userx, const int* bgusrx, int nrusrx,
            int priadj, int reglom, int& nrxy, int* begxy, int& frstry,
            bool xmeans, bool elong) {
    using namespace prm;
    (void)userx;
    (void)bgusrx;
    (void)nrusrx;
    constexpr double ONE = 1.0;
    constexpr double ZERO = 0.0;
    constexpr int PLOM = 2, PLOQ = 3;
    constexpr int PXY = PLEN * (PB + 1);

    model_cmn& M = ctx.model;
    mdldat_cmn& D = ctx.mdldat;

    std::vector<double> tsrs(static_cast<std::size_t>(PLEN), 0.0);
    std::unique_ptr<bool[]> begrgm_buf(new bool[PLEN]);
    bool* begrgm = begrgm_buf.get();
    double emean[PSP];

    bool lckurg = true;
    (void)lckurg;
    addate(D.begspn.data(), M.sp, -nbcst, begxy);
    nrxy = D.nspobs + nbcst + std::max(0, nfcst - fctdrp);

    // Check the dimensions of the data and regressor variables.
    if (nrxy * M.ncxy > PXY) {
        errhdr(ctx);
        writln(ctx, " Too many elements in [X:y]", stdio::STDERR, ctx.units.mt2,
               true);
        abend(ctx);
        return;
    }

    // Add the possibly transformed data to the last column of Xy.
    frstry = nbcst * M.ncxy + 1;
    copycl(y, nobpf, 1, 1, M.ncxy, M.ncxy, D.xy.data() + (frstry - 1));

    // Observations of Xy that stand in for the backcasts are zero.
    if (nbcst > 0) {
        for (int irow = 1; irow <= nbcst; ++irow) {
            int ielt = irow * M.ncxy;
            D.xy(ielt) = ZERO;
        }
    }

    // Add each regression variable or group.
    int igrp = 0;
    int lstngp = M.ngrp;
    while (igrp < M.ngrp) {
        igrp = igrp + 1;
        setlg(true, PLEN, begrgm);

        std::string igrptl;
        int nchr;
        getstr(ctx, M.grpttl.data(), M.grpptr.data(), M.ngrp, igrp, igrptl, nchr);
        if (ctx.error.lfatal) return;
        int nigrpc = indx(std::string_view(igrptl).substr(0, static_cast<std::size_t>(nchr)), '[') - 1;
        if (nigrpc == -1) nigrpc = nchr;
        std::string_view h3 = std::string_view(igrptl).substr(0, 3);
        std::string_view h2 = std::string_view(igrptl).substr(0, 2);
        if (h3 == "AOS" || h3 == "LSS") {
            nigrpc = 3;
        } else if (h2 == "AO" || h2 == "LS" || h2 == "Rp" || h2 == "Mi" ||
                   h2 == "TC" || h2 == "SO" || h2 == "TL" || h2 == "QI" ||
                   h2 == "QD") {
            nigrpc = 2;
        }

        // Determine the beginning and ending columns in the group.
        int begcol = M.grp(igrp - 1);
        int endcol = M.grp(igrp) - 1;
        int rtype2 = M.rgvrtp(begcol);
        if (rtype2 > 100) rtype2 = rtype2 - 100;

        // Determine the type of regression variable (computed GO TO).
        // Label map for rtype2 = 1..65:
        static const int label[66] = {0,
            10, 20, 30, 40, 50, 50, 60, 70, 80, 90,
            100, 110, 120, 120, 120, 130, 130, 140, 150, 150,
            150, 150, 150, 150, 150, 150, 90, 120, 90, 155,
            155, 155, 155, 155, 155, 155, 155, 140, 120, 120,
            40, 150, 155, 120, 130, 70, 150, 155, 140, 140,
            140, 140, 140, 120, 120, 140, 140, 140, 140, 140,
            140, 140, 140, 140, 140};
        int lab = (rtype2 >= 1 && rtype2 <= 65) ? label[rtype2] : 0;

        // Change-of-regime (regvar.f:332-364): recover the regime date from the
        // group title, build the begrgm mask, then remap to the base builder so
        // the shared case handlers below produce the regime columns.
        if (lab == 150 || lab == 155) {
            std::string_view gv =
                std::string_view(igrptl).substr(0, static_cast<std::size_t>(nchr));
            int idtpos;
            int rgzero;
            if (lab == 150) {
                idtpos = idxof(gv, "(before ") + 8;
                if (idtpos == 8) idtpos = idxof(gv, "(change for before ") + 19;
                rgzero = 1;
            } else {
                idtpos = idxof(gv, "(starting ") + 10;
                if (idtpos == 10) idtpos = idxof(gv, "(change for after ") + 18;
                rgzero = -1;
            }
            int regmdt[2] = {0, 0};
            bool lok = true;
            ctodat(std::string_view(igrptl).substr(0, static_cast<std::size_t>(nchr - 1)),
                   M.sp, idtpos, regmdt, lok);
            gtrgpt(ctx, begxy, regmdt, rgzero, begrgm, nrxy);
            // Remap rtype2 to the base-effect builder label.
            static const int bl[9] = {0, 20, 30, 40, 50, 50, 60, 70, 80};
            const int base0 = (lab == 150) ? PRRTSE : PRATSE;
            if (rtype2 == PRR1TD || rtype2 == PRA1TD) lab = 40;
            else if (rtype2 == PRR1ST || rtype2 == PRA1ST) lab = 70;
            else {
                int bi = rtype2 - base0 + 1;
                lab = (bi >= 1 && bi <= 8) ? bl[bi] : 0;
            }
        }

        switch (lab) {
        case 10: {
            // Constant is a column of ones filtered by 1/Diff(B).
            setdp(ONE, nrxy, tsrs.data());
            ratpos(nrxy, D.arimap.data(), M.arimal.data(), M.opr.data(),
                   M.mdl(DIFF - 1), M.mdl(DIFF) - 1, nrxy, tsrs.data());
            copycl(tsrs.data(), nrxy, 1, 1, M.ncxy, begcol, D.xy.data());
            break;
        }
        case 20: {
            // Seasonal effects.
            addsef(ctx, begxy, nrxy, M.ncxy, begcol, endcol, D.xy.data(), begrgm);
            if (ctx.error.lfatal) return;
            break;
        }
        case 40: {
            // Trading Day effects.
            bool ltd1 = rtype2 == PRG1TD || rtype2 == PRR1TD || rtype2 == PRA1TD;
            td6var(ctx, begxy, M.sp, nrxy, M.ncxy, begcol, endcol, 0, D.xy.data(),
                   begrgm, ltd1);
            if (ctx.error.lfatal) return;
            break;
        }
        case 50: {
            // Length-of-Month and Length-of-Quarter effects.
            td7var(begxy, M.sp, nrxy, M.ncxy, begcol, true, false, false,
                   D.xy.data(), begrgm);
            break;
        }
        case 60: {
            // Leap Year effect.
            td7var(begxy, M.sp, nrxy, M.ncxy, begcol, false, false, false,
                   D.xy.data(), begrgm);
            break;
        }
        case 70: {
            // Stock Trading Day effects.
            bool ltd1 = rtype2 == PRG1ST || rtype2 == PRR1ST || rtype2 == PRA1ST;
            int ipos = nigrpc + 2;
            int smpday = ctoi(std::string_view(igrptl).substr(0, static_cast<std::size_t>(nchr)), ipos);
            td6var(ctx, begxy, M.sp, nrxy, M.ncxy, begcol, endcol, smpday,
                   D.xy.data(), begrgm, ltd1);
            if (ctx.error.lfatal) return;
            break;
        }
        case 80: {
            // Stock Length-of-Month effect.
            td7var(begxy, M.sp, nrxy, M.ncxy, begcol, true, true, false,
                   D.xy.data(), begrgm);
            break;
        }
        case 90: {
            // Easter holiday effect.
            for (int ielt = begcol; ielt <= endcol; ++ielt) {
                std::string colttl;
                getstr(ctx, M.colttl.data(), M.colptr.data(), M.nb, ielt, colttl,
                       nchr);
                if (ctx.error.lfatal) return;
                int nigc = indx(std::string_view(colttl).substr(0, static_cast<std::size_t>(nchr)), '[') - 1;
                int ipos = nigc + 2;
                int ndays = ctoi(std::string_view(colttl).substr(0, static_cast<std::size_t>(nchr)), ipos);
                estrmu(begxy, nrxy, M.sp, ndays, elong, emean, rtype2 == PRGTES);
                adestr(begxy, nrxy, M.ncxy, M.sp, ielt, ndays, M.easidx,
                       D.xy.data(), xmeans, emean, rtype2 == PRGTES);
            }
            break;
        }
        case 30: {
            // Trigonometric (sine-cosine) seasonal effects.
            adsncs(ctx, begxy, nrxy, M.ncxy, begcol, endcol, D.xy.data(), begrgm);
            if (ctx.error.lfatal) return;
            break;
        }
        case 100: {
            // Labor Day holiday effect.
            int ipos = nigrpc + 2;
            int ndays = ctoi(std::string_view(igrptl).substr(0, static_cast<std::size_t>(nchr)), ipos);
            adlabr(begxy, nrxy, M.ncxy, begcol, ndays, D.xy.data(), xmeans);
            break;
        }
        case 110: {
            // Thanksgiving-Christmas holiday effect.
            int ipos = nigrpc + 2;
            int ndays = ctoi(std::string_view(igrptl).substr(0, static_cast<std::size_t>(nchr)), ipos);
            adthnk(begxy, nrxy, M.ncxy, begcol, ndays, D.xy.data(), xmeans);
            break;
        }
        case 120:   // AO/LS/MV/TC/SO/TL/ramp regressors
        case 130:   // automatically identified outliers
            addotl(ctx, begxy, nrxy, nbcst, begcol, endcol);
            if (ctx.error.lfatal) return;
            break;   // GO TO 160
        case 140:
            not_ported(ctx, "user-defined regressors");
            return;
        case 150:
        case 155:
            // Remapped to a base builder above; reaching here means an
            // unhandled regime base type.
            not_ported(ctx, "change-of-regime base type " + std::to_string(rtype2));
            return;
        default:
            not_ported(ctx, "regression variable type " + std::to_string(rtype2));
            return;
        }

        // 160: in case a group has been deleted do not index igrp.
        if (lstngp > M.ngrp) igrp = igrp - 1;
        lstngp = M.ngrp;
    }

    // Generate length of month factors and adjust the regression variables.
    // For the regression adjustment none=1, td=2, or all=3.
    if (reglom > 1 && priadj > 1) {
        bool lom = (priadj == PLOM || priadj == PLOQ);
        // The 7th trading day factors.
        setlg(true, PLEN, begrgm);
        td7var(begxy, M.sp, nrxy, 1, 1, lom, false, true, tsrs.data(), begrgm);

        int begcol, endcol;
        if (reglom == 3) {
            begcol = 1;
            endcol = M.ncxy - 1;
        } else {
            int igrp2 = strinx(false, M.grpttl.raw(), M.grpptr.data(), 1, M.ngrptl,
                               "Trading Day");
            if (igrp2 <= 0) goto l170;
            begcol = M.grp(igrp2 - 1);
            endcol = M.grp(igrp2) - 1;
        }
        for (int irow = 1; irow <= nrxy; ++irow) {
            int begelt = M.ncxy * (irow - 1);
            int endelt = begelt + endcol;
            begelt = begelt + begcol;
            double lomadj = tsrs[static_cast<std::size_t>(irow - 1)];
            for (int ielt = begelt; ielt <= endelt; ++ielt)
                D.xy(ielt) = D.xy(ielt) * lomadj;
        }
    }
l170:
    return;
}

}  // namespace x13
