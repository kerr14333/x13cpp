# Build data/*.rda from the frozen corpus series in tests/corpus/data/.
#
# This script is NOT run at package build/install time (data-raw/ is
# .Rbuildignore'd) -- it is a developer tool to regenerate the bundled
# datasets if the upstream .dat files ever change. Run with the working
# directory set to r-pkg/ (the package root):
#
#   cd r-pkg && Rscript data-raw/build_data.R
#
# Source of truth for start/period metadata: the *.README.md files next to
# each .dat file in tests/corpus/data/.

pkg_root <- getwd()
corpus_dir <- file.path(pkg_root, "..", "tests", "corpus", "data")
stopifnot(
  "Run this script with the working directory set to r-pkg/ (package root)" =
    dir.exists(corpus_dir)
)

read_series <- function(file, start, frequency) {
  values <- scan(file.path(corpus_dir, file), what = double(), quiet = TRUE)
  stats::ts(values, start = start, frequency = frequency)
}

# name -> (file, ts start, frequency) -- see the per-series README.md for
# provenance (source, units, download vintage).
airline <- read_series("airline.dat", start = c(1949, 1), frequency = 12)
expgs   <- read_series("expgs.dat",   start = c(1947, 1), frequency = 4)
payems  <- read_series("payems.dat",  start = c(2000, 1), frequency = 12)
unrate  <- read_series("unrate.dat",  start = c(1948, 1), frequency = 12)

out_dir <- file.path(pkg_root, "data")
stopifnot(dir.exists(out_dir))

save(airline, file = file.path(out_dir, "airline.rda"), compress = "xz")
save(expgs, file = file.path(out_dir, "expgs.rda"), compress = "xz")
save(payems, file = file.path(out_dir, "payems.rda"), compress = "xz")
save(unrate, file = file.path(out_dir, "unrate.rda"), compress = "xz")

# TODO(datasets): add `shoe_sales` (Easter-effect testing) and
# `retail_sales` (trading-day testing) once those series are fetched into
# tests/corpus/data/ via the same pinned-FRED-Wayback pattern as
# tests/corpus/data/fetch_fred.py. See R/data.R for the placeholder docs
# that should gain real content alongside this script.

message("Wrote airline.rda, expgs.rda, payems.rda, unrate.rda to ", out_dir)
