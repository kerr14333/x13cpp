#' Bundled example series
#'
#' Four public-domain economic time series, bundled as `ts` objects for
#' experimenting with [seasonal_adjust()] without needing your own data.
#' These are the same frozen series used as the base corpus for this
#' project's Fortran-vs-C++ parity tests (`tests/corpus/data/` in the
#' `x13new` source repository) -- see the `@source` note on each dataset for
#' provenance.
#'
#' @name x13cpp_datasets
NULL

#' International airline passengers (Box-Jenkins Series G)
#'
#' Monthly totals of international airline passengers, 1949-1960. The
#' classic Box & Jenkins "Series G" dataset -- the canonical
#' `(0 1 1)(0 1 1)` airline-model example used throughout the X-13
#' documentation. Also distributed as `AirPassengers` in base R.
#'
#' @format A `ts` object, monthly, 1949.01-1960.12 (144 observations), units
#'   of passengers in thousands.
#' @source Box, G.E.P. and Jenkins, G.M. (1976), *Time Series Analysis:
#'   Forecasting and Control*, Series G. Public domain. See
#'   `tests/corpus/data/airline.README.md` in the `x13new` source repository.
#' @examples
#' plot(airline)
"airline"

#' US exports of goods and services (FRED EXPGS)
#'
#' Quarterly US exports of goods and services, 1947 Q1 onward.
#'
#' @format A `ts` object, quarterly, starting 1947 Q1 (317 observations),
#'   units of billions of US dollars (seasonally adjusted annual rate at
#'   source; bundled here as a raw input series).
#' @source U.S. Bureau of Economic Analysis (NIPA) via FRED, St. Louis Fed,
#'   series `EXPGS`. Public domain (U.S. Government work). Retrieved via a
#'   pinned Internet Archive Wayback Machine capture; see
#'   `tests/corpus/data/expgs.README.md` and `tests/corpus/data/fetch_fred.py`
#'   in the `x13new` source repository for exact provenance.
#' @examples
#' plot(expgs)
"expgs"

#' US total nonfarm employment (FRED PAYEMS)
#'
#' Monthly count of all employees, total nonfarm, 2000-01 onward (trimmed
#' from the full FRED history, which begins 1939-01).
#'
#' @format A `ts` object, monthly, starting 2000.01 (308 observations),
#'   units of thousands of persons. Seasonally adjusted at the FRED source;
#'   bundled here as a raw input series for exercising code paths, not for
#'   economic analysis.
#' @source U.S. Bureau of Labor Statistics via FRED, St. Louis Fed, series
#'   `PAYEMS`. Public domain (U.S. Government work). Retrieved via a pinned
#'   Internet Archive Wayback Machine capture; see
#'   `tests/corpus/data/payems.README.md` and `tests/corpus/data/fetch_fred.py`
#'   in the `x13new` source repository for exact provenance.
#' @examples
#' plot(payems)
"payems"

#' US unemployment rate (FRED UNRATE)
#'
#' Monthly US unemployment rate, full history, 1948-01 onward.
#'
#' @format A `ts` object, monthly, starting 1948.01 (932 observations),
#'   units of percent.
#' @source U.S. Bureau of Labor Statistics via FRED, St. Louis Fed, series
#'   `UNRATE`. Public domain (U.S. Government work). Retrieved via a pinned
#'   Internet Archive Wayback Machine capture; see
#'   `tests/corpus/data/unrate.README.md` and `tests/corpus/data/fetch_fred.py`
#'   in the `x13new` source repository for exact provenance.
#' @examples
#' plot(unrate)
"unrate"

# TODO(datasets): two more example series are planned but not yet bundled:
#
#   - `shoe_sales`   -- a shoe/footwear retail sales series, for exercising
#     Easter-effect regressors (regression{ variables = easter[1] } and
#     friends).
#   - `retail_sales` -- a general retail sales series, for exercising
#     trading-day regressors (regression{ variables = td }).
#
# Both should be sourced the same way as the existing FRED series: a pinned
# Internet Archive Wayback Machine capture of the FRED CSV, following the
# pattern in tests/corpus/data/fetch_fred.py. Once tests/corpus/data/ gains
# the corresponding *.dat + *.README.md files, add them to
# r-pkg/data-raw/build_data.R and give them full roxygen docs here matching
# the four datasets above.
