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

## Snapshot — 2026-07-18 19:50 EDT

| metric | value |
|---|---|
| start (first commit) | 2026-07-18 14:06 EDT |
| latest commit | 2026-07-19 00:14 EDT |
| commits | 25 |
| span, first→latest | 10h 08m |
| active (gaps ≤45m) | ~3h 20m (3 breaks excluded) |
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
- **M3 (in progress)** — regARIMA estimation leaves, all oracle-verified at
  bit level (`tools/ref_tier0.f`/`ref_tier1.f`/`ref_tier2.f` drive the real
  Fortran; `test_numeric` = 19 checks, ctest 5/5 green):
  - **Tier-0** (`core/src/numeric/`): `dpmpar`, `dpeq`, `scrmlt`, `maxvec`,
    `dcopy`, `daxpy`, `ddot` (underflow-skip), `revrse`, `enorm` (MINPACK 3-bin).
  - **Tier-1**: `yprmy`, `logdet`, `uconv`, `xpand`, `euclid` (numeric);
    `ratneg` (beside `ratpos` in regarima); `arflt` (new `regarima/armafilt`);
    `xprmx`, `dppfa` (Census packed Cholesky), `dsolve` (numeric).
  - Fable-model agent audited the Tier-0 port (all faithful) and produced the
    Tier-1 dependency-ordered plan; both build-level parity cautions (FP
    contraction, ddot log10) checked -- oracle & C++ both `-ffp-contract=off`.
- **Next** — `mltpos` (last of the Tier-1 batch; needs model.prm PXA/PB/PLEN/
  PORDER dims + the two-pass `secpas` zero-padding), then `chkrts` (setdp-only),
  then `intgpg`/`exctma` -> **armafl** (first big parity target: filter,
  residuals + Lndtcv at rtol 1e-12) -> `olsreg`/`rgarma` (estimation driver).
  `rpoly` is a separate mini-cluster (quad/fxshfr) for `roots`/`setmdl`, not on
  the armafl path. See `tools/m3_scouting.md`.

_Update this snapshot by pasting fresh `python tools/worklog.py` output; the git
timeline is the authority._
