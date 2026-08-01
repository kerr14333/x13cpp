// test_ctod.cpp -- pins ctod.f's decimal reader, bit for bit.
//
// WHY THIS EXISTS. `ctod` is the character-to-double behind gtdpvc/getdbl, so
// it converts EVERY decimal literal in every spec file: regression{b=}
// coefficients, critical values, aicdiff, pvaic, sigma limits, user-regressor
// data. It is not strtod and it is not "accumulate a mantissa, divide once" --
// it adds each fractional digit's own quotient (`val = val + digit/scl`), so
// the result carries the rounding error of every intermediate division. 52 of
// the 286 distinct decimal literals in tests/corpus therefore land exactly
// 1 ulp away from the correctly-rounded double, in BOTH directions.
//
// That is faithful, and it is load-bearing: replacing it with a correctly-
// rounded conversion moves real output. But it moves it almost invisibly --
// measured, the swap breaks exactly ONE parity gate, at the 10th significant
// digit of one d9a row of one generated spec. A single accidental canary is
// not a guardrail, which is what this test is for: it fails immediately and
// says why.
//
// EXPECTED VALUES ARE ORACLE OUTPUTS, not a re-transcription of the algorithm
// -- re-deriving them from the same reading that produced the C++ would prove
// nothing. They come from `tools/ref_ctod.f` compiled against the vendored
// oracle/fortran/ctod.f with rtools44 gfortran -O2; that file carries the build
// line. Compared as BIT PATTERNS, because a decimal literal in this source
// cannot express a 1-ulp distinction unambiguously -- writing 0.95 here would
// be read by the C++ compiler's own (correctly-rounded) parser and silently
// compare the wrong value.
#include "microtest.hpp"
#include "specparse/specparse.hpp"

#include <cstdint>
#include <cstring>
#include <string>

using namespace x13;

namespace {

double from_bits(std::uint64_t b) {
    double d;
    std::memcpy(&d, &b, sizeof d);
    return d;
}

std::uint64_t to_bits(double d) {
    std::uint64_t b;
    std::memcpy(&b, &d, sizeof b);
    return b;
}

// Parse `lit` exactly as the spec reader does, from position 1.
double parse(const char* lit, int& ipos) {
    ipos = 1;
    return ctod(std::string_view(lit), ipos);
}

}  // namespace

TEST("ctod: oracle bit patterns, both rounding directions") {
    struct Case {
        const char* lit;
        std::uint64_t bits;   // tools/ref_ctod.f against oracle/fortran/ctod.f
        int ipos;             // and its position advance
    };
    // The handoff's example, the two families the corpus leans on
    // (regression{b=} coefficients and user-regressor data), and literals
    // ctod rounds DOWN as well as up.
    static const Case cases[] = {
        {"0.95",     0x3FEE666666666667ULL,  5},
        {"0.045",    0x3FA70A3D70A3D70AULL,  6},
        {"0.0045",   0x3F726E978D4FDF3CULL,  7},
        {"-0.0045",  0xBF726E978D4FDF3CULL,  8},
        {"0.015451", 0x3F8FA4C61D8622C3ULL,  9},
        {"0.022700", 0x3F973EAB367A0F90ULL,  9},
        {"0.047553", 0x3FA858DDE7A743A7ULL,  9},
        {"0.069500", 0x3FB1CAC083126E97ULL,  9},
        {"18.33",    0x4032547AE147AE15ULL,  6},
        {"20.197",   0x4034326E978D4FE0ULL,  7},
        {"2.37",     0x4002F5C28F5C28F5ULL,  5},
        {"7.32",     0x401D47AE147AE147ULL,  5},
    };
    for (const Case& c : cases) {
        int ipos = 0;
        double got = parse(c.lit, ipos);
        CHECK_EQ(to_bits(got), c.bits);
        CHECK_EQ(ipos, c.ipos);
    }
}

TEST("ctod: differs from correctly-rounded conversion, and by 1 ulp") {
    // The property the test above pins, stated directly: for these literals a
    // correctly-rounded reader gives a DIFFERENT double, one ulp away. If this
    // ever passes trivially (got == strtod for all of them) the accumulator has
    // been replaced and the case above will already have failed -- this is the
    // statement of intent that explains why.
    struct Case { const char* lit; std::uint64_t bits; };
    static const Case cases[] = {
        {"0.95",     0x3FEE666666666667ULL},   // ctod rounds UP
        {"0.069500", 0x3FB1CAC083126E97ULL},   // ctod rounds DOWN
        {"2.37",     0x4002F5C28F5C28F5ULL},
    };
    for (const Case& c : cases) {
        int ipos = 0;
        double got = parse(c.lit, ipos);
        double rounded = std::stod(c.lit);     // correctly rounded
        CHECK(to_bits(got) != to_bits(rounded));
        std::int64_t d = static_cast<std::int64_t>(to_bits(got)) -
                         static_cast<std::int64_t>(to_bits(rounded));
        CHECK(d == 1 || d == -1);
        CHECK_EQ(got, from_bits(c.bits));
    }
}

TEST("ctod: exactly representable literals are untouched") {
    // The accumulator is only visible where the decimal is not a dyadic
    // rational. These must agree with a correctly-rounded reader, or the
    // deviation above is not the one being described.
    static const char* exact[] = {"3.5", "19.0", "0.25", "1.96", "0.5", "2.0"};
    for (const char* lit : exact) {
        int ipos = 0;
        double got = parse(lit, ipos);
        CHECK_EQ(to_bits(got), to_bits(std::stod(lit)));
    }
}

TEST("ctod: no digits consumed leaves ipos where it started") {
    // ctod.f's `havdbl` guard -- getdbl keys "was this a number?" on ipos not
    // advancing, so a reader that returned 0.0 while advancing would turn every
    // non-numeric token into a silent zero.
    int ipos = 1;
    double got = ctod(std::string_view("abc"), ipos);
    CHECK_EQ(got, 0.0);
    CHECK_EQ(ipos, 1);
}

int main() { return mt::run_all(); }
