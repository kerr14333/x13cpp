# Fable review queue — things to double-check

Running list of work I'm **not fully certain about** and want a fresh review pass
(fable) to verify against the Fortran oracle. Most are correct-by-construction or
unreachable-by-default, but none of the items below are yet proven bit-for-bit
against the oracle. Newest first. Remove an item once it's verified + gated.

## Open

0. **SEATS poly + root leaves (CONV/CONJ/MULTFN/DIVFCN, C02AEF/C02AEZ, RPQ) —
   `core/src/seats/{poly,roots}.cpp`.** VERIFIED bit-exact: `C02AEF` (the #1 SEATS
   parity hinge) was cross-checked against a standalone oracle driver built from
   the real `ansub2.f` C02AEF/C02AEZ (gfortran -O2 -ffp-contract=off, linking the
   oracle dpmpar/dpeq). All roots matched to the LAST BIT on three iterative cases
   (cubic 2/3/4, mixed real+conjugate `(z^2+4)(z-1)`, quintic 2..6) — those exact
   bit-patterns are now the golden values in `test_seats.cpp`. Two small residuals
   to keep an eye on when SEATS pushes more diverse polynomials through it:
   (a) `tol2 = pow(tol, 1.5)` ports Fortran `tol**1.5` via `std::pow`; it produced
   bit-identical roots on all tested cases, but `pow` vs gfortran's `**` could
   differ at last ULP for some `tol` — `tol2` only gates the convergence test, so
   at most it shifts an iteration boundary. (b) `scale = 2**(-k)` uses `ldexp`
   (exact power of two, guaranteed identical). RPQ's post-classification
   (modulus/arg/period) is checked for correctness, not oracle bit-capture, since
   it is plain arithmetic downstream of the (bit-exact) roots. Re-confirm once the
   canonical decomposition (SECOND/PARFRA/MAK1) actually consumes these roots.

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

- **`labor` / `thank` holiday regressors — FIXED + gated.** Ported `adlabr.f`
  (Labor Day) and `adthnk.f` (Thanksgiving-Christmas) column builders into
  `core/src/regarima/adhol.cpp`, wired regvar cases 100/110 (single-column,
  ndays from the group title via ctoi, case-70 pattern). `airline_reg-labor` /
  `airline_reg-thank` gate BIT-EXACT vs oracle in test_m3_estimate.py.
- **User-specified outlier regressors — FIXED** for AO/LS/TC/RP/MV/SO/TL/QI/QD.
  getreg_vars.cpp's outlier-variable case now parses the date (rdotlr), validates
  the window per type, and registers the group (regvar case 120 builds the column
  via addotl); user TC defaults tcalfa (0.7^(12/sp)) as the outlier{} path does.
  All four common types (AO/LS/TC/RP) gated bit-exact vs oracle
  (`airline_out-ao1950-jan` etc.). STILL DEFERRED: AOS/LSS (typidx 10/11) need
  `rdotls.f` (not ported) — they abend loudly.

## Verified / closed
(none yet)

## M4 aictest family (tdaic / easaic / addtd / addeas) -- 2026-07-20

Ported the automatic-model-selection AIC-test regressors into
`core/src/automdl/aictst.{cpp,hpp}` and gated them oracle-exact
(`tests/parity/test_m4_aictest.py`, 9 cases, rtol 1e-6). All nine
`aictest.diff.td` / `aictest.diff.e` values across airline, 03-automdl, expgs,
payems, unrate reproduce the oracle `.udg` BIT-EXACTLY.

Notes / deferred:

- **xrlkhd** was NOT re-ported: it already lives in
  `core/src/regarima/estimate.cpp` and belongs to the x11regression AIC path.
  tdaic/easaic call **prlkhd** (also ported); its `ctx.lkhd.aicc` is the AICC the
  tests compare. The task brief conflated the two.

- **chkchi.f DEFERRED** (blocker): its callees `chitst` (only a doc comment
  exists in numeric.hpp -- NOT implemented), `dlusrg`, and `savchi` are unported,
  and no corpus spec exercises the user-defined-holiday chi-square path. Port
  chitst + dlusrg when a `regression{ user=... usertype=holiday }` + `aictest`
  spec enters the corpus.

- **lomaic / usraic**: not ported (out of the requested TD/Easter scope; siblings
  of tdaic/easaic for length-of-month and user regressors). Straightforward
  follow-ups on the same pattern.

- **The harness (`x13run_iddiff --aictest`) supplies two pieces of state the
  ported pre-model phase does not set**, because the eventual wiring lives in
  automd (main thread), not here:
  1. The aictest argument state (`Tdayvc/Ntdvec/Easvec/Neasvc/Itdtst/Eastst/...`).
     The getreg/editor.f aictest parser is unported; the harness installs the
     `aictest=(td easter)` defaults (editor.f:1151-1442) for the no-regime,
     no-stock, no-existing-regressor case. Port that editor.f slice when wiring.
  2. The prior-adjustment span `Begadj/Nadj/Adj1st` (adjsrs.f:20-21,89-90).
     `run_pre_model.cpp` does the prior adjustment inline (via `lpfac`) and never
     sets these, so tdaic's LOG-transform leap-year PREADJUSTMENT (td7var over the
     adjustment span) produced an all-ones factor => wrong `aictest.diff.td` on
     log data (off by ~7.5 AICC) until the harness set them. **When wiring aictest
     into automd, ensure Begadj/Nadj/Adj1st are populated** (adjsrs runs in editor
     before automd in the oracle) or the log leap-year TD test will be wrong.

- **automd round-1 vs round-2 sequencing.** The oracle runs the AIC tests TWICE
  (automd.f:220-240 on the default airline model, then automd.f:513-524 on the
  identified model) and the `.udg` saves the LAST run. So the harness picks a mode
  per case: `--aictest` (identified == default airline: airline / 03-automdl),
  `--afterauto` (identify first: expgs diff.td, payems/unrate diff.e), and
  `--afterauto --preeas` (also install Easter before the TD test, matching the
  round-2 model state: payems/unrate diff.td). A single unified run reproducing
  BOTH diff.td and diff.e of a differing-model series in one pass needs the full
  round-1 -> chkmu -> iddiff/amdid -> round-2 interleave carrying round-1's exact
  regressor state through identification -- i.e. the real automd wire. Each value
  is individually bit-exact; only the single-pass unification is deferred.
