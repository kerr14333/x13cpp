// dump_diag.hpp -- the QS and NP savelog blocks, shared by the X-11 and SEATS
// CLI harnesses.
//
// x11ari.f reaches genqs (:277-281), spcdrv (:282-287) and gennpsa (:322-326)
// AFTER its Lseats / Lx11 branch rejoins, so all three blocks belong to every
// adjustment path, not to X-11. They lived in x13run_x11.cpp until the SEATS arms landed; moved here
// rather than duplicated so the two harnesses cannot drift in format.
//
// Like every other savelog block in this port these go through fwrite_fmt with
// the Fortran routine's own FORMAT, not printf -- field overflow and the
// missing space after `qs*`'s colon are part of the contract.
//
// `keypfx` is prepended to every emitted line and is for the COMPOSITE harness,
// which runs a whole metafile in one process: its components print as
// `<base>:<key>` so one invocation covers every spec, while the total prints
// unprefixed and reads like any other single-series run. It is not part of the
// oracle's output -- the oracle writes one .udg per spec.
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
inline void dump_qs(const x13::X13Context& ctx, const char* keypfx = "",
                     std::string* sink = nullptr) {
    using x13::fwrite_fmt;
    const auto& q = ctx.qs;
    if (!q.ran) return;
    auto line = [&](const std::string& t) {
        if (sink) { sink->append(keypfx); sink->append(t); sink->push_back('\n'); }
        else std::printf("%s%s\n", keypfx, t.c_str());
    };
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
inline void dump_np(const x13::X13Context& ctx, const char* keypfx = "",
                     std::string* sink = nullptr) {
    using x13::fwrite_fmt;
    const auto& np = ctx.np;
    if (!np.ran) return;
    auto line = [&](const std::string& t) {
        if (sink) { sink->append(keypfx); sink->append(t); sink->push_back('\n'); }
        else std::printf("%s%s\n", keypfx, t.c_str());
    };
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



// svfreq.f / svpeak.f / smpeak.f / mxpeak.f / savpk.f -- the spectrum peak
// canaries:
//   svfreq 1000: (a,': ',i5)                  nspecfreq / ntdfreq / nsfreq
//   svfreq 1010: (a,i1,'.',a,': ',f12.8)      <s|t>N.freq
//   svfreq 1020: (a,i1,'.',a,': ',i5)         <s|t>N.index
//   svpeak 1010: (a,'.',a,': ',e20.10)        <prefix>.median / .range
//   smpeak 1010: (a,'.',a,': ',a)             a `nopeak` row, and <prefix>.<s|t>.dom
//   smpeak 1020: (a,'.',a,': ',f6.1,' ',a)    a star-height row + its '+' marker
//   mxpeak 1010: (a,'.dom: ',a)               <prefix>.dom
//   savpk  1000: (a,a)                        peaks.seas / peaks.td
inline void dump_spec_peaks(const x13::X13Context& ctx, const char* keypfx = "",
                     std::string* sink = nullptr) {
    using x13::fwrite_fmt;
    const auto& sp = ctx.spcout;
    auto line = [&](const std::string& t) {
        if (sink) { sink->append(keypfx); sink->append(t); sink->push_back('\n'); }
        else std::printf("%s%s\n", keypfx, t.c_str());
    };
    if (!sp.ran || !sp.grid.ok) return;

    // svfreq.f: with `saveallfreq` off (the default) the index columns are
    // LITERAL constants, not grid positions -- which is why they read 10/20/30/
    // 40/50 and 42/52 while the enhanced grid has those peaks at 11/21/31/41/54
    // and 44/57. Only the Svallf branch prints the real indices.
    static const int TIDX[2] = {42, 52};
    static const int SIDX[5] = {10, 20, 30, 40, 50};
    const auto& g = sp.grid;
    line(fwrite_fmt("(a,': ',i5)", "nspecfreq", ctx.rho.svallf ? g.nfreq : 61));
    line(fwrite_fmt("(a,': ',i5)", "ntdfreq", static_cast<int>(g.tfreq.size())));
    for (std::size_t i = 0; i < g.tfreq.size(); ++i) {
        const int n = static_cast<int>(i) + 1;
        line(fwrite_fmt("(a,i1,'.',a,': ',f12.8)", "t", n, "freq", g.tfreq[i]));
        if (ctx.rho.svallf) {
            line(fwrite_fmt("(a,i1,'.',a,': ',i5)", "t", n, "index", g.tpeak[i] - 1));
            line(fwrite_fmt("(a,i1,'.',a,': ',i5)", "t", n, "index.lower", g.tlow[i] - 1));
            line(fwrite_fmt("(a,i1,'.',a,': ',i5)", "t", n, "index.upper", g.tup[i] - 1));
        } else if (i < 2) {
            line(fwrite_fmt("(a,i1,'.',a,': ',i5)", "t", n, "index", TIDX[i]));
        }
    }
    line(fwrite_fmt("(a,': ',i5)", "nsfreq", static_cast<int>(g.sfreq.size())));
    for (std::size_t i = 0; i < g.sfreq.size(); ++i) {
        const int n = static_cast<int>(i) + 1;
        line(fwrite_fmt("(a,i1,'.',a,': ',f12.8)", "s", n, "freq", g.sfreq[i]));
        if (ctx.rho.svallf) {
            line(fwrite_fmt("(a,i1,'.',a,': ',i5)", "s", n, "index", g.speak[i] - 1));
            line(fwrite_fmt("(a,i1,'.',a,': ',i5)", "s", n, "index.lower", g.slow[i] - 1));
            if (i + 1 < g.sfreq.size())
                line(fwrite_fmt("(a,i1,'.',a,': ',i5)", "s", n, "index.upper", g.sup[i] - 1));
        } else if (i < 5) {
            line(fwrite_fmt("(a,i1,'.',a,': ',i5)", "s", n, "index", SIDX[i]));
        }
    }

    // spcdrv.f emits svpeak for rsd (from the regARIMA phase), then sa, irr,
    // ori. run_spectrum computes them in a different order, so key on prefix.
    static const char* ORDER[4] = {"spcrsd", "spcsa", "spcirr", "spcori"};
    for (const char* want : ORDER) {
        for (const auto& p : sp.peaks) {
            if (p.prefix != want) continue;
            line(fwrite_fmt("(a,'.',a,': ',e20.10)", p.prefix, "median", p.median));
            line(fwrite_fmt("(a,'.',a,': ',e20.10)", p.prefix, "range", p.range));
            auto rows = [&](const std::vector<x13::SpecPeakRow>& v,
                            const std::string& dom, const char* domkey) {
                if (v.empty()) return;
                for (const auto& r : v) {
                    if (r.nopeak)
                        line(fwrite_fmt("(a,'.',a,': ',a)", p.prefix, r.label,
                                        "nopeak"));
                    else
                        line(fwrite_fmt("(a,'.',a,': ',f6.1,' ',a)", p.prefix,
                                        r.label, r.stars,
                                        r.above_median ? "+" : " "));
                }
                line(fwrite_fmt("(a,'.',a,': ',a)", p.prefix, domkey, dom));
            };
            if (p.have_td) rows(p.td, p.tdom, "t.dom");
            rows(p.seas, p.sdom, "s.dom");
            line(fwrite_fmt("(a,'.dom: ',a)", p.prefix, p.dom));
        }
    }

    if (!sp.peaks_seas.empty()) {
        line(fwrite_fmt("(a,a)", "peaks.seas: ", sp.peaks_seas));
        line(fwrite_fmt("(a,a)", "peaks.td: ", sp.peaks_td));
        // savpk.f:88-115 -- on a composite total the ACCUMULATED lists are
        // additionally split into their direct and indirect halves. The unsplit
        // pair above is written either way (savpk.f:86-87).
        if (sp.ran_ind) {
            line(fwrite_fmt("(a,a)", "peaks.seas.dir: ", sp.peaks_seas_dir));
            line(fwrite_fmt("(a,a)", "peaks.seas.ind: ", sp.peaks_seas_ind));
            line(fwrite_fmt("(a,a)", "peaks.td.dir: ", sp.peaks_td_dir));
            line(fwrite_fmt("(a,a)", "peaks.td.ind: ", sp.peaks_td_ind));
        }
    }

    // The Tukey half. Two emit points in the oracle, in this order:
    //   spcdrv.f:996 / spcrsd.f  1080 FORMAT(a,'.tukey.m: ',i5)  -- at compute
    //     time, so all four `.m` rows come out before any probability row.
    //   svtukp.f                 1010 FORMAT(a,a,i1,a,f9.4)      -- the s1..s6
    //                            1020 FORMAT(a,a,f9.4)           -- the td row
    // then savtpk.f:81-88's four label lists.
    if (!sp.tukey.empty()) {
        for (const auto& e : sp.tukey)
            line(fwrite_fmt("(a,'.tukey.m: ',i5)", "spc" + e.label, e.pk.m));
        for (const auto& e : sp.tukey) {
            for (int k = 1; k <= 6; ++k)
                line(fwrite_fmt("(a,a,i1,a,f9.4)", "spc" + e.label, ".tukey.s",
                                k, ": ", e.pk.ps[k - 1]));
            line(fwrite_fmt("(a,a,f9.4)", "spc" + e.label, ".tukey.td: ",
                            e.pk.ptd));
        }
        const auto& L = sp.tukey_labels;
        line(fwrite_fmt("(a,a)", "peaks.tukey.seas: ", L.seas));
        line(fwrite_fmt("(a,a)", "peaks.tukey.td: ", L.td));
    }
    // --- the INDIRECT (Iagr==4) block, composite totals only ---------------
    // x11ari.f:352-357: the second spcdrv's own `spcindsa`/`spcindirr` peak
    // rows and Tukey probabilities, then svtukp again over that pass's Itukey
    // entries alone. Note the ORDER the oracle writes these in -- the indirect
    // `peaks.tukey.{seas,td}.ind` land between the direct `peaks.tukey.td` and
    // the direct `peaks.tukey.p90.*`, because svtukp emits its four lists per
    // call and savtpk merges them afterwards.
    if (sp.ran_ind) {
        const auto& LI = sp.tukey_labels_ind;
        line(fwrite_fmt("(a,a)", "peaks.tukey.seas.ind: ", LI.seas));
        line(fwrite_fmt("(a,a)", "peaks.tukey.td.ind: ", LI.td));
    }
    if (!sp.tukey.empty()) {
        const auto& L = sp.tukey_labels;
        line(fwrite_fmt("(a,a)", "peaks.tukey.p90.seas: ", L.p90_seas));
        line(fwrite_fmt("(a,a)", "peaks.tukey.p90.td: ", L.p90_td));
    }
    if (sp.ran_ind) {
        const auto& LI = sp.tukey_labels_ind;
        line(fwrite_fmt("(a,a)", "peaks.tukey.p90.seas.ind: ", LI.p90_seas));
        line(fwrite_fmt("(a,a)", "peaks.tukey.p90.td.ind: ", LI.p90_td));
    }
}

// The INDIRECT (Iagr==4) spectrum + NP blocks: the second spcdrv/gennpsa pass
// x11ari.f:344-370 runs after agr3 replaced the D-table buffers with the
// aggregated adjustment. Emitted separately from dump_spec_peaks/dump_np
// because only a composite TOTAL has them, and because the `peaks.*` label
// lists they contribute to are interleaved with the direct ones above.
//
// There is deliberately NO indirect QS block: see CB-31 -- x11ari.f:346 passes
// genqs a SAVELOG index (LSLIQS=69) where genqs.f:439 uses it as a table-log
// subscript, so the oracle emits no `qsind*` key at all.
inline void dump_diag_indirect(const x13::X13Context& ctx,
                               const char* keypfx = "",
                               std::string* sink = nullptr) {
    using x13::fwrite_fmt;
    auto line = [&](const std::string& t) {
        if (sink) { sink->append(keypfx); sink->append(t); sink->push_back('\n'); }
        else std::printf("%s%s\n", keypfx, t.c_str());
    };
    const auto& sp = ctx.spcout;
    if (sp.ran_ind) {
        for (const auto& p : sp.peaks_ind) {
            line(fwrite_fmt("(a,'.',a,': ',e20.10)", p.prefix, "median", p.median));
            line(fwrite_fmt("(a,'.',a,': ',e20.10)", p.prefix, "range", p.range));
            auto rows = [&](const std::vector<x13::SpecPeakRow>& v,
                            const std::string& dom, const char* domkey) {
                if (v.empty()) return;
                for (const auto& r : v) {
                    if (r.nopeak)
                        line(fwrite_fmt("(a,'.',a,': ',a)", p.prefix, r.label,
                                        "nopeak"));
                    else
                        line(fwrite_fmt("(a,'.',a,': ',f6.1,' ',a)", p.prefix,
                                        r.label, r.stars,
                                        r.above_median ? "+" : " "));
                }
                line(fwrite_fmt("(a,'.',a,': ',a)", p.prefix, domkey, dom));
            };
            if (p.have_td) rows(p.td, p.tdom, "t.dom");
            rows(p.seas, p.sdom, "s.dom");
            line(fwrite_fmt("(a,'.dom: ',a)", p.prefix, p.dom));
        }
        for (const auto& e : sp.tukey_ind)
            line(fwrite_fmt("(a,'.tukey.m: ',i5)", "spc" + e.label, e.pk.m));
        for (const auto& e : sp.tukey_ind) {
            for (int k = 1; k <= 6; ++k)
                line(fwrite_fmt("(a,a,i1,a,f9.4)", "spc" + e.label, ".tukey.s",
                                k, ": ", e.pk.ps[k - 1]));
            line(fwrite_fmt("(a,a,f9.4)", "spc" + e.label, ".tukey.td: ",
                            e.pk.ptd));
        }
    }
    const auto& np = ctx.np_ind;
    if (np.ran) {
        auto verdict = [&](const char* key, int v) {
            if (v == x13::prm::NOTSET) return;
            line(fwrite_fmt("(a,': ',a)", key, v ? "yes" : "no"));
        };
        // gennpsa.f:139-176 under Iagr==4. NOTE there is no indirect `nplog`:
        // that line is written only on the `Iagr.lt.4` branch, like `qslog`.
        if (np.lnp()) {
            verdict("npindsadj", np.npsadj);
            verdict("npindsadjevadj", np.npsadj2);
        }
        if (np.lnps()) {
            verdict("npsindsadj", np.npsadjs);
            verdict("npsindsadjevadj", np.npsadjs2);
        }
    }
}

#endif  // X13_TOOLS_DUMP_DIAG_HPP
