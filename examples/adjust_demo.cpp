// adjust_demo.cpp -- a small example of the modern C++ framework.
//
// Shows the whole intended usage: hand a .spc file to the facade, get a
// `SeasonalAdjustment` value back, and read its components as ordinary series.
// No engine internals appear anywhere in this file -- that's the point of the
// framework layer.
//
//   adjust_demo <spec.spc>
#include <cstdio>
#include <exception>

#include "x13/api/x13.hpp"

namespace {

void printHead(const char* label, const x13::api::TimeSeries& s, std::size_t n) {
    if (s.empty()) {
        std::printf("  %-22s (not produced by this run)\n", label);
        return;
    }
    std::printf("  %-22s %zu obs, %s, starting %d-%02d\n", label, s.size(),
                s.frequency() == x13::api::Frequency::Monthly ? "monthly" : "quarterly",
                s.start().year, s.start().period);
    const std::size_t show = n < s.size() ? n : s.size();
    for (std::size_t i = 0; i < show; ++i) {
        const auto d = s.dateAt(i);
        std::printf("      %d-%02d  %14.4f\n", d.year, d.period, s[i]);
    }
    if (show < s.size()) std::printf("      ...\n");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: adjust_demo <spec.spc>\n");
        return 2;
    }

    try {
        const auto result = x13::api::adjustFromSpecFile(argv[1]);
        const auto& info = result.info();

        const char* modeName = "multiplicative";
        switch (info.mode) {
            case x13::api::DecompositionMode::Additive: modeName = "additive"; break;
            case x13::api::DecompositionMode::LogAdditive: modeName = "log-additive"; break;
            case x13::api::DecompositionMode::PseudoAdditive: modeName = "pseudo-additive"; break;
            case x13::api::DecompositionMode::Multiplicative: break;
        }

        std::printf("Seasonal adjustment of '%s'\n", info.seriesName.c_str());
        std::printf("  %s, %zu observations, %s, %s\n",
                    info.frequency == x13::api::Frequency::Monthly ? "monthly" : "quarterly",
                    info.observations, info.modelBased ? "model-based" : "direct X-11",
                    modeName);
        if (!info.arimaModel.empty())
            std::printf("  model: %s\n", info.arimaModel.c_str());
        std::printf("\n");

        printHead("original (input)", result.original(), 3);
        printHead("seasonal (D10)", result.seasonal(), 3);
        printHead("seasonally adjusted", result.seasonallyAdjusted(), 3);
        printHead("trend-cycle (D12)", result.trend(), 3);
        printHead("irregular (D13)", result.irregular(), 3);
        if (result.hasForcedSeasonallyAdjusted())
            printHead("forced SA (D11A)", result.forcedSeasonallyAdjusted(), 3);
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "adjust_demo: %s\n", e.what());
        return 1;
    }
}
