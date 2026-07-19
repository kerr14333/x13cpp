// readers_val.cpp -- typed value-list readers and their support routines:
// intlst.f, insptr.f, putstr.f, getivc.f, gtdpvc.f, gtdtvc.f, getdat.f,
// ctodat.f, gtnmvc.f, getttl.f, gtdcvc.f, and the print/save consumers
// getprt.f/getsav.f/getsvl.f (token-faithful; table-dictionary application
// deferred -- see report).
#include "specparse/specparse.hpp"
#include "notset.hpp"
#include "srslen.hpp"

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
static void getdat(X13Context& ctx, bool& havesp, int& sp, int* idate, bool& argok, bool& inptok) {
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

// --- print / save / savelog consumers ------------------------------------
// M1 consumes the print/save value token-faithfully; applying the selection to
// the table dictionaries (Prttab/Savtab/Svltab) is deferred to the output
// milestone.
static void consume_prtsav(X13Context& ctx, bool& locok) {
    LexState& L = ctx.lex;
    if (L.nxtktp == EOFTOK) { locok = false; return; }
    if (L.nxtktp == LPAREN || L.nxtktp == LBRAKT) {
        skplst(ctx, clsgrp(L.nxtktp));
    } else if (L.nxtktp == MINUS || L.nxtktp == PLUS) {
        lex(ctx);
        if (L.nxtktp == NAME || L.nxtktp == QUOTE) lex(ctx);
    } else if (L.nxtktp == NAME || L.nxtktp == QUOTE) {
        lex(ctx);
    } else {
        inpter(ctx, PERROR, L.lstpos.data() + 1,
               "Expected a table name or list, not \"" + cur_tok(ctx) + "\"");
        locok = false;
        lex(ctx);
    }
}

void getprt(X13Context& ctx, int lspsrs, int nspsrs, bool& locok) {
    (void)lspsrs; (void)nspsrs;
    consume_prtsav(ctx, locok);
}
void getsav(X13Context& ctx, int lspsrs, int nspsrs, bool& locok) {
    (void)lspsrs; (void)nspsrs;
    consume_prtsav(ctx, locok);
}
void getsvl(X13Context& ctx, int lsvsrs, int nsvsrs, bool& locok) {
    (void)lsvsrs; (void)nsvsrs;
    consume_prtsav(ctx, locok);
}

} // namespace x13
