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
void dump(std::string& out, const std::string& prefix, const char* tag,
          const int* begspn, int sp, int pos1ob, int last, const double* arr) {
    char buf[128];
    for (int i = pos1ob; i <= last; ++i) {
        int idate[2];
        x13::addate(begspn, sp, i - pos1ob, idate);
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
        } else {
            dump(out, prefix, "d10", begspn, sp, sf_frst, sf_last, ctx.x11srs.sts.data());
            dump(out, prefix, "d11", begspn, sp, pos1ob, posfob, ctx.x11srs.stci.data());
            dump(out, prefix, "d12", begspn, sp, pos1ob, posfob, ctx.x11srs.stc.data());
            dump(out, prefix, "d13", begspn, sp, pos1ob, posfob, ctx.x11srs.sti.data());
        }
    }
    std::printf("OUTCOME: OK\n");
    std::fputs(out.c_str(), stdout);
    return 0;
}
