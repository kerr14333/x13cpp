// rev_outlier.cpp -- rmotrv.f / chkorv.f. See rev_outlier.hpp for why these are
// on the DEFAULT history{} path rather than behind `outlier=`.
#include "driver/rev_outlier.hpp"

#include "common/x13context.hpp"
#include "regarima/outlier.hpp"      // rdotlr
#include "specparse/specparse.hpp"   // getstr, dlrgef, adrgef
#include "gen/model.hpp"             // prm:: PRGT*, AO/LS/TC/RP/TLS/SO/QI/QD

#include <algorithm>
#include <cctype>
#include <string>

namespace x13 {

namespace {

// The outlier regressor types rmotrv considers (rmotrv.f:50-56). Note it takes
// the AUTOMATICALLY identified ones (PRGTAA/PRGTAL/PRGTAT) as well as the
// user-specified ones -- an `outlier{}` spec's finds are ordinary columns by
// the time the history runs, and are held back the same way.
bool is_outlier_col(int rtype) {
    using namespace prm;
    return rtype == PRGTAO || rtype == PRGTLS || rtype == PRGTRP ||
           rtype == PRGTAA || rtype == PRGTAL || rtype == PRGTTC ||
           rtype == PRGTQD || rtype == PRGTQI || rtype == PRGTAT ||
           rtype == PRGTSO || rtype == PRGTTL;
}

// chkorv.f's `otypvc`: the Rgvrtp variable type an outlier of each rdotlr type
// is re-added as. Indexed 1..9 by otltyp (AO LS TC RP MV TLS SO QI QD). Note
// index 5 (MV) maps to PRGTAO, verbatim.
int otypvc(int otltyp) {
    using namespace prm;
    switch (otltyp) {
    case 1: return PRGTAO;   // AO
    case 2: return PRGTLS;   // LS
    case 3: return PRGTTC;   // TC
    case 4: return PRGTRP;   // RP
    case 5: return PRGTAO;   // MV  (as written)
    case 6: return PRGTTL;   // TLS
    case 7: return PRGTSO;   // SO
    case 8: return PRGTQI;   // QI
    case 9: return PRGTQD;   // QD
    default: return 0;
    }
}

// chkorv.f's `opref`: which outlier type WINS when several land on the very last
// observation of a span (only one of them is estimable there). DATA opref/1,4,
// 2,0,0,0,3/ -- indexed 1..7 by otltyp, HIGHER wins (the loser is deleted).
int opref(int otltyp) {
    static const int pref[8] = {0, 1, 4, 2, 0, 0, 0, 3};
    return (otltyp >= 1 && otltyp <= 7) ? pref[otltyp] : 0;
}

// The design dictionary half of ssprep.f, re-taken after a structural change so
// the next span's restor keeps it (rmotrv's `IF(Notrtl.gt.0)CALL ssprep` at
// revdrv.f:305 and chkorv.f:189-202's inline update, which is the same list).
void snapshot_design(X13Context& ctx) {
    ssprep_cmn& p = ctx.ssprep;
    const model_cmn& m = ctx.model;
    p.ngr2 = m.ngrp;
    p.ngrt2 = m.ngrptl;
    p.ncxy2 = m.ncxy;
    p.nbb = m.nb;
    p.nct2 = m.ncoltl;
    p.cttl = m.colttl.raw();
    p.gttl = m.grpttl.raw();
    cpyint(m.colptr.data(), prm::PB + 1, 1, p.clptr.data());
    cpyint(m.grp.data(), prm::PGRP + 1, 1, p.g2.data());
    cpyint(m.grpptr.data(), prm::PGRP + 1, 1, p.gptr.data());
    copy(ctx.mdldat.b.data(), prm::PB, 1, p.bb.data());
    cpyint(m.rgvrtp.data(), prm::PB, 1, p.rgv2.data());
}

// strinx(chksub=T, ...) over the store: case-insensitive PREFIX match on the
// first len(str) characters of each entry, first hit wins, 0 when none.
int store_index(const RevOtlStore& st, const std::string& str) {
    for (int i = 0; i < st.size(); ++i) {
        const std::string& e = st.ttl[static_cast<std::size_t>(i)];
        if (e.size() < str.size()) continue;
        bool eq = true;
        for (std::size_t k = 0; k < str.size() && eq; ++k)
            eq = std::tolower(static_cast<unsigned char>(e[k])) ==
                 std::tolower(static_cast<unsigned char>(str[k]));
        if (eq) return i + 1;
    }
    return 0;
}

void store_erase(RevOtlStore& st, int icol1) {          // delstr + the Botr shift
    const std::size_t i = static_cast<std::size_t>(icol1 - 1);
    st.ttl.erase(st.ttl.begin() + static_cast<long>(i));
    st.b.erase(st.b.begin() + static_cast<long>(i));
    st.fix.erase(st.fix.begin() + static_cast<long>(i));
}

}  // namespace

void rmatot(X13Context& ctx, int otlrev, int nrxy) {
    using namespace prm;
    model_cmn& m = ctx.model;
    int nauto = 0;
    // rmatot.f:41-90, backwards so a delete cannot disturb the columns still to
    // check. Only the AUTOMATICALLY identified types are taken -- a user's own
    // `regression{variables=(ao1957.jan)}` is not an "outlier identified in a
    // previous run" and stays (rmotrv above is what holds those back).
    for (int i = m.nb; i >= 1; --i) {
        const int rtype = m.rgvrtp(i);
        if (!(rtype == PRGTAA || rtype == PRGTAL || rtype == PRGTAT)) continue;
        dlrgef(ctx, i, nrxy, 1);
        if (ctx.error.lfatal) return;
        nauto += 1;
    }
    if (nauto == 0) return;
    // rmatot.f:95-127's Otlrev>=2 arm -- re-enter the ones dated before
    // `Begrev-Otlwin` as ORDINARY outliers (type rtype-3, or rtype-1 for a TC)
    // so the spans that cannot re-find them still carry them -- is deliberately
    // NOT here: it only means anything when each span re-runs the automatic
    // identification, which this port does not do. run_history fatals on
    // otlrev>=2 before reaching this. See the note there.
    (void)otlrev;
    // revdrv.f:305's ssprep is for rmotrv's store; rmatot updates the snapshot
    // itself (rmatot.f:107-126), and does so on the DELETE path too -- otherwise
    // the first span's restor would put every automatic outlier straight back.
    snapshot_design(ctx);
    copylg(m.regfx.data(), prm::PB, 1, ctx.ssprep.regfx2.data());
}

void rmotrv(X13Context& ctx, const int* begxy, int begrev, int nrxy,
            RevOtlStore& st, bool lotlrv) {
    using namespace prm;
    model_cmn& m = ctx.model;
    const int sp = m.sp;
    // Backwards, so deleting a column cannot disturb the ones still to check.
    for (int icol = m.nb; icol >= 1; --icol) {
        if (!is_outlier_col(m.rgvrtp(icol))) continue;
        std::string str;
        int nstr = 0;
        getstr(ctx, m.colttl.data(), m.colptr.data(), m.ncoltl, icol, str, nstr);
        if (ctx.error.lfatal) return;
        int otltyp = 0, begotl = 0, endotl = 0;
        bool locok = true;
        rdotlr(ctx, str, begxy, sp, otltyp, begotl, endotl, locok);
        if (!locok) ctx.error.lfatal = true;
        if (ctx.error.lfatal) return;
        // rmotrv.f:64-66 -- a ramp / temporary level shift counts when EITHER
        // endpoint falls after the first revision date; everything else on its
        // (single) date alone.
        const bool span_type = (otltyp == RP || otltyp == TLS);
        const bool after = span_type ? (begotl > begrev || endotl > begrev)
                                     : (begotl > begrev);
        if (!after) continue;
        if (lotlrv) {                              // save it for chkorv
            st.ttl.push_back(str);
            st.b.push_back(ctx.mdldat.b(icol));
            st.fix.push_back(m.regfx(icol) ? 1 : 0);
        }
        dlrgef(ctx, icol, nrxy, 1);
        if (ctx.error.lfatal) return;
    }
    // revdrv.f:305 -- IF(Notrtl.gt.0) CALL ssprep(Lmodel,F,F). Faithfully keyed
    // on the STORE being non-empty, not on anything having been deleted: with
    // lotlrv false the columns are dropped and the snapshot is NOT re-taken, so
    // the first span's restor puts them straight back. (Which is consistent --
    // `outlier=remove` reaches here only after rmatot has already deleted the
    // automatic ones and re-snapshotted.)
    if (!st.empty()) {
        snapshot_design(ctx);
        // revdrv.f:305 calls the FULL ssprep, so the fix flags -- which dlrgef
        // shifted down with the columns -- are re-snapshotted too. chkorv's own
        // inline update (chkorv.f:189-202) pointedly does NOT include them;
        // that asymmetry is the Fortran's, and it is reproduced by keeping this
        // pair here and out of chkorv. (Calling the whole ssprep_snapshot would
        // also re-take ctx.saved.lterm0/nterm0/ksdev0 from the MAIN run's
        // already-resolved values, which the span loop reads back -- the same
        // state-leak trap xrgdrv's span_mode documents.)
        copylg(m.regfx.data(), prm::PB, 1, ctx.ssprep.regfx2.data());
        ctx.ssprep.irfx2 = m.iregfx;
    }
}

void chkorv(X13Context& ctx, const int* begxy, int endrev, RevOtlStore& st,
            bool otlfix, int nrxy, bool lmdl) {
    using namespace prm;
    model_cmn& m = ctx.model;
    const int sp = m.sp;
    int endcol = st.size();
    int icol = 1;                                  // 1-based cursor into the store
    bool update = false;
    int nlast = 0;
    while (icol <= endcol) {
        const std::string str = st.ttl[static_cast<std::size_t>(icol - 1)];
        int otltyp = 0, begotl = 0, endotl = 0;
        bool locok = true;
        rdotlr(ctx, str, begxy, sp, otltyp, begotl, endotl, locok);
        if (ctx.error.lfatal) return;
        // chkorv.f:66-71, transcribed as written. The `.or.` chain in the
        // non-ramp half is a Fortran precedence quirk -- `otltyp.ne.RP .and.
        // otltyp.ne.TLS .or. otltyp.ne.QI .or. otltyp.ne.QD` is true for every
        // type (a value cannot be both QI and QD), so the second branch tests
        // only `begotl.le.Endrev`. Reproduced by construction: the ramp branch
        // is taken when the type is a span type, the plain one otherwise.
        const bool span_type =
            (otltyp == RP || otltyp == TLS || otltyp == QI || otltyp == QD);
        const bool defined = span_type ? (begotl <= endrev && endotl <= endrev)
                                       : (begotl <= endrev);
        if (!defined) { ++icol; continue; }

        const bool fx = st.fix[static_cast<std::size_t>(icol - 1)] != 0 || otlfix;
        adrgef(ctx, st.b[static_cast<std::size_t>(icol - 1)], str, str,
               otypvc(otltyp), fx, false);
        if (ctx.error.lfatal) return;
        update = true;
        if (m.iregfx == 3 && !fx) m.iregfx = 2;
        // chkorv.f:76-84 -- an outlier landing exactly ON the span's last
        // observation stays in the store for now: the singularity pass below
        // may have to take it (or a rival) back out again.
        if (!span_type && begotl == endrev) {
            nlast += 1;
            ++icol;
        } else {
            store_erase(st, icol);
            endcol -= 1;
        }
    }
    // chkorv.f:118-170 -- with more than one outlier on the last observation
    // only one is estimable. Walk the DESIGN backwards keeping the highest
    // `opref`, deleting the loser's column each time; the survivor is then
    // dropped from the store.
    if (nlast > 0) {
        int ltype = 0, ilast = 0, lcol = 0, lchr = 0;
        std::string lstr;
        for (int c = m.nb; c >= 1; --c) {
            const int rtype = m.rgvrtp(c);
            if (!(rtype == PRGTAO || rtype == PRGTAA || rtype == PRGTLS ||
                  rtype == PRGTAL || rtype == PRGTTC || rtype == PRGTAT ||
                  rtype == PRGTSO))
                continue;
            std::string str2;
            int jchr = 0;
            getstr(ctx, m.colttl.data(), m.colptr.data(), m.nb, c, str2, jchr);
            if (ctx.error.lfatal) return;
            int otltyp = 0, begotl = 0, endotl = 0;
            bool locok = true;
            rdotlr(ctx, str2, begxy, sp, otltyp, begotl, endotl, locok);
            if (ctx.error.lfatal) return;
            if (otltyp == RP || begotl != endrev) continue;
            ilast += 1;
            if (ilast == 1) {
                ltype = otltyp; lcol = c; lstr = str2; lchr = jchr;
            } else if (opref(ltype) < opref(otltyp)) {
                dlrgef(ctx, c, nrxy, 1);           // this one loses
                if (ctx.error.lfatal) return;
                if (ilast < nlast) lcol -= 1;
            } else {
                dlrgef(ctx, lcol, nrxy, 1);        // the incumbent loses
                if (ctx.error.lfatal) return;
                ltype = otltyp; lcol = c; lstr = str2; lchr = jchr;
            }
        }
        (void)lchr;
        const int delcol = store_index(st, lstr);
        if (delcol > 0) store_erase(st, delcol);
    }
    // chkorv.f:189-202 -- make the addition survive the next span's restor.
    if (update && lmdl) snapshot_design(ctx);
}

}  // namespace x13
