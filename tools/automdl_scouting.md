# automdl Scouting Report — Automatic ARIMA Model Selection (M4)

Scouted 2026-07-19 against `oracle/fortran` (v1.1 b61). Scope: the `automdl{}`
TRAMO-style automatic model identification driven by `automd.f`. Sibling
`pickmdl{}` (X-11-ARIMA method, `automx.f`) is a separate, later milestone.

Corpus payoff: **24 / 76 specs use `automdl`** (5 use `pickmdl`). Everything
funnels through `rgarma` + `regvar`, both already ported and oracle-verified
(see `m3_scouting.md` §7). automdl is orchestration + a handful of new numeric
leaves on top of that engine.

## 1. Entry point & algorithm

```
arima.f            CALL automd(Trnsrs,Frstry,Nefobs,A,Na,Lsumm,Lidotl,...)
automd.f:2         SUBROUTINE automd   (1038 lines) — the driver
```

TRAMO / Gomez-Maravall (2000) flow inside `automd`:
1. Init default model (mdlint/mdlset → airline (0 1 1)(0 1 1)), ssprep.
2. **Regressor AIC tests on the default model**: tdaic (trading day), lomaic
   (length-of-month/leap-year), easaic (Easter), usraic (user regressors),
   chkchi (chi-square group test), chkmu (mean significance). Each estimates
   with/without a regressor group via rgarma and compares AICC.
3. Estimate default model (rgarma); optional automatic outlier ID (amidot →
   idotlr, already ported); pass0 (initial ACF-based screen).
4. Save default-model state (a0/adj0/trns0).
5. **iddiff** — identify differencing orders (d, D) by unit-root / mean tests.
6. **amdid** — identify ARMA orders (p,q,P,Q) at the chosen differencing via a
   BIC grid search (amdid2 estimates each candidate; bestmd tracks the winner).
7. Rebuild regressors (regvar), re-estimate, redo the AIC tests.
8. **Model-adequacy tests** (loop back if they fail): tstmd1 (Ljung-Box + final
   checks), tstmd2 (unit-root nearness via chkurt), tstodf (over-differencing).
9. Finalize: compare identified vs default (chkchi/bkdfmd), pick, mkmdsn builds
   the model designation string.

## 2. Leaf port status (checked against core/src, 2026-07-19)

Already ported (reused): rgarma, regvar, adrgef, dlrgef, dppdi, dscal, armats,
arflt (armafilt), prlkhd, setmdl, setint, idotlr, eltfcn, copy, mdlint.

New leaves needed (leaf-first tier order):

Pure-numeric leaf tier: **COMPLETE** (all unit-verified in test_numeric).
Everything below is stateful (needs a built ctx.model) — verify through the
corpus gate, not microtests.

| Tier | leaf        | lines | role                                        | status |
|------|-------------|-------|---------------------------------------------|--------|
| 0    | chsppf      | 223   | chi-square PPF (AIC-test crit value)        | **DONE** f318ca6 |
| 0    | sumf+smeadl | 9+17  | series sum + mean-deletion of a span        | **DONE** 7029e70 |
| 0    | gauss+chisq | 60+60 | normal/chi-square probs (chitst)            | **DONE** f72424c |
| 0    | totals+sdev | 51+46 | strided sum-avg / std-dev (iddiff mean test)| **DONE** f822257 (unit-tested) |
| 0*   | genrtt      | 76    | mean/regression t-stat (chkmu) — STATEFUL   | todo   |
| 0*   | chitst      | 61    | chi-square group stat (chkchi) — STATEFUL   | todo   |
| 1    | chkrt1      | 93    | AR/MA root moduli (iddiff) — STATEFUL       | **DONE** f822257 (automdl/idmodel.cpp) |
| 1    | chkurt      | 100   | unit-root nearness (tstmd2) — STATEFUL      | **DONE** f822257 (automdl/idmodel.cpp) |
| 1    | cnvmdl      | 60    | model-order -> flat TRAMO order variables   | **DONE** c35bd48 (automdl/amdest.cpp) |
| 1    | mdlchk      | 74    | model validity check (Ljung-Box, needs acf) | todo (adequacy) |
| 1    | mdlint/mdlset/mkmdsn/setopr | 33+208+43+122 | programmatic model construction | **DONE** b7d4814 (automdl/mdlset.cpp) |
| 2    | bestmd/bestget| 46  | best-model bookkeeping (amdid grid)         | todo   |
| 2    | acf/acfar/hrest | 143+59+227 | sample ACF + Hannan-Rissen regression | **DONE** c35bd48 (automdl/amdest.cpp) |
| 2    | amdest      | 102   | estimate-one-candidate wrapper (HR)         | **DONE** c35bd48 (automdl/amdest.cpp) |
| 2    | bkdfmd      | ~     | back up / restore default model             | todo   |
| —    | gtinpt automdl defaults | | Ub1lim/Ub2lim/Cancel/Frstar/Exdiff/... | **DONE** 7cdb252 |
| 3    | iddiff      | 576   | differencing-order identification driver    | **DONE** 322af67 (gated (1,1) on log-airline) |
| 3    | prterr      | 335   | estimation-error report (non-print core)    | **DONE** 322af67 |
| 3    | amdid+amdid2| 411+104| ARMA-order identification driver           | **DONE** c4461b7 (full model (0 1 1)(0 1 1) gated) |
| 2    | bestmd/mdlmch | 46+30 | best-5 BIC ranking + dedup                | **DONE** c4461b7 |
| 3.5  | gtauto tail defaults | | maxorder/maxdiff/lautom (no-arg)         | **DONE** c4461b7 (arg VALUES still stubbed) |
| 3.5  | gtauto args | ~450  | real automdl{} arg parser (replaces gt_generic stub) | todo |
| 4    | tdaic/lomaic/easaic/usraic | | regressor-AIC-test family (rgarma) | todo |
| 4    | genrtt/chkmu | 76+117| regressor t-stats / mean (Constant) test    | **DONE** (banked; automdl/chkmu.cpp) |
| 4    | chkchi      | 74    | regressor chi-square test                   | todo   |
| 5    | tstmd1/tstmd2/tstodf | | model-adequacy tests + retry loop       | todo   |
| 6    | pass0/pass2 | ~     | TRAMO ACF pre-screen passes                 | todo   |
| 7    | automd      | 1038  | the driver; wire into run_m2 behind automdl{} | todo |
| —    | amdprt/prtamd/prtmsp/prtnfn | | print engines — DEFER (like fcstout) | defer |

### Landed (M4 substrate — the numeric + construction core)

- `core/src/automdl/idmodel.cpp` — chkrt1, chkurt (AR/MA root checkers).
- `core/src/automdl/mdlset.cpp` — mdlint, mkmdsn, setopr, mdlset (build a trial
  model from six order counts, no lexer; reuses insopr/iscrfn/mkoprt/maxlag).
- `core/src/automdl/amdest.cpp` — cnvmdl, acf, acfar, hrest, amdest (the
  Hannan-Rissen initial-estimate engine — the hard numeric core; reuses
  olsreg/arflt/copy/chisq).
- `core/src/specparse/gtinpt.cpp` — automdl parameter defaults.
All compile clean and reuse the ported roots/olsreg/regvar/rgarma engine.
`prterr` (335 lines) is almost all deferred print — the only non-print effect
(unknown-error branch: Convrg=F, Var=0) is a ~3-line port when iddiff needs it.

### GATING NOTE — iddiff cannot be unit-gated in isolation

Stateful driver routines have no lightweight ctx.model builder, and the first
corpus fixture (`03-automdl.spc`) feeds iddiff through a `transform=auto` +
`aictest=(td easter)` preamble — iddiff's (d,D) is only reachable after that.
**iddiff + amdid + a minimal automd driver must be gated together** against
`03-automdl`'s oracle `.udg`, which reports iddiff's decision as
`idnonseasonaldiff.first: 1` / `idseasonaldiff.first: 1`, the amdid BIC grid as
`automdl.best5.mdl1..5`, and the final model as `arimamdl: (0 1 1)(0 1 1)`.
Plan: port iddiff (+prterr) + gtauto + amdid, wire a minimal automd into run_m2
behind `automdl{}`, then gate the whole chain on 03-automdl in one step.

## 3. Parity risks (automdl-specific; general M3 risks in x13cpp-ideas)

- **rgarma call-count bookkeeping**: automd calls rgarma many times; the
  cumulative Nliter/Nfev and the SAVE-state carried across calls (stpitr oldobj,
  armafl nextma) must match the oracle's sequence exactly or trajectories
  diverge. Re-verify counters at THIS milestone (see x13cpp-ideas M4+ note).
- **AIC-test tie-breaking**: `Dfaict.gt.Rgaicd` (strict >) vs `.not.(...)` — the
  default vs candidate choice hinges on exact float comparison of AICC diffs.
  With Pvaic=DNOTST the threshold Rgaicd is a fixed default (chsppf NOT called).
- **iddiff / amdid grid order**: candidate models are tried in a fixed order;
  BIC ties resolve to the first/lowest-order model. Preserve iteration order.
- **chkurt near-unit-root**: MALIM=0.001 threshold on summed MA coefficients
  decides overdifferencing — exact-equality-sensitive.
- **regvar rebuilds mid-loop**: automd toggles regressor groups and re-runs
  regvar; the [X:y] column order must match what each AIC test expects (same Nb
  provenance concerns as m3_scouting §7).

### Next session — stateful iddiff/amdid subtree

The pure leaves are banked. The next unit is the **differencing/ARMA-order
identification core**, ported as a group and verified end-to-end (no microtests
for stateful routines — there is no lightweight ctx.model builder; construct
state via the real getmdl/regvar front end and diff the identified orders vs the
oracle `.udg`). Suggested order:
1. chkrt1 + chkurt (root-modulus checkers) — trivial once you mirror the
   estimate.cpp:290-370 loop; `roots(ctx,...)` already ported.
2. amdest (+amdprt deferred) — the single-candidate estimate wrapper.
3. bestmd/bestget — best-model bookkeeping for the grid.
4. iddiff driver — differencing selection; verify d/D against oracle first
   (isolate before ARMA-order search).
5. amdid + amdid2 — the BIC ARMA-order grid.
Gate incrementally: iddiff's chosen (d,D) is a discrete decision — check it
exactly against the oracle before wiring amdid.

## 3b. amdid BIC parity — expgs/region gap DIAGNOSED (D=0 candidate estimation)

iddiff differencing matches the oracle on all six no-preamble corpus series;
amdid's full model + best-5 BIC match the oracle **exactly** for airline
(`automdl.best5.bic1..4` identical to 3 decimals). Two series still miss the
ARMA orders — expgs `(0 1 2)` vs `(2 1 0)`, region_north `(1 2 2)(0 1 1)` vs
`(3 2 1)(0 1 1)`. Both are **D=0** (no seasonal differencing); airline (D=1)
matches. Findings:

- **NOT the mean.** `X13_AMDID_DEBUG=1` shows `Nb=0, Ngrp=0` in the amdid grid
  for BOTH expgs and airline — no constant is present when candidates are
  estimated (the oracle likewise removes it before the grid; automd re-adds it
  only AFTER amdid, automd.f:479). `chkmu`/`genrtt` are ported
  (`automdl/chkmu.cpp`) but a chkmu preamble did not change the result.
- **The candidate ARMA estimation itself differs for D=0.** For expgs (quarterly,
  nefobs=316) our amdid scores `(2 1 0)` at BIC −3.276 → lnlkhd≈523.4 (dnp=2),
  while the oracle's best-5 `(2 1 0)` is −3.221 → lnlkhd≈514.7, and a *standalone*
  x13run_m3 estimate of `(2 1 0)` gives 517.5 — three different likelihoods for
  the same model/series. So rgarma is producing a different exact-ML optimum
  inside the amdid path (Lextar/Lextma=T, Nintvl=Mxdflg, Tol loosened to 1e-3)
  than the oracle for the no-seasonal-difference case. Airline's exact-ML path
  matches, so the divergence is specific to D=0/quarterly setup — likely the
  Nintvl/Nextvl or starting-value state amdid inherits. Resolve by diffing
  rgarma's exact-ML state for a single expgs candidate against the oracle when
  the full `automd` driver is assembled. `X13_AMDID_DEBUG=1` dumps best-5 + grid
  Nb/Ngrp/id/Sp.
- **Lead: the exact-AR likelihood path.** amdid forces `Lextar=Lextma=T` (exact
  ML). Airline's winner is pure-MA `(0 1 1)(0 1 1)` — exercises exact-**MA** and
  matches. expgs `(2 1 0)` and region `(3 2 1)…` are AR-heavy — exercise
  exact-**AR**. First debugging step next session: estimate a single expgs
  `(2 1 0)` candidate with `Lextar=T` and diff rgarma's exact-AR likelihood
  against the oracle (check whether the M3 corpus already covers any pure-AR
  exact-ML model; if not, the exact-AR branch may be under-tested).

## 4. First corpus gate target

`tests/corpus/census-examples/03-automdl.spc` — the canonical automdl example.
Compare identified model orders + final estimates against its oracle `.udg`
(mdl orders, niter/nfev, coefficients, variance, loglikelihood/AIC). Add to the
M3 estimation corpus gate once automd is wired.

## 5. Build/run reminder

Build & test ONLY via `powershell -File tools/build.ps1` (handles the rtools44
`ld` DLL-PATH requirement — otherwise `ld returned 9`). Run test exes from
PowerShell, not `./x.exe` in Git Bash (msys "Exec format error").
