// test_api.cpp -- unit tests for the framework value types (header-only). These
// check the calendar arithmetic that maps positional indices to dates and back;
// the engine-facing facade is covered by the parity suite, not here.
#include "microtest.hpp"
#include "x13/api/timeseries.hpp"

using namespace x13::api;

int main() { return mt::run_all(); }

TEST("monthly dateAt walks and wraps the year") {
    TimeSeries s(Date{1949, 1}, Frequency::Monthly, {1, 2, 3});
    CHECK_EQ(s.size(), static_cast<std::size_t>(3));
    CHECK(s.dateAt(0) == (Date{1949, 1}));
    CHECK(s.dateAt(11) == (Date{1949, 12}));
    CHECK(s.dateAt(12) == (Date{1950, 1}));
}

TEST("monthly dateAt from a mid-year start") {
    TimeSeries s(Date{1949, 11}, Frequency::Monthly, {1, 2, 3, 4});
    CHECK(s.dateAt(0) == (Date{1949, 11}));
    CHECK(s.dateAt(1) == (Date{1949, 12}));
    CHECK(s.dateAt(2) == (Date{1950, 1}));
    CHECK(s.dateAt(3) == (Date{1950, 2}));
}

TEST("quarterly dateAt wraps every four periods") {
    TimeSeries s(Date{2000, 3}, Frequency::Quarterly, {10, 20, 30});
    CHECK_EQ(s.periodsPerYear(), 4);
    CHECK(s.dateAt(0) == (Date{2000, 3}));
    CHECK(s.dateAt(1) == (Date{2000, 4}));
    CHECK(s.dateAt(2) == (Date{2001, 1}));
}

TEST("value() is the inverse of dateAt") {
    TimeSeries s(Date{1949, 1}, Frequency::Monthly, {112, 118, 132, 129, 121});
    for (std::size_t i = 0; i < s.size(); ++i)
        CHECK_EQ(s.value(s.dateAt(i)), s[i]);
    CHECK_EQ(s.value(Date{1949, 3}), 132.0);
}

TEST("value() throws outside the span") {
    TimeSeries s(Date{1949, 1}, Frequency::Monthly, {1, 2, 3});
    bool threw = false;
    try {
        s.value(Date{1948, 12});
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}

TEST("default series is empty") {
    TimeSeries s;
    CHECK(s.empty());
    CHECK_EQ(s.size(), static_cast<std::size_t>(0));
}
