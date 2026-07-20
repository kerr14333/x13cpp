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
