# Census X-13ARIMA-SEATS — bug & wart ledger

Defects and dubious constructs found in the vendored Census Fortran (v1.1 b61)
**while porting**. The C++ port reproduces every one of these *faithfully* — bit
parity with the oracle is the contract, so bugs are ported as-is, not fixed.
This ledger exists so the behavior can be **modernized deliberately later**
(behind a flag / in a v2), with each entry pinned to a test so a fix is
verifiable.

Legend — **Severity**: `latent` (wrong in theory, never triggered by real X-13
inputs) · `benign` (observable but harmless / dead code) · `active` (can affect
real results). **Port**: where the behavior is reproduced in C++ and the test
that pins it.

---

## CB-1 — rpoly coefficient-scaling collapse (`max|coeff| >= 10` → no roots)

- **Where:** `oracle/fortran/rpoly.f:63-66` (constants) + `:155-172` (scaling),
  interacting with `dpeq.f` (absolute threshold `DELTA = 3.834e-20`).
- **Severity:** `latent` (for X-13's own AR/MA polynomials) — but a genuine
  correctness bug for general polynomials.
- **Symptom:** `rpoly` returns `Fail=.true.` with `Degree` reset to 0 (finds
  ZERO roots) for any real polynomial whose largest coefficient magnitude is
  `>= 10`, regardless of where the roots lie. E.g. `(x-1)(x-2)(x-3)` (max|c|=11)
  fails; `20x^3-12x^2-0.2x+0.6` with roots {0.5,0.3,-0.2} *inside* the unit
  circle also fails (max|c|=20).
- **Root cause:** the routine hardcodes single-precision-era machine constants
  into a double routine — `Eta=0.5*base**(1-15)≈5e-15`, `smalno=1e-38` — so
  `lo = smalno/Eta ≈ 2e-24`. The scaling block is skipped only when `xmax < 10`
  (`rpoly.f:158`). Once any coefficient magnitude reaches 10, `sc = lo/xmin` is
  ~2e-24; `dpeq(sc,0)` is TRUE (its threshold is an *absolute* 3.834e-20), so
  `sc` is reset to `smalno=1e-38`, giving `l=-37`, `factor=1e-37`. The whole
  polynomial is multiplied by ~1e-37; thereafter every downstream `dpeq(.,0)`
  test sees "zero" (`newest.f:43` returns `Uu=Vv=0`, `quadit.f:99` aborts,
  `fxshfr.f:53` never moves the convergence sequences), so all 20 shifts exhaust
  and it reports failure.
- **Why X-13 is unaffected:** the AR/MA operator polynomials `theta(B)`/`phi(B)`
  have constant term 1 and estimated coefficients well below 10, so `xmax < 10`
  and the broken branch is never entered. `roots.f` also degrades gracefully
  (prints a warning, leaves `Allinv` unchanged) if `rpoly` ever did fail.
- **Port:** `core/src/numeric/rpoly.cpp` — reproduced exactly (constants + the
  `dpow_ri` faithful `base**int`, and `dpeq`); a comment at the scaling block
  flags it. Pinned by `tests/unit/test_numeric.cpp` "rpoly: deterministic
  failure when max|coeff| >= 10 (Census bug)".
- **Modernize:** replace the hardcoded constants with IEEE-double values
  (`Eta=dpmpar(1)=2.22e-16`, `smalno=dpmpar(2)`, `infin=dpmpar(3)` — the source
  even has these commented out at `rpoly.f:73-75`), or normalize the polynomial
  by its leading coefficient before the scaling test. Either removes the
  `max|coeff|>=10` failure while leaving well-scaled inputs unchanged.

## CB-2 — stpitr dead store (`oldobj = 0` immediately overwritten)

- **Where:** `oracle/fortran/stpitr.f` — the ELSE (first-iteration) branch sets
  `oldobj = ZERO`, then the routine's unconditional tail sets `oldobj = Objfcn`
  before returning.
- **Severity:** `benign` (dead store — no observable effect).
- **Symptom:** none; the `oldobj = ZERO` assignment can never be read.
- **Port:** `core/src/regarima/estimate.cpp` (stpitr) — reproduced faithfully;
  `ctx.saved.stpitr_oldobj`. Pinned by the stpitr multi-call test.
- **Modernize:** drop the dead assignment. Zero risk.

## CB-3 — roots.f uses a typo'd 2pi when reporting root frequency

- **Where:** `oracle/fortran/roots.f:83` — `Zerof(i) = datan2(Zeroi,Zeror) /
  6.28318730707959D0`.
- **Severity:** `benign` (output-only; the frequency feeds root-summary tables,
  not the estimation math).
- **Symptom:** the reported root frequency is wrong in ~its 7th significant
  digit. The divisor `6.28318730707959` is a digit-transposition typo of 2*pi
  (true value `6.283185307179586`); relative error ~3.18e-7. Example: a purely
  imaginary root pair `+-2i` should report frequency `0.25`, but the oracle
  yields `0.24999992042653249`.
- **Port:** `core/src/regarima/estimate.cpp` (roots) -- reproduced as
  `constexpr double CENSUS_TWOPI = 6.28318730707959;` with a comment. Pinned by
  `tests/unit/test_numeric.cpp` "roots: MA(2) invertible, ..." (the complex-pair
  case asserts the `0.24999992...` value).
- **Modernize:** use the correct 2*pi (`std::atan2` result divided by
  `6.283185307179586` or `2.0 * M_PI`). Pure output fix; no estimation impact.

## CB-4 — setmdl `lagind` array undersized (PORDER, indexed over PARIMA)

- **Where:** `oracle/fortran/setmdl.f` — `DIMENSION lagind(PORDER)` (PORDER=36),
  written as `lagind(ilag)` where `ilag` ranges over the lag space that
  `arimap`/`arimal`/`arimaf` use (dimension PARIMA=133).
- **Severity:** `latent` (out-of-bounds for models whose lag indices exceed 36).
- **Symptom:** for a model where a free operator's `ilag` exceeds PORDER, the
  non-first-call shrinkage bookkeeping `lagind(ilag)=Nestpm` writes past the end
  of `lagind` (Fortran silently clobbers adjacent storage). Never triggered by
  ordinary seasonal ARMA models (lag indices stay small).
- **Port:** `core/src/regarima/estimate.cpp` (setmdl) — sized `lagind[PARIMA]`
  to avoid the C++ UB while preserving behavior for in-range models. Pinned
  indirectly by the setmdl shrinkage test (ilag=1).
- **Modernize:** size `lagind` to PARIMA in the Fortran too (harmless, matches
  the index domain).

## CB-5 — setmdl doc comment is stale ("differences the X:y matrix")

- **Where:** `oracle/fortran/setmdl.f:7-9` header comment.
- **Severity:** `benign` (comment only; no code effect).
- **Symptom:** the comment says setmdl "differences the X:y matrix and changes
  the model to remove the differencing", but this version does no such thing --
  it only packs `estprm` and root-checks the starting values. Differencing of
  Xy lives elsewhere (regvar/rgcpnt). Misleading to a reader.
- **Port:** `core/src/regarima/estimate.cpp` (setmdl) -- the port's doc comment
  states what the code actually does and flags the stale original.
- **Modernize:** fix the Fortran comment.

## CB-6 — armats `itv` counts fixed ARMA lags but Armacm is packed by free params

- **Where:** `oracle/fortran/armats.f` — the `itv=itv+1` /
  `tval(itv)=Arimap(ilag)/sqrt(Var*Armacm(itv,itv))` loop over AR..MA lags.
- **Severity:** `latent`→`active` — wrong whenever a model **fixes** an ARMA
  coefficient (a supported X-13 feature; the B-section corpus includes fixed-coef
  models), harmless when every ARMA lag is free.
- **Symptom:** `itv` advances on **every** lag in the AR..MA operators, fixed or
  free, but `Armacm` (the ARMA parameter covariance from `covar`) is packed by
  the **free** parameters only (`Nestpm` of them, in free-lag order). So for a
  model with a fixed ARMA lag, `Armacm(itv,itv)` reads the wrong diagonal (and,
  once `itv` exceeds `Nestpm`, past the filled block), giving a bogus t-stat for
  every ARMA parameter at or after the fixed one. `sqrt` of a garbage/negative
  value can even NaN. Contrast `setmdl`/`upespm`/`armacr`, which all correctly
  skip fixed lags (`.not.arimaf`) when indexing the free-param space.
- **Port:** `core/src/regarima/estimate.cpp` (armats) — reproduced verbatim with
  a code comment. Pinned by the all-free ARMA(1,1) case in the "rgarma:
  ...no-regression" test (where itv==free index, so the bug is dormant and the
  t-stats match the oracle to 1e-12).
- **Modernize:** gate the `itv` increment on `.not.Arimaf(ilag)` (and skip
  emitting a t-stat for fixed lags), matching `armacr`'s correct indexing.

## CB-7 — endsf `Savg(j1)/Sumwt` divides the seasonal-MA centre point twice

- **Where:** `oracle/fortran/endsf.f` — the two unconditional
  `Savg(j1)=Savg(j1)/Sumwt` / `Savg(j2)=Savg(j2)/Sumwt` normalizations in the
  seasonal-MA end-weight application (the `jk<=K` branch).
- **Severity:** `latent` — `j1` and `j2` are a symmetric pair of end positions;
  for odd `K` at the centre span they collapse to `j1==j2`, so the single centre
  element is normalized by `Sumwt` **twice** (i.e. divided by `Sumwt²`).
- **Symptom:** the centre seasonal-MA end value is under-weighted by a factor of
  `Sumwt` whenever the odd-K centre case is hit. Does not trigger for the common
  3x3/3x5/3x9/3x15 filters at the params exercised so far, but is a real
  off-by-a-division at that boundary.
- **Port:** `core/src/x11/x11filt.cpp` (endsf) — reproduced verbatim with a code
  comment. Not yet pinned by a failing case (the X-11 leaf tests use params where
  `j1!=j2`); revisit when the D10 seasonal end-filter gate lands.
- **Modernize:** guard the second division with `IF(j2.ne.j1)`.

## CB-8 — sdxtrm tau-MAD scale divides by the stale DO-loop index, not the count

- **Where:** `oracle/fortran/sdxtrm.f:75` — `sdxtrm=sqrt(sdxtrm*sdxtrm*stau/n)`
  in the `Imad>=3` (rho2 tau-adjusted MAD) branch.
- **Severity:** `latent` — only reached for the tau-adjusted MAD scale options
  (`Imad` 3/4, the robust `calendarsigma`/heteroskedastic variants); the default
  extreme-value path is `Imad==0` (RMS), so real X-13 default runs never hit it.
- **Symptom:** the tau scale estimate `s^2 * (Σ rho2(r_i)) / N` must divide by the
  **observation count** `N` (the routine's own `ixn`/`int(xn)`). Instead it divides
  by `n`, the `DO n=L,M,Nsp` loop variable, whose value **after** the range loop is
  the Fortran terminal `L + ceil((M-L+1)/Nsp)*Nsp` — i.e. the first index past `M`,
  not a count. For a stride `Nsp>1` (per-month grouping) `n ≈ M` while the true
  count is `≈ (M-L)/Nsp`, so the scale is understated by roughly `Nsp`; even for
  `Nsp==1` it is off by the omitted-extreme count plus one. The result is a
  too-small sigma → over-flagging of extremes.
- **Port:** `core/src/x11/x11xtrm.cpp` (sdxtrm) — reproduced verbatim (`sdx =
  std::sqrt(sdx * sdx * stau / n)`, where `n` holds the C++ for-loop's post-exit
  value, identical to the Fortran terminal index) with a code comment. Not yet
  pinned by a failing case (the x11xtrm leaf tests exercise `Imad` 0/1, where the
  branch is dormant); revisit when a tau-MAD `calendarsigma` gate lands.
- **Modernize:** divide by `ixn` (or `xn`), the actual number of accumulated
  deviations, matching the intent of the tau estimator.

## CB-9 — grRoots unit-root tolerance if/else has two identical branches (lost distinction)

- **Where:** `oracle/fortran/ansub2.f:1920-1924` (subroutine `grRoots`, the
  root-grouping stage of MAK1's spectral factorization):
  ```fortran
  if (abs(modul(i)-1.0d0).lt.xeps) then
    xeps2=1.0D-30
  else
    xeps2=1.0D-30
  end if
  ```
- **Severity:** `latent` — both arms assign the same `1.0D-30`, so grouping uses
  the same equality tolerance for unit-modulus and interior roots. The near-zero
  `xeps2` means `getRoot`/`getRootc` only ever match *bit-identical* roots, which
  is what the C02AEF-produced conjugate pairs are, so current decompositions are
  unaffected. But the structure (a modulus-1 test that then does nothing) is an
  abandoned special-case: the author clearly intended a looser tolerance for
  unit-circle roots (where two "equal" seasonal roots may differ by rounding) and
  the value never got filled in. A model whose seasonal roots land *near* but not
  *on* a shared point would fail to group and could mis-factor.
- **Related no-op in the same routine (ansub2.f:1940-1942):** when the conjugate
  of a complex root is not found, `if (ic.eq.0) then ic=ic` — a self-assignment
  standing in for an unimplemented error path.
- **Port:** `core/src/seats/factor.cpp` (`gr_roots`) — reproduced verbatim
  (identical-branch `if/else` and the `ic==0` no-op as a comment). Pinned by
  `tests/unit/test_seats.cpp` "mak1 C" (complex-root path exercises the grouping).
- **Modernize:** either delete the dead `if` (documenting that a single tolerance
  is intended) or supply the looser unit-circle tolerance the branch was meant to
  carry, and implement the `ic==0` conjugate-not-found error path.

---

_Append new entries as they are found while porting. Keep each pinned to a test._

## CB-10: prtref/chkadj SO-outlier operator-precedence quirk (latent)

`prtref.f:277` gates the SO-outlier factor accumulation as
`IF((Adjso.eq.1).and.rtype.eq.PRGTSO.or.rtype.eq.PRGUSO)`. Fortran binds `.and.`
before `.or.`, so this parses as `((Adjso==1 && rtype==PRGTSO) || rtype==PRGUSO)`
-- a user-defined SO regressor (`PRGUSO`) contributes its factor regardless of the
`Adjso` adjustment flag, unlike every sibling type (TD/holiday/AO/LS/TC) which is
uniformly gated. Almost certainly a missing paren (intended
`Adjso==1 && (rtype==PRGTSO || rtype==PRGUSO)`). Reproduced verbatim in
`core/src/x11/x11drv.cpp` regeff(). Inert on the current corpus (no SO regressors).

---

## CB-11: rndsa yearly-total discrepancy loops use the default DO step (only |d|==1 corrected)

- **Where:** `oracle/fortran/rndsa.f:103-116` (the force{ round=yes } rounded-SA
  benchmark).
- **Severity:** `active` — changes the rounded seasonally adjusted series (`rnd`
  table) whenever a year's rounding discrepancy `|d| > 1`.
- **Symptom:** after rounding each year's SA values to integers, `d = round(sum)
  - sum(round)` is the leftover discrepancy that should be spread by adding/
  subtracting 1 from the `|d|` obs with the largest/smallest residuals. Both
  distribution loops are written without an explicit step:
  - `IF(d.gt.0) DO j=nn,nn-d+1` — Fortran's default step is `+1`, so with
    `nn-d+1 < nn` this executes **once for d==1** (j=nn) and **zero times for
    d>=2**. Only the single largest-residual obs is ever bumped.
  - `ELSE IF(d.lt.0) DO j=1,d` — with `d<0` this runs **zero times**, so a
    negative discrepancy is never corrected at all.
  Intended code almost certainly had `,-1` on the first loop and `1,-d` (or
  `nn,nn+d+1,-1`) on the second. Net effect: the rounded SA series is only
  guaranteed to re-sum to the rounded total when `|d| <= 1`.
- **Port:** reproduced exactly in `core/src/x11/x11force.cpp` rndsa() (the `d>0`
  loop uses `for (jj=nn; jj<=nn-d+1; ++jj)`, the `d<0` loop `for (jj=1; jj<=d;
  ++jj)` -- both intentionally degenerate). Pinned by
  `tests/parity/test_force_tables.py` (`airline_automdl-x11-force`, tag `rnd`,
  bit-exact against the oracle golden).

## ansub7.f trend-min split point: `0.5d0*2D0` typo for `0.5*pi` — latent
- **Where:** `ansub7.f:474,481` set `lb/ub = 0.5d0*2D0` (== 1.0) as the split
  frequency between the two trend-spectrum-minimum searches (GlobalMINIM over
  [1.0, pi] and [0, 1.0]). The parallel diagnostic code in `spectrum.f:1016,1024`
  correctly uses `0.5d0*pi` (== 1.5708). `0.5*2D0` is almost certainly a typo for
  `0.5*pi` (the surrounding cycle/seasonal splits all use `pi`-based bounds).
- **Severity:** latent. GlobalMINIM is a 12-step global search over each half, so
  moving the split from pi/2 to 1.0 does not change which global minimum `enot`
  lands on for the airline (0 1 1)(0 1 1) case (verified: enot identical either
  way). It could matter for a model whose trend-spectrum minimum sits between 1.0
  and pi/2, but no gated spec exercises that.
- **Port:** NOT reproduced — the C++ (`core/src/seats/spectru.cpp` trend `enot`)
  mirrors `spectrum.f`'s correct `0.5*pi`, and `sigex.f:709` shows `SPECTRUM`
  (spectrum.f), not ansub7.f, is what actually fills the ct/cs/cc + enot fed to
  ESTBUR. So the ansub7.f copy is dead for the s-table path. Logged for the record;
  no test needed (the live path never uses the typo'd value).

---

## CB-12: sicp2 AR order-selection is dead code — the fit always returns the full order

- **Where:** `oracle/fortran/sicp2.f` (the Levinson-Durbin AR fit behind the
  `spectrum{ type=arspec }` AR spectrum, called from `spgrh.f`).
- **Severity:** `active` — sets the AR order (and thus every arspec `sp0/sp1/sp2/
  spr` value) for every arspec run.
- **Symptom:** the routine's header advertises AIC order selection, and the
  `DO m=1,l` loop faithfully tracks the minimum-AIC order in `Moar`/`Osd`/`Oaic`
  (`IF(Oaic.ge.aic) THEN Oaic=aic; Osd=sd; Moar=m`). But the `10 CONTINUE` tail —
  reached both by falling through the loop and by the `sdr<cst01` early `GO TO 10`
  — then unconditionally does `Oaic=aic; Osd=sd; Moar=l; DO i=1,l: Coef(i)=-a(i)`,
  **overwriting the AIC selection with the full order `l = min(Mxarsp,n-1)`**. The
  AIC bookkeeping is entirely dead; `spgrh` always gets the full-order model. (The
  `5/15/80` comment marks this as an intentional-looking Census edit.)
- **Port:** reproduced verbatim in `core/src/driver/run_spectrum.cpp` `sicp2()` —
  the loop keeps the (unused) AIC tracking, then the label-10 override forces
  `moar=l`, `osd=sd`, `coef[i]=-a[i]`. Pinned by
  `tests/parity/test_spectrum_tables.py` (`airline_spectrum-arspec`, tags
  `sp0/sp1/sp2/spr`, bit-exact ~7e-14 against the oracle goldens).

## CB-13 — shrink.f/glbshk.f `lx11(Mtype)` out-of-bounds for a stable filter

`shrink.f:57` computes the shrinkage weight as `dlx11=DBLE(lx11(Mtype))` where
`lx11` is `DATA lx11/1,3,5,9,15/` (5 entries) and `Mtype` is the final seasonal
moving-average code from `vsfb.f` (2=3x3, 3=3x5, 4=3x9, 5=3x15, **6=stable**,
7=3-term). The `Mtype.ne.7` branch indexes `lx11(Mtype)` for ANY other value, so
`Mtype==6` (stable seasonal) reads `lx11(6)` — one past the array. Undefined in
Fortran; a latent bug. Not reachable on the gated shrink specs (the default MSR
filters resolve to `Mtype in {2,3,4,5}`), so no shrink+stable spec exists to pin
it. `Mtype==7` (3-term) is special-cased to `wtx11=1/3` and avoids the access.
- **Port:** `core/src/x11/shrink.cpp` transcribes `lx11[mtype-1]` verbatim
  (same latent OOB for `mtype==6`), commented at the access site. Reproduce the
  bug if a shrink+stable path is ever gated.

---

## CB-14 — `seats{bias=0}` is validated, accepted, then silently overwritten with 1

- **Where:** `oracle/fortran/analts.f:1647-1653`, on the SEATS options path
  (`gtseat.f:280-293` → `Bias2` → `ansub9.f:1118-1122` → `L_Bias` → `Bias`).
- **Severity:** `active` — every `seats{bias=0}` run is affected.
- **Symptom:** `gtseat.f` explicitly validates the argument against `-1, 0, 1`
  ("Bias must be either -1, 0, or 1.") and stores 0 in `Bias2`; `ansub9.f`
  faithfully carries it into `L_Bias`. Then, before the decomposition,
  `analts.f` does `IF (Bias .eq. 0) THEN Bias = 1` and prints
  `BIAS SET EQUAL TO 1` into the `.out`. So the documented "no bias correction"
  setting is unreachable: `sigsub.f:1535-1585`'s `bias1c = bias3c = 1` branch —
  the one `bias=0` exists to select — is dead for user input, and
  `seats{bias=0}` produces exactly the same s10–s18 as the `bias=1` default.
  Only `bias=-1` (BIASCORR, `ansub4.f:1244-1581`) actually changes anything.
- **Port:** reproduced verbatim in `core/src/seats/seatopts.cpp`
  (`seats_resolve_options`: `r.bias = (bias2==NOTSET) ? 1 : bias2;` then
  `if (r.bias == 0) r.bias = 1;`). Pinned by
  `tests/parity/test_seats_tables.py`, corpus
  `generated/{airline,payems,expgs,unrate}_bias0-seats` — those specs carry
  `seats{bias=0}` and gate s10–s18 bit-exact (~5e-15) against goldens that are
  numerically identical to the no-`bias` baseline. If someone ever "fixes" this
  by making `bias=0` skip the `bias1c/bias2c/bias3c` correction, all four fail.

## CB-15 — `seats{hplan=}` silently re-enables an explicit `hpcycle=no`

- **Where:** `oracle/fortran/ansub9.f:1109-1117` (the SEATS options→internal
  bridge, `NMLSTS` icode==1).
- **Severity:** `active` — changes which output tables a run produces, on any
  spec that sets both arguments.
- **Symptom:** the bridge resolves the Hodrick-Prescott switch twice. Block 1
  (`:1081-1090`) reads `Lhp` (the `hpcycle=` argument, defaulted TRUE at
  `gtinpt.f:536`) and sets `L_hpcycle = 0` when the user said `hpcycle=no`.
  Block 2 then runs:

      IF(.not.dpeq(Hplan2,DNOTST))THEN
       L_hplan=Hplan2
       IF(L_hpcycle.eq.0)THEN
        L_hpcycle=1
        IF(Hptrgt.ne.NOTSET)L_hpcycle=Hptrgt
       END IF
      END IF

  `L_hpcycle` can only BE 0 at that point because block 1 saw `hpcycle=no` —
  the other branch yields -1, 1, 2 or 3 — so the test is really "did the user
  turn HP off?", and the answer is used to turn it back **on**. Setting the
  unrelated smoothing parameter `hplan=` therefore overrides an explicit
  `hpcycle=no`, and with `hptarget=` present it also picks the target.
- **Measured:** `seats{hpcycle=no hplan=40}` on airline emits `.cyc`/`.ltt`
  and echoes `hpcycle= 1` in the `.sum` INPUT block; `seats{hpcycle=no}` alone
  emits neither and echoes `hpcycle= 0`. Adding `hptarget=orig` to the first
  makes the echo `hpcycle= 3`.
- **Port:** reproduced verbatim in `core/src/seats/seatopts.cpp`
  (`seats_resolve_options`, block 2), commented at the site.
- **Pinned by:** `tests/parity/test_seats_hpopts.py`, corpus
  `generated/{airline,payems}_hp-relock-seats` (gated two ways — the resolved
  value against the oracle's `.sum` echo, and the presence of the oracle's
  `.cyc`/`.ltt` goldens). `*_hp-off-seats` pins the un-overridden "no".

## CB-16 — `seats{finite=yes}` zeroes the `pctreductionyr1..5` savelog stats (uninitialised COMMON)

- **Where:** `oracle/fortran/sigsub.f:577-598` (SECOND) reading `relRevs` from
  `/firevs/` (`oracle/fortran/revs.i`), which is filled only inside
  `oracle/fortran/getdiag.f:1402`'s `IF (out .eq. 0)` guard.
- **Severity:** `active` — hits every `seats{finite=yes}` run made through the
  X-13 wrapper at its default SEATS output level.
- **Symptom:** SECOND prints/stores the "percentage reduction in the standard
  error of the revision after additional years" statistics. With `Lfinit` false
  it computes them from `vretre/vreadj`; with `Lfinit` true it substitutes the
  finite-sample values `tmp(i)=relRevs(3,i)`, `tmp1(i)=relRevs(2,i)`, which then
  reach `setCovt1/setCovsa1/setCovt5/setCovsa5` and `USRENTRY(...,1517/1518)` —
  i.e. the `.udg` savelog keys `pctreductionyr1..5`. But `relRevs` is only ever
  written by `getDiag`'s **`IF (out .eq. 0)`**-guarded `compRevs` call
  (getdiag.f:1402, "Modified by REG, on 27 Apr 2008, to restrict finite
  revisions calculations unless out=0"). ansub9.f:1040-1056 hands SEATS
  `L_OUT = 2`, or `3` when any SEATS table is printed/saved — never 0 unless the
  user sets `seats{out=0}` *and* prints/saves nothing. So the substituted array
  is never filled and SECOND reads uninitialised `/firevs/` COMMON. Measured on
  all four corpus series: `pctreductionyr1` goes from `76.2582 42.7151`
  (airline), `76.3342 0.9996` (payems), `62.2269 3.0984` (unrate),
  `91.0536 0.9989` (expgs) to `0.0000 0.0000` in every case. Loader-zeroed
  COMMON makes that look like a clean zero for a single-series run; under `-x` /
  iterative multi-series processing it is stale data from a prior series. The
  guard asymmetry is the defect: `Lfinit` gates the *substitution*
  (sigsub.f:577) while `out` gates the *computation* (getdiag.f:1402).
- **Port:** nothing to reproduce yet — the port emits no SEATS savelog keys, and
  the whole finite-sample subsystem (`getDiag` plus its ~7.6 kloc closure:
  `getdiag/bldcov/blddif/extsgnl/compmse/complagdiag/compcrodiag/comprevs/getgr/
  getrevdec/procflts`, plus `altundovrtst` and ansub4.f `UnderOverTest`) is
  unported. `core/src/seats/seatopts.cpp` carries `Lfinit` onto
  `SeatsOptions::finite`; the decomposition correctly ignores it (measured
  invariant over 24 oracle configurations — see the note in `seatopts.hpp`).
  **If the SEATS savelog is ever ported, `pctreductionyr*` must reproduce this
  zeroing**, not the "obviously intended" finite-sample value. Pinned indirectly
  by `tests/parity/test_seats_tables.py` (`*_finite-seats`), whose blessed
  `.udg` goldens carry the zeroed values.
- **Related, not a bug — worth knowing when scoping `finite`:** the same
  `IF (Lfinit) CALL getDiag(...)` at sigex.f:1502 is the only path that sets the
  `lSAFlt/lSAGain/lSATmShf/lTreFlt/lTreGain/ltreTmShf` flags (procflts.f:111-116)
  that seatdg.f:187-275 requires before it will write the `faf/fac/ftf/ftc`
  filter-weight, `gaf/gac/gtf/gtc` squared-gain and `tac/ttc` time-shift save
  tables. Without `finite=yes` the oracle accepts those `save=` tokens and
  silently writes no file at all; with it, ten files appear. So `finite` is a
  real output gate, not only a print switch.

## CB-17 — fvalue.f zeroes its own argument, destroying the caller's F-statistic

`fvalue.f` returns the F-distribution upper-tail probability
`P(F_{m,n} > X)`. On its two "probability is 1" exits — `X <= 0`, and the
computed `p <= 0` — it falls to label 10 and executes:

```fortran
   10 fvalue=1D0
      X=0D0
      RETURN
```

`X` is the **dummy argument**, passed by reference like everything in F77, so
the routine silently zeroes the caller's variable. Every caller passes a live
F-statistic and stores it *after* the call:

- `ftest.f:104-110` — `prob=fvalue(f,kdfb,kdfr)*100D0` then `Fstabl=f` /
  `Fpres=f`, so the savelog would report `f2.fsd8: 0.000` / `f2.fsb1: 0.000`.
- `mstest.f:110-111` — `P2=fvalue(Fmove,n1,ndgfre)*100D0` writes straight
  through `/tests/ Fmove`, so `f2.msf` and the M7 input `Test2 = 3*Fmove/Fstabl`
  both see 0.

The `X<=0` exit is the reachable one: a series with no between-season variation
gives `fmsm==0`, hence `f==0`, and the statistic and its probability are then
mutually inconsistent (`F=0.000` alongside `prob=100.00`). The `p<=0` exit needs
catastrophic cancellation in the series and has not been observed.

- **Port:** `core/src/numeric/numeric.cpp` `fvalue(double& x, int m, int n)` —
  the reference parameter exists *only* to reproduce this. Both exits assign
  `x = 0.0` verbatim, commented at the site. Callers in
  `core/src/x11/x11tests.cpp` pass their live statistic (`ftest`'s `f`,
  `mstest`'s `ctx.tests.fmove`) exactly as the oracle does. Pinned by
  `tests/parity/test_x11_diagnostics.py`, which gates `f2.fsb1`/`f2.fsd8`/
  `f2.kw`/`f2.msf`/`f2.idseasonal` against the oracle `.udg` on 114 corpus
  specs — any "fix" that made the argument `const` would still pass today's
  corpus (no gated spec reaches the exit) but would diverge the moment one does,
  which is why the shape is documented here rather than only in code.

---

## CB-18 — x11pt4's `allgud` is the NEGATION of what its name and its users mean

`x11pt4.f:332-336` decides whether the Part-F summary measures may use the
unguarded `divsub`/`addmul` (all observations usable) or must use the good-obs
`divgud`:

```fortran
      allgud=T
      IF(Muladd.ne.1.and.(.not.dpeq(Cnstnt,DNOTST)))THEN
       CALL copylg(gudbak,POBS,1,Gudval)
       allgud=isfals(Gudval,Pos1ob,Posfob)
      END IF
```

`isfals.f` returns `.true.` when **at least one element is FALSE** — i.e. when
at least one observation is NOT good. So the assignment reads "allgud is true
when some observation is bad", the exact complement of the intent. Every one of
the six consumer blocks (`:373-380`, `:409-414`, `:479-490`, `:519-526`,
`:583-618`, `:620-654`) then takes the wrong branch: a span containing bad
observations divides them anyway (`divsub`), and a span where every observation
is good takes the DNOTST-producing `divgud`.

Reachability: the whole `IF` needs `Muladd != 1` **and** a user constant
(`transform{constant=}`), so the base path never evaluates it and `allgud`
stays at its `T` initialiser. The constant path is now ported and gated by
`generated/airline_constant`, whose `f2.*`/`f3.*` block matches the oracle
bit-for-bit with the negation in place — it is transcribed verbatim rather
than corrected.

- **Port:** `core/src/x11/x11summ.cpp`, `x11pt4_partf`, the `allgud` assignment
  and each of the six `if (allgud) ... else ...` blocks. Commented at the site.

---

## CB-19 — x11pt4's modified-SA restore copies the wrong way round

The Part-F measures temporarily strip the level-shift / user-regression factors
out of a series, take the measure, then put them back. Three of the four such
blocks restore from the saved copy correctly; the fourth (`x11pt4.f:643-654`,
the modified seasonally adjusted series E2) does not:

```fortran
      IF(allgud)THEN
       IF(.not.Finls.and.Adjls.eq.1)
     &    CALL addmul(Stcime,Stcime,Facls,Pos1bk,Posffc)
       ...
      ELSE
       CALL copy(Stcime,Posffc,1,Temp)     ! <-- arguments reversed
      END IF
```

Compare the identical construct at `:413` (`CALL copy(Temp,Posffc,1,Stome)`) and
`:617` (`CALL copy(Temp,Posffc,1,Stci)`), both of which copy the SAVED buffer
back into the series. Here the copy runs the other way: `Stcime` is left holding
the divided-out values (the LS/user effect is never restored) and the `/work/`
scratch `Temp` is clobbered as a side effect — `Temp` at that moment holds the
detrended E1 from `:528`, which `:694-696` then reads back into `Stmcd`.

Reachability: same gate as CB-18 — the `ELSE` is the `.not.allgud` branch, so it
needs `Muladd != 1` plus a `transform{constant=}`. That is no longer unreachable:
the constant is ported and gated by `generated/airline_constant`, whose f2/f3
block matches the oracle bit-for-bit with the reversal in place.

- **Port:** `core/src/x11/x11summ.cpp`, `x11pt4_partf`, the E2 restore block.
  Transcribed verbatim with the reversal commented.

## CB-20 — the `.tdh` (R 9.B) save rows write their separators into the wrong buffer

`revdrv.f`'s trading-day-coefficient history builds each save row's *values* in
`outARMA` but each row's *tab separators* in `outTDrg`, then writes `outARMA`
(`revdrv.f:1172-1191`):

```fortran
         CALL itoc(rdbdat,outARMA,ipos)
         DO k=1,NrvTDrg
          outTDrg(ipos:ipos)=TABCHR      ! <-- separator into outTDrg ...
          ipos=ipos+1
          CALL dtoc(CncTDrg(k,Revptr),outARMA,ipos)   ! ... value into outARMA
         END DO
         WRITE(fh,1120)outARMA(1:ipos-1)
```

`ipos` still advances, so the emitted row has a one-character gap where each tab
belongs, filled with whatever `outARMA` happened to hold. The header row two
lines above is written from `outTDrg` and *is* fully tab-separated, so the file's
columns do not line up with its own header. Measured on
`tests/corpus/extra/airline_history-model` and a `estimates=(td)`-only variant of
it:

- with `estimates=(aic arma td)`, the R 9.A block ran first and left tabs in
  `outARMA`, so the row reads `195501<TAB>-0.8246...E-02 -0.1155...E-02 ...` —
  one inherited tab, then single spaces.
- with `estimates=(td)` alone `outARMA` was never touched, and the same row reads
  `195501<NUL>-0.8246...E-02 ...` — a literal NUL byte.

So the separator in a `.tdh` file depends on which *other* history tables were
requested. The values themselves are correct and in the right order.

Reachability: any `history{estimates=(td) save=(tdh)}` run.

- **Port:** not reproduced, and deliberately so. This is a defect in the Fortran's
  assembly of an output FILE's character buffer, and this port writes no save
  files (results live on the result object; the harness formats them). The
  numbers and their column order are ported faithfully in
  `core/src/driver/run_history.cpp`'s `rvtdrg`; the gate
  (`tests/parity/test_history_tables.py::test_history_model_table`) therefore
  takes the column COUNT from the golden's tab-separated header and reads the
  data rows by whitespace split.

## CB-21 — restor.f restores `Arimaf` with the WRONG array bound (`PB`, not `PARIMA`)

`ssprep.f:79` snapshots the ARMA fixed-parameter flags with the correct extent:

```fortran
       CALL copylg(Arimaf,PARIMA,1,Fxa)
```

but its inverse, `restor.f:67`, restores them with `PB`:

```fortran
       CALL copy(Ap2,PARIMA,1,Arimap)      ! <-- PARIMA, correct
       CALL copy(Bb,PB,1,B)
       CALL copylg(Fxa,PB,1,Arimaf)        ! <-- PB, should be PARIMA
```

`Arimaf` is `LOGICAL Arimaf(PARIMA)` and `Fxa` is `LOGICAL Fxa(PARIMA)`; `PB` is
the REGRESSION column bound, which belongs to the two lines around it (`B`,
`Regfx2`), not to this one. It is a copy-paste of the neighbouring bound.

Both slidingspans (`ssx11a.f`) and history (`revdrv.f`) call `restor` before
every span, so on those paths elements `PARIMA` down to `PB+1` of `Arimaf` are
never restored from the snapshot — they keep whatever the previous span left
there.

Reachability: **none in practice, and the direction of the bound is why.**
`model.prm` sets `PB = 80` and `PARIMA = 133`, so the under-copy leaves the
tail 81..133 stale rather than reading out of bounds. Those entries only matter
for a model with more than 80 ARMA coefficients (`Mdl`/`Opr` lag slots), which
no admissible X-13 model has: the ARMA lag structure is bounded by
`PORDER`-degree operators, and the corpus's largest model uses four. Nothing
in the span loops writes them either, so stale == correct here.

- **Port:** NOT reproduced. `core/src/x11/slidingspans.cpp`'s `restor_span` uses
  `copylg(p.fxa.data(), prm::PARIMA, 1, m.arimaf.data())` — the bound the
  snapshot side uses. Since `PB < PARIMA` and elements 81..133 are identical on
  both sides for every reachable model, the two are bit-equivalent; writing
  `PB` here would only make the C++ harder to read for no observable gain.
  Pinned by `tests/parity/test_history_tables.py` (the `airline_history-fixmdl`
  spec is the one that exercises `restor`'s ARMA-flag restore most directly:
  `history{fixmdl=yes}` sets all `PARIMA` flags true and relies on every span's
  `restor` to keep them true).

---

## CB-22 — `Lr1y2y` is derived from BOTH target lists but used only by the SA tables

`history{sadjlags= trendlags=}` add "the estimate N periods later" columns to the
revision tables. When both a 1-year and a 2-year lag survive, `revchk.f` sets
`Lr1y2y`, which makes `prtrev` emit one MORE column — the 2-year estimate
against the 1-year one:

```fortran
      IF(Ntarsa.gt.0)THEN
       CALL intsrt(Ntarsa,Targsa)
       ...
         IF(Targsa(i).eq.Ny.or.Targsa(i).eq.2*Ny)i2=i2+1
       END DO
       Lr1y2y=i2.eq.2                        ! revchk.f:1080  (from sadjlags)
      END IF
      IF(Ntartr.gt.0)THEN
       CALL intsrt(Ntartr,Targtr)
       ...
       Lr1y2y=i2.eq.2                        ! revchk.f:1109  (from trendlags)
      END IF
```

There is one flag, and the trend block **overwrites whatever the sadj block
decided**. But the flag is only ever *consumed* by the SA-family tables:

```fortran
      CALL prtrev(Finsa ,Cncsa ,Rvstrt,1,LREVR1,Ntarsa,Targsa,...,lr1y2y)
      CALL prtrev(Finch ,Cncch ,Rvstrt,2,LREVR2,Ntarsa,Targsa,...,lr1y2y)
      CALL prtrev(Finisa,Cncisa,Rvstrt,3,LREVR3,Ntarsa,Targsa,...,lr1y2y)
      CALL prtrev(Fintrn,Cnctrn,Rvstrt,4,LREVR4,Ntartr,Targtr,...,F)   ! :852
      CALL prtrev(Fintch,Cnctch,Rvstrt,5,LREVR5,Ntartr,Targtr,...,F)   ! :859
```

So the *trend* lag list silently decides whether the *seasonally adjusted*
series' revision table gets its `(1yr-2yr)` column, and the trend tables can
never have one of their own. Two observable consequences:

* `sadjlags=(12 24)` with `trendlags=(6)` — the SA table LOSES the column it
  would have had on its own (`Lr1y2y` ends up false).
* `sadjlags=(6 18)` with `trendlags=(12 24)` — the SA table GAINS a column it
  cannot fill: `prtrev.f:203-226` only assigns `rev(ncol,Revptr)` inside the
  `Vtargt(i2).eq.2*Ny` branch, so with no 1yr/2yr lag among *its* lags the
  column is written straight from the uninitialised `rev` scratch array.

- **Port:** reproduced for the first case (`core/src/driver/run_history.cpp`
  derives the flag from both lists in the same order and passes `false` for the
  two trend tables), *not* for the second — an uninitialised-stack read has no
  faithful value to reproduce; the port leaves that column at 0. Gated by
  `tests/parity/test_history_tables.py::test_history_target_columns`, whose
  corpus specs (`extra/airline_history-sadjlags*`) give BOTH lists the same
  1yr/2yr pair or neither, which is the only configuration where the flag means
  what its name says.
- **Third face, same defect:** with `trendlags=(12 24)` and NO `sadjlags=` at
  all, `Ntarsa==0` and `Lr1y2y==T` reach `prtrev` together. `prtrev.f:90-91`
  sizes the SA table to `0+1 = 1` column, and `prtrev.f:202`'s
  `IF(Lr1y2y.and.i2.gt.0)` sits inside `DO i2=0,Ntargt` — which with
  `Ntargt==0` runs the `i2==0` pass only — so the single column is never
  assigned. The port omits the column rather than emit a fabricated value
  (`run_history.cpp`'s `ncol_sa` guard carries the reasoning at the line).
  Ungated: no corpus spec sets `trendlags=` without `sadjlags=`.

## CB-23 — `chkorv.f:54-58`: the span-outlier end-date guard can never fire

`chkorv` decides whether a held-back outlier may be re-introduced into the
span's design. The intent is plainly "a ramp/temporary-LS/quadratic needs BOTH
endpoints inside the revision span; everything else needs only its start":

```fortran
      IF(((otltyp.eq.RP.or.otltyp.eq.TLS.or.otltyp.eq.QI.or.
     &       otltyp.eq.QD).and.(begotl.le.Endrev.and.
     &    endotl.le.Endrev)).or.((otltyp.ne.RP.and.otltyp.ne.TLS.or.
     &       otltyp.ne.QI.or.otltyp.ne.QD).and.
     &    begotl.le.Endrev))THEN
```

The second disjunct's type test was meant to be `.not.(RP.or.TLS.or.QI.or.QD)`.
Written with mixed connectives it groups (Fortran binds `.and.` tighter than
`.or.`) as `(t/=RP .and. t/=TLS) .or. t/=QI .or. t/=QD`, which is **true for
every outlier type** — `RP` and `TLS` each satisfy `t/=QI`, and `QI`/`QD` each
satisfy the leading pair. So the second disjunct reduces to `begotl<=Endrev`,
and since the first disjunct implies it, the whole condition is just
`begotl <= Endrev`. A ramp whose END lies past the revision span is re-added
regardless.

Note `chkorv.f:69-70`'s neighbouring `nlast` test spells the same intent with
`.and.` throughout and is correct — which is what makes this a typo rather than
a deliberate rule.

- **Port:** reproduced verbatim (`core/src/driver/rev_outlier.cpp` tests
  `begotl <= endrev` alone and keeps `span_type` only for the `nlast` test).
  This was initially mis-ported as the *intended* ternary — the comment named
  the quirk and the code then "fixed" it — and was caught in review.
- **Reachability:** needs an `rp`/`tls`/`qi`/`qd` regressor in a `history{}`
  run whose end date falls past the revision span. No corpus spec has one, so
  it is currently unreachable and ungated.

## CB-24 -- `acfdgn.f`'s seasonal-ACF `.udg` block is unreachable

`acfdgn.f:79-91` writes an `acf$NN` line per SEASONAL lag to the unified
diagnostics file:

```fortran
      IF(Ldiag.and.(Sp.eq.4.or.Sp.eq.12))THEN
       i=i+1
       ilag=Sp*i
       DO WHILE (ilag.le.Mxlag)
```

`i` is never initialised in this block. It is set to 1 only inside the
*log-file* block at `:64-77`, which is behind `Svltab(LSLSAC)`; with that off,
`i` still holds whatever the preceding counting loop at `:57` left it --
`Nlagbl+1`. `i=i+1` then starts the walk at `Sp*(Nlagbl+2)`, which is past
`Mxlag` for every admissible Mxcklg, so the DO WHILE body never runs. And when
the log block IS on, it leaves `i` one past the last seasonal lag it printed,
so `i=i+1` overshoots by one more -- the udg block never fires either way.

Not reproduced: there is no output to reproduce. Confirmed empirically -- not
one of the 331 `.udg` goldens in `tests/golden/` contains an `acf$NN` line,
including the 287 that carry the rest of the check{} block.

Pinned by: `tests/parity/test_check_diagnostics.py`, whose key-set assertion
runs in BOTH directions -- if the port ever emitted an `acf$NN` line the gate
would fail with "keys the oracle does not emit".

## CB-25 -- `QsRsd`/`QsRsd2` are initialised inside `arima`, so a model-free run reports an uninitialised COMMON as a statistic

`genqs.f` reports seven QS seasonality statistics; six it computes itself, and
the seventh -- the regARIMA RESIDUAL pair `QsRsd`/`QsRsd2` -- it only reads:

```fortran
      IF(.not.dpeq(QSrsd,DNOTST).and.Iagr.lt.4)
     &    WRITE(Nform,1030)'qsrsd',QSrsd,chisq(QSrsd,2)
```

Both live in `/arima/` (`arima.cmn:126-127`) and are assigned in exactly two
places, both inside `arima` itself: the reset at `arima.f:129-130`

```fortran
      QsRsd=DNOTST
      QsRsd2=DNOTST
```

and the computation at `arima.f:1105-1118`. But `x11ari.f:106` calls `arima`
only `IF(Lmodel)`. On a run with no regARIMA model at all -- an `x11{}` spec
with no `arima{}`/`regression{}`/`estimate{}` -- that routine is never entered,
so neither assignment happens and `genqs` reads the COMMON's static zero. The
test is `.not.dpeq(QSrsd,DNOTST)`, and 0.0 is not DNOTST, so the row is emitted:

```
qsrsd:         0.00000    1.00000
```

reported as "no evidence of residual seasonality" for a run that has no
residuals. `tests/golden/generated/airline_x11-default` is one of many.

The contrast that proves the mechanism is `tests/golden/extra/airline_identify`:
`identify{}` sets `Lmodel`, so `arima` DOES run and the reset fires, but nothing
is estimated (`Var` stays zero) so the `:1105` block does not -- and that golden
correctly carries no `qsrsd`/`qssrsd` line at all.

Because it is a COMMON rather than a local, the value also SURVIVES between the
specs of a metafile: a model-free spec that follows one with a model inherits
its predecessor's value or its predecessor's DNOTST. That is visible in
`tests/golden/census-examples/composite-history-mismatch/total`, whose
model-free total carries no `qsrsd` row -- it inherited DNOTST from a component.

- **Port:** reproduced. `QsStats::qsrsd`/`qsrsd2` (`core/src/diag/genqs.hpp`)
  default to **0.0**, not DNOTST, and `run_pre_model` resets them to DNOTST at
  the `arima` entry point under `ctx.captured.has_model` -- nowhere else. The
  metafile carry-over is NOT reproduced: `tools/x13run_composite.cpp` carries
  only the aggregation COMMONs between specs, and the composite goldens are not
  in this gate's discovery.
- **Pinned by:** `tests/parity/test_qs_diagnostics.py`, which compares key sets
  in BOTH directions -- dropping the zero default makes every no-model spec fail
  with "keys missing from the engine: ['qsrsd', 'qssrsd']", and dropping the
  reset makes `extra/airline_identify` fail the other way.

## CB-26 -- `gennpsa.f`'s span-block test compares INTEGERs against the DOUBLE sentinel

`gennpsa.f:111-112` decides whether either NP block has anything to report:

```fortran
      lnp=.not.((Npsadj.eq.NOTSET).and.(Npsadj2.eq.NOTSET))
      lnps=.not.((NpsadjS.eq.DNOTST).and.(NpsadjS2.eq.DNOTST))
```

`NPsadj`, `NPsadj2`, `NPsadjS` and `NPsadjS2` are all declared INTEGER at
`:28-29` and all four are initialised to `NOTSET` (-32767). The first line tests
that correctly. The second tests against **`DNOTST`**, the DOUBLE PRECISION
-999.0 sentinel from `notset.prm` -- so the comparison promotes each integer to
double and asks whether -32767.0 equals -999.0. It never can, for any value
these variables take (NOTSET, 0 or 1), so **`lnps` is unconditionally TRUE**.

The savelog block gets away with it: `IF(Savtab(Tblind).and.lnps)` opens, but
each row inside re-tests `NOTSET` individually (`:162`, `:170`), so nothing
extra is written and the `.udg` is correct. The PRINT block at `:124-128` does
not -- it emits the

```
  (Series start in <date>)
```

header and calls `OutNP` with two NOTSET values on a run where the diagnostic
span starts at the series start and there are no span statistics at all.

- **Port:** the predicate is transcribed with the widening cast made explicit
  (`NpStats::lnps` in `core/src/diag/genqs.cpp`), so it returns true exactly as
  the Fortran does. The consequence is not reproduced because this port emits
  no print surface -- the savelog block, which is what the goldens carry, is
  identical either way.
- **Pinned by:** `tests/parity/test_qs_diagnostics.py`. Correcting the sentinel
  to NOTSET would leave every `npssadj`/`npssadjevadj` row unchanged (the inner
  guards already suppress them), which is exactly why this is worth writing
  down rather than "fixing".

## CB-27 -- `svtukp.f`'s `oriIdx` compares a TABLE index against a FREQUENCY index

```fortran
      oriIdx=NOTSET
...
       ELSE IF(Itukey(i).eq.LSPTS0.or.Itukey(i).eq.LSPT0C)THEN
        CALL copy(Ptso,6,1,thisPk)
...
        IF(.not.Lsadj)oriIdx=i          ! svtukp.f:43  -- i indexes ITUKEY
...
        DO k=1,6
         IF(k.ne.oriIdx)THEN            ! svtukp.f:85  -- k indexes FREQUENCIES
          IF(thisPk(k).gt.0.90D0)npk90=npk90+1
          IF(thisPk(k).gt.0.99D0)npk=npk+1
         END IF
        END DO
```

`i` is the position of the ORIGINAL series' spectrum in the `Itukey` table list
(1..Ntukey, at most four entries: rsd, ori, sa, irr). `k` is one of the six
seasonal frequencies. The two index completely different things.

The evident intent is "on a run that produces no seasonal adjustment
(`Lsadj = Lx11.or.Lseats` false), leave the original series out of the
`peaks.tukey.*` lists" -- searching an unadjusted original for a seasonal peak
says nothing. What the code does instead is **drop one seasonal FREQUENCY --
whichever number happens to equal that table's slot -- from the peak counts of
EVERY table in the list**, the original included.

On a model-only run the list is `(rsd, ori)`, so `oriIdx` is 2 and the
k=2 frequency (2/12, the four-month cycle) is excluded from both tables' `npk`
and `npk90` tallies. A `spcrsd.tukey.s2` of 0.995 therefore does not put `rsd`
into `peaks.tukey.seas`, while the same value at s1 or s3 would.

`oriIdx` stays NOTSET on any run that DID adjust, so the defect cannot reach an
X-11 or SEATS spec -- which is why it survives: those are the runs anyone looks
at.

- **Port:** transcribed verbatim in `tukey_peak_labels`
  (`core/src/driver/spectrum_peaks.cpp`), including the NOTSET sentinel, so the
  comparison is against an index that can never match on an adjustment run.
- **Pinned by:** `tests/parity/test_spectrum_peaks.py`'s `peaks.tukey.*` keys
  over the model-only corpus specs.

## CB-28 -- `spcrsd.f` builds the shifted residual span and then passes the unshifted one

```fortran
       ntmp=na-ipos+1
       IF(ntmp.ge.80)THEN
        IF(ipos.gt.1)THEN
         DO i=ipos,Na
          Temp(i-ipos+1)=a(i)           ! spcrsd.f:115-117 -- repack into Temp
         END DO
        END IF
        CALL getTPeaks(a,ntmp,Sp,Hrsd,mrsd,Pttdr,Ptsr,mvrsd)   ! :119 -- passes `a`
```

`ipos` is where the diagnostic span (`Bgspec`, eight years back from the series
end by default) starts inside the residual vector. The block repacks
`a(ipos..na)` into `Temp` so the Tukey spectrum can be taken over that span --
and then hands `getTPeaks` **`a`**, not `Temp`. The repack is dead: `Temp` is
never read again on this path.

So only the LENGTH reflects `ipos`. The residual Tukey spectrum is always taken
from `a(1 .. na-ipos+1)` -- the FIRST `ntmp` residuals -- rather than from the
last `ntmp`. Whenever `Bgspec` is later than the residual start (any series
longer than the eight-year default window, i.e. most of this corpus), the
`spcrsd.tukey.*` probabilities describe a different, earlier stretch of the
residuals than the `spcrsd.*` AR-spectrum peaks printed beside them.

The three `getTPeaks` call sites in `spcdrv.f` (`:252-255`, `:389-392`,
`:507-510`) do the identical repack and pass the repacked array, which is what
makes this a slip rather than a convention.

- **Port:** reproduced in `run_spectrum.cpp` -- the residual Tukey call passes
  the residual buffer from element 1 with length `ntmp`, and the comment at the
  call site names this entry.
- **Pinned by:** `tests/parity/test_spectrum_peaks.py`'s `spcrsd.tukey.s1`-`s6`
  and `.td` keys (239 goldens). Fixing the argument moves them.

## CB-29 — `getreg.f:97` — the `noapply` dictionary runs two tokens together, so `seasonal` is unusable

`MDLDIC` for the `regression{noapply=}` argument is

```
PARAMETER(MDLDIC='tdaolsholidayuserseasonalusertcso')
DATA mdlptr/1,3,5,7,14,26,30,32,34/
```

The intended entry list is `td ao ls holiday user seasonal user tc so` — but
`user` and `seasonal` were concatenated into ONE dictionary entry and `mdlptr`
was built around the result. Decoding the pointers gives:

| entry | span | token |
|---|---|---|
| 1-4 | 1..14 | `td` `ao` `ls` `holiday` |
| **5** | **14..26** | **`userseasonal`** (12 characters) |
| 6 | 26..30 | `user` |
| 7-8 | 30..34 | `tc` `so` |

Entry 5 is the one whose branch sets `Adjsea=-1` (`getreg.f:305-306`). So the
documented spelling `noapply=(seasonal)` matches no entry and is **rejected**,
and the only string that reaches the seasonal-regressor arm is the nonsense
token `noapply=(userseasonal)`.

Measured on the oracle (airline, `variables=(td easter[8] ao1955.jan
ls1958.jul)`, comparing `d11.f`):

```
no noapply     0.42911     seasonal       ERROR: rejected
td             0.61979     userseasonal   parses (inert here -- no seasonal regressor)
ao             0.44078     tc / so        parse, inert here
ls             0.52636
holiday        0.66599
```

The argument's own error message — "Choices for the noapply argument are td, ao,
ls, holiday, or user." — lists neither `seasonal` nor `tc` nor `so`, so the
defect is invisible from the diagnostics as well as from the manual.

- **Port:** reproduced verbatim in `gt_regression`'s `argidx == 12` branch —
  `MDLDIC`/`mdlptr` are transcribed character for character, so `seasonal` is
  rejected and `userseasonal` reaches `adjsea` exactly as in the oracle. A
  comment at the line says so, since it otherwise reads as a typo.
- **Pinned by:** `tests/corpus/generated/airline_noapply-{td,ao,ls,holiday}` —
  the four reachable groups. The `userseasonal` spelling is not gated: it needs
  a `regression{variables=(seasonal)}` spec, which is its own front.

## CB-30 — `mkpeak.f:253-392` — with `altfreq` and `peakwidth>1`, two trading-day peak indices are never assigned

`spectrum{altfreq=yes}` adds a THIRD trading-day frequency, so `nTfreq` is 3 and
`Tlow`/`Tup`/`Tpeak` all need three entries. The `Lfqalt` branch assigns all
three only at `Peakwd == 1`:

| `Peakwd` | `Tlow` | `Tup` | `Tpeak` |
|---|---|---|---|
| 1 | 37, 45, 58 | 41, 49, 62 | 39, 47, 60 |
| 2 | 36, 44, 57 | 42, 50, 63 | **47, 60** |
| 3 | 35, 42, 56 | **51, 64** | **47, 60** |
| 4 | 34, 41, 55 | **52, 65** | **47, 60** |

At `Peakwd >= 2` the third `Tpeak` is never set, and at `Peakwd >= 3` the third
`Tup` is not either. `/spcidx/` is a COMMON and `mkpeak` runs once per run, so
those slots hold the COMMON's initial storage — `mkfreq.f:34-38` then reads
`Tpeak(3)`/`Tup(3)` to place a frequency, and `ispeak` reads them again to score
the peak.

The non-`Lfqalt` branch has the mirror-image slip in the other direction:
`Peakwd = 4` assigns `Tup(3) = 62` (`mkpeak.f:374`) where `nTfreq` is 2 and
`Tlow`/`Tpeak` stop at two entries — a stray write past the used range. That one
is harmless, because nothing reads `Tup(3)` when `nTfreq` is 2.

- **Port:** the stray `Tup(3) = 62` IS transcribed (the table matches the source
  row for row). The `Lfqalt` branches are **not** ported: `spectrum_peak_grid`
  declines for `altfreq=yes` at every `Peakwd`, so the engine emits no peak
  block rather than an unverifiable one. Reproducing an uninitialised read is
  not something to guess at, and `Peakwd == 1` — the one complete `Lfqalt`
  row — is held back with the rest so the argument has a single behaviour.
- **Pinned by:** nothing yet; `altfreq` needs the COMMON's initial contents
  established before a golden means anything. `tests/corpus/extra/
  airline_spectrum-peakwidth{2,3,4}` gate the four non-`Lfqalt` rows, which is
  what makes the table above trustworthy as a transcription.

## CB-31

**`x11ari.f:346` hands `genqs` a SAVELOG index where the routine uses it as a
TABLE-log subscript, so the indirect QS block is never written.**

- **File:line:** `x11ari.f:346-348` (the call), `genqs.f:439` / `:481` (the use).
- **Severity:** silent loss of an entire diagnostic block on every composite run.

`genqs` takes a `Tblind` argument and gates both of its savelog blocks on
`Savtab(Tblind)`. The DIRECT call at `x11ari.f:279` passes `LSPCQS`, which is
`spctbl.i:30`'s **113** — a table-log index, and one that `gtinpt.f:121-123`
copies in from `sumtab.var` whenever `Lsumm > 0`, which is why every `-s` golden
in this corpus carries the direct `qs*` rows.

The INDIRECT call one screen later passes **`LSLIQS`**, which is
`spcsvl.i:12`'s **69** — a **savelog** index from a different enumeration
entirely. `Savtab(69)` is some unrelated table's save flag, so the whole
`qsind*` family (`qsindsadj`, `qsindsadjevadj`, `qsindirr`, `qsindirrevadj`, and
their `qssind*` twins) is gated on a condition that has nothing to do with it.

`spctbl.i:30` defines **`LSPQSI = 114`**, immediately after `LSPCQS`, and it is
plainly the intended argument. It is passed nowhere in the source.

The sibling calls are all correct, which is what makes this a slip rather than a
convention: `gennpsa` gets `LSPNPA` (117) directly and `LSPNPI` (118) indirectly,
both from `spctbl.i`, and both families appear in the golden.

- **Confirmed empirically:** `tests/golden/census-examples/composite-fixed/total/
  total.udg` carries a full set of `npind*`/`npsind*` and ~44 `spcind*` keys and
  **not one `qsind*`**.
- **Port:** reproduced by NOT calling `genqs` a second time — there is nothing to
  compute, since the oracle's own gate is false. The reasoning is recorded at the
  call site in `core/src/driver/run_x11.cpp`.
- **Pinned by:** `tests/parity/test_composite_tables.py::
  test_composite_no_indirect_qs`, which asserts BOTH that the golden has no
  `qsind*` and that the engine emits none. If the golden ever grows them the test
  fails loudly rather than silently accepting a changed oracle.

## CB-32

**`agr3s.f` never stores the DIRECT seasonally adjusted series, so the composite
comparison statistics measure the roughness of the UNADJUSTED aggregate.**

- **File:line:** `agr3s.f:99-109` (the missing store), against `agr3.f:101-108`
  (which has it); consumed at `agr2.f:89`/`:96`.
- **Severity:** the direct-vs-indirect comparison is not direct-vs-indirect at
  all on the SEATS composite path -- the numbers are printed and saved with the
  DIRECT label.

`agr2`'s `Iagr==4` branch is the direct-vs-indirect roughness comparison. It
reads the DIRECT seasonally adjusted series out of `Orig2` and the direct trend
out of `Tem`. Neither is an argument: `agr3` puts them there on its way past.

`agr3.f:58` EQUIVALENCEs a local scratch array `tempo` onto `Orig2`, and
`agr3.f:101-108` fills it with the direct SA (`Stci` under Lx11, `Seatsa`
otherwise) in the same loop that fills `Tem`. `agr3s.f:99-109` is that loop
minus the `tempo` line: it writes `Tem`, and `Stci` (dead -- `:139-142`
overwrites it), and nothing else.

`Orig2` is therefore still what `editor.f:2492` and `arima.f:1432-1440` left
there: the ORIGINAL series with backcasts and forecasts appended, which is
exactly the buffer `agr2.f:267` aggregates the components' originals from. So
`aggmea(Orig2, Tem, ...)` measures the roughness of the aggregate ORIGINAL -- a
series that still contains its seasonality -- and reports it as the direct
adjustment's.

- **Confirmed empirically**, on `tests/corpus/census-examples/composite-seats/`
  (the same synthetic data as `composite-fixed/`, components adjusted by SEATS
  instead of X-11): the oracle prints DIRECT R1-MSE **66.777** where the agr3
  path prints **53.586** for the same data, and 66.777 is R1 of the summed input
  `.dat` files to all six printed digits (66.77653631). The INDIRECT column
  agrees between the two paths, as it should.
- **Port:** reproduced by leaving the store out -- `core/src/composite/agr3s.cpp`
  carries the analysis at the loop. Adding it would be an improvement, which is
  what this project does not do.
- **Pinned by:** `tests/parity/test_composite_seats.py::
  test_seats_composite_roughness_table`, which compares all twelve `di()` values
  against the oracle's own printed table.

## CB-33

**`automx.f:922`'s backcast acceptance test is `.and.` where the algorithm wants
`.or.`, so `pickmdl{bcstlim=}` can never reject anything.**

- **File:line:** `automx.f:922-928`.
- **Severity:** an option that is documented, parsed, validated and printed
  about, and that cannot change the run.

`pickmdl{}` re-scores its selected model over the BACKCAST span
(`automx.f:906` -> `amdfct` with `Bckcst=T`) and is meant to drop backcasting
when the backward extrapolation is poor:

```
IF(mape(4).gt.Bcklim.and.(.not.argok))THEN
 Nbcst=0
 Pos1bk=Pos1ob
 IF(Prttab(LAXMCH))WRITE(Mt1,1110)
ELSE IF(Prttab(LAXMCH))THEN
 WRITE(Mt1,1090)Mdldsn(1:Nmddcr)
END IF
```

`argok` is the estimation-success flag. A model that CONVERGED therefore passes
the screen no matter how large `mape(4)` is, and a model that did NOT converge
has already been dealt with upstream -- so the branch is effectively dead. The
neighbouring comment ("check to see if argok false and print out error message
for backcasts", BCM May 2007) reads as two independent reasons to reject, which
is what `.or.` would give.

**The program contradicts itself out loud.** `prtamd` evaluates the screens on
its own and prints its verdict; the action branch above does not agree with it.

- **Confirmed empirically.** `pickmdl{bcstlim=1}` on airline (default 18):
  `.out` prints

  ```
   MODEL   2 REJECTED:
     Average backcast error >   1.00%

                 The model chosen is (0 1 2)(0 1 1)
  ```

  -- the REJECTED line from prtamd, the "model chosen" line from the `ELSE`
  arm -- and the table footer still reads `Includes 12 backcasts.` Every `.udg`
  key is identical to the `bcstlim` default run except `bcstlimit` itself.
- **Port:** transcribed with `&&`
  (`core/src/automdl/automx.cpp`, the `ctx.aape.mape[3] > ar.bcklim && !argok`
  test), with the analysis at the line.
- **Pinned by:** `tests/parity/test_backcast_aape.py::test_backcasts_survive`,
  which asserts the golden's own "Includes 12 backcasts" footer -- so if the
  oracle is ever fixed the test fails rather than the port silently diverging.

## CB-34

**`automx.f:264`'s Picktd restore assigns `padj2=Priadj` where its two
identical siblings assign `Priadj=padj2`, so the prior-adjustment mode is not
restored -- the SAVE is overwritten with the live value instead.**

- **File:line:** `automx.f:264` (against `automx.f:330` and `automx.f:725`).
- **Severity:** silent. Nothing errors; the run continues with a `Priadj` that
  describes a different candidate's trading-day verdict.

`pickmdl{}` saves the entry state at `automx.f:97-100`:

```
      pktd=Picktd
      padj2=Priadj
      CALL copy(Trnsrs,PLEN,1,tsrs)
      CALL copy(Adj,PLEN,1,a2)
```

`Picktd` ("a trading-day regressor is in the model") decides whether the
program's length-of-month / leap-year prior is divided into the series, so a
candidate whose AIC verdict differs from the entry state is modelling a
DIFFERENT series. Three places put the entry state back, and all three are the
same four-line idiom -- copy `tsrs` into `Trnsrs`, copy `a2` into `Adj`, set
`Picktd`, restore `Priadj`. Two of them do:

```
          Picktd=pktd
          Priadj=padj2
```

and the third, inside the post-loop re-estimation of the best model, does:

```
         IF(bstptd.eqv.pktd)THEN
          CALL copy(tsrs,PLEN,1,Trnsrs)
          CALL copy(a2,PLEN,1,Adj)
          padj2=Priadj
```

The two series copies are correct and `Picktd=bstptd` was set one line above,
so the only difference is the direction of the `Priadj` assignment. It reads as
a transposition of the neighbouring lines rather than a deliberate choice:
`padj2` is a save slot that is written once at entry and read nowhere after
this point, so the statement has no effect other than to skip the restore.

- **Reachability.** Only on the branch that re-estimates a best model whose
  `Picktd` differs from the LAST candidate's but equals the ENTRY state -- i.e.
  a `regression{aictest=(td)}` run where the AIC verdict flips between
  candidates. That configuration is separately walled in this port (see
  `docs/WALLS.md`, `automx.cpp`'s Picktd-flip fatal), so the defect is not
  currently exercised.
- **Port:** transcribed as written, at the `bstptd == pktd` sub-branch in
  `core/src/automdl/automx.cpp`, with the analysis at the line.
- **Not the cause of the walled discrepancy.** Mutation-tested by reversing the
  assignment to what the siblings do: the walled spec's d11/d13 February gap is
  unchanged (identical 5 failures), so CB-34's direction and that gap are
  independent.

---

## CB-35

**`x11aic.f` reads `aicnus` UNINITIALIZED whenever the x11regression trading-day
test is accepted and no Easter test runs alongside it -- and the value it reads
DECIDES the user-defined AIC test.**

- **File:line:** `x11aic.f:245` (the arm that should have seeded it),
  `x11aic.f:481` / `:557` (the reads).
- **Severity:** `active`. It selects the wrong model, not merely the wrong
  printout.

`aicnus` is the AICC of the irregular-regression model WITHOUT the user-defined
regressors. It is a plain local with no initializer, and there are exactly three
routes to a value at `:481`:

| route | when |
|---|---|
| `:470` computes it | `estend` still true at `:463` -- nothing upstream left a fitted design |
| `:457` seeds it from the winning Easter AICC | the Easter test ran (`aictest` contains `easter`) |
| **nothing** | the TRADING-DAY test was accepted and there is no Easter test |

The third case exists because of a copy-paste slip:

```
       IF(Xeastr)THEN
        aichol=aictd
       ELSE IF(Xeastr)THEN
        aicnus=aictd
       END IF
```

`:243` has already tested `Xeastr`, so `:245`'s `ELSE IF(Xeastr)` can never be
true. From the shape of the surrounding code the intent was plainly
`ELSE IF(Xuser)` -- the two arms hand `aictd` to whichever of the two remaining
tests is going to run next.

- **What it does to the result.** The vendored `x13as_ascii_O2.exe` reads
  **exactly 0.0** out of the uninitialized slot. The verdict at `:557` is
  `aicusr + Xraicd < aicnus`, so with `aicnus == 0` any negative AICC wins:
  **the user-defined regressors are accepted unconditionally on this path**, no
  matter how badly they fit. On the gate spec the accepted AICC is -785.9.
- **Reachability.** `x11regression{ variables=(td) aictest=(td user) }` with the
  trading day accepted, i.e. the common configuration. Add `easter` to the
  aictest list, or make the TD test reject, and the defect disappears.
- **Not a compiler artefact of one run.** The 0.0 reproduces across a changed
  ARIMA model. It is still one Fortran build's stack value rather than a
  language guarantee, which is exactly why it is pinned by a gate rather than
  argued about.
- **Port:** `core/src/x11/x11reg.cpp`, `x11aic`'s `double aicnus = 0.0;` with the
  three-route analysis at the declaration, and the dead `ELSE IF(Xeastr)` arm
  transcribed in the trading-day branch above it.
- **Pinned by:** `tests/corpus/extra/airline_x11regression-aictest-tduser`
  (uninitialized, `aictest.xu.aicc.nouser: 0.000000000000000E+00`), against
  `-aictest-tduser-reject` (`:470` computes it) and `-aictest-easuser` (`:457`
  seeds it) -- the three specs cover all three routes. Mutating the initializer
  to `DNOTST` fails the first.

## CB-36

**`editor.f:1710` decides whether the irregular regression has a HOLIDAY group
by reading a local that is only ever ASSIGNED on a user trading-day column --
so a `usertype=td` column declares the NEXT column to be the holiday group, and
that flips the whole run from 2.5-sigma clipping to automatic AO outlier
identification.**

- **File:line:** `editor.f:1690-1716` (the loop), `:1710` (the stale read),
  `:1727-1747` (what it decides).
- **Severity:** `active`. It changes the extreme-value method, hence the fitted
  irregular regression, hence D10-D13.

```fortran
        IF(Nusxrg.gt.0)THEN
         iusr=1
         DO icol=1,Nbx
          IF(Rgxvtp(icol).eq.PRGUTD.and.Nusxrg.gt.0)THEN
           rtype=Usxtyp(iusr)          <-- assigned ONLY here
           iusr=iusr+1
           ...
          ELSE IF((.not.(Holgrp.gt.0.or.Axruhl)).and.
     &            rtype.ge.PRGTUH)THEN <-- read on every OTHER column
           Holgrp=icol
```

`rtype` is a plain local. On the columns the `ELSE IF` actually runs for it is
either UNINITIALIZED (no user-TD column seen yet) or STALE -- it holds the
`usertype=` of the last user trading-day column. The test `rtype.ge.PRGTUH`
means "is this a user HOLIDAY type", and PRGUTD (57) is >= PRGTUH (49) while
PRGTUD (18) is not. So:

- a `usertype=td` column makes the very next column the holiday group, whatever
  that column really is -- a plain user regressor, or a trading-day contrast;
- a genuine `usertype=holiday` column, the case the arm was written for, never
  triggers it, because its own type is never in `rtype` when its own column is
  examined.

Both halves are the same defect. The intent was plainly to test the CURRENT
column's type.

- **What it does to the result.** `Holgrp>0` disqualifies the 2.5-sigma
  `tdxtrm` clip at `editor.f:1729-1736`, so the run takes the other arm and
  runs automatic AO outlier identification instead. Measured on two specs that
  differ ONLY in the order of `usertype = (user td)` vs `(td user)`: the second
  picks up seven AO regressors the first does not, and every row of
  b16/c16/d10/d11 differs between them.
- **It also silently rewrites the aictest.** In the same loop, a user TD column
  taken as `Tdgrp` when no real trading-day group exists sets `Xtdtst=0` and
  `Xuser=T` -- an `aictest=(td)` becomes an `aictest=(user)`.
- **Port:** reproduced, in `xrg_editor_setup`
  (`core/src/specparse/readers_spec.cpp`) along with the rest of the block --
  the group pointers, the Tdgrp/Stdgrp promotion, the Xtdtst->Xuser rewrite and
  the extreme-value rule. It was WALLED at first because taking the arm left
  c16 1.1e-3 out; that turned out to be an unrelated hole (x11mdl.f:531-540's
  effective-type remap, `docs/M5_PORT_NOTES.md` entry 64), not this defect.
- **Pinned by:** `tests/corpus/extra/airline_x11regression-aictest-user2swap`,
  which reaches the arm, against `-aictest-user2`, which is the same spec with
  the `usertype=` order reversed and does not. Removing the arm costs 14 gates.
  `-easter` gates the other route into the same rule (`Holgrp>0` through a real
  `easter[8]` regressor).

## CB-37

**`editor.f:1786` reads `Grpx(-1)` when the AIC trading-day test is requested
but `variables=` names no trading day. The subscript is out of bounds; the
ADDRESS is not. COMMON storage association puts `Clxptr(PB)` there, it is 0,
and the comparison it feeds silently rewrites the user's `td` request to
`td1coef`.**

- **File:line:** `editor.f:1783-1790`, inside the
  `IF(Xtdtst.eq.1.or.Xtdtst.eq.3)` arm of the "Check options for AIC trading
  day test" block.

```fortran
ELSE IF (Xtdtst.eq.1.or.Xtdtst.eq.3)THEN
 begcol=Grpx(Tdgrp-1)          ! Tdgrp==0 -> Grpx(-1)
 endcol=Grpx(Tdgrp)-1
 IF((Xtdtst.eq.1).and.(begcol.eq.endcol))THEN
  Xtdtst=3                     ! td  ->  td1coef
```

- **Why it is not undefined behaviour.** `Grpx` is `DIMENSION Grpx(0:PGRP)`
  (`PGRP=PB=80`), so `-1` is one below the lower bound. But `xrgmdl.cmn:49`
  declares

  ```fortran
  COMMON /cx11rg/ Clxptr,Grpx,Gpxptr,Nbx,Ncoltx,...
  ```

  with `Clxptr(0:PB)` immediately BEFORE `Grpx(0:PGRP)`, and Fortran storage
  association makes a COMMON block contiguous in declaration order. `Grpx(-1)`
  therefore resolves to `Clxptr(PB)` -- 81 integers INSIDE the block, not off
  the end of it. No wild pointer, no fault, no compiler dependence. The
  subscript is non-conforming; the address is fully determined.

- **Measured, not inferred.** `tools/ref_grpx.f` compiles against the vendored
  headers read-only, poisons both arrays with distinguishable values
  (`Clxptr(i)=1000+i`, `Grpx(i)=2000+i`) and runs editor's own two lines from a
  subroutine so `-O0` cannot fold them:

  ```
  Clxptr(PB)          =     1080
  Grpx(0)             =     2000
  begcol = Grpx(-1)   =     1080
  alias is Clxptr(PB)? T
  ```

- **Why the comparison then succeeds.** `Clxptr(PB)` is `Colptr(PB)`:
  `loadxr.f:38` does `cpyint(Colptr(0),PB+1,1,Clxptr(0))`, all `PB+1` elements,
  regardless of how many are meaningful. `Colptr(80)` is only written by a model
  carrying 79 regressors (`insptr.f:54-55` writes up to `Ptrvec(Nelt+1)`;
  `adrgef.f:363` passes `PB` as the bound), so in practice it holds the block's
  static 0. `Grpx(0)` is 1, `endcol` is 0, `begcol` is 0, and `Xtdtst` flips
  1 -> 3.

- **What it does to the result.** The user asked to AIC-test `td` (six
  contrast columns); the oracle tests `td1coef` (one) instead, reports
  `aictest.xtd.reg: td1coef`, and on the airline series rejects it --
  `aictest.xtd: no`, `aictest: none` -- landing in `x11mdl.f:308`'s
  identity-factor NOTE branch. Measured stable across 1, 2 and 4 x11regression
  columns; the control, with `td` present in `variables=`, reports `td`.

- **Port:** reproduced, in `xrg_editor_setup`
  (`core/src/specparse/readers_spec.cpp`). The C++ COMMON mirrors are separate
  `farray1lb` objects (`xrgmdl_cmn.hpp:11-13`) and are NOT storage-associated,
  so the alias is written explicitly:

  ```cpp
  const int begcol = (tdgrp == 0) ? xg.clxptr(prm::PB) : xg.grpx(tdgrp - 1);
  ```

  Reading the real value rather than hardcoding the flip is deliberate:
  `Colptr(PB)` IS writable by a 79-regressor model, so assuming 0 would be an
  approximation that breaks silently at the limit. `farray1lb::operator()`
  bounds-checks, so co-locating the arrays to make the alias implicit was
  rejected -- it would make every stray subscript "work" and cost a live
  guardrail.

- **Pinned by:** `tests/corpus/extra/expgs_x11regression-aictest-tdflip-qtr`.
  The flip has exactly one PARSE-time consequence, and that is what the gate
  uses: on QUARTERLY data the rewritten `Xtdtst==3` walks into `editor.f:1832`'s
  `(Xtdtst.eq.3.or.Xtdtst.eq.4).and.Sp.ne.12` and the run is refused with "Need
  monthly data to perform aictest for stock trading day." Without the flip
  `Xtdtst` is still 1, that arm does not fire, and `Sp==4` passes the next arm
  cleanly -- so the refusal cannot happen unless the aliased read happened.
  Replacing the alias with a sentinel costs 2 gates.

  It could not be gated on the AICCs: x11regression demands a trading-day OR
  holiday regressor, so "no TD group" forces a holiday, which forces
  `editor.f:1727`'s auto-AO branch, whose ~7.9 AICC gap is a separate open
  front and is walled.

## CB-38

**A fixed-form continuation splits the word `when` in an error message, and the
blank pad at column 72 lands inside it: the oracle prints "less than zero w hen
specifying".**

- **File:line:** `editor.f:1655-1656`.
- **Severity:** `cosmetic`. It is a printed message, not a number -- but it is a
  message this port has to emit VERBATIM, so it earns an entry rather than a
  code comment nobody would trust.

`editor.f:1655` is 71 characters long:

```
          CALL writln('       that imply daily weights less than zero w
     &hen specifying',STDERR,Mt2,F)
```

A fixed-form character constant continued across lines is the concatenation of
columns 7-72 of each line, and a line shorter than 72 columns is treated as
blank-padded to 72. The initial line's literal therefore ends `...zero w` plus
one blank, and the continuation contributes `hen specifying`. The neighbouring
`editor.f:1663` breaks at a word boundary instead, so there the same pad
supplies exactly the space the text needed and nothing looks wrong -- which is
why this one reads as a typo rather than as a mechanism.

- **What it does to the result.** Nothing numeric. It changes one line of the
  `.err`, which IS a compared surface for a refused spec.

- **Measured, not inferred.** Ran the vendored `x13as_ascii_O2.exe` on an
  `x11regression{variables=(td) b=(-1.5f ...) reweight=yes}` spec; the `.err`
  reads:

```
 ERROR: Cannot specify fixed coefficients for the trading day regressors
        that imply daily weights less than zero w hen specifying
        reweight=yes in the x11regression spec.
```

- **Port:** reproduced verbatim in `xrg_editor_setup`
  (`core/src/specparse/readers_spec.cpp`), with the column arithmetic in the
  comment above it so the next reader does not "correct" it.

- **Pinned by:** `tests/corpus/extra/airline_x11regression-reweight-fixneg`,
  through `test_m1_parse::test_outcome_matches_oracle`, which compares the
  `ERROR:` block against the blessed `.err` line for line. Rewriting the string
  to `zero when specifying` fails that gate.

## CB-39

**`ssmdl.f:159` searches a change-of-regime group title for `'(change from
before '`, a string no title producer in the tree ever writes -- so
`slidingspans{}` plus any change-of-regime regressor halts the oracle.**

- **File:line:** `ssmdl.f:157-160` (and the same typo again at `rdregm.f:25`).
- **Severity:** `fatal`. The run stops with "Program error(s) halt execution"
  and writes no sliding-spans table at all.

`ssmdl` needs the regime date to decide whether the change of regime is defined
over every span, and recovers it from the group TITLE the way the rest of the
program does (`Xaicrg`, CB-entry-84's family):

```fortran
idtpos=index(igrptl(1:nchr),'(before ')+8
IF(idtpos.eq.8)
&     idtpos=index(igrptl(1:nchr),'(change from before ')+20
CALL ctodat(igrptl(1:nchr-1),Sp,idtpos,regmdt,Locok)
```

The idiom is `index(...)+k` with `IF(idtpos.eq.k)` meaning "not found", and it
FALLS THROUGH: if the second search also misses, `ctodat` runs at position 20
regardless.

**The second search always misses.** Every producer of a change-of-regime title
writes `for`, not `from`:

| writer | string |
|---|---|
| `addlom.f:63` | `' (change for before '` |
| `addtd.f:88` | `' (change for before '` |
| `adrgim.f:72` | `' (change for before '` |
| `adrgim.f:178` | `'Leap Year (change for before '` |

and every other READER agrees with the producers -- `regvar.f:334`,
`savmdl.f:346` and `editor.f:1816` all spell `'(change for before '`. Only
`ssmdl.f:159` and `rdregm.f:25` spell `from`.

So for the common title shape `Trading Day (change for before 1955.Jan)`:
`'(before '` is not a substring (the `(` is eight characters to the left of
`before`), the `from` search misses too, `ctodat` is handed position 20 -- the
space before `for` -- and returns `Locok=.false.` with `Idate` left at whatever
`ctoi` made of `" for before..."`. `dfdate` then compares a garbage date, the
`begrgm.le.Sp` arm fires, the change-of-regime NOTE is printed, and the run
halts.

- **What it does to the result.** No sliding-spans analysis is produced for any
  spec that combines `slidingspans{}` with a change-of-regime regressor
  (`regression{variables=(td/1955.jan/)}` and the `lom`/`lpyear`/`seasonal`
  equivalents). The `.err` carries only the NOTE, not an `ERROR:` line -- the
  halt shows up on the console and in the ABSENCE of the `.sfs`/`.chs` files.

- **Measured, not inferred.** `oracle/fortran/x13as_ascii_O2.exe` on
  airline + `slidingspans{save=(sfs chs)}` + `regression{variables=(td/1955.jan/)}`:
  console ends `Program error(s) halt execution for ....spc`, and the run
  directory holds `.d10`-`.d16` but neither `.sfs` nor `.chs`.

- **Port:** NOT reproduced. This port walls the arm instead
  (`ssmdl_fix_model`, `core/src/x11/slidingspans.cpp`) and refuses with its own
  message, because reproducing the halt faithfully would mean porting the whole
  change-of-regime block for the sole purpose of arriving at a garbage date.
  The wall is inventoried in `docs/WALLS.md`; when that block is ported, the
  fall-through has to be transcribed with the `from` intact or this bug
  disappears silently.

- **Pinned by:** `tests/corpus/extra/airline_slidingspans-regime-td`, through
  `test_slidingspans_tables::test_slidingspans_halt_matches_oracle` -- which
  derives its case list from the blessed `.stdout.txt` ("Program error(s) halt
  execution") rather than from a name list, and asserts the engine produces no
  span table where the oracle produced none. Deleting the wall fails it.


## CB-40

**`bakusr.f:50` and `:52` displace the SOURCE of the two backup copies instead
of the destination, so the `Rind=1` call reads `Xuserx`/`Usxtyp` past the end of
both arrays and writes what it finds into slot 0 -- leaving slot 1, the one
`addusr(1)` reads, never written by anything.**

- **File:line:** `bakusr.f:49-52`.
- **Severity:** `wrong-numbers` (plus an out-of-bounds READ). The
  x11regression user regressors are restored with an identically ZERO data
  matrix and type 0 instead of their own values.

`bakusr` keeps two slots of everything, indexed by `Rind` -- 0 for the regARIMA
design, 1 for the x11regression one. Three of the five writes displace the
DESTINATION and are right:

```fortran
      disp=((PUREG+1)*Rind)+1
      CALL cpyint(Usrptr(0),PUREG+1,1,Usrpt2(disp))
      Ncusx2(Rind)=Ncusrx
      Usrtt2(Rind)=Usrttl
```

The other two displace the SOURCE:

```fortran
      disp=(PUSERX*Rind)+1
      CALL copy(Userx(disp),PUSERX,1,Userx2)
      disp=(PUREG*Rind)+1
      CALL cpyint(Usrtyp(disp),PUREG,1,Usrty2)
```

`Userx` and `Usrtyp` here are the DUMMY arguments, dimensioned `PUSERX` and
`PUREG`. For `Rind=0` the two readings coincide (`disp` is 1 either way) and
nothing is wrong. For `Rind=1` -- `ssxmdl.f:146`, `sspdrv.f:159`,
`revdrv.f:317`, `revdrv.f:626` and `editor.f:1543`, i.e. any
`x11regression{usertype=}` under `slidingspans{}` or `history{}` -- it reads
`Xuserx(PUSERX+1 …)` and `Usxtyp(PUREG+1 …)`, one element past the end of each,
for a whole array's length, and writes the result over SLOT 0.

- **Measured, not inferred.** A bounds-checked build of the vendored sources
  (`gfortran -fcheck=bounds`, scratchpad copy -- the vendored tree is never
  edited) on airline + `slidingspans{}` + `x11regression{user=(u1)
  usertype=(user)}`:

  ```
  At line 50 of file bakusr.f
  Fortran runtime error: Index '53041' of dimension 1 of array 'userx'
  above upper bound of 53040
  ```

  and an instrumented build prints the source displacement and the values it
  picks up (`/cx11rd/` places `Cvxalf`/`Cvxrdc` after `Xuserx`):

  ```
  BAKUSR Rind= 1 Ncusrx=  1 nstored=  1 Buser= -0.5810479
  BAKUSR src Userx(disp..) disp=   53041  0.5000000E-01  0.5000000  0.000000
  BAKUSR src Usrtyp(disp..) disp=      53        1        3        0
  ADDUSR Rind= 1 Ncusx2=  1 disp=   53041   0.000000  0.000000  0.000000
  ADDUSR after Rind= 1 Ncusrx=  1 Usrtyp= 0 0 0 Userx= 0.000000 …
  ```

  `Buser`/`Fxuser` (displaced correctly) come back right; `Userx` and `Usrtyp`
  come back as zeros, because slot 1 was never written.

- **What it does to the result.** The restored x11regression user column is
  identically zero and lands in the generic `User-defined` group whatever its
  `usertype=` said. That is DETERMINISTIC -- the garbage goes to slot 0, which
  only `addusr(0)` reads -- so the observable is reproducible and the oracle's
  own `_O0` and `_O2` binaries agree on it, as does a fresh
  `gfortran -O2 -fno-automatic` rebuild.

- **Port:** the EFFECT is reproduced (`bakusr` in
  `core/src/regarima/usrbak.cpp` leaves slot 1 alone for `rind==1`, so
  `addusr(1)` restores zeros). The slot-0 CLOBBER is **not** reproduced -- it
  cannot be; the bytes are whatever the link map put after `/cx11rd/` -- and
  until 2026-08-09 the combination that reads it was refused with its own
  message. That wall is gone: it refused a run the oracle completes, and the
  refusal was never the conservative choice it looked like.

  **What replaced it, and what was measured to justify that** (instrumented
  scratchpad build, on `extra/airline_x11regression-reg-user-bothfixed`):

  | probe | d10/d11/d12/d13/b16/c16 | aape |
  |---|---|---|
  | stock oracle | baseline | baseline |
  | poison `Userx` with 1e30 right after `addusr(0)` | **move** | unchanged |
  | `bakusr` fixed to displace the DESTINATION | **move** | unchanged |
  | `bakusr` made to behave like THIS PORT (slot 0 written only for `rind==0`) | **move** | unchanged |

  So the oracle really does read the garbage, and really does compute its
  within-sample aape before the restore. And yet the engine -- which restores
  the correct backup -- is bit-exact against the STOCK oracle on all 23 gates of
  that spec and all 26 of `extra/airline_slidingspans-reg-x11regression-user-bothfixed`.
  **The port agrees by cancellation, not by faithfulness:** it captures aape
  AFTER the restore where the oracle captures it before, and its x11 factors do
  not re-derive from `Userx` after that point at all. Mutation: zeroing the
  matrix `addusr` restores fails **23 gates**, all of them aape/x11 lines on the
  regression-user specs, and moves nothing on the six tables the oracle's own
  poison moves. Both halves are recorded in the code, because either one
  changing alone breaks the other.

- **Pinned by:** `tests/corpus/extra/airline_x11regression-user-fixed`, and
  pinned BY MEASUREMENT: "fixing" `bakusr` to displace the destination (so slot
  1 holds the real user matrix) fails **21 gates**. The defect's whole
  observable is that the restored column comes back identically zero, and the
  spec's `xrm` golden carries it.

  **What that spec has and the five sliding-spans ones do not: `x11regression{b=}`.**
  A fixed x11reg coefficient is what sets `Userfx` (`gtxreg.f:864`) and leaves
  `Iregfx=2`, which is what routes `x11mdl.f:391/460` through `rmfix`/`addfix`
  and so reaches `addusr(1)` at all -- on the MAIN run, with no span driver
  anywhere. The earlier note here read "nothing downstream of `addfix`'s restore
  in a span reads the x11regression design again", and that was true of the
  spans; the consumers are one phase out, in `x11mdl` itself: the `regvar` at
  `:462` that rebuilds `Xy` from the restored `Userx`, and the `xrm` punch at
  `:499-508` that saves it. The old caveat was a claim about the corpus, written
  as a claim about the program.

## CB-41

**`sspdrv.f`'s per-span user-regressor undo uses ONE buffer for TWO saves and
restores the wrong array's length.**

- **File:line:** `sspdrv.f:154`, `:165`, `:229`.
- **Severity:** `wrong-numbers`, and unreachable in the corpus (see below).

The per-span block saves the fix flags before `chusrg` may change them and puts
them back after the span:

```fortran
       IF(Nusxrg.gt.0)THEN
        CALL copylg(Regfxx,Nbx,1,bfx2)      ! :154  x11regression design
        ...
       IF(Ncusrx.gt.0)THEN
        CALL copylg(Regfx,Nb,1,bfx2)        ! :165  regARIMA design -- SAME bfx2
...
       IF(upusrx)THEN
        CALL copylg(bfx2,Nb,1,Regfxx)       ! :229  Nb, not Nbx
```

Two defects in three lines:

1. `bfx2` is a single `PB`-sized local and both saves write it. A spec carrying
   user regressors in BOTH designs has its x11regression flags overwritten by
   the regARIMA ones before either is restored, so `:229` reinstates the
   regARIMA design's `Regfx` into `Regfxx`.
2. `:229` copies `Nb` elements where the array it is writing is `Nbx` long. The
   two counts are unrelated once the two designs differ in width.

- **Port:** transcribed verbatim -- the shared buffer is modelled by the single
  `ss_user_state::bfx2` member and the restore is sized `nb`
  (`ssp_user_span_undo`, `core/src/x11/slidingspans.cpp`).

- **STILL not pinned by a spec, and as of 2026-08-09 half of it never can be.**
  `extra/airline_slidingspans-x11regression-user-spanzero` is the first spec in
  this corpus that makes `chusrg` fire at all. It needs three things: `fixmdl=no`
  (the default fixes every coefficient, so `chusrg.f:43`'s `.not.Regfx(i)` test
  rejects every column), a non-fixed `regression{user=}` column beside an
  `x11regression{usertype=}`, and an x11reg user column shaped so that SPAN 1's
  strided window differences to zero -- the values `chusrg` reads are the
  x11regression ones, because `loadxr.f:43`'s copy back into `Xuserx` is
  commented out. It reaches both `bfx2` blocks with `upusrx` true. Neither
  defect fires.

  **(a), the shared buffer, is unreachable BY CONSTRUCTION.** Both branches call
  `chusrg` on the SAME regARIMA arrays (`Nb`, `Rgvrtp`, `Regfx`) with the same
  predicate, and the first call fixes every column that qualifies -- so the
  second can never find one. `upusrx` true implies `upuser` false; and with
  `Nusxrg == 0` the first branch does not run at all and `bfx2` has a single
  writer. The two saves can never both be live. That is a theorem about the
  program, not a gap in the corpus, and it will not change with a better spec.

  **(b), the `Nb`-instead-of-`Nbx` restore, now RUNS and is still not
  observable.** Measured on that spec: mutating `Nb` to `Nbx` fails 0 gates, and
  so does mutating the restore to write `Regfxx` all-true. `Regfxx` is not read
  again after the span loop. What is wanted is a phase AFTER `sspdrv` that reads
  it -- `slidingspans{}` and `history{}` in one run is the next thing to try.

## CB-42

**`x11ref.f:88` reads `Trumlt`, a LOGICAL local that nothing in the program ever
assigns.**

- **File:line:** `x11ref.f:19` (declaration), `x11ref.f:88` (read).
- **Severity:** `undefined-behaviour`. It selects between two different
  normalisations of the holiday factor, so on a build where the slot happens to
  come up `.false.` a TD + holiday x11regression adjustment differs.

```fortran
      LOGICAL Psuadd,Axruhl,Trumlt,Calfrc,Xhlnln     ! :19
      ...
      IF(Holgrp.gt.0)THEN
       IF((Muladd.eq.2.or.Trumlt).and.Tdgrp.gt.0)THEN   ! :88
        CALL mulref(Nrxy,Fcal,Fhol,Xdev,Xnstar,DNOTST,F)
        CALL mulref(Nrxy,Fhol,Fhol,Xdev,Xnstar,DNOTST,T)
       ELSE
        CALL mulref(Nrxy,Fcal,Fhol,Xdev,Xnstar,ONE,F)
       END IF
      END IF
```

`Trumlt` is not a dummy argument of `x11ref` and is in none of its four
INCLUDEs (`srslen.prm`, `model.prm`, `xrgum.cmn`, `xtdtyp.cmn`). Grep the whole
tree: it is declared here and read here, and assigned nowhere.

- **Measured, not inferred.** Instrumented build of the vendored sources
  (scratchpad copy -- the vendored tree is never edited), printing the condition
  inputs at `:87` on `extra/airline_x11regression-aictest-easter8`:

  ```
  DBGREF Holgrp= 2  Tdgrp= 1  Stdgrp= 0  Trumlt= T  Muladd= 0  Easidx= 0  Nb= 8
  ```

  Both the earlier gate-level inference (entry 76: every TD + holiday spec is
  bit-exact taking the `.true.` arm) and the direct read agree.

- **Reachability.** The condition is `.and.Tdgrp.gt.0`, so with no trading-day
  group it is false whatever `Trumlt` holds and the defect cannot reach the
  no-TD path. `x11aic.f:328` and `:449` read the same uninitialized name into
  `tdhol`/`xm`, under the same `Tdgrp.gt.0` conjunction.

- **Port:** reproduced by taking the `.true.` arm
  (`core/src/x11/x11reg.cpp`, `x11ref_td`), with the caveat recorded there that
  this is one build's value and not a language guarantee -- the gates are what
  pin it.

- **Pinned by:** every gated TD + holiday x11regression spec; the `.false.` arm
  is a different normalisation and fails them.
