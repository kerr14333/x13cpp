// x13run_iddiff.cpp -- focused test harness for the automatic differencing-order
// identifier (iddiff.f). Parses a spec, runs the pre-model phase (series read +
// transform + regvar, no estimation) via x13::run_m2, then drives x13::iddiff on
// the transformed series and prints the identified orders as `KEY value` lines.
//
// Usage: x13run_iddiff <specfile.spc> [maxdr] [maxds]   (defaults 2 1)
//
// The oracle reports iddiff's decision in the automdl .udg as
// `idnonseasonaldiff.first` / `idseasonaldiff.first`; the parity gate diffs the
// printed idr/ids against those. No output files are written (package rule).
#include <cstdio>
#include <cstdlib>
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

#include "automdl/iddiff.hpp"
#include "automdl/amdid.hpp"
#include "specparse/specparse.hpp"
#include "gen/srslen.hpp"   // prm::PLEN
#include "gen/model.hpp"    // prm::PORDER

namespace {
std::string dirname_of(const std::string& p) {
    std::size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? std::string() : p.substr(0, s);
}
std::string basename_of(const std::string& p) {
    std::size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? p : p.substr(s + 1);
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: x13run_iddiff <specfile.spc> [maxdr] [maxds]\n");
        return 2;
    }
    std::string path = argv[1];
    int maxdr = 2, maxds = 1;
    {
        int pos = 0;  // positional numeric args, skipping --flags
        for (int i = 2; i < argc; ++i) {
            std::string s = argv[i];
            if (s.rfind("--", 0) == 0) continue;
            if (pos == 0) maxdr = std::atoi(s.c_str());
            else if (pos == 1) maxds = std::atoi(s.c_str());
            ++pos;
        }
    }

    std::string dir = dirname_of(path);
    std::string full = basename_of(path);
    std::string base = full.size() >= 4 && full.substr(full.size() - 4) == ".spc"
                           ? full.substr(0, full.size() - 4)
                           : full;

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "x13run_iddiff: cannot open %s\n", path.c_str());
        return 2;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string text = ss.str();
    in.close();

    if (!dir.empty()) chdir(dir.c_str());

    auto ctxp = std::make_unique<x13::X13Context>();
    x13::X13Context& ctx = *ctxp;
    bool ok;
    try {
        ok = x13::run_m2(ctx, text, base, /*estimate=*/false);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "x13run_iddiff: exception: %s\n", e.what());
        return 3;
    }
    if (!ok || ctx.error.lfatal) {
        std::printf("OUTCOME: FATAL\n");
        return 1;
    }

    constexpr int PA = x13::prm::PLEN + 2 * x13::prm::PORDER;
    std::vector<double> a(static_cast<std::size_t>(PA), 0.0);
    int idr = maxdr, ids = maxds, nefobs = 0, frstry = 0, na = 0;
    bool lmu = false;
    try {
        x13::iddiff(ctx, idr, ids, ctx.series.tsrs.data(), nefobs, frstry, a.data(),
                    na, /*imu=*/0, lmu, /*svldif=*/false, /*lsumm=*/0);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "x13run_iddiff: iddiff exception: %s\n", e.what());
        return 3;
    }
    if (ctx.error.lfatal) {
        std::printf("OUTCOME: FATAL (iddiff)\n");
        return 1;
    }

    std::printf("OUTCOME: OK\n");
    std::printf("idnonseasonaldiff.first: %d\n", idr);
    std::printf("idseasonaldiff.first: %d\n", ids);
    std::printf("checkmu: %s\n", lmu ? "yes" : "no");

    // Optional: continue into ARMA-order identification (amdid) on the chosen
    // differencing. Requires an automdl{} spec (maxorder/maxdiff defaults).
    bool do_amdid = false;
    for (int i = 2; i < argc; ++i)
        if (std::string(argv[i]) == "--amdid") do_amdid = true;
    if (do_amdid) {
        int irar = 0, irma = 0, isar = 0, isma = 0;
        bool locok = true;
        try {
            x13::amdid(ctx, irar, idr, irma, isar, ids, isma,
                       ctx.series.tsrs.data(), frstry, nefobs, a.data(), na, lmu,
                       /*lsumm=*/0, locok);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "x13run_iddiff: amdid exception: %s\n", e.what());
            return 3;
        }
        if (ctx.error.lfatal) {
            std::printf("OUTCOME: FATAL (amdid)\n");
            return 1;
        }
        std::printf("arimamdl: (%d %d %d)(%d %d %d)\n", irar, idr, irma, isar, ids,
                    isma);
    }
    return 0;
}
