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

## Snapshot — 2026-07-19 18:40 EDT

| metric | value |
|---|---|
| start (first commit) | 2026-07-18 14:06 EDT |
| latest commit | 2026-07-19 18:40 EDT |
| commits | 72 |
| span, first→latest | 28h 17m |
| active (gaps ≤45m) | ~7h 15m (11 breaks excluded) |
| calendar days | 2 |

**Census bug ledger:** `tools/census_bugs.md` records Census-source defects the
port reproduces faithfully, for deliberate modernization later (each pinned to a
test). CB-1 rpoly scaling collapse (max|coeff|>=10), CB-2 stpitr dead store,
CB-3 roots typo'd 2pi, CB-4 setmdl lagind undersize, CB-5 setmdl stale comment.

**Toolchain note:** adding the first NEW source file (rpoly.cpp) tripped CMake's
CONFIGURE_DEPENDS glob reconfigure, which the machine's cmake 3.14.3 rejects
(CMakeLists requires 3.16). Installed cmake 4.4.0 user-locally via `py -m pip
install cmake` (at `%APPDATA%\Python\Python314\site-packages\cmake\data\bin`) and
re-configured `build/` with it. `tools/build.ps1` still points at the 3.14.3
cmake for the no-reconfigure incremental path; drive fresh configures with the
pip cmake when adding files.

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
  - **`lmdif`** (`numeric/minpack`) — the 530-line Census-modified LM core
    ([[x13cpp-lmdif-modified]]; vendored, not textbook). Two nested loops + a
    single goto for the 6 GO-TO-20 termination sites, dpeq at all 7 exact-zero
    tests. The model-specific `upespm`/`prtitr` calls are threaded as optional
    model-independent hooks (`MinpackSync`/`MinpackPrtitr`) so the optimizer
    stays decoupled from regarima; empty hooks are a bit-for-bit no-op.
    Census-vs-stock diffs documented in a header block. Oracle-verified on
    `tools/ref_lmdif.f` (3 cases: Rosenbrock full-converge info=2; exp-fit M>N
    mode=2 info=1; re-entrant cumulative-counter info=5) — integer nliter/nfev +
    Info exact, x/fjac/qtf/diag at rtol 1e-12. Port audit in
    `tools/lmdif_port_spec.md`.
  - **`covar`** (`numeric/minpack`) — covariance `(R'R)⁻¹` from lmdif's QR
    output; literal `|R(k,k)|<=tolr` singularity test, NOTSET=-32767 sentinel.
    Oracle-verified (`tools/ref_covar.f`, permuted ipvt, both tolerance paths).
  - `test_numeric` = 49 tests / all MINPACK optimizer leaves landed.
- **rgarma sub-leaves (in progress)** — porting the Tier-2 helpers rgarma needs:
  - **`rpoly`** (`numeric/rpoly`) — the Jenkins-Traub root-finder suite (9
    routines sharing global.cmn -> module-private RpolyState). Reproduces a
    documented Census bug (max|coeff|>=10 -> the hardcoded-constant scaling
    branch zeroes the polynomial -> fail); never fires for real AR/MA polys.
  - **`strtvl`** (`regarima/estimate`) — ARMA starting values.
  - **`stpitr`** (`regarima/estimate`) — IGLS step/convergence test; SAVEd
    oldobj -> `ctx.saved.stpitr_oldobj`.
  - **`roots`** (`regarima/estimate`) — revrse+rpoly wrapper; modulus/frequency
    + invertibility flag (Allinv). Reproduces CB-3 (typo'd 2pi frequency).
  - **`setmdl`** (`regarima/estimate`) — pack estprm + starting-value root check
    (theta invertibility, phi stationarity), near-unit MA operator shrinkage,
    SAVEd `first` -> `ctx.saved.setmdl_first`, abend/laumts handshake. Does NOT
    difference Xy (CB-5, stale header comment). CB-4 lagind undersize.
  - `test_numeric` = 57 tests. All oracle-verified (rpoly bit-for-bit incl.
    failures; the rest rtol 1e-12 / exact int+bool).
  - **All rgarma prerequisites now landed.**
- **`rgarma` (rgarma.f, 475 lines): the capstone — LANDED, oracle-verified.**
  The IGLS driver glued end-to-end: strtvl/setmdl start values, olsreg (GLS
  betas) / yprmy (Nb=0 path), resid, the `lmdif(fcnar)` nonlinear ARMA step with
  cumulative Nliter/Nfev, stpitr convergence, then fdjac2/qrfac/covar for the
  post-convergence ARMA covariance. Fortran labels 10 (RETURN) / 20 (loop) map
  to `return`/`continue`; the EQUIVALENCE workspace overlay (diag/qtf/wa1..4/
  tmpa on txy) becomes disjoint arrays (no live overlap, parity risk 11). `Na+
  tnltol` snapshotted before the ref counter is passed to lmdif (risk 4).
  `chkrt2` ported too (vendored version inverts nothing -- inverr=0 + deferred
  root-table print); `savitr` LESTIT save guarded/deferred; prtitr/Frstcl/Scndcl
  deferred with the print hook (no numeric effect). **First full end-to-end
  estimation parity: `ref_rgarma.f` drives an Nb=0 ARMA(1,1) on a 24-point
  series -> C++ matches the oracle bit-for-bit** -- Nliter=14, Nfev=47 EXACT
  (the trajectory-identical canary), phi/theta/Var/Lnlkhd/Lndtcv + covariance
  diag at rtol 1e-12, Convrg/Armaer/Lcalcm exact.
  - **Nb>0 regression case** (`ref_rgarma2.f`): ARMA(1,1) + intercept
    (Ncxy=2), driving the olsreg GLS solve, `Nfev+=Ncxy+1`/pass, and the
    multi-pass IGLS outer loop (tnltol 2/n·Nltol0 -> 2/n·Nltol after iter 2).
    Oracle converges to a NEAR-UNIT MA root (theta~0.99999) -- a penalty-wall
    stress case (parity risk 1) -- and C++ still matches bit-for-bit:
    Nliter=12/Nfev=56 exact, b1/phi/theta/Var/Lnlkhd + covariance at rtol
    1e-12. `test_numeric` = 59 tests.
- **Tier-6 likelihood stats (started)** — the post-estimation numeric leaves that
  consume rgarma's output: `xrlkhd` (corrected AIC / AICC from Lnlkhd, Nspobs,
  Ncxy less fixed betas) and `armats` (ARMA t-stats, Arimap/sqrt(Var·Armacm_kk)).
  Verified against `ref_armastat.f` (rgarma -> xrlkhd -> armats on the Nb=0
  case): AICC + both t-stats at rtol 1e-12. Found **CB-6**: armats counts fixed
  ARMA lags while Armacm is packed by free params -> misindexed t-stats for
  fixed-coef models (ported verbatim, logged). `armacr`/`prlkhd` are print-heavy,
  deferred to the .out milestone.
- **run_m2 -> rgarma real-data estimation — LANDED (the M3 headline).** `run_m2`
  gained a defaulted `estimate` flag: off keeps the M2 save-only binary (M2
  parity gate still 94 pass), on estimates the built model in place after regvar
  via `rgarma`. A self-contained inline airline spec (144 obs, log, ARMA
  (0 1 1)(0 1 1), no regression) driven through the **whole front end**
  (parse_spec -> getsrs/transform/getmdl/regvar -> rgarma) matches the oracle
  `.udg` golden (`airline_check`) **bit-for-bit**: `niter=6`/`nfev=19` EXACT (the
  trajectory canary), MA nonseasonal `0.40180794878596` / seasonal
  `0.55694564337114`, `variance$mle 0.13480973219978E-02`, both armats t-stats
  (5.0946 / 7.3037) at rtol 1e-9..1e-12. First **real-data** end-to-end
  estimation parity (not hand-set common state). Scouting §7 open questions
  resolved: (a) Tsrs written by rgarma->resid from Xy, (b) rgarma differences Xy
  internally via Nintvl (no external differencing), (c) Nb=0/Ncxy=1 yprmy path
  consistent post-regvar. `arimap` is packed by operator-coefficient position
  over DIFF..AR..MA, so the free MA coefs sit at slots 3/4 behind the two fixed
  differencing slots. `test_numeric` = 62 tests.
- **`prlkhd` likelihood stats — LANDED.** Ported prlkhd.f's numeric core
  (prints deferred): the transform-Jacobian-adjusted log likelihood
  Olkhd=Lnlkhd+jacadj and the criteria Aic/Aicc/Hnquin/Bic/Bic2/Eic into
  ctx.lkhd. jacadj sums the per-obs log Jacobian of the Box-Cox/logit transform
  over the effective span (for log: -sum log(y)); unlike xrlkhd (AIC-test path,
  drops the constant jacadj) the reported criteria include it. run_m2 runs it
  after rgarma. **Subtlety:** the .udg `loglikelihood` key is the RAW `Lnlkhd`
  (arima.f:973 writes it directly), while AIC/AICC/BIC/HQ use the adjusted
  Olkhd -- the two differ by jacadj.
- **Nb>0 real-data estimation — LANDED.** airline + regression{ (td easter[8]) }
  through run_m2->rgarma drives the full GLS engine (regvar TD/Easter design,
  olsreg normal-eqn GLS, multi-pass IGLS) from a real spec and matches the
  oracle .udg (02-airline-log-td-easter) bit-for-bit: niter=9/nfev=76 EXACT,
  MA 0.21534/0.55175, TD-Mon + Easter betas, variance$mle, loglikelihood
  259.3105. Nb=7/Ncxy=8. `test_numeric` = 63 tests.
- **Tier-7 forecasting (started) — `fcstxy` LANDED.** The MMSE forecast engine:
  forecasts of the transformed/regression-adjusted series + forecast standard
  errors. Forecast recursion runs the full AR·diff and MA operators (polyml)
  forward, seeded by the exact ARMA-filtered residuals (armafl); resid applies
  the regression adjustment; SEs from the psi(B)=MA/AR weights (ratpos) plus the
  design-uncertainty term X_f(X'X)⁻¹X_f' (dppsl/yprmy) for unfixed regressors.
  Prereq `dppsl` (LINPACK packed single-RHS solve, Census `alt` forward-only
  extension) ported too. Oracle-verified on the ARMA(1,1) case both no-regression
  (`ref_fcstxy.f`, Rgvar=0) and +intercept (`ref_fcstxy2.f`, dppsl design term,
  Rgvar≠0) at rtol 1e-11/1e-12. `test_numeric` = 66 tests. **Not yet
  exercised:** the differencing branch (mxdfar>0 tfcst seeding) — covered once
  fcstxy is wired into run_m2 behind a `forecast{}` request for real airline
  forecasts.
- **Estimation corpus parity GATE — LANDED (test-plan C1).** New `x13run_m3` CLI
  (run_m2 estimate=true, stdout dump, writes no file) + `test_m3_estimate.py`
  sweep: **25 corpus specs** estimate and match the oracle `.udg` (niter/nfev/
  nreg/nefobs exact, loglikelihood/aic/aicc/bic/hq/variance + ARMA coefs rtol
  1e-6); 9 skip as pre-model parser gaps (distinguished from real regressions by
  checking x13run_m2 also fatals). This replaces the 2-hand-spec estimation
  coverage — quarterly/AR/mixed corpus models now checked automatically. Plus
  unit coverage: fixed-ARMA-coefficient estimation (C2 partial, ref_rgarma_fixed)
  and all three `fcstxy` branches (C4, differencing via ref_fcstxy3). test_numeric
  = 68.
- **Forecast-output leaves: `invfcn` + `lgnrmc` LANDED.** The inverse
  Box-Cox/logit transform (invfcn.f) and the lognormal mean-correction (lgnrmc.f)
  that put fcstxy's transformed forecasts back on the original scale. Shared
  transform leaves (x11/tables reuse them later). Oracle-verified all branches at
  rtol 1e-14.
- **Forecast-output chain COMPLETE — real `.fct` forecasts LANDED.** Ported the
  remaining leaves `dinvnr` (inverse-normal critical value) + its helpers
  `cumnor`/`stvaln`/`devlpl`, and `eltfcn` (elementwise add/sub/mul/div); cumnor
  inlines the two spmpar machine constants (0.5·DBL_EPSILON / DBL_MIN, exact
  under the active ipmpar block). New `fcstout` (regarima/forecast.cpp) is the
  numeric core of prtfct.f's LFOROS path: fcstxy → invfcn/lgnrmc point forecast,
  then the two-tailed band `invfcn(fcst ± dinvnr((Ciprob+1)/2)·se)`. Wired into
  run_m2 behind `forecast{}` (results on `ctx.forecasts`, no file output); the
  forecast{} reader now also parses `probability`/`lognormal`, and gtinpt seeds
  the Ciprob=0.95 / Lognrm=F defaults. New parity gate `test_m3_forecast.py`:
  the C++ original-scale forecast table matches the oracle `.fct` bit-for-bit at
  the printed precision across 4 real series (airline/expgs/payems/unrate, log
  airline model) — **this exercises fcstxy's differencing branch on real airline
  data end-to-end** (rtol 1e-9). `test_numeric` = 74; full suite 227 passed / 9
  skipped / 11 xfailed. (The oracle's seats-driven forecast-count override — 3·sp
  vs the requested maxlead — is a later seats-phase concern; the gate compares the
  overlapping leads, which are count-independent.)
- **Automatic outlier identification — LANDED (idotlr end-to-end).** The full
  AO/LS/TC outlier scan wired into run_m2 behind `outlier{}`. Leaves ported +
  oracle-verified: `shlsrt`/`medabs` (robust residual mse), `makotl` (AO/LS/TC
  regressor construction), `ttest` (forward add-one proportional t via augmented
  Cholesky), `dppdi`+`dscal` (backward-deletion se via packed inverse), `deltst`
  (backward t-test), `coladd`/`addotl` (Xy column insert + reconstruction),
  `rdotlr`/`wrtdat`/`wrtotl` (outlier title <-> type+date), and the default
  critical-value chain `setcv`/`setcvl`/`ppnd`/`lassol` (Cvalfa=0.05 default ->
  aocrit 3.890 for the 144-obs airline). The `idotlr` driver does forward
  addition (scan every test point, add the largest over `Critvl*rbmse` via
  adrgef/coladd/addotl, re-estimate, repeat) then backward deletion (deltst ->
  dlrgef -> re-estimate). Also completed the two `adrgef` auto-outlier
  date-ordering branches that were `not_ported` stubs (group + column insertion),
  now that rdotlr exists. **End-to-end oracle parity:** airline+td+outlier
  identifies `AO1951.May` and re-estimates to niter=12/nfev=85 EXACT, MA
  0.068877/0.518092, variance 9.4073e-4, AO coef 0.115444506648419 -- all
  bit-for-bit vs the oracle `.udg`. The M3 estimation gate now includes 5
  outlier specs (4 series' fixed-airline-x11 + payems lsrun): **30 specs pass**
  (was 25). `test_numeric` = 81. Deferred (as with fcstout): all iteration/table
  printing + save files, the x11-regression (lxreg) path, and the diagnostic
  "almost outlier" reduced-critical re-scan (never changes the model); the
  corrected (Cvtype) critical-value variant.
- **M4 (automatic model selection / `automdl`) STARTED.** Scouted the
  `automd.f` TRAMO flow (`tools/automdl_scouting.md`): call graph, leaf tiers,
  parity risks, first gate = `census-examples/03-automdl.spc`. **24/76 corpus
  specs use automdl** — the biggest single remaining feature. Everything funnels
  through the already-ported rgarma + regvar. Pure-numeric leaf tier landed and
  unit-verified: `chsppf` (chi-square PPF for the AIC-test critical value),
  `gauss`+`chisq` (central-normal / chi-square-upper-tail probs for the
  regressor chi-square test), `sumf`+`smeadl` (span sum + mean-deletion for
  iddiff/amdid). Golden values from `ref_chsppf.f`/`ref_chisq.f` (oracle driven
  directly); `test_numeric` = 86. Remaining automdl leaves
  (chkrt1/chkurt/genrtt/chitst/amdest/iddiff/amdid) are **stateful** (need a
  built ctx.model) — verified through the corpus gate, not microtests; port
  order + approach recorded in the scouting doc §2. (chkrt1/chkurt just mirror
  the verified estimate.cpp:290-370 root-iteration loop.)
- **M4 substrate LANDED — the numeric + model-construction core of automdl**
  (~10h 12m active dev to date). Everything below `automd`'s driver logic is
  now ported and compiling, reusing the M3 roots/olsreg/regvar/rgarma engine:
  - `automdl/idmodel.cpp` — `chkrt1`/`chkurt` (AR/MA root-modulus checkers for
    the unit-root and over-differencing screens), mirroring estimate.cpp's
    setmdl root loop.
  - `automdl/mdlset.cpp` — `mdlint`/`mkmdsn`/`setopr`/`mdlset`: build a trial
    ARIMA model from six explicit order counts (the no-lexer analogue of the
    spec-driven getmdl path), reusing insopr/iscrfn/mkoprt/maxlag/polyml.
  - `automdl/amdest.cpp` — the **Hannan-Rissen initial-estimate engine**:
    `cnvmdl` (model → flat TRAMO orders), `acf` (sample ACF + Bartlett SEs +
    Ljung-Box/Box-Pierce Q into ctx.autoq), `acfar` (AR-filtered autocovs),
    `hrest` (Levinson-Durbin innovations + HR design-matrix OLS via the ported
    olsreg; third-stage HR left disabled per the vendored source), and `amdest`
    (drive one candidate; mixed AR+MA adds an AR-filter + MA-only pass).
  - `totals`/`sdev` numeric leaves (strided sum-avg / std-dev for iddiff's
    mean-significance test) — unit-verified, `test_numeric` = 88.
  - `specparse/gtinpt.cpp` — the automdl parameter defaults (Ub1lim=1/0.96,
    Ub2lim=0.88, Cancel=0.1, Frstar=2, Exdiff=2, Lchkmu/Lmixmd/Lotmod=T, …),
    ported from gtinpt.f 221-267 (were unset while automdl was deferred).
  The stateful routines have no lightweight microtest — they gate through the
  corpus (per the scouting plan). `prterr` is ~all deferred print (only the
  unknown-error branch has a non-print effect).
- **`iddiff` (differencing-order identification) + `prterr` — LANDED & GATED.**
  The first automd driver: the TRAMO/Gomez-Maravall DO WHILE loop that builds
  trial (p 0 0)(P 0 0) models, HR-estimates them (the landed amdest/hrest),
  inspects the AR roots (chkrt1), re-estimates by exact MLE (rgarma) and
  accumulates d/D until the roots leave the unit circle, then the mean-
  significance test. Control flow (GO TO 10/20/30) mapped to a continue-flag +
  break; the Cancel/tolerance/Lextar/Mxiter mutations preserved verbatim. All
  WRITE/Prttab output deferred; `prterr`'s only non-print effect (unknown-error
  → Convrg=F, Var=0) is kept. **Gate:** the new `x13run_iddiff` harness drives
  pre-model + iddiff on `extra/airline_iddiff.spc` (log airline, no regression)
  and returns `idnonseasonaldiff.first=1`/`idseasonaldiff.first=1` — exactly the
  census 03-automdl oracle `.udg`. Exercised end-to-end: mdlint/mdlset →
  amdest/hrest → chkrt1 → rgarma → the differencing loop. `test_m4_iddiff.py`
  locks it; full suite **233 passed / 9 skipped / 11 xfailed**. (Enabling change:
  run_pre_model now stashes the transformed series into ctx.series.tsrs, its
  Tsrs semantics, which estimation later overwrites with the same content.)
- **`amdid` (ARMA-order grid) — LANDED & GATED. Automatic model identification
  now works end to end.** Ported amdid.f/amdid2.f/bestmd.f/mdlmch.f: `amdid2`
  estimates one candidate (mdlint/mdlset + optional HR init + rgarma + prlkhd
  BIC — all reused), `bestmd`/`mdlmch` keep the best-five BIC ranking, and
  `amdid` runs the seasonal(AR3)→regular→seasonal grid over maxorder then the
  BIC-closeness/model-balance tie-break and a final rgarma of the winner. Wired
  the gtauto.f no-arg tail defaults into `gt_automdl` (maxorder=(2,1),
  maxdiff=(2,1), Lautom/Lautod=T; arg VALUES still gt_generic-stubbed), and
  stashed the prior factors into ctx.adj.adj so amdid2's prlkhd reads
  Adj(Adj1st) faithfully. **Gate:** `x13run_iddiff --amdid` on
  `extra/airline_automdl.spc` (log airline, bare automdl{}) → iddiff (1,1) then
  amdid `arimamdl=(0 1 1)(0 1 1)` — the census 03-automdl oracle model.
  `test_amdid_full_model` locks it. Full suite **234 passed**.
- **Multi-series identification GATE + genrtt/chkmu banked.** Gated iddiff+amdid
  against six real automdl corpus specs with no auto-transform/aictest preamble
  (airline/payems/unrate/expgs/span/region_north). **iddiff differencing matches
  the oracle `.udg` on all six**; amdid's full model matches on airline
  `(0 1 1)(0 1 1)`, payems `(0 1 2)`, unrate `(0 1 1)`, span `(0 1 2)` — and its
  best-5 BIC matches the oracle **exactly** for airline (bic1..4 to 3 decimals),
  validating the prlkhd/estimation engine. Ported `genrtt`/`chkmu` (the
  Constant-term test, banked in `automdl/chkmu.cpp`) for the eventual driver.
  `test_m4_iddiff.py` = 8 cases; full suite **240 passed**.
- **amdid matches the oracle on ALL SIX series (gap RESOLVED).** The earlier
  expgs/region miss was a HARNESS bug, not amdid: the harness passed
  `ctx.series.tsrs` as the `Trnsrs` input to iddiff AND amdid, but rgarma
  overwrites Tsrs with residuals during estimation — so iddiff's internal rgarma
  clobbered the series amdid then read, and amdid estimated candidates on
  residuals (expgs `(2 1 0)` scored the residual-fit lnlkhd 526.3 vs the correct
  517.5, flipping the ranking). `Trnsrs` and `Tsrs` are separate buffers in real
  X-13; the harness now copies the series for the identification routines. amdid
  now emits airline `(0 1 1)(0 1 1)`, expgs `(2 1 0)`, payems `(0 1 2)`, unrate
  `(0 1 1)`, span `(0 1 2)`, region `(1 2 2)(0 1 1)` (== oracle `best5.mdl1`).
  `test_m4_iddiff.py` asserts the model for all six. region's `arimamdl
  (3 2 1)(0 1 1)` is set by automd's later adequacy stage (tstmd1/2/odf), a
  separate unported feature. **Lesson for the automd wire: keep the transformed-
  series buffer distinct from ctx.series.tsrs.**
- **Reduced `automd` driver — LANDED. Automatic model selection works end to
  end, bit-exact vs the oracle on 4 series.** `core/src/automdl/automd.cpp` ports
  the automd.f spine for automdl specs with no auto-transform/aictest/outlier
  preamble: default airline model → `chkmu` (mean test) → `iddiff` → `amdid` →
  re-add the mean when significant → final estimate; reuses every ported piece
  and keeps the transformed-series buffer distinct from `ctx.series.tsrs`. Gate
  (`x13run_iddiff --automd`): reproduces the oracle `.udg` **full estimation** —
  arimamdl + variance (rtol 1e-9) + loglikelihood — bit-for-bit on airline
  `(0 1 1)(0 1 1)` (automean=no), expgs `(2 1 0)` (automean=yes), payems
  `(0 1 2)`, unrate `(0 1 1)`: seasonal/nonseasonal, with/without a mean.
  `test_automd_full_estimation` locks the four; suite **244 passed**.
  span-modelspan (model span ≠ series span) and region_north (arimamdl from the
  adequacy stage) are gated on identification only, pending those features.
  Deferred in the reduced driver: the regressor AIC-test family, auto outlier ID,
  pass0, and the tstmd1/tstmd2/tstodf adequacy retry.
- **automd WIRED into run_m2 (production path).** run_pre_model dispatches to
  automd when `ctx.arima.lautom`; the four supported series now produce the
  oracle model + estimation through the standard x13run_m3 harness (converged +
  variance rtol 1e-9 + loglikelihood), and `test_automd_full_estimation` gates
  that real path. Safe for existing gates (M3 excludes automdl/function=auto/
  aictest; M2/M1 don't estimate). A scan of every no-auto automdl corpus spec
  confirms the achievable set is exactly airline/expgs/payems/unrate;
  region_north/south need the adequacy retry (arimamdl (3 2 1)) and span-modelspan
  needs model-span handling.
- **`trnaic` (transform=auto) — LANDED (parallel subagent, merged 5c7591a).**
  Estimates the default airline model untransformed vs log, compares AICC, picks
  the transform; bit-exact vs the oracle on airline/expgs/payems/03-automdl
  (`test_m4_trnaic.py`, 8 tests). Wired into run_m2 (function=auto -> trnaic
  before transforming). **03-automdl now runs end-to-end** (was FATAL): transform
  auto-selects Log, automd identifies (0 1 1)(0 1 1). Its variance still differs
  from the oracle only because the `aictest=(td easter)` regressor selection is
  unported (the oracle's final model carries td+easter; mine is plain airline).
- **Broader parity testing (user-requested).** Added 13 diverse fixed-model
  specs (pure-AR/MA/mixed/seasonal) + oracle goldens -> M3 gate now covers the
  estimation engine across model orders, not just airline (a 23-model
  oracle-vs-C++ sweep confirmed bit-exactness). Added **NSA seasonal datasets**
  from R (nottem/ukgas/co2/usdeaths) since the FRED corpus series are all SA at
  source (network blocked -- see `data/NSA_DATA_DROP.md`): the seasonal automdl
  path (D=1) is now bit-exact on nottem (1 0 0)(1 1 1) / ukgas (1 0 2)(0 1 0) /
  co2 (0 1 1)(0 1 1). **usdeaths exposed a real iddiff bug** (picks d=0 vs oracle
  d=1; scouting 3c). Suite **293 passed**.
- **Next — the remaining automd features.** aictest regressor family
  (tdaic/lomaic/easaic/chkchi) -- needed for 03-automdl's full estimation and any
  `aictest=` spec; auto outlier ID within automd (amidot), pass0, and the
  tstmd1/testodf adequacy retry (fixes usdeaths + region arimamdl). Then pickmdl
  (the X-11-ARIMA alternative). After M4: X-11 and SEATS (the seasonal-adjustment
  engines -- not started). A full end-to-end
  automdl estimation gate vs the oracle `.udg` still needs: (a) resolve the
  exact-AR gap; (b) `transform=auto` selection (autotrans/aictrans — 03-automdl
  uses it; independently gate-able via `aictest.trans.aicc.*`); (c) the regressor
  AIC-test family (tdaic/easaic/chkchi) for `aictest=(td easter)`; (d) the
  model-adequacy tests (tstmd1/tstmd2 via the ported chkurt / tstodf) + retry
  loop; (e) the `automd` driver (1038 lines) wiring it into run_m2 behind
  `automdl{}` (default model → chkmu → iddiff → amdid → finalize; genrtt/chkmu
  ready). Also open: deferred M2 regressor branches and the .out/.fct print
  engine. See `tools/automdl_scouting.md` (§2 status table + §3b).
- **Packaging scaffolding (r-pkg/py-pkg) built by an agent, parked on branch
  `worktree-agent-ae1edc2bc3b17f4d4`** (NOT merged; merge after the main
  milestones). R CMD check / twine check clean; datasets bundled; result-object
  API honors the no-auto-file-output rule. See second-brain
  `x13cpp-pending-pkg-merge`.

## Session — M4 complete + M5 (X-11/SEATS) started (~14h active, 132 commits)

Large multi-agent session. Landmarks (all bit-exact vs oracle unless noted):

- **M4 automatic model selection — the identification engine is DONE + GATED.**
  iddiff (differencing) + amdid (ARMA orders) + automd driver wired into run_m2 +
  trnaic (transform=auto). Produces the oracle model + estimation end-to-end on
  airline/expgs/payems/unrate/nottem/ukgas/co2. `aictest` family (tdaic/easaic)
  ported + gated (all `aictest.diff.td/e` match). Adequacy routines
  (mdlchk/tstmd2/testodf/bkdfmd/tstmd1) ported but BANKED — wiring needs the full
  automd finalization as one unit (tstmd1 alone perturbs the 4 non-revert cases;
  FABLE_REVIEW §1). usdeaths/region final model traced to tstmd1 revert.
- **M5 seasonal adjustment — leaves STARTED (dirs were empty).** X-11 Tier 0–3:
  mode arithmetic, Henderson trend chain, seasonal-MA (vsfa/vsfb/vsfc),
  extreme-value (xtrm chain) — `core/src/x11/`, 27 unit tests. SEATS: poly
  arithmetic + the **C02AEF root finder verified bit-exact** (the #1 SEATS parity
  hinge) + RPQ — `core/src/seats/`, 12 unit tests. Driver spines (x11pt*, PARFRA/
  MAK1) not yet ported.
- **Bug fixed: user-specified outlier regressors** (`regression{variables=(ao…)}`
  AO/LS/TC/RP) were FATAL, now bit-exact (getreg_vars wired to rdotlr/addotl;
  user-TC defaults tcalfa). AOS/LSS still deferred (rdotls unported).
- **Formal code coverage** (`tools/coverage.ps1`, gcov/gcovr): **73.1% lines,
  89.6% functions** over core/src. `adqtst.cpp` 0% (banked, uncalled).
- **~50 new parity specs**: diverse ARIMA orders, NSA seasonal series (R
  datasets), regression/outlier effects, transforms, prior adjust, exact-ML,
  span, forecast. Suite **244 → 365 passed**.
- **Census bugs logged**: CB-7 (endsf double-divide), CB-8 (sdxtrm stale loop var).
  **Found gaps**: labor/thank holiday regressors FATAL (adlabr/adthnk unported).
- **Next**: wire the automd finalization (fixes usdeaths/region + 03-automdl full
  estimation via aictest); the X-11 `x11pt*` decomposition spine (first gate
  airline_x11-default D-tables); SEATS PARFRA/MAK1 decomposition; labor/thank +
  AOS/LSS + lomaic/usraic follow-ups. See tools/{automdl,x11,seats}_scouting.md,
  TEST_COVERAGE.md, FABLE_REVIEW.md.

_Update this snapshot by pasting fresh `python tools/worklog.py` output; the git
timeline is the authority._
