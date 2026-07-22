// test_api_facade.cpp -- integration test for the framework facade.
//
// The header-only value type is covered by test_api; this checks the other half:
// that `adjustFromSpecFile` runs the engine and extracts the decomposition tables
// CORRECTLY -- same numbers, same calendar alignment -- as the committed oracle
// goldens, to the same 1e-8 the parity suite uses. It catches span/date/table
// mapping mistakes the compile-only build cannot.
//
// It drives one committed no-model X-11 spec (airline_x11-logadd) whose goldens
// already ship, and compares the facade's seasonal/SA/trend/irregular series
// against tables d10/d11/d12/d13 by calendar key.
#include "microtest.hpp"
#include "x13/api/x13.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>

#ifndef X13_REPO_ROOT
#define X13_REPO_ROOT "."
#endif

using namespace x13::api;

namespace {

std::string repoPath(const std::string& rel) {
    return std::string(X13_REPO_ROOT) + "/" + rel;
}

// Parse an oracle save-table golden: lines of "<YYYYMM><ws><signed float>",
// Fortran 'D' exponents allowed, header lines ignored. Keyed by the YYYYMM code.
std::map<int, double> readGolden(const std::string& path) {
    std::map<int, double> out;
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        std::size_t i = 0;
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
        // Need six leading digits for the date.
        if (line.size() - i < 6) continue;
        bool sixDigits = true;
        for (std::size_t k = 0; k < 6; ++k)
            if (!std::isdigit(static_cast<unsigned char>(line[i + k]))) { sixDigits = false; break; }
        if (!sixDigits) continue;
        const int date = std::stoi(line.substr(i, 6));
        std::size_t j = i + 6;
        while (j < line.size() && std::isspace(static_cast<unsigned char>(line[j]))) ++j;
        if (j >= line.size() || (line[j] != '+' && line[j] != '-' && !std::isdigit(static_cast<unsigned char>(line[j]))))
            continue;
        std::string tok = line.substr(j);
        for (char& c : tok) { if (c == 'D' || c == 'd') c = 'E'; }
        try {
            out[date] = std::stod(tok);
        } catch (...) { /* skip non-numeric */ }
    }
    return out;
}

Date dateOfCode(int yyyymm) { return Date{yyyymm / 100, yyyymm % 100}; }

// Decomposition tables are pure arithmetic (no estimation), so they match the
// oracle to the golden's printed-precision floor (~1e-15). We gate them at 1e-12
// -- three orders of margin over libm-transcendental/text-truncation noise, per
// the tolerance policy (arithmetic 1e-12; estimation-derived quantities 1e-6).
constexpr double kArithTol = 1e-12;

// Compare one facade series against a golden table by calendar key.
void checkTable(const char* tag, const TimeSeries& series, const std::string& goldenPath,
                double tol = kArithTol) {
    const auto gold = readGolden(goldenPath);
    if (gold.empty()) { CHECK(false); std::printf("    [%s] empty/missing golden %s\n", tag, goldenPath.c_str()); return; }
    if (series.empty()) { CHECK(false); std::printf("    [%s] facade produced no series\n", tag); return; }

    double worst = 0.0;
    int worstKey = 0;
    for (const auto& [code, g] : gold) {
        const double v = series.value(dateOfCode(code));  // throws if misaligned
        const double rel = (g != 0.0) ? std::fabs(v - g) / std::fabs(g) : std::fabs(v - g);
        if (rel > worst) { worst = rel; worstKey = code; }
    }
    CHECK(worst <= tol);
    if (worst > tol)
        std::printf("    [%s] worst rel err %.3e at %d (tol %.0e)\n", tag, worst, worstKey, tol);
}

}  // namespace

int main() { return mt::run_all(); }

TEST("facade tables match the X-11 goldens (arithmetic, 1e-12)") {
    const std::string spec =
        repoPath("tests/corpus/generated/airline_x11-logadd.spc");
    const std::string gdir =
        repoPath("tests/golden/generated/airline_x11-logadd/airline_x11-logadd");

    SeasonalAdjustment r = adjustFromSpecFile(spec);

    // Basic shape: monthly airline, 144 obs.
    CHECK_EQ(r.info().observations, static_cast<std::size_t>(144));
    CHECK(r.seasonallyAdjusted().frequency() == Frequency::Monthly);

    checkTable("d10", r.seasonal(), gdir + ".d10");
    checkTable("d11", r.seasonallyAdjusted(), gdir + ".d11");
    checkTable("d12", r.trend(), gdir + ".d12");
    checkTable("d13", r.irregular(), gdir + ".d13");
}

TEST("facade handles the model-based path (automdl + aictest)") {
    const std::string spec =
        repoPath("tests/corpus/generated/airline_automdl-aictest-x11.spc");
    const std::string gdir = repoPath(
        "tests/golden/generated/airline_automdl-aictest-x11/airline_automdl-aictest-x11");

    SeasonalAdjustment r = adjustFromSpecFile(spec);

    // Metadata should reflect that a regARIMA model drove this run.
    CHECK(r.info().modelBased);
    CHECK(!r.info().arimaModel.empty());

    // Tables must still be bit-exact against the goldens on the model path.
    checkTable("d10", r.seasonal(), gdir + ".d10");
    checkTable("d11", r.seasonallyAdjusted(), gdir + ".d11");
    checkTable("d12", r.trend(), gdir + ".d12");
    checkTable("d13", r.irregular(), gdir + ".d13");
}

TEST("facade surfaces the forced SA series (force{} D11A)") {
    const std::string spec = repoPath("tests/corpus/extra/airline_force-denton.spc");
    const std::string gdir =
        repoPath("tests/golden/extra/airline_force-denton/airline_force-denton");

    SeasonalAdjustment r = adjustFromSpecFile(spec);

    // The run forced yearly totals, so the D11A series must be present...
    CHECK(r.info().forced);
    CHECK(r.hasForcedSeasonallyAdjusted());
    // ...and match the oracle's .saa golden by calendar key.
    checkTable("saa", r.forcedSeasonallyAdjusted(), gdir + ".saa");

    // A non-force run must not report a forced series.
    SeasonalAdjustment plain = adjustFromSpecFile(
        repoPath("tests/corpus/generated/airline_x11-logadd.spc"));
    CHECK(!plain.info().forced);
    CHECK(!plain.hasForcedSeasonallyAdjusted());
}
