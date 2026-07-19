"""M3 gate: the C++ regARIMA estimation phase (x13run_m3) vs the oracle .udg.

For every corpus spec that (a) has an explicit ``arima`` model, (b) carries a
``.udg`` golden with estimation keys, and (c) is estimation-reproducible from the
pre-model + rgarma path alone (no automatic model/outlier/transform selection,
regressors drawn from the ported set), this test runs ``x13run_m3`` and checks
its estimation summary against the oracle:

  * integer canaries ``niter`` / ``nfev`` EXACT (the optimizer-trajectory match),
  * ``nreg`` / ``nefobs`` EXACT,
  * ``loglikelihood`` / ``aic`` / ``aicc`` / ``bic`` / ``hq`` and the ARMA
    coefficients at rtol 1e-6 (the .udg prints 4-6 significant digits).

This is the estimation analog of the M2 save-file gate: instead of 2 hand-built
unit-test specs, estimation is checked across the whole reproducible corpus.
"""
import math
import os
import re
import shutil
import subprocess
import tempfile

import pytest

_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", ".."))
_CORPUS = os.path.join(_REPO, "tests", "corpus")
_GOLDEN = os.path.join(_REPO, "tests", "golden")

RTOL = 1e-6


def _find_binary(name: str, env_var: str, required: bool = True):
    cands = [
        os.path.join(_REPO, "build", name + ".exe"),
        os.path.join(_REPO, "build", name),
        os.path.join(_REPO, "build", "Release", name + ".exe"),
    ]
    for c in cands:
        if os.path.exists(c):
            return c
    env = os.environ.get(env_var)
    if env and os.path.exists(env):
        return env
    if required:
        raise FileNotFoundError(
            f"{name} binary not found; build it first (cmake --build build).")
    return None


BIN = _find_binary("x13run_m3", "X13RUN_M3")
# Optional: distinguishes a pre-model parser gap (M2 also fatals -> skip) from an
# estimation regression (M2 ok but M3 fatals -> fail).
BIN_M2 = _find_binary("x13run_m2", "X13RUN_M2", required=False)


@pytest.fixture(scope="session")
def workdir():
    d = tempfile.mkdtemp(prefix="x13m3_")
    shutil.copytree(_CORPUS, os.path.join(d, "corpus"))
    yield os.path.join(d, "corpus")
    shutil.rmtree(d, ignore_errors=True)


def _golden_dir(spc: str) -> str:
    rel = os.path.relpath(spc, _CORPUS)
    return os.path.join(_GOLDEN, rel[:-4])


def _udg_path(spc: str) -> str:
    base = os.path.basename(spc)[:-4]
    return os.path.join(_golden_dir(spc), base + ".udg")


def _estimation_reproducible(spc: str) -> bool:
    """True when the model can be estimated by the pre-model + rgarma path alone.

    Excluded (need machinery beyond the current M3 slice):
      * automatic model / span identification (automdl, pickmdl),
      * automatic outlier identification (outlier{}), which the regvar branch
        still abends on,
      * automatic transform / AIC tests (function=auto, aictest),
      * regressor families outside the ported set (user, sincos, change-of-regime,
        outlier regressors ao/ls/tc/so/rp/tls, stock td),
      * fixed-model or fixed-coefficient runs (a separate branch, unit-tested),
      * composite runs.
    """
    txt = open(spc, "r", encoding="utf-8", errors="replace").read().lower()
    flat = txt.replace(" ", "")
    if "arima{" not in flat or "model=" not in flat:
        return False
    for bad in ("automdl", "pickmdl", "outlier{", "function=auto", "aictest",
                "tdstock", "sincos", "user=", "usertype", "tcrate",
                "composite{"):
        if bad in flat:
            return False
    # Outlier / change-of-regime regressors in a variables list.
    if re.search(r"variables\s*=\s*\([^)]*\b(ao|ls|tc|so|rp|tls)\d", txt):
        return False
    m = re.search(r"variables\s*=\s*\(([^)]*)\)", txt)
    if m and "/" in m.group(1):
        return False  # change-of-regime (regressor/date split)
    # Fixed coefficients (ar/ma/b = (...f)) -- a separate, unit-tested branch.
    if re.search(r"\d\s*f[\s,)]", txt):
        return False
    return True


def _specs():
    out = []
    for root, _dirs, files in os.walk(_CORPUS):
        for f in files:
            if not f.endswith(".spc"):
                continue
            spc = os.path.join(root, f)
            if not os.path.exists(_udg_path(spc)):
                continue
            if _estimation_reproducible(spc):
                out.append(spc)
    return sorted(out)


_SPECS = _specs()


# ---- parsing helpers --------------------------------------------------------

def _premodel_fatal(spc_in_workdir: str) -> bool:
    """True when x13run_m2 (pre-model, no estimation) also fatals on this spec --
    i.e. the failure is a known pre-model/parser gap, not an estimation bug."""
    if BIN_M2 is None:
        return False
    p = subprocess.run([BIN_M2, spc_in_workdir], capture_output=True, text=True)
    return p.returncode != 0 or "OUTCOME: FATAL" in p.stdout


def _run(spc_in_workdir: str) -> dict:
    """Run x13run_m3 and parse its `key: value` stdout into a dict."""
    proc = subprocess.run([BIN, spc_in_workdir], capture_output=True, text=True)
    if proc.returncode != 0 or "OUTCOME: FATAL" in proc.stdout:
        # Pre-model parser gap (M2 fatals too) -> skip; genuine estimation abend
        # (M2 succeeds) -> fail loudly.
        if _premodel_fatal(spc_in_workdir):
            pytest.skip("pre-model parser gap (x13run_m2 also fatals)")
        assert False, f"x13run_m3 estimation FATAL:\n{proc.stdout}\n{proc.stderr}"
    d = {}
    coefs = []
    for line in proc.stdout.splitlines():
        if ":" not in line:
            continue
        k, v = line.split(":", 1)
        k, v = k.strip(), v.strip()
        if k.startswith("arima."):
            coefs.append(float(v))
        else:
            d[k] = v
    d["_coefs"] = coefs
    return d


def _parse_udg(spc: str) -> dict:
    """Pull the estimation keys we compare out of the oracle .udg."""
    text = open(_udg_path(spc), "r", encoding="utf-8", errors="replace").read()
    d = {}
    coefs = []
    for line in text.splitlines():
        m = re.match(r"\s*([A-Za-z][\w$\[\]]*)\s*:\s*(.*)", line)
        if not m:
            continue
        key, rest = m.group(1), m.group(2).strip()
        # AR/MA coefficient lines: "MA$Nonseasonal$01$01: <coef> <se> <t>".
        if key.startswith(("AR$", "MA$")):
            coefs.append((key, float(rest.split()[0])))
        else:
            d[key] = rest
    # udg lists AR before MA, nonseasonal before seasonal -- the same order as
    # x13run_m3 walks the operators, so compare the coef values positionally.
    d["_coefs"] = [c for _, c in coefs]
    return d


def _close(a: float, b: float, rtol: float = RTOL) -> bool:
    return math.isclose(a, b, rel_tol=rtol, abs_tol=1e-9)


# ---- the test ---------------------------------------------------------------

@pytest.mark.parametrize("spc", _SPECS, ids=lambda s: os.path.relpath(s, _CORPUS))
def test_estimation_matches_oracle(spc, workdir):
    rel = os.path.relpath(spc, _CORPUS)
    work_spc = os.path.join(workdir, rel)
    got = _run(work_spc)
    exp = _parse_udg(spc)

    assert got["converged"] == "yes", "model did not converge"

    # Integer canaries -- exact.
    for k in ("niter", "nfev", "nreg", "nefobs"):
        if k in exp:
            assert int(got[k]) == int(float(exp[k])), (
                f"{k}: got {got[k]} != oracle {exp[k]}")

    # Scalar likelihood statistics -- rtol 1e-6.
    scal = {"loglikelihood": "loglikelihood", "aic": "aic", "aicc": "aicc",
            "bic": "bic", "hq": "hq", "variance": "variance$mle"}
    for gk, ek in scal.items():
        if ek in exp and gk in got:
            a, b = float(got[gk]), float(exp[ek])
            assert _close(a, b), f"{gk}: got {a} != oracle {b}"

    # ARMA coefficients -- positional, rtol 1e-6.
    assert len(got["_coefs"]) == len(exp["_coefs"]), (
        f"coef count: got {len(got['_coefs'])} != oracle {len(exp['_coefs'])}")
    for i, (a, b) in enumerate(zip(got["_coefs"], exp["_coefs"])):
        assert _close(a, b), f"arima coef #{i}: got {a} != oracle {b}"


def test_at_least_one_spec():
    assert _SPECS, "no estimation-reproducible corpus specs discovered"
