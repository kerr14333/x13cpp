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

## ROOT CAUSE — DECISIVELY FOUND & FIXED (the flat-coeff bug)
The earlier "B13 input differs / rmlnvr-as-input-LOM-adjust" hypothesis was WRONG.
The real cause, proven by a with/without-x11reg B1/B13 sweep + the oracle's own
`np=3` estimation report:

1. **The x11regression TD regressors were leaking into the regARIMA ML estimate.**
   `gt_x11regression`→`gtpdrg(x11reg=true)` built the 6 TD (+ leap year) into
   `ctx.model` (the regARIMA store), so `estimate{}` fit airline+TD and produced a
   TD-adjusted B1 → the irregular had NO TD signal left → the OLS returned flat
   (~0) coefficients. The oracle keeps them SEPARATE: its regARIMA model is bare
   airline (`np=3`; the .out explicitly notes the x11reg estimates are not ML),
   and the TD lives in a dedicated x11-regression model store (`xrgmdl.cmn`).
   **FIX — ported `loadxr.f`** (`core/src/x11/loadxr.{hpp,cpp}` + `ctx.xrgmdl`):
   the parser clears the working regressors, gtpdrg builds the TD into the bare
   model, `loadxr(true)` saves it into `ctx.xrgmdl`, then the working model is
   restored to bare airline (gtinpt.f:804-834 ssprep/dlrgef/loadxr(T)/restor). The
   x11pt2 call site wraps `x11mdl_td` with `loadxr(false)`/`loadxr(true)`
   (x11pt2.f:720/724) to swap the TD regressors in for the irregular OLS.

2. **The leap-year regressor must be stripped for the mult/log x11reg td.**
   gtpdrg's picktd "td" appends a Leap Year (PRGTLY) column; the oracle's x11reg
   model is 6 day-contrasts only (nxreg=6) — the length/leap effect is carried by
   the Xnstar day-count normalization in x11ref, not a regression column.
   **FIX — gtxreg.f:186-192**: after gtpdrg, when `Picktd` and (the caller's
   `Priadj`, which gtxreg binds to its param named `Muladd`) != 1/NOTSET, call
   `rmlnvr(NOTSET, 0, Nspobs)` to drop the Leap Year column.

**Result:** b16/c16 went from ~3e-2 off (flat coeffs) to **~1.5e-3** vs the
goldens; coeffs now track the oracle (C16 Sat 0.179 vs 0.168, Tue -0.123 vs
-0.112, etc). No parity regressions (596 pass / 9 skip / 18 xfail).

### GATED bit-exact: xrm design matrix
`xrm` (the Nb TD-contrast columns over the forecast-extended 156-row span) is a
pure integer/arithmetic day-contrast design — snapshotted from md.xy in x11mdl_td,
byte-identical to the oracle (maxdiff 0.0). Gated at 1e-12 in
`tests/parity/test_x11regression_tables.py` (parity 596→597).

### STILL OPEN: the ~1.5e-3 b16/c16 residual + d10-d13 ~1.2e-2 at leap-Feb
RULED OUT this pass (Codex-assisted deep read of x11pt1/x11pt2/x11mdl/x11ari):
- **Outer two-pass:** THERE IS NONE for a single series. `x11ari.f:334` sets
  `Ixreg=0` and re-runs only for INDIRECT composite adjustment (Iagr==3). x11pt2
  runs once; x11mdl is called at Kpart=2 (B) and Kpart=3 (C) in that one pass.
- **B1 pre-adjustment:** for Ixreg==1 the first B-iteration input is RAW
  (x11pt1.f:71/80 copy Series/Orig to Stcsi/Sto; the prior-TD divide-out is gated
  `Kswv.ne.0 .or. Ixreg.ge.2`, x11pt1.f:189 — NOT plain Ixreg==1). No priming
  xrgdrv pass (x11ari.f:93 gates it on Ixreg==2/Khol==1). My raw B1 is correct.
- **Stcsi feedback (Codex's primary lead):** the oracle rebuilds Stcsi from the
  raw extended Series then /Sprior /Faccal (x11pt2.f:846-894). On THIS spec that is
  bit-equivalent to the STCSI=STO shortcut (Sto is already Orig/Sprior from x11pt1,
  and there are no outlier/user priors) — verified empirically (rebuilding from the
  extended Series gave identical d-tables; using the un-extended ctx.inpt.series
  made it worse, confirming the buffer identity, not a logic gap).

REMAINING (needs oracle instrumentation, blocked by the pristine-oracle rule):
- **tdxtrm exclusion + coefficient precision at leap-Feb.** My C-iteration tdxtrm
  excludes 12 pts vs the oracle C14's 10 (8/10 shared; I add leap-Febs 1956/1960,
  miss 1958-Aug). tdxtrm is a faithful port, so this is coupled to the ~1.5e-3
  faccal reference (itself the B-iteration coefficient residual). The B-iteration
  input B13 differs from the no-x11reg baseline at leap-Feb by ~3% — but that is
  EXPECTED (picktd applies a leap Sprior to B1 that the no-x11reg run lacks), so
  the no-x11reg baseline is the wrong comparison. Cracking this needs the oracle's
  own B-iteration irregular / exclusion set (not saved; oracle can't be
  instrumented under the parity rule). Likely a small port-level precision residual
  in the leap-Feb calendar term, not a gross structural bug.

Two faithful fixes from the prior pass (still in place, not the coeff cause):
- x11pt2.f:846-889 Stcsi feedback (`divsub(Stcsi,Stcsi,Faccal)` for Axrgtd).
- Forecast-extended design span (nobspf=posffc-pos1ob+1, real nfcst to regvar).

## DEBUGGING STATUS — SUPERSEDED (see "ROOT CAUSE — DECISIVELY FOUND & FIXED")
The narrative below is HISTORICAL and its conclusion was WRONG. It claimed the
design/olsreg were fine and the "~0.05 vs 0.13" coeff gap was a scaling/leap
puzzle in the design or Dx11 build. The actual cause (see the ROOT CAUSE section
above) was that the x11reg TD regressors were being fit by the regARIMA ML
estimate (removing the TD signal from the irregular → flat coeffs) and that the
leap-year column was not stripped. Both are now fixed (loadxr + rmlnvr); b16/c16
are at ~1.5e-3. Kept only for the verified sub-facts (leap factors, design row0).

<details><summary>Old (wrong-conclusion) notes</summary>

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
  coeffs as regx11 — so the OLS is faithful.
- **The gap**: those coeffs (~0.05) don't reconcile with the oracle. [This was
  the flat-coeff symptom of the regARIMA-estimate leak, not a design/scaling bug.]
</details>

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
