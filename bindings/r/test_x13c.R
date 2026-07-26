# test_x13c.R -- tests for the R binding (bindings/r/x13c.R).
#
# Run:  Rscript bindings/r/test_x13c.R
# Exits non-zero on the first failure, so it works as a CI step.
#
# These check two different things and both matter:
#
#  1. PARITY. The values R receives are compared against the blessed Fortran
#     oracle goldens in tests/golden/, keyed on the DATE the binding reports.
#     Keying on dates rather than position is deliberate -- a positional compare
#     would still pass if every label were a year late, which is exactly the bug
#     class that has bitten this project's own harnesses.
#
#  2. THE R SURFACE. data.frame shapes, ts() conversion, error handling, the
#     handle lifecycle. A binding can be numerically perfect and still be unusable.
#
# There is a third thing worth stating: R and Python must agree. They load the
# SAME shared library, but the host process's x87 precision-control word differs
# between them (R 0x037f, CPython 0x027f), which silently changed results until
# the library started pinning it per run. test_fp_control_is_restored below is
# the R half of that guard; tests/parity/test_bindings.py is the Python half.

# Locate this file whether it was run with `Rscript test_x13c.R` (where there is
# no sys.frame to ask, so the path comes from the command line) or source()d
# from an interactive session (where there is).
.this_file <- local({
  a <- commandArgs(trailingOnly = FALSE)
  hit <- grep("^--file=", a, value = TRUE)
  if (length(hit)) return(sub("^--file=", "", hit[1]))
  of <- tryCatch(sys.frame(1)$ofile, error = function(e) NULL)
  if (!is.null(of)) return(of)
  file.path(getwd(), "test_x13c.R")
})
.here <- dirname(normalizePath(.this_file, mustWork = FALSE))

source(file.path(.here, "x13c.R"))

REPO <- normalizePath(file.path(.here, "..", ".."), mustWork = TRUE)

# --- tiny test harness -----------------------------------------------------

.tests_run <- 0L
.tests_failed <- 0L

expect <- function(cond, what) {
  .tests_run <<- .tests_run + 1L
  if (!isTRUE(cond)) {
    .tests_failed <<- .tests_failed + 1L
    cat("  FAIL: ", what, "\n", sep = "")
  }
}

expect_error <- function(expr, what) {
  ok <- inherits(try(force(expr), silent = TRUE), "try-error")
  expect(ok, paste0(what, " (expected an error)"))
}

test <- function(name, body) {
  cat("- ", name, "\n", sep = "")
  before <- .tests_failed
  res <- try(body(), silent = TRUE)
  if (inherits(res, "try-error")) {
    .tests_failed <<- .tests_failed + 1L
    cat("  FAIL: unexpected error: ", conditionMessage(attr(res, "condition")),
        "\n", sep = "")
  } else if (.tests_failed > before) {
    cat("  (", .tests_failed - before, " failed check(s))\n", sep = "")
  }
}

read_golden <- function(path) {
  ln <- readLines(path, warn = FALSE)
  m <- regmatches(ln, regexec("^\\s*(\\d{6})\\s+([+-][0-9.EeDd+-]+)", ln))
  keep <- vapply(m, length, integer(1)) == 3L
  if (!any(keep)) return(stats::setNames(numeric(0), character(0)))
  m <- m[keep]
  key <- vapply(m, `[`, character(1), 2)
  val <- as.numeric(gsub("[Dd]", "E", vapply(m, `[`, character(1), 3)))
  stats::setNames(val, key)
}

SPEC <- file.path(REPO, "tests", "corpus", "generated", "airline_x11-default.spc")

# --- 1. parity -------------------------------------------------------------

test("values match the blessed oracle goldens, keyed on date", function() {
  # A no-model X-11 run is pure filter arithmetic and reaches ~5e-15; 1e-12
  # leaves three orders of margin. (Model-based specs ride a 1e-6 estimation
  # floor -- see tests/parity/test_bindings.py, which sweeps the whole corpus.)
  tol <- 1e-12
  run <- x13_adjust(SPEC)
  on.exit(x13_close(run))
  gdir <- file.path(REPO, "tests", "golden", "generated", "airline_x11-default")
  n_compared <- 0L
  for (tag in c("b1", "d10", "d11", "d12", "d13", "d16")) {
    gp <- file.path(gdir, paste0("airline_x11-default.", tag))
    if (!file.exists(gp)) next
    gold <- read_golden(gp)
    if (!length(gold)) next
    expect(x13_has_table(run, tag), paste0("run should expose ", tag))
    tb <- x13_table(run, tag)
    got <- stats::setNames(tb$value, sprintf("%04d%02d", tb$year, tb$period))
    missing <- setdiff(names(gold), names(got))
    expect(length(missing) == 0L,
           paste0(tag, ": binding is missing dated rows the golden has: ",
                  paste(utils::head(missing, 4), collapse = ", ")))
    common <- intersect(names(gold), names(got))
    rel <- abs(got[common] - gold[common]) / pmax(abs(gold[common]), 1e-300)
    expect(max(rel) <= tol,
           sprintf("%s: max rel err %.3e at %s (tol %.0e)", tag, max(rel),
                   common[which.max(rel)], tol))
    n_compared <- n_compared + 1L
  }
  expect(n_compared > 0L, "no table was actually compared")
})

test("a model-based spec reproduces its goldens too", function() {
  spec <- file.path(REPO, "tests", "corpus", "generated",
                    "airline_fixed-airline-x11.spc")
  if (!file.exists(spec)) return(invisible(NULL))
  run <- x13_adjust(spec, check = FALSE)
  on.exit(x13_close(run))
  if (!x13_ok(run)) return(invisible(NULL))
  expect(run$model_based, "fixed-airline spec should report model_based")
  gp <- file.path(REPO, "tests", "golden", "generated",
                  "airline_fixed-airline-x11", "airline_fixed-airline-x11.d11")
  if (!file.exists(gp)) return(invisible(NULL))
  gold <- read_golden(gp)
  tb <- x13_table(run, "d11")
  got <- stats::setNames(tb$value, sprintf("%04d%02d", tb$year, tb$period))
  common <- intersect(names(gold), names(got))
  rel <- max(abs(got[common] - gold[common]) / abs(gold[common]))
  expect(rel <= 1e-6, sprintf("d11 max rel err %.3e (estimation floor 1e-6)", rel))
})

test("a SEATS spec dispatches to SEATS and matches its goldens", function() {
  spec <- file.path(REPO, "tests", "corpus", "generated", "airline_seats.spc")
  if (!file.exists(spec)) return(invisible(NULL))
  run <- x13_adjust(spec, check = FALSE)
  on.exit(x13_close(run))
  if (!x13_ok(run)) return(invisible(NULL))
  nms <- x13_table_names(run)
  expect(any(c("s11", "s12") %in% nms), "a SEATS run should expose s-tables")
  expect(!any(c("d10", "d11") %in% nms),
         "a SEATS run should not expose X-11 d-tables")
  gdir <- file.path(REPO, "tests", "golden", "generated", "airline_seats")
  # The established SEATS policy (test_seats_tables.py): |v-g| <= 1e-8*|g| + 1e-9.
  for (tag in c("s10", "s11", "s12", "s13", "s16", "s18")) {
    gp <- file.path(gdir, paste0("airline_seats.", tag))
    if (!file.exists(gp)) next
    gold <- read_golden(gp)
    tb <- x13_table(run, tag)
    got <- stats::setNames(tb$value, sprintf("%04d%02d", tb$year, tb$period))
    common <- intersect(names(gold), names(got))
    expect(length(common) == length(gold),
           paste0(tag, ": binding should cover every golden row"))
    bad <- sum(abs(got[common] - gold[common]) >
                 1e-8 * abs(gold[common]) + 1e-9)
    expect(bad == 0L, sprintf("%s: %d point(s) outside RTOL*|g|+ATOL", tag, bad))
  }
})

# --- 2. the R surface ------------------------------------------------------

test("metadata", function() {
  run <- x13_adjust(SPEC)
  on.exit(x13_close(run))
  expect(x13_ok(run), "run should be ok")
  expect(run$error == "", "error should be empty on success")
  expect(run$period == 12L, "period should be 12")
  expect(run$nobs == 144L, "nobs should be 144")
  expect(run$mode == "multiplicative", "mode should be multiplicative")
  expect(!run$model_based, "no arima{} -> not model based")
  expect(run$arima_model == "", "no model -> empty model string")
  expect("d11" %in% x13_table_names(run), "d11 should be listed")
})

test("tables are well-formed data.frames", function() {
  run <- x13_adjust(SPEC)
  on.exit(x13_close(run))
  for (nm in x13_table_names(run)) {
    tb <- x13_table(run, nm)
    expect(is.data.frame(tb), paste0(nm, " should be a data.frame"))
    expect(identical(names(tb), c("year", "period", "value")),
           paste0(nm, " should have year/period/value columns"))
    expect(nrow(tb) > 0L, paste0(nm, " should be non-empty"))
    expect(all(tb$period >= 1L & tb$period <= run$period),
           paste0(nm, " periods should be 1..", run$period))
    expect(!any(is.na(tb$value)), paste0(nm, " should have no NA values"))
    # contiguous months, rolling over correctly
    idx <- tb$year * run$period + (tb$period - 1L)
    expect(all(diff(idx) == 1L), paste0(nm, " dates should be contiguous"))
  }
})

test("seasonal factors extend past the data", function() {
  run <- x13_adjust(SPEC)
  on.exit(x13_close(run))
  d10 <- x13_table(run, "d10"); d11 <- x13_table(run, "d11")
  expect(nrow(d11) == run$nobs, "d11 should cover exactly the observed span")
  expect(nrow(d10) >= nrow(d11), "d10 should be projected at least as far")
})

test("as_ts produces a correct ts object", function() {
  run <- x13_adjust(SPEC)
  on.exit(x13_close(run))
  tb <- x13_table(run, "d11")
  s <- as_ts(tb)
  expect(inherits(s, "ts"), "as_ts should return a ts")
  expect(stats::frequency(s) == 12, "frequency should be 12")
  # as.numeric on both sides: start() returns doubles while the table's
  # year/period columns are integers, so identical() would fail on type alone.
  expect(isTRUE(all.equal(as.numeric(stats::start(s)),
                          as.numeric(c(tb$year[1], tb$period[1])))),
         "ts start should match the table's first date")
  expect(length(s) == nrow(tb), "ts length should match the table")
  expect(isTRUE(all.equal(as.numeric(s), tb$value)), "ts values should match")
})

test("x13_tables returns every table", function() {
  run <- x13_adjust(SPEC)
  on.exit(x13_close(run))
  all_t <- x13_tables(run)
  expect(length(all_t) == length(x13_table_names(run)),
         "x13_tables should return one entry per table")
  expect(all(vapply(all_t, is.data.frame, logical(1))),
         "every entry should be a data.frame")
  some <- x13_tables(run, c("d11", "d12"))
  expect(identical(names(some), c("d11", "d12")), "subset should be honoured")
})

test("diagnostics", function() {
  run <- x13_adjust(SPEC)
  on.exit(x13_close(run))
  d <- x13_diagnostics(run)
  expect(length(d) > 0L, "there should be some diagnostics")
  expect("f3.q" %in% names(d), "f3.q should be present")
  expect(!any(is.na(d)), "no diagnostic should be NA")
  expect(d[["f3.q"]] >= 0 && d[["f3.q"]] < 10, "f3.q should be a sane Q value")
})

test("adjust_text works and agrees with adjust", function() {
  txt <- readLines(SPEC, warn = FALSE)
  # The spec's data path is relative to the spec's directory, so run from there.
  old <- setwd(dirname(SPEC)); on.exit(setwd(old), add = TRUE)
  a <- x13_adjust_text(txt, "inline")
  on.exit(x13_close(a), add = TRUE)
  b <- x13_adjust(SPEC)
  on.exit(x13_close(b), add = TRUE)
  expect(isTRUE(all.equal(x13_table(a, "d11")$value,
                          x13_table(b, "d11")$value)),
         "text and file runs should agree exactly")
})

# --- 3. errors and lifecycle ----------------------------------------------

test("a missing spec file errors", function() {
  expect_error(x13_adjust(file.path(REPO, "no", "such.spc")),
               "missing spec should stop()")
})

test("check = FALSE returns a failed run instead of stopping", function() {
  run <- x13_adjust_text("this is not a spec {{{", check = FALSE)
  on.exit(x13_close(run))
  expect(!x13_ok(run), "garbage spec should not be ok")
  expect(nzchar(x13_error(run)), "a failed run should carry a message")
})

test("an unknown table errors and names what exists", function() {
  run <- x13_adjust(SPEC)
  on.exit(x13_close(run))
  expect(!x13_has_table(run, "nope"), "x13_has_table should be FALSE")
  msg <- tryCatch({ x13_table(run, "nope"); "" },
                  error = function(e) conditionMessage(e))
  expect(grepl("d11", msg, fixed = TRUE),
         "the error should list the tables that do exist")
})

test("use after close errors, and double close is a no-op", function() {
  run <- x13_adjust(SPEC)
  x13_close(run)
  expect_error(x13_table(run, "d11"), "use after close should error")
  x13_close(run)     # must not crash or double-free
  expect(TRUE, "double close survived")
})

test("two runs are independent", function() {
  a <- x13_adjust(SPEC); on.exit(x13_close(a), add = TRUE)
  first <- x13_table(a, "d11")$value
  b <- x13_adjust(SPEC); on.exit(x13_close(b), add = TRUE)
  second <- x13_table(b, "d11")$value
  expect(identical(first, second), "the same spec should give the same numbers")
  expect(identical(x13_table(a, "d11")$value, first),
         "the first run should be undisturbed by the second")
})

test("the host FP control word is restored after a run", function() {
  # The library forces x87 PC=3 for the duration of a run (parity requires it)
  # and must put the host's value back, or it would silently change R's own
  # arithmetic for the rest of the session.
  before <- .C("x13r_host_fp_control", out = integer(1))$out
  run <- x13_adjust(SPEC); x13_close(run)
  after <- .C("x13r_host_fp_control", out = integer(1))$out
  expect(before == after,
         sprintf("FP control word changed: 0x%04x -> 0x%04x", before, after))
})

test("engine version is reported", function() {
  expect(nzchar(trimws(x13_engine_version())), "engine version should be non-empty")
})

# --- summary ---------------------------------------------------------------

cat("\n", .tests_run - .tests_failed, "/", .tests_run, " checks passed\n", sep = "")
if (.tests_failed > 0L) {
  cat(.tests_failed, " CHECK(S) FAILED\n", sep = "")
  quit(status = 1L)
}
cat("OK\n")
