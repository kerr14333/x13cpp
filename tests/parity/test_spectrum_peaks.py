"""Spectrum PEAK gate: the `.udg` savelog block spcdrv.f produces on every
monthly run (svfreq.f / svpeak.f / smpeak.f / mxpeak.f / ispeak.f / savpk.f).

WHAT THIS COVERS, and why it is a wrong-behaviour fix rather than a new table:

``x11ari.f:282-287`` calls ``spcdrv`` under a plain ``IF(Ny.eq.12)`` -- there is
no dependence on the ``spectrum{}`` spec at all -- so the oracle computes the
whole spectrum diagnostic block on EVERY monthly run. This port gated
``run_spectrum`` on ``ctx.spcout.requested``, which only a ``spectrum{}`` block
sets, and it had none of the peak arithmetic. 278 goldens carry
``spcori.median`` and the engine emitted nothing for any of them.

The keys:

  * ``nspecfreq`` / ``ntdfreq`` / ``nsfreq``      -- svfreq.f's grid inventory
  * ``tN.freq`` / ``tN.index`` / ``sN.freq`` / ``sN.index``
  * ``<prefix>.median`` / ``.range``             -- svpeak.f, off the SORTED
    61-point spectrum (not the enhanced peak grid)
  * ``<prefix>.t1``..``.t2`` / ``.s1``..``.s5``  -- smpeak.f's star heights,
    the literal string ``nopeak`` where the peak sits at or below its base, and
    a trailing ``+`` where it clears the median
  * ``<prefix>.t.dom`` / ``.s.dom`` / ``.dom``   -- smpeak.f + mxpeak.f
  * ``peaks.seas`` / ``peaks.td``                -- savpk.f's accumulated label
    lists, driven by ispeak.f's COUNT (a stricter test than the star height,
    so it is not derivable from the rows above)

``<prefix>`` is mkspky.f's: ``spcori`` / ``spcsa`` / ``spcirr`` / ``spcrsd``.

ZERO NEW GOLDENS. Every key above already ships in the corpus `.udg` goldens --
they were blessed with the oracle's ``-s`` flag, which is what sets ``Lsumm>0``
and hence ``Lsavpk``.

TOLERANCE: byte-exact against the golden TEXT on 138 of the 140 compared specs.
The engine writes each line through the port's own ``fwrite_fmt`` with the
Fortran format that produced it (svfreq 1000/1010/1020, svpeak 1010, smpeak
1010/1020, mxpeak 1010), so this is a stronger contract than a numeric bound and
the right one -- the printed line IS the oracle's published diagnostic.

The exceptions are ``spcrsd.median`` and ``spcrsd.range`` on three
AUTO-SELECTED-model specs, which fall back to 5e-5 relative. Measured, not
assumed: on the two ``airline_automdl-x11`` specs median -30.74540986 vs
-30.74547793 (2.2e-7 relative) and range 15.44528266 vs 15.44540075 (7.6e-6);
on ``co2_automdl`` range 12.78893717 vs 12.78920580 (2.1e-5). Every star height
and every dominant-frequency label on all three stays byte-identical. Both keys
are printed E20.10 -- ten significant digits, finer than an auto-selected
model's residuals agree -- and the fallback is restricted to those two key
suffixes so a wrong label or a wrong peak count can never be absorbed by it.

NOT COVERED YET, deliberately and visibly (these skip with a reason rather than
being filtered out of discovery):

  * ``.tukey.*`` and ``peaks.tukey.*`` -- getTPeaks (specpeak.f), a separate
    ~850-line front. The Tukey SPECTRA are already ported; only the peak
    probabilities are missing.
  * SEATS specs (51 of the 278). ``run_seats`` does not call ``run_spectrum``
    at all, and spcdrv's SEATS branch reads ``Hvstsa``/``Hvstir`` (the SEATS SA
    and irregular) where the X-11 branch reads Stcime/Stime -- a different
    input, not just a different driver. Next increment.

Run:  python -m pytest tests/parity/test_spectrum_peaks.py -q
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

# The scalars this gate owns.
_SCALARS = {"nspecfreq", "ntdfreq", "nsfreq", "peaks.seas", "peaks.td"}
# svfreq.f's per-frequency rows, and svpeak/smpeak/mxpeak's per-table rows.
_FREQ_RE = re.compile(r"^[st][0-9]\.(freq|index|index\.lower|index\.upper)$")
_PEAK_RE = re.compile(
    r"^spc(ori|sa|irr|rsd|extrsd|comp|indsa|indirr)\."
    r"(median|range|dom|[st]\.dom|[st][0-9])$")

_KEY_RE = re.compile(r"^([A-Za-z][A-Za-z0-9._$]*):(.*)$")


def _is_peak_key(key: str) -> bool:
    if "tukey" in key:
        return False       # getTPeaks -- see the module docstring
    return (key in _SCALARS or bool(_FREQ_RE.match(key))
            or bool(_PEAK_RE.match(key)))


# The ONLY numeric fallback, and it is deliberately narrow -- see the tolerance
# note in the module docstring. `median` and `range` are printed E20.10, i.e. ten
# significant digits, which is finer than an auto-selected model's residuals
# agree; every other key (the star heights at f6.1, the labels, the indices) is
# compared byte-exact and stays so.
_RTOL = 5e-5

# Spec families whose disagreement is upstream of this gate skip with the
# measurement written at the skip rather than being filtered out of discovery.
_AUTOMD_IDDIFF_GAP = {"generated/usdeaths_automdl"}


def _close(key: str, want: str, got: str) -> bool:
    if not (key.endswith(".median") or key.endswith(".range")):
        return False
    try:
        w, g = float(want), float(got)
    except ValueError:
        return False
    return abs(w - g) <= _RTOL * abs(w)


def _find_binary() -> str:
    for c in (os.path.join(_REPO, "build", "x13run_x11.exe"),
              os.path.join(_REPO, "build", "x13run_x11"),
              os.path.join(_REPO, "build", "Release", "x13run_x11.exe")):
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_X11")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13run_x11 binary not found; build it first (cmake --build build).")


BIN = _find_binary()


def _read_block(lines) -> dict[str, str]:
    out: dict[str, str] = {}
    for ln in lines:
        m = _KEY_RE.match(ln.rstrip("\n").lstrip())
        if not m:
            continue
        if _is_peak_key(m.group(1)):
            out[m.group(1)] = m.group(2).rstrip()
    return out


def _discover() -> list[str]:
    """Corpus-relative spec ids whose golden `.udg` carries the peak block."""
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
                    if "\nspcori.median:" not in fh.read():
                        continue
                out.append(rel.replace("\\", "/"))
    return sorted(out)


CASES = _discover()


@functools.lru_cache(maxsize=None)
def _run(rel: str) -> dict[str, str]:
    spec = os.path.join(_CORPUS, rel + ".spc")
    txt = open(spec, encoding="utf-8", errors="replace").read().lower()
    if "seats{" in txt:
        pytest.skip("run_seats does not call run_spectrum, and spcdrv's SEATS "
                    "branch reads Hvstsa/Hvstir rather than Stcime/Stime -- "
                    "a different input, not just a different driver. "
                    "Next increment; see the module docstring.")
    if "pickmdl{" in txt:
        pytest.skip("pickmdl{} model selection is parse-only (M1); the model "
                    "the engine fits is not the oracle's")
    if rel in _AUTOMD_IDDIFF_GAP:
        pytest.skip(
            "automd selects (1 0 1)(0 1 1) here against the oracle's "
            "(0 1 1)(0 1 1) -- the iddiff d=0/d=1 discrepancy only NSA data "
            "reaches, already known at test_m4_iddiff's _AUTOMD_EST_CASES "
            "omission and in tools/automdl_scouting.md. Different residuals, "
            "hence spcrsd.median 47.27871166 vs 47.03301580 (5.2e-3).")
    r = subprocess.run([BIN, spec], capture_output=True, text=True,
                       cwd=os.path.dirname(spec))
    assert r.returncode == 0, f"{rel}: harness exit {r.returncode}\n{r.stderr}"
    first = r.stdout.splitlines()[0].strip() if r.stdout else ""
    if first != "OUTCOME: OK":
        pytest.skip(f"engine declined this spec ({first or 'no output'})")
    return _read_block(r.stdout.splitlines())


def test_every_owned_key_is_readable() -> None:
    """_KEY_RE + _is_peak_key must MATCH every key this gate claims to own.

    A too-narrow key pattern drops a key silently -- the golden side and the
    engine side both lose it, the comparison still passes, and the gate looks
    green while covering nothing. That has happened once already on
    test_check_diagnostics (``aape.0``..``aape.3``), so it is asserted here.
    """
    samples = [f"{k}: x" for k in sorted(_SCALARS)]
    samples += ["t1.freq: 0.34820000", "t2.index:    52",
                "s5.freq: 0.41666667", "s1.index:    10",
                "s1.index.lower:    10", "s1.index.upper:    12",
                "spcori.median:    -0.2778718458E+02",
                "spcori.range:     0.3129385606E+02",
                "spcsa.t1:   13.9 +", "spcirr.s3: nopeak",
                "spcrsd.t.dom: t2", "spcori.s.dom: s1", "spcori.dom: s1"]
    unreadable = []
    for line in samples:
        m = _KEY_RE.match(line)
        if not m or not _is_peak_key(m.group(1)):
            unreadable.append(line.split(":")[0])
    assert not unreadable, f"keys this gate owns but cannot parse: {unreadable}"


def test_tukey_keys_are_not_claimed() -> None:
    """The `.tukey.*` families belong to the unported getTPeaks front.

    They must be excluded from BOTH sides, or the golden's tukey keys would
    show up as `missing` on every spec. Pinned so that porting getTPeaks has
    to come here and delete this deliberately.
    """
    for k in ("spcori.tukey.s1", "spcrsd.tukey.m", "peaks.tukey.seas",
              "peaks.tukey.p90.td"):
        assert not _is_peak_key(k), f"{k} is claimed but getTPeaks is unported"


@pytest.mark.skipif(not CASES, reason="no corpus golden ships the peak block")
@pytest.mark.parametrize("rel", CASES)
def test_spectrum_peak_block(rel: str) -> None:
    """Every spectrum peak savelog line matches the golden EXACTLY."""
    udg = os.path.join(_GOLDEN, rel, os.path.basename(rel) + ".udg")
    with open(udg, encoding="utf-8", errors="replace") as fh:
        want = _read_block(fh)
    assert want, f"{rel}: golden has no peak block (discovery is wrong)"
    got = _run(rel)

    # Both directions. A missing key is an unported statistic; an EXTRA key is
    # the engine emitting one the oracle suppressed -- e.g. a `t*` row where
    # spcdrv.f:153's Ltdfrq is false (a spectrum span of 60 observations or
    # fewer searches no trading-day frequency at all).
    missing = sorted(set(want) - set(got))
    extra = sorted(set(got) - set(want))
    assert not missing, f"{rel}: keys missing from the engine: {missing}"
    assert not extra, f"{rel}: keys the oracle does not emit: {extra}"

    bad = [(k, want[k], got[k]) for k in sorted(want)
           if want[k] != got[k] and not _close(k, want[k], got[k])]
    assert not bad, "\n".join(
        f"{rel}.{k}: golden {w!r} != engine {g!r}" for k, w, g in bad)
