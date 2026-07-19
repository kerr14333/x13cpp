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

## Snapshot — 2026-07-19 18:24 EDT

| metric | value |
|---|---|
| start (first commit) | 2026-07-18 14:06 EDT |
| latest commit | 2026-07-19 18:24 EDT |
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
- **Next — wire fcstxy into run_m2** (forecast{} path → transformed forecasts,
  then the inverse-transform + prediction-interval step and the .fct/.ftr output
  the packaging must expose from the object, not a file). Then outlier detection
  (idotlr/rdotlr AO/LS/TC scan), the deferred M2 regressor branches, and the
  .out print engine. See `tools/m3_scouting.md` §5 Tier-6/7 and §7.

_Update this snapshot by pasting fresh `python tools/worklog.py` output; the git
timeline is the authority._
