// test_capi.cpp -- unit tests for the flat C ABI (core/include/x13/capi.h).
//
// These cover the ABI's CONTRACT rather than the engine's numbers (the numbers
// are gated against the oracle by tests/parity/test_bindings.py, through the
// Python binding). What matters here is that every promise the header makes to
// a foreign-language caller actually holds -- because a violated promise on
// this boundary is not an exception, it is a segfault inside R or Python.
//
// Specifically: NULL handles are safe, out-of-range indices are safe, the
// size-then-fill protocol reports the needed size without writing, a failed run
// still returns a usable handle, and nothing throws across the boundary.
#include "microtest.hpp"

#include "x13/capi.h"

#include <cstring>
#include <string>
#include <vector>

// A minimal, self-contained spec: the data is inline, so this test needs no
// corpus file and no working directory of its own.
const char* kSpec =
    "series{\n"
    "  title = \"capi test\"\n"
    "  start = 1990.01\n"
    "  period = 12\n"
    "  data = (\n"
    "    112 118 132 129 121 135 148 148 136 119 104 118\n"
    "    115 126 141 135 125 149 170 170 158 133 114 140\n"
    "    145 150 178 163 172 178 199 199 184 162 146 166\n"
    "    171 180 193 181 183 218 230 242 209 191 172 194\n"
    "    196 196 236 235 229 243 264 272 237 211 180 201\n"
    "    204 188 235 227 234 264 302 293 259 229 203 229\n"
    "    242 233 267 269 270 315 364 347 312 274 237 278\n"
    "    284 277 317 313 318 374 413 405 355 306 271 306\n"
    "  )\n"
    "}\n"
    "x11{ }\n";

// --- NULL safety -----------------------------------------------------------
// R's .C shim can hand us a NULL for a stale integer handle, so this is a live
// path, not a theoretical one.
TEST("null_handle_is_safe") {
    CHECK_EQ(x13_ok(nullptr), 0);
    CHECK(x13_error(nullptr) != nullptr);
    CHECK_EQ(std::strlen(x13_error(nullptr)), 0u);
    CHECK_EQ(x13_period(nullptr), 0);
    CHECK_EQ(x13_nobs(nullptr), 0);
    CHECK_EQ(x13_model_based(nullptr), 0);
    CHECK_EQ(x13_mode(nullptr), 0);
    CHECK(x13_arima_model(nullptr) != nullptr);
    CHECK_EQ(x13_table_count(nullptr), 0);
    CHECK(x13_table_name(nullptr, 0) != nullptr);
    CHECK_EQ(x13_table_length(nullptr, "d11"), 0);
    CHECK_EQ(x13_table_start_year(nullptr, "d11"), 0);
    CHECK_EQ(x13_table_start_period(nullptr, "d11"), 0);
    CHECK_EQ(x13_diag_count(nullptr), 0);
    double v = 0.0;
    CHECK_EQ(x13_diag_value(nullptr, "f3.q", &v), 0);
    x13_run_free(nullptr);           // must not crash
}

TEST("null_spec_returns_null") {
    CHECK(x13_run_spec_text(nullptr, "s") == nullptr);
    CHECK(x13_run_spec_file(nullptr) == nullptr);
}

// --- failure paths ---------------------------------------------------------
// A failed run must still hand back a handle: callers gate on x13_ok(), and
// returning NULL would make "failed" indistinguishable from "out of memory".
TEST("missing_file_yields_a_failed_handle") {
    x13_run* r = x13_run_spec_file("no/such/file/anywhere.spc");
    CHECK(r != nullptr);
    CHECK_EQ(x13_ok(r), 0);
    CHECK(std::string(x13_error(r)).find("cannot open") != std::string::npos);
    x13_run_free(r);
}

TEST("garbage_spec_fails_without_throwing") {
    x13_run* r = x13_run_spec_text("not a spec {{{{ ][", "junk");
    CHECK(r != nullptr);
    CHECK_EQ(x13_ok(r), 0);
    CHECK(std::strlen(x13_error(r)) > 0);
    x13_run_free(r);
}

TEST("empty_spec_fails_cleanly") {
    x13_run* r = x13_run_spec_text("", "empty");
    CHECK(r != nullptr);
    CHECK_EQ(x13_ok(r), 0);
    x13_run_free(r);
}

// --- a successful run ------------------------------------------------------
TEST("successful_run_metadata") {
    x13_run* r = x13_run_spec_text(kSpec, "capi");
    CHECK(r != nullptr);
    CHECK_EQ(x13_ok(r), 1);
    CHECK_EQ(std::strlen(x13_error(r)), 0u);
    CHECK_EQ(x13_period(r), 12);
    CHECK_EQ(x13_nobs(r), 96);
    CHECK_EQ(x13_mode(r), 0);              // multiplicative default
    CHECK_EQ(x13_model_based(r), 0);       // no arima{} in kSpec
    CHECK(x13_table_count(r) > 0);
    x13_run_free(r);
}

TEST("tables_are_named_and_findable") {
    x13_run* r = x13_run_spec_text(kSpec, "capi");
    const int n = x13_table_count(r);
    CHECK(n > 0);

    bool sawD11 = false;
    for (int i = 0; i < n; ++i) {
        const char* nm = x13_table_name(r, i);
        CHECK(nm != nullptr && std::strlen(nm) > 0);
        // Every enumerated name must resolve -- enumeration and lookup must not
        // be able to disagree.
        CHECK(x13_table_length(r, nm) > 0);
        if (std::strcmp(nm, "d11") == 0) sawD11 = true;
    }
    CHECK(sawD11);
    x13_run_free(r);
}

TEST("out_of_range_index_is_empty_not_a_crash") {
    x13_run* r = x13_run_spec_text(kSpec, "capi");
    const int n = x13_table_count(r);
    CHECK_EQ(std::strlen(x13_table_name(r, n)), 0u);
    CHECK_EQ(std::strlen(x13_table_name(r, -1)), 0u);
    CHECK_EQ(std::strlen(x13_table_name(r, 1000000)), 0u);
    CHECK_EQ(std::strlen(x13_diag_name(r, -5)), 0u);
    x13_run_free(r);
}

TEST("unknown_table_is_zero_length") {
    x13_run* r = x13_run_spec_text(kSpec, "capi");
    CHECK_EQ(x13_table_length(r, "definitely_not_a_table"), 0);
    CHECK_EQ(x13_table_length(r, ""), 0);
    CHECK_EQ(x13_table_length(r, nullptr), 0);
    double buf[4];
    CHECK_EQ(x13_table_values(r, "definitely_not_a_table", buf, 4), 0);
    x13_run_free(r);
}

// --- the size protocol -----------------------------------------------------
// The header promises: too small -> return -needed and write NOTHING. That is
// what makes "ask, allocate, fill" safe for a caller that cannot catch an
// overflow.
TEST("short_buffer_reports_need_and_writes_nothing") {
    x13_run* r = x13_run_spec_text(kSpec, "capi");
    const int n = x13_table_length(r, "d11");
    CHECK(n > 4);

    std::vector<double> buf(4, -12345.0);
    CHECK_EQ(x13_table_values(r, "d11", buf.data(), 4), -n);
    for (double d : buf) CHECK_EQ(d, -12345.0);

    std::vector<int> yy(4, -7), pp(4, -7);
    CHECK_EQ(x13_table_dates(r, "d11", yy.data(), pp.data(), 4), -n);
    for (int v : yy) CHECK_EQ(v, -7);
    for (int v : pp) CHECK_EQ(v, -7);
    x13_run_free(r);
}

TEST("exact_buffer_fills_completely") {
    x13_run* r = x13_run_spec_text(kSpec, "capi");
    const int n = x13_table_length(r, "d11");
    std::vector<double> vals(static_cast<std::size_t>(n), 0.0);
    CHECK_EQ(x13_table_values(r, "d11", vals.data(), n), n);
    // A seasonally adjusted series of positive data is positive throughout;
    // this catches a buffer that was only partly written.
    for (double v : vals) CHECK(v > 0.0);
    x13_run_free(r);
}

TEST("oversized_buffer_is_accepted") {
    x13_run* r = x13_run_spec_text(kSpec, "capi");
    const int n = x13_table_length(r, "d11");
    std::vector<double> vals(static_cast<std::size_t>(n) + 10, -1.0);
    CHECK_EQ(x13_table_values(r, "d11", vals.data(), n + 10), n);
    CHECK_EQ(vals[static_cast<std::size_t>(n)], -1.0);   // tail untouched
    x13_run_free(r);
}

TEST("null_output_pointers_are_rejected_not_dereferenced") {
    x13_run* r = x13_run_spec_text(kSpec, "capi");
    const int n = x13_table_length(r, "d11");
    CHECK_EQ(x13_table_values(r, "d11", nullptr, n), -n);
    // Dates: one side may be NULL (skip it), both NULL is a no-op.
    std::vector<int> yy(static_cast<std::size_t>(n), 0);
    CHECK_EQ(x13_table_dates(r, "d11", yy.data(), nullptr, n), n);
    CHECK_EQ(x13_table_dates(r, "d11", nullptr, nullptr, n), -n);
    x13_run_free(r);
}

// --- dates -----------------------------------------------------------------
TEST("dates_are_calendar_correct") {
    x13_run* r = x13_run_spec_text(kSpec, "capi");
    const int n = x13_table_length(r, "d11");
    std::vector<int> yy(static_cast<std::size_t>(n)), pp(static_cast<std::size_t>(n));
    CHECK_EQ(x13_table_dates(r, "d11", yy.data(), pp.data(), n), n);

    CHECK_EQ(x13_table_start_year(r, "d11"), 1990);
    CHECK_EQ(x13_table_start_period(r, "d11"), 1);
    CHECK_EQ(yy[0], 1990);
    CHECK_EQ(pp[0], 1);

    // Contiguous, 1-based, rolling over at 12.
    for (int i = 1; i < n; ++i) {
        const int wantY = (pp[static_cast<std::size_t>(i) - 1] == 12)
                              ? yy[static_cast<std::size_t>(i) - 1] + 1
                              : yy[static_cast<std::size_t>(i) - 1];
        const int wantP = (pp[static_cast<std::size_t>(i) - 1] == 12)
                              ? 1
                              : pp[static_cast<std::size_t>(i) - 1] + 1;
        CHECK_EQ(yy[static_cast<std::size_t>(i)], wantY);
        CHECK_EQ(pp[static_cast<std::size_t>(i)], wantP);
    }
    // 96 monthly observations from 1990.01 end at 1997.12.
    CHECK_EQ(yy[static_cast<std::size_t>(n) - 1], 1997);
    CHECK_EQ(pp[static_cast<std::size_t>(n) - 1], 12);
    x13_run_free(r);
}

// d10's seasonal factors are projected past the data while d11 stops with it --
// the reason each table carries its own start rather than sharing one.
TEST("tables_may_have_different_spans") {
    x13_run* r = x13_run_spec_text(kSpec, "capi");
    const int nd10 = x13_table_length(r, "d10");
    const int nd11 = x13_table_length(r, "d11");
    CHECK(nd10 >= nd11);
    CHECK_EQ(x13_table_start_year(r, "d10"), x13_table_start_year(r, "d11"));
    x13_run_free(r);
}

// --- diagnostics -----------------------------------------------------------
TEST("diagnostics_enumerate_and_resolve") {
    x13_run* r = x13_run_spec_text(kSpec, "capi");
    const int n = x13_diag_count(r);
    CHECK(n > 0);
    bool sawQ = false;
    for (int i = 0; i < n; ++i) {
        const char* nm = x13_diag_name(r, i);
        CHECK(nm != nullptr && std::strlen(nm) > 0);
        double v = 0.0;
        CHECK_EQ(x13_diag_value(r, nm, &v), 1);
        CHECK(v == v);                       // not NaN
        if (std::strcmp(nm, "f3.q") == 0) sawQ = true;
    }
    CHECK(sawQ);

    double v = -1.0;
    CHECK_EQ(x13_diag_value(r, "no.such.diagnostic", &v), 0);
    CHECK_EQ(v, -1.0);                            // untouched on miss
    CHECK_EQ(x13_diag_value(r, "f3.q", nullptr), 0);
    x13_run_free(r);
}

// --- independence ----------------------------------------------------------
// The engine is a wall of COMMON blocks. Each handle must own its own copy of
// the results, so a second run cannot disturb a first that is still open.
TEST("runs_are_independent") {
    x13_run* a = x13_run_spec_text(kSpec, "a");
    const int n = x13_table_length(a, "d11");
    std::vector<double> first(static_cast<std::size_t>(n));
    CHECK_EQ(x13_table_values(a, "d11", first.data(), n), n);

    x13_run* b = x13_run_spec_text(kSpec, "b");
    std::vector<double> second(static_cast<std::size_t>(n));
    CHECK_EQ(x13_table_values(b, "d11", second.data(), n), n);
    x13_run_free(b);

    std::vector<double> again(static_cast<std::size_t>(n));
    CHECK_EQ(x13_table_values(a, "d11", again.data(), n), n);
    for (int i = 0; i < n; ++i) {
        CHECK_EQ(first[static_cast<std::size_t>(i)],
                  second[static_cast<std::size_t>(i)]);   // deterministic
        CHECK_EQ(first[static_cast<std::size_t>(i)],
                  again[static_cast<std::size_t>(i)]);    // and undisturbed
    }
    x13_run_free(a);
}

// --- the FP environment ----------------------------------------------------
// A run must leave the host's x87 control word exactly as it found it: a
// library that silently changed the interpreter's FP mode would corrupt the
// caller's own arithmetic. (The run itself deliberately forces PC=3 while it
// works -- see FpPrecisionGuard.)
TEST("run_restores_the_host_fp_control_word") {
    const unsigned before = x13_host_fp_control();
    x13_run* r = x13_run_spec_text(kSpec, "capi");
    CHECK_EQ(x13_ok(r), 1);
    x13_run_free(r);
    CHECK_EQ(x13_host_fp_control(), before);
}

TEST("version_reporting") {
    CHECK_EQ(x13_abi_version(), 1);
    CHECK(std::strlen(x13_engine_version()) > 0);
}

int main() { return mt::run_all(); }
