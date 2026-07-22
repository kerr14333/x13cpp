// x13/api/seasonal_adjustment.hpp -- the result object returned by the public
// framework after running an X-13 seasonal adjustment.
//
// A `SeasonalAdjustment` is an immutable value: it carries the decomposition
// tables as `TimeSeries` and some light metadata about the run. It owns copies of
// the numbers, so it outlives the engine context that produced it. Tables that a
// given run did not produce (e.g. `priorAdjusted` on a plain no-model X-11 run)
// are reported as empty series; use `has*()` to test presence.
#ifndef X13_API_SEASONAL_ADJUSTMENT_HPP
#define X13_API_SEASONAL_ADJUSTMENT_HPP

#include <string>

#include "x13/api/timeseries.hpp"

namespace x13::api {

// How the components combine to reconstruct the original series.
enum class DecompositionMode {
    Multiplicative,  // original = trend * seasonal * irregular  (the X-13 default)
    Additive,        // original = trend + seasonal + irregular
    LogAdditive,     // additive in the logs
    PseudoAdditive,
};

// Light metadata describing the run that produced a result.
struct RunInfo {
    std::string seriesName;      // the spec's series name / base
    Frequency frequency = Frequency::Monthly;
    std::size_t observations = 0;
    bool modelBased = false;     // true if a regARIMA model drove the run
    DecompositionMode mode = DecompositionMode::Multiplicative;
    std::string arimaModel;      // e.g. "(0 1 1)(0 1 1)"; empty if none/unknown
    bool forced = false;         // true if force{} revised the SA yearly totals
};

// The outcome of an X-13 seasonal adjustment.
//
// Table naming follows the domain, not the Fortran letter-number codes, so the
// API reads for someone who knows seasonal adjustment but not X-13's internals.
// The X-11 letter code each accessor corresponds to is noted for cross-reference.
class SeasonalAdjustment {
public:
    SeasonalAdjustment() = default;

    // --- metadata ----------------------------------------------------------
    const RunInfo& info() const { return info_; }

    // --- components --------------------------------------------------------
    // The input series the engine decomposed. For a model-based run this is the
    // prior-adjusted B1 input; for a plain X-11 run it is the raw input series.
    const TimeSeries& original() const { return original_; }

    // The final seasonal factors (X-11 table D10).
    const TimeSeries& seasonal() const { return seasonal_; }

    // The final seasonally adjusted series (X-11 table D11).
    const TimeSeries& seasonallyAdjusted() const { return seasonallyAdjusted_; }

    // The final trend-cycle (X-11 table D12).
    const TimeSeries& trend() const { return trend_; }

    // The final irregular component (X-11 table D13).
    const TimeSeries& irregular() const { return irregular_; }

    // The seasonally adjusted series after force{} revised its calendar-year
    // totals to match a target (X-11 table D11A). Empty unless the run forced
    // yearly totals; info().forced reports whether it is present.
    const TimeSeries& forcedSeasonallyAdjusted() const {
        return forcedSeasonallyAdjusted_;
    }

    // --- presence tests ----------------------------------------------------
    bool hasSeasonal() const { return !seasonal_.empty(); }
    bool hasSeasonallyAdjusted() const { return !seasonallyAdjusted_.empty(); }
    bool hasTrend() const { return !trend_.empty(); }
    bool hasIrregular() const { return !irregular_.empty(); }
    bool hasForcedSeasonallyAdjusted() const {
        return !forcedSeasonallyAdjusted_.empty();
    }

    // --- builder (used by the facade; not part of typical user code) -------
    // Kept public and simple so the engine-facing translation unit can populate
    // a result without a friend declaration. User code should treat results as
    // read-only.
    void setInfo(RunInfo info) { info_ = std::move(info); }
    void setOriginal(TimeSeries s) { original_ = std::move(s); }
    void setSeasonal(TimeSeries s) { seasonal_ = std::move(s); }
    void setSeasonallyAdjusted(TimeSeries s) { seasonallyAdjusted_ = std::move(s); }
    void setTrend(TimeSeries s) { trend_ = std::move(s); }
    void setIrregular(TimeSeries s) { irregular_ = std::move(s); }
    void setForcedSeasonallyAdjusted(TimeSeries s) {
        forcedSeasonallyAdjusted_ = std::move(s);
    }

private:
    RunInfo info_{};
    TimeSeries original_{};
    TimeSeries seasonal_{};
    TimeSeries seasonallyAdjusted_{};
    TimeSeries trend_{};
    TimeSeries irregular_{};
    TimeSeries forcedSeasonallyAdjusted_{};
};

}  // namespace x13::api

#endif  // X13_API_SEASONAL_ADJUSTMENT_HPP
