// readers_val.cpp -- typed value-list readers and their support routines:
// intlst.f, insptr.f, putstr.f, getivc.f, gtdpvc.f, gtdtvc.f, getdat.f,
// ctodat.f, gtnmvc.f, getttl.f, gtdcvc.f, and the print/save consumers
// getprt.f/getsav.f/getsvl.f (token-faithful; table-dictionary application
// deferred -- see report).
#include "specparse/specparse.hpp"
#include "notset.hpp"
#include "srslen.hpp"
#include "gen/model.hpp"   // prm::PB (gtrgvl's bvec/fixvec bound)
#include "gen/tbltab.hpp"  // prm::LEVEL (getprt.f:207's level fill)

#include <string>

namespace x13 {

using namespace lexprm;

// intlst.f
void intlst(int pelt, int* ptrvec, int& nstr) {
    (void)pelt;
    nstr = 0;
    ptrvec[0] = 1;
    ptrvec[1] = 1;
}

// insptr.f  (ptrvec is 0-based DIMENSION(0:Pelt))
void insptr(X13Context& ctx, bool addcat, int niunit, int ielt, int pelt,
            int nunit, int* ptrvec, int& nelt) {
    ptrvec[0] = 1;
    int disp = addcat ? 1 : 0;
    if (nelt + disp > pelt) { abend(ctx); return; }
    if (ptrvec[nelt] + niunit - 1 > nunit) { abend(ctx); return; }
    if (ielt > nelt + disp || ielt < 1) { abend(ctx); if (ctx.error.lfatal) return; }
    else {
        for (int i = nelt; i >= ielt - disp; --i) ptrvec[i + disp] = ptrvec[i] + niunit;
    }
    if (addcat) nelt = nelt + disp;
}

// putstr.f
void putstr(X13Context& ctx, std::string_view str, int pstr, std::string& chrvec,
            int* ptrvec, int& nstr) {
    insptr(ctx, true, static_cast<int>(str.size()), nstr + 1, pstr,
           static_cast<int>(chrvec.size()), ptrvec, nstr);
    if (!ctx.error.lfatal) {
        int b = ptrvec[nstr - 1];
        int e = ptrvec[nstr] - 1;
        for (int i = b; i <= e; ++i)
            chrvec[static_cast<std::size_t>(i - 1)] = str[static_cast<std::size_t>(i - b)];
    }
}

// Helper: report "<label> vector exceeds N, the maximum number of elements."
static void exceed_err(X13Context& ctx, const int* ptr, const char* label, int pelt) {
    std::string s = std::string(label) + " exceeds ";
    int ipos = static_cast<int>(s.size()) + 1;  // itoc uses 1-based position
    s.resize(LINLEN, ' ');
    itoc(ctx, pelt, s, ipos);
    std::string tail = ", the maximum number of elements.";
    s.replace(static_cast<std::size_t>(ipos - 1), tail.size(), tail);
    ipos += static_cast<int>(tail.size());
    inpter(ctx, PERROR, ptr, std::string_view(s).substr(0, static_cast<std::size_t>(ipos - 1)));
}

// getivc.f
void getivc(X13Context& ctx, int grpchr, bool flgnul, int pelt, int* avec,
            int& nelt, bool& locok, bool& inptok) {
    LexState& L = ctx.lex;
    locok = true;
    if (L.nxtktp == EOFTOK) {
        locok = false;
    } else if (getint(ctx, avec[0])) {
        nelt = 1;
    } else if (L.nxtktp != grpchr) {
        inpter(ctx, PERROR, L.lstpos.data() + 1,
               "Expected an integer or an integer list, not \"" + cur_tok(ctx) + "\"");
        locok = false;
        lex(ctx);
    } else {
        nelt = 0;
        bool opngrp = true, hvcmma = false;
        int clsgtp = clsgrp(grpchr);
        lex(ctx);
        while (true) {
            bool broke = false;
            while (true) {
                if (L.nxtktp != clsgtp) {
                    if (L.nxtktp == COMMA) {
                        if (hvcmma || opngrp) {
                            if (flgnul) {
                                inpter(ctx, PERROR, L.lstpos.data() + 1,
                                       "Found a NULL value; check your commas.");
                                locok = false;
                            } else if (nelt >= pelt) {
                                exceed_err(ctx, L.lstpos.data() + 1, "Integer vector", pelt);
                                locok = false;
                            } else {
                                nelt = nelt + 1;
                                avec[nelt - 1] = prm::NOTSET;
                            }
                        }
                        lex(ctx);
                        hvcmma = true;
                        opngrp = false;
                        continue;   // GO TO 10
                    }
                    int tmp;
                    if (!getint(ctx, tmp)) {
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "Expected an integer not \"" + cur_tok(ctx) + "\"");
                        locok = false;
                    } else if (nelt >= pelt) {
                        exceed_err(ctx, L.lstpos.data() + 1, "Integer vector", pelt);
                        locok = false;
                    } else {
                        nelt = nelt + 1;
                        avec[nelt - 1] = tmp;
                        hvcmma = false;
                        opngrp = false;
                        broke = true; break;   // GO TO 20
                    }
                } else if (hvcmma && !opngrp) {
                    if (flgnul) {
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "Found a NULL value; check your commas.");
                        locok = false;
                    } else if (nelt >= pelt) {
                        exceed_err(ctx, L.lstpos.data() + 1, "Integer vector", pelt);
                        locok = false;
                    } else {
                        nelt = nelt + 1;
                        avec[nelt - 1] = prm::NOTSET;
                    }
                } else if (opngrp && flgnul) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Found a NULL value; check for null list.");
                    locok = false;
                }
                if (locok) lex(ctx); else skplst(ctx, clsgtp);
                break;   // GO TO 30
            }
            if (broke) continue;   // loop again after adding element
            break;                 // reached 30
        }
    }
    inptok = inptok && locok;
}

// gtdpvc.f
void gtdpvc(X13Context& ctx, int grpchr, bool flgnul, int pelt, double* avec,
            int& nelt, bool& locok, bool& inptok) {
    LexState& L = ctx.lex;
    locok = true;
    if (L.nxtktp == EOFTOK) {
        locok = false;
    } else if (getdbl(ctx, avec[0])) {
        nelt = 1;
    } else if (L.nxtktp != grpchr) {
        inpter(ctx, PERROR, L.lstpos.data() + 1,
               "Expected a real number or a list of real numbers, not \"" + cur_tok(ctx) + "\"");
        locok = false;
        lex(ctx);
    } else {
        nelt = 0;
        bool opngrp = true, hvcmma = false;
        int clsgtp = clsgrp(grpchr);
        lex(ctx);
        while (true) {
            bool broke = false;
            while (true) {
                if (L.nxtktp != clsgtp) {
                    if (L.nxtktp == COMMA) {
                        if (hvcmma || opngrp) {
                            if (flgnul) {
                                inpter(ctx, PERROR, L.lstpos.data() + 1,
                                       "Found a NULL value; check your commas.");
                                locok = false;
                            } else if (nelt >= pelt) {
                                exceed_err(ctx, L.lstpos.data() + 1, "Real vector", pelt);
                                locok = false;
                            } else {
                                nelt = nelt + 1;
                                avec[nelt - 1] = prm::DNOTST;
                            }
                        }
                        lex(ctx);
                        hvcmma = true;
                        opngrp = false;
                        continue;
                    }
                    double tmp;
                    if (!getdbl(ctx, tmp)) {
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "Expected a real number not \"" + cur_tok(ctx) + "\"");
                        locok = false;
                    } else if (nelt >= pelt) {
                        exceed_err(ctx, L.lstpos.data() + 1, "Real vector", pelt);
                        locok = false;
                    } else {
                        nelt = nelt + 1;
                        avec[nelt - 1] = tmp;
                        hvcmma = false;
                        opngrp = false;
                        broke = true; break;
                    }
                } else if (hvcmma && !opngrp) {
                    if (flgnul) {
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "Found a NULL value; check your commas.");
                        locok = false;
                    } else if (nelt >= pelt) {
                        exceed_err(ctx, L.lstpos.data() + 1, "Real vector", pelt);
                        locok = false;
                    } else {
                        nelt = nelt + 1;
                        avec[nelt - 1] = prm::DNOTST;
                    }
                } else if (opngrp && flgnul) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Found a NULL value; check for null list.");
                    locok = false;
                }
                if (locok) lex(ctx); else skplst(ctx, clsgtp);
                break;
            }
            if (broke) continue;
            break;
        }
    }
    inptok = inptok && locok;
}

// ---------------------------------------------------------------------------
// gtinvl.f / gtrgvl.f -- the INITIAL/FIXED coefficient readers.
//
// Both parse a list of reals in which each value may carry a trailing `f`
// (fixed) or `e` (estimated) name token: `ma = (0.55f, 0.35f)`,
// `b = (0.05f, -0.04)`. FIXDIC is 'fe' and FIXVAL is 1, so the flag is
// `fixidx == 1`. A NULL element (a bare comma) advances the cursor WITHOUT
// writing either array -- the Fortran's `Bvec(ielt)=PTONE` /
// `Arimap(ielt)=PTONE` writebacks are commented out upstream, so the slot
// keeps whatever the model builder put there (DNOTST for "estimate me").
// That is what makes `b = (,0.05f)` mean "leave column 1 alone".
//
// gtdcnm leaves the token unconsumed when it is a NAME that is not in the
// dictionary, so a following argument name (`b = 0.05 print = ...`) is seen
// by gtarg as usual -- but note it also reports argok=true there, hence the
// Fortran's `IF(argok)` writes a FALSE flag in that case. Transcribed.
// ---------------------------------------------------------------------------
namespace {

constexpr int FIXVAL = 1;
constexpr char FIXDIC[] = "fe";
constexpr int fixptr[3] = {1, 2, 3};
constexpr int PFIX = 2;

// The trailing f/e flag, shared by both readers (gtinvl.f:57-59 and its
// three repetitions, gtrgvl.f:60-62 and its two).
void read_fixflag(X13Context& ctx, bool& fixflg) {
    int fixidx = 0;
    bool argok = true;
    gtdcnm(ctx, FIXDIC, fixptr, PFIX, fixidx, argok);
    if (argok) fixflg = (fixidx == FIXVAL);
}

}  // namespace

// gtinvl.f -- ARIMA operator coefficients (arima{ diff= / ar= / ma= }).
// Optype is gtarma.f:67's `argidx-2`, i.e. prm::DIFF / AR / MA, and indexes
// Mdl -> Opr to find the lag range this argument fills.
void gtinvl(X13Context& ctx, int optype, bool& inptok) {
    LexState& L = ctx.lex;
    model_cmn& M = ctx.model;
    mdldat_cmn& D = ctx.mdldat;

    // OPDIC/opptr only exist to name the operator in the error message.
    static const char* const OPNAME[3] = {"diff", "ar", "ma"};
    const std::string opnm =
        (optype >= 1 && optype <= 3) ? OPNAME[optype - 1] : "";
    const std::string errmsg =
        "Number of initial values must equal sum of all the " + opnm +
        " parameters in all the factors.";

    const int begopr = M.mdl(optype - 1);
    const int endopr = M.mdl(optype) - 1;
    const int beglag = M.opr(begopr - 1);
    const int endlag = M.opr(endopr) - 1;
    int ielt = beglag - 1;

    bool locok = true;
    bool hvcmma = false;
    double tmp = 0.0;

    if (L.nxtktp == EOFTOK) {
        locok = false;
    } else if (getdbl(ctx, tmp)) {
        // Only a single value (gtinvl.f:54-59). Note the Fortran does NOT
        // bound-check this branch against endlag; the tail check at :30 does.
        ++ielt;
        D.arimap(ielt) = tmp;
        read_fixflag(ctx, M.arimaf(ielt));
    } else if (L.nxtktp != LPAREN) {
        inpter(ctx, PERROR, L.lstpos.data() + 1,
               "Expected a real number or a list of real numbers, not \"" +
               cur_tok(ctx) + "\"");
        locok = false;
    } else {
        bool opngrp = true;
        lex(ctx);
        while (true) {
            bool broke = false;
            if (L.nxtktp != RPAREN) {
                if (L.nxtktp == COMMA) {
                    if (hvcmma || opngrp) {
                        if (ielt >= endlag) {
                            inpter(ctx, PERROR, L.errpos.data() + 1, errmsg);
                            locok = false;
                        } else {
                            ++ielt;   // NULL: leave Arimap(ielt) alone
                        }
                    }
                    lex(ctx);
                    hvcmma = true;
                    opngrp = false;
                    continue;
                }
                if (!getdbl(ctx, tmp)) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Expected an real number not \"" + cur_tok(ctx) + "\"");
                    locok = false;
                } else if (ielt >= endlag) {
                    inpter(ctx, PERROR, L.errpos.data() + 1, errmsg);
                    locok = false;
                } else {
                    ++ielt;
                    D.arimap(ielt) = tmp;
                    read_fixflag(ctx, M.arimaf(ielt));
                    hvcmma = false;
                    opngrp = false;
                    broke = true;
                }
            } else if (hvcmma && !opngrp) {
                if (ielt >= endlag) {
                    inpter(ctx, PERROR, L.errpos.data() + 1, errmsg);
                    locok = false;
                } else {
                    ++ielt;
                }
            }
            if (broke) continue;
            if (locok) lex(ctx); else skplst(ctx, RPAREN);
            break;
        }
    }
    // gtinvl.f:30 -- an empty list `ma=()` (ielt still beglag-1) is as if the
    // argument had not been given; anything short of a full fill is an error.
    if (ielt > beglag - 1 && ielt != endlag) {
        inpter(ctx, PERROR, L.errpos.data() + 1, errmsg);
        locok = false;
    }
    inptok = inptok && locok;
}

// gtrgvl.f -- regression{ b= }. Unlike gtinvl this writes CALLER-owned
// scratch (bvec/fixvec), because Nb is not final until gtpdrg has run every
// variables= group: getreg.f:519-553 does the writeback into B/Regfx at the
// parse tail. It also has no bound check of its own -- the tail's
// `nbvec != Nb+Ncusrx` test is what rejects a wrong-length list -- so the
// PB cap here can only bite on a list longer than any admissible model,
// which that test rejects anyway.
void gtrgvl(X13Context& ctx, int& ielt, bool* fixvec, double* bvec,
            bool& inptok) {
    LexState& L = ctx.lex;
    ielt = 0;
    bool locok = true;
    bool hvcmma = false;
    double tmp = 0.0;

    const auto put = [&](double v, bool haveval) {
        ++ielt;
        if (ielt >= 1 && ielt <= prm::PB && haveval) bvec[ielt - 1] = v;
    };

    if (L.nxtktp == EOFTOK) {
        locok = false;
    } else if (getdbl(ctx, tmp)) {
        put(tmp, true);
        if (ielt >= 1 && ielt <= prm::PB) read_fixflag(ctx, fixvec[ielt - 1]);
    } else if (L.nxtktp != LPAREN) {
        inpter(ctx, PERROR, L.lstpos.data() + 1,
               "Expected a real number or a list of real numbers, not \"" +
               cur_tok(ctx) + "\"");
        locok = false;
    } else {
        bool opngrp = true;
        lex(ctx);
        while (true) {
            bool broke = false;
            if (L.nxtktp != RPAREN) {
                if (L.nxtktp == COMMA) {
                    if (hvcmma || opngrp) put(0.0, false);  // NULL: skip a slot
                    lex(ctx);
                    hvcmma = true;
                    opngrp = false;
                    continue;
                }
                if (!getdbl(ctx, tmp)) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Expected an real number not \"" + cur_tok(ctx) + "\"");
                    locok = false;
                } else {
                    put(tmp, true);
                    if (ielt >= 1 && ielt <= prm::PB)
                        read_fixflag(ctx, fixvec[ielt - 1]);
                    hvcmma = false;
                    opngrp = false;
                    broke = true;
                }
            } else if (hvcmma && !opngrp) {
                put(0.0, false);
            }
            if (broke) continue;
            if (locok) lex(ctx); else skplst(ctx, RPAREN);
            break;
        }
    }
    inptok = inptok && locok;
}

// ctodat.f  (idate[0]=year, idate[1]=period)
void ctodat(std::string_view str, int sp, int& ipos, int* idate, bool& locok) {
    static const char* MODIC = "JanFebMarAprMayJunJulAugSepOctNovDec";
    static const int moptr[13] = {1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37};
    locok = true;
    int lstpt = ipos;
    idate[0] = ctoi(str, ipos);
    idate[1] = 0;
    if (ipos < static_cast<int>(str.size())) {
        if (str[static_cast<std::size_t>(ipos - 1)] == '.') {
            ipos = ipos + 1;
            if (indx("0123456789", str[static_cast<std::size_t>(ipos - 1)]) > 0) {
                idate[1] = ctoi(str, ipos);
            } else {
                std::string_view mo = str.substr(static_cast<std::size_t>(ipos - 1), 3);
                idate[1] = strinx(false, MODIC, moptr, 1, 12, mo);
                if (idate[1] > 0) {
                    if (sp == 12) ipos = ipos + 3;
                    else { locok = false; ipos = lstpt; }
                }
            }
        }
    }
    if (!isdate(idate, sp)) { locok = false; ipos = lstpt; }
}

// getdat.f
void getdat(X13Context& ctx, bool& havesp, int& sp, int* idate, bool& argok, bool& inptok) {
    LexState& L = ctx.lex;
    argok = true;
    int ipos = 1;
    int llstps[2] = {L.lstpos[PLINE], L.lstpos[PCHAR]};
    std::string datstr(11, ' ');
    int nchr = 1;
    if (L.nxtktp == INTGR) {
        if (havesp && sp != 1) {
            inpter(ctx, PERRNP, L.lstpos.data() + 1,
                   "Invalid date, seasonal period of data not annual.");
            argok = false;
        } else {
            if (!havesp) { havesp = true; sp = 1; }
            nchr = L.nxtkln;
            datstr = cur_tok(ctx);
            datstr.resize(11, ' ');
        }
    } else if (L.nxtktp == DBL) {
        if (L.nxttok[static_cast<std::size_t>(L.nxtkln - 1)] != '.') {
            if (havesp && sp == 1) {
                inpter(ctx, PERROR, llstps, "Invalid date, no period for nonseasonal data");
                argok = false;
            } else if (!havesp) {
                sp = prm::PSP;
            }
            nchr = L.nxtkln;
            datstr = cur_tok(ctx);
            datstr.resize(11, ' ');
        } else if (havesp && sp != 12) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "Invalid date, seasonal period of data not monthly.");
            argok = false;
        } else {
            if (!havesp) { havesp = true; sp = 12; }
            nchr = L.nxtkln;
            datstr = cur_tok(ctx);
            datstr.resize(11, ' ');
            lex(ctx);
            if (L.nxtktp != NAME) {
                inpter(ctx, PERROR, llstps, "Invalid date, expected a monthly abbreviation");
                argok = false;
            } else {
                std::string t = cur_tok(ctx);
                for (int k = 0; k < L.nxtkln; ++k) datstr[static_cast<std::size_t>(nchr + k)] = t[static_cast<std::size_t>(k)];
                nchr = nchr + L.nxtkln;
            }
        }
    }
    if (argok) {
        ctodat(datstr.substr(0, static_cast<std::size_t>(nchr)), sp, ipos, idate, argok);
        if (!argok) {
            if (L.nxtktp == QUOTE)
                inpter(ctx, PERROR, L.lstpos.data() + 1, "Not a valid date - remove quotes.");
            else
                inpter(ctx, PERROR, L.lstpos.data() + 1, "Not a valid date");
        }
    }
    lex(ctx);
    inptok = argok && inptok;
}

// gtdtvc.f  (Datvec is int[2][Pelt] column-major -> avec[(k-1)*2 + (i-1)])
void gtdtvc(X13Context& ctx, bool& havesp, int& sp, int grpchr, bool flgnul,
            int pelt, int* avec, int& nelt, bool& locok, bool& inptok) {
    LexState& L = ctx.lex;
    const int defval[2] = {prm::NOTSET, prm::NOTSET};
    locok = true;
    if (L.nxtktp == EOFTOK) {
        locok = false;
    } else if (L.nxtktp != grpchr) {
        bool argok;
        getdat(ctx, havesp, sp, avec, argok, locok);
        if (argok) {
            nelt = 1;
        } else if (L.nxtktp != grpchr) {
            inpter(ctx, PERROR, L.errpos.data() + 1, "Expected a date or a list of dates");
            locok = false;
        }
    } else {
        nelt = 0;
        bool opngrp = true, hvcmma = true;
        int clsgtp = clsgrp(grpchr);
        lex(ctx);
        while (true) {
            bool broke = false;
            if (L.nxtktp != clsgtp) {
                if (L.nxtktp == COMMA) {
                    if (hvcmma || opngrp) {
                        if (flgnul) {
                            inpter(ctx, PERROR, L.errpos.data() + 1, "Found a NULL date; check your commas.");
                            locok = false;
                        } else if (nelt >= pelt) {
                            exceed_err(ctx, L.errpos.data() + 1, "Date vector", pelt);
                            locok = false;
                        } else {
                            nelt = nelt + 1;
                            avec[(nelt - 1) * 2 + 0] = defval[0];
                            avec[(nelt - 1) * 2 + 1] = defval[1];
                        }
                    }
                    lex(ctx);
                    hvcmma = true;
                    opngrp = false;
                    continue;
                }
                int tmpdat[2];
                bool argok;
                getdat(ctx, havesp, sp, tmpdat, argok, locok);
                if (!argok) {
                    inpter(ctx, PERROR, L.errpos.data() + 1, "Expected a date not \"" + cur_tok(ctx) + "\"");
                    locok = false;
                } else if (nelt >= pelt) {
                    exceed_err(ctx, L.errpos.data() + 1, "Date vector", pelt);
                    locok = false;
                } else {
                    nelt = nelt + 1;
                    avec[(nelt - 1) * 2 + 0] = tmpdat[0];
                    avec[(nelt - 1) * 2 + 1] = tmpdat[1];
                    hvcmma = false;
                    opngrp = false;
                    broke = true;
                }
            } else if (hvcmma && !opngrp) {
                if (flgnul) {
                    inpter(ctx, PERROR, L.errpos.data() + 1, "Found a NULL date; check your commas.");
                    locok = false;
                } else if (nelt >= pelt) {
                    exceed_err(ctx, L.errpos.data() + 1, "Date vector", pelt);
                    locok = false;
                } else {
                    nelt = nelt + 1;
                    avec[(nelt - 1) * 2 + 0] = defval[0];
                    avec[(nelt - 1) * 2 + 1] = defval[1];
                }
            }
            if (broke) continue;   // getdat already advanced; loop (GO TO 10)
            if (locok) lex(ctx); else skplst(ctx, clsgtp);
            break;   // GO TO 20
        }
    }
    inptok = inptok && locok;
}

// gtnmvc.f
void gtnmvc(X13Context& ctx, int grpchr, bool flgnul, int pelt, std::string& chrvec,
            int* ptrvec, int& nelt, int maxchr, bool& locok, bool& inptok) {
    LexState& L = ctx.lex;
    locok = true;
    intlst(pelt, ptrvec, nelt);
    if (L.nxtktp == EOFTOK) {
        locok = false;
    } else if (L.nxtktp == NAME || L.nxtktp == QUOTE) {
        if (L.nxtkln == 0) {
            if (L.nxtktp == NAME)
                inpter(ctx, PERROR, L.lstpos.data() + 1,
                       "Expected a NAME, QUOTE, or list of either, not an empty string.");
            locok = false;
        } else {
            if (pelt == 1 && L.nxtkln > static_cast<int>(chrvec.size())) {
                std::string s1(5, ' '); int n1 = 1;
                itoc(ctx, static_cast<int>(chrvec.size()), s1, n1);
                inpter(ctx, PERROR, L.lstpos.data() + 1,
                       "Values for this argument cannot be longer than " +
                       s1.substr(0, static_cast<std::size_t>(n1 - 1)) + " characters.");
                locok = false;
            } else {
                putstr(ctx, cur_tok(ctx), pelt, chrvec, ptrvec, nelt);
                if (ctx.error.lfatal) return;
            }
        }
        lex(ctx);
    } else if (L.nxtktp != grpchr) {
        inpter(ctx, PERROR, L.lstpos.data() + 1,
               "Expected a NAME or a QUOTE or a list of either, not \"" + cur_tok(ctx) + "\"");
        locok = false;
        lex(ctx);
    } else {
        bool opngrp = true, hvcmma = false;
        int clsgtp = clsgrp(grpchr);
        while (true) {
            lex(ctx);
            bool broke = false;
            if (L.nxtktp != clsgtp) {
                if (L.nxtktp == COMMA) {
                    if (hvcmma || opngrp) {
                        if (flgnul) {
                            inpter(ctx, PERROR, L.lstpos.data() + 1, "Found a NULL value; check your commas.");
                            locok = false;
                        } else if (nelt >= pelt) {
                            exceed_err(ctx, L.lstpos.data() + 1, "List of names", pelt);
                            locok = false;
                        } else {
                            putstr(ctx, std::string(1, prm::CNOTST), pelt, chrvec, ptrvec, nelt);
                            if (ctx.error.lfatal) return;
                        }
                    }
                    hvcmma = true;
                    opngrp = false;
                    continue;
                }
                if (L.nxtktp != NAME && L.nxtktp != QUOTE) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1, "Expected a NAME or QUOTE not \"" + cur_tok(ctx) + "\"");
                    locok = false;
                } else if (nelt >= pelt) {
                    exceed_err(ctx, L.lstpos.data() + 1, "List of names", pelt);
                    locok = false;
                } else {
                    if (L.nxtkln == 0) {
                        if (L.nxtktp == NAME)
                            inpter(ctx, PERROR, L.lstpos.data() + 1,
                                   "Expected a NAME, QUOTE, or list of either, not an empty string.");
                        locok = false;
                    } else if (pelt > 1 && L.nxtkln > maxchr) {
                        std::string s1(5, ' '); int n1 = 1;
                        itoc(ctx, maxchr, s1, n1);
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "Values for this argument cannot be longer than " +
                               s1.substr(0, static_cast<std::size_t>(n1 - 1)) + " characters.");
                        locok = false;
                    } else {
                        putstr(ctx, cur_tok(ctx), pelt, chrvec, ptrvec, nelt);
                        if (ctx.error.lfatal) return;
                    }
                    hvcmma = false;
                    opngrp = false;
                    broke = true;
                }
            } else if (hvcmma && !opngrp) {
                if (flgnul) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1, "Found a NULL value; check your commas.");
                    locok = false;
                } else if (nelt >= pelt) {
                    exceed_err(ctx, L.lstpos.data() + 1, "List of names", pelt);
                    locok = false;
                } else {
                    putstr(ctx, std::string(1, prm::CNOTST), pelt, chrvec, ptrvec, nelt);
                    if (ctx.error.lfatal) return;
                }
            }
            if (broke) continue;
            if (locok) lex(ctx); else skplst(ctx, clsgtp);
            break;
        }
    }
    inptok = inptok && locok;
}

// getttl.f  (like gtnmvc but truncates over-long single titles with a warning)
void getttl(X13Context& ctx, int grpchr, bool flgnul, int pelt, std::string& chrvec,
            int* ptrvec, int& nelt, bool& locok, bool& inptok) {
    LexState& L = ctx.lex;
    locok = true;
    intlst(pelt, ptrvec, nelt);
    if (L.nxtktp == EOFTOK) {
        locok = false;
    } else if (L.nxtktp == NAME || L.nxtktp == QUOTE) {
        if (L.nxtkln == 0) {
            if (L.nxtktp == NAME)
                inpter(ctx, PERROR, L.lstpos.data() + 1,
                       "Expected a NAME, QUOTE, or list of either, not an empty string.");
            locok = false;
        } else {
            int clen = static_cast<int>(chrvec.size());
            if (pelt == 1 && L.nxtkln > clen) {
                std::string s1(5, ' '); int n1 = 1;
                itoc(ctx, clen, s1, n1);
                inpter(ctx, PWARN, L.lstpos.data() + 1,
                       "This title will be truncated at the first " +
                       s1.substr(0, static_cast<std::size_t>(n1 - 1)) + " characters.");
                putstr(ctx, cur_tok(ctx).substr(0, static_cast<std::size_t>(clen)), pelt, chrvec, ptrvec, nelt);
            } else {
                putstr(ctx, cur_tok(ctx), pelt, chrvec, ptrvec, nelt);
                if (ctx.error.lfatal) return;
            }
        }
        lex(ctx);
    } else if (L.nxtktp != grpchr) {
        inpter(ctx, PERROR, L.lstpos.data() + 1,
               "Expected a NAME or a QUOTE or a list of either, not \"" + cur_tok(ctx) + "\"");
        locok = false;
        lex(ctx);
    } else {
        // List form (unused by the corpus titles); reuse gtnmvc-style consumption.
        bool opngrp = true, hvcmma = false;
        int clsgtp = clsgrp(grpchr);
        while (true) {
            lex(ctx);
            bool broke = false;
            if (L.nxtktp != clsgtp) {
                if (L.nxtktp == COMMA) {
                    if ((hvcmma || opngrp) && !flgnul && nelt < pelt)
                        putstr(ctx, std::string(1, prm::CNOTST), pelt, chrvec, ptrvec, nelt);
                    hvcmma = true; opngrp = false; continue;
                }
                if (L.nxtktp != NAME && L.nxtktp != QUOTE) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1, "Expected a NAME or QUOTE not \"" + cur_tok(ctx) + "\"");
                    locok = false;
                } else {
                    putstr(ctx, cur_tok(ctx), pelt, chrvec, ptrvec, nelt);
                    if (ctx.error.lfatal) return;
                    hvcmma = false; opngrp = false; broke = true;
                }
            }
            if (broke) continue;
            if (locok) lex(ctx); else skplst(ctx, clsgtp);
            break;
        }
    }
    inptok = inptok && locok;
}

// gtdcvc.f
void gtdcvc(X13Context& ctx, int grpchr, bool flgnul, int pelt, std::string_view dic,
            const int* dicptr, int ndic, std::string_view errmsg, int* avec,
            int& nelt, bool& locok, bool& inptok) {
    LexState& L = ctx.lex;
    locok = true;
    nelt = 0;
    bool hvcmma = false;
    if (L.nxtktp == EOFTOK) {
        locok = false;
    } else if (L.nxtktp == NAME || L.nxtktp == QUOTE) {
        int argidx; bool argok;
        gtdcnm(ctx, dic, dicptr, ndic, argidx, argok);
        if (!argok || argidx == 0) {
            inpter(ctx, PERROR, L.lstpos.data() + 1, errmsg);
            locok = false;
        } else {
            nelt = 1;
            avec[nelt - 1] = argidx;
        }
        locok = locok && argok;
    } else if (L.nxtktp != grpchr) {
        inpter(ctx, PERROR, L.lstpos.data() + 1,
               "Expected a name or a quote or a list of names or quotes, not \"" + cur_tok(ctx) + "\"");
        locok = false;
        lex(ctx);
    } else {
        bool opngrp = true;
        int clsgtp = clsgrp(grpchr);
        lex(ctx);
        while (true) {
            bool broke = false;
            if (L.nxtktp != clsgtp) {
                if (L.nxtktp == COMMA) {
                    if (!(hvcmma || opngrp)) {
                        lex(ctx);
                    } else if (flgnul) {
                        inpter(ctx, PERROR, L.lstpos.data() + 1, "Found a NULL value; check your commas.");
                        lex(ctx);
                        locok = false;
                    } else if (nelt >= pelt) {
                        exceed_err(ctx, L.lstpos.data() + 1, "List of names", pelt);
                        locok = false;
                    } else {
                        int argidx; bool argok;
                        gtdcnm(ctx, dic, dicptr, ndic, argidx, argok);
                        if (!argok || argidx == 0) {
                            inpter(ctx, PERROR, L.lstpos.data() + 1, errmsg);
                            lex(ctx);
                            locok = false;
                        }
                        nelt = nelt + 1;
                        avec[nelt - 1] = argidx;
                        locok = locok && argok;
                    }
                    hvcmma = true; opngrp = false; continue;
                }
                if (L.nxtktp != NAME && L.nxtktp != QUOTE) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1, "Expected a NAME or QUOTE not \"" + cur_tok(ctx) + "\"");
                    locok = false;
                } else if (nelt >= pelt) {
                    exceed_err(ctx, L.lstpos.data() + 1, "List of names", pelt);
                    locok = false;
                } else {
                    int argidx; bool argok;
                    gtdcnm(ctx, dic, dicptr, ndic, argidx, argok);
                    if (!argok || argidx == 0) {
                        inpter(ctx, PERROR, L.lstpos.data() + 1, errmsg);
                        lex(ctx);
                        locok = false;
                    }
                    nelt = nelt + 1;
                    avec[nelt - 1] = argidx;
                    locok = locok && argok;
                    hvcmma = false; opngrp = false; broke = true;
                }
            } else if (hvcmma && !opngrp) {
                if (flgnul) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1, "Found a NULL value; check your commas.");
                    locok = false;
                } else if (nelt >= pelt) {
                    exceed_err(ctx, L.lstpos.data() + 1, "List of names", pelt);
                    locok = false;
                } else {
                    int argidx; bool argok;
                    gtdcnm(ctx, dic, dicptr, ndic, argidx, argok);
                    if (!argok || argidx == 0) {
                        inpter(ctx, PERROR, L.lstpos.data() + 1, errmsg);
                        lex(ctx);
                        locok = false;
                    }
                    nelt = nelt + 1;
                    avec[nelt - 1] = argidx;
                    locok = locok && argok;
                }
            }
            if (broke) continue;
            if (locok) lex(ctx); else skplst(ctx, clsgtp);
            break;
        }
    }
    inptok = inptok && locok;
}

// --- print / save / savelog readers --------------------------------------
// All three VALIDATE against the calling spec's own slice of a shared
// dictionary. getprt's and getsav's STORES are now ported too (entry 110):
// Prttab carries getprt.f:205-208's level() fill, Savtab getsav.f:57. Only
// Svltab's store is still deferred -- it has no reader in this engine.
//
// Almost none of the store is gated: `deftab`'s defaults, the tblmsk guard and
// Savtab all mutate to zero failures on this corpus. The one slot with an
// observable consumer is LESTIE, read back as Lprier by gt_estimate.
//
// getprt.f:28 -- the five print LEVELS, tried BEFORE the table dictionary and
// shared by every spec. `alltables` and `all` are distinct levels here, unlike
// savelog's single `all`.
static const char LVLDIC[] = "defaultnonebriefalltablesall";
static const int lvlptr[6] = {1, 8, 12, 17, 26, 29};
constexpr int NLVL = 5;

// getprt.f:75-91 / getsav.f:35-46 -- one table name, already known not to be a
// level. Reports the spec-appropriate refusal and consumes the token.
static void tbl_lookup(X13Context& ctx, bool save, int lsp, int nsp,
                       bool& locok, std::vector<std::string>* cap = nullptr,
                       int* tblout = nullptr) {
    LexState& L = ctx.lex;
    // The name has to be taken BEFORE the lookup: gtdcnm consumes the token on
    // a hit. `run_pre_model`'s wants_save() reads this list, so validating the
    // save argument must not cost the capture the old consumer provided.
    if (cap && (L.nxtktp == NAME || L.nxtktp == QUOTE)) {
        std::string s = cur_tok(ctx);
        if (L.nxtktp == NAME)
            for (char& c : s)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        cap->push_back(std::move(s));
    }
    // getprt.f:97 / getsav.f:56 -- the dictionary index counts LONG and SHORT
    // names, two per table, so the table is `Spcdsp+(tblidx+1)/2`.
    const int idx = tbldic_lookup(ctx, save, lsp, nsp);
    if (idx != 0) {
        const int tbl = lsp + (idx + 1) / 2;
        if (tblout) *tblout = tbl;
        if (save) ctx.tbllog.savtab(tbl) = true;   // getsav.f:57
        return;
    }
    if (save) {
        inpter(ctx, PERROR, L.lstpos.data() + 1, "Save argument is not defined.");
        writln(ctx, "        Check the available table names for this spec.",
               stdio::STDERR, ctx.units.mt2, false);
    } else {
        inpter(ctx, PERROR, L.lstpos.data() + 1,
               "Print or level argument is not defined.");
        writln(ctx,
               "        Check the available table names and levels for this spec.",
               stdio::STDERR, ctx.units.mt2, false);
    }
    lex(ctx);
    locok = false;
}

// getprt.f:52-101 / :131-182 -- one print element: a LEVEL, or a table name
// with an optional +/- prefix. Returns false only when the LIST arm bailed --
// getprt.f:151's `GO TO 10` skips to the next element there, and the
// single-value arm at :60-71 has NO such jump: it reports the bad prefix,
// consumes the token, and FALLS THROUGH into the table lookup, which then runs
// against whatever came next. That produces the oracle's characteristic
// two-error cascade on `print = 7` (the prefix error, then "Print or level
// argument is not defined." pointing at the closing brace) and it is the whole
// reason edge/print-prefix-bad exists.
//
// PORTED CENSUS INCONSISTENCY: the two prefix messages differ by one character.
// getprt.f:67 (single value) ends `or nothing.` and getprt.f:148 (inside a
// list) ends `or nothing` with no period.
static bool prt_element(X13Context& ctx, int lsp, int nsp, bool inlist,
                        bool& locok, int& lvlidx, std::vector<bool>& tblmsk) {
    LexState& L = ctx.lex;
    int itmp = 0;
    bool argok = true;
    gtdcnm(ctx, LVLDIC, lvlptr, NLVL, itmp, argok);
    if (argok && itmp > 0) {
        lvlidx = itmp;                                      // getprt.f:56 / :136
        return true;
    }
    bool addtbl = true;
    if (!argok) {
        if (L.nxtktp == MINUS || L.nxtktp == PLUS) {
            if (L.nxtktp == MINUS) addtbl = false;
            lex(ctx);
        } else {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   inlist ? "Prefix must be \"+\", \"-\", or nothing"
                          : "Prefix must be \"+\", \"-\", or nothing.");
            lex(ctx);
            locok = false;
            if (inlist) return false;
        }
    }
    int tbl = 0;
    tbl_lookup(ctx, /*save=*/false, lsp, nsp, locok, nullptr, &tbl);
    if (tbl != 0) {
        // getprt.f:98-99 / :179-180 -- a NAMED table is set directly and taken
        // out of the level fill below, whichever order the two appear in.
        tblmsk[static_cast<std::size_t>(tbl - lsp - 1)] = false;
        ctx.tbllog.prttab(tbl) = addtbl;
    }
    return true;
}

// getprt.f / getsav.f -- one routine for both, because the list arm, the
// NULL-comma checks and the EOF handling are identical; only the element
// reader differs (getsav has no levels and no +/- prefix).
static void read_prtsav(X13Context& ctx, bool save, int lsp, int nsp,
                        bool& locok, std::vector<std::string>* cap) {
    LexState& L = ctx.lex;
    // getprt.f:41-44 -- the mask starts TRUE over this spec's slice and the
    // level starts at `default`, or at `none` under the -n flag. Both are
    // getprt's own locals; getsav has neither.
    std::vector<bool> tblmsk(static_cast<std::size_t>(nsp), true);
    int lvlidx = ctx.hiddn.lnoprt ? 2 : 1;
    // getprt.f:205-208 -- every table the user did NOT name takes the level's
    // value. Guarded on the SAME flag the Fortran guards it on: `Inptok`, the
    // accumulated parse-ok flag the caller passes in, not a local -- a spec
    // that failed to parse leaves the defaults alone.
    struct LevelFill {
        X13Context& ctx; bool save; int lsp, nsp; bool& ok;
        const std::vector<bool>& msk; const int& lvl;
        ~LevelFill() {
            if (save || !ok) return;
            for (int i = 1; i <= nsp; ++i)
                if (msk[static_cast<std::size_t>(i - 1)])
                    ctx.tbllog.prttab(lsp + i) =
                        prm::LEVEL[lsp + i - 1][lvl - 1];
        }
    } fill{ctx, save, lsp, nsp, locok, tblmsk, lvlidx};

    if (L.nxtktp == EOFTOK) {
        locok = false;
        return;
    }
    if (L.nxtktp != LPAREN) {
        if (save) tbl_lookup(ctx, true, lsp, nsp, locok, cap);
        else      prt_element(ctx, lsp, nsp, /*inlist=*/false, locok, lvlidx,
                              tblmsk);
        return;
    }
    bool opngrp = true, hvcmma = false;
    lex(ctx);
    while (true) {
        if (L.nxtktp == EOFTOK) {
            inpter(ctx, PERROR, L.lstpos.data() + 1, "Unexpected EOF");
            locok = false;
            return;
        }
        if (L.nxtktp != RPAREN) {
            if (L.nxtktp == COMMA) {
                if (hvcmma || opngrp) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Found a NULL value; check your commas.");
                    locok = false;
                }
                lex(ctx);
                hvcmma = true;
                opngrp = false;
                continue;
            }
            if (save) {
                tbl_lookup(ctx, true, lsp, nsp, locok, cap);
            } else if (!prt_element(ctx, lsp, nsp, /*inlist=*/true, locok,
                                    lvlidx, tblmsk)) {
                continue;   // getprt.f:151's GO TO 10 -- hvcmma/opngrp unchanged
            }
            if (ctx.error.lfatal) return;
            hvcmma = false;
            opngrp = false;
        } else {
            if (hvcmma) {
                inpter(ctx, PERROR, L.lstpos.data() + 1,
                       "Found a NULL value; check your commas.");
                locok = false;
            }
            lex(ctx);
            return;
        }
    }
}

void getprt(X13Context& ctx, int lspsrs, int nspsrs, bool& locok) {
    read_prtsav(ctx, /*save=*/false, lspsrs, nspsrs, locok, nullptr);
}
void getsav(X13Context& ctx, int lspsrs, int nspsrs, bool& locok,
            std::vector<std::string>* cap) {
    read_prtsav(ctx, /*save=*/true, lspsrs, nspsrs, locok, cap);
}
// ---------------------------------------------------------------------------
// getsvl.f -- the SAVELOG reader, and unlike getprt/getsav above it is NOT a
// token-faithful consumer: it looks every name up in SVLDIC and a name outside
// the calling spec's own slice is a parse ERROR. That matters for OUTCOME, not
// just for output: `savelog = all` is legal in eight specs and REFUSED in six
// (transform, pickmdl, regression, outlier, x11regression, slidingspans),
// because `alldiagnostics`/`all` is simply not in those six slices of the
// dictionary. This port used to accept everything everywhere, so
// `regression{savelog = all}` returned OUTCOME: OK where the oracle stops.
//
// SVLDIC / svlptr are svltbl.prm + svltbl.var verbatim; each name has a LONG
// and a SHORT form, written adjacent, so entries 2i-1 and 2i both map to table
// i (svllog.prm). getsvl passes svlptr(2*Spcdsp) as its local element 0 and
// 2*Nspctb entries, i.e. exactly this spec's names -- so the slice IS the
// validation, and there is no global lookup to fall back on.
//
// The `Svltab(tblidx)=T` store is still deferred with the rest of the table
// selection (this harness dumps everything and the goldens compare stdout);
// only the lookup is ported, which is the half that decides whether the run
// happens at all.
// ---------------------------------------------------------------------------
static const char SVLDIC[] =
    "autotransformatrautomodelamdautodiffadfbestfivemdlb5mmeanmufinalunit"
    "rootfuralldiagnosticsallautomodelamdaicaicaiccaccbicbichannanquinnhq"
    "eiceicaveragefcsterrafcrootsrtsalldiagnosticsallaictestatschi2testct"
    "sidentifiedidnormalitytestnrmseasonalacfsacljungboxqlbqboxpierceqbpq"
    "seasftestsfttdftesttftdurbinwatsondwfriedmantestfrtalldiagnosticsall"
    "m1m1m2m2m3m3m4m4m5m5m6m6m7m7m8m8m9m9m10m10m11m11qqq2q2movingseasrati"
    "omsricratioicrfstableb1fb1fstabled8fd8movingseasfmsfidseasonalidsall"
    "diagnosticsallaictestatsaveabsrevsaasaaveabsrevchngachaveabsrevindsa"
    "iaaaveabsrevtrendatraveabsrevtrendchngatcaveabsrevsfasfaveabsrevsfpr"
    "ojaspavesumsqfcsterrafealldiagnosticsallpercentpctpercentspcspeakssp"
    "kdirpeaksdpkindpeaksipktukeypeakstpkdirtukeypeaksdtpindtukeypeaksitp"
    "qsqsdirqsdqsindqsiqsqcheckqchnpsanpadirnpsadnpindnpsainpalldiagnosti"
    "csallindm1im1indm2im2indm3im3indm4im4indm5im5indm6im6indm7im7indm8im"
    "8indm9im9indm10imtindm11imeindqiqindq2iq2indmovingseasratioisrindicr"
    "atioiirindfstabled8id8indmovingseasfisfindidseasonaliidindtestittall"
    "diagnosticsallseatsmodelsmdx13modelxmdx12modelx2mnormalitytestnrmtot"
    "alsquarederrortsecomponentvariancecvrconcurrentesterrorceepercentred"
    "uctionseprsaverageabsdiffannualaadoverunderestimationoueoverundersta"
    "tisticsousseasonalsignifssgdurbinwatsondwsfriedmanfrsalldiagnosticsa"
    "ll";

static const int svlptr[219] = {
    1, 14, 17, 26, 29, 37, 40, 51, 54, 58, 60, 73, 76, 90, 93, 102, 105,
    108, 111, 115, 118, 121, 124, 135, 137, 140, 143, 157, 160, 165, 168,
    182, 185, 192, 195, 203, 206, 216, 218, 231, 234, 245, 248, 257, 260,
    270, 273, 282, 285, 292, 295, 307, 309, 321, 324, 338, 341, 343, 345,
    347, 349, 351, 353, 355, 357, 359, 361, 363, 365, 367, 369, 371, 373,
    375, 377, 380, 383, 386, 389, 390, 391, 393, 395, 410, 413, 420, 423,
    432, 435, 444, 447, 458, 461, 471, 474, 488, 491, 498, 501, 512, 515,
    528, 531, 545, 548, 562, 565, 583, 586, 597, 600, 615, 618, 633, 636,
    650, 653, 660, 663, 671, 674, 679, 682, 690, 693, 701, 704, 714, 717,
    730, 733, 746, 749, 751, 753, 758, 761, 766, 769, 775, 778, 782, 785,
    792, 795, 802, 805, 819, 822, 827, 830, 835, 838, 843, 846, 851, 854,
    859, 862, 867, 870, 875, 878, 883, 886, 891, 894, 900, 903, 909, 912,
    916, 918, 923, 926, 944, 947, 957, 960, 972, 975, 989, 992, 1005, 1008,
    1015, 1018, 1032, 1035, 1045, 1048, 1056, 1059, 1067, 1070, 1083, 1086,
    1103, 1106, 1123, 1126, 1144, 1147, 1165, 1168, 1188, 1191, 1210, 1213,
    1232, 1235, 1249, 1252, 1264, 1267, 1275, 1278, 1292, 1295
};

// getsvl.f:34-46 / :80-96 -- the lookup, and its two-line refusal. The Fortran
// tests only tblidx, so a non-NAME token (which leaves gtdcnm's argok false and
// tblidx 0) takes the same arm: `savelog = -all` is refused as an undefined
// argument, not as a syntax error.
static void svl_lookup(X13Context& ctx, int lsvsrs, int nsvsrs, bool& locok) {
    LexState& L = ctx.lex;
    int tblidx = 0;
    bool argok = true;
    gtdcnm(ctx, SVLDIC, &svlptr[2 * lsvsrs], 2 * nsvsrs, tblidx, argok);
    if (tblidx == 0) {
        inpter(ctx, PERROR, L.lstpos.data() + 1,
               "Savelog argument is not defined.");
        writln(ctx, "        Check the available diagnostics for this spec.",
               stdio::STDERR, ctx.units.mt2, false);
        lex(ctx);
        locok = false;
    }
    // else: Svltab(Spcdsp+(tblidx+1)/2) = T -- deferred with table selection.
}

void getsvl(X13Context& ctx, int lsvsrs, int nsvsrs, bool& locok) {
    LexState& L = ctx.lex;
    if (L.nxtktp == EOFTOK) {
        locok = false;
    } else if (L.nxtktp != LPAREN) {
        svl_lookup(ctx, lsvsrs, nsvsrs, locok);
    } else {
        // getsvl.f:50-115 -- the list arm, with its own NULL-element checks.
        bool opngrp = true, hvcmma = false;
        lex(ctx);
        while (true) {
            if (L.nxtktp == EOFTOK) {
                inpter(ctx, PERROR, L.lstpos.data() + 1, "Unexpected EOF");
                locok = false;
                return;
            }
            if (L.nxtktp != RPAREN) {
                if (L.nxtktp == COMMA) {
                    if (hvcmma || opngrp) {
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "Found a NULL value; check your commas.");
                        locok = false;
                    }
                    lex(ctx);
                    hvcmma = true;
                    opngrp = false;
                    continue;
                }
                svl_lookup(ctx, lsvsrs, nsvsrs, locok);
                if (ctx.error.lfatal) return;
                hvcmma = false;
                opngrp = false;
            } else {
                if (hvcmma) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Found a NULL value; check your commas.");
                    locok = false;
                }
                lex(ctx);
                return;
            }
        }
    }
}

} // namespace x13
