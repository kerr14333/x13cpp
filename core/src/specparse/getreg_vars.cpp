// getreg_vars.cpp -- regression{ variables = (...) } structure builders:
// gtpdrg.f, adpdrg.f, adrgef.f, rdregm.f, rmlnvr.f, dlrgef.f.
//
// These build the regression group/column state in model.cmn (Grp/Grpttl/
// Colttl/Rgvrtp/Regfx/Nb/Ncxy + the picktd.cmn trading-day flags) that
// regvar.f consumes to fill the [X:y] design matrix.
//
// Ported slice: const, seasonal, sincos, the td family (td/tdnolpyear/
// td1coef/td1nolpyear/lom/loq/lpyear/lomstock/tdstock/tdstock1coef), easter/
// sceaster/easterstock, labor, thank. Outlier regressors (AO/LS/TC/...),
// change-of-regime (`/`), and automatic-outlier ordering paths abend loudly --
// they need rdotlr.f/adrgim.f (M4 outlier milestone).
#include "specparse/specparse.hpp"
#include "notset.hpp"
#include "srslen.hpp"
#include "gen/model.hpp"
#include "regarima/outlier.hpp"   // rdotlr (outlier title parser)
#include "numeric/numeric.hpp"    // dpeq (tcalfa default check)
#include <cmath>                  // std::pow (TC decay rate)

#include <string>

namespace x13 {

using namespace lexprm;

namespace {
// Fortran INDEX(str, pat): 1-based position of first occurrence, 0 if none.
int findx(std::string_view str, std::string_view pat) {
    std::size_t p = str.find(pat);
    return (p == std::string_view::npos) ? 0 : static_cast<int>(p + 1);
}

void not_ported(X13Context& ctx, const std::string& what) {
    errhdr(ctx);
    writln(ctx, "ERROR: " + what +
           " not yet ported (M2 regression-matrix slice).",
           stdio::STDERR, ctx.units.mt2, true);
    abend(ctx);
}
}  // namespace

// rdregm.f
void rdregm(X13Context& ctx, std::string_view rgmttl, const int* begspn, int sp,
            int& zeroz, int& rgmidx, bool& locok) {
    locok = true;
    rgmidx = 0;
    zeroz = 0;
    int nchr = static_cast<int>(rgmttl.size());
    int ipos = findx(rgmttl.substr(0, static_cast<std::size_t>(nchr)), "(starting ");
    if (ipos == 0) {
        ipos = findx(rgmttl.substr(0, static_cast<std::size_t>(nchr)), "(before ");
        if (ipos == 0) {
            ipos = findx(rgmttl.substr(0, static_cast<std::size_t>(nchr)),
                         "(change from before ");
            return;
        }
        zeroz = 1;
    } else {
        zeroz = -1;
    }
    if (zeroz == 1) ipos = ipos + 8;
    else ipos = ipos + 10;
    int begrgm[2];
    ctodat(rgmttl.substr(0, static_cast<std::size_t>(nchr - 1)), sp, ipos, begrgm,
           locok);
    if (locok) {
        dfdate(begrgm, begspn, sp, rgmidx);
        rgmidx = rgmidx + 1;
    }
}

// adrgef.f
void adrgef(X13Context& ctx, double initvl, std::string_view effttl,
            std::string_view igrptl, int vartyp, bool varfix, bool userin) {
    using namespace prm;
    model_cmn& M = ctx.model;
    mdldat_cmn& D = ctx.mdldat;

    static const char DAYDIC[] = "montuewedthufrisat";
    static const int dayptr[7] = {1, 4, 7, 10, 13, 16, 19};
    constexpr int PDAY = 6;
    static const char MONDIC[] = "janfebmaraprmayjunjulaugsepoctnov";
    static const int monptr[12] = {1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34};
    constexpr int PMON = 11;
    // dspzro(-1:1) = /PRATSE, PRGTSE, PRRTSE/
    auto dspzro = [](int z) { return z == -1 ? PRATSE : (z == 0 ? PRGTSE : PRRTSE); };

    // Find the group.  If the group doesn't exist, create it.
    int igrp = strinx(false, M.grpttl.raw(), M.grpptr.data(), 1, M.ngrp, igrptl);
    bool addcat;
    if (igrp == 0) {
        addcat = true;
        igrp = 1;
        if (M.ngrp > 0) {
            bool isotl = (vartyp == PRGTAO || vartyp == PRGTLS || vartyp == PRGTRP ||
                          vartyp == PRGTMV || vartyp == PRGTTC || vartyp == PRGTTL ||
                          vartyp == PRGTSO || vartyp == PRGTQI || vartyp == PRGTQD ||
                          vartyp == PRSQAO || vartyp == PRSQLS);
            if (isotl && !userin) {
                // Sort-by-date insertion of program-supplied outliers needs
                // rdotlr.f (outlier milestone).
                not_ported(ctx, "automatic outlier regression ordering (rdotlr.f)");
                return;
            } else if (vartyp == PRGTLM || vartyp == PRGTLQ || vartyp == PRGTLY ||
                       vartyp == PRRTLM || vartyp == PRRTLQ || vartyp == PRRTLY ||
                       vartyp == PRATLM || vartyp == PRATLQ || vartyp == PRATLY) {
                // Try to find a corresponding set of trading day regressors.
                for (int jgrp = 1; jgrp <= M.ngrp; ++jgrp) {
                    int icol = M.grp(jgrp - 1);
                    int itype = M.rgvrtp(icol);
                    if ((itype == PRGTTD || itype == PRG1TD) &&
                        (vartyp == PRGTLM || vartyp == PRGTLQ || vartyp == PRGTLY)) {
                        igrp = jgrp + 1;
                    } else if ((itype == PRRTTD || itype == PRR1TD) &&
                               (vartyp == PRRTLM || vartyp == PRRTLQ ||
                                vartyp == PRRTLY)) {
                        igrp = jgrp + 1;
                    } else if ((itype == PRATTD || itype == PRA1TD) &&
                               (vartyp == PRATLM || vartyp == PRATLQ ||
                                vartyp == PRATLY)) {
                        igrp = jgrp + 1;
                    }
                }
                if (igrp == 1) igrp = M.ngrp + 1;
            } else if (vartyp == PRGTSE || vartyp == PRGTTS || vartyp == PRGTTD ||
                       vartyp == PRGTST || vartyp == PRGTSL || vartyp == PRRTSE ||
                       vartyp == PRRTTS || vartyp == PRRTTD || vartyp == PRRTST ||
                       vartyp == PRRTSL || vartyp == PRATSE || vartyp == PRATTS ||
                       vartyp == PRATTD || vartyp == PRATST || vartyp == PRATSL ||
                       vartyp == PRG1TD || vartyp == PRR1TD || vartyp == PRA1TD ||
                       vartyp == PRG1ST || vartyp == PRR1ST || vartyp == PRA1ST) {
                // Keep change-of-regime families together, sorted by type and
                // regime date.
                int zeroz, rgmidx;
                bool locok;
                rdregm(ctx, igrptl, D.begspn.data(), M.sp, zeroz, rgmidx, locok);
                if (!locok) { abend(ctx); return; }
                bool havreg = false;
                int dsptyp = vartyp - dspzro(zeroz);
                int jgrp;
                bool jumped = false;
                for (jgrp = 1; jgrp <= M.ngrp; ++jgrp) {
                    int icol = M.grp(jgrp - 1);
                    int itype = M.rgvrtp(icol);
                    std::string tmpttl;
                    int nchr;
                    getstr(ctx, M.grpttl.data(), M.grpptr.data(), M.ngrp, jgrp,
                           tmpttl, nchr);
                    if (ctx.error.lfatal) return;
                    int zero2, rgmid2;
                    rdregm(ctx, tmpttl, D.begspn.data(), M.sp, zero2, rgmid2, locok);
                    if (!locok) { abend(ctx); return; }
                    if (itype == dsptyp + dspzro(zero2)) {
                        havreg = true;
                        if (zeroz == zero2) {
                            errhdr(ctx);
                            writln(ctx, std::string(" ERROR: ") + std::string(effttl) +
                                   " is already in the regression.",
                                   stdio::STDERR, ctx.units.mt2, true);
                            abend(ctx);
                            return;
                        } else if (zeroz == 0 || (zeroz == 1 && zero2 == -1)) {
                            jumped = true;   // GO TO 2
                            break;
                        }
                    } else if (havreg) {
                        jumped = true;       // GO TO 2
                        break;
                    }
                }
                (void)jumped;
                igrp = jgrp;   // 2 igrp=jgrp (loop leaves jgrp=Ngrp+1)
            } else {
                igrp = M.ngrp + 1;
            }
        }
        if (igrp == M.ngrp + 1) {
            putstr(ctx, igrptl, PGRP, M.grpttl.data(),
                   static_cast<int>(M.grpttl.size()), M.grpptr.data(), M.ngrptl);
            if (ctx.error.lfatal) return;
        } else {
            insstr(ctx, igrptl, igrp, PGRP, M.grpttl.data(),
                   static_cast<int>(M.grpttl.size()), M.grpptr.data(), M.ngrptl);
            if (ctx.error.lfatal) return;
        }
    } else {
        addcat = false;
    }

    if (M.nb >= PB) {
        errhdr(ctx);
        writln(ctx, std::string(" ERROR: Adding ") + std::string(effttl) +
               " exceeds the number of regression effects allowed",
               stdio::STDERR, ctx.units.mt2, true);
        writln(ctx, "        in the model (80).", stdio::STDERR, ctx.units.mt2,
               false);
        abend(ctx);
        return;
    }

    insptr(ctx, addcat, 1, igrp, PGRP, PB, M.grp.data(), M.ngrp);
    if (ctx.error.lfatal) return;

    // Find out if the column titles already exist.
    int icol = strinx(false, M.colttl.raw(), M.colptr.data(), 1, M.ncoltl, effttl);
    if (icol > 0) {
        errhdr(ctx);
        writln(ctx, std::string(" ERROR: ") + std::string(effttl) +
               " is already in the regression.",
               stdio::STDERR, ctx.units.mt2, true);
        abend(ctx);
        return;
    }

    // Column placement within the group.
    if (vartyp == PRGTAA || vartyp == PRGTAL || vartyp == PRGTAT) {
        // Sort automatically identified outliers by date (adrgef.f). Grp is
        // already updated for the new column, so scan to Grp(igrp)-2.
        int otlidx = 0, begotl = 0, endotl = 0;
        bool locok = true;
        rdotlr(ctx, std::string(effttl), D.begspn.data(), M.sp, otlidx, begotl,
               endotl, locok);
        if (!locok) { abend(ctx); return; }
        bool placed = false;
        for (icol = M.grp(igrp - 1); icol <= M.grp(igrp) - 2; ++icol) {
            std::string tmpttl;
            int nchr;
            getstr(ctx, M.colttl.data(), M.colptr.data(), M.nb, icol, tmpttl, nchr);
            if (ctx.error.lfatal) return;
            int otlid2 = 0, bgotl2 = 0, endot2 = 0;
            rdotlr(ctx, tmpttl, D.begspn.data(), M.sp, otlid2, bgotl2, endot2,
                   locok);
            if (!locok) { abend(ctx); return; }
            if (begotl == bgotl2) {
                if (otlidx == otlid2) {
                    errhdr(ctx);
                    writln(ctx, std::string(" ERROR: ") + std::string(effttl) +
                           " already exists.", stdio::STDERR, ctx.units.mt2, true);
                    abend(ctx);
                    return;
                } else if (otlidx < otlid2) {
                    placed = true;   // GO TO 10
                    break;
                }
            } else if (begotl < bgotl2) {
                placed = true;       // GO TO 10
                break;
            }
        }
        if (!placed) icol = M.grp(igrp) - 1;
    } else if (vartyp == PRGTTD || vartyp == PRGTST || vartyp == PRRTTD ||
               vartyp == PRRTST || vartyp == PRATTD || vartyp == PRATST) {
        int newreg = strinx(false, DAYDIC, dayptr, 1, PDAY, effttl.substr(0, 3));
        bool placed = false;
        for (icol = M.grp(igrp - 1); icol <= M.grp(igrp) - 2; ++icol) {
            std::string tmpttl;
            int nchr;
            getstr(ctx, M.colttl.data(), M.colptr.data(), M.nb, icol, tmpttl, nchr);
            if (ctx.error.lfatal) return;
            int ireg = strinx(false, DAYDIC, dayptr, 1, PDAY,
                              std::string_view(tmpttl).substr(0, 3));
            if (newreg == ireg) {
                errhdr(ctx);
                writln(ctx, std::string(" ERROR: ") + std::string(effttl) +
                       " already exists.", stdio::STDERR, ctx.units.mt2, true);
                abend(ctx);
                return;
            } else if (newreg < ireg) {
                placed = true;   // GO TO 10
                break;
            }
        }
        if (!placed) icol = M.grp(igrp) - 1;
    } else if (vartyp == PRGTSE || vartyp == PRRTSE || vartyp == PRATSE) {
        int ipos = 1;
        int newreg;
        if (M.sp == 12)
            newreg = strinx(false, MONDIC, monptr, 1, PMON, effttl.substr(0, 3));
        else
            newreg = ctoi(effttl, ipos);
        bool placed = false;
        for (icol = M.grp(igrp - 1); icol <= M.grp(igrp) - 2; ++icol) {
            ipos = 1;
            std::string tmpttl;
            int nchr;
            getstr(ctx, M.colttl.data(), M.colptr.data(), M.nb, icol, tmpttl, nchr);
            if (ctx.error.lfatal) return;
            int ireg;
            if (M.sp == 12)
                ireg = strinx(false, MONDIC, monptr, 1, PMON,
                              std::string_view(tmpttl).substr(0, 3));
            else
                ireg = ctoi(tmpttl, ipos);
            if (newreg == ireg) {
                errhdr(ctx);
                writln(ctx, std::string(" ERROR: ") + std::string(effttl) +
                       " already exists.", stdio::STDERR, ctx.units.mt2, true);
                abend(ctx);
                return;
            } else if (newreg < ireg) {
                placed = true;
                break;
            }
        }
        if (!placed) icol = M.grp(igrp) - 1;
    } else {
        icol = M.grp(igrp) - 1;
    }

    // 10: insert the column title and companion vectors.
    insstr(ctx, effttl, icol, PB, M.colttl.data(),
           static_cast<int>(M.colttl.size()), M.colptr.data(), M.ncoltl);
    if (ctx.error.lfatal) return;
    M.nb = M.ncoltl;
    M.ncxy = M.nb + 1;
    if (addcat) {
        double dptmp[PB];
        int itmp[PB];
        bool ltmp[PB];
        int begcol = M.grp(igrp - 1);
        int endcol = M.grp(igrp) - 1;
        for (int jcol = begcol; jcol <= endcol; ++jcol) {
            dptmp[jcol - begcol] = initvl;
            itmp[jcol - begcol] = vartyp;
            ltmp[jcol - begcol] = varfix;
        }
        insdbl(ctx, dptmp, igrp, M.grp.data(), M.ngrp, D.b.data());
        if (!ctx.error.lfatal)
            insint(ctx, itmp, igrp, M.grp.data(), M.ngrp, M.rgvrtp.data());
        if (!ctx.error.lfatal)
            inslg(ctx, ltmp, igrp, M.grp.data(), M.ngrp, M.regfx.data());
        if (ctx.error.lfatal) return;
    } else {
        if (M.nb > icol) {
            copy(D.b.data() + (icol - 1), M.nb - icol, -1, D.b.data() + icol);
            cpyint(M.rgvrtp.data() + (icol - 1), M.nb - icol, -1,
                   M.rgvrtp.data() + icol);
            copylg(M.regfx.data() + (icol - 1), M.nb - icol, -1,
                   M.regfx.data() + icol);
        }
        D.b(icol) = initvl;
        M.rgvrtp(icol) = vartyp;
        M.regfx(icol) = varfix;
    }
}

// adpdrg.f
void adpdrg(X13Context& ctx, const int* begsrs, const int* endmdl, int nobs,
            bool havsrs, bool havesp, std::string rgname, int nrgchr,
            bool x11reg, bool& havtd, bool& havhol, bool& havln, bool& havlp,
            bool& locok, bool& inptok) {
    using namespace prm;
    (void)endmdl;
    (void)havsrs;
    LexState& L = ctx.lex;
    model_cmn& M = ctx.model;
    picktd_cmn& P = ctx.picktd;

    static const char REGDIC[] =
        "constseasonalsincostdtdnolpyearlomloqlpyeartdstocklomstockeaster"
        "sceasterlaborthanktd1coeftd1nolpyeartdstock1coefeasterstock";
    static const int regptr[19] = {1, 6, 14, 20, 22, 32, 35, 38, 44, 51, 59, 65,
                                   73, 78, 83, 90, 101, 113, 124};
    constexpr int PREG = 18;
    static const char TYPDIC[] = "aolsrpmvtcsotlqiqdaoslss";
    static const int typptr[12] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 22, 25};
    constexpr int POTYPE = 11;
    static const char* day[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    static const char* cmonth[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    static const char* ordend[10] = {"th", "st", "nd", "rd", "th", "th", "th",
                                     "th", "th", "th"};
    static const int ewlim[2] = {25, 24};

    locok = true;
    int zeroz = 0;
    int tmpdat[2], ivec[1], nelt;
    bool argok;
    std::string effttl(PCOLCR, ' ');

    int regidx = strinx(false, REGDIC, regptr, 1, PREG,
                        std::string_view(rgname).substr(0, static_cast<std::size_t>(nrgchr)));
    if (x11reg && (regidx <= 3 || (regidx > 4 && regidx < 9) || regidx == 10 ||
                   regidx == 16))
        regidx = 0;

    switch (regidx) {
    case 1: {   // 10: overall constant on the AR side
        adrgef(ctx, prm::DNOTST, "Constant", "Constant", PRGTCN, false, true);
        if (ctx.error.lfatal) return;
        lex(ctx);
        break;
    }
    case 2: {   // 20: seasonal effects
        lex(ctx);
        if (!havesp) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "No seasonal period specified to determine seasonal effects.");
            locok = false;
        } else if (M.sp == 1) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Seasonal effects with nonseasonal data.");
            locok = false;
        } else if (L.nxtktp != SLASH && (M.lseff || M.lseadf || M.lidsdf)) {
            if (M.lidsdf)
                inpter(ctx, PERROR, L.lstpos.data() + 1,
                       "Already have a seasonal difference in the identify spec.");
            else
                inpter(ctx, PERROR, L.lstpos.data() + 1,
                       "Already have seasonal effects or seasonal difference.");
            locok = false;
        } else if (M.lrgmse && L.nxtktp == SLASH) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Already have change of regime seasonal effects.");
            locok = false;
        } else {
            if (!M.lseff) {
                int spm1 = M.sp - 1;
                if (M.sp == 12) {
                    for (int i = 1; i <= spm1; ++i) {
                        adrgef(ctx, prm::DNOTST, cmonth[i - 1], "Seasonal", PRGTSE,
                               false, true);
                        if (ctx.error.lfatal) return;
                    }
                } else {
                    for (int i = 1; i <= spm1; ++i) {
                        std::string t(PCOLCR, ' ');
                        int ipos = 1;
                        itoc(ctx, i, t, ipos);
                        if (ctx.error.lfatal) return;
                        const char* suf = (i % 100 >= 11 && i % 100 <= 13)
                                              ? "th" : ordend[i % 10];
                        t.replace(static_cast<std::size_t>(ipos - 1), 2, suf);
                        int nchr = ipos + 1;
                        adrgef(ctx, prm::DNOTST,
                               std::string_view(t).substr(0, static_cast<std::size_t>(nchr)),
                               "Seasonal", PRGTSE, false, true);
                        if (ctx.error.lfatal) return;
                    }
                }
            }
            if (L.nxtktp == SLASH) {
                not_ported(ctx, "change-of-regime seasonal regressors (adrgim.f)");
                return;
            }
        }
        if (locok && zeroz == 0) M.lseff = true;
        if (ctx.error.lfatal) return;
        break;
    }
    case 3: {   // 30: seasonal sine-cosine
        lex(ctx);
        int isncos[PSP / 2];
        int nsncos;
        getivc(ctx, LBRAKT, true, PSP / 2, isncos, nsncos, locok, inptok);
        if (ctx.error.lfatal) return;
        if (nsncos <= 0) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Must specify the sine-cosine term explicitly.");
            locok = false;
        } else if (!havesp) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "No seasonal period specified to determine seasonal effects.");
            locok = false;
        } else if (M.sp == 1) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Seasonal effects with nonseasonal data.");
        } else if (L.nxtktp != SLASH && (M.lseff || M.lseadf || M.lidsdf)) {
            if (M.lidsdf)
                inpter(ctx, PERROR, L.lstpos.data() + 1,
                       "Already have a seasonal difference in the identify spec");
            else
                inpter(ctx, PERROR, L.lstpos.data() + 1,
                       "Already have seasonal effects or seasonal difference");
            locok = false;
        } else if (L.nxtktp == SLASH && M.lrgmse) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Already have change of regime seasonal effects.");
            locok = false;
        } else {
            if (!M.lseff) {
                if (isncos[nsncos - 1] == M.sp / 2) nsncos = 2 * nsncos - 1;
                else nsncos = 2 * nsncos;
                for (int i = 2; i <= nsncos + 1; i += 2) {
                    std::string t(PCOLCR, ' ');
                    t.replace(0, 8, "cos(2pi*");
                    int ipos = 9;
                    itoc(ctx, isncos[i / 2 - 1], t, ipos);
                    if (ctx.error.lfatal) return;
                    t.replace(static_cast<std::size_t>(ipos - 1), 2, "t/");
                    ipos = ipos + 2;
                    itoc(ctx, M.sp, t, ipos);
                    if (ctx.error.lfatal) return;
                    t[static_cast<std::size_t>(ipos - 1)] = ')';
                    adrgef(ctx, prm::DNOTST,
                           std::string_view(t).substr(0, static_cast<std::size_t>(ipos)),
                           "Trigonometric Seasonal", PRGTTS, false, true);
                    if (ctx.error.lfatal) return;
                    if (isncos[i / 2 - 1] < M.sp / 2) {
                        std::string s(PCOLCR, ' ');
                        s.replace(0, 8, "sin(2pi*");
                        ipos = 9;
                        itoc(ctx, isncos[i / 2 - 1], s, ipos);
                        if (ctx.error.lfatal) return;
                        s.replace(static_cast<std::size_t>(ipos - 1), 2, "t/");
                        ipos = ipos + 2;
                        itoc(ctx, M.sp, s, ipos);
                        if (ctx.error.lfatal) return;
                        s[static_cast<std::size_t>(ipos - 1)] = ')';
                        adrgef(ctx, prm::DNOTST,
                               std::string_view(s).substr(0, static_cast<std::size_t>(ipos)),
                               "Trigonometric Seasonal", PRGTTS, false, true);
                        if (ctx.error.lfatal) return;
                    }
                    if (isncos[i / 2 - 1] > M.sp / 2 || isncos[i / 2 - 1] < 1) {
                        errhdr(ctx);
                        writln(ctx, " ERROR: Cannot have a sin-cos variable pair.",
                               stdio::STDERR, ctx.units.mt2, true);
                        locok = false;
                        break;
                    }
                }
            }
            if (L.nxtktp == SLASH) {
                not_ported(ctx,
                           "change-of-regime trigonometric seasonal regressors "
                           "(adrgim.f)");
                return;
            }
            if (ctx.error.lfatal) return;
        }
        if (locok && zeroz == 0) M.lseff = true;
        break;
    }
    case 4: case 5: case 6: case 7: case 8: case 10: case 15: case 16: {
        // 40: td family (td/tdnolpyear/lom/loq/lpyear/lomstock/td1coef/
        //     td1nolpyear)
        lex(ctx);
        if (!havesp) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "No seasonal period specified in series spec.");
            locok = false;
        } else if (M.sp != 12 && M.sp != 4) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   " Need monthly or quarterly data for trading day");
            locok = false;
        } else if (begsrs[0] < 1776) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "No trading variables before 1776.  Try including the century in");
            writln(ctx, "        the start date", ctx.units.mt2, stdio::STDERR, false);
            locok = false;
        } else {
            if (regidx == 4 || regidx == 5 || regidx == 15 || regidx == 16) {
                if (M.isrflw == 2) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Cannot use flow trading day regressors for stock series.");
                    locok = false;
                } else if (P.fulltd && !(L.nxtktp == SLASH)) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Already have trading day effects.");
                    locok = false;
                } else if (L.nxtktp == SLASH && P.lrgmtd) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Already have change of regime trading day effects.");
                    locok = false;
                } else {
                    if (!P.fulltd) {
                        P.picktd = (regidx == 4 || regidx == 15);
                        if (regidx == 15 || regidx == 16) {
                            adrgef(ctx, prm::DNOTST, "Weekday",
                                   "1-Coefficient Trading Day", PRG1TD, false, true);
                            if (ctx.error.lfatal) return;
                        } else {
                            for (int i = 1; i <= 6; ++i) {
                                adrgef(ctx, prm::DNOTST, day[i - 1], "Trading Day",
                                       PRGTTD, false, true);
                                if (ctx.error.lfatal) return;
                            }
                        }
                    }
                    if (L.nxtktp == SLASH) {
                        not_ported(ctx,
                                   "change-of-regime trading day regressors "
                                   "(adrgim.f)");
                        return;
                    } else {
                        P.fulltd = true;
                    }
                }
            }
            // Seventh trading day variables: lom, loq, lpyear, lomstock.
            if ((regidx == 6 || regidx == 7 || regidx == 8) && P.picktd) {
                inpter(ctx, PERROR, L.lstpos.data() + 1,
                       "Can't add a length of month, quarter, or leap year variable "
                       "when using");
                writln(ctx, "        the td or td1coef option.", ctx.units.mt2,
                       stdio::STDERR, false);
                locok = false;
            } else if (regidx == 6 || regidx == 7) {
                if (havlp) {
                    if (M.sp == 12)
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "Can't add a length of month variable when using the "
                               "leap year");
                    else
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "Can't add a length of quarter variable when using the "
                               "leap year");
                    writln(ctx, "        variable.", ctx.units.mt2, stdio::STDERR,
                           false);
                    locok = false;
                } else if (M.isrflw == 2) {
                    if (regidx == 6)
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "Cannot use flow length of month regressor for stock "
                               "series.");
                    else
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "Cannot use flow length of quarter regressor for stock "
                               "series.");
                    locok = false;
                } else if (M.sp == 12) {
                    if (!P.fullln) {
                        adrgef(ctx, prm::DNOTST, "Length-of-Month", "Length-of-Month",
                               PRGTLM, false, true);
                        if (ctx.error.lfatal) return;
                    }
                    if (L.nxtktp == SLASH) {
                        not_ported(ctx,
                                   "change-of-regime length-of-month regressors "
                                   "(adrgim.f)");
                        return;
                    }
                } else {
                    if (!P.fullln) {
                        adrgef(ctx, prm::DNOTST, "Length-of-Quarter",
                               "Length-of-Quarter", PRGTLQ, false, true);
                        if (ctx.error.lfatal) return;
                    }
                    if (L.nxtktp == SLASH) {
                        not_ported(ctx,
                                   "change-of-regime length-of-quarter regressors "
                                   "(adrgim.f)");
                        return;
                    }
                }
                if (zeroz == 0) P.fullln = true;
            } else if ((!havtd && (regidx == 4 || regidx == 15)) || regidx == 8) {
                if (havln) {
                    if (M.sp == 12)
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "Can't add a leap year variable when using the length "
                               "of month");
                    else
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "Can't add a leap year variable when using the length "
                               "of quarter");
                    writln(ctx, "        variable.", ctx.units.mt2, stdio::STDERR,
                           false);
                    locok = false;
                } else if (M.isrflw == 2) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Cannot use flow leap year regressor for stock series.");
                    locok = false;
                } else {
                    if (!P.fulllp && !(P.lrgmtd && P.picktd)) {
                        adrgef(ctx, prm::DNOTST, "Leap Year", "Leap Year", PRGTLY,
                               false, true);
                    }
                    if (ctx.error.lfatal) return;
                    if (regidx == 8) {
                        if (L.nxtktp == SLASH) {
                            not_ported(ctx,
                                       "change-of-regime leap year regressors "
                                       "(adrgim.f)");
                            return;
                        }
                        if (zeroz == 0) P.fulllp = true;
                    }
                }
            } else if (regidx == 10) {
                if (M.isrflw == 1) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Cannot use stock length of month regressor for flow "
                           "series.");
                    locok = false;
                } else if (havlp) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Can't add a stock length of month variable when using the");
                    writln(ctx, "        leap year variable.", ctx.units.mt2,
                           stdio::STDERR, false);
                    locok = false;
                } else if (M.sp != 12) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Need monthly data for stock trading day");
                    locok = false;
                    lex(ctx);
                } else {
                    if (!P.fullln) {
                        adrgef(ctx, prm::DNOTST, "Stock Length-of-Month",
                               "Stock Length-of-Month", PRGTSL, false, true);
                    }
                    if (ctx.error.lfatal) return;
                    if (L.nxtktp == SLASH) {
                        not_ported(ctx,
                                   "change-of-regime stock length-of-month regressors "
                                   "(adrgim.f)");
                        return;
                    }
                }
                if (zeroz == 0) P.fullln = true;
            }
            if (ctx.error.lfatal) return;
            if ((regidx == 4 || regidx == 5 || regidx == 15 || regidx == 16) && locok)
                havtd = true;
            if ((regidx == 6 || regidx == 7 || regidx == 10) && locok) havln = true;
            if (regidx == 8 && locok) havlp = true;
        }
        break;
    }
    case 9: case 17: {   // 50: first six stock trading day effects
        lex(ctx);
        if (!havesp) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "No seasonal period specified in series spec.");
            locok = false;
        } else if (M.sp != 12) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Need monthly data for stock trading day");
            locok = false;
        } else if (begsrs[0] < 1776) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "No trading variables before 1776.  Try including the century in");
            writln(ctx, "        the start date.", ctx.units.mt2, stdio::STDERR,
                   false);
            locok = false;
        } else if (M.isrflw == 1) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Cannot use stock trading day regressors for flow series.");
            locok = false;
        }
        getivc(ctx, LBRAKT, true, 1, ivec, nelt, argok, locok);
        if (ctx.error.lfatal) return;
        {
            int tdspdy = ivec[0];
            if (nelt <= 0) {
                inpter(ctx, PERROR, L.lstpos.data() + 1,
                       "Must specify the Stock TD sample day explicitly");
                locok = false;
            } else if (tdspdy <= 0 || tdspdy > 31) {
                inpter(ctx, PERROR, L.lstpos.data() + 1,
                       "Stock TD sample day must be (1:31)");
                locok = false;
            } else {
                std::string tgrptl(PGRPCR, ' ');
                tgrptl.replace(0, 18, "Stock Trading Day[");
                int ipos = 19;
                itoc(ctx, tdspdy, tgrptl, ipos);
                if (ctx.error.lfatal) return;
                tgrptl[static_cast<std::size_t>(ipos - 1)] = ']';
                int nchr = ipos;
                if (P.fulltd && !(L.nxtktp == SLASH)) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Already have stock trading day effects.");
                    locok = false;
                } else if (L.nxtktp == SLASH && P.lrgmtd) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Already have change of regime stock trading day effects.");
                    locok = false;
                } else {
                    if (!P.fulltd) {
                        if (regidx == 17) {
                            adrgef(ctx, prm::DNOTST, "Weekday",
                                   "1-Coefficient " + tgrptl.substr(0, static_cast<std::size_t>(nchr)),
                                   PRG1ST, false, true);
                        } else {
                            for (int i = 1; i <= 6; ++i) {
                                adrgef(ctx, prm::DNOTST, day[i - 1],
                                       std::string_view(tgrptl).substr(0, static_cast<std::size_t>(nchr)),
                                       PRGTST, false, true);
                                if (ctx.error.lfatal) return;
                            }
                        }
                    }
                    if (L.nxtktp == SLASH) {
                        not_ported(ctx,
                                   "change-of-regime stock trading day regressors "
                                   "(adrgim.f)");
                        return;
                    } else {
                        P.fulltd = true;
                    }
                    havtd = true;
                    if (ctx.error.lfatal) return;
                }
            }
        }
        break;
    }
    case 11: case 12: case 18: {   // 60: easter / sceaster / easterstock
        lex(ctx);
        if (regidx == 18) M.easidx = 0;
        else M.easidx = regidx - 11;
        if (!havesp) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "No seasonal period specified in series spec.");
            locok = false;
        } else if (M.sp != 12 && M.sp != 4) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   " Need monthly or quarterly data for an Easter effect");
            locok = false;
        } else if (begsrs[0] < 1901) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "No Easter effect before 1901.  Try including the century in the");
            writln(ctx, "        start date.", ctx.units.mt2, stdio::STDERR, false);
            locok = false;
        } else if (M.isrflw == 2 && regidx < 18) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Cannot use Easter regressor for stock series.");
            locok = false;
        } else if (M.isrflw == 1 && regidx == 18) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Cannot use stock Easter regressor for flow series.");
            locok = false;
        }
        addate(begsrs, M.sp, nobs - 1, tmpdat);
        if (tmpdat[0] > 2100) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Cannot compute holiday effect after 2100");
            locok = false;
        }
        getivc(ctx, LBRAKT, true, 1, ivec, nelt, argok, locok);
        if (ctx.error.lfatal) return;
        {
            int neastr = ivec[0];
            if (nelt <= 0) {
                inpter(ctx, PERROR, L.errpos.data() + 1,
                       "Must specify the Easter window length explicitly");
                locok = false;
            } else if (neastr < (0 + M.easidx) || neastr > ewlim[M.easidx]) {
                if (M.easidx == 0)
                    inpter(ctx, PERROR, L.errpos.data() + 1,
                           "The Easter window must be from 0 to 25.");
                else
                    inpter(ctx, PERROR, L.errpos.data() + 1,
                           "The Statistics Canada Easter window must be from 1 to 24.");
                locok = false;
            } else {
                int ipos;
                if (M.easidx == 0) {
                    if (regidx == 18) {
                        effttl.replace(0, 12, "StockEaster[");
                        ipos = 13;
                    } else {
                        effttl.replace(0, 7, "Easter[");
                        ipos = 8;
                    }
                } else {
                    effttl.replace(0, 14, "StatCanEaster[");
                    ipos = 15;
                }
                itoc(ctx, neastr, effttl, ipos);
                if (ctx.error.lfatal) return;
                effttl[static_cast<std::size_t>(ipos - 1)] = ']';
                int nchr = ipos;
                std::string_view ettl =
                    std::string_view(effttl).substr(0, static_cast<std::size_t>(nchr));
                if (M.easidx == 0) {
                    if (regidx == 18)
                        adrgef(ctx, prm::DNOTST, ettl, "StockEaster", PRGTES, false,
                               true);
                    else
                        adrgef(ctx, prm::DNOTST, ettl, "Easter", PRGTEA, false, true);
                } else {
                    adrgef(ctx, prm::DNOTST, ettl, "StatCanEaster", PRGTEC, false,
                           true);
                }
                if (ctx.error.lfatal) return;
                havhol = true;
            }
        }
        break;
    }
    case 13: {   // 70: labor day
        lex(ctx);
        int igrp = strinx(true, M.grpttl.raw(), M.grpptr.data(), 1, M.ngrptl,
                          "Labor");
        if (!havesp) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "No seasonal period specified in series spec.");
            locok = false;
        } else if (M.sp != 12) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Need monthly data for a Labor Day effect");
            locok = false;
        } else if (begsrs[0] < 1901) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "No Labor Day effect before 1901.  Try including the century in");
            writln(ctx, "        the start date", ctx.units.mt2, stdio::STDERR, false);
            locok = false;
        } else if (M.isrflw == 2) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Cannot use Labor Day regressor for stock series.");
            locok = false;
        } else if (igrp > 0) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "A Labor Day regressor is already included in the regARIMA model.");
            locok = false;
        }
        addate(begsrs, M.sp, nobs - 1, tmpdat);
        if (tmpdat[0] > 2100) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Cannot compute holiday effect after 2100");
            locok = false;
        }
        getivc(ctx, LBRAKT, true, 1, ivec, nelt, argok, locok);
        if (ctx.error.lfatal) return;
        {
            int nlabor = ivec[0];
            if (nelt <= 0) {
                inpter(ctx, PERROR, L.errpos.data() + 1,
                       "Must specify the Labor Day window length explicitly");
                locok = false;
            } else if (nlabor <= 0 || nlabor > 25) {
                inpter(ctx, PERROR, L.errpos.data() + 1,
                       "The Labor Day window must be from 1 to 25.");
                locok = false;
            } else {
                effttl.replace(0, 6, "Labor[");
                int ipos = 7;
                itoc(ctx, nlabor, effttl, ipos);
                if (ctx.error.lfatal) return;
                effttl[static_cast<std::size_t>(ipos - 1)] = ']';
                int nchr = ipos;
                std::string_view ettl =
                    std::string_view(effttl).substr(0, static_cast<std::size_t>(nchr));
                adrgef(ctx, prm::DNOTST, ettl, ettl, PRGTLD, false, true);
                if (ctx.error.lfatal) return;
                havhol = true;
            }
        }
        break;
    }
    case 14: {   // 80: thanksgiving-christmas
        lex(ctx);
        int igrp = strinx(true, M.grpttl.raw(), M.grpptr.data(), 1, M.ngrptl,
                          "Thanksgiving");
        if (!havesp) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "No seasonal period specified in series spec.");
            locok = false;
        } else if (M.sp != 12) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Need monthly data for a Thanksgiving-Christmas day effect");
            locok = false;
        } else if (begsrs[0] < 1939) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Thanksgiving-Christmas day effect not defined before 1939.");
            if (begsrs[0] < 100)
                writln(ctx, "        Try including the century in the start date.",
                       ctx.units.mt2, stdio::STDERR, false);
            locok = false;
        } else if (M.isrflw == 2) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Cannot use Thanksgiving-Christmas regressor for stock series.");
            locok = false;
        } else if (igrp > 0) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "A Thanksgiving-Christmas regressor is already included ");
            writln(ctx, "        in the regARIMA model.", ctx.units.mt2,
                   stdio::STDERR, false);
            locok = false;
        }
        addate(begsrs, M.sp, nobs - 1, tmpdat);
        if (tmpdat[0] > 2100) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Cannot compute holiday effect after 2100");
            locok = false;
        }
        getivc(ctx, LBRAKT, true, 1, ivec, nelt, argok, locok);
        if (ctx.error.lfatal) return;
        {
            int nthank = ivec[0];
            if (nelt <= 0) {
                inpter(ctx, PERROR, L.errpos.data() + 1,
                       "Must specify the Thanksgiving day window length explicitly");
                locok = false;
            } else if (nthank < -8 || nthank > 17 || nthank == 0) {
                inpter(ctx, PERROR, L.errpos.data() + 1,
                       "The Thanksgiving day window must be from -8 to 17 "
                       "(excluding 0)");
            } else {
                effttl.replace(0, 13, "Thanksgiving[");
                int ipos = 14;
                itoc(ctx, nthank, effttl, ipos);
                if (ctx.error.lfatal) return;
                effttl[static_cast<std::size_t>(ipos - 1)] = ']';
                int nchr = ipos;
                std::string_view ettl =
                    std::string_view(effttl).substr(0, static_cast<std::size_t>(nchr));
                adrgef(ctx, prm::DNOTST, ettl, ettl, PRGTTH, false, true);
                if (ctx.error.lfatal) return;
                havhol = true;
            }
        }
        break;
    }
    default: {
        // AO, LS, Ramp, or error.
        int typidx = strinx(false, TYPDIC, typptr, 1, POTYPE,
                            std::string_view(rgname).substr(0, 3));
        if (typidx == 0)
            typidx = strinx(false, TYPDIC, typptr, 1, POTYPE,
                            std::string_view(rgname).substr(0, 2));
        if (x11reg && typidx > 1) typidx = 0;
        if (typidx > 0) {
            // AOS/LSS (typidx 10/11) need rdotls.f (not ported); the rest are
            // user-specified point/level/ramp outliers -- parse the date with
            // rdotlr, validate the window, and register the group (regvar case
            // 120 builds the column via addotl). adpdrg.f:90-360.
            if (typidx >= 10) {
                not_ported(ctx, "AOS/LSS outlier regressors (rdotls.f)");
                return;
            }
            int otlind = typidx, begotl = 0, endotl = 0;
            bool argok = true;
            rdotlr(ctx, rgname.substr(0, static_cast<std::size_t>(nrgchr)), begsrs,
                   M.sp, otlind, begotl, endotl, argok);
            if (ctx.error.lfatal) return;
            if (!argok) {
                inpter(ctx, PERROR, L.lstpos.data() + 1,
                       "See the above AO, LS, RP, SO, TL, TC, QI, or QD error.");
                locok = false;
                lex(ctx);
                break;
            }
            // Per-type window validation + the type prefix / vartyp.
            const char* pfx;
            int vartyp;
            bool bad = false;
            const char* msg = "Not within series";
            switch (typidx) {
            case 1: pfx = "AO"; vartyp = PRGTAO; bad = begotl > nobs || begotl < 1; break;
            case 4: pfx = "MV"; vartyp = PRGTMV; bad = begotl > nobs || begotl < 1; break;
            case 2: pfx = "LS"; vartyp = PRGTLS; bad = begotl > nobs - 1 || begotl < 2; break;
            case 5: pfx = "TC"; vartyp = PRGTTC; bad = begotl > nobs || begotl < 1; break;
            case 6: pfx = "SO"; vartyp = PRGTSO; bad = begotl > nobs || begotl < 1; break;
            case 8: pfx = "QI"; vartyp = PRGTQI; bad = begotl > nobs || begotl < 1; break;
            case 9: pfx = "QD"; vartyp = PRGTQD; bad = begotl > nobs || begotl < 1; break;
            case 3:  // ramp: endpoint within series, ordered
                pfx = "RP"; vartyp = PRGTRP;
                if (endotl > nobs) { bad = true; msg = "End of ramp not within series"; }
                else if (begotl < 1) { bad = true; msg = "Beginning of ramp not within series"; }
                else if (endotl <= begotl) { bad = true; msg = "Beginning and end of ramp reversed"; }
                break;
            default:  // 7 = TL (temporary level shift)
                pfx = "TL"; vartyp = PRGTTL;
                if (endotl > nobs) { bad = true; msg = "End of temporary LS not within series"; }
                else if (begotl < 1) { bad = true; msg = "Beginning of temporary LS not within series"; }
                else if (endotl <= begotl) { bad = true; msg = "Beginning and end of temporary LS reversed"; }
                break;
            }
            if (bad) {
                inpter(ctx, PERROR, L.lstpos.data() + 1, msg);
                locok = false;
                lex(ctx);
                break;
            }
            // A user TC needs the temporary-change decay rate; default it as the
            // outlier{} path does (0.7^(12/sp)) when not set, so addotl builds the
            // geometric-decay column instead of a degenerate one.
            if (vartyp == PRGTTC && dpeq(M.tcalfa, prm::DNOTST))
                M.tcalfa = std::pow(0.7, 12.0 / M.sp);
            // Normalize the two-char type prefix (adpdrg.f Rgname(1:2)=pfx).
            rgname[0] = pfx[0];
            rgname[1] = pfx[1];
            std::string_view rttl =
                std::string_view(rgname).substr(0, static_cast<std::size_t>(nrgchr));
            adrgef(ctx, prm::DNOTST, rttl, rttl, vartyp, false, true);
            if (ctx.error.lfatal) return;
            lex(ctx);   // 210: consume the outlier token (caller loop re-tests)
            break;
        }
        // Not an ao, ls, or rp.
        if (x11reg)
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Irregular Component Regression variable name \"" +
                   rgname.substr(0, static_cast<std::size_t>(nrgchr)) +
                   "\" not found");
        else
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Regression variable name \"" +
                   rgname.substr(0, static_cast<std::size_t>(nrgchr)) +
                   "\" not found");
        // 200
        locok = false;
        lex(ctx);
        break;
    }
    }

    // 230
    inptok = inptok && locok;
}

// gtpdrg.f
void gtpdrg(X13Context& ctx, const int* begsrs, const int* endmdl, int nobs,
            bool havsrs, bool havesp, bool x11reg, bool& havtd, bool& havhol,
            bool& havln, bool& havlp, bool& locok, bool& inptok) {
    LexState& L = ctx.lex;
    locok = true;
    bool argok;
    if (L.nxtktp != EOFTOK) {
        if (L.nxtktp == NAME || L.nxtktp == QUOTE) {
            ctx.captured.regression_vars.push_back(cur_tok(ctx));
            adpdrg(ctx, begsrs, endmdl, nobs, havsrs, havesp, cur_tok(ctx),
                   L.nxtkln, x11reg, havtd, havhol, havln, havlp, argok, locok);
            if (ctx.error.lfatal) return;
        } else if (L.nxtktp != LPAREN) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Expected regression variable name or \"(\" but found \"" +
                   cur_tok(ctx) + "\"");
            lex(ctx);
            locok = false;
        } else {
            bool hvcmma = false;
            bool opngrp = true;
            lex(ctx);
            while (true) {
                bool broke = false;
                if (L.nxtktp != RPAREN && L.nxtktp != EOFTOK) {
                    if (L.nxtktp == COMMA) {
                        if (hvcmma || opngrp) {
                            inpter(ctx, PERROR, L.lstpos.data() + 1,
                                   "Found a NULL value; check your commas.");
                            locok = false;
                        }
                        lex(ctx);
                        hvcmma = true;
                        opngrp = false;
                        continue;   // GO TO 10
                    }
                    if (L.nxtktp != NAME && L.nxtktp != QUOTE) {
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "Expected regression variable name or \")\" but "
                               "found \"" + cur_tok(ctx) + "\"");
                        locok = false;
                        skplst(ctx, RPAREN);
                    } else {
                        ctx.captured.regression_vars.push_back(cur_tok(ctx));
                        adpdrg(ctx, begsrs, endmdl, nobs, havsrs, havesp,
                               cur_tok(ctx), L.nxtkln, x11reg, havtd, havhol,
                               havln, havlp, argok, locok);
                        if (ctx.error.lfatal) return;
                        hvcmma = false;
                        opngrp = false;
                        continue;   // GO TO 20 (outer loop re-test)
                    }
                } else if (hvcmma) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Found a NULL value; check your commas.");
                    locok = false;
                }
                lex(ctx);
                broke = true;   // GO TO 30
                if (broke) break;
            }
        }
    }
    // 30: overall error checks
    inptok = inptok && locok;
}

// rmlnvr.f
void rmlnvr(X13Context& ctx, int& priadj, int kfulsm, int nspobs) {
    model_cmn& M = ctx.model;
    picktd_cmn& P = ctx.picktd;
    // Reset prior adjustment variable.
    if (priadj == 0) {
        if (kfulsm == 2) {
            if (M.sp == 12) priadj = 2;
            else if (M.sp == 4) priadj = 3;
        } else {
            priadj = 4;
        }
    }
    // Check for length-of-period or leap year regressor and remove it.
    int icol = 1;
    while (icol > 0) {
        icol = strinx(true, M.colttl.raw(), M.colptr.data(), 1, M.ncoltl,
                      "Length-of-");
        if (icol == 0)
            icol = strinx(true, M.colttl.raw(), M.colptr.data(), 1, M.ncoltl,
                          "Leap Year");
        if (icol > 0) {
            dlrgef(ctx, icol, nspobs, 1);
            if (ctx.error.lfatal) return;
        }
    }
    if (P.lndate(1) != prm::NOTSET) {
        P.lnzero = 0;
        setint(prm::NOTSET, 2, P.lndate.data());
    }
}

// dlrgef.f
void dlrgef(X13Context& ctx, int begcol, int nrxy, int ndelc) {
    using namespace prm;
    model_cmn& M = ctx.model;
    mdldat_cmn& D = ctx.mdldat;

    int noldc = M.ncxy;
    int endcol = begcol + ndelc - 1;
    if (begcol < 1 || endcol > M.nb) {
        errhdr(ctx);
        writln(ctx, " ERROR: Deleted columns not within the regression matrix.",
               stdio::STDERR, ctx.units.mt2, true);
        abend(ctx);
        return;
    }
    // Delete the column titles for the deleted columns.
    for (int i = endcol; i >= begcol; --i) {
        delstr(ctx, i, M.colttl.data(), M.colptr.data(), M.ncoltl, PB);
        if (ctx.error.lfatal) return;
    }
    // Delete the regression coefficients and the regression types.
    int e1 = endcol + 1;
    if (e1 <= PB) {
        copy(D.b.data() + (e1 - 1), noldc - 1 - endcol, 1, D.b.data() + (begcol - 1));
        cpyint(M.rgvrtp.data() + (e1 - 1), noldc - 1 - endcol, 1,
               M.rgvrtp.data() + (begcol - 1));
        copylg(M.regfx.data() + (e1 - 1), noldc - 1 - endcol, 1,
               M.regfx.data() + (begcol - 1));
    }
    // Compress Xy row by row.
    M.ncxy = noldc - ndelc;
    M.nb = M.ncxy - 1;
    int iend = begcol - 1;
    for (int i = 1; i <= nrxy - 1; ++i) {
        int noffst = i * ndelc;
        int ibeg = iend + 1;
        iend = iend + M.ncxy;
        for (int j = ibeg; j <= iend; ++j) D.xy(j) = D.xy(j + noffst);
    }
    {
        int noffst = nrxy * ndelc;
        int ibeg = iend + 1;
        for (int j = ibeg; j <= nrxy * M.ncxy; ++j) D.xy(j) = D.xy(j + noffst);
    }
    // Update the grp and grpttl indices.
    int noffst = 0;
    int nloop = M.ngrp;
    int ntdelc = ndelc;
    for (int igrp = 1; igrp <= nloop; ++igrp) {
        int ibeg = M.grp(igrp - 1);
        int gend = M.grp(igrp) - 1;
        if (gend >= begcol && ntdelc > 0) {
            int ncol;
            eltlen(ctx, igrp, M.grp.data(), M.ngrp, ncol);
            if (ctx.error.lfatal) return;
            int idelc;
            if (ntdelc > 0) {
                idelc = std::min(gend, begcol + ntdelc - 1) - std::max(ibeg, begcol) + 1;
                ncol = ncol - idelc;
                ntdelc = ntdelc - idelc;
                if (ntdelc > 0) begcol = ibeg + ncol;
            } else {
                idelc = 0;
            }
            if (ncol > 0) {
                for (int i = igrp + noffst; i <= M.ngrp; ++i)
                    M.grp(i) = M.grp(i) - idelc;
            } else {
                delstr(ctx, igrp, M.grpttl.data(), M.grpptr.data(), M.ngrptl, PGRP);
                if (ctx.error.lfatal) return;
                M.ngrp = M.ngrp - 1;
                noffst = noffst - 1;
                for (int i = igrp; i <= M.ngrp; ++i)
                    M.grp(i) = M.grp(i + 1) - idelc;
            }
        }
    }
}

}  // namespace x13
