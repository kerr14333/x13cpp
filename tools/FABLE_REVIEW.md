# Fable review queue — things to double-check

Running list of work I'm **not fully certain about** and want a fresh review pass
(fable) to verify against the Fortran oracle. Most are correct-by-construction or
unreachable-by-default, but none of the items below are yet proven bit-for-bit
against the oracle. Newest first. Remove an item once it's verified + gated.

## Open

1. **Banked adequacy routines are UNGATED** — `mdlchk`, `tstmd2`, `testodf`,
   `bkdfmd`, `tstmd1` (`core/src/automdl/adqtst.cpp`) are ported but NOT wired
   into automd. `tstmd1` in isolation DOES correctly revert usdeaths to
   `(0 1 1)(0 1 1)` (ichk=4, near-unit AR verified live). BUT wiring only tstmd1
   broke parity on the 4 non-revert automd-estimation cases: the oracle
   re-estimates AFTER tstmd1 (the redomd + testodf finalization + a final rgarma,
   automd.f:580-850), so tstmd1's intermediate fit must NOT be the reported one,
   and its unconditional initial rgarma perturbs the estimate at ~1e-8. **The
   adequacy stage must be wired as ONE unit** — tstmd1 + the full finalization +
   the final re-estimate — not tstmd1 alone. Also note: bkdfmd restores `Var` but
   NOT `Lnlkhd`; the finalization's rgarma/prlkhd must recompute the likelihood.
   Reverted the wire (293 passed); routines stay banked.

2. **usdeaths / region final-model rewrite = `tstmd1` (TRACED, not yet ported).**
   Our `iddiff` (0,1) and `amdid` `(1 0 1)(0 1 1)` match the oracle exactly. The
   rewrite to `(0 1 1)(0 1 1)` is `tstmd1` reverting to the airline default when
   the identified model's AR(1) is near-unit (ichk=4/5, Arimap(2)>=0.82;
   tstmd1.f:167-174). Fix = port tstmd1 + bkdfmd (+maybe ssprep) and plumb the
   default-model stats (Pdfm/Rsddfm/Tair) through automd. Verify the exact ichk
   thresholds + the default-stats capture match the oracle when ported. See
   automdl_scouting.md §3c.

3. **testodf deferred branches** — the `Lsovdf` seasonal-regressor path (needs
   `sftest`, not ported) and the outlier branches (`amidot`/`clrotl`) are stubbed
   with comments. Confirm they are genuinely inactive for every gated spec
   (`Lsovdf` default false; no-outlier specs). If a spec ever hits them, testodf
   is wrong there.

4. **Reduced `automd` driver — deferred features.** aictest regressor family
   (tdaic/lomaic/easaic/chkchi), pass0, adequacy retry, in-automd outlier ID are
   all skipped. Verify that specs needing them **abend cleanly** rather than
   silently mis-fitting. Known-expected divergence: 03-automdl gets the right
   model + transform but wrong variance because `aictest=(td easter)` regressor
   selection is unported (oracle's final model carries td+easter). Confirm no
   OTHER silent divergences on the ~24 automdl corpus specs.

5. **chkmu / genrtt** (`core/src/automdl/chkmu.cpp`) — the constant-significance
   test (t-value vs 1.96 kstep 0 / 1.6 kstep 1) is exercised only through automd's
   default-model mean check. Verify the t-value + the add/remove-constant decision
   against the oracle for a mean-significant series (expgs is automean=yes and its
   final estimation matches, which is indirect evidence, but genrtt's per-regressor
   t-stats aren't directly gated).

6. **trnaic merge (from subagent).** Merged cherry-pick `5c7591a`. The subagent
   flagged a subtlety: the oracle's `Ap1` initial-ARMA backup (editor.f:918,
   guarded `Nopr>0`) never runs on the automatic-model path, so log-fit free ARMA
   lags restart from 0 (not a 0.1 strtvl reseed); adding the backup broke the log
   AICC by ~1e-9. Verify this holds across MORE `function=auto` specs than the 4
   in `test_m4_trnaic.py`.

7. **NSA data precision.** The R-extracted `nottem/ukgas/co2/usdeaths.dat` use
   `formatC(..., digits=4)`. Goldens were generated from these same files (so
   internally consistent), but if higher precision matters, regenerate both. When
   the user drops real FRED NSA data (PAYNSA/UNRATENSA), regenerate goldens.

## Found gaps (features that FATAL / diverge — real bugs)

- **User-specified outlier regressors FATAL.** `regression{ variables = (ao1950.jan) }`
  (and `ls<date>`, `tc<date>`, ramps) → `OUTCOME: FATAL` in x13run_m3, while the
  oracle estimates fine. Automatic outlier ID (`outlier{}`) works and is gated;
  the gap is parsing/building USER-specified point-outlier regressors in the
  regression-variable list (getreg/adpdrg doesn't recognize the `ao/ls/tc<date>`
  variable syntax, or regvar can't build the column). A common X-13 feature —
  worth fixing. Excluded from the reg/outlier gate; found by the sweep.

## Verified / closed
(none yet)
