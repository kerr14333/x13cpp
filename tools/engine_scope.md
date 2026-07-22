# Engine conversion scope — what remains (2026-07-20)

Read-only survey of the Fortran→C++ port. Priority #1 is finishing the `core/`
engine (per CLAUDE.md); this ranks the remaining Fortran so we pick milestones by
value, not by file order. No code was changed to produce this.

## Snapshot — what is green today

Parity-passing paths (see `tests/parity/`):

- **Spec parse** (`specparse/`, 13 cpp) — M1/M2 green.
- **Transforms** (`transform/`) — log/logistic/Box-Cox.
- **regARIMA** (`regarima/`, 14 cpp) — estimate, forecast, outlier detection
  (`outlier.cpp`), TD/holiday regressors (`td6var`/`td7var`/`adhol`), prior adj,
  mean. M3 green.
- **automdl** (`automdl/`, 9 cpp) — iddiff/amdid + aictest block-1 (tdaic/easaic)
  with the a0/ismd0 revert. M4 green.
- **X-11 spine** (`x11/`, 5 cpp) — x11pt1→pt2→pt3 mainline decomposition;
  b1/d10/d11/d12/d13 bit-exact on the no-model path AND (as of this session, fable's
  Finhol fix) near-exact on the airline aictest model path.
- **SEATS** (`seats/`) — decomposition DONE (update 2026-07-21): the canonical
  model-based decomposition (SIGEX/ESTBUR/AUTOCOMP, WK filters, spectrum) is
  ported; every SEATS corpus spec's s10-s18 tables gate bit-exact, including the
  near-non-invertible fixed-airline variants (CALCFX residual port). What remains
  in SEATS is diagnostics only (slidingspans/history), not decomposition.
- **tables/** — save-table numeric emit scaffolding.

## Gaps, ranked by value to a usable library

### 1. SEATS decomposition — DONE (closed 2026-07-21)
The canonical model-based decomposition (Wiener–Kolmogorov filters, canonical
trend/seasonal/irregular, pseudo-spectrum, the `seats*` family: SIGEX/ESTBUR/
AUTOCOMP/SPECTRU/decompspectrum + the CALCFX forecast-residual port) is ported;
every SEATS corpus spec's s10-s18 gate bit-exact (~5e-15). This was "the biggest
hole" in the 2026-07-20 survey; it is now closed. Remaining SEATS = diagnostics
(slidingspans/history), covered under §2/§3.

### 2. x11pt3 Part-E and optional branches (~24 `not_ported` in `x11parts.cpp`)
The X-11 mainline finals work, but many switches fatal-out:
- Part-E fold-ins: outlier/user/prior/AO/TC re-adjustment of the final series.
- Force yearly totals (`force/` empty; `qmap`/`qmap2`).
- Pseudo-additive D10/D11 (`Psuadd`), seasonal shrinkage (`Ishrnk`), rounding.
- Kfulsm summary-measures / full-seasonal branches.
- Sliding-spans (`ssrit`) and revisions-history (`getrev`) stores.
These are individually small; each un-stubs a documented spec option.

### 3. Diagnostics — `diag/` empty
No spectrum, sliding-spans, revisions history, or M/Q quality stats. Not needed
for *numbers* but needed for the "nice interfaces + all sorts of plots" goal
(priority #3) — the plots read these. Bridges #1-engine and the wrapper work.

### 4. Regressor coverage (`getreg_vars.cpp`, 11 `not_ported`)
User-defined regressors, trig seasonal (`adsncs`), change-of-regime
(`adrgim`/`gtrgpt`), AOS/LSS outliers (`rdotls`), automatic-outlier ordering
(`rdotlr`). Each blocks a class of real specs. Independent, incremental.

### 5. In-flight residuals (small, already scoped)
- airline aictest-x11: uniform ~1e-6 (TD-beta-level) residual — fable is on it.
- aictest non-default series (expgs/payems/unrate): need automd nloop/tstmd1
  round-2 finalization.
- fixed-airline: post-outlier TD forecast not bit-exact
  ([[x13cpp-outlier-forecast-deferred]]).

## Recommended milestone order

1. **Close the aictest residuals** (in flight) — finishes the model→X-11 story;
   small, unblocks the first calendar-regressor series end-to-end.
2. **x11pt3 Part-E + force totals** — completes X-11 as a *whole* engine; many
   users only ever run X-11. Medium, mechanical, well-bounded by `not_ported` keys.
3. **SEATS decomposition** — the other half of the product. Largest; do it as its
   own multi-step milestone once X-11 is fully closed.
4. **Diagnostics (`diag/`)** — pairs naturally with starting the R/Py wrappers and
   plotting (priorities #2/#3); the plots need these tables.
5. **Regressor coverage** — fold in opportunistically as corpus specs demand.

Rationale: keep each step landing a *complete* user-visible capability (a whole
spec option or engine), not a half-wired path. X-11 first (nearly done), SEATS
second (large), diagnostics as the bridge to the wrapper/plot work.
