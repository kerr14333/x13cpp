"""check{} gate: the regARIMA residual diagnostics (arima.f:1044-1102) vs the
oracle's own `.udg` savelog lines.

WHAT THIS COVERS. Everything the oracle reports about the fitted model's
residuals, none of which existed in this port before:

  * ``qlimit`` / ``acflimit``        -- the two significance thresholds
  * ``nlbq`` / ``lbq$NN`` / ``lblags``   -- Ljung-Box Q at every flagged lag
  * ``nbpq`` / ``bpq$NN`` / ``bplags``   -- Box-Pierce Q, same shape
  * ``nsigacf`` / ``sigacf$NN`` / ``sigacflags``   -- significant residual ACF
  * ``nsigpacf`` / ``sigpacf$NN`` / ``sigpacflags`` -- significant partial ACF
  * ``skewness`` / ``a`` / ``kurtosis`` -- the normality battery, each carrying
    the oracle's own significance marker (``-``/``+`` for skewness, ``*`` for
    Geary's a and kurtosis)
  * ``durbinwatson``                 -- the first-order residual serial test
  * ``friedman``                     -- Kendall's statistic, df, chi-square p

It also covers the rest of the ESTIMATION savelog block, which was equally
absent (core/src/diag/estdgn.cpp):

  * ``outlier.ao``/``.ls``/``.tc``/``.so``/``.rp``/``.tls``/``.user``/
    ``.total`` and ``autoout`` -- savotl.f's counts by regressor TYPE
  * ``roots.<filter>.<period>.<NN>`` -- prtrts.f's roots of every ARMA
    operator (real, imaginary, modulus, frequency), TAB-separated
  * ``nonseasonaldiff`` / ``seasonaldiff`` / ``nmodel`` -- prtmdl.f's model
    shape counters
  * ``aape.mode`` and ``aape.0``-``aape.3`` -- amdfct.f's average absolute
    percentage forecast error over each of the last three years, and their
    average (which the oracle emits FIRST, as ``aape.0``)
  * ``<AR|MA>$<period>$<factor>$<lag>`` -- prtmdl.f's ARMA coefficient table:
    the estimate, its standard error and its t-value, plus the ``(fixed)``
    marker. Note this key keeps the operator title's own CASE while the
    ``roots.`` key beside it lowercases both halves.

WHY IT REACHES SO FAR. None of the corpus specs need a ``check{}`` spec for any
of this: ``editor.f:909-910`` sets ``Mxcklg = 2*Sp`` on ANY model run with
``Lsumm > 0`` -- the ``-s`` flag every golden here was blessed with -- so the
oracle emits the whole block regardless. 287 of the 331 ``.udg`` goldens carry
it, and **zero new goldens were blessed for this gate**.

TOLERANCE. Byte-exact wherever it holds, which is 271 of the 275 compared
specs (measured, not assumed).
The engine writes each line with the Fortran format that produced it
(acfdgn.f 1110/1150/1160/1170/1180/1190, nrmtst.f 1030, arima.f 9000/9001)
through the port's own ``fwrite_fmt``, so the comparison is against the golden
TEXT -- a stronger contract than a numeric tolerance, and the right one, since
the printed line *is* the oracle's published diagnostic. It includes Fortran
field OVERFLOW: a partial autocorrelation with |t| = 10.169 does not fit
acfdgn's f7.4, and both sides print ``*******`` (``generated/unrate_sar-seats``,
``sigpacf$24``).

Where the text differs the gate falls back to a per-field numeric compare at
``max(5e-4 absolute, 5e-5 relative)``: the print ULP of the widest field, or
the ordinary engine-vs-oracle estimation floor, whichever is larger. Integer
fields (Q degrees of freedom, Friedman df), the significance MARKER characters
and the lag LISTS are always exact -- only the statistics themselves fall back.
Four specs need it, all of them auto-selecting their model, and both places it
bites are RATIOS, which is why they show what the underlying coefficients do
not:

  * ``durbinwatson`` is printed E15.8, i.e. eight significant digits -- finer
    than the coefficients agree. Measured 4.7e-6 (``co2_automdl``, 1.9550272
    vs 1.9550181) to 1.3e-5 (``airline_automdl-x11``, 1.9710439 vs 1.9710698).
  * the ``sigacf$NN``/``sigpacf$NN`` t-column is (p)acf/se, so it resolves a
    difference the f7.4 (p)acf and se columns beside it round away --
    ``generated/airline_seats`` ``sigpacf$19`` is -1.6001 against -1.6000 with
    both -0.1398 / 0.0874 identical (``sigpacf$20`` on the same three specs is
    the same story).

That second one also marks the real limit of this gate: those t-values are
tested against ``acflimit`` (1.6) to decide MEMBERSHIP of the significant-lag
list, so a lag sitting on the threshold could in principle be flagged by one
side and not the other. It does not happen on this corpus -- every spec agrees
on which lags are significant -- and the gate asserts key sets in both
directions, so it would be a loud failure rather than a silent one.

Run:  python -m pytest tests/parity/test_check_diagnostics.py -q
"""
from __future__ import annotations

import functools
import os
import re
import subprocess

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus")
_GOLDEN = os.path.join(_REPO, "tests", "golden")

_TREES = ["generated", "extra", "census-examples", "ces"]

# The keys this gate owns. Order is the oracle's emit order; `$NN` families are
# matched by prefix.
_SCALARS = ["qlimit", "nlbq", "lblags", "nbpq", "bplags", "acflimit",
            "nsigacf", "sigacflags", "nsigpacf", "sigpacflags",
            "skewness", "a", "kurtosis", "durbinwatson", "friedman",
            # savotl.f -- the outlier counts by regressor type
            "outlier.ao", "outlier.ls", "outlier.tc", "outlier.so",
            "outlier.rp", "outlier.tls", "outlier.user", "outlier.total",
            "autoout",
            # prtmdl.f -- the model's differencing shape
            "nonseasonaldiff", "seasonaldiff", "nmodel",
            # amdfct.f -- the average absolute percentage forecast error
            "aape.mode", "aape.0", "aape.1", "aape.2", "aape.3"]
# prtrts.f roots are `roots.<filter>.<period>.<NN>`, so they are matched by
# prefix like the `$NN` families.
# prtmdl.f's ARMA coefficient rows are `<AR|MA>$<period>$<factor>$<lag>` --
# matched by prefix like the rest. The `seats$` variants belong to SEATS'
# own getdiag block, which is a separate (unported) front, so they are NOT
# claimed here.
_FAMILIES = ["lbq$", "bpq$", "sigacf$", "sigpacf$", "roots.", "AR$", "MA$"]

_KEY_RE = re.compile(r"^([A-Za-z][A-Za-z0-9._$]*):(.*)$")

# Numeric fallback bound -- see the tolerance note in the module docstring.
# The print ULP of the widest field (f7.4/E15.8 -> 5e-4 on the value) or the
# estimation floor (5e-5 relative), whichever is larger.
_ATOL = 5e-4
_RTOL = 5e-5

# Specs where the engine fits a DIFFERENT MODEL than the oracle, so its
# residuals are a different series and no residual diagnostic can agree. Not a
# check{} defect and deliberately not hidden inside the comparison: these are
# pre-existing, already-tracked automdl gaps, and this gate is simply the first
# thing to run on the spec at all.
#
#   usdeaths_automdl -- automd selects (1 0 1)(0 1 1) against the oracle's
#   (0 1 1)(0 1 1): a regular-differencing (d=0 vs d=1) discrepancy in iddiff
#   that only NSA data reaches. Already recorded at the omission in
#   test_m4_iddiff.py's _AUTOMD_EST_CASES and in tools/automdl_scouting.md.
#   Visible here as nefobs 60 vs 59 -- which is why the ACF standard errors
#   differ (se = 1/sqrt(nefobs), 0.1291 vs 0.1302) before any statistic does.
_WRONG_MODEL = {}


def _close(want: str, got: str) -> bool:
    """Per-field numeric compare of one savelog line.

    Fields that do not parse as a number -- the significance markers, the lag
    lists, the ``*******`` overflow fill -- must match exactly; so must the
    field COUNT, which is what keeps a dropped column from passing.
    """
    wf, gf = want.split(), got.split()
    if len(wf) != len(gf):
        return False
    for a, b in zip(wf, gf):
        try:
            fa, fb = float(a), float(b)
        except ValueError:
            if a != b:
                return False
            continue
        if abs(fa - fb) > max(_ATOL, _RTOL * abs(fa)):
            return False
    return True


def _find_binary() -> str:
    for c in (os.path.join(_REPO, "build", "x13run_m3.exe"),
              os.path.join(_REPO, "build", "x13run_m3"),
              os.path.join(_REPO, "build", "Release", "x13run_m3.exe")):
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_M3")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13run_m3 binary not found; build it first (cmake --build build).")


BIN = _find_binary()


def _is_check_key(key: str) -> bool:
    if key in _SCALARS:
        return True
    return any(key.startswith(f) for f in _FAMILIES)


def _read_block(lines) -> dict[str, str]:
    """Every check{} key in a `.udg`-shaped stream -> its raw value text."""
    out: dict[str, str] = {}
    for ln in lines:
        m = _KEY_RE.match(ln.rstrip("\n").lstrip())
        if not m:
            continue
        key = m.group(1)
        if _is_check_key(key):
            out[key] = m.group(2).rstrip()
    return out


def _discover() -> list[str]:
    """Corpus-relative spec ids whose golden `.udg` carries the check block."""
    out: list[str] = []
    for tree in _TREES:
        root = os.path.join(_CORPUS, tree)
        if not os.path.isdir(root):
            continue
        for dirpath, _dirs, files in os.walk(root):
            # A metafile means these specs are composite COMPONENTS: the oracle
            # ran them in one process with the aggregation COMMONs live, so a
            # standalone run is a different run.
            if any(f.endswith(".mta") for f in files):
                continue
            for fn in sorted(files):
                if not fn.endswith(".spc"):
                    continue
                base = fn[:-4]
                rel = os.path.relpath(os.path.join(dirpath, base), _CORPUS)
                udg = os.path.join(_GOLDEN, rel, base + ".udg")
                if not os.path.exists(udg):
                    continue
                with open(udg, encoding="utf-8", errors="replace") as fh:
                    if "nlbq:" not in fh.read():
                        continue
                out.append(rel.replace("\\", "/"))
    return sorted(out)


CASES = _discover()


@functools.lru_cache(maxsize=None)
def _run(rel: str) -> dict[str, str]:
    spec = os.path.join(_CORPUS, rel + ".spc")
    txt = open(spec, encoding="utf-8", errors="replace").read().lower()
    r = subprocess.run([BIN, spec], capture_output=True, text=True,
                       cwd=os.path.dirname(spec))
    assert r.returncode == 0, f"{rel}: harness exit {r.returncode}\n{r.stderr}"
    first = r.stdout.splitlines()[0].strip() if r.stdout else ""
    if first != "OUTCOME: OK":
        pytest.skip(f"engine declined this spec ({first or 'no output'})")
    return _read_block(r.stdout.splitlines())


def test_every_owned_key_is_readable() -> None:
    """_KEY_RE must actually MATCH every key this gate claims to own.

    A too-narrow key regex drops a key silently -- the golden side and the
    engine side both lose it, the comparison still passes, and the gate looks
    green while covering nothing. That happened once already: the pattern
    allowed no digits after a dot, so ``aape.0``..``aape.3`` were invisible
    while ``aape.mode`` beside them was compared.
    """
    samples = [f"{k}: 1.0" for k in _SCALARS]
    samples += ["lbq$03: 1", "bpq$04: 1", "sigacf$20: 1", "sigpacf$07: 1",
                "roots.ma.nonseasonal.01: 1", "MA$Nonseasonal$01$01: 1",
                "AR$Seasonal$12$12: 1"]
    unreadable = []
    for line in samples:
        m = _KEY_RE.match(line)
        if not m or not _is_check_key(m.group(1)):
            unreadable.append(line.split(":")[0])
    assert not unreadable, f"keys this gate owns but cannot parse: {unreadable}"


@pytest.mark.skipif(not CASES, reason="no corpus golden ships the check block")
@pytest.mark.parametrize("rel", CASES)
def test_check_block(rel: str) -> None:
    """Every check{} savelog line matches the golden EXACTLY."""
    if rel in _WRONG_MODEL:
        pytest.skip(_WRONG_MODEL[rel])
    udg = os.path.join(_GOLDEN, rel, os.path.basename(rel) + ".udg")
    with open(udg, encoding="utf-8", errors="replace") as fh:
        want = _read_block(fh)
    assert want, f"{rel}: golden has no check block (discovery is wrong)"
    got = _run(rel)

    # Both directions: a missing key is an unported statistic, an extra key is
    # the engine emitting one the oracle suppressed (nrmtst.f returns early out
    # of its middle when a statistic's table cannot cover nefobs, which drops
    # every statistic after it too).
    missing = sorted(set(want) - set(got))
    extra = sorted(set(got) - set(want))
    assert not missing, f"{rel}: keys missing from the engine: {missing}"
    assert not extra, f"{rel}: keys the oracle does not emit: {extra}"

    bad = []
    for k in sorted(want):
        w, g = want[k], got[k]
        if w == g:
            continue
        if _close(w, g):
            continue
        bad.append((k, w, g))
    assert not bad, "\n".join(
        f"{rel}.{k}: golden {w!r} != engine {g!r}" for k, w, g in bad)
