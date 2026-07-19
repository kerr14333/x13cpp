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

---

_Append new entries as they are found while porting. Keep each pinned to a test._
