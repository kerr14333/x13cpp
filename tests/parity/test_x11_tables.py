"""M5 X-11 gate: the C++ X-11 decomposition spine (x13run_x11) vs the oracle
goldens.

This slice assembles the classic X-11 decomposition (x11pt1 -> x11pt2 -> x11pt3)
on the no-model direct-X11 path and produces:

  * b1  -- the prior-adjusted B1 input series,
  * d10 -- final seasonal factors,   d11 -- final seasonally adjusted series,
  * d12 -- final trend-cycle,        d13 -- final irregular.

For every corpus spec whose golden bundle ships all of them and that runs on the
no-model path (series{} + x11{} only), this test runs ``x13run_x11`` and checks
each produced table matches the golden numerically at rtol 1e-8, period-key
exact. (d12 supersedes the earlier D7 trend checkpoint -- x11pt3 recomputes the
trend into D12.) The model-bearing x11 specs gate once the estimate/forecast/
extend/adjreg glue lands.

Run:  python -m pytest tests/parity/test_x11_tables.py -q
"""
from __future__ import annotations

import os
import re
import subprocess
import sys

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus", "generated")
_GOLDEN = os.path.join(_REPO, "tests", "golden", "generated")
# The hand-authored corpus too -- see _discover. Feature gates (force,
# x11regression, prior-adj, ...) already cover parts of extra/, but each covers
# only the specs ITS feature owns, so a spec belonging to no feature front (the
# pickmdl-* family, slidingspans-td) had its whole decomposition ungated.
_TREES = ("generated", "extra")
# base -> (corpus dir, golden tree), filled by _discover.
_WHERE: dict[str, tuple[str, str]] = {}


def _spec_path(base: str) -> str:
    return os.path.join(_WHERE[base][0], base + ".spc")


def _gold_path(base: str, tag: str) -> str:
    return os.path.join(_WHERE[base][1], base, base + "." + tag)


# Tolerance policy (see the second-brain note / tools/x11_regeff_handoff.md):
# the goldens are printed to 15 significant digits, so ~1e-15 is the hard
# comparison floor. Pure-arithmetic decomposition tables (the no-model direct-X11
# path) reach that floor and are gated tight at 1e-12 (three orders of margin over
# libm-transcendental + text-truncation noise). Model-based specs carry
# estimation-derived quantities whose last digits track the optimizer's
# convergence path, so they get the realistic estimation floor of 1e-6.
RTOL_ARITHMETIC = 1e-12   # no-model / direct-X11 decomposition tables
RTOL_ESTIMATION = 1e-6    # model-based (automdl/aictest/arima) runs
RTOL = RTOL_ARITHMETIC    # back-compat alias; the per-spec choice is made below


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

_GOLD_RE = re.compile(r"(\d{6})\s+([+\-][0-9.EeDd+\-]+)")


def _read_golden(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _GOLD_RE.match(ln.strip())
            if m:
                out[m.group(1)] = float(m.group(2).replace("D", "E").replace("d", "e"))
    return out


def _is_no_model(spec_path: str) -> bool:
    """No-model direct-X11 spec: only series{} and x11{} present."""
    txt = open(spec_path, encoding="utf-8", errors="replace").read().lower()
    modeling = ("arima{", "automdl{", "estimate{", "regression{", "outlier{",
                "forecast{", "seats{", "pickmdl{", "check{")
    return "x11{" in txt and not any(k in txt for k in modeling)


# d16 (the COMBINED seasonal + calendar factor) is gated where its golden ships
# but is not required for discovery: it is the table that carries the calendar
# effect, so it is the one that catches a Faccal that is missing (or double-
# counting) the trading-day / holiday factor while d10-d13 still look right.
# sac / tac are the transform{constant=} pair: the D11 and published D12 with the
# constant still in them (the oracle's Stcipc / stc2pc), gated where they ship.
#
# b1 is NOT part of the discovery requirement. It used to be, and that single
# tag was silently dropping 18 specs whose entire d10-d13 decomposition then
# reached no gate anywhere -- the whole pickmdl-* family, airline_slidingspans-td,
# noapply-{ao,ls,td,holiday}, reg-tcrate, outlier-tcrate, fcst-lognormal,
# reg-eastermeans, outofsample. None of them saves b1, so `all(_CORE_TAGS)`
# excluded them outright. Requiring only the four D tables and gating everything
# else where-shipped is what the d16/sac/tac tags already did.
_CORE_TAGS = ["d10", "d11", "d12", "d13"]
_TAGS = ["b1"] + _CORE_TAGS + ["d16", "sac", "tac"]


def _discover() -> list[str]:
    """Every spec shipping the four D-table goldens, in BOTH corpus trees."""
    specs: list[str] = []
    for tree in _TREES:
        cdir = os.path.join(_REPO, "tests", "corpus", tree)
        gtree = os.path.join(_REPO, "tests", "golden", tree)
        if not os.path.isdir(cdir):
            continue
        for fn in sorted(os.listdir(cdir)):
            if not fn.endswith(".spc"):
                continue
            base = fn[:-4]
            gdir = os.path.join(gtree, base)
            if not all(os.path.exists(os.path.join(gdir, base + "." + t))
                       for t in _CORE_TAGS):
                continue
            _WHERE[base] = (cdir, gtree)
            specs.append(base)
    return sorted(specs)


CASES = _discover()


def test_x11_cases_discovered() -> None:
    """A shrunken parametrisation passes silently -- assert the floor."""
    assert len(CASES) >= 140, f"only {len(CASES)} x11 table specs discovered"
    for tree in _TREES:
        assert any(_WHERE[b][0].endswith(tree) for b in CASES), \
            f"no x11 table spec discovered under tests/corpus/{tree}"


@pytest.mark.skipif(not CASES, reason="no x11 spec ships the d10-d13 goldens")
@pytest.mark.parametrize("base", CASES)
@pytest.mark.parametrize("tag", _TAGS)
def test_x11_table(base: str, tag: str) -> None:
    goldpath = _gold_path(base, tag)
    if not os.path.exists(goldpath):
        pytest.skip(f"{base} does not ship the {tag} golden")
    spec = _spec_path(base)
    # Tolerance by path: no-model decomposition is pure arithmetic (tight);
    # model-based runs carry estimation-derived values (loose).
    tol = RTOL_ARITHMETIC if _is_no_model(spec) else RTOL_ESTIMATION
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.returncode == 0, f"{base}: harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:200]

    produced: dict[str, float] = {}
    for ln in r.stdout.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == tag:
            produced[p[1]] = float(p[2])

    gold = _read_golden(goldpath)
    assert gold, f"{base}.{tag}: empty golden"

    # Set EQUALITY, not "every golden date was produced". The overlap-only form
    # accepted produced > gold, so an engine emitting extra rows under the same
    # tag -- a widened punch range, a forecast/backcast row that does not belong
    # -- passed while looking like a full match. Both directions are named in
    # the message because they mean different things: missing = the engine did
    # not emit a date the oracle did; extra = it emitted one the oracle did not.
    keys = sorted(set(gold) & set(produced))
    missing = sorted(set(gold) - set(produced))
    extra = sorted(set(produced) - set(gold))
    assert not missing and not extra, (
        f"{base}.{tag}: produced {len(produced)} rows, golden {len(gold)}; "
        f"missing {missing[:6]}{'...' if len(missing) > 6 else ''}, "
        f"extra {extra[:6]}{'...' if len(extra) > 6 else ''}")

    worst = 0.0
    worst_k = None
    for k in keys:
        g, v = gold[k], produced[k]
        rel = abs(v - g) / abs(g) if g else abs(v - g)
        if rel > worst:
            worst, worst_k = rel, k
    assert worst <= tol, f"{base}.{tag}: max rel err {worst:.3e} at {worst_k} (tol {tol:.0e})"
