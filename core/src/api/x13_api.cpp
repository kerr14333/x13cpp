// x13_api.cpp -- implementation of the public framework facade.
//
// This is the one translation unit that bridges the modern API to the ported
// engine: it owns an `X13Context`, drives `run_x11`, and copies the resulting
// decomposition arrays out into `TimeSeries` values. Everything Fortran-shaped
// (1-based `farray`s, the `x11ptr` span, the `has_model` flag) is confined here.
#include "x13/api/x13.hpp"

#include <fstream>
#include <memory>
#include <sstream>

#ifdef _WIN32
#include <direct.h>
#define x13_chdir _chdir
#define x13_getcwd _getcwd
#else
#include <unistd.h>
#define x13_chdir chdir
#define x13_getcwd getcwd
#endif

#include "common/x13context.hpp"
#include "specparse/specparse.hpp"

namespace x13::api {
namespace {

// Copy one engine table (a 1-based array spanning the observation range
// [pos1ob, posfob]) into a chronological, calendar-anchored TimeSeries.
TimeSeries extractTable(const X13Context& ctx, const double* oneBased) {
    const int sp = ctx.model.sp;
    const int pos1ob = ctx.x11ptr.pos1ob;
    const int posfob = ctx.x11ptr.posfob;
    if (posfob < pos1ob) return {};

    int startDate[2];
    x13::addate(ctx.mdldat.begspn.data(), sp, 0, startDate);

    std::vector<double> values;
    values.reserve(static_cast<std::size_t>(posfob - pos1ob + 1));
    for (int i = pos1ob; i <= posfob; ++i) values.push_back(oneBased[i - 1]);

    const Frequency freq = (sp == 4) ? Frequency::Quarterly : Frequency::Monthly;
    return TimeSeries(Date{startDate[0], startDate[1]}, freq, std::move(values));
}

// Map the engine's Muladd code (+ the pseudo-additive flag) to the API enum.
// Encoding from the spec parser (gtinpt.cpp: getx11.f mode translation):
//   mult -> 0, add -> 1, logadd -> 2; pseudoadd -> 0 with Psuadd set.
DecompositionMode mapMode(int muladd, bool psuadd) {
    if (psuadd) return DecompositionMode::PseudoAdditive;
    switch (muladd) {
        case 1: return DecompositionMode::Additive;
        case 2: return DecompositionMode::LogAdditive;
        default: return DecompositionMode::Multiplicative;  // 0
    }
}

}  // namespace

SeasonalAdjustment adjustFromSpec(const std::string& specText,
                                  const std::string& seriesName) {
    // The context holds ~25k of engine COMMON blocks -- keep it off the stack.
    auto ctxp = std::make_unique<X13Context>();
    X13Context& ctx = *ctxp;

    bool ok = false;
    try {
        ok = x13::run_x11(ctx, specText, seriesName);
    } catch (const std::exception& e) {
        throw AdjustmentError(std::string("engine error: ") + e.what());
    }
    if (!ok) throw AdjustmentError("engine reported a fatal condition for '" +
                                   seriesName + "'");

    const bool modelBased = ctx.captured.has_model;

    // b1: model runs decompose the prior-adjusted B1 snapshot (Stoap); plain
    // X-11 runs decompose the raw input series. Mirrors the reference driver.
    const double* b1 = modelBased ? ctx.orisrs.stoap.data()
                                  : ctx.inpt.series.data();

    SeasonalAdjustment result;
    result.setOriginal(extractTable(ctx, b1));
    result.setSeasonal(extractTable(ctx, ctx.x11srs.sts.data()));           // D10
    result.setSeasonallyAdjusted(extractTable(ctx, ctx.x11srs.stci.data()));// D11
    result.setTrend(extractTable(ctx, ctx.x11srs.stc.data()));             // D12
    result.setIrregular(extractTable(ctx, ctx.x11srs.sti.data()));          // D13

    // force{}: when yearly totals were forced, Stci2 holds the revised SA series
    // (D11A). Iyrt==0 means force was not requested (Stci2 is just a copy of D11).
    const bool forced = ctx.force.iyrt > 0;
    if (forced)
        result.setForcedSeasonallyAdjusted(extractTable(ctx, ctx.adxser.stci2.data()));

    RunInfo info;
    info.seriesName = seriesName;
    info.frequency = (ctx.model.sp == 4) ? Frequency::Quarterly : Frequency::Monthly;
    info.observations = result.original().size();
    info.modelBased = modelBased;
    info.mode = mapMode(ctx.x11opt.muladd, ctx.x11msc.psuadd);
    info.arimaModel = ctx.arima.bstdsn.str();  // empty when no model was identified
    info.forced = forced;
    result.setInfo(std::move(info));

    return result;
}

SeasonalAdjustment adjustFromSpecFile(const std::string& specPath) {
    std::ifstream in(specPath, std::ios::binary);
    if (!in) throw AdjustmentError("cannot open spec file: " + specPath);
    std::ostringstream ss;
    ss << in.rdbuf();
    const std::string text = ss.str();

    // Split into directory + base name so relative data paths in the spec resolve
    // against the spec's own directory (as the reference driver does).
    const std::size_t slash = specPath.find_last_of("/\\");
    const std::string dir =
        (slash == std::string::npos) ? std::string() : specPath.substr(0, slash);
    std::string base =
        (slash == std::string::npos) ? specPath : specPath.substr(slash + 1);
    if (base.size() >= 4 && base.substr(base.size() - 4) == ".spc")
        base = base.substr(0, base.size() - 4);

    if (dir.empty()) return adjustFromSpec(text, base);

    // Run with the spec's directory as CWD, then restore -- keep the facade a
    // pure function from the caller's point of view.
    char saved[4096];
    const bool haveCwd = x13_getcwd(saved, sizeof(saved)) != nullptr;
    if (x13_chdir(dir.c_str()) != 0)
        throw AdjustmentError("cannot enter spec directory: " + dir);

    try {
        SeasonalAdjustment result = adjustFromSpec(text, base);
        if (haveCwd) x13_chdir(saved);
        return result;
    } catch (...) {
        if (haveCwd) x13_chdir(saved);
        throw;
    }
}

}  // namespace x13::api
