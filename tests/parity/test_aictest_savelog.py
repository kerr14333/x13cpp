"""The whole `aictest.*` savelog surface, compared as TEXT.

Six routines over two harnesses. On the regARIMA side (`x13run_m3`):
`tdaic.f` / `easaic.f` / `lomaic.f` each emit a per-candidate AICC table from
inside the test, and `svaict.f` then reports which group the final model kept,
the AICC difference that decided it, and the threshold when a non-default one
was in force. On the X-11 side (`x13run_x11`): `x11aic.f`'s trading-day and
Easter tables and `x11mdl.f`'s verdicts -- the `xtd` and `xe` families. Both
harnesses emit through the port's
Fortran-format facility with the oracle's own FORMATs, so this gate diffs the
golden `.udg` lines directly rather than parsing numbers -- the same approach
`test_check_diagnostics.py` takes, and for the same reason: the formatting is
part of what is reproduced.

Three places where only a TEXT comparison catches the difference:
  * `easaic.f`'s 1020 has no space before its colon; `tdaic.f`'s 1020 has one.
  * `testalleaster` carries no `aictest.` prefix at all.
  * `aictest.xe.window` is written through TWO different FORMATs -- `(a,i3)`
    when the Easter is accepted, a literal string when it is rejected. The
    corpus covers both arms (`-aictest` accepts, `-aicdiff` rejects); with only
    one of them the reject-side spacing would never have been checked.

Both x11aic.f verdicts are covered on BOTH arms, each reject arm reached by an
`aicdiff=` tuned past the AICC gap the accept-arm spec measures. And
`-aictest-tdeas` runs the two tests together, which is the only configuration
that shows their coupling: with trading day accepted, `estend` goes false and
the Easter loop's first candidate reuses the trading-day AICC instead of
re-estimating, so `aictest.xe.aicc.noeaster` has to equal `aictest.xtd.aicc.td`
to the last digit.

The AICC tables appear ONLY on the explicit-aictest path: every
tdaic/easaic/lomaic call outside `arima.f` passes `Lsumm = 0`, which is why an
automdl or pickmdl golden carries `aictest.td` but never `aictest.td.num`.

Every one of these keys was previously UNCOMPARED. The `.udg` goldens have
carried them all along, the engine emitted none, and no gate looked at the
intersection -- the missing-key blind spot. Adding the emitter without this
file would have been a check that cannot fail; it found a real defect on its
first run (see the automx.f:308 note in core/src/automdl/automx.cpp), and the
`xe` half then found a second one: `x11regression{aicdiff=}` was being parsed
and discarded, so the reject arm was unreachable from the corpus. The `xtd`
half then found a third: `x11regression{aictest=(td)}` was itself being parsed
and dropped, so a spec asking for the test ran the plain fixed-TD path and
returned `OUTCOME: OK`.

WHAT THIS GATE OWNS is spelled out below rather than left implicit. An unowned
key is listed with the routine that writes it, and a key matching NEITHER list
fails the run -- so a new oracle key cannot slip in unclassified and the set
cannot quietly become an allowlist for a regression.
"""
import os
import pathlib
import re
import subprocess

import pytest

ROOT = pathlib.Path(__file__).resolve().parents[2]
CORPUS = ROOT / "tests" / "corpus"
GOLDEN = ROOT / "tests" / "golden"
BIN = ROOT / "build" / ("x13run_m3.exe" if os.name == "nt" else "x13run_m3")
BIN_X11 = ROOT / "build" / ("x13run_x11.exe" if os.name == "nt" else "x13run_x11")

# The keys this block writes, as regexes over the whole line's key. Three
# routines' worth: svaict.f's verdicts, and the per-candidate AICC tables that
# tdaic.f / easaic.f / lomaic.f write from inside the tests themselves. The
# length-of-month family names its keys from the stem it chose, hence _LN.
_LN = r"(?:lom|loq|lpyear)"
OWNED = re.compile(
    # -- svaict.f
    r"^aictest\.(?:"
    r"td|diff\.td|cvaic\.td"
    r"|" + _LN + r"|" + _LN + r"\.reg|diff\." + _LN + r"|cvaic\." + _LN +
    r"|easter\.reg|e|e\.window|diff\.e|cvaic\.e"
    r"|u|diff\.u|cvaic\.u"
    # -- tdaic.f's own table. The candidate NAME is part of the key, and a
    # stock-trading-day candidate carries its day-of-month in brackets
    # (`aictest.td.aicc.tdstock[31]`, mktdlb.f:36) -- so the suffix is not \w+.
    r"|td\.num|td\.reg|td\.reg2|td\.aicc\.[\w\[\]]+"
    # -- easaic.f's
    r"|easter\.num|e\.aicc\.[\w\[\]]+"
    # -- lomaic.f's
    r"|" + _LN + r"\.aicc\.[\w\[\]]+"
    r")$"
    # easaic.f:69-73 writes this one WITHOUT the prefix, and it is part of the
    # same block -- an unprefixed key is exactly the kind that goes unnoticed.
    r"|^testalleaster$")

# The x11regression AIC tests (x11aic.f's tables + x11mdl.f's verdicts): the
# TRADING-DAY family `xtd` and the EASTER family `xe`. Owned too, but by a
# DIFFERENT harness: they run on the X-11 path, which `x13run_m3` never
# reaches, so they get their own parametrisation below.
OWNED_X11 = re.compile(r"^aictest\.(?:"
                       r"xe(?:\.aicc\.\w+|\.window)?"
                       r"|xtd(?:\.aicc\.\w+|\.reg)?"
                       r"|xu(?:\.aicc\.\w+)?"
                       r")$")

# Everything else sharing the prefix, and the routine that writes it. NOT
# ported; each is separate work.
#
#   aictest.trans.aicc.{log,nolog}  trnaic.f  -- ported, emitted by run_m2 and
#                                   gated elsewhere, not part of this block
#   aictest.pv                      arima.f:463, svaict's CALLER, not svaict;
#                                   needs regression{pvaictest=}, which no
#                                   corpus spec sets, so it has no golden
#
# `aictest.xu*` (x11aic.f:462-591, the USER-defined test) MOVED from this table
# to OWNED_X11 when the branch was ported. Note what caught the omission: the
# engine started emitting the keys while the golden filter still classified
# them as unowned, so the key-set comparison failed in the extra-in-engine
# direction. That is the both-directions assert earning its keep.
UNOWNED = re.compile(r"^aictest\.(?:trans\.|pv$)")


def _golden_keys(udg: pathlib.Path):
    """The aictest lines of a golden .udg, key -> the raw text after the colon.

    Split by HARNESS, not just by ownership: the svaict/AICC keys come out of
    `x13run_m3` and the x11regression families out of `x13run_x11`."""
    owned, owned_x11, unowned = {}, {}, []
    for line in udg.read_text(errors="replace").splitlines():
        if not (line.startswith("aictest.") or line.startswith("testalleaster")):
            continue
        key = line.split(":", 1)[0]
        if UNOWNED.match(key):
            unowned.append(key)
        elif OWNED_X11.match(key):
            owned_x11[key] = line.split(":", 1)[1].rstrip()
        elif OWNED.match(key):
            owned[key] = line.split(":", 1)[1].rstrip()
        else:
            # A key matching neither list is the dangerous case: a new oracle
            # key nobody has classified. Fail loudly rather than skip it.
            pytest.fail(f"{udg.name}: unclassified aictest key {key!r} -- add it "
                        "to OWNED or to the UNOWNED table with its routine")
    return owned, owned_x11, unowned


def _cases(which):
    out = []
    for gdir in sorted(GOLDEN.rglob("*")):
        if not gdir.is_dir():
            continue
        udg = gdir / f"{gdir.name}.udg"
        if not udg.exists():
            continue
        golden = _golden_keys(udg)[which]
        if not golden:
            continue
        spec = next(iter(CORPUS.rglob(f"{gdir.name}.spc")), None)
        if spec is not None:
            out.append((gdir.name, spec, golden))
    return out


CASES = _cases(0)
CASES_X11 = _cases(1)


def _compare(binary, base, spec, golden):
    assert binary.exists(), f"missing harness {binary}"
    proc = subprocess.run([str(binary), spec.name], cwd=str(spec.parent),
                          capture_output=True, text=True, timeout=300)
    assert "OUTCOME: FATAL" not in proc.stdout, (
        f"{base}: harness fatal\n{proc.stdout[:800]}\n{proc.stderr[-800:]}")

    got = {}
    for line in proc.stdout.splitlines():
        if line.startswith("aictest.") or line.startswith("testalleaster"):
            k, v = line.split(":", 1)
            got[k] = v.rstrip()

    # Both directions. A key the oracle writes and the engine does not is the
    # defect this whole file exists to make visible; the reverse would mean the
    # engine emits a line on a run where the oracle stays silent.
    assert sorted(got) == sorted(golden), (
        f"{base}: aictest key sets differ\n"
        f"  missing from engine: {sorted(set(golden) - set(got))}\n"
        f"  extra in engine:     {sorted(set(got) - set(golden))}")

    for k in sorted(golden):
        assert got[k] == golden[k], (
            f"{base}: {k}\n  golden {golden[k]!r}\n  engine {got[k]!r}")


@pytest.mark.parametrize("base,spec,golden", CASES, ids=[c[0] for c in CASES])
def test_aictest_savelog(base, spec, golden):
    _compare(BIN, base, spec, golden)


@pytest.mark.parametrize("base,spec,golden", CASES_X11,
                         ids=[c[0] for c in CASES_X11])
def test_aictest_x11_savelog(base, spec, golden):
    """x11aic.f's two AICC tables + x11mdl.f's verdicts, off the X-11 harness.

    Split from the test above only because `x13run_m3` stops before X-11 --
    the comparison is identical, and both directions of the key set are
    checked, so an engine that stopped emitting these would fail here rather
    than quietly drop to zero compared keys."""
    _compare(BIN_X11, base, spec, golden)


def test_at_least_one_case():
    """The corpus must actually reach this block -- an empty parametrisation
    passes silently and would hide the emitter being dead."""
    assert len(CASES) >= 15, f"only {len(CASES)} aictest specs discovered"
    assert CASES_X11, "no x11regression aictest spec discovered"
