#' x13cpp: Seasonal Adjustment with the X13cpp Engine
#'
#' An R interface to X13cpp, a C++17 port of the U.S. Census Bureau's
#' X-13ARIMA-SEATS seasonal adjustment program. See [seasonal_adjust()] for
#' the main entry point and [write_outputs()] for the one, explicit way to
#' persist results to disk.
#'
#' @section Design rules:
#' * **No automatic file output.** Running [seasonal_adjust()] never writes
#'   anything to disk. Every result lives on the returned `x13_result`
#'   object; call [write_outputs()] yourself when you want files.
#' * **Bundled example series.** A handful of named datasets ship with the
#'   package for experimentation without needing your own data -- see
#'   `?x13cpp_datasets`.
#'
#' @section Status:
#' The C++ estimation core this package wraps is still mid-port (see the
#' parent repository, `x13new`). [seasonal_adjust()] currently stops with an
#' informative error rather than silently returning wrong numbers; the
#' surrounding API (result object, accessors, dataset bundling) is otherwise
#' final and ready for the core to be wired in.
#'
#' @keywords internal
"_PACKAGE"
