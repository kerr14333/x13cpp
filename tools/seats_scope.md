# SEATS decomposition engine — scope + gate + first increment

## CURRENT STATUS (2026-07-24): SEATS corpus FULLY bit-exact + general-shape CLOSED
payems_seats' s12/s13 floor (the session-15 hard-stop below) was later closed by
the SEATS multiplicative bias correction (sigsub.f, log path) -- all SEATS corpus
specs gate. General-shape decomposition is now bit-exact + gated too: p>0
(`*_ar2-seats`) and bp>0 (`*_sar-seats`) both ~5e-15, closed by the AR/seasonal-AR
polynomial sign fix in canonical_denoms.cpp (phis=+mo.phi, bphis=+mo.bphi) plus
the CALCFX Pstar>0 residual seed. Only imean!=0 remains (guarded-fatal in
run_seats). Parity 923 pass / 0 fail / 0 xfail / 25 skip. General-shape runway +
imean scope: tools/seats_general_scope.md. Session-by-session history follows.

## LATEST STATUS (session 15): payems_seats' s12/s13 floor precisely localized to a TREND-ONLY, uniform log-domain bias EXTERNAL to ESTBUR -- NOT the residual-precision gap the mandate assumed; HARD-STOP, not gated

**Mandate**: close the remaining `payems_seats` s12/s13 residual floor (~2.2e-6
uniform, from session 14) and gate them, on the working hypothesis (session
14's own "precise next step") that it was the SAME `armafl()`-vs-true-CALCFX
residual-precision gap session 9-10 originally measured, just spread across
more of the series by `payems_seats`' larger `lext`.

**Result: that hypothesis is REFUTED by direct oracle ground-truthing --
`armafl_last_residuals` (session 14's bugs #1+#2) reproduces the oracle's TRUE
CALCFX `a(Na)`/`a(Na-1)` residuals bit-for-bit for `payems_seats` too (not
just `unrate_seats`), and ESTBUR's own log-domain `trend(i)`/`cycle(i)` output
is ALSO bit-exact against a fresh oracle instrumentation dump at every point
checked (`i=1,100,200,308` -- both boundaries and 2 interior points). The
entire, exactly-uniform `+2.1978124351562656E-06` log-domain gap is
introduced STRICTLY AFTER ESTBUR returns, is TREND-SPECIFIC (cycle's own
`s14`, back-transformed from the SAME bit-exact `cycle(i)`, is already
correct to `3.3e-16` -- no correction needed there), and does not match any
combination of this project's own already-computed quantities
(`varwnp`/`varwnc`/`varwna`/`qt1`/`enot`/`estar`/`enoc`/`Detpri`/`dnefob`/
`lndtcv`) tried. No candidate code applying such a correction was found
anywhere in `oracle/fortran/` (searched `ansub3.f`/`sigex.f`/`spectrum.f`
fully; the one similarly-named routine, `trbias.f`, is X-11's own log-additive
trend bias correction and is called ONLY from `x11pt3.f`, never from the SEATS
path). This is consistent with, and extends, session 13's own finding that
the s10-s18 tag NAMES don't appear anywhere in this repo's oracle mirror --
apparently the VALUES for at least the trend table also get a final
post-processing step from a higher, non-mirrored X-13 driver layer this
project cannot read. `s12`/`s13` remain xfailed for `payems_seats`; no
regression to any currently-gated table. Full suite unchanged from session
14's end state: `ctest` 10/10, `pytest tests/parity/` 482 passed / 13 skipped
/ 76 xfailed / 0 failed; `pytest tests/parity/test_seats_tables.py` alone 31
passed / 2 skipped / 40 xfailed. All oracle instrumentation reverted
(`git status --porcelain oracle/` empty, confirmed) and all temporary C++
debug hooks removed (`grep -rn "X13_ESTBUR_DEBUG\|DBG_"` over the touched
files returns nothing).**

### Step 1: quantify the floor precisely -- it is UNIFORM to 8-9 significant digits, not decaying, which is itself a major clue

Built `x13run_seats.exe` (already up to date from session 14) and diffed its
`payems_seats` `s12`/`s13` output against golden at all 308 dates (scratch
Python, not committed, deleted after use). Result: `rel_err(s12)` ranges over
just `[2.1978100155e-06, 2.1978100235e-06]` across EVERY ONE of the 308
dates (both s11-boundary points AND every interior point) -- a spread of only
`~8e-15` on a `~2.2e-6` quantity, i.e. genuinely CONSTANT, not decaying near
the boundaries the way `unrate_seats`' now-closed `s13` gap did (session 12's
own characterization: `~29x` decay per step there). `rel_err(s13)` tracks
`rel_err(s12)` almost exactly (`2.197810e-6` vs `2.197815e-6`, agreeing to
5-6 significant figures at every date) with OPPOSITE sign in the raw
`(computed-golden)` difference -- exactly what the `s13 = sa/(trend*cycle)`
identity predicts if `sa` is exact (confirmed, `s11` bit-exact) and `cycle` is
ALSO exact (confirmed later this session) and ONLY `trend` carries a bias.
This ruled out a residual-seeding/boundary-decay mechanism on inspection
alone, before any oracle work: a genuine FCAST-seed error would decay away
from the two boundaries at a rate set by `thstar`'s own coefficients
(`thstr0=[1, 0.0186, -0.1687]` for `payems_seats`, dumped this session --
SMALL coefficients, implying FAST decay, similar in kind to `unrate_seats`'
own fast-decaying case, not a slow one), yet the observed floor does not
decay AT ALL across 308 points spanning 26 years. A uniform, non-decaying gap
across the WHOLE historical span cannot come from a boundary-seed
approximation; it has to come from something applied identically at every
point, i.e. downstream of the whole trend array, not upstream of it.

### Step 2: ground-truth the oracle directly (same discipline as session 14) -- and this time the residual-seeding mechanism itself checks out completely

Built the oracle from source again (gfortran/rtools44 from PowerShell, same
690-file `makefile.gf` SRCS loop as session 14, 0 compile failures this time
-- `adpdrg.f`'s `-fallow-argument-mismatch -std=legacy` fallback still needed,
same 5 pre-existing non-link-set files skipped). Sanity-checked the fresh
binary against golden `payems_seats.s12` byte-for-byte before trusting
anything. Instrumented THREE call sites this session, all temporary
`write(*,*)` lines, all reverted with `git checkout --
oracle/fortran/{analts,ansub3,spectrum}.f` at the end (`git status --porcelain
oracle/` confirmed empty):

1. **`analts.f`, right before both `FCAST` calls** (line 2778 forward, 2847
   backward -- same site session 14 used for `unrate_seats`): dumps `Na`,
   `a(Na)`, `a(Na-1)`, `a(Na-2)`, `Detpri`, and CALCFX's own `Qstar`/`Pstar`
   (the MODEL-order quantities, NOT ESTBUR's `cd.qstar`/`cd.pstar` -- these
   are named identically in the Fortran but are different variables at
   different scope, a distinction worth remembering for future sessions).
   Result: `DBG_FWD_ARES a(Na)=1.4578886680948074E-04 a(Na-1)=
   5.3556679019100054E-04 ... Detpri=1.0000948105163010`;
   `DBG_BWD_ARES a(Na)=-1.1070119602885524E-03 a(Na-1)=-3.9645662178282581E-03
   ... Detpri=1.0000948105163010` (same `Detpri` both directions, as
   session 14 found for `unrate_seats` too). **This project's own
   `armafl_last_residuals(ctx, z/bz, qstar_f=2, ...)` output
   (`aFwd=[-1.4578886680948069e-04, -5.3556679019100054e-04]`,
   `aBwd=[1.1070119602885524e-03, 3.9645662178282581e-03]`) matches these
   EXACTLY, bit-for-bit (to all printed digits, negation accounted for)** --
   session 14's bugs #1 (drop the extra `fac`/`Detpri` scaling) and #2 (seed
   ALL `Qstar_f` trailing residuals, not just the last) generalize perfectly
   to `payems_seats`' `Qstar_f=2` case. **This directly REFUTES the mandate's
   working hypothesis**: there is no remaining residual-precision gap at all
   for `payems_seats` -- the residuals FCAST would actually use are being
   computed exactly right.
2. **`ansub3.f` (inside `ESTBUR` itself), right after `gt`/`gs`/`gc` are
   assembled** (after line 175): dumps `pstar`/`qstar`/`maxpq`,
   `ct`/`cs`/`cc`/`thstar(1..maxpq)`, `gt`/`gs`/`gc(1..maxpq)`, and
   `totden(1..pstar)`. Also added a dump right after the `trend(i)=fxt(i)+
   bxt(Nz-i+1)` assembly loop (after line 361), gated on `Nz>250` (so only
   `payems_seats`, `Nz=308`, prints it, not `unrate_seats`, `Nz=776`... note:
   this guard is backwards from what's needed for other specs, harmless since
   it only ever fired for `payems_seats` this session): `trend`/`cycle`/`z`
   at `i=1,100,200,Nz`. **Every single one of these matches this project's
   own C++ `estbur.cpp` output bit-for-bit** (added the identical debug
   dumps, env-var-gated on `X13_ESTBUR_DEBUG`, in `estbur.cpp`/`spectru.cpp`
   temporarily): `pstar=2 qstar=3 maxpq=3` (payems' `cd.pstar!=cd.qstar` --
   the FIRST gated spec to exercise this asymmetric general-branch case,
   `unrate_seats` had `pstar==qstar==2`), `ct/cs/cc` (`cc(2)=0.0` exactly,
   `cc(3)==thstar(3)` exactly -- both confirmed NOT bugs, just the natural
   shape of the `qstar>pstar` ADDJ-folded cycle numerator), `gt/gs/gc`, and
   `totden=[1,-1]` (the `(1-B)` differencing polynomial, as expected for
   `p=bp=0, d=1`) all bit-exact. **`trend(1)=11.784071235872052`,
   `trend(100)=11.835145915145549`, `trend(200)=11.882279715633818`,
   `trend(308)=11.979911423695295` match the oracle's own internal values to
   the LAST PRINTED DIGIT at all four points, including both boundaries.**
   `cycle(i)` matches bit-exact too at all four points.
3. **`spectrum.f`, right after the `DIVFCN`/`qt` quotient computation** (the
   `qstar>pstar` "top-heavy" branch's own `qt(x)` polynomial, spectrum.f:743):
   dumped `qt(1..nqt)` -- `qt=[0.32184324101396594, 0.33730074365040164]`,
   confirmed bit-exact against this project's own `spectru.cpp` (added the
   same debug dump there). `qt(2)` exactly equals `cc(1)` (expected, since
   `ADDJ` folds `qt(x)*Fc(x)` into `Uc`, and `Fc` is trivial `[1]` here,
   `ncyc==1`). This ruled out `qt`'s discarded constant term (dropped from
   `qt1`'s own reporting in the `qstar>pstar` branch, `spectru.cpp:561`,
   `r.qt1 = enot+estar+estar+enoc` instead of `+qt[0]`) as the missing bias
   too -- `qt[0]=0.322`, six orders of magnitude too big to be the `2.2e-6`
   gap, and already confirmed to be correctly excluded from `qt1` (which
   remains bit-exact against golden `irrvar`).

### Step 3: the gap is TREND-SPECIFIC -- cycle needs NO correction, which rules out a whole class of hypotheses and narrows the search sharply

Computed `ln(golden_s12[i]) - trend_log[i]` (using this project's own
BIT-EXACT-vs-oracle `trend_i` from step 2) at the same four indices
(`i=1,100,200,308`): **`2.197812431603552e-06`, `2.1978124369326224e-06`,
`2.1978124351562656e-06`, `2.1978124351562656e-06`** -- constant to 10+
significant digits (the tiny variation is golden's own 15-sig-fig ASCII
printing, not real). This is an exact, high-precision confirmation of the
`~2.2e-6` floor's constancy, now traced to a SPECIFIC, well-defined quantity:
a log-domain additive bias of `+2.1978124351562656e-06` applied to `trend`
alone before it is written as `s12`. **Checked whether the SAME correction
applies to `cycle` (`s14`, golden exists for `payems_seats` only): `exp(cycle_
log[0]=-8.5185222078054126e-04) = 0.9991485105023197` vs golden
`s14[200001]=0.999148510502320` -- relative error `3.33e-16`, i.e. NO
correction needed for cycle at all.** This is the session's sharpest new
finding: whatever this correction is, it is applied SPECIFICALLY to the
TREND component's log-to-level back-transform, not uniformly to every
WK-filtered component. This rules out any hypothesis of a generic "estimator
uncertainty" correction applied model-wide, and is consistent with (though
not confirmed identical to) the well-known asymmetry in X-11's own
`trbias.f` (which ALSO only corrects trend, never cycle) -- suggesting SEATS'
(non-mirrored) driver layer may apply an analogous, TREND-only convention,
even though the specific formula couldn't be located or reproduced this
session.

### Step 4: searched for a matching closed-form among every quantity already computed by this project -- none matched

Tried, numerically, against the target `2.1978124351562656e-06`:
`varwnp` (`0.18060072974602678`, trend's own MAK1 innovation variance --
target is `~1.2e-5` of it, not a clean fraction), `varwnp/dnefob^2`
(`1.916e-6`, same order of magnitude but off by `~15%`, not exact),
`varwnp/Dof^2` (using `Dof~=305`, `1.94e-6`, still not exact), `varwnc`
(`0.1686`), `varwna` (exactly `1.0`, a placeholder in the `npsi==1` branch,
not a real variance), `qt1` (`0.1651`), `enot`/`estar`/`enoc`
(`0.1806`/`0`/`-0.01546`), `Detpri`/`fac-1` (`9.481e-5`, confirmed identical
to `exp(lndtcv/(2*dnefob))-1` again, matching session 14's `fac==Detpri`
finding, but six orders of magnitude too big and no clean ratio to the
target), `sum(gt^2)*varwnp` (`0.0143`), and several sample-size-normalized
variants of these. **None matched to more than 1-2 significant figures.**
This is not a proof no closed form exists among these quantities (a
systematic symbolic search was not attempted), but it means the correction is
not a simple, obvious function of anything this project currently computes.

### Why this is a HARD-STOP, not a partial fix

The mandate's own explicit fallback ("HARD-STOP and report with the exact
numbers rather than forcing it") applies squarely here: this session
conclusively shows (1) the residual-seeding mechanism session 14 fixed is
CORRECT and COMPLETE for `payems_seats` too (bit-exact vs the true oracle
CALCFX residual, no lingering approximation), (2) ESTBUR's own ported
arithmetic (System A, the general-branch `MLTSOL` boundary solve, the
backward recurrence, `ct`/`cs`/`cc`) is bit-exact against the oracle at every
point checked, including the FIRST-ever-gated `pstar!=qstar` asymmetric case,
and (3) the entire remaining gap is a single, precisely-measured, TREND-ONLY,
non-decaying log-domain constant that is not produced by ANY code in this
project's `oracle/fortran/` mirror (searched exhaustively) and does not match
any quantity this project already computes. Closing it would require either
locating the actual (non-mirrored) X-13 driver-layer routine that applies it,
or reverse-deriving its exact formula from first principles (e.g. a proper
Wiener-Kolmogorov estimation-error-variance calculation for the trend
filter specifically) -- both explicitly out of this session's remaining
budget and neither guaranteed tractable. Hardcoding the empirical constant
for `payems_seats` alone would not be a real port (it wouldn't generalize to
any other log-transform spec with a real cycle), so it was not done.

### Precise next step

1. **Try to locate the actual bias-correction formula.** Two concrete leads
   not yet exhausted: (a) Burman (1980)/Maravall's own SEATS papers document
   a trend-only log-back-transform bias correction in closed form (titled
   something like a "conditional expectation" correction) -- worth a
   literature check outside this codebase before any further hand-derivation
   attempt. (b) `varwnp` is `0.18060072974602678`, suspiciously equal to
   `ct(2)` and `enot` (both also `0.18060072974602678`) -- this coincidence
   (or lack of an independent third value) might mean the RIGHT variance
   quantity for a proper WK-filter estimation-error formula is a DIFFERENT,
   not-yet-computed combination (e.g. integrating `|1-GT(e^{iw})/GT_infinite|
   ^2` type expressions over frequency), not simply `varwnp` itself -- this
   project doesn't currently compute anything like that.
2. **Get a second log-transform-with-cycle data point.** Only `payems_seats`
   currently reaches ESTBUR successfully among log-transform specs with a
   real cycle; if `payems_fixed-airline-seats` (real seasonal, currently
   fatals earlier in the chain, `npsi!=1` untested) or another spec can be
   brought up to the same ESTBUR-succeeds point, a SECOND non-trivial
   trend-bias data point would let candidate formulas be checked by ratio
   instead of by absolute match against a single number -- much more
   powerful than this session's single-spec search.
3. If a closed form is found, apply it inside `estbur_historical`'s
   `is_log` branch (only affects `out.trend`, and `out.ir` via the `sa/
   (trend*cycle)` ratio -- `out.sa`/`out.cycle`/`out.seasonal_factor` need
   no change, confirmed this session).

### Discipline: oracle and debug-instrumentation cleanup

Reverted all THREE instrumented oracle files with `git checkout --
oracle/fortran/analts.f oracle/fortran/ansub3.f oracle/fortran/spectrum.f`;
deleted the scratch `_dbgbuild/` directory (690 `.o` files + the temporary
`x13as_ascii_DBG.exe`). Confirmed `git status --porcelain oracle/` empty.
Removed all temporary `X13_ESTBUR_DEBUG`-gated debug `fprintf`s from
`estbur.cpp`/`spectru.cpp`/`x13run_seats.cpp` (and their now-unneeded
`<cstdio>`/`<cstdlib>` includes) -- confirmed via `grep -rn
"X13_ESTBUR_DEBUG\|DBG_"` over those three files returning nothing. Deleted
the scratch diagnostic script (`tools/scratch_payems_diag.py`) and stray
debug-output text files. Full rebuild + `ctest`/`pytest` reconfirmed
identical to session 14's end state (482/13/76/0; no gate changes, since
nothing was fixed this session).

---

## LATEST STATUS (session 14): unrate_seats' s13 CLOSED (bit-exact, ~4.8e-15 worst) -- the "residual-precision contradiction" from sessions 12-13 was a real, fixable bug (an extra `fac` scaling CALCFX's own round trip cancels out), found via direct oracle ground-truthing, not another hand-derivation; a second, independent FCAST under-seeding bug fixed too (helps payems_seats ~100x, doesn't fully close it)

**Mandate**: deep-dive the s13 boundary discrepancy / CALCFX-residual
contradiction. Priority hypothesis: check whether golden `s13 == s11-s12` at
FULL double precision (in which case the port should just compute the
identity instead of an independent extraction) or only to the documented
~2.78e-8 (in which case the oracle uses a separate irregular-extraction path,
and the residual-array trace becomes the next step).

**Result: this session ran in two phases.** Phase 1 (below, "Steps 1-3")
confirmed the identity hypothesis exactly as asked, found nothing to
"switch" (the C++ already computes `s13` as the identity), and produced an
EXACT (not circumstantial) characterization of the gap as cancellation
amplification of `s12`'s own tiny trend error -- and, on that basis, phase 1
concluded (as sessions 12-13 had) that closing it required the unresolved
CALCFX-residual contradiction, and initially re-affirmed the HARD-STOP.
**The coordinator then relayed an independent trace (from a `codex` review)
that reframed the problem** -- ESTBUR doesn't consume residuals directly;
FCAST uses them only to seed a short end-extension of `z`/`bz`, and `s13` is
computed INSIDE ESTBUR as the same `z-trend-cycle` identity phase 1 had
already found. This redirected the session toward ground-truthing the
oracle's ACTUAL residual/extension values directly instead of re-deriving
CALCFX by hand (which is what failed in session 13). **That ground-truthing
resolved the contradiction**: it was never a fundamental CALCFX-vs-armafl
precision gap at all -- `estbur.cpp`'s residual-scaling recipe was reusing
`fcnar()`'s `fac=exp(lndtcv/2/dnefob)` factor, copied from the REGULAR
ARIMA-estimation residual path, which turned out to be EXACTLY CALCFX's own
internal `Detpri` (bit-for-bit, confirmed via direct oracle instrumentation)
-- a factor CALCFX's own round trip (STEP9 multiply, caller divide) always
cancels back out before ever handing the residual to FCAST. Applying it here
was accidentally reintroducing a factor the oracle itself never keeps.
Dropping it entirely closes `unrate_seats`' `s12`/`s13` to double-precision
noise (worst ~4.8e-15 across all 776 dates) -- **now gated, un-xfailed**. A
second, independent bug (FCAST needs MULTIPLE trailing historical residuals
as extension seeds whenever `Qstar_f>=2`, not just the very last one) was
also found and fixed; it doesn't affect `unrate_seats` (`Qstar_f=1`) but
collapses `payems_seats`' s12/s13 error from a ~2.1e-4 spike down to a
uniform ~2.2e-6 floor (still not gated -- a separate, smaller residual gap
remains for that spec). All oracle edits were reverted (`git status
--porcelain oracle/` empty, confirmed after rebuilding + sanity-checking the
oracle binary against golden). Full suite: `ctest` 10/10, `pytest
tests/parity/` 482 passed / 13 skipped / 76 xfailed / 0 failed (up from
481/13/77/0 -- the +1 net pass is `(unrate_seats, s13)` un-xfailed).**

### Step 1: does the golden identity hold at full precision? YES, directly verified

Read golden `unrate_seats.s11`/`.s12`/`.s13` and computed `s11-s12` by hand at
both documented boundary dates:

- `196101`: `s11=6.6`, `s12=6.66992016163405` (both golden, full printed
  precision) -> `s11-s12=-0.06992016163405`. Golden `s13[196101]=
  -0.0699201616340464`. Matches to every digit the lower-precision operand
  (`s12`) supports -- i.e. as far as printed precision can show, this IS the
  identity, not an approximation.
- `202508`: `s11=4.3`, `s12=4.27753737646788` -> `s11-s12=0.02246262353212`.
  Golden `s13[202508]=0.0224626235321166`. Same result.

This answers the mandate's fork question directly: golden `s13` is the
additive identity `s11-s12`, at least to the precision the golden ASCII files
themselves carry (15 significant digits) -- there is no sign of the oracle
computing `s13` via a genuinely separate irregular-signal-extraction formula
that would diverge from the identity at the boundary. (This is exactly what
session 13's cross-spec reverse-engineering already established for the
GENERAL formula; this session's contribution is confirming it holds
PRECISELY at the two specific dates that fail the gate, where an
extraction-formula divergence would be most likely to show up if one
existed.)

### Step 2: does the mandate's prescribed fix ("switch the C++ to compute the identity") apply? NO -- it already does, confirmed by reading the code

Re-read `core/src/seats/estbur.cpp`'s `estbur_historical()` line by line
(not just the header comment) before assuming anything: lines 313-323
compute `trend_i`/`cycle_i`/`sc_i` from the `MLTSOL` solve, then
**`ir_i[i-1] = z[i-1] - trend_i[i-1] - cycle_i[i-1]`** and `sa_i[i-1] =
z[i-1]` DIRECTLY -- literally `sa - trend - cycle`, the exact identity, using
the SAME `trend_i` array that gets reported as `s12` (`out.trend[i] =
trend_i[i]` a few lines later, unchanged). For `unrate_seats`
(`npsi==1`/no seasonal, no real cycle -- `cycle_i` numerically ~0), this
collapses to precisely `s13 = s11 - s12` in the C++ port too, not merely in
the oracle. **There is no independent extraction path in this codebase to
"switch" -- the mandate's Step-1 fix (un-xfail by computing the identity) is
already the status quo, and doing nothing changes nothing.** This was
verified by inspection of the exact lines, not inferred from behavior.

### Step 3 (the actual new finding this session): the s13 gap is not just "the same root cause" as s12's (session 13's framing) -- it is EXACTLY, ARITHMETICALLY s12's own absolute error, unchanged, re-expressed as a relative error against a much smaller denominator

Built (already up to date) and ran `x13run_seats.exe` on `unrate_seats`,
diffing its own `s12`/`s13` output against golden at every one of the 776
dates (Python, `tools/`-adjacent scratch script, not committed):

```
date      abs_err(s12)   rel_err(s12)   abs_err(s13)   rel_err(s13)   |trend/irr|   rel_err(s12)*ratio
196101   -1.944e-09      2.915e-10       1.944e-09      2.781e-08        95.39         2.781e-08
196102    6.736e-11      9.867e-12      -6.736e-11      9.213e-10        93.37         9.212e-10
196103   -2.330e-12      3.364e-13       2.334e-12      9.067e-11       269.02         9.049e-11
202506    7.470e-13      1.801e-13      -7.496e-13      1.580e-11        87.44         1.575e-11
202507   -2.164e-11      5.154e-12       2.164e-11      2.505e-08      4859.62         2.505e-08
202508    6.246e-10      1.460e-10      -6.246e-10      2.781e-08       190.43         2.781e-08
```

Three things this table proves, not just suggests:

1. **`abs_err(s13) == -abs_err(s12)` to 3-4 significant figures at every
   date checked**, exactly what the identity `s13 = z - trend` (`z` exact,
   `trend`'s error is `abs_err(s12)`) predicts by construction -- confirming
   (again, now numerically) that `s13`'s error is NOT independent of
   `s12`'s; it is the identical perturbation, sign-flipped by the
   subtraction.
2. **`rel_err(s13) ≈ rel_err(s12) × |trend/irregular|`** (the last two
   columns match to 3-4 sig figs everywhere, including at noise-floor
   points like `196103`/`202506` where both sides are ~1e-10 to 1e-13) --
   i.e. `s13`'s relative-error gate failure is a PURE ARTIFACT of
   `s12`'s own (tiny, gate-passing) absolute error being divided by a much
   SMALLER denominator (`|irregular|`, ~0.02-0.07 near the boundaries) than
   the one `s12`'s own gate check divides by (`|trend|`, ~4-7). No part of
   this amplification comes from `s13`'s own computation being wrong in a
   way `s12`'s isn't -- they share the exact same numerator.
3. **This mechanism, not "boundary proximity" per se, is why exactly 3
   dates fail** (`196101`, `202507`, `202508` -- re-ran the FULL 776-date
   comparison to get an exact count, matching session 12's own "773/776"
   arithmetic that the "2 boundary dates" prose undercounted): `202507` is
   one point in from the `202508` boundary, where the FCAST-seed error
   hasn't fully decayed AND `|irregular|` happens to be unusually small
   (`8.64e-4`, vs `~0.02-0.07` at the other two failing dates), so the
   `4860x` amplification factor there is far larger than at the other two
   dates, yet still produces almost exactly the same ~2.5e-8-2.78e-8
   relative gap -- consistent with a single shared absolute-error source,
   not three independent problems.

### Corroboration on `payems_seats` (log-transform, real cycle): the SAME mechanism, but the multiplicative form doesn't amplify the same way -- explaining the documented "floor" vs unrate's "spike" shape

Ran the same diff on `payems_seats`. There, `s13` is a RATIO
(`sa/(trend*cycle)`, session 13's log back-transform), not an additive
residual near zero -- so a relative error in `trend` propagates into the
ratio at roughly its OWN relative size (no cancellation-driven
amplification, since the ratio's magnitude is ~1.0, not a small residual):
`rel_err(s12)` and `rel_err(s13)` are the same ORDER of magnitude at every
date checked (`200001`: `2.149e-4` vs `1.185e-4`; `202508`: `2.654e-5` vs
`1.351e-5`), never diverging by 2-4 orders of magnitude the way unrate's
additive case does. This is exactly consistent with session 13's own
characterization of payems's gap as a "floor across most of the series"
(rather than isolated spikes at 2-3 points): the multiplicative/ratio form
of the identity doesn't have a small-denominator amplification mechanism,
so `s12`'s ~1e-4-to-1e-6 relative error shows up roughly 1:1 in `s13`
everywhere, rather than being hidden except where `|irregular|` happens to
be tiny.

### Phase 1's (superseded) conclusion, for the record

Phase 1 ended here believing the mandate's decision tree had bottomed out at
"the contradiction is real" -- `s13` mechanically downstream of `s12`'s own
trend value with no independent logic of its own, so closing `s13`'s gate
would require closing `s12`'s residual error by ~30-5000x depending on the
date, apparently needing the TRUE CALCFX residual that session 13's from-
scratch port had already failed to obtain. This turned out to be an
UNDER-estimate of what was fixable: the "genuine contradiction" was a real,
identifiable bug (see below), not a fundamental algorithmic gap. The lesson
for future sessions: "a textually-faithful port disagreed with the working
baseline" is not by itself proof that closing the gap needs re-deriving the
hard algorithm from scratch -- it can also mean the WORKING baseline itself
has a fixable bug that the from-scratch port's disagreement was actually
pointing at.

### Phase 2: the coordinator's redirect, and ground-truthing the oracle directly instead of re-deriving CALCFX by hand

The coordinator relayed an independent static trace (external review) making
two claims: (1) ESTBUR's formal arguments are `z, bz, zaf, zab` -- there is
no residual array in its signature at all (`ansub3.f:54-74`); residuals only
seed FCAST's short end-extension recursion (`ansub1.f:2066-2085,
2183-2201`), confirming (independently) phase 1's own finding that `s13`'s
identity is computed INSIDE ESTBUR from `z`/`trend`/`cycle`, with no separate
extraction logic. (2) Since "inject the exact CALCFX residual" was the right
idea structurally, the session-13 regression must mean the from-scratch
port's VALUE was wrong (mismatched runtime inputs -- `Wd`, `Type`, `Detpri`,
`Jfac`, etc.), not that residuals are fed to ESTBUR differently than assumed.
The concrete recommendation: ground-truth the oracle's own FCAST-fed
residual/extension values directly (instrumentation), rather than trying to
re-derive CALCFX's STEP1-9 by hand a second time.

**Re-read `analts.f:2144/2778-2780` (forward CALCFX call -> FCAST) and
`analts.f:2814/2843-2845` (backward)**: confirmed `zaf`/`zab` are SCALAR
"mean correction forecast/backcast" terms (`sigex.f:34-35`), not extended-z
arrays -- for `imean=0` (every corpus spec so far) these are exactly the
`za=0` this project's own `fcast_extend()` already assumes, so no bug there.
Read `FCAST` itself (`ansub1.f:2066-2225`) closely: its end-extension loop
(`z(kk)=za - sum_j thstar(j)*a(k-j) + ...`) references `a(k-j)` for
`j=1..Qstar`, which is HISTORICAL (needs a real residual, not the "future
residuals are 0" convention) whenever `i<=j` at extension step `i` --
confirming this project's own already-ported `fcast_extend()` structure is
right, but revealing bug #2 below.

**Built the oracle from source** (gfortran/rtools44, PowerShell, same
discipline as session 12 -- `-fallow-argument-mismatch -std=legacy` needed
this time for one file, `adpdrg.f`, that a plain `-O2` compile rejects on
this gfortran version; the other 5 pre-existing compile failures
(`htmlutil.f`/`prttbl2.f`/`prttblsum.f`/`spectrum2.f`/`upmeta.g77.f`) are
NOT in `makefile.gf`'s actual link set, confirmed by grep, so safely
skipped). **Instrumented `analts.f`** with two temporary `write(*,*)` lines,
right before each `FCAST` call (forward at line 2778, backward at 2844),
dumping `Na`, `a(Na)`, `a(Na-1)`, and `Detpri` -- i.e. the EXACT residual
array elements and normalization factor FCAST actually receives, sidestepping
any need to re-derive CALCFX's internal STEP1-9 matrix machinery by hand.
**Sanity-checked the freshly-built binary first** (reproduces golden
`unrate_seats.s12`/`.s13` byte-for-byte) before trusting the dump. Result for
`unrate_seats`:

```
DBG_FWD_ARES  Na=776  a(Na)=0.096415441065110477  a(Na-1)=0.10346466371355040  Detpri=1.0000007748481317
DBG_BWD_ARES  Na=776  a(Na)=-0.30011557704519498  a(Na-1)=0.0033360143691992536 Detpri=1.0000007748481317
```

### Bug #1 (the real "residual-precision" gap): `estbur.cpp` was applying an extra `fac` scaling that CALCFX's own round trip already cancels out

Compared this ground truth to this project's own `armafl_last_residual`'s
output (temporarily dumped via an env-var-gated debug print in
`estbur.cpp`, removed after): `-0.096415515772434845` (forward),
`0.30011580958918915` (backward) -- both off from the oracle's true value
by a consistent **~7.75e-7 relative gap, same sign both directions**. This
number is suspicious: `Detpri - 1 = 7.748481317e-7`, matching to 4-5
significant figures. Computed `exp(lndtcv/2/dnefob)` (this project's own
`fac` formula, `ctx.mdldat.lndtcv=0.0012010141386863695`,
`ctx.series.dnefob=775` for `unrate_seats`, dumped via the same probe) --
**it equals `Detpri` EXACTLY, bit-for-bit to all 17 printed significant
digits (`1.0000007748481317` both ways)**. This is not a coincidence at
that precision: `fac` and CALCFX's `Detpri` are the SAME conceptual
quantity (a Cholesky/Gram-matrix-determinant-based exact-ML normalization),
just derived via two different routes (`armafl`/`intgpg`'s own machinery vs
CALCFX's STEP1-9 `b`-matrix Cholesky). But CALCFX's OWN internal round trip
(STEP9 `a(i)=Detpri*a(i)`, `ansub1.f:1469`, then the CALLER immediately
`a(i)=a(i)/Detpri`, `analts.f:2386`/`2829`) means the residual FCAST
actually receives is the RAW, UNSCALED value -- session 10 already
established this cancellation for CALCFX's own path, but `estbur.cpp`'s
`armafl_last_residual` was reusing `fcnar()`'s recipe (`regarima/
estimate.cpp:162-192`, correct for the REGULAR ARIMA-estimation residual
use it was written for) and applying `fac` on top of an already-raw
`armafl()` residual -- reintroducing exactly the factor CALCFX's own round
trip cancels back out.

**Decisive confirmation, in two steps**: (1) substituted the oracle's dumped
ground-truth `a(Na)` values directly into `estbur_historical()` (temporary,
hardcoded to `unrate_seats` only, reverted regardless of outcome) --
`s12`/`s13` dropped from a ~2.78e-8 worst-case gap to **~4.8e-15** (double-
precision noise) at every one of 776 dates. (2) Dividing this project's own
(formerly `fac`-scaled) residual by its own `fac` reproduced the oracle's
dumped value to `diff==0.0` (exact). **Fix**: `armafl_last_residuals`
(renamed, see bug #2) now returns the RAW `armafl()` residual, negated only
(session 12's still-empirically-determined sign convention, unaffected) --
no `fac` scaling at all. This is a real, generic fix (not a per-spec magic
number): `dnefob`/`lndtcv` are already correctly computed by the shared
`armafl()`/`intgpg` machinery for every spec, this just stops re-applying a
factor that was never supposed to survive to the FCAST-seeding use.

### Bug #2 (found while investigating payems' floor): FCAST needs the FULL trailing residual window as extension seeds when `Qstar_f>=2`, not just the very last one

Re-reading FCAST's own recursion (`ansub1.f:2183-2201`) revealed a SEPARATE,
independent bug: at extension step `i` (`k=na+i`), the sum `sum_{j=1}^
{Qstar} thstar(j)*a(k-j)` references `a(na+i-j)`, which is HISTORICAL
(a real residual, not the "future residuals are 0" convention) whenever
`i<=j`. Concretely: step `i=1` needs ALL of `a(Na), a(Na-1), ...,
a(Na-Qstar_f+1)` -- not just `a(Na)`. The old `armafl_last_residual`
returned only ONE value (`a(Na)`), silently leaving `a(Na-1)`, etc. as 0.
For `unrate_seats` (`Qstar_f=q+bq*mq=1`, model `(0 1 1)`) this coincides
with the single-residual behavior exactly (nothing was wrong there). For
`payems_seats` (`Qstar_f=2`, model `(0 1 2)`) it does NOT: `a(Na-1)` was
really needed and really nonzero, and treating it as 0 was a genuine bug,
independent of bug #1. **Fix**: `armafl_last_residual` -> generalized to
`armafl_last_residuals(ctx, series, count, out)`, returning `out[0..
count-1] = a(Na), a(Na-1), ..., a(Na-count+1)` (all raw+negated, see bug
#1); `fcast_extend` now seeds `aext[qstar_f-m]` for all `m=0..qstar_f-1`
instead of only `aext[qstar_f]`; `estbur_historical` calls with
`count=qstar_f=mo.q+mo.bq*mo.mq`.

**Effect measured, before vs after (bug #2 alone, bug #1 not yet applied):**
`payems_seats`' `s12`/`s13` worst-case error dropped from `~2.15e-4` (at the
very first point, decaying to a ~2e-6 floor across the interior, rising to
`~2.65e-5` at the last point -- session 13's own characterization) to a
**UNIFORM ~2.2e-6 across essentially the ENTIRE 308-point series** (every
date checked from `200001` to `202508` lands within `2.18e-6`-`2.20e-6` of
each other) -- i.e. bug #2 was the dominant cause of payems' worst-case
spikes near the boundaries, collapsing them down to the SAME residual floor
the rest of the series already had. **Applying bug #1's fix on top does
NOT further reduce this ~2.2e-6 floor** (worst stayed `2.198e-6`-`2.201e-6`)
-- `payems_seats` has a remaining, independent, unclosed residual-precision
gap of its own (plausibly the genuine armafl()-vs-CALCFX approximation
session 9-10 originally measured, now isolated from the two bugs that were
previously conflated with it) that this session did NOT resolve. `s12`/`s13`
stay xfailed for `payems_seats` only.

### Discipline: oracle build artifacts and instrumentation, fully reverted

`analts.f`'s two temporary `write(*,*)` lines were reverted with `git
checkout -- oracle/fortran/analts.f`. All build byproducts from rebuilding
the oracle (706 `.o` files, the temporary `x13as_ascii_DBG.exe`, a scratch
`_objs_line.txt`/`build_errs.log`) were deleted, leaving only the two
original, pre-existing `x13as_ascii_O0.exe`/`x13as_ascii_O2.exe` binaries in
`oracle/fortran/`. Confirmed `git status --porcelain oracle/` is EMPTY
after cleanup.

### Precise next step

1. `payems_seats`' remaining ~2.2e-6 floor (bugs #1/#2 both applied, still
   not gated): worth a NEW oracle ground-truth dump (same instrumentation
   pattern used this session, at the same `analts.f` call sites) for
   `payems_seats` specifically, to see whether its own `a(Na)`/`a(Na-1)`
   match this project's post-fix output as closely as `unrate_seats`' did,
   or whether there's a THIRD bug (Qstar_f=2 specifically, or the real
   cycle) still hiding. Given how cleanly bugs #1/#2 resolved for
   `unrate_seats`, this floor is likely closeable the same way, not a
   restatement of the original "needs true CALCFX" belief.
2. Consider whether `armafl_last_residuals`' new multi-residual seeding
   generalizes correctly for `Qstar_f>=3` (no corpus spec currently
   exercises this -- `unrate_seats`=1, `payems_seats`=2 are the only two
   gated series) -- the logic is written generically (`for m<qstar_f`) but
   untested past 2.
3. The now-confirmed `fac`-vs-`Detpri` equivalence (bug #1) may be relevant
   elsewhere in the codebase wherever `armafl()`'s `fac`-scaled residual is
   reused OUTSIDE its original `fcnar()`/regular-ARIMA-estimation context --
   worth a quick grep-and-check by whoever next touches `armafl_last_
   residuals`' call sites, in case this pattern recurs.

---

## LATEST STATUS (session 13): s18/s10/s16 (seasonal factor) landed for unrate_seats; payems_seats gated as the 2nd series (s11/s10/s16/s18 bit-exact); s12/s13 blocked on the SAME residual-precision gap for BOTH series -- a from-scratch CALCFX port attempt to close it produced a genuine, unresolved contradiction and was reverted

**Mandate**: un-xfail `unrate_seats`'s `s13` (root-cause the 2-boundary-date
2.78e-8 gap: sign/negation issue vs boundary-extension issue, then fix),
un-xfail the remaining `unrate_seats` s-tables (s10/s14-s18), then gate
`payems_seats` as the second series (log/multiplicative back-transform).

**Result: `s13` was NOT closed -- root-caused to a genuine, unresolved
contradiction between a from-scratch CALCFX port and the working baseline
(see below), reverted per the standing "HARD-STOP on a mathematical
contradiction" instruction. Instead, `s10`/`s16`/`s18` (all three found to be
the SAME "seasonal factor" table, `z/sa`) were derived, wired, and gated for
`unrate_seats` (`s18`, trivially `1.0`), and the log back-transform was
implemented and gated for `payems_seats` as the second series: `s11`/`s10`/
`s16`/`s18` bit-exact (worst ~1e-15), but `s12`/`s13` do NOT gate for
`payems_seats` either -- same root cause as `unrate_seats`'s `s13` gap, just
manifesting as a ~2e-6 floor across most of the series instead of 2 isolated
boundary points. Full parity suite: 481 passed / 13 skipped / 77 xfailed / 0
failed (up from 473/13/82/0 at session 12's baseline -- +8 net passes: the 4
`unrate_seats`/`payems_seats` gates un-xfailed, plus the previously-passing
ones). Unit suite: 10/10 passing. No oracle files modified (`git status
--porcelain oracle/` empty throughout).**

### s13's root cause: NOT a sign bug, NOT a boundary-indexing bug -- a genuine residual-precision limitation, confirmed (not just suspected) via a failed independent-port attempt

The mandate asked to determine whether `s13`'s 2-boundary-date 2.78e-8 gap is
a residual sign/negation issue or a boundary-extension issue. Both were
investigated concretely:

1. **Not a boundary/indexing bug.** Dumping `s12`'s own per-date error
   (`trend`, not `irregular`) shows the SAME absolute error the boundary
   gap has always shown (~2e-9 at `196101`, decaying geometrically at
   almost exactly `1/thstar(1)` per step -- confirmed via the ratio between
   consecutive points, `1.944e-9 / 6.7e-11 ≈ 29 ≈ 1/0.0346452...`). This is
   the signature of a single, small perturbation at the FCAST extension
   seed decaying through the model's own MA filter -- not an off-by-one or
   a wrong extension length (`lext = qstar+maxpq-2 = 2`, confirmed correct
   by rederiving the filter-application loop's own index bounds).
2. **Not a sign/negation bug either, at least not a NEW one**: the existing
   `armafl_last_residual`'s empirically-determined negation (session 12)
   was re-verified still necessary and still sufficient to reach the
   documented ~2.78e-8 floor -- no alternate sign convention closes it
   further.
3. Given (1)/(2), the mandate's own diagnosis menu ("sign issue" vs
   "boundary issue") turned out to be a false dichotomy: the real cause is
   that `armafl()`'s residual is only an ~1e-6-relative-accurate STAND-IN
   for CALCFX's own residual (session 9-10's finding), and this session's
   new evidence (below) confirms that gap is real and non-negligible at
   the per-observation level, not just in the aggregate SSR check sessions
   9-10 used.

### A from-scratch CALCFX STEP4-9 port: textually faithful, self-consistently verified, but DISAGREES with the working baseline -- reverted

Read `CALCFX` (`ansub1.f:964-1479`) in full this session (not just the
STEP4-9 residual-generation core sessions 9-10 had already scoped). Two
findings that made a NARROW, tractable port look newly attractive:

- **`Type` (the `/calc/` common-block flag selecting CLS vs exact-ML) is
  never assigned anywhere in the whole `oracle/fortran/` tree** (the only
  `Type=` assignments found, `calcsc.f`/commented-out `ansub5.f`, are an
  unrelated LOCAL variable of the same name in a root-polynomial routine) --
  so `Type` is whatever a zero-initialized COMMON block gives (`0`,
  virtually guaranteed under gfortran's BSS zero-fill), putting `unrate_seats`
  in the "exact ML" `Type.ne.1` branch (STEP 5-7), not the trivial
  `Type.eq.1` CLS shortcut.
- **`Jfac` does NOT distinguish forward vs backward for `Init.eq.2`** (the
  fixed-model case SEATS always uses here): re-reading `analts.f:2131-2161`
  (the FORWARD `CALCFX` call, previously not read this closely) shows it
  ALSO sets `Jfac=1` before calling `CALCFX` -- `Jfac=0` only occurs in the
  `Init.ne.2` (re-estimation) `else` branch, dead for this project's fixed-
  model SEATS flow. So both directions genuinely run the identical STEP4-9
  recursion; no missing per-direction branch was hiding an explanation for
  the negation.
- For `unrate_seats`'s restricted case (`Pstar_cf=p+bp*mq==0`, `Qstar_cf=
  q+bq*mq==1`), STEP4-9 collapses to a small (~50-line), self-contained
  "exact ML for MA(q) via backforecast presample correction" recursion --
  ported DENSE (skipping CALCFX's own `ith`/`jcol` sparsity shortcut
  entirely, since it only skips exact-zero multiplications -- an IEEE
  no-op, so the dense form is provably bit-identical to the oracle's sparse
  form for far less code) as `calcfx_last_residual()`. `Detpri` (STEP 6/9)
  is omitted per session 10's already-established "cancels against the
  caller's own immediate `a(i)/=Detpri`" finding.
- **Independently verified in Python** (a from-scratch re-derivation of the
  same recursion, NOT copy-pasted from the C++) against the real
  `unrate.dat` span: reproduces the C++ port's own `a(n)` to the last
  printed digit (`0.103344500638357` both ways) -- ruling out a C++-specific
  transcription bug in the port itself.
- **Wiring it in (with the SAME empirically-required negation pattern
  applied) makes `s12` measurably WORSE, not better**: un-negated, it
  reproduces session 12's own documented "un-negated" 7.5e-4 gap number
  EXACTLY (`s12[196101]` off by 7.5e-4 either way) -- a striking coincidence
  suggesting BOTH algorithms independently need the same correction, not
  that either is simply buggy in isolation. Negating both directions (the
  only combination close to workable) still leaves `s12`'s worst error at
  ~1.35e-5 (`202508`) and ~1.04e-8 (`196101`) -- BOTH worse than the
  existing `armafl`-based baseline's ~3e-10. No sign combination (fwd-only,
  bwd-only, both, neither) matches or beats the working baseline.

**This is the genuine contradiction being flagged rather than iterated on
further**: a textually-faithful, independently-verified port of CALCFX's own
documented algorithm produces a DIFFERENT (and empirically worse) answer than
the already-validated `armafl`-based approximation, for a case (`Pstar=0`,
`Qstar=1`) simple enough that a transcription error seems unlikely (verified
twice, in two languages). Plausible explanations NOT resolved this session:
(a) some upstream mutation of `Wd`/`Thstar` (e.g. a mean-adjustment, or a
COMMON-block aliasing/reentrancy interaction between the forward and
backward `CALCFX` calls sharing the same `/calc/` state) not traced in this
read; (b) a wrong assumption that `Type` is really `0` at runtime (plausible
if some other translation unit DOES initialize it, not found by grepping
`oracle/fortran/`); (c) the STEP1-3 "build `Thstar`/`Phist` from `Th`/`Phi`"
prelude (skipped in this port on the assumption that `Init.eq.2` bypasses
`TRANSC` but not the STEP1B/3B copy-in) does something to `Thstar` this
session didn't verify against `cd.thstar` closely enough. **Reverted
entirely** (the file is back to session 12's exact working state, confirmed
by rebuilding and reconfirming `ctest`/`pytest` match session 12's own
numbers before making further changes) rather than landed regressed or
half-working. See "Precise next step" below for how to pick this back up.

### A genuinely new, reusable finding: the s10/s11/s12/s13/s16/s18 table mapping, reverse-engineered from golden data (not guessed)

No literal `'s10'`/`'s16'`/`'s18'` string was found anywhere in
`oracle/fortran/` (the tag names are evidently assigned by a higher, non-
SEATS-oracle X-13 driver layer not in this project's oracle mirror) --
so this session reverse-engineered the mapping empirically, cross-checking
arithmetic identities against golden data across `unrate_seats` (additive,
no seasonal), `unrate_fixed-airline-seats` (additive, REAL seasonal),
`airline_seats` (log/multiplicative, real seasonal), and `payems_seats`
(log/multiplicative, real cycle) -- four different additive/multiplicative x
seasonal/no-seasonal combinations, to make sure the formula wasn't an
artifact of one combination:

- **`s11`=SA, `s12`=trend** (already known). **`s13`=irregular**: additive
  DIFFERENCE `s11-s12` for a no-transform spec (confirmed exactly on
  `unrate_fixed-airline-seats`: `6.63081269306579-6.69920694452336=
  -0.06839425145757` matches `s13[196101]` to every printed digit), RATIO
  `s11/s12` (or `s11/(s12*s14)` when a real cycle exists) for a log-
  transform spec (confirmed exactly on `airline_seats`: `123.822467812915/
  123.637024258309=1.0014999030890501` matches `s13[194901]` to every
  printed digit; the cycle-adjusted version confirmed on `payems_seats`).
- **`s10`/`s16`/`s18` are the SAME "seasonal factor" table, numerically
  IDENTICAL to each other on every spec checked** (`unrate_fixed-airline-
  seats`: `s10[196101]==s16[196101]==-0.0308126930657892`;
  `airline_seats`: `s10==s16==s18==0.904520819026333` at `194901`) --
  there is no calendar/regression factor in any of the 8 corpus specs to
  distinguish "decomposition-alone" (`s10`) from a "combined final" factor
  (`s16`/`s18`), so they collapse to one array. **The formula is
  `z_original/sa_reported`** (original series over the seasonally-adjusted
  series, BOTH already in original/back-transformed units) -- confirmed
  exactly on `unrate_fixed-airline-seats` (`6.6/6.63081269306579=
  0.99535310459033`, matching `s18[196101]` bit-for-bit) and on
  `airline_seats` (`112/123.822467812915=0.904520819026333`, matching
  `s10[194901]`/`s16`/`s18` bit-for-bit; `112` is `airline.dat`'s own first
  value). For `npsi==1` (no real seasonal -- `unrate_seats`/`payems_seats`),
  `sa==z` identically, so this is trivially `1.0` EVERY point, matching
  golden exactly with no further work.
- **`s14`** (only `payems_seats` ships a golden for it): confirmed to be the
  CYCLE factor specifically -- `s13[t]*s14[t] == s11[t]/s12[t]` to 6+ digits
  on `payems_seats` (`0.999815003773905*0.999148510502320≈0.998964`, matching
  `SA/trend=131011/131146.911242651≈0.998964`), i.e. `s11 = s12*s14*s13`
  (trend × cycle-factor × irregular-factor = SA) when a real cycle exists.
  NOT wired this session (`s14` isn't in `test_seats_tables.py`'s `_TAGS`
  list, and it inherits the SAME `s12`/`s13` residual-precision blocker
  described below anyway -- no point adding it to the gate list yet).

This mapping is a real, load-bearing finding for every future session
working on `s10`-`s18` -- it replaces guessing with a checkable identity
(`s11==s12*s14*s13` or `s11==s12+s13`, `s10==s16==s18==z/s11`) that can be
verified against ANY new golden bundle without needing to find the (still
unlocated) X-13-side table-naming code.

### The log back-transform: implemented and gated (payems_seats, the mandate's 2nd series)

`ctx.arima.lam` (Box-Cox power, already resolved to its final value by the
time SEATS runs) is `0.0` for a log-transform spec, `1.0` for none --
confirmed via a probe (`payems_seats`: `lam=0`; `unrate_seats`: `lam=1`).
Since `z` (`ctx.series.tsrs`) is the model's own log-linearized series for a
log spec, and log(z)=log(trend)+log(seasonal)+log(cycle)+log(irregular)
decomposes ADDITIVELY in log-space (exactly what ESTBUR's own `trend`/`sc`/
`cycle`/`ir` arrays already are, unconditionally), the back-transform is
simply: `exp()` each additive component for `lam==0`, forming `s13`
(irregular) as the RATIO `sa/(trend*cycle)` instead of the difference, and
`s10/s16/s18` as `exp(z)/exp(sa)`. Implemented in `estbur_historical()`
(now producing REPORTED, back-transformed arrays directly in
`EstburResult`, branching once on `is_log = fabs(ctx.arima.lam)<1e-9`) and
verified against `payems_seats`: `s11` worst error `7.8e-16` (bit-exact),
`s10`/`s16`/`s18` EXACT (0.0 worst, every one of 308 dates), `s18` worst
`1e-15`. **`s12`/`s13` do NOT gate** -- see next section, same root cause as
`unrate_seats`'s `s13`.

### payems_seats's s12/s13: the SAME residual-precision gap as unrate_seats's s13, but manifesting as a floor across most of the series instead of 2 isolated points

`payems_seats` has `Q=2` (`qstar_cf=2`, vs `unrate_seats`'s `1`) and a REAL
cycle (`ncycth=1`, already independently confirmed bit-exact at the
`decompspectrum`/`.mdc` level via the passing, non-xfailed `test_seats_mdc`
gate -- so `cd.thetc`/`cyc`/`trvar` are NOT the source of this gap). Dumping
`s12`'s per-date relative error: `2.15e-4` at the very first point,
decaying rapidly but PLATEAUING at a `~2e-6` floor across the entire middle
of the 308-point series (not decaying to noise like `unrate_seats` does),
rising again to `2.65e-5` at the very last point. This floor sits almost
exactly at session 9-10's own measured `armafl()`-vs-CALCFX residual gap
(`~1.5e-6` relative, from the `Dof`-corrected `varres` check) -- strong
circumstantial evidence this is the SAME approximate-residual limitation as
`unrate_seats`'s `s13` gap, just spread across more of the series because
`payems_seats`'s larger `lext` (driven by `qstar_cf=2` and a real cycle)
gives the approximation's error more room to propagate before the MA
filter's own geometric decay damps it out. **Not a new, separate bug** --
the same root cause, now confirmed to matter beyond `unrate_seats`'s 2
boundary points once a spec's `lext` is large enough.

### What's left (explicitly not done this session, in scope order)

- **Closing `s12`/`s13` for real** (both `unrate_seats`'s 2-point gap and
  `payems_seats`'s broader floor) needs the TRUE CALCFX residual, not an
  approximation. This session's from-scratch attempt hit a genuine
  contradiction (above) rather than landing a working port -- the fork to
  resolve first, before trying again: is `Type` really `0` at runtime
  (confirm via an ACTUAL oracle instrumentation run, `write(*,*) Type`
  right before the STEP4-9 block, rather than inferring from a
  never-assigned COMMON block), and does the STEP1B/3B `Thstar`/`Phist`
  rebuild (skipped in this port) change anything relative to `cd.thstar`
  for the `Init.eq.2` path specifically? A ground-truth oracle dump of
  CALCFX's own `a(1)`/`a(Na)` (temporarily instrumented, then reverted --
  same discipline as session 12's `sigex.f`/`ansub3.f` dump) would settle
  this decisively instead of more hand-derivation.
- **`s14`** (cycle factor): formula now known (`s11/(s12*s13)`, confirmed
  above), not wired into `_TAGS`/the gate since it inherits the same
  residual-precision blocker as `s12`/`s13` and isn't in the mandate's
  6-tag list.
- **`airline_seats`/`airline_fixed-airline-seats`/`expgs_fixed-airline-
  seats`/`unrate_fixed-airline-seats`/`payems_fixed-airline-seats`**: all
  have REAL seasonal structure (`npsi!=1`), exercising `cd.thets`/`psi`/
  `varwns` and `estbur.cpp`'s `fxs`/`bxs`/`sc` machinery for the first
  time against real numbers -- structurally wired (the general `MLTSOL`
  branch handles `s`/`c` symmetrically with `t` already) but genuinely
  untested. `expgs_seats` has no golden bundle at all (oracle rejects the
  model, unchanged since session 2).
- **`unrate_fixed-airline-seats`/`payems_fixed-airline-seats`**: additive
  and log-transform respectively, WITH real seasonal structure -- good
  next targets once `npsi!=1` is validated, since the `s10`/`s16`/`s18`
  formula (`z/sa`) is now known to hold in the real-seasonal case too
  (verified against `unrate_fixed-airline-seats` this session), just not
  yet reachable since `run_seats()` still fatals for every spec except
  `unrate_seats`/`payems_seats`.
- `p>0`/`bp>0` (a real AR term) remains entirely unsupported (`phist!=0` in
  `build_bphist`) -- none of the 8 corpus specs need it (all are `p=bp=0`),
  so this has not blocked anything yet.

### Actions taken this session

- Read `CALCFX` (`ansub1.f:964-1479`) in full; ported its STEP4-9 exact-ML
  residual recursion (`Pstar_cf==0` case) as `calcfx_last_residual()` +
  `diff_series()` in `core/src/seats/estbur.cpp`; independently verified
  the port's output in Python; found it disagrees with (and, wired in,
  regresses) the working `armafl`-based baseline; **reverted the file
  entirely back to its session-12 state** (rewritten from the exact text
  captured at this session's start, then rebuilt and reconfirmed the
  gate numbers match session 12's own before proceeding).
- Reverse-engineered the `s10`/`s13`/`s14`/`s16`/`s18` table formulas from
  golden-data arithmetic identities (see above) across 4 specs spanning
  additive/log x seasonal/no-seasonal.
- `core/src/seats/estbur.hpp`/`.cpp`: added `EstburResult::seasonal_factor`
  (`z/sa`) and the `ctx.arima.lam`-gated log back-transform (`exp()` of the
  additive/log-domain `trend`/`sc`/`cycle`/`sa`, ratio-form `ir`) to
  `estbur_historical()`.
- `tools/x13run_seats.cpp`: added `s10`/`s16`/`s18` dumps (all three from
  the one `seasonal_factor` array).
- `tests/parity/test_seats_tables.py`: un-xfailed `(unrate_seats, s18)`,
  `(payems_seats, s10/s11/s16/s18)`.
- Full verification: `ctest` 10/10 passing, `pytest tests/parity/` 481
  passed / 13 skipped / 77 xfailed / 0 failed (up from 473/13/82/0);
  `pytest tests/parity/test_seats_tables.py` alone 30 passed / 2 skipped /
  41 xfailed (up from 25/2/48). Confirmed `git status --porcelain oracle/`
  empty (no oracle files modified, despite reading several this session --
  no instrumentation was actually added to the oracle this time, only
  read).

### Precise next step

1. Ground-truth `CALCFX`'s `Type` value and its STEP1-9 residual for
   `unrate_seats` via an ACTUAL oracle instrumentation run (temporary
   `write(*,*)` at `ansub1.f`'s STEP4/STEP7/STEP9, reverted after, same
   discipline as session 12) rather than more hand-derivation -- this
   session's from-scratch port was independently verified CORRECT AS
   WRITTEN but still disagrees with the working baseline, so the bug (if
   any) is in an assumption about CALCFX's runtime state, not the
   transcription; a real dump settles which.
2. Once the true residual is confirmed, re-test it against BOTH
   `unrate_seats`'s 2-point `s13` gap AND `payems_seats`'s broader `s12`/
   `s13` floor -- if it closes one it should close both, since this
   session established they're the same root cause.
3. Extend to a real-seasonal spec (`unrate_fixed-airline-seats` is the
   simplest -- additive, so no back-transform complexity on top) to
   validate `cd.thets`/`psi`/`estbur.cpp`'s `fxs`/`bxs`/`sc` for the first
   time against real numbers; the `s10`/`s16`/`s18` formula is already
   confirmed to hold in this case.

---

## LATEST STATUS (session 12): unrate_seats's s11/s12 gate bit-exact; s13 within 2.78e-8 (RTOL 1e-8) at 2 boundary dates only -- first real SEATS s-tables landed

**Mandate**: ground-truth session 11's `qstar==1` contradiction by instrumenting
and building the ORACLE Fortran directly (not more hand-derivation), then port
whatever the oracle's own execution shows, gate `unrate_seats`'s s10-s13,
smoke-test `s12[196101]==6.66992016163405` first.

**Result: smoke test passes (to ~3e-10). `s11`/`s12` now gate bit-exact
(RTOL=1e-8) for `unrate_seats`; `s13` (irregular) is 2.78e-8 at exactly the 2
boundary dates (196101, 202508) -- 773/776 dates are within 1e-8, decaying to
double-precision noise within 2-3 points of each boundary -- so `s13` is
documented but NOT un-xfailed this session (see "what's left" below). Full
parity suite: 473 passed / 13 skipped / 82 xfailed / 0 failed. Unit suite:
10/10 passing.**

### Oracle instrumentation: session 11's `qstar==1` contradiction was a real bug in this project's own understanding, not the oracle's

Built the vendored oracle from source with gfortran (rtools44, driven from
PowerShell per the standing toolchain note -- msys `make` was avoided
entirely; drove `gfortran -c` directly over all 690 `makefile.gf` `SRCS`
files in a PowerShell loop, then linked with `gfortran -static`). **Sanity
check first**: the freshly-built binary reproduced golden `unrate_seats.s12`
and `.mdc` byte-for-byte (only the `.udg` timestamp differed) -- confirms
the build is trustworthy before instrumenting anything.

Added temporary `write(*,*)` diagnostic lines at `sigex.f`'s `ESTBUR` call
site (~line 1395, dumping `pstar`/`qstar`/`Nz`/`mq`/`d`/`bd`/`totden`/
`thstr0`/`ct`/`cs`/`cc`/`z`/`bz`) and inside `ansub3.f`'s `ESTBUR` body (the
`maxpq`/branch decision, the general-branch `irow`, and `trend`/`sc`/
`cycle` after assembly), rebuilt, ran on `unrate_seats`, then **reverted
both oracle edits with `git checkout` and confirmed `git status` on
`oracle/` is clean** before any further work (mandatory, honored).

The dump: `pstar=2 qstar=2 Nz=776`, **general branch** (`irow=2`), `Totden=
[1,-1,0]`, `Thstr0=[1,0.0346452...,0]`, `ct=[0.535245394617457,
0.267622697308728]`, and -- decisively -- `DBG_TREND` matched golden
`s12[196101]` to the last printed digit. This directly confirmed the
coordinator's (Codex-derived) prediction and refuted session 11's `pstar=
qstar=1`/trivial-branch conclusion.

**Root cause of session 11's error, now fully understood**: `cd.pstar`/
`cd.qstar` (`canonical_denoms.cpp`, via `conv()`'s own `lplus1` return) were
ALREADY 2 all along -- session 10-11 misremembered them as 1 without
re-checking, then chased a nonexistent "B-J switch" to explain a gap that
didn't exist. Verified directly with a probe (`cd.pstar=2 cd.qstar=2`,
independent of the oracle dump) -- **no switch is needed at all**; ESTBUR
consumes `cd.pstar`/`cd.qstar`/`cd.thstar`/`Totden=conv(cd.psi,cd.chcyc)`
DIRECTLY. `decompspectrum.hpp`'s header comment corrected accordingly (the
switch blocks at `sigex.f:924-974`/`1320-1380` are real, but irrelevant to
ESTBUR's inputs -- whatever they do to SIGEX's own local copies nets out to
the original values in every case checked, and this project never carries
those particular locals across the switch anyway).

### `ct`/`cs`/`cc`: ported for real this session (`decompspectrum.cpp`)

`spectrum.f:1529-1533/1570-1573/1625-1628` (`ct(1)=us(1)`, `ct(j)=0.5*us(j)`
for `j=2..nus`, via two chained `MULTFN` calls through the OTHER two
components' `F`-harmonic polynomials) is now computed in `decomp_spectrum()`,
reusing the already-computed `utf`/`vf`/`ucf` locals and the public
`SpectruHarmonics::fc/fs/ft` fields. New `SeatsComponentModels::ct/cs/cc`
(+`nct`/`ncs`/`ncc`) fields, sized 80 (not the oracle's 32 -- see the
buffer-overflow note below). Verified bit-for-bit against the oracle dump
(`ct=[0.535245394617457, 0.267622697308728]`).

### ESTBUR historical-span port (`core/src/seats/estbur.{hpp,cpp}`, new)

Ported `ansub3.f`'s general `MLTSOL` branch (`qstar!=1` case) for the
historical span only (`i=1..Nz`) -- confirmed by inspection that the
forecast-region blocks (`ansub3.f:356-678`) never write `trend`/`sc`/
`cycle(i<=Nz)`. Pipeline: System A (`gt`/`gs`/`gc`, the shared 1-sided
filter, `ansub3.f:151-174`) -> a small forward/backward extension (FCAST-
style, `ansub1.f:2183-2201`, `lext=qstar+maxpq-2` points -- 2 for
`unrate_seats`) seeded by `armafl()`-derived residuals -> filter application
(`ansub3.f:196-225`) -> the general-branch boundary `MLTSOL` solve
(`ansub3.f:238-305`) -> the backward recurrence (`ansub3.f:282-312`) ->
`trend(i)=fxt(i)+bxt(Nz-i+1)` etc. `bz` is a plain reversal of `z` (session
10, re-confirmed). `sa(i)=z(i)`, `sc(i)=0`, `ir(i)=z(i)-trend(i)-cycle(i)`
for the `npsi==1` (no seasonal) case (`ansub3.f:508-523`).

### The armafl-residual sign: BOTH directions need negation (not just backward)

`armafl()`'s last residual (`fcnar()`'s own recipe, `armafl(linit=true)` +
`exp(lndtcv/2/dnefob)` scaling) needs to be **negated** before feeding
FCAST's forward recursion, for BOTH the forward (`z`) and backward (`bz`)
directions. Found empirically via the smoke test:

- Un-negated: `s12[196101]` off by a 7.5e-4 relative gap (`6.6749...` vs
  golden `6.6699...`).
- Negating only the backward seed: closes `s12[196101]` to ~2e-9, but
  `s12[202508]` (the OTHER boundary, where `s13`'s error is dominated by
  `fxt(Nz)` instead of `bxt(Nz)`) still misses by enough to blow up `s13`'s
  relative error there (near-zero irregular value amplifies a small
  absolute trend error).
- Negating BOTH seeds: closes both boundaries to double-precision noise.

This applying uniformly to both directions (not a reversal-specific
artifact) suggests a general sign-convention difference between `armafl()`'s
and CALCFX's "last residual" definitions -- NOT re-derived from the Fortran
text this session (budget), documented as empirically-determined in
`estbur.cpp`'s own comment, matching this project's established practice
for this kind of gap (c.f. the session-5 `thstar` sign finding).

### Two real crash bugs found and fixed while wiring this into `run_seats.cpp` for ALL specs

Wiring `run_seats()` to attempt the full chain for every spec (needed so
`OUTCOME: OK` reflects reality) surfaced two genuine, unrelated bugs when
exercised against specs with larger seasonal polynomials
(`airline_fixed-airline-seats`, `pstar=qstar=14`) that don't manifest for
`unrate_seats`'s small (`pstar=qstar=2`) case:

1. `decompspectrum.cpp`'s new `ct`/`cs`/`cc` `MULTFN` chain can produce more
   than 32 coefficients for larger models -- widened `SeatsComponentModels::
   ct/cs/cc` from 32 to 80 (matching the local scratch-buffer size) and
   added an explicit size guard (skip rather than overflow) as a second
   layer of defense.
2. `estbur.cpp`'s `fxt`/`bxt`/etc buffers were sized `n196+2` (based only on
   the filter-application loop's own needs), but the general branch's
   OUTPUT-STORE loop actually needs indices up to `n+irow = Nz+2*qstar-2`,
   which exceeds `n196+2` whenever `qstar>3` -- a real heap-corruption crash
   (`STATUS_HEAP_CORRUPTION`, confirmed via checkpoint `fprintf` tracing
   bisection) on `airline_fixed-airline-seats` before this was found and
   fixed (buffers widened to `Nz+2*maxpq+4`).

Both fixes are generic (not `unrate_seats`-specific) and were verified via
the full `pytest tests/parity/` suite (473 passed / 0 failed after the fix,
vs 18 failures including a hard crash before it).

### What's left (explicitly not done this session, in scope order)

- **`s13` at the 2 boundary dates** (2.78e-8, RTOL 1e-8): would need either
  (a) a more precise armafl-vs-CALCFX residual match at the exact boundary
  (likely requires the real `CALCFX`, previously declined as a ~520-line
  port), or (b) discovering the exact reason `armafl()`'s residual needs
  negating (which might also explain the remaining tiny gap) via a closer
  read of `analts.f:2810-2830`'s `Jfac=1` branch. Given the tiny magnitude
  and localization, this is a reasonable place to stop for this pass.
- **`s10`**: doesn't ship for `unrate_seats` (`npsi==1`) -- nothing to gate.
  Untested for the `npsi!=1` case (needs a real seasonal spec).
- **`s16`/`s18`**: not wired at all this session.
- **Generalizing to airline/payems/expgs (real seasonal structure) and any
  log-transformed spec**: explicitly out of scope this pass (per the
  coordinator's instruction). Concretely still needed for those:
  - The log back-transform (SEATS decomposes the LINEARIZED/transformed
    series; `sa`/`trend`/etc need `exp()`-ing back for log specs -- not
    touched anywhere in this port yet).
  - `cs`/`cc` (seasonal/cycle filter numerators) are now computed
    (`decompspectrum.cpp`) but UNVALIDATED -- no oracle ground truth was
    captured for a real-seasonal `ct`/`cs`/`cc` triple this session, only
    confirmed not to crash (the buffer-overflow fix). `estbur.cpp`'s
    `fxs`/`bxs`/`fxc`/`bxc`/`sc`/`cycle` machinery is implemented but
    likewise unvalidated for `npsi!=1`/real-cycle specs.
  - `FCAST`'s `bphist` construction only supports `p=bp=0` (`phist=0`) --
    `unrate_seats`'s exact case; any spec with `p>0`/`bp>0` needs that
    extended.
  - `imean!=0` (mean term) is not supported (`za=0` hardcoded).

### Actions taken this session

- Built the oracle from source (gfortran, rtools44, PowerShell), sanity
  checked, instrumented `sigex.f`+`ansub3.f`, dumped, **reverted both oracle
  files with `git checkout` and confirmed `git status` on `oracle/` is
  clean**. No oracle files remain modified.
- `core/src/seats/decompspectrum.hpp`/`.cpp`: added `ct`/`cs`/`cc` fields +
  computation; corrected the header's B-J-switch narrative.
- `core/src/seats/estbur.hpp`/`.cpp` (new): the historical-span ESTBUR port
  described above.
- `core/src/driver/run_seats.cpp`: wired the full decode -> canonical-denoms
  -> SPECTRU -> DecompSpectrum -> ESTBUR chain; returns `true` (OK) when
  `estbur_historical()` succeeds, falls back to the existing
  `seats_not_ported()` fatal otherwise.
- `tools/x13run_seats.cpp`: re-runs the same chain to dump `s11`/`s12`/`s13`
  in the `<tag> YYYYMM <value>` format `test_seats_tables.py` expects
  (duplicated rather than sharing state via `ctx`, since `x13context.hpp` is
  out of scope for this pass -- owned by the concurrent span agent).
- `tests/parity/test_seats_tables.py`: un-xfailed `(unrate_seats, s11)` and
  `(unrate_seats, s12)`; documented (but left xfailed) `(unrate_seats,
  s13)`.
- Full verification: `ctest` 10/10 suites passing, `pytest tests/parity/`
  473 passed / 13 skipped / 82 xfailed / 0 failed.

### Precise next step

1. If closing `s13`'s last 2.78e-8 is worth the investment: read
   `analts.f:2810-2830`'s `Jfac=1` branch closely to understand exactly what
   CALCFX does differently for backward/boundary residuals that `armafl()`'s
   substitution doesn't replicate -- may also explain WHY negation is
   needed at all, which is currently empirical/undocumented-in-theory.
2. To generalize to `payems`/`expgs`/`airline_seats` (additive, real
   seasonal structure): validate `cs`/`cc` against an oracle dump the same
   way `ct` was validated this session (extend the same instrumentation
   pattern to a seasonal spec), then validate `estbur.cpp`'s `fxs`/`bxs`/
   `fxc`/`bxc`/`sc`/`cycle` machinery -- structurally already wired via the
   generic `MLTSOL` branch, just unverified against real numbers.
3. To generalize to the `*-fixed-airline-seats` variants and any
   log-transformed spec: add the log back-transform (SEATS's own `sa`/
   `trend`/`sc`/`ir` all need `exp()`-ing back for `ILam==0`/log specs --
   not investigated this session at all).

---

## LATEST STATUS (session 11): smoke test NOT run -- found a genuine mathematical contradiction in ESTBUR's `qstar==1` trivial branch that hand-derivation could not resolve; HARD STOP, no table gated

**Mandate**: land the payoff pass -- port the B-J switch (session 10's
checklist item 1), pin down "Totden" (item 2), do ESTBUR's historical
`MLTSOL` solve for `trend(i)` (item 3), smoke-test `s12[196101]==
6.66992016163405` BEFORE running the full series (item 4), then gate
`s10/s11/s12/s13` un-xfailed with worst-error per table if it lands.

**Result: no code was written or run this session. Careful hand-derivation
(deliberately done on paper before touching the build, so a wrong
dimensional assumption wouldn't burn a cycle producing a bogus number) hit a
genuine, unresolved mathematical contradiction: the `qstar==1` trivial
branch that BOTH session 10 and this session's corrected re-derivation land
on provably forces `trend(i)` to be a constant multiple of `z(i)` for
`unrate_seats` -- which cannot match golden `s12` (a non-constant-ratio
series). Per the HARD STOP instruction, stopping here rather than guessing
further or writing code against an unresolved contradiction.**

### Two switch blocks, not one -- and they interact in a way that resolves to a no-op for `Totden`/`Thstr0`, then hits a wall

Session 10 found ONE switch block (`sigex.f:1320-1380`, "SWITCH FROM B-J TO
POLYNOMIAL NOTATION") and assumed it was the only relevant one. Reading
further back this session found a SECOND, EARLIER block: `sigex.f:924-974`,
"SWITCH ALL THE ARRAYS NEEDED FOR ROUTINE DECFB INTO B-J NOTATION" -- the
literal inverse operation (shrinks `Thetp/Chi/.../Totden/Thstr0` by 1 via a
simple `X(i)=-X(i+1)` shift, decrementing `Nchi`/`pstar`/`Qstar0`/etc), which
runs BEFORE the DECFB calls (`sigex.f:1005-1128`) and BEFORE the
line-1320 block.

Working through the round trip for `unrate_seats` (`cd.pstar=1`,
`cd.qstar=1`) surfaces a subtlety that invalidates this session's earlier
(now superseded) "the switch grows arrays to 2x2" framing:

- Block 1 (`sigex.f:941-946`): `do i=1,pstar-1(=0): ...` and
  `do i=1,Qstar0-1(=0): ...` -- **both are ZERO-TRIP loops** (`pstar-1=0`
  when `pstar=1`), so `Totden`/`Thstr0`'s CONTENT is untouched by block 1.
  But the SCALARS still decrement: `pstar=pstar-1=0`, `Qstar0=Qstar0-1=0`
  (`sigex.f:973-974`) -- unlike the array content, the scalars move
  regardless of the loop trip count.
- Block 2 (`sigex.f:1334-1339`) then runs using the CURRENT (post-block-1)
  scalars as ITS loop bound: `do i=1,pstar(=0): ...`,
  `do i=1,Qstar0(=0): ...` -- **also zero-trip**, since `pstar`/`Qstar0`
  are 0 at this point (this session's earlier read used the WRONG,
  pre-block-1 value of `pstar=1`/`Qstar0=1` for block 2's bound, which is
  the arithmetic error being corrected here). Then `Totden(1)=1.0`/
  `Thstr0(1)=1.0` get force-set (no-ops, already 1), and
  `pstar=pstar+1=1`, `Qstar0=Qstar0+1=1` -- back to the ORIGINAL values.

**Net result: for `unrate_seats`, `Totden` and `Thstr0` are completely
UNCHANGED by the two-block round trip -- `Totden=[1,-1]`,
`Thstr0=cd.thstar=[1,0.0346452...]`, `pstar=1`, `qstar=1`, exactly their
pre-switch values.** (Verified the SAME round-trip-cancellation
algebraically for `Thetp`/`Chi` too, where `pstar-1`/`Qstar0-1`-style
bounds don't apply -- those genuinely DO cancel via a length-1 shift/negate
followed by its own inverse, confirmed by hand for both `Thetp` and
`Chcyc`'s slightly different-looking loop forms.)

### The contradiction: `maxpq=1` (unchanged dimensions) forces a constant-ratio trend, which cannot be right

With `pstar=1`, `qstar=1` restored, `maxpq=MAX(pstar,qstar)=1`, putting
`unrate_seats` back in ESTBUR's `if(qstar.eq.1)` TRIVIAL branch
(`ansub3.f:226-237`: `fxt=fyt`, `bxt=byt` directly, no `MLTSOL` general-branch
solve). Working through what this branch produces, algebraically, for
ANY CT/thstar values (not specific numbers -- a general proof):

```
trend(i) = fxt(i) + bxt(Nz-i+1)
         = fyt(i) + byt(Nz-i+1)                      [qstar==1: fxt=fyt,bxt=byt]
         = gt(1)*extZ(i) + gt(1)*bz(Nz-i+1)           [maxpq==1: single-tap filter]
         = gt(1)*z(i) + gt(1)*z(i)                    [bz(Nz-i+1)=z(i), the plain-
                                                        reversal identity, confirmed
                                                        session 10]
         = 2*gt(1)*z(i)
```

**This says `trend(i)` is ALWAYS a constant multiple of `z(i)` whenever
`maxpq==1`, regardless of what `gt(1)` (hence CT/thstar) actually equal.**
Golden `s12`/`s11` are NOT a constant ratio (`s12[196101]/s11[196101] =
6.66992.../6.6 = 1.01059`, `s12[196102]/s11[196102] = 6.82688.../6.9 =
0.98941` -- different ratios at consecutive periods). This is a hard
contradiction: `maxpq` genuinely cannot be 1 for a correct trend
computation, yet the (now twice-checked) switch round-trip says
`pstar=qstar=1` unchanged. One of the following must be true, and this
session ran out of budget to determine which:

1. `bz(Nz-i+1)=z(i)` (session 10's finding, re-used here) is wrong, or
   applies to a DIFFERENT `Nz` than the one ESTBUR's `trend(i)` formula
   uses (`Nz` is reassigned multiple times in `analts.f` between `bz`'s
   construction (~line 2807) and the eventual `sigex.f`/`ESTBUR` call
   chain -- NOT re-verified this session that it's the same value at both
   points).
2. `unrate_seats`'s `pstar`/`qstar` as ESTBUR actually receives them are
   NOT simply `cd.pstar`/`cd.qstar` -- some OTHER mutation (not yet found)
   changes them between `canonical_denoms()`'s computation and the
   `ESTBUR` call, independent of the two switch blocks examined here.
3. The `qstar==1` trivial branch is not actually meant to be reached for
   models like this one -- i.e. something upstream (not yet identified)
   ensures `qstar` (as ESTBUR receives it) is never exactly 1 for a model
   with real MA structure, and `cd.qstar=1` specifically undercounts it
   somehow (e.g. if ESTBUR's "qstar" is meant to include `Pstar` too, or a
   different degree convention than `canonical_denoms.cpp`'s `out.qstar`
   uses).

None of these were resolved this session -- flagging precisely because
they're the fork the next session needs to resolve FIRST, before any
`MLTSOL` solve is attempted again.

### CT/CS/CC: a real, previously-mis-scoped gap in `decompspectrum.hpp`, now corrected (still valid, unaffected by the above)

Traced where SIGEX's `ct`/`cs`/`cc` (ESTBUR's trend/seasonal/cycle numerator
inputs) actually come from: `sigex.f:709-721` passes them as OUTPUT params
into `SPECTRUM()` (`spectrum.f:82-100`), the SAME outer wrapper this project
already ported (SPECTRU itself at spectrum.f:558, DecompSpectrum/MAspectrum
at 1380-2513/2710-2890, this project's own `decomp_spectrum()`). Inside
`SPECTRUM`, CT is built at `spectrum.f:1529-1533`: `vn=MULTFN(utf,Fc);
us=MULTFN(vn,Fs); ct(1)=us(1); ct(j)=0.5*us(j)` for `j=2..nus` -- using
`utf` (ALREADY computed by this project's `decomp_spectrum()`, just not
exposed past the function's local scope) and `Fc`/`Fs` (`sr.h.fc`/`sr.h.fs`,
already public fields). CS/CC follow the identical pattern from `vf`/`ucf`.

**This corrects a mistaken assessment left in `decompspectrum.hpp`'s own
header comment** (from an earlier session): it claimed the CT/CS/CC lines
were "purely for PLOTFILTERS-style plotting, all commented out in the
vendored source anyway" -- true only of the `PLOTFILTERS()` calls
immediately surrounding them, NOT of the `ct(1)=us(1)`/etc assignment lines
themselves, which are live and DO feed ESTBUR. Fixed the comment in place
(`core/src/seats/decompspectrum.hpp`) to point the next session at the
right lines; did not change `decompspectrum.cpp`'s actual logic (out of
caution -- it's a currently-gated, passing file, and this session ran out
of budget before writing tested code for the addition, especially with the
`qstar==1` contradiction above still unresolved -- no point wiring CT/CS/CC
into a solve whose dimensions are in question).

Confirmed **CT/CS/CC do NOT go through either B-J switch block** -- neither
touches `ct`/`cs`/`cc` at all, only
`Thetp/Chi/Thets/Psi/Cyc/Thetc/Totden/Thstr0/Chcyc/Thadj` (+ `s`-variants).

### What's still confirmed and reusable

- `bz`'s historical range is a plain reversal of `z` (session 10) -- BUT see
  contradiction-hypothesis (1) above; the precise `Nz` it uses relative to
  ESTBUR's own needs `Nz` is now flagged as unverified, not just assumed
  fine.
- For `d=1`/`bd=0`, reversing then differencing naturally reproduces
  CALCFX's `kd=(-1)^(d+bd)` sign flip -- `bz`'s residuals should be
  obtainable via `armafl()` directly on `bz` (plain reversal of `tsrs`), no
  separate sign-flip step. Still believed correct; not run.
- `FCAST`'s own `Pstar`/`Qstar` (`P+Bp*Mq`/`Q+Bq*Mq`) are a THIRD, separate
  convention from `Totden`/`Thstr0`'s -- for `unrate_seats`, FCAST's own
  Qstar=1, so only one seed residual per direction is needed IF/WHEN a
  forecast/backcast extension turns out to be necessary (now uncertain
  again pending the `maxpq` question above).
- `ESTBUR`'s "`n`" gets REASSIGNED partway through the general branch
  (`ansub3.f:196` vs `ansub3.f:251`) -- a likely off-by-one trap for
  whoever implements the general branch, documented for reuse.

### Actions taken this session

- Fixed a misleading comment in `core/src/seats/decompspectrum.hpp` (CT/CS/
  CC are live, ESTBUR-consumed code, not dead plotting-only code) --
  doc-only, no logic change. Rebuilt and reran the full gate suite to
  confirm: `ctest -R test_seats` 1/1 passing, `pytest tests/parity/
  test_seats_tables.py` 23 passed / 2 skipped / 48 xfailed (unchanged).
- No other files touched, no probe code added to `tools/x13run_seats.cpp`
  -- the smoke test was never reached; the blocker is a dimensional/
  logical contradiction found by hand-derivation, not a numeric mismatch
  from running code.

### Precise next step

1. Resolve the `maxpq==1` contradiction FIRST (the fork of 3 hypotheses
   above) before writing any ESTBUR code. The cheapest check: re-read
   `analts.f` from the `bz` construction (~line 2787) through to the
   `sigex.f`/`ESTBUR` call chain, tracking every reassignment of `Nz` and
   `pstar`/`Qstar0`-equivalents along the way, to see whether ESTBUR's
   actual runtime `pstar`/`qstar` genuinely equal `cd.pstar`/`cd.qstar` or
   something else. Alternatively, search for another SEATS oracle test
   fixture/log with `OUT=0` (verbose) that might print `pstar`/`qstar`/
   `irow`/`maxpq` directly, sidestepping the hand-derivation entirely.
2. Once `maxpq` is pinned down correctly (and non-degenerately), re-run the
   Toeplitz-matrix derivation with the CT/CS/CC formula from this session
   (now correctly identified) and the right `Totden`/`Thstr0` values.
3. Only then smoke-test `s12[196101]==6.66992016163405` before writing the
   full-series extraction, per the standing instruction.

---

## LATEST STATUS (session 10): Detpri closes the residual gap (1.5e-6) -- but ESTBUR needs an unported "B-J switch" step before any s-table can be attempted; not yet gated

**Mandate**: close session 9's ~4e-5-ish residual gap by extracting CALCFX's
`Detpri` normalization specifically (not the full evaluator), re-check against
golden `varres`/`varsd`; if closed, wire armafl-residuals + `bz` (reversed
series) -> `FCAST` -> `ESTBUR`'s historical matrix-solve core and gate
`unrate_seats`'s s10-s18 tables, reporting worst-error per table.

**Result: Detpri closes the gap -- to 1.5e-6 relative, effectively at this
project's own tolerance bar for estimation-derived quantities. ESTBUR was NOT
wired and no s-table is gated: reading ESTBUR's real call site (sigex.f:1395,
not read closely enough in session 7) surfaced a genuine, previously-unknown
prerequisite -- a "B-J sign switch" transform (sigex.f:1320-1380) that MUST
run on `Thetp/Chi/Thets/Psi/Cyc/Thetc/Totden/Thstr0/Chcyc/Thadj` before
ESTBUR's `ct/cs/cc` inputs are valid, and it is not ported anywhere in this
codebase yet.** Two numerically-grounded attempts to guess around it both
failed with dimensionally-obvious wrong answers, confirming (not just
suspecting) that the switch is required. This is a new, well-scoped, bounded
next step -- not another CALCFX-sized unknown.

### Part 1: Detpri traced and closed (armafl-reuse for residuals: HOLDS)

Read `CALCFX`'s `Detpri` computation in full (`ansub1.f:964-1471`, focusing on
STEP 4-9, lines 1234-1471) rather than guessing at a scale factor. Two
findings, both load-bearing:

1. **`Detpri` is provably a no-op on the final residuals/SSR for the `Init==2`
   path.** CALCFX computes `Detpri=detbnp` (STEP 9, `ansub1.f:1467`) and
   scales its OWN output by it (`a(i)=Detpri*a(i)`, `f=sum(a(i)^2)`,
   `ansub1.f:1468-1470`) -- but the CALLER (`analts.f:2159`, `s=s/Detpri**2`,
   and `analts.f:2385-2386`, `a(i)=a(i)/Detpri`) immediately divides it back
   out. Net effect on both the returned SSR and the returned residual array:
   zero. So "extract Detpri and apply it as a scale factor" (session 9's
   framing, and this session's original mandate) was testing a quantity that
   provably cancels in the interface CALCFX actually exposes to `analts.f`
   for the `Init==2` (fixed-model, no re-estimation) path -- the ONLY path
   relevant here. Session 9's `fac=exp(lndtcv/2/dnefob)` scaling (X-13's own,
   already-applied determinant normalization from `fcnar()`) was already
   doing the right kind of correction; there was no separate "Detpri factor"
   left to bolt on.
2. **The REAL culprit was the comparison target, not the residuals.**
   Traced golden `varres`/`varsd` (`.udg`) all the way through: `seatdg.f:
   399-401` (`varres=Sdres**2`) <- `USRENTRY IFUNC=1047` (`ansub9.f:285-286`,
   `Sdres=RBUFF(...)`) <- the ONLY caller passing that code, `htmlout.f:
   1277-1278` (`dvec(1)=Sqf`) <- `Sqf=SQRT(f)`, `f=s/Dof`
   (`analts.f:2159-2161`), where **`Dof=Nw-Pstar-nx-Imean`**
   (`analts.f:2141`/`2276`), NOT `Na=Nw-Pstar+Qstar` (`analts.f:2140`) that
   session 9 used via `DVAR(Na,a)`. For `unrate_seats`: `Nw=775`, `Pstar=0`,
   `nx=1` (one free MA coefficient), `Imean=0` -> `Dof=774` vs `Na=776`.
   `f` is ALSO a raw sum-of-squares with no mean subtraction, unlike `DVAR`.

Recomputed armafl-derived residuals (`fcnar()`'s own recipe: `armafl()` +
`fac=exp(lndtcv/2/dnefob)` scaling, exactly as session 9 did) against the
CORRECT statistic (`sum(a^2)/Dof`, `Dof=774`, no mean subtraction) instead of
`DVAR(776,a)`:

```
fArma = sum(a_i^2) / 774 = 0.186027346268383
golden varres                = 0.1860270580
relative gap                 = 1.5496e-06
```

**This is the decisive number: 1.5e-6 relative, essentially at this
project's own stated tolerance policy for estimation-derived quantities
(~1e-6).** Session 9's ~0.13%-0.26% gap was ~99.4% pure Dof/Na bookkeeping
error, not a residuals problem; correcting the bookkeeping alone closes
nearly all of it, and no separate "Detpri factor" was needed on top --
confirming (2) subsumes and supersedes the original "extract Detpri" framing
from this session's mandate.

A from-scratch C++ port of CALCFX's own STEP 4-9 (the exact Qstar=1/Pstar=0
presample-correction algorithm: STEP 4 init, STEP 5 conditional + sensitivity
recursion, STEP 6 `det=b(1,1)`, STEP 7 corrected presample residual, STEP 8
final recursion) was also attempted as an independent, second cross-check.
It has a real (not yet diagnosed) indexing/normalization bug -- its
element-wise residuals match armafl's on some positions (e.g. position 2,
4: agree to ~1e-7) and diverge on others (position 1, 3: same magnitude,
flipped sign; positions 5+: diverge outright) -- so it should NOT be trusted
as a second confirmation on its own. It was diagnostic-only scratch code and
has been removed rather than landed half-working; the armafl-vs-golden-varres
match above is the evidence actually being relied on.

### Part 2: the real ESTBUR blocker -- an unported B-J sign switch, discovered by testing the CT/CS/CC input hypothesis and getting dimensionally-wrong answers

Re-read ESTBUR's ACTUAL call site (`sigex.f:1395`, previously only read as
far as `sigex.f:1005-1128`'s DECFB call in session 7 -- that DECFB call
computes the SEPARATE filter-weight save tables `psi/pss/psc/...`, NOT
ESTBUR's inputs; conflating the two was session 7's gap). Two concrete,
numerically-tested findings:

1. **`ESTBUR` is called at `sigex.f:1395`, immediately AFTER a "SWITCH FROM
   B-J TO POLYNOMIAL NOTATION" block (`sigex.f:1320-1380`, ~60 lines) that
   negates+reverses+shifts `Thetp/Chi/Thets/Psi/Cyc/Thetc/Totden/Thstr0` (and
   the `Chcyc`/`Thadj` pair via a slightly different loop, `sigex.f:1349-
   1356`) and grows each array's length by 1** (e.g. `Thetp(Nchi+2-i) =
   -Thetp(Nchi+1-i)` for `i=1..Nchi`, then `Thetp(1)=1.0d0` forced,
   `Nthetp=Nthetp+1`). `decompspectrum.hpp`'s own header comment already
   flagged this switch as unported and necessary for `DECFB` ("a SEPARATE
   'switch into Box-Jenkins notation' step... DECFB... DOES need that B-J
   switch applied on top; not implemented here") -- this session confirms
   it is ALSO a hard prerequisite for `ESTBUR`, which was not previously
   known (session 7's ESTBUR read didn't trace back to its real call site
   closely enough to see this).
2. **Confirmed by direct experiment, not just by reading**: tried feeding
   ESTBUR's 1x1 (`maxpq=max(Pstar,Qstar)=1` for `unrate_seats`) Toeplitz
   system two different PRE-switch candidates for `CT` (the trend numerator):
   - `CT=comp.thadj` (`[1, 0.0346...]`, matching `cd.thstar` numerically
     since `nchi=2`, `Nchcyc=2` turned out non-degenerate but numerically
     close): the 1x1 system's self-consistency makes `gt(1)=0.5` exactly,
     which forces `trend(i) = 0.5*z(i) + 0.5*bz(Nz-i+1)`. Traced `bz`'s
     historical-range construction precisely this session too (`analts.f:
     2806-2808`: `bz(Nz-i+1)=z(i)`, a PLAIN reversal, no sign flip, no
     residuals -- the residuals/CALCFX-on-reversed-series machinery from
     sessions 8-9 is ONLY needed for `bz`'s FORECAST-region extension
     beyond `Nz`, which for `unrate_seats`'s `maxpq=1` filter turns out to
     never be touched by the historical trend/sa/ir formulas at all).
     Substituting the reversal identity gives `trend(i)=z(i)` EXACTLY --
     contradicts golden `s12` (`6.66992...` vs `s11`/`z` `6.6` at
     `196101`). Wrong.
   - `CT=comp.thetp` directly (`[1,1]`, confirmed via a probe dump:
     `nthetp=2, nchi=2, thetp=[1,1], chi=[1,-1], varwnp=0.2676...` -- NOT
     the degenerate/trivial `nchi==1` case this session initially guessed
     from the golden `.mdc`'s absence of a `tcnum`/`tcden` key; that
     absence is a REPORTING gate unrelated to whether `thetp`/`chi` are
     computed non-trivially, which they are): gives `gt(1)=ct(1)/(2*
     thstar(1)) = 1.0/0.0693 ~= 14.4`, i.e. `trend(i) ~= 14.4*z(i) ~= 95`
     for `z(i)~6.6`. Off by an order of magnitude. Wrong.

   Both failures are consistent with, and only explained by, CT/CS/CC
   needing the POST-switch (B-J sign, reversed, length+1) values, not the
   PRE-switch `decompspectrum.cpp` output used directly.

### What's confirmed usable as-is for ESTBUR (no further work needed)

- `bz`'s historical range (`bz(1..Nz)`) is a plain reversal of `z`
  (`analts.f:2806-2808`), needing NO residuals/CALCFX/armafl at all --
  confirmed by direct derivation this session, not assumed.
- For `unrate_seats` specifically (`maxpq=1`), the filter-application loop
  (`ansub3.f:196-225`) only ever reads `extZ(i)`/`bz(i)` for `i` in the
  historical range `1..Nz` -- no forward/backward extension beyond `Nz` is
  needed for the s11/s12/s13 tables (only for forecast-region tables this
  project isn't targeting yet). This means `FCAST`'s own recursion (the
  piece needing residuals `a(na)` as a seed) is NOT actually on the critical
  path for `unrate_seats`'s historical s-tables at all -- a smaller
  dependency footprint than sessions 7-9 assumed.
- `npsi==1` (confirmed, no seasonal structure) forces `sc(i)=0` and
  `sa(i)=z(i)` identically (`ansub3.f:508-523`) -- confirmed by golden
  `s11` matching `unrate.dat`'s raw values exactly at `196101`
  (`6.60000000000000`). `ir(i)=z(i)-trend(i)-cycle(i)`, and `cycle(i)=0`
  is expected (golden `.mdc` has no `trnum`/`trden` -- `ncycth==0` and
  `ncyc==1`, matching this project's existing `spectru`/`decomp_spectrum`
  gate condition already in `x13run_seats.cpp`).
- So **the entire remaining unknown for `unrate_seats`'s s11/s12/s13 is:
  `trend(i) = fxt(i)+bxt(Nz-i+1)`, driven by the correctly-B-J-switched
  `ct`/`thstar`, via the 1x1 `MLTSOL` Toeplitz solve** (`ansub3.f:151-174`).
  Once the switch is ported this should be a small, mechanical finish.

### Precise next step

1. Port the "SWITCH FROM B-J TO POLYNOMIAL NOTATION" block (`sigex.f:1320-
   1380`, ~60 lines, negate+reverse+shift over ~10 arrays) as a small,
   self-contained function -- e.g. `seats_bj_switch()` in a new or existing
   `core/src/seats/*` file, taking `SeatsCanonicalDenoms`+`SeatsComponentModels`
   and mutating a switched copy (or in place, matching the oracle, since
   nothing downstream needs the pre-switch values again per this session's
   ESTBUR-only scope).
2. Apply the switch to `thstar`/`totden`(=`chi`? -- pin down exactly which
   canonical-denoms field plays "Totden" at the `sigex.f:1395` call site;
   not yet done this session) and `thetp`/`chi` (the `ct`/trend inputs).
3. Re-run the 1x1 Toeplitz `gt(1)` solve with the POST-switch `ct`/`thstar`
   values and check `trend(i)=0.5-branch-free` formula against golden `s12`
   at `196101` (`6.66992016163405`) as the first, single-number smoke test
   before attempting the full series.
4. If that lands, extend across the full historical range and gate
   `s11`/`s12`/`s13` (and `s18` if it turns out to need nothing further)
   against golden, worst-error per table, per the standing instruction.

### Actions taken this session

- Diagnostic/experimental probe code (armafl-vs-CALCFX-STEP4-9 comparison,
  `PROBE_COMP`/`PROBE_COMP2` dumps of `thetp`/`chi`/`thadj`/`chcyc`) was
  added to, then fully REMOVED from, `tools/x13run_seats.cpp` -- diagnostic
  only, findings captured here instead. The file is back to its clean,
  pre-session-10 shape.
- Rebuilt (`x13run_seats`, `test_seats`) and reran the full gate suite:
  `ctest -R test_seats` 1/1 passing; `pytest tests/parity/
  test_seats_tables.py` 23 passed / 2 skipped / 48 xfailed (unchanged from
  session 9 -- no regressions, no new xfails cleared yet, expected since no
  s-table code was landed).
- No files touched outside `tools/x13run_seats.cpp` (reverted) and this
  scope doc.

---

## LATEST STATUS (session 9): armafl-reuse hypothesis tested for unrate_seats -- DOES NOT HOLD to the required precision; CALCFX not ported

**Mandate**: a single high-leverage verification pass, NOT a port. Test
whether X-13's own already-bit-exact `armafl()` can stand in for `CALCFX`'s
forward (and reversed/`bz`) residual computation, per the "exact-ML residuals
for a fixed model are unique" argument from session 8. Decision gate: HOLDS
(match to ~1e-8) -> wire it and port `FCAST`+`ESTBUR`'s historical
matrix-solve core; DOES NOT HOLD -> stop, do not port `CALCFX`, report why.

**Verdict: does NOT hold to the required precision. No golden per-observation
residual table exists in the corpus to check at 1e-8 either way, and the best
achievable indirect check (matching a *summary statistic* derived from the
residuals) lands ~4e-5 relative, four orders of magnitude short of the bar.**
`CALCFX` was NOT ported this session, per the coordinator's stop condition.

### What was tried

Reproduced `fcnar()`'s own pattern (`core/src/regarima/estimate.cpp:162-192`,
the exact recipe X-13 itself uses to compute its own regARIMA residuals) as a
standalone probe in `tools/x13run_seats.cpp`, run against `unrate_seats`
(ARIMA(0,1,1), no seasonal, `nreg=0`, additive -- chosen deliberately as the
cleanest possible case): copy `ctx.series.tsrs` into a buffer, call
`x13::armafl(ctx, nspobs, 1, linit=true, lckrts=false, a, na, PA, info)`,
then (since `ctx.model.lextma` is true for this model) scale by
`fac = exp(ctx.mdldat.lndtcv / 2.0 / ctx.series.dnefob)` -- `fcnar`'s own
determinant-normalization step, structurally the same role as CALCFX's own
`a(i) = a(i) / Detpri` (`analts.f:2386`).

This produced `na=776` residuals (`nspobs=776`, `mxdflg=1`, `nnsedf=1` --
`armafl`'s internal `arflt` does NOT shrink the array on differencing, it
leaves it at `nr` length). Their population variance
(`DVAR`-style, `mean(a^2) - mean(a)^2`, `na=776`) is `0.1855398`, close to
but not matching X-13's own converged `ctx.mdldat.var = 0.1857870231`
(~0.13% off); skipping the first `mxdflg=1` element (`na2=775`) tightens
this to `0.1857789653` (~0.0043% off `ctx.mdldat.var` -- plausible edge
effect from the DIFF operator). Compared against golden `unrate_seats.udg`
`varres: 0.1860270580`, both attempts sit ~0.13%-0.26% off.

### The real explanation for the gap: golden `varres` is NOT `DVAR(Na,a)`

Traced the golden `varres`/`varsd` keys to their source rather than guessing
further. Chain: `seatdg.f:399-401` writes `varsd`/`varres` from X-13's own
`Sdres` (`varres = Sdres**2`); `Sdres` is set via `USRENTRY` `IFUNC=1047`
(`ansub9.f:285-286`); the ONLY caller passing `IFUNC=1047` with the relevant
payload is `htmlout.f:1277-1278`, `dvec(1)=Sqf`; and `Sqf=SQRT(f)` with
`f = s/Dof` (`analts.f:2159-2161`), where **`Dof = Nw - Pstar - nx - Imean`**
(`analts.f:2141`/`2276`) -- NOT `Na = Nw - Pstar + Qstar` (`analts.f:2140`).

For `unrate_seats`: `Nw=775` (776 less `d=1`), `Pstar=0`, `nx=1` (one free MA
coefficient), `Imean=0` (no mean estimated post-differencing) -> `Dof=774`,
vs `Na=776`. Also, critically, `s` (hence `f`) is a RAW sum-of-squares with
**no mean subtraction**, unlike `DVAR`'s `mean(a^2)-mean(a)^2`.

`776/774 = 1.002584` -- a **+0.2584%** predicted gap from the denominator
choice alone. The measured gap using the unskipped (`na=776`) armafl variance
was **+0.2627%** (`0.1860270580 / 0.1855398 - 1`). These match to within
~0.004 percentage points. **In other words: the ~0.26% gap that looked like
a residuals mismatch is almost entirely a bookkeeping artifact of comparing
`DVAR(Na,·)` against a differently-normalized statistic (`s/Dof`, no mean
correction) -- not evidence the residuals themselves are wrong.**

### Why this still falls short of "HOLDS" -- and why it stops here

Recomputing `f = SSR/Dof` properly from the armafl residuals (dropping the
mean-subtraction and using `Dof=774`) narrows the gap to genuinely
promising territory, but two things prevent calling this a pass:

1. **No per-observation golden residual table exists for `unrate_seats`**
   (checked the full golden bundle: `.a1/.b1/.dor/.dsa/.dtr/.mdc/.psi/.s11/
   .s12/.s13/.s18/.stl/.sum/.tbs/.tfd/.tse/.udg/.wkf` -- none of these are a
   raw residual series). The only available check is against SUMMARY
   statistics (`varres`/`varsd`/skewness/kurtosis/DW), which are themselves
   several transform-steps removed from the raw `a(i)` array (`Dof`, `Type`
   CLS-vs-exact branch at `analts.f:2392`, mean-correction re-run at
   `analts.f:2417-2420` if `rtval > ta`). A summary-statistic match, even a
   tight one, is not the element-wise ~1e-8 bar the decision gate requires.
2. Even at the summary-statistic level, the residual gap after correcting
   the `Dof`-vs-`Na` bookkeeping is **~4e-5 relative** (774-adjusted `f`
   vs golden `varres`, back-of-envelope), not ~1e-8. The most likely
   remaining cause: `fac = exp(lndtcv/2/dnefob)` (X-13's own determinant
   normalization) is only a PROXY for CALCFX's actual `Detpri` -- these are
   two independently-coded determinant-scaling constants from two different
   codebases, plausibly equal in exact arithmetic but not proven identical,
   and not verified numerically identical here.

Per the coordinator's decision gate, this is **DOES NOT HOLD** -- the
positive finding (residuals plausible, gap mostly explained, not a wild
mismatch) is real and worth keeping, but it is not proof at the required
precision, and there's no golden data available to close the last ~4e-5 gap
without either (a) porting `Detpri`'s exact formula out of `CALCFX` (a much
smaller, extractable piece -- worth scoping separately from the full
~520-line Kalman filter) or (b) accepting the summary-statistic-level match
as "good enough," which contradicts this project's bit-exact standard.

**Best hypothesis for the residual gap, ranked:**
1. Most likely: `Detpri` (CALCFX's own determinant normalization) differs
   numerically from `fac=exp(lndtcv/2/dnefob)` (X-13's own) -- same
   conceptual role, different exact formula/derivation between the two
   codebases' exact-ML implementations.
2. Possible: SEATS's `Type` (CLS vs exact) branch (`analts.f:2392-2410`)
   changes which/how many leading residuals are treated specially in ways
   `armafl()`'s single `linit=true` call doesn't replicate.
3. Ruled out: regression-parameter degrees-of-freedom effects (`nreg: 0` in
   golden `unrate_seats.udg` -- no regressors at all for this spec).
4. Ruled out: the DIFF-operator edge effect alone (`mxdflg=1` skip narrows
   but does not close the gap either against `ctx.mdldat.var` or golden
   `varres`).

### Actions taken this session

- Experimental probe code added to, then REMOVED from, `tools/
  x13run_seats.cpp` (it was diagnostic-only and the hypothesis test is now
  resolved negatively; no lasting value in leaving ad hoc printf probes in
  the harness). The file is back to its pre-session-9 shape (model-decode +
  canonical-denoms + SPECTRU + DecompSpectrum/.mdc probes only).
- Rebuilt (`x13run_seats`, `test_seats`) and reran the full gate suite:
  `ctest -R test_seats` still 1/1 passing; `pytest tests/parity/
  test_seats_tables.py` still 23 passed / 2 skipped / 48 xfailed (unchanged
  from session 8 -- no regressions, no new xfails cleared, as expected since
  no s-table code was added).
- No files touched outside `tools/x13run_seats.cpp` (reverted) and this
  scope doc.

### Precise next step

Per this session's negative result, `FCAST`/`ESTBUR` should NOT be ported
yet -- there is still no verified source of real residuals. Two honest paths
forward, for the coordinator/user to choose between:

1. **Scope a MUCH smaller CALCFX extraction**: rather than the full
   ~520-line Kalman filter, isolate and port just the pieces needed to
   produce `a(1..Na)` and `Detpri` for the `Init==2` (fixed-model, no
   re-estimation) path specifically -- `ansub1.f:964` through roughly
   `1120` was already read in session 8 and confirmed to assemble
   `Phist`/`Bphist`; the residual-filtering recursion and `Detpri`'s exact
   definition still need to be read in full (not yet done) to size this
   sub-scope precisely. This is very plausibly much smaller than the full
   routine since the `Init.lt.2` branches (parameter transform) can be
   skipped entirely.
2. **Accept armafl-reuse as an approximation** and proceed with
   `FCAST`+`ESTBUR` anyway, documenting the ~4e-5-level residual
   normalization uncertainty as a known, bounded gap rather than a bit-exact
   result -- a deliberate departure from this project's stated tolerance
   policy (estimation-derived quantities should gate at ~1e-6), so this
   would need explicit user sign-off before proceeding.

Given the coordinator's explicit instruction ("if it does NOT hold... report
exactly why it fails... so the coordinator can bring it to the user before
committing to a CALCFX session"), this session stops here without picking
between those two paths.

---

## LATEST STATUS (session 8): unknowns (a)/(b) traced to a common root -- CALCFX, ~520 unported lines, unavoidable for BOTH

**Read this section first.** Session 8's mandate was narrow and concrete:
resolve session 7's two open unknowns for `unrate_seats` ONLY (additive, no
log transform) -- (a) where `bz` gets built, (b) the regARIMA-residuals ->
`FCAST` `a(1..na)` mapping -- via reading + verification, then port `ESTBUR`'s
historical-only matrix-solve core and gate `unrate_seats`'s s-tables.

**Neither unknown could be safely resolved into working code this session,
and no s-table was ported or gated (0 of 48 xfails cleared) -- but both
unknowns WERE fully traced, and they converge on the SAME root cause: a
single unported ~520-line routine, `CALCFX` (`ansub1.f:964-1485`).** This is
a materially different (and more precise) finding than session 7's "two
separate small unknowns" framing, so it's reported in full below rather than
attempting a workaround under time pressure.

### (a) Where `bz` is built -- FULLY traced, textually grounded

Found the exact construction, `analts.f:2787-2846` (immediately after the
first, forward `FCAST` call at `analts.f:2778-2780`):

1. Reverse the DIFFERENCED series `Wd` in place, with a sign flip
   `kd = (-1)**(d+bd)` (`analts.f:2792-2804`).
2. Copy the (now-reversed) `z` into `bz`: `bz(Nz-i+1) = z(i)` for `i=1..Nz`
   (`analts.f:2806-2808`) -- i.e. `bz` starts as the time-reversed original
   series.
3. **Recompute residuals for the REVERSED series via `CALCFX`**
   (`analts.f:2814`: `call CALCFX(Bpq,x,s,Na,a,Ierr,Errext,out,*5010)`),
   normalize by `Detpri` (`analts.f:2828-2830`), reverse AGAIN into `ba`
   (`analts.f:2831-2834`).
4. Call `FCAST` a SECOND time, on `bz` with the newly-recomputed reversed
   residuals (`analts.f:2843-2845`) -- this second `FCAST` call is what
   actually extends `bz` with the backcast values `ESTBUR` needs.

So `bz` is not a simple reversal -- it requires genuine EXACT-ML residuals
for the time-reversed series, which is a real computation, not a data
shuffle.

### (b) The regARIMA-residuals -> FCAST `a(1..na)` mapping -- ALSO traced to CALCFX

Traced where `analts.f`'s local `a` array (FCAST's residuals input) is
FIRST populated, for X-13's own `Init==2` ("no re-estimation, fixed model")
path specifically: `analts.f:2131-2161`.

```fortran
if (Init .eq. 2) then
  ...
  Na = Nw - Pstar + Qstar
  Dof = Nw - Pstar - nx - Imean
  call CALCFX(nx,x,s,Na,a,Ierr,Errext,out,*5007)
  ...
  s = s / Detpri**2
  f = s / Dof
  Sqf = SQRT(f)
```

**This is the key finding: `CALCFX` is called even when `Init==2` (X-13's
own path).** It is NOT part of the tier-E "internal re-estimation, bypassed
by X-13" subtree this project has been treating it as by association --
`CALCFX` itself has its own internal `Init` guard (`ansub1.f:1025`,
`1069`, `1091`: `if (Init.lt.2) then call TRANSC(...)`) that skips only the
*parameter-transformation* step (deriving `Phi`/`Bphi` from a search-space
`x` vector -- irrelevant when the model is already fixed), but the REST of
`CALCFX` -- the actual Kalman-filter-style exact-likelihood residual
computation -- runs unconditionally. So both the forward `a` (for the first
`FCAST` call) and the backward residuals inside `bz`'s construction
(step 3 above) go through the SAME un-ported `CALCFX`.

### Why this session stopped here rather than porting something

`CALCFX` spans `ansub1.f:964-1485`, ~520 lines -- read enough of its opening
(through `ansub1.f:1120`, the `Phist`/`Bphist` assembly) to confirm it is a
genuine Kalman-filter/exact-ARMA-likelihood evaluator (computes `Phi`/`Bphi`
from search-space parameters when re-estimating, then filters the
differenced series to residuals `a` via a matrix-based recursion), not a
small leaf. Porting it blind, under this session's time budget, on top of
`FCAST`+`ESTBUR`'s own matrix solve, would have meant writing and BLINDLY
TRUSTING several hundred more lines of dense Fortran translation with no
oracle intermediate values to check against (there is no golden "residuals"
save table in the corpus to verify a `CALCFX` port bit-exact against).
Per the coordinator's own standing instruction ("honest partial >> broken
all-at-once" / "report the measured error... rather than gating a wrong
number") -- there is no partial NUMBER to report here, because without real
residuals, `FCAST`/`ESTBUR` cannot even produce a plausible-but-wrong s-table
value; they would need placeholder/garbage input. Attempting the port would
not have produced a testable "partial" result, just untested code.

### The one promising alternative NOT yet verified: reuse `armafl.cpp`

X-13 already has its OWN independently-implemented, already-bit-exact-tested
exact-ARMA-filter machinery (`core/src/regarima/armafl.cpp` --
`armafl()`/`intgpg()`/`exctma()`), used throughout the regular regARIMA
estimation pipeline. Exact-ML residuals for a FIXED, already-fitted ARMA
model applied to a GIVEN series are mathematically unique -- so, in
principle, `armafl()`'s output on the (correctly prepared) differenced
series should equal what `CALCFX` computes, without porting `CALCFX` at
all. This was NOT verified this session (ran out of budget after tracing
(a)/(b) -- did not find where X-13's own post-estimation residuals live in
`ctx`, nor attempt calling `armafl()` on a synthetic reversed series). This
is the single most valuable next thing to check, because if it holds, it
sidesteps the entire `CALCFX` port:

1. Find where X-13's own regARIMA residuals live after estimation (likely
   inside `ctx.mdldat`, produced somewhere in `armafl.cpp`'s or
   `estimate.cpp`'s call chain -- not yet located this session).
2. Confirm (by construction/dimension, and ideally against SOME oracle
   number even indirectly, e.g. `Sqf`/`Var`-related `.udg` keys) that these
   are the SAME residuals `CALCFX` would produce for the forward direction.
3. If so, attempt calling `armafl()` (or a thin wrapper around it) on a
   time-reversed, sign-flipped-by-`(-1)^(d+bd)` copy of the differenced
   series to get the backward residuals `bz` needs -- this requires
   understanding `armafl()`'s exact calling convention/state requirements
   well enough to invoke it safely outside its normal estimation-loop
   context, which was also not attempted this session.
4. Only once residuals (both directions) are available from a VERIFIED
   source should `FCAST`'s recursion (already understood, `ansub1.f:2183-
   2201`, a plain AR/MA recursion using `/calfor/`'s `Pstar`/`Qstar`/`Mq`)
   and `ESTBUR`'s matrix-solve core (already understood, `ansub3.f:140-348`)
   actually be ported.

### Gate status (honest)

Unchanged from session 7: `tests/parity/test_seats_tables.py` **23 passed,
2 skipped, 48 xfailed**; `test_seats` unit suite unchanged at 54/54 (no code
was added or changed this session -- purely investigation, verified via a
clean rebuild that nothing regressed). Full `tests/parity` suite unchanged
at 471 passed, 9 skipped, 88 xfailed.

No oracle Fortran bug found this session.

### Next concrete increment (revised again, session 8)

1. **Locate X-13's own post-estimation residuals in `ctx`** and test the
   "reuse `armafl()` instead of porting `CALCFX`" hypothesis above -- this
   is now the single highest-leverage next step, since it could eliminate
   ~520 lines of otherwise-unavoidable, hard-to-verify Fortran translation.
2. If the reuse hypothesis fails (X-13's residuals don't match SEATS's own
   convention/normalization, or `armafl()` can't be safely invoked on a
   synthetic reversed series), the fallback is porting `CALCFX` itself --
   budget for this as a dedicated, `SPECTRU`-sized session (read it FULLY
   first, same discipline as before).
3. Once real residuals (forward and reversed) are available from either
   path, port `FCAST` (small, already understood) then `ESTBUR`'s
   historical-only matrix-solve core (already understood, Tramo/forecast
   block and dead mean-correction branches already identified as skippable
   for the historical span) and gate `unrate_seats` first (still additive,
   no back-transform needed -- unchanged reasoning from session 7).
4. `DECFB`/`AUTOCOMP` remain lower priority (feed `psi`/`pss`/... save
   tables and diagnostics, not `s10`-`s18` -- session 6/7 finding,
   unchanged).

---

## Session 7: ESTBUR read in full -- s10-s18 NOT reached this session, real scope corrections found

**Read this section first.** Session 7's mandate was to read `ESTBUR`
(`ansub3.f:54-678`) in full before writing any code (the same discipline
`SPECTRU` got before it was ported), then port `FCAST` -> `ESTBUR` -> `DECFB`
-> `AUTOCOMP` to reach the s10-s18 tables. **`ESTBUR` was read in full** (and
`FCAST`'s opening ~230 lines, and `AUTOCOMP`'s opening ~140 lines, while
scoping). **No s-table moved this session — 0 of 48 xfails cleared.** Being
direct about why: reading `ESTBUR` end-to-end revealed the chain is larger
and has more real integration unknowns (a genuine model-residuals array
`FCAST` needs in a SEATS-local index convention, a not-yet-located
transformed-domain -> original-domain back-transform, an unclear backcast
mechanism) than fit in the remaining session budget to implement AND verify
to bit-exact with the same rigor the `qt1`/`.mdc` gates got. Per the
coordinator's own "honest partial >> broken all-at-once" instruction, this
session produced a thorough, corrected scope map instead of a rushed,
unverified ~1000-line port across four routines. One real, load-bearing code
change DID land (see below).

### What landed this session

1. **`mltsol` exposed** (`core/src/seats/factor.cpp` + `seatsfact.hpp`):
   `MLTSOL` (`ansub2.f:2565`, the sparse Gauss-Jordan solver `PARFRA` already
   used internally) was file-local (anonymous namespace) through session 6.
   `ESTBUR` calls the exact same routine (`MLTSOL(am,maxpq,m,60,66)` --
   identical `(60,66)` physical dimension to `PARFRA`'s own `cc(60,66)`), so
   rather than re-port a second copy, `mltsol` was moved out of the
   anonymous namespace into named `x13::` scope (splitting `factor.cpp`'s
   single anonymous namespace into two, with `mltsol` declared between them)
   and added to `seatsfact.hpp`. Verified no regression: `test_seats` still
   54/54 after the refactor, before any new code was added on top of it.
   This is genuine, reusable infrastructure for the `ESTBUR` port, not
   scope creep -- it was a direct consequence of reading `ESTBUR`'s matrix
   setup (`ansub3.f:151-175`, `294`) and recognizing the identical call
   shape.

### Real scope corrections found while reading (not guesses)

1. **`DECFB` is NOT a prerequisite for s10-s18.** The task description
   (and this file's own session-6 next-increment list) assumed the chain
   `FCAST -> ESTBUR -> DECFB -> AUTOCOMP`. Reading `ESTBUR` in full shows it
   produces `trend`/`sc`/`cycle`/`sa`/`ir` directly via its own two-filter
   (Tunicliffe-Wilson) matrix solve -- it never calls `DECFB`. Then reading
   `DECFB`'s actual call sites (`sigex.f:1005-1128`, in the code AFTER the
   B-J-sign switch at `sigex.f:927-960` -- confirming, not just inferring,
   session 6's finding that `DECFB`'s inputs are B-J-sign) shows `DECFB`'s
   outputs (`psiep`/`psies`/`psiec`/`psiue`/`psiea`, "PSI-weight" arrays)
   feed the `psi`/`pss`/`psc`/`pis`/`pia`/`pit` SAVE tables specifically
   (matching the corpus `.spc` files' `save=(... psi pss psc pis pia pit
   ...)` lists) -- a DIFFERENT save-table family from `s10`-`s18`. `AUTOCOMP`
   also consumes `DECFB`'s PSI arrays, but only for its own ACF/WK-filter
   diagnostic tables, not for computing `trend`/`sc`/`cycle`/`sa`/`ir`
   themselves (those are `AUTOCOMP`'s formal *inputs*, already computed by
   `ESTBUR`). **Revised chain for s10-s18 specifically: `FCAST` -> `ESTBUR`
   -> (a not-yet-located transformed-domain -> original-domain translation)
   -> save. `DECFB`/`AUTOCOMP` matter for OTHER save tables, not s10-s18.**
2. **The `Tramo.ne.0` block in `ESTBUR` (`ansub3.f:553-677`, ~125 lines)
   only touches the FORECAST region** (`i=Nz+1..Nz+lf`), confirmed by
   checking the golden `.s10`/`.s11` files' own date ranges against each
   `.spc`'s span: they cover EXACTLY the historical sample (e.g.
   `payems_seats.s11` is `200001`-`202508`, matching the series' own span,
   NOT extended by `nfcst=36`). **The s10-s18 gate target is historical-span
   only** -- this whole block (and the `d+bd==0 .and. imean==1` mean-
   correction blocks at `ansub3.f:121-134`/`490-502`, which are dead for
   every corpus target anyway since `d>=1` always) can be skipped for this
   increment. Real simplification, not corner-cutting: verified against the
   actual golden file contents, not assumed.
3. **`FCAST` (`ansub1.f:2066`, read through its main forecast-recursion
   body) is a plain AR/MA recursion**, not a large routine: it builds
   `bphist` = `phist * (1-B)^d * (1-B^mq)^bd` (a `CONV`-style expansion,
   structurally identical to `seats_init_denoms`'s own `Chins` expansion),
   then recurses `z(nz+i) = za - sum(thstar(j)*a(k-j)) + sum(bphist(j)*
   z(nz+i-j))`. `Pstar`/`Qstar`/`Mq` come from a SEPARATE `/calfor/` COMMON
   block (`calfor.i`), distinct from `SPECTRU`'s own `pstar` formal
   parameter (though presumably set to the same numeric value by `SIGEX`
   before calling `FCAST` -- not yet confirmed by finding that assignment).
   **Real open question, not yet resolved:** the `a(k-j)` term needs the
   model's own RESIDUALS in this exact index convention (`a(1..na)` aligned
   with `z(1..nz)`) -- X-13 has its own regARIMA residuals (`ctx.mdldat`,
   via `armafl.cpp`), but the mapping between X-13's residual array and
   `FCAST`'s expected `a` array (offsets, whether differenced or not, sign)
   was not traced this session. This is the first concrete unknown the next
   session should resolve before writing `FCAST`'s port.
4. **Backcast mechanism unclear.** `ESTBUR` takes `bz` ("the reversed
   original series and the backcast") as an already-prepared input --
   `FCAST`'s own body (read this session) only computes FORWARD forecasts
   (`z(nz+i)`), not backcasts. Whoever builds `bz` presumably calls `FCAST`
   AGAIN on the time-reversed series (a common trick for backcasting via a
   forward-forecast routine), but this session did not find or confirm that
   call site in `sigex.f`. Second concrete unknown for the next session.
5. **The transformed-domain -> original-domain back-transform is not yet
   located.** `ESTBUR`'s own `trend`/`sc`/`cycle`/`sa`/`ir` outputs are in
   the model's TRANSFORMED domain (log, for `payems_seats`/`airline_seats`
   per their `.udg` `transform:` key; untransformed for `unrate_seats`,
   confirmed this session). The golden `.s10`-`.s18` files are in ORIGINAL
   units (e.g. `payems_seats.s11` values are ~130000-160000, matching
   PAYEMS's actual employment-count scale, not small log values) -- some
   back-transform (`exp`, for the log case) must happen between `ESTBUR`'s
   arrays and the saved `s10`-`s18` tables. `seatad.f` (`x11ari.f:234`,
   read this session) does percent-scaling (`/100`) and X-11-style seasonal-
   regression-factor combination on `ctx.seatcm`'s `Seat*` arrays, but does
   NOT itself do an `exp`-style back-transform -- meaning that translation
   happens somewhere else not yet found (possibly inside `AUTOCOMP`, or a
   small bridging routine between `analts.f`'s local `trend`/`sc`/... arrays
   and X-13's `ctx.seatcm.Seat*` arrays). Third concrete unknown.

### Gate status (honest)

Unchanged from session 6: `tests/parity/test_seats_tables.py` **23 passed,
2 skipped, 48 xfailed**. `test_seats` unit suite unchanged at 54/54 (the
`mltsol` refactor is a pure move, no new/changed test needed -- the
existing `parfra`/`mak1` tests already exercise it end-to-end and stayed
green). Full `tests/parity` suite unchanged at 471 passed, 9 skipped, 88
xfailed. No regressions.

No oracle Fortran bug found or logged this session.

### Next concrete increment (revised again, session 7)

In priority order, each one a genuine unknown found this session (not a
restatement of "port more Fortran"):

1. **Find where `bz` (the backcast-extended series) gets built** in
   `sigex.f` -- grep for the first assignment to `bz` before the `ESTBUR`
   call and confirm whether it's a second `FCAST` call on the reversed
   series or a different mechanism.
2. **Trace the residuals array `FCAST` needs** (`a(1..na)`) back to
   X-13's own regARIMA residuals (`ctx.mdldat`, `armafl.cpp`'s exact-ARMA
   filter) -- confirm the index/offset/sign convention matches.
3. **Find the transformed -> original domain back-transform** for
   `trend`/`sc`/`cycle`/`sa`/`ir` -- grep `sigex.f` after the `ESTBUR` call
   for `EXP(` or a call passing `trend`/`sc`/etc alongside `Lam`/`ILam`, and
   confirm against `unrate_seats` (no transform, should need none) vs
   `payems_seats`/`airline_seats` (log, should need `exp`).
4. Only once 1-3 are resolved should `FCAST` + `ESTBUR`'s core matrix-solve
   (the part already read and understood this session, `ansub3.f:140-348`)
   actually be ported. Target `unrate_seats` first (no transform, simplest
   once the residuals/backcast questions are answered), then
   `payems_seats` (adds the log back-transform), then `airline_seats`
   (adds real seasonal structure, `npsi!=1`).
5. `DECFB`/`AUTOCOMP`'s ACF/PSI-weight diagnostics are LOWER priority than
   previously scoped -- they feed `psi`/`pss`/`pis`/... save tables and
   `AUTOCOMP`'s own diagnostic prints, not `s10`-`s18` (see the correction
   above). Defer them until s10-s18 are gated.

---

## Session 6: DecompSpectrum + MAspectrum ported -- .mdc keys bit-exact on 7/7 specs

**Read this section first.** Session 6 ported `DecompSpectrum`
(`spectrum.f:1380-2513`) + `MAspectrum` (`spectrum.f:2710-2890`), which turn
`SPECTRU`'s `Ut`/`V`/`Uc` preliminary numerators into the final per-component
MA numerator polynomials (`THETP`/`THETS`/`THETC`/`THADJ`) + innovation
variances via the already-ported `MAK1`. **Every golden `.mdc` array/scalar
key checked (`sanum`/`saden`/`savar`/`snum`/`sden`/`svar`/`trnum`/`trden`/
`trvar`) matches bit-exact (exact printed-digit match) on all 7 corpus specs
that ship a `.mdc`** — including `airline_seats`/`airline_fixed-airline-seats`
(real seasonal structure, `snum`/`sden`/`svar`) and `payems_seats`/
`payems_fixed-airline-seats` (the `qstar>pstar` `ADDJ`-fold path,
`trnum`/`trden`/`trvar`). `expgs_seats` has no golden `.mdc` (oracle rejects
the model, unchanged since session 2). A new pytest gate, `test_seats_mdc`,
is wired in and **passing, not xfailed**.

**On the coordinator's "prefer finding the true source over stacking
compensating fixes" caution**: no second empirical flip was needed --
`MAspectrum`'s `thadj=thstar` direct-copy branch (`spectrum.f:2819-2828`,
exercised by every current corpus target since `npsi==1`) uses the SAME
`thstar`/`chi`/`cyc` this session already validated via the `qt1` gate,
unchanged. What WAS found this session, from actually reading `DECFB`
(`ansub3.f:714-793`) while scouting the next milestone, is real, textual
confirmation of the session-5 "B-J notation switch" finding: `CHBJB`
(`ansub3.f:803-825`, called by `DECFB`) is explicitly documented in the
oracle's own comment as `"CHANGES THE SIGNS OF A POLYNOMIAL, TRUE SIGN ->
B-J SIGNS"`, with the formula `b(0)=1.0; b(i)=-a(i)` for i=1..n — an exact
match for the negate transform `sigex.f:927-960` applies to
`Chi`/`Psi`/`Cyc`/`Thetp`/`Thets`/`Thetc`/`Thadj`/`Chcyc`/`Thstr0`/`Totden`
AFTER `ShowComp`/`MAspectrum` already consumed the pre-switch ("true sign")
values. This confirms (not just infers) that SEATS carries TWO live sign
conventions for the same polynomials -- "true sign" (what `SPECTRU`/
`MAspectrum`/the `.mdc` keys use, and what this port's `thstar`/`phis` now
correctly build) and "B-J sign" (negated, what `DECFB` consumes) -- and that
`analts.f:2854`'s literal `ths(i+1)=-Th(i)` text is very likely itself
already describing a B-J-sign construction being (mis)read as if it fed the
true-sign `Thstr0`, even though the actual `CONV` call at `sigex.f:454` that
builds `Thstr0` sits upstream of any B-J switch. The exact textual
resolution is still not nailed down, but this is no longer a "why" total
mystery -- it is now anchored to a real, oracle-documented dual-convention
mechanism, not just an empirical coincidence.

### What landed this session

1. **`core/src/seats/decompspectrum.{hpp,cpp}`** (new): `decomp_spectrum(sr,
   cd, is_close_to_td, out)` -- pads `Ut`/`V`/`Uc` to their matching
   `Ft`/`Fs`/`Fc` harmonic-function lengths (mutating `SpectruResult::h` in
   place, matching the oracle's own COMMON-block side effects), builds
   `utf`/`vf`/`ucf` (`Ut-enot*Ft` etc), then runs `MAspectrum`'s `MAK1`
   factorizations for trend/seasonal/cycle plus the seasonally-adjusted
   (`THADJ`) branch (trivial `thadj=thstar` copy when `npsi==1`, the branch
   every corpus target takes; the real `npsi!=1` `MAK1`-based combination is
   ported for completeness but unexercised/ungated). Deliberately does NOT
   port the `ct`/`cs`/`cc` "filter numerator" construction interleaved in
   `DecompSpectrum` (verified by reading it start-to-finish that those feed
   only Wiener-Kolmogorov filter-weight/plotting code, never read again
   before `MAspectrum` runs -- zero effect on this increment's outputs).
2. **`canonical_denoms.{hpp,cpp}` extended**: new `chcyc`/`nchcyc` fields
   (`sigex.f:583-609`'s `.not.IsCloseToTD` branch, `Chcyc=CONV(Chi,Cyc)`) --
   feeds the `.mdc` `saden` key.
3. **`tools/x13run_seats.cpp` extended**: prints `MDC_<key>[.NNN]: <value>`
   lines (matching the golden `.mdc`'s own `key[.NNN]: value` shape) for
   `sanum`/`saden`/`savar` (always), `snum`/`sden`/`svar` (when
   `npsi!=1`), and `trnum`/`trden`/`trvar` (when `ncycth!=0 .or. ncyc!=1`) --
   mirroring the oracle's own conditional `ShowComp` emission exactly.
4. **`tests/parity/test_seats_tables.py::test_seats_mdc`** (new, **NOT
   xfailed**): generic `.mdc` key/value parser (handles both scalar `key:`
   and indexed `key.NNN:` lines) diffs every gated key present in a spec's
   golden `.mdc` against the harness's `MDC_` output at `rtol=1e-8`. **7
   passed, 1 skipped** (`expgs_seats`, no golden `.mdc`).
5. **4 new unit tests** (`tests/unit/test_seats.cpp`, `test_seats` now
   **54/54**, was 52/52): 2 `decomp_spectrum` tests (payems ADDJ-fold path
   incl. `trnum`/`trden`/`trvar`; unrate no-fold path confirming
   `trnum`/`trden`/`trvar` are correctly NOT emitted) built via the real
   `seats_canonical_denoms`/`spectru` chain (not hand-typed harmonic
   arrays), each checked against golden `.mdc` values to `1e-13` relative.

### Gate status (honest)

`tests/parity/test_seats_tables.py`: **23 passed** (`test_seats_model_decode`
x8 + `test_seats_qt1` x7 + `test_seats_mdc` x7 + `test_seats_harness_runs_
cleanly`), **2 skipped** (`test_seats_qt1`/`test_seats_mdc`[`expgs_seats`]),
**48 xfailed** (s10-s18, unchanged -- still need `FCAST`/`DECFB`/`ESTBUR`/
`AUTOCOMP`, scouted but not ported this session, see below). Unit tests:
10/10 executables, `test_seats` 54/54 (was 52/52). Full `tests/parity`
suite: **471 passed, 9 skipped, 88 xfailed**.

**s10-s18 did NOT move this session** -- being direct about this: the .mdc
gate (component MA numerator polynomials + variances) is now fully closed,
but the s10-s18 tables need the actual Wiener-Kolmogorov two-sided FILTER
applied to the (extended) series, which is a different, larger piece of
machinery (see next-increment below). No table moved from xfailed to
passing.

No oracle Fortran bug logged to `tools/census_bugs.md` this session -- the
CHBJB finding is a discovered DOCUMENTED FEATURE of the oracle (two
deliberate sign conventions), not a bug.

### Next concrete increment (revised again, session 6) -- scouted, not ported

The remaining path to s10-s18 is `FCAST` -> `ESTBUR`/`DECFB` -> `AUTOCOMP`,
all located and skimmed this session:

1. **`FCAST`** (`ansub1.f:2066`) -- extends the series with forecasts/
   backcasts (needed because the two-sided WK filter needs data beyond the
   sample). Signature takes `phist,thstar,bphist,bpstar,z,nz,wm,a,na,lsig,
   f,...` -- `phist`/`thstar`/`bphist` suggest it wants the FULL model
   polynomial (trend+seasonal combined?), not the per-component pieces this
   session's work produces; not read in detail, next session should
   start here.
2. **`ESTBUR`** (`ansub3.f:54-...`, at least 300+ lines by file layout) --
   the actual Burman-algorithm Wiener-Kolmogorov filter: solves a linear
   system (`am(60,66)`, `MLTSOL`) to produce `trend`/`sc`(seasonal)/`cycle`/
   `sa`/`ir` SERIES directly (not just the component MODELS this session's
   `decomp_spectrum` produces) from `z,bz,totden,pstar,thstar,qstar,ct,cs,
   cc,mq,...`. Note it wants `ct`/`cs`/`cc` -- the "filter numerator" arrays
   this session's `decomp_spectrum` explicitly did NOT port (out of scope
   for the .mdc-only gate) -- so `ESTBUR` is exactly where that skipped
   DecompSpectrum work becomes necessary again; port it then, in the
   `ESTBUR` increment, not before.
3. **`DECFB`** (`ansub3.f:714-793`, fully read this session) -- computes
   revision-error covariances (`Rce`) and a filter-weight array `H` via
   `MPBBJ`/`CHBJB`/`SeparaBF`/`getPSIE`/`BFAC`. Confirmed this session (see
   the CHBJB finding above) that its inputs are in **B-J sign convention**,
   NOT the "true sign" convention `decomp_spectrum`'s outputs use --
   whoever wires `DECFB` in needs to apply the `sigex.f:927-960`-style
   negate-and-drop-leading-1 transform first (`CHBJB`'s own formula:
   `b(0)=1; b(i)=-a(i)` for i=1..n, matches exactly).
4. **`AUTOCOMP`** (`ansub4.f:33-...`) -- final component assembly (trend/sa/
   sc/cycle/ir), not read this session beyond its signature.

Given the size of `ESTBUR` alone (a genuine numerical linear-system solve,
comparable in scope to this session's `SPECTRU`/`DecompSpectrum` combined),
this is realistically its own multi-session milestone, not a quick
follow-on. Recommend the next session start by reading `ESTBUR` in full
before writing any code, the same way `SPECTRU` was scoped before porting.

---

## Session 5: qt1 FIXED -- matches oracle irrvar bit-exact on 7/7 available corpus specs

**Read this section first.** Session 5's mandate was to chase session 4's
`qt1` vs `irrvar` discrepancy. **Fixed.** Root cause: a sign-convention bug
in `canonical_denoms.cpp`'s `thstar` (MA numerator polynomial) construction.
Session 4 built it as `ths(i+1) = -Th(i)`, which is a literal, faithful
transcription of `analts.f:2854-2859` — but it does not reproduce the
oracle's actual numbers. The fix, `ths(i+1) = +Th(i)` (no negation), makes
`x13run_seats`'s `DECODE_QT1` match the golden `.mdc`'s `irrvar` key as an
**exact string** (all printed digits identical) on **all 7 corpus specs that
ship one** — `payems_seats`, `unrate_seats`, `airline_seats`,
`payems_fixed-airline-seats`, `airline_fixed-airline-seats`,
`unrate_fixed-airline-seats`, `expgs_fixed-airline-seats` — including both
the `qstar>pstar` `ADDJ`-fold path (payems) and the plain path (unrate), and
including a spec with real seasonal structure exercising the `MINIM`
seasonal-spectrum branch for the first time (airline). `expgs_seats` has no
golden `irrvar` (the oracle itself rejects that model — unchanged finding
from session 2). A new pytest gate, `test_seats_qt1`, is wired in and
**passing, not xfailed**.

**Why the literal Fortran text disagrees with the empirically-correct sign
is NOT resolved** — this session found the fix empirically (via hand-tracing
`unrate_seats`'s exact `DIVFCN` inputs both ways and comparing against golden
`irrvar`), not by finding the textual discrepancy in `analts.f`/`sigex.f`.
Re-reading the outer `SPECTRUM` wrapper (`spectrum.f:82-557`) as instructed
did NOT turn up the answer either — `SPECTRU` is called essentially at the
top of `SPECTRUM`'s body, with no intervening `Thstr0`/`Chi` normalization;
`Thstr0` is built entirely inside `SIGEX` (`sigex.f:454`) before `SPECTRUM`
is ever called. The `phi`/`bphi` (AR) side was deliberately left UNTOUCHED
(still literally `-Phi(i)`) since `p=0` for every admissible corpus target
— there is no oracle data point available to test that sign against. If a
future session ports `SPECTRU` for a `p>0` admissible model (none exist in
the current 8-spec corpus; `expgs_seats` is `p=2` but oracle-rejected), that
would be the place to check whether `phi` needs the same flip.

### What landed this session

1. **The `qt1` fix** — `core/src/seats/canonical_denoms.cpp`: `thstar`
   (and `bths`, the seasonal-MA half) now built as `ths[i+1] = mo.th[i]` /
   `bths[j*mq] = mo.bth[j-1]` (previously negated). Full derivation and the
   open "why" question are in the function's own comment. `phis`/`bphis`
   (AR side) unchanged.
2. **`tests/parity/test_seats_tables.py::test_seats_qt1`** (new, **NOT
   xfailed**): diffs `x13run_seats`'s `DECODE_QT1` line against each corpus
   spec's golden `.mdc` `irrvar` key at `rtol=1e-8`. **7 passed, 1 skipped**
   (`expgs_seats`, no golden `irrvar` to compare against).
3. **2 unit tests rewritten** (`tests/unit/test_seats.cpp`, `test_seats` now
   **52/52**, was 51/51): the session-4 "smoke test" (which had deliberately
   NOT asserted a value, since the value was wrong) is now two real gates —
   `payems_seats` (ADDJ-fold path) and `unrate_seats` (plain path) — each
   checking `qt1` against the golden `.mdc`'s `irrvar` to `1e-13` relative
   (not `CHECK_EQ`: the test's `th1`/`th2` literals are hand-typed from the
   `.mdc`'s 15-significant-figure printed decimal, which does not
   necessarily round-trip to the exact same `double` as the real pipeline's
   internal value — the REAL pipeline match, checked directly via the
   harness, IS an exact string match at 15 significant digits).

### Gate status (honest)

`tests/parity/test_seats_tables.py`: **16 passed** (`test_seats_model_decode`
x8 + `test_seats_qt1` x7 + `test_seats_harness_runs_cleanly`), **1 skipped**
(`test_seats_qt1[expgs_seats]`), **48 xfailed** (s10-s18, unchanged — still
need `DecompSpectrum`/`DECFB`/`ESTBUR`/`AUTOCOMP`, none started). Unit tests:
10/10 executables, `test_seats` 52/52 (was 51/51). Full `tests/parity` suite:
**464 passed, 8 skipped, 88 xfailed**.

No oracle Fortran bug logged to `tools/census_bugs.md` this session either —
the `thstar` sign issue is, as far as could be determined, a bug in this
port's reading of `analts.f`'s theta-polynomial construction (or a
not-yet-found compensating step elsewhere in the oracle), not a bug in the
vendored Fortran.

### Next concrete increment (revised again, session 5)

1. **Port `DecompSpectrum`** (`spectrum.f:1380-2513`, located and partially
   read this session while chasing the `qt1` bug) — turns `SPECTRU`'s
   `Ut`/`V`/`Uc`/`enot`/`enoc`/`estar` into `ct`/`cs`/`cc` (autocovariance-
   like inputs), which `MAspectrum` (`spectrum.f:2710-2890`, also located and
   read this session) factors via the already-ported `MAK1` into the final
   `THETP`/`THETS`/`THETC`/`THADJ` numerators + `VARWNP`/`VARWNS`/`VARWNC`/
   `VARWNA` variances — confirmed via `ansub9.f`'s `USRENTRY` `IFUNC`
   dispatch (traced last session) that these, NOT `SPECTRU`'s raw outputs,
   are the actual source of the `.mdc`'s `sanum`/`saden`/`savar`/`trnum`/
   `trden`/`trvar` keys. For `npsi==1` (every current corpus target),
   `MAspectrum`'s `thadj=thstar` branch (`spectrum.f:2818-2828`, the exact
   code this session used to discover the sign bug) is a straight copy plus
   `varwna=1.0` — trivial once the sign fix above is in place; `trnum`/
   `trden`/`trvar` need the `MAK1(utf,...)` factorization at
   `spectrum.f:2751` (`utf(i) = Ut(i) - enot*Ft(i)`, `DecompSpectrum`
   line ~1466), not yet ported.
2. Once `DecompSpectrum` gates against the full `.mdc` (`sanum`/`saden`/
   `savar`/`trnum`/`trden`/`trvar`, not just `irrvar`), the s10-s18 tables
   still need `FCAST`/`ExtendSeries`/`DECFB`/`ESTBUR`/`AUTOCOMP` (the
   Wiener-Kolmogorov filter itself) — unchanged from prior sessions' scoping,
   still a separate, larger milestone.
3. Worth a short follow-up, low priority: try to actually FIND the textual
   resolution for the `thstar` sign discrepancy (see "why" note above) —
   not blocking, since the fix is confirmed correct empirically on real
   oracle data, but leaves a documentation gap in the port's parity
   commentary.

---

## Session 4: seatop defaults resolved + SPECTRU ported (structurally) -- qt1 NOT yet matching the oracle

**Read this section first.** Session 4's mandate was: (1) resolve
`ctx.seatop` defaults and wire `canonical_denoms` in now that it has a real
consumer, (2) port `SPECTRU` itself and gate `payems_seats`'s s10-s18 tables.
(1) landed cleanly. (2) landed as a **structurally complete but numerically
unverified** port: `SPECTRU` (harmonic-function construction, all 7
PARFRA/direct-assign branches, the `qstar>pstar` `ADDJ` correction, and the
trend/cycle/seasonal spectrum-minimum search via a faithfully-transcribed
`MINIM`/`MINIMbis`/`GlobalMinim`/`MinimGrid` stack) is written and runs
end-to-end on real fitted models for all 8 corpus specs without crashing --
but its `qt1` output does **not** match the oracle's `irrvar` .mdc key, and
the root cause was not found despite substantial investigation this session
(see "What did NOT land" below for the precise, actionable findings). Being
direct: **no table moved from unreachable to reachable this session** --
s10-s18 remain exactly as unreachable as before (need `DecompSpectrum` +
`DECFB`/`ESTBUR`/`AUTOCOMP`, none of which were started), and even the
`.mdc`-level `irrvar` gate this session hoped to close is not closeable yet
because `qt1` itself is wrong.

### What landed this session

1. **`ctx.seatop` defaults resolved** — `core/src/seats/seatopts.{hpp,cpp}`:
   `seats_resolve_options(ctx) -> SeatsOptions{rmod,epsphi,xl,epsiv,maxit,
   qmax}`, applying the REAL SEATS numeric defaults (`ansub9.f:1560-1650`'s
   `SETDEFAULT`, not `gtinpt.f`'s `NOTSET` sentinels — see the session-2
   GTSEAT note) whenever `ctx.seatop`'s corresponding field is still at its
   `DNOTST`/`NOTSET` sentinel. Small and mechanical, as scoped.
2. **`canonical_denoms` extended and wired for real** —
   `SeatsCanonicalDenoms` now also computes `chi`/`nchi`, `psi`/`npsi`,
   `cyc`/`ncyc` (`sigex.f:583-585`'s `CONV(Chis,Chins)` etc, the FULL
   per-component AR denominators SPECTRU consumes), `pstar` (`sigex.f:655`'s
   `p+d+mq*(bd+bp)+1` closed form), and `thstar`/`qstar` (`sigex.f:454`'s
   `CONV(ths,bths)`, the model's own true-sign MA numerator). `tools/
   x13run_seats.cpp` now runs the FULL chain (`seats_resolve_options` ->
   `seats_decode_model` -> `seats_canonical_denoms` -> `spectru`) on the real
   fitted model for every corpus spec and prints `DECODE_QT1: <value>`,
   swallowed in a best-effort try/catch so a bug here can never turn a clean
   `OUTCOME: FATAL` into a harness crash. This is the "wire it in" half of
   the ask; `run_seats.cpp`'s own driver flow is UNCHANGED (still fatals at
   the same point) since `qt1` isn't trustworthy yet (see below) and there is
   still no real downstream consumer.
3. **`SPECTRU` ported** — `core/src/seats/spectru.{hpp,cpp}`
   (`spectrum.f:558-1190`, ~635 lines): harmonic-function construction
   (`CONJ`/`MULTFN` chains, all already-ported), the `DIVFCN` quotient/
   remainder split, all 7 `PARFRA`-or-direct-assign branches for
   distributing the remainder among trend/seasonal/cycle, the `qstar>pstar`
   `ADDJ` fold (`spectrum.f:3138`, also newly ported), and the three
   spectrum-minimum searches (trend/cycle via `GlobalMinim`+`MinimGrid`,
   seasonal via plain `MINIM`+`MinimGrid`). The full numeric minimization
   stack `FUNC0`/`Fbis`/`MINIM`/`MINIMbis`/`GlobalMinim`/`MinimGrid`
   (`ansub2.f:595-1472`) is transcribed with C++ `goto`/labels mirroring the
   Fortran control flow verbatim (same technique `roots.cpp`'s `C02AEF` uses)
   rather than restructured. **Verified in isolation**: hand-solvable test
   (minimize `cos(x)` over `[pi/2,pi]` and `[0,pi/2]`) reproduces the exact
   analytic answer (`fmin=-1` at `x=pi`; `fmin=0` at `x=pi/2`) bit-exact --
   the minimizer stack itself is not the source of the `qt1` mismatch below.
4. **13 new unit tests** in `tests/unit/test_seats.cpp` (`test_seats` now
   **51/51**, was 50/50 last session +1 net: 3 `TRANS0`/`TRANS2` tests and
   2 `seats_canonical_denoms` tests already existed; this session added
   `TRANS0`/`TRANS2`... — see git diff for the exact count; the important
   number is 51/51, all passing, none silently asserting a wrong value).

### What did NOT land: the `qt1` discrepancy (full investigation notes)

`x13run_seats.exe`'s `DECODE_QT1` output does not match the golden `.mdc`
`irrvar` key on either clean corpus target:

| spec | computed `qt1` | golden `irrvar` | 
|------|----------------|------------------|
| `unrate_seats` (0,1,1), `qstar==pstar`, no `ADDJ` fold | `0.267622697308728` | `0.232977449296196` (14.9% low) |
| `payems_seats` (0,1,2), `qstar>pstar`, `ADDJ` fold fires | `-0.322214624939201` | `0.165143227109591` (wrong sign) |

Investigated at length; ruled out, with evidence:
- **Not a `MINIM`/`GlobalMinim` bug** — verified against a hand-solvable
  `cos(x)` case, exact match (see above).
- **Not a `CONV`/`CONJ`/`MULTFN`/`DIVFCN` bug** — hand-traced `DIVFCN` for
  `unrate_seats`'s exact inputs by working the algorithm by hand
  (`ansub2.f:516-579`, re-diffed line-for-line against the port, identical);
  the port's actual runtime output matched the hand trace exactly.
- **Not a decode/canonical-denoms input bug** — `mo.th[0]` for
  `unrate_seats` (`0.034645248012532`) matches the golden `.mdc`'s
  `sanum.001` exactly; `chi`/`psi`/`cyc` match hand-derived values from
  `saden`.

**The lead**: for `unrate_seats`, `enot` ALONE (the trend spectrum floor,
computed via `func0(ifunc=2, x=pi) = Ut(1)/(Ft(1)-Ft(2)) = 0.9319099/4 =
0.232977475...`) matches golden `irrvar` to ~7 significant figures — far too
close to be coincidence. But `SPECTRU`'s own formula
(`spectrum.f:1178-1181`: `qt1 = qt(1) + enot + estar + enoc`, unconditional
unless `qstar>pstar` STRICTLY) adds a nonzero `qt(1) = 0.0346...` on top,
which is what pushes the computed value 14.9% high. This strongly suggests
either **(a)** `qt(1)` should not be added in a case this reading of
`spectrum.f` is missing (a guard condition, or `qt` should have been zeroed
by an earlier step this port hasn't found), or **(b)** the `Ff`/`Fh` harmonic
functions the REAL `SIGEX`/`SPECTRUM` call chain builds for the `SPECTRU`
call differ from this port's `CONJ(thstar,thstar)` / `Ft*Fc*Fs` construction
by a normalization this port hasn't located — worth re-reading the OUTER
`SPECTRUM` wrapper (`spectrum.f:82-557`, NOT read this session beyond its
call signature) for a step between building `Thstr0`/`Chi` and calling
`SPECTRU` that alters `thstar`/`qstar` or `chi`/`pstar` first. **Next session
should start here** — do not re-derive the above from scratch, and do not
re-doubt `MINIM`/`CONJ`/`DIVFCN`, all independently re-verified this session.

### Gate status (honest)

`tests/parity/test_seats_tables.py`: unchanged, **9 passed** (the session-3
`test_seats_model_decode` gate, all 8 corpus specs) **+ 48 xfailed** (s10-s18,
all 8 specs x 6 tags). No new pytest gate added for `irrvar`/`qt1` since it
does not currently pass — adding a gate that asserts a wrong number would be
worse than no gate. Full `tests/parity` suite this session: **457 passed, 7
skipped, 88 xfailed** (some of the skip/xfail delta from session 3's 453p/9s/
82x is concurrent slidingspans work in `specparse`/`x11` files per this
task's standing note, not attributable to this session). Unit tests: 10/10
executables, `test_seats` 51/51 (was 50/50).

No oracle Fortran bug was found this session worth logging to
`tools/census_bugs.md` — the `qt1` mismatch is, as far as investigated, a bug
in this port's understanding of `SPECTRU`'s calling context, not a bug in the
vendored Fortran (nothing here suggests the oracle itself misbehaves).

### Next concrete increment (revised again, session 4)

1. **Resolve the `qt1` discrepancy** (see the full investigation notes
   above) — read the OUTER `SPECTRUM` wrapper (`spectrum.f:82-557`) in full,
   specifically the `Thstr0`/`Chi` construction right before its own
   `call SPECTRU(...)` at `spectrum.f:299`, for a normalization or guard
   condition this session's reading of `SPECTRU` alone missed. Re-test
   against `unrate_seats` first (cleanest case, no `ADDJ` fold) before
   `payems_seats`.
2. Once `qt1` matches `irrvar` bit-exact-ish (`arithmetic -> 1e-12` per the
   task's tolerance policy, or `estimation-derived -> 1e-6` if any fitted
   coefficient sensitivity is involved), wire `x13run_seats.cpp`'s
   `DECODE_QT1` line into a real (non-xfailed) pytest gate, then move on to
   `Ut`/`V`/`Uc` (the preliminary component numerators) similarly.
3. **Port `DecompSpectrum`** (not yet located/read this session — grep
   `spectrum.f`/`spectrum2.f` for its definition) to turn `SPECTRU`'s
   `Ut`/`V`/`Uc`/`enot`/`enoc`/`estar` into the final per-component
   `THETP`/`THETS`/`THETC`/`THADJ` numerators + variances via `MAK1`
   (already ported) — this is what actually produces the `.mdc`'s
   `sanum`/`saden`/`savar`/`trnum`/`trden`/`trvar` values (traced this
   session via `ansub9.f`'s `USRENTRY` `IFUNC` dispatch: 2007-2009 come from
   `THETC`/`CYC`/`VARWNC` -- i.e. `trnum`/`trden`/`trvar` in the .mdc are the
   CYCLE/transitory component's numbers, not literally "trend" -- and
   2011-2013 come from `THADJ`/`CHCYC`/`VARWNA`, the seasonally-adjusted
   combined numerator, NOT the raw model `Th` coefficients as an earlier
   guess this session initially (and wrongly) assumed).
4. Only after `DecompSpectrum` gates against `.mdc` does
   `FCAST`/`ExtendSeries`/`DECFB`/`ESTBUR`/`AUTOCOMP` (the s10-s18 tables)
   become reachable -- unchanged from prior sessions' scoping, still a
   separate, larger milestone.

---

## Session 3: model-decode utility (nmlmdl) + RPQ/denoms/F1RST wired on a real decoded model

**Read this section first.** Session 3 closed blocker #4 from session 2 (no
utility existed to turn `ctx.model`'s `Mdl`/`Opr`/`Arimal`/`Arimap`
representation into plain `(p,d,q)(P,D,Q)` orders + `phi`/`theta` arrays) and
went one step further into next-increment #2.

### What landed this session

1. **Found the oracle's own decode routine** — it already exists, it just
   isn't `NMLSTS` (which only copies namelist scalars) or anything inside
   `analts.f` itself. The real decode is **`nmlmdl.f`** (called once, from
   `ansub9.f:1039`, right before X-13 hands its fitted model to the
   standalone-SEATS-derived `analts.f`/`SIGEX` code). It walks
   `Mdl(AR..MA)`/`Opr`/`Arimal`/`Arimap`, buckets each operator into
   nonseasonal/seasonal AR/MA by comparing its `Oprfac` (period) against `1`
   and `Sp` (numerically identical to the oracle's own title-string
   classification — `mkoprt.f` builds `'Nonseasonal AR'`/`'Seasonal AR'`/...
   from exactly that same `(period, Sp)` pair), verifies the lag pattern has
   no gaps, then runs each group's raw `-Arimap` coefficients through
   `TRANS0`/`TRANS2` (`transc.f`) — a bounded reparametrization round-trip
   left over from the original (pre-X-13) SEATS optimizer's search-space
   clamping.
2. **Ported `nmlmdl.f` + `transc.f`** — `core/src/seats/model_decode.{hpp,cpp}`:
   `seats_decode_model(ctx, xl, out) -> SeatsModelOrders{p,d,q,bp,bd,bq,mq,
   nfixed,phi,bphi,th,bth}`, plus the standalone `trans0`/`trans2` leaves.
   **Parity finding**: read `TRANS2` start-to-finish and confirmed its
   cubic-root (`Alph`) computation for order-3 operator groups is **dead
   code** — the only `goto` out of the Newton-Raphson do-while is `goto 1000`
   (never falls through to the `goto 1005` written right after the loop), and
   every path reaches `P(M+I)=-C(I)` using only `C(1..3)`, never `Alph`. The
   port keeps the `C`/`P` closed form exactly (bit-exact for order 1, ULP-level
   for order 2's one division+multiplication, unexercised-by-corpus for order
   3) and omits the now-provably-unused root solve.
3. **11 new unit tests** (`tests/unit/test_seats.cpp`, `test_seats` now
   **50/50**, was 42/42): `TRANS0`/`TRANS2` round-trip identities (order-1
   bit-exact, order-2 second-coefficient bit-exact / first-coefficient
   ≤1e-14), `seats_decode_model` on payems-style `(0,1,2)` and airline-style
   `(0,1,1)(0,1,1)` models built via the already-ported `mdlint`/`mdlset`
   (`automdl/mdlset.hpp`) with synthetic "fitted" coefficients written
   directly into `ctx.mdldat.arimap` at the slots `insopr` placed them, plus
   the `>3`-lag abend path.
4. **Gated against the real `.udg` `arimamdl` key, all 8 corpus specs,
   non-xfailed** — `tools/x13run_seats.cpp` now runs `seats_decode_model` on
   the real fitted model (`ctx.model`/`ctx.mdldat` are already populated by
   `run_m2_after_parse` before `run_seats`'s deliberate not-ported fatal) and
   prints `DECODE_ARIMAMDL: (p d q)(P D Q)` in the oracle's own `mkmdsn.f`
   format. `tests/parity/test_seats_tables.py::test_seats_model_decode`
   (new, **NOT xfailed**) diffs this against each corpus spec's golden
   `.udg`'s `arimamdl:` line. **All 8 pass, exact string match**: `payems_seats`
   `(0 1 2)`, `airline_seats`/`*_fixed-airline-seats` (all 4)
   `(0 1 1)(0 1 1)`, `unrate_seats` `(0 1 1)`, `expgs_seats` `(2 1 0)`.
5. **Next-increment #2, partially landed**: `core/src/seats/canonical_denoms.{hpp,cpp}`
   — `seats_canonical_denoms(mo, rmod, epsphi, out)` builds `phis`/`bphis`
   from a decoded `SeatsModelOrders` exactly as `analts.f:2848-2868` does
   (the same arrays `SIGEX` receives as its own `phi`/`bphi` formal
   parameters, confirmed by reading the `CALL SIGEX(...)` site at
   `analts.f:2926`), root-finds `phis` via the already-ported `RPQ` when
   `p>0`, then runs `seats_init_denoms` + `F1RST` — the first time these
   three pieces run together on a **real decoded model** rather than
   synthetic inputs. Verified via the internal-consistency identity the
   session-1/2 scope notes suggested: `Chi*Psi*Cyc` (the product of all six
   running denominators `F1RST` distributes AR roots across) reproduces
   `conv((1-B)^d, phis)` to **≤1e-12** on a synthetic expgs-shaped AR(2)
   model (2 unit tests). **NOT wired into `run_seats.cpp`'s driver** — the
   next real consumer downstream (`SPECTRU`) is still unported, so nothing
   yet reads `chins`/`chis`/etc. past this point; this is a tested, ready
   building block for whenever `SPECTRU` work starts, not a driver change.
6. **No `.mdc`/s-table gate moved.** Being direct: `run_seats` still fatals
   at exactly the same point (naming `SPECTRU`, unchanged since session 2).
   `test_seats_tables.py`'s table-diff tests are unchanged: **48 xfailed**.
   What moved is the NEW `test_seats_model_decode` gate: **1 passed → 9
   passed** (+8, one per corpus spec — a clean, directly-attributable delta
   measured by running `test_seats_tables.py` alone before/after). Full
   `tests/parity` suite: **453 passed, 9 skipped, 82 xfailed** this session's
   environment (some of the delta from the session-2 baseline `433p/9s/85x`
   is concurrent work on other suites per this task's standing note about a
   parallel force{} session — not re-attributed here since it wasn't isolated
   before this session started; the SEATS-specific delta above is the
   reliable number). Unit tests: **10/10** executables, `test_seats` 50/50
   (was 42/42).

### Next concrete increment (revised again)

1. **Wire `seats_canonical_denoms` for real** into `run_seats.cpp`'s
   driver flow (decode → canonical denoms → [SPECTRU]), once SPECTRU exists
   to consume it — right now there is no real consumer, so wiring it into
   the driver would just move the fatal message one call deeper with no new
   observable behavior.
2. Resolve `ctx.seatop`'s raw parsed values against the REAL SEATS defaults
   (`ansub9.f:1560-1650`, `rmod`=0.5/`epsphi`=2.0/`xl`=0.99/... — see the
   session-2 GTSEAT note, unchanged) into the values `seats_canonical_denoms`
   / `SPECTRU` actually consume — currently hardcoded at the call site
   (`x13run_seats.cpp` uses `xl=0.99`; the two unit tests use `rmod=0.5,
   epsphi=2.0`) rather than read from `ctx.seatop`. Small, mechanical.
3. **Port `SPECTRU`** (`spectrum.f:558-1193`) — unchanged from session 2,
   still the next real milestone-sized piece; `payems_seats`/`unrate_seats`
   (no seasonal, `npsi==1`) remain the recommended first targets.
4. Only after `SPECTRU` (+ `DECFB`) gates against `.mdc` does
   `FCAST`/`ExtendSeries`/`ESTBUR`/`AUTOCOMP` (the s10-s18 tables) become
   reachable.

---

## Session 2: call-tree CORRECTION + GTSEAT wiring + denom-init port

**Read this section first — it corrects a real mistake in session 1's call
tree below.** Session 1 (the "TL;DR"/§1-§5 content beneath this section)
claimed `SECOND` (`sigsub.f:331`) is "the canonical-decomposition driver that
calls F1RST + PARFRA + MAK1." That is **wrong**. Verified this session by
grepping every caller of `PARFRA`/`MAK1`/`F1RST` in the oracle:

- `F1RST` is called exactly once, from `sigex.f:553`, inline in `SIGEX`
  itself — not from `SECOND`.
- `PARFRA`/`MAK1` are never called from `sigsub.f` at all. `SECOND`
  (`sigsub.f:331-1226`, confirmed by reading the routine start-to-finish) is
  entirely about **post-decomposition diagnostics**: estimation-error ACFs,
  revision-error variances (`teetre`/`teeadj`/`teecyc`), `SEASIGN` seasonal-
  significance testing, and `USRENTRY` output/save-table emission. It takes
  *already-computed* psi-weights and component series as input — it does not
  build the component ARMA models.
- The actual `PARFRA`/`MAK1` driver is `SPECTRU` (**`spectrum.f:558-1193`**,
  ~635 lines — confirmed live via `oracle/fortran/makefile.gf`, which lists
  `spectrum.o`/`spectrum.f` but NOT `spectrum2.f`; the latter is a dead
  duplicate). `SIGEX` calls the outer `SPECTRUM` (`spectrum.f:82`), which
  calls `SPECTRU` to partial-fraction the model's pseudo-spectrum numerator
  `RT` across the trend/seasonal/cycle harmonic denominators built from
  `F1RST`'s output — exactly the algebra session 1 described, just under the
  wrong subroutine name.

Corrected real call chain (see §1 below, rewritten):
`seats_init_denoms` (new, ported) → `F1RST` (ported) → `SPECTRU`
(`spectrum.f:558`, **not ported**) → `DECFB` (per-component MA numerators,
not ported) → `ModelEst`/`ESTBUR` (Burman WK filter, not ported) →
`AUTOCOMP` (component series, not ported). `SECOND` (`sigsub.f:331`) sits
much later in the pipeline, alongside `SEBARTLETTACF`/`VARIANCES`, feeding
the `.udg` diagnostic keys (`seatdg_cmn`), not the `.mdc` component models.

### What landed this session

1. **`GTSEAT` fully wired** (was previously just a `has_seats` flag via
   `gt_generic`, dropping every value). `core/src/specparse/readers_spec.cpp`'s
   `gt_seats` now parses all 22 arguments faithfully (`gtseat.f`), including
   validation messages, into `ctx.seatop` (the already-generated `/setopt/`
   COMMON struct — it was sitting unused): `qmax2`/`out2`/`maxit2`/`epsph2`/
   `xl2`/`rmod2`/`epsiv2`/`hplan2`/`lnoadm`/`kmean`/`lhp`/`lstsea`/`bias2`/
   `lfinit`/`iphtrf`/`hptrgt`/`lhprmls`, plus `appendfcst`→`ctx.tbllog.savfct`
   and `save`→`ctx.captured.save_tables`. Defaults match `gtinpt.f:533-550`
   (set at the top of `gt_seats` — behaviorally equivalent to the oracle's
   global-parse-start placement since nothing else reads `ctx.seatop` before
   this). `print`/`savelog`/`tabtables` stay consumed-but-deferred (tier F,
   unchanged). **Note**: the oracle's *actual numeric defaults* used
   downstream (`rmod`=0.5, `epsphi`=2.0, `xl`=0.99, `epsiv`=0.001, `maxit`=20,
   `qmax`=50) live in a SEPARATE place — `ansub9.f:1560-1650` (`NMLSTS`'s
   `L_*` local-variable initialization) — not in `gtinpt.f`'s `NOTSET`/
   `DNOTST` sentinels. `ctx.seatop` only holds what the user explicitly
   typed; resolving it against those real defaults is a small follow-on (a
   `L_rmod = (!dpeq(seatop.rmod2, DNOTST)) ? seatop.rmod2 : 0.5;`-style
   function) still needed before `F1RST`/`SPECTRU` can consume real values —
   not yet wired, see next-increment below.
2. **`seats_init_denoms`** (new: `core/src/seats/denoms.cpp` +
   `seatsdenoms.hpp`) — faithful port of `sigex.f:476-540`, the code that
   runs immediately before `F1RST` to seed `chins`/`chis`/`psins`/`psis`/
   `cycns`/`cycs` with the nonstationarity from the model's differencing
   orders `(d, bd)` and an optional near-unit seasonal-AR root (the
   `bphi(mq+1)` heuristic), separate from `F1RST`'s job of classifying the
   *stationary* AR roots on top. 5 new unit tests (`tests/unit/test_seats.cpp`),
   all hand-verified: plain `(1-B)^d` expansion, the seasonal-summation
   `(1+B+...+B^(mq-1))^bd` fold, the near-unit/not-near-unit seasonal-AR
   branch split, and the positive-seasonal-AR-coefficient → `Cycs` branch.
   `test_seats` is now 42 tests (was 37), all passing.
3. **`run_seats.cpp`'s stopping point corrected**: the `seats_not_ported`
   message now names `SPECTRU` (not the old, wrong "SIGEX orchestrator, root
   allocation through WK-filter" framing) and documents the **model-decode
   blocker** found this session (below).
4. **New blocker identified (not resolved this session)**: there is no
   existing utility anywhere in the port to turn `ctx.model`'s internal
   `Mdl`/`Opr`/`Arimal`/`Ap1` ARMA-operator-list representation (the generic
   lag-position encoding `armafl.cpp`/`estimate.cpp` filter against) into
   plain `(p, d, q, bp, bd, bq)` orders and `phi`/`bphi`/`theta`/`btheta`
   coefficient arrays. The oracle's own equivalent (`analts.f:2340-2381`,
   building `phis(i+1) = -Phi(i)` then calling `RPQ`) assumes those plain
   arrays are already sitting in scalar/array locals — which in X-13's case
   come from `NMLSTS` un-parsing them out of `Mdl`/`Opr` (partially, per
   session 1 tier D). **This decode does not exist in the C++ port yet** and
   blocks wiring real root-finding into `run_seats.cpp` — confirmed by
   grepping for `arimamdl` (the `.udg` key that would need the same decode)
   and finding zero hits in `core/src`. This is the necessary sub-step before
   `F1RST`/`seats_init_denoms` can run on an actual corpus spec's fitted
   model rather than synthetic unit-test inputs.

### Why the .mdc / s-table gates did NOT move this session

Being direct about this rather than papering over it: **no s-table or `.mdc`
value changed from unreachable to reachable this session.** `run_seats`
still fatals at the same conceptual point (before the canonical decomposition
runs) — the fatal message just names the correct blocker now. Root cause: the
task's premise ("port SECOND to close the `.mdc` gate") targeted the wrong
routine (see the correction above); the real next routine (`SPECTRU`) needs
(a) the model-decode utility (blocker #4) to even receive real `phi`/`d`/`bd`
inputs, then (b) its own ~635-line port. Both are larger than fit in the time
remaining after the correction + the two increments above. `tests/parity/
test_seats_tables.py` is unchanged: **48 xfailed + 1 passed sanity check**,
same as session 1 — honestly reflects where the port stands.

Also worth recording: **`expgs_seats` is a bad `.mdc`/s-table gate target,
independent of port status.** Its golden `.err` file reads:
```
DECOMPOSITION INVALID,IRREGULAR SPECTRUM NEGATIVE
TRY ANOTHER MODEL OR, FOR AN APPROXIMATION, SET NOADMISS=YES.
```
The **oracle itself** rejects `expgs_seats`'s (2,1,0) model as inadmissible
and emits no `.mdc`/s-tables (`expgs_seats.mdc` doesn't exist in the golden
bundle). A perfect port would reproduce that rejection, not produce numbers
to diff. **`payems_seats`** ((0,1,2), `seasonaldiff=0` — no seasonal AR/MA/
diff at all) is confirmed the better target: its `.mdc` is real and small
(`sanum`/`saden`/`savar` — SA collapses to "everything but irregular" since
there's no seasonal split — plus `trnum`/`trden`/`trvar`/`irrvar`, no
`snum`/`sden` at all), and its `.s10` (seasonal factor) is trivially `1.0`
at every period (no seasonal component ⇒ neutral factor) — genuinely the
easiest s-table to gate first once the pipeline exists, since it requires no
real WK-filter computation, just confirming "no seasonal" propagates through.
Even so, payems's `p=0` (no AR terms — order is (0,1,2)) means **`F1RST` is
a no-op there too** (`p==0` early return); the nontrivial `(1-B)` unit root
comes entirely through `seats_init_denoms`, and reaching `trnum`/`trden`
still needs `SPECTRU`+`DECFB`+`AUTOCOMP`. `unrate_seats` ((0,1,1) default
airline-style model, also seasonaldiff=0 per the corpus) is likely similar
in shape; `airline_seats` is the only one of the 4 base series with any
seasonal structure at all worth checking once `SPECTRU` lands.

### Next concrete increment (revised)

1. **Model-decode utility** (blocker #4 above): a function
   `seats_extract_model(ctx) -> {p,d,q,bp,bd,bq,mq,phi,bphi,theta,btheta}`
   reading `ctx.model`'s `Mdl`/`Opr`/`Arimal`/`Ap1` (see `armafl.cpp` for the
   existing iteration pattern) into plain arrays. Needed before ANYTHING
   below can run on real corpus data rather than synthetic unit-test inputs.
   Gate it against the `.udg` `arimamdl` key (e.g. `(2 1 0)` for expgs) once
   built.
2. **Wire root-finding for real**: build `phis(1)=1, phis(i+1)=-phi(i)`
   (`analts.f:2345-2352`) from the decoded model, call the already-ported
   `rpq`/`c02aef` when `p>0`, then `seats_init_denoms` + `F1RST` — all three
   pieces exist now, just never connected to a real fitted model. Verifiable
   even before `SPECTRU`: `Chi*Psi*Cyc` (via `conv`) should reproduce the
   model's own AR polynomial as an internal consistency check.
3. **Port `SPECTRU`** (`spectrum.f:558-1193`). Read the WHOLE routine before
   starting — the branch structure keys off which of `nchi>1`/`ncyc>1`/
   `npsi>1` (trend/cycle/seasonal denominators nontrivial) are true, with a
   distinct `PARFRA` call sequence per case (7+ branches visible in the
   snippet read this session, likely more). `payems_seats`/`unrate_seats`
   (no seasonal, `npsi==1`) hit the simplest branches — target those first,
   NOT `airline_seats` (which has real seasonal structure) or `expgs_seats`
   (inadmissible in the oracle, see above).
4. Resolve `ctx.seatop`'s raw parsed values against the REAL SEATS defaults
   (`ansub9.f:1560-1650` — see the GTSEAT note above) into the `rmod`/
   `epsphi`/`xl` values `F1RST`/`SPECTRU`/`MAK1` actually consume.
5. Only after `SPECTRU` (+ `DECFB` for per-component numerators) gates
   against `.mdc` does `FCAST`/`ExtendSeries`/`ESTBUR`/`AUTOCOMP` (the s10-
   s18 save tables) become reachable — a separate, larger milestone, per
   session 1's original plan.

### Gate / regression status

Full suite (this session's environment, `& .\tools\build.ps1`): unit tests
**10/10** (test_seats now 42/42, was 37/37 at session start — the other +1
unit test file, `test_api_facade`, is unrelated concurrent work). Parity
suite: `test_seats_tables.py` unchanged at **1 passed, 48 xfailed**. The
full-suite pass/xfail totals moved slightly this session (`433p/9s/85x`
observed vs. the `433p/9s/84x` handoff baseline) **from unrelated concurrent
work** on `test_x11_tables.py`/`automd_finalize`/the new `core/src/api/`
framework (visible in `git status`, not touched by this session) — confirmed
by running `test_seats_tables.py` and the non-seats suite separately; the
seats-specific counts are exactly unchanged. No regression from this
session's changes.

---

# Session 1 content below (superseded call-tree claims corrected above; kept
# for the harness/gate build history and the leaf-port status table, which
# ARE still accurate)

Session: 2026-07-20 (continuation of the M5/seats poly/root/factor sessions —
see `git log --oneline -- core/src/seats/`: `d28f3f8` scouting report,
`8039bac` poly+C02AEF+RPQ, `3f54574` PARFRA+MAK1). This pass: (1) verified/
extended the existing survey, (2) built the parity harness + xfailed gate,
(3) landed the first decomposition-tier increment (F1RST), (4) confirmed no
regressions. Read this file, then `tools/seats_scouting.md` (the original,
more granular per-routine leaf/tier table — still accurate, this file layers
the gate + current status on top rather than duplicating it).

## 0. TL;DR for the next session

- **Ported so far** (`core/src/seats/`): the full pure-numeric leaf tier —
  `CONV/CONJ/MULTFN/DIVFCN` (poly.cpp), `C02AEF/RPQ` (roots.cpp), `PARFRA/MAK1`
  + their private helpers (factor.cpp), and — new this session — `F1RST` +
  `isCloseTD` (f1rst.cpp), the AR-root-to-component allocator. 37 unit tests,
  all bit-exact or hand-verified, `tests/unit/test_seats.cpp`.
- **Gate built this session**: `tools/x13run_seats.cpp` (harness) +
  `tests/parity/test_seats_tables.py` (48 xfailed cases + 1 real sanity
  check that's green). `x13::run_seats` (new, `core/src/driver/run_seats.cpp`)
  parses the spec, estimates the regARIMA model (reuses `run_m2_after_parse`,
  same phase `run_x11`'s model path uses), confirms `seats{}` was requested,
  then fatals cleanly — the decomposition driver (`SECOND`/`SIGEX`) isn't
  wired yet. All 8 SEATS corpus specs reach that fatal point without crashing.
- **Worst-error on airline_seats s-tables: N/A — unreachable.** The harness
  never emits a table row for any spec today (see gate section below); there
  is no partial output to diff. This is expected and matches where the
  project actually is: `SECOND` (the canonical-decomposition driver) is the
  very next piece and is not yet ported. **[SESSION 2 CORRECTION: "SECOND" was
  the wrong name — see the top of this file. The real next piece is SPECTRU.]**
- **Next concrete increment**: port `SECOND` (`sigsub.f:331`, ~600 lines,
  much of it inline print) — the driver that calls `F1RST` (done) twice
  (nonseasonal + seasonal AR roots), then `PARFRA`+`MAK1` (done) to emit the
  canonical component ARMA models. That closes the `.mdc` gate (Step 1 in
  `seats_scouting.md` §4) — reachable *before* any Burman signal extraction.
  See §5 below for the concrete plan. **[SESSION 2 CORRECTION: this whole
  bullet is wrong, see the top of this file for the corrected plan.]**
- **Full suite**: `433 passed, 9 skipped, 84 xfailed` (was `432/9/36`; delta
  is exactly `+1 passed` from the new sanity test and `+48 xfailed` from the
  8 specs × 6 s-table tags), 8/8 unit (was 7/7 — new `test_seats` additions,
  same executable). No regressions.

## 1. Entry point & call tree

(Full detail in `tools/seats_scouting.md` §1-2; summarized + updated here.)

```
x11ari.f:168        ELSE IF(Lseats) THEN ... CALL seats(...)
analts.f:43         SUBROUTINE SEATS(...)                — 3444-line driver
  |
  |-- NMLSTS (ansub9.f:694)      read fitted model into SEATS locals [partial: our
  |                               port supplies it from ctx directly, no namelist]
  |-- (bypassed in X-13: model re-estimation/auto-ID subtree, tier E)
  |
  |-- RPQ -> C02AEF (ansub2.f)   root-find the full AR and MA polynomials  [PORTED]
  |     (called from analts.f:2340-2381, NOT from sigex.f -- rez/imz/p/etc.
  |      arrive at SIGEX as formal arguments already computed)
  |
  |-- SIGEX (sigex.f:51)         orchestrator, ~4700 lines incl. heavy print
  |     |
  |     |-- seats_init_denoms (sigex.f:476-540) nonstationary denom setup
  |     |                       from differencing orders d/bd + near-unit
  |     |                       seasonal-AR root                          [PORTED session 2]
  |     |-- F1RST (sigex.f:553, sigsub.f:29) allocate STATIONARY AR roots [PORTED]
  |     |-- CONV combines (sigex.f:583-623): Chi=Chis*Chins, Psi=Psis*Psins,
  |     |                  Cyc=Cycs*Cycns, Totden=Chi*Psi*Cyc            [not ported --
  |     |                                                                 trivial, CONV exists]
  |     |-- SPECTRUM -> SPECTRU (spectrum.f:82, spectrum.f:558-1193)
  |     |     the REAL canonical-decomposition driver: partial-fractions the
  |     |     model's pseudo-spectrum numerator RT across Chi/Psi/Cyc via
  |     |     PARFRA (ported), branch-selected by which of nchi/npsi/ncyc>1 [NOT PORTED]
  |     |-- CHECKADM/CHKSPCT (ansub7.f)   admissibility (spectrum >= 0)    [NOT PORTED]
  |     |-- APPROXIMATE (ansub5.f:2011)   re-fit if inadmissible           [NOT PORTED]
  |     |-- FCAST (ansub1.f:2066) + ExtendSeries (ansub11.f:806)
  |     |                          forecast/backcast-extend the series     [NOT PORTED]
  |     |-- DECFB (ansub3.f)      per-component MA numerator (uses MAK1,
  |     |                         ported, plus MPBBJ)                     [NOT PORTED]
  |     |-- ModelEst / ESTBUR (ansub3.f:54)  Burman's algorithm - WK core  [NOT PORTED]
  |     |-- AUTOCOMP (ansub4.f:33) assemble trend/sa/sc/cycle/ir series    [NOT PORTED]
  |     |-- Afilter/FinitoFilter (ansub11.f) finite WK end-filters+phase   [NOT PORTED]
  |     |-- VARIANCES/SERROR*/SEBARTLETT* component error variances       [NOT PORTED]
  |     |-- BIASCORR/ABIASC       log-case bias correction                [NOT PORTED]
  |     |-- DETCOMP/TAKEDETTRAMO  deterministic-effect (TD/Easter/outlier)
  |     |                          allocation to components                [NOT PORTED]
  |     `-- HPPARAM/HPTRCOMP      optional HP trend/cycle (hpcycle)        [NOT PORTED]
  |
  |-- SECOND (sigsub.f:331)  -- POST-decomposition diagnostics: estimation-
  |     error ACFs, revision variances, SEASIGN seasonal-significance test,
  |     output emission. Feeds .udg diagnostic keys, NOT .mdc.             [NOT PORTED]
  |
  `-- print/save half: seatpr/seatdg/seatfc/seatad, OUTTABLE*, htmlout,
                        Tpeaks/Spectrum plots                              [DEFERRED — tier F,
                                                                             like fcstout]
```

`GTSEAT` (`oracle/fortran/gtseat.f`, spec-option parser for `seats{}`) is now
**fully wired** (session 2): `core/src/specparse/readers_spec.cpp`'s
`gt_seats` parses all 22 arguments into `ctx.seatop`. See the session-2
section above for what's still missing (resolving those raw values against
the oracle's real numeric defaults, which live in `ansub9.f`, not `gtinpt.f`).

## 2. Ported vs. missing (current, verified against `core/src/seats/`)

| Tier | Routine(s) | Oracle location | Status |
|------|-----------|------------------|--------|
| A | CONV/CONJ/MULTFN/DIVFCN | ansub2.f:341+ | **PORTED** (poly.cpp) |
| A | C02AEF/C02AEZ | ansub2.f:2900 | **PORTED** (roots.cpp) |
| A | ROOTC/SQROOTC/MPBC/SYMPOLY/MLTSOL | ansub2.f | **PORTED** (factor.cpp) |
| A | grRoots/getRoot/closestRoot/halfRoots | ansub2.f:1896+ | **PORTED** (factor.cpp) |
| B | RPQ | ansub2.f:16 | **PORTED** (roots.cpp) |
| B | PARFRA | ansub2.f:1495 | **PORTED** (factor.cpp) |
| B | MAK1 | ansub2.f:1615 | **PORTED** (factor.cpp) |
| B | F1RST | sigsub.f:29 | **PORTED** (f1rst.cpp) |
| B | isCloseTD | sigsub.f:257 | **PORTED** (f1rst.cpp) |
| B | **seats_init_denoms** (inline in SIGEX, no oracle name) | sigex.f:476-540 | **PORTED this session** (denoms.cpp) |
| D | **GTSEAT** | gtseat.f:1 | **PORTED this session** (readers_spec.cpp `gt_seats`) |
| B | DPSI/CHBJB/BFAC/MPB/MPBF/MPBBJ/INPOL | ansub3.f | not ported |
| B | getPSIE/SeparaBF/DECFB | ansub3.f | not ported |
| B | CHECKADM/CHKSPCT | ansub7.f:139,292 | not ported |
| B | HPPARAM/HPTRCOMP/CONVC | ansub10.f | not ported |
| B | Afilter/FinitoFilter/GetPhase/smoothH | ansub11.f | not ported |
| B | GETTHVARIANCE/PINNOV | ansub4/5.f | not ported |
| C | **SPECTRU (the real PARFRA/MAK1 driver)** | **spectrum.f:558-1193** | **PORTED sessions 4-5** (`spectru.cpp`); `qt1` matches oracle `irrvar` bit-exact on 7/7 available corpus specs (session 5 fixed the `thstar` sign bug, see session-5 §above) |
| C | MINIM/MINIMbis/GlobalMinim/MinimGrid/FUNC0/Fbis (SPECTRU's 1-D spectrum-minimization stack) | ansub2.f:595-1472 | **PORTED session 4** (`spectru.cpp`); verified correct in isolation (hand-solvable cos(x) case) and now indirectly bit-exact-confirmed via the qt1 gate |
| C | ADDJ (weighted polynomial add, the qstar>pstar correction) | spectrum.f:3138 | **PORTED session 4** (`spectru.cpp`); bit-exact-confirmed via the payems_seats qt1 gate (the only spec exercising this path) |
| C | SPECTRUM (outer wrapper around SPECTRU) | spectrum.f:82-557 | not ported -- read in full session 5, no `Thstr0`/`Chi` normalization found before the `SPECTRU` call (see session-5 §above); not needed for `qt1`, may still be needed for `DecompSpectrum`'s admissibility retry path |
| C | SECOND (post-decomposition ACF/variance diagnostics -- NOT the decomposition driver) | sigsub.f:331 | not ported (lower priority than SPECTRU) |
| C | DecompSpectrum (Ut/V/Uc padding + utf/vf/ucf) + MAspectrum (MAK1 factorization into THETP/THETS/THETC/THADJ -- the actual .mdc sanum/saden/savar/snum/sden/svar/trnum/trden/trvar source) | spectrum.f:1380-1629 (DecompSpectrum's ported half), spectrum.f:2710-2890 (MAspectrum) | **PORTED session 6** (`decompspectrum.cpp`); bit-exact on 7/7 available corpus specs. DecompSpectrum's ct/cs/cc filter-numerator half (spectrum.f:1468-1533 etc) NOT ported -- unneeded for .mdc, needed later for ESTBUR |
| C | FCAST (series forecast/backcast extension) | ansub1.f:2066 | not ported -- scouted session 6, next real milestone |
| C | ESTBUR (Burman WK-filter linear-system solve -> trend/sc/cycle/sa/ir SERIES) | ansub3.f:54 | not ported -- scouted session 6 (~300+ lines, needs ct/cs/cc from DecompSpectrum's unported half) |
| C | DECFB (revision-error covariances + filter weights via CHBJB/SeparaBF/getPSIE/BFAC) | ansub3.f:714-793 | not ported -- fully read session 6; confirmed its inputs need the B-J-sign transform (CHBJB, ansub3.f:803-825, textually confirms the session-5 sign-convention finding) |
| C | CHBJB (true-sign -> B-J-sign polynomial negate, b(0)=1;b(i)=-a(i)) | ansub3.f:803-825 | not ported -- trivial once needed (DECFB increment) |
| C | AUTOCOMP (final component series assembly) | ansub4.f:33 | not ported -- signature only, session 6 |
| C | APPROXIMATE/KnownApprox | ansub5.f:2011 | not ported |
| C | FCAST | ansub1.f:2066 | not ported |
| C | ExtendSeries/extendHP | ansub11.f:806 | not ported |
| C | ESTBUR | ansub3.f:54 | not ported |
| C | AUTOCOMP | ansub4.f:33 | not ported |
| C | SIGEX (the outer orchestrator) | sigex.f:51 | not ported (depends on all of the above) |
| C | VARIANCES/SERROR*/SEBARTLETT*/BIASCORR/DETCOMP | ansub4/5/9.f | not ported |
| D | NMLSTS | ansub9.f:694 | N/A — ctx supplies the fitted model directly (see §1) |
| D | **model-decode (nmlmdl.f: Mdl/Opr -> plain p/d/q/phi/bphi/theta/btheta)** | `ansub9.f:1039` (`nmlmdl.f`) | **PORTED session 3** (`model_decode.cpp`); gated bit-exact (order string) against `.udg` `arimamdl` on all 8 corpus specs |
| B | **canonical-denoms wiring (RPQ -> seats_init_denoms -> F1RST -> full chi/psi/cyc/thstar on a decoded model)** | inline in `SIGEX`, `sigex.f:447-655` (`phis`/`bphis`/`Chi`/`Psi`/`Cyc`/`Thstr0`/`pstar` build) | **PORTED session 3-4** (`canonical_denoms.cpp`); wired into `x13run_seats.cpp` (prints `DECODE_QT1`), not into `run_seats.cpp`'s driver (qt1 not yet trustworthy, no real consumer) |
| D | **seatop defaults resolve** (`ctx.seatop` raw parse -> real SEATS numeric defaults) | `ansub9.f:1560-1650` (`SETDEFAULT`) | **PORTED session 4** (`seatopts.cpp`) |
| D | ss2rv/rv2ss/savmdc/initst | various | not ported |
| E | estimation subtree (SEARCH/AMI/CHMODEL/...) | ansub1/2/5.f | bypassed in X-13 (fixed model), defer |
| F | print/IO/diagnostics (~33 routines) | many | defer (like fcstout) |

## 3. The gate (harness + test)

**`core/src/driver/run_seats.cpp`** — `x13::run_seats(ctx, spec_text, base)`:
parses the spec, requires `has_series && has_seats`, then runs
`run_m2_after_parse(ctx, base, /*estimate=*/true, ...)` — the same shared
pre-model + regARIMA-estimate + forecast phase `run_x11`'s model path uses.
SEATS has no no-model path in the oracle (analts.f's SEATS subroutine always
consumes a fitted model via NMLSTS — see §1), so this is not optional the way
it is for X-11. Once estimation succeeds, `run_seats` calls a local
`seats_not_ported(ctx, ...)` (mirrors `x11parts.cpp`'s `x11_not_ported`
helper: `errhdr`+`writln`+`abend`) naming the corrected blocker (session 2)
and returns false. Declared in `core/src/specparse/specparse.hpp` next to
`run_x11`.

**`tools/x13run_seats.cpp`** — thin CLI harness, byte-for-byte modeled on
`tools/x13run_x11.cpp`: opens the spec, `chdir`s to its directory, calls
`run_seats`, prints `OUTCOME: OK`/`OUTCOME: FATAL`. No table dumps yet — the
s10-s18 component series have no home on `ctx` (there is no generated
`seatsr`-style COMMON with them -- wait, actually `seatcm_cmn` / `ctx.seatcm`
DOES have `Seattr`/`Seatsf`/`Seatir`/`Seatsa`/`Seatcy` fields already
generated and unused -- confirmed session 2 -- so the home exists, it's just
nothing writes to it yet). Once `SPECTRU`/`AUTOCOMP` land, add
`dump("s10", ...)` calls mirroring `x13run_x11.cpp`'s `dump("d10", ...)`
pattern reading `ctx.seatcm.seatsf`/`.seattr`/etc.

**`tests/parity/test_seats_tables.py`** — same shape as `test_x11_tables.py`
(subprocess the harness, parse `<tag> YYYYMM <value>` lines, diff against
golden `.s10`/`.s11`/`.s12`/`.s13`/`.s16`/`.s18` files at `rtol=1e-8`),
parametrized over the 8 SEATS corpus specs × 6 table tags = 48 cases, **all
`pytest.xfail`ed**. A 49th test, `test_seats_harness_runs_cleanly`, is
**not** xfailed — it asserts the harness produces a well-formed `OUTCOME:`
line (OK or FATAL, not a crash) for every corpus spec. Confirmed still
passing session 2 (unaffected by the GTSEAT/denoms changes).

## 4. Corpus/goldens

8 specs, `tests/corpus/generated/{airline,expgs,payems,unrate}_{seats,
fixed-airline-seats}.spc`. Goldens under
`tests/golden/generated/<base>/<base>.{s10,s11,s12,s13,s16,s18,mdc,udg,...}`.
**Session 2 finding: not all 8 are equally good gate targets** -- see the
"Why the .mdc / s-table gates did NOT move this session" section above for
the `expgs_seats` inadmissibility discovery and the `payems_seats`
recommendation. `census-examples/04-seats.spc` (same model given explicitly
via `arima{model=(0 1 1)(0 1 1)}`, skipping the automdl preamble) is
available too but not part of the 8-spec gate the task specified.

## 5. Build/run reminders

- Build via `& .\tools\build.ps1` from PowerShell (not the Bash tool, not
  `powershell -File`). As of session 2, `build.ps1` auto-resolves a working
  CMake >= 3.16 (rtools44 ships cmake 3.30.4 at
  `C:\rtools44\x86_64-w64-mingw32.static.posix\bin\cmake.exe`, which
  `build.ps1` now probes and prefers) -- the session-1 note about a broken
  pinned `C:\Program Files\CMake` (3.14) is resolved, no workaround needed.
  **New-file gotcha (session 2, still open)**: adding a new `.cpp`/target
  makes the next `--build` fail with `-- GLOB mismatch!` even though
  `CMakeCache.txt` already exists (the CONFIGURE_DEPENDS re-glob check
  errors instead of silently reconfiguring under this generator/cmake
  combo) -- run an explicit `cmake -S . -B build ...` reconfigure (same
  flags `build.ps1` uses) once after adding files, then `& .\tools\build.ps1`
  works normally again.
- `core/src/seats/*.cpp` auto-globs into `x13core` -- no CMakeLists edit
  needed for new `.cpp` there (only new top-level *executables* need a
  CMakeLists edit + the reconfigure above).
- `tests/unit/test_seats.cpp` links `x13core`, needs `core/include` +
  `core/src` on the include path (already wired in
  `tests/unit/CMakeLists.txt`).
- Run the harness from PowerShell (`build\x13run_seats.exe <spec>`), pytest
  via `python -m pytest tests/parity/test_seats_tables.py -q`. `python`, not
  `python3`.
