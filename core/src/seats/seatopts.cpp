// seatopts.cpp -- see seatopts.hpp.
#include "seats/seatopts.hpp"
#include "gen/notset.hpp"
#include "gen/tbllog.hpp"
#include "numeric/numeric.hpp"

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
    // CENSUS BUG (CB-14, tools/census_bugs.md): that re-enable does not test
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
