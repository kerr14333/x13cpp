# x11regression{} OLS-estimated prior trading day (Ixreg>=2 / xrgdrv) — CLOSED

**STATUS (2026-07-23): DONE, bit-exact.** `airline_x11regression-td` gates xrm +
d10/d11/d12/d13 at the estimation floor (measured ~5e-15) in
`tests/parity/test_x11regression_tables.py`. This note is kept as the port map and
root-cause record; the "open residual" narrative below the line is HISTORICAL.

## The feature
A modeled `-td` variable in `x11regression{}` regresses trading-day contrasts on
the X-11 irregular and removes them as a prior calendar factor. Because the spec
also carries `arima{(0 1 1)(0 1 1)}`, `gtinpt.f:1201`
(`IF(Lmodel.and.Ixreg.eq.1)Ixreg=2`) promotes Ixreg 1->2, so the oracle runs the
**transparent prior-TD path via `xrgdrv`** (x11ari.f:93:
`IF(Ixreg.eq.2.or.Khol.eq.1) CALL xrgdrv`, BEFORE x11pt1/arima), NOT an in-line
Ixreg==1 iteration. `xrgdrv` = a model-free, no-forecast X-11 run whose x11mdl OLS
estimates the TD on the irregular, builds `Faccal`, and sets Ixreg=3; the main
x11pt1 then divides `Sto` by `Faccal` (x11pt1.f:180), so regARIMA fits the
TD-adjusted series (converged nonseasonal MA1 ~= 0.2607, vs bare-airline ~0.40).

## C++ port (phase-inverted)
The C++ estimates regARIMA in an earlier phase (`run_pre_model`) than the main
X-11 (`run_x11`), so `xrgdrv` is hoisted ahead of the estimate — same inversion as
the `tdprior` (Kswv=1) case. Pieces:
- **gtinpt promotion** — `core/src/specparse/gtinpt.cpp`: Ixreg 1->2 when a model
  is present; also sets `Nfcstx`/`Nbcstx` (forecast-extended factor span,
  gtinpt.f:1172-1181).
- **`core/src/x11/xrgdrv.{cpp,hpp}`** (NEW) — self-contained transparent SA:
  mirrors run_x11's no-model setup + easter pre-pass, `loadxr(false)` swaps the TD
  columns in for the OLS, runs x11pt1/x11pt2->x11mdl_td, `loadxr(true)` +
  `xrg_clear_working` restores the bare model, stashes the forecast-extended
  `Faccal` in `ctx.x11_faccal_prior`.
- **`run_pre_model.cpp`** — divides the estimation input `padj` by the stashed
  `Faccal` (Sprior stays applied — one Sprior + one Faccal divide, per Codex
  cross-check against x11pt1.f:157-182).
- **`x11parts.cpp` x11pt1** — restores `Faccal` for the Ixreg==3 divide; x11pt2's
  x11mdl/Stcsi guards extended to ixreg 1||2.

## The two decisive bugs (both fixed => bit-exact)
1. **TD-regressor + Faccal-stash leaks into the ML estimate.** loadxr(true) only
   *saves* the swapped-in TD columns back to xrgmdl; it does not clear the working
   model. The oracle calls `loadxr(T)` THEN `restor` (xrgdrv.f:206-207). Fix:
   `xrg_clear_working(ctx)` after loadxr(true). Also the Faccal stash was off by
   one: farray `.data()[i-1]` == logical `faccal(i)`, so stash `.data()+(pos1ob-1)`.
   (=> model bit-exact: ncxy=1, nfev=19/nliter=6, MA=0.260701/0.566029 == oracle.)
2. **The transparent pass leaked two /x11/ state vars into the main run** (same
   class as the slidingspans `xtrm.ksdev` and history `Lterm`/`Nterm` per-span
   reset bugs) — this was the whole d10-d13 residual:
   - **Lterm** — transparent setup sets Lterm NOTSET->6; run_x11's editor.f:2042-2103
     re-resolves Lter/Lmsr/Lstabl/L3x5 ONLY when Lterm==NOTSET, so the main run
     inherited the transparent MSR-selected per-period Lter (0 instead of 6).
   - **Ksdev** (xtrm, Bundesbank spread) — transparent x11pt2 vtest/entsch sets it;
     the main x11pt2 test re-derives only when Ksdev<4, so it inherited the spread.
   Neither alone closes it (filter-only 2.1%, ksdev-only 0.79%; BOTH -> ~5e-15).
   **Fix (xrgdrv.cpp): save `sv_lterm`+`sv_ksdev` before the transparent pass,
   restore after** (mirrors restor.f restoring Lter + the per-span ksdev reset).
   The start-dominant decay (2.4%@1949 -> 0@1954) was a RELATIVE-error artifact:
   airline's seasonal amplitude grows, so a uniform absolute error reads largest at
   the low-level start. Diagnosis path: DBG the global MSR (opt.ratis) — mine 2.183
   vs oracle 2.228 (golden .log "Moving seasonality ratio: 2.228"); lter[0..2]
   printed 0 0 0 (leak) vs 6 6 6 (fixed).

Also landed faithfully (orthogonal to the d-table close): the **Nfcstx
forecast-extended factor** in x11mdl_td (kpart==3 restores Nfcst=Nfcstx per
x11mdl.f:124-134) — matches the oracle's forecast-extended design span. Did not
change observed d10 (forecasts are already TD-consistent) but correct for D16.

## Routine map (reference)
| routine | status | role |
|---|---|---|
| gtxreg parser | reuse gtpdrg | Ixreg promotion, Axrgtd, build TD group |
| tdset.f | ported | calendar day-type quantities Xn/Xnstar/Xlpyr/Daybar |
| xrgtrn.f | ported (reuse logar) | log-transform the irregular for mult/logadd |
| tdxtrm.f | ported | extreme-value exclusion (Sigxrg sigma -> Rgxcld/Nxcld) |
| regvar | ported | design matrix Xy (TD columns) |
| regx11.f | ported (reuse olsreg/resid/yprmy) | Cholesky OLS: B, Chlxpx, Var, Lnlkhd |
| dlrgrw.f | ported | delete Nxcld excluded rows from txy |
| x11ref.f | ported | factor series from B via daxpy + mulref mean-normalize |
| mulref.f | ported | X-11 mean-correction of a raw factor by Xnstar |
| x11mdl (TD-only) | ported | the above + b16/c16 emit + divsub(Sti,Sti,Faccal) |
| loadxr.f | ported | regARIMA<->x11reg regression-state swap (ctx.xrgmdl) |
| xrgdrv.f | ported | transparent-SA wrapper + save/restore lterm/ksdev |

## Still fatal (unported, clean guards)
- OLS prior-TD in **additive / pseudo-additive** mode (muladd != 0).
- The regression-based **aictest** TD variant (keep/drop TD by AICC;
  `airline_x11regression-aictest`).

---
_Historical: earlier passes framed this as an open Ixreg==1 in-line iteration with
a "~1.5e-3 b16/c16 floor" / "d10-d13 ~1.2e-2 at leap-Feb". That premise was wrong
on two counts (the spec is Ixreg==2 via the gtinpt promotion, and the floor was the
Lterm/Ksdev leak above, not a coefficient-precision residual). Both closed._
