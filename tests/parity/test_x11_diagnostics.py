"""M5 X-11 diagnostics gate: the F2/F3 savelog block vs the oracle .udg.

x11pt4.f's savelog block (svf2f3.f:59-64) reports five statistics that between
them decide whether X-11 thinks the series has identifiable seasonality:

  * ``f2.fsb1``       -- stable-seasonality F on the B1 SI ratios (ftest.f, Ind=2)
  * ``f2.fsd8``       -- stable-seasonality F on the D8 unmodified SI (Ind=0)
  * ``f2.kw``         -- Kruskal-Wallis, the nonparametric equivalent (kwtest.f)
  * ``f2.msf``        -- MOVING-seasonality F, years x seasons (mstest.f)
  * ``f2.idseasonal`` -- the combined verdict (combft.f)

Each ships a statistic and its probability level (except the verdict, a
yes/no). ``combft`` also writes /tests/ Test1,Test2, which are the inputs to M7
and hence to the F3 Q statistic -- so this gate is the floor the quality
statistics stand on.

The rest of the block is the Part-F summary measures and the F3 quality
statistics (x11pt4.f:320-713 -> sumry/vars/avedur + f3cal.f):

  * ``f2.a01``..``a12`` -- mean absolute span-k change of each component
  * ``f2.b01``..``b12`` -- those squared, as a share of the total
  * ``f2.c01``..``c12`` -- the signed means and their standard deviations
  * ``f2.d`` / ``f2.e`` / ``f2.mcd`` -- average durations of run, I/C ratios, MCD
  * ``f2.f``            -- relative contributions to the variance (Vi/Vc/Vs/Vp/Vtd/Rv)
  * ``f2.g``            -- autocorrelations of the irregular, lags 1..Ny+2
  * ``f2.ic`` / ``f2.is`` -- the global I/C and moving-seasonality ratios
  * ``f3.m01``..``m11``, ``f3.q``, ``f3.qm2``, ``f3.fail`` -- the M statistics,
    the composite Q, Q without M2, and the count of M's at or above 1

TOLERANCE. These are savelog canaries, not save tables, so each value is pinned
only to its own PRINTED precision: E15.8 for the a/c blocks, F8.2 for the
percentage and ratio lines (and the b block, which carries a 2P scale factor, so
half a printed digit is 5e-5 on the underlying fraction), f6.3 for the M
statistics, F5.2 for Q. That printed precision IS the tolerance here (same
policy as test_m3_estimate's ``_print_ulp``) -- there is no 15-digit golden for
these values to compare against.

Run:  python -m pytest tests/parity/test_x11_diagnostics.py -q
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

# Corpus subtrees this gate covers. `edge/` is parse-only fixtures (M1) and
# `census-examples/composite*/` needs the multi-spec driver, not x13run_x11.
_TREES = ("generated", "extra", "census-examples")

# Half the last printed digit, per svf2f3.f's F11.3 / F8.2 formats.
_ATOL_STAT = 5e-4
_ATOL_PROB = 5e-3

_KEYS = ("f2.fsb1", "f2.fsd8", "f2.kw", "f2.msf")


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


def _discover() -> list[str]:
    """Corpus-relative spec ids whose golden .udg carries the F2 battery."""
    out: list[str] = []
    for tree in _TREES:
        root = os.path.join(_CORPUS, tree)
        if not os.path.isdir(root):
            continue
        for dirpath, _dirs, files in os.walk(root):
            # A metafile in the directory means these specs are components of a
            # composite run: the oracle ran them in ONE process with the
            # aggregation COMMONs live, so a standalone run is a different run.
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
                    if "f2.fsd8:" not in fh.read():
                        continue
                out.append(rel.replace("\\", "/"))
    return sorted(out)


CASES = _discover()


def _read_udg(path: str) -> dict[str, list[str]]:
    """Every f2./f3. key in a golden .udg, split on whitespace.

    Whitespace splitting is correct for the E15.8 blocks and for every F-format
    line the corpus actually produces; ``_fixed`` below re-splits the F-format
    lines by column when a value is wide enough to touch its neighbour.
    """
    want: dict[str, list[str]] = {}
    pat = re.compile(r"((?:f2|f3)\.[a-z0-9]+):\s*(.*)$")
    with open(path, encoding="utf-8", errors="replace") as fh:
        for ln in fh:
            m = pat.match(ln.rstrip("\n").lstrip())
            if m:
                want[m.group(1)] = m.group(2).split()
    return want


def _read_udg_raw(path: str) -> dict[str, str]:
    """The same keys, but keeping the raw (column-significant) value text."""
    raw: dict[str, str] = {}
    pat = re.compile(r"((?:f2|f3)\.[a-z0-9]+):(.*)$")
    with open(path, encoding="utf-8", errors="replace") as fh:
        for ln in fh:
            m = pat.match(ln.rstrip("\n").lstrip())
            if m:
                raw[m.group(1)] = m.group(2)
    return raw


def _fixed(text: str, width: int, n: int) -> list[float | None]:
    """Split a Fortran fixed-width numeric field list.

    Returns None for a field the oracle overflowed into asterisks, so the
    caller can skip a column the golden simply does not pin.
    """
    out: list[float | None] = []
    for i in range(n):
        chunk = text[i * width:(i + 1) * width].strip()
        try:
            out.append(float(chunk))
        except ValueError:
            out.append(None)
    return out


@functools.lru_cache(maxsize=None)
def _run(rel: str) -> dict[str, list[str]]:
    spec = os.path.join(_CORPUS, rel + ".spc")
    txt = open(spec, encoding="utf-8", errors="replace").read().lower()
    if "pickmdl{" in txt:
        pytest.skip("pickmdl{} model selection is parse-only (M1); the model "
                    "the engine fits is not the oracle's")

    r = subprocess.run([BIN, spec], capture_output=True, text=True)
    assert r.returncode == 0, f"{rel}: harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:200]

    got: dict[str, list[str]] = {}
    for ln in r.stdout.splitlines():
        p = ln.split()
        if p and (p[0].startswith("f2.") or p[0].startswith("f3.")):
            got[p[0]] = p[1:]
    return got


@pytest.mark.skipif(not CASES, reason="no corpus golden ships the F2 test battery")
@pytest.mark.parametrize("rel", CASES)
def test_f2_seasonality_tests(rel: str) -> None:
    got = _run(rel)
    assert got, f"{rel}: harness emitted no f2.* canaries"
    want = _read_udg(os.path.join(_GOLDEN, rel, os.path.basename(rel) + ".udg"))

    for k in _KEYS:
        if k not in want:
            continue
        assert k in got, f"{rel}: {k} missing from harness output"
        stat_g, prob_g = float(got[k][0]), float(got[k][1])
        stat_w, prob_w = float(want[k][0]), float(want[k][1])
        assert abs(stat_g - stat_w) <= _ATOL_STAT, (
            f"{rel}: {k} statistic {stat_g} != {stat_w}")
        assert abs(prob_g - prob_w) <= _ATOL_PROB, (
            f"{rel}: {k} prob {prob_g} != {prob_w}")

    if "f2.idseasonal" in want:
        assert got.get("f2.idseasonal") == want["f2.idseasonal"], (
            f"{rel}: identifiable-seasonality verdict "
            f"{got.get('f2.idseasonal')} != {want['f2.idseasonal']}")


# --- Part-F summary measures + F3 quality statistics -------------------------

# E15.8 pins ~9 significant digits; the abs floor covers a measure that is
# legitimately ~0 (a component with no variation, e.g. Tdbar with no TD).
_RTOL_E15 = 1e-7
_ATOL_E15 = 1e-8
# Half the last printed digit of an F8.2 percentage / ratio.
_ATOL_F82 = 5e-3
# f2.b carries a 2P scale factor: the printed field is the value x 100, so the
# comparison is done in the SCALED space and half a printed digit is 5e-3 there.
_ATOL_2PF82 = 5e-3
# f3.mNN is f6.3, f3.q / f3.qm2 are F5.2.
_ATOL_M = 5e-4
_ATOL_Q = 5e-3


def _cmp_e15(rel: str, key: str, got: list[str], want: list[str]) -> None:
    assert len(got) == len(want), (
        f"{rel}: {key} has {len(got)} values, golden has {len(want)}")
    for i, (g, w) in enumerate(zip(got, want)):
        gv, wv = float(g), float(w)
        assert abs(gv - wv) <= max(_ATOL_E15, _RTOL_E15 * abs(wv)), (
            f"{rel}: {key}[{i}] {gv} != {wv}")


def _cmp_fixed(rel: str, key: str, got: list[str], raw: str, width: int,
               atol: float, scale: float = 1.0) -> None:
    ref = _fixed(raw, width, len(got))
    for i, (g, w) in enumerate(zip(got, ref)):
        if w is None:
            continue  # the oracle overflowed the field; nothing to compare to
        gv = float(g) * scale
        assert abs(gv - w) <= atol, f"{rel}: {key}[{i}] {gv} != {w}"


@pytest.mark.skipif(not CASES, reason="no corpus golden ships the F2 test battery")
@pytest.mark.parametrize("rel", CASES)
def test_f2_summary_measures(rel: str) -> None:
    """x11pt4.f's Part F: sumry / vars / avedur / MCD / the autocorrelations."""
    got = _run(rel)
    udg = os.path.join(_GOLDEN, rel, os.path.basename(rel) + ".udg")
    want = _read_udg(udg)
    raw = _read_udg_raw(udg)
    if "f2.a01" not in want:
        pytest.skip("golden carries no Part-F summary block")
    assert "f2.a01" in got, (
        f"{rel}: the oracle emitted the Part-F block but the engine did not "
        "(x11pt4_partf returned false -- a variance it could not compute)")

    # a / c: the E15.8 blocks, one line per span k = 1..Ny.
    for prefix in ("f2.a", "f2.c"):
        for i in range(1, 13):
            k = f"{prefix}{i:02d}"
            if k not in want:
                continue
            assert k in got, f"{rel}: {k} missing from harness output"
            _cmp_e15(rel, k, got[k], want[k])

    # b: 5 shares + the literal 100.00 column + Osq2, all under a 2P scale.
    for i in range(1, 13):
        k = f"f2.b{i:02d}"
        if k not in want:
            continue
        assert k in got and len(got[k]) == 6, f"{rel}: {k} malformed"
        # svf2f3.f:1020 puts a `1x` between the colon and the first field, so
        # the F8.2 columns start one character in (the a/c/d/e/f/g formats do
        # not have it).
        ref = _fixed(raw[k][1:], 8, 7)
        # svf2f3.f:1020 -- fields 1-5 are Isq/Csq/Ssq/Psq/Tdsq, field 6 is the
        # hardcoded '  100.00' total, field 7 is Osq2.
        cols = ref[:5] + [ref[6]]
        for j, (g, w) in enumerate(zip(got[k], cols)):
            if w is None:
                continue
            gv = float(g) * 100.0
            assert abs(gv - w) <= _ATOL_2PF82, f"{rel}: {k}[{j}] {gv} != {w}"

    if "f2.d" in want:
        _cmp_fixed(rel, "f2.d", got["f2.d"], raw["f2.d"], 8, _ATOL_F82)
    if "f2.e" in want:
        _cmp_fixed(rel, "f2.e", got["f2.e"], raw["f2.e"], 8, _ATOL_F82)
    if "f2.f" in want:
        _cmp_fixed(rel, "f2.f", got["f2.f"], raw["f2.f"], 8, _ATOL_F82)
    if "f2.g" in want:
        _cmp_fixed(rel, "f2.g", got["f2.g"], raw["f2.g"], 8, _ATOL_F82)
    if "f2.ic" in want:
        _cmp_fixed(rel, "f2.ic", got["f2.ic"], raw["f2.ic"], 12, _ATOL_F82)
    if "f2.is" in want:
        _cmp_fixed(rel, "f2.is", got["f2.is"], raw["f2.is"], 12, _ATOL_F82)
    if "f2.mcd" in want:
        assert int(got["f2.mcd"][0]) == int(want["f2.mcd"][0]), (
            f"{rel}: MCD {got['f2.mcd'][0]} != {want['f2.mcd'][0]}")


@pytest.mark.skipif(not CASES, reason="no corpus golden ships the F2 test battery")
@pytest.mark.parametrize("rel", CASES)
def test_f3_quality_statistics(rel: str) -> None:
    """f3cal.f: M1-M11, the composite Q, Q without M2, and the failure count."""
    got = _run(rel)
    want = _read_udg(os.path.join(_GOLDEN, rel, os.path.basename(rel) + ".udg"))
    if "f3.q" not in want:
        pytest.skip("golden carries no F3 quality block")
    assert "f3.q" in got, f"{rel}: the engine emitted no F3 block"

    for i in range(1, 12):
        k = f"f3.m{i:02d}"
        if k not in want:
            # Nn == 7 (a stable seasonal filter or a span under six years)
            # suppresses M8-M11 entirely -- so must the engine.
            assert k not in got, (
                f"{rel}: engine emitted {k} but the oracle suppressed it "
                "(Nn==7: Lstabl, or fewer than six years)")
            continue
        assert k in got, f"{rel}: {k} missing from harness output"
        assert abs(float(got[k][0]) - float(want[k][0])) <= _ATOL_M, (
            f"{rel}: {k} {got[k][0]} != {want[k][0]}")

    for k, atol in (("f3.q", _ATOL_Q), ("f3.qm2", _ATOL_Q)):
        if k in want:
            assert abs(float(got[k][0]) - float(want[k][0])) <= atol, (
                f"{rel}: {k} {got[k][0]} != {want[k][0]}")
    if "f3.fail" in want:
        assert int(got["f3.fail"][0]) == int(want["f3.fail"][0]), (
            f"{rel}: f3.fail {got['f3.fail'][0]} != {want['f3.fail'][0]}")


# --- D8B / D9A (prtd8b.f / prtd9a.f) ---------------------------------------
#
# Both call sites in x11pt3 were marked "deferred" alongside their PRINT
# tables, but each also writes a `.udg` savelog block that is not print
# surface -- 172 goldens carry `d8b.NN` and 169 carry `d9a.NN`. The engine
# emits both with the oracle's own formats, so these compare EXACTLY:
# `d8b` is a text row (years plus label characters) and `d9a` is E17.10,
# ten significant digits, which the arithmetic reaches on every gated spec.


def _read_udg_prefixed(path: str, prefix: str) -> dict[str, str]:
    out: dict[str, str] = {}
    pat = re.compile(r"^(" + prefix + r"\.[0-9]+):(.*)$")
    with open(path, encoding="utf-8", errors="replace") as fh:
        for ln in fh:
            m = pat.match(ln.rstrip("\n").lstrip())
            if m:
                out[m.group(1)] = m.group(2).rstrip()
    return out


@functools.lru_cache(maxsize=None)
def _run_raw(rel: str) -> str:
    spec = os.path.join(_CORPUS, rel + ".spc")
    txt = open(spec, encoding="utf-8", errors="replace").read().lower()
    if "pickmdl{" in txt:
        pytest.skip("pickmdl{} model selection is parse-only (M1)")
    r = subprocess.run([BIN, spec], capture_output=True, text=True,
                       cwd=os.path.dirname(spec))
    assert r.returncode == 0, f"{rel}: harness exit {r.returncode}\n{r.stderr}"
    assert r.stdout.splitlines()[0].strip() == "OUTCOME: OK", r.stdout[:200]
    return r.stdout


def _produced_prefixed(text: str, prefix: str) -> dict[str, str]:
    out: dict[str, str] = {}
    pat = re.compile(r"^(" + prefix + r"\.[0-9]+):(.*)$")
    for ln in text.splitlines():
        m = pat.match(ln)
        if m:
            out[m.group(1)] = m.group(2).rstrip()
    return out


@pytest.mark.skipif(not CASES, reason="no corpus golden ships the F2 test battery")
@pytest.mark.parametrize("rel", CASES)
@pytest.mark.parametrize("prefix", ["d8b", "d9a"])
def test_d8b_d9a(rel: str, prefix: str) -> None:
    """prtd8b.f / prtd9a.f savelog rows, compared line-exact."""
    udg = os.path.join(_GOLDEN, rel, os.path.basename(rel) + ".udg")
    want = _read_udg_prefixed(udg, prefix)
    if not want:
        pytest.skip(f"golden carries no {prefix} block")
    got = _produced_prefixed(_run_raw(rel), prefix)

    # Both directions: a missing row is an unreported period, an extra row is
    # the engine reporting one the oracle suppressed.
    missing = sorted(set(want) - set(got))
    extra = sorted(set(got) - set(want))
    assert not missing, f"{rel}: {prefix} rows missing from the engine: {missing}"
    assert not extra, f"{rel}: {prefix} rows the oracle does not emit: {extra}"

    bad = [(k, want[k], got[k]) for k in sorted(want) if want[k] != got[k]]
    assert not bad, "\n".join(
        f"{rel}.{k}: golden {w!r} != engine {g!r}" for k, w, g in bad)


# --- the remaining single-line X-11 savelog canaries ------------------------
#
# ftest.f's residual-seasonality F-test on D11 (`d11.f`, and `d11.3y.f` over
# just the last three years -- the SAME routine, gone round its DO WHILE a
# second time), sfmsr.f's global-MSR filter-selection trace (one
# `autosf.msrNN` per pass, then the `sfmsr` filter it settled on), and the two
# Henderson trend lengths the run chose (`d7trendma` at D7, `finaltrendma` at
# the final trend). All four routines were already ported; only the savelog
# rows were missing, and the D11 ftest call site was deferred outright.
#
# Compared LINE-EXACT: the engine writes each with the Fortran format that
# produced it, so there is nothing to round.

_X11_MISC = ["d11.f", "d11.3y.f", "sfmsr", "d7trendma", "finaltrendma"]
_X11_MISC_PREFIX = "autosf.msr"


def _read_udg_keys(path: str, keys, prefix: str) -> dict[str, str]:
    out: dict[str, str] = {}
    pat = re.compile(r"^([A-Za-z][A-Za-z0-9._]*):(.*)$")
    with open(path, encoding="utf-8", errors="replace") as fh:
        for ln in fh:
            m = pat.match(ln.rstrip("\n").lstrip())
            if m and (m.group(1) in keys or m.group(1).startswith(prefix)):
                out[m.group(1)] = m.group(2).rstrip()
    return out


def _produced_keys(text: str, keys, prefix: str) -> dict[str, str]:
    out: dict[str, str] = {}
    pat = re.compile(r"^([A-Za-z][A-Za-z0-9._]*):(.*)$")
    for ln in text.splitlines():
        m = pat.match(ln)
        if m and (m.group(1) in keys or m.group(1).startswith(prefix)):
            out[m.group(1)] = m.group(2).rstrip()
    return out


@pytest.mark.skipif(not CASES, reason="no corpus golden ships the F2 test battery")
@pytest.mark.parametrize("rel", CASES)
def test_x11_misc_canaries(rel: str) -> None:
    """d11.f / d11.3y.f / sfmsr / autosf.msrNN / d7trendma / finaltrendma."""
    udg = os.path.join(_GOLDEN, rel, os.path.basename(rel) + ".udg")
    want = _read_udg_keys(udg, _X11_MISC, _X11_MISC_PREFIX)
    if not want:
        pytest.skip("golden carries none of these canaries")
    got = _produced_keys(_run_raw(rel), _X11_MISC, _X11_MISC_PREFIX)

    missing = sorted(set(want) - set(got))
    extra = sorted(set(got) - set(want))
    assert not missing, f"{rel}: keys missing from the engine: {missing}"
    assert not extra, f"{rel}: keys the oracle does not emit: {extra}"

    bad = [(k, want[k], got[k]) for k in sorted(want) if want[k] != got[k]]
    assert not bad, "\n".join(
        f"{rel}.{k}: golden {w!r} != engine {g!r}" for k, w, g in bad)
