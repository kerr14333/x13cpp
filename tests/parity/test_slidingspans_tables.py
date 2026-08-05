"""slidingspans{} gate: the X-11 sliding-spans stability diagnostics
(ssap.f/sspdrv.f/ssrit.f) vs the oracle goldens.

`slidingspans{}` re-runs the regARIMA+X11 adjustment over `numspans`
overlapping sub-spans of the series (each `length` long, sliding by one
period), then reports the per-period value of the seasonal factor / SA
series / trading-day factor / period-to-period change from each span side
by side, plus a cross-span max-%-difference column. The per-span values
(not a single summary statistic) are what land in the save tables:

  * sfs -- seasonal-factor spans
  * chs -- month-to-month (or quarter-to-quarter) SA change spans
  * ads -- SA spans (only emitted when the run has TD adj, holiday adj,
           round=yes, or force{} yearly totals -- see ssap.f:206-211)
  * tds -- trading-day-factor spans (only emitted when Itd==1, i.e. a TD
           regressor is active)
  * ycs -- year-to-year change spans (only requested/printed when Lyy)

The indirect/composite variants (sis/cis/ais/yis) are not reachable from a
single-series spec and are out of scope here.

STATUS: GATED bit-exact (all 4 spans, every table each spec ships --
sfs/chs/tds/ads -- worst ~5e-15). The re-entrant
sub-span replay driver (driver/run_x11_span.{hpp,cpp}) + the slidingspans{}
orchestration (x11/slidingspans.{hpp,cpp} -- setssp.f/sspdrv.f/ssrit.f/ssap.f's
xchng+mflag+rplus, scoped to sfs/chs) run to completion (Issap 1->2->3). Port
bugs found and fixed en route (all our own, not Census), each latent because no
previously-gated spec ever exercised a replayed sub-span (Pos1bk>1):
  1. x11pt2/x11pt3's Stex (COMMON /mq10/) and x11pt3's Stsie (COMMON /work3/)
     were function-local C arrays, losing history between calls; a replayed
     sub-span reads that history (x11pt3.f Ksdev>1: `copy(Stsie,Posfob,1,Stsi)`).
     Promoted to ctx-persistent buffers (ctx.mq10_stex, ctx.work3_stsie).
  2. ssmdl.f Ssinit==1 "fix the whole model" writes ssprep.cmn's snapshot
     Fxa/Ap2 (which restor_span resets the live Arimaf/Arimap FROM); fixing only
     the live copy was undone by the next restor_span, so every span re-estimated.
  3. THE span-1 ~3.45% gap: `run_x11_span` reset `kersa=0` before each sub-span
     but NOT `xtrm.ksdev`, so a span inherited the MAIN run's entsch-evolved
     Ksdev (2) instead of the parsed spec default (1). That flips entsch's
     `k=Kersa1+Ksdev1+1` decode (2 -> k=3 -> Ksdev=Iv=2 vs default 1 -> k=2 ->
     Ksdev=0), selecting a different extreme-value sigma mode -> different xtrm
     weights from IDENTICAL irregulars -> different modified-SI -> different
     seasonal MA -> a different Henderson trend-filter length (9 vs oracle 13)
     via the I/C ratio (0.9065 vs >=1.0) -> the whole cascade. Ground-truthed
     against the oracle (temporary vtc.f/si.f probes, reverted): with identical
     B1/forecast/Lter, the FIRST diverging quantity was xtrm's Stwt, driven by
     Ksdev. Fix: capture the parsed Ksdev in ssprep_snapshot (runs before the
     main run mutates it) as ctx.saved.ksdev0, restore it per span in
     run_x11_span -- each sub-span now starts its own fresh adjustment.

The produced-side reader is real (not a stub); this test is a genuine gate.

Run:  python -m pytest tests/parity/test_slidingspans_tables.py -q
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

# Sliding-spans tables are per-period differences over re-run adjustments; if
# the adjustment path is bit-exact the differences should be too, but each
# span carries a re-estimated regARIMA model, so gate at the estimation floor
# (see the tolerance policy in tests/parity/test_x11_tables.py).
RTOL = 1e-6

# Tags actually shipped by tests/golden/extra/airline_slidingspans/ (verified
# by listing the golden dir -- ads/tds are legitimately absent for that spec
# per the ssap.f gating above, not a missing-golden oversight). Do not add
# sis/cis/ais/yis (composite-only) or ycs (not requested by this spec, and
# would need >=5 years of spans to populate) without a golden to back them.
_TAGS = ["sfs", "chs", "ads", "tds"]
_CORE_TAGS = ["sfs", "chs"]

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

# Wide-table row: date, then one value per span column, then a trailing
# Max_%_DIFF column. Number-of-spans is read off the header (Span1..SpanN).
_SENTINEL = -999.0
_NUM_RE = re.compile(r"[+\-][0-9.EeDd+\-]+")


def _read_golden(path: str) -> tuple[dict[tuple[str, int], float], dict[str, float]]:
    """Returns ({(date, span_idx): value}, {date: max_pct_diff}), dropping
    the -999 "span doesn't cover this date" sentinel cells."""
    cells: dict[tuple[str, int], float] = {}
    maxdiff: dict[str, float] = {}
    with open(path, encoding="utf-8", errors="replace") as f:
        for ln in f:
            parts = ln.split()
            if len(parts) < 2 or not re.match(r"^\d{6}$", parts[0]):
                continue
            date = parts[0]
            nums = [float(v.replace("D", "E").replace("d", "e"))
                    for v in parts[1:] if _NUM_RE.fullmatch(v)]
            if not nums:
                continue
            *spans, mdiff = nums
            for i, v in enumerate(spans, start=1):
                if v != _SENTINEL:
                    cells[(date, i)] = v
            if mdiff != _SENTINEL:
                maxdiff[date] = mdiff
    return cells, maxdiff


def _read_produced(text: str, tag: str
                    ) -> tuple[dict[tuple[str, int], float], dict[str, float]]:
    """Parses tools/x13run_x11.cpp's `dump_span_table` convention: lines
    `<tag> <date> <span1> .. <spanN> <maxdiff>`, -999 sentinel, same shape as
    `_read_golden`."""
    cells: dict[tuple[str, int], float] = {}
    maxdiff: dict[str, float] = {}
    for ln in text.splitlines():
        parts = ln.split()
        if len(parts) < 3 or parts[0] != tag:
            continue
        date = parts[1]
        nums = [float(v) for v in parts[2:]]
        *spans, mdiff = nums
        for i, v in enumerate(spans, start=1):
            if v != _SENTINEL:
                cells[(date, i)] = v
        if mdiff != _SENTINEL:
            maxdiff[date] = mdiff
    return cells, maxdiff


def _spec_text(base: str) -> str:
    return open(os.path.join(_CORPUS, base + ".spc"),
                encoding="utf-8", errors="replace").read().lower()

# A SEATS spec reaches the span drivers through the same revdrv/sspdrv the X-11
# path does (x11ari.f takes Lseats and substitutes the SEATS chain for x11pt3),
# but this port's harness is split by decomposition method -- so pick the driver
# the spec actually selects. `seats{}` with no `x11{}` is the SEATS path.
def _seats_binary() -> str:
    for c in (os.path.join(_REPO, "build", "x13run_seats.exe"),
              os.path.join(_REPO, "build", "x13run_seats"),
              os.path.join(_REPO, "build", "Release", "x13run_seats.exe")):
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_SEATS")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13run_seats binary not found; build it first (cmake --build build).")


SEATS_BIN = _seats_binary()


def _binary_for(base: str) -> str:
    txt = _spec_text(base)
    return SEATS_BIN if ("seats{" in txt and "x11{" not in txt) else BIN


def _discover() -> list[str]:
    specs: list[str] = []
    if not os.path.isdir(_CORPUS):
        return specs
    for fn in sorted(os.listdir(_CORPUS)):
        if not fn.endswith(".spc"):
            continue
        base = fn[:-4]
        gdir = os.path.join(_GOLDEN, base)
        if "slidingspans{" not in _spec_text(base):
            continue
        if not all(os.path.exists(os.path.join(gdir, base + "." + t))
                   for t in _CORE_TAGS):
            continue
        specs.append(base)
    return specs


CASES = _discover()


def _run(base: str) -> str:
    specpath = os.path.join(_CORPUS, base + ".spc")
    proc = subprocess.run([_binary_for(base), specpath], cwd=_CORPUS, capture_output=True,
                           text=True, timeout=120)
    assert proc.returncode == 0, (
        f"{base}: x13run_x11 exited {proc.returncode}\n"
        f"stdout tail: {proc.stdout[-2000:]}\nstderr: {proc.stderr[-2000:]}")
    return proc.stdout


# _KNOWN_GAPS IS EMPTY, AND IS KEPT SO THE NEXT ONE HAS SOMEWHERE TO GO.
# Every slidingspans spec in the corpus now gates bit-exact on every table it
# ships (worst ~5e-15 across sfs/chs/tds/ads, all four spans). The three gaps
# that lived here are worth keeping a record of, because two of them were
# mis-attributed for months and the third was mis-attributed within one session:
#
#   1. `airline_slidingspans-x11regression` chs -- CLOSED by ssxmdl's
#      fixx11reg= default TOGETHER with the Ixreg demote (entry 79).
#   2. `airline_slidingspans-td` chs -- the Feb/Mar cells, ~3.5 percentage
#      points, sign following the leap year. The note here blamed "a per-span
#      phase problem in how the prior series is indexed", which was right, and
#      then named the wrong half: it said each span "places Adj[0] at its own
#      Setpri" and treated that as correct. Setpri is an EDITOR-ONLY assignment
#      (editor.f:851); the oracle leaves it at 1 while Pos1bk slides 25/37/49/61
#      with the span, which is exactly what keeps Adj date-aligned. The port
#      re-anchored it per span. Entry 83.
#   3. `fixmdl=no` + `regression{}` -- span 1 bit-exact, spans 2-4 out by a few
#      1e-7, because arima.f:1430's unconditional `CALL ssprep` was missing from
#      the span replay, so every span restarted from the MAIN run's model
#      instead of from its predecessor's. Entry 83.
#
# Rule that survives all three: the observable was the SAME table each time, and
# the owner was different each time. Isolate by building the spec WITHOUT the
# feature before believing any attribution -- and do not "fix" one of these by
# dropping a tag from a spec's save list.
_KNOWN_GAPS: dict[tuple[str, str], str] = {}

# NOTE blocks the oracle writes to Mt2 from a routine this port has not ported.
# Same family as ssphdr (entry 81) and prterx (entry 73) -- a load-bearing Mt2
# fragment swallowed by a routine skipped for what it MOSTLY does. Here it is
# arima.f:936-960's fixed-coefficient NOTE (two arms: `istrue(Arimaf,...)` for a
# fixed ARIMA parameter, `istrue(Regfx,1,Nb)` for a fixed regression one; the
# specs below reach only the second).
#
# Filtered out of the GOLDEN side rather than skipping the spec, so the ssphdr
# NOTE on the same file is still compared. Porting it makes this gate fail
# (produced gains a block gold does not have), which is the point: the list
# cannot outlive the gap.
#
# A SIBLING is unported and deliberately NOT listed here: prtmdl.f:174-177's
# `Nliter > 200` estimation-iteration NOTE. It appeared on an early draft of
# airline_slidingspans-regfixed whose fixed coefficients (0.39) were so far from
# the data that the ARMA maximisation ran past the udg's `niter` field width,
# and it left when the coefficients became plausible. No corpus spec carries it
# today, so an entry here would fail the freshness check below on day one.
_UNPORTED_NOTES = (
    "  NOTE: Fixed values have been assigned to some regression coefficients.",
)


def test_slidingspans_cases_discovered() -> None:
    """Floor assertion: this gate is TWO derived lists (specs x tags) crossed
    with a per-tag `skip` for an absent golden, which is exactly the shape that
    can shrink to nothing and still report green. Cross the parametrisation
    against the goldens ON DISK -- both sides derived, so it cannot go stale --
    and fail if any blessed span table has no case pointing at it.

    This is the check that would have caught `tds`: the engine produced no tds
    table for months, the spec did not save one, and the gate skipped it with
    the reassuring message "spec does not produce this tag"."""
    assert len(CASES) >= 3, f"slidingspans discovery shrank: {CASES}"
    on_disk = set()
    for base in os.listdir(_GOLDEN):
        gdir = os.path.join(_GOLDEN, base)
        if not os.path.isdir(gdir):
            continue
        for tag in _TAGS:
            if os.path.exists(os.path.join(gdir, base + "." + tag)):
                on_disk.add((base, tag))
    covered = {(b, t) for b in CASES for t in _TAGS}
    orphans = sorted(on_disk - covered)
    assert not orphans, f"blessed span tables with no gate case: {orphans}"


def _err_block(text: str) -> list[str]:
    """The Mt2 channel the harness dumps between ===ERR===/===END ERR===."""
    m = re.search(r"===ERR===\n(.*?)===END ERR===", text, re.S)
    return m.group(1).splitlines() if m else []


def _note_blocks(lines: list[str]) -> list[list[str]]:
    """Every ` NOTE:` block: the NOTE line plus its continuations, which are
    indented seven spaces and may contain a blank separator line (FORMAT 2000
    carries a `//`). Trailing blanks trimmed so the two sides compare equal."""
    out: list[list[str]] = []
    for i, ln in enumerate(lines):
        if not ln.lstrip().startswith("NOTE:"):
            continue
        blk = [ln.rstrip()]
        for cont in lines[i + 1:]:
            if not cont.strip():
                blk.append("")
                continue
            if cont.startswith("       "):
                blk.append(cont.rstrip())
            else:
                break
        while blk and not blk[-1]:
            blk.pop()
        out.append(blk)
    return out


@pytest.mark.skipif(not CASES, reason="no slidingspans spec ships the sfs/chs goldens")
@pytest.mark.parametrize("base", CASES)
def test_slidingspans_notes(base: str) -> None:
    """The NOTE blocks on the Mt2/.err channel, compared verbatim against the
    blessed oracle `.err` -- in BOTH directions.

    This is the gate for a diagnostic whose entire effect is an ABSENCE. When
    ssxmdl.f:27-39 (an `x11regression{span=}`) or setssp.f:47 (`fixmdl=yes`
    with a regARIMA trading day) demotes Itd/Ihol from 1 to -1, the run simply
    stops producing the tds -- and, for Itd, the ads -- table. A save-table gate
    skips an absent golden with a reassuring message, so the only thing that can
    tell "correctly suppressed" from "silently broken" is the text the oracle
    prints when it suppresses (ssphdr.f:145-152). It was unported: the blessed
    `airline_slidingspans-td` golden has carried that NOTE since the spec
    landed, and nothing read it.

    Both directions matter. Four of the discovered specs have NO note block, so
    an implementation that emitted the NOTE unconditionally would fail here --
    without those, this test could not tell a correct emitter from one that
    shouts on every run.

    Scope: NOTE blocks only. The WARNING blocks in the same `.err` files come
    from the spectrum section of the deferred `.out` print engine (the peaks
    themselves are gated through the savelog `spcrsd`/`peaks` udg keys), so a
    whole-file comparison would fail for an unrelated and already-tracked
    reason."""
    goldpath = os.path.join(_GOLDEN, base, base + ".err")
    if not os.path.exists(goldpath):
        pytest.skip(f"{base} has no blessed .err")
    with open(goldpath, encoding="utf-8", errors="replace") as f:
        gold = [b for b in _note_blocks([ln.rstrip("\n") for ln in f])
                if b[0] not in _UNPORTED_NOTES]

    produced = _note_blocks(_err_block(_run(base)))
    assert produced == gold, (
        f"{base}: NOTE blocks differ\n  want: {gold}\n  got:  {produced}")


def test_unported_notes_still_unported() -> None:
    """`_UNPORTED_NOTES` subtracts from the golden side of the gate above, so a
    stale entry would silently excuse a NOTE the engine had started emitting
    correctly -- or, worse, one it had started emitting WRONG. Require every
    entry to still appear in some blessed golden, and delete it the day the
    routine behind it is ported."""
    seen: set[str] = set()
    for base in CASES:
        p = os.path.join(_GOLDEN, base, base + ".err")
        if not os.path.exists(p):
            continue
        with open(p, encoding="utf-8", errors="replace") as f:
            for blk in _note_blocks([ln.rstrip("\n") for ln in f]):
                if blk[0] in _UNPORTED_NOTES:
                    seen.add(blk[0])
    assert seen == set(_UNPORTED_NOTES), (
        "_UNPORTED_NOTES is out of date -- no golden carries "
        f"{sorted(set(_UNPORTED_NOTES) - seen)}")


def test_slidingspans_notes_can_fail() -> None:
    """The corpus must hold at least one spec on each side of the NOTE gate,
    or the assertion above degenerates: all-empty passes trivially, all-present
    cannot catch an unconditional emitter."""
    withnote, without = [], []
    for base in CASES:
        p = os.path.join(_GOLDEN, base, base + ".err")
        if not os.path.exists(p):
            continue
        with open(p, encoding="utf-8", errors="replace") as f:
            (withnote if _note_blocks([ln.rstrip("\n") for ln in f])
             else without).append(base)
    assert withnote, "no slidingspans golden carries a NOTE block"
    assert without, "no slidingspans golden is NOTE-free"


@pytest.mark.skipif(not CASES, reason="no slidingspans spec ships the sfs/chs goldens")
@pytest.mark.parametrize("base", CASES)
@pytest.mark.parametrize("tag", _TAGS)
def test_slidingspans_table(base: str, tag: str) -> None:
    if (base, tag) in _KNOWN_GAPS:
        pytest.skip(f"KNOWN GAP -- {_KNOWN_GAPS[(base, tag)]}")
    goldpath = os.path.join(_GOLDEN, base, base + "." + tag)
    if not os.path.exists(goldpath):
        # An ABSENT golden is a claim too, and until now it was the one thing
        # here that could not fail. When the spec ASKED for the table and the
        # oracle still wrote none, the engine must write none either -- that is
        # the whole observable of an Itd/Ihol demote to -1 (ssxmdl.f:31-33,
        # setssp.f:47). Only assert it when the save list names the tag: the
        # harness dumps every span table it computed regardless of save=, so on
        # a spec that never asked, an absent golden says nothing.
        if re.search(r"save\s*=[^}]*\b" + tag + r"\b", _spec_text(base)):
            pcells, _ = _read_produced(_run(base), tag)
            assert not pcells, (
                f"{base}.{tag}: the oracle produced no {tag} table for a spec "
                f"that saves it, but the engine produced {len(pcells)} cells")
            pytest.skip(f"{base}: {tag} correctly suppressed (verified empty)")
        pytest.skip(f"{base} does not produce the {tag} table")

    cells, maxdiff = _read_golden(goldpath)
    assert cells, f"{base}.{tag}: empty golden"

    out = _run(base)
    pcells, pmaxdiff = _read_produced(out, tag)
    assert pcells, f"{base}.{tag}: produced no rows (Issap likely stuck at 1)"

    missing = set(cells) - set(pcells)
    assert not missing, f"{base}.{tag}: missing {len(missing)} cells, e.g. {sorted(missing)[:5]}"
    worst = 0.0
    worst_key = None
    for k, gv in cells.items():
        pv = pcells[k]
        err = abs(gv - pv) / abs(gv) if gv != 0 else abs(gv - pv)
        if err > worst:
            worst, worst_key = err, k
    assert worst <= RTOL, f"{base}.{tag}: worst rel err {worst} at {worst_key}"

    worst_md = 0.0
    worst_md_key = None
    for k, gv in maxdiff.items():
        pv = pmaxdiff.get(k)
        assert pv is not None, f"{base}.{tag}: missing Max_%_DIFF at {k}"
        err = abs(gv - pv) / abs(gv) if gv != 0 else abs(gv - pv)
        if err > worst_md:
            worst_md, worst_md_key = err, k
    assert worst_md <= RTOL, f"{base}.{tag}: worst Max_%_DIFF rel err {worst_md} at {worst_md_key}"
