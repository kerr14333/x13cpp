// x13parse.cpp -- M1 CLI test harness.
//
// Usage: x13parse <specfile.spc>
//
// Parses the spec file through x13::parse_spec (running the ported spec-parser
// subsystem) and prints:
//   * the .err-channel (Mt2) content -- for parity against the oracle .err;
//   * an OUTCOME line (OK / FATAL);
//   * a CAPTURED block echoing key parsed settings (period, span, model, etc.).
//
// Relative data-file paths in the spec resolve against the spec file's
// directory (matching the oracle, which is run from that directory).
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

namespace {
std::string dirname_of(const std::string& p) {
    std::size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? std::string() : p.substr(0, s);
}
std::string basename_of(const std::string& p) {
    std::size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? p : p.substr(s + 1);
}
} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: x13parse <specfile.spc>\n");
        return 2;
    }
    std::string path = argv[1];
    std::string dir = dirname_of(path);
    std::string base = basename_of(path);

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::fprintf(stderr, "x13parse: cannot open %s\n", path.c_str());
        return 2;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string text = ss.str();
    in.close();

    // Resolve relative data paths as the oracle does: run from the spec's dir.
    if (!dir.empty()) chdir(dir.c_str());

    auto ctxp = std::make_unique<x13::X13Context>();
    x13::X13Context& ctx = *ctxp;
    bool ok;
    try {
        ok = x13::parse_spec(ctx, text, base);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "x13parse: exception: %s\n", e.what());
        return 3;
    }

    // Emit the .err channel content.
    std::string err = ctx.channels_.unit(ctx.units.mt2).str();
    std::fputs("===ERR===\n", stdout);
    std::fputs(err.c_str(), stdout);
    std::fputs("===END ERR===\n", stdout);
    std::printf("OUTCOME: %s\n", ok ? "OK" : "FATAL");

    const auto& c = ctx.captured;
    std::printf("CAPTURED:\n");
    std::printf("  specs = ");
    for (std::size_t i = 0; i < c.spec_order.size(); ++i)
        std::printf("%s%s", c.spec_order[i].c_str(), i + 1 < c.spec_order.size() ? "," : "");
    std::printf("\n");
    std::printf("  period = %d\n", c.period);
    std::printf("  nobs = %d\n", c.nobs);
    std::printf("  series_start = %d.%d\n", c.series_start[0], c.series_start[1]);
    std::printf("  span = %d.%d - %d.%d\n", c.span_start[0], c.span_start[1],
                c.span_end[0], c.span_end[1]);
    if (!c.transform_function.empty())
        std::printf("  transform_function = %s\n", c.transform_function.c_str());
    if (!c.model_desc.empty())
        std::printf("  arima_model = %s\n", c.model_desc.c_str());
    if (c.forecast_maxlead >= 0)
        std::printf("  forecast_maxlead = %d\n", c.forecast_maxlead);
    if (!c.x11_mode.empty())
        std::printf("  x11_mode = %s\n", c.x11_mode.c_str());
    if (!c.regression_vars.empty()) {
        std::printf("  regression_vars = ");
        for (auto& v : c.regression_vars) std::printf("%s ", v.c_str());
        std::printf("\n");
    }
    if (!c.aictest_vars.empty()) {
        std::printf("  aictest_vars = ");
        for (auto& v : c.aictest_vars) std::printf("%s ", v.c_str());
        std::printf("\n");
    }
    return ok ? 0 : 1;
}
