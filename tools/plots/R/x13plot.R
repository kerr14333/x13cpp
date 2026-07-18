# X13cpp plot-family — R / ggplot2 mirror of tools/plots/x13plot.py
#
# Reproduces the five prototype plots (overview, seasonal subseries, SI ratios,
# forecast fan, M-stats dashboard) from oracle/golden output bundles, using the
# same light-mode design system as the matplotlib prototype.
#
# Design system:
#   surface #fcfcfb, ink #0b0b0b / #52514e, muted #898781,
#   grid #e1e0d9, baseline #c3c2b7,
#   blue #2a78d6, blue-dark #1c5cab, blue-light #cde2fb,
#   green #008300, red #e34948, critical #d03b3b, good-text #006300.

suppressPackageStartupMessages({
  library(ggplot2)
})

# --------------------------------------------------------------------------- #
# Palette / chrome
# --------------------------------------------------------------------------- #
SURFACE    <- "#fcfcfb"
INK        <- "#0b0b0b"
INK2       <- "#52514e"
MUTED      <- "#898781"
GRID       <- "#e1e0d9"
BASELINE   <- "#c3c2b7"

BLUE       <- "#2a78d6"   # primary series (SA)
BLUE_DARK  <- "#1c5cab"   # emphasis / forecast
BLUE_LIGHT <- "#cde2fb"   # interval fills
GREEN      <- "#008300"   # trend
RED        <- "#e34948"   # flagged points
CRITICAL   <- "#d03b3b"   # failing stat
GOOD_TEXT  <- "#006300"   # success text

# Prefer Segoe UI where the graphics device can resolve it; harmless elsewhere.
BASE_FONT <- "Segoe UI"
try(suppressWarnings(windowsFonts(`Segoe UI` = windowsFont("Segoe UI"))),
    silent = TRUE)

MONTHS <- c("Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec")

MISSING <- -999.0

# --------------------------------------------------------------------------- #
# Bundle access
# --------------------------------------------------------------------------- #

# Locate the base name of a bundle directory (the stem of its .out file).
bundle_base <- function(path) {
  out <- list.files(path, pattern = "\\.out$")
  if (length(out) == 0L) stop(sprintf("no .out in %s", path))
  sub("\\.out$", "", out[[1L]])
}

# Read an X-13 save table into a data frame.
#
# Save files have two header lines (date<TAB>label(s), then dashes), followed by
# YYYYMM<TAB>value(<TAB>value...) rows. The sentinel -999 marks missing values
# and is mapped to NA. Multi-column tables (e.g. .fct: forecast/lowerci/upperci)
# keep every value column, named from the header line.
read_save <- function(path, code, base = bundle_base(path)) {
  f <- file.path(path, paste0(base, ".", code))
  lines <- readLines(f, warn = FALSE)
  lines <- lines[nzchar(trimws(lines))]
  if (length(lines) < 3L) stop(sprintf("save file too short: %s", f))

  hdr <- trimws(strsplit(lines[[1L]], "\t")[[1L]])
  value_names <- hdr[-1L]

  body <- lines[-(1:2)]
  toks <- strsplit(trimws(body), "\\s+")
  ncol <- length(toks[[1L]])

  period <- vapply(toks, `[`, character(1L), 1L)
  vals <- matrix(NA_real_, nrow = length(toks), ncol = ncol - 1L)
  for (j in seq_len(ncol - 1L)) {
    vals[, j] <- as.numeric(vapply(toks, `[`, character(1L), j + 1L))
  }
  vals[abs(vals - MISSING) < 1e-6] <- NA_real_

  df <- data.frame(period = period, date = period_to_date(period),
                   stringsAsFactors = FALSE)
  vnames <- if (length(value_names) == ncol - 1L) {
    make.names(value_names, unique = TRUE)
  } else {
    if (ncol - 1L == 1L) "value" else paste0("value", seq_len(ncol - 1L))
  }
  # Canonical first-column name for single-series tables.
  if (ncol - 1L == 1L) vnames <- "value"
  for (j in seq_len(ncol - 1L)) df[[vnames[[j]]]] <- vals[, j]
  attr(df, "value_names") <- vnames
  df
}

# Convert a YYYYMM (or YYYY) period string to a Date on the first of the period.
period_to_date <- function(period) {
  period <- as.character(period)
  n <- nchar(period)
  y <- ifelse(n >= 4L, as.integer(substr(period, 1L, 4L)), NA_integer_)
  m <- ifelse(n == 6L, as.integer(substr(period, 5L, 6L)), 1L)
  as.Date(sprintf("%04d-%02d-01", y, m))
}

# Parse a .udg file ("key: value" lines) into a named list of trimmed strings.
read_udg <- function(path, base = bundle_base(path)) {
  f <- file.path(path, paste0(base, ".udg"))
  lines <- readLines(f, warn = FALSE)
  lines <- lines[grepl(":", lines, fixed = TRUE)]
  out <- list()
  for (ln in lines) {
    idx <- regexpr(":", ln, fixed = TRUE)
    key <- trimws(substr(ln, 1L, idx - 1L))
    val <- trimws(substr(ln, idx + 1L, nchar(ln)))
    if (nzchar(key)) out[[key]] <- val
  }
  out
}

udg_num <- function(udg, key, default = NA_real_) {
  v <- udg[[key]]
  if (is.null(v)) return(default)
  suppressWarnings(as.numeric(v))
}

bundle_period <- function(udg) {
  p <- udg_num(udg, "freq", 12)
  if (is.na(p)) 12L else as.integer(p)
}

# --------------------------------------------------------------------------- #
# Theme
# --------------------------------------------------------------------------- #

# Base ggplot theme implementing the design system. `time_series = TRUE` drops
# vertical grid lines and adds bottom/left axis lines (for line/scatter panels);
# `time_series = FALSE` keeps vertical grid (for the horizontal bar dashboard).
theme_x13 <- function(base_size = 11, time_series = TRUE) {
  th <- theme_minimal(base_size = base_size, base_family = BASE_FONT) +
    theme(
      plot.background   = element_rect(fill = SURFACE, colour = NA),
      panel.background  = element_rect(fill = SURFACE, colour = NA),
      legend.background = element_rect(fill = SURFACE, colour = NA),
      legend.key        = element_rect(fill = SURFACE, colour = NA),
      text              = element_text(colour = INK),
      plot.title        = element_text(colour = INK, face = "bold",
                                       size = base_size + 3, hjust = 0,
                                       margin = margin(b = 2)),
      plot.subtitle     = element_text(colour = INK2, size = base_size - 1,
                                       hjust = 0, margin = margin(b = 10)),
      plot.title.position = "plot",
      axis.title        = element_text(colour = INK2, size = base_size - 1),
      axis.text         = element_text(colour = MUTED, size = base_size - 2),
      axis.ticks        = element_line(colour = BASELINE, linewidth = 0.4),
      panel.grid.minor  = element_blank(),
      legend.title      = element_blank(),
      legend.position   = "top",
      legend.justification = "left",
      legend.direction  = "horizontal",
      legend.margin     = margin(0, 0, 0, 0),
      legend.box.spacing = unit(2, "pt"),
      strip.text        = element_text(colour = INK2, size = base_size - 2),
      plot.margin       = margin(12, 16, 10, 12)
    )
  if (time_series) {
    th <- th + theme(
      panel.grid.major.x = element_blank(),
      panel.grid.major.y = element_line(colour = GRID, linewidth = 0.4),
      axis.line.x        = element_line(colour = BASELINE, linewidth = 0.5),
      axis.line.y        = element_line(colour = BASELINE, linewidth = 0.5)
    )
  } else {
    th <- th + theme(
      panel.grid.major.y = element_blank(),
      panel.grid.major.x = element_line(colour = GRID, linewidth = 0.4)
    )
  }
  th
}

# --------------------------------------------------------------------------- #
# 1. Overview: original, SA, trend
# --------------------------------------------------------------------------- #
plot_overview <- function(bundle, title, subtitle) {
  base <- bundle_base(bundle)
  o  <- read_save(bundle, "a1",  base)
  sa <- read_save(bundle, "d11", base)
  tr <- read_save(bundle, "d12", base)

  lvl <- c("Original (A1)", "Seasonally adjusted (D11)", "Trend-cycle (D12)")
  df <- rbind(
    data.frame(date = o$date,  value = o$value,  series = lvl[1]),
    data.frame(date = sa$date, value = sa$value, series = lvl[2]),
    data.frame(date = tr$date, value = tr$value, series = lvl[3])
  )
  df$series <- factor(df$series, levels = lvl)

  ggplot(df, aes(date, value, colour = series, linewidth = series)) +
    geom_line(na.rm = TRUE) +
    scale_colour_manual(values = c(MUTED, BLUE, GREEN)) +
    scale_linewidth_manual(values = c(1.1, 1.9, 1.6) * 0.4) +
    scale_x_date(expand = expansion(mult = c(0.01, 0.01))) +
    scale_y_continuous(labels = function(v) format(v, big.mark = ",",
                                                    scientific = FALSE,
                                                    trim = TRUE)) +
    labs(title = title, subtitle = subtitle, x = NULL, y = NULL) +
    guides(linewidth = "none",
           colour = guide_legend(override.aes = list(linewidth = 1.2))) +
    theme_x13()
}

# --------------------------------------------------------------------------- #
# helpers for period indexing
# --------------------------------------------------------------------------- #
period_index <- function(dates, per) {
  m <- as.integer(format(dates, "%m"))
  if (per == 12L) m else (m - 1L) %/% 3L + 1L
}
period_labels <- function(per) {
  if (per == 12L) MONTHS else paste0("Q", 1:4)
}

# --------------------------------------------------------------------------- #
# 2. Seasonal factor subseries (per-period panels)
# --------------------------------------------------------------------------- #
plot_seasonal_subseries <- function(bundle, title, subtitle) {
  base <- bundle_base(bundle)
  udg  <- read_udg(bundle, base)
  per  <- bundle_period(udg)
  d10  <- read_save(bundle, "d10", base)

  idx <- period_index(d10$date, per)
  labs_v <- period_labels(per)
  df <- data.frame(year = as.integer(format(d10$date, "%Y")),
                   value = d10$value, idx = idx)
  df$panel <- factor(labs_v[df$idx], levels = labs_v)

  mean_all <- mean(df$value, na.rm = TRUE)
  ref <- if (mean_all > 50) 100.0 else if (mean_all > 0.5 && mean_all < 2) 1.0 else 0.0
  lo <- min(df$value, na.rm = TRUE); hi <- max(df$value, na.rm = TRUE)
  pad <- if (hi > lo) 0.08 * (hi - lo) else 0.01
  ylim <- c(min(lo, ref) - pad, max(hi, ref) + pad)

  means <- aggregate(value ~ panel, df, mean, na.rm = TRUE)

  ggplot(df, aes(year, value)) +
    geom_hline(yintercept = ref, colour = BASELINE, linewidth = 0.4) +
    geom_hline(data = means, aes(yintercept = value),
               colour = BLUE_DARK, linewidth = 0.4, linetype = "22") +
    geom_line(colour = BLUE, linewidth = 0.6, na.rm = TRUE) +
    facet_wrap(~panel, nrow = 1, scales = "free_x") +
    coord_cartesian(ylim = ylim) +
    labs(title = title, subtitle = subtitle, x = NULL, y = NULL) +
    theme_x13() +
    theme(axis.text.x = element_blank(), axis.ticks.x = element_blank(),
          axis.line.x = element_blank(),
          panel.spacing.x = unit(4, "pt"))
}

# --------------------------------------------------------------------------- #
# 3. SI ratios with replacement + seasonal factors
# --------------------------------------------------------------------------- #
plot_si_ratios <- function(bundle, title, subtitle) {
  base <- bundle_base(bundle)
  udg  <- read_udg(bundle, base)
  per  <- bundle_period(udg)
  d8   <- read_save(bundle, "d8",  base)   # unmodified SI
  d9   <- read_save(bundle, "d9",  base)   # replaced SI (sparse)
  d10  <- read_save(bundle, "d10", base)   # seasonal factors
  labs_v <- period_labels(per)

  mk <- function(sv, dates) {
    data.frame(year = as.integer(format(dates, "%Y")), value = sv,
               idx = period_index(dates, per))
  }
  si  <- mk(d8$value,  d8$date)
  sf  <- mk(d10$value, d10$date)
  rep <- mk(d9$value,  d9$date)
  rep <- rep[is.finite(rep$value), , drop = FALSE]

  si$panel  <- factor(labs_v[si$idx],  levels = labs_v)
  sf$panel  <- factor(labs_v[sf$idx],  levels = labs_v)
  if (nrow(rep)) rep$panel <- factor(labs_v[rep$idx], levels = labs_v)

  fin <- d8$value[is.finite(d8$value)]
  lo <- min(fin); hi <- max(fin); pad <- 0.08 * (hi - lo)

  p <- ggplot(mapping = aes(year, value)) +
    geom_point(data = si, colour = MUTED, size = 0.9, na.rm = TRUE) +
    geom_line(data = sf, colour = BLUE, linewidth = 0.6, na.rm = TRUE)
  if (nrow(rep)) {
    p <- p + geom_point(data = rep, colour = RED, size = 1.7, na.rm = TRUE)
  }
  p +
    facet_wrap(~panel, nrow = 1, scales = "free_x") +
    coord_cartesian(ylim = c(lo - pad, hi + pad)) +
    labs(title = title, subtitle = subtitle, x = NULL, y = NULL) +
    theme_x13() +
    theme(axis.text.x = element_blank(), axis.ticks.x = element_blank(),
          axis.line.x = element_blank(),
          panel.spacing.x = unit(4, "pt"))
}

# --------------------------------------------------------------------------- #
# 4. Forecast fan
# --------------------------------------------------------------------------- #
plot_forecast <- function(bundle, title, subtitle, history = 48) {
  base <- bundle_base(bundle)
  a1  <- read_save(bundle, "a1",  base)
  fct <- read_save(bundle, "fct", base)  # forecast, lowerci, upperci
  vn  <- attr(fct, "value_names")

  n <- nrow(a1)
  hist <- a1[max(1L, n - history + 1L):n, , drop = FALSE]
  cut <- hist$date[nrow(hist)]

  obs <- data.frame(date = hist$date, value = hist$value)
  fc  <- data.frame(date = fct$date,
                    forecast = fct[[vn[1]]],
                    lower = if (length(vn) >= 2) fct[[vn[2]]] else NA_real_,
                    upper = if (length(vn) >= 3) fct[[vn[3]]] else NA_real_)

  lvl <- c("Observed", "Forecast")
  p <- ggplot()
  if (all(is.finite(fc$lower)) && all(is.finite(fc$upper))) {
    p <- p + geom_ribbon(data = fc, aes(date, ymin = lower, ymax = upper,
                                        fill = "95% interval")) +
      scale_fill_manual(values = c("95% interval" = BLUE_LIGHT))
  }
  p +
    geom_vline(xintercept = as.numeric(cut), colour = BASELINE, linewidth = 0.4) +
    geom_line(data = obs, aes(date, value, colour = lvl[1]), linewidth = 0.76,
              na.rm = TRUE) +
    geom_line(data = fc, aes(date, forecast, colour = lvl[2]), linewidth = 0.76,
              linetype = "52", na.rm = TRUE) +
    scale_colour_manual(values = setNames(c(BLUE, BLUE_DARK), lvl),
                        breaks = lvl) +
    scale_x_date(expand = expansion(mult = c(0.01, 0.02))) +
    scale_y_continuous(labels = function(v) format(v, big.mark = ",",
                                                    scientific = FALSE,
                                                    trim = TRUE)) +
    labs(title = title, subtitle = subtitle, x = NULL, y = NULL) +
    guides(colour = guide_legend(order = 1,
                                 override.aes = list(linetype = c("solid", "52"))),
           fill = guide_legend(order = 2)) +
    theme_x13()
}

# --------------------------------------------------------------------------- #
# 5. M-statistics dashboard
# --------------------------------------------------------------------------- #
M_DESC <- c(
  m01 = "Irregular contribution",
  m02 = "Irregular in trend",
  m03 = "I/C ratio",
  m04 = "Irregular autocorrelation",
  m05 = "MCD months",
  m06 = "I/S yearly",
  m07 = "Moving seasonality",
  m08 = "Seasonal fluctuation",
  m09 = "Seasonal linear movement",
  m10 = "Recent seasonal fluct.",
  m11 = "Recent seasonal movement"
)

plot_mstats <- function(bundle, title, subtitle) {
  base <- bundle_base(bundle)
  udg  <- read_udg(bundle, base)
  keys <- sprintf("m%02d", 1:11)
  vals <- vapply(keys, function(k) udg_num(udg, paste0("f3.", k)), numeric(1))
  q <- udg_num(udg, "f3.q")

  df <- data.frame(
    key = keys,
    label = sprintf("%s  ·  %s", toupper(keys), M_DESC[keys]),
    value = vals,
    fail = !is.na(vals) & vals >= 1.0
  )
  # m01 at top -> reverse level order (ggplot puts first level at bottom).
  df$label <- factor(df$label, levels = rev(df$label))

  xmax <- max(3.2, max(vals, na.rm = TRUE) * 1.15)
  labpos <- df$value + 0.04 * xmax
  q_col <- if (!is.na(q) && q < 1.0) GOOD_TEXT else CRITICAL
  verdict <- if (!is.na(q) && q < 1.0) "accepted" else "not accepted"

  ggplot(df, aes(value, label)) +
    geom_col(aes(fill = fail), width = 0.62, na.rm = TRUE) +
    geom_vline(xintercept = 1.0, colour = INK2, linewidth = 0.5) +
    geom_text(aes(x = labpos, label = ifelse(is.na(value), "",
                                             sprintf("%.2f", value)),
                  colour = fail),
              hjust = 0, size = 3, na.rm = TRUE) +
    annotate("text", x = 1.02, y = length(keys) + 0.4, label = "acceptance limit",
             colour = INK2, size = 3, hjust = 0, vjust = 1) +
    annotate("text", x = xmax, y = length(keys) + 0.9,
             label = sprintf("Q = %.2f  (%s)", q, verdict),
             colour = q_col, fontface = "bold", size = 4, hjust = 1, vjust = 1) +
    scale_fill_manual(values = c(`FALSE` = BLUE, `TRUE` = CRITICAL),
                      guide = "none") +
    scale_colour_manual(values = c(`FALSE` = INK2, `TRUE` = CRITICAL),
                        guide = "none") +
    scale_x_continuous(limits = c(0, xmax), expand = expansion(mult = c(0, 0.02))) +
    coord_cartesian(clip = "off") +
    labs(title = title, subtitle = subtitle, x = NULL, y = NULL) +
    theme_x13(time_series = FALSE) +
    theme(axis.text.y = element_text(colour = INK, hjust = 0),
          axis.ticks.y = element_blank(),
          plot.margin = margin(12, 20, 10, 12))
}
