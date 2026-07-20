# Fable review queue — things to double-check

Running list of work I'm **not fully certain about** and want a fresh review pass
(fable) to verify against the Fortran oracle. Most are correct-by-construction or
unreachable-by-default, but none of the items below are yet proven bit-for-bit
against the oracle. Newest first. Remove an item once it's verified + gated.

## Open

1. **Banked adequacy routines are UNGATED** — `mdlchk`, `tstmd2`, `testodf`
   (`core/src/automdl/adqtst.cpp`) are ported but have **no caller yet**, so
   nothing exercises them. They must be verified against the oracle once the
   adequacy stage is wired into `automd` (after `amdid`). Until then treat their
   correctness as unproven.

2. **usdeaths / region final-model transformation is NOT yet explained.** Our
   `iddiff` (0,1) and `amdid` `(1 0 1)(0 1 1)` match the oracle's "Automatic model
   choice" EXACTLY (verified). The oracle then rewrites it to `(0 1 1)(0 1 1)`.
   I first guessed `testodf`, but testodf's regular branch needs `ldr>0` and the
   amdid model has `ldr=0` — so testodf does NOT do it. **The actual routine
   (likely `tstmd1` Ljung-Box re-identification, or a differencing recheck in
   automd's finalization loop) is unconfirmed.** Trace `automd.f` lines ~500-850
   for the `(1 0 1)(0 1 1) -> (0 1 1)(0 1 1)` step before porting the "fix".
   Gated on identification only; see automdl_scouting.md §3c.

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

## Verified / closed
(none yet)
