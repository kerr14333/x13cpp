// test_fformat.cpp -- validate the Fortran FORMAT engine against gfortran itself.
//
// A single case table drives BOTH sides: for each {format, args} the test
//   (a) runs it through x13::fwrite_fmt, and
//   (b) emits an equivalent Fortran program, compiles it with gfortran (from the
//       rtools44 toolchain), runs it, and captures the output.
// The two are compared. Because the Fortran source is generated from the same
// table, the reference can never drift from what we test.
//
// The gfortran executable is located via the FFORMAT_GFORTRAN env var (set by
// the build) or falls back to a known rtools44 path / bare "gfortran".
#include "microtest.hpp"
#include "x13/fformat.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace x13;

namespace {

struct Case {
    std::string fmt;
    std::vector<FmtArg> args;
};

std::string scratch_dir() {
    const char* e = std::getenv("FFORMAT_SCRATCH");
    if (e && *e) return e;
    return ".";
}

std::string gfortran_path() {
    const char* e = std::getenv("FFORMAT_GFORTRAN");
    if (e && *e) return e;
    return "gfortran";
}

// Emit a real literal that Fortran reads as double precision.
std::string real_literal(double v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.17e", v);
    std::string s(buf);
    for (char& c : s) if (c == 'e' || c == 'E') c = 'd';
    return s;
}

std::string escape_fortran_str(const std::string& s) {
    std::string out;
    for (char c : s) { if (c == '\'') out += "''"; else out += c; }
    return out;
}

// Generate a Fortran program that prints each case followed by a sentinel line.
std::string gen_fortran(const std::vector<Case>& cases) {
    // Find the widest character arg so a single character array length suffices.
    std::size_t maxlen = 1, maxargs = 1;
    for (const auto& c : cases) {
        maxargs = std::max(maxargs, c.args.size());
        for (const auto& a : c.args)
            if (a.kind() == FmtArg::Kind::Str) maxlen = std::max(maxlen, a.as_str().size());
    }
    std::ostringstream f;
    f << "      program fmttest\n";
    f << "      implicit none\n";
    // Distinct indexed variables per position so multiple same-type args in one
    // WRITE do not overwrite each other.
    f << "      real(8) :: r(" << maxargs << ")\n";
    f << "      integer :: iv(" << maxargs << ")\n";
    f << "      character(len=" << maxlen << ") :: cv(" << maxargs << ")\n";
    f << "      logical :: lv(" << maxargs << ")\n";
    for (const auto& c : cases) {
        // Build the write-args list and any needed assignments.
        std::string writeargs;
        std::ostringstream pre;
        int n = 0;
        for (const auto& a : c.args) {
            if (n) writeargs += ", ";
            std::string idx = std::to_string(n + 1);
            switch (a.kind()) {
                case FmtArg::Kind::Int:
                    pre << "      iv(" << idx << ") = " << a.as_int() << "\n";
                    writeargs += "iv(" + idx + ")"; break;
                case FmtArg::Kind::Real:
                    pre << "      r(" << idx << ") = " << real_literal(a.as_real()) << "\n";
                    writeargs += "r(" + idx + ")"; break;
                case FmtArg::Kind::Logical:
                    pre << "      lv(" << idx << ") = ." << (a.as_logical() ? "true" : "false") << ".\n";
                    writeargs += "lv(" + idx + ")"; break;
                case FmtArg::Kind::Str:
                    pre << "      cv(" << idx << ") = '" << escape_fortran_str(a.as_str()) << "'\n";
                    writeargs += "cv(" + idx + ")(1:" +
                        std::to_string(a.as_str().size() ? a.as_str().size() : 1) + ")";
                    break;
            }
            ++n;
        }
        f << pre.str();
        // Delimit the format with double quotes so single-quote literals inside
        // the format (e.g. 'val=') do not clash.
        f << "      write(*,\"" << c.fmt << "\") " << writeargs << "\n";
        f << "      write(*,'(A)') '@@@'\n";
    }
    f << "      end program\n";
    return f.str();
}

// Run gfortran, return true and fill `output` on success.
bool run_gfortran(const std::string& src, std::string& output, std::string& err) {
    std::string dir = scratch_dir();
    std::string fsrc = dir + "/fformat_ref.f90";
    std::string fexe = dir + "/fformat_ref.exe";
    std::string fout = dir + "/fformat_ref.out";
    { std::ofstream o(fsrc); o << src; }
    // On Windows, cmd.exe strips the outer quote pair when a command both starts
    // and ends with a quote, corrupting multi-quoted commands. Wrap the whole
    // command in an extra pair so the inner quotes survive.
    std::string cmd = "\"\"" + gfortran_path() + "\" -o \"" + fexe + "\" \"" + fsrc + "\" 2> \"" + dir + "/fformat_ref.cerr\"\"";
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::ifstream ce(dir + "/fformat_ref.cerr");
        std::stringstream ss; ss << ce.rdbuf(); err = ss.str();
        return false;
    }
    std::string runcmd = "\"\"" + fexe + "\" > \"" + fout + "\"\"";
    rc = std::system(runcmd.c_str());
    if (rc != 0) { err = "gfortran program exited nonzero"; return false; }
    std::ifstream in(fout, std::ios::binary);
    std::stringstream ss; ss << in.rdbuf();
    output = ss.str();
    return true;
}

// Split gfortran output into per-case blocks on the "@@@" sentinel.
// Normalize CRLF -> LF. Each block excludes the sentinel line.
std::vector<std::string> split_blocks(const std::string& out) {
    std::string s;
    s.reserve(out.size());
    for (char c : out) if (c != '\r') s += c;   // strip CR
    std::vector<std::string> blocks;
    std::istringstream is(s);
    std::string line;
    std::string cur;
    bool first = true;
    while (std::getline(is, line)) {
        if (line == "@@@") { blocks.push_back(cur); cur.clear(); first = true; }
        else { if (!first) cur += "\n"; cur += line; first = false; }
    }
    return blocks;
}

const std::vector<Case>& cases() {
    static const std::vector<Case> c = {
        // Integers
        {"(I5)",    {42}},
        {"(I5)",    {-42}},
        {"(I3)",    {12345}},        // overflow -> ***
        {"(I5.3)",  {7}},            // zero-padded to 3
        {"(I6)",    {0}},
        {"(I4)",    {-1}},
        // Fixed
        {"(F8.3)",  {3.14159}},
        {"(F8.3)",  {-3.14159}},
        {"(F8.2)",  {0.005}},        // rounding
        {"(F8.2)",  {0.015}},        // tie
        {"(F8.2)",  {0.025}},        // tie
        {"(F6.2)",  {12345.6}},      // overflow
        {"(F10.4)", {-0.00001}},     // negative near zero
        {"(F7.1)",  {99.95}},
        {"(F9.4)",  {1234.5678}},
        // Scientific
        {"(E12.4)", {12345.678}},
        {"(E12.4)", {-12345.678}},
        {"(E12.4)", {0.00012345}},
        {"(E12.4)", {0.0}},
        {"(E15.6)", {6.022e23}},
        {"(E12.3E3)",{1.5e-120}},    // 3-digit exponent
        {"(1PE12.4)",{12345.678}},   // scale factor
        {"(1PE12.4)",{-0.00012345}},
        // General
        {"(G12.4)", {12345.678}},    // -> E branch
        {"(G12.4)", {1.2345}},       // -> F branch
        {"(G12.4)", {0.012345}},     // -> E branch (below 0.1)
        {"(G14.6)", {123.456}},
        {"(G12.5)", {0.0}},
        // Character
        {"(A)",     {std::string("hello")}},
        {"(A8)",    {std::string("hi")}},       // right justified in 8
        {"(A3)",    {std::string("toolong")}},  // truncated
        // Logical
        {"(L2)",    {true}},
        // X, literals, repeats, combos
        {"(I3,2X,I3)", {7, 9}},
        {"('val=',I4)", {123}},
        {"(3I4)",   {1, 2, 3}},
        {"(2(I3,1X))", {5, 6}},
        {"(F6.2,A,I3)", {2.5, std::string(":"), 8}},
        // Multi-record (slash) and format reversion
        {"(I3/I3)", {1, 2}},
        {"(2I3)", {1, 2, 3}},            // reversion -> second record
        {"(I2,I2,I2,I2)", {1, 2, 3, 4}},
        // Tab positioning
        {"(T5,I3)", {7}},
        {"(F5.1,TR2,F5.1)", {1.5, 2.5}},
        {"(I3,T2,I2)", {100, 9}},        // tab left overwrites
        // More rounding ties (half-even boundaries)
        {"(F4.1)", {0.25}},
        {"(F4.1)", {0.35}},
        {"(F4.1)", {2.5}},
        {"(F6.0)", {2.5}},
        {"(F6.0)", {3.5}},
        {"(E13.5)", {9.999995e0}},       // rounding cascade
        {"(1PE12.3)", {9.9996e3}},        // scale + rounding overflow
        // G with scale and larger widths
        {"(G15.7)", {1234567.0}},
        {"(G10.3)", {0.0001234}},
        // Negative exponents
        {"(E12.4)", {-6.022e-23}},
        // Integer edge
        {"(I11)", {-2000000000}},
    };
    return c;
}

} // namespace

TEST("fformat matches gfortran reference") {
    const auto& cs = cases();
    std::string src = gen_fortran(cs);
    std::string out, err;
    bool ok = run_gfortran(src, out, err);
    if (!ok) {
        // If gfortran is unavailable, skip loudly rather than fail the build.
        std::printf("    [skip] gfortran unavailable: %s\n", err.c_str());
        std::printf("    (set FFORMAT_GFORTRAN to the gfortran path to enable)\n");
        return;
    }
    auto blocks = split_blocks(out);
    REQUIRE_EQ(blocks.size(), cs.size());
    for (std::size_t i = 0; i < cs.size(); ++i) {
        std::string got = fwrite_fmt_v(cs[i].fmt, cs[i].args);
        // gfortran block has no trailing newline (getline strips it); our single
        // record output likewise has none. Multi-record (slash) not in battery.
        const std::string& want = blocks[i];
        if (got != want) {
            std::printf("    case %zu  fmt=%s\n", i, cs[i].fmt.c_str());
            std::printf("        gfortran=[%s]\n", want.c_str());
            std::printf("        fwrite  =[%s]\n", got.c_str());
        }
        CHECK_EQ(got, want);
    }
}

int main() { return mt::run_all(); }
