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

## x11mdl.f (890 lines) — the OLS core, MAPPED to implementation depth
The TD-only, non-aictest path (airline_x11regression-td) is COMPACT and reuses
already-ported routines. The 890 lines are mostly holiday/user/aictest/outlier/
stock-TD/reweight/print branches that the TD-only path skips. Core sequence:
1. **Reset transform for the AIC** (x11mdl:104-111): Lam=1, Fcntyp=4, Nestpm=0
   (the irregular regression is always fit on the (log-)irregular directly).
2. **Copy the irregular into trnsrs** (x11mdl:188-197): `copy(Sti(irridx),Nobspf,
   -1,trnsrs)`, irridx=Pos1ob+nbeg. This is the B13 (Kpart=2) / C13 (Kpart=3)
   irregular the x11pt2 iteration just produced.
3. **xrgtrn** (x11mdl:203-205, `xrgtrn.f` — NEW, small): log-transform the
   irregular for mult/logadd (trumlt). Reuses `logar`.
4. **tdxtrm** (x11mdl:216, `tdxtrm.f` — NEW): flag extreme irregular values to
   EXCLUDE from the regression (sets Rgxcld/Nxcld, the `xclude` common). Sigxrg
   default (2.5 sigma). This is the x11-regression-specific extreme test.
5. **regvar** (x11mdl:388, PORTED) — build the design Xy (TD columns = the same
   `td` contrasts as regARIMA; getreg/gtpdrg already build them).
6. **regx11** (`regx11.f`, 97 lines — THIN, reuses PORTED olsreg/resid/yprmy):
   copy Xy→txy; `dlrgrw` deletes the Nxcld excluded rows (NEW, small); `olsreg`
   → B (coeffs) + Chlxpx (Cholesky); `resid`→A; Var=A'A/Dnefob; Lnlkhd.
7. **rgtdhl** (x11mdl:418, `rgtdhl.f`): **NO-OP for TD-only** (returns unless the
   nonlinear Bell-Hilmer Easter+TD case). Skip.
8. **Dx11 daily-weight construction** (x11mdl:541-660): from the TD coeffs B build
   the 7 daily weights Dx11 (Mon..Sun); mult: `Dx11(i)=1+B(i)`, `Dx11(7)=1-sumB`.
   The `Lxrneg` reweighting (negative-weight fixup) is a sub-branch (default off).
9. **Factd build + b16/c16 save** (x11mdl:660-~830, still to read): apply the daily
   weights across the calendar to form the TD-factor series `Factd`, antilog for
   mult, `punch` to LXRTDF+Kpart-2 (b16 for Kpart=2, c16 for Kpart=3).
NEW routines to port: xrgtrn (tiny), tdxtrm (extreme test), dlrgrw (row delete),
the Dx11+Factd calendar build, b16/c16 emit. REUSED: regvar, olsreg, resid, yprmy,
logar/antilg, regfix, addate/dfdate. `xrm` = prtxrg's regression-matrix summary.

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

## COMPLETE routine inventory (TD-only path, reconnaissance DONE)
The factor build after regx11 is `x11ref` (x11mdl:694), NOT rgtdhl. Full surface:

| routine | lines | status | role |
|---|---|---|---|
| gtxreg parser | — | NEW (reuse gtpdrg) | Ixreg=1, Axrgtd, build TD group |
| tdset.f | 127 | NEW | calendar day-type quantities Xn/Xnstar/Xlpyr/Daybar (the /xtdtyp/ /tdtyp/ commons) — the standardized trading-day counts per month-type |
| xrgtrn.f | 55 | NEW (reuse logar) | log-transform the irregular for mult/logadd |
| tdxtrm.f | 126 | NEW | extreme-value exclusion (Sigxrg sigma test → Rgxcld/Nxcld) |
| regvar | — | PORTED | design matrix Xy (TD columns) |
| regx11.f | 97 | NEW-thin | OLS: copy Xy→txy, dlrgrw excluded rows, olsreg→B, resid→A, Var/Lnlkhd — REUSES ported olsreg/resid/yprmy |
| dlrgrw.f | 34 | NEW-tiny | delete Nxcld excluded rows from txy |
| x11ref.f | 162 | NEW | factor series ftd/fcal from B: `daxpy` accumulate B(icol)*Xy(:,icol) into Ftd for TD cols, then `mulref` mean-normalize by Xnstar, then mult finish `Ftd += Xn/Xnstar` |
| mulref.f | 32 | NEW-tiny | X-11-style mean-correction of a raw factor by Xnstar |
| daxpy | — | trivial | BLAS y+=a*x |
| x11mdl orchestration | 890 | NEW (TD-only subset) | the above sequence + b16/c16 emit + `divsub(Sti,Sti,Faccal)` fold |

REUSED (already ported): regvar, olsreg, resid, yprmy, logar/antilg, divsub,
addate/dfdate. NEW C++ ≈ 700 lines across ~10 routines + the x11pt2 B/C wiring +
harness b16/c16/xrm emit. **No partial gate exists** (the TD is removed each x11
iteration, so nothing validates until the whole chain is bit-exact) — this is a
focused multi-session port on the scale of the SEATS or automdl engines, not a
leaf-routine increment. Reconnaissance is COMPLETE; the port is mechanical from
this map.

## ROOT CAUSE (as of the Codex-assisted debug pass)
The coefficient bug is **the B13 irregular fed to the regression differs from
the oracle at TD/leap-sensitive months**. Proven decisively:
- My design X is BYTE-IDENTICAL to the oracle's saved regression matrix (.xrm),
  maxdiff 0.0 over all 144 data rows.
- regx11/olsreg is faithful (numpy lstsq on my X,y == my regx11 coeffs).
- So the ONLY input differing is y = Xnstar*(Sti-1): my Sti[Feb1949]=1.0132 vs
  oracle B13 1.005 (a ~0.8% gap ~= the Feb leap factor 28/28.25=0.9912); Jan/Mar
  match. My y regressed on X gives the wrong (flat) coeffs; the oracle-B13-derived
  y gives the right ones.
- Direction + the Feb-worst error => **the LOM/leap prior on the x11regression
  input is mishandled**. gtxreg.f:186-192 calls `rmlnvr` (remove length-of-month
  variation) when Picktd; my gt_x11regression does NOT set Picktd / call rmlnvr,
  so the base-x11 input series (and thus B13) carries a different length-of-month
  adjustment than the oracle at Feb/leap months. NEXT: port rmlnvr + the Picktd
  path in the parser (or the priadj/Sprior LOM handling for the x11reg TD case),
  so b1/B13 match the oracle at Feb; then the regression y is correct and the
  coeffs/b16/c16/d10-d13 should follow.

Two faithful fixes ALREADY LANDED this pass (needed, not the coeff root cause):
- x11pt2.f:846-889 Stcsi feedback: after x11mdl, `divsub(Stcsi,Stcsi,Faccal)`
  for the Axrgtd Ixreg==1 case so the next iteration works on the TD-adjusted
  series (core/src/x11/x11parts.cpp, `Sto/Faccal`).
- Forecast-extended design span: x11mdl_td now uses nobspf=posffc-pos1ob+1 and
  passes the real nfcst to regvar, so Factd/Faccal cover [pos1bk,posffc] (fixes
  a NaN the Stcsi divsub hit on the un-extended factors).

## DEBUGGING STATUS (wired end-to-end, ~1-4% off — coeff/design scaling bug)
The full TD path runs (commit 4ba4940). b16/c16 ~1e-2 off; d10-d13 ~2-4% off,
**worst at February** (196002). Localization done — most of the pipeline is
VERIFIED CORRECT:
- **Input Sti** matches the oracle B13 exactly (Jan1949 mine 0.98439 vs oracle
  xr.out:1838 "98.5") — so the base x11 + leap-adjusted b1 (111.30, matches
  oracle B1 xr.out:1279) are right.
- **Leap factors** right: tdset Xn/Xnstar give Feb non-leap 28/28.25=0.9912,
  30/31-day = 1.0 (matches oracle xr.out:4548 "Leap Year factors").
- **Design** looks right: regvar TD row0 = [0,-1,-1,-1,-1,0] = the correct
  Jan1949 (Sun5 Mon5 Tue-Fri4 Sat5) day contrast.
- **regx11/olsreg CORRECT**: a numpy lstsq on the dumped X/y gives the SAME
  coeffs as regx11 (~[-0.066,0.050,-0.031,0.037,-0.053,0.017]) — so the OLS is
  faithful.
- **The gap**: those coeffs (~0.05) don't reconcile with the oracle. The oracle
  .udg `Trading Day$Mon: -0.1305` vs the F4 daily-weight table (xr.out:4530,
  31-day Mon=99.14 -> B~-0.0086) shows a parameterization/scaling difference to
  chase. NEXT: (a) read the oracle "Irregular Component Regression Matrix"
  (xr.out:386) and diff my design row-by-row; (b) compare my Dx11 daily weights
  (x11ref) to the F4 table; (c) check whether the design needs mean-adjustment
  (xmeans) applied/not, or the y transform has a leap term. The Feb-worst error
  points at the leap/length interaction in the design or the Dx11 build.

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
