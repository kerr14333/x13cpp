// x13_rabi.cpp -- a `.C()`-shaped façade over the same engine as capi.h, so R
// can call it with NO R headers and NO compiled R glue.
//
// WHY A SECOND FACADE
// -------------------
// R offers two native interfaces. `.Call` is the good one, but its functions
// take and return `SEXP`, which means including <Rinternals.h> and therefore
// building against an R installation -- a dependency this project does not want
// just to be callable. `.C` needs no R headers at all, but it can only pass
// POINTERS TO BASIC TYPES: int*, double*, char**. It cannot carry an opaque
// handle pointer.
//
// So this file adds exactly two things on top of capi.h:
//   * an integer handle registry, because `.C` can pass an int but not an
//     x13_run*; and
//   * out-parameter-only signatures, because `.C` ignores return values.
//
// Everything else -- running, table extraction, ranges -- is capi.h's, called
// straight through. There is no second copy of the engine logic here.
//
// STRING RETURNS: `.C` copies a character vector out and back, so the caller
// pre-allocates a wide-enough string in R and this side writes into it with a
// hard bound. Callers must pass the capacity; nothing here ever writes past it.
//
// THREAD SAFETY: the registry is guarded by a mutex, but the underlying
// x13_run_spec_file still changes the process working directory for the
// duration of a run, so concurrent runs are unsafe regardless. Documented in
// bindings/r/x13c.R.
#include "x13/capi.h"

#include <cstring>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace {

std::mutex g_mutex;
std::map<int, x13_run*> g_runs;
int g_next = 1;

int store(x13_run* r) {
    std::lock_guard<std::mutex> lock(g_mutex);
    const int id = g_next++;
    g_runs[id] = r;
    return id;
}

// Resolve `id` and run `fn(handle)` with the registry lock STILL HELD.
//
// The previous shape -- a lookup() that returned the raw pointer and released
// the lock before the caller dereferenced it -- was a use-after-free: nothing
// stops x13r_close from erasing and x13_run_free-ing the same id in between.
// Holding the lock across the whole read is the fix; a run is cheap to read
// from and no callee here re-enters the registry, so there is no deadlock and
// no contention worth caring about. `fn` is called with nullptr for an unknown
// or already-closed handle, which is what every accessor below already
// tolerates (the C API treats a NULL run as "not ok / empty").
template <typename F>
auto withRun(int id, F fn) -> decltype(fn(static_cast<const x13_run*>(nullptr))) {
    std::lock_guard<std::mutex> lock(g_mutex);
    auto it = g_runs.find(id);
    return fn(it == g_runs.end() ? nullptr : it->second);
}

// Every X13_CAPI entry point below funnels through this: an exception crossing
// an R `.C()` boundary is undefined behaviour, not an error R can catch. The
// engine proper is already no-throw behind the C API, but std::string
// construction, the std::map insert in store() and putStr's memcpy target are
// all on this side of it.
template <typename F>
void guarded(F fn) {
    try {
        fn();
    } catch (...) {
        // Nothing to report through a void .C() signature; the out-parameters
        // each entry point pre-initialises stay at their "empty/not ok" value.
    }
}

// Copy `src` into the caller's pre-allocated buffer, always NUL-terminated,
// never exceeding `cap` bytes INCLUDING the terminator.
//
// This TRUNCATES, and deliberately does not implement capi.h's size-then-fill
// "return -needed and write nothing" protocol: these are `.C()` entry points
// with a void return and out-parameters only, so there is no channel to report
// the needed length on. The numeric entry points (x13r_table) DO honour the
// protocol, because their length is queried separately via x13r_table_length.
// Callers on the R side therefore size string buffers generously -- see
// .x13_str's `cap` in bindings/r/x13c.R. Truncation is silent; a string
// arriving exactly `cap-1` bytes long is the symptom.
void putStr(char* dst, int cap, const char* src) {
    if (!dst || cap <= 0) return;
    if (!src) src = "";
    const int n = static_cast<int>(std::strlen(src));
    const int k = (n < cap - 1) ? n : cap - 1;
    std::memcpy(dst, src, static_cast<std::size_t>(k));
    dst[k] = '\0';
}

}  // namespace

extern "C" {

// Open a spec FILE. handle <- id (0 on allocation failure); ok <- 1/0.
X13_CAPI void x13r_open_file(char** path, int* handle, int* ok) {
    *handle = 0;
    *ok = 0;
    guarded([&] {
        x13_run* r = x13_run_spec_file(path && path[0] ? path[0] : "");
        if (!r) return;
        // store() can throw (map insert); free the run rather than leak it.
        try {
            *handle = store(r);
        } catch (...) {
            x13_run_free(r);
            throw;
        }
        *ok = x13_ok(r);
    });
}

// Open a spec given as TEXT.
X13_CAPI void x13r_open_text(char** text, char** name, int* handle, int* ok) {
    *handle = 0;
    *ok = 0;
    guarded([&] {
        x13_run* r = x13_run_spec_text(text && text[0] ? text[0] : "",
                                       name && name[0] ? name[0] : "series");
        if (!r) return;
        try {
            *handle = store(r);
        } catch (...) {
            x13_run_free(r);
            throw;
        }
        *ok = x13_ok(r);
    });
}

// The host's x87 control word -- see x13_host_fp_control in capi.h for why this
// is worth being able to read from the interpreter.
X13_CAPI void x13r_host_fp_control(int* out) {
    *out = static_cast<int>(x13_host_fp_control());
}

X13_CAPI void x13r_close(int* handle) {
    x13_run* r = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        auto it = g_runs.find(*handle);
        if (it != g_runs.end()) {
            r = it->second;
            g_runs.erase(it);
        }
    }
    x13_run_free(r);
}

// out[0]=ok out[1]=period out[2]=nobs out[3]=model_based out[4]=mode
// out[5]=table_count out[6]=diag_count
// NOTE on the string entry points: the `const char*` the C API returns aliases
// a std::string owned by the run, so the putStr copy has to happen INSIDE
// withRun's lambda, while the lock is held. Resolving the handle under the lock
// and copying after it is the same use-after-free in a subtler dress.
X13_CAPI void x13r_meta(int* handle, int* out) {
    for (int i = 0; i < 7; ++i) out[i] = 0;
    guarded([&] {
        withRun(*handle, [&](const x13_run* r) {
            out[0] = x13_ok(r);
            out[1] = x13_period(r);
            out[2] = x13_nobs(r);
            out[3] = x13_model_based(r);
            out[4] = x13_mode(r);
            out[5] = x13_table_count(r);
            out[6] = x13_diag_count(r);
            return 0;
        });
    });
}

X13_CAPI void x13r_error(int* handle, char** buf, int* cap) {
    guarded([&] {
        withRun(*handle, [&](const x13_run* r) {
            putStr(buf[0], *cap, x13_error(r));
            return 0;
        });
    });
}

X13_CAPI void x13r_arima_model(int* handle, char** buf, int* cap) {
    guarded([&] {
        withRun(*handle, [&](const x13_run* r) {
            putStr(buf[0], *cap, x13_arima_model(r));
            return 0;
        });
    });
}

X13_CAPI void x13r_engine_version(char** buf, int* cap) {
    guarded([&] { putStr(buf[0], *cap, x13_engine_version()); });
}

X13_CAPI void x13r_table_name(int* handle, int* index, char** buf, int* cap) {
    guarded([&] {
        withRun(*handle, [&](const x13_run* r) {
            putStr(buf[0], *cap, x13_table_name(r, *index));
            return 0;
        });
    });
}

// len <- number of observations in the table (0 if absent).
X13_CAPI void x13r_table_length(int* handle, char** name, int* len) {
    *len = 0;
    guarded([&] {
        withRun(*handle, [&](const x13_run* r) {
            *len = x13_table_length(r, name[0]);
            return 0;
        });
    });
}

// Fill caller-allocated years/periods/values, each of length *n (which must be
// x13r_table_length's answer). n <- what was actually written, 0 on mismatch.
X13_CAPI void x13r_table(int* handle, char** name, int* years, int* periods,
                         double* values, int* n) {
    const int want = *n;
    *n = 0;
    guarded([&] {
        withRun(*handle, [&](const x13_run* r) {
            if (x13_table_values(r, name[0], values, want) != want) return 0;
            if (x13_table_dates(r, name[0], years, periods, want) != want)
                return 0;
            *n = want;
            return 0;
        });
    });
}

X13_CAPI void x13r_diag_name(int* handle, int* index, char** buf, int* cap) {
    guarded([&] {
        withRun(*handle, [&](const x13_run* r) {
            putStr(buf[0], *cap, x13_diag_name(r, *index));
            return 0;
        });
    });
}

X13_CAPI void x13r_diag_value(int* handle, char** name, double* out, int* found) {
    *found = 0;
    guarded([&] {
        withRun(*handle, [&](const x13_run* r) {
            *found = x13_diag_value(r, name[0], out);
            return 0;
        });
    });
}

}  // extern "C"
