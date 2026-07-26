# x13c.R -- call the X-13ARIMA-SEATS engine from R, in-process, no package.
#
# This is deliberately NOT an R package and needs NO compilation on your side:
# it is one file you source(), which dyn.load()s the `x13c` shared library
# (built by CMake from core/src/api/) and returns ordinary R values.
#
#   source("bindings/r/x13c.R")
#   run <- x13_adjust("tests/corpus/generated/airline_x11-default.spc")
#   run
#   d11 <- x13_table(run, "d11")     # data.frame(year, period, value)
#   plot(as_ts(d11))
#   x13_diagnostics(run)[["f3.q"]]
#   x13_close(run)
#
# Why .C() and not .Call(): .Call is the nicer interface but its functions take
# and return SEXP, so using it would mean compiling R glue against <Rinternals.h>
# -- an R build dependency this project avoids. .C() needs no R headers, at the
# cost of only being able to pass pointers to basic types; the shim in
# core/src/api/x13_rabi.cpp bridges that with an integer handle registry.
#
# Notes that matter:
#  * Nothing is written to disk. The engine's standing rule; this layer keeps it.
#  * Tables do NOT share one start date -- d10 (seasonal factors) is projected a
#    year past the data, backcasts precede it. Each table carries its own
#    year/period columns; merge on those rather than assuming alignment.
#  * x13_adjust() on a PATH is not safe to run in parallel: resolving the spec's
#    relative data paths means changing the process working directory for the
#    duration of the run. Use x13_adjust_text() with absolute paths if needed.
#  * A failed run stop()s with the engine's message. Pass check = FALSE to get
#    the handle back and inspect run$ok / run$error yourself.
#  * Handles are freed by x13_close() or by a finalizer at garbage collection.

.x13c <- new.env(parent = emptyenv())

`%||%` <- function(a, b) if (is.null(a) || !length(a) || !nzchar(a[1])) b else a

.x13_libname <- function() {
  if (.Platform$OS.type == "windows") "x13c.dll"
  else if (Sys.info()[["sysname"]] == "Darwin") "libx13c.dylib"
  else "libx13c.so"
}

#' Where x13_load() will look, in order. Set X13C_LIBRARY to override.
x13_library_candidates <- function() {
  env <- Sys.getenv("X13C_LIBRARY", "")
  if (nzchar(env)) return(env)
  nm <- .x13_libname()
  here <- .x13c$source_dir %||% getwd()
  repo <- normalizePath(file.path(here, "..", ".."), mustWork = FALSE)
  unique(c(file.path(repo, "build", nm),
           file.path(getwd(), "build", nm),
           file.path(here, nm),
           file.path(getwd(), nm)))
}

# Remember where this file lives so the default search works regardless of the
# caller's working directory.
local({
  of <- tryCatch(sys.frame(1)$ofile, error = function(e) NULL)
  if (!is.null(of)) .x13c$source_dir <- dirname(normalizePath(of, mustWork = FALSE))
})

x13_load <- function(path = NULL) {
  if (!is.null(.x13c$dll) && is.null(path)) return(invisible(TRUE))
  cand <- if (is.null(path)) x13_library_candidates() else path
  hit <- cand[file.exists(cand)]
  if (!length(hit)) {
    stop("could not find the x13c shared library. Build it with ",
         "tools/build.ps1 (CMake target `x13c`), or set X13C_LIBRARY to its ",
         "path.\nTried:\n  ", paste(cand, collapse = "\n  "), call. = FALSE)
  }
  .x13c$dll <- dyn.load(hit[1])
  .x13c$path <- hit[1]
  invisible(TRUE)
}

x13_unload <- function() {
  if (!is.null(.x13c$dll)) dyn.unload(.x13c$path)
  .x13c$dll <- NULL
  invisible(NULL)
}

# A pre-allocated character buffer for the .C string-out convention.
.x13_buf <- function(n = 512L) paste(rep(" ", n), collapse = "")

.x13_str <- function(sym, ..., cap = 512L) {
  out <- .C(sym, ..., buf = .x13_buf(cap), cap = as.integer(cap))
  out$buf
}

# --- running ---------------------------------------------------------------

.x13_wrap <- function(handle, ok, check, what) {
  if (handle == 0L) {
    stop("the engine could not allocate a run for ", what, call. = FALSE)
  }
  run <- structure(new.env(parent = emptyenv()), class = "x13_run")
  run$handle <- handle
  run$closed <- FALSE
  # get() rather than `$`: the finalizer runs during GC, where dispatching
  # through the S3 `$.x13_run` method would re-enter this file's code.
  reg.finalizer(run, function(e) {
    if (!isTRUE(get("closed", envir = e, inherits = FALSE)))
      try(.C("x13r_close",
             handle = as.integer(get("handle", envir = e, inherits = FALSE))),
          silent = TRUE)
  }, onexit = TRUE)
  if (check && ok != 1L) {
    msg <- x13_error(run)
    x13_close(run)
    stop(msg, call. = FALSE)
  }
  run
}

#' Run a .spc file. Relative data paths inside it resolve against the spec's own
#' directory, matching the reference driver.
x13_adjust <- function(spec_path, check = TRUE, library = NULL) {
  x13_load(library)
  p <- path.expand(spec_path)
  if (!file.exists(p)) stop("no such spec file: ", spec_path, call. = FALSE)
  r <- .C("x13r_open_file", path = as.character(p),
          handle = integer(1), ok = integer(1))
  .x13_wrap(r$handle, r$ok, check, sQuote(spec_path))
}

#' Run a spec supplied as text (a character scalar, or a vector joined with
#' newlines). Relative data paths inside it resolve against getwd(), so prefer
#' absolute paths here.
x13_adjust_text <- function(spec_text, series_name = "series", check = TRUE,
                            library = NULL) {
  x13_load(library)
  r <- .C("x13r_open_text",
          text = paste(spec_text, collapse = "\n"),
          name = as.character(series_name),
          handle = integer(1), ok = integer(1))
  .x13_wrap(r$handle, r$ok, check, "the supplied spec text")
}

# A run is an ENVIRONMENT carrying a class, so `$` is overloaded below and
# unclass() does not work on it (R refuses to unclass an environment). Reach the
# raw fields with get(), which ignores the class entirely and so cannot recurse
# back into `$.x13_run`.
.x13_field <- function(run, name) get(name, envir = run, inherits = FALSE)

x13_close <- function(run) {
  if (!isTRUE(.x13_field(run, "closed"))) {
    .C("x13r_close", handle = as.integer(.x13_field(run, "handle")))
    assign("closed", TRUE, envir = run)
  }
  invisible(NULL)
}

.x13_check <- function(run) {
  if (isTRUE(.x13_field(run, "closed")))
    stop("this x13 run has been closed", call. = FALSE)
  as.integer(.x13_field(run, "handle"))
}

# --- status / metadata -----------------------------------------------------

.x13_meta <- function(run) {
  m <- .C("x13r_meta", handle = .x13_check(run), out = integer(7))$out
  list(ok = m[1] == 1L, period = m[2], nobs = m[3],
       model_based = m[4] == 1L, mode = m[5],
       n_tables = m[6], n_diagnostics = m[7])
}

X13_MODES <- c("multiplicative", "additive", "log-additive", "pseudo-additive")

x13_ok    <- function(run) .x13_meta(run)$ok
# cap is generous here on purpose: the C shim's putStr TRUNCATES silently (a
# void .C() signature cannot report a needed length -- see x13_rabi.cpp), and an
# engine diagnostic is the one string with no bounded length.
x13_error <- function(run)
  .x13_str("x13r_error", handle = .x13_check(run), cap = 4096L)

x13_engine_version <- function() {
  x13_load()
  .x13_str("x13r_engine_version", cap = 128L)
}

#' @export
`$.x13_run` <- function(x, name) {
  if (name %in% c("handle", "closed")) return(.x13_field(x, name))
  if (name == "ok") return(x13_ok(x))
  if (name == "error") return(x13_error(x))
  if (name == "arima_model") {
    return(.x13_str("x13r_arima_model", handle = .x13_check(x), cap = 128L))
  }
  m <- .x13_meta(x)
  if (name == "mode") return(X13_MODES[m$mode + 1L])
  if (name %in% names(m)) return(m[[name]])
  NULL
}

#' @export
print.x13_run <- function(x, ...) {
  if (isTRUE(.x13_field(x, "closed"))) {
    cat("<x13 run: closed>\n")
    return(invisible(x))
  }
  if (!x13_ok(x)) {
    cat("<x13 run FAILED: ", x13_error(x), ">\n", sep = "")
    return(invisible(x))
  }
  m <- .x13_meta(x)
  mdl <- x$arima_model
  cat(sprintf("<x13 run: %d obs, period=%d, mode=%s, model=%s, %d tables>\n",
              m$nobs, m$period, X13_MODES[m$mode + 1L],
              if (nzchar(mdl)) mdl else "none", m$n_tables))
  invisible(x)
}

# --- tables ----------------------------------------------------------------

x13_table_names <- function(run) {
  h <- .x13_check(run)
  n <- .x13_meta(run)$n_tables
  if (n == 0L) return(character(0))
  vapply(seq_len(n) - 1L, function(i) {
    .x13_str("x13r_table_name", handle = h, index = as.integer(i), cap = 64L)
  }, character(1))
}

x13_has_table <- function(run, name) {
  .C("x13r_table_length", handle = .x13_check(run),
     name = as.character(name), len = integer(1))$len > 0L
}

#' One output table as data.frame(year, period, value).
x13_table <- function(run, name) {
  h <- .x13_check(run)
  n <- .C("x13r_table_length", handle = h, name = as.character(name),
          len = integer(1))$len
  if (n <= 0L) {
    stop("this run produced no table '", name, "' (available: ",
         paste(x13_table_names(run), collapse = ", "), ")", call. = FALSE)
  }
  r <- .C("x13r_table", handle = h, name = as.character(name),
          years = integer(n), periods = integer(n), values = double(n),
          n = as.integer(n))
  if (r$n != n) stop("table '", name, "': engine returned ", r$n, " of ", n,
                     " observations", call. = FALSE)
  data.frame(year = r$years, period = r$periods, value = r$values)
}

#' Every table (or just `names`) as a named list of data.frames.
x13_tables <- function(run, names = NULL) {
  nm <- if (is.null(names)) x13_table_names(run) else names
  stats::setNames(lapply(nm, function(n) x13_table(run, n)), nm)
}

#' Convert an x13_table data.frame to a base R `ts`.
as_ts <- function(tbl, frequency = NULL) {
  f <- if (is.null(frequency)) (if (max(tbl$period) > 4L) 12L else 4L) else frequency
  stats::ts(tbl$value, start = c(tbl$year[1], tbl$period[1]), frequency = f)
}

# --- diagnostics -----------------------------------------------------------

#' The scalar quality statistics as a named numeric vector
#' (f3.q, f3.m01..m11, f2.ic, ...).
x13_diagnostics <- function(run) {
  h <- .x13_check(run)
  n <- .x13_meta(run)$n_diagnostics
  if (n == 0L) return(stats::setNames(numeric(0), character(0)))
  nms <- vapply(seq_len(n) - 1L, function(i) {
    .x13_str("x13r_diag_name", handle = h, index = as.integer(i), cap = 64L)
  }, character(1))
  vals <- vapply(nms, function(nm) {
    r <- .C("x13r_diag_value", handle = h, name = as.character(nm),
            out = double(1), found = integer(1))
    if (r$found == 1L) r$out else NA_real_
  }, numeric(1))
  stats::setNames(vals, nms)
}
