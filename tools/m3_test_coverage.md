# M3 regARIMA — test coverage plan

Consolidated from three Fable research passes (model-structure, corpus-series,
spec-settings), 2026-07-19. Drives which ref-driver cases and corpus specs to
add. Ranked by parity risk. Check items off as landed.

## A. `armafl` / filter model-structure cases (testable NOW — all leaves ported)

These need only a `tools/ref_*.f` driver that hand-builds the model in the
model/mdldat commons + a matching `test_numeric` case. Highest risk first.

- [x] **A1. Airline (0 1 1)(0 1 1)₁₂** — THE production model. Exercises the DIFF
  block in `arflt` (Mxdflg=13, untested), sequential regular→seasonal
  `ratpos`/`ratneg` in `intgpg` (order-dependent FP), 13×13 packed `Chlgpg`,
  `exctma` with neltq=13. θ₁=0.6, Θ₁₂=0.5; diff ops (lag1 & lag12, coef 1.0
  fixed). Nr≈40.
- [x] **A2. Multi-column Nc=3, ARMA(1,1)** — `rgarma` calls `armafl(Nspobs,Ncxy,…)`
  on the whole [X:y] each IGLS iter, so Nc>1 is the normal case, tested nowhere.
  Exercises `Arimal*=Nc` scale/restore, overlapping `copy(…,-1,…)` shift,
  multi-RHS `dsolve`/`exctma`, strided `ddot(…,Nc)` unequal-increment branch.
- [x] **A3. chkrts FMA boundary** (THE canary) — `1-c²` is FMA-contractible; a
  build without `-ffp-contract=off` flips the ≤0 invertibility decision within
  1 ulp of |θ|=1. Cases: θ=1.0 (cfncsq=0→non-inv); θ=1-1e-16 (inv); degree-2
  (0.5,0.5) intermediate coef>1; seasonal Θ=1.0 lag12 fac12 (degree=12/12=1).
- [x] **A4. Error-path Info codes, Lckrts=F** (rgarma's calling convention) — AR
  φ=1.05 → PACFER=12 (assert partial Mata too); fixed φ=1.0 → Inf/NaN → PVWPER=13;
  fixed θ=1.2 → PGPGER=11. rgarma/fcnar branch on the exact code; Inf/NaN
  propagation must NOT be "cleaned up".
- [x] **A5. Seasonal AR (0 1 0)(1 0 0)₁₂** — pure-AR exact path (untested `ELSE
  IF(Lar)` Chlvwp=acv fill), sparse fular (zeros lags 1..11), `euclid` Mxmalg=0
  branch ×12, `Lndtcv` accumulation on intgpg's Lma=F zero.
- [x] **A6. Mixed (1 0 1)(1 0 1)₁₂** — `mltpos` secpas on real data (fular lags
  {1,12,13}), largest D machinery (13×13 Σp−D′D), regular-then-seasonal order FP.
- [x] **A7. Linit=F reuse** — init on Nr=8, re-call `armafl(12,…,.false.,…)`:
  SAVE nextma recompute with new Nr while Matd/Chlgpg hold old factorization;
  ddot reads zero-init Matd tail; Lndtcv NOT re-accumulated. This is exactly
  what forecasting (`fcstxy`) and outlier detection (`idotlr`) do.
- [x] **A8. q>p ARMA(1,2)** + companion ARMA(2,2) — opposite `euclid` branch mix
  from the covered AR(2)MA(1); D-loop `max(1,Mxarlg-row+1)` clamp.
- [x] **A9. ratneg exact-zero staleness** — engineer `sum==0` (MA θ=0.5,
  C=[-1,2,0,…]) so C(i) keeps its OLD value (no write); a naive port writes 0.
- [x] **A10. ratpos/ddot underflow-skip in anger** — MA θ=0.1 len 170 (θ^k cross
  1e-150); θ=1e-160 (whole term skipped, first-order visible diff).
- [x] **A11. Sparse lags [2 4]** and decreasing [4 2] — chkrts degree-recompute,
  ratpos multi-lag begelt branch, maxlag non-monotone scan. Most off-by-one-prone.
- [x] **A12. Pure differencing (0 1 0)(0 1 0)₁₂** and (0 2 0) — Lar=Lma=F, both
  Linit branches false, empty-range DO loops, `Nopr` global side-effect write.
- [x] **A13. Partially-fixed operator** — pins that Arimaf has NO effect on filter
  numerics, only on the chkrts gate.
- [x] **A14. Nopr=0 no-op** — Na=Nr, Mata untouched, Info=0. One-liner guard.

Covered already: nonseasonal ARMA(1,1), AR(2)MA(1) (full armafl); MA(2)
intgpg+exctma; chkrts degree-1/2/non-inv/all-fixed; olsreg/resid/upespm.

## B. Corpus specs — estimation coverage (need olsreg/rgarma/lmdif landed first)

The corpus (76 specs) is broad on *pipeline* but narrow on *estimation*: one
fixed model shape (0 1 1)(0 1 1), four benign regressors, no AR, no fixed params,
no outlier regressors, no negative/near-zero data. Tier A = new specs on existing
series (no new data):

- [ ] **B1. `estimate{exact=none}` and `exact=ma`** — the only switch for armafl's
  conditional-vs-exact likelihood (Lextar/Lextma); every current spec is default
  `exact=arma`. Airline + one AR model.
- [ ] **B2. arima model sweep** — (2 1 0), (1 1 1)(0 1 1), (0 2 2), (1 0 0)(1 0 0)
  +const, 3-factor, lag-gap (0 1 [1 3]). Corpus tests exactly one lag structure.
- [ ] **B3. Fixed coefficients** `ma=(0.4f,0.55f)`, fully-fixed model (mdlfix
  evaluate-only), partially-fixed (changes lmdif param-vector length).
- [ ] **B4. `estimate{tol=,maxiter=}`** — forced non-convergence exit path;
  nltol defaulting (tol-given-not-nltol → Nltol0=100*Tol).
- [ ] **B5. PAYEMS + explicit COVID outliers** `regression{variables=(ls2020.mar
  ls2020.apr ao2020.apr td)}` — the currently-abending regvar branch; dominant-LS
  beta stresses IGLS. (needs unported outlier regvar branches → strict-xfail now.)
- [ ] **B6. `forecast{maxback=12}`** — backcast span extension, distinct filter
  direction; no corpus spec sets maxback.

Tier B = new FRED series (via existing Wayback-pinned `fetch_fred.py`, no auth;
avoid bls.gov 403s). First two to add:

- [ ] **B7. RSXFSN** (Retail Sales ex Food, NSA monthly ~400obs) — real TD+Easter
  signal (significant betas → exercises tdaic/easaic decisions) + moving
  seasonality. Best single addition.
- [ ] **B8. FEDFUNDS** (monthly, values to 0.05) — log-of-near-zero edge, auto
  transform log-vs-none decision, essentially nonseasonal.
- [ ] B9. HOUSTNSA (volatile weather seasonality, AR-heavy automdl); BOPGSTB
  (uniformly negative → transform must reject log).

## C. Estimation/forecast parity INFRASTRUCTURE (beyond hand-built ref drivers)

Tier A/B verify individual routines/models via `ref_*.f` drivers or new specs.
These items are about *systematic* end-to-end coverage — the analog of the M2
save-file parity gate, plus branch gaps in the routines already landed.

- [ ] **C1. Estimation corpus parity gate (highest value).** A new `x13run_m3`
  CLI binary (run_m2 with `estimate=true`, plus fcstxy when `forecast{}` present)
  + a pytest that sweeps EVERY corpus spec with an arima model and compares
  `arima.ar`/`arima.ma`/`variance`/`loglikelihood`/`aic..hq`/`nfev`/`niter`
  against the `.udg` goldens (via x13compare). Today estimation is checked on
  exactly 2 hand specs. This subsumes B1/B2 breadth automatically (quarterly, AR,
  mixed models already in the corpus get checked for free) and catches
  regressions corpus-wide. Depends on: the "no auto file output" rule (results
  read off the object/captured state, opt-in write).
- [ ] **C2. Fixed-coefficient spec — pins live bug CB-6.** `armats` misindexes
  t-stats when an ARMA lag is held fixed (`itv` counts every lag, Armacm packed by
  free params). Ported verbatim but NEVER exercised on a real fixed-coef spec. A
  `model=(0 1 1)` with a held MA coef (`ma=(0.4f)` / `exact=none`) pins the
  faithful-but-buggy behavior end-to-end. Overlaps B3; C2 is specifically the
  armats-index assertion.
- [ ] **C3. `prlkhd` branch coverage.** Only log+airline (jacadj=-Σlog y) tested.
  Untested: `Var<=0` (returns, no likelihood); non-exact-ML `lclaic=F` (Olkhd set
  but Aic/Aicc/etc NOT computed); logit `jacadj` (Fcntyp=3); prior-adjustment
  `jacadj` with `Adjmod>=2` (the second jacadj loop); `Eick>0` → Eic computed.
- [x] **C4. `fcstxy` differencing branch.** DONE via `ref_fcstxy3.f` -- (0 1 1)
  differenced model (mxdfar=1) exercises the tfcst-seeding-from-the-last-row +
  ndltar-offset recursion. All three fcstxy branches (Nb=0/no-diff, Nb>0/dppsl,
  differenced) now covered. Real airline (mxdfar=13) still comes for free once C1
  wires fcstxy into run_m2.
- [ ] **C5. Determinism re-gate with estimation.** The M0 O0-vs-O2 determinism
  sweep predates all M3. The `nfev`/`nliter` trajectory canary is FP-sensitive
  (lmdif secant path) — re-run the determinism gate with estimation+forecast in
  the pipeline to confirm optimizer trajectories stay bit-identical across opt
  levels (and, later, compilers). Guards against silent FP-contraction regressions
  that the rtol-1e-12 unit tests would still pass.
- [ ] **C6. `xrlkhd` on a real spec.** The AIC-test-path AICC (jacadj-free) is
  unit-tested on the hand ARMA(1,1) only; not exercised through a spec that would
  drive automatic model/transform AIC decisions (needs automdl/trnaic, later).

## Cross-cutting parity invariants (assert in every relevant ref driver)

- **Operator order load-bearing**: regular factor before seasonal within each
  type block; DIFF before AR before MA. All expansions are sequential per-op → FP
  depends on order.
- **`-ffp-contract=off`** on both sides. chkrts (1−c²) / euclid (1−r²) / uconv /
  all inner products are the FMA-sensitive spots. A3 is the canary.
- **Assert mutated commons, not just Mata**: armafl scales Arimal by Nc (and
  Mxarlg for the D pass) and restores by integer division; exctma writes Nopr;
  intgpg/armafl write Lndtcv/Chlgpg/Chlvwp/Matd/Prbfac. SAVE nextma persists.
- **Don't "clean up" the math**: Inf/NaN propagation (A4), exact-zero no-write
  (A9), underflow-skip (A10) must match bit-for-bit.
