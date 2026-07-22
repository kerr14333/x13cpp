// x13/api/timeseries.hpp -- a small, value-semantic time-series type for the
// public C++ framework that wraps the X-13 engine.
//
// This is part of the modern, readable API surface (the "framework" layer): it
// deliberately hides the engine's Fortran-derived internals (1-based `farray`s,
// COMMON-block structs, column-major buffers) behind an ordinary C++ value type.
// It holds a calendar-anchored, fixed-frequency series and nothing else -- no
// engine state, no ownership of buffers. Copyable and movable like any value.
#ifndef X13_API_TIMESERIES_HPP
#define X13_API_TIMESERIES_HPP

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace x13::api {

// Observation frequency. The underlying value is the number of periods per year,
// which is exactly the engine's `Sp`, so it converts directly.
enum class Frequency : int {
    Quarterly = 4,
    Monthly = 12,
};

// A calendar position: a 1-based period within a year (Jan == 1, Q1 == 1).
struct Date {
    int year = 0;
    int period = 0;

    friend bool operator==(const Date& a, const Date& b) {
        return a.year == b.year && a.period == b.period;
    }
    friend bool operator!=(const Date& a, const Date& b) { return !(a == b); }
};

// A fixed-frequency series of observations anchored at a start `Date`. Values are
// stored contiguously in chronological order; the date of the i-th value is
// derived from `start()` and `frequency()`.
class TimeSeries {
public:
    TimeSeries() = default;

    TimeSeries(Date start, Frequency freq, std::vector<double> values)
        : start_(start), freq_(freq), values_(std::move(values)) {}

    // --- shape -------------------------------------------------------------
    bool empty() const { return values_.empty(); }
    std::size_t size() const { return values_.size(); }
    Frequency frequency() const { return freq_; }
    int periodsPerYear() const { return static_cast<int>(freq_); }
    Date start() const { return start_; }

    // --- values ------------------------------------------------------------
    // Positional access, 0-based and chronological (index 0 == start()).
    double operator[](std::size_t i) const { return values_[i]; }
    double at(std::size_t i) const {
        if (i >= values_.size()) throw std::out_of_range("TimeSeries::at index");
        return values_[i];
    }
    const std::vector<double>& values() const { return values_; }

    // --- calendar ----------------------------------------------------------
    // The calendar date of the i-th observation.
    Date dateAt(std::size_t i) const {
        const int ppy = periodsPerYear();
        // Convert the start period (1-based) to a 0-based absolute period index,
        // add the offset, then split back into (year, 1-based period).
        const long long abs0 =
            static_cast<long long>(start_.year) * ppy + (start_.period - 1) +
            static_cast<long long>(i);
        Date d;
        d.year = static_cast<int>(abs0 / ppy);
        d.period = static_cast<int>(abs0 % ppy) + 1;
        return d;
    }

    // The value at a given calendar date, or throws if the date is outside the
    // series span.
    double value(const Date& when) const {
        const int ppy = periodsPerYear();
        const long long offset =
            (static_cast<long long>(when.year) * ppy + (when.period - 1)) -
            (static_cast<long long>(start_.year) * ppy + (start_.period - 1));
        if (offset < 0 || offset >= static_cast<long long>(values_.size()))
            throw std::out_of_range("TimeSeries::value date out of span");
        return values_[static_cast<std::size_t>(offset)];
    }

    // Chronological iteration over the raw values.
    std::vector<double>::const_iterator begin() const { return values_.begin(); }
    std::vector<double>::const_iterator end() const { return values_.end(); }

private:
    Date start_{};
    Frequency freq_ = Frequency::Monthly;
    std::vector<double> values_{};
};

}  // namespace x13::api

#endif  // X13_API_TIMESERIES_HPP
