"""SEATS Hodrick-Prescott option-bridge gate: the C++ resolution of
`seats{hpcycle/hplan/hptarget/hprmls}` vs the oracle's own resolved values.

WHAT THIS GATES (and, just as importantly, what it does not)
------------------------------------------------------------
The HP trend/cycle FILTER itself is NOT ported -- see
`tools/seats_hp_scouting.md`. What IS ported (core/src/seats/seatopts.cpp) is
the options->internal bridge at ansub9.f:1080-1090 + 1109-1117 and the
"auto" resolution at sigex.f:2370-2387, i.e. the code that decides *whether*
and *against which target* the filter runs. That decision is observable from
outside the oracle in two independent ways, and this file gates both:

  1. The SEATS "INPUT" echo in the `.sum` file (ansub10.f:3613/4043) prints
     `hpcycle=` / `hplan=` exactly when the resolved value differs from the
     SEATS namelist default (ansub9.f:1641 l_hpcycle=-1, :1646 l_hplan=-1.0).
     A spec with no echo therefore asserts "the port must also resolve to the
     default", which is why every ordinary seats corpus spec is swept in too.
  2. Whether the oracle wrote `.cyc` / `.ltt` at all. HPOUTPUT (ansub10.f:1157)
     runs only when the resolved hpcycle >= 1 AND L_OUT == 0, so the presence
     of those goldens is an end-to-end check of the ansub9 interlocks *and* the
     sigex.f:2371-2377 minimum-span rule -- from a completely different code
     path than the .sum echo.

NOT gated here: `HPOPT_out`. ansub9.f:1050 pushes L_OUT to 3 whenever any seats
PRINT table in [LSETRN, NTBL-11] is selected, but gt_seats token-consumes
`print=` without populating ctx.tbllog.prttab (the seats table dictionary is
unported print surface), so the port cannot see it. Consequence: for a spec
carrying `seats{print=all}` the oracle has out==3 and the port reports 0. The
6 `*_hp-*-seats` specs deliberately omit `print=` so that both are 0 and check
(2) above is meaningful.

Run:  python -m pytest tests/parity/test_seats_hpopts.py -q
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

# ansub9.f:1641 / :1646 -- the SEATS namelist defaults the .sum echo suppresses.
_DEFAULT_HPCYCLE = -1
_DEFAULT_HPLAN = -1.0


def _find_binary() -> str:
    for c in (
        os.path.join(_REPO, "build", "x13run_seats.exe"),
        os.path.join(_REPO, "build", "x13run_seats"),
        os.path.join(_REPO, "build", "Release", "x13run_seats.exe"),
    ):
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_SEATS")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13run_seats binary not found; build it first (cmake --build build "
        "--target x13run_seats).")


BIN = _find_binary()


def _discover() -> list[str]:
    if not os.path.isdir(_CORPUS):
        return []
    out = []
    for fn in sorted(os.listdir(_CORPUS)):
        if not fn.endswith(".spc") or not fn[:-4].endswith("seats"):
            continue
        base = fn[:-4]
        if os.path.exists(os.path.join(_GOLDEN, base, base + ".sum")):
            out.append(base)
    return out


CASES = _discover()


def _oracle_input_echo(base: str) -> tuple[int, float]:
    """Resolved (hpcycle, hplan) as the oracle's .sum INPUT block reports them.

    Absent keys mean "equal to the SEATS default" -- the echo only prints a
    value that differs (ansub10.f:3613 `if (hpcycle .ne. l_hpcycle)`).
    """
    path = os.path.join(_GOLDEN, base, base + ".sum")
    with open(path, encoding="utf-8", errors="replace") as f:
        txt = f.read()
    i = txt.find("INPUT")
    assert i >= 0, f"{base}.sum: no SEATS INPUT block"
    # The block ends at the "Decomposition :" line that follows it.
    j = txt.find("Decomposition :", i)
    block = txt[i:j if j > i else i + 2000]
    m = re.search(r"\bhpcycle=\s*(-?\d+)", block)
    hpcycle = int(m.group(1)) if m else _DEFAULT_HPCYCLE
    m = re.search(r"\bhplan=\s*(-?[0-9.]+(?:[eEdD][-+]?\d+)?)", block)
    hplan = float(m.group(1).replace("d", "e").replace("D", "E")) if m \
        else _DEFAULT_HPLAN
    return hpcycle, hplan


def _run(base: str) -> dict[str, str]:
    spec = os.path.join(_CORPUS, base + ".spc")
    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.returncode == 0, f"{base}: harness exit {r.returncode}\n{r.stderr}"
    lines = r.stdout.splitlines()
    assert lines and lines[0].strip() == "OUTCOME: OK", r.stdout[:200]
    out: dict[str, str] = {}
    for ln in lines:
        if ln.startswith("HPOPT_"):
            k, _, v = ln.partition(":")
            out[k.strip()] = v.strip()
    return out


@pytest.mark.skipif(not CASES, reason="no seats{} spec ships a .sum golden")
@pytest.mark.parametrize("base", CASES)
def test_hp_option_bridge(base: str) -> None:
    """The port's PRE-auto-resolution hpcycle/hplan == the oracle's echo."""
    got = _run(base)
    assert got, f"{base}: harness printed no HPOPT_ canaries"
    want_hpcycle, want_hplan = _oracle_input_echo(base)
    assert int(got["HPOPT_hpcycle_raw"]) == want_hpcycle, (
        f"{base}: hpcycle resolved to {got['HPOPT_hpcycle_raw']}, oracle .sum "
        f"echo says {want_hpcycle} (ansub9.f:1080-1090 / 1109-1117)")
    assert float(got["HPOPT_hplan"]) == pytest.approx(want_hplan, rel=1e-6), (
        f"{base}: hplan resolved to {got['HPOPT_hplan']}, oracle .sum echo "
        f"says {want_hplan}")


# The hand-authored HP specs are the ones with no `seats{print=}`, so the
# oracle's L_OUT is 0 and HPOUTPUT's cyc/ltt tables actually get written when
# the filter runs. That makes their presence/absence a real end-to-end check
# of the resolved hpcycle -- via a code path (sigex.f:2370-2388 -> ansub10.f
# HPOUTPUT) entirely separate from the .sum echo above.
_OUT0_CASES = [b for b in CASES if "_hp-" in b]


@pytest.mark.skipif(not _OUT0_CASES, reason="no out==0 HP spec in the corpus")
@pytest.mark.parametrize("base", _OUT0_CASES)
def test_hp_cycle_tables_presence(base: str) -> None:
    """Resolved hpcycle >= 1  <=>  the oracle wrote .cyc/.ltt."""
    got = _run(base)
    assert int(got["HPOPT_out"]) == 0, (
        f"{base}: expected L_OUT==0 (spec carries no seats print=)")
    resolved = int(got["HPOPT_hpcycle"])
    gdir = os.path.join(_GOLDEN, base)
    have_cyc = os.path.exists(os.path.join(gdir, base + ".cyc"))
    have_ltt = os.path.exists(os.path.join(gdir, base + ".ltt"))
    assert have_cyc == have_ltt, f"{base}: .cyc/.ltt goldens disagree"
    assert have_cyc == (resolved >= 1), (
        f"{base}: port resolved hpcycle={resolved} but the oracle "
        f"{'wrote' if have_cyc else 'wrote no'} cyc/ltt")
    # The HP filter itself is unported, so when the oracle DID write those
    # tables the port has no counterpart. Assert that gap loudly rather than
    # letting a future half-port slip through as a silent no-op.
    if have_cyc:
        spec = os.path.join(_CORPUS, base + ".spc")
        r = subprocess.run([BIN, spec], capture_output=True, text=True)
        assert not re.search(r"^(cyc|ltt) ", r.stdout, re.M), (
            f"{base}: the harness now emits cyc/ltt -- the HP filter has been "
            "ported; gate it against the goldens in test_seats_tables.py and "
            "delete this assertion (tools/seats_hp_scouting.md)")
