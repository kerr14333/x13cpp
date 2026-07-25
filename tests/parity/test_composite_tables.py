"""Composite (metafile / indirect adjustment) gate -- increments 1 and 2:
the DIRECT composite total and the INDIRECT adjustment.

Composite adjustment is the one X-13 feature that does not fit in a single spec
run: the oracle executes every spec of a metafile in ONE process so the
aggregation COMMONs (/mq11/ agr.cmn, /agreg/ agrsrs.cmn) persist across them, and
that persistence IS the feature. ``x13run_composite`` reproduces it -- each spec
gets its own X13Context, with the two aggregation blocks carried from one to the
next -- and prints the composite total's X-11 tables unprefixed, each component's
prefixed ``<base>:``.

Gated here (all bit-exact, ~5e-15):

* each COMPONENT's d10-d13 (printed ``<base>:d10`` etc.),
* the composite total's DIRECT d10-d13 -- the aggregate `O` summed from the
  components' originals (agr.f/agr1.f/getcmp.f + agr2.f), handed to the
  ``composite{}`` spec as its series and adjusted like any other run,
* the composite total's INDIRECT isf/isa/itn/iir (agr3.f/agrxpt.f + the
  O1..O5/Ci/Omod buffers) -- the seasonal adjustment rebuilt from the aggregated
  component RESULTS rather than from the aggregate.

The two are genuinely different objects: `isa` is exactly the sum of the
components' d11 (verified to 5e-16 on both the oracle and the engine side),
while d11 is the aggregate adjusted in its own right.

The corpus case is ``census-examples/composite-fixed/`` -- the same synthetic
data as the shipped ``composite/`` example but with a FIXED airline model on the
components, precisely because the indirect adjustment is that sum: any component
estimation drift lands undiluted in it. (With ``composite/``'s automdl{} both
components sit ~1e-4 out and the indirect tables inherit exactly that -- an
automdl-front issue, not an aggregation one. ``composite/`` stays untouched as
the illustrative Census-style example and as an automdl identification case.)

Increment 3 adds the direct-vs-indirect COMPARISON STATISTICS (agr2.f's Iagr==4
branch + aggmea.f): the R1/R2 measures of roughness of both adjustments, over the
full series and the last three years, plus the percentage change between them.
Those are gated here two ways -- the whole di(1..24) against the printed
roughness table in ``total.out``, and the four savelog canaries against
``total.udg`` -- at the oracle's own printed precision (3 decimals), which is all
either output carries.

Increment 4 adds the INDIRECT DIAGNOSTICS front, which x11pt4 had been blocking:
agr3.f:288-350's indirect D8/D9 SI ratios and seasonality test battery, plus the
second x11pt4 pass at x11ari.f:341 -- the whole if2.*/if3.* savelog block and the
sixteen id8/id9/ie*/ip*/iee/i18/ita save tables. See the two increment-4 sections
at the bottom of this file.

Still deferred (tools/composite_scouting.md): cmpchi's chi-square/F diagnostics,
the aggregate-composition header table, the forced/rounded indirect series, the
SEATS branch (agr3s.f) and pseudo-additive.

Run:  python -m pytest tests/parity/test_composite_tables.py -q
"""
from __future__ import annotations

import os
import re
import subprocess

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus", "census-examples", "composite-fixed")
_GOLDEN = os.path.join(_REPO, "tests", "golden", "census-examples",
                       "composite-fixed")

# The aggregate is a pure sum of the component originals followed by an ordinary
# X-11 decomposition, so it reaches the arithmetic floor. Measured worst 5.1e-15.
RTOL = 1e-12

_TAGS = ["d10", "d11", "d12", "d13"]              # direct (aggregate adjusted)
_IND_TAGS = ["isf", "isa", "itn", "iir"]          # indirect (sum of components)
_COMPONENTS = ["region_north", "region_south"]
_GOLD_RE = re.compile(r"(\d{6})\s+([+\-][0-9.EeDd+\-]+)")
_MTA = "composite.mta"
_TOTAL = "total"     # the last spec of the metafile: the composite total


def _find_binary() -> str:
    cands = [
        os.path.join(_REPO, "build", "x13run_composite.exe"),
        os.path.join(_REPO, "build", "x13run_composite"),
        os.path.join(_REPO, "build", "Release", "x13run_composite.exe"),
    ]
    for c in cands:
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_COMPOSITE")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13run_composite binary not found; build it first (cmake --build build).")


BIN = _find_binary()


def _read_golden(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _GOLD_RE.match(ln.strip())
            if m:
                out[m.group(1)] = float(m.group(2).replace("D", "E").replace("d", "e"))
    return out


_HAVE = (os.path.exists(os.path.join(_CORPUS, _MTA)) and
         all(os.path.exists(os.path.join(_GOLDEN, _TOTAL, _TOTAL + "." + t))
             for t in _TAGS + _IND_TAGS))


def _check(run_output: str, base: str, tag: str, prefix: str) -> None:
    """Compare one emitted table against its golden, period-key exact."""
    produced: dict[str, float] = {}
    want = prefix + tag
    for ln in run_output.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == want:
            produced[p[1]] = float(p[2])

    gold = _read_golden(os.path.join(_GOLDEN, base, base + "." + tag))
    assert gold, f"{base}.{tag}: empty golden"

    keys = sorted(set(gold) & set(produced))
    assert len(keys) == len(gold), (
        f"{base}.{tag}: produced {len(produced)} rows, golden {len(gold)}, "
        f"overlap {len(keys)}")

    worst = 0.0
    worst_k = None
    for k in keys:
        g, v = gold[k], produced[k]
        rel = abs(v - g) / abs(g) if g else abs(v - g)
        if rel > worst:
            worst, worst_k = rel, k
    assert worst <= RTOL, (
        f"{base}.{tag}: max rel err {worst:.3e} at {worst_k} (tol {RTOL:.0e})")


@pytest.fixture(scope="module")
def run_output() -> str:
    r = subprocess.run([BIN, _MTA], cwd=_CORPUS, capture_output=True, text=True)
    assert r.returncode == 0, f"harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:400]
    return r.stdout


@pytest.mark.skipif(not _HAVE, reason="composite metafile corpus/golden not present")
@pytest.mark.parametrize("tag", _TAGS)
def test_composite_total_direct(run_output: str, tag: str) -> None:
    """The aggregate adjusted in its own right (unprefixed d10-d13)."""
    _check(run_output, _TOTAL, tag, prefix="")


@pytest.mark.skipif(not _HAVE, reason="composite metafile corpus/golden not present")
@pytest.mark.parametrize("tag", _IND_TAGS)
def test_composite_total_indirect(run_output: str, tag: str) -> None:
    """The adjustment rebuilt from the aggregated component results."""
    _check(run_output, _TOTAL, tag, prefix="")


@pytest.mark.skipif(not _HAVE, reason="composite metafile corpus/golden not present")
@pytest.mark.parametrize("base", _COMPONENTS)
@pytest.mark.parametrize("tag", _TAGS)
def test_composite_component(run_output: str, base: str, tag: str) -> None:
    """Each component's own adjustment -- what the indirect tables are built from."""
    _check(run_output, base, tag, prefix=base + ":")


# --- increment 3: the direct-vs-indirect comparison statistics ----------------
#
# Both the printed table and the savelog carry these at 3 decimals, so they are
# compared as the oracle formats them: same rounded string, which pins the value
# to +/-5e-4 absolute. That is the tightest statement either output supports.
_ROWS = {                       # printed label -> the di() indices on that row
    "R1-MEAN SQUARE ERROR": range(1, 7),
    "R1-ROOT MEAN SQUARE ERROR": range(7, 13),
    "R2-MEAN SQUARE ERROR": range(13, 19),
    "R2-ROOT MEAN SQUARE ERROR": range(19, 25),
}
_NUM_RE = re.compile(r"-?\d+\.\d+")
_HAVE_STATS = _HAVE and os.path.exists(os.path.join(_GOLDEN, _TOTAL, _TOTAL + ".out"))


def _emitted_scalars(run_output: str, tag: str) -> dict[str, str]:
    """`<tag> <key> <value>` lines -> {key: value}."""
    out = {}
    for ln in run_output.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == tag:
            out[p[1]] = p[2]
    return out


@pytest.mark.skipif(not _HAVE_STATS, reason="composite golden .out not present")
def test_composite_roughness_table(run_output: str) -> None:
    """di(1..24) against the MEASURES OF ROUGHNESS table in total.out."""
    got = _emitted_scalars(run_output, "cmpstat")
    assert len(got) == 24, f"harness emitted {len(got)} cmpstat values, want 24"

    with open(os.path.join(_GOLDEN, _TOTAL, _TOTAL + ".out"),
              encoding="utf-8", errors="replace") as f:
        lines = f.read().splitlines()

    seen = 0
    for label, idx in _ROWS.items():
        row = next((ln for ln in lines if ln.strip().startswith(label)), None)
        assert row is not None, f"{label} row missing from total.out"
        nums = _NUM_RE.findall(row[len(label) + row.index(label):])
        assert len(nums) == 6, f"{label}: parsed {nums}"
        for k, want in zip(idx, nums):
            assert f"{float(got[str(k)]):.3f}" == f"{float(want):.3f}", (
                f"di({k}) = {got[str(k)]}, oracle prints {want} ({label})")
            seen += 1
    assert seen == 24


@pytest.mark.skipif(not _HAVE_STATS, reason="composite golden .udg not present")
def test_composite_savelog_canaries(run_output: str) -> None:
    """indtrendma + r1mse/r1rmse/r2mse/r2rmse against total.udg."""
    udg = {}
    with open(os.path.join(_GOLDEN, _TOTAL, _TOTAL + ".udg"),
              encoding="utf-8", errors="replace") as f:
        for ln in f:
            if ":" in ln:
                k, _, v = ln.partition(":")
                udg[k.strip()] = v.split()

    lines = {p[0]: p[1:] for p in (ln.split() for ln in run_output.splitlines())}
    assert lines.get("indtrendma") == udg["indtrendma"], "indirect Henderson length"
    for key in ("r1mse", "r1rmse", "r2mse", "r2rmse"):
        assert key in udg, f"{key} missing from total.udg"
        got = [f"{float(v):.3f}" for v in lines[key]]
        want = [f"{float(v):.3f}" for v in udg[key]]
        assert got == want, f"{key}: {got} vs oracle {want}"


# --- increment 4: the INDIRECT diagnostics block (if2.* / if3.*) --------------
#
# x11ari.f:341 runs the SAME x11pt4 a second time, over the buffers agr3 installs,
# so the aggregate gets a full second set of Part-F summary measures and F3
# quality statistics -- what the .udg writes with an `i` prefix. Two pieces had to
# land for this: agr3.f:288-334's indirect D8 SI ratios and seasonality test
# battery (ftest/kwtest/mstest/combft feed /tests/ Test1,Test2, which ARE the M7
# inputs, plus vsfa's Ratis = if2.is), and x11pt4's own Iagr==4 branches (O5 as
# the calendar-adjusted original behind E8, O rather than Series behind E18/EB).
#
# `if2.fsb1` is the ONE field with no indirect counterpart: it is Fpres from the
# B1 F-test in x11pt2, which the indirect pass never runs, so the oracle repeats
# the direct value there. The gate asserts that premise explicitly.
#
# Tolerance is the .udg's printed precision, same policy as
# test_x11_diagnostics.py: E15.8 for the a/c blocks, F8.2 for the ratio lines, the
# 2P-scaled space for b, f6.3 for the M statistics and F5.2 for Q. Zero new
# goldens were blessed -- total.udg already shipped the whole block.
_IF_RTOL_E15 = 1e-7
_IF_ATOL_E15 = 1e-8
_IF_ATOL_F82 = 5e-3
_IF_ATOL_M = 5e-4
_IF_ATOL_Q = 5e-3


def _udg_raw(path: str) -> dict[str, str]:
    """{key: the raw text after the colon} -- fixed-width fields need the columns."""
    out: dict[str, str] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            if ":" in ln:
                k, _, v = ln.partition(":")
                out[k.strip()] = v.rstrip("\n")
    return out


def _emitted_vec(run_output: str, key: str) -> "list[str] | None":
    for ln in run_output.splitlines():
        p = ln.split()
        if p and p[0] == key:
            return p[1:]
    return None


def _fixed_fields(text: str, width: int, n: int) -> "list[float | None]":
    out: list[float | None] = []
    for i in range(n):
        chunk = text[i * width:(i + 1) * width].strip()
        try:
            out.append(float(chunk))
        except ValueError:
            out.append(None)
    return out


@pytest.mark.skipif(not _HAVE_STATS, reason="composite golden .udg not present")
def test_composite_indirect_f2(run_output: str) -> None:
    """The indirect Part-F summary measures, if2.*."""
    raw = _udg_raw(os.path.join(_GOLDEN, _TOTAL, _TOTAL + ".udg"))
    if "if2.a01" not in raw:
        pytest.skip("total.udg carries no indirect F2 block")
    assert _emitted_vec(run_output, "if2.a01") is not None, (
        "the engine emitted no indirect F2 block")

    # The seasonality-test canaries: statistic F11.3, probability F8.2.
    for key in ("if2.fsb1", "if2.fsd8", "if2.kw", "if2.msf"):
        want = raw[key].split()
        got = _emitted_vec(run_output, key)
        assert got is not None, f"{key} not emitted"
        for i, (g, w) in enumerate(zip(got, want)):
            assert abs(float(g) - float(w)) <= 5e-3, f"{key}[{i}] {g} != {w}"
    assert _emitted_vec(run_output, "if2.idseasonal") == raw["if2.idseasonal"].split()
    # if2.fsb1 has no indirect counterpart -- x11pt2's B1 test runs once, on the
    # DIRECT pass, and the oracle reprints that value here.
    assert raw["if2.fsb1"].split() == raw["f2.fsb1"].split(), (
        "premise broken: the oracle's if2.fsb1 is no longer the direct Fpres")

    for block in ("a", "c"):
        for i in range(1, 13):
            key = f"if2.{block}{i:02d}"
            if key not in raw:
                continue
            got = _emitted_vec(run_output, key)
            want = raw[key].split()
            assert got is not None and len(got) == len(want), f"{key} width"
            for j, (g, w) in enumerate(zip(got, want)):
                gv, wv = float(g), float(w)
                assert abs(gv - wv) <= max(_IF_ATOL_E15, _IF_RTOL_E15 * abs(wv)), (
                    f"{key}[{j}] {gv} != {wv}")

    # The b block: `1x` then 5 x 2P-F8.2, a hardcoded '  100.00', then 2P-F8.2.
    # Compare in the SCALED (x100) space, and skip the constant total column.
    for i in range(1, 13):
        key = f"if2.b{i:02d}"
        if key not in raw:
            continue
        got = _emitted_vec(run_output, key)
        ref = _fixed_fields(raw[key][1:], 8, 7)
        cols = ref[:5] + [ref[6]]
        for j, (g, w) in enumerate(zip(got, cols)):
            if w is None:
                continue
            assert abs(float(g) * 100.0 - w) <= _IF_ATOL_F82, f"{key}[{j}]"

    for key, width in (("if2.d", 8), ("if2.e", 8), ("if2.f", 8), ("if2.g", 8),
                       ("if2.ic", 12), ("if2.is", 12)):
        if key not in raw:
            continue
        got = _emitted_vec(run_output, key)
        ref = _fixed_fields(raw[key], width, len(got))
        for j, (g, w) in enumerate(zip(got, ref)):
            if w is None:
                continue
            assert abs(float(g) - w) <= _IF_ATOL_F82, f"{key}[{j}] {g} != {w}"

    if "if2.mcd" in raw:
        got = _emitted_vec(run_output, "if2.mcd")
        assert int(got[0]) == int(raw["if2.mcd"].split()[0]), "indirect MCD"


@pytest.mark.skipif(not _HAVE_STATS, reason="composite golden .udg not present")
def test_composite_indirect_f3(run_output: str) -> None:
    """The indirect F3 quality statistics, if3.m01-m11 / q / qm2 / fail."""
    raw = _udg_raw(os.path.join(_GOLDEN, _TOTAL, _TOTAL + ".udg"))
    if "if3.q" not in raw:
        pytest.skip("total.udg carries no indirect F3 block")
    assert _emitted_vec(run_output, "if3.q") is not None, (
        "the engine emitted no indirect F3 block")

    for i in range(1, 12):
        key = f"if3.m{i:02d}"
        got = _emitted_vec(run_output, key)
        if key not in raw:
            assert got is None, (
                f"the engine emitted {key} but the oracle suppressed it")
            continue
        assert got is not None, f"{key} not emitted"
        assert abs(float(got[0]) - float(raw[key])) <= _IF_ATOL_M, (
            f"{key} {got[0]} != {raw[key].strip()}")
    for key in ("if3.q", "if3.qm2"):
        got = _emitted_vec(run_output, key)
        assert abs(float(got[0]) - float(raw[key])) <= _IF_ATOL_Q, (
            f"{key} {got[0]} != {raw[key].strip()}")
    assert int(_emitted_vec(run_output, "if3.fail")[0]) == int(raw["if3.fail"])


# --- increment 4b: the INDIRECT D8/D9 + Part-E save tables --------------------
#
# agr3.f:288-350 produces id8 (the unmodified SI, Series/stc2in) and id9 (the
# final replacement values -- the modified SI, but only where the extreme-value
# factor moved the unmodified SI by at least 1e-4 relative; blank elsewhere).
# x11pt4's Part E then produces the rest over agr3's buffers: ie1/ie2/ie3 are
# Stome/Stcime/Stime as they stand, ie5-ie8 the period-to-period changes with
# their ip* percent twins, iee the robust SA series (E11), i18 the final
# adjustment ratios O/D11 (E18) and ita the total adjustment factors (EB).
#
# Two Iagr==4 branches inside x11pt4 had to land for these: O5 (the aggregate
# with the indirect calendar factor divided out) is the calendar-adjusted
# original behind ie8, and O rather than Series is the numerator of i18/ita.
_IND_E_TAGS = ["id8", "id9", "ie1", "ie2", "ie3", "ie5", "ip5", "ie6", "ip6",
               "ie7", "ip7", "ie8", "ip8", "iee", "i18", "ita"]
# The change tables are differences of neighbouring values, so they carry no
# relative precision where the change is ~0; same absolute-floor policy as
# test_x11_etables.py.
_IND_CHANGE = {"ie5", "ip5", "ie6", "ip6", "ie7", "ip7", "ie8", "ip8"}


@pytest.mark.skipif(not _HAVE, reason="composite metafile corpus/golden not present")
@pytest.mark.parametrize("tag", _IND_E_TAGS)
def test_composite_indirect_etable(run_output: str, tag: str) -> None:
    """The indirect D8/D9 and Part-E save tables."""
    goldpath = os.path.join(_GOLDEN, _TOTAL, _TOTAL + "." + tag)
    if not os.path.exists(goldpath):
        pytest.skip(f"total ships no {tag} golden")
    gold = _read_golden(goldpath)
    assert gold, f"total.{tag}: empty golden"

    produced = {}
    for ln in run_output.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == tag:
            produced[p[1]] = float(p[2])
    assert produced, f"total.{tag}: harness emitted no rows"

    keys = sorted(set(gold) & set(produced))
    assert len(keys) == len(gold), (
        f"total.{tag}: produced {len(produced)} rows, golden {len(gold)}, "
        f"overlap {len(keys)}")

    afloor = RTOL * max(abs(v) for v in gold.values()) if tag in _IND_CHANGE else 0.0
    worst, worst_k = 0.0, None
    for k in keys:
        g, v = gold[k], produced[k]
        if abs(v - g) <= afloor:
            continue
        rel = abs(v - g) / abs(g) if g else abs(v - g)
        if rel > worst:
            worst, worst_k = rel, k
    assert worst <= RTOL, (
        f"total.{tag}: max rel err {worst:.3e} at {worst_k} (tol {RTOL:.0e})")
