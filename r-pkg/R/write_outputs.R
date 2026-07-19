#' Write an x13_result's contents to disk
#'
#' `seasonal_adjust()` never writes files as a side effect -- everything it
#' computes lives on the returned [x13_result] object. `write_outputs()` is
#' the single, explicit, opt-in way to persist that object's contents,
#' mirroring the file layout the Census X-13ARIMA-SEATS program itself
#' writes (one file per save table, plus a `.udg` diagnostics file and a
#' `.fct` forecast file), so output from this package can be diffed against
#' the Fortran oracle's output if desired.
#'
#' @param x An [x13_result] object.
#' @param path Directory to write into. Created if it does not exist.
#' @param basename File basename (without extension) for the written files.
#'   Defaults to `"series"`.
#' @param overwrite Logical; overwrite existing files at `path`? Default
#'   `FALSE`, which errors if any target file already exists.
#' @param ... Passed on to methods; currently unused.
#'
#' @return `path`, invisibly.
#' @export
write_outputs <- function(x, path, ...) UseMethod("write_outputs")

#' @rdname write_outputs
#' @export
write_outputs.x13_result <- function(x, path, basename = "series",
                                      overwrite = FALSE, ...) {
  if (!dir.exists(path)) {
    dir.create(path, recursive = TRUE)
  }

  target_exists <- function(ext) file.exists(file.path(path, paste0(basename, ext)))
  candidates <- c(
    if (length(x$udg)) ".udg",
    if (!is.null(x$fct)) ".fct",
    if (length(x$save)) paste0(".", names(x$save))
  )
  clashes <- Filter(target_exists, candidates)
  if (length(clashes) && !overwrite) {
    stop(
      "Refusing to overwrite existing file(s): ",
      paste(file.path(path, paste0(basename, clashes)), collapse = ", "),
      ". Pass overwrite = TRUE to replace them.",
      call. = FALSE
    )
  }

  written <- character(0)

  if (length(x$udg)) {
    udg_file <- file.path(path, paste0(basename, ".udg"))
    lines <- vapply(names(x$udg), function(nm) {
      paste0(nm, ": ", paste(format(x$udg[[nm]]), collapse = " "))
    }, character(1))
    writeLines(lines, udg_file)
    written <- c(written, udg_file)
  }

  if (!is.null(x$fct)) {
    fct_file <- file.path(path, paste0(basename, ".fct"))
    utils::write.csv(as.data.frame(x$fct), fct_file, row.names = FALSE)
    written <- c(written, fct_file)
  }

  for (tbl in names(x$save)) {
    tbl_file <- file.path(path, paste0(basename, ".", tbl))
    utils::write.csv(as.data.frame(x$save[[tbl]]), tbl_file, row.names = FALSE)
    written <- c(written, tbl_file)
  }

  message("Wrote ", length(written), " file(s) to ", normalizePath(path, mustWork = FALSE))
  invisible(path)
}
