"""Pseudo-additive composite gate -- agr3.f:267-272 and :389-400.

The last unported piece of ``composite{}``. ``agr3.f:266-276`` is an
``IF(Psuadd)``/``ELSE`` pair; the ELSE arm was ported with the rest of agr3 and
the Psuadd one was not, so a pseudo-additive composite total returned
``OUTCOME: OK`` with a wrong ``isf`` and no ``isd`` at all.

**The measurement is the point.** With ``mode=pseudoadd``, every other indirect
table was ALREADY bit-exact -- d10-d13, isa, itn, iir, id8, ie1, i18, all at
~5e-15 -- and ``isf`` alone sat **1.6e-06** out. The multiplicative and
pseudo-additive seasonal factors are numerically close on a well-behaved series,
which is precisely how an unported arm of a two-arm branch survives a corpus
that never sets the option. ``isd`` (the seasonal DIFFERENCES, table D10B) is
produced on this arm only, and was absent entirely.

Three details of the arm that the multiplicative line does not predict, all read
off the Fortran:

* the numerator is ``O2`` (the calendar-adjusted aggregate), not ``O5``,
* the denominator is ``Stc`` -- the raw filter output -- and NOT the ``stc2in``
  that the ``Sti`` on the same line was formed against,
* it subtracts that ``Sti`` back out and adds one.

The corpus case drops ``transform{function=log}`` from all three specs, because
a log and pseudo-additive are incompatible. That is why the on-vs-off
measurement above was taken against this same corpus with ``mode=mult`` rather
than against ``composite-fixed``: otherwise it would have been measuring the
transform.

The residual-seasonality F-test rows for this case are gated by
``test_composite_force.py``, which discovers every composite corpus carrying an
``id11.f`` golden rather than listing them.

Run:  python -m pytest tests/parity/test_composite_psuadd.py -q
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

_CASE = "composite-psuadd"
_MTA = "composite.mta"
_TOTAL = "total"

# Arithmetic floor, as everywhere else in the composite family. Measured worst
# 5.1e-15 on isf, 4.7e-15 on isd.
RTOL = 1e-12

# Every indirect table the spec asks for. isf and isd are the two the Psuadd arm
# owns; the rest are here so that a change to the arm which moved something ELSE
# cannot pass -- the on-vs-off measurement says they were already right, and
# that claim is worth pinning.
_IND_TAGS = ["isf", "isd", "isa", "itn", "iir", "id8", "id9",
             "ie1", "ie2", "ie3", "iee", "i18"]
_DIRECT_TAGS = ["d10", "d11", "d12", "d13"]
_COMPONENTS = ["region_north", "region_south"]

# Composite corpora that are NOT pseudo-additive. agr3.f:389 punches `isd` only
# under Psuadd, so the engine must emit none for these -- an absence gated on
# the MODE, not on the golden.
_NON_PSUADD = ["composite-fixed", "composite-force", "composite-force-indno"]

_GOLD_RE = re.compile(r"(\d{6})\s+([+\-][0-9.EeDd+\-]+)")


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


def _golden(case: str, spec: str = _TOTAL) -> str:
    return os.path.join(_GOLDEN_ROOT, case, spec)


_HAVE = (os.path.exists(os.path.join(_corpus(_CASE), _MTA)) and
         os.path.exists(os.path.join(_golden(_CASE), _TOTAL + ".isd")))

pytestmark = pytest.mark.skipif(
    not _HAVE, reason="composite-psuadd corpus/golden not present")


def _read_golden(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _GOLD_RE.match(ln.strip())
            if m:
                out[m.group(1)] = float(
                    m.group(2).replace("D", "E").replace("d", "e"))
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


def _check(case: str, spec: str, tag: str, prefix: str) -> None:
    produced = _emitted(_run(case), prefix + tag)
    gold = _read_golden(os.path.join(_golden(case, spec), spec + "." + tag))
    assert gold, f"{case}/{spec}.{tag}: empty golden"
    keys = sorted(set(gold) & set(produced))
    assert len(keys) == len(gold), (
        f"{case}/{spec}.{tag}: produced {len(produced)} rows, golden "
        f"{len(gold)}, overlap {len(keys)}")
    worst, worst_k = 0.0, None
    for k in keys:
        g, v = gold[k], produced[k]
        rel = abs(v - g) / abs(g) if g else abs(v - g)
        if rel > worst:
            worst, worst_k = rel, k
    assert worst <= RTOL, (
        f"{case}/{spec}.{tag}: max rel err {worst:.3e} at {worst_k} "
        f"(tol {RTOL:.0e})")


def test_psuadd_tags_discovered() -> None:
    """Every requested indirect table has a golden -- a shrinking list reads green."""
    missing = [t for t in _IND_TAGS
               if not os.path.exists(os.path.join(_golden(_CASE),
                                                  _TOTAL + "." + t))]
    assert not missing, f"composite-psuadd ships no golden for {missing}"


@pytest.mark.parametrize("tag", _IND_TAGS)
def test_psuadd_indirect(tag: str) -> None:
    _check(_CASE, _TOTAL, tag, prefix="")


@pytest.mark.parametrize("tag", _DIRECT_TAGS)
def test_psuadd_total_direct(tag: str) -> None:
    """The aggregate adjusted in its own right, in pseudo-additive mode."""
    _check(_CASE, _TOTAL, tag, prefix="")


@pytest.mark.parametrize("base", _COMPONENTS)
@pytest.mark.parametrize("tag", _DIRECT_TAGS)
def test_psuadd_component(base: str, tag: str) -> None:
    """What the indirect tables are summed from."""
    _check(_CASE, base, tag, prefix=base + ":")


def test_isd_is_not_isf() -> None:
    """`isd` is Stc*(Sts-1), not Sts -- a port that aliased them would pass every
    table gate above only if the two series were equal. They are not; pin it."""
    out = _run(_CASE)
    isf, isd = _emitted(out, "isf"), _emitted(out, "isd")
    keys = sorted(set(isf) & set(isd))
    assert keys, "no overlap between isf and isd"
    worst = max(abs(isd[k] - isf[k]) / max(abs(isf[k]), 1e-30) for k in keys)
    assert worst > 1e-3, f"isd is within {worst:.3e} of isf everywhere"


def _err_block(run_output: str, who: str) -> list[str]:
    """The `===ERR <who>===` .. `===END ERR===` block, stripped of blanks."""
    out, on = [], False
    for ln in run_output.splitlines():
        if ln == f"===ERR {who}===":
            on = True
            continue
        if on and ln == "===END ERR===":
            break
        if on and ln.strip():
            out.append(ln.rstrip())
    return out


def _golden_err_body(case: str, spec: str) -> list[str]:
    p = os.path.join(_GOLDEN_ROOT, case, spec, spec + ".err")
    out = []
    with open(p, encoding="utf-8", errors="replace") as f:
        for ln in f:
            if ln.strip():
                out.append(ln.rstrip())
    return out


def test_psuadd_forecast_warning() -> None:
    """editor.f:2528-2543, the fourth arm -- a WARNING, not a refusal.

    The composite total sets no forecasts, so it fires there and nowhere else in
    this metafile. It is also the reason `x13run_composite` now dumps its Mt2
    channel on a SUCCESSFUL run: it dumped only on FATAL, so every non-fatal
    NOTE and WARNING a composite emitted was discarded unread (entry 81's trap,
    still open in this harness). Compared against the oracle's own .err."""
    got = _err_block(_run(_CASE), "total")
    want = _golden_err_body(_CASE, _TOTAL)
    assert any("WARNING: Pseudo-additive" in ln for ln in want), (
        "golden total.err carries no pseudo-additive WARNING; this gate is "
        "asserting against the wrong file")
    assert got == want, f"total .err mismatch\n  golden: {want}\n  cpp:    {got}"


@pytest.mark.parametrize("base", _COMPONENTS)
def test_psuadd_component_err_is_clean(base: str) -> None:
    """The components DO set forecasts, so the warning must not fire for them --
    the arm is keyed on Nfcst, and a port that emitted it unconditionally would
    pass the total's gate above."""
    got = _err_block(_run(_CASE), base)
    assert not any("WARNING" in ln or "ERROR" in ln for ln in got), (
        f"{base}: unexpected diagnostic {got}")
    assert got == _golden_err_body(_CASE, base), (
        f"{base}: .err mismatch vs oracle")


@pytest.mark.parametrize("case", _NON_PSUADD)
def test_isd_absent_off_the_psuadd_arm(case: str) -> None:
    """agr3.f:389 punches D10B only under Psuadd. Gated on the MODE: these three
    corpora never set it, so the absence is a claim about the engine and not
    about which goldens happen to exist."""
    if not os.path.exists(os.path.join(_corpus(case), _MTA)):
        pytest.skip(f"{case} corpus not present")
    spc = open(os.path.join(_corpus(case), "total.spc"),
               encoding="utf-8", errors="replace").read()
    assert "pseudoadd" not in spc, f"{case} IS pseudo-additive; wrong list"
    assert not _emitted(_run(case), "isd"), (
        f"{case}: engine emitted `isd` on a non-pseudo-additive run")
    assert not os.path.exists(os.path.join(_golden(case), _TOTAL + ".isd")), (
        f"{case}: the oracle DID write isd; the assertion above is wrong")
