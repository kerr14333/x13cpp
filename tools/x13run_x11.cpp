// x13run_x11.cpp -- M5 X-11 CLI harness: parse a spec, assemble the X-11
// decomposition spine (x13::run_x11, the no-model direct-X11 path), then print
// the X-11 tables as `<tag> YYYYMM <value>` lines to stdout for the X-11 parity
// gate to diff against the oracle save-table goldens.
//
// Usage: x13run_x11 <specfile.spc>
//
// Like the other run_* harnesses this writes no output file -- the library keeps
// everything on the context (the package's "no auto file output" rule); this thin
// driver only surfaces the ctx x11srs arrays to stdout, numerically. The exact
// oracle-format .b1/.d* save emission is a separate deferred milestone.
//
// Tables printed (span [Pos1ob, Posfob]):
//   b1  -- prior-adjusted B1 input          (orisrs Stcsi)
// plus, for inspection while the D-pass finals (x11pt3) land, the working arrays
// left by x11pt2 at D7: trend (Stc), seasonal (Sts), SA (Stci), irregular (Sti).
// Only b1 is a stable golden target until x11pt3/x11pt4 are wired.
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
#include "common/x13context.hpp"

namespace {
std::string dirname_of(const std::string& p) {
    std::size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? std::string() : p.substr(0, s);
}
std::string basename_of(const std::string& p) {
    std::size_t s = p.find_last_of("/\\");
    return (s == std::string::npos) ? p : p.substr(s + 1);
}

// Emit one table section: `<tag> YYYYMM <value>` over the span [pos1ob, posfob].
void dump(const char* tag, const int* begspn, int sp, int pos1ob, int posfob,
          const double* arr /*1-based*/) {
    for (int i = pos1ob; i <= posfob; ++i) {
        int idate[2];
        x13::addate(begspn, sp, i - pos1ob, idate);
        std::printf("%s %04d%02d %.15E\n", tag, idate[0], idate[1], arr[i - 1]);
    }
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: x13run_x11 <specfile.spc>\n");
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
        std::fprintf(stderr, "x13run_x11: cannot open %s\n", path.c_str());
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
        ok = x13::run_x11(ctx, text, base);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "x13run_x11: exception: %s\n", e.what());
        return 3;
    }

    std::printf("OUTCOME: %s\n", ok ? "OK" : "FATAL");
    if (!ok) return 1;

    const int sp = ctx.model.sp;
    const int* begspn = ctx.mdldat.begspn.data();
    const int pos1ob = ctx.x11ptr.pos1ob;
    const int posfob = ctx.x11ptr.posfob;

    // Gate targets produced by the x11pt1 + x11pt2 spine:
    //   b1 -- prior-adjusted B1 input. x11pt2 rewrites Stcsi in place for the
    //         C/D passes, so B1 is read from the input Series (== B1 on the
    //         no-prior path; a dedicated snapshot follows once priors wire in).
    //   d7 -- final X-11 trend-cycle (Stc) at the D7 return point of x11pt2.
    dump("b1", begspn, sp, pos1ob, posfob, ctx.inpt.series.data());
    dump("d7", begspn, sp, pos1ob, posfob, ctx.x11srs.stc.data());
    return 0;
}
