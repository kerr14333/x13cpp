# X-11 model-path regression effects — handoff / open bug

## LATEST STATUS (session 6): finalization landed -- 3/4 aictest-x11 specs GREEN

The session-5 milestone (below) is DONE. Ported automd.f l.322-982 (Lidotl=F
path) as one unit into `core/src/automdl/automd.cpp`, replacing the reduced
ctx-snapshot ismd0-only revert:

- New `core/src/automdl/automd_finalize.{hpp,cpp}`: faithful ports of
  `ssprep.f`/`restor.f` (Lmodel branch), `rmfix.f`/`addfix.f` (Fxindx=2, with
  the real Cfxttl/Bfx/Fxtype/Grpfix dictionary -- not the old inline strip),
  `tstdrv.f` + `pass0.f` (final TD/Easter/Constant significance recheck,
  including the `rmlpyr.f` leap-year-prior removal branch pass0 can trigger),
  `clrotl.f`, `autoer.f`.
- `automd.cpp` grew `automd_aic_round` (shared block-2/block-3 tdaic+easaic
  retest), `automd_reestim` (the repeated rgarma+prterr+abend checkpoint), and
  `automd_finalize_tail` (label 30 onward: mdlchk, pass0 Istep=1 recheck,
  chkrt1+redomd, testodf, the residual-mean Constant-add, the tstmd2
  insignificant-coefficient loop, autoer) -- ONE function reached from BOTH
  the ismd0 a0-revert shortcut and the non-ismd0 block-2-AIC/tstmd1/block-3-AIC
  path, per the FABLE_REVIEW §1 constraint (no piecemeal tstmd1 wiring).

**Result:** `airline` now bit-exact at ~5e-15 (was ~9.7e-7, the old ismd0-revert
residual -- root cause confirmed as exactly what session 5 predicted: the
reduced-rmfix aictest re-estimation wasn't the oracle's full nloop). `expgs`
and `payems` (the non-default-model series) ALSO reach bit-exact (worst
~8e-10 and ~8e-9). Un-xfailed all three in `test_x11_tables.py`.

**`unrate` still xfailed** -- not a control-flow gap, a regARIMA-estimation
edge case: the DEFAULT (0,1,1)(0,1,1) model's own seasonal-MA `armats` t-stat
comes out ~62 (a boundary/near-unit-root signature) where the oracle's
equivalent is presumably small, which flips tstmd1's `i1dfm+i2dfm==0`
early-return check (automd.f:396) from "leave the identified (0,1,1) model
untouched" to "enter the insignificant-lag-drop loop", which then correctly
(per a faithful tstmd1 port) drops the regular MA term, landing on (0,1,0)
instead of the golden's (0,1,1) (`arimamdl: (0 1 1)` per the golden .udg, MA1
estimate -0.0369 se 0.03589 t~1.03 -- insignificant by BOTH the 1.8 and 1.96
thresholds, yet the oracle keeps it, meaning its own tstmd1 call took the
early-return path instead). The default-model rgarma/armafl call that produces
this t-stat is pre-existing code (unchanged by this session), so this is a
regARIMA-engine precision/boundary-case investigation, not an automd.cpp
control-flow fix. d10 worst 5.5, d13 worst 8.9 (O(1) drift, not a tolerance
miss) confirm the model itself diverged, not a downstream decomposition bug.

Gate after landing: `432 passed, 9 skipped, 36 xfailed` (was 417/9/51), 8/8
unit, m4 aictest/iddiff/trnaic gates unchanged (32/32) -- do_aictest stays
false in that harness so this change couldn't touch it. No regressions.

## NEXT MILESTONE (scoped session 5): port the automd label-30 finalization tail

The one milestone that closes ALL four `*-aictest-x11` xfails (airline residual +
expgs/payems/unrate). All primitives are already ported (rmfix/addfix/restor/
ssprep/bkdfmd/regvar/clrotl/dlrgef/strinx/adrgef in automd.cpp/adqtst.cpp/aictst.cpp;
tstmd1/bkdfmd/testodf in adqtst.cpp). What's missing is the oracle's control flow
from `automd.f` — our automd.cpp stops after amdid + a reduced ismd0 ctx-snapshot
revert and never runs the finalization.

Oracle path to port (line numbers in `oracle/fortran/automd.f`):

- **Save-a0 + strip (l.360-388):** after the default-model estimate, save
  `a0`/`adj0`/`trns0`, `nloop=1`, then `GO TO 10`: `ssprep(T,F,F)`, `bkdfmd(T)`,
  `rmfix(...,2)` + `regvar(...)` to strip regressors before ID.
- **ID + ismd0 (l.390-441):** iddiff, amdid, compute `ismd0` (identified == default).
- **addfix + ismd0 revert (l.444-506):** `addfix(...,0,2)` puts regressors back; for
  `nloop==1 && ismd0` (airline): `restor(T,F,F)`, `copy(a0,A)`, `regvar(...)`,
  `GO TO 30`. This faithful sequence — NOT our ctx snapshot — is what makes the a0
  betas bit-exact.
- **Block-2 AIC (l.508-548):** non-ismd0 re-runs tdaic/easaic on the identified model.
- **`GO TO 40`/tstmd1 block-3 (l.549-653):** the model-adequacy loop; wire as ONE
  unit (piecemeal tstmd1 perturbs green cases — FABLE_REVIEW §1).
- **Label 30 finalization tail (l.654-982) — THE airline residual source:**
  `mdlchk` → (`pass2` if Lidotl) → `pass0(...,1,...)` a FINAL AIC significance
  recheck of TD/Easter that can re-estimate (l.677) → `chkrt1` unit-root check
  (l.717) → `redomd` re-estimate loop → final `rgarma`. Airline is `Lidotl=F` so
  pass2 is skipped, but pass0 + chkrt1 + the final rgarma still run and produce the
  final betas our reduced port never computes (hence factd off ~6e-8).
- **Label 70 exit (l.983):** `autoer`.

**Approach:** replace the reduced ismd0 block in `automd.cpp` (currently ~l.173-320)
with a faithful port of l.360-982, guarded on `!aictest_vars.empty()` so no-aictest
specs are untouched. Gate on all four `*-aictest-x11` specs PLUS the green
`automdl-x11` (no regressor) and the 5 green m4 `--afterauto` cases (must not
regress). Port status (verified session 5): pass0, chkrt1, redomd, mkmdsn, amidot, mdlchk,
tstmd1, testodf are ALL ported. Only **pass2** is MISSING — but pass2 runs only
under `Lidotl` (outlier ID), and the four `*-aictest-x11` specs have no `outlier{}`
(`Lidotl=F`), so pass2 is not on their path. This milestone is dependency-complete
for the four specs; it is purely a control-flow port of automd.f l.360-982.

## LATEST STATUS (2026-07-20, session 5): residual LOCALIZED to the aictest beta

Built in the session-4 `finhol=true` fix and reran: **417 passed, 9 skipped, 51
xfailed**, 8/8 unit — no regression. d11 worst dropped to **9.6e-7 at 196003**
(was 1.58e-2 at Easter).

Localized the remaining ~1e-6. Temporarily dumped `ctx.x11fac.factd`/`.fachol`
from `tools/x13run_x11.cpp` and diffed against the `.td`/`.hol` goldens (dumps
reverted after):

| table | worst rel err | at |
|-------|---------------|-----|
| factd vs .td   | 6.4e-8 | 195109 |
| fachol vs .hol | 7.1e-8 | 195604 |
| b1             | 9.0e-8 | 195604 |
| d11            | 9.6e-7 | 196003 |

**Conclusion:** the TD/Easter factors and the x11pt2/pt3 combine are ALL at
tolerance (~6-9e-8). d11 is 9.6e-7 because the X-11 same-month seasonal moving
average **amplifies** the ~6e-8 weekday-correlated error in `Factd` ~15x. So this
is NOT a decomposition bug — it is the estimated `td1coef` beta being off ~6e-8
relative (golden `-0.264375490834406E-2`). That is estimation last-ulp, same class
as the deferred fixed-airline forecast, and most likely the **same root** as the
non-default aictest series: our reduced-rmfix aictest re-estimation is not the
oracle's full `nloop`/`addfix`/`restor` finalization. Closing that finalization
(the milestone in the "Scope" section below) should fix airline AND
expgs/payems/unrate together. Not worth chasing as an isolated 1e-8 target.

## LATEST STATUS (2026-07-20, session 4): Easter fold FIXED; small TD-beta residual remains

Root-caused and fixed the session-3 "D11/D13 off at Easter months" bug. Mechanism:

- Oracle `editor.f:1410-1442` is the (until now unported) code that, when
  `regression{ aictest=(... easter) }` puts Easter in the AIC-test candidate set with
  no pre-existing Easter regressor, sets **`Finhol=T`** (editor.f:1440,
  `IF(.not.Finhol)Finhol=T`) alongside the `Easvec=(-1,1,8,15)` setup.
- `Finhol` gates which branch `x11pt3.f:525-541` takes when rebuilding the combined
  calendar factor `Faccal` for D11/D13: `IF(.not.Finhol.and.(...))THEN` — when
  `Finhol=T` this ENTIRE rebuild block is skipped, so `Faccal` keeps x11pt2's
  `Factd*Fachol` (TD **and** Easter). The subsequent `divsub(Stci,Stci,Faccal,...)`
  at x11pt3.f:548 then removes both from D11, and D13 (`divsub(Sti,Stci,Stc,...)`)
  inherits the same clean D11.
- Our port (`core/src/automdl/automd.cpp`, `automd_aictest_block1`, the port of
  exactly this editor.f range) had ported `Easvec`/`Neasvc`/`Eastst` but not the
  `Finhol=T` line — `ctx.x11adj.finhol` was never set true anywhere in the codebase
  (confirmed by grep), so it stayed at its zero-initialized `false`. That drove
  `x11parts.cpp`'s x11pt3 port (already a faithful line-for-line port of
  x11pt3.f:525-541/545-550, including the `finhol` checks) down the `Haveum=F`
  `divsub(Faccal,Faccal,Fachol,...)` branch, stripping Easter back out of `Faccal`
  before the D11/D13 divide.
- **Fix** (one line): in `automd_aictest_block1`'s `want_easter` branch, added
  `if (!ctx.x11adj.finhol) ctx.x11adj.finhol = true;` mirroring editor.f:1440.
- **Result**: D11/D13 worst-case error dropped from ~1.58e-2 at Easter months to
  ~9.7e-7, in line with d10/d12's pre-existing residual. Full parity suite
  unchanged at 417 passed / 9 skipped / 51 xfailed (no regression); 8/8 unit tests
  pass.

**Still open — small non-Easter residual (all 5 tables ~1e-7 to 1e-6):**
`b1` worst ~9.0e-8, `d10/d11/d12/d13` worst ~1e-6. Diagnostic evidence gathered this
session (via a scratch diff script, not committed):
- The residual is **not** Easter-specific anymore -- e.g. b1's worst rows are still
  Mar/Apr (195604, 195103, ...) but the *size* no longer matches the Easter-factor
  magnitude (5.7e-3); it's ~9e-8 relative, same order as non-Easter months.
- The **relative** error is a constant per weekday-composition class of month,
  independent of year: 195704 and 195804 (same weekday pattern, different years)
  have *identical* absolute b1 diffs (1.777e-05), and the diff scales with the raw
  series level (e.g. 195604 vs 195104 diffs are in ratio ~1.92, matching the
  1956/1951 series-level ratio). That signature -- a fixed relative offset per
  calendar class, proportional to level -- points to a tiny constant multiplicative
  discrepancy in the combined TD factor `Factd` itself (i.e. the td1coef `beta` from
  the regARIMA/tdaic estimation), not a decomposition-side (x11pt2/x11pt3) bug: the
  TD/Easter regressor *design values* are exact rationals (see
  `tests/golden/generated/airline_automdl-aictest-x11/airline_automdl-aictest-x11.rmx`
  -- values like -4, 3, -1.5, -0.5, 2, ±0.266, all exact in double), so any FP noise
  has to come from `beta` or the `exp()` combine, not the design matrix.
- The m4 `aictest.diff.td`/`aictest.diff.e` gate (`tests/parity/test_m4_aictest.py`)
  only checks AICC deltas at **rtol 1e-6** -- a beta-level relative error of ~1e-8
  would sail through undetected there. Likely NOT reachable without oracle
  intermediate dumps of `Factd`/the fitted `B` vector (WRITE statements in
  `x11pt2.f`/`arima.f` around the estimate, or a `.rmx`-style save for `td`/`hol`
  golden factor values at full precision -- `.td`/`.hol` goldens already exist and
  can be diffed directly against a new `ctx.x11fac.factd`/`.fachol` dump in
  `tools/x13run_x11.cpp` as the next concrete step).
- Gate stays `pytest.xfail`ed (rtol 1e-8, must not weaken) at
  `tests/parity/test_x11_tables.py` with an updated reason string.

---

## Session 3 status (superseded by session 4 above, kept for history)

`airline_automdl-aictest-x11` now runs the full pipeline and is **near bit-exact**:
`b1 ~9e-8`, `d10/d12 ~1e-6` (essentially correct), `d11/d13 ~2.3e-2 at leap Februaries`.
Still xfailed (gate is 1e-8). What landed this session (all gated, no regression —
417 passed):
- **Crash fixed** (heap overflow): `run_pre_model.cpp` sized the `trnsrs` scratch to
  `nobspf` but automd/tdaic do `copy(trnsrs, PLEN,...)` — now PLEN-sized. (fable)
- **x11pt2 factor combine** (`x11parts.cpp`): Faccal = Factd*Fachol (x11pt2.f:158-186),
  guarding the still-unported outlier/user/seasonal/x11reg activations.
- **makadj/tdlom** (`x11parts.cpp`, x11pt2.f:115-129): the leap-year prior combine.
- **setpri = pos1bk** (`run_x11.cpp`, editor.f:851) — was never ported; needed once
  Nadj>0. Fixes the x11int/tdlom Sprior indexing (was an OOB).
- **B1 snapshot** (`run_x11.cpp`): copy the adjreg-adjusted Stcsi into **Stoap** (a
  x11pt1 scratch buffer, dead afterwards) before x11pt2 overwrites Stcsi; the harness
  dumps b1 from Stoap on the model path. IMPORTANT: do NOT snapshot into Series --
  x11pt3 reads Series as the ORIGINAL for D11 (=Series/seasonal/Faccal); overwriting it
  double-removes the calendar (this cost hours -- was the "leap double-count" red
  herring).
- **Leap-year threading** (`run_pre_model.cpp`): re-capture `out_trnsrs` AFTER automd,
  so the tdaic leap-year adjustment (priadj=4) reaches the X-11 series -> B1. No-op for
  non-aictest automd. Took b1 from 2.4e-2 to ~9e-8; leap Februaries now correct.

**OPEN last mile (two items):**
1. **D11/D13 off at EASTER months** (~5.7e-3, e.g. 195603/195604, EVERY Mar+Apr; worst
   1.58e-2 at 195103/195603). The error equals the Easter factor exactly (fhol =
   +-0.005671 at Apr/Mar -> exp = 5.69e-3/5.66e-3). D11 misses one Easter removal.
   Mechanism: x11pt2 builds Faccal = Factd*Fachol, but x11pt3's holiday combined-factor
   rebuild (`x11pt3.f:525-540`, cpp `x11parts.cpp` ~617-628) takes the `Haveum=F`
   branch `divsub(Faccal,Faccal,Fachol)` -> Faccal = Factd, so D11 = Series/seasonal/
   Factd removes seasonal+TD but NOT Easter. The golden D11 removes Easter. Where does
   the oracle remove it? Needs oracle-intermediate dumps (or careful x11pt3.f read
   around 495-550, incl. the `Stci=Series` reset at 516-517 and the Sts division). Note
   b1 is EXACT at Easter months, so this is purely a decomposition-side fold, not B1.
2. **Residual ~1e-6 in b1/d10/d12**: near-exact but not 1e-8. Probably op-ordering vs
   the oracle in the leap/TD combine or invfcn. Check once (1) is resolved.

`*-fixed-airline-x11` still xfailed (post-outlier td forecast, separate). expgs/payems/
unrate aictest still need the non-default nloop/tstmd1 (see below).

---

Status as of the M5/x11 regeff wiring (commits b1c70e5 / c35e43a). The
`*-aictest-x11` and `*-fixed-airline-x11` specs are xfailed:
`test_x11_tables.py` — "x11 model path: regression-effect factors in adjreg unported".

## Confirmed root cause (this session)

For `airline_automdl-aictest-x11` (aictest selects **td1coef + easter**, oracle
betas: TD weekday `-2.6438e-3`, Easter `+2.1321e-2`):

- **cpp `b1` == the raw integer airline series** (112, 118, 132, …). The TD/Easter
  effects are **not removed at all**; golden `b1` = raw ÷ combined factor.
  Worst rel-err: b1 2.4e-2, d10/d12 1.6e-2, d11 3.9e-2, d13 4.5e-2. All drift is
  inherited from b1.
- Instrumenting `run_x11.cpp` just before `adjreg`:
  ```
  DBG nb=0 ncxy=1 nrxy=156 adjtd=0 adjhol=0 nhol=0 have_eff=0
  DBG ftd[0..2]=0 0 0 fhol[0..2]=0 0 0
  ```
  **`ctx.model.nb == 0`** at the effect stage. So `chkadj`'s `for icol=1..nb`
  loop never runs → ntd=0 → `adjtd/adjhol` stay 0 → `have_eff` false → `regvar`+
  `regeff` skipped → `ftd/fhol` all zero → `adjreg` subtracts nothing → b1 = raw.

So the commit-message claim "factors apply but land ~1.6e-2" is optimistic: the
factors are **identically zero**; ~1.6e-2 is just the size of the un-removed
calendar effect.

## The actual fix needed

Propagate the estimated regression model onto `ctx.model` / `ctx.mdldat` for the
X-11 stage: after `run_m2_after_parse(..., estimate=true, ...)` returns, `nb`
(regressor count), the design `xy` (ncxy leading dim — note DBG shows ncxy=1, also
wrong), and betas `b` must reflect the aictest-finalized model. Trace where the
automdl/aictest final model writes its regressor count and whether run_m2 leaves a
local model copy that never lands on `ctx.model`. The no-regressor `automdl-x11`
gate is green precisely because nb=0 is *correct* there — which is why this slipped.

## Scope: this is the deferred automd-finalization milestone, not a small wire

Two unported pieces, plus a betas-source subtlety:

1. **aictest state-setup is unported.** The parser captures
   `ctx.captured.aictest_vars = ["td","easter"]` (readers_spec.cpp) but nothing
   translates it into the estimation state. Port `getreg.f:240-277` (token→`Itdtst`
   / `Leastr`+`Eastst` / `Lomtst` / `Luser`; **td→Itdtst=1, easter→Leastr,Eastst=1**),
   `editor.f:1151-1166` (`Tdayvc`/`Ntdvec`: Itdtst=1 → (0,1,4),ntdvec=3 — the "0=no
   TD,1=6-coef,4=td1coef" candidate set, from which tdaic picks td1coef), the Easter
   `editor.f:~1410-1442` (`Easvec`=(-1,1,8,15),`Neasvc`=4), the `gtinpt.f:282-301`
   defaults (`Rgaicd`/`Pvaic`/`Traicd`), and the adjsrs prior-adj span
   (`Begadj`/`Nadj`/`Adj1st`). **`tools/x13run_iddiff.cpp` (the `--aictest` harness,
   lines 129-260) is a working reference for all of this** on the airline case — the
   m4 `aictest.diff.td/e` gate passes through it.

2. **`tdaic`/`easaic` are never called in `automd`.** `oracle/fortran/automd.f`
   calls them in **three** blocks: block 1 (l.216-251, AIC tests on the DEFAULT
   model, before chkmu), block 2 (l.508-544, on the IDENTIFIED model), block 3
   (l.585-620, inside the `tstmd1` non-airline finalization). Order within a block:
   `tdaic` then (`.not.lester`) `easaic`. `automd.cpp` today has none of them.

3. **Final betas come from the LAST AIC round, not block 1.** tdaic/easaic leave the
   model rebuilt+re-estimated to the chosen candidate; the B1 factors need those
   final betas. Block-1-only would end at amdid's fit, which differs.

**Series reachability (identified model, from the udg goldens):**
- `airline` → (0 1 1)(0 1 1) = the default airline model → reachable with **blocks
  1+2** (ismd0=T, block-3/tstmd1 not entered).
- `expgs` → (2 1 0), `payems` → (0 1 2), `unrate` → (0 1 1) — all **non-default**, so
  the oracle enters the `tstmd1`/redomd/testodf loop (block 3). These need the **full
  deferred automd finalization** (see automdl_scouting.md 3c / FABLE_REVIEW §1: wiring
  tstmd1 piecemeal perturbs the previously-exact cases — wire the whole loop as ONE
  unit).

**Recommendation:** do this as the automd-finalization milestone (state-setup +
blocks 1/2/3 + tstmd1 loop) in one focused pass, gated on all four `*-aictest-x11`
specs plus the currently-green regressor-free `automdl-x11` (must not regress). Guard
the state-setup on `!aictest_vars.empty()` so no-aictest specs are untouched.

## Session update (2026-07-20): PART 1 DONE (automd aictest + ismd0 revert). PART 2 = x11pt2 combine.

The blocker splits into TWO independent milestones. Part 1 is implemented and green;
part 2 is newly identified and is now the thing standing between HEAD and un-xfailing
`airline_automdl-aictest-x11`.

### PART 1 — automd aictest selection + a0/ismd0 revert  ✅ DONE (in `automd.cpp`)

`automd` grew a gated `do_aictest` param (default false). When true and the spec has
`aictest=(...)`:
- **Block-1 tdaic/easaic** run on the default airline model (helper
  `automd_aictest_block1`, state-setup lifted from `x13run_iddiff.cpp:185-260`). On
  `airline` this selects td1coef (`Aicint=4`) + Easter (`Aicind=1`), rgvrtp `[41,10]`.
- **a0/ismd0 revert** replaces the oracle's rmfix/ssprep/addfix/restor nloop with a
  C++ approach valid for the ismd0 (identified==default) case: snapshot the model
  commons + the trnsrs buffer, do a **reduced rmfix** (strip regressors from the
  matrix via `dlrgef` + subtract their `daxpy` effect from trnsrs — rmfix.f Fxindx=2's
  numeric essence, the fxreg dictionary bookkeeping elided since restore is by
  snapshot not addfix), run iddiff/amdid on the clean series, then if `ismd0`
  (orders==default & mean unchanged) restore the snapshot. Airline: orders come out
  `(0 1 1)(0 1 1)`, ismd0=T, model restored to a0 = airline+td1coef+Easter.
- **Verified end-to-end through run_x11**: model reaches X-11 with nb=2, `chkadj`
  sets ntd=1/adjtd=1/adjhol=1, regeff computes nonzero ftd (`ftd0≈0.0106`), adjreg
  completes. (Was: nb=0, factors identically zero.)

Gotchas hit (all resolved):
- **Stack overflow** (`0xC00000FD`) from snapshotting `mdldat_cmn` (Armacm ~1.2 MB) on
  the stack — heap-allocate snapshots with `make_unique`.
- **m4 `--afterauto` double-count**: that harness calls automd then runs the AIC tests
  itself; automd owning them regressed 5 m4 cases. Fixed by the `do_aictest` gate —
  only `run_pre_model` (real x11 path) passes true; the harness leaves it false.

### PART 2 — x11pt2 model-based factor combine  ⛔ THE REMAINING BLOCKER

With the correct model reaching X-11, `x11pt2` now fatals at
`core/src/x11/x11parts.cpp:275` (`x11_not_ported "x11pt2 model-based adjustment-factor
combine/emit"`) — the guard at l.270-277 trips on `adjtd==1 || adjhol==1` and returns
**before PART B**, so NO X-11 tables emit (harness prints no b1/d10-d13). This is the
port of **`oracle/fortran/x11pt2.f:74-352`** (the combine region before `C --- PART B`
at l.353). For the airline case (Priadj=1, no sliding spans, no outliers) it reduces to:
- `IF(Adjtd==1) addmul(Faccal, Faccal, Factd, Pos1bk, n2)`  (x11pt2.f:169)
- `IF(Adjhol==1 ...) addmul(Faccal, Faccal, Fachol, Pos1bk, n2)`  (x11pt2.f:185)
- plus the `ntype`/Stocal `divsub` tail (l.188-327) and deferred table/punch emits.

**Key prerequisite**: `ctx.x11fac.factd` / `.fachol` must hold the regression factors.
Today `run_x11.cpp`'s `regeff` writes them to LOCAL `ftd`/`fhol` vectors only — wire
those into `ctx.x11fac.factd`/`.fachol` (that is where the Fortran regeff/adjreg leave
them). Then the x11pt2 combine builds `Faccal`.

**Good news**: `x11pt3` ALREADY handles adjtd/adjhol (`x11parts.cpp:615-637` folds
`ctx.x11fac.faccal` into D11/D16; the l.605 not_ported is only for adjls/adjao/adjtc/
adjusr — outliers/user, which airline doesn't have). So once x11pt2 populates
faccal/fachol, d10-d13 should emit and gate.

### Checklist to un-xfail `airline_automdl-aictest-x11`
1. Wire `regeff`'s ftd/fhol (run_x11.cpp) into `ctx.x11fac.factd`/`.fachol`.
2. Port `x11pt2.f:74-352` combine (replace the `x11parts.cpp:270-277` not_ported):
   build `Faccal` from Factd/Fachol via `addmul`; compute `Stocal`; keep table/punch
   emits deferred; guard the still-unported activations (Priadj>1 makadj/tdlom,
   sliding spans, outlier Adj*) so non-airline specs still fatal cleanly.
3. Un-xfail only `airline_` aictest in `test_x11_tables.py`; gate b1/d10-d13.
   (expgs/payems/unrate stay xfailed — they still need the non-default nloop/tstmd1.)

### For non-default-model aictest series (expgs/payems/unrate) — still deferred
They identify to a non-default model, so ismd0=F and the reduced automd leaves an
incomplete identified model. They need the faithful nloop: `addfix` (rmfix.f/addfix.f
+ fxreg dictionary + insstr/insptr/intlst + dlusrg), round-2 AIC tests on the
identified model, and the tstmd1 finalization. Separate, larger milestone.

## Repro

```
# PATH must have rtools44 ahead of rtools40 (see second-brain
# rtools40-shadows-rtools44-lto) or the LTO link aborts "ld returned 9".
cmake --build build --target x13run_x11
python tools/../<scratch>/diffx11.py airline_automdl-aictest-x11   # per-table worst rel-err
build/x13run_x11.exe tests/corpus/generated/airline_automdl-aictest-x11.spc
```
(diff harness: dumps produced vs golden b1/d10-d13; kept in scratchpad this session.)
