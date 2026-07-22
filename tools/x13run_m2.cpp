// x13run_m2.cpp -- M2 CLI test harness.
//
// Usage: x13run_m2 <specfile.spc>
//
// Parses the spec, runs the M2 pre-model phase (x13::run_m2), writes every
// captured save table to <base>.<ext> in the spec's directory (as the oracle
// CLI does), and prints an OUTCOME line plus a per-table summary. Save-file
// bytes are written verbatim from the captured text so a parity gate can
// compare them against the oracle's save files.
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
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: x13run_m2 <specfile.spc>\n");
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
        std::fprintf(stderr, "x13run_m2: cannot open %s\n", path.c_str());
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
        ok = x13::run_m2(ctx, text, base);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "x13run_m2: exception: %s\n", e.what());
        return 3;
    }

    std::printf("OUTCOME: %s\n", ok ? "OK" : "FATAL");

    // Emit both error channels so runtime (transform/model) FATALs can be gated
    // on their message text: Mt2 is the .err channel (parity vs the oracle .err),
    // the stdio::STDERR channel is the separate stderr stream. trnfcn.f and other
    // routines deliberately word the two differently (e.g. "log of a zero" to Mt2,
    // "log of zero" to STDERR); dumping both locks that split.
    std::string errm = ctx.channels_.unit(ctx.units.mt2).str();
    std::fputs("===ERR===\n", stdout);
    std::fputs(errm.c_str(), stdout);
    std::fputs("===END ERR===\n", stdout);
    std::string errs = ctx.channels_.unit(x13::stdio::STDERR).str();
    std::fputs("===STDERR===\n", stdout);
    std::fputs(errs.c_str(), stdout);
    std::fputs("===END STDERR===\n", stdout);

    for (const auto& st : ctx.saves.tables) {
        // Write the save file verbatim (binary: bytes exactly as captured).
        std::ofstream of(st.filename, std::ios::binary);
        of.write(st.text.data(), static_cast<std::streamsize>(st.text.size()));
        of.close();
        std::printf("SAVE: %s  ext=%s  n=%zu\n", st.filename.c_str(),
                    st.ext.c_str(), st.data.size());
    }
    return ok ? 0 : 1;
}
