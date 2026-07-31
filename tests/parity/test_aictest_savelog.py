"""svaict.f savelog block -- the `aictest.*` keys, compared as TEXT.

svaict.f reports which AIC-tested regressor group the final model kept, the
AICC difference that decided it, and the threshold when a non-default one was
in force. `x13run_m3` emits it through the port's Fortran-format facility with
the oracle's own FORMATs (1010/1020/1025/1040), so this gate diffs the golden
`.udg` lines directly rather than parsing numbers -- the same approach
`test_check_diagnostics.py` takes, and for the same reason: the formatting is
part of what is being reproduced.

Every one of these keys was previously UNCOMPARED. The `.udg` goldens have
carried them all along, the engine emitted none, and no gate looked at the
intersection -- the missing-key blind spot. Adding the emitter without this
file would have been a check that cannot fail; it found a real defect on its
first run (see the automx.f:308 note in core/src/automdl/automx.cpp).

WHAT THIS GATE OWNS is spelled out below rather than left implicit, because
the `aictest.` prefix is shared by four different Fortran routines and only
one of them is ported. An unowned key is listed with the routine that writes
it, so the set cannot quietly become an allowlist for a regression.
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

# The keys svaict.f writes, as regexes over the part after `aictest.`. The
# length-of-month family names its own keys from the stem (lom / loq / lpyear),
# hence the alternation.
_LN = r"(?:lom|loq|lpyear)"
OWNED = re.compile(
    r"^aictest\.(?:"
    r"td|diff\.td|cvaic\.td"
    r"|" + _LN + r"|" + _LN + r"\.reg|diff\." + _LN + r"|cvaic\." + _LN +
    r"|easter\.reg|e|e\.window|diff\.e|cvaic\.e"
    r"|u|diff\.u|cvaic\.u"
    r")$")

# Everything else sharing the prefix, and the routine that writes it. These are
# NOT svaict's and are NOT ported; each is a separate piece of work.
#
#   aictest.trans.aicc.{log,nolog}  trnaic.f  -- ported, but emitted by run_m2
#                                   and gated elsewhere, not part of this block
#   aictest.td.{num,reg,reg2}       tdaic.f   -- the per-candidate TD table
#   aictest.td.aicc.*               tdaic.f
#   aictest.easter.num              easaic.f
#   aictest.e.aicc.*                easaic.f
#   aictest.lom.aicc.*              lomaic.f
#   aictest.xe*                     x11aic.f  -- the x11regression Easter test
#   aictest.pv                      arima.f:463, its CALLER, not svaict; needs
#                                   regression{pvaictest=}, which no spec sets
UNOWNED = re.compile(
    r"^aictest\.(?:trans\.|td\.(?:num|reg|reg2|aicc\.)|easter\.num"
    r"|e\.aicc\.|" + _LN + r"\.aicc\.|xe|pv$)")


def _golden_keys(udg: pathlib.Path):
    """The svaict lines of a golden .udg, key -> the raw text after the colon."""
    owned, unowned = {}, []
    for line in udg.read_text(errors="replace").splitlines():
        if not line.startswith("aictest."):
            continue
        key = line.split(":", 1)[0]
        if UNOWNED.match(key):
            unowned.append(key)
        elif OWNED.match(key):
            owned[key] = line.split(":", 1)[1].rstrip()
        else:
            # A key matching neither list is the dangerous case: a new oracle
            # key nobody has classified. Fail loudly rather than skip it.
            pytest.fail(f"{udg.name}: unclassified aictest key {key!r} -- add it "
                        "to OWNED or to the UNOWNED table with its routine")
    return owned, unowned


def _cases():
    out = []
    for gdir in sorted(GOLDEN.rglob("*")):
        if not gdir.is_dir():
            continue
        udg = gdir / f"{gdir.name}.udg"
        if not udg.exists():
            continue
        owned, _ = _golden_keys(udg)
        if not owned:
            continue
        spec = next(iter(CORPUS.rglob(f"{gdir.name}.spc")), None)
        if spec is not None:
            out.append((gdir.name, spec, owned))
    return out


CASES = _cases()


@pytest.mark.parametrize("base,spec,golden", CASES, ids=[c[0] for c in CASES])
def test_aictest_savelog(base, spec, golden):
    assert BIN.exists(), f"missing harness {BIN}"
    proc = subprocess.run([str(BIN), spec.name], cwd=str(spec.parent),
                          capture_output=True, text=True, timeout=300)
    assert "OUTCOME: FATAL" not in proc.stdout, (
        f"{base}: harness fatal\n{proc.stdout[:800]}\n{proc.stderr[-800:]}")

    got = {}
    for line in proc.stdout.splitlines():
        if line.startswith("aictest."):
            k, v = line.split(":", 1)
            got[k] = v.rstrip()

    # Both directions. A key the oracle writes and the engine does not is the
    # defect this whole file exists to make visible; the reverse would mean the
    # engine emits a line on a run where the oracle stays silent.
    assert sorted(got) == sorted(golden), (
        f"{base}: svaict key sets differ\n"
        f"  missing from engine: {sorted(set(golden) - set(got))}\n"
        f"  extra in engine:     {sorted(set(got) - set(golden))}")

    for k in sorted(golden):
        assert got[k] == golden[k], (
            f"{base}: {k}\n  golden {golden[k]!r}\n  engine {got[k]!r}")


def test_at_least_one_case():
    """The corpus must actually reach this block -- an empty parametrisation
    passes silently and would hide the emitter being dead."""
    assert len(CASES) >= 8, f"only {len(CASES)} aictest specs discovered"
