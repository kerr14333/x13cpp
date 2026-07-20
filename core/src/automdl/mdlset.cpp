// mdlset.cpp -- mdlint.f / mkmdsn.f / setopr.f / mdlset.f: programmatic ARIMA
// model construction for automatic model identification. mdlset lays down a
// trial model from six order counts using the same insopr/iscrfn/mkoprt/maxlag
// primitives as the spec-driven getmdl path (see specparse/getmdl.cpp), with
// setopr replacing getopr (orders come from arguments, not the token stream).
#include "automdl/mdlset.hpp"

#include <string>
#include "specparse/specparse.hpp"   // intlst, setint/setdp/setlg (inline),
                                     // polyml, iscrfn, mkoprt, insopr, maxlag,
                                     // itoc, getstr, writln, errhdr, abend
#include "gen/model.hpp"             // prm::AR, DIFF, MA, PORDER, PDIFOR, POPRCR
#include "gen/notset.hpp"            // prm::DNOTST

namespace x13 {

void mdlint(X13Context& ctx) {
    using namespace prm;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;
    intlst(POPR, m.opr.data(), m.nopr);
    intlst(POPR, m.oprptr.data(), m.noprtl);
    m.oprttl = std::string_view("");        // setchr(' ', POPRCR*POPR, Oprttl)
    intlst(PMDL, m.mdl.data(), m.nmdl);
    m.mdl(AR) = 1;
    m.mdl(MA) = 1;
    setlg(false, PARIMA, m.arimaf.data());
    setint(0, PARIMA, m.arimal.data());
    setdp(0.0, PARIMA, d.arimap.data());
    setdp(0.0, static_cast<int>(d.armacm.size()), d.armacm.data());
    d.lndtcv = 0.0;
    m.mxarlg = 0;
    m.mxdflg = 0;
    m.mxmalg = 0;
    m.nseadf = 0;
    m.lseadf = false;
}

void mkmdsn(X13Context& ctx, int nrar, int nrdiff, int nrma, int nsar,
            int nsdiff, int nsma) {
    std::string s(132, ' ');
    s[0] = '(';
    int pos = 2;
    itoc(ctx, nrar, s, pos);
    if (ctx.error.lfatal) return;
    s[pos - 1] = ' ';
    ++pos;
    itoc(ctx, nrdiff, s, pos);
    if (ctx.error.lfatal) return;
    s[pos - 1] = ' ';
    ++pos;
    itoc(ctx, nrma, s, pos);
    if (ctx.error.lfatal) return;
    s[pos - 1] = ')';
    if (nsar > 0 || nsma > 0 || nsdiff > 0) {
        ++pos;
        s[pos - 1] = '(';
        ++pos;
        itoc(ctx, nsar, s, pos);
        if (ctx.error.lfatal) return;
        s[pos - 1] = ' ';
        ++pos;
        itoc(ctx, nsdiff, s, pos);
        if (ctx.error.lfatal) return;
        s[pos - 1] = ' ';
        ++pos;
        itoc(ctx, nsma, s, pos);
        if (ctx.error.lfatal) return;
        s[pos - 1] = ')';
    }
    ctx.model.mdldsn = std::string_view(s);
    ctx.model.nmddcr = pos;
}

namespace {
// setopr.f's order-overflow error: message format deferred to the print engine
// (unreachable for automdl orders, which are single digits); the load-bearing
// effect is clearing locok.
void setopr_err(X13Context& ctx, std::string_view msg, bool& locok) {
    errhdr(ctx);
    writln(ctx, msg, stdio::STDERR, ctx.units.mt2, true);
    locok = false;
}
}  // namespace

void setopr(X13Context& ctx, int optype, double* coef, int* lag, bool* fix,
            int& ncoef, int nd, int& naimcf, bool& locok, bool& inptok) {
    using namespace prm;
    locok = true;
    int mxord = (optype == DIFF) ? PDIFOR : PORDER;

    if (ncoef > mxord) {
        setopr_err(ctx, " ERROR: Maximum number of operator lags exceeded.",
                   locok);
    } else if (naimcf + ncoef - 1 > PARIMA) {
        setopr_err(ctx,
                   " ERROR: Maximum number of ARIMA coefficients exceeded.  "
                   "Reduce the model order.",
                   locok);
    } else if (optype == DIFF) {
        // Set up the (1-B)^nd difference operator.
        if (nd > 0) {
            ncoef = 0;
            int ivec[1] = {1};
            double dpvec[1] = {1.0};
            for (int i = 1; i <= nd; ++i)
                polyml(dpvec, ivec, 1, coef, lag, ncoef, PDIFOR, coef, lag,
                       ncoef);
        }
    } else if (ncoef > 0) {
        // Fill in the AR/MA lags with default 0.1 starting values (DNOTST).
        for (int i = 1; i <= ncoef; ++i) lag[i - 1] = i;
    }

    if (optype != DIFF) setdp(DNOTST, ncoef, coef);
    setlg(optype == DIFF, ncoef, fix);
    naimcf = naimcf + ncoef;
    inptok = inptok && locok;
}

void mdlset(X13Context& ctx, int nrar, int nrdiff, int nrma, int nsar,
            int nsdiff, int nsma, bool& inptok) {
    using namespace prm;
    constexpr int MULT = 3;
    auto& m = ctx.model;

    double arcoef[PORDER], macoef[PORDER], dfcoef[PDIFOR];
    int arlag[PORDER], malag[PORDER], dflag[PDIFOR];
    bool arfix[PORDER], mafix[PORDER], dffix[PDIFOR];
    std::string str;
    int nchr, itmp = 0, naimcf = 0;
    bool argok = true;

    inptok = true;
    m.nseadf = nsdiff;
    m.nnsedf = nrdiff;

    mkmdsn(ctx, nrar, nrdiff, nrma, nsar, nsdiff, nsma);
    if (ctx.error.lfatal) return;

    // ---- nonseasonal AR ----
    if (nrar > 0) {
        setopr(ctx, AR, arcoef, arlag, arfix, nrar, itmp, naimcf, argok, inptok);
        if (ctx.error.lfatal) return;
        if (inptok) {
            iscrfn(MULT, 1, arlag, nrar, PORDER, arlag);
            mkoprt(ctx, AR, 1, m.sp, str, nchr);
            if (!ctx.error.lfatal)
                insopr(ctx, AR, arcoef, arlag, arfix, nrar, 1,
                       std::string_view(str).substr(0, static_cast<std::size_t>(nchr)), argok, inptok);
            if (ctx.error.lfatal) return;
            maxlag(m.arimal.data(), m.opr.data(), m.mdl(AR - 1), m.mdl(AR) - 1,
                   m.mxarlg);
        }
    }
    // ---- nonseasonal differencing ----
    if (nrdiff > 0) {
        int ndcoef = nrdiff;
        setopr(ctx, DIFF, dfcoef, dflag, dffix, ndcoef, nrdiff, naimcf, argok,
               inptok);
        if (ctx.error.lfatal) return;
        if (ndcoef > PDIFOR) {
            errhdr(ctx);
            writln(ctx, " ERROR: Order of the differencing operator is too "
                        "large.",
                   stdio::STDERR, ctx.units.mt2, true);
            inptok = false;
        } else {
            iscrfn(MULT, 1, dflag, ndcoef, PDIFOR, dflag);
            mkoprt(ctx, DIFF, 1, m.sp, str, nchr);
            if (!ctx.error.lfatal)
                insopr(ctx, DIFF, dfcoef, dflag, dffix, ndcoef, 1,
                       std::string_view(str).substr(0, static_cast<std::size_t>(nchr)), argok, inptok);
            if (ctx.error.lfatal) return;
        }
        maxlag(m.arimal.data(), m.opr.data(), m.mdl(DIFF - 1), m.mdl(DIFF) - 1,
               m.mxdflg);
    }
    // ---- nonseasonal MA ----
    if (nrma > 0) {
        setopr(ctx, MA, macoef, malag, mafix, nrma, itmp, naimcf, argok, inptok);
        if (ctx.error.lfatal) return;
        if (inptok) {
            iscrfn(MULT, 1, malag, nrma, PORDER, malag);
            mkoprt(ctx, MA, 1, m.sp, str, nchr);
            if (!ctx.error.lfatal)
                insopr(ctx, MA, macoef, malag, mafix, nrma, 1,
                       std::string_view(str).substr(0, static_cast<std::size_t>(nchr)), argok, inptok);
            if (ctx.error.lfatal) return;
            maxlag(m.arimal.data(), m.opr.data(), m.mdl(MA - 1), m.mdl(MA) - 1,
                   m.mxmalg);
        }
    }
    // ---- seasonal AR ----
    if (nsar > 0) {
        setopr(ctx, AR, arcoef, arlag, arfix, nsar, itmp, naimcf, argok, inptok);
        if (ctx.error.lfatal) return;
        if (inptok) {
            iscrfn(MULT, m.sp, arlag, nsar, PORDER, arlag);
            mkoprt(ctx, AR, m.sp, m.sp, str, nchr);
            if (!ctx.error.lfatal)
                insopr(ctx, AR, arcoef, arlag, arfix, nsar, m.sp,
                       std::string_view(str).substr(0, static_cast<std::size_t>(nchr)), argok, inptok);
            if (ctx.error.lfatal) return;
            maxlag(m.arimal.data(), m.opr.data(), m.mdl(AR - 1), m.mdl(AR) - 1,
                   m.mxarlg);
            if (m.mxarlg > PORDER) {
                errhdr(ctx);
                writln(ctx, " ERROR: Order of the AR operator is too large.",
                       stdio::STDERR, ctx.units.mt2, true);
                inptok = false;
            }
        }
    }
    // ---- seasonal differencing ----
    if (nsdiff > 0) {
        int ndcoef = nsdiff;
        setopr(ctx, DIFF, dfcoef, dflag, dffix, ndcoef, nsdiff, naimcf, argok,
               inptok);
        if (ctx.error.lfatal) return;
        // Cannot mix a seasonal difference with seasonal regression effects.
        m.lseadf = (m.sp > 1) || (m.sp == 1 && ndcoef == m.sp - 1);
        if (m.lseadf && m.lseff) {
            errhdr(ctx);
            writln(ctx, " ERROR: Cannot have a seasonal difference with "
                        "seasonal regression effects.",
                   stdio::STDERR, ctx.units.mt2, true);
            inptok = false;
        }
        if (ndcoef > PDIFOR) {
            errhdr(ctx);
            writln(ctx, " ERROR: Order of the differencing operator is too "
                        "large.",
                   stdio::STDERR, ctx.units.mt2, true);
            inptok = false;
        } else {
            iscrfn(MULT, m.sp, dflag, ndcoef, PDIFOR, dflag);
            mkoprt(ctx, DIFF, m.sp, m.sp, str, nchr);
            if (!ctx.error.lfatal)
                insopr(ctx, DIFF, dfcoef, dflag, dffix, ndcoef, m.sp,
                       std::string_view(str).substr(0, static_cast<std::size_t>(nchr)), argok, inptok);
            if (ctx.error.lfatal) return;
        }
        maxlag(m.arimal.data(), m.opr.data(), m.mdl(DIFF - 1), m.mdl(DIFF) - 1,
               m.mxdflg);
        if (m.mxdflg > PDIFOR) {
            errhdr(ctx);
            writln(ctx, " ERROR: Order of the full differencing operator is "
                        "too large.",
                   stdio::STDERR, ctx.units.mt2, true);
            inptok = false;
        }
    }
    // ---- seasonal MA ----
    if (nsma > 0) {
        setopr(ctx, MA, macoef, malag, mafix, nsma, itmp, naimcf, argok, inptok);
        if (ctx.error.lfatal) return;
        if (inptok) {
            iscrfn(MULT, m.sp, malag, nsma, PORDER, malag);
            mkoprt(ctx, MA, m.sp, m.sp, str, nchr);
            if (!ctx.error.lfatal)
                insopr(ctx, MA, macoef, malag, mafix, nsma, m.sp,
                       std::string_view(str).substr(0, static_cast<std::size_t>(nchr)), argok, inptok);
            if (ctx.error.lfatal) return;
            maxlag(m.arimal.data(), m.opr.data(), m.mdl(MA - 1), m.mdl(MA) - 1,
                   m.mxmalg);
            if (m.mxmalg > PORDER) {
                errhdr(ctx);
                writln(ctx, " ERROR: Order of the MA operator is too large.",
                       stdio::STDERR, ctx.units.mt2, true);
                inptok = false;
            }
        }
    }

    // Effective-observation counts and |G'G| initialization.
    m.lar = m.lextar && m.mxarlg > 0;
    m.lma = m.lextma && m.mxmalg > 0;
    if (m.lextar) {
        m.nintvl = m.mxdflg;
        m.nextvl = m.mxarlg + m.mxmalg;
    } else {
        m.nintvl = m.mxdflg + m.mxarlg;
        m.nextvl = 0;
        if (m.lextma) m.nextvl = m.mxmalg;
    }
    if (inptok) m.nmdl = m.nmdl + 1;
}

}  // namespace x13
