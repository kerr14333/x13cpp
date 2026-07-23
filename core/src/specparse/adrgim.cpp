// adrgim.cpp -- change-of-regime regression parser: gtrgdt.f (read the /date/
// tokens + zeroz) and adrgim.f (build the regime analogs of a full-effect group
// and rename the surviving full-effect group). See tools/x11regression_scope.md
// sibling note and the second-brain change-of-regime plan.
//
// C++ ngrp/ngrptl split (the Fortran has one Ngrp): column-pointer / Rgvrtp / Grp
// indexing uses M.ngrp; grpttl getstr/delstr/insstr title ops use M.ngrptl. They
// move in lockstep (adrgef bumps both; a rename = delstr+insstr = net-zero).
#include "specparse/specparse.hpp"

#include <string>

#include "common/x13context.hpp"
#include "specparse/lexstate.hpp"
#include "regarima/outlier.hpp"   // wrtdat
#include "gen/model.hpp"          // PRR*/PRA*/PRG* regressor types
#include "gen/notset.hpp"         // prm::DNOTST

namespace x13 {

using namespace lexprm;

// gtrgdt.f -- read the change-of-regime date and set Zeroz. The /date/ wrapper
// slashes are consumed here; a DOUBLE slash (leading // or trailing //) signals
// zeros before / after the regime date. A single trailing / is just the wrapper
// (zeroz stays 0), because getdat leaves the lookahead at that slash and the
// post-getdat lex() consumes it before the "is it another slash?" test.
static void gtrgdt(X13Context& ctx, bool havesp, int sp, int* regdat, int& zeroz,
                   bool& locok, bool& inptok) {
    LexState& L = ctx.lex;
    locok = true;
    zeroz = 0;
    if (L.nxtktp == EOFTOK) {
        locok = false;
    } else {
        lex(ctx);
        if (L.nxtktp == SLASH) {   // leading //  -> zeros before
            zeroz = -1;
            lex(ctx);
        }
        bool spf = havesp;
        int spv = sp;
        bool argok = true;
        getdat(ctx, spf, spv, regdat, argok, locok);
        if (!argok) {
            inpter(ctx, PERROR, L.errpos.data() + 1,
                   "Expected a date not \"" +
                       L.nxttok.substr(0, static_cast<std::size_t>(L.nxtkln)) +
                       "\"");
            locok = false;
        } else {
            lex(ctx);
            if (L.nxtktp == SLASH) {   // trailing //  -> zeros after
                zeroz = 1 - zeroz;
                lex(ctx);
            }
        }
    }
    inptok = inptok && locok;
}

// adrgim.f -- add change-of-regime regression variables.
void adrgim(X13Context& ctx, const int* begsrs, int nobs, bool havesp,
            std::string_view grptxt, int vartyp, int vrtyp2, int& zeroz,
            bool delreg, bool& lregim, bool fullef, bool& locok, bool& inptok) {
    using namespace prm;
    model_cmn& M = ctx.model;
    picktd_cmn& P = ctx.picktd;
    const int sp = M.sp;

    int strgim[2];
    bool argok = true;
    gtrgdt(ctx, havesp, sp, strgim, zeroz, argok, locok);
    if (!argok || ctx.error.lfatal) {
        locok = false;
        return;
    }
    const std::string rdtstr = wrtdat(strgim, sp);
    if (ctx.error.lfatal) return;

    // Validate the change date is strictly inside the series.
    int dfrgim = 0;
    dfdate(strgim, begsrs, sp, dfrgim);
    if (dfrgim <= 0 || dfrgim >= nobs) {
        inpter(ctx, PERROR, ctx.lex.lstpos.data() + 1,
               "Date given for change of regime not within the series.");
        locok = false;
        return;
    }

    // Locate the full-effect group.
    int igrp = strinx(false, M.grpttl.raw(), M.grpptr.data(), 1, M.ngrp, grptxt);
    int begcol = M.grp(igrp - 1);
    int endcol = M.grp(igrp) - 1;

    // Picktd TD: a leap-year "after" analog precedes the day-contrast analogs.
    if (zeroz == 0 && (vartyp == PRRTTD || vartyp == PRR1TD) && P.picktd) {
        adrgef(ctx, DNOTST, "Leap Year", "Leap Year (after " + rdtstr + ")",
               PRGTLY, false, true);
        if (ctx.error.lfatal) return;
    }

    // For each full-effect column, add a regime analog (built in reverse so the
    // inserted columns land in forward order after the full-effect block).
    for (int icol = endcol; icol >= begcol; --icol) {
        std::string colstr;
        int ncolcr = 0;
        getstr(ctx, M.colttl.data(), M.colptr.data(), M.ncoltl, icol, colstr,
               ncolcr);
        if (ctx.error.lfatal) return;
        colstr = colstr.substr(0, static_cast<std::size_t>(ncolcr));
        const std::string gt(grptxt);
        if (zeroz == 0 || (zeroz == 1 && fullef)) {
            adrgef(ctx, DNOTST, colstr + " I",
                   gt + " (change for before " + rdtstr + ")", vartyp, false,
                   true);
        } else if (zeroz > 0) {
            adrgef(ctx, DNOTST, colstr + " I", gt + " (before " + rdtstr + ")",
                   vartyp, false, true);
            if (zeroz == 2)
                adrgef(ctx, DNOTST, colstr + " II",
                       gt + " (starting " + rdtstr + ")", vrtyp2, false, true);
        } else if (fullef) {
            adrgef(ctx, DNOTST, colstr + " II",
                   gt + " (change for after " + rdtstr + ")", vrtyp2, false,
                   true);
        } else {
            adrgef(ctx, DNOTST, colstr + " II",
                   gt + " (starting " + rdtstr + ")", vrtyp2, false, true);
        }
        if (ctx.error.lfatal) return;
    }

    // Delete the full-effect columns when the regime effects stand alone.
    if (delreg && zeroz != 0) {
        dlrgef(ctx, begcol, nobs, endcol - begcol + 1);
        if (ctx.error.lfatal) return;
    }

    // Rename the surviving full-effect group's title (and, for the 1-coefficient
    // pairing, an adjacent related group).
    if (zeroz == 0 || fullef) {
        int varori = 0, varor1 = 0, varor2 = 0;
        if (zeroz == 0) {
            varori = M.rgvrtp(begcol);
            varor1 = varori + 1;
            varor2 = varori + 3;
        } else if (vartyp == PRR1TD) {
            varori = PRG1TD;
            varor1 = varori + 1;
            varor2 = varori + 3;
        } else if (vartyp == PRG1ST) {
            varori = PRG1ST;
        } else {
            varori = vartyp - 17;
            varor1 = varori + 1;
            varor2 = varori + 3;
        }
        int g = M.ngrp;
        while (g > 0) {
            int icol = M.grp(g - 1);
            if (M.rgvrtp(icol) == varori) {
                std::string igrptl;
                int nchr = 0;
                getstr(ctx, M.grpttl.data(), M.grpptr.data(), M.ngrptl, g,
                       igrptl, nchr);
                if (ctx.error.lfatal) return;
                igrptl = igrptl.substr(0, static_cast<std::size_t>(nchr));
                delstr(ctx, g, M.grpttl.data(), M.grpptr.data(), M.ngrptl, PGRP);
                if (ctx.error.lfatal) return;
                const std::string suf =
                    (zeroz >= 0) ? " (after " + rdtstr + ")"
                                 : " (before " + rdtstr + ")";
                insstr(ctx, igrptl + suf, g, PGRP, M.grpttl.data(),
                       M.grpttl.length, M.grpptr.data(), M.ngrptl);
                if (ctx.error.lfatal) return;
                // Adjacent related group (1-coefficient TD/stock pairing).
                if (g < M.ngrp) {
                    int icol2 = M.grp(g);
                    if (M.rgvrtp(icol2) >= varor1 && M.rgvrtp(icol2) <= varor2) {
                        std::string t2;
                        int n2 = 0;
                        getstr(ctx, M.grpttl.data(), M.grpptr.data(), M.ngrptl,
                               g + 1, t2, n2);
                        if (ctx.error.lfatal) return;
                        t2 = t2.substr(0, static_cast<std::size_t>(n2));
                        delstr(ctx, g + 1, M.grpttl.data(), M.grpptr.data(),
                               M.ngrptl, PGRP);
                        if (ctx.error.lfatal) return;
                        insstr(ctx, t2 + suf, g + 1, PGRP, M.grpttl.data(),
                               M.grpttl.length, M.grpptr.data(), M.ngrptl);
                        if (ctx.error.lfatal) return;
                    }
                }
                g = 0;
            } else {
                g = g - 1;
            }
        }
    }

    // Picktd TD: leap-year regime analogs mirror the day-contrast ones.
    if ((vartyp == PRRTTD || vartyp == PRR1TD) && P.picktd) {
        if (zeroz == 0) {
            adrgef(ctx, DNOTST, "Leap Year I",
                   "Leap Year (change for before " + rdtstr + ")", PRRTLY, false,
                   true);
        } else if (zeroz > 0) {
            adrgef(ctx, DNOTST, "Leap Year I",
                   "Leap Year (before " + rdtstr + ")", PRRTLY, false, true);
            if (zeroz == 2)
                adrgef(ctx, DNOTST, "Leap Year II",
                       "Leap Year (starting " + rdtstr + ")", PRATLY, false, true);
        } else if (fullef) {
            adrgef(ctx, DNOTST, "Leap Year II",
                   "Leap Year (change for after " + rdtstr + ")", PRATLY, false,
                   true);
        } else {
            adrgef(ctx, DNOTST, "Leap Year II",
                   "Leap Year (starting " + rdtstr + ")", PRATLY, false, true);
        }
        if (ctx.error.lfatal) return;
    }

    if (!lregim) lregim = true;
    if (vartyp == PRRTTD || vartyp == PRRTST || vartyp == PRR1TD ||
        vartyp == PRR1ST) {
        P.tddate(1) = strgim[0];
        P.tddate(2) = strgim[1];
        if ((vartyp == PRRTTD || vartyp == PRR1TD) && P.picktd) {
            P.lndate(1) = strgim[0];
            P.lndate(2) = strgim[1];
        }
    }
    if (vartyp == PRRTLM || vartyp == PRRTLQ || vartyp == PRRTLY) {
        P.lndate(1) = strgim[0];
        P.lndate(2) = strgim[1];
    }
}

}  // namespace x13
