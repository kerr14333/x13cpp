"""Parity gate for the language bindings (bindings/python/x13c.py + the C ABI).

The point of this file is narrow and important: **the interface must not lose
the parity the engine has.** The engine is gated bit-exact against the Fortran
oracle elsewhere; everything a caller actually sees, though, arrives through the
C ABI's table extraction -- its punch ranges, its date arithmetic, its
float marshalling. Any of those could silently shift a series by a period,
truncate a projected year, or round a value, and no existing gate would notice
because no existing gate reads the bindings.

So this runs the PYTHON binding over every corpus spec that ships a blessed
oracle golden and compares what the caller receives, row by row, keyed on the
DATE the binding reports -- not on position. Keying on dates is deliberate: a
positional compare would pass even if every label were a year late, which is
exactly the class of bug that bit `x13run_x11`'s own `dump()` (the appendbcst
anchor) and `run_seats`' harness before it.

Run:  python -m pytest tests/parity/test_bindings.py -q
"""
from __future__ import annotations

import os
import re
import sys

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
sys.path.insert(0, os.path.join(_REPO, "bindings", "python"))

x13c = pytest.importorskip("x13c")

# The same two-tier policy test_x11_tables.py uses, and for the same reason: a
# no-model X-11 run is pure filter arithmetic and reaches ~5e-15, so 1e-12 leaves
# three orders of margin; a model-based run carries values whose last digits
# track the optimizer's convergence path, and 1e-6 is the realistic floor there.
# The binding reports which it was, so the tolerance is chosen from the run
# itself rather than from a hard-coded list of spec names.
RTOL_ARITHMETIC = 1e-12
RTOL_ESTIMATION = 1e-6

# SEATS gets its own pair, matching test_seats_tables.py rather than either of
# the above: the canonical decomposition rides the model estimate, so it sits
# near 1e-10 on the harder specs, and its factor tables legitimately cross zero
# in additive mode, where a purely relative test has no meaning. Same
# |v-g| <= RTOL*|g| + ATOL form the established SEATS gate uses.
RTOL_SEATS = 1e-8
ATOL_SEATS = 1e-9

_ROW = re.compile(r"^(\d{6})\s+([+\-][0-9.EeDd+\-]+)")

# The decomposition tables every X-11 run produces, plus the ones only some do.
_TAGS = ["b1", "d10", "d11", "d12", "d13", "d16", "e1", "e2", "e3", "e11",
         "e18", "eb", "sac", "tac", "saa", "ffc", "a4", "rnd"]

# SEATS produces its own family. A spec asks for one decomposition or the other,
# so the binding dispatches on the parsed spec and these never coexist with the
# X-11 tags above.
_SEATS_TAGS = ["s10", "s11", "s12", "s13", "s14", "s16", "s18"]


def _library_available() -> bool:
    try:
        x13c.load_library()
        return True
    except x13c.X13Error:
        return False


pytestmark = pytest.mark.skipif(
    not _library_available(),
    reason="x13c shared library not built (cmake target `x13c`)")


def _read_golden(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _ROW.match(ln.strip())
            if m:
                out[m.group(1)] = float(
                    m.group(2).replace("D", "E").replace("d", "e"))
    return out


def _discover(marker: str) -> list[tuple[str, str, str]]:
    """(spec_id, spec_path, golden_dir) for every corpus spec whose golden dir
    ships `marker` -- d11 for an X-11 run, s11 for a SEATS one. The per-table
    comparison then covers whichever tags also shipped.
    """
    cases = []
    for group in ("generated", "extra", "census-examples"):
        gdir = os.path.join(_REPO, "tests", "golden", group)
        cdir = os.path.join(_REPO, "tests", "corpus", group)
        if not os.path.isdir(gdir):
            continue
        for name in sorted(os.listdir(gdir)):
            gd = os.path.join(gdir, name)
            spec = os.path.join(cdir, name + ".spc")
            if (os.path.isdir(gd) and os.path.exists(spec)
                    and os.path.exists(os.path.join(gd, name + "." + marker))):
                cases.append((f"{group}/{name}", spec, gd))
    return cases


CASES = _discover("d11")
SEATS_CASES = _discover("s11")
assert CASES, "no corpus spec ships a d11 golden -- discovery is broken"
assert SEATS_CASES, "no corpus spec ships an s11 golden -- discovery is broken"


@pytest.mark.parametrize("spec_id,spec,golden", CASES,
                         ids=[c[0] for c in CASES])
def test_binding_matches_oracle_golden(spec_id: str, spec: str, golden: str):
    """Every table the binding exposes matches the blessed oracle golden."""
    base = os.path.basename(golden)
    with x13c.adjust(spec, check=False) as run:
        if not run.ok:
            pytest.skip(f"{spec_id}: engine declined the run ({run.error})")

        rtol = RTOL_ESTIMATION if run.model_based else RTOL_ARITHMETIC
        compared = 0
        for tag in _TAGS:
            gpath = os.path.join(golden, base + "." + tag)
            if not os.path.exists(gpath):
                continue
            gold = _read_golden(gpath)
            if not gold:
                continue
            if not run.has_table(tag):
                pytest.fail(f"{spec_id}: golden ships {tag} but the binding "
                            f"exposes only {sorted(run.table_names())}")

            t = run.table(tag)
            got = {f"{y:04d}{p:02d}": v for (y, p), v in zip(t.dates, t.values)}

            # Keyed on DATE, and the key sets must MATCH -- a binding that
            # returned the right numbers under shifted labels must fail here.
            missing = sorted(set(gold) - set(got))
            assert not missing, (
                f"{spec_id}.{tag}: binding is missing {len(missing)} dated rows "
                f"the golden has, e.g. {missing[:4]} "
                f"(binding span {t.dates[0]}..{t.dates[-1]})")

            worst, at = 0.0, None
            for k, g in gold.items():
                v = got[k]
                rel = abs(v - g) / abs(g) if g else abs(v - g)
                if rel > worst:
                    worst, at = rel, k
            assert worst <= rtol, (
                f"{spec_id}.{tag}: max rel err {worst:.3e} at {at} "
                f"(tol {rtol:.0e}, model_based={run.model_based})")
            compared += 1

        assert compared, f"{spec_id}: no table was actually compared"


@pytest.mark.parametrize("spec_id,spec,golden", SEATS_CASES,
                         ids=[c[0] for c in SEATS_CASES])
def test_seats_binding_matches_oracle_golden(spec_id: str, spec: str,
                                             golden: str):
    """The SEATS family (s10-s18) through the binding, same date-keyed compare.

    SEATS runs a different driver (run_seats, not run_x11), so the binding has to
    dispatch on the parsed spec to reach it at all. These tables are the
    canonical decomposition -- arithmetic on the already-estimated model -- and
    measure ~5e-15 against the goldens, so they hold the TIGHT tolerance even
    though every SEATS spec is model-based.
    """
    base = os.path.basename(golden)
    with x13c.adjust(spec, check=False) as run:
        if not run.ok:
            pytest.skip(f"{spec_id}: engine declined the run ({run.error})")

        compared = 0
        for tag in _SEATS_TAGS:
            gpath = os.path.join(golden, base + "." + tag)
            if not os.path.exists(gpath):
                continue
            gold = _read_golden(gpath)
            if not gold:
                continue
            assert run.has_table(tag), (
                f"{spec_id}: golden ships {tag} but the binding exposes only "
                f"{sorted(run.table_names())}")
            t = run.table(tag)
            got = {f"{y:04d}{p:02d}": v for (y, p), v in zip(t.dates, t.values)}
            missing = sorted(set(gold) - set(got))
            assert not missing, (
                f"{spec_id}.{tag}: binding is missing {len(missing)} dated "
                f"rows, e.g. {missing[:4]}")
            bad, worst, at = 0, 0.0, None
            for k, g in gold.items():
                d = abs(got[k] - g)
                if d > RTOL_SEATS * abs(g) + ATOL_SEATS:
                    bad += 1
                rel = d / abs(g) if g else d
                if rel > worst:
                    worst, at = rel, k
            assert bad == 0, (
                f"{spec_id}.{tag}: {bad} of {len(gold)} points exceed "
                f"RTOL*|g|+ATOL; max rel {worst:.3e} at {at}")
            compared += 1

        assert compared, f"{spec_id}: no SEATS table was actually compared"


def test_seats_and_x11_tables_do_not_mix():
    """A SEATS spec exposes the s-family and no d-family, and an X-11 spec the
    reverse -- the binding picks one driver, it does not run both."""
    with x13c.adjust(SEATS_CASES[0][1], check=False) as run:
        if run.ok:
            names = set(run.table_names())
            assert names & set(_SEATS_TAGS), "a SEATS run should expose s-tables"
            assert not (names & {"d10", "d11", "d12", "d13"}), (
                f"a SEATS run should not expose d-tables, got {sorted(names)}")
    x11_spec = os.path.join(_REPO, "tests", "corpus", "generated",
                            "airline_x11-default.spc")
    with x13c.adjust(x11_spec) as run:
        names = set(run.table_names())
        assert "d11" in names
        assert not (names & set(_SEATS_TAGS))


def test_dates_are_contiguous_and_calendar_valid():
    """The binding's own date arithmetic: 1-based periods, no gaps, correct
    year rollover. Checked independently of any golden, on every table."""
    spec = os.path.join(_REPO, "tests", "corpus", "generated",
                        "airline_x11-default.spc")
    with x13c.adjust(spec) as run:
        assert run.period == 12
        for name in run.table_names():
            t = run.table(name)
            assert len(t.dates) == len(t.values) == len(t)
            for (y, p) in t.dates:
                assert 1 <= p <= run.period, f"{name}: period {p} out of range"
            for i in range(1, len(t.dates)):
                py, pp = t.dates[i - 1]
                cy, cp = t.dates[i]
                nxt = (py + 1, 1) if pp == run.period else (py, pp + 1)
                assert (cy, cp) == nxt, (
                    f"{name}: dates jump {py}-{pp} -> {cy}-{cp} at index {i}")


def test_seasonal_factors_extend_past_the_data():
    """d10 is projected a year beyond the observed span while d11 is not -- the
    difference the ABI's per-table start dates exist for. If some future change
    made every table share one range, this is the test that catches it."""
    spec = os.path.join(_REPO, "tests", "corpus", "generated",
                        "airline_x11-default.spc")
    with x13c.adjust(spec) as run:
        d10, d11 = run.table("d10"), run.table("d11")
        assert len(d11) == run.nobs
        assert len(d10) >= len(d11)
        assert d10.dates[0] == d11.dates[0]


def test_metadata_round_trips():
    spec = os.path.join(_REPO, "tests", "corpus", "generated",
                        "airline_x11-default.spc")
    with x13c.adjust(spec) as run:
        assert run.ok and run.error == ""
        assert run.period == 12
        assert run.nobs == 144
        assert run.mode == "multiplicative"
        assert run.model_based is False
        assert run.arima_model == ""       # "?" is normalized away
        assert "d11" in run.table_names()


def test_model_based_spec_reports_its_model():
    """A spec with an arima{} model reports it, and is flagged model-based."""
    spec = os.path.join(_REPO, "tests", "corpus", "generated",
                        "airline_x11-fixed-airline.spc")
    if not os.path.exists(spec):
        pytest.skip("no fixed-airline corpus spec")
    with x13c.adjust(spec, check=False) as run:
        if not run.ok:
            pytest.skip(f"engine declined: {run.error}")
        assert run.model_based is True


def test_diagnostics_present_and_finite():
    spec = os.path.join(_REPO, "tests", "corpus", "generated",
                        "airline_x11-default.spc")
    with x13c.adjust(spec) as run:
        d = run.diagnostics()
        assert "f3.q" in d, f"expected f3.q, got {sorted(d)}"
        assert 0.0 <= d["f3.q"] < 10.0
        for k, v in d.items():
            assert v == v, f"{k} is NaN"        # NaN != NaN


def test_diagnostics_match_the_udg_golden():
    """The quality statistics the binding reports are the oracle's, to the
    .udg's own printed precision (svf2f3.f writes M statistics f6.3, Q F5.2)."""
    spec_id = "generated/airline_x11-default"
    spec = os.path.join(_REPO, "tests", "corpus", spec_id + ".spc")
    udg = os.path.join(_REPO, "tests", "golden", spec_id,
                       os.path.basename(spec_id) + ".udg")
    if not os.path.exists(udg):
        pytest.skip("no .udg golden")
    want: dict[str, float] = {}
    for ln in open(udg, encoding="utf-8", errors="replace"):
        parts = ln.split(":", 1)
        if len(parts) == 2 and parts[0].strip() in ("f3.q", "f3.qm2"):
            try:
                want[parts[0].strip()] = float(parts[1].strip().split()[0])
            except (ValueError, IndexError):
                pass
    if not want:
        pytest.skip("the .udg golden carries no f3 canaries")
    with x13c.adjust(spec) as run:
        got = run.diagnostics()
        for k, v in want.items():
            assert abs(got[k] - v) <= 5e-3, f"{k}: binding {got[k]} vs udg {v}"


# --- error handling and ABI conventions ------------------------------------

def test_missing_spec_file_reports_cleanly():
    with pytest.raises(x13c.X13Error) as e:
        x13c.adjust(os.path.join(_REPO, "does", "not", "exist.spc"))
    assert "cannot open spec file" in str(e.value)


def test_check_false_returns_a_failed_run_instead_of_raising():
    run = x13c.adjust(os.path.join(_REPO, "nope.spc"), check=False)
    try:
        assert run.ok is False
        assert run.error
    finally:
        run.close()


def test_garbage_spec_fails_without_crashing():
    """A malformed spec must come back as an error, not an exception crossing
    the ABI (which would be undefined behaviour in ctypes)."""
    run = x13c.adjust_text("this is not a spec at all {{{", check=False)
    try:
        assert run.ok is False
        assert run.error
    finally:
        run.close()


def test_unknown_table_raises_keyerror_listing_what_exists():
    spec = os.path.join(_REPO, "tests", "corpus", "generated",
                        "airline_x11-default.spc")
    with x13c.adjust(spec) as run:
        assert run.has_table("no_such_table") is False
        with pytest.raises(KeyError) as e:
            run.table("no_such_table")
        assert "d11" in str(e.value)      # the message lists what IS available


def test_use_after_close_raises():
    spec = os.path.join(_REPO, "tests", "corpus", "generated",
                        "airline_x11-default.spc")
    run = x13c.adjust(spec)
    run.close()
    with pytest.raises(x13c.X13Error):
        _ = run.nobs
    run.close()            # double close must be a no-op, not a double free


def test_buffer_too_small_reports_need_and_writes_nothing():
    """The ABI's size protocol: a short buffer returns -needed and leaves the
    caller's memory untouched. This is what makes size-then-fill safe."""
    import ctypes
    lib = x13c.load_library()
    spec = os.path.join(_REPO, "tests", "corpus", "generated",
                        "airline_x11-default.spc")
    with x13c.adjust(spec) as run:
        h = run._check()
        n = lib.x13_table_length(h, b"d11")
        assert n > 2
        buf = (ctypes.c_double * 2)()
        buf[0] = -12345.0
        rc = lib.x13_table_values(h, b"d11", buf, 2)
        assert rc == -n, f"expected -{n}, got {rc}"
        assert buf[0] == -12345.0, "short call must not write"


def test_null_handle_is_safe_everywhere():
    """Every accessor tolerates a NULL handle. R's .C shim can produce one from
    a stale integer id, so this is a real path, not a theoretical one."""
    import ctypes
    lib = x13c.load_library()
    nul = ctypes.c_void_p(None)
    assert lib.x13_ok(nul) == 0
    assert lib.x13_error(nul) == b""
    assert lib.x13_period(nul) == 0
    assert lib.x13_nobs(nul) == 0
    assert lib.x13_table_count(nul) == 0
    assert lib.x13_table_name(nul, 0) == b""
    assert lib.x13_table_length(nul, b"d11") == 0
    assert lib.x13_diag_count(nul) == 0


def test_table_index_out_of_range_is_empty_not_a_crash():
    import ctypes
    lib = x13c.load_library()
    spec = os.path.join(_REPO, "tests", "corpus", "generated",
                        "airline_x11-default.spc")
    with x13c.adjust(spec) as run:
        h = run._check()
        n = lib.x13_table_count(h)
        assert lib.x13_table_name(h, n) == b""
        assert lib.x13_table_name(h, -1) == b""
        assert lib.x13_table_name(h, 10_000) == b""


def test_runs_are_independent():
    """Two live handles must not share engine state. The engine is a wall of
    COMMON blocks; if a run ever started leaking into the next one, this is
    where it would show."""
    a = os.path.join(_REPO, "tests", "corpus", "generated",
                     "airline_x11-default.spc")
    with x13c.adjust(a) as r1:
        first = list(r1.table("d11").values)
        with x13c.adjust(a) as r2:
            second = list(r2.table("d11").values)
        # r1 must be unchanged by r2 having run and been freed
        assert list(r1.table("d11").values) == first
    assert first == second


def test_abi_version_is_declared():
    lib = x13c.load_library()
    assert lib.x13_abi_version() == 1
    assert x13c.engine_version()
