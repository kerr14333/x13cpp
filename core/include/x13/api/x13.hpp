// x13/api/x13.hpp -- the public entry point of the modern C++ framework that
// wraps the X-13ARIMA-SEATS engine.
//
// Include this one header to get the whole surface: the value types
// (`TimeSeries`, `Date`, `Frequency`), the result object (`SeasonalAdjustment`),
// and the `adjust*` facade functions that run the engine and hand back a result.
//
// Design intent: a caller who knows seasonal adjustment -- but nothing about the
// Fortran port underneath -- should be able to read and use this without meeting
// a single `farray`, COMMON block, or `Lfatal` flag. The engine stays exactly as
// ported (bit-exact against the oracle); this layer only translates its inputs
// and outputs to and from ordinary C++ values.
//
//   #include "x13/api/x13.hpp"
//   auto result = x13::api::adjustFromSpecFile("airline.spc");
//   const auto& sa = result.seasonallyAdjusted();
//   for (std::size_t i = 0; i < sa.size(); ++i)
//       std::printf("%d-%02d  %.4f\n", sa.dateAt(i).year, sa.dateAt(i).period, sa[i]);
#ifndef X13_API_X13_HPP
#define X13_API_X13_HPP

#include <stdexcept>
#include <string>

#include "x13/api/seasonal_adjustment.hpp"
#include "x13/api/timeseries.hpp"

namespace x13::api {

// Thrown when the engine cannot complete a run (parse error, non-convergence,
// or any path the engine reports as fatal). The message carries what context the
// engine surfaced.
class AdjustmentError : public std::runtime_error {
public:
    explicit AdjustmentError(const std::string& what) : std::runtime_error(what) {}
};

// Run an X-13 seasonal adjustment described by the text of a `.spc` spec.
//
// `seriesName` names the run for metadata/diagnostics; it does not need to match
// anything in the spec. If the spec references an external data file by relative
// path, resolve it relative to the current working directory (prefer
// `adjustFromSpecFile`, which handles the spec's own directory for you).
//
// Throws `AdjustmentError` if the engine reports a fatal condition.
SeasonalAdjustment adjustFromSpec(const std::string& specText,
                                  const std::string& seriesName = "series");

// Run an X-13 seasonal adjustment described by a `.spc` file on disk. Relative
// data-file paths inside the spec are resolved against the spec file's own
// directory, matching the reference driver's behavior.
//
// Throws `AdjustmentError` if the file cannot be read or the engine reports a
// fatal condition.
SeasonalAdjustment adjustFromSpecFile(const std::string& specPath);

}  // namespace x13::api

#endif  // X13_API_X13_HPP
