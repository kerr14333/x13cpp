"""QS seasonality gate: the `.udg` savelog block genqs.f writes (genqs.f:439-517).

WHAT THIS COVERS, and why it is a wrong-behaviour fix rather than a new table:

``x11ari.f:277-281`` runs ``genqs`` whenever the LSPCQS table is wanted, and
``gtinpt.f:122`` makes the oracle's ``-s`` flag (``Lsumm>0``) copy ``sumtab``
into ``Savtab`` -- whose entry 113 (LSPCQS) is TRUE. So every golden in this
corpus was blessed with the block present and this port emitted none of it: 327
of the 331 ``.udg`` goldens carry a ``qs*`` key.

Note what is NOT in front of it. The spectrum block sits behind a plain
``IF(Ny.eq.12)``; genqs does not, and editor.f:855-863's monthly-only ``Savtab``
clear covers LSPCS0..LSPS0C (93..102) and LSPCTP/LSPCQC (115/116) but NOT
LSPCQS. **Quarterly specs are in scope here**, unlike the spectrum peak gate.

The keys, in genqs.f's own emit order:

  * ``qslog``                        -- yes/no, whether any series was logged
  * ``qsori`` / ``qsorievadj``       -- the original, and the original adjusted
    for extreme values (the same Stcsi + pseudo-additive-rebuild-or-Stex-fold
    construction spcdrv.f:161-176 builds for sp0)
  * ``qsrsd``                        -- the regARIMA residuals (arima.f:1107,
    NOT genqs -- it needs the estimation-time span)
  * ``qssadj`` / ``qssadjevadj``     -- the SA series, and its EV twin with the
    level shift divided back out (the same unconditional ``Adjls`` Facls divide
    as spcdrv.f:318)
  * ``qsirr`` / ``qsirrevadj``       -- the irregular and its EV twin, which go
    STRAIGHT to calcqs: no differencing, no mean deletion, no log
  * the ``qss*`` twins               -- the same seven over the span starting at
    ``Bgspec`` (eight years back from the series end), emitted only when that
    start is actually later than the series start

Each row is ``(statistic, chi-square p-value on 2 df)``.

ZERO NEW GOLDENS.

TOLERANCE: byte-exact against the golden TEXT on 155 of the 157 compared specs.
The engine writes each line through the port's own ``fwrite_fmt`` with genqs.f's
own formats -- ``1030 FORMAT(a,':',f16.5,1x,f10.5)``, which has NO space after
the colon, and ``1040 FORMAT(a,': ',a)``.

The two exceptions are ``qssrsd`` on the two ``airline_automdl-x11`` specs,
which fall back to ``max(5e-4 abs, 5e-5 rel)`` -- the same measured bound
``test_check_diagnostics`` uses, and for the same reason. Measured: 2.30823 vs
2.30858 (1.5e-4 relative) with p 0.31534 vs 0.31528, every other key on those
specs byte-identical, and ``qsrsd`` over the full span identical too. The
statistic is ``nr(nr+2)*r(sp)^2/(nr-sp)`` on the AUTO-SELECTED model's
residuals, so an r that agrees to ~1e-5 (which is what a 1e-6 coefficient
agreement buys) lands here. The fallback is restricted to the two residual keys
so a drift in any X-11 statistic still fails loudly.

SEATS specs run through ``x13run_seats`` rather than ``x13run_x11`` -- the two
harnesses share the emit (``tools/dump_diag.hpp``) because ``x11ari.f`` reaches
genqs (:277) and gennpsa (:322) only after its Lseats/Lx11 branch rejoins. What
the SEATS arms read is described in genqs.cpp's header; the short version is
that ``Seatsa``/``Stocsa`` and ``Seatir``/``Stocir`` are the SAME sigex arrays
and only ``seatad.f`` separates them, so ``qsirr`` and ``qsirrevadj`` are
provably identical on that path in both modes.

Note what the SEATS goldens can and cannot discriminate. Every one of them
reports ``qssadj``/``qsirr`` as ``0.00000`` except ``payems_seats``, and
``payems_seats`` has ``npsi==1`` (no seasonal component, so ``sa == z`` and
``qssadj`` is trivially ``qsori``). The one genuinely non-degenerate SEATS value
in the corpus is its ``qsirr`` 0.01133 / ``qssirr`` 0.00498. The real coverage
these specs add is the ``qsori``/``qsorievadj``/``qsrsd`` block -- nonzero and
discriminating (167.64858 on the airline family) -- which the whole-spec skip
was throwing away along with the rest.

NOT COVERED YET, deliberately and visibly:

  * The ``Iagr==4`` indirect names (``qsindsadj`` etc.), which belong with the
    composite front. Pinned by ``test_indirect_keys_are_not_claimed``.

Run:  python -m pytest tests/parity/test_qs_diagnostics.py -q
"""
from __future__ import annotations

import functools
import os
import re
import subprocess

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus")
_GOLDEN = os.path.join(_REPO, "tests", "golden")

_TREES = ["generated", "extra", "census-examples", "ces"]

# Every key genqs.f can write on the DIRECT (Iagr<4) path. Spelled out rather
# than matched by prefix so that the indirect names (`qsindsadj`, `qssindirr`,
# ...) stay visibly unclaimed until the composite increment ports them.
_KEYS = {
    "qslog",
    "qsori", "qsorievadj", "qsrsd", "qssadj", "qssadjevadj",
    "qsirr", "qsirrevadj",
    "qssori", "qssorievadj", "qssrsd", "qsssadj", "qsssadjevadj",
    "qssirr", "qssirrevadj",
    # gennpsa.f's NP residual-seasonality verdicts (yes/no, not statistics).
    # Only the SA series and its EV twin are tested, so this block is absent
    # from any run that produces no adjustment -- 101 goldens carry no `np*`
    # key against 4 with no `qs*` key.
    "nplog", "npsadj", "npsadjevadj", "npssadj", "npssadjevadj",
}

_KEY_RE = re.compile(r"^([A-Za-z][A-Za-z0-9._$]*):(.*)$")

# Specs whose disagreement is upstream of this gate skip with the measurement
# written at the skip rather than being filtered out of discovery.
_AUTOMD_IDDIFF_GAP = {"generated/usdeaths_automdl"}

# The ONLY numeric fallback, and it is deliberately narrow -- see the tolerance
# note in the module docstring.
_RESID_KEYS = {"qsrsd", "qssrsd"}
_ATOL, _RTOL = 5e-4, 5e-5


def _close(key: str, want: str, got: str) -> bool:
    if key not in _RESID_KEYS:
        return False
    w, g = want.split(), got.split()
    if len(w) != len(g):
        return False
    try:
        pairs = [(float(a), float(b)) for a, b in zip(w, g)]
    except ValueError:
        return False
    return all(abs(a - b) <= max(_ATOL, _RTOL * abs(a)) for a, b in pairs)


def _find_binary(stem: str) -> str:
    for c in (os.path.join(_REPO, "build", stem + ".exe"),
              os.path.join(_REPO, "build", stem),
              os.path.join(_REPO, "build", "Release", stem + ".exe")):
        if os.path.exists(c):
            return c
    env = os.environ.get(stem.upper())
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        f"{stem} binary not found; build it first (cmake --build build).")


BIN = _find_binary("x13run_x11")
BIN_SEATS = _find_binary("x13run_seats")


def _read_block(lines) -> dict[str, str]:
    out: dict[str, str] = {}
    for ln in lines:
        m = _KEY_RE.match(ln.rstrip("\n").lstrip())
        if m and m.group(1) in _KEYS:
            out[m.group(1)] = m.group(2).rstrip()
    return out


def _discover() -> list[str]:
    """Corpus-relative spec ids whose golden `.udg` carries the QS block."""
    out: list[str] = []
    for tree in _TREES:
        root = os.path.join(_CORPUS, tree)
        if not os.path.isdir(root):
            continue
        for dirpath, _dirs, files in os.walk(root):
            # A metafile means these specs are composite COMPONENTS: the oracle
            # ran them in one process with the aggregation COMMONs live, so a
            # standalone run is a different run.
            if any(f.endswith(".mta") for f in files):
                continue
            for fn in sorted(files):
                if not fn.endswith(".spc"):
                    continue
                base = fn[:-4]
                rel = os.path.relpath(os.path.join(dirpath, base), _CORPUS)
                udg = os.path.join(_GOLDEN, rel, base + ".udg")
                if not os.path.exists(udg):
                    continue
                with open(udg, encoding="utf-8", errors="replace") as fh:
                    if "\nqsori:" not in fh.read():
                        continue
                out.append(rel.replace("\\", "/"))
    return sorted(out)


CASES = _discover()


@functools.lru_cache(maxsize=None)
def _run(rel: str) -> dict[str, str]:
    spec = os.path.join(_CORPUS, rel + ".spc")
    txt = open(spec, encoding="utf-8", errors="replace").read().lower()
    # seats{} and x11{} are mutually exclusive, so the spec text picks the
    # harness. Both emit the identical two blocks (tools/dump_diag.hpp).
    binary = BIN_SEATS if "seats{" in txt else BIN
    if "pickmdl{" in txt:
        pytest.skip("pickmdl{} model selection is parse-only (M1); the model "
                    "the engine fits is not the oracle's")
    if rel in _AUTOMD_IDDIFF_GAP:
        pytest.skip(
            "automd selects (1 0 1)(0 1 1) here against the oracle's "
            "(0 1 1)(0 1 1) -- an iddiff regular-differencing (d=0 vs d=1) "
            "discrepancy only NSA data reaches, already known at "
            "test_m4_iddiff's _AUTOMD_EST_CASES omission and in "
            "tools/automdl_scouting.md. Visible here as the residual span "
            "starting a period earlier, so `qssrsd` is emitted where the "
            "oracle emits none.")
    r = subprocess.run([binary, spec], capture_output=True, text=True,
                       cwd=os.path.dirname(spec))
    assert r.returncode == 0, f"{rel}: harness exit {r.returncode}\n{r.stderr}"
    first = r.stdout.splitlines()[0].strip() if r.stdout else ""
    if first != "OUTCOME: OK":
        pytest.skip(f"engine declined this spec ({first or 'no output'})")
    return _read_block(r.stdout.splitlines())


def test_every_owned_key_is_readable() -> None:
    """_KEY_RE must MATCH every key this gate claims to own.

    A too-narrow key pattern drops a key silently -- the golden side and the
    engine side both lose it, the comparison still passes, and the gate looks
    green while covering nothing. That has happened once already on
    test_check_diagnostics (``aape.0``..``aape.3``), so it is asserted here.
    """
    unreadable = [k for k in sorted(_KEYS)
                  if not (_KEY_RE.match(f"{k}:  1.0  2.0")
                          and _KEY_RE.match(f"{k}:  1.0  2.0").group(1) in _KEYS)]
    assert not unreadable, f"keys this gate owns but cannot parse: {unreadable}"


def test_indirect_keys_are_not_claimed() -> None:
    """The Iagr==4 names belong to the unported composite/indirect path.

    They must be excluded from BOTH sides, or a composite total's indirect rows
    would read as `missing` on every such spec. Pinned so that porting them has
    to come here and do it deliberately.
    """
    for k in ("qsindsadj", "qsindsadjevadj", "qsindirr", "qsindirrevadj",
              "qssindsadj", "qssindirr",
              "npindsadj", "npindsadjevadj", "npsindsadj", "npsindsadjevadj"):
        assert k not in _KEYS, f"{k} is claimed but the indirect path is unported"


@pytest.mark.skipif(not CASES, reason="no corpus golden ships the QS block")
@pytest.mark.parametrize("rel", CASES)
def test_qs_block(rel: str) -> None:
    """Every QS savelog line matches the golden EXACTLY."""
    udg = os.path.join(_GOLDEN, rel, os.path.basename(rel) + ".udg")
    with open(udg, encoding="utf-8", errors="replace") as fh:
        want = _read_block(fh)
    assert want, f"{rel}: golden has no QS block (discovery is wrong)"
    got = _run(rel)

    # Both directions. A missing key is an unported statistic; an EXTRA key is
    # the engine reporting one the oracle suppressed -- e.g. a `qss*` row on a
    # series short enough that Bgspec lands on the series start, or a `qsirr`
    # on an x11{type=} run where Kfulsm is not 0.
    missing = sorted(set(want) - set(got))
    extra = sorted(set(got) - set(want))
    assert not missing, f"{rel}: keys missing from the engine: {missing}"
    assert not extra, f"{rel}: keys the oracle does not emit: {extra}"

    bad = [(k, want[k], got[k]) for k in sorted(want)
           if want[k] != got[k] and not _close(k, want[k], got[k])]
    assert not bad, "\n".join(
        f"{rel}.{k}: golden {w!r} != engine {g!r}" for k, w, g in bad)
