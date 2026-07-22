# x11regression{} compute — port plan (execution-ready)

Parse-acceptance is DONE (gt_x11regression = gt_generic, commit 73dca96). This
note scopes the COMPUTE: the irregular-component regression that removes user-
chosen trading-day / holiday effects estimated on the X-11 irregular, and its
save tables **b16 / c16 / xrm**. Ground truth: `oracle/fortran/x11mdl.f` (890
lines, the OLS core) + `gtxreg.f` (913, the parser) + the `Ixreg`/`Axrgtd`
branches woven through `x11pt2.f`. Corpus:
`tests/corpus/extra/airline_x11regression-td.spc` (variables=(td), NO model —
pure X-11 path) and `airline_x11regression-aictest.spc` (aictest=td). Goldens:
`.b16 .c16 .xrm` (144 rows each) under `tests/golden/extra/…`.

## KEY flow finding — Ixreg=1, NOT Ixreg=2 (no xrgdrv on this path)
`gtxreg.f:884` sets `Ixreg=1` when `Nb>0` (regressors present); `Ixreg=2` only
when `prior=yes` (`lprior`, gtxreg.f:885). The `-td` corpus spec has no `prior`,
so **Ixreg=1**. Consequently `x11ari.f:94` (`IF Ixreg.eq.2.or.Khol.eq.1 CALL
xrgdrv`) does NOT fire — `xrgdrv.f` (the "transparent seasonal adjustment"
prior-mode wrapper) is only for Ixreg=2. The Ixreg=1 regression is done
**in-line inside the main x11pt2 iteration**:
- `x11pt2.f:711` `IF(Ixreg.eq.1.or.Ixreg.eq.2)` → `CALL x11mdl(Sti,Muladd,...,
  Kpart,Kswv,Lgraf)` at both Kpart=2 (B iteration) and Kpart=3 (C iteration),
  right after Sti (the B13/C13 irregular) is computed.
- Ixreg=1 wraps the call with `loadxr(F)` before and `loadxr(T)` + `restor`
  after (x11pt2.f:720,723-725) — the regARIMA↔X-11 regression-state swap.
So the minimal port is: **x11mdl + its x11pt2 wiring + the Axrgtd factor folds**,
NO xrgdrv. (xrgdrv/Ixreg=2 = the `prior=yes` variant, a later increment.)

## What b16 / c16 / xrm are
- **b16** = B-iteration regression trading-day factors (`Factd`, Kpart=2). Table
  code `LXRTDF+Kpart-2` (x11mdl saves it).
- **c16** = C-iteration regression trading-day factors (Kpart=3). Same code +1.
- **xrm** = the irregular-regression model summary (coeffs / t-stats / the OLS
  design) — `prtxrg.f` / the LXRMDL table. Confirm exact content vs the golden.

## x11mdl.f (890 lines) — the OLS core (the bulk of the work)
Regresses the (logged, for mult) irregular `Sti` on the user's TD design:
1. Build the TD regression design (reuse the ported `regvar`/`getreg` machinery
   — the TD columns are the same 6 (or 1-coef) contrasts as regARIMA `td`).
2. `trumlt=(.not.Psuadd).and.Muladd.eq.0` → work in logs for mult.
3. OLS normal equations via the Cholesky factor `Chlxpx` (PXPX) — the codebase
   already has `olsreg`/a Cholesky solve in `regarima/estimate.cpp`; check whether
   it's directly reusable or needs the x11-specific weighting.
4. Extreme-value weighting of the irregular before the fit (Stwt / the C17
   weights) — the regression is on the extreme-modified irregular.
5. Form `Factd` (and `Fachol`/`Faccal` for holiday) from the fitted coeffs ×
   design; antilog for mult. Save b16/c16.
6. Diagnostics (xrm): coeff table, t-stats, the aictest branch.
For `variables=(td)` only: the TD-only path — skip holiday/user/outlier/aictest
sub-branches (guarded, later increments).

## Axrgtd folds in x11pt2.f (combine Factd into the SA)
~30 `Ixreg`/`Axrgtd`/`Axrghl` conditional points. The load-bearing ones for the
TD-only Ixreg=1 path (verify each):
- `:115,323` Itd/Priadj interplay (LOM suppression when TD chosen).
- `:791` `IF(Axrgtd.and.Kswv.eq.4)` — emit the combined TD/calendar table
  (LXRTDC = b18/c18).
- `:846,877` fold Factd into Factd/Faccal for the final SA combine.
- The C++ x11parts.cpp already has partial Ixreg-aware branches (lines
  169/183/270/320) and the FATAL guard at x11parts.cpp:353-354 (`xl.axrgtd ||
  xl.axrghl` → `x11_not_ported`). Porting replaces that guard.
All Ixreg branches are guarded by `Ixreg>0` / `Axrgtd`, which are FALSE for every
existing gated spec — so the port is ADDITIVE (no regression risk to the current
bit-exact X-11 path), as long as the guards are preserved exactly.

## Suggested increments (each committable)
1. **gtxreg parser** → set `Ixreg=1`, `Axrgtd`, build the TD regression group
   (reuse `getreg`/`gtpdrg`; the parse side already builds `td` columns for
   regARIMA). Replace the gt_generic stub for x11regression. Gate: the spec
   parses + reaches x11 without the axrg FATAL (still wrong numbers).
2. **x11mdl TD-only OLS** → Factd + b16 (Kpart=2) + c16 (Kpart=3). Gate b16/c16.
3. **Axrgtd folds** in x11pt2 → d10-d13 bit-exact (TD removed from the SA).
4. **xrm** diagnostic table. Gate xrm.
5. **aictest variant** (airline_x11regression-aictest): the regression-based AIC
   TD test (keep/drop TD by AICC). Gate that spec.
6. (later) **Ixreg=2 / prior=yes** → the xrgdrv transparent-SA wrapper.

## Risk / effort
x11mdl is ~890 lines and interleaves with x11pt2's extreme-value + iteration
machinery — this is the single largest remaining engine feature (comparable to a
regARIMA sub-engine). Budget it as a multi-session port. The Cholesky OLS is the
reusable part; the x11pt2 weave + the exact extreme-weighting + the factor
antilog/combine are the exacting parts. Instrument against the b16/c16 goldens
early (they isolate the regression output before the SA fold).
