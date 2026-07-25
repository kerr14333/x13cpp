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
