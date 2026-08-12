"""The `.err` of every run the oracle HALTED after it had already reported.

`oracle_outcome.expected_exit` taught the phase gates to accept a run that
halts. It did not make anyone read WHAT the halting run said. That leaves the
whole error-reporting front -- prterr.f, itrerr.f, prarma.f, chkmu.f's
constant-term NOTE, x11mdl.f's reweight abend -- in the class CLAUDE.md calls a
channel nobody can read: the harness prints the Mt2 block, the goldens carry
the oracle's, and until this file nothing compared them outside the PARSE gate.

test_m1_parse does compare ERROR text, but only on specs it classifies as
REJECTED, and rejection there means "errored with no complete report behind
it". A late halt is by construction the other case: `.udg` non-empty, so
`_oracle_ok` says accepted, so the `.err` comparison is skipped. Both of the
specs that closed the estimation-halt gap sit exactly in that hole -- they
would have passed test_m1_parse with the engine emitting an EMPTY `.err`, and
did, before this gate existed.

DISCOVERY, not a case list: every blessed golden whose `.err` carries an
`ERROR:` line and whose `.udg` is non-empty. That is `oracle_halted() and
oracle_reported()` -- the same two predicates the phase gates use, so the set
cannot drift away from them. A floor assertion guards the parametrisation
against silently shrinking to nothing.

Comparison is the WHOLE block, verbatim, not just its ERROR lines: a blank
line in the wrong place is a transcription bug like any other, and comparing
only the ERROR lines is how one survived (x11mdl.f:614's reweight abend was
emitting a leading blank the FORMAT does not have).

Run:  python -m pytest tests/parity/test_halt_err.py -q
"""
from __future__ import annotations

import os
import re
import subprocess

import pytest

from oracle_outcome import oracle_halted, oracle_reported
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


def _cases() -> list[str]:
    """Corpus-relative spec ids whose golden shows a LATE halt."""
    out: list[str] = []
    for root, _dirs, files in os.walk(_GOLDEN):
        for f in files:
            if not f.endswith(".err"):
                continue
            gdir = root
            base = f[:-4]
            if os.path.basename(gdir) != base:
                continue
            if not (oracle_halted(gdir, base) and oracle_reported(gdir, base)):
                continue
            rel = os.path.relpath(gdir, _GOLDEN)
            if os.path.exists(os.path.join(_CORPUS, rel + ".spc")):
                out.append(rel.replace(os.sep, "/"))
    return sorted(out)


CASES = _cases()

# The engine does not yet emit these blocks, so they are subtracted from the
# GOLDEN side. Each entry is the block's first line, matched after stripping.
# Same contract as test_slidingspans_tables._UNPORTED_NOTES: a stale entry
# would excuse a block the engine had started emitting -- possibly wrongly --
# so test_unported_halt_blocks_still_unported requires every entry to still be
# carried by some golden here.
#
# spcrsd.f:140-184 -- the residual-spectrum peak WARNINGs. The peaks themselves
# ARE computed and gated (the `spcrsd`/`peaks.*` udg keys, test_spectrum_peaks);
# it is the three-line WARNING that has no C++. Three sibling texts (seasonal,
# trading day, both), each with a SEATS and a regARIMA wording.
_UNPORTED_BLOCKS = (
    "WARNING: At least one visually significant seasonal peak has been found",
)


def _err_block(text: str) -> list[str]:
    m = re.search(r"===ERR===\n(.*?)===END ERR===", text, re.S)
    return m.group(1).split("\n") if m else []


def _run(spec_id: str) -> tuple[int, list[str], str]:
    spc = os.path.abspath(os.path.join(_CORPUS, *spec_id.split("/")) + ".spc")
    binp = BIN_X11
    with open(spc, encoding="utf-8", errors="replace") as fh:
        if "seats{" in fh.read().lower().replace(" ", ""):
            binp = BIN_SEATS
    p = subprocess.run([binp, spc], capture_output=True, text=True)
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


def _drop_unported(lines: list[str]) -> list[str]:
    """Remove each unported block: its first line plus the indented
    continuations under it, and the blank line the block's FORMAT put ahead
    of it."""
    out: list[str] = []
    i = 0
    while i < len(lines):
        if lines[i].strip() in _UNPORTED_BLOCKS:
            # writln's `lblnk` puts exactly ONE blank ahead of the block; the
            # others belong to whatever printed before it.
            if out and not out[-1].strip():
                out.pop()
            i += 1
            while i < len(lines) and lines[i].startswith("       "):
                i += 1
            continue
        out.append(lines[i])
        i += 1
    return out


@pytest.mark.parametrize("spec_id", CASES)
def test_halt_err_matches_oracle(spec_id: str) -> None:
    base = spec_id.split("/")[-1]
    gpath = os.path.join(_GOLDEN, *spec_id.split("/"), base + ".err")
    with open(gpath, encoding="utf-8", errors="replace") as fh:
        gold = _trim(_drop_unported(fh.read().replace("\r\n", "\n").split("\n")))

    rc, produced, stdout = _run(spec_id)
    # The oracle halted, so the engine must halt too -- and it must still have
    # printed the block. A harness that returns early on a fatal throws the
    # only observable this class of spec has (entry 85, from the other side).
    assert rc == 1, f"{spec_id}: oracle halted, engine exited {rc}"
    assert produced, f"{spec_id}: engine printed no ===ERR=== block at all"
    assert _trim(produced) == gold, (
        f"{spec_id}: .err differs\n"
        + "\n".join(_diff(gold, _trim(produced))))


@pytest.mark.parametrize("spec_id", CASES)
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


def _diff(want: list[str], got: list[str]) -> list[str]:
    import difflib
    return list(difflib.unified_diff(want, got, "oracle", "engine", lineterm=""))


def test_halt_cases_discovered() -> None:
    """A discovery predicate that shrinks to nothing reports green. This set is
    derived from the goldens on disk, so it only grows when a halting spec is
    blessed -- but the floor is what makes a broken predicate visible."""
    assert len(CASES) >= 8, f"only {len(CASES)} late-halt cases discovered: {CASES}"
    # Both halves of the estimation-halt front must be present: the automatic
    # arm of itrerr (two remedies) and the explicit arm (three, with prarma's
    # start values under option 2). They are the reason this file exists.
    for need in ("extra/airline_automdl-user-reg-noconverge",
                 "extra/airline_estimate-maxiter-noconverge"):
        assert need in CASES, f"{need} is no longer discovered"


def test_unported_halt_blocks_still_unported() -> None:
    seen: set[str] = set()
    for spec_id in CASES:
        base = spec_id.split("/")[-1]
        p = os.path.join(_GOLDEN, *spec_id.split("/"), base + ".err")
        with open(p, encoding="utf-8", errors="replace") as fh:
            for ln in fh:
                if ln.strip() in _UNPORTED_BLOCKS:
                    seen.add(ln.strip())
    assert seen == set(_UNPORTED_BLOCKS), (
        "_UNPORTED_BLOCKS is out of date -- no golden carries "
        f"{sorted(set(_UNPORTED_BLOCKS) - seen)}")
