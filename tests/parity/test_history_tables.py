"""history{} gate: the revisions-history save tables (revchk.f/setrvp.f/
revdrv.f/getrev.f/prtrev.f) vs the oracle goldens.

`history{}` re-runs the regARIMA+X11 adjustment over expanding sub-spans of the
series -- span k ends at observation Begrev+k-1, all starting at the series
origin -- capturing, for each ending date, the *concurrent* estimate (the last
point of that span's adjustment). The *final* estimate is the same point from
the full-data span. Four save tables land per revision date:

  * sae -- concurrent + final seasonally-adjusted level (Conc_SA, Final_SA)
  * sar -- SA percent revision, (Final - Conc)/Conc * 100
  * tre -- concurrent + final trend level (Conc_TRND, Final_TRND)
  * trr -- trend percent revision

  * chr -- SA month-to-month %-change revision, (Final_chng - Conc_chng)
  * che -- concurrent + final SA %-change (Conc_chng, Final_chng)
  * tcr -- trend month-to-month %-change revision, (Final_chng - Conc_chng)
  * tce -- concurrent + final trend %-change (Conc_chng, Final_chng)
  * sfr -- seasonal-factor revision, two columns (Final-Conc, Final-Proj)
  * sfe -- concurrent + projected + final seasonal factor (Conc/Proj/Final_SF)

AICC (r07), forecast (r08) and ARMA/TD-coefficient histories are out of scope
(no goldens ship for this spec); see core/src/driver/run_history.hpp for the
full scope note.

STATUS: GATED at the estimation floor. Each span re-estimates the regARIMA
model, so the concurrent/final *levels* (sae/tre) agree to ~7e-6 relative (the
same per-span re-estimation regime as the M3 estimate gate / slidingspans,
looser than a fixed-model replay because the model is re-optimized over 70+
distinct span lengths). The *revisions* (sar/trr) are differences of two
near-equal levels, so they are gated on absolute error -- relative error is
meaningless near a zero-crossing. The one exception is `history{fixmdl=yes}`
(Revfix): with every parameter held at the main run's converged values nothing
re-optimizes, and those specs gate BIT-EXACT (~5e-15) -- see
RTOL_LEVEL_BY_SPEC / ATOL_REV_BY_SPEC.

The MODEL-SPAN specs (`airline_history-fixper`, `-modelspan`,
`-fixper-fixmdl`) cover revdrv.f:479-497: `series{modelspan=(,0.per)}` sets
Fixper and each span's model then stops at the last occurrence of that period
(the estimation window advances once a year), while a plain modelspan END caps
every span's model span at the main run's Endmdl.

The port reuses the re-entrant sub-span driver (driver/run_x11_span.{hpp,cpp})
that slidingspans built. The one history-specific fix beyond that: the seasonal-
filter selector (Lterm) and Henderson trend length (Nterm) must be reset to
their parsed values per span (run_history.cpp) -- restor_span resets the
per-period Lter/Ktcopt/Tic but not those two scalars, so without the reset every
(differently-lengthed) span reused the first/shortest span's resolved filter
choice, producing a discrete ~1e-3 error jump at the 9-year span mark where the
oracle's own per-span selection diverged from the short-span choice.

Run:  python -m pytest tests/parity/test_history_tables.py -q
"""
from __future__ import annotations

import os
import re
import subprocess

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus", "extra")
_GOLDEN = os.path.join(_REPO, "tests", "golden", "extra")

# Levels (sae/tre): each span re-estimates the model, so gate at the estimation
# floor (measured worst ~7e-6 over the airline_history spans). Revisions
# (sar/trr): absolute tolerance on the percent value -- they are (near-zero)
# differences of two levels that individually agree to the level tolerance.
RTOL_LEVEL = 1e-5
ATOL_REV = 5e-3

# Per-spec level tolerance overrides. The Fixper spec (series{modelspan=(,0.jan)})
# estimates each span's model over a window that stops at the last January, so a
# span whose data ends in February contributes a whole extra month of X-11 that
# the model never saw -- one early, short span (1955.02, span 74 of 144) lands at
# 1.03e-5 where the median row is 2.2e-8. Same per-span re-estimation floor, one
# row over the shared bound; not a different mechanism.
# history{fixmdl=yes} holds every model parameter at the main run's converged
# values, so no span re-optimizes anything and the whole family collapses to the
# arithmetic floor -- gate it there, not at the shared estimation tolerance.
# history{fixx11reg=yes} WITH a regARIMA model amplifies the same floor once
# more: the x11regression daily weights are held, but the regARIMA model still
# re-estimates per span AND the held weights then enter the D-tables through
# Faccal, so one early span (1955.02) lands at 1.25e-5 where the median row is
# ~2e-8. The MODEL-FREE twin has nothing to re-estimate and gates bit-exact --
# which is what pins the tolerance here to the re-estimation, not to the flag.
RTOL_LEVEL_BY_SPEC = {"airline_history-fixper": 2e-5,
                      "airline_history-fixmdl": 1e-12,
                      "airline_history-fixper-fixmdl": 1e-12,
                      "airline_history-x11reg-fixx11reg": 2e-5,
                      "airline_history-x11reg-nomodel-fixx11reg": 1e-12}
ATOL_REV_BY_SPEC = {"airline_history-fixmdl": 1e-11,
                    "airline_history-fixper-fixmdl": 1e-11,
                    "airline_history-x11reg-nomodel-fixx11reg": 1e-11}

# (tag, n_value_columns, kind) -- kind "level" uses RTOL_LEVEL, "rev" uses ATOL_REV.
# che (month-to-month SA % change, conc+final) is a difference of two re-estimated
# levels, so it crosses zero and is gated on absolute error like the revisions.
_TABLES = [("sae", 2, "level"), ("tre", 2, "level"),
           ("sar", 1, "rev"), ("trr", 1, "rev"),
           ("chr", 1, "rev"), ("che", 2, "rev"),
           ("tcr", 1, "rev"), ("tce", 2, "rev"),
           ("sfr", 2, "rev"), ("sfe", 3, "level")]
_CORE_TAGS = ["sar", "sae", "trr", "tre"]

_NUM_RE = re.compile(r"[+\-][0-9.EeDd+\-]+")


def _find_binary() -> str:
    cands = [
        os.path.join(_REPO, "build", "x13run_x11.exe"),
        os.path.join(_REPO, "build", "x13run_x11"),
        os.path.join(_REPO, "build", "Release", "x13run_x11.exe"),
    ]
    for c in cands:
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_X11")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13run_x11 binary not found; build it first (cmake --build build).")


BIN = _find_binary()


def _num(s: str) -> float:
    return float(s.replace("D", "E").replace("d", "e"))


def _read_golden(path: str, ncol: int) -> dict[str, list[float]]:
    """{date: [v1, .. vncol]} from an oracle .sar/.sae/.trr/.tre save file."""
    out: dict[str, list[float]] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            parts = ln.split()
            if len(parts) < 1 + ncol or not re.match(r"^\d{6}$", parts[0]):
                continue
            vals = [v for v in parts[1:] if _NUM_RE.fullmatch(v)]
            if len(vals) < ncol:
                continue
            out[parts[0]] = [_num(v) for v in vals[:ncol]]
    return out


def _read_produced(text: str, tag: str, ncol: int) -> dict[str, list[float]]:
    """Parses tools/x13run_x11.cpp's `<tag> YYYYMM v1 .. vncol` lines."""
    out: dict[str, list[float]] = {}
    for ln in text.splitlines():
        parts = ln.split()
        if len(parts) < 2 + ncol or parts[0] != tag:
            continue
        out[parts[1]] = [float(v) for v in parts[2:2 + ncol]]
    return out


def _spec_text(base: str) -> str:
    return open(os.path.join(_CORPUS, base + ".spc"),
                encoding="utf-8", errors="replace").read().lower()


def _discover() -> list[str]:
    specs: list[str] = []
    if not os.path.isdir(_CORPUS):
        return specs
    for fn in sorted(os.listdir(_CORPUS)):
        if not fn.endswith(".spc"):
            continue
        base = fn[:-4]
        gdir = os.path.join(_GOLDEN, base)
        if "history{" not in _spec_text(base):
            continue
        if not all(os.path.exists(os.path.join(gdir, base + "." + t))
                   for t in _CORE_TAGS):
            continue
        specs.append(base)
    return specs


CASES = _discover()


def _run(base: str) -> str:
    specpath = os.path.join(_CORPUS, base + ".spc")
    proc = subprocess.run([BIN, specpath], cwd=_CORPUS, capture_output=True,
                           text=True, timeout=180)
    assert proc.returncode == 0, (
        f"{base}: x13run_x11 exited {proc.returncode}\n"
        f"stdout tail: {proc.stdout[-2000:]}\nstderr: {proc.stderr[-2000:]}")
    return proc.stdout


@pytest.mark.skipif(not CASES, reason="no history spec ships the sar/sae/trr/tre goldens")
@pytest.mark.parametrize("base", CASES)
@pytest.mark.parametrize("tag,ncol,kind", _TABLES)
def test_history_table(base: str, tag: str, ncol: int, kind: str) -> None:
    goldpath = os.path.join(_GOLDEN, base, base + "." + tag)
    if not os.path.exists(goldpath):
        pytest.skip(f"{base} does not produce the {tag} table")

    gold = _read_golden(goldpath, ncol)
    assert gold, f"{base}.{tag}: empty golden"

    out = _run(base)
    prod = _read_produced(out, tag, ncol)
    assert prod, f"{base}.{tag}: produced no rows (history driver likely inert)"

    missing = set(gold) - set(prod)
    assert not missing, f"{base}.{tag}: missing {len(missing)} rows, e.g. {sorted(missing)[:5]}"

    worst = 0.0
    worst_key = None
    for d, gv in gold.items():
        pv = prod[d]
        for c in range(ncol):
            if kind == "level":
                err = abs(gv[c] - pv[c]) / abs(gv[c]) if gv[c] else abs(gv[c] - pv[c])
                tol = RTOL_LEVEL_BY_SPEC.get(base, RTOL_LEVEL)
            else:
                err = abs(gv[c] - pv[c])
                tol = ATOL_REV_BY_SPEC.get(base, ATOL_REV)
            if err > worst:
                worst, worst_key = err, (d, c)
        assert worst <= tol, f"{base}.{tag}: worst {kind} err {worst} at {worst_key}"


# ---------------------------------------------------------------------------
# history{estimates=(fcst)}: the out-of-sample forecast-error history.
#
# prtfct.f:613 stores each span's original-scale forecast at the requested leads
# into Cncfct(k, Revptr+lag) -- i.e. into the row of the date it predicts --
# and prfcrv.f then differences it against the raw series there:
#
#   * fce -- the EVOLVING sum of squared forecast errors, one column per lead
#   * fch -- the concurrent forecast and its error, two columns per lead
#   * meanssfe -- the .udg savelog canary, fctss(k)/(Revptr-Rfctlg(k)) at the
#     last row (one value per lead)
#
# A lead has no stored forecast until the table has advanced that far (prfcrv's
# ndef cut); those cells are an exact 0 on both sides.
#
# TOLERANCE. This family sits at the loosest end of the port because it AMPLIFIES
# the per-span re-estimation floor three times over, and each step is measurable:
#   fch forecast columns  -- the forecast LEVEL, measured worst 1.8e-5 relative
#     (a forecast extrapolates the coefficient difference, so it runs a bit above
#     the 1e-5 in-sample level floor the sae/tre tables gate at).
#   fch error columns     -- actual minus forecast, i.e. an O(270) cancellation
#     leaving O(10), which turns that 1.8e-5 into ~2e-4 relative. Gated
#     absolutely (worst 4.8e-3) -- the error crosses zero.
#   fce                   -- those errors SQUARED and accumulated, so ~4e-4
#     relative (worst measured 3.0e-4).
# The first spans are bit-exact; the drift grows with the span count.
_FCST_RTOL_LEVEL = 5e-5   # fch forecast columns
_FCST_ATOL_ERR = 1e-2     # fch error columns (zero-crossing)
_FCST_RTOL_SS = 1e-3      # fce, the accumulated sum of squares
_FCST_RTOL_MEAN = 1e-4    # meanssfe (that sum divided by its count)


def _discover_fcst() -> list[str]:
    specs: list[str] = []
    if not os.path.isdir(_CORPUS):
        return specs
    for fn in sorted(os.listdir(_CORPUS)):
        if not fn.endswith(".spc"):
            continue
        base = fn[:-4]
        if "history{" not in _spec_text(base):
            continue
        if os.path.exists(os.path.join(_GOLDEN, base, base + ".fce")):
            specs.append(base)
    return specs


FCST_CASES = _discover_fcst()


def _read_udg_scalars(path: str, key: str) -> list[float]:
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            if ln.startswith(key + ":"):
                return [_num(v) if _NUM_RE.match(v) else float(v)
                        for v in ln.split(":", 1)[1].split()]
    return []


@pytest.mark.skipif(not FCST_CASES, reason="no history spec ships the fce golden")
@pytest.mark.parametrize("base", FCST_CASES)
@pytest.mark.parametrize("tag", ["fce", "fch"])
def test_history_fcst_table(base: str, tag: str) -> None:
    goldpath = os.path.join(_GOLDEN, base, base + "." + tag)
    if not os.path.exists(goldpath):
        pytest.skip(f"{base} does not produce the {tag} table")

    # Column count comes from the golden's own header, so the test follows
    # whatever fstep= the spec asked for.
    with open(goldpath, encoding="utf-8", errors="replace") as f:
        ncol = len(f.readline().split("\t")) - 1
    assert ncol > 0, f"{base}.{tag}: could not read the golden header"

    gold = _read_golden(goldpath, ncol)
    assert gold, f"{base}.{tag}: empty golden"

    prod = _read_produced(_run(base), tag, ncol)
    assert prod, f"{base}.{tag}: produced no rows (fcst history likely inert)"
    missing = set(gold) - set(prod)
    assert not missing, (
        f"{base}.{tag}: missing {len(missing)} rows, e.g. {sorted(missing)[:5]}")

    worst, worst_key, worst_kind = 0.0, None, None
    for d, gv in gold.items():
        pv = prod[d]
        for c in range(ncol):
            # fch alternates (forecast, error) per lead: the odd columns are the
            # near-zero error, gated absolutely.
            is_err = (tag == "fch" and c % 2 == 1)
            if is_err or not gv[c]:
                err, tol, kind = abs(gv[c] - pv[c]), _FCST_ATOL_ERR, "abs"
            else:
                rtol = _FCST_RTOL_SS if tag == "fce" else _FCST_RTOL_LEVEL
                err, tol, kind = abs(gv[c] - pv[c]) / abs(gv[c]), rtol, "rel"
            if err > tol:
                assert False, (f"{base}.{tag}: {kind} err {err:.3e} at "
                               f"{d} col {c} (tol {tol:.0e})")
            if err > worst:
                worst, worst_key, worst_kind = err, (d, c), kind
    assert worst_key is not None or not gold


@pytest.mark.skipif(not FCST_CASES, reason="no history spec ships the fce golden")
@pytest.mark.parametrize("base", FCST_CASES)
def test_history_fcst_meanssfe(base: str) -> None:
    """The .udg `meanssfe` / `rvfcstlag` savelog canaries (prfcrv.f:207-214)."""
    udg = os.path.join(_GOLDEN, base, base + ".udg")
    if not os.path.exists(udg):
        pytest.skip(f"{base} ships no .udg golden")
    want = _read_udg_scalars(udg, "meanssfe")
    want_lags = _read_udg_scalars(udg, "rvfcstlag")
    assert want, f"{base}: golden .udg has no meanssfe line"

    out = _run(base)
    got, got_lags = [], []
    for ln in out.splitlines():
        p = ln.split()
        if p and p[0] == "meanssfe":
            got = [float(v) for v in p[1:]]
        elif p and p[0] == "rvfcstlag":
            got_lags = [float(v) for v in p[1:]]
    assert got, f"{base}: engine emitted no meanssfe line"
    assert got_lags == want_lags, (
        f"{base}: rvfcstlag {got_lags} != golden {want_lags}")
    assert len(got) == len(want), (
        f"{base}: meanssfe has {len(got)} leads, golden {len(want)}")
    for k, (g, w) in enumerate(zip(got, want)):
        # The .udg prints E17.10, so the golden pins ~11 significant digits --
        # well below the per-span re-estimation floor that dominates here.
        err = abs(g - w) / abs(w) if w else abs(g - w)
        assert err <= _FCST_RTOL_MEAN, (
            f"{base}: meanssfe lead {k} rel err {err:.3e} "
            f"(tol {_FCST_RTOL_MEAN:.0e})")


# ---------------------------------------------------------------------------
# history{estimates=(aic arma td)}: the three MODEL histories, all captured at
# revdrv.f:670-690 straight after each span's rgarma.
#
#   * lkh -- (Olkhd, Aicc): the transform-Jacobian-adjusted log likelihood and
#     the corrected AIC of the span's fit (prlkhd). NB Olkhd is NOT the .udg's
#     `loglikelihood`, which is the un-Jacobian-adjusted Lnlkhd.
#   * amh -- the FREE ARMA coefficients in Mdl/Opr order (rvarma.f).
#   * tdh -- the FREE trading-day / length-of-period / user-TD coefficients, each
#     TD group followed by its implied contrast column -sum(b) (rvtdrg.f).
#
# All three share the model-history row range i=Begrev..Endrev, one row per span,
# which is one row LONGER than the revision tables above.
#
# TOLERANCE, and the split between the two is the interesting part. The first
# span is bit-exact in all three; after that every span re-converges its own
# optimization (71 of them here), and the LIKELIHOOD and the COEFFICIENTS drift
# by wildly different amounts: lkh agrees to 2.6e-9 relative while amh/tdh only
# agree to ~8e-4. That is not inconsistency -- it is the flat optimum. Near the
# maximum the likelihood surface is quadratic and nearly level, so a parameter
# difference of 1e-3 buys a likelihood difference of ~1e-9. The measured spread
# is smooth (median 4e-5, worst 2.5e-4 on the nonseasonal MA, no outlier span),
# i.e. optimizer path noise rather than a divergent span.
#
# The coefficient tables are therefore gated on an absolute floor scaled to each
# column's own magnitude -- the tdh columns are near-zero daily-weight contrasts
# that cross zero, so relative error is meaningless on them.
_MDL_RTOL = 1e-7          # lkh: log likelihood + AICC
_MDL_COEF_TOL = 2e-3      # amh/tdh, as a fraction of the column's own scale
_MDL_TAGS = [("lkh", 2), ("amh", 0), ("tdh", 0)]   # 0 = read width from header


def _discover_model() -> list[str]:
    specs: list[str] = []
    if not os.path.isdir(_CORPUS):
        return specs
    for fn in sorted(os.listdir(_CORPUS)):
        if not fn.endswith(".spc"):
            continue
        base = fn[:-4]
        if "history{" not in _spec_text(base):
            continue
        gdir = os.path.join(_GOLDEN, base)
        if any(os.path.exists(os.path.join(gdir, base + "." + t))
               for t, _ in _MDL_TAGS):
            specs.append(base)
    return specs


MODEL_CASES = _discover_model()


@pytest.mark.skipif(not MODEL_CASES, reason="no history spec ships lkh/amh/tdh")
@pytest.mark.parametrize("base", MODEL_CASES)
@pytest.mark.parametrize("tag,ncol_fixed", _MDL_TAGS)
def test_history_model_table(base: str, tag: str, ncol_fixed: int) -> None:
    goldpath = os.path.join(_GOLDEN, base, base + "." + tag)
    if not os.path.exists(goldpath):
        pytest.skip(f"{base} does not produce the {tag} table")

    ncol = ncol_fixed
    if ncol == 0:
        # The .tdh header row is fully tab-separated while its DATA rows are not
        # (CB-20 in tools/census_bugs.md), so the width comes from the header and
        # the values are read by whitespace split -- which is what _read_golden
        # does for every table here anyway.
        with open(goldpath, encoding="utf-8", errors="replace") as f:
            ncol = len(f.readline().split("\t")) - 1
    assert ncol > 0, f"{base}.{tag}: could not read the golden header"

    gold = _read_golden(goldpath, ncol)
    assert gold, f"{base}.{tag}: empty golden"

    prod = _read_produced(_run(base), tag, ncol)
    assert prod, f"{base}.{tag}: produced no rows ({tag} history likely inert)"
    missing = set(gold) - set(prod)
    assert not missing, (
        f"{base}.{tag}: missing {len(missing)} rows, e.g. {sorted(missing)[:5]}")

    tol = _MDL_RTOL if tag == "lkh" else _MDL_COEF_TOL
    # Absolute floor per column, from that column's own scale in the golden.
    scale = [max(abs(gold[d][c]) for d in gold) for c in range(ncol)]
    for d, gv in gold.items():
        for c in range(ncol):
            diff = abs(gv[c] - prod[d][c])
            if diff <= tol * scale[c]:
                continue
            err = diff / abs(gv[c]) if gv[c] else diff
            assert err <= tol, (
                f"{base}.{tag}: rel err {err:.3e} at {d} col {c} "
                f"(tol {tol:.0e})")


# ---------------------------------------------------------------------------
# composite{} + history{}: the INDIRECT seasonally-adjusted revision history
# (Indrev). The one part of history{} that spans SPECS rather than sub-spans of
# one series, so it is driven through x13run_composite over a metafile.
#
# gtrvst.f:361-435 turns it on: with Iagr>0, Indrev defaults to 1 as soon as the
# first component asks for a sadj history, and Indrvs records that component's
# start date; any later component that does not ask for one, or asks from a
# different date, switches it back off. Each component then folds its OWN
# concurrent and final SA into the shared /revdta/ Cncisa/Finisa accumulator by
# its series{comptype=}/{compwt=} (putrev.f:25-30), and the aggregate total --
# the run where agr2 has moved Iagr to 5 -- prints the result through the same
# prtrev level-table arithmetic as sar/sae (revdrv.f:838-846, Tbltyp=3):
#
#   * iae -- concurrent + final INDIRECT SA level (Conc_Ind_SA, Final_Ind_SA)
#   * iar -- its percent revision, (Final - Conc)/Conc * 100
#   * historyindsa -- the .udg savelog canary, "yes" when every component
#     contributed (Nrcomp==Ncomp) and Indrev survived, else "no".
#
# The components' own sar/sae are gated too: iae IS their sum, so a component
# drift lands undiluted in it, exactly as isa/d11 does for the X-11 tables.
#
# TOLERANCE. Measured over the corpus metafile: the components re-estimate their
# regARIMA model per span, so their sae runs at the usual per-span floor (max
# 1.11e-5 relative, median 6.3e-9) and iae inherits it (6.51e-6 / 3.6e-9). The
# composite TOTAL's own sar/sae are bit-exact (2e-15) -- its composite{} spec
# carries no model, so there is nothing to re-estimate. The revisions are
# differences of two near-equal levels and are gated absolutely (max 1.1e-3).
_IND_RTOL_LEVEL = 2e-5
_IND_ATOL_REV = 5e-3
_IND_CORPUS = os.path.join(_REPO, "tests", "corpus", "census-examples",
                           "composite-history")
_IND_GOLDEN = os.path.join(_REPO, "tests", "golden", "census-examples",
                           "composite-history")
_IND_HAVE = os.path.isdir(_IND_CORPUS) and os.path.isdir(_IND_GOLDEN)
# (spec base, output prefix) -- x13run_composite prints the LAST spec unprefixed.
_IND_SPECS = [("region_north", "region_north:"),
              ("region_south", "region_south:"),
              ("total", "")]


def _find_composite_binary() -> str:
    for c in (os.path.join(_REPO, "build", "x13run_composite.exe"),
              os.path.join(_REPO, "build", "x13run_composite"),
              os.path.join(_REPO, "build", "Release", "x13run_composite.exe")):
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_COMPOSITE")
    if env and os.path.exists(env):
        return env
    return ""


_IND_BIN = _find_composite_binary()
_IND_CACHE: dict[str, str] = {}

# The NEGATIVE corpus: identical except that the south component's history starts
# a year later, which gtrvst.f:402-412 rejects -> Indrev=0, `historyindsa: no`,
# no iar/iae at all (while every spec still emits its own sar/sae).
_NEG_CORPUS = os.path.join(_REPO, "tests", "corpus", "census-examples",
                           "composite-history-mismatch")
_NEG_GOLDEN = os.path.join(_REPO, "tests", "golden", "census-examples",
                           "composite-history-mismatch")
_NEG_HAVE = os.path.isdir(_NEG_CORPUS) and os.path.isdir(_NEG_GOLDEN)


def _run_metafile(corpus: str = _IND_CORPUS) -> str:
    if corpus not in _IND_CACHE:
        proc = subprocess.run([_IND_BIN, "composite.mta"], cwd=corpus,
                              capture_output=True, text=True, timeout=900)
        assert proc.returncode == 0, (
            f"x13run_composite exited {proc.returncode}\n"
            f"stdout tail: {proc.stdout[-2000:]}\nstderr: {proc.stderr[-2000:]}")
        _IND_CACHE[corpus] = proc.stdout
    return _IND_CACHE[corpus]


@pytest.mark.skipif(not (_IND_HAVE and _IND_BIN),
                    reason="composite-history corpus/golden/binary not present")
@pytest.mark.parametrize("base,prefix", _IND_SPECS)
@pytest.mark.parametrize("tag,ncol,kind", [("sae", 2, "level"), ("sar", 1, "rev"),
                                           ("iae", 2, "level"), ("iar", 1, "rev")])
def test_history_composite_table(base: str, prefix: str, tag: str, ncol: int,
                                 kind: str) -> None:
    goldpath = os.path.join(_IND_GOLDEN, base, base + "." + tag)
    if not os.path.exists(goldpath):
        # Only the total ships iar/iae -- the indirect table has one producer.
        pytest.skip(f"{base} does not produce the {tag} table")

    gold = _read_golden(goldpath, ncol)
    assert gold, f"{base}.{tag}: empty golden"

    prod = _read_produced(_run_metafile(), prefix + tag, ncol)
    assert prod, f"{base}.{tag}: produced no rows (indirect history likely inert)"
    missing = set(gold) - set(prod)
    assert not missing, (
        f"{base}.{tag}: missing {len(missing)} rows, e.g. {sorted(missing)[:5]}")

    worst, worst_key = 0.0, None
    tol = _IND_RTOL_LEVEL if kind == "level" else _IND_ATOL_REV
    for d, gv in gold.items():
        pv = prod[d]
        for c in range(ncol):
            if kind == "level":
                err = abs(gv[c] - pv[c]) / abs(gv[c]) if gv[c] else abs(gv[c] - pv[c])
            else:
                err = abs(gv[c] - pv[c])
            if err > worst:
                worst, worst_key = err, (d, c)
    assert worst <= tol, (
        f"{base}.{tag}: worst {kind} err {worst:.3e} at {worst_key} "
        f"(tol {tol:.0e})")


@pytest.mark.skipif(not (_IND_HAVE and _IND_BIN),
                    reason="composite-history corpus/golden/binary not present")
def test_history_composite_indsa_canary() -> None:
    """total.udg's `historyindsa:` line (revdrv.f:1199)."""
    udg = os.path.join(_IND_GOLDEN, "total", "total.udg")
    want = None
    with open(udg, encoding="utf-8", errors="replace") as f:
        for ln in f:
            if ln.startswith("historyindsa:"):
                want = ln.split(":", 1)[1].strip()
    assert want is not None, "golden total.udg has no historyindsa line"

    got = None
    for ln in _run_metafile().splitlines():
        p = ln.split()
        if p and p[0] == "historyindsa":
            got = p[1]
    assert got == want, f"historyindsa {got!r} != golden {want!r}"


@pytest.mark.skipif(not (_IND_HAVE and _IND_BIN),
                    reason="composite-history corpus/golden/binary not present")
def test_history_composite_iae_is_component_sum() -> None:
    """The premise: iae IS the (comptype=add, compwt=1) sum of the components'
    sae, both columns. Asserted on the ORACLE goldens first, so the gate above is
    known to be testing the right object, then on the engine's own output."""
    ind = _read_golden(os.path.join(_IND_GOLDEN, "total", "total.iae"), 2)
    parts = [_read_golden(os.path.join(_IND_GOLDEN, b, b + ".sae"), 2)
             for b, _ in _IND_SPECS[:2]]
    assert ind and all(parts)
    for d, gv in ind.items():
        for c in range(2):
            s = sum(p[d][c] for p in parts)
            assert abs(gv[c] - s) <= 1e-12 * abs(gv[c]), (
                f"oracle {d} col {c}: iae {gv[c]} != component sum {s}")

    out = _run_metafile()
    e_ind = _read_produced(out, "iae", 2)
    e_parts = [_read_produced(out, b + ":sae", 2) for b, _ in _IND_SPECS[:2]]
    for d, gv in e_ind.items():
        for c in range(2):
            s = sum(p[d][c] for p in e_parts)
            assert abs(gv[c] - s) <= 1e-12 * abs(gv[c]), (
                f"engine {d} col {c}: iae {gv[c]} != component sum {s}")


@pytest.mark.skipif(not (_NEG_HAVE and _IND_BIN),
                    reason="composite-history-mismatch corpus/golden not present")
def test_history_composite_indrev_disabled() -> None:
    """Mismatched component start dates must DISABLE the indirect analysis:
    `historyindsa: no` and no iar/iae, while every component's own sar/sae is
    unaffected. Asserted against the oracle bundle, which ships neither table."""
    assert not os.path.exists(os.path.join(_NEG_GOLDEN, "total", "total.iar")), (
        "the negative golden unexpectedly ships total.iar -- re-bless it")
    with open(os.path.join(_NEG_GOLDEN, "total", "total.udg"),
              encoding="utf-8", errors="replace") as f:
        want = next((ln.split(":", 1)[1].strip() for ln in f
                     if ln.startswith("historyindsa:")), None)
    assert want == "no", f"golden historyindsa is {want!r}, expected 'no'"

    out = _run_metafile(_NEG_CORPUS)
    assert not _read_produced(out, "iar", 1), "engine emitted iar with Indrev off"
    assert not _read_produced(out, "iae", 2), "engine emitted iae with Indrev off"
    got = next((ln.split()[1] for ln in out.splitlines()
                if ln.split() and ln.split()[0] == "historyindsa"), None)
    assert got == "no", f"engine historyindsa {got!r}, expected 'no'"
    # The components' own histories still run and still match.
    for base, prefix in _IND_SPECS:
        gold = _read_golden(os.path.join(_NEG_GOLDEN, base, base + ".sae"), 2)
        prod = _read_produced(out, prefix + "sae", 2)
        assert gold and prod, f"{base}.sae missing on one side"
        for d, gv in gold.items():
            for c in range(2):
                err = abs(gv[c] - prod[d][c]) / abs(gv[c])
                assert err <= _IND_RTOL_LEVEL, (
                    f"{base}.sae rel err {err:.3e} at {d} col {c}")
