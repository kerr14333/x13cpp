/* x13/capi.h -- the flat C ABI over the X-13ARIMA-SEATS engine.
 *
 * WHY THIS EXISTS
 * ---------------
 * The C++ facade (`x13/api/x13.hpp`) is pleasant to call from C++ and useless
 * from anywhere else: it hands back `std::string`, `std::vector`, and throws
 * exceptions, none of which cross a language boundary. This header is the same
 * engine behind a boundary that R (`dyn.load` + `.C`) and Python (`ctypes` /
 * `cffi`) can both call in-process, with no build step on the caller's side and
 * no packaging.
 *
 * The two shipped loaders are `bindings/python/x13c.py` and
 * `bindings/r/x13c.R`. Neither is a package; both just open the shared library.
 *
 * DESIGN
 * ------
 *  * One opaque handle per run (`x13_run`). It owns its own engine context, so
 *    runs are independent; the engine's COMMON-block state does not leak
 *    between them.
 *  * Tables are reached BY NAME, not by one accessor per table. The engine
 *    produces dozens of tables and grows more as the port advances; enumerating
 *    them in the ABI would mean breaking it every time. `x13_table_count` /
 *    `x13_table_name` let a caller discover what a given run actually produced.
 *  * Buffers are caller-allocated. Every getter takes a capacity and returns the
 *    number of elements it wrote (or the negative of the number it needed), so a
 *    caller can size-then-fill without the ABI owning any memory the caller must
 *    remember to free. The only thing to free is the handle.
 *  * NOTHING IS WRITTEN TO DISK. This is the project's standing rule and it is
 *    enforced here: a run produces values on the handle, never files.
 *  * No exception may cross this boundary -- every entry point catches
 *    everything and reports through `x13_ok` / `x13_error`.
 *
 * ERRORS
 * ------
 * A run that fails still returns a non-NULL handle: call `x13_ok()` (0 = failed)
 * and `x13_error()` for the message, then free it as usual. Only an allocation
 * failure returns NULL. Passing NULL as a handle is safe everywhere and yields
 * the zero/empty answer for the accessor in question.
 *
 * DATES
 * -----
 * Tables are calendar-anchored but do not all share one start date -- the
 * seasonal factors are projected past the end of the data, backcasts run before
 * it. So each table carries its OWN start (`x13_table_start_year` /
 * `_start_period`) and `x13_table_dates` fills the full year/period vectors.
 * Periods are 1-based (January == 1, Q1 == 1).
 */
#ifndef X13_CAPI_H
#define X13_CAPI_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && defined(X13_CAPI_BUILD_SHARED)
#define X13_CAPI __declspec(dllexport)
#elif defined(_WIN32) && defined(X13_CAPI_USE_SHARED)
#define X13_CAPI __declspec(dllimport)
#else
#define X13_CAPI
#endif

/* Opaque handle to one completed (or failed) engine run. */
typedef struct x13_run x13_run;

/* --- library metadata ---------------------------------------------------- */

/* Version of the C ABI itself. Bumped only on incompatible changes. */
X13_CAPI int x13_abi_version(void);

/* The engine's own version string (the ported Census release). */
X13_CAPI const char* x13_engine_version(void);

/* The HOST process's x87 control word, as this library found it. Diagnostic.
 *
 * It is exported because a host that starts the x87 unit at 53-bit precision
 * (PC=2, control word 0x027f -- CPython does this; MinGW executables and R do
 * not, both being 0x037f) changes the last bits of every long-double
 * intermediate in the engine, which on a near-non-invertible model amplifies to
 * a visible difference (measured 8.2e-6 in d10 on unrate_sfshort-x11).
 *
 * Each run therefore forces PC=3 for its duration and restores the host's value
 * afterwards -- so this reports the HOST's setting, not the one runs use, and a
 * value of 0x027f here is informative rather than a problem. If a future host
 * shows a difference this does not explain, compare MXCSR too. */
X13_CAPI unsigned x13_host_fp_control(void);

/* --- running ------------------------------------------------------------- */

/* Run a spec given as text. `series_name` labels the run (may be NULL -> the
 * default "series"). Relative data-file paths inside the spec resolve against
 * the current working directory; prefer x13_run_spec_file when the spec is on
 * disk. Returns NULL only on allocation failure. */
X13_CAPI x13_run* x13_run_spec_text(const char* spec_text, const char* series_name);

/* Run a spec file. Relative data paths inside it resolve against the spec's own
 * directory, matching the reference driver. Returns NULL only on allocation
 * failure. */
X13_CAPI x13_run* x13_run_spec_file(const char* spec_path);

/* Release a handle. Safe on NULL. */
X13_CAPI void x13_run_free(x13_run* run);

/* --- status -------------------------------------------------------------- */

/* 1 if the run completed, 0 if it failed (or `run` is NULL). */
X13_CAPI int x13_ok(const x13_run* run);

/* The failure message, or "" when the run succeeded. Owned by the handle;
 * valid until x13_run_free. Never NULL. */
X13_CAPI const char* x13_error(const x13_run* run);

/* --- run metadata -------------------------------------------------------- */

/* Observations per year: 12 monthly, 4 quarterly. 0 if unknown. */
X13_CAPI int x13_period(const x13_run* run);

/* Number of observations in the adjusted span. */
X13_CAPI int x13_nobs(const x13_run* run);

/* 1 if a regARIMA model drove the run, 0 for a plain X-11 run. */
X13_CAPI int x13_model_based(const x13_run* run);

/* Decomposition mode: 0 multiplicative, 1 additive, 2 log-additive,
 * 3 pseudo-additive. */
X13_CAPI int x13_mode(const x13_run* run);

/* The identified/specified ARIMA model, e.g. "(0 1 1)(0 1 1)". "" if none.
 * Owned by the handle. Never NULL. */
X13_CAPI const char* x13_arima_model(const x13_run* run);

/* --- tables -------------------------------------------------------------- */

/* How many tables this run produced. */
X13_CAPI int x13_table_count(const x13_run* run);

/* Name of table `index` (0-based), e.g. "d11". "" if out of range. Owned by the
 * handle. Never NULL. */
X13_CAPI const char* x13_table_name(const x13_run* run, int index);

/* Number of observations in `name`, or 0 if this run has no such table. Use
 * this to test presence as well as to size a buffer. */
X13_CAPI int x13_table_length(const x13_run* run, const char* name);

/* Calendar start of `name`. 0 if there is no such table. Tables do NOT all share
 * a start date -- seasonal factors extend past the data, backcasts precede it. */
X13_CAPI int x13_table_start_year(const x13_run* run, const char* name);
X13_CAPI int x13_table_start_period(const x13_run* run, const char* name);

/* Copy `name`'s values into `out` (capacity `capacity`). Returns the number
 * written, 0 if there is no such table, or -needed if `capacity` is too small
 * (nothing is written in that case). */
X13_CAPI int x13_table_values(const x13_run* run, const char* name, double* out,
                              int capacity);

/* Fill the per-observation calendar labels for `name`. Either output pointer may
 * be NULL to skip it. Same return convention as x13_table_values. */
X13_CAPI int x13_table_dates(const x13_run* run, const char* name, int* years,
                             int* periods, int capacity);

/* --- scalar diagnostics -------------------------------------------------- */

/* The named scalar diagnostics this run produced (the X-11 F2/F3 quality
 * statistics: "f3.q", "f3.m01".."f3.m11", "f2.ic", ...). Same name-keyed shape
 * as tables. */
X13_CAPI int x13_diag_count(const x13_run* run);
X13_CAPI const char* x13_diag_name(const x13_run* run, int index);

/* Fetch one diagnostic. Returns 1 and writes *out on success, 0 if there is no
 * such diagnostic (*out untouched). */
X13_CAPI int x13_diag_value(const x13_run* run, const char* name, double* out);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* X13_CAPI_H */
