// trnaic.cpp -- automatic transform selection, ported from oracle/fortran/trnaic.f
// (see trnaic.hpp for scope). The routine estimates the default airline model
// under two transforms and keeps the one with the lower AICC.
//
// Sequence (trnaic.f, reduced to the transform=auto + automdl path):
//   1. Fcntyp=4, Lam=1  -> untransformed default airline model.
//   2. mdlint/mdlset build (0 1 1)(0 1 1); Ap1 backs up the initial ARMA coefs
//      (editor.f:918, done here since the reduced driver skips editor).
//   3. regvar + rgarma estimate; prlkhd -> aicno (Aicc, no Jacobian: Fcntyp=4).
//   4. Restore Ap1 into Arimap so the log fit restarts from the same seeds.
//   5. Fcntyp=1, Lam=0, Adjmod=1; log-transform the series; regvar + rgarma;
//      prlkhd -> aiclog (Aicc including the log Jacobian -sum log y).
//   6. Pick log iff aiclog+Traicd < aicno; otherwise revert to no transform.
//
// Deferred (touch no numeric state on this path, or are print/save only):
//   - Ixreg = -Ixreg xreg toggle (no xreg regressors here).
//   - The picktd / Lmodel leap-year prior branches (Lmodel=false, no picktd).
//   - Model-span reset (setspn) and Begspn/Endspn juggling (no model span).
//   - The x11 prior-factor re-initialization block and every WRITE/savelog.
#include "transform/trnaic.hpp"

#include "automdl/iddiff.hpp"        // prterr
#include "automdl/mdlset.hpp"        // mdlint, mdlset
#include "regarima/estimate.hpp"     // rgarma, prlkhd
#include "regarima/regvar.hpp"       // regvar
#include "transform/transform.hpp"   // trnfcn
#include "specparse/specparse.hpp"   // copy, setdp, abend
#include "numeric/numeric.hpp"       // dpeq
#include "gen/srslen.hpp"            // prm::PLEN
#include "gen/model.hpp"             // prm::PORDER, PMXIER/... error codes
#include "gen/notset.hpp"            // prm::DNOTST

#include <vector>

namespace x13 {

namespace {
// A hard estimation error (trnaic.f:142) discontinues the routine; a mere
// warning code is reset to zero (trnaic.f:155).
bool armaer_is_fatal(int e) {
    using namespace prm;
    return e == PMXIER || e == PSNGER || e == PISNER || e == PNIFER ||
           e == PNIMER || e == PCNTER || e == POBFN0 || e < 0;
}

// One default-airline estimation under the current transform. Returns the AICC
// from prlkhd (ctx.lkhd.aicc). The model must already be built (mdlint/mdlset)
// on the first call; the caller re-transforms `trnsrs` in place between calls.
void estimate_once(X13Context& ctx, const double* trnsrs, const double* y,
                   const double* fac, int adjmod, int nobspf, double& aicc) {
    auto& ar = ctx.arima;
    int nrxy = 0, frstry = 0, nefobs = 0, na = 0;
    constexpr int PA = prm::PLEN + 2 * prm::PORDER;
    std::vector<double> a(static_cast<std::size_t>(PA), 0.0);

    regvar(ctx, trnsrs, nobspf, ar.fctdrp, ctx.extend.nfcst, 0, ar.userx.data(),
           ar.bgusrx.data(), ar.nrusrx, ctx.prior.priadj, ar.reglom, nrxy,
           ar.begxy.data(), frstry, true, ar.elong);
    if (ctx.error.lfatal) return;
    ar.nrxy = nrxy;

    bool argok = false;   // trnaic.f: argok=Lautom(=F) -> not an automatic pass
    rgarma(ctx, true, ar.mxiter, ar.mxnlit, false, a.data(), na, nefobs, argok);
    if (ctx.error.lfatal) return;

    if (armaer_is_fatal(ctx.mdldat.armaer)) {
        prterr(ctx, nefobs, false);
        abend(ctx);
        return;
    }
    ctx.mdldat.armaer = 0;   // reset warning-level indicator (trnaic.f:156)

    prlkhd(ctx, y, fac, adjmod, ar.fcntyp, ar.lam);
    if (ctx.error.lfatal) return;
    aicc = ctx.lkhd.aicc;
}
}  // namespace

void trnaic(X13Context& ctx, const double* y, int frstsy, int nspobs, int nobspf,
            bool lmodel, double& aicno, double& aiclog) {
    (void)frstsy;
    using namespace prm;
    auto& ar = ctx.arima;
    auto& m = ctx.model;
    auto& d = ctx.mdldat;

    // prlkhd reads d.nspobs for the Jacobian span; keep it aligned to the caller.
    d.nspobs = nspobs;

    // A prior factor of 1 over the span (no leap-year prior on this path); the
    // no-transform fit has a zero Jacobian, the log fit's Jacobian is -sum log y.
    std::vector<double> fac(static_cast<std::size_t>(nspobs > 0 ? nspobs : 1), 1.0);
    // Working series buffer: starts as the untransformed span series, then is
    // log-transformed in place for the second fit (trnaic.f trnsrs).
    std::vector<double> trnsrs(static_cast<std::size_t>(nobspf > 0 ? nobspf : 1));
    copy(y, nobspf, 1, trnsrs.data());

    // ---- 1. no transform (Fcntyp=4, Lam=1) --------------------------------
    ar.fcntyp = 4;
    ar.lam = 1.0;

    // Suppress automatic model identification during the two estimations
    // (trnaic.f:130); the reduced automd driver runs later, after the choice.
    bool lax2 = ar.lautox, lam2 = ar.lautom, lad2 = ar.lautod;
    ar.lautox = false;
    ar.lautom = false;
    ar.lautod = false;

    // Default airline model (0 1 1)(0 1 1); drop the seasonal factor only when an
    // explicit model with seasonal-effect regressors is present (trnaic.f:110).
    int lds0 = 1, lqs0 = 1;
    if (lmodel && m.lseff) {
        lds0 = 0;
        lqs0 = 0;
    }
    mdlint(ctx);
    bool inptok = true;
    mdlset(ctx, 0, 1, 1, 0, lds0, lqs0, inptok);
    if (ctx.error.lfatal) {
        ar.lautox = lax2; ar.lautom = lam2; ar.lautod = lad2;
        return;
    }

    // NB: the initial-ARMA backup (Ap1=Arimap) lives in editor.f:918, guarded by
    // Nopr>0. On the automatic-model path editor runs BEFORE any model exists
    // (Nopr==0), so Ap1 is never written and stays zero. It is therefore NOT
    // backed up here -- the restore below copies that zero, seeding the log fit's
    // free ARMA lags at 0 (strtvl leaves a non-sentinel value untouched), exactly
    // as the oracle does. (Backing up DNOTST here would make strtvl reseed 0.1
    // instead and perturb the log estimate at ~1e-9.)

    aicno = DNOTST;
    estimate_once(ctx, trnsrs.data(), y, fac.data(), ctx.adj.adjmod, nobspf, aicno);
    if (ctx.error.lfatal) {
        ar.lautox = lax2; ar.lautom = lam2; ar.lautod = lad2;
        return;
    }

    // Restore the initial ARMA coefficients so the log fit restarts identically
    // (trnaic.f:175). Free lags carry the not-set sentinel, which strtvl reseeds.
    if (m.nopr > 0) {
        int endlag = m.opr(m.nopr) - 1;
        for (int ilag = 1; ilag <= endlag; ++ilag)
            if (!m.arimaf(ilag)) d.arimap(ilag) = m.ap1(ilag);
    }

    // ---- 2. log transform (Fcntyp=1, Lam=0) -------------------------------
    ar.fcntyp = 1;
    ar.lam = 0.0;
    ctx.adj.adjmod = 1;
    setdp(1.0, PLEN, ctx.adj.adj.data());

    trnfcn(ctx, trnsrs.data(), nobspf, ar.fcntyp, ar.lam, trnsrs.data());
    if (ctx.error.lfatal) {
        ar.lautox = lax2; ar.lautom = lam2; ar.lautod = lad2;
        return;
    }

    aiclog = DNOTST;
    estimate_once(ctx, trnsrs.data(), y, fac.data(), /*adjmod=*/1, nobspf, aiclog);
    if (ctx.error.lfatal) {
        ar.lautox = lax2; ar.lautom = lam2; ar.lautod = lad2;
        return;
    }

    // ---- 3. choose the transform (trnaic.f:256) ---------------------------
    // Traicd (aicdiff) default: -2 for monthly/quarterly, 0 otherwise
    // (editor.f:165, applied here since the reduced driver skips editor).
    double traicd = ar.traicd;
    if (dpeq(traicd, DNOTST))
        traicd = (m.sp == 4 || m.sp == 12) ? -2.0 : 0.0;

    if (aiclog + traicd < aicno) {
        // Log transformation preferred: keep Fcntyp=1/Lam=0/Adjmod=1/Adj=1.
        ar.fcntyp = 1;
        ar.lam = 0.0;
        ctx.adj.adjmod = 1;
        // trnaic.f:301-302: with a log transform + X-11, the seasonal adjustment
        // is multiplicative -- override the parse-time Muladd (which resolved to
        // additive under the not-yet-decided Fcntyp==0) to Muladd=0/Tmpma=0.
        ctx.x11opt.muladd = 0;
        ctx.x11opt.tmpma = 0;
    } else {
        // No transformation: revert to Fcntyp=4/Lam=1, additive adjustment.
        ar.fcntyp = 4;
        ar.lam = 1.0;
        ctx.adj.adjmod = 2;
        ctx.prior.priadj = 1;
        setdp(0.0, PLEN, ctx.adj.adj.data());
    }

    // Restore the automatic-model flags for the downstream automdl driver.
    ar.lautox = lax2;
    ar.lautom = lam2;
    ar.lautod = lad2;
}

}  // namespace x13
