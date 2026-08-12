// automx.cpp -- automx.f (the pickmdl candidate search) plus the four small
// routines it is the only caller of: mdlinp.f, setamx.f, bstmdl.f, bstget.f,
// and nofcst.f. See automx.hpp for the algorithm and the walled branches.
#include "automdl/automx.hpp"

#include <cmath>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "automdl/aictst.hpp"          // tdaic/lomaic/easaic + the candidate vectors
#include "automdl/amdest.hpp"          // acf (Ljung-Box Q into ctx.autoq)
#include "automdl/automd_finalize.hpp" // ssprep_save
#include "automdl/mdlset.hpp"          // mdlint, mdlset, mkmdsn
#include "diag/amdfct.hpp"             // aape_diagnostics (amdfct.f)
#include "numeric/numeric.hpp"         // dpeq
#include "regarima/estimate.hpp"       // rgarma
#include "regarima/outlier.hpp"        // idotlr
#include "regarima/regvar.hpp"         // regvar, gtrgpt, td7var
#include "transform/transform.hpp"     // trnfcn
#include "specparse/lexstate.hpp"
#include "gen/model.hpp"             // prm PARIMA/POPR/PB/PMDL/AR/MA/DIFF
#include "gen/notset.hpp"            // prm::DNOTST
#include "gen/srslen.hpp"            // prm::PLEN
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

// automx.f:407-408 / :750-751 -- does this run replace the plain rgarma estimate
// with the AIC-regressor tests? The two call sites word the condition
// differently (:407 leads with Itdtst, :750 with Leastr) but test the same set.
bool amx_has_aictest(const X13Context& ctx) {
    const auto& ar = ctx.arima;
    return ar.itdtst > 0 || ar.leastr || ar.lomtst > 0 ||
           (ar.luser && ctx.usrreg.ncusrx > 0);
}

// usraic (user-regressor AIC) and chkchi (chi-square user-holiday) have no C++
// at all; automd.cpp and aictst.cpp's explicit path decline them the same way.
// Fatal rather than silently skipping a test that changes the model.
bool amx_aic_unported(const X13Context& ctx) {
    const auto& ar = ctx.arima;
    return ar.ch2tst && ctx.usrreg.nguhl > 0;
}

// automx.f:404-500 and :748-845 -- the AIC-regressor tests standing in for the
// plain rgarma. tdaic/lomaic/easaic each self-estimate, so there is no rgarma
// call on this branch; `argok` becomes `.not.lester`. Unlike arima.f's explicit
// path there is no ssprep between the tests -- automx's second call site takes
// ONE ssprep before the whole block (:748) and the in-loop one takes none, the
// per-candidate `ssprep_save` at the head of the loop having already run.
void amx_aictest(X13Context& ctx, double* trnsrs, double* a, int& nefobs,
                 int& na, int& frstry, bool& lester) {
    auto& ar = ctx.arima;
    if (ar.itdtst > 0) {
        int tdauto = 0;
        // arima.f:125 -- `ltdlom = Kfulsm.eq.2`, i.e. the pseudo-additive mode
        // takes the length-of-month regressor where the others take Leap Year.
        const bool ltdlom = ctx.x11opt.kfulsm == 2;
        tdaic(ctx, trnsrs, a, nefobs, na, frstry, tdauto, ltdlom, lester,
              /*lsumm=*/false);
        if (ctx.error.lfatal) return;
    }
    if (!lester && ar.lomtst > 0) {
        lomaic(ctx, trnsrs, a, nefobs, na, frstry, lester, /*lsumm=*/false);
        if (ctx.error.lfatal) return;
    }
    if (!lester && ar.leastr) {
        easaic(ctx, trnsrs, a, nefobs, na, frstry, lester, /*lsumm=*/false);
        if (ctx.error.lfatal) return;
    }
    // automx.f:465-483. Lsumm is a literal 0 at every automx/automd call site,
    // so the per-candidate AICC table is NOT written here -- only arima.f's
    // explicit path passes it true.
    if (!lester && ar.luser && ctx.usrreg.ncusrx > 0) {
        usraic(ctx, trnsrs, a, nefobs, na, frstry, lester, /*lsumm=*/false);
        if (ctx.error.lfatal) return;
        // automx.f:482 -- usraic may empty the user design, and chkchi has
        // nothing left to test if it did.
        if (ctx.usrreg.ncusrx == 0 && ar.ch2tst) ar.ch2tst = false;
    }
}

// automx.f:266-296 -- rebuild the transformed series and the prior-adjustment
// factors to match a NEW setting of Picktd. The AIC trading-day test owns
// Picktd, and with a log transform the length-of-month / leap-year prior is
// applied only when a TD regressor is NOT in the model -- so flipping Picktd
// between candidates changes the series being modelled, not just the design.
// `Priadj` follows: 4 (the program-supplied TD prior) or 1 (none).
void amx_picktd_rebuild(X13Context& ctx, double* trnsrs) {
    using namespace prm;
    auto& ar = ctx.arima;
    auto& m = ctx.model;
    auto& aj = ctx.adj;
    auto& pu = ctx.priusr;
    auto& pad = ctx.priadj;
    const int nspobs = ctx.mdldat.nspobs;

    if (ctx.picktd.picktd) {
        auto begrgm = std::make_unique<bool[]>(PLEN);
        if (ctx.picktd.lrgmtd && (ctx.picktd.tdzero % 2) != 0) {
            gtrgpt(ctx, aj.begadj.data(), ctx.picktd.tddate.data(),
                   ctx.picktd.tdzero, begrgm.get(), aj.nadj);
            if (ctx.error.lfatal) return;
        } else {
            setlg(true, PLEN, begrgm.get());
        }
        td7var(aj.begadj.data(), m.sp, aj.nadj, 1, 1, false, false, true,
               &aj.adj(1), begrgm.get());
        if (pu.nustad > 0)
            eltfcn(ELT_MULT, &aj.adj(1), &pad.usrtad(pu.frstat), nspobs, &aj.adj(1));
        if (pu.nuspad > 0)
            eltfcn(ELT_MULT, &aj.adj(1), &pad.usrpad(pu.frstap), nspobs, &aj.adj(1));
        eltfcn(ELT_DIV, &ar.y(ar.frstsy), &aj.adj(aj.adj1st), nspobs, trnsrs);
        ctx.prior.priadj = 4;
    } else {
        if (pu.nustad > 0 || pu.nuspad > 0) {
            if (pu.nustad > 0)
                eltfcn(ELT_DIV, &ar.y(ar.frstsy), &pad.usrtad(pu.frstat), nspobs,
                       trnsrs);
            if (pu.nuspad > 0)
                eltfcn(ELT_DIV, &ar.y(ar.frstsy), &pad.usrpad(pu.frstap), nspobs,
                       trnsrs);
        } else {
            copy(&ar.y(ar.frstsy), nspobs, -1, trnsrs);
        }
        ctx.prior.priadj = 1;
    }
    const int ntrn = (m.lmvaft || m.ln0aft) ? nspobs : ctx.extend.nobspf;
    trnfcn(ctx, trnsrs, ntrn, ar.fcntyp, ar.lam, trnsrs);
}

}  // namespace

void automx(X13Context& ctx, double* trnsrs, int& frstry, int& nefobs,
            double* a, int& na, bool& hvmdl, int& hvstar, bool lsadj,
            bool lidotl) {
    auto& ar = ctx.arima;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    if (amx_aic_unported(ctx)) {
        fatal(ctx, "pickmdl{} with user-defined holiday chi-square testing is "
                   "not yet ported (chkchi.f has no C++; automx.f:484-500 runs "
                   "it inside the candidate loop). The user-regressor half of "
                   "this wall came down when usraic.f was ported.");
        return;
    }
    const bool laictst = amx_has_aictest(ctx);

    // The oracle builds these in the editor, once, before automx is entered.
    if (laictst && ar.itdtst > 0) aictest_td_vectors(ctx);
    if (laictst && ar.leastr) aictest_eas_vectors(ctx);

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

    // automx.f:97-100 -- the entry state of the trading-day selection. The AIC
    // test can flip Picktd per candidate, and Picktd decides whether the
    // length-of-month / leap-year prior is IN the series, so the transformed
    // series and the prior factors have to be recoverable too.
    const bool pktd = ctx.picktd.picktd;
    int padj2 = ctx.prior.priadj;
    std::vector<double> tsrs0(trnsrs, trnsrs + PLEN);
    std::vector<double> a2(&ctx.adj.adj(1), &ctx.adj.adj(1) + PLEN);

    RegSnapshot reg0;
    reg_save(ctx, reg0);

    // ---- the candidate loop (automx.f:181-688) ----------------------------
    bool estbst = false, bstptd = false, anymdl = false;
    int nummdl = 0, numbst = 0;
    bool tstmdl = true;
    bool gsovdf = false;
    bool argok = true;
    bool id = false;
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
            // automx.f:259-296 -- the winner's Picktd is not the live one, so
            // put the series and the prior factors back the way that model saw
            // them before re-estimating it.
            if (bstptd != ctx.picktd.picktd) {
                // This branch was WALLED for two increments, and the wall was
                // never about the transcription below -- it is verbatim
                // automx.f:259-296. The cause was structural and upstream, and
                // it is worth keeping the shape of it here because the branch
                // has no other gate: reaching it at all needs the AIC
                // trading-day verdict to DIFFER between candidates, which on
                // this corpus takes `regression{aicdiff=}` tuned between two
                // candidates' AICC gaps (18.33 18.82 18.85 18.49 20.20 on
                // airline, so 19.0 splits them).
                //
                // The failure was: d10, d12 and d16 BIT-EXACT and only d11/d13
                // moving, on FEBRUARIES ONLY, by exactly engine = oracle *
                // (days-in-Feb / 28.25). The restore below puts Adj back to its
                // entry value (all-1) while Sprior must keep the prior
                // tdaic.f:600-623 wrote during model selection -- and that
                // write was DEAD, because Setpri was assigned only in
                // x11_prestage, after the model stage, so its `Setpri >= 1`
                // guard never fired. The post-model x11int copy compensated,
                // correctly whenever Adj == Sprior at that point, which is
                // every other spec in the corpus: a saturated precondition, not
                // a proof. Setpri now comes from run_pre_model, ahead of the
                // model stage as in the oracle, and the compensating copy is
                // suppressed on the model path. See M5_PORT_NOTES entry 55 for
                // the instrumented-oracle trace, and entry 57 for the fix.
                ctx.picktd.picktd = bstptd;
                if (bstptd == pktd) {
                    copy(tsrs0.data(), PLEN, 1, trnsrs);
                    copy(a2.data(), PLEN, 1, &ctx.adj.adj(1));
                    // **CB-34**: automx.f:264 is `padj2=Priadj`, where its two
                    // sibling restores (:330, :725) are `Priadj=padj2`. The
                    // series and the factors have just gone back to their entry
                    // values, so Priadj should follow; instead the ENTRY value
                    // is overwritten with the last candidate's, leaving Priadj
                    // describing a series that is no longer there AND
                    // corrupting the saved value for a later restore.
                    // Transcribed as written.
                    padj2 = ctx.prior.priadj;
                } else {
                    amx_picktd_rebuild(ctx, trnsrs);
                    if (ctx.error.lfatal) return;
                }
            }
        } else {
            mdlint(ctx);
            if (ctx.error.lfatal) return;
            // automx.f:302-324 -- with per-candidate identification the design
            // has to go back to the original columns first.
            //
            // `Itdtst.gt.0` is part of the guard and was DROPPED here once. A
            // spec with `aictest=` but no `outlier{}` has lidotl false, so the
            // restore was skipped and every regressor the previous candidate's
            // tdaic/easaic had selected stayed in the design. Invisible while
            // only `aictest=(td)` was gated -- tdaic replaces the TD group
            // itself each round -- and it surfaced the moment svaict started
            // reporting `aictest.diff.td`: on extra/airline_pickmdl-aictest-tdeas
            // the leaked EASTER column moved the last candidate's TD test to
            // 22.3677 against the oracle's 20.1970, which is the value the
            // easter-free sibling also has. Note the Fortran's guard here is
            // (Lidotl .or. Itdtst.gt.0) with no Leastr -- unlike label 20's,
            // which carries all three; transcribed as written.
            if (!ar.id1st && (lidotl || ar.itdtst > 0) && nummdl > 0) {
                reg_restore(ctx, reg0);
                // automx.f:326-331 -- and the series with it, if the previous
                // candidate's AIC test moved Picktd.
                if (pktd != ctx.picktd.picktd) {
                    copy(tsrs0.data(), PLEN, 1, trnsrs);
                    copy(a2.data(), PLEN, 1, &ctx.adj.adj(1));
                    ctx.picktd.picktd = pktd;
                    ctx.prior.priadj = padj2;
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
            // automx.f:345 -- assigned only when a new candidate is read; the
            // re-estimation pass keeps the last value (and gates on !estbst).
            id = (ar.id1st && nummdl == 1) || !ar.id1st;
        }

        int nrxy2 = 0;
        regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
               ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
               ar.reglom, nrxy2, ar.begxy.data(), frstry, true, ar.elong);
        if (ctx.error.lfatal) return;

        // automx.f:404-503 -- the AIC-regressor tests REPLACE the plain estimate
        // for a freshly identified candidate; the aic routines self-estimate, so
        // `argok` is just "no estimation error inside them".
        argok = true;
        bool lester = false;
        if (laictst && id && !estbst) {
            amx_aictest(ctx, trnsrs, a, nefobs, na, frstry, lester);
            if (ctx.error.lfatal) return;
            argok = !lester;
            // automx.f:420-427 etc -- with identification on the first model
            // only, an estimation error inside the tests ends the whole search.
            if (lester && ar.id1st) {
                ar.bstdsn = std::string_view("none");
                ar.nbstds = 4;
                hvmdl = false;
                return;
            }
        } else {
            rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
            if (ctx.error.lfatal) return;
        }

        if (!argok) {
            // automx.f:663-686 -- an estimation failure only counts as an error;
            // the loop moves on to the next candidate.
            if (d.armaer < 0 || d.armaer > 1) { ++nerr; d.armaer = 0; }
            continue;
        }

        // automx.f:503 -- the re-estimation of an already-chosen winner is done.
        if (estbst) break;

        // automx.f:515 -- `Lidotl.and.id.and.argok.and.(.not.lester)`.
        if (lidotl && id && argok && !lester) {
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
    if (ar.id1st && (lidotl || ar.leastr || ar.itdtst > 0) && numbst > 1) {
        reg_restore(ctx, reg0);
        // automx.f:721-726 -- and the series, if the AIC test moved Picktd.
        if (pktd != ctx.picktd.picktd) {
            copy(tsrs0.data(), PLEN, 1, trnsrs);
            copy(a2.data(), PLEN, 1, &ctx.adj.adj(1));
            ctx.picktd.picktd = pktd;
            ctx.prior.priadj = padj2;
        }
        set_intvl(ctx);
        int nrxy2 = 0;
        regvar(ctx, trnsrs, ctx.extend.nobspf, ar.fctdrp, ctx.extend.nfcst, 0,
               ar.userx.data(), ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj,
               ar.reglom, nrxy2, ar.begxy.data(), frstry, true, ar.elong);
        if (ctx.error.lfatal) return;
        argok = true;
        // automx.f:748-847 -- the AIC tests again, this time behind ONE ssprep.
        if (laictst) {
            ssprep_save(ctx);
            bool lester = false;
            amx_aictest(ctx, trnsrs, a, nefobs, na, frstry, lester);
            if (ctx.error.lfatal) return;
            if (lester) {
                ar.bstdsn = std::string_view("none");
                ar.nbstds = 4;
                hvmdl = false;
                return;
            }
        } else {
            rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a, na, nefobs, argok);
            if (ctx.error.lfatal) return;
        }
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

    // automx.f:903-928 -- re-score the SELECTED model over the BACKCAST span and,
    // if it fails, drop backcasting for the run. Skipped when the starred
    // default was used (hvstar==2), which has forecasting off already.
    if (ctx.extend.nbcst > 0 && hvstar != 2) {
        aape_diagnostics(ctx, trnsrs, &argok, /*bckcst=*/true);
        if (ctx.error.lfatal) return;
        // prtamd prints these and writes no savelog key; publish them so the
        // gate can read the printed block instead of nothing.
        ctx.aape_bcst = ctx.aape;
        ctx.aape_bcst_ran = true;
        // **CB-33** -- `IF(mape(4).gt.Bcklim.and.(.not.argok))`. The `.and.` is
        // almost certainly a slip for `.or.`: the whole point of `bcstlim=` is to
        // un-select a model whose backward extrapolation is poor, and the
        // neighbouring comment ("check to see if argok false and print out error
        // message for backcasts") reads as two independent reasons. As written,
        // a model that CONVERGED can never fail the screen no matter how bad the
        // backcast error is -- and prtamd, which evaluates the screens itself,
        // says so out loud. Measured on the oracle with `bcstlim=1`: the .out
        // prints "MODEL 2 REJECTED: Average backcast error > 1.00%" and then
        // "The model chosen is (0 1 2)(0 1 1)", and the table footer still reads
        // "Includes 12 backcasts". Transcribed with the `&&`.
        if (ctx.aape.mape[3] > ar.bcklim && !argok) {
            ctx.extend.nbcst = 0;
            ctx.x11ptr.pos1bk = ctx.x11ptr.pos1ob;
        }
    }
}

}  // namespace x13
