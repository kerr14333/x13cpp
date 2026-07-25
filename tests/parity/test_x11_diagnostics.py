"""M5 X-11 diagnostics gate: the F2 seasonality test battery vs the oracle .udg.

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

TOLERANCE. These are savelog canaries, not save tables: the oracle prints the
statistic as F11.3 and the probability as F8.2, so the golden pins them only to
+/-5e-4 and +/-5e-3 respectively. That printed precision IS the tolerance here
(same policy as test_m3_estimate's ``_print_ulp``) -- there is no 15-digit
golden for these values to compare against.

Run:  python -m pytest tests/parity/test_x11_diagnostics.py -q
"""
from __future__ import annotations

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
    want = {}
    pat = re.compile(r"(f2\.(?:fsb1|fsd8|kw|msf|idseasonal)):\s*(.*)$")
    with open(path, encoding="utf-8", errors="replace") as fh:
        for ln in fh:
            m = pat.match(ln.strip())
            if m:
                want[m.group(1)] = m.group(2).split()
    return want


@pytest.mark.skipif(not CASES, reason="no corpus golden ships the F2 test battery")
@pytest.mark.parametrize("rel", CASES)
def test_f2_seasonality_tests(rel: str) -> None:
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
        if p and p[0].startswith("f2."):
            got[p[0]] = p[1:]
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
