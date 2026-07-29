"""The BACKCAST average absolute percentage error -- amdfct.f's `Bckcst` arm.

`pickmdl{}` re-scores its selected model over the BACKCAST span before the run
continues (automx.f:903-928), and amdfct computes that with the design time
REVERSED so the same forward machinery extrapolates backwards. Until this gate
existed the whole combination FATALED: `pickmdl{}` with `forecast{maxback=}` was
walled, on a spec the oracle runs to completion.

**This is the one amdfct output with no savelog key.** `prtamd` PRINTS the four
numbers and writes nothing to the `.udg`; the only trace the block leaves there
is whether `Nbcst` survived. So the gate reads the oracle's own printed table out
of the golden `.out` and compares the harness's `bcstaape.*` lines against it, at
the two decimals prtamd formats them to. Same shape as the composite gate's
roughness-table check, and for the same reason.

Both arms are covered, because they are different code paths that only meet
here:

* ``extra/airline_pickmdl-backcast``      -- within-sample backcasts,
* ``extra/airline_pickmdl-backcast-zero`` -- the same, on a series crossing
  zero, i.e. through the `ivalue==1` absolute-error scale,
* ``extra/airline_pickmdl-backcast-oos`` -- out-of-sample backcasts, where each
  pass walks the model span START forward instead of the end back and the
  ACTUALs are the dropped year taken in REVERSE order (amdfct.f:212-219 and
  :239, which skips `subset` on exactly this path).

The rest of both specs -- the selected model, d10-d13, the whole `.udg` -- is
gated by the ordinary auto-discovering suites; only the printed block is here.

Run:  python -m pytest tests/parity/test_backcast_aape.py -q
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

CASES = [
    ("airline_pickmdl-backcast", "within-sample"),
    ("airline_pickmdl-backcast-oos", "out-of-sample"),
    # ... and the same within-sample arm on a series that crosses zero, which is
    # the ONLY corpus spec reaching amdfct's `ivalue==1` scale branch (and hence
    # the only gate on the `ave` seed defect). It is also what discriminates
    # WHICH window the scale is taken from: airline_zero is negative in its first
    # three years and positive in its last, so a backcast pass that reads the
    # forecast window passes every other spec in the suite and fails here.
    ("airline_pickmdl-backcast-zero", "within-sample"),
]

# prtamd writes the three yearly errors as f8.2 and the average as f9.2, so the
# golden pins each to +/-5e-3. Compared as the oracle formats them -- the same
# rounded string -- which is the tightest statement the printed output supports.
_HDR = re.compile(
    r"Average absolute percentage error in (within-sample|out-of-sample) backcasts:")
_YEARS = re.compile(r"Last year:\s*([\d.]+)\s+Last-1 year:\s*([\d.]+)\s+"
                    r"Last-2 year:\s*([\d.]+)")
_AVG = re.compile(r"Last three years:\s*([\d.]+)")


def _find_binary() -> str:
    for c in (os.path.join(_REPO, "build", "x13run_m3.exe"),
              os.path.join(_REPO, "build", "x13run_m3"),
              os.path.join(_REPO, "build", "Release", "x13run_m3.exe")):
        if os.path.exists(c):
            return c
    raise FileNotFoundError("x13run_m3 not found; build it first.")


BIN = _find_binary()


def _oracle_block(base: str) -> tuple[str, list[float]]:
    """(mode, [avg, yr1, yr2, yr3]) from the golden .out's printed table."""
    path = os.path.join(_GOLDEN, base, base + ".out")
    lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
    for i, ln in enumerate(lines):
        m = _HDR.search(ln)
        if not m:
            continue
        y = _YEARS.search(lines[i + 1])
        a = _AVG.search(lines[i + 2])
        assert y and a, f"{base}: malformed backcast block at line {i}"
        return m.group(1), [float(a.group(1)), float(y.group(1)),
                            float(y.group(2)), float(y.group(3))]
    raise AssertionError(f"{base}.out carries no backcast error block")


def _engine(base: str) -> dict[str, str]:
    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], cwd=_CORPUS, capture_output=True, text=True,
                       timeout=180)
    assert r.returncode == 0, f"{base}: harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:400]
    out = {}
    for ln in r.stdout.splitlines():
        k, sep, v = ln.partition(":")
        if sep:
            out[k.strip()] = v.strip()
    return out


@pytest.mark.parametrize("base,mode", CASES)
def test_backcast_aape(base: str, mode: str) -> None:
    if not os.path.exists(os.path.join(_GOLDEN, base, base + ".out")):
        pytest.skip(f"{base} golden not present")
    want_mode, want = _oracle_block(base)
    assert want_mode == mode, (
        f"{base}: the oracle now reports a {want_mode} backcast, not {mode} -- "
        f"the spec is testing the other arm")
    eng = _engine(base)
    assert "bcstaape.0" in eng, (
        f"{base}: the engine emitted no backcast block; automx's acceptance pass "
        f"(automx.f:906) did not run")
    assert eng["bcstaape.mode"] == (
        "outofsample" if mode == "out-of-sample" else "withinsample")
    for i, w in enumerate(want):
        got = float(eng[f"bcstaape.{i}"])
        assert f"{got:.2f}" == f"{w:.2f}", (
            f"{base}: bcstaape.{i} = {got}, oracle prints {w:.2f}")


@pytest.mark.parametrize("base,mode", CASES)
def test_backcasts_survive(base: str, mode: str) -> None:
    """CB-33: `bcstlim=` cannot reject, so the backcasts are always kept.

    automx.f:922's `IF(mape(4).gt.Bcklim.and.(.not.argok))` is almost certainly a
    slip for `.or.` -- a model that converged can never fail the screen no matter
    how bad the backward extrapolation is. prtamd, which evaluates the screens
    itself, disagrees out loud: with `bcstlim=1` the oracle prints "MODEL n
    REJECTED: Average backcast error > 1.00%" and then "The model chosen is ..."
    and keeps all twelve backcasts. Pinned here from the golden's own footer.
    """
    path = os.path.join(_GOLDEN, base, base + ".out")
    if not os.path.exists(path):
        pytest.skip(f"{base} golden not present")
    text = open(path, encoding="utf-8", errors="replace").read()
    assert re.search(r"Includes\s+12 backcasts", text), (
        f"{base}: the oracle no longer keeps the backcasts -- re-read "
        f"automx.f:922 before changing the port's `&&`")
