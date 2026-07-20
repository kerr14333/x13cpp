#' X13cpp result object
#'
#' `seasonal_adjust()` returns an object of S3 class `x13_result`. It is a
#' plain list with a fixed set of named elements, described below, plus the
#' accessor functions on this page for reading them out. Nothing about a
#' result object writes to disk on its own -- see [write_outputs()].
#'
#' @section Structure:
#' \describe{
#'   \item{`call`}{The matched call that produced the result.}
#'   \item{`series`}{The original input series (a `ts` object).}
#'   \item{`spec`}{The resolved spec list used for the run (mirrors an X-13
#'     `.spc` file structure).}
#'   \item{`udg`}{A named list of scalar diagnostics, mirroring the `.udg`
#'     (user diagnostics) file X-13 writes -- e.g. `f3.m01` (M1 quality
#'     statistic), `nobs`, `aicc`, and so on. Empty until the core is wired
#'     in.}
#'   \item{`fct`}{A forecast table: a `ts`/`data.frame`-like object with the
#'     forecast values, standard errors, and confidence bounds. `NULL`
#'     until the core is wired in or if the run did not request forecasts.}
#'   \item{`save`}{A named list of "save tables", keyed by the X-13 table
#'     code (`"d10"`, `"d11"`, `"d12"`, `"d13"`, `"s10"`, `"s11"`, ...),
#'     each a `ts` object. Empty until the core is wired in.}
#'   \item{`status`}{One of `"stub"` (core not yet wired in) or `"ok"`.}
#' }
#'
#' @param x An `x13_result` object.
#' @param table A save-table code, e.g. `"d11"` for the seasonally adjusted
#'   series, `"d12"` for the trend, `"d13"` for the irregular. See the X-13
#'   documentation for the full table code list.
#' @param ... Passed on to methods; currently unused.
#' @param call The matched call that produced the result (see "Structure").
#' @param series The original input series, a `ts` object (see "Structure").
#' @param spec The resolved spec list for the run (see "Structure").
#' @param udg A named list of scalar diagnostics (see "Structure").
#' @param fct A forecast table, or `NULL` (see "Structure").
#' @param save A named list of save tables (see "Structure").
#' @param status Either `"stub"` or `"ok"` (see "Structure").
#'
#' @name x13_result
NULL

#' @rdname x13_result
#' @export
new_x13_result <- function(call = sys.call(-1),
                            series = NULL,
                            spec = list(),
                            udg = list(),
                            fct = NULL,
                            save = list(),
                            status = c("stub", "ok")) {
  status <- match.arg(status)
  structure(
    list(
      call = call,
      series = series,
      spec = spec,
      udg = udg,
      fct = fct,
      save = save,
      status = status
    ),
    class = "x13_result"
  )
}

#' @rdname x13_result
#' @export
udg <- function(x, ...) UseMethod("udg")

#' @export
udg.x13_result <- function(x, ...) x$udg

#' @rdname x13_result
#' @export
fct <- function(x, ...) UseMethod("fct")

#' @export
fct.x13_result <- function(x, ...) x$fct

#' @rdname x13_result
#' @export
save_table <- function(x, table, ...) UseMethod("save_table")

#' @export
save_table.x13_result <- function(x, table, ...) {
  if (!table %in% names(x$save)) {
    stop(
      "Save table \"", table, "\" is not present in this result. ",
      "Available tables: ",
      if (length(x$save)) paste(names(x$save), collapse = ", ") else "(none)",
      call. = FALSE
    )
  }
  x$save[[table]]
}

#' @describeIn x13_result Convenience wrapper for the seasonally adjusted
#'   series (save table `"d11"`).
#' @export
seasadj <- function(x, ...) UseMethod("seasadj")

#' @export
seasadj.x13_result <- function(x, ...) save_table(x, "d11")

#' @describeIn x13_result Convenience wrapper for the trend component (save
#'   table `"d12"`).
#' @export
trend <- function(x, ...) UseMethod("trend")

#' @export
trend.x13_result <- function(x, ...) save_table(x, "d12")

#' @describeIn x13_result Convenience wrapper for the irregular component
#'   (save table `"d13"`).
#' @export
irregular <- function(x, ...) UseMethod("irregular")

#' @export
irregular.x13_result <- function(x, ...) save_table(x, "d13")

#' @export
print.x13_result <- function(x, ...) {
  cat("<x13_result>", if (identical(x$status, "stub")) "[STUB - core not wired in]" else "", "\n")
  cat("  series class:", if (is.null(x$series)) "(none)" else paste(class(x$series), collapse = "/"), "\n")
  n_obs <- if (is.null(x$series)) 0L else length(x$series)
  cat("  observations:", n_obs, "\n")
  cat("  udg entries:", length(x$udg), "\n")
  cat("  forecasts:", if (is.null(x$fct)) "(none)" else paste(NROW(x$fct), "periods"), "\n")
  cat("  save tables:", if (length(x$save)) paste(names(x$save), collapse = ", ") else "(none)", "\n")
  invisible(x)
}

#' @export
summary.x13_result <- function(object, ...) {
  print(object, ...)
  if (length(object$udg)) {
    cat("\nDiagnostics (udg):\n")
    print(utils::head(unlist(object$udg), 20L))
  }
  invisible(object)
}
