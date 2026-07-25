// seatopts.cpp -- see seatopts.hpp.
#include "seats/seatopts.hpp"
#include "gen/model.hpp"  // prm::PRGTCN (the Constant/mean regressor type)
#include "gen/notset.hpp"
#include "gen/tbllog.hpp"
#include "numeric/numeric.hpp"

#include <cmath>

namespace x13 {

namespace {
// seattb.i:19/25 -- the first and (NTBL-11)th entries of the seats table
// block that ansub9.f:1050's istrue() scans. Note the deliberate NTBL-11
// upper bound: LSECYC (388) / LSELTT (389) sit ABOVE it, which is why
// `seats{print=(cyc ltt)}` does NOT push L_OUT to 3 (verified against the
// oracle: that spec still writes .cyc/.ltt, while `print=all` does not).
constexpr int LSETRN = 349;
}  // namespace

SeatsOptions seats_resolve_options(const X13Context& ctx) {
    const seatop_cmn& o = ctx.seatop;
    SeatsOptions r;
    if (!dpeq(o.rmod2, prm::DNOTST)) r.rmod = o.rmod2;
    if (!dpeq(o.epsph2, prm::DNOTST)) r.epsphi = o.epsph2;
    if (!dpeq(o.xl2, prm::DNOTST)) r.xl = o.xl2;
    if (!dpeq(o.epsiv2, prm::DNOTST)) r.epsiv = o.epsiv2;
    if (o.maxit2 != prm::NOTSET) r.maxit = o.maxit2;
    if (o.qmax2 != prm::NOTSET) r.qmax = o.qmax2;

    // ansub9.f:1064-1068 -- unconditional, no NOTSET sentinel.
    r.noadmiss = o.lnoadm ? 1 : 0;

    // ansub9.f:1072-1080. Kmean==NOTSET -> derive from the presence of a
    // 'Constant' regressor GROUP in the fitted model (the oracle's
    // strinx(F,Grpttl,Grpptr,1,Ngrptl,'Constant')); this port keys off the
    // regressor TYPE prm::PRGTCN, the same test estbur/run_seats already used
    // before seats{imean=} was wired. An explicit seats{imean=} overrides.
    if (o.kmean == prm::NOTSET) {
        r.imean = 0;
        const auto& M = ctx.model;
        for (int i = 1; i <= M.nb; ++i) {
            if (M.rgvrtp(i) == prm::PRGTCN) {
                r.imean = 1;
                break;
            }
        }
    } else {
        r.imean = (o.kmean == 1) ? 1 : 0;
    }

    // ansub9.f:1090-1094.
    r.statseas = o.lstsea ? 1 : 0;

    // ansub9.f:1118-1122: the l_bias=-2 sentinel becomes 1, an explicit
    // Bias2 wins. PORTED CENSUS QUIRK (analts.f:1647-1653, CB-14): SEATS then
    // silently promotes bias=0 to bias=1 and prints "BIAS SET EQUAL TO 1", so
    // seats{bias=0} is a no-op even though gtseat.f:281-291 accepts it.
    r.bias = (o.bias2 == prm::NOTSET) ? 1 : o.bias2;
    if (r.bias == 0) r.bias = 1;

    // ---- Hodrick-Prescott family. Two blocks, in ansub9.f's own order; the
    // ORDER is load-bearing because the second can undo the first.
    //
    // Block 1 (ansub9.f:1080-1090): Lhp (the `hpcycle=` switch, defaulted TRUE
    // at gtinpt.f:536) decides. With `hplan=` also given, hpcycle is forced to
    // an explicit 1 (or Hptrgt) instead of the -1 "auto" sentinel.
    r.hptarget = (o.hptrgt != prm::NOTSET) ? o.hptrgt : 0;
    if (o.lhp) {
        if (dpeq(o.hplan2, prm::DNOTST)) {
            r.hpcycle = -1;
        } else {
            r.hpcycle = 1;
            if (o.hptrgt != prm::NOTSET) r.hpcycle = o.hptrgt;
        }
    } else {
        r.hpcycle = 0;
    }

    // Block 2 (ansub9.f:1109-1117): `hplan=` sets L_hplan AND, if hpcycle came
    // out of block 1 as 0, turns the filter back ON.
    //
    // CENSUS BUG (CB-15, tools/census_bugs.md): that re-enable does not test
    // WHY hpcycle is 0, so `seats{hpcycle=no hplan=40}` runs the HP filter
    // anyway -- an explicit "no" is silently overridden by an unrelated tuning
    // argument. Verified against the oracle: that spec emits .cyc/.ltt and
    // echoes `hpcycle= 1` in the .sum INPUT block. Ported verbatim.
    if (!dpeq(o.hplan2, prm::DNOTST)) {
        r.hplan = o.hplan2;
        if (r.hpcycle == 0) {
            r.hpcycle = 1;
            if (o.hptrgt != prm::NOTSET) r.hpcycle = o.hptrgt;
        }
    }

    // Lhprmls has no l_ mirror -- sigex.f reads /setopt/ directly
    // (sigex.f:2421/2448/2480/2506).
    r.hprmls = o.lhprmls;

    // l_out: ansub9.f:1615 (default 0) -> :1049 (`out=` override) -> :1050
    // (any seats PRINT table in [LSETRN, NTBL-11] forces 3). The :1050
    // override must come LAST -- verified against the oracle, `seats{print=all
    // out=0}` still suppresses HPOUTPUT.
    //
    // GAP (not this file's): gt_seats token-consumes `print=` without
    // populating ctx.tbllog.prttab -- the seats table dictionary (DSEDIC,
    // seatpr.f) is unported print surface -- so the istrue() term below is
    // presently always false and `out` resolves to 0 for every spec that does
    // not say `out=` explicitly. Do NOT read `out == 0` as "the oracle would
    // have written HPOUTPUT's cyc/ltt here". See tools/seats_hp_scouting.md.
    r.out = 0;
    if (o.out2 != prm::NOTSET) r.out = o.out2;
    for (int i = LSETRN; i <= prm::NTBL - 11; ++i) {
        if (ctx.tbllog.prttab(i)) { r.out = 3; break; }
    }

    return r;
}

// ansub1.f:3634-3747, condition tree only (the oracle's `nochmodel` is
// l_Nochmodel==0 always -- ansub9.f:1576, never overridden by the bridge --
// so the outer guard is unconditionally true here). Returns `cambiado`.
//
// SIGN CONVENTION (the trap here): NMLSTS hands analts.f nmlmdl.f's outputs,
// which are exactly what seats_decode_model puts in SeatsModelOrders -- but
// analts.f:1125-1136 then NEGATES them, `Phi(i)=-Phi(i)` / `Th(i)=-Th(i)` /
// `Bth(1)=-Bth(1)` / `Bphi(1)=-Bphi(1)`, BEFORE the CHANGEMODEL call at
// analts.f:1715. So every coefficient test below runs on -mo.*. (Note the
// oracle only flips element 1 of Bth/Bphi, not the whole array -- faithful,
// and irrelevant here since CHANGEMODEL only ever reads element 1.)
int seats_changemodel_cambiado(const SeatsModelOrders& mo, double rmod,
                               int statseas, int posbphi) {
    const int p = mo.p, q = mo.q, d = mo.d;
    const int bp = mo.bp, bq = mo.bq, bd = mo.bd;
    const double phi[3] = {-mo.phi[0], -mo.phi[1], -mo.phi[2]};
    const double bphi[3] = {-mo.bphi[0], -mo.bphi[1], -mo.bphi[2]};
    const double th[3] = {-mo.th[0], -mo.th[1], -mo.th[2]};
    const double bth[3] = {-mo.bth[0], -mo.bth[1], -mo.bth[2]};
    int cambiado = 0;

    if (bd == 0) {
        if (bp == 1 && bq == 1 && std::fabs(bphi[0]) < std::fabs(bth[0])) {
            if (bphi[0] > 0.0 && bth[0] < 0.0 && statseas == 1) {
                cambiado = 1;  // bd=1, bp=0
            } else if (bphi[0] > 0.0 && bth[0] > 0.0) {
                cambiado = 1;  // bq=0
            }
        } else if (bp == 1 && bq == 0 && statseas == 1) {
            if (bphi[0] > rmod - 0.2) {
                cambiado = 1;  // bp=0, bq=1, bd=1
            } else if (bphi[0] <= 0.0 && bphi[0] >= -rmod + 0.2) {
                cambiado = 1;  // bp=0
            }
        }
    } else {
        if (posbphi == 1 && bp == 1 && bphi[0] <= 0.0) {
            cambiado = 1;  // bp=0, bq=1 (dead: posbphi is always 0 here)
        }
    }

    if (d == 0) {
        if (p == 1 && q == 1) {
            if (std::fabs(phi[0]) < std::fabs(th[0])) {
                if (phi[0] > 0.0 && th[0] < 0.0 && statseas == 1)
                    cambiado = 1;  // d=1, p=0
            }
        } else if (p == 1 && statseas == 1 && phi[0] > rmod) {
            cambiado = 1;  // p=0, q=1, d=1
        } else if (p == 0 && q == 1 && bp == 0 && bd == 0 && bq == 0) {
            cambiado = 4;  // q=0
        }
    }

    return cambiado;
}

const char* seats_model_unported_reason(const SeatsOptions& opts,
                                        const SeatsModelOrders& mo,
                                        bool is_log) {
    // CHANGEMODEL rewrote the ARIMA orders -> the oracle sets init=0 and
    // RE-ESTIMATES the changed model inside SEATS (analts.f:1715/2260 ->
    // `goto 10`), printing "MODEL CHANGED TO :". This port decodes the fitted
    // regARIMA model and never re-estimates, so decomposing anyway would
    // silently use the wrong model.
    if (seats_changemodel_cambiado(mo, opts.rmod, opts.statseas,
                                   /*posbphi=*/0) != 0) {
        return "SEATS CHANGEMODEL model rewrite (ansub1.f:3634-3747; "
               "seats{statseas=yes} and/or a bd==0/d==0 model shape) -- the "
               "oracle re-estimates the rewritten ARIMA model inside SEATS "
               "(init=0), which this port does not implement";
    }
    // BIASCORR (ansub4.f:1244-1581): sigsub.f:1535's bias==-1 branch. Needs
    // the forecast-span component arrays (forbias/forsbias/fortbias over
    // Nz+lfor) that this historical-span-only ESTBUR never forms. Inert when
    // the decomposition is additive: sigsub.f:1526's `lamd.eq.1` branch is
    // taken first and never consults bias at all.
    if (opts.bias == -1 && is_log) {
        return "seats{bias=-1} (BIASCORR, ansub4.f:1244-1581) -- the "
               "forecast-span bias correction is not ported";
    }
    return nullptr;
}

const char* seats_decomp_unported_reason(const SeatsOptions& opts, double qt1,
                                         int ncycth, int ncyc, double varwnc) {
    // spectrum.f:389 `qmin = qt1`, :446/:492/:520 -- qmin<0 means the
    // canonical decomposition is INADMISSIBLE (irregular spectrum negative).
    // noadmiss=0: SPECTRUM writes "DECOMPOSITION INVALID..." and `return 1`,
    // i.e. SEATS produces NO s-tables at all. noadmiss=1: noadmiss:=3 and
    // sigex.f:824 hands the model to APPROXIMATE, which rewrites the orders
    // and re-estimates. Neither is ported.
    if (qt1 < 0.0) {
        return opts.noadmiss != 0
                   ? "SEATS inadmissible decomposition with "
                     "seats{noadmiss=yes} -- APPROXIMATE (ansub5.f:2011) "
                     "rewrites the ARIMA model and SEATS re-estimates it; "
                     "not ported"
                   : "SEATS inadmissible decomposition (SPECTRU qt1 < 0, "
                     "irregular spectrum negative) -- the oracle aborts SEATS "
                     "and writes no s-tables; try another model or "
                     "seats{noadmiss=yes}";
    }
    // spectrum.f:1709-1737 (inside DecompSpectrum): the same verdict reached
    // through a negative CYCLE white-noise variance.
    if ((ncycth != 0 || ncyc != 1) && varwnc < 0.0) {
        return opts.noadmiss != 0
                   ? "SEATS inadmissible decomposition (cycle varwnc < 0) "
                     "with seats{noadmiss=yes} -- APPROXIMATE (ansub5.f:2011) "
                     "rewrites the ARIMA model and SEATS re-estimates it; "
                     "not ported"
                   : "SEATS inadmissible decomposition (cycle varwnc < 0) -- "
                     "the oracle aborts SEATS and writes no s-tables; try "
                     "another model or seats{noadmiss=yes}";
    }
    return nullptr;
}

int seats_resolve_hpcycle(const SeatsOptions& o, int mq, int nz) {
    if (o.hpcycle != -1) return o.hpcycle;
    // sigex.f:2371-2377 -- the minimum span the auto mode requires.
    const bool long_enough = (mq == 12 && nz >= 120) || (mq == 6 && nz >= 60) ||
                             (mq == 4 && nz >= 48) || (mq == 3 && nz >= 45) ||
                             (mq == 2 && nz >= 30) || (mq == 1 && nz >= 15);
    if (!long_enough) return 0;                 // sigex.f:2385
    if (o.hptarget != 0) return o.hptarget;     // sigex.f:2379-2380
    return 1;                                   // sigex.f:2382
}

}  // namespace x13
