#!/usr/bin/env python
"""genextra.py -- extra X-13ARIMA-SEATS spec generator for the parity corpus.

Covers the spec blocks the systematic ``generated/`` corpus does NOT exercise,
needed by milestones M4-M10:

  spectrum, history, slidingspans, x11regression, force, metadata, pickmdl,
  seats variants (tabtables/out/hpcycle/qmax/rmod), outlier(types=all,lsrun),
  check(savelog=all), identify(diff/sdiff grids).

Deterministic and idempotent: running it wipes and regenerates every ``*.spc``
file (plus ``MANIFEST``) in this directory. It never touches sibling corpus
directories (``../data/``, ``../generated/``, ``../census-examples/``,
``../edge/``). The ``pickmdl.mdl`` model file and ``data/`` provenance README
are committed by hand alongside this generator, not regenerated here.

Run:  python genextra.py

Save-table lists are the explicit valid tokens for each spec, taken from the
X-13 table dictionaries in ../../../oracle/fortran/stable.prm (TB1DIC..TB4DIC)
and validated against the per-spec getsav ranges by running every spec through
the oracle until run_ok=True. See the module-level SAVE dict for provenance of
each token. Every spec also carries ``print = all``; blocks whose savelog range
includes the 'all' entry carry ``savelog = all``.
"""

from __future__ import annotations

import os

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = "../data"  # shared corpus data lives in the sibling data/ directory

# ---------------------------------------------------------------------------
# Save-table tokens per spec block (3-char abbreviations from TB1DIC..TB4DIC).
#
# spectrum : sp0 specorig, sp1 specsa, sp2 specirr, spr specresiduals,
#            st0 spectukeyorig, st1 spectukeysa, st2 spectukeyirr,
#            s1s specseatssa, s2s specseatsirr (seats runs only).
# seats    : the SEATS component tables (tb4DIC); reused from genspecs.py's
#            validated list, trimmed to the single-series (non-composite) set.
# identify : acf sample-ACF, pacf sample-PACF, iac inverse-ACF, ipc
#            inverse-PACF (the identify getsav range accepts these four; the
#            display token for PACF is 'pacf', not 'pcf').
# check    : acf, pcf, ac2 (acf-squared).
# outlier  : oit outlieriterations, fts finaltests.
# history  : sar saRevisions, sae saEstimates, chr chngRevisions,
#            che chngEstimates, trr trendRevisions, tre trendEstimates,
#            tcr trendChngRevisions, tce trendChngEstimates,
#            sfr sfRevisions, sfe sfEstimates (tb2DIC).
# sspans   : sfs sfSpans, ads saSpans, chs chngSpans (tb3DIC).
# x11reg   : xrm xregressionMatrix, b16/c16 x11reg trading-day factors (tb2DIC).
# force    : saa seasadjTotal, ffc forceFactor, rnd saRound (tb2DIC).
# force_full: force{}'s complete 9-tag save dictionary (LSPFRC=208, NSPFRC=9,
#           frctbl.i LFCSAA..LFRFAC), decoded programmatically from
#           stable.prm/stable.var (see tools/testgen_scope.md for the decoder).
#           saa seasadjTotal (D11A), rnd saRound (written only when round=yes),
#           cr cratio / rr rratio (type=regress only, via qmap2.f -- empty for
#           type=denton), ffc forceFactor, e6a/p6a revised-SA period changes
#           (diff/pct) and e6r/p6r rounded-SA period changes (diff/pct) --
#           all populated on a plain single-series run with type=regress +
#           round=yes (verified against the oracle: see
#           airline_automdl-x11-force.spc's golden, all 9 tags non-empty).
#           Not composite{}-only, despite qmap2.f's Iagr parameter name.
# ---------------------------------------------------------------------------
SAVE = {
    "spectrum_x11": ["sp0", "sp1", "sp2", "spr", "st0", "st1", "st2"],
    "spectrum_seats": ["sp0", "sp1", "sp2", "spr", "s1s", "s2s",
                        "st0", "st1", "st2"],
    "seats": ["s12", "s10", "s13", "s11", "s14", "s16", "sec", "tfd", "sfd",
              "ofd", "afd", "yfd", "s18", "sta", "wkf", "dor", "dsa", "dtr",
              "cyc", "ltt"],
    "identify": ["acf", "pacf", "iac", "ipc"],
    "check": ["acf", "pcf", "ac2"],
    "outlier": ["oit", "fts"],
    "history": ["sar", "sae", "chr", "che", "trr", "tre", "tcr", "tce",
                "sfr", "sfe"],
    "sspans": ["sfs", "ads", "chs"],
    "x11reg": ["xrm", "b16", "c16"],
    "x11d": ["d10", "d11", "d12", "d13"],
    "tdprior": ["a4"],
    "force": ["saa", "ffc", "rnd"],
    "force_full": ["saa", "rnd", "e6a", "p6a", "e6r", "p6r", "cr", "rr", "ffc"],
}

# Specs whose savelog range includes the 'all' shortcut (svltbl.prm).
SAVELOG_ALL = {"estimate", "check", "x11", "seats", "history", "spectrum",
               "automdl"}


# ---------------------------------------------------------------------------
# Rendering helpers (mirrors generated/genspecs.py wrapping to stay < 132-char
# input-record limit; wrap conservatively at 100).
# ---------------------------------------------------------------------------
def _tbl(names, indent="    ", first_prefix_len=9):
    limit = 100
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


def block(name, arglines, *, save_key=None, print_all=True, savelog=False):
    lines = list(arglines)
    if print_all:
        lines.append("print = all")
    if save_key is not None:
        lines.append("save = " + _tbl(SAVE[save_key]))
    if savelog and name in SAVELOG_ALL:
        lines.append("savelog = all")
    body = "\n".join("  " + ln for ln in lines)
    return name + "{\n" + body + "\n}"


def series_airline():
    return block("series", [
        'title = "International Airline Passengers"',
        'file = "%s/airline.dat"' % DATA,
        "start = 1949.01",
        "period = 12",
    ], save_key=None, print_all=False)


def series_payems(span=None):
    lines = [
        'title = "US Total Nonfarm Employment (PAYEMS)"',
        'file = "%s/payems.dat"' % DATA,
        "start = 2000.01",
        "period = 12",
    ]
    if span:
        lines.append("span = %s" % span)
    return block("series", lines, print_all=False)


def transform_log():
    return block("transform", ["function = log"], print_all=False)


def arima_airline():
    return "arima{\n  model = (0 1 1)(0 1 1)\n}"


def estimate_block():
    return block("estimate", [], print_all=True, savelog=True)


def x11_block(mode=None):
    args = [] if mode is None else ["mode = %s" % mode]
    return block("x11", args, print_all=True, savelog=True)


def forecast_block(maxlead=12):
    return block("forecast", ["maxlead = %d" % maxlead], print_all=True)


def transform_auto():
    return block("transform", ["function = auto"], print_all=False)


def automdl_block():
    return block("automdl", [], print_all=True, savelog=True)


HEADER = (
    "# Generated by genextra.py -- DO NOT EDIT BY HAND.\n"
    "# coverage: {cover}\n"
    "# Regenerate with: python genextra.py\n"
)


def assemble(cover, blocks):
    return HEADER.format(cover=cover) + "\n".join(blocks) + "\n"


# ---------------------------------------------------------------------------
# Spec builders. Each returns (filename, text).
# ---------------------------------------------------------------------------
def spec_spectrum():
    blocks = [
        series_airline(),
        transform_log(),
        block("regression", ["variables = (td)"], print_all=True),
        arima_airline(),
        estimate_block(),
        forecast_block(),
        x11_block(),
        block("spectrum", ["type = periodogram"],
              save_key="spectrum_x11", print_all=True, savelog=True),
    ]
    return "airline_spectrum.spc", assemble(
        "spectrum{} periodogram+AR spectra, sp0/sp1/sp2/spr + Tukey st0/st1/st2,"
        " savelog peaks/qs", blocks)


def spec_spectrum_arspec():
    blocks = [
        series_airline(),
        transform_log(),
        arima_airline(),
        estimate_block(),
        x11_block(),
        block("spectrum", ["type = arspec", "maxar = 30", "qcheck = yes"],
              save_key="spectrum_x11", print_all=True, savelog=True),
    ]
    return "airline_spectrum-arspec.spc", assemble(
        "spectrum{} AR(30) spectrum + QS check variant", blocks)


def spec_history():
    blocks = [
        series_airline(),
        transform_log(),
        arima_airline(),
        estimate_block(),
        x11_block(),
        block("history",
              ["estimates = (sadj sadjchng seasonal trend trendchng)",
               "start = 1955.jan"],
              save_key="history", print_all=True, savelog=True),
    ]
    return "airline_history.spc", assemble(
        "history{} sadj+sadjchng+seasonal+trend+trendchng revisions, span from 1955",
        blocks)


def spec_slidingspans():
    blocks = [
        series_airline(),
        transform_log(),
        arima_airline(),
        estimate_block(),
        x11_block(),
        block("slidingspans", [], save_key="sspans", print_all=True),
    ]
    return "airline_slidingspans.spc", assemble(
        "slidingspans{} default seasonal-stability spans", blocks)


def spec_slidingspans_cutseas():
    blocks = [
        series_airline(),
        transform_log(),
        arima_airline(),
        estimate_block(),
        x11_block(),
        block("slidingspans", ["cutseas = 5.0", "cutchng = 5.0"],
              save_key="sspans", print_all=True),
    ]
    return "airline_slidingspans-cutseas.spc", assemble(
        "slidingspans{} cutseas/cutchng threshold variant", blocks)


def spec_x11regression():
    blocks = [
        series_airline(),
        transform_log(),
        arima_airline(),
        estimate_block(),
        # Modeled -td: gtinpt promotes Ixreg 1->2, so the oracle runs the
        # transparent prior-TD pass (xrgdrv) and fits the model to the
        # TD-adjusted series. Save d10-d13 to gate the whole OLS-prior-TD chain
        # bit-exact (test_x11regression_tables.py), alongside the xrm/b16/c16
        # regression outputs.
        block("x11", [], save_key="x11d", print_all=True, savelog=True),
        block("x11regression", ["variables = (td)"],
              save_key="x11reg", print_all=True),
    ]
    return "airline_x11regression-td.spc", assemble(
        "x11regression{} irregular-component trading-day regression", blocks)


def spec_x11regression_aictest():
    blocks = [
        series_airline(),
        transform_log(),
        arima_airline(),
        estimate_block(),
        x11_block(),
        # aictest on TD alone would drop the sole regressor on the airline
        # irregular (no TD signal) and abort with exit=2; keep a fixed td
        # regressor and AIC-test the Easter holiday so the estimation still
        # runs while exercising the x11regression aictest code path.
        block("x11regression", ["variables = (td)", "aictest = (easter)"],
              save_key="x11reg", print_all=True),
    ]
    return "airline_x11regression-aictest.spc", assemble(
        "x11regression{} fixed TD + AIC-tested Easter on the irregular", blocks)


def spec_x11regression_tdprior():
    # tdprior: seven user prior trading-day weights (Mon..Sun, standardized to
    # sum 7.0). Gates the a4 prior-TD factor bit-exact (pritd.f: td6var contrasts
    # x weights / Xnstar). The d10-d13 goldens ride along for the future pre-model
    # plumbing gate (the oracle fits the model to the prior-adjusted series).
    blocks = [
        series_airline(),
        transform_log(),
        arima_airline(),
        estimate_block(),
        block("x11", [], save_key="x11d", print_all=True, savelog=True),
        block("x11regression", ["tdprior = (1.4 1.4 1.4 1.4 1.4 0.5 0.5)"],
              save_key="tdprior", print_all=True),
    ]
    return "airline_x11regression-tdprior.spc", assemble(
        "x11regression{} user prior trading-day weights (tdprior, Kswv=1)", blocks)


def spec_force_denton():
    blocks = [
        series_airline(),
        transform_log(),
        arima_airline(),
        estimate_block(),
        x11_block(),
        block("force", ["type = denton", "target = original"],
              save_key="force", print_all=True),
    ]
    return "airline_force-denton.spc", assemble(
        "force{} Denton benchmarking of the SA total to original", blocks)


def spec_force_regress():
    blocks = [
        series_airline(),
        transform_log(),
        arima_airline(),
        estimate_block(),
        x11_block(),
        block("force", ["type = regress", "target = original", "rho = 0.9"],
              save_key="force", print_all=True),
    ]
    return "airline_force-regress.spc", assemble(
        "force{} regression (Cholette-Dagum) benchmarking to original", blocks)


def spec_force_automdl_x11():
    # M5 x11pt3 force-yearly-totals proof case: the SAME automdl-x11 config
    # genspecs.py already produces in ../generated/airline_automdl-x11.spc
    # (whose b1/d10-d13 already pass the C++ x13run_x11 gate) with a force{}
    # block layered on top. type = regress + round = yes populates all 9 of
    # force's save tags on this single series (verified against the oracle
    # golden) -- see force_full's comment above.
    blocks = [
        series_airline(),
        transform_auto(),
        automdl_block(),
        forecast_block(),
        x11_block(),
        block("force",
              ["type = regress", "target = original", "round = yes",
               "rho = 0.9"],
              save_key="force_full", print_all=True),
    ]
    return "airline_automdl-x11-force.spc", assemble(
        "force{} regress+round=yes on automdl-x11 -- x11pt3 force gate proof",
        blocks)


def spec_metadata():
    blocks = [
        series_airline(),
        transform_log(),
        arima_airline(),
        estimate_block(),
        x11_block(),
        block("metadata",
              ['keys = (source frequency units)',
               'values = ("test-corpus" "monthly" "passengers")'],
              print_all=False),
    ]
    return "airline_metadata.spc", assemble(
        "metadata{} keys/values roundtrip into .udg", blocks)


def spec_pickmdl():
    blocks = [
        series_airline(),
        transform_log(),
        block("regression", ["variables = (td)"], print_all=True),
        block("pickmdl",
              ["mode = fcst", 'file = "pickmdl.mdl"', "method = best",
               "identify = all"],
              print_all=True),
        estimate_block(),
        forecast_block(),
        x11_block(),
    ]
    return "airline_pickmdl.spc", assemble(
        "pickmdl{} classic X-11-ARIMA 5-model selection (pickmdl.mdl)", blocks)


def spec_seats_tabtables():
    blocks = [
        series_airline(),
        transform_log(),
        arima_airline(),
        estimate_block(),
        forecast_block(),
        block("seats",
              ['tabtables = "xo,n,s,p"', "out = 2", "hpcycle = yes"],
              save_key="seats", print_all=True, savelog=True),
    ]
    return "airline_seats-tabtables.spc", assemble(
        "seats{} tabtables/.tbs + out=2 + HP cycle", blocks)


def spec_seats_qmax_rmod():
    blocks = [
        series_airline(),
        transform_log(),
        arima_airline(),
        estimate_block(),
        forecast_block(),
        block("seats", ["qmax = 30", "rmod = 0.7", "noadmiss = yes"],
              save_key="seats", print_all=True, savelog=True),
    ]
    return "airline_seats-qmax-rmod.spc", assemble(
        "seats{} qmax/rmod/noadmiss decomposition variant", blocks)


def spec_outlier_lsrun():
    # PAYEMS 2000.01-2025.08 contains the 2020 COVID level shifts; types=all +
    # lsrun surfaces them. No x11 needed -- outlier ID is a regARIMA step.
    blocks = [
        series_payems(),
        transform_log(),
        arima_airline(),
        block("outlier",
              ["types = all", "lsrun = 3", "critical = 3.5"],
              save_key="outlier", print_all=True),
        estimate_block(),
    ]
    return "payems_outlier-lsrun.spc", assemble(
        "outlier{types=all,lsrun} COVID-2020 level shifts on PAYEMS", blocks)


def spec_check():
    blocks = [
        series_airline(),
        transform_log(),
        arima_airline(),
        estimate_block(),
        block("check", ["maxlag = 24"],
              save_key="check", print_all=True, savelog=True),
    ]
    return "airline_check.spc", assemble(
        "check{} residual diagnostics with savelog=all", blocks)


def spec_identify():
    blocks = [
        series_airline(),
        transform_log(),
        block("identify",
              ["diff = (0 1)", "sdiff = (0 1)", "maxlag = 36"],
              save_key="identify", print_all=True),
    ]
    return "airline_identify.spc", assemble(
        "identify{} ACF/PACF over diff x sdiff grid", blocks)


# ---------------------------------------------------------------------------
# transform{} user prior-adjustment factors (getadj.f Usrpad/Usrtad).
#
# A synthetic 6-month "strike" dip in 1955 expressed as percentages: 100 = no
# adjustment, 112.5 = the series ran 12.5% above trend that month. The same 144
# values are used inline (data=) and, for the no-model case, read from the
# generated prior-strike.dat -- so the two input routes gate identical
# arithmetic.
PRIOR_PCT = ["100.0"] * 144
for _k in range(72, 78):                       # 1955.01 .. 1955.06
    PRIOR_PCT[_k] = "112.5"
PRIOR_FILE = "prior-strike.dat"


def _prior_rows(vals):
    return "\n".join("    " + " ".join(vals[i:i + 12])
                     for i in range(0, len(vals), 12))


def transform_prior(inline=True, kind="permanent", adjust=None, trend=False):
    """transform{} carrying one set of user prior-adjustment factors."""
    lines = ["function = log"]
    if adjust is not None:
        lines.append("adjust = " + adjust)
    if inline:
        lines.append("data = (\n" + _prior_rows(PRIOR_PCT) + "\n  )")
    else:
        lines.append('file = "%s"' % PRIOR_FILE)
    lines += ["type = " + kind, "mode = percent", "start = 1949.01"]
    if trend:
        lines.append("temppriortrend = yes")
    return block("transform", lines, print_all=False)


def _prior_spec(fname, cover, model, **kw):
    blocks = [series_airline(), transform_prior(**kw)]
    if model:
        blocks += [arima_airline(), estimate_block()]
    blocks.append(block("x11", [], save_key="x11d", print_all=True, savelog=True))
    return fname, assemble(cover, blocks)


def spec_prior_perm_file():
    # NO model: the only path where run_x11 has to build the /adjcmn/ record
    # itself (adjsrs.f) and let x11int/x11pt1 remove the prior.
    return _prior_spec(
        "airline_prior-perm-file.spc",
        "transform{file=} permanent user prior factors, no regARIMA model",
        model=False, inline=False, kind="permanent")


def spec_prior_temp():
    # Temporary factors: stripped from the irregular (D13), kept in the SA
    # series (D11) -- that is what makes them temporary (x11pt3.f:591-604).
    return _prior_spec(
        "airline_prior-temp.spc",
        "transform{type=temporary} user prior factors (Usrtad) with a model",
        model=True, inline=True, kind="temporary")


def spec_prior_temp_trend():
    # ... and folded back into the published trend as well (x11pt3.f:937-946).
    return _prior_spec(
        "airline_prior-temp-trend.spc",
        "transform{temppriortrend=yes}: temporary prior folded back into D12",
        model=True, inline=True, kind="temporary", trend=True)


def spec_prior_lom_user():
    # The predefined length-of-month prior COMBINED with a user permanent one:
    # adjsrs multiplies both into the same Adj factor series.
    return _prior_spec(
        "airline_prior-lom-user.spc",
        "transform{adjust=lom} predefined prior combined with a user permanent one",
        model=True, inline=True, kind="permanent", adjust="lom")


BUILDERS = [
    spec_spectrum,
    spec_spectrum_arspec,
    spec_history,
    spec_slidingspans,
    spec_slidingspans_cutseas,
    spec_x11regression,
    spec_x11regression_aictest,
    spec_x11regression_tdprior,
    spec_force_denton,
    spec_force_regress,
    spec_force_automdl_x11,
    spec_metadata,
    spec_pickmdl,
    spec_seats_tabtables,
    spec_seats_qmax_rmod,
    spec_outlier_lsrun,
    spec_check,
    spec_identify,
    spec_prior_perm_file,
    spec_prior_temp,
    spec_prior_temp_trend,
    spec_prior_lom_user,
]


def main():
    for fn in os.listdir(HERE):
        if fn.endswith(".spc"):
            os.remove(os.path.join(HERE, fn))

    # The prior-factor data file the no-model prior spec reads. Synthetic and
    # deterministic, so it is generated here rather than committed by hand.
    with open(os.path.join(HERE, PRIOR_FILE), "w", newline="\n") as fh:
        fh.write("\n".join(PRIOR_PCT) + "\n")

    written = []
    for builder in BUILDERS:
        fname, text = builder()
        with open(os.path.join(HERE, fname), "w", newline="\n") as fh:
            fh.write(text)
        written.append(fname)

    written.sort()
    with open(os.path.join(HERE, "MANIFEST"), "w", newline="\n") as fh:
        fh.write("# Extra spec inventory -- produced by genextra.py\n")
        fh.write("# %d specs covering blocks absent from generated/\n"
                 % len(written))
        for fname in written:
            fh.write(fname + "\n")

    print("genextra.py: wrote %d specs to %s" % (len(written), HERE))
    for fname in written:
        print("  " + fname)


if __name__ == "__main__":
    main()
