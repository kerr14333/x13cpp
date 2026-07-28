# automdl Scouting Report — Automatic ARIMA Model Selection (M4)

Scouted 2026-07-19 against `oracle/fortran` (v1.1 b61). Scope: the `automdl{}`
TRAMO-style automatic model identification driven by `automd.f`. Sibling
`pickmdl{}` (X-11-ARIMA method, `automx.f`) is a separate, later milestone.

Corpus payoff: **33 of 366 specs use `automdl`** (1 uses `pickmdl`; counts
refreshed 2026-07-28 — the 2026-07-19 figures were 24/76 and 5). Everything
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
| 5    | mdlchk      | 74    | Ljung-Box + residual-mean stat (shared)     | **DONE** d3aa598 (banked, automdl/adqtst.cpp) |
| 5    | tstmd2      | 147   | unit-root-nearness parameter reduction      | **DONE** a371c74 (banked, automdl/adqtst.cpp) |
| 5    | tstmd1/testodf | 268+297 | LB adequacy / over-diff -- need bkdfmd, ssprep, sftest, amidot/clrotl + the automd nloop retry wire | todo |
| 6    | pass0/pass2 | ~     | TRAMO ACF pre-screen passes                 | todo   |
| 7    | automd (reduced) | 1038 | driver spine (default→chkmu→iddiff→amdid→mean→final) | **DONE** 5ca900e (4 series bit-exact est.) |
| 7    | automd run_m2 wire | | dispatch to automd when lautom (production path) | **DONE** 08059a8 |
| 7    | automd (full) | | AIC tests + outlier + pass0 + adequacy retry | todo |
| —    | trnaic      | 436   | transform=auto selection (aictrans) — unblocks 03-automdl | **NEXT** |
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

## 3b. amdid parity — RESOLVED. amdid matches the oracle on all six series.

iddiff differencing matches the oracle on all six no-preamble corpus series, and
amdid's identified model now matches the oracle's BIC winner for all six too:
airline `(0 1 1)(0 1 1)`, expgs `(2 1 0)`, payems `(0 1 2)`, unrate `(0 1 1)`,
span `(0 1 2)`, region_north `(1 2 2)(0 1 1)` (== the oracle `automdl.best5.mdl1`).

**Root cause of the earlier expgs/region miss = a HARNESS bug, not amdid.** The
harness passed `ctx.series.tsrs` as the `Trnsrs` argument to both iddiff and
amdid, but rgarma overwrites Tsrs with regression residuals during estimation
(estimate.cpp resid→s.tsrs). So iddiff's internal rgarma clobbered the series
amdid then read, and amdid estimated its candidates on residuals — e.g. expgs
`(2 1 0)` came out at lnlkhd 526.3 (residual-fit) instead of the correct 517.5,
flipping the `(2 1 0)` vs `(0 1 2)` ranking. In real X-13 `Trnsrs` (transformed
series) and `Tsrs` (rgarma working/residual vector) are SEPARATE buffers; the
harness now hands iddiff/amdid their own copy. Misleading intermediate readings
(the "Nb=0 / exact-AR" leads) were all artifacts of the clobbered series.

- region_north's `arimamdl (3 2 1)(0 1 1)` differs from its `best5.mdl1
  (1 2 2)(0 1 1)` because automd's **model-adequacy stage** (tstmd1/tstmd2/tstodf)
  revises the BIC winner after amdid — a later automd feature, not an amdid
  discrepancy. amdid correctly emits the BIC winner.
- `X13_AMDID_DEBUG=1` dumps amdid's best-5 and per-candidate lnlkhd/bic2/nb.
- Lesson for the run_m2 automd wire: keep the transformed-series buffer distinct
  from ctx.series.tsrs (rgarma owns Tsrs).

## 3c. usdeaths / region final model = the unported ADEQUACY stage (NOT a bug)

NSA seasonal testing (R `datasets`, dropped into the corpus) validates the
seasonal automdl path bit-for-bit on nottem `(1 0 0)(1 1 1)`, ukgas
`(1 0 2)(0 1 0)`, co2 `(0 1 1)(0 1 1)` — arimamdl + variance + loglikelihood all
match the oracle. **usdeaths** (US monthly accidental deaths, 72 obs) differs on
the FINAL model only, and it is NOT an iddiff/amdid bug — the oracle `.out` trace
confirms our engine is correct through identification:

```
Results of Unit Root Test ... Regular 0, Seasonal 1     <- iddiff (mine matches)
Automatic model choice : (1 0 1)(0 1 1)                 <- amdid  (mine matches EXACTLY)
Checking for Unit Roots. No unit roots found.           <- tstmd2
Checking for nonseasonal overdifferencing.              <- testodf
Final automatic model choice : (0 1 1)(0 1 1)           <- testodf REWRITES the model
```

The rewrite is **`tstmd1`** (NOT testodf — testodf's regular branch needs ldr>0
and the amdid model has ldr=0). tstmd1 compares the identified model to the
DEFAULT airline model and, on any of five adequacy conditions (`ichk=1..5`),
**reverts to the airline default** `(0 1 1)(0 1 1)` (idr=ids=iqr=iqs=1,
ipr=ips=0; ids=iqs=0 for Sp=1). usdeaths hits **`ichk=5`/`ichk=4`**: the model is
`(1 0 1)(0 1 1)` (idr=0,ids=1,ipr=1,ips=0,iqr=1,iqs=1) and its AR(1)
`Arimap(2)` is near-unit (>=0.82, tstmd1.f:167-174) → revert to airline default.
So it is a near-unit AR root being recognized as the airline model. region_north
is the same routine (a different ichk / insignificant-lag reduction).

### UPDATE 2026-07-28 — every leaf is ported; the blocker is the WIRING

**This section's original "Fix =" plan is done and was not sufficient.** All of
`mdlchk`, `tstmd2`, `testodf`, **`bkdfmd`** (`adqtst.cpp:269`) and **`tstmd1`**
(`:344`, which already calls bkdfmd at `:476`) are ported. What is missing is
the *finalization wiring*, and the reason it is still missing is recorded at
`automd.cpp`'s closing note: **wiring `tstmd1` alone broke parity on the
non-revert cases, because the oracle RE-ESTIMATES after `tstmd1`** — so
tstmd1's intermediate fit must not be the reported one. The stage has to land
as a unit: tstmd1 revert + redomd/testodf finalization + the final re-estimate.

**The blast radius is much larger than this section knew.** It is not only
usdeaths/region's final model. The same stage (`automd.f:577-850`) plus the
`pass2`/nloop call at `:665` is what blocks the entire remaining `automdl{}`
option surface — see `tools/dropped_options_scouting.md` round 3:

* three arguments are consumed **only** there and are now FATAL rather than
  silently returning the default model: `urfinal` (`chkrt1` at `:717`),
  `noautooutlier` (`tstmd1` at `:577`), `ljungboxlimit` (`Pcr` at
  `pass2.f:164-169`, which *increments* it);
* five more parse correctly and still land somewhere other than the oracle:
  `mixed`, `checkmu`, `maxorder`, `maxdiff`, `cancel`.

That those five share this cause is a **hypothesis, not a measurement** —
isolate it, and re-measure each argument separately after the stage lands
rather than declaring the group closed.

Both usdeaths and region remain gated on identification only until it lands.

### UPDATE 2026-07-28b — the blocker was a BRANCH, not a missing port

The hypothesis above was **half right, and the framing was wrong**. There was no
unported "stage". `automd_finalize_tail` — `mdlchk` / `pass0` / `chkrt1`+redomd /
`testodf` / the residual-mean Constant add / the `tstmd2` lag-drop loop, i.e.
`automd.f:654-983` — **was already ported and already bit-exact.** It was simply
unreachable on the plain path.

`automd.f` has **one** path: label 10 → iddiff → amdid → label 40 → label 30 →
label 70, with the AIC tests as conditional *blocks* inside it. This driver
split them into an `if (aic)` branch and a non-aic branch and called the tail
from the aic branch only (`automd.cpp`, sole call site). So every plain
`automdl{}` spec skipped the unit-root redomd, the over-differencing check, the
residual-mean Constant and the insignificant-lag drop — silently, because on the
corpus series those are all no-ops and the whole suite stayed green.

**One call, added on the non-aic path. Measured before/after** on the round-2/3
probe set (ukgas, nottem, ces_leis, ces_accfood):

| argument | before | after |
|---|---|---|
| `urfinal` | FATAL | **applied** on all 3 series where the oracle moves |
| `checkmu` | DIFFERS | **applied** (ukgas); harness-blind elsewhere |
| `cancel` | DIFFERS | **applied** (nottem); inert elsewhere |
| `mixed` | DIFFERS | applied (ukgas); still DIFFERS on 3 |
| `maxdiff` | DIFFERS | applied (ces_leis); still DIFFERS on 3 |
| `maxorder` | DIFFERS | unchanged |

Zero regressions (5509 → 5516 passing, the +7 being the two new `urfinal`
gates). `urfinal`'s fatal is removed and gated by
`generated/{ukgas,ces_accfood}_automdl-urfinal`; mutation-tested (hardcoding
`chkrt1`'s limit to the 1.05 default fails 4 of 7).

**Still open here:** label 40 itself — `IF(Lidotl) amidot ELSE tstmd1` — is not
wired on the plain path. `Lidotl` is true by default (Lotmod's BIGCV AO scan),
so the default is observationally fine; the `tstmd1` arm is what
`noautooutlier=tramo` selects, and that stays fatal. It needs
`automd.f:322-344`'s default-model `rgarma`/`mdlchk`/`armats` hoisted out of
`if (aic)` first, for `blpct0`/`rvr0`/`rtval0`/`adj0`/`trns0`/`tair`.

**The generalizable bit: "the routine is unported" and "the routine is
unreachable" produce the same symptom — an option that moves the oracle and not
the engine — and the fix costs are three orders of magnitude apart.** The
closing note in `automd.cpp` had asserted the former for two sessions on the
strength of one failed experiment (wiring `tstmd1` alone). Check the call graph
before sizing the port.

## 4. First corpus gate target

`tests/corpus/census-examples/03-automdl.spc` — the canonical automdl example.
Compare identified model orders + final estimates against its oracle `.udg`
(mdl orders, niter/nfev, coefficients, variance, loglikelihood/AIC). Add to the
M3 estimation corpus gate once automd is wired.

## 5. Build/run reminder

Build & test ONLY via `powershell -File tools/build.ps1` (handles the rtools44
`ld` DLL-PATH requirement — otherwise `ld returned 9`). Run test exes from
PowerShell, not `./x.exe` in Git Bash (msys "Exec format error").
