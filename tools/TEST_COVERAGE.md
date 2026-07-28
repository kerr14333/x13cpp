# Test-coverage roadmap — inputs & interactions

## Formal code coverage (gcov / gcovr)

Run `powershell -ExecutionPolicy Bypass -File tools/coverage.ps1` — builds an
instrumented tree in `build-cov` (--coverage at -O2; -O0 breaks the last-ULP
root-finder goldens), runs the unit (ctest) + parity (pytest, driving the
instrumented harness exes via `X13_BIN_DIR`) suites, and writes
`coverage/summary.txt` + browsable `coverage/index.html`. gcovr needs `-j 1` on
Windows (parallel workers CreateProcess-fail). Both `build-cov/` and `coverage/`
are gitignored.

**Baseline (this commit):** lines **72.7%** (6756/9288), functions **88.7%**
(244/275), branches 47.2%, over `core/src/` (generated `gen/` headers excluded).
The uncovered lines are mostly: deferred print/error branches, and —
- ~~**`adqtst.cpp` 0%**: the banked adequacy routines (mdlchk/tstmd2/testodf/
  bkdfmd/tstmd1) have NO caller yet (wire pending the automd finalization).~~
  **STALE as of 2026-07-26** — `automd.cpp` now calls all five: `mdlchk` at
  :186/:252/:264/:433/:637, `testodf` at :260, `tstmd2` at :291, `bkdfmd` at
  :464, `tstmd1` at :582. The percentages above are from the commit that wrote
  them and have not been re-measured since; re-run `tools/coverage.ps1` before
  quoting any of them.
- **`getreg_vars.cpp` 45% / `readers_val.cpp` 43%**: untested regression-variable
  TYPES and value-reader branches — directly raised by the "more calendar/holiday
  regressors" sweep below.
Coverage is the objective driver for which specs to add next.



What the parity corpus covers, what's testable NOW (features ported), and what's
blocked on unported subsystems. Gate everything against the oracle
(`oracle/fortran/x13as_ascii_O2.exe` via `oracle/run_oracle.py -s`), the pattern
used for the 30+ model specs and the reg/outlier sweep.

## Covered + gated (bit-exact vs oracle)
- **ARIMA estimation** across model TYPES: pure-AR `(2 0 0)/(3 1 0)/(3 1 1)`,
  MA `(0 1 2)`, mixed `(2 1 2)`, seasonal `(1 1 1)(1 1 1)`, on 7 series.
- **Series**: airline + FRED expgs/payems/unrate (SA) + R NSA nottem/ukgas/co2/
  usdeaths (seasonal). Monthly + quarterly.
- **Transforms**: log, none, and `function=auto` (trnaic AICC selection).
- **Regression**: trading day (td), Easter, td+easter, constant/mean.
- **Automatic outlier ID**: `outlier{}`, `types=(ao ls tc)`.
- **Automatic model selection**: iddiff + amdid + automd (identification +
  estimation) on SA and NSA series.

## Testable NOW (features ported — just need specs + goldens)
Priorities for the next sweep, each an oracle-vs-x13run_m3 diff like sweep_reg.py:
- **More holidays/calendar regressors**: `tdstock`, `td1coef`, `tdnolpyear`,
  `easterstock`, `labor`, `thank`, `sceaster`, `sincos` trig seasonal, fixed
  `seasonal` dummies. (regvar/td6var/td7var/adestr are ported.)
- **Prior adjustment**: `transform{ adjust = lom | loq | lpyear }` (length-of-
  month / leap-year priors — the priadj path is ported, exercised by m2_* specs
  but not broadly swept).
- **More transforms**: `sqrt`, Box-Cox `power=`, `logistic`, `inverse`.
- **Fixed / partially-fixed ARIMA coefficients**: `arima{ model=(0 1 1) ma=(0.5f) }`
  (the fixed-coef path — CB-6/armats fixed-lag handling is worth stressing here).
- **Estimation options**: `exact = ma | none` (conditional vs exact ML — note
  the default is exact=arma), `tol`/`maxiter` extremes.
- **Span / modelspan**: `series{ span=(a,b) }`, `modelspan` subsets, start/end
  trims (span-modelspan already flagged the model-span estimation gap).
- **Outlier options**: `outlier{ critical=3.5 }`, `lsrun`, `span`, `method`.
- **Forecast / backcast**: `forecast{ maxlead maxback }` prediction intervals on
  NON-outlier models (post-outlier forecasting is a known deferred non-parity).
- **Edge series**: near the min-obs limit, the 780-obs cap (edge/long780), very
  long series, quarterly vs monthly boundary cases.
- **automdl variations**: `maxorder`, `maxdiff`, `acceptdefault`, `checkmu=no`.

## Known gaps found by testing (real bugs — fix, don't just gate around)
- **User-specified outlier regressors FATAL** — `regression{ variables=(ao1950.jan) }`
  / `ls<date>` / `tc<date>` / ramps. Automatic outlier ID works; the regression-
  spec point-outlier syntax doesn't. (FABLE_REVIEW "Found gaps".)
- **03-automdl full estimation** — **Done, bit-exact.** `aictest=(td easter)`
  regressor selection + adequacy finalization landed; the model X-11 path is closed.

## Landed since this doc was first written (now bit-exact)
- **X-11 tables** (B/C/D/E ladder, D10/D11/D12/D13/D16) — `core/src/x11`; gates via
  `test_x11_tables.py`. Plus the spec-option front: `type` / `shrink` / `sigmavec` /
  `x11easter` / user-regression prior factor.
- **SEATS components** (s10–s18, .mdc models) — `core/src/seats`; the full SEATS
  corpus gates ~5e-15 via `test_seats_tables.py`.
- **Diagnostics** — `spectrum{}`, `history{}`/revisions, `slidingspans{}`, `force{}`
  all bit-exact (`test_spectrum` / `test_history` / `test_slidingspans` /
  `test_force`).

## Still blocked on unported subsystems

*(Section rewritten 2026-07-28 — four of its five bullets had been closed and
were still listed as "not started". Check `CLAUDE.md` before trusting this
list; it is the one that gets updated per feature.)*

- **pickmdl** (X-11-ARIMA model selection) — still parse-only (M1). **1 corpus
  spec** (`extra/airline_pickmdl`), not the 5 previously claimed, but it is the
  largest single source of real feature skips (9).
- **`automdl{}`'s label-40 `tstmd1` arm and the `pass2`/nloop call at `:665`.**
  Narrowed 2026-07-28b: the label-30 finalization tail previously listed here
  was ported all along and merely unreachable on the plain (non-aictest) path —
  wiring the existing call closed `urfinal`, `checkmu` and `cancel`. What
  remains blocks `noautooutlier` (fatal) and `ljungboxlimit` (fatal), and leaves
  `mixed`/`maxdiff`/`maxorder` partly wrong. See `tools/automdl_scouting.md`
  §3c and `tools/dropped_options_scouting.md` round 3.
- **`composite{}` SEATS branch** (`agr3s.f`), pseudo-additive, and the
  forced/rounded indirect series. The X-11 composite front (direct + indirect
  + comparison statistics + indirect diagnostics) is CLOSED and gated.
- **`history{}` leftovers** — `outlier=auto` / `outlierwin=` (now a clean fatal,
  not silent), `x11outlier=no`, `additivesa=`.
- **Misc open** — `spectrum{altfreq=yes}` (pending CB-30), the slidingspans
  `chs` per-span prior phase, and `generated/usdeaths_automdl`'s iddiff
  d=0/d=1 discrepancy.

**Closed since this list was written:** `check{}` (Ljung-Box / normality,
line-exact over 275 specs), the F2/F3 and M/Q statistics (x11pt4 increments
1–3), composite/indirect X-11 adjustment, user PRIOR factors (Nuspad/Nustad),
the Adjsea/Adjso regARIMA-seasonal combine, OLS-estimated x11regression
prior-TD (Ixreg>=2), and force non-original targets (Iftrgt>0).

## Interaction matrix worth building (once the pieces land)
automdl × outlier × aictest (the 03-automdl class) · transform=auto × regression ·
outlier × forecast · fixed-model × td × Easter · X-11 modes (mult/add/logadd/
pseudo-add) × seasonalma variants · SEATS × automdl-identified model · span ×
outlier × forecast. Plus error/edge specs (malformed args, out-of-span outliers,
conflicting options) checked for oracle-matching ABEND behavior.
