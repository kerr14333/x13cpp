// automx.cpp -- automx.f (the pickmdl candidate search) plus the four small
// routines it is the only caller of: mdlinp.f, setamx.f, bstmdl.f, bstget.f,
// and nofcst.f. See automx.hpp for the algorithm and the walled branches.
#include "automdl/automx.hpp"

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "automdl/amdest.hpp"          // acf (Ljung-Box Q into ctx.autoq)
#include "automdl/automd_finalize.hpp" // ssprep_save
#include "automdl/mdlset.hpp"          // mdlint, mdlset, mkmdsn
#include "diag/amdfct.hpp"             // aape_diagnostics (amdfct.f)
#include "numeric/numeric.hpp"         // dpeq
#include "regarima/estimate.hpp"       // rgarma
#include "regarima/outlier.hpp"        // idotlr
#include "regarima/regvar.hpp"         // regvar
#include "specparse/lexstate.hpp"
#include "gen/model.hpp"             // prm PARIMA/POPR/PB/PMDL/AR/MA/DIFF
#include "gen/notset.hpp"            // prm::DNOTST
#include "specparse/specparse.hpp"     // getmdl, getstr, maxlag
#include "x11/x11drv.hpp"              // setxpt

namespace x13 {
namespace {

using namespace prm;

// Raise a run-ending error the way x11parts.cpp's x11_not_ported does: a
// message on the error channel plus abend, so the harness reports OUTCOME:
// ERROR rather than OK-with-wrong-numbers.
void fatal(X13Context& ctx, const std::string& what) {
    errhdr(ctx);
    writln(ctx, "ERROR: " + what, stdio::STDERR, ctx.units.mt2, true);
    abend(ctx);
}

// ---- mdlinp.f -------------------------------------------------------------
// Point the lexer at the candidate-model file. The Fortran REWINDs Inputx onto
// the new unit; this port's lexer reads from an in-memory line vector, so the
// equivalent is to replace it and re-run intinp.f's reset sequence. Nothing
// re-reads the spec text after gtinpt has returned, so the overwrite is safe.
bool mdlinp(X13Context& ctx, const std::string& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    LexState& L = ctx.lex;
    L.load(ss.str());
    // rngbuf's SAVEd ring buffer has to be reset with the source, or the first
    // lex() returns lines left over from the spec file.
    L.begbuf = 0;
    L.endbuf = 0;
    L.crntbf = 0;
    L.crntln = 0;
    L.psteof = false;
    L.lineno = 0;
    L.lineln = 0;
    bool ldmy = rngbuf(ctx, 1, L.lineno, L.linex, L.lineln);
    if (!ldmy || ctx.error.lfatal) return false;
    L.pos[lexprm::PLINE] = 0;
    L.pos[lexprm::PCHAR] = 1;
    L.lstpos[lexprm::PLINE] = 0;
    L.lstpos[lexprm::PCHAR] = 1;
    L.errpos[lexprm::PLINE] = 0;
    L.errpos[lexprm::PCHAR] = 1;
    lex(ctx);
    return !ctx.error.lfatal;
}

// ---- setamx.f -------------------------------------------------------------
// The five built-in candidates, used when pickmdl{} carries no `file=`. They
// are the X-11-ARIMA/88 list and are exactly what tests/corpus/extra/pickmdl.mdl
// spells out. `Lseff` (stable seasonal effects in the regression) drops the
// seasonal difference and MA to 0.
void setamx(X13Context& ctx, int mdindx, bool lseff, bool& locok, bool& inptok) {
    const int sdiff = lseff ? 0 : 1;
    const int sma = lseff ? 0 : 1;
    int nsar = 0, nsdiff = 1, nsma = 1;
    switch (mdindx) {
    case 1: nsar = 0; nsdiff = 1; nsma = 1; break;
    case 2: nsar = 0; nsdiff = 1; nsma = 2; break;
    case 3: nsar = 2; nsdiff = 1; nsma = 0; break;
    case 4: nsar = 0; nsdiff = 2; nsma = 2; break;
    default: nsar = 2; nsdiff = 1; nsma = 2; break;   // 5
    }
    locok = true;
    mdlset(ctx, nsar, nsdiff, nsma, 0, sdiff, sma, locok);
    inptok = inptok && locok;
}

// automx.f:359-371 and bstget.f:80-96 -- the shared "recompute the effective
// observation split" tail. Lextar/Lextma decide whether the AR/MA lags are
// consumed from the interval or extended past it.
void set_intvl(X13Context& ctx) {
    auto& m = ctx.model;
    m.lar = m.lextar && m.mxarlg > 0;
    m.lma = m.lextma && m.mxmalg > 0;
    if (m.lextar) {
        m.nintvl = m.mxdflg;
        m.nextvl = m.mxarlg + m.mxmalg;
    } else {
        m.nintvl = m.mxdflg + m.mxarlg;
        m.nextvl = m.lextma ? m.mxmalg : 0;
    }
}

// ---- bstmdl.f / bstget.f --------------------------------------------------
// The best-model store. bstmdl.f zeroes every /bstcmn/ field before copying, so
// a later, SHORTER model cannot leave the previous winner's tail behind; the
// zeroing is reproduced rather than relying on assignment covering it.
void bstmdl(X13Context& ctx, int& nbstds, bool& bstptd) {
    auto& b = ctx.bstmdl;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    setlg(false, PARIMA, b.bstafx.data());
    setint(0, PARIMA, b.bstalg.data());
    setint(0, POPR, b.bstofc.data());
    setdp(0.0, PB, b.bstb.data());
    setdp(0.0, PARIMA, b.bstap.data());
    intlst(POPR, b.bsto.data(), b.bstno);
    intlst(POPR, b.bstopt.data(), b.bstnot);
    intlst(3 * PMDL, b.bstm.data(), b.bstnm);
    b.bstm(AR) = 1;
    b.bstm(MA) = 1;
    b.bstot = std::string_view("");

    cpyint(m.opr.data(), POPR + 1, 1, b.bsto.data());
    cpyint(m.oprptr.data(), POPR + 1, 1, b.bstopt.data());
    cpyint(m.oprfac.data(), POPR, 1, b.bstofc.data());
    cpyint(m.mdl.data(), 3 * PMDL + 1, 1, b.bstm.data());
    copy(d.arimap.data(), PARIMA, 1, b.bstap.data());
    copy(d.b.data(), PB, 1, b.bstb.data());
    copylg(m.arimaf.data(), PARIMA, 1, b.bstafx.data());
    cpyint(m.arimal.data(), PARIMA, 1, b.bstalg.data());
    b.bstsdf = m.lseadf;
    b.bstot = m.oprttl.raw();
    b.bstno = m.nopr;
    b.bstnot = m.noprtl;
    b.bstnm = m.nmdl;
    ctx.arima.bstdsn = m.mdldsn.raw().substr(0, static_cast<std::size_t>(m.nmddcr));
    b.bnsedf = m.nnsedf;
    b.bseadf = m.nseadf;
    nbstds = m.nmddcr;
    bstptd = ctx.picktd.picktd;

    b.bstngr = m.ngrp;
    b.bsngrt = m.ngrptl;
    b.bsncxy = m.ncxy;
    b.bstnb = m.nb;
    b.bstnct = m.ncoltl;
    b.bstctl = m.colttl.raw();
    b.bstgtl = m.grpttl.raw();
    cpyint(m.colptr.data(), PB + 1, 1, b.bclptr.data());
    cpyint(m.grp.data(), PGRP + 1, 1, b.bstgrp.data());
    cpyint(m.grpptr.data(), PGRP + 1, 1, b.bsgptr.data());
    cpyint(m.rgvrtp.data(), PB, 1, b.bstrgv.data());
}

void bstget(X13Context& ctx, int nbstds) {
    auto& b = ctx.bstmdl;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    setlg(false, PARIMA, m.arimaf.data());
    setint(0, PARIMA, m.arimal.data());
    setint(0, POPR, m.oprfac.data());
    setdp(0.0, PB, d.b.data());
    setdp(0.0, PARIMA, d.arimap.data());
    intlst(POPR, m.opr.data(), m.nopr);
    intlst(POPR, m.oprptr.data(), m.noprtl);
    intlst(3 * PMDL, m.mdl.data(), m.nmdl);
    m.mdl(AR) = 1;
    m.mdl(MA) = 1;
    m.oprttl = std::string_view("");

    cpyint(b.bsto.data(), POPR + 1, 1, m.opr.data());
    cpyint(b.bstopt.data(), POPR + 1, 1, m.oprptr.data());
    cpyint(b.bstofc.data(), POPR, 1, m.oprfac.data());
    cpyint(b.bstm.data(), 3 * PMDL + 1, 1, m.mdl.data());
    copy(b.bstap.data(), PARIMA, 1, d.arimap.data());
    copy(b.bstb.data(), PB, 1, d.b.data());
    copylg(b.bstafx.data(), PARIMA, 1, m.arimaf.data());
    cpyint(b.bstalg.data(), PARIMA, 1, m.arimal.data());
    m.lseadf = b.bstsdf;
    m.oprttl = b.bstot.raw();
    m.nopr = b.bstno;
    m.noprtl = b.bstnot;
    m.nmdl = b.bstnm;
    m.mdldsn = ctx.arima.bstdsn.raw().substr(0, static_cast<std::size_t>(nbstds));
    m.nnsedf = b.bnsedf;
    m.nseadf = b.bseadf;
    m.nmddcr = nbstds;

    m.ngrp = b.bstngr;
    m.ngrptl = b.bsngrt;
    m.ncxy = b.bsncxy;
    m.nb = b.bstnb;
    m.ncoltl = b.bstnct;
    m.colttl = b.bstctl.raw();
    m.grpttl = b.bstgtl.raw();
    cpyint(b.bclptr.data(), PB + 1, 1, m.colptr.data());
    cpyint(b.bstgrp.data(), PGRP + 1, 1, m.grp.data());
    cpyint(b.bsgptr.data(), PGRP + 1, 1, m.grpptr.data());
    cpyint(b.bstrgv.data(), PB, 1, m.rgvrtp.data());

    // bstget.f:68-78 -- clear the parameters so the re-estimation starts from
    // scratch rather than from wherever the LAST candidate happened to leave
    // this model's slots.
    for (int i = 1; i <= m.nb; ++i) d.b(i) = DNOTST;
    if (m.nopr > 0) {
        int endlag = m.opr(m.nopr) - 1;
        for (int ilag = 1; ilag <= endlag; ++ilag)
            if (!m.arimaf(ilag)) d.arimap(ilag) = DNOTST;
    }

    maxlag(m.arimal.data(), m.opr.data(), m.mdl(DIFF - 1), m.mdl(DIFF) - 1,
           m.mxdflg);
    maxlag(m.arimal.data(), m.opr.data(), m.mdl(AR - 1), m.mdl(AR) - 1,
           m.mxarlg);
    maxlag(m.arimal.data(), m.opr.data(), m.mdl(MA - 1), m.mdl(MA) - 1,
           m.mxmalg);
    // bstget.f:80-96 -- the effective-observation split belongs HERE, not at
    // the call site. Leaving it out crashed the re-estimation pass: Nintvl
    // still described the LAST candidate, so rgarma read past the differenced
    // series when the winner had a different differencing order.
    set_intvl(ctx);
}

// ---- nofcst.f -------------------------------------------------------------
// Zero the forecast/backcast counts and rebuild every pointer and the design
// around them. Reached only on the hvstar==2 fallback: a default model kept
// solely to produce regARIMA preadjustment factors does not forecast.
void nofcst(X13Context& ctx, double* trnsrs, int& frstry, bool lx11) {
    auto& ar = ctx.arima;
    auto& ex = ctx.extend;
    const int nf2 = ex.nfcst;
    ex.nfcst = 0;
    if (ex.nbcst > 0) ex.nbcst = 0;
    if (ex.nfdrp > 0) ex.nfdrp = 0;
    setxpt(ctx, nf2, lx11, ar.fctdrp);
    if (ctx.error.lfatal) return;
    regvar(ctx, trnsrs, ex.nobspf, ar.fctdrp, ex.nfcst, 0, ar.userx.data(),
           ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj, ar.reglom, ar.nrxy,
           ar.begxy.data(), frstry, true, ar.elong);
}

// ---- the design snapshot automx keeps across candidates -------------------
// automx.f:163-177 saves the regression DICTIONARY (not the coefficients --
// those live in /bstcmn/) so that `identify=all`, which lets each candidate run
// its own outlier identification, can put the original columns back before the
// next one. Same field set restor.f:50-64 carries.
struct RegSnapshot {
    int ngrp = 0, ngrptl = 0, ncxy = 0, nb = 0, ncoltl = 0;
    fstring<PCOLCR * PB> colttl;
    fstring<PGRPCR * PGRP> grpttl;
    std::vector<int> colptr, grp, grpptr, rgvrtp;
    std::vector<double> b;
};

void reg_save(const X13Context& ctx, RegSnapshot& s) {
    const auto& m = ctx.model;
    s.ngrp = m.ngrp;
    s.ngrptl = m.ngrptl;
    s.ncxy = m.ncxy;
    s.nb = m.nb;
    s.ncoltl = m.ncoltl;
    s.colttl = m.colttl.raw();
    s.grpttl = m.grpttl.raw();
    s.colptr.assign(m.colptr.data(), m.colptr.data() + PB + 1);
    s.grp.assign(m.grp.data(), m.grp.data() + PGRP + 1);
    s.grpptr.assign(m.grpptr.data(), m.grpptr.data() + PGRP + 1);
    s.rgvrtp.assign(m.rgvrtp.data(), m.rgvrtp.data() + PB);
    s.b.assign(ctx.mdldat.b.data(), ctx.mdldat.b.data() + PB);
}

void reg_restore(X13Context& ctx, const RegSnapshot& s) {
    auto& m = ctx.model;
    m.ngrp = s.ngrp;
    m.ngrptl = s.ngrptl;
    m.ncxy = s.ncxy;
    m.nb = s.nb;
    m.ncoltl = s.ncoltl;
    m.colttl = s.colttl.raw();
    m.grpttl = s.grpttl.raw();
    cpyint(s.colptr.data(), PB + 1, 1, m.colptr.data());
    cpyint(s.grp.data(), PGRP + 1, 1, m.grp.data());
    cpyint(s.grpptr.data(), PGRP + 1, 1, m.grpptr.data());
    cpyint(s.rgvrtp.data(), PB, 1, m.rgvrtp.data());
    copy(s.b.data(), PB, 1, ctx.mdldat.b.data());
}

// automx.f:508-517 -- outlier identification on the current candidate, then a
// full-Nobspf design rebuild. Same shape as automd.cpp's amidot.
void amx_idotlr(X13Context& ctx, double* trnsrs, int& frstry, int& nefobs,
                double* a, bool& argok) {
    auto& ar = ctx.arima;
    const int sp = ctx.model.sp;
    int begtst[2] = {ctx.mdldat.begspn(1), ctx.mdldat.begspn(2)};
    int endtst[2];
    addate(ctx.mdldat.begspn.data(), sp, ctx.mdldat.nspobs - 1, endtst);
    if (dpeq(ctx.model.tcalfa, DNOTST))
        ctx.model.tcalfa = std::pow(0.7, 12.0 / sp);
    int nobtst = 0;
    dfdate(endtst, begtst, sp, nobtst);
    nobtst += 1;
    double cv = setcv(nobtst, ar.cvalfa);
    for (int t = 1; t <= POTLR; ++t)
        if (dpeq(ar.critvl(t), DNOTST)) ar.critvl(t) = cv;
    double critvl[POTLR] = {ar.critvl(1), ar.critvl(2), ar.critvl(3)};
    idotlr(ctx, ar.ltstao, ar.ltstls, ar.ltsttc, ar.ladd1, critvl, ar.cvrduc,
           begtst, endtst, nefobs, ar.lestim, ar.mxiter, ar.mxnlit,
           /*lauto=*/false, a);
    if (ctx.error.lfatal) return;
    if (!ctx.mdldat.convrg) { argok = false; return; }
    int nrxy2 = 0;
    regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
           ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
           ar.reglom, nrxy2, ar.begxy.data(), frstry, true, ar.elong);
}

// automx.f:600-628 -- sum each MA operator's coefficients. Overdifferencing is
// judged separately for the nonseasonal and seasonal MA, but only the
// NONSEASONAL verdict gates acceptance (:643); the seasonal one drives a
// warning (:692).
void ma_sums(X13Context& ctx, double& rma, double& sma) {
    auto& m = ctx.model;
    rma = 0.0;
    sma = 0.0;
    const int begopr = m.mdl(MA - 1);
    const int endopr = m.mdl(MA) - 1;
    std::string ttl;
    int ntmp = 0;
    for (int iopr = begopr; iopr <= endopr; ++iopr) {
        const int beglag = m.opr(iopr - 1);
        const int endlag = m.opr(iopr) - 1;
        getstr(ctx, m.oprttl.data(), m.oprptr.data(), m.noprtl, iopr, ttl, ntmp);
        if (ctx.error.lfatal) return;
        if (ttl == "Seasonal MA") {
            for (int ilag = beglag; ilag <= endlag; ++ilag) sma += ctx.mdldat.arimap(ilag);
        } else if (ttl == "Nonseasonal MA") {
            for (int ilag = beglag; ilag <= endlag; ++ilag) rma += ctx.mdldat.arimap(ilag);
        }
    }
}

// automx.f:404-411 / :748-753 -- the AIC-regressor tests inside the candidate
// loop. Walled: each candidate would re-run tdaic/easaic/lomaic/usraic, which
// can flip Picktd between candidates and so drags in the trnsrs/Adj restore at
// :255-292 as well.
bool amx_aic_walled(const X13Context& ctx) {
    const auto& ar = ctx.arima;
    return ar.itdtst > 0 || ar.leastr || ar.lomtst > 0 ||
           (ar.luser && ctx.usrreg.ncusrx > 0) ||
           (ar.ch2tst && ctx.usrreg.nguhl > 0);
}

}  // namespace

void automx(X13Context& ctx, double* trnsrs, int& frstry, int& nefobs,
            double* a, int& na, bool& hvmdl, int& hvstar, bool lsadj,
            bool lidotl) {
    auto& ar = ctx.arima;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    if (amx_aic_walled(ctx)) {
        fatal(ctx, "pickmdl{} with regression{aictest=} / user or holiday "
                   "chi-square testing is not yet ported (automx.f:404-500 "
                   "runs the AIC tests inside the candidate loop).");
        return;
    }

    hvmdl = false;
    double loclim = ar.fctlim;
    int nerr = 0;
    bool inptok = true;

    const std::string autofl = ar.autofl.str();
    const bool havfil = !(autofl.size() >= 1 && autofl[0] == '?');
    if (havfil) {
        if (!mdlinp(ctx, autofl)) {
            if (ctx.error.lfatal) return;
            fatal(ctx, "Must have user supplied models stored in " + autofl + ".");
            return;
        }
    }

    const bool pktd = ctx.picktd.picktd;

    RegSnapshot reg0;
    reg_save(ctx, reg0);

    // ---- the candidate loop (automx.f:181-688) ----------------------------
    bool estbst = false, bstptd = false, anymdl = false;
    int nummdl = 0, numbst = 0;
    bool tstmdl = true;
    bool gsovdf = false;
    bool argok = true;
    int nbstds = 0;

    while (tstmdl) {
        bool mdskip = false;
        const bool exhausted =
            (havfil && ctx.lex.nxtktp == lexprm::EOFTOK) ||
            (!havfil && nummdl == 5);

        if (exhausted) {
            if (havfil && nummdl == 0) {
                fatal(ctx, "No ARIMA models stored in " + autofl + ".");
                return;
            }
            if (!hvmdl && hvstar == 0) {
                ar.bstdsn = std::string_view("none");
                ar.nbstds = 4;
                return;
            }
            if (!anymdl) {
                fatal(ctx, "Every pickmdl candidate model failed to estimate.");
                return;
            }
            // automx.f:246 -- the winner is already the live model when it was
            // the LAST one estimated; otherwise reload and re-estimate it.
            if (nummdl == numbst) break;
            bstget(ctx, nbstds);
            estbst = true;
            if (bstptd != ctx.picktd.picktd) {
                fatal(ctx, "pickmdl{}: the Picktd trading-day restore "
                           "(automx.f:255-292) is not ported; it is reachable "
                           "only with regression{aictest=(td)}, which is "
                           "walled above.");
                return;
            }
        } else {
            mdlint(ctx);
            if (ctx.error.lfatal) return;
            // automx.f:302-324 -- with per-candidate identification the design
            // has to go back to the original columns first.
            if (!ar.id1st && lidotl && nummdl > 0) {
                reg_restore(ctx, reg0);
                if (pktd != ctx.picktd.picktd) {
                    fatal(ctx, "pickmdl{}: the Picktd per-candidate restore "
                               "(automx.f:317-323) is not ported.");
                    return;
                }
            }
            // automx.f:335-341 -- skip forward to the model's opening paren.
            if (havfil) {
                while (ctx.lex.nxtktp != lexprm::LPAREN) {
                    lex(ctx);
                    if (ctx.error.lfatal) return;
                    if (ctx.lex.nxtktp == lexprm::EOFTOK) break;
                }
                if (ctx.lex.nxtktp == lexprm::EOFTOK) continue;
            }
            ++nummdl;
            argok = true;
            if (havfil) {
                getmdl(ctx, argok, inptok, true);
                if (ctx.error.lfatal) return;
            } else {
                setamx(ctx, nummdl, m.lseff, argok, inptok);
                if (ctx.error.lfatal) return;
            }
            set_intvl(ctx);
            ssprep_save(ctx);
            if (!argok) continue;
            // automx.f:382-391 -- a trailing `*` marks the DEFAULT model. Only
            // the first star counts; without a file the first built-in is it.
            if (havfil && ctx.lex.nxtktp == lexprm::STAR) {
                if (hvstar == 0) hvstar = 1;
            } else if (!havfil && nummdl == 1) {
                hvstar = 1;
            }
        }

        int nrxy2 = 0;
        regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
               ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
               ar.reglom, nrxy2, ar.begxy.data(), frstry, true, ar.elong);
        if (ctx.error.lfatal) return;

        argok = true;
        rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
        if (ctx.error.lfatal) return;

        if (!argok) {
            // automx.f:663-686 -- an estimation failure only counts as an error;
            // the loop moves on to the next candidate.
            if (d.armaer < 0 || d.armaer > 1) { ++nerr; d.armaer = 0; }
            continue;
        }

        // automx.f:503 -- the re-estimation of an already-chosen winner is done.
        if (estbst) break;

        const bool id = (ar.id1st && nummdl == 1) || !ar.id1st;
        if (lidotl && id) {
            amx_idotlr(ctx, trnsrs, frstry, nefobs, a, argok);
            if (ctx.error.lfatal) return;
            if (!argok && ar.id1st) {
                ar.bstdsn = std::string_view("none");
                ar.nbstds = 4;
                hvmdl = false;
                return;
            }
        }

        if (d.armaer < 0 || d.armaer > 1) {
            ++nerr;
            nefobs = d.nspobs - m.nintvl;
            // automx.f:637-645 -- some estimation errors disqualify the model
            // from the search entirely.
            if (d.armaer == PMXIER || d.armaer == PSNGER || d.armaer == PISNER ||
                d.armaer == PNIFER || d.armaer == PNIMER || d.armaer == PCNTER ||
                d.armaer == POBFN0 || d.armaer == PACSER || d.armaer < 0) {
                mdskip = true;
                if (hvstar == 1) hvstar = 0;
            }
            d.armaer = 0;
        }
        anymdl = anymdl || argok;
        if (!argok && !mdskip) mdskip = true;
        if (mdskip) continue;

        // ---- score the candidate (automx.f:562-628) ----------------------
        // automx.f:575 passes `argok` as Lauto: on the out-of-sample path a
        // failed re-estimation clears it and the candidate is dropped.
        aape_diagnostics(ctx, trnsrs, &argok);
        if (ctx.error.lfatal) return;
        const bool fctok = ctx.aape.ok;
        const double mape4 = ctx.aape.mape[3];

        double blchi = DNOTST;
        if (argok) {
            int i = (m.sp == 4) ? 12 : 24;
            int np = 0;
            const int endlag = m.opr(m.nopr) - 1;
            for (int ilag = 1; ilag <= endlag; ++ilag)
                if (!m.arimaf(ilag)) ++np;
            if (i < nefobs) {
                std::vector<double> smpac(static_cast<std::size_t>(i) + 1, 0.0);
                std::vector<double> seacf(static_cast<std::size_t>(i) + 1, 0.0);
                int nr = i;
                acf(ctx, a + (na - nefobs), nefobs, nefobs, smpac.data(),
                    seacf.data(), nr, np, m.sp, 0, true, false);
                if (ctx.error.lfatal) return;
                if (!dpeq(ctx.autoq.qpv(i), DNOTST)) blchi = ctx.autoq.qpv(i) * 100.0;
            }
        }

        bool ovrdff = false;
        if (argok) {
            double rma = 0.0, sma = 0.0;
            ma_sums(ctx, rma, sma);
            if (ctx.error.lfatal) return;
            if (m.nnsedf > 0 && rma >= ar.ovrdif) ovrdff = true;
            if (m.nseadf > 0 && sma >= ar.ovrdif) gsovdf = true;
        }

        // ---- the acceptance test (automx.f:642-657) -----------------------
        if (fctok && argok && mape4 <= loclim && blchi > ar.qlim && !ovrdff) {
            if (!hvmdl) {
                hvmdl = true;
                if (ar.pck1st) tstmdl = false;
            }
            numbst = nummdl;
            loclim = mape4;   // `method=best`: the bar tightens to this model
            bstmdl(ctx, nbstds, bstptd);
            if (hvstar == 2) hvstar = 3;
        } else if (!hvmdl && hvstar == 1 && argok) {
            // Nothing accepted yet, but this is the starred default -- keep it
            // as the fallback.
            hvstar = 2;
            numbst = nummdl;
            bstmdl(ctx, nbstds, bstptd);
        }
    }

    (void)gsovdf;   // the seasonal-overdifferencing WARNING is print surface
    (void)nerr;

    if (nbstds > 0) ar.nbstds = nbstds;

    // ---- label 20: `identify=first` re-estimation (automx.f:700-856) ------
    // With identification done on the FIRST candidate only, a later winner was
    // estimated against that model's outlier/TD columns; put the original
    // design back and re-estimate it.
    if (ar.id1st && lidotl && numbst > 1) {
        reg_restore(ctx, reg0);
        if (pktd != ctx.picktd.picktd) {
            fatal(ctx, "pickmdl{}: the Picktd restore at automx.f:717-724 is "
                       "not ported.");
            return;
        }
        set_intvl(ctx);
        int nrxy2 = 0;
        regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
               ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
               ar.reglom, nrxy2, ar.begxy.data(), frstry, true, ar.elong);
        if (ctx.error.lfatal) return;
        argok = true;
        rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
        if (ctx.error.lfatal) return;
        if (!argok) {
            fatal(ctx, "pickmdl{}: the selected model failed to re-estimate.");
            return;
        }
        if (lidotl) {
            amx_idotlr(ctx, trnsrs, frstry, nefobs, a, argok);
            if (ctx.error.lfatal) return;
            if (!argok) {
                ar.bstdsn = std::string_view("none");
                ar.nbstds = 4;
                hvmdl = false;
                return;
            }
        }
    }

    // ---- the starred-default fallback (automx.f:874-897) ------------------
    if (!hvmdl && hvstar == 2) {
        const auto& adj = ctx.x11adj;
        const bool needed =
            adj.adjtd == 1 || (adj.adjao == 1 && adj.nao > 0) ||
            (adj.adjls == 1 && (adj.nls > 0 || adj.nramp > 0)) ||
            (adj.adjtc == 1 && adj.ntc > 0) || (adj.adjso == 1 && adj.nso > 0) ||
            adj.adjsea == 1 || adj.adjusr == 1 || adj.adjhol == 1 ||
            adj.finusr || adj.finao || adj.finls || adj.fintc || adj.finhol;
        if (needed) {
            hvmdl = true;
            nofcst(ctx, trnsrs, frstry, lsadj);
            if (ctx.error.lfatal) return;
        } else {
            ar.bstdsn = std::string_view("none");
            ar.nbstds = 4;
            return;
        }
    }

    // automx.f:903-928's backcast pass re-scores the winner over the BACKCAST
    // span, which amdfct's Bckcst arm computes -- unported alongside the
    // out-of-sample arm. Reject rather than report a forecast-only number.
    if (ctx.extend.nbcst > 0 && hvstar != 2) {
        fatal(ctx, "pickmdl{} with forecast{maxback=} is not yet ported: the "
                   "backcast acceptance pass (automx.f:903-928) needs amdfct's "
                   "Bckcst arm.");
        return;
    }
}

}  // namespace x13
