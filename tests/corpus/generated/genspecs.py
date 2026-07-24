#!/usr/bin/env python
"""genspecs.py -- systematic X-13ARIMA-SEATS spec generator for the parity corpus.

Deterministic and idempotent: running it wipes and regenerates every ``*.spc``
file (plus ``MANIFEST``) in this directory. It never touches the sibling corpus
directories (``data/``, ``census-examples/``, ``edge/``).

It crosses the four base corpus series with a set of adjustment configurations,
decorating each spec with ``print = all``, an explicit ``save = (...)`` of every
savable table for that spec, and ``savelog = all`` on the specs where that is
legal. See the module comments for why explicit save lists are used instead of
``save = all``.

Run:  python genspecs.py
"""

from __future__ import annotations

import os

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = "../data"  # data files live in the sibling data/ directory

# ---------------------------------------------------------------------------
# Authoritative table dictionaries.
#
# save = all is NOT accepted by X-13ARIMA-SEATS v1.1 b61: the `save` argument
# only takes explicit table names (oracle/fortran/getsav.f looks the token up in
# the save table dictionary; there is no `all` level, unlike `print`). To honor
# the intent ("save every table") with valid specs we emit the *complete*
# explicit list of savable tables for each spec. These lists were extracted
# programmatically from the save dictionary in oracle/fortran/stable.prm /
# stable.var (verified against getsav.f's addressing).
# ---------------------------------------------------------------------------
SAVE = {
    "series": ["a1", "spc", "mv", "a18", "a19", "b1"],
    "transform": ["a1c", "a2", "a2p", "a2t", "a3", "a3p", "a4d", "a4p", "trn"],
    "regression": ["rmx", "otl", "ao", "ls", "tc", "so", "td", "hol", "usr",
                   "a10", "a13"],
    "estimate": ["itr", "mdl", "rcm", "est", "acm", "lks", "rts", "ref", "rsd",
                 "rrs"],
    "outlier": ["oit", "fts"],
    "forecast": ["ftr", "fvr", "fct", "btr", "bct"],
    "x11": ["c1", "d1", "e1", "f1", "b2", "c2", "d2", "e2", "b3", "e3", "c4",
            "d4", "b5", "c5", "d5", "e5", "pe5", "b6", "c6", "d6", "e6", "pe6",
            "b7", "c7", "d7", "e7", "pe7", "b8", "d8", "d8b", "e8", "pe8", "c9",
            "d9", "b10", "c10", "d10", "psf", "fsd", "ars", "sns", "b11", "c11",
            "d11", "sac", "e11", "d12", "tal", "bcf", "tac", "b13", "c13", "d13",
            "pir", "ira", "b17", "c17", "b20", "c20", "h1", "chl", "d16", "paf",
            "fad", "d18", "e18", "tad", "b19", "c19", "e4", "saf", "trf", "iwf"],
    "seats": ["s12", "stc", "s10", "pss", "s13", "psi", "s11", "sec", "s14",
              "psc", "s16", "psa", "tfd", "sfd", "ofd", "afd", "yfd", "s18",
              "sta", "wkf", "mdc", "pic", "pis", "pit", "pia", "gaf", "gac",
              "gtf", "gtc", "tac", "ttc", "faf", "fac", "ftf", "ftc", "dor",
              "dsa", "dtr", "ssm", "cyc", "ltt", "sse", "ase", "tse", "cse",
              "se2", "se3", "stl"],
}

# Specs whose savelog range includes the alldiagnostics/'all' entry, i.e. where
# `savelog = all` is legal (from oracle/fortran/svltbl.prm + svllog.i). transform,
# regression, outlier, etc. have savelog diagnostics but no 'all' shortcut.
SAVELOG_ALL = {"automdl", "estimate", "check", "x11", "seats", "history",
               "spectrum", "composite"}

# Specs that accept a print argument (everything with print/save tables; NOT arima).
PRINT_SPECS = {"series", "transform", "regression", "identify", "automdl",
               "pickmdl", "estimate", "outlier", "check", "forecast", "spectrum",
               "x11", "force", "x11regression", "history", "slidingspans",
               "composite", "seats"}

# ---------------------------------------------------------------------------
# Base series registry.
# ---------------------------------------------------------------------------
SERIES = {
    "airline": dict(title="International Airline Passengers", file="airline.dat",
                    start="1949.01", period=12, rate=False, span=None),
    "payems": dict(title="US Total Nonfarm Employment (PAYEMS)", file="payems.dat",
                   start="2000.01", period=12, rate=False, span=None),
    # UNRATE full history (932 obs) exceeds POBS=780; span keeps the analysed
    # window <= 780. It is a rate, so no transform is ever applied.
    "unrate": dict(title="US Unemployment Rate (UNRATE)", file="unrate.dat",
                   start="1948.01", period=12, rate=True, span="(1961.01, )"),
    "expgs": dict(title="US Exports of Goods and Services (EXPGS)", file="expgs.dat",
                  start="1947.1", period=4, rate=False, span=None),
}


# ---------------------------------------------------------------------------
# Block rendering helpers.
# ---------------------------------------------------------------------------
def _tbl(names, indent="    ", first_prefix_len=9):
    """Render a (name name ...) list wrapped to stay under X-13's 132-char
    input-record limit (longer lines abort the run with an error that goes
    only to stdout: "ERROR: Input record longer than limit :  133")."""
    limit = 100  # conservative: leaves room for the two-space block indent
    out_lines, cur = [], []
    cur_len = first_prefix_len + 1
    for n in names:
        if cur and cur_len + len(n) + 1 > limit:
            out_lines.append(" ".join(cur))
            cur, cur_len = [], len(indent)
        cur.append(n)
        cur_len += len(n) + 1
    if cur:
        out_lines.append(" ".join(cur))
    joiner = "\n" + indent
    return "(" + joiner.join(out_lines) + ")"


def spec(name, arglines, *, save_key=None, savelog=False):
    """Render one spec block with the standard print/save/savelog decorations."""
    lines = list(arglines)
    if name in PRINT_SPECS:
        lines.append("print = all")
    if save_key is not None:
        lines.append("save = " + _tbl(SAVE[save_key]))
    if savelog and name in SAVELOG_ALL:
        lines.append("savelog = all")
    body = "\n".join("  " + ln for ln in lines)
    return name + "{\n" + body + "\n}"


def series_block(s):
    lines = [
        'title = "%s"' % s["title"],
        'file = "%s/%s"' % (DATA, s["file"]),
        "start = %s" % s["start"],
        "period = %d" % s["period"],
    ]
    if s["span"]:
        lines.append("span = %s" % s["span"])
    return spec("series", lines, save_key="series")


def transform_log():
    return spec("transform", ["function = log"], save_key="transform")


def transform_auto():
    return spec("transform", ["function = auto"], save_key="transform")


def arima_airline():
    # No print/save/savelog: the arima spec has none.
    return "arima{\n  model = (0 1 1)(0 1 1)\n}"


def arima_ar2():
    # General-shape SEATS probe: a nonseasonal AR(2) (p>0), the first non-
    # airline-family model the SEATS canonical decomposition gates on.
    return "arima{\n  model = (2 1 0)(0 1 1)\n}"


def arima_sar():
    # Seasonal-AR probe: Bp>0 (a seasonal AR(1) in place of the seasonal MA).
    # Exercises the seasonal-AR branch of the SEATS canonical decomposition.
    return "arima{\n  model = (0 1 1)(1 1 0)\n}"


def forecast_block(period):
    maxlead = 12 if period == 12 else 8
    return spec("forecast", ["maxlead = %d" % maxlead], save_key="forecast")


def x11_block(mode=None):
    args = [] if mode is None else ["mode = %s" % mode]
    return spec("x11", args, save_key="x11", savelog=True)


def seats_block():
    return spec("seats", [], save_key="seats", savelog=True)


def automdl_block():
    return spec("automdl", [], savelog=True)


def estimate_block():
    return spec("estimate", [], save_key="estimate", savelog=True)


def outlier_block():
    return spec("outlier", [], save_key="outlier")


# ---------------------------------------------------------------------------
# Configurations. Each returns the list of spec blocks that follow the series
# block, for a given series dict `s`.
#
# Rule: when a transform{} spec is present (positive series) the x11 mode is
# left to the program to harmonise with the transform (x11{} with no mode).
# When there is no transform (rate series, or pure X-11), the x11 mode is set
# explicitly. Pure multiplicative/log-additive X-11 needs positive data, which
# all four series satisfy.
# ---------------------------------------------------------------------------
def cfg_x11_default(s):
    return [x11_block("add" if s["rate"] else "mult")]


def cfg_x11_logadd(s):
    return [x11_block("logadd")]


def cfg_x11_additive(s):
    return [x11_block("add")]


def cfg_seats(s):
    blocks = []
    if not s["rate"]:
        blocks.append(transform_log())
    blocks.append(automdl_block())
    blocks.append(seats_block())
    return blocks


def cfg_automdl_x11(s):
    blocks = []
    if not s["rate"]:
        blocks.append(transform_auto())
    blocks.append(automdl_block())
    blocks.append(forecast_block(s["period"]))
    blocks.append(x11_block("add" if s["rate"] else None))
    return blocks


def cfg_fixed_airline_x11(s):
    blocks = []
    if not s["rate"]:
        blocks.append(transform_log())
    blocks.append(spec("regression", ["variables = (td)"], save_key="regression"))
    blocks.append(arima_airline())
    blocks.append(estimate_block())
    blocks.append(outlier_block())
    blocks.append(forecast_block(s["period"]))
    blocks.append(x11_block("add" if s["rate"] else None))
    return blocks


def cfg_fixed_airline_seats(s):
    blocks = []
    if not s["rate"]:
        blocks.append(transform_log())
    blocks.append(arima_airline())
    blocks.append(estimate_block())
    blocks.append(forecast_block(s["period"]))
    blocks.append(seats_block())
    return blocks


def cfg_ar2_seats(s):
    # Explicit nonseasonal-AR(2) model -> general-shape SEATS decomposition
    # (p>0). No forecast block: SEATS does its own FCAST extension internally.
    blocks = []
    if not s["rate"]:
        blocks.append(transform_log())
    blocks.append(arima_ar2())
    blocks.append(estimate_block())
    blocks.append(seats_block())
    return blocks


def cfg_sar_seats(s):
    # Explicit seasonal-AR model (0 1 1)(1 1 0) -> the Bp>0 branch of the SEATS
    # canonical decomposition. Complements ar2-seats (p>0) with seasonal AR.
    blocks = []
    if not s["rate"]:
        blocks.append(transform_log())
    blocks.append(arima_sar())
    blocks.append(estimate_block())
    blocks.append(seats_block())
    return blocks


def cfg_mean_seats(s):
    # Constant (mean) regressor -> Imean=1: SEATS keeps the mean in the
    # decomposed series and folds its drift back via the wm centering + za/
    # wmf/wmb path (estbur.cpp). The airline (0 1 1)(0 1 1) shape isolates the
    # pure drift case (d>=1, Pstar=0).
    blocks = []
    if not s["rate"]:
        blocks.append(transform_log())
    blocks.append(spec("regression", ["variables = (const)"],
                       save_key="regression"))
    blocks.append(arima_airline())
    blocks.append(estimate_block())
    blocks.append(seats_block())
    return blocks


def cfg_mean_d0_seats(s):
    # Constant (mean) regressor with a d==0 model (2 0 0)(0 1 1): exercises the
    # mean path with NO regular differencing (Pstar=2 AR, kd=(-1)^(d+bd)=-1 so
    # the backward CALCFX center / FCAST seed sign-flip). The d==0 trend has no
    # unit root; verified the mean handling is d-agnostic (im2).
    blocks = []
    if not s["rate"]:
        blocks.append(transform_log())
    blocks.append(spec("regression", ["variables = (const)"],
                       save_key="regression"))
    blocks.append("arima{\n  model = (2 0 0)(0 1 1)\n}")
    blocks.append(estimate_block())
    blocks.append(seats_block())
    return blocks


def cfg_automdl_aictest_x11(s):
    blocks = []
    if not s["rate"]:
        blocks.append(transform_auto())
    aic = "(td easter)" if s["period"] == 12 else "(td)"
    blocks.append(spec("regression", ["aictest = " + aic], save_key="regression"))
    blocks.append(automdl_block())
    blocks.append(x11_block("add" if s["rate"] else None))
    return blocks


# Ordered so filenames sort deterministically.
CONFIGS = [
    ("x11-default", cfg_x11_default),
    ("x11-logadd", cfg_x11_logadd),
    ("x11-additive", cfg_x11_additive),
    ("seats", cfg_seats),
    ("automdl-x11", cfg_automdl_x11),
    ("fixed-airline-x11", cfg_fixed_airline_x11),
    ("fixed-airline-seats", cfg_fixed_airline_seats),
    ("ar2-seats", cfg_ar2_seats),
    ("sar-seats", cfg_sar_seats),
    ("mean-seats", cfg_mean_seats),
    ("mean-d0-seats", cfg_mean_d0_seats),
    ("automdl-aictest-x11", cfg_automdl_aictest_x11),
]

HEADER = (
    "# Generated by genspecs.py -- DO NOT EDIT BY HAND.\n"
    "# series = {series}, config = {config}\n"
    "# Regenerate with: python genspecs.py\n"
)


def build_spec(series_name, s, config_name, config_fn):
    blocks = [series_block(s)] + config_fn(s)
    header = HEADER.format(series=series_name, config=config_name)
    return header + "\n".join(blocks) + "\n"


def main():
    # Idempotent: remove previously generated specs first.
    for fn in os.listdir(HERE):
        if fn.endswith(".spc"):
            os.remove(os.path.join(HERE, fn))

    written = []
    for series_name in sorted(SERIES):
        s = SERIES[series_name]
        for config_name, config_fn in CONFIGS:
            fname = "%s_%s.spc" % (series_name, config_name)
            text = build_spec(series_name, s, config_name, config_fn)
            with open(os.path.join(HERE, fname), "w", newline="\n") as fh:
                fh.write(text)
            written.append(fname)

    written.sort()
    with open(os.path.join(HERE, "MANIFEST"), "w", newline="\n") as fh:
        fh.write("# Generated spec inventory -- produced by genspecs.py\n")
        fh.write("# %d specs = %d series x %d configs\n"
                 % (len(written), len(SERIES), len(CONFIGS)))
        for fname in written:
            fh.write(fname + "\n")

    print("genspecs.py: wrote %d specs (%d series x %d configs) to %s"
          % (len(written), len(SERIES), len(CONFIGS), HERE))
    for fname in written:
        print("  " + fname)


if __name__ == "__main__":
    main()
