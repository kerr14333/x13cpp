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
| 3    | iddiff      | 576   | differencing-order identification driver    | **NEXT** |
| 3    | amdid+amdid2| 411+104| ARMA-order identification driver           | todo   |
| 3.5  | gtauto      | ~450  | real automdl{} arg parser (replaces gt_generic stub) | todo |
| 4    | tdaic/lomaic/easaic/usraic | | regressor-AIC-test family (rgarma) | todo |
| 4    | chkchi/chkmu | 74+?  | regressor chi-square / mean tests           | todo   |
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

## 4. First corpus gate target

`tests/corpus/census-examples/03-automdl.spc` — the canonical automdl example.
Compare identified model orders + final estimates against its oracle `.udg`
(mdl orders, niter/nfev, coefficients, variance, loglikelihood/AIC). Add to the
M3 estimation corpus gate once automd is wired.

## 5. Build/run reminder

Build & test ONLY via `powershell -File tools/build.ps1` (handles the rtools44
`ld` DLL-PATH requirement — otherwise `ld returned 9`). Run test exes from
PowerShell, not `./x.exe` in Git Bash (msys "Exec format error").
