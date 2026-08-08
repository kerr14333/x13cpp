"""Composite + ``force{}`` gate -- agr3.f:404-547, the forced/rounded INDIRECT
seasonally adjusted series on the X-11 composite path.

``agr3s.f``'s twin of this block was ported with the SEATS composite branch and
the X-11 one was not, so ``force{}`` on an X-11 composite total computed no
``Stci2``/``Stcirn``, emitted none of ``iaa``/``iff``/``irn``, and returned
``OUTCOME: OK``. Two things kept that quiet:

* the composite harness dumped the three tables only from its ``agr3s`` branch,
  and
* it emitted no residual-seasonality F-test row at all, on either branch -- so
  ``agr3.f:417``'s ``ftest`` (the ``id11.f`` / ``id11.3y.f`` pair) was unported
  with no channel that could have said so. Both are wired now, which is why this
  file gates the savelog rows beside the tables.

ORACLE on-vs-off, ``census-examples/composite-fixed`` plus
``force{type=denton round=yes}``: three save files appear, ``adjtot`` flips
no->yes, and ``id11.f`` moves 0.02200 -> 0.87565 -- because ``:493``'s test on
the forced series and ``:537``'s on the rounded one overwrite ``:417``'s under
the same savelog key. Nothing else in the run moves: forcing is a tail, it does
not feed back into the decomposition.

The two corpus cases take the two arms of ``agr3.f:436``:

* ``census-examples/composite-force/``       -- ``indforce`` at its default yes:
  the indirect SA is benchmarked in its own right (qmap), then rounded (rndsa).
* ``census-examples/composite-force-indno/`` -- ``indforce=no``: the forced
  indirect series is the AGGREGATE of the components' own forced SA (``Ci2``),
  so the components carry ``force{}`` too.

Run:  python -m pytest tests/parity/test_composite_force.py -q
"""
from __future__ import annotations

import os
import re
import subprocess

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS_ROOT = os.path.join(_REPO, "tests", "corpus", "census-examples")
_GOLDEN_ROOT = os.path.join(_REPO, "tests", "golden", "census-examples")

_MTA = "composite.mta"
_TOTAL = "total"

# Same arithmetic floor as the rest of the composite family: the forced series is
# a benchmarking of a sum. Measured worst 2.5e-15 on iaa, 4.4e-16 on iff, exact
# on irn (rndsa returns whole units).
RTOL = 1e-12
# The .udg prints the F-test rows at five decimals, so a matching value is
# pinned to half of the last digit and no tighter.
ATOL_UDG = 5e-6

_GOLD_RE = re.compile(r"(\d{6})\s+([+\-][0-9.EeDd+\-]+)")

# case -> (tables the spec asks for and the oracle writes,
#          tables the spec CANNOT produce and the engine must not emit,
#          the expected `indforce` savelog value)
_CASES = {
    "composite-force": (["iaa", "iff", "irn"], [], "yes"),
    # No round=yes here, so `irn` is not merely absent from the golden -- the
    # spec never asked for it. Asserted as an absence, per the rule that an
    # absent golden is a claim and a skip checks nothing.
    "composite-force-indno": (["iaa", "iff"], ["irn"], "no"),
}


def _find_binary() -> str:
    for c in (os.path.join(_REPO, "build", "x13run_composite.exe"),
              os.path.join(_REPO, "build", "x13run_composite"),
              os.path.join(_REPO, "build", "Release", "x13run_composite.exe")):
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_COMPOSITE")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13run_composite binary not found; build it first (cmake --build build).")


BIN = _find_binary()


def _corpus(case: str) -> str:
    return os.path.join(_CORPUS_ROOT, case)


def _golden(case: str) -> str:
    return os.path.join(_GOLDEN_ROOT, case, _TOTAL)


def _have(case: str) -> bool:
    tags = _CASES[case][0]
    return (os.path.exists(os.path.join(_corpus(case), _MTA)) and
            all(os.path.exists(os.path.join(_golden(case), _TOTAL + "." + t))
                for t in tags))


_PRESENT = [c for c in _CASES if _have(c)]


def _read_golden(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _GOLD_RE.match(ln.strip())
            if m:
                out[m.group(1)] = float(
                    m.group(2).replace("D", "E").replace("d", "e"))
    return out


def _udg(case: str) -> dict[str, str]:
    out: dict[str, str] = {}
    p = os.path.join(_golden(case), _TOTAL + ".udg")
    if not os.path.exists(p):
        return out
    with open(p, encoding="utf-8", errors="replace") as f:
        for ln in f:
            if ":" in ln:
                k, _, v = ln.partition(":")
                out[k.strip()] = v.strip()
    return out


_RUNS: dict[str, str] = {}


def _run(case: str) -> str:
    if case not in _RUNS:
        r = subprocess.run([BIN, _MTA], cwd=_corpus(case), capture_output=True,
                           text=True)
        assert r.returncode == 0, f"harness exit {r.returncode}\n{r.stderr}"
        assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:400]
        _RUNS[case] = r.stdout
    return _RUNS[case]


def _emitted(run_output: str, tag: str) -> dict[str, float]:
    out: dict[str, float] = {}
    for ln in run_output.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == tag and re.fullmatch(r"\d{6}", p[1]):
            out[p[1]] = float(p[2])
    return out


def _scalar(run_output: str, key: str) -> list[str]:
    """`<key> <rest...>` lines -> the rest, one entry per line."""
    out = []
    for ln in run_output.splitlines():
        p = ln.split()
        if p and p[0] == key:
            out.append(" ".join(p[1:]))
    return out


def _check(case: str, tag: str) -> None:
    produced = _emitted(_run(case), tag)
    gold = _read_golden(os.path.join(_golden(case), _TOTAL + "." + tag))
    assert gold, f"{case}/{tag}: empty golden"
    keys = sorted(set(gold) & set(produced))
    assert len(keys) == len(gold), (
        f"{case}/{tag}: produced {len(produced)} rows, golden {len(gold)}, "
        f"overlap {len(keys)}")
    worst, worst_k = 0.0, None
    for k in keys:
        g, v = gold[k], produced[k]
        rel = abs(v - g) / abs(g) if g else abs(v - g)
        if rel > worst:
            worst, worst_k = rel, k
    assert worst <= RTOL, (
        f"{case}/{tag}: max rel err {worst:.3e} at {worst_k} (tol {RTOL:.0e})")


# --- the parametrisation floor ------------------------------------------------
# Both cases must be discovered. A corpus dir that goes missing, or a golden that
# was never blessed, otherwise shrinks this file to nothing and reports green.
def test_force_cases_discovered() -> None:
    assert sorted(_PRESENT) == sorted(_CASES), (
        f"expected both composite force cases, found {sorted(_PRESENT)}")


# --- the tables ---------------------------------------------------------------
@pytest.mark.parametrize(
    "case,tag",
    [(c, t) for c in sorted(_CASES) for t in _CASES[c][0]])
def test_forced_indirect_tables(case: str, tag: str) -> None:
    """iaa (forced indirect SA), iff (the forcing factor), irn (rounded)."""
    if case not in _PRESENT:
        pytest.skip(f"{case} corpus/golden not present")
    _check(case, tag)


@pytest.mark.parametrize(
    "case,tag",
    [(c, t) for c in sorted(_CASES) for t in _CASES[c][1]])
def test_forced_indirect_absent(case: str, tag: str) -> None:
    """A table the SPEC cannot ask for must not be emitted."""
    if case not in _PRESENT:
        pytest.skip(f"{case} corpus/golden not present")
    produced = _emitted(_run(case), tag)
    assert not produced, (
        f"{case}: engine emitted {len(produced)} rows of `{tag}`, which this "
        f"spec never requested (no round=yes)")
    assert not os.path.exists(os.path.join(_golden(case), _TOTAL + "." + tag)), (
        f"{case}: the oracle DID write {tag}; the absence assertion above is "
        f"gating the wrong thing")


@pytest.mark.parametrize("case", sorted(_CASES))
def test_indforce_savelog(case: str) -> None:
    """agr3.f:437/477's `indforce:` line -- which arm of :436 the run took."""
    if case not in _PRESENT:
        pytest.skip(f"{case} corpus/golden not present")
    want = _CASES[case][2]
    gold = _udg(case).get("indforce")
    assert gold == want, f"{case}: golden indforce={gold!r}, expected {want!r}"
    got = _scalar(_run(case), "indforce")
    assert got == [want], f"{case}: engine indforce={got!r}, oracle {want!r}"


# --- the residual-seasonality F-test rows -------------------------------------
# ftest.f:188/221. `id11.f` is agr3's, written three times over on a forced +
# rounded run (:417 on Stci, :493 on Stci2, :537 on Stcirn) -- last write wins,
# which is the whole reason a forced run's value differs so much from an
# unforced one's. `d11.f` is the total's own DIRECT adjustment, from x11pt3.
#
# The gate runs over EVERY composite case that ships these rows, not just the two
# force ones, and that is load-bearing: on a forced run :417's write is
# overwritten twice, so deleting it leaves a force-only gate byte-identical. The
# mutation that removes `agr3.f:417` measured exactly zero until the unforced
# cases were pulled in here. Cases are discovered from the goldens on disk rather
# than listed, with a floor below.
_FTEST_KEYS = ["d11.f", "d11.3y.f", "id11.f", "id11.3y.f"]


def _discover_udg_cases() -> list[str]:
    out = []
    if not os.path.isdir(_GOLDEN_ROOT):
        return out
    for case in sorted(os.listdir(_GOLDEN_ROOT)):
        if not os.path.exists(os.path.join(_corpus(case), _MTA)):
            continue
        u = os.path.join(_GOLDEN_ROOT, case, _TOTAL, _TOTAL + ".udg")
        if not os.path.exists(u):
            continue
        with open(u, encoding="utf-8", errors="replace") as f:
            if any(ln.startswith("id11.f:") for ln in f):
                out.append(case)
    return out


_UDG_CASES = _discover_udg_cases()


def test_ftest_cases_discovered() -> None:
    """Both force cases plus the unforced composites that carry the rows."""
    assert len(_UDG_CASES) >= 4, (
        f"expected at least 4 composite cases with id11.f goldens, found "
        f"{_UDG_CASES}")
    for c in _CASES:
        assert c in _UDG_CASES, f"{c} missing from the F-test row gate"


@pytest.mark.parametrize("case", _UDG_CASES)
@pytest.mark.parametrize("key", _FTEST_KEYS)
def test_ftest_savelog_rows(case: str, key: str) -> None:
    gold = _udg(case).get(key)
    got = _scalar(_run(case), key)
    if not gold:
        # Not a hole in the golden -- a SEATS-adjusted total has no direct D11
        # and the oracle writes no `d11.f` for it. The engine must write none
        # either; skipping here would assert nothing, which is how a spurious
        # savelog row survives.
        assert not got, (
            f"{case}: oracle writes no `{key}`, engine emitted {got!r}")
        return
    want = [float(x) for x in gold.split()]
    assert len(got) == 1, f"{case}: engine emitted {len(got)} `{key}` rows"
    have = [float(x) for x in got[0].split()]
    assert len(have) == len(want) == 2, f"{case}/{key}: {got[0]!r} vs {gold!r}"
    for h, w in zip(have, want):
        assert abs(h - w) <= ATOL_UDG, (
            f"{case}/{key}: engine {have} vs oracle {want} (tol {ATOL_UDG})")


# --- the force actually moved something ---------------------------------------
# Without this the three table gates above would pass just as well against an
# engine whose "forced" series is the unforced one: `iaa` is `isa` benchmarked,
# and on a well-behaved synthetic series the two are close. Pin that they are
# nonetheless distinguishable, so a no-op force cannot ride through.
@pytest.mark.parametrize("case", sorted(_CASES))
def test_force_is_not_a_no_op(case: str) -> None:
    if case not in _PRESENT:
        pytest.skip(f"{case} corpus/golden not present")
    out = _run(case)
    isa, iaa = _emitted(out, "isa"), _emitted(out, "iaa")
    keys = sorted(set(isa) & set(iaa))
    assert keys, f"{case}: no overlap between isa and iaa"
    worst = max(abs(iaa[k] - isa[k]) / abs(isa[k]) for k in keys)
    assert worst > 1e-6, (
        f"{case}: iaa is within {worst:.3e} of isa everywhere -- the forced "
        f"series is indistinguishable from the unforced one")
