// dump_diag.hpp -- the QS and NP savelog blocks, shared by the X-11 and SEATS
// CLI harnesses.
//
// x11ari.f reaches genqs (:277-281) and gennpsa (:322-326) AFTER its Lseats /
// Lx11 branch rejoins, so both blocks belong to every adjustment path, not to
// X-11. They lived in x13run_x11.cpp until the SEATS arms landed; moved here
// rather than duplicated so the two harnesses cannot drift in format.
//
// Like every other savelog block in this port these go through fwrite_fmt with
// the Fortran routine's own FORMAT, not printf -- field overflow and the
// missing space after `qs*`'s colon are part of the contract.
#ifndef X13_TOOLS_DUMP_DIAG_HPP
#define X13_TOOLS_DUMP_DIAG_HPP

#include <cstdio>
#include <string>

#include "x13/fformat.hpp"
#include "common/x13context.hpp"
#include "numeric/numeric.hpp"  // chisq
#include "gen/notset.hpp"       // prm::DNOTST, prm::NOTSET

// genqs.f:439-517 -- the QS seasonality savelog block. Two formats:
//   1030 FORMAT(a,':',f16.5,1x,f10.5)   -- NOTE no space after the colon
//   1040 FORMAT(a,': ',a)               -- the qslog yes/no line
// A DNOTST statistic suppresses its row entirely, and the whole `qss*` block
// only appears when the diagnostic span starts after the series does.
inline void dump_qs(const x13::X13Context& ctx) {
    using x13::fwrite_fmt;
    const auto& q = ctx.qs;
    if (!q.ran) return;
    auto line = [](const std::string& t) { std::printf("%s\n", t.c_str()); };
    auto stat = [&](const char* key, double v) {
        if (v == x13::prm::DNOTST) return;
        line(fwrite_fmt("(a,':',f16.5,1x,f10.5)", key, v, x13::chisq(v, 2)));
    };
    if (q.lqs()) {
        line(fwrite_fmt("(a,': ',a)", "qslog", q.lplog ? "yes" : "no"));
        stat("qsori", q.qsori);
        stat("qsorievadj", q.qsori2);
        stat("qsrsd", q.qsrsd);
        stat("qssadj", q.qssadj);
        stat("qssadjevadj", q.qssadj2);
        stat("qsirr", q.qsirr);
        stat("qsirrevadj", q.qsirr2);
    }
    if (q.lqss()) {
        stat("qssori", q.qsoris);
        stat("qssorievadj", q.qsoris2);
        stat("qssrsd", q.qsrsd2);
        stat("qsssadj", q.qssadjs);
        stat("qsssadjevadj", q.qssadjs2);
        stat("qssirr", q.qsirrs);
        stat("qssirrevadj", q.qsirrs2);
    }
}

// gennpsa.f:133-178 -- the NP residual-seasonality savelog block. One format
// (`1040 FORMAT(a,': ',a)`) and four yes/no verdicts plus `nplog`, all through
// getstr(YSNDIC) on `NPsadj+1`. Absent entirely from a run that produces no
// seasonal adjustment, since only the SA series is tested.
inline void dump_np(const x13::X13Context& ctx) {
    using x13::fwrite_fmt;
    const auto& np = ctx.np;
    if (!np.ran) return;
    auto line = [](const std::string& t) { std::printf("%s\n", t.c_str()); };
    auto verdict = [&](const char* key, int v) {
        if (v == x13::prm::NOTSET) return;
        line(fwrite_fmt("(a,': ',a)", key, v ? "yes" : "no"));
    };
    if (np.lnp()) {
        line(fwrite_fmt("(a,': ',a)", "nplog", np.lplog ? "yes" : "no"));
        verdict("npsadj", np.npsadj);
        verdict("npsadjevadj", np.npsadj2);
    }
    if (np.lnps()) {
        verdict("npssadj", np.npsadjs);
        verdict("npssadjevadj", np.npsadjs2);
    }
}

#endif  // X13_TOOLS_DUMP_DIAG_HPP
