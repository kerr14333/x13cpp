"""The `.err` of EVERY run the oracle blessed -- not only the ones it halted.

This started life as `test_halt_err.py`, over the 8 goldens whose run ended in
an ERROR. That scope was itself the defect. 525 blessed goldens ship a
non-empty `.err`; 8 of them were compared here, a handful more in
test_m1_parse (parse REJECTIONS only), test_transform_err,
test_slidingspans_tables (two NOTEs) and test_composite_psuadd (one case). The
other ~490 carried the oracle's WARNING and NOTE text and nothing read a line
of it.

What that hid, measured the day this file was widened: **396 of the 525
differed.** 313 were the spectrum peak WARNINGs -- three texts in `spcrsd.f`
and two in `spcdrv.f`, on every monthly run that finds a visually significant
peak, computed and gated as NUMBERS since entry 46 and never once emitted as
text. The rest is a long tail of unported blocks (`_UNPORTED_BLOCKS` below -- nineteen
when this file was widened, and nineteen still: the "sixteen by the end of the
same day" this docstring and the handoff both claimed was never applied to the
tuple. Sixteen after entry 110, fourteen after entry 111, THIRTEEN after entry
112 -- do not write a number here that the tuple does not show), and three real
defects that had nothing to do with the warnings:

  * `x11mdl.f:316`'s AIC-reject NOTE was routed through `writln(.., Mt2, Mt2,
    ..)` -- the same unit twice -- so every line came out DOUBLED and indented
    one column too far;
  * that same block is CB-45, where the ORACLE's process is killed by the
    Fortran runtime, and this engine ran a whole X-11 adjustment past the point
    the oracle stopped;
  * `gtxreg.f:894`'s refusal was emitting a blank line its FORMAT does not have
    (the second instance of that; `x11mdl.f:614` was the first).

None of the three is a spectrum diagnostic. They were found because the channel
became readable, which is this project's oldest lesson arriving at its largest
scale: a channel nobody can read is a channel nobody gates.

DISCOVERY, not a case list: every blessed golden with a non-empty `.err` and a
corpus spec to run. Composite cases are covered too -- the composite harness
emits one `===ERR <base>===` block per component, which is exactly what the
oracle writes to each component's own `.err`.

Comparison is the WHOLE block, verbatim: a blank line in the wrong place is a
transcription bug like any other, and comparing only the ERROR lines is how two
of them survived.

Run:  python -m pytest tests/parity/test_err_block.py -q
"""
from __future__ import annotations

import os
import re
import subprocess

import pytest

from oracle_outcome import expected_exit, oracle_halted, oracle_reported
from spec_text import spec_body

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus")
_GOLDEN = os.path.join(_REPO, "tests", "golden")


def _binary(name: str) -> str:
    for c in (os.path.join(_REPO, "build", name + ".exe"),
              os.path.join(_REPO, "build", name)):
        if os.path.exists(c):
            return c
    raise FileNotFoundError(f"{name} not found; build it first.")


BIN_X11 = _binary("x13run_x11")
BIN_SEATS = _binary("x13run_seats")
BIN_COMPOSITE = _binary("x13run_composite")


def _mta_for(spec_id: str) -> str | None:
    """The composite `.mta` that OWNS this spec, if any.

    A composite component is not runnable on its own: the oracle drives the
    whole set from the metafile and each component's `.err` is written during
    that one run. So the case is the unit, and the harness emits one
    `===ERR <base>===` block per member.
    """
    d = os.path.dirname(os.path.join(_CORPUS, *spec_id.split("/")))
    mta = os.path.join(d, "composite.mta")
    return mta if os.path.exists(mta) else None


def _cases() -> list[str]:
    """Corpus-relative spec ids for every golden that ships a non-empty .err."""
    out: list[str] = []
    for root, _dirs, files in os.walk(_GOLDEN):
        for f in files:
            if not f.endswith(".err"):
                continue
            base = f[:-4]
            if os.path.basename(root) != base:
                continue
            if os.path.getsize(os.path.join(root, f)) == 0:
                continue
            rel = os.path.relpath(root, _GOLDEN)
            if os.path.exists(os.path.join(_CORPUS, rel + ".spc")):
                out.append(rel.replace(os.sep, "/"))
    return sorted(out)


CASES = _cases()
HALT_CASES = [c for c in CASES
              if oracle_halted(os.path.join(_GOLDEN, *c.split("/")),
                               c.split("/")[-1])
              and oracle_reported(os.path.join(_GOLDEN, *c.split("/")),
                                  c.split("/")[-1])]

# The engine does not yet emit these blocks, so they are subtracted from the
# GOLDEN side. Each entry is the block's first line, matched after stripping;
# the indented continuations under it go with it.
#
# Same contract as test_slidingspans_tables._UNPORTED_NOTES: a stale entry would
# excuse a block the engine had started emitting -- possibly wrongly -- so
# test_unported_blocks_still_unported requires every entry to still be carried
# by some golden AND to still be absent from every engine run.
#
# This list IS the inventory of what the `.err` front is missing, and it is
# meant to shrink. It opened at nineteen; x11pt3.f's three force NOTEs came
# off it the same day. Count the tuple, do not read a number here.
#
# EVERY `.f` attribution below was re-derived on 2026-08-18 by joining the
# oracle's continuation records and grepping the message text, because SEVEN
# of them had been wrong. Do that again rather than trusting these -- a
# citation is a claim, and this file is where that has been proved most
# often.
_UNPORTED_BLOCKS = (
    # (the SEATS longer-forecast NOTE used to sit here, credited to seatop.f.
    # It is editor.f:387-406, and it is PORTED. The RAISE had been ported
    # alone months earlier -- parse_spec.cpp already forced nfcst up to
    # max(12,3*Sp) and said so in a comment -- so the gap was three writln
    # lines under a rule the port had already agreed with. A silent horizon
    # change is precisely what the NOTE exists to prevent.)
    # (fcnar.f's three root WARNINGs used to sit here, credited to chkrt2.f.
    # They are PORTED -- the blocker was never the message, it was `Lprier` =
    # `Prttab(LESTIE)`, i.e. the print-table store. chkrt2.f writes a different
    # sentence that no golden carries.)
    # (arima.f:935-960's two fixed-coefficient wordings used to sit here. They
    # are PORTED -- they were behind Prttab(LESTES), the same store that was
    # blocking fcnar's; entry 110 landed it and entry 111 read it.)
    # (editor.f:2831's "No seasonal adjustment this run" used to sit here,
    # credited to x11ari.f/prtsum. It is PORTED, and the transcription was one
    # writln: the work was that `Readok` is NOT `inptok`. x12run.f:105 calls
    # editor only `IF(Rok.and.Lexok)`, so a reader-level error skips editor and
    # the trailer never prints -- 6 goldens carry it, 31 carry an ERROR without
    # it. See ParseSettings::readok and editor_refusal().)
    # (amdfct.f:56's "Insufficient data ..." NOTE used to sit here. PORTED --
    # and the work was not the message, it was WHERE amdfct runs: arima.f:874
    # precedes prlkhd and chkres, this port called it after both. Porting it
    # also moves five specs off `_collapse` and back onto the strict
    # comparison, which is what makes entry 113's leading-blank mutation
    # measurable on them -- it went from 1 to 6.)
    # (the "User-defined prior adjustment factor not provided" WARNING used to
    # sit here, credited to x11pt1.f. It has TWO emitters and neither is that
    # one: prtfct.f:489 for the FORECAST window and mkback.f:288 for the
    # BACKCAST window. The forecast half is PORTED and gated by the three
    # goldens that carry it. The backcast half is behind Prttab(LFORBC), whose
    # deftab entry is F -- see forecast.cpp's bcstout.)
    # (prlkhd.f:251's AIC NOTE used to sit here. PORTED -- and the message was
    # the small half: prlkhd.f:248-355 is a THREE-armed chain that this port
    # had fused into `if (!lclaic || !d.convrg) return;`, dropping the NOTE
    # AND the third arm, which resets every statistic to DNOTST on an
    # exact-ML fit that did not converge. Its FORMAT opens with a `/`, so the
    # record above it is EMPTY, not writln's two-space blank.)
    # (checkres' normality NOTEs used to sit here. All FIVE of nrmtst.f's are
    # ported -- the two the corpus carries plus the kurtosis upper bound and
    # both of Geary's a, which no golden reaches. See checkres.cpp.)
    # ssmdl.f:285,300's change-of-regime arm (CB-39, deliberately a wall).
    "NOTE: The following change of regime regression variables are not",
    # revchk.f:801-805 and :1131 -- the history{} option NOTEs. NOT revdrv.f,
    # and the second is a FORMAT (leading `/` = an EMPTY record), not a writln.
    # (x11mdl.f:639's reweight NOTE used to sit here. PORTED -- and the reason
    # it was skipped is the interesting part: the code beside it said "part of
    # the deferred .out print engine: writes only to Mt1/Mt2, never STDERR",
    # which was TRUE and is not a reason. Mt2 is the `.err`. Entry 109 made
    # that channel compared and the note beside the skip was never revisited.)
    # (idotlr.f:485's "Unable to test ..." used to sit here, credited to
    # otlaic.f. PORTED -- and it is the FIRST gated consumer of getprt's LEVEL
    # fill: its guard is `Prttab(LOTLIT)`, whose `deftab` entry is F, so all
    # three goldens reach it through `outlier{print=all}`. It was matched by
    # PREFIX because the message names the outlier -- one spec's `TC2020.Mar`
    # against the next spec's `AO2020.Apr` -- which is still how any new entry
    # here should be written.)
    # pracf2.f:2 -- the squared-residual ACF on too short a series. The NOTE
    # fronts an ENTIRELY unported routine: there is no `ac2` table at all.
    "NOTE: X-13ARIMA-SEATS will not compute the ACF of the squared residuals for",
    # spectrum.f:2583 -- and a CENSUS DEFECT in its own right: the message is
    # assembled from two pieces in the wrong order, so the oracle prints
    # "transitoryThe innovation variance of the greater than one is".
    "transitoryThe innovation variance of the greater than one is",
)

# Specs where the ENGINE writes a refusal the ORACLE does not: a WALL. The run
# STOPS there, so everything after it is missing by construction and all that can
# be asserted is that the wall is present and that everything AHEAD of it matches
# line for line -- a walled spec cannot quietly acquire a second divergence
# earlier in the file. Keyed by spec id with the wall's own first line, so a wall
# whose text changes lands here rather than passing.
_ENGINE_WALLS = {
    "generated/expgs_ar2-seats": "ERROR: SEATS inadmissible decomposition",
    "generated/expgs_seats": "ERROR: SEATS inadmissible decomposition",
    "edge/airline_slidingspans-regime-td":
        "ERROR: slidingspans{} with a change-of-regime regression variable",
}

# ...and specs where the engine writes something extra and CARRIES ON: a NOTE
# about a gap it works around, not a refusal. These need the opposite treatment
# -- the block is cut out of the ENGINE side (the mirror of _UNPORTED_BLOCKS on
# the golden side) and the rest is compared in full.
#
# They were in _ENGINE_WALLS at first, and the mutation test is what separated
# them: doubling agr3s's NOTE -- the `writln(.., Mt2, Mt2, ..)` defect this file
# had just found in x11mdl.f -- changed nothing, because the wall contract only
# looks at the block's first line and at what precedes it. A block the engine
# emits after a NOTE is exactly the region that assertion cannot see.
_ENGINE_NOTES = {
    "census-examples/composite-seats/total":
        "NOTE: The indirect adjustment past the end of the series",
    "census-examples/composite-seats-total/total":
        "NOTE: The indirect adjustment past the end of the series",
}


def _err_block(text: str, who: str | None = None) -> list[str]:
    marker = "===ERR===" if who is None else f"===ERR {who}==="
    m = re.search(re.escape(marker) + r"\n(.*?)===END ERR===", text, re.S)
    return m.group(1).split("\n") if m else []


_RUNS: dict[str, subprocess.CompletedProcess] = {}


def _run_raw(key: str, argv: list[str]) -> subprocess.CompletedProcess:
    """One process per KEY, cached: a composite case is 6 goldens' worth of
    output from a single run, and re-running it per parametrisation would cost
    more than the rest of the file put together."""
    if key not in _RUNS:
        _RUNS[key] = subprocess.run(argv, capture_output=True, text=True)
    return _RUNS[key]


def _run(spec_id: str) -> tuple[int, list[str], str]:
    """(exit code, the spec's own `.err` block, the whole stdout)."""
    mta = _mta_for(spec_id)
    if mta is not None:
        p = _run_raw(mta, [BIN_COMPOSITE, mta])
        return p.returncode, _err_block(p.stdout, spec_id.split("/")[-1]), p.stdout
    spc = os.path.abspath(os.path.join(_CORPUS, *spec_id.split("/")) + ".spc")
    binp = BIN_SEATS if "seats{" in spec_body(spc).lower().replace(" ", "") \
        else BIN_X11
    p = _run_raw(spc, [binp, spc])
    return p.returncode, _err_block(p.stdout), p.stdout


_SAVE_RE = re.compile(r"save\s*=\s*\(([^)]*)\)", re.I)


def _requested_tags(spec_id: str) -> set[str]:
    """Every table tag the SPEC asked for, across all its save= lists."""
    spc = os.path.join(_CORPUS, *spec_id.split("/")) + ".spc"
    body = spec_body(spc)
    tags: set[str] = set()
    for m in _SAVE_RE.finditer(body):
        for tok in re.split(r"[\s,]+", m.group(1).strip()):
            if tok:
                tags.add(tok.lower())
    return tags


def _dumped_tags(stdout: str) -> set[str]:
    """Table tags the harness actually printed after the ===END ERR=== marker."""
    tail = stdout.split("===END ERR===", 1)
    if len(tail) < 2:
        return set()
    out: set[str] = set()
    for ln in tail[1].splitlines():
        parts = ln.split()
        if len(parts) >= 3 and re.fullmatch(r"\d{6}", parts[1]):
            out.add(parts[0].lower())
    return out


def _trim(lines: list[str]) -> list[str]:
    out = [ln.rstrip() for ln in lines]
    while out and not out[-1]:
        out.pop()
    return out


_BLOCK_START = re.compile(r"^\s*(NOTE|WARNING|ERROR)\b")


def _block_end(lines: list[str], i: int) -> int:
    """Index of the LAST line belonging to the block that starts at `i`.

    Not "the next blank line": x11mdl.f's reweight NOTE ends with a blank line
    and then a two-row daily-weight table, all of it one WRITE. So scan forward
    over blanks, keeping the last INDENTED non-blank line, and stop at
      * another NOTE/WARNING/ERROR -- a new block, and
      * anything at column 1 -- the `*-*-*` rules that head the sliding-spans
        and history sections, and the file's own two header lines.
    Trailing blanks are deliberately left behind: each block owns only the ONE
    blank that writln's `lblnk` put AHEAD of it."""
    last = i
    j = i + 1
    while j < len(lines):
        ln = lines[j]
        if ln.strip():
            if not ln.startswith(" ") or _BLOCK_START.match(ln):
                break
            last = j
        j += 1
    return last


def _drop_unported(lines: list[str]) -> list[str]:
    """Remove each unported block, and the blank line ahead of it."""
    out: list[str] = []
    i = 0
    while i < len(lines):
        s = lines[i].strip()
        if any(s.startswith(b) for b in _UNPORTED_BLOCKS):
            # writln's `lblnk` puts exactly ONE blank ahead of the block; the
            # others belong to whatever printed before it.
            if out and not out[-1].strip():
                out.pop()
            i = _block_end(lines, i) + 1
            continue
        out.append(lines[i])
        i += 1
    return out


def _diff(want: list[str], got: list[str]) -> list[str]:
    import difflib
    return list(difflib.unified_diff(want, got, "oracle", "engine", lineterm=""))


def _collapse(lines: list[str]) -> list[str]:
    """Runs of blank lines collapsed to one.

    Used ONLY on a spec whose golden carried an unported block. Cutting a block
    out of a text stream cannot preserve the blank structure around it: writln's
    `lblnk` owns the blank AHEAD of a block, but the FORMAT-based writers own
    one or two blanks AFTER, and there is no way to tell from the text which
    kind was removed. Everywhere else the comparison stays exact -- which is
    where it earns its keep, because two of the three defects this file found
    on its first run were a blank line in the wrong place. As blocks get
    ported, specs move back to the strict comparison by themselves."""
    out: list[str] = []
    for ln in lines:
        if not ln.strip() and out and not out[-1].strip():
            continue
        out.append(ln)
    return out


def _golden_raw(spec_id: str) -> list[str]:
    base = spec_id.split("/")[-1]
    gpath = os.path.join(_GOLDEN, *spec_id.split("/"), base + ".err")
    with open(gpath, encoding="utf-8", errors="replace") as fh:
        return fh.read().replace("\r\n", "\n").split("\n")


def _golden_err(spec_id: str) -> tuple[list[str], bool]:
    """(the golden block with unported text removed, whether any was removed)."""
    raw = _golden_raw(spec_id)
    kept = _drop_unported(raw)
    return _trim(kept), len(kept) != len(raw)


def _drop_engine_note(lines: list[str], first: str) -> list[str]:
    """Cut an _ENGINE_NOTES block out of the ENGINE side -- the mirror of
    _drop_unported, same block-extent rule, same one leading blank."""
    out: list[str] = []
    i = 0
    while i < len(lines):
        if lines[i].strip().startswith(first):
            if out and not out[-1].strip():
                out.pop()
            i = _block_end(lines, i) + 1
            continue
        out.append(lines[i])
        i += 1
    return out


@pytest.mark.parametrize("spec_id", CASES)
def test_err_block_matches_oracle(spec_id: str) -> None:
    gold, dropped = _golden_err(spec_id)
    _rc, produced, _stdout = _run(spec_id)
    got = _trim(produced)

    note = _ENGINE_NOTES.get(spec_id)
    if note is not None:
        # EXACTLY once, and the count is not a formality: _drop_engine_note
        # ends a block at the next NOTE/WARNING/ERROR, so a note emitted TWICE
        # is two blocks and both get cut -- which is precisely the
        # `writln(.., Mt2, Mt2, ..)` defect this file found in x11mdl.f, and
        # the mutation that restored it here passed until this line existed.
        n = sum(1 for ln in got if ln.strip().startswith(note))
        assert n == 1, (
            f"{spec_id}: the engine NOTE {note!r} appears {n} times, expected "
            "exactly 1 (0 -> drop it from _ENGINE_NOTES so the block is "
            "compared in full; 2 -> it is being written to two channels that "
            "are the same unit)")
        cut = _trim(_drop_engine_note(got, note))
        assert cut == (_collapse(gold) if dropped else gold), (
            f"{spec_id}: .err differs outside the engine's own NOTE\n"
            + "\n".join(_diff(gold, cut)))
        return

    if dropped:
        gold, got = _collapse(gold), _collapse(got)

    wall = _ENGINE_WALLS.get(spec_id)
    if wall is not None:
        # A walled spec: the engine refuses where the oracle continues, so the
        # blocks cannot match. What IS asserted is that the wall is the only
        # reason -- its text is present, and everything the oracle wrote AHEAD
        # of the refusal point still matches line for line.
        assert any(ln.strip().startswith(wall) for ln in got), (
            f"{spec_id}: expected the wall {wall!r} in the engine .err; "
            "remove the _ENGINE_WALLS entry if it is gone\n"
            + "\n".join(_diff(gold, got)))
        cut = next(i for i, ln in enumerate(got) if ln.strip().startswith(wall))
        head = _trim(got[:cut])
        assert head == _trim(gold[:len(head)]), (
            f"{spec_id}: the engine diverges BEFORE its wall\n"
            + "\n".join(_diff(_trim(gold[:len(head)]), head)))
        return

    assert got == gold, (f"{spec_id}: .err differs\n" + "\n".join(_diff(gold, got)))


@pytest.mark.parametrize("spec_id", CASES)
def test_exit_matches_oracle(spec_id: str) -> None:
    """`expected_exit` is an EQUALITY. Where the oracle halted -- or was killed
    by its own runtime, CB-45 -- the engine must stop too; where it completed,
    the engine must not invent a refusal."""
    gdir = os.path.join(_GOLDEN, *spec_id.split("/"))
    base = spec_id.split("/")[-1]
    want = expected_exit(gdir, base)
    if spec_id in _ENGINE_WALLS:
        want = 1          # the wall is the whole point of the entry
    if _mta_for(spec_id) is not None:
        # The composite harness returns ONE code for the whole metafile, so a
        # component's own outcome is not separable from it.
        pytest.skip("composite: one exit code covers the whole case")
    rc, _produced, _stdout = _run(spec_id)
    assert rc == want, f"{spec_id}: oracle wanted exit {want}, engine gave {rc}"


@pytest.mark.parametrize("spec_id", HALT_CASES)
def test_halt_writes_no_table_the_oracle_withheld(spec_id: str) -> None:
    """A run that halts before the adjustment must dump NO adjustment tables.

    The mirror of the assertion above, and it needs its own test because the
    table gates cannot express it: they skip an absent golden. So gate the
    absence on the REQUEST -- for every tag the spec's `save=` asked for, if
    the oracle wrote no file, the engine must print no rows.

    `extra/airline_estimate-maxiter-noconverge` is why: the harness decided
    whether it had tables to print from the x11ptr span pointers, which the
    pre-MODEL editor geometry sets, so a run that died in estimation printed
    b1/d10/d11/d12/d13 as 144 zeros apiece. Every table gate skipped them and
    every .err comparison passed."""
    if _mta_for(spec_id) is not None:
        pytest.skip("composite: the harness's table dump is not per component")
    base = spec_id.split("/")[-1]
    gdir = os.path.join(_GOLDEN, *spec_id.split("/"))
    _rc, _err, stdout = _run(spec_id)
    dumped = _dumped_tags(stdout)
    for tag in sorted(_requested_tags(spec_id)):
        if os.path.exists(os.path.join(gdir, base + "." + tag)):
            continue
        assert tag not in dumped, (
            f"{spec_id}: the spec asked for `{tag}`, the oracle wrote no "
            f"`{base}.{tag}`, and the engine printed one anyway")


def test_err_cases_discovered() -> None:
    """A discovery predicate that shrinks to nothing reports green. This set is
    derived from the goldens on disk, so it only grows when a spec is blessed --
    but the floor is what makes a broken predicate visible.

    The floor was 8 while this file only looked at halting runs. That number was
    the bug."""
    assert len(CASES) >= 525, f"only {len(CASES)} .err cases discovered"
    assert len(HALT_CASES) >= 8, f"halt subset shrank to {len(HALT_CASES)}"
    # One of each shape that cost an increment to find, by name.
    for need in ("extra/airline_automdl-user-reg-noconverge",   # itrerr, automatic arm
                 "extra/airline_estimate-maxiter-noconverge",   # itrerr, explicit arm
                 "extra/airline_x11regression-aictest-tdrej",   # CB-45
                 "census-examples/composite-fixed/region_north",  # composite path
                 "census-examples/01-basic-x11",                # spcdrv's warning
                 # The one spec in the corpus that pins WHERE the residual
                 # spectrum runs: its residual WARNING (arima.f:1126, the
                 # estimation phase) has to come out AHEAD of three x11pt3
                 # force NOTEs. Derive the residual block late -- with the
                 # other three spectra, where it used to live -- and it lands
                 # with the last of them instead. Every other .err golden is
                 # blind to the difference.
                 "extra/airline_force-constant-rsdpeak"):
        assert need in CASES, f"{need} is no longer discovered"


def test_unported_blocks_still_unported() -> None:
    """Both directions, because either one alone rots.

    A block that no golden carries is a stale entry excusing nothing. A block
    the ENGINE has started emitting is worse: it stays subtracted from the
    golden side, so the comparison would pass whatever the engine wrote."""
    seen: set[str] = set()
    for spec_id in CASES:
        base = spec_id.split("/")[-1]
        p = os.path.join(_GOLDEN, *spec_id.split("/"), base + ".err")
        with open(p, encoding="utf-8", errors="replace") as fh:
            for ln in fh:
                s = ln.strip()
                seen.update(b for b in _UNPORTED_BLOCKS if s.startswith(b))
    assert seen == set(_UNPORTED_BLOCKS), (
        "_UNPORTED_BLOCKS is out of date -- no golden carries "
        f"{sorted(set(_UNPORTED_BLOCKS) - seen)}")

    emitted: set[str] = set()
    for spec_id in CASES:
        _rc, produced, _stdout = _run(spec_id)
        for ln in produced:
            s = ln.strip()
            emitted.update(b for b in _UNPORTED_BLOCKS if s.startswith(b))
    assert not emitted, (
        "the engine now emits blocks still listed as unported -- delete them "
        f"from _UNPORTED_BLOCKS so they are compared: {sorted(emitted)}")


def test_engine_walls_still_walled() -> None:
    """Every _ENGINE_WALLS / _ENGINE_NOTES entry names a spec where the engine
    still says something the oracle does not. Delete the entry when it stops --
    and the comparison above becomes the full one for that spec on the same
    commit."""
    for table, what in ((_ENGINE_WALLS, "_ENGINE_WALLS"),
                        (_ENGINE_NOTES, "_ENGINE_NOTES")):
        for spec_id, first in table.items():
            assert spec_id in CASES, f"{spec_id} is no longer a discovered case"
            _rc, produced, _stdout = _run(spec_id)
            assert any(ln.strip().startswith(first) for ln in produced), (
                f"{spec_id}: {first!r} is gone -- drop it from {what}")
    assert not (set(_ENGINE_WALLS) & set(_ENGINE_NOTES)), (
        "a spec cannot be both: a wall STOPS the run and only its head is "
        "comparable, a note does not and the whole rest is")
