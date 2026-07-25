// x13run_m3.cpp -- M3 CLI test harness: parse a spec, run the pre-model phase
// AND estimate the regARIMA model (x13::run_m2 with estimate=true), then print
// the estimation results as `KEY value` lines to stdout for the estimation
// parity gate to diff against the oracle .udg goldens.
//
// Usage: x13run_m3 <specfile.spc>
//
// This harness deliberately does NOT write any output file -- the library
// captures everything on the context (the package's "no auto file output"
// rule); only this thin test driver surfaces it, and only to stdout. The full
// oracle-format .udg emission is a separate (deferred) milestone; here we print
// the numeric estimation summary the gate needs.
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

#include "specparse/specparse.hpp"
#include "gen/model.hpp"  // prm::AR, prm::MA

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
        std::fprintf(stderr, "usage: x13run_m3 <specfile.spc>\n");
        return 2;
    }
    std::string path = argv[1];
    std::string dir = dirname_of(path);
    std::string full = basename_of(path);
    std::string base = full.size() >= 4 && full.substr(full.size() - 4) == ".spc"
                           ? full.substr(0, full.size() - 4)
                           : full;

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "x13run_m3: cannot open %s\n", path.c_str());
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
        ok = x13::run_m2(ctx, text, base, /*estimate=*/true);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "x13run_m3: exception: %s\n", e.what());
        return 3;
    }

    std::printf("OUTCOME: %s\n", ok ? "OK" : "FATAL");
    if (!ok) return 1;
    if (!ctx.captured.has_model) {
        std::printf("MODEL: none\n");  // no arima model -> nothing to estimate
        return 0;
    }

    const auto& m = ctx.model;
    const auto& d = ctx.mdldat;
    const auto& lk = ctx.lkhd;
    // Nspobs is the SERIES span after a modelspan run (setspn.f restores it), so
    // prefer the count the fit recorded; fall back for the no-estimate paths.
    int nefobs = ctx.est_nefobs > 0 ? ctx.est_nefobs : d.nspobs - m.nintvl;

    // Integer counters + dimensions.
    std::printf("converged: %s\n", d.convrg ? "yes" : "no");
    std::printf("armaer: %d\n", d.armaer);
    std::printf("niter: %d\n", d.nliter);
    std::printf("nfev: %d\n", d.nfev);
    std::printf("nreg: %d\n", m.nb);
    std::printf("nefobs: %d\n", nefobs);

    // Scalar estimation results (%.14E mirrors the oracle Nform precision).
    std::printf("variance: %.14E\n", d.var);
    std::printf("loglikelihood: %.14E\n", d.lnlkhd);
    std::printf("aic: %.14E\n", lk.aic);
    std::printf("aicc: %.14E\n", lk.aicc);
    std::printf("bic: %.14E\n", lk.bic);
    std::printf("hq: %.14E\n", lk.hnquin);

    // ARMA coefficients in operator/lag order (AR then MA), skipping the fixed
    // differencing slots; label each free coef by type + lag.
    for (int iflt = x13::prm::AR; iflt <= x13::prm::MA; ++iflt) {
        int begopr = m.mdl(iflt - 1);
        int endopr = m.mdl(iflt) - 1;
        const char* kind = (iflt == x13::prm::AR) ? "ar" : "ma";
        for (int iopr = begopr; iopr <= endopr; ++iopr) {
            int beglag = m.opr(iopr - 1);
            int endlag = m.opr(iopr) - 1;
            for (int ilag = beglag; ilag <= endlag; ++ilag)
                std::printf("arima.%s.lag%d: %.14E\n", kind, m.arimal(ilag),
                            d.arimap(ilag));
        }
    }

    // Regression betas in design-column order.
    for (int j = 1; j <= m.nb; ++j)
        std::printf("beta%d: %.14E\n", j, d.b(j));

    // Forecast-output table (fcstout / prtfct .fct path): original-scale point
    // forecast + two-tailed confidence band, one line per lead.
    const auto& fo = ctx.forecasts;
    std::printf("nfcst: %d\n", fo.nfcst);
    for (int i = 0; i < fo.nfcst; ++i)
        std::printf("fcst%d: %.14E %.14E %.14E\n", i + 1, fo.fcst[i],
                    fo.lwrci[i], fo.uprci[i]);

    return 0;
}
