"""Dropped-option sweep -- find spec arguments the engine accepts and ignores.

    python tools/option_sweep.py option_sweep_cases           # the misc blocks
    python tools/option_sweep.py option_sweep_cases_automdl   # automdl{}

Run from the repo root with `build/` built. Scratch runs land in
`tools/_sweep/` (override with X13_SWEEP_WORK); it is rewritten every case, so
do NOT try to inspect a directory after a multi-case run -- re-run the single
case you care about. That mistake cost a false reading once already.


For each candidate spec argument, run THREE times:

  A  oracle, base spec (option absent)
  B  oracle, base spec + option=<non-default value>
  C  engine, same spec as B

and classify from the `.udg` savelog keys the two sides share:

  INERT     A == B                  -- the probe never moved the oracle. NO
                                       CONCLUSION; the value or the base spec is
                                       wrong, not the engine. (The trap
                                       tools/history_options_scouting.md walked
                                       into: a null measured under the wrong
                                       preconditions is not a null.)
  DROPPED   A != B and C == A       -- the oracle moved and the engine did the
                                       default thing: parsed and thrown away.
  APPLIED   A != B and C == B       -- honoured.
  DIFFERS   A != B, C != A, C != B  -- read, but not the oracle's answer.
  REJECTED  the oracle refuses the spec -- the value is invalid, not a finding.

Engine keys come from the harness stdout, oracle keys from `<base>.udg`.
"""
import os
import re
import shutil
import subprocess
import sys

REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
ORACLE = REPO + "/oracle/fortran/x13as_ascii_O2.exe"
WORK = os.environ.get("X13_SWEEP_WORK",
                      os.path.dirname(os.path.abspath(__file__)) + "/_sweep")

KEY = re.compile(r"^([A-Za-z][A-Za-z0-9._$]*):(.*)$")
# Keys that echo the spec back rather than measuring anything -- an option that
# only changes its own echo has not changed the ADJUSTMENT.
ECHO = re.compile(r"^(spectype|decibel|siglevel|peakwd|specmaxar|startspec|"
                  r"diffspec|saveallspecfreq|showseasonalfreq|specseries|"
                  r"specrobustsa|altfreq|qslog|nplog|aape\.mode|"
                  r"transform|adjust|samode|seatsmodel|automdl|outlier|time$|date$|maxorder|maxdiff|balanced|exactdiff|mixed|checkmu|fcstlim|diff$|siglevel|peakwidth|tukey|qcheck|localpeak|maxar|taper|trendic|calendarsigma|keepholiday|finalsa|lognormal|probability|maxlead|exclude|method|types|lsrun|span|tcrate|almost|critical|aicdiff|pvaictest|tlimit|chi2|centeruser|noapply|eastermeans|power|outofsample|removeconstant|exact|maxiter|tol|start$|sdiff|maxlag)")


def read_keys(text):
    out = {}
    for ln in text.splitlines():
        m = KEY.match(ln.strip())
        if m and not ECHO.match(m.group(1)):
            out[m.group(1)] = m.group(2).rstrip()
    return out


def same(x, y, rtol=1e-6):
    """Value equality with a tolerance, field by field.

    A STRING compare makes this whole script useless on any auto-selected
    model: re-convergence noise moves `aic` in its 9th digit and a byte compare
    calls that a finding, so every automdl row reports DIFFERS and the real
    ones are indistinguishable. The bound is deliberately loose -- the question
    here is "did the engine do the same THING", not "is it bit-exact"; the
    parity gates answer that.
    """
    if x == y:
        return True
    if x is None or y is None:
        return False
    xs, ys = x.split(), y.split()
    if len(xs) != len(ys):
        return False
    for p, q in zip(xs, ys):
        if p == q:
            continue
        try:
            fp, fq = float(p), float(q)
        except ValueError:
            return False
        if abs(fp - fq) > rtol * max(abs(fp), abs(fq), 1e-30):
            return False
    return True


def diff(a, b, keys=None):
    """Differing keys, restricted to `keys` when given.

    The restriction is the whole point once the ENGINE is one of the operands:
    the harness prints a few keys of its own (OUTCOME) and does not print every
    savelog block the oracle does, so an unrestricted compare reports the
    port's unported surface on every single row and drowns the signal. The
    caller passes the intersection of all three runs.
    """
    ks = (set(a) | set(b)) if keys is None else keys
    return sorted(k for k in ks if not same(a.get(k), b.get(k)))


def stage_data(d):
    """Copy every corpus data file into the scratch dir.

    Case files name their own series, so staging only airline/payems silently
    made any other series unrunnable (the oracle reports a missing file, the
    run reads as REJECTED, and the argument looks tested when it was not).
    """
    src = REPO + "/tests/corpus/data"
    for f in os.listdir(src):
        if f.endswith(".dat"):
            shutil.copy(os.path.join(src, f), d)


def run_oracle(spec_text, tag):
    d = os.path.join(WORK, tag)
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    stage_data(d)
    with open(os.path.join(d, "s.spc"), "w") as fh:
        fh.write(spec_text)
    r = subprocess.run([ORACLE, "s", "-s"], cwd=d, capture_output=True, text=True)
    err = os.path.join(d, "s.err")
    if os.path.exists(err) and "ERROR" in open(err).read():
        return None
    udg = os.path.join(d, "s.udg")
    if not os.path.exists(udg):
        return None
    return read_keys(open(udg, errors="replace").read())


def run_engine(spec_text, tag, binary):
    d = os.path.join(WORK, tag + "_eng")
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    stage_data(d)
    with open(os.path.join(d, "s.spc"), "w") as fh:
        fh.write(spec_text)
    r = subprocess.run([REPO + "/build/" + binary + ".exe", "s.spc"], cwd=d,
                       capture_output=True, text=True)
    if not r.stdout.startswith("OUTCOME: OK"):
        return None
    return read_keys(r.stdout)


def main(cases):
    os.makedirs(WORK, exist_ok=True)
    width = max(len(c["name"]) for c in cases)
    for c in cases:
        base, withopt, binary = c["base"], c["with"], c.get("bin", "x13run_x11")
        name = c["name"]
        A = run_oracle(base, "a")
        B = run_oracle(withopt, "b")
        if A is None:
            print(f"{name:<{width}}  BASE-REJECTED")
            continue
        if B is None:
            print(f"{name:<{width}}  REJECTED     (oracle refuses the value)")
            continue
        C = run_engine(withopt, "b", binary)
        # Two separate questions, and conflating them is how this script lied
        # the first time round. (1) Did the ORACLE move at all? -- decided on the
        # oracle's own full key set, because restricting to keys the ENGINE also
        # prints makes an option the harness is simply blind to look INERT.
        # (2) Can we SEE the move from here? -- the 3-way intersection.
        dAB_all = diff(A, B)
        if not dAB_all:
            print(f"{name:<{width}}  INERT        (oracle unmoved -- bad probe)")
            continue
        common = set(A) & set(B) & (set(C) if C else set(A))
        dAB = diff(A, B, common)
        if not dAB:
            print(f"{name:<{width}}  HARNESS-BLIND (oracle moves {len(dAB_all)} "
                  f"keys, none the harness prints: {', '.join(dAB_all[:3])})")
            continue
        if C is None:
            print(f"{name:<{width}}  ENGINE-FATAL (oracle moved {len(dAB)} keys)")
            continue
        dCA, dCB = diff(C, A, common), diff(C, B, common)
        if not dCB:
            print(f"{name:<{width}}  applied      ({len(dAB)} keys move)")
        elif not dCA:
            ex = ", ".join(dAB[:3])
            print(f"{name:<{width}}  ** DROPPED **  {len(dAB)} keys: {ex}")
        else:
            ex = ", ".join(dCB[:3])
            print(f"{name:<{width}}  ~ DIFFERS      vsB {len(dCB)}, vsA {len(dCA)}: {ex}")


if __name__ == "__main__":
    sys.exit(main(__import__(sys.argv[1]).CASES))
