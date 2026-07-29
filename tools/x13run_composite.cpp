// x13run_composite.cpp -- composite (metafile) harness: run every spec listed in
// a .mta file in order, carrying the aggregation state across the runs, then
// print the LAST run's X-11 tables as `<tag> YYYYMM <value>` lines for the parity
// gate. Component runs' tables are printed too, prefixed with `<base>:`.
//
// Usage: x13run_composite <metafile.mta>
//
// Composite adjustment is the one X-13 feature that does not fit in a single
// spec run: the oracle executes all of a metafile's specs in ONE process so the
// aggregation COMMONs (/mq11/ agr.cmn, /agreg/ agrsrs.cmn) persist across them.
// This driver reproduces exactly that persistence -- each spec still gets a fresh
// X13Context (as every other harness does), but the two aggregation blocks are
// copied out of the finished context and into the next one. See
// tools/composite_scouting.md.
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

#include "common/x13context.hpp"
#include "specparse/specparse.hpp"
#include "dump_diag.hpp"          // dump_qs / dump_np / dump_spec_peaks

namespace {

std::string dirname_of(const std::string& p) {
    std::size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? std::string() : p.substr(0, s);
}
std::string basename_of(const std::string& p) {
    std::size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? p : p.substr(s + 1);
}
std::string trim(const std::string& s) {
    std::size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return std::string();
    std::size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// Emit one table section over [first,last] (1-based positions into arr) into
// `out`. Buffered rather than printed so `OUTCOME:` can lead the output the way
// every other harness does -- the outcome is only known after the last spec.
// `anchor` is the buffer position that corresponds to begspn; it differs from the
// range start whenever the range does not begin at Pos1ob (the change tables start
// one period in, backcast-widened ranges one or more periods before it). Defaults
// to the range start, which is the ordinary case.
void dump(std::string& out, const std::string& prefix, const char* tag,
          const int* begspn, int sp, int pos1ob, int last, const double* arr,
          int anchor = -1) {
    char buf[128];
    if (anchor < 0) anchor = pos1ob;
    for (int i = pos1ob; i <= last; ++i) {
        int idate[2];
        x13::addate(begspn, sp, i - anchor, idate);
        std::snprintf(buf, sizeof buf, "%s %04d%02d %.15E\n", tag, idate[0],
                      idate[1], arr[i - 1]);
        out += prefix;
        out += buf;
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: x13run_composite <metafile.mta>\n");
        return 2;
    }
    const std::string mta = argv[1];
    const std::string dir = dirname_of(mta);

    std::ifstream mf(mta);
    if (!mf) {
        std::fprintf(stderr, "x13run_composite: cannot open %s\n", mta.c_str());
        return 2;
    }
    std::vector<std::string> bases;
    for (std::string line; std::getline(mf, line);) {
        const std::string b = trim(line);
        if (!b.empty() && b[0] != '#') bases.push_back(basename_of(b));
    }
    mf.close();
    if (bases.empty()) {
        std::fprintf(stderr, "x13run_composite: %s lists no specs\n", mta.c_str());
        return 2;
    }
    if (!dir.empty()) chdir(dir.c_str());

    // The carried aggregation state (the Fortran COMMONs that outlive a run).
    x13::agr_cmn carry_agr{};
    x13::agrsrs_cmn carry_agrsrs{};
    // ... and, for history{} on a composite, the INDIRECT revision accumulator:
    // /revdta/ Cncisa/Finisa plus the three /revcmn/ scalars that steer it
    // (Indrev, Indrvs, Nrcomp). agr1 initializes them once for the metafile
    // (composite/agr.cpp) and every component folds itself in (putrev.f:25-30),
    // so they persist exactly like /mq11/ and /agreg/ do.
    auto carry_revsrs = std::make_unique<x13::revsrs_cmn>();
    int carry_indrev = 0, carry_nrcomp = 0;
    int carry_indrvs[2] = {0, 0};
    bool have_carry = false;
    std::string out;   // buffered table lines (printed after OUTCOME:)

    for (std::size_t k = 0; k < bases.size(); ++k) {
        const std::string& base = bases[k];
        const bool last = (k + 1 == bases.size());
        const std::string spec = base + ".spc";
        std::ifstream in(spec);
        if (!in) {
            std::fprintf(stderr, "x13run_composite: cannot open %s\n", spec.c_str());
            return 2;
        }
        std::ostringstream ss;
        ss << in.rdbuf();
        const std::string text = ss.str();
        in.close();

        auto ctxp = std::make_unique<x13::X13Context>();
        x13::X13Context& ctx = *ctxp;
        if (have_carry) {
            ctx.agr = carry_agr;
            ctx.agrsrs = carry_agrsrs;
            ctx.revsrs = *carry_revsrs;
            ctx.rev.indrev = carry_indrev;
            ctx.rev.indrvs(1) = carry_indrvs[0];
            ctx.rev.indrvs(2) = carry_indrvs[1];
            ctx.rev.nrcomp = carry_nrcomp;
        }

        bool ok;
        try {
            ok = x13::run_x11(ctx, text, base);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "x13run_composite: %s: exception: %s\n",
                         base.c_str(), e.what());
            return 3;
        }
        if (!ok) {
            std::printf("OUTCOME: FATAL\n");
            std::fprintf(stderr, "x13run_composite: %s failed\n", base.c_str());
            std::fputs(ctx.channels_.unit(ctx.units.mt2).str().c_str(), stderr);
            return 1;
        }

        carry_agr = ctx.agr;
        carry_agrsrs = ctx.agrsrs;
        *carry_revsrs = ctx.revsrs;
        carry_indrev = ctx.rev.indrev;
        carry_indrvs[0] = ctx.rev.indrvs(1);
        carry_indrvs[1] = ctx.rev.indrvs(2);
        carry_nrcomp = ctx.rev.nrcomp;
        have_carry = true;

        // Tables: the composite total (the last spec) prints unprefixed so the
        // gate reads it like any other single-series run; components print with a
        // `<base>:` prefix so one invocation covers the whole metafile.
        const std::string prefix = last ? std::string() : base + ":";
        const int sp = ctx.model.sp;
        const int* begspn = ctx.mdldat.begspn.data();
        const int pos1ob = ctx.x11ptr.pos1ob;
        const int posfob = ctx.x11ptr.posfob;
        const int sf_frst = ctx.tbllog.savbct ? ctx.x11ptr.pos1bk : pos1ob;
        const int sf_last = !ctx.tbllog.savfct
                                ? posfob
                                : (ctx.extend.nfcst > 0 ? ctx.x11ptr.posffc
                                                        : posfob + sp);
        // The composite total is the run that produced an indirect adjustment.
        // (Iagr no longer says so: agr2's Iagr==4 branch clears it at the end of
        // the run, exactly as the oracle does.)
        if (!ctx.agr_direct_d11.empty()) {
            // The DIRECT adjustment of the aggregate, snapshotted by run_x11
            // before agr3 overwrote the buffers.
            dump(out, prefix, "d10", begspn, sp, sf_frst, sf_last, ctx.agr_direct_d10.data());
            dump(out, prefix, "d11", begspn, sp, pos1ob, posfob, ctx.agr_direct_d11.data());
            dump(out, prefix, "d12", begspn, sp, pos1ob, posfob, ctx.agr_direct_d12.data());
            dump(out, prefix, "d13", begspn, sp, pos1ob, posfob, ctx.agr_direct_d13.data());
            // The composite total's run: agr3 has REPLACED the D-table buffers
            // with the INDIRECT adjustment (Iagr 3 -> 4), so d10-d13 are gone and
            // what is left is isf/isa/itn/iir. agr3.f:365/404/558/570 --
            // isf=Sts (over [frstf,lastf] when appendfcst/bcst, else the observed
            // span), isa=Stci, itn=stc2in, iir=Sti.
            dump(out, prefix, "isf", begspn, sp, sf_frst, sf_last, ctx.x11srs.sts.data());
            dump(out, prefix, "isa", begspn, sp, pos1ob, posfob, ctx.x11srs.stci.data());
            if (!ctx.agr_stc2in.empty())
                dump(out, prefix, "itn", begspn, sp, pos1ob, posfob, ctx.agr_stc2in.data());
            dump(out, prefix, "iir", begspn, sp, pos1ob, posfob, ctx.x11srs.sti.data());
            // The savelog canaries the oracle writes to the .udg: agr3's
            // Henderson length and agr2's direct-vs-indirect roughness
            // percentage changes (agr2.f:174-181, di(5)/(6), (11)/(12),
            // (17)/(18), (23)/(24)).
            char cbuf[160];
            std::snprintf(cbuf, sizeof cbuf, "indtrendma %d\n", ctx.agr_indtrendma);
            out += cbuf;
            if (ctx.agr_cmpstat.size() > 24) {
                const double* di = ctx.agr_cmpstat.data();
                static const char* const kNames[4] = {"r1mse", "r1rmse", "r2mse",
                                                      "r2rmse"};
                static const int kIdx[4] = {5, 11, 17, 23};
                for (int t = 0; t < 4; ++t) {
                    std::snprintf(cbuf, sizeof cbuf, "%s %.15E %.15E\n", kNames[t],
                                  di[kIdx[t]], di[kIdx[t] + 1]);
                    out += cbuf;
                }
                // ... and the whole di(1..24), which is the printed roughness
                // table: direct full/last-3 then indirect full/last-3 then the
                // two percentage changes, four rows of six.
                for (int k = 1; k <= 24; ++k) {
                    std::snprintf(cbuf, sizeof cbuf, "cmpstat %d %.15E\n", k, di[k]);
                    out += cbuf;
                }
            }
            // The INDIRECT D8/D9 SI ratios (agr3.f:288-350) and the indirect
            // Part-E family (the same x11pt4_etables, run over agr3's buffers).
            // Names follow filext.var's indirect block: id8/id9, ie1-ie3, the
            // ie5-ie8 change tables with their ip* percent twins, iee (E11),
            // i18 (E18) and ita (the total adjustment factors, EB).
            if (!ctx.agr_id8.empty()) {
                dump(out, prefix, "id8", begspn, sp, pos1ob, posfob, ctx.agr_id8.data());
                dump(out, prefix, "id9", begspn, sp, pos1ob, posfob, ctx.agr_id9.data());
            }
            if (ctx.x11_etables_set) {
                dump(out, prefix, "ie1", begspn, sp, pos1ob, posfob,
                     ctx.adxser.stome.data());
                dump(out, prefix, "ie2", begspn, sp, pos1ob, posfob,
                     ctx.adxser.stcime.data());
                dump(out, prefix, "ie3", begspn, sp, pos1ob, posfob,
                     ctx.mq5a_stime.data());
                // The change tables start one period in (x11pt4.f's mfda), and
                // each has a percent twin that is the SAME series scaled x100 in
                // multiplicative/log-additive mode and unscaled in additive mode
                // (pragr2.f passes Muladd.ne.1 as punch's percent flag).
                const int chg1 = pos1ob + 1;
                const double pscale = (ctx.x11opt.muladd != 1) ? 100.0 : 1.0;
                struct { const char* t; const char* p; const std::vector<double>* v; }
                    chg[] = {{"ie5", "ip5", &ctx.x11_e5}, {"ie6", "ip6", &ctx.x11_e6},
                             {"ie7", "ip7", &ctx.x11_e7}, {"ie8", "ip8", &ctx.x11_e8}};
                for (const auto& c : chg) {
                    if (c.v->empty()) continue;
                    dump(out, prefix, c.t, begspn, sp, chg1, posfob, c.v->data(),
                         pos1ob);
                    for (int i = chg1; i <= posfob; ++i) {
                        int idate[2];
                        x13::addate(begspn, sp, i - pos1ob, idate);
                        std::snprintf(cbuf, sizeof cbuf, "%s%s %04d%02d %.15E\n",
                                      prefix.c_str(), c.p, idate[0], idate[1],
                                      (*c.v)[i - 1] * pscale);
                        out += cbuf;
                    }
                }
                dump(out, prefix, "iee", begspn, sp, pos1ob, posfob,
                     ctx.x11_e11.data());
                const int e18_frst = ctx.tbllog.savbct ? ctx.x11ptr.pos1bk : pos1ob;
                const int e18_last = ctx.tbllog.savfct ? ctx.x11ptr.posffc : posfob;
                dump(out, prefix, "i18", begspn, sp, e18_frst, e18_last,
                     ctx.x11_e18.data(), pos1ob);
                dump(out, prefix, "ita", begspn, sp, e18_frst, e18_last,
                     ctx.x11_eb.data(), pos1ob);
            }
            // The INDIRECT x11pt4 diagnostics (x11ari.f:341) -- the .udg's
            // `if2.*` / `if3.*` block. Same field layout as the direct `f2.*` /
            // `f3.*` in tools/x13run_x11.cpp; only the source snapshot differs.
            if (ctx.agr_f3_set) {
                const x13::tests_cmn& t = ctx.agr_f2tests;
                const x13::inpt2_cmn& q = ctx.agr_f2inpt2;
                const x13::work2_cmn& w = ctx.agr_f2work2;
                // Fpres/P3 is the B1 F-test from x11pt2, which has no indirect
                // counterpart: the oracle prints the DIRECT value under `if2.fsb1`.
                std::snprintf(cbuf, sizeof cbuf, "if2.fsb1 %.3f %.2f\n", t.fpres, t.p3);
                out += cbuf;
                std::snprintf(cbuf, sizeof cbuf, "if2.fsd8 %.3f %.2f\n", t.fstabl, t.p1);
                out += cbuf;
                std::snprintf(cbuf, sizeof cbuf, "if2.kw %.3f %.2f\n", t.chikw, t.p5);
                out += cbuf;
                std::snprintf(cbuf, sizeof cbuf, "if2.msf %.3f %.2f\n", t.fmove, t.p2);
                out += cbuf;
                std::snprintf(cbuf, sizeof cbuf, "if2.idseasonal %s\n",
                              t.iqfail == 1 ? "yes" : "no");
                out += cbuf;
                std::string line;
                for (int i = 1; i <= sp; ++i) {
                    std::snprintf(cbuf, sizeof cbuf, "if2.a%02d", i);
                    line = cbuf;
                    const double v[11] = {q.obar(i), q.cibar(i), q.ibar(i), q.cbar(i),
                                          q.sbar(i), w.pbar(i), q.tdbar(i), q.smbar(i),
                                          q.ombar(i), q.cimbar(i), q.imbar(i)};
                    for (double d : v) {
                        std::snprintf(cbuf, sizeof cbuf, " %.15E", d);
                        line += cbuf;
                    }
                    out += line + "\n";
                }
                for (int i = 1; i <= sp; ++i) {
                    std::snprintf(cbuf, sizeof cbuf, "if2.b%02d", i);
                    line = cbuf;
                    const double v[6] = {q.isq(i), q.csq(i), q.ssq(i), w.psq(i),
                                         q.tdsq(i), q.osq2(i)};
                    for (double d : v) {
                        std::snprintf(cbuf, sizeof cbuf, " %.15E", d);
                        line += cbuf;
                    }
                    out += line + "\n";
                }
                for (int i = 1; i <= sp; ++i) {
                    std::snprintf(cbuf, sizeof cbuf, "if2.c%02d", i);
                    line = cbuf;
                    const double v[12] = {q.obar2(i), q.osd(i), q.ibar2(i), q.isd(i),
                                          q.cbar2(i), q.csd(i), q.sbar2(i), q.ssd(i),
                                          q.cibar2(i), q.cisd(i), q.smbar2(i), q.smsd(i)};
                    for (double d : v) {
                        std::snprintf(cbuf, sizeof cbuf, " %.15E", d);
                        line += cbuf;
                    }
                    out += line + "\n";
                }
                std::snprintf(cbuf, sizeof cbuf, "if2.d %.15E %.15E %.15E %.15E\n",
                              q.adrci, q.adri, q.adrc, q.adrmcd);
                out += cbuf;
                line = "if2.e";
                for (int i = 1; i <= sp; ++i) {
                    std::snprintf(cbuf, sizeof cbuf, " %.15E", q.smic(i));
                    line += cbuf;
                }
                out += line + "\n";
                std::snprintf(cbuf, sizeof cbuf, "if2.mcd %d\n", ctx.agr_f2mcd);
                out += cbuf;
                std::snprintf(cbuf, sizeof cbuf,
                              "if2.f %.15E %.15E %.15E %.15E %.15E %.15E\n",
                              q.vi, q.vc, q.vs, q.vp, q.vtd, q.rv);
                out += cbuf;
                line = "if2.g";
                for (int i = 1; i <= sp + 2; ++i) {
                    std::snprintf(cbuf, sizeof cbuf, " %.15E", w.autoc(i));
                    line += cbuf;
                }
                out += line + "\n";
                std::snprintf(cbuf, sizeof cbuf, "if2.ic %.15E\n", ctx.agr_f2ratic);
                out += cbuf;
                std::snprintf(cbuf, sizeof cbuf, "if2.is %.15E\n", ctx.agr_f2ratis);
                out += cbuf;
                for (int i = 1; i <= w.nn; ++i) {
                    if (i == 6 && ctx.x11opt.kfulsm >= 2) continue;
                    std::snprintf(cbuf, sizeof cbuf, "if3.m%02d %.15E\n", i, w.qu(i));
                    out += cbuf;
                }
                std::snprintf(cbuf, sizeof cbuf, "if3.q %.15E\n", w.qual);
                out += cbuf;
                std::snprintf(cbuf, sizeof cbuf, "if3.qm2 %.15E\n", w.q2m2);
                out += cbuf;
                std::snprintf(cbuf, sizeof cbuf, "if3.fail %d\n", w.kfail);
                out += cbuf;
            }
        } else {
            dump(out, prefix, "d10", begspn, sp, sf_frst, sf_last, ctx.x11srs.sts.data());
            dump(out, prefix, "d11", begspn, sp, pos1ob, posfob, ctx.x11srs.stci.data());
            dump(out, prefix, "d12", begspn, sp, pos1ob, posfob, ctx.x11srs.stc.data());
            dump(out, prefix, "d13", begspn, sp, pos1ob, posfob, ctx.x11srs.sti.data());
        }

        // The DIRECT QS / spectrum-peak / NP savelog blocks (x11ari.f:277-326).
        // They belong to every spec of the metafile, components included -- the
        // oracle writes one .udg per spec and every one of them carries these.
        // This harness had never emitted them at all, so a composite run
        // reported none of the ~90 keys per spec that the other two harnesses
        // have gated since the spectrum-peak and QS increments landed.
        dump_qs(ctx, prefix.c_str(), &out);
        dump_np(ctx, prefix.c_str(), &out);
        dump_spec_peaks(ctx, prefix.c_str(), &out);

        // history{} on a composite: each spec's own sar/sae (the DIRECT
        // revisions of that series) plus, on the aggregate total only, the
        // INDIRECT iar/iae the components folded into /revdta/ (Indrev) and the
        // `historyindsa` savelog line (revdrv.f:1199).
        if (ctx.hist_out.ran) {
            const auto& ho = ctx.hist_out;
            char hbuf[160];
            // history{sadjlags=} appends one "N later" column per surviving lag
            // (plus the 1yr-2yr one on the revision tables), in the save file's
            // own order, so the leading columns stay where every gate expects.
            auto ext = [&](const std::vector<double>& v, int n, std::size_t r) {
                for (int k = 0; k < n; ++k) {
                    std::snprintf(hbuf, sizeof hbuf, " %.15E",
                                  v[r * static_cast<std::size_t>(n) + k]);
                    out += hbuf;
                }
            };
            for (std::size_t r = 0; r < ho.dates.size(); ++r) {
                if (ho.have_sa) {
                    std::snprintf(hbuf, sizeof hbuf, "%ssar %06d %.15E",
                                  prefix.c_str(), ho.dates[r], ho.sar[r]);
                    out += hbuf;
                    ext(ho.sar_t, ho.ncol_sa, r);
                    out += "\n";
                    std::snprintf(hbuf, sizeof hbuf, "%ssae %06d %.15E %.15E",
                                  prefix.c_str(), ho.dates[r], ho.sae_cnc[r],
                                  ho.sae_fin[r]);
                    out += hbuf;
                    ext(ho.sae_t, ho.ntarsa, r);
                    out += "\n";
                }
                if (ho.have_ind) {
                    std::snprintf(hbuf, sizeof hbuf, "%siar %06d %.15E",
                                  prefix.c_str(), ho.dates[r], ho.iar[r]);
                    out += hbuf;
                    ext(ho.iar_t, ho.ncol_sa, r);
                    out += "\n";
                    std::snprintf(hbuf, sizeof hbuf, "%siae %06d %.15E %.15E",
                                  prefix.c_str(), ho.dates[r], ho.iae_cnc[r],
                                  ho.iae_fin[r]);
                    out += hbuf;
                    ext(ho.iae_t, ho.ntarsa, r);
                    out += "\n";
                }
            }
            if (ho.ind_reported) {
                std::snprintf(hbuf, sizeof hbuf, "%shistoryindsa %s\n",
                              prefix.c_str(), ho.ind_yes ? "yes" : "no");
                out += hbuf;
            }
        }
    }
    std::printf("OUTCOME: OK\n");
    std::fputs(out.c_str(), stdout);
    return 0;
}
