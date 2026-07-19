"""M2 gate: the C++ pre-model table/save phase (x13run_m2) vs the oracle goldens.

This milestone slice ports the table-output save path (savtbl.f / punch.f /
dtoc.f) and produces table **a1** -- the original series over the analyzed span
-- both as the library's numeric data surface and as the byte-identical /rdb
save file. For every corpus spec whose golden bundle contains a ``<base>.a1``
save file, this test runs ``x13run_m2`` and checks that the produced ``.a1``:

  * is byte-identical to the golden (line endings normalized: the only allowed
    difference is CRLF vs LF, an OS text-mode artifact), and
  * matches numerically at rtol 1e-8, period-key exact (via x13compare).

a1 is a pre-model table (raw original series over the span), so it is reachable
without ARMA estimation and is produced for essentially every corpus spec.

Run:  pytest tests/parity/test_m2_tables.py -v
"""
from __future__ import annotations

import math
import os
import re
import shutil
import subprocess
import sys
import tempfile

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus")
_GOLDEN = os.path.join(_REPO, "tests", "golden")
_COMPARE = os.path.join(_REPO, "tests", "compare")

if _COMPARE not in sys.path:
    sys.path.insert(0, _COMPARE)
from x13compare import parse_save  # noqa: E402

RTOL = 1e-8


def _find_binary() -> str:
    cands = [
        os.path.join(_REPO, "build", "x13run_m2.exe"),
        os.path.join(_REPO, "build", "x13run_m2"),
        os.path.join(_REPO, "build", "Release", "x13run_m2.exe"),
    ]
    for c in cands:
        if os.path.exists(c):
            return c
    env = os.environ.get("X13RUN_M2")
    if env and os.path.exists(env):
        return env
    raise FileNotFoundError(
        "x13run_m2 binary not found; build it first (cmake --build build).")


BIN = _find_binary()


@pytest.fixture(scope="session")
def workdir():
    """An isolated copy of the corpus tree the tool can write save files into."""
    d = tempfile.mkdtemp(prefix="x13m2_")
    shutil.copytree(_CORPUS, os.path.join(d, "corpus"))
    yield os.path.join(d, "corpus")
    shutil.rmtree(d, ignore_errors=True)


def _golden_dir(spc: str) -> str:
    rel = os.path.relpath(spc, _CORPUS)
    return os.path.join(_GOLDEN, rel[:-4])


def _specs_with_a1():
    out = []
    for root, _dirs, files in os.walk(_CORPUS):
        for f in files:
            if not f.endswith(".spc"):
                continue
            spc = os.path.join(root, f)
            base = f[:-4]
            if os.path.exists(os.path.join(_golden_dir(spc), base + ".a1")):
                out.append(spc)
    return sorted(out)


def _specs_with_ext(ext: str):
    """All corpus specs whose golden bundle contains a <base>.<ext> save file."""
    out = []
    for root, _dirs, files in os.walk(_CORPUS):
        for f in files:
            if not f.endswith(".spc"):
                continue
            spc = os.path.join(root, f)
            base = f[:-4]
            if os.path.exists(os.path.join(_golden_dir(spc), base + "." + ext)):
                out.append(spc)
    return sorted(out)


_A1_SPECS = _specs_with_a1()


def _trn_reproducible(spc: str) -> bool:
    """True when table trn is reproducible from the pre-model phase alone.

    trn is the prior-adjusted, Box-Cox/logit-transformed modeling series. With an
    explicit transform and no prior/calendar adjustment it is exactly
    trnfcn(original series). It is NOT pre-model reproducible when:
      * the transform is auto-selected (needs the AIC transform test), or
      * trading-day / length-of-period regressors induce the implicit leap-year
        prior adjustment (needs the prior-adjustment + regression-matrix chain), or
      * the ARIMA model / span is auto-identified (automdl/pickmdl), which can
        shrink the trn span to a model span.
    Those are estimation-dependent and out of the M2 pre-model scope.
    """
    txt = open(spc, "r", encoding="utf-8", errors="replace").read().lower()
    txt = txt.replace(" ", "")
    if "function=auto" in txt or "automdl" in txt or "pickmdl" in txt:
        return False
    if "aictest" in txt:
        return False
    # The predefined adjust=lom/loq/lpyear priors ARE reproduced (priadj.cpp);
    # only reject user-data priors (transform{data=...}) which need file input.
    if "data=" in txt and "transform{" in txt:
        return False
    # Trading-day regressors with a log transform induce the implicit leap-year
    # prior (gtinpt.f/rmlnvr.f) -- reproduced by the regression-matrix chunk,
    # so td specs are no longer excluded. Stock td remains unported.
    if "tdstock" in txt:
        return False
    return True


def _implicit_lpyear(spc: str) -> bool:
    """True when a td/td1coef regressor plus a log transform induces the
    implicit leap-year prior adjustment (gtinpt.f Picktd -> rmlnvr.f), which the
    pre-model phase reproduces (a2/a3 = lpyear factors / adjusted data)."""
    txt = open(spc, "r", encoding="utf-8", errors="replace").read().lower().replace(" ", "")
    if "function=log" not in txt:
        return False
    if "tdstock" in txt or "automdl" in txt or "pickmdl" in txt or "aictest" in txt:
        return False
    return ("(td)" in txt or "(td," in txt or ",td)" in txt or ",td," in txt or
            "(td" in txt)


def _has_predefined_adjust(spc: str) -> bool:
    """True when transform{ adjust = lom|loq|lpyear } is requested -- the
    predefined length-of-period / leap-year prior reproduced by priadj.cpp.
    (Other paths that emit a2/a3, e.g. the SEATS a3, are out of the pre-model
    scope.)"""
    txt = open(spc, "r", encoding="utf-8", errors="replace").read().lower().replace(" ", "")
    return "adjust=lom" in txt or "adjust=loq" in txt or "adjust=lpyear" in txt


_TRN_SPECS = [s for s in _specs_with_ext("trn") if _trn_reproducible(s)]
_A2_SPECS = [s for s in _specs_with_ext("a2")
             if _has_predefined_adjust(s) or _implicit_lpyear(s)]
_A3_SPECS = [s for s in _specs_with_ext("a3")
             if _has_predefined_adjust(s) or _implicit_lpyear(s)]


def _rmx_reproducible(spc: str) -> bool:
    """True when the rmx regression matrix is pre-model reproducible: every
    column is a deterministic calendar/differencing function. NOT reproducible
    when automatic outlier identification (outlier spec) or automatic model /
    regressor selection can add or drop columns before arima.f's savmtx call,
    or when the spec uses regressor families the M2 slice has not ported."""
    txt = open(spc, "r", encoding="utf-8", errors="replace").read().lower()
    txt = txt.replace(" ", "")
    if "outlier{" in txt or "automdl" in txt or "pickmdl" in txt:
        return False
    if "aictest" in txt or "function=auto" in txt:
        return False
    if "x11regression" in txt:
        return False
    # Unported regressor families (loud-abend paths in the C++): keep only
    # specs whose variables list is drawn from the ported set.
    raw = open(spc, "r", encoding="utf-8", errors="replace").read().lower()
    m = re.search(r"variables\s*=\s*\(([^)]*)\)", raw)
    if m:
        ported = {"const", "seasonal", "td", "tdnolpyear", "td1coef",
                  "td1nolpyear", "lom", "loq", "lpyear"}
        for tok in re.split(r"[\s,]+", m.group(1).strip()):
            if not tok:
                continue
            base = tok.split("[")[0]
            if base in ported or base in ("easter", "sceaster"):
                continue
            return False
    return True


_RMX_SPECS = [s for s in _specs_with_ext("rmx") if _rmx_reproducible(s)]


def _check_save_table(spc, workdir, ext):
    """Run x13run_m2 on spc and assert its <base>.<ext> matches the golden:
    byte-identical (LF-normalized) AND numeric parity at rtol 1e-8."""
    rel = os.path.relpath(spc, _CORPUS)
    base = os.path.basename(spc)[:-4]
    work_spc = os.path.join(workdir, rel)

    proc = subprocess.run([BIN, work_spc], capture_output=True, text=True)
    assert "OUTCOME: OK" in proc.stdout, f"{rel}: run failed\n{proc.stdout}\n{proc.stderr}"

    mine = os.path.join(os.path.dirname(work_spc), base + "." + ext)
    gold = os.path.join(_golden_dir(spc), base + "." + ext)
    assert os.path.exists(mine), f"{rel}: no .{ext} produced"

    a = open(mine, "rb").read().replace(b"\r\n", b"\n")
    b = open(gold, "rb").read().replace(b"\r\n", b"\n")
    assert a == b, f"{rel}: .{ext} not byte-identical (LF-normalized)"

    # Numeric parity, period-key exact, rtol 1e-8.
    ma = parse_save.parse_save(a.decode("utf-8", "replace")).values
    mb = parse_save.parse_save(b.decode("utf-8", "replace")).values
    assert set(ma) == set(mb), f"{rel}: period keys differ"
    for k in mb:
        assert math.isclose(ma[k], mb[k], rel_tol=RTOL, abs_tol=0.0), (
            f"{rel}: value mismatch at {k}: {ma[k]!r} vs {mb[k]!r}")


@pytest.mark.parametrize("spc", _A1_SPECS, ids=lambda p: os.path.relpath(p, _CORPUS))
def test_a1_save_matches_oracle(spc, workdir):
    _check_save_table(spc, workdir, "a1")


@pytest.mark.parametrize("spc", _TRN_SPECS, ids=lambda p: os.path.relpath(p, _CORPUS))
def test_trn_save_matches_oracle(spc, workdir):
    """Transformed (prior-adjusted) series -- transform subsystem (trnfcn.f)."""
    _check_save_table(spc, workdir, "trn")


@pytest.mark.parametrize("spc", _A2_SPECS, ids=lambda p: os.path.relpath(p, _CORPUS))
def test_a2_save_matches_oracle(spc, workdir):
    """Prior-adjustment factors -- prior-adjustment subsystem (td7var.f/adjsrs.f)."""
    _check_save_table(spc, workdir, "a2")


@pytest.mark.parametrize("spc", _A3_SPECS, ids=lambda p: os.path.relpath(p, _CORPUS))
def test_a3_save_matches_oracle(spc, workdir):
    """Prior-adjusted data (original / prior factors)."""
    _check_save_table(spc, workdir, "a3")


def _check_matrix_table(spc, workdir, ext):
    """Like _check_save_table but for matrix tables (rmx): byte-identical AND
    every column of every row matches at rtol 1e-8, period-key exact."""
    rel = os.path.relpath(spc, _CORPUS)
    base = os.path.basename(spc)[:-4]
    work_spc = os.path.join(workdir, rel)

    proc = subprocess.run([BIN, work_spc], capture_output=True, text=True)
    assert "OUTCOME: OK" in proc.stdout, f"{rel}: run failed\n{proc.stdout}\n{proc.stderr}"

    mine = os.path.join(os.path.dirname(work_spc), base + "." + ext)
    gold = os.path.join(_golden_dir(spc), base + "." + ext)
    assert os.path.exists(mine), f"{rel}: no .{ext} produced"

    a = open(mine, "rb").read().replace(b"\r\n", b"\n")
    b = open(gold, "rb").read().replace(b"\r\n", b"\n")
    assert a == b, f"{rel}: .{ext} not byte-identical (LF-normalized)"

    pa = parse_save.parse_save(a.decode("utf-8", "replace"))
    pb = parse_save.parse_save(b.decode("utf-8", "replace"))
    assert set(pa.rows) == set(pb.rows), f"{rel}: period keys differ"
    for k in pb.rows:
        ra, rb = pa.rows[k], pb.rows[k]
        assert len(ra) == len(rb), f"{rel}: column count differs at {k}"
        for j, (va, vb) in enumerate(zip(ra, rb)):
            assert math.isclose(va, vb, rel_tol=RTOL, abs_tol=0.0), (
                f"{rel}: value mismatch at {k} col {j + 1}: {va!r} vs {vb!r}")


@pytest.mark.parametrize("spc", _RMX_SPECS, ids=lambda p: os.path.relpath(p, _CORPUS))
def test_rmx_save_matches_oracle(spc, workdir):
    """Regression design matrix -- regvar.f subtree (td6var/td7var/addsef/
    adestr/estrmu/ratpos) + savmtx.f. All-or-nothing: every column at once."""
    _check_matrix_table(spc, workdir, "rmx")


def test_a1_corpus_coverage():
    """Guard: the gate actually exercises a broad set of specs."""
    assert len(_A1_SPECS) >= 30, f"expected >=30 a1 specs, found {len(_A1_SPECS)}"


def test_trn_corpus_coverage():
    """Guard: the transform gate exercises the transform variants (log/sqrt/none/power)."""
    assert len(_TRN_SPECS) >= 5, f"expected >=5 trn specs, found {len(_TRN_SPECS)}"


def test_rmx_corpus_coverage():
    """Guard: the rmx gate exercises the ported regressor families (constant,
    td, easter, seasonal, quarterly, forecast extension)."""
    assert len(_RMX_SPECS) >= 7, f"expected >=7 rmx specs, found {len(_RMX_SPECS)}"


if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-v"]))
