"""X13cpp plot-family prototype (matplotlib, light mode).

Consumes oracle/golden bundles via the x13compare parsers. This is the design
prototype for the eventual x13cpp Python package plotting API; the same visual
system will be mirrored in ggplot2 for the R package.

Design system (see project plan / dataviz method):
  surface #fcfcfb, ink #0b0b0b / #52514e / muted #898781,
  grid #e1e0d9, baseline #c3c2b7,
  series slots: 1 blue #2a78d6, 2 green #008300, 3 magenta #e87ba4,
                6 orange #eb6834, 8 red #e34948
  status: good #0ca30c, warning #fab219, critical #d03b3b
"""
from __future__ import annotations

import os
import sys
from datetime import datetime

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import numpy as np

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(_HERE, "..", "..", "tests", "compare"))
from x13compare import parse_save, parse_udg  # noqa: E402

# --------------------------------------------------------------------------- #
# Palette / chrome
# --------------------------------------------------------------------------- #
SURFACE = "#fcfcfb"
INK = "#0b0b0b"
INK2 = "#52514e"
MUTED = "#898781"
GRID = "#e1e0d9"
BASELINE = "#c3c2b7"

BLUE = "#2a78d6"       # slot 1 — primary series (SA)
BLUE_DARK = "#1c5cab"  # sequential 550 — emphasis of the blue family
BLUE_LIGHT = "#cde2fb"  # sequential 100 — fills
BLUE_MID = "#9ec5f4"    # sequential 200
GREEN = "#008300"      # slot 2 — trend
RED = "#e34948"        # slot 8 — flagged points
CRITICAL = "#d03b3b"   # status — failing stat
GOOD_TEXT = "#006300"  # success text on light

FONT = ["Segoe UI", "DejaVu Sans", "sans-serif"]

RC = {
    "figure.facecolor": SURFACE,
    "axes.facecolor": SURFACE,
    "savefig.facecolor": SURFACE,
    "font.family": "sans-serif",
    "font.sans-serif": FONT,
    "text.color": INK,
    "axes.edgecolor": BASELINE,
    "axes.labelcolor": INK2,
    "axes.titlecolor": INK,
    "xtick.color": MUTED,
    "ytick.color": MUTED,
    "xtick.labelsize": 9,
    "ytick.labelsize": 9,
    "axes.labelsize": 10,
    "axes.grid": True,
    "grid.color": GRID,
    "grid.linewidth": 0.8,
    "axes.axisbelow": True,
    "axes.spines.top": False,
    "axes.spines.right": False,
    "legend.frameon": False,
    "legend.fontsize": 9,
    "axes.titlesize": 13,
    "axes.titleweight": "bold",
    "axes.titlelocation": "left",
}


def _apply_style():
    plt.rcParams.update(RC)


def _finish(ax):
    ax.grid(axis="x", visible=False)
    for s in ("bottom", "left"):
        ax.spines[s].set_linewidth(0.9)


def _title(fig, title, subtitle):
    fig.suptitle(title, x=0.06, y=0.975, ha="left", fontsize=14,
                 fontweight="bold", color=INK)
    fig.text(0.06, 0.905, subtitle, ha="left", fontsize=10, color=INK2)


# --------------------------------------------------------------------------- #
# Bundle access
# --------------------------------------------------------------------------- #
def _period_to_date(period: str) -> datetime:
    p = str(period)
    if len(p) == 6:                       # YYYYMM
        return datetime(int(p[:4]), int(p[4:6]), 1)
    if len(p) == 4:                       # YYYY
        return datetime(int(p), 1, 1)
    raise ValueError(f"unrecognized period {period!r}")


class Bundle:
    """Read save tables / udg from an oracle output bundle directory."""

    def __init__(self, path: str):
        self.path = path
        self.base = None
        for f in os.listdir(path):
            if f.endswith(".out"):
                self.base = f[:-4]
        if self.base is None:
            raise FileNotFoundError(f"no .out in {path}")

    def table(self, code: str):
        """Return (dates, values) for a saved table as (list, 2-D array).

        Prefers the multi-column ``rows`` mapping (e.g. .fct has
        forecast/lowerci/upperci). X-13 encodes missing values as the
        sentinel -999.0 — mapped to NaN here.
        """
        f = os.path.join(self.path, f"{self.base}.{code}")
        parsed = parse_save.parse_save(open(f, encoding="utf-8").read())
        rows = getattr(parsed, "rows", None)
        if rows:
            items = sorted(rows.items())
        else:
            items = sorted(parsed.values.items())
        dates = [_period_to_date(k) for k, _ in items]
        vals = np.array([v if isinstance(v, (list, tuple)) else [v]
                         for _, v in items], dtype=float)
        vals[np.isclose(vals, -999.0)] = np.nan
        return dates, vals

    def udg(self) -> dict:
        f = os.path.join(self.path, f"{self.base}.udg")
        return parse_udg.parse_udg(open(f, encoding="utf-8").read())

    @property
    def period(self) -> int:
        try:
            return int(self.udg().get("freq", 12))
        except Exception:
            return 12


MONTHS = ["Jan", "Feb", "Mar", "Apr", "May", "Jun",
          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"]


# --------------------------------------------------------------------------- #
# 1. Overview: original, SA, trend
# --------------------------------------------------------------------------- #
def plot_overview(bundle: Bundle, title, subtitle, out_png):
    _apply_style()
    d_o, v_o = bundle.table("a1")
    d_sa, v_sa = bundle.table("d11")
    d_tr, v_tr = bundle.table("d12")

    fig, ax = plt.subplots(figsize=(9.6, 4.6), dpi=150)
    fig.subplots_adjust(top=0.86, left=0.09, right=0.97, bottom=0.11)
    ax.plot(d_o, v_o[:, 0], color=MUTED, lw=1.1, label="Original (A1)")
    ax.plot(d_sa, v_sa[:, 0], color=BLUE, lw=1.9,
            label="Seasonally adjusted (D11)")
    ax.plot(d_tr, v_tr[:, 0], color=GREEN, lw=1.6, label="Trend-cycle (D12)")
    ax.legend(loc="upper left", ncols=3, bbox_to_anchor=(0.0, 1.02))
    ax.yaxis.set_major_formatter(
        matplotlib.ticker.FuncFormatter(lambda v, _: f"{v:,.0f}"))
    ax.xaxis.set_major_locator(mdates.AutoDateLocator())
    _finish(ax)
    _title(fig, title, subtitle)
    fig.savefig(out_png)
    plt.close(fig)


# --------------------------------------------------------------------------- #
# 2. Seasonal factor subseries (per-period panels)
# --------------------------------------------------------------------------- #
def plot_seasonal_subseries(bundle: Bundle, title, subtitle, out_png):
    _apply_style()
    d10_d, d10_v = bundle.table("d10")
    per = bundle.period
    months = [d.month if per == 12 else (d.month - 1) // 3 + 1 for d in d10_d]
    years = [d.year for d in d10_d]
    labels = MONTHS if per == 12 else [f"Q{i}" for i in range(1, 5)]

    fig, axes = plt.subplots(1, per, figsize=(11.5, 4.2), dpi=150,
                             sharey=True)
    fig.subplots_adjust(top=0.80, left=0.07, right=0.985, bottom=0.10,
                        wspace=0.06)
    mean_all = np.nanmean(d10_v[:, 0])
    if mean_all > 50:          # multiplicative, percent scale (~100)
        ref = 100.0
    elif 0.5 < mean_all < 2:   # multiplicative, ratio scale (~1)
        ref = 1.0
    else:                      # additive (~0)
        ref = 0.0
    lo = np.nanmin(d10_v[:, 0]); hi = np.nanmax(d10_v[:, 0])
    pad = 0.08 * (hi - lo) if hi > lo else 0.01
    axes[0].set_ylim(min(lo, ref) - pad, max(hi, ref) + pad)
    for m in range(1, per + 1):
        ax = axes[m - 1]
        yy = [v for mm, v in zip(months, d10_v[:, 0]) if mm == m]
        xx = [y for mm, y in zip(months, years) if mm == m]
        ax.axhline(ref, color=BASELINE, lw=0.9)
        ax.plot(xx, yy, color=BLUE, lw=1.6)
        ax.axhline(np.nanmean(yy), color=BLUE_DARK, lw=1.0, ls=(0, (4, 3)))
        ax.set_title(labels[m - 1], fontsize=9, fontweight="normal",
                     color=INK2, loc="center")
        ax.set_xticks([])
        ax.grid(visible=False)
        for s in ("bottom", "left"):
            ax.spines[s].set_linewidth(0.9)
        if m > 1:
            ax.spines["left"].set_visible(False)
            ax.tick_params(left=False)
    _title(fig, title, subtitle)
    fig.savefig(out_png)
    plt.close(fig)


# --------------------------------------------------------------------------- #
# 3. SI ratios with replacement + seasonal factors
# --------------------------------------------------------------------------- #
def plot_si_ratios(bundle: Bundle, title, subtitle, out_png):
    _apply_style()
    d8_d, d8_v = bundle.table("d8")     # unmodified SI
    d9_d, d9_v = bundle.table("d9")     # replaced SI (sparse: mostly missing)
    d10_d, d10_v = bundle.table("d10")  # seasonal factors
    per = bundle.period
    labels = MONTHS if per == 12 else [f"Q{i}" for i in range(1, 5)]

    d9_map = {d: v for d, v in zip(d9_d, d9_v[:, 0]) if np.isfinite(v)}

    fig, axes = plt.subplots(1, per, figsize=(11.5, 4.2), dpi=150,
                             sharey=True)
    fig.subplots_adjust(top=0.80, left=0.07, right=0.985, bottom=0.10,
                        wspace=0.06)
    fin = d8_v[:, 0][np.isfinite(d8_v[:, 0])]
    lo, hi = fin.min(), fin.max()
    pad = 0.08 * (hi - lo)
    axes[0].set_ylim(lo - pad, hi + pad)
    for m in range(1, per + 1):
        ax = axes[m - 1]
        si = [(d.year, v) for d, v in zip(d8_d, d8_v[:, 0])
              if (d.month if per == 12 else (d.month - 1) // 3 + 1) == m]
        sf = [(d.year, v) for d, v in zip(d10_d, d10_v[:, 0])
              if (d.month if per == 12 else (d.month - 1) // 3 + 1) == m]
        rep = [(d.year, v) for d, v in d9_map.items()
               if (d.month if per == 12 else (d.month - 1) // 3 + 1) == m]
        ax.scatter([x for x, _ in si], [y for _, y in si], s=7, color=MUTED,
                   linewidths=0)
        ax.plot([x for x, _ in sf], [y for _, y in sf], color=BLUE, lw=1.6)
        if rep:
            ax.scatter([x for x, _ in rep], [y for _, y in rep], s=16,
                       color=RED, linewidths=0, zorder=3)
        ax.set_title(labels[m - 1], fontsize=9, fontweight="normal",
                     color=INK2, loc="center")
        ax.set_xticks([])
        ax.grid(visible=False)
        if m > 1:
            ax.spines["left"].set_visible(False)
            ax.tick_params(left=False)
    _title(fig, title, subtitle)
    fig.savefig(out_png)
    plt.close(fig)


# --------------------------------------------------------------------------- #
# 4. Forecast fan
# --------------------------------------------------------------------------- #
def plot_forecast(bundle: Bundle, title, subtitle, out_png, history=48):
    _apply_style()
    a1_d, a1_v = bundle.table("a1")
    f_d, f_v = bundle.table("fct")      # cols: forecast, lower, upper

    fig, ax = plt.subplots(figsize=(9.6, 4.6), dpi=150)
    fig.subplots_adjust(top=0.86, left=0.09, right=0.97, bottom=0.11)
    hd, hv = a1_d[-history:], a1_v[-history:, 0]
    ax.plot(hd, hv, color=BLUE, lw=1.9, label="Observed")
    if f_v.shape[1] >= 3:
        ax.fill_between(f_d, f_v[:, 1], f_v[:, 2], color=BLUE_LIGHT,
                        lw=0, label="95% interval")
    ax.plot(f_d, f_v[:, 0], color=BLUE_DARK, lw=1.9, ls=(0, (5, 3)),
            label="Forecast")
    ax.axvline(hd[-1], color=BASELINE, lw=0.9)
    ax.legend(loc="upper left", ncols=3, bbox_to_anchor=(0.0, 1.02))
    _finish(ax)
    _title(fig, title, subtitle)
    fig.savefig(out_png)
    plt.close(fig)


# --------------------------------------------------------------------------- #
# 5. M-statistics dashboard
# --------------------------------------------------------------------------- #
M_DESC = {
    "m01": "Irregular contribution",
    "m02": "Irregular in trend",
    "m03": "I/C ratio",
    "m04": "Irregular autocorrelation",
    "m05": "MCD months",
    "m06": "I/S yearly",
    "m07": "Moving seasonality",
    "m08": "Seasonal fluctuation",
    "m09": "Seasonal linear movement",
    "m10": "Recent seasonal fluct.",
    "m11": "Recent seasonal movement",
}


def plot_mstats(bundle: Bundle, title, subtitle, out_png):
    _apply_style()
    u = bundle.udg()
    keys = [f"m{i:02d}" for i in range(1, 12)]
    vals = []
    for k in keys:
        v = u.get(f"f3.{k}")
        vals.append(float(v) if v is not None else np.nan)
    q = float(u.get("f3.q", np.nan))

    fig, ax = plt.subplots(figsize=(9.6, 5.0), dpi=150)
    fig.subplots_adjust(top=0.85, left=0.30, right=0.93, bottom=0.09)
    y = np.arange(len(keys))[::-1]
    colors = [CRITICAL if v >= 1.0 else BLUE for v in vals]
    ax.barh(y, vals, height=0.62, color=colors, zorder=3)
    ax.axvline(1.0, color=INK2, lw=1.1, zorder=4)
    ax.text(1.0, len(keys) - 0.1, " acceptance limit", color=INK2,
            fontsize=8.5, va="bottom", ha="left")
    ax.set_yticks(y)
    ax.set_yticklabels([f"{k.upper()}  ·  {M_DESC[k]}" for k in keys],
                       fontsize=9, color=INK)
    for yi, v in zip(y, vals):
        if np.isfinite(v):
            ax.text(v + 0.04, yi, f"{v:.2f}", va="center", fontsize=8.5,
                    color=(CRITICAL if v >= 1.0 else INK2), zorder=5)
    ax.set_xlim(0, max(3.2, np.nanmax(vals) * 1.12))
    ax.grid(axis="y", visible=False)
    ax.grid(axis="x", visible=True)
    ax.spines["left"].set_visible(False)
    ax.tick_params(left=False)
    q_col = GOOD_TEXT if q < 1.0 else CRITICAL
    verdict = "accepted" if q < 1.0 else "not accepted"
    fig.text(0.93, 0.90, f"Q = {q:.2f}  ({verdict})", ha="right", fontsize=11,
             color=q_col, fontweight="bold")
    _title(fig, title, subtitle)
    fig.savefig(out_png)
    plt.close(fig)
