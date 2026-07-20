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
#include "automdl/automd.hpp"
#include "automdl/amdest.hpp"   // cnvmdl (read identified orders)
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

    bool do_amdid = false, do_automd = false, do_trnaic = false;
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--amdid") do_amdid = true;
        if (std::string(argv[i]) == "--automd") do_automd = true;
        if (std::string(argv[i]) == "--trnaic") do_trnaic = true;
    }

    // --trnaic: report the automatic transform-selection result. run_m2 already
    // ran trnaic during the pre-model phase (transform{function=auto} sets
    // Fcntyp==0); print the two default-airline-model AICC values and the choice,
    // matching the oracle .udg aictest.trans.aicc.nolog / .log / aictrans keys.
    if (do_trnaic) {
        std::printf("OUTCOME: OK\n");
        if (!ctx.trnaic_result.ran) {
            std::printf("trnaic.ran: no\n");
            return 0;
        }
        std::printf("aictest.trans.aicc.nolog: %.15E\n",
                    ctx.trnaic_result.aicno);
        std::printf("aictest.trans.aicc.log: %.15E\n", ctx.trnaic_result.aiclog);
        std::printf("aictrans: %s\n",
                    ctx.trnaic_result.selected_log ? "Log(y)" : "None");
        return 0;
    }

    // --automd: run the unified (reduced) driver end to end, then read back the
    // identified orders and estimation results.
    if (do_automd) {
        constexpr int PAd = x13::prm::PLEN + 2 * x13::prm::PORDER;
        std::vector<double> ad(static_cast<std::size_t>(PAd), 0.0);
        std::vector<double> trn(static_cast<std::size_t>(x13::prm::PLEN));
        x13::copy(ctx.series.tsrs.data(), x13::prm::PLEN, 1, trn.data());
        int fr = 0, nef = 0, nad = 0;
        try {
            x13::automd(ctx, trn.data(), fr, nef, ad.data(), nad);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "x13run_iddiff: automd exception: %s\n", e.what());
            return 3;
        }
        if (ctx.error.lfatal) {
            std::printf("OUTCOME: FATAL (automd)\n");
            return 1;
        }
        int ipr, ips, idr2, ids2, iqr, iqs, id, ip, iq, iprs, iqrs, n;
        x13::cnvmdl(ctx, ipr, ips, idr2, ids2, iqr, iqs, id, ip, iq, iprs, iqrs, n);
        std::printf("OUTCOME: OK\n");
        if (ips == 0 && ids2 == 0 && iqs == 0)
            std::printf("arimamdl: (%d %d %d)\n", ipr, idr2, iqr);
        else
            std::printf("arimamdl: (%d %d %d)(%d %d %d)\n", ipr, idr2, iqr, ips,
                        ids2, iqs);
        std::printf("variance: %.10E\n", ctx.mdldat.var);
        std::printf("loglikelihood: %.10E\n", ctx.mdldat.lnlkhd);
        std::printf("checkmu: %s\n", ctx.model.nb > 0 ? "yes" : "no");
        return 0;
    }

    constexpr int PA = x13::prm::PLEN + 2 * x13::prm::PORDER;
    std::vector<double> a(static_cast<std::size_t>(PA), 0.0);
    int idr = maxdr, ids = maxds, nefobs = 0, frstry = 0, na = 0;
    bool lmu = false;

    // The identification routines take Trnsrs -- the transformed series -- as a
    // read-only input, a SEPARATE buffer from Tsrs (which rgarma overwrites with
    // regression residuals during estimation). Give them their own copy so an
    // internal rgarma cannot clobber the series a later stage reads.
    std::vector<double> trnsrs(static_cast<std::size_t>(x13::prm::PLEN));
    x13::copy(ctx.series.tsrs.data(), x13::prm::PLEN, 1, trnsrs.data());

    try {
        x13::iddiff(ctx, idr, ids, trnsrs.data(), nefobs, frstry, a.data(),
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
    if (do_amdid) {
        int irar = 0, irma = 0, isar = 0, isma = 0;
        bool locok = true;
        try {
            x13::amdid(ctx, irar, idr, irma, isar, ids, isma, trnsrs.data(),
                       frstry, nefobs, a.data(), na, lmu, /*lsumm=*/0, locok);
        } catch (const std::exception& e) {
            std::fprintf(stderr, "x13run_iddiff: amdid exception: %s\n", e.what());
            return 3;
        }
        if (ctx.error.lfatal) {
            std::printf("OUTCOME: FATAL (amdid)\n");
            return 1;
        }
        // Match the oracle .udg format: omit the seasonal group when it is all
        // zero (the (P D Q) block is only printed when present).
        if (isar == 0 && ids == 0 && isma == 0)
            std::printf("arimamdl: (%d %d %d)\n", irar, idr, irma);
        else
            std::printf("arimamdl: (%d %d %d)(%d %d %d)\n", irar, idr, irma, isar,
                        ids, isma);
    }
    return 0;
}
