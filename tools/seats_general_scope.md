# General-shape SEATS (p>0 / bp>0 / imean!=0) — port scope

**STATUS: p>0 AND bp>0 CLOSED (bit-exact + gated). *_ar2-seats ((2 1 0)(0 1 1),
p>0) gate s10-s18 ~5e-15 on airline/payems/unrate; *_sar-seats ((0 1 1)(1 1 0),
bp>0) gate on all 4 series. Both bugs were AR-polynomial signs (phis=+mo.phi,
bphis=+mo.bphi). Only imean!=0 remains open (now FATALS cleanly via
run_seats.cpp seats_has_mean, no longer silent-wrong).**

## imean!=0 runway (the last general-shape gap)
Probed this session (scratchpad/seatprobe/im1,im2). A Constant (PRGTCN) regressor
sets Imean=1. Measured error with the current za=0 hardcode: im1 (airline log,
`regression{const}` + (0 1 1)(0 1 1), d=1 drift) ~16% on s11/s12; im2 (unrate,
const + (2 0 0)(0 1 1), d=0) ~1580% on s10 -- the mean genuinely drives the
decomposition, not accidentally-close. Now guarded (seats_has_mean -> fatal).

To port (three coupled pieces):
1. **Center the differenced series** by wm before decomposing. analts.f:1955-1979:
   wm = mean of Wd (regular case: sum(Wd)/Nw; if d==0: sum over nn=Nw-mod(Nw,mq)
   points / nn), then when Imean!=0 subtract wm from every Wd(j). SEATS operates
   on the CENTERED Wd; wm is added back downstream.
2. **za in FCAST** (fcast_extend, ansub1.f:2166-2181): za = wm*(1 - sum phist(j))
   when Pstar>0 else wm; the extension recursion seeds `sz = za - ...` (currently
   sz=0). wm is the differenced-series mean from step 1.
3. **wmf/wmb (zaf/zab) in ESTBUR's general branch** (estbur.cpp, ansub3.f): the
   AM2 rows 1..iqrow use AM2(i,irow+1)=wmf / (i,irow+2)=wmb (currently 0). These
   are the forward/backward mean contributions.
   Plus: **d=0 drops the trend unit root** (chi becomes [1] not (1-B)^d) -- im2's
   giant error is mostly this structural change, a separate sub-item from the mean
   itself. Do the d>=1 drift case (im1) first; it isolates the pure za/wm path.
Sourcing wm in C++: the fitted Constant coefficient is a PRGTCN regressor in
ctx.model; but the oracle's wm is the DIFFERENCED-SERIES mean (step 1), not the
raw coefficient -- compute it from Wd, don't read the coefficient.

## What actually closed it (the real bug was ONE sign)
The estbur general branch, the ct/cs/cc/MLTSOL solve, and the FCAST extension
were ALREADY correct for p>0. The dominant ~10% error traced to a single sign in
`canonical_denoms.cpp`: `phis[i+1] = -mo.phi[i]` should be `+mo.phi[i]` (mo.phi is
already the true-sign AR polynomial coefficient, mirroring the MA-side session-5
correction `ths=+Th`). Verified on the (2 1 0)(0 1 1) probe: with -mo.phi, cyc =
[1,-0.3616,-0.0637] / Totden = [1,-1.3616,...]; with +mo.phi both reproduce the
oracle's transitory AR [1,+0.3616,+0.0637] / Totden [1,-0.6384,-0.2979,-0.0637,...].
Then the CALCFX Pstar>0 branch (calcfx_last_residuals: stationary AR filter u =
(1 - sum Phist B^j) Wd, np = Nw-Pstar, ansub1.f:5006-5007) closed payems_ar2's
near-non-invertible seasonal MA (~0.985), where the armafl seed fallback diverged
~1.6e-5 at the tail. sub-gaps #1/#2/#3 below are all DONE. #4 (imean) still open.

---
### ORIGINAL PLAN (historical; superseded by the note above)

## Goal
Gate SEATS s10-s18 bit-exact for models beyond the airline family. Every current
SEATS corpus spec is `(0 1 1)(0 1 1)`, imean=0 (p=bp=0). The general branch
(AR present, or a mean) produces WRONG values today, silently — it does NOT fatal.

## Concrete target (admissible, oracle ships golden)
`(2 1 0)(0 1 1)` on airline (log): the oracle produces s10-s14 (only spectral-peak
WARNINGs, not fatal). AR estimates: Lag1=-0.3616, Lag2=-0.0637. Other admissible
probes: `(1 1 1)(0 1 1)`, `(1 1 0)(0 1 1)`. Inadmissible: `(2 1 1)(0 1 1)` (0 tables).
Scratch probes live in the session scratchpad (not corpus — keep the 0-xfail
invariant until this gates green).

## The gap (measured, this session)
After the build_bphist phist fix, s12 worst rel err vs oracle is **~1.0e-1** (10%),
sample 194901 mine 128.51 vs gold 123.81. So the forecast-extension seed is a
SMALL part; the bulk error is in the historical decomposition itself for AR models.

## Three coupled sub-gaps (in rough dependency order)

1. **build_bphist phist fill — DONE (this session).** `bphist(i+1) = -phist(i)`
   where phist = the full AR polynomial (1 - sum phi B^i)(1 - sum bphi B^{k*mq}),
   ansub1.f:2116-2121. mo.phi/mo.bphi are already in the `arp(B) = 1 + sum(mo.phi)
   B^i` convention (verified: pure-AR(2) arp=[1, phi0, phi1] == the oracle's
   -phist; mo.phi=[+0.3616,+0.0637] = -Lag_reported). Strict no-op for p=bp=0.
   Seasonal (bp>0) sign S=1+mo.bphi B^mq assumed by analogy — UNVERIFIED (no bp>0
   probe yet).

2. **CALCFX general-branch residual seed (Pstar>0) — OPEN, the hard core.**
   `calcfx_last_residuals` (estbur.cpp:92) bails `if (mo.p||mo.bp) return false`
   and the caller falls back to armafl_last_residuals, which is NOT exact for AR.
   The Pstar==0 CALCFX port (the Morf-Sidhu-Kailath constrained-LS residuals, the
   sessions 9-14 saga) must be generalized to Pstar>0: the `u` recursion gains the
   AR terms, np=Nw-Pstar (not Nw), n=Nw+Qstar changes, and the STEP-3B..8 solve
   folds phist. Ground truth: ansub1.f:964-1471 (CALCFX), the Pstar>0 branches.

3. **ESTBUR historical AR solve — OPEN, likely the dominant 10% error.** The
   trend/cycle Wiener-Kalman recursion (estbur.cpp:393+, ansub3.f) uses cd.pstar/
   cd.qstar. For AR models the canonical denominators carry the AR roots in the
   trend/transitory components ("STATIONARY AUTOREGRESSIVE TRANSITORY COMPONENT"
   in the oracle .out). Verify seats_canonical_denoms + decomp_spectrum produce
   the right component AR polynomials for AR models, then the gt/gs/gc + MLTSOL
   solve + the trend(i)=fxt+bxt recursion with those denominators. This is where
   the bulk error lives and is the biggest chunk.

4. **imean / za — OPEN (smaller).** fcast_extend hardcodes za=0 (estbur.cpp:377);
   ansub1.f:2166-2181: za = (1 - sum phist)*wm when Pstar>0 else wm, when imean!=0.
   Needs a non-differenced-mean target series (airline log d=1 kills the mean, so
   this needs a different model/series). Do LAST.

## Sequencing
Do #3 (historical AR solve) first with the oracle-diff discipline (instrument the
C++ trend/cycle vs a fresh oracle dump at a few interior points, same technique as
sessions 12-14). #2 (CALCFX seed) refines the boundaries once #3 is close. #1 done.
#4 last. Then add `(2 1 0)(0 1 1)` to the corpus (genspecs), bless, gate.

## Reuse / refs
- oracle: analts.f (FCAST callers 2778/2843, Phist setup 2088-2106), ansub1.f
  (bphist 2113-2148, FCAST recursion 2183-2201, za 2166-2181, CALCFX 964-1471),
  ansub3.f (ESTBUR historical solve).
- C++: estbur.cpp (build_bphist, fcast_extend, calcfx_last_residuals,
  estbur_historical), canonical_denoms.cpp, decomp_spectrum.cpp, model_decode.cpp
  (mo.phi/bphi population, line 149 raw = -arimap then trans0/trans2).
- estbur.hpp scope block documents the p=bp=0 limits that this lifts.
