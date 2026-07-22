"""SEATS gate (scoping pass): the C++ SEATS decomposition (x13run_seats) vs
the oracle s-table goldens.

SEATS ("ARIMA-model-based" seasonal adjustment) decomposes the fitted
regARIMA model into trend/seasonal/SA/transitory/irregular components via
Wiener-Kolmogorov filtering (Burman's algorithm). The orchestrating driver
(SIGEX/ESTBUR/AUTOCOMP -- see tools/seats_scope.md) is now ported: **every
SEATS corpus spec's s10-s18 tables gate bit-exact** (added to `_gated` below),
including the near-non-invertible fixed-airline variants (payems_fixed /
expgs_fixed), whose forecast-extension seed residuals are computed by a
faithful CALCFX port (estbur.cpp `calcfx_last_residuals`).

The test still uses per-tag gating: a `(base, tag)` NOT in `_gated` short-
circuits via `pytest.xfail` (this remains for any future SEATS spec/table not
yet landed). The harness-subprocess + golden-diff plumbing (rtol=1e-8, plus an
ATOL=1e-9 floor for zero-crossing tables) mirrors test_x11_tables.py's shape.

Run:  python -m pytest tests/parity/test_seats_tables.py -q
"""
from __future__ import annotations

import os
import re
import subprocess

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus", "generated")
_GOLDEN = os.path.join(_REPO, "tests", "golden", "generated")

RTOL = 1e-8
# Absolute-tolerance floor (numpy-allclose style): a point passes if
# |v-g| <= RTOL*|g| + ATOL. Needed for tables whose values legitimately cross
# zero (the additive seasonal component s10/s16, the irregular s13), where a
# pure relative error explodes near the crossing even though the absolute error
# is double-precision noise (~1e-12). ATOL=1e-9 sits far above that noise and
# far below any real decomposition error (which run ~1e-3+), so it only ever
# forgives zero-crossings, never masks a genuine discrepancy.
ATOL = 1e-9

# The 8 SEATS corpus specs (tools/seats_scope.md section 4 / the task's gate
# list): the 4 base series x {default seats, fixed-airline-seats}.
_TAGS = ["s10", "s11", "s12", "s13", "s16", "s18"]


def _find_binary() -> str:
    cands = [
        os.path.join(_REPO, "build", "x13run_seats.exe"),
        os.path.join(_REPO, "build", "x13run_seats"),
        os.path.join(_REPO, "build", "Release", "x13run_seats.exe"),
    ]
    for c in cands:
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_SEATS")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13run_seats binary not found; build it first (cmake --build build "
        "--target x13run_seats).")


BIN = _find_binary()

_GOLD_RE = re.compile(r"(\d{6})\s+([+\-][0-9.EeDd+\-]+)")


def _read_golden(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _GOLD_RE.match(ln.strip())
            if m:
                out[m.group(1)] = float(m.group(2).replace("D", "E").replace("d", "e"))
    return out


def _discover() -> list[str]:
    specs: list[str] = []
    if not os.path.isdir(_CORPUS):
        return specs
    for fn in sorted(os.listdir(_CORPUS)):
        if not fn.endswith(".spc"):
            continue
        base = fn[:-4]
        if not base.endswith("seats"):
            continue
        gdir = os.path.join(_GOLDEN, base)
        # Require at least one gated table's golden to exist -- some tags
        # (s18) are shipped but sometimes empty; s10-s13 always ship for the
        # SEATS corpus.
        if not os.path.isdir(gdir):
            continue
        specs.append(base)
    return specs


CASES = _discover()


@pytest.mark.skipif(not CASES, reason="no seats{} spec ships a golden bundle")
@pytest.mark.parametrize("base", CASES)
@pytest.mark.parametrize("tag", _TAGS)
def test_seats_table(base: str, tag: str) -> None:
    # SEATS decomposition (SIGEX/ESTBUR/AUTOCOMP) is not yet ported for the
    # general case -- see tools/seats_scope.md. Session 12 landed the
    # historical-span ESTBUR general-branch solve for the additive/no-
    # seasonal/no-cycle case (unrate_seats): s11 (SA)/s12 (trend) gate
    # bit-exact. Session 13 added the log back-transform and s10/s16/s18
    # (seasonal factor, z/sa), gating payems_seats (log-transform, real
    # cycle) as the second series. Session 14 CLOSED unrate_seats' s13
    # (irregular): the ~2.78e-8-at-2-boundary-dates gap (session 12/13) was
    # ground-truthed against the oracle directly (temporary analts.f
    # instrumentation, reverted) and found to be `estbur.cpp`'s
    # `armafl_last_residuals` applying an EXTRA `fac=exp(lndtcv/2/dnefob)`
    # scaling that CALCFX's own round trip (STEP9 `a(i)=Detpri*a(i)`,
    # analts.f's caller `a(i)=a(i)/Detpri`) cancels back out before ever
    # handing residuals to FCAST -- `Detpri` turned out to be EXACTLY
    # `fac`, bit-for-bit, so the fed-to-FCAST residual is the RAW
    # (unscaled) armafl() value, not the `fac`-scaled one. Dropping the
    # scaling closes unrate_seats' s12/s13 to double-precision noise
    # (worst ~4.8e-15 across all 776 dates) -- now gated. A second,
    # independent bug found+fixed the same session: FCAST's own end-
    # extension recursion needs the trailing `Qstar_f` historical residuals
    # as seeds (not just the very last one) whenever `Qstar_f=q+bq*mq>=2`;
    # `unrate_seats` (`Qstar_f=1`) never exercised the gap, but
    # `payems_seats` (`Qstar_f=2`) was silently treating `a(Na-1)` as 0.
    # Fixing this (`armafl_last_residuals` now returns a trailing vector,
    # `fcast_extend` seeds all of it) collapsed payems' s12/s13 error from a
    # ~2.1e-4 spike at the first point down to a UNIFORM ~2.2e-6 floor
    # across the whole 308-point series. That floor was NOT a residual gap:
    # it was the SEATS multiplicative bias correction (sigsub.f:1539-1585,
    # bias==1 default per ansub9.f:1118) missing from the log back-transform.
    # For payems (npsi==1) it reduces to log(mean(exp(ir))) applied trend-
    # only == +2.1978124351562656e-06. Porting bias1c/bias2c/bias3c into
    # estbur.cpp's is_log block closed payems' s12/s13 to double-precision
    # noise -- now gated. s10 doesn't ship for unrate_seats (npsi==1, no
    # seasonal component). Every other spec (expgs/airline_seats and the
    # *-fixed-airline-seats variants -- real seasonal structure, npsi!=1,
    # untested cs/cc/general-branch path) still fatals at the decomposition
    # dispatch point and stays xfailed. Un-xfail more (base, tag) pairs
    # (including payems' s12/s13, if/when its remaining gap closes) as they
    # land.
    _gated = {
        ("unrate_seats", "s11"), ("unrate_seats", "s12"),
        ("unrate_seats", "s13"), ("unrate_seats", "s18"),
        ("payems_seats", "s10"), ("payems_seats", "s11"),
        ("payems_seats", "s12"), ("payems_seats", "s13"),
        ("payems_seats", "s16"), ("payems_seats", "s18"),
        ("airline_seats", "s10"), ("airline_seats", "s11"),
        ("airline_seats", "s12"), ("airline_seats", "s13"),
        ("airline_seats", "s16"), ("airline_seats", "s18"),
        # airline_fixed-airline-seats: same (0 1 1)(0 1 1) model, fixed (no
        # automdl) -- all 6 tables bit-exact.
        ("airline_fixed-airline-seats", "s10"),
        ("airline_fixed-airline-seats", "s11"),
        ("airline_fixed-airline-seats", "s12"),
        ("airline_fixed-airline-seats", "s13"),
        ("airline_fixed-airline-seats", "s16"),
        ("airline_fixed-airline-seats", "s18"),
        # unrate_fixed-airline-seats: additive (no transform) + real seasonal.
        # SA (s11)/trend (s12)/seasonal-factor (s18=z/sa ratio) bit-exact, and
        # s10/s16 (z-sa additive difference) now that both conventions are
        # wired. All 6 bit-exact in absolute terms (~4e-12); s10/s13/s16 rely
        # on the ATOL floor at their zero crossings.
        ("unrate_fixed-airline-seats", "s10"),
        ("unrate_fixed-airline-seats", "s11"),
        ("unrate_fixed-airline-seats", "s12"),
        ("unrate_fixed-airline-seats", "s13"),
        ("unrate_fixed-airline-seats", "s16"),
        ("unrate_fixed-airline-seats", "s18"),
        # payems_fixed / expgs_fixed-airline-seats: log-transform airline with a
        # near-NON-invertible seasonal MA (bth ~ -0.978, capped to -0.99). The
        # SEATS forecast-extension seed residuals are computed by a faithful
        # CALCFX port (estbur.cpp calcfx_last_residuals) on the CAPPED canonical
        # model -- armafl's exact-ML/GLS residuals diverged ~4.5%/~7e-4 at the
        # tail for this near-boundary MA. All 6 tables bit-exact (~5e-15).
        ("payems_fixed-airline-seats", "s10"),
        ("payems_fixed-airline-seats", "s11"),
        ("payems_fixed-airline-seats", "s12"),
        ("payems_fixed-airline-seats", "s13"),
        ("payems_fixed-airline-seats", "s16"),
        ("payems_fixed-airline-seats", "s18"),
        ("expgs_fixed-airline-seats", "s10"),
        ("expgs_fixed-airline-seats", "s11"),
        ("expgs_fixed-airline-seats", "s12"),
        ("expgs_fixed-airline-seats", "s13"),
        ("expgs_fixed-airline-seats", "s16"),
        ("expgs_fixed-airline-seats", "s18"),
    }
    if (base, tag) not in _gated:
        pytest.xfail("SEATS decomposition (SIGEX orchestrator) not yet ported "
                     "for this spec/table -- see tools/seats_scope.md")
    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.returncode == 0, f"{base}: harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:200]

    produced: dict[str, float] = {}
    for ln in r.stdout.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == tag:
            produced[p[1]] = float(p[2])

    gold_path = os.path.join(_GOLDEN, base, base + "." + tag)
    if not os.path.exists(gold_path):
        pytest.skip(f"{base}.{tag}: no golden shipped")
    gold = _read_golden(gold_path)
    assert gold, f"{base}.{tag}: empty golden"

    keys = sorted(set(gold) & set(produced))
    assert len(keys) == len(gold), (
        f"{base}.{tag}: produced {len(produced)} rows, golden {len(gold)}, "
        f"overlap {len(keys)}")

    worst = 0.0
    worst_k = None
    bad = 0
    for k in keys:
        g, v = gold[k], produced[k]
        if abs(v - g) <= RTOL * abs(g) + ATOL:
            continue
        bad += 1
        rel = abs(v - g) / abs(g) if g else abs(v - g)
        if rel > worst:
            worst, worst_k = rel, k
    assert bad == 0, (
        f"{base}.{tag}: {bad} pts exceed RTOL*|g|+ATOL; "
        f"max rel err {worst:.3e} at {worst_k}")


_ARIMAMDL_RE = re.compile(r"^arimamdl:\s*(.+)$")


def _read_golden_arimamdl(base: str) -> str | None:
    udg = os.path.join(_GOLDEN, base, base + ".udg")
    if not os.path.exists(udg):
        return None
    with open(udg, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _ARIMAMDL_RE.match(ln.strip())
            if m:
                return m.group(1).strip()
    return None


@pytest.mark.skipif(not CASES, reason="no seats{} spec ships a golden bundle")
@pytest.mark.parametrize("base", CASES)
def test_seats_model_decode(base: str) -> None:
    """Model-decode gate (tools/seats_scope.md next-increment #1, session 3):
    x13::seats_decode_model (nmlmdl.f port, core/src/seats/model_decode.cpp)
    reads ctx.model's Mdl/Opr/Arimal ARMA-operator-list representation --
    already populated by the shared regARIMA estimate phase that runs before
    run_seats's still-open SPECTRU fatal -- into plain (p,d,q)(P,D,Q) orders.
    x13run_seats prints that decode (best-effort, regardless of OUTCOME) as
    `DECODE_ARIMAMDL: ...`, formatted exactly like the oracle's `arimamdl`
    .udg key (mkmdsn.f's "(p d q)(P D Q)" shape, seasonal group omitted when
    all-zero). NOT xfailed: this is a real, currently-passing gate for all 8
    SEATS corpus specs, independent of the (still not ported) canonical
    decomposition itself.
    """
    gold = _read_golden_arimamdl(base)
    if gold is None:
        pytest.skip(f"{base}: no arimamdl key in golden .udg")
    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.stdout.splitlines(), f"{base}: no output at all (exit {r.returncode})\n{r.stderr}"
    produced = None
    for ln in r.stdout.splitlines():
        if ln.startswith("DECODE_ARIMAMDL:"):
            produced = ln[len("DECODE_ARIMAMDL:"):].strip()
            break
    assert produced is not None, (
        f"{base}: harness printed no DECODE_ARIMAMDL line\n{r.stdout}\n{r.stderr}")
    assert produced == gold, f"{base}: decoded {produced!r}, golden arimamdl {gold!r}"


_IRRVAR_RE = re.compile(r"^irrvar:\s*(.+)$")


def _read_golden_irrvar(base: str) -> float | None:
    mdc = os.path.join(_GOLDEN, base, base + ".mdc")
    if not os.path.exists(mdc):
        return None
    with open(mdc, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _IRRVAR_RE.match(ln.strip())
            if m:
                return float(m.group(1).strip().replace("D", "E").replace("d", "e"))
    return None


@pytest.mark.skipif(not CASES, reason="no seats{} spec ships a golden bundle")
@pytest.mark.parametrize("base", CASES)
def test_seats_qt1(base: str) -> None:
    """SPECTRU qt1 gate (tools/seats_scope.md session 4-5): x13::spectru
    (spectrum.f:558-1190 port, core/src/seats/spectru.cpp) computes qt1, the
    canonical-decomposition admissibility number, from the decoded model via
    x13::seats_canonical_denoms's chi/psi/cyc/thstar/pstar. x13run_seats
    prints it (best-effort) as `DECODE_QT1: <value>`; this diffs it against
    the golden `.mdc`'s `irrvar` key (traced this session, via ansub9.f's
    USRENTRY IFUNC=2010 dispatch, to be exactly SPECTRU's own qt1 output,
    unchanged, saved by ShowComp/DecompSpectrum).

    SESSION-5 FIX: this gate did not pass at the end of session 4 (computed
    qt1 was 14.9% high on unrate_seats, wrong-signed on payems_seats). Traced
    to a sign-convention bug in canonical_denoms.cpp's `thstar` (MA numerator
    polynomial) construction -- built as `ths(i+1)=-Th(i)`, literally
    matching analts.f:2854-2859's text, but empirically wrong; the fix
    (`ths(i+1)=+Th(i)`, no negation) makes qt1 match golden irrvar BIT-EXACT
    (every printed digit) on all 7 corpus specs with a golden `.mdc` (the
    8th, expgs_seats, has none -- the oracle itself rejects that model, see
    tools/seats_scope.md's session-2 note). WHY the literal Fortran text
    disagrees with the empirically-correct sign is not resolved (see
    canonical_denoms.cpp's comment); flagged for a future session. The `phi`
    side (phis/bphis, AR) was NOT touched -- p=0 for every admissible corpus
    target, so there is no data point to test that sign against.
    """
    gold = _read_golden_irrvar(base)
    if gold is None:
        pytest.skip(f"{base}: no irrvar key in golden .mdc (oracle likely rejected the model)")
    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.stdout.splitlines(), f"{base}: no output at all (exit {r.returncode})\n{r.stderr}"
    produced = None
    for ln in r.stdout.splitlines():
        if ln.startswith("DECODE_QT1:"):
            produced = float(ln[len("DECODE_QT1:"):].strip())
            break
    assert produced is not None, (
        f"{base}: harness printed no DECODE_QT1 line\n{r.stdout}\n{r.stderr}")
    rel = abs(produced - gold) / abs(gold) if gold else abs(produced - gold)
    assert rel <= 1e-8, f"{base}: qt1={produced!r}, golden irrvar={gold!r}, rel err {rel:.3e}"


# .mdc scalar/array keys DecompSpectrum/MAspectrum (core/src/seats/
# decompspectrum.cpp, session 6) produce, keyed by their golden .mdc name.
# `n<key>` count lines (e.g. `nsanum:`) are read too but only used to assert
# the array lengths agree; the comparison itself is per-element.
_MDC_ARRAY_KEYS = ["sanum", "saden", "snum", "sden", "trnum", "trden"]
_MDC_SCALAR_KEYS = ["savar", "svar", "trvar"]
_MDC_LINE_RE = re.compile(r"^(\w+)(?:\.(\d+))?:\s*(.+)$")


def _read_mdc_dict(path: str) -> dict[str, float]:
    """Parses a `.mdc`-shaped file (golden or, with an `MDC_` prefix
    stripped by the caller, the harness's own output) into a flat
    {key or key.NNN: float} dict, matching the file's own `key: value` /
    `key.NNN: value` line shape."""
    out: dict[str, float] = {}
    if not os.path.exists(path):
        return out
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _MDC_LINE_RE.match(ln.strip())
            if not m:
                continue
            key, idx, val = m.group(1), m.group(2), m.group(3)
            try:
                fval = float(val.strip().replace("D", "E").replace("d", "e"))
            except ValueError:
                continue
            out[f"{key}.{idx}" if idx is not None else key] = fval
    return out


def _read_produced_mdc_dict(stdout: str) -> dict[str, float]:
    out: dict[str, float] = {}
    for ln in stdout.splitlines():
        if not ln.startswith("MDC_"):
            continue
        m = _MDC_LINE_RE.match(ln[len("MDC_"):].strip())
        if not m:
            continue
        key, idx, val = m.group(1), m.group(2), m.group(3)
        out[f"{key}.{idx}" if idx is not None else key] = float(val)
    return out


@pytest.mark.skipif(not CASES, reason="no seats{} spec ships a golden bundle")
@pytest.mark.parametrize("base", CASES)
def test_seats_mdc(base: str) -> None:
    """DecompSpectrum/MAspectrum .mdc gate (tools/seats_scope.md session 6):
    x13::decomp_spectrum (spectrum.f:1380-2513 + spectrum.f:2710-2890 port,
    core/src/seats/decompspectrum.cpp) turns SPECTRU's Ut/V/Uc into the final
    per-component MA numerator polynomials (THETP/THETS/THETC/THADJ) and
    innovation variances via the already-ported MAK1. x13run_seats prints
    the ones that map to golden `.mdc` keys (traced via ansub9.f's USRENTRY
    IFUNC=2001-2013 dispatch: THADJ/CHCYC/VARWNA -> sanum/saden/savar,
    THETS/PSI/VARWNS -> snum/sden/svar when npsi!=1, THETC/CYC/VARWNC ->
    trnum/trden/trvar when ncycth!=0 or ncyc!=1) as `MDC_<key>[.NNN]: value`
    lines; this diffs every key BOTH sides have against the golden `.mdc`.

    Confirmed bit-exact (exact-string match, session 6) on every corpus spec
    checked: payems_seats/unrate_seats (sanum/saden/savar, no seasonal
    structure), payems_fixed-airline-seats/airline_fixed-airline-seats/
    airline_seats/unrate_fixed-airline-seats/expgs_fixed-airline-seats
    (sanum/saden/savar plus, for airline_seats and airline_fixed-airline-
    seats which have real seasonal structure, snum/sden/svar too). payems
    also exercises trnum/trden/trvar (its qstar>pstar ADDJ fold sets
    ncycth=1). expgs_seats has no golden `.mdc` at all (oracle rejects the
    model, unchanged since session 2) -- skipped.
    """
    gold_path = os.path.join(_GOLDEN, base, base + ".mdc")
    gold = _read_mdc_dict(gold_path)
    if not gold:
        pytest.skip(f"{base}: no .mdc golden (oracle likely rejected the model)")
    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.stdout.splitlines(), f"{base}: no output at all (exit {r.returncode})\n{r.stderr}"
    produced = _read_produced_mdc_dict(r.stdout)

    checked_any = False
    worst = 0.0
    worst_k = None
    for base_key in _MDC_ARRAY_KEYS + _MDC_SCALAR_KEYS:
        gold_keys = [k for k in gold if k == base_key or k.startswith(base_key + ".")]
        if not gold_keys:
            continue  # this .mdc doesn't ship this key (e.g. no seasonal structure)
        for k in gold_keys:
            assert k in produced, (
                f"{base}: golden has {k}={gold[k]!r} but harness produced no MDC_{k} line\n"
                f"{r.stdout}\n{r.stderr}")
            checked_any = True
            g, v = gold[k], produced[k]
            rel = abs(v - g) / abs(g) if g else abs(v - g)
            if rel > worst:
                worst, worst_k = rel, k
    assert checked_any, f"{base}: golden .mdc has none of the gated keys -- nothing to check"
    assert worst <= 1e-8, f"{base}: max rel err {worst:.3e} at {worst_k}"


def test_seats_harness_runs_cleanly() -> None:
    """Sanity gate (NOT xfailed): the harness must parse + estimate every
    SEATS corpus spec without crashing, even though it can't complete the
    decomposition yet. Catches regressions in the shared parse/estimate path
    independent of the (still-unported) SEATS-specific work."""
    assert CASES, "no seats{} spec discovered"
    for base in CASES:
        spec = os.path.join(_CORPUS, base + ".spc")
        r = subprocess.run([BIN, spec], capture_output=True, text=True)
        assert r.stdout.splitlines(), f"{base}: no output at all (exit {r.returncode})"
        outcome = r.stdout.splitlines()[0].strip()
        assert outcome in ("OUTCOME: OK", "OUTCOME: FATAL"), (
            f"{base}: harness produced unexpected first line {outcome!r} "
            f"(exit {r.returncode})\nstderr:\n{r.stderr}")
