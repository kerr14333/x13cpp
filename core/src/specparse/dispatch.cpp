// dispatch.cpp -- spec/argument dictionary machinery and skip routines:
// gtdcnm.f, gtarg.f, getfcn.f, skparg.f, skpfcn.f, skplst.f, skparm.f,
// getint.f, getdbl.f, eltlen.f.
#include "specparse/specparse.hpp"
#include "notset.hpp"

namespace x13 {

using namespace lexprm;

// gtdcnm.f
void gtdcnm(X13Context& ctx, std::string_view args, const int* argptr, int nargs,
            int& argidx, bool& argok) {
    LexState& L = ctx.lex;
    argidx = 0;
    argok = true;
    if (L.nxtktp != EOFTOK) {
        if (L.nxtktp != NAME) {
            argok = false;
        } else {
            argidx = strinx(false, args, argptr, 1, nargs, cur_tok(ctx));
            if (argidx > 0) lex(ctx);
        }
    }
}

// gtarg.f
bool gtarg(X13Context& ctx, std::string_view args, const int* argptr, int nargs,
           int& argidx, int* arglog, bool& inptok) {
    LexState& L = ctx.lex;
    bool result = true;
    while (true) {
        int argpos[2] = {L.lstpos[PLINE], L.lstpos[PCHAR]};
        int nargcr = L.nxtkln;
        std::string argnm = cur_tok(ctx);
        bool argok;
        gtdcnm(ctx, args, argptr, nargs, argidx, argok);
        if (L.nxtktp == EOFTOK) {
            result = false;
        } else if (L.nxtktp == RBRACE) {
            lex(ctx);
            result = false;
        } else if (!argok) {
            inpter(ctx, PERROR, argpos,
                   "Expected argument name or \"}\" but found \"" +
                   argnm.substr(0, static_cast<std::size_t>(nargcr)) + "\"");
            inptok = false;
            lex(ctx);
            result = false;
        } else if (argidx == 0) {
            inpter(ctx, PERROR, argpos,
                   "Argument name \"" + argnm.substr(0, static_cast<std::size_t>(nargcr)) +
                   "\" not found");
            inptok = false;
            lex(ctx);
            skparg(ctx);
            if (ctx.error.lfatal) return result;
            continue;
        } else if (L.nxtktp != EQUALS) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   " Expected \"=\" but found \"" + cur_tok(ctx) + "\"");
            inptok = false;
            skparg(ctx);
            if (ctx.error.lfatal) return result;
            continue;
        } else {
            int base = (argidx - 1) * 2;
            if (arglog[base + 0] != prm::NOTSET) {
                std::string linnum(5, ' '), colnum(5, ' ');
                int ilin = 1, icol = 1;
                itoc(ctx, arglog[base + 0], linnum, ilin);
                itoc(ctx, arglog[base + 1], colnum, icol);
                inpter(ctx, PERROR, argpos,
                       "Argument name \"" + argnm.substr(0, static_cast<std::size_t>(nargcr)) +
                       "\" also found on line " + linnum.substr(0, static_cast<std::size_t>(ilin - 1)) +
                       " position " + colnum.substr(0, static_cast<std::size_t>(icol - 1)) +
                       " of the input file.");
                inptok = false;
                lex(ctx);
                skparg(ctx);
                if (ctx.error.lfatal) return result;
                continue;
            } else {
                arglog[base + 0] = argpos[0];
                arglog[base + 1] = argpos[1];
                lex(ctx);
            }
        }
        return result;
    }
}

// getfcn.f
bool getfcn(X13Context& ctx, std::string_view fcns, const int* fcnptr, int nfcns,
            int& fcnidx, int* fcnlog, bool& inptok) {
    LexState& L = ctx.lex;
    bool result;
    while (true) {
        if (L.nxtktp == EOFTOK) {
            result = false;
        } else {
            result = true;
            std::string fname = cur_tok(ctx);
            int nfname = L.nxtkln;
            int fcnpos[2] = {L.lstpos[PLINE], L.lstpos[PCHAR]};
            bool argok;
            gtdcnm(ctx, fcns, fcnptr, nfcns, fcnidx, argok);
            if (!argok) {
                inpter(ctx, PERROR, fcnpos,
                       "Expected specification name but found \"" +
                       fname.substr(0, static_cast<std::size_t>(nfname)) + "\"");
                inptok = false;
                skpfcn(ctx, fname, nfname);
                continue;
            } else if (fcnidx == 0) {
                inpter(ctx, PERROR, fcnpos,
                       fname.substr(0, static_cast<std::size_t>(nfname)) + " is not a valid spec name.");
                inptok = false;
                skpfcn(ctx, fname, nfname);
                continue;
            } else {
                if (L.nxtktp == EOFTOK) result = false;
                if (L.nxtktp != LBRACE) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           " Expected \"{\" but found " + cur_tok(ctx));
                    inptok = false;
                    skpfcn(ctx, fname, nfname);
                    continue;
                } else {
                    int base = (fcnidx - 1) * 2;
                    if (fcnlog[base + 0] != prm::NOTSET) {
                        std::string linnum(5, ' '), colnum(5, ' ');
                        int ilin = 1, icol = 1;
                        itoc(ctx, fcnlog[base + 0], linnum, ilin);
                        itoc(ctx, fcnlog[base + 1], colnum, icol);
                        inpter(ctx, PERROR, fcnpos,
                               fname.substr(0, static_cast<std::size_t>(nfname)) +
                               " also found on line " + linnum.substr(0, static_cast<std::size_t>(ilin - 1)) +
                               " position " + colnum.substr(0, static_cast<std::size_t>(icol - 1)) +
                               " of the input file.");
                        inptok = false;
                        skpfcn(ctx, fname, nfname);
                        continue;
                    } else {
                        fcnlog[base + 0] = fcnpos[0];
                        fcnlog[base + 1] = fcnpos[1];
                        lex(ctx);
                    }
                }
            }
        }
        return result;
    }
}

// skparg.f
void skparg(X13Context& ctx) {
    LexState& L = ctx.lex;
    if (L.nxtktp == EQUALS) lex(ctx);
    if (L.nxtktp == LPAREN || L.nxtktp == LBRAKT) {
        skplst(ctx, clsgrp(L.nxtktp));
    } else if (L.nxtktp == DBL || L.nxtktp == INTGR || L.nxtktp == NAME ||
               L.nxtktp == QUOTE) {
        lex(ctx);
    } else {
        inpter(ctx, PERROR, L.lstpos.data() + 1,
               "Expected NAME=VALUE or NAME=(LIST) not \"" + cur_tok(ctx) + "\"");
        abend(ctx);
    }
}

// skpfcn.f
void skpfcn(X13Context& ctx, std::string_view fname, int nfn) {
    LexState& L = ctx.lex;
    while (true) {
        if (L.nxtktp == EOFTOK) {
            inpter(ctx, PERROR, L.lstpos.data() + 1,
                   "No closing brace \"}\" on specification, \"" +
                   std::string(fname.substr(0, static_cast<std::size_t>(nfn))) + "\"");
        } else if (L.nxtktp != RBRACE) {
            lex(ctx);
            continue;
        } else {
            lex(ctx);
        }
        return;
    }
}

// skplst.f
void skplst(X13Context& ctx, int clsgtp) {
    LexState& L = ctx.lex;
    while (true) {
        if (L.nxtktp != clsgtp && L.nxtktp != EOFTOK) {
            lex(ctx);
        } else {
            lex(ctx);
            return;
        }
    }
}

// skparm.f
void skparm(X13Context& ctx, bool lauto) {
    LexState& L = ctx.lex;
    while (true) {
        if (L.nxtktp != NAME && L.nxtktp != QUOTE && L.nxtktp != RBRACE &&
            L.nxtktp != EOFTOK) {
            if (lauto && L.nxtktp == STAR) return;
            lex(ctx);
            continue;
        }
        return;
    }
}

// getint.f
bool getint(X13Context& ctx, int& tmp) {
    LexState& L = ctx.lex;
    bool result = false;
    if (L.nxtktp != EOFTOK) {
        int ipos = L.lstpos[PCHAR];
        std::string line = L.linex.substr(0, static_cast<std::size_t>(L.lineln));
        tmp = ctoi(line, ipos);
        if (ipos > L.lstpos[PCHAR]) {
            L.pos[PCHAR] = ipos;
            result = true;
            lex(ctx);
        }
    }
    return result;
}

// getdbl.f
bool getdbl(X13Context& ctx, double& tmp) {
    LexState& L = ctx.lex;
    bool result = false;
    if (L.nxtktp != EOFTOK) {
        int ipos = L.lstpos[PCHAR];
        std::string line = L.linex.substr(0, static_cast<std::size_t>(L.lineln));
        tmp = ctod(line, ipos);
        if (ipos > L.lstpos[PCHAR]) {
            L.pos[PCHAR] = ipos;
            result = true;
            lex(ctx);
        }
    }
    return result;
}

// eltlen.f  (Ptrvec is 0-based DIMENSION(0:Nstr))
void eltlen(X13Context& ctx, int istr, const int* ptrvec, int nstr, int& length) {
    if (istr < 1 || istr > nstr) {
        abend(ctx);
        return;
    }
    length = ptrvec[istr] - ptrvec[istr - 1];
}

} // namespace x13
