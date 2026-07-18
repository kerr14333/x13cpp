# Render the five X13cpp prototype plots (R / ggplot2 mirror) as PNGs.
#
# Usage:  Rscript render_gallery.R
# Output: tools/plots/R/gallery/*.png  (150 dpi; sizes match the Python gallery)

suppressPackageStartupMessages(library(ggplot2))

args <- commandArgs(trailingOnly = FALSE)
this <- sub("^--file=", "", args[grepl("^--file=", args)])
here <- if (length(this)) dirname(normalizePath(this)) else getwd()
source(file.path(here, "x13plot.R"))

# Oracle output bundles produced by oracle/run_oracle.py.
TMP <- "C:/Users/cyg50/AppData/Local/Temp/corpusval"
BUNDLE_PAYEMS  <- file.path(TMP, "payems_x11-default")
BUNDLE_AIRLINE <- file.path(TMP, "airline_fixed-airline-x11")

outdir <- file.path(here, "gallery")
dir.create(outdir, showWarnings = FALSE, recursive = TRUE)

save_plot <- function(p, name, w, h) {
  f <- file.path(outdir, name)
  ggsave(f, p, width = w, height = h, dpi = 150, units = "in",
         bg = SURFACE)
  message("wrote ", f)
}

plots <- list(
  list(fn = function() plot_overview(
         BUNDLE_PAYEMS,
         "Seasonal adjustment overview",
         "PAYEMS · X-11 default · original, seasonally adjusted, trend-cycle"),
       name = "01_overview.png", w = 9.6, h = 4.6),

  list(fn = function() plot_seasonal_subseries(
         BUNDLE_PAYEMS,
         "Seasonal factors by month",
         "PAYEMS · D10 seasonal factors, per-month subseries with mean"),
       name = "02_seasonal_subseries.png", w = 11.5, h = 4.2),

  list(fn = function() plot_si_ratios(
         BUNDLE_PAYEMS,
         "SI ratios and seasonal factors",
         "PAYEMS · D8 SI ratios (grey), D9 replacements (red), D10 factors (blue)"),
       name = "03_si_ratios.png", w = 11.5, h = 4.2),

  list(fn = function() plot_forecast(
         BUNDLE_AIRLINE,
         "Forecast with 95% interval",
         "Airline · observed history, ARIMA forecast and confidence band"),
       name = "04_forecast.png", w = 9.6, h = 4.6),

  list(fn = function() plot_mstats(
         BUNDLE_PAYEMS,
         "X-11 quality diagnostics",
         "PAYEMS · M1–M11 seasonal-adjustment statistics and overall Q"),
       name = "05_mstats.png", w = 9.6, h = 5.0)
)

for (p in plots) {
  withCallingHandlers(
    save_plot(p$fn(), p$name, p$w, p$h),
    warning = function(w) {
      message("  [warning] ", p$name, ": ", conditionMessage(w))
      invokeRestart("muffleWarning")
    }
  )
}

message("done: ", length(plots), " plots -> ", outdir)
