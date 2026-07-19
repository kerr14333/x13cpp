# X13cpp — work log

This project is a **proof of concept**: can Claude (Claude Code, Opus 4.8) port
Census X-13ARIMA-SEATS — 712 Fortran-77 files, ~167k lines — into parity-tested
C++/R/Python libraries? **Elapsed development time is the headline metric.**

The objective ledger is `git` commit timestamps. Regenerate this summary anytime:

```
python tools/worklog.py           # span + active-time estimate + commit timeline
python tools/worklog.py --gap 60  # tune the idle-break threshold (minutes)
```

- **span, first→latest commit** — raw wall-clock from the first commit to the last.
- **active** — sums the gaps between commits, excluding any gap longer than the
  `--gap` threshold (default 45 min) as a break, so long idle periods (agent runs
  the user stepped away from, overnight) don't inflate the figure.

## Snapshot — 2026-07-19 08:25 EDT

| metric | value |
|---|---|
| start (first commit) | 2026-07-18 14:06 EDT |
| latest commit | 2026-07-19 08:22 EDT |
| commits | 43 |
| span, first→latest | 18h 16m |
| active (gaps ≤45m) | ~4h 20m (6 breaks excluded) |
| calendar days | 2 |

## What was reached in that window

- **M0** — vendored Fortran oracle; built the golden reference binary; parity
  harness (oracle runner, `x13compare`, corpus); determinism gate **1,154,614
  values, 0 mismatches** (O0 vs O2).
- **M1** — spec-parser port (lexer, dispatch, series I/O), `x13parse` CLI; parse
  outcomes match the oracle on 42 specs; malformed-input `.err` byte-identical.
- **M2** — transform (`trn`), prior adjustment (`a2`/`a3`), the a1 save path, and
  the regression design matrix (`rmx`: constant/seasonal/TD/LOM-LOQ/leap-year/
  stock/Easter regressors via `regvar`→`savmtx`) — all **byte-identical** to the
  oracle at rtol 1e-8. rmx parity green on 7 specs (TD, Easter, quarterly,
  forecast-extension, no-log variants). Outlier/user/sincos/change-of-regime
  regressor branches deferred (abend loudly) — they need M3 estimation state.
- **Corpus** — 61 specs / 200 extra goldens (spectrum, history, slidingspans,
  x11regression, force, metadata, pickmdl, seats, outlier), Git LFS.
- **Scouted & ready** — M3 (regARIMA estimation) and the `.out` print engine, each
  with a written call-graph/parity-risk map under `tools/`.
- **M3 (in progress)** — regARIMA estimation, all oracle-verified at bit level
  (`tools/ref_*.f` drive the real Fortran; `test_numeric` = 26 checks, ctest
  5/5 green):
  - **Tier-0** (`core/src/numeric/`): `dpmpar`, `dpeq`, `scrmlt`, `maxvec`,
    `dcopy`, `daxpy`, `ddot` (underflow-skip), `revrse`, `enorm` (MINPACK 3-bin).
  - **Tier-1**: `yprmy`, `logdet`, `uconv`, `xpand`, `euclid` (numeric);
    `ratneg` (beside `ratpos` in regarima); `arflt`, `mltpos` (new
    `regarima/armafilt`); `xprmx`, `dppfa` (Census packed Cholesky), `dsolve`.
  - **Stateful cluster** (new `regarima/armafl`, takes `X13Context&`): `chkrts`
    (invertibility detector), `intgpg` (builds+factors G'G, sets Lndtcv),
    `exctma` (exact MA filter w*=-(G'G)⁻¹G'Hw), and **`armafl`** itself — the
    first big parity target, the whole exact ARMA filter (chkrts gate + intgpg +
    the ACV chain uconv/euclid/xpand + D matrix + chol(var(w_p|z)) + ddot
    correction + dsolve). The SAVEd Fortran local `nextma` lives in
    `X13Context::saved`. Verified end-to-end against `tools/ref_armaflx.f` on
    ARMA(1,1) and AR(2)MA(1) (2×2 D/Chlvwp, multi-lag ddot): residuals + Lndtcv
    match at **rtol 1e-12**. intgpg/exctma also verified alone on MA(2)
    (`tools/ref_armafl.f`).
  - 23 routines ported + verified so far (`test_numeric` = 50 checks). Fable
    audited Tier-0 + produced the Tier-1 plan; FP-contraction parity confirmed.
  - **Estimation driver stack** (`regarima/estimate`): `olsreg` (normal-eqn OLS),
    `resid`, `upespm` (param scatter), `fcnar` (lmdif objective: upespm→armafl→
    exact-ML scale; numeric path exact, info!=0 warning prints deferred to .out).
  - **MINPACK optimizer leaves** (`numeric/minpack`, Census-modified — dpeq for
    .eq.0): `qrfac`, `qrsolv`, `lmpar` (LM trust-region secant), `fdjac2`
    (forward-diff Jacobian via a `MinpackFcn` callback so the optimizer stays
    model-independent). All oracle-verified.
  - **armafl coverage** — all 14 A-gaps in `tools/m3_test_coverage.md` closed:
    airline/seasonal-AR/mixed/multi-col/differencing/reuse/error-code/FMA-canary/
    underflow-skip/exact-zero-no-write. `test_numeric` = 163 checks.
- **Next** — `lmdif` (the 530-line Census-modified LM core; [[x13cpp-lmdif-modified]]
  — port the vendored version, not textbook) → `covar` → then `rgarma` glues the
  IGLS loop (olsreg + lmdif(fcnar)). B-section corpus specs (exact=none, AR
  models, fixed coeffs, RSXFSN/FEDFUNDS) land once rgarma runs end-to-end. See
  `tools/m3_test_coverage.md` and `tools/m3_scouting.md`.

_Update this snapshot by pasting fresh `python tools/worklog.py` output; the git
timeline is the authority._
