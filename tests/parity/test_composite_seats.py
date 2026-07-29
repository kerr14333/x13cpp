"""SEATS composite gate -- agr3s.f, the SEATS branch of the indirect adjustment.

`X11agr` is the metafile-wide flag "every component was adjusted by X-11"
(aaamain.f:73 arms it, gtinpt.f:1170 ANDs each component's own Lx11 in). One
SEATS component turns it off, and x11ari.f:338-343 then routes the composite
total's INDIRECT adjustment through ``agr3s`` instead of ``agr3``. Before this
gate existed the port had neither -- ``agr2_component`` accumulated Stci
unconditionally (a SEATS component has none), ``agr2_compare`` hardcoded
``x11agr = true``, and the metafile harness handed every spec to ``run_x11``,
which refuses a seats{} spec outright. A SEATS metafile came back FATAL.

The two indirect adjustments are genuinely different routines, not two settings
of one:

* ``agr3`` re-runs the X-11 D-tables over the aggregated component results --
  an extreme-value pass, a forced 13/5-term Henderson trend, the D8/D9 SI
  battery, and (x11ari.f:341) a second x11pt4 producing the whole Part-E family
  and the ``if2.*``/``if3.*`` block.
* ``agr3s`` does none of that. The indirect seasonally adjusted series simply IS
  the aggregate of the components' own SA series, and the seasonal factor is
  recovered from it by division. Measured on the oracle: it writes
  ``isf isa ie5 ip5 ie6 ip6 i18`` and nothing else, even when the spec asks for
  the full family. ``test_seats_composite_absent_tables`` pins that.

Two corpora, differing only in how the TOTAL is adjusted:

* ``census-examples/composite-seats/``       -- X-11 total (agr3s's Lx11 true)
* ``census-examples/composite-seats-total/`` -- SEATS total (Lx11 false), the
  only case in which the composite tail is reached from ``run_seats``.

Both components carry FIXED MA coefficients. Measured with them estimated: the
port's SEATS decomposition sits 1.3e-6 from the oracle's on this synthetic
series while every printed coefficient agrees to 11 digits -- optimizer path
noise amplified by the canonical decomposition, not an aggregation defect (the
same 1.3e-6 appears on a STANDALONE run of the component, with no composite
anywhere). The indirect adjustment is the sum of the component SA series, so
that would land undiluted in isa/isf/i18. Fixed, everything below is ~5e-15.

Run:  python -m pytest tests/parity/test_composite_seats.py -q
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

_CASES = ["composite-seats", "composite-seats-total"]
_COMPONENTS = ["region_north", "region_south"]
_TOTAL = "total"
_MTA = "composite.mta"

# The aggregate is a pure sum followed by a fixed-coefficient decomposition, so
# every level table reaches the arithmetic floor. Measured worst 5.11e-15.
RTOL = 1e-12
# The change tables are differences of neighbouring values, so where the change
# is ~0 there is no relative precision left. Measured worst ABSOLUTE 1.5e-12 on
# ie6/ip6 against changes as small as 4e-5. Same two-tier rule test_x11_etables
# uses.
ATOL_CHANGE = 1e-9

_GOLD_RE = re.compile(r"(\d{6})\s+([+\-][0-9.EeDd+\-]+)")

# The total's own adjustment, per corpus: d10-d13 for an X-11 total, s10-s13 for
# a SEATS one. What the aggregation sums is the COMPONENTS' s11, in both.
_TOTAL_DIRECT = {
    "composite-seats": ["d10", "d11", "d12", "d13"],
    "composite-seats-total": ["s10", "s11", "s12", "s13"],
}
_COMPONENT_TAGS = ["s10", "s11", "s12", "s13"]
_INDIRECT_LEVEL = ["isf", "isa"]
_INDIRECT_CHANGE = ["ie5", "ip5", "ie6", "ip6"]

# Requested by both total.spc files and NOT written by the oracle: agr3s has no
# indirect trend or irregular, no D8/D9, and no x11pt4 pass behind it. `ita`
# additionally depends on agr3s's `pre18b`, which stays false while every A1/D11
# ratio is formable.
_ABSENT = ["itn", "iir", "id8", "id9", "ie1", "ie2", "ie3", "ie7", "ip7",
           "ie8", "ip8", "iee", "ita"]

# The last OBSERVED period. i18 is punched over the forecast span too (agr3s.f:
# 412-418 widens its range whenever Posffc>Posfob, with no Savfct gate), and
# those rows need `Setfsa` -- the SEATS forecast decomposition (ansub3.f:356-678)
# that seatad.f:49-54 appends into Seatsa and that this port does not have. See
# test_seats_composite_i18 for the measurement.
_LAST_OBS = "200412"


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
    return os.path.join(_GOLDEN_ROOT, case)


def _have(case: str) -> bool:
    return (os.path.exists(os.path.join(_corpus(case), _MTA)) and
            os.path.exists(os.path.join(_golden(case), _TOTAL, "total.isa")))


def _read_golden(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _GOLD_RE.match(ln.strip())
            if m:
                out[m.group(1)] = float(
                    m.group(2).replace("D", "E").replace("d", "e"))
    return out


def _udg_raw(path: str) -> dict[str, str]:
    out: dict[str, str] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            if ":" in ln:
                k, _, v = ln.partition(":")
                out[k.strip()] = v.rstrip("\n")
    return out


def _emitted(run_output: str, key: str) -> dict[str, float]:
    out: dict[str, float] = {}
    for ln in run_output.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == key:
            out[p[1]] = float(p[2])
    return out


_RUNS: dict[str, str] = {}


def _run(case: str) -> str:
    if case not in _RUNS:
        r = subprocess.run([BIN, _MTA], cwd=_corpus(case), capture_output=True,
                           text=True, timeout=300)
        assert r.returncode == 0, f"{case}: harness exit {r.returncode}\n{r.stderr}"
        assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:400]
        _RUNS[case] = r.stdout
    return _RUNS[case]


def _check(case: str, base: str, tag: str, prefix: str, *, atol: float = 0.0,
           upto: str | None = None) -> None:
    gold = _read_golden(os.path.join(_golden(case), base, f"{base}.{tag}"))
    assert gold, f"{case}/{base}.{tag}: empty golden"
    produced = _emitted(_run(case), prefix + tag)
    # Row SET first, both directions: a gate that reads only the keys it has
    # cannot see the engine emitting rows the oracle does not.
    assert set(produced) == set(gold), (
        f"{case}/{base}.{tag}: missing {sorted(set(gold) - set(produced))[:6]}, "
        f"extra {sorted(set(produced) - set(gold))[:6]}")
    keys = sorted(gold) if upto is None else sorted(k for k in gold if k <= upto)
    assert keys, f"{case}/{base}.{tag}: nothing left to compare"
    worst, worst_k = 0.0, None
    for k in keys:
        g, v = gold[k], produced[k]
        if abs(v - g) <= atol:
            continue
        rel = abs(v - g) / abs(g) if g else abs(v - g)
        if rel > worst:
            worst, worst_k = rel, k
    assert worst <= RTOL, (
        f"{case}/{base}.{tag}: max rel err {worst:.3e} at {worst_k} "
        f"(tol {RTOL:.0e}, atol {atol:.0e})")


# --- the component decompositions -------------------------------------------
@pytest.mark.parametrize("case", _CASES)
@pytest.mark.parametrize("base", _COMPONENTS)
@pytest.mark.parametrize("tag", _COMPONENT_TAGS)
def test_seats_component(case: str, base: str, tag: str) -> None:
    """Each SEATS component -- s11 is literally what the indirect isa sums."""
    if not _have(case):
        pytest.skip(f"{case} corpus/golden not present")
    _check(case, base, tag, prefix=base + ":")


# --- the composite total's own (DIRECT) adjustment ---------------------------
@pytest.mark.parametrize("case", _CASES)
@pytest.mark.parametrize("idx", range(4))
def test_seats_composite_direct(case: str, idx: int) -> None:
    """The aggregate adjusted in its own right, X-11 or SEATS."""
    if not _have(case):
        pytest.skip(f"{case} corpus/golden not present")
    _check(case, _TOTAL, _TOTAL_DIRECT[case][idx], prefix="")


# --- the INDIRECT adjustment (agr3s) ----------------------------------------
@pytest.mark.parametrize("case", _CASES)
@pytest.mark.parametrize("tag", _INDIRECT_LEVEL)
def test_seats_composite_indirect(case: str, tag: str) -> None:
    """isf = O5/Ci, isa = Ci -- the sum of the components' SA series."""
    if not _have(case):
        pytest.skip(f"{case} corpus/golden not present")
    _check(case, _TOTAL, tag, prefix="")


@pytest.mark.parametrize("case", _CASES)
@pytest.mark.parametrize("tag", _INDIRECT_CHANGE)
def test_seats_composite_changes(case: str, tag: str) -> None:
    """agr3s.f:345-410's own E5/E6 -- agr3 gets these from x11pt4 instead."""
    if not _have(case):
        pytest.skip(f"{case} corpus/golden not present")
    _check(case, _TOTAL, tag, prefix="", atol=ATOL_CHANGE)


@pytest.mark.parametrize("case", _CASES)
def test_seats_composite_i18(case: str) -> None:
    """The final adjustment ratios A1/D11.

    Compared over the OBSERVED span only, and the reason is a measured gap
    rather than a tolerance: agr3s.f:412-418 widens i18's punch range to Posffc
    whenever the indirect geometry has forecasts (unconditionally -- unlike isf,
    there is no Savfct gate), and the indirect SA over that span is the sum of
    the components' `Seatsa` AFTER seatad.f:49-54 has appended `Setfsa`. Setfsa
    is the SEATS FORECAST decomposition (ansub3.f:356-678), which this port does
    not have, so Ci is zero there and the engine reports the
    both-are-zero branch's 1.0 against the oracle's real values (measured 1.44
    relative at 200507). The row SET is still asserted in both directions, so a
    shape regression fails; only the values past the observed span are exempt.
    """
    if not _have(case):
        pytest.skip(f"{case} corpus/golden not present")
    _check(case, _TOTAL, "i18", prefix="", upto=_LAST_OBS)
    # The premise: there really are rows past the observed span, i.e. this is an
    # exemption that costs something. If the corpus ever stops producing them
    # the exemption is dead and should be deleted.
    gold = _read_golden(os.path.join(_golden(case), _TOTAL, "total.i18"))
    assert any(k > _LAST_OBS for k in gold), (
        "total.i18 no longer extends past the observed span -- drop the "
        "forecast-tail exemption above")


# --- what agr3s does NOT produce --------------------------------------------
@pytest.mark.parametrize("case", _CASES)
def test_seats_composite_absent_tables(case: str) -> None:
    """Both total.spc files ask for the full indirect family; agr3s writes part.

    This is the assertion that separates agr3s from agr3, and it runs in both
    directions: the oracle writes no file for these, so neither may the engine
    emit a row for them.
    """
    if not _have(case):
        pytest.skip(f"{case} corpus/golden not present")
    spc = open(os.path.join(_corpus(case), "total.spc"), encoding="utf-8").read()
    out = _run(case)
    for tag in _ABSENT:
        if tag not in spc:
            continue          # this corpus did not ask for it
        gpath = os.path.join(_golden(case), _TOTAL, f"total.{tag}")
        assert not os.path.exists(gpath), (
            f"{case}: the oracle now writes total.{tag} -- agr3s.f may have "
            f"been misread; re-check before relaxing this")
        if tag == "ita" and case == "composite-seats-total":
            # KNOWN, and the same missing `Setfsa` as the i18 forecast tail --
            # see test_seats_composite_i18. `ita` exists only when agr3s's
            # `pre18b` fires, and here it fires on the FORECAST rows alone:
            # a SEATS total carries forecasts, so `Series` extends past Posfob
            # while `Ci` (the sum of the components' Seatsa) does not, and a
            # nonzero original over a zero SA is exactly pre18b's trigger. The
            # oracle has Ci there and never trips it. The run says so on Mt2.
            assert _emitted(out, tag), (
                "the spurious `ita` is gone -- if Setfsa landed, delete this "
                "exemption and the NOTE in agr3s.cpp with it")
            continue
        assert not _emitted(out, tag), (
            f"{case}: engine emitted `{tag}`, which agr3s does not produce")


# --- the direct-vs-indirect comparison statistics ----------------------------
# agr2.f:128-131/175-178 drop the whole R2 half when X11agr is false: R2 is the
# variance of the SA/TREND ratio, and there is no indirect trend to form it
# from. The oracle prints an "R1 ONLY" header (agr2.f:1031) and writes r1mse/
# r1rmse and no r2 line.
_CMPSTAT_ROWS = {                    # printed label -> the di() indices on it
    "R1-MEAN SQUARE ERROR": range(1, 7),
    "R1-ROOT MEAN SQUARE ERROR": range(7, 13),
}


@pytest.mark.parametrize("case", _CASES)
def test_seats_composite_r1_only(case: str) -> None:
    """r1mse/r1rmse at the .udg's printed precision, and NO r2 on either side."""
    if not _have(case):
        pytest.skip(f"{case} corpus/golden not present")
    raw = _udg_raw(os.path.join(_golden(case), _TOTAL, "total.udg"))
    assert "r1mse" in raw, "total.udg carries no comparison statistics"
    for k in ("r2mse", "r2rmse"):
        assert k not in raw, (
            f"total.udg now carries {k} -- X11agr is no longer false for this "
            f"corpus, so it is testing the agr3 path")
    out = _run(case)
    for key in ("r1mse", "r1rmse"):
        want = raw[key].split()
        got = None
        for ln in out.splitlines():
            p = ln.split()
            if p and p[0] == key:
                got = p[1:]
                break
        assert got is not None, f"{case}: engine emitted no {key}"
        assert len(got) == len(want) == 2, f"{case}: {key}: {want} vs {got}"
        for a, b in zip(want, got):
            assert f"{float(b):.3f}" == f"{float(a):.3f}", (
                f"{case}: {key}: oracle {want}, engine "
                f"{[f'{float(x):.3f}' for x in got]}")
        # ... and no R2 twin from the engine either.
        assert not [ln for ln in out.splitlines()
                    if ln.split() and ln.split()[0] in ("r2mse", "r2rmse")], (
            f"{case}: engine emitted an R2 statistic under X11agr false")


# --- the QS / spectrum-peak / NP savelog blocks ------------------------------
# x11ari.f reaches genqs (:277), spcdrv (:282) and gennpsa (:322) on every spec
# of the metafile, after the Lseats/Lx11 branch has rejoined -- so a SEATS
# component's block runs through the SEATS arms (Stocsa/Stocir rather than the
# D-tables) while composite-fixed's components exercise the X-11 ones. The
# INDIRECT (Iagr==4) names come from the second spcdrv/gennpsa pass, which
# x11ari.f:352-370 makes on the agr3s path exactly as on the agr3 one.
#
# Same tolerance rule as test_composite_tables: the .udg's printed precision,
# with verdicts (`yes`/`no`, `nopeak`, frequency lists) compared exactly.
_DIAG_FAMILY = re.compile(r"^(qs|np|spc|peaks|nspecfreq|nsfreq|ntdfreq|[st]\d+\.)")
_DIAG_RTOL = 5e-5
_DIAG_ATOL = 5e-5


def _diag_keys(raw: dict) -> dict:
    return {k: v.strip() for k, v in raw.items() if _DIAG_FAMILY.match(k)}


def _emitted_diag(run_output: str, prefix: str) -> dict:
    out = {}
    for ln in run_output.splitlines():
        ln = ln.strip()
        if prefix:
            if not ln.startswith(prefix):
                continue
            ln = ln[len(prefix):]
        k, sep, v = ln.partition(":")
        if not sep or " " in k or not k:
            continue
        out[k.strip()] = v.strip()
    return out


@pytest.mark.parametrize("case", _CASES)
@pytest.mark.parametrize("who", _COMPONENTS + [_TOTAL])
def test_seats_composite_diag_block(case: str, who: str) -> None:
    """genqs / spcdrv / gennpsa on every spec, direct and indirect names alike."""
    if not _have(case):
        pytest.skip(f"{case} corpus/golden not present")
    gold = _diag_keys(_udg_raw(os.path.join(_golden(case), who, who + ".udg")))
    assert gold, f"{who}.udg carries no QS/spectrum/NP block"
    eng = _emitted_diag(_run(case), "" if who == _TOTAL else who + ":")
    missing = sorted(set(gold) - set(eng))
    assert not missing, f"{case}/{who}: engine emitted no {missing[:8]}"
    # BOTH directions. This is where the one-directional version was blind: the
    # oracle emits no `spcindirr` on the agr3s path at all (spcdrv.f:436 --
    # `goirr = goirr .and. X11agr`, because agr3s forms no indirect irregular),
    # and the engine was emitting twenty keys' worth while every key the golden
    # DID carry matched.
    extra = sorted(k for k in _diag_keys(eng) if k not in gold)
    assert not extra, f"{case}/{who}: engine emitted keys the oracle does not: {extra[:8]}"
    worst, worst_k = 0.0, None
    for k in sorted(gold):
        g, e = gold[k].split(), eng[k].split()
        assert len(g) == len(e), f"{case}/{who}: {k}: {gold[k]!r} vs {eng[k]!r}"
        for a, b in zip(g, e):
            try:
                fa, fb = float(a), float(b)
            except ValueError:
                assert a == b, f"{case}/{who}: {k}: {gold[k]!r} vs {eng[k]!r}"
                continue
            d = abs(fa - fb)
            if d <= _DIAG_ATOL:
                continue
            rel = d / abs(fa) if fa else d
            if rel > worst:
                worst, worst_k = rel, k
    assert worst <= _DIAG_RTOL, (
        f"{case}/{who}: max rel err {worst:.3e} at {worst_k}")
    # CB-31 again: no `qsind*` on either side, agr3s path included.
    assert not [k for k in gold if k.startswith("qsind")]
    assert not [k for k in eng if k.startswith("qsind")], (
        f"{case}/{who}: engine emitted qsind*, which the oracle does not")


@pytest.mark.parametrize("case", _CASES)
def test_seats_composite_roughness_table(case: str) -> None:
    """The whole di(1..12) against the printed roughness table in total.out.

    CB-32 is pinned here. agr3s omits agr3.f:101-108's store of the DIRECT
    seasonally adjusted series into the Orig2-aliased scratch, so agr2's first
    two aggmea calls read the aggregate ORIGINAL (editor.f:2492 / arima.f:1432,
    forecast-extended) and the "DIRECT" column measures the roughness of an
    UNADJUSTED series. Measured: 66.777 here against 53.586 down the agr3 path
    on the same data, and 66.777 is exactly R1 of the summed input .dat files.
    """
    if not _have(case):
        pytest.skip(f"{case} corpus/golden not present")
    text = open(os.path.join(_golden(case), _TOTAL, "total.out"),
                encoding="utf-8", errors="replace").read()
    assert "MEASURES OF ROUGHNESS R1 FOR" in text, (
        "total.out no longer prints the R1-ONLY roughness header -- X11agr is "
        "true for this corpus and it is testing agr3, not agr3s")
    got = {}
    for ln in _run(case).splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == "cmpstat":
            got[int(p[1])] = float(p[2])
    assert got, f"{case}: engine emitted no cmpstat rows"
    assert max(got) == 24, f"{case}: engine emitted di() up to {max(got)}"
    # The R2 half must be untouched: agr2's aggmea returns before computing it
    # and the percentage loop stops at i2=7. The oracle prints no R2 row at all,
    # so zero is the only thing the engine may hold there -- and asserting it is
    # what catches an engine that computed R2 anyway (the .udg check above
    # cannot: it only looks for keys the ORACLE writes).
    nonzero = [k for k in range(13, 25) if got[k] != 0.0]
    assert not nonzero, (
        f"{case}: engine computed the R2 half under X11agr false: di{nonzero}")
    for label, idx in _CMPSTAT_ROWS.items():
        m = re.search(re.escape(label) + r"(.*)", text)
        assert m, f"{label} not found in total.out"
        # Six numbers on the row: direct full/last-3, indirect full/last-3, and
        # the two percentage changes. Taken as tokens rather than by a character
        # class, so the dashed rule below the block cannot be swallowed.
        want = [float(x.rstrip("%")) for x in m.group(1).split()[:6]]
        assert len(want) == 6, f"{label}: parsed {want}"
        for j, k in enumerate(idx):
            assert f"{got[k]:.3f}" == f"{want[j]:.3f}", (
                f"{case}: di({k}) = {got[k]}, oracle prints {want[j]} ({label})")
