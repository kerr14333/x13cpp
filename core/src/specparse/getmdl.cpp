// getmdl.cpp -- arima{ model = (p d q)(P D Q)s } structure builders:
// getmdl.f, getopr.f, insopr.f, iscrfn.f, mkoprt.f, maxlag.f, polyml.f,
// insort.f, inbtwn.f, intsrt.f, mdlfix.f.
//
// These populate the model COMMON state (Mdl/Opr/Oprfac/Arimal/Arimap/Arimaf/
// Oprttl + max-lag summaries) that the regression-matrix builder (regvar.f)
// needs -- most importantly the differencing operators whose expansion via
// ratpos.f produces the integrated Constant column.
#include "specparse/specparse.hpp"
#include "notset.hpp"
#include "srslen.hpp"
#include "gen/model.hpp"
#include "numeric/numeric.hpp"   // dpeq (dpeq.f tolerance equality)

#include <string>

namespace x13 {

using namespace lexprm;

// inbtwn.f
void inbtwn(double tcoef, int lagt, int in, int& ncoef, double* coef, int* lag) {
    int after = in + 1;
    for (int i = ncoef; i >= after; --i) {
        coef[i] = coef[i - 1];   // Coef(i+1)=Coef(i)
        lag[i] = lag[i - 1];
    }
    coef[after - 1] = tcoef;
    lag[after - 1] = lagt;
    ncoef = ncoef + 1;
}

// insort.f
void insort(double tcoef, int lagt, int& ncoef, double* coef, int* lag) {
    for (int i = ncoef; i >= 1; --i) {
        if (lagt > lag[i - 1]) {
            inbtwn(tcoef, lagt, i, ncoef, coef, lag);
            return;
        } else if (lagt == lag[i - 1]) {
            coef[i - 1] = coef[i - 1] + tcoef;
            return;
        }
    }
    inbtwn(tcoef, lagt, 0, ncoef, coef, lag);
}

// polyml.f  (sign convention: polynomials are 1 - sum c(i) B^lag(i))
void polyml(const double* polya, const int* alag, int na, const double* polyb,
            const int* blag, int nb, int pc, double* polyc, int* clag, int& nc) {
    (void)pc;
    constexpr int PCOEF = 200;
    double polyt[PCOEF];
    int tlag[PCOEF];
    int nt = 0;
    for (int i = 1; i <= na; ++i) {
        double tmp = polya[i - 1];
        int lagt = alag[i - 1];
        insort(tmp, lagt, nt, polyt, tlag);
    }
    for (int i = 1; i <= nb; ++i) {
        double tmpb = polyb[i - 1];
        int lagb = blag[i - 1];
        insort(tmpb, lagb, nt, polyt, tlag);
        for (int j = 1; j <= na; ++j) {
            double tmp = -tmpb * polya[j - 1];
            int lagt = lagb + alag[j - 1];
            insort(tmp, lagt, nt, polyt, tlag);
        }
    }
    for (int i = 1; i <= nt; ++i) {
        polyc[i - 1] = polyt[i - 1];
        clag[i - 1] = tlag[i - 1];
    }
    nc = nt;
}

// intsrt.f (shell sort)
void intsrt(int nr, int* vecx) {
    int gap = nr;
    while (true) {
        gap = gap / 2;
        if (gap <= 0) break;
        int nsrt = nr - gap;
        int bot = 0;
        while (true) {
            bot = bot + 1;
            if (bot > nsrt) break;
            int b = bot;
            while (true) {
                int top = b + gap;
                if (vecx[b - 1] <= vecx[top - 1]) break;
                int tmp = vecx[top - 1];
                vecx[top - 1] = vecx[b - 1];
                vecx[b - 1] = tmp;
                if (b <= gap) break;
                b = b - gap;
            }
        }
    }
}

// iscrfn.f
void iscrfn(int oprn, int scr, const int* avec, int nelt, int pc, int* cvec) {
    (void)pc;
    constexpr int ADD = 1, SUB = 2, MULT = 3, DIV = 4;
    for (int i = 0; i < nelt; ++i) {
        if (oprn == ADD)       cvec[i] = scr + avec[i];
        else if (oprn == SUB)  cvec[i] = avec[i] - scr;
        else if (oprn == MULT) cvec[i] = scr * avec[i];
        else if (oprn == DIV)  cvec[i] = avec[i] / scr;
    }
}

// mkoprt.f
void mkoprt(X13Context& ctx, int optype, int period, int sp, std::string& oprnam,
            int& noprcr) {
    using namespace prm;
    oprnam.assign(POPRCR, ' ');
    int ipos;
    if (period == 1) {
        oprnam.replace(0, 11, "Nonseasonal");
        ipos = 12;
    } else if (period == sp) {
        oprnam.replace(0, 8, "Seasonal");
        ipos = 9;
    } else {
        oprnam.replace(0, 7, "Period ");
        ipos = 8;
        itoc(ctx, period, oprnam, ipos);
        if (ctx.error.lfatal) return;
    }
    if (optype == DIFF) {
        noprcr = ipos + 10;
        oprnam.replace(static_cast<std::size_t>(ipos - 1), 11, " Difference");
    } else {
        noprcr = ipos + 2;
        oprnam.replace(static_cast<std::size_t>(ipos - 1), 3,
                       optype == AR ? " AR" : " MA");
    }
}

// maxlag.f
void maxlag(const int* arimal, const int* opr, int begopr, int endopr, int& mxlag) {
    mxlag = 0;
    for (int iopr = begopr; iopr <= endopr; ++iopr) {
        int imax = 0;
        int i1 = opr[iopr - 1];
        int i2 = opr[iopr] - 1;
        if (i2 >= i1) {
            imax = arimal[i2 - 1];
            if (i2 > i1) {
                for (int i = i2 - 1; i >= i1; --i)
                    if (imax < arimal[i - 1]) imax = arimal[i - 1];
            }
            mxlag = imax + mxlag;
        }
    }
}

// getopr.f
void getopr(X13Context& ctx, int optype, double* coef, int* lag, bool* fix,
            int& ncoef, int& nd, int& naimcf, bool& locok, bool& inptok) {
    using namespace prm;
    LexState& L = ctx.lex;
    static const char OPRDIC[] = "DIFFARMA";
    static const int oprptr[4] = {1, 5, 7, 9};
    constexpr int POPDIC = 3;

    locok = true;
    bool resort = false;
    nd = 0;
    int mxord = (optype == DIFF) ? PDIFOR : PORDER;
    bool mislag = false;
    int lastlg = 0;
    int tmp;

    if (L.nxtktp == EOFTOK) {
        locok = false;
    } else if (getint(ctx, tmp)) {
        ncoef = tmp;
        mislag = false;
    } else if (L.nxtktp != LBRAKT) {
        inpter(ctx, PERROR, L.lstpos.data() + 1,
               "Expected an INTEGER or \"[\" not \"" + cur_tok(ctx) + "\"");
        locok = false;
    } else {
        ncoef = 0;
        mislag = true;
        bool opngrp = true, hvcmma = false;
        lex(ctx);
        while (true) {
            bool broke = false;
            if (L.nxtktp != RBRAKT) {
                if (L.nxtktp == COMMA) {
                    if (hvcmma || opngrp) {
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "Found a NULL lag; check your commas.");
                        locok = false;
                    }
                    lex(ctx);
                    hvcmma = true;
                    opngrp = false;
                    continue;   // GO TO 10
                } else if (!getint(ctx, tmp)) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Expected an integer not \"" + cur_tok(ctx) + "\"");
                    locok = false;
                } else if (ncoef >= mxord) {
                    std::string str;
                    int nchr;
                    getstr(ctx, OPRDIC, oprptr, POPDIC, optype, str, nchr);
                    if (ctx.error.lfatal) return;
                    str.resize(LINLEN, ' ');
                    int ipos = nchr + 17;
                    str.replace(static_cast<std::size_t>(nchr), 16, " vector exceeds ");
                    itoc(ctx, mxord, str, ipos);
                    if (ctx.error.lfatal) return;
                    str.replace(static_cast<std::size_t>(ipos - 1), 8, " lag[s].");
                    ipos = ipos + 8;
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           std::string_view(str).substr(0, static_cast<std::size_t>(ipos - 1)));
                    locok = false;
                } else if (tmp < 0) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Lags specified in model must be positive integers.");
                    locok = false;
                } else {
                    ncoef = ncoef + 1;
                    if (naimcf + ncoef - 1 > PARIMA) {
                        inpter(ctx, PERROR, L.lstpos.data() + 1,
                               "No room to add more ARIMA coefficients. Reduce the model order.");
                        locok = false;
                    }
                    if (ncoef == 1) {
                        lastlg = tmp;
                    } else if (!resort) {
                        if (tmp < lastlg) {
                            resort = true;
                            inpter(ctx, PWARN, L.lstpos.data() + 1,
                                   "Lags must be specified from smallest to largest; "
                                   "lags will be sorted.");
                        } else {
                            lastlg = tmp;
                        }
                    }
                    lag[ncoef - 1] = tmp;
                    hvcmma = false;
                    opngrp = false;
                    continue;   // GO TO 10
                }
            } else if (hvcmma) {
                inpter(ctx, PERROR, L.lstpos.data() + 1,
                       "Found a NULL lag; check your commas.");
                locok = false;
            }
            (void)broke;
            if (locok) lex(ctx);
            else skplst(ctx, RBRAKT);
            break;   // GO TO 20
        }
    }

    // 20 CONTINUE
    if (!mislag) {
        if (ncoef > mxord) {
            std::string str(LINLEN, ' ');
            int ipos = 19;
            str.replace(0, 18, "Maximum number of ");
            std::string opn;
            int nchr;
            getstr(ctx, OPRDIC, oprptr, POPDIC, optype, opn, nchr);
            if (ctx.error.lfatal) return;
            str.replace(static_cast<std::size_t>(ipos - 1), opn.size(), opn);
            ipos = ipos + nchr;
            str.replace(static_cast<std::size_t>(ipos - 1), 7, " lags, ");
            ipos = ipos + 7;
            itoc(ctx, mxord, str, ipos);
            if (ctx.error.lfatal) return;
            str.replace(static_cast<std::size_t>(ipos - 1), 11, ", exceeded.");
            ipos = ipos + 11;
            inpter(ctx, PERROR, L.errpos.data() + 1,
                   std::string_view(str).substr(0, static_cast<std::size_t>(ipos - 1)));
            locok = false;
        } else if (naimcf + ncoef - 1 > PARIMA) {
            std::string str(LINLEN, ' ');
            int ipos = 39;
            str.replace(0, 38, "Maximum number of ARIMA coefficients, ");
            itoc(ctx, PARIMA, str, ipos);
            if (ctx.error.lfatal) return;
            str.replace(static_cast<std::size_t>(ipos - 1), 36,
                        ", exceeded.  Reduce the model order.");
            ipos = ipos + 36;
            inpter(ctx, PERROR, L.errpos.data() + 1,
                   std::string_view(str).substr(0, static_cast<std::size_t>(ipos - 1)));
            locok = false;
        } else if (optype == DIFF) {
            // Set up the (1-B)^nd difference operator.
            nd = ncoef;
            if (nd > 0) {
                ncoef = 0;
                int ivec[1] = {1};
                double dpvec[1] = {1.0};
                for (int i = 1; i <= nd; ++i)
                    polyml(dpvec, ivec, 1, coef, lag, ncoef, PDIFOR, coef, lag, ncoef);
            }
        } else if (ncoef > 0) {
            for (int i = 1; i <= ncoef; ++i) lag[i - 1] = i;
        }
    }

    // Set the coefficient and fix vector if it hasn't been done.
    if (mislag || optype != DIFF) setdp(prm::DNOTST, ncoef, coef);
    if (mislag && resort) intsrt(ncoef, lag);
    setlg(optype == DIFF, ncoef, fix);

    naimcf = naimcf + ncoef;
    inptok = inptok && locok;
}

// insopr.f
void insopr(X13Context& ctx, int optype, const double* coef, const int* lag,
            const bool* fix, int ncoef, int facsp, std::string_view ioprtl,
            bool& locok, bool& inptok) {
    using namespace prm;
    locok = true;
    int i = 3;
    insptr(ctx, false, 1, optype, 3, POPR, ctx.model.mdl.data(), i);
    if (ctx.error.lfatal) return;

    if (ctx.model.opr(ctx.model.nopr) + ncoef - 1 > PARIMA) locok = false;

    if (locok) {
        int iopr = ctx.model.mdl(optype) - 1;
        insptr(ctx, true, ncoef, iopr, POPR, PARIMA, ctx.model.opr.data(),
               ctx.model.nopr);
        if (ctx.error.lfatal) return;
        // Add the factor of the operator.
        for (i = ctx.model.nopr; i >= iopr + 1; --i)
            ctx.model.oprfac(i) = ctx.model.oprfac(i - 1);
        ctx.model.oprfac(iopr) = facsp;

        insdbl(ctx, coef, iopr, ctx.model.opr.data(), ctx.model.nopr,
               ctx.mdldat.arimap.data());
        if (!ctx.error.lfatal)
            insint(ctx, lag, iopr, ctx.model.opr.data(), ctx.model.nopr,
                   ctx.model.arimal.data());
        if (!ctx.error.lfatal)
            inslg(ctx, fix, iopr, ctx.model.opr.data(), ctx.model.nopr,
                  ctx.model.arimaf.data());
        if (!ctx.error.lfatal)
            insstr(ctx, ioprtl, iopr, POPR, ctx.model.oprttl.data(),
                   static_cast<int>(ctx.model.oprttl.size()),
                   ctx.model.oprptr.data(), ctx.model.noprtl);
        if (ctx.error.lfatal) return;
    }
    inptok = inptok && locok;
}

// getmdl.f  (also reconstructs ctx.captured.model_desc for the M1 echo)
void getmdl(X13Context& ctx, bool& locok, bool& inptok, bool lauto) {
    using namespace prm;
    LexState& L = ctx.lex;
    model_cmn& M = ctx.model;

    double arcoef[PORDER], dfcoef[PDIFOR], macoef[PORDER];
    int arlag[PORDER], dflag[PDIFOR], malag[PORDER];
    bool arfix[PORDER], dffix[PDIFOR], mafix[PORDER];
    int arerr[2], dferr[2], maerr[2], begmdl[2];

    locok = true;
    bool havreg = false, hvsea = false;
    cpyint(L.lstpos.data() + 1, 2, 1, begmdl);
    // Mdldsn(1:(1+Lineln-begmdl(PCHAR))) = Linex(begmdl(PCHAR):Lineln)
    {
        int b = begmdl[PCHAR - 1];
        if (b >= 1 && b <= L.lineln)
            M.mdldsn = std::string_view(L.linex).substr(
                static_cast<std::size_t>(b - 1),
                static_cast<std::size_t>(L.lineln - b + 1));
    }
    M.nseadf = 0;
    M.nnsedf = 0;
    int naimcf = 0;
    int numopr = 0;

    std::string desc;   // M1 echo (was getmdl_consume)

    bool sawfac = (L.nxtktp == LPAREN);
    bool argok;
    while (L.nxtktp == LPAREN) {
        lex(ctx);
        cpyint(L.lstpos.data() + 1, 2, 1, arerr);
        int nar, itmp;
        getopr(ctx, AR, arcoef, arlag, arfix, nar, itmp, naimcf, argok, locok);
        if (ctx.error.lfatal) return;
        if (L.nxtktp == COMMA) lex(ctx);
        cpyint(L.lstpos.data() + 1, 2, 1, dferr);
        int ndcoef, ndf;
        getopr(ctx, DIFF, dfcoef, dflag, dffix, ndcoef, ndf, naimcf, argok, locok);
        if (ctx.error.lfatal) return;
        if (L.nxtktp == COMMA) lex(ctx);
        cpyint(L.lstpos.data() + 1, 2, 1, maerr);
        int nma;
        getopr(ctx, MA, macoef, malag, mafix, nma, itmp, naimcf, argok, locok);
        if (ctx.error.lfatal) return;

        desc += "(" + std::to_string(nar) + " " + std::to_string(ndf) + " " +
                std::to_string(nma) + ")";

        if (L.nxtktp != RPAREN) {
            inpter(ctx, PERROR, L.lstpos.data() + 1, "Expected \")\" after (AR DIFF MA");
            locok = false;
        } else {
            M.nmddcr = L.pos[PCHAR] - begmdl[PCHAR - 1];
            lex(ctx);
            // Get the period of the factor if it exists.
            int facsp = 0;
            if (L.nxtktp == INTGR) {
                M.nmddcr = L.pos[PCHAR] - begmdl[PCHAR - 1];
                argok = getint(ctx, facsp);
                desc += std::to_string(facsp);
                if (facsp <= 0) {
                    inpter(ctx, PERROR, L.lstpos.data() + 1,
                           "Period specified in (AR DIFF MA)period must be greater "
                           "than zero.");
                    locok = false;
                }
            } else if (!havreg) {
                facsp = 1;
                havreg = true;
            } else if (!hvsea && M.sp > 1) {
                facsp = M.sp;
                hvsea = true;
            } else {
                inpter(ctx, PERROR, L.lstpos.data() + 1,
                       "Must explicitly specify the period in (AR DIFF MA)period");
                locok = false;
            }

            if (locok) {
                std::string str;
                int nchr;
                if (nar > 0) {
                    numopr = numopr + 1;
                    if (numopr > POPR) {
                        inpter(ctx, PERROR, arerr,
                               "Too many operators in specified ARIMA model");
                        locok = false;
                        break;   // GO TO 20
                    }
                    iscrfn(3, facsp, arlag, nar, PORDER, arlag);
                    mkoprt(ctx, AR, facsp, M.sp, str, nchr);
                    if (!ctx.error.lfatal)
                        insopr(ctx, AR, arcoef, arlag, arfix, nar, facsp,
                               std::string_view(str).substr(0, static_cast<std::size_t>(nchr)),
                               argok, locok);
                    if (ctx.error.lfatal) return;
                    maxlag(M.arimal.data(), M.opr.data(), M.mdl(AR - 1),
                           M.mdl(AR) - 1, M.mxarlg);
                    if (M.mxarlg > PORDER) {
                        inpter(ctx, PERROR, arerr, "Order of the AR operator is too large.");
                        locok = false;
                    }
                } else if (nar < 0) {
                    inpter(ctx, PERROR, arerr,
                           "Order of the AR operator cannot be less than zero.");
                    locok = false;
                }

                if (ndcoef > 0) {
                    numopr = numopr + 1;
                    if (numopr > POPR) {
                        inpter(ctx, PERROR, dferr,
                               "Too many operators in specified ARIMA model");
                        locok = false;
                        break;   // GO TO 20
                    }
                    M.lseadf = (M.sp > 1 && facsp == M.sp) ||
                               (M.sp == 1 && ndcoef == M.sp - 1);
                    if (M.lseadf && M.lseff) {
                        inpter(ctx, PERROR, dferr,
                               "Cannot have a seasonal difference with seasonal "
                               "regression effects.");
                        locok = false;
                    }
                    if (facsp == 1) M.nnsedf = M.nnsedf + ndf;
                    if (facsp == M.sp && M.sp > 1) M.nseadf = M.nseadf + ndf;
                    if (ndcoef > PDIFOR) {
                        inpter(ctx, PERROR, dferr,
                               "Order of the differencing operator is too large.");
                        locok = false;
                    } else {
                        iscrfn(3, facsp, dflag, ndcoef, PDIFOR, dflag);
                        mkoprt(ctx, DIFF, facsp, M.sp, str, nchr);
                        if (!ctx.error.lfatal)
                            insopr(ctx, DIFF, dfcoef, dflag, dffix, ndcoef, facsp,
                                   std::string_view(str).substr(0, static_cast<std::size_t>(nchr)),
                                   argok, locok);
                        if (ctx.error.lfatal) return;
                        maxlag(M.arimal.data(), M.opr.data(), M.mdl(DIFF - 1),
                               M.mdl(DIFF) - 1, M.mxdflg);
                        if (M.mxdflg > PDIFOR) {
                            inpter(ctx, PERROR, dferr,
                                   "Order of the full differencing operator is too large.");
                            locok = false;
                        }
                    }
                } else if (ndf < 0) {
                    inpter(ctx, PERROR, dferr,
                           "Order of the differencing operator cannot be less than zero.");
                    locok = false;
                }

                if (nma > PORDER) {
                    inpter(ctx, PERROR, maerr, "Order of the MA operator is too large");
                    locok = false;
                } else if (nma > 0) {
                    numopr = numopr + 1;
                    if (numopr > POPR) {
                        inpter(ctx, PERROR, maerr,
                               "Too many operators in specified ARIMA model");
                        locok = false;
                        break;   // GO TO 20
                    }
                    iscrfn(3, facsp, malag, nma, PORDER, malag);
                    mkoprt(ctx, MA, facsp, M.sp, str, nchr);
                    if (!ctx.error.lfatal)
                        insopr(ctx, MA, macoef, malag, mafix, nma, facsp,
                               std::string_view(str).substr(0, static_cast<std::size_t>(nchr)),
                               argok, locok);
                    if (ctx.error.lfatal) return;
                    maxlag(M.arimal.data(), M.opr.data(), M.mdl(MA - 1),
                           M.mdl(MA) - 1, M.mxmalg);
                    if (M.mxmalg > PORDER) {
                        inpter(ctx, PERROR, L.errpos.data() + 1,
                               "Order of the MA operator is too large.");
                        locok = false;
                    }
                } else if (nma < 0) {
                    inpter(ctx, PERROR, L.errpos.data() + 1,
                           "Order of the MA operator cannot be less than zero.");
                    locok = false;
                }

                // If there is another factor, process it.
                if (L.nxtktp == LPAREN) continue;   // GO TO 10
            }
        }
        // GO TO 20 (we processed the last factor; break out).
        goto done;
    }
    // Fortran: falling out of the DO WHILE means the first token was not '('
    // (re-entry only happens via GO TO 10, which requires another '(').
    if (!sawfac) {
        inpter(ctx, PERROR, L.lstpos.data() + 1, "Expected \"(\" in  (AR DIFF MA)");
        locok = false;
    }
done:
    if (locok) {
        M.nmdl = M.nmdl + 1;
        ctx.captured.model_desc = desc;
    } else {
        skparm(ctx, lauto);
    }
    inptok = inptok && locok;
}

// mdlfix.f
void mdlfix(X13Context& ctx) {
    using namespace prm;
    model_cmn& M = ctx.model;
    bool lmdlfx = true;
    M.imdlfx = 0;
    for (int iflt = DIFF; iflt <= MA; ++iflt) {
        int begopr = M.mdl(iflt - 1);
        int endopr = M.mdl(iflt) - 1;
        for (int iopr = begopr; iopr <= endopr; ++iopr) {
            int beglag = M.opr(iopr - 1);
            int endlag = M.opr(iopr) - 1;
            for (int ilag = beglag; ilag <= endlag; ++ilag) {
                if (dpeq(ctx.mdldat.arimap(ilag), prm::DNOTST)) {  // mdlfix.f:33
                    if (lmdlfx) lmdlfx = false;
                } else {
                    lmdlfx = lmdlfx && M.arimaf(ilag);
                    if (M.imdlfx == 0) M.imdlfx = 1;
                    if (M.arimaf(ilag) && M.imdlfx == 1) M.imdlfx = 2;
                }
            }
        }
    }
    if (lmdlfx && M.imdlfx > 0) M.imdlfx = 3;
}

}  // namespace x13
