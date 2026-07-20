#' Run a seasonal adjustment
#'
#' `seasonal_adjust()` is the main entry point of the package: it runs the
#' X-13ARIMA-SEATS pipeline (regARIMA modeling, X-11 or SEATS decomposition,
#' diagnostics) over an input series and returns an [x13_result] object.
#'
#' @section Status -- core not yet wired in:
#' The C++ estimation engine this function calls into is still mid-port (see
#' the `x13new` repository: regARIMA estimation is implemented, but the X-11
#' and SEATS decomposition paths are not). This function currently always
#' `stop()`s with an explanatory message rather than returning incorrect
#' numbers.
#'
#' TODO(core wiring): once `core/` exposes a stable C ABI / Rcpp-callable
#' entry point for a full run (regARIMA + X-11 or SEATS + diagnostics),
#' replace the `stop()` below with the real call and populate `udg`, `fct`,
#' and `save` on the returned [x13_result]. The rest of this function's
#' contract (argument names, the shape of the returned object, the "never
#' write files automatically" rule) is intended to be final already.
#'
#' @param x A `ts` object (or a numeric vector plus `start`/`frequency`, see
#'   `stats::ts()`) giving the series to seasonally adjust.
#' @param transform Transformation to apply before modeling: `"auto"`
#'   (choose automatically), `"log"`, or `"none"`.
#' @param arima ARIMA order specification. Not yet processed; reserved for
#'   the same syntax as an X-13 `arima{ model = (p d q)(P D Q) }` spec, e.g.
#'   `"(0 1 1)(0 1 1)"`.
#' @param x11 Logical; if `TRUE`, run the X-11 decomposition. Mutually
#'   exclusive with `seats`.
#' @param seats Logical; if `TRUE`, run the SEATS decomposition. Mutually
#'   exclusive with `x11`.
#' @param forecast_periods Number of periods to forecast beyond the end of
#'   the series (0 disables forecasting).
#' @param ... Additional spec options, reserved for future use.
#'
#' @return An [x13_result] object. This function never writes to disk; call
#'   [write_outputs()] on the result explicitly if you want files.
#'
#' @examples
#' \dontrun{
#' # Once the core is wired in:
#' fit <- seasonal_adjust(x13cpp::airline, transform = "log",
#'                         arima = "(0 1 1)(0 1 1)", x11 = TRUE)
#' seasadj(fit)
#' udg(fit)
#' }
#' @export
seasonal_adjust <- function(x,
                             transform = c("auto", "log", "none"),
                             arima = NULL,
                             x11 = FALSE,
                             seats = FALSE,
                             forecast_periods = 0L,
                             ...) {
  transform <- match.arg(transform)
  if (!stats::is.ts(x)) {
    stop("`x` must be a ts object (see stats::ts()).", call. = FALSE)
  }
  if (x11 && seats) {
    stop("Choose one of `x11` or `seats`, not both.", call. = FALSE)
  }

  # TODO(core wiring): call into the C++ core here once it exposes a full
  # regARIMA + X-11/SEATS run. See the docs above for what's already
  # implemented on the C++ side (core/) vs. what's still missing.
  stop(
    "seasonal_adjust() is a stub: the X13cpp C++ core does not yet expose ",
    "a full seasonal-adjustment run (X-11/SEATS decomposition is not ",
    "implemented; regARIMA estimation alone is not sufficient for this ",
    "entry point). This package's API (result object, accessors, ",
    "datasets) is ready; only the core call is pending. See ",
    "https://github.com/kerr14333/x13cpp for status.",
    call. = FALSE
  )
}
