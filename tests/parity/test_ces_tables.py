"""CES gate: the BLS Current Employment Statistics production specs vs the oracle.

These are real, unedited BLS specs (https://www.bls.gov/web/empsit/ces.spec.ae.zip)
-- only the ``FILE=`` paths were rewritten to be repo-relative. They exercise the
port along axes the generated corpus never touched:

  * **ALL-UPPERCASE keywords and argument values** (every BLS spec is uppercase).
    This is what surfaced the case-sensitive value-match bug fixed in 8e27426 --
    ``TRANSFORM{ FUNCTION = LOG }`` silently dropped the log transform.
  * ``series{ name= }`` as the save-table column label (the oracle's Serlbl).
  * ``x11{ appendfcst=yes }`` -- Savfct set from the x11 spec (not series{}),
    which widens the d10/d16 punch range to the forecast span (getx11.f:348-352,
    x11pt3.f:1132-1140).
  * ``x11{ final=user }`` -- Finusr (getx11.f:290-311). A no-op here because the
    user regressors carry ``usertype=td`` (their effect lands in Factd, not
    Facusr), but the argument must parse and route.
  * 11 user-defined trading-day regressors (``usertype=td``) + AO outlier
    identification at ``critical=10.5`` + a fixed ``(0 1 0)(0 1 1)`` model.

Each spec is run through ``x13run_x11`` and every table the golden bundle ships
(b1/d10/d11/d16) is compared period-key exact. The specs are model-based, so the
tolerance is the universal 1e-8 contract rather than the pure-arithmetic floor --
in practice they land at ~5e-15.

Run:  python -m pytest tests/parity/test_ces_tables.py -q
"""
from __future__ import annotations

import os
import re
import subprocess

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus", "ces")
_GOLDEN = os.path.join(_REPO, "tests", "golden", "ces")

# Universal bit-parity contract (CLAUDE.md). Measured worst case over these
# specs is ~5e-15, i.e. the 15-significant-digit print floor of the goldens.
RTOL = 1e-8

_TAGS = ["b1", "d10", "d11", "d12", "d13", "d16"]
_GOLD_RE = re.compile(r"(\d{6})\s+([+\-][0-9.EeDd+\-]+)")


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


def _read_golden(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            m = _GOLD_RE.match(ln.strip())
            if m:
                out[m.group(1)] = float(m.group(2).replace("D", "E").replace("d", "e"))
    return out


def _discover() -> list[tuple[str, str]]:
    """(spec base, table tag) for every golden table the CES corpus ships.

    The golden bundle is named after the spec, but its table files carry the
    spec base name -- except the goldens blessed for a differently-named run
    (AE1011330000_simple/ ships AE1011330000_simple.*), so resolve per file.
    """
    cases: list[tuple[str, str]] = []
    if not os.path.isdir(_CORPUS):
        return cases
    for fn in sorted(os.listdir(_CORPUS)):
        if not fn.endswith(".spc"):
            continue
        base = fn[:-4]
        gdir = os.path.join(_GOLDEN, base)
        for tag in _TAGS:
            if os.path.exists(os.path.join(gdir, base + "." + tag)):
                cases.append((base, tag))
    return cases


CASES = _discover()


@pytest.mark.skipif(not CASES, reason="no CES spec ships x11 table goldens")
@pytest.mark.parametrize("base,tag", CASES, ids=[f"{b}-{t}" for b, t in CASES])
def test_ces_table(base: str, tag: str) -> None:
    # Run from the corpus dir: the BLS specs reference their data relatively.
    r = subprocess.run([BIN, base + ".spc"], cwd=_CORPUS, capture_output=True,
                       text=True)
    assert r.returncode == 0, f"{base}: harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:200]

    produced: dict[str, float] = {}
    for ln in r.stdout.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == tag:
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
