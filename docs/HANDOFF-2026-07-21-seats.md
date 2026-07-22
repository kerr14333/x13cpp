# Handoff — SEATS seasonal decomposition session (2026-07-21)

This document hands off the state of the X-13ARIMA-SEATS C++ port after a session
focused on the SEATS canonical decomposition (the `s10`–`s18` save tables). It
covers what changed, the current parity position, the open items with concrete
re-entry points, and the working rules a new contributor needs.

## 1. Current position

**Parity gate: 542 passed / 14 skipped / 45 xfailed / 0 failed** (538 after the
SEATS CALCFX closure below; +4 from the later slidingspans{} closure -- all 4
spans sfs+chs bit-exact via the per-span `xtrm.ksdev` reset in run_x11_span).
Unit suite
10/10. Oracle Fortran tree clean (`git status --porcelain oracle/fortran/*.f`
empty). Both oracle binaries (`x13as_ascii_O0.exe`, `x13as_ascii_O2.exe`) rebuilt
from pristine sources.

The 482 → 526 session closed most of the seasonal decomposition path for the
corpus. **Update (later, same day):** the last two open specs below —
`payems_fixed-airline-seats` and `expgs_fixed-airline-seats` (open items 1 & 2)
— were subsequently closed with a faithful port of Fortran CALCFX
(`ansub1.f`, STEP 3B–8) as `calcfx_last_residuals` in `estbur.cpp`, replacing
`armafl_last_residuals` for the near-non-invertible fixed-airline seasonal MA.
That brought the gate to **538 / 14 / 49 / 0** and made the **SEATS
decomposition corpus fully bit-exact** (every SEATS spec's s10–s18 tables now
gate, ~5e-15).

## 2. What was accomplished this session

All changes are bit-exact against the `-O0` Fortran oracle unless noted.

1. **payems_seats SEATS bias — closed.** The uniform `+2.1978124351562656e-06`
   trend/irregular offset was the **SEATS multiplicative bias correction**
   (`oracle/fortran/sigsub.f:1539-1585`, gated `bias==1`, the default for the log
   path per `ansub9.f:1118`) missing from the log back-transform. Ported
   `bias1c`/`bias2c`/`bias3c` into `core/src/seats/estbur.cpp`'s `is_log` block.
   payems_seats `s12`/`s13` now gated.

2. **airline_seats — closed (first full-seasonal, npsi!=1, spec gated).** Root
   cause of a uniform 0.89% `s12`/`s13` offset: `ir_i`/`sa_i` in `estbur.cpp`
   used the no-seasonal formula (`ir = z - trend - cycle`, `sa = z`)
   unconditionally. The oracle's seasonal branch (`ansub3.f:537/542/546`) is
   `ir = z - sc - trend - cycle`, `sa = z - sc`. The output tables recompute `sa`
   correctly (`out.sa = zorig/out.sc`), so `s11` stayed exact and hid the bug —
   but `ir_i` feeds `bias2c = mean(exp(ir))`, which was inflating the trend
   `bias3c`. Fixed to the unified form (`sc_i==0` when npsi==1, so unrate/payems
   untouched). All 6 airline_seats tables gated.

3. **Fixed-model variants.** `airline_fixed-airline-seats` all 6 tables gated
   (same machinery). `unrate_fixed-airline-seats` (additive + real seasonal) all
   6 gated after wiring the additive factor convention (see #4).

4. **Additive-domain seasonal-factor split.** The three "factor" tables diverge
   in additive (no-transform) mode: `s18 = z/sa` (ratio), but
   `s10 = s16 = z - sa` (additive difference). All three equal `z/sa` in log
   mode. Added `EstburResult::seasonal_add` (feeds `s10`/`s16`) alongside
   `seasonal_factor` (feeds `s18`); the harness dumps them separately. Added an
   `ATOL = 1e-9` floor to `tests/parity/test_seats_tables.py` (numpy-allclose
   style) so tables that legitimately cross zero (`s10`/`s13`/`s16`) can gate on
   absolute error — it only forgives zero-crossings, never masks a real (~1e-3)
   discrepancy.

5. **Forecast-extension model fix (correctness, partial parity).** The SEATS
   forecast/backcast extension of `z` (`fcast_extend`) was using the
   SEATS-**capped** `thstar` (seasonal MA snapped to the `xl=0.99` invertibility
   bound). The oracle extends `z` in regARIMA FCAST using the **uncapped
   estimated** MA (e.g. 0.978), before SEATS caps. Now stores the raw
   (pre-TRANS0) MA coefficients on `SeatsModelOrders` (`th_raw`/`bth_raw`) and
   builds `thstar_est = conv(theta_raw, Theta_raw)` for the extension. Reduced
   `payems_fixed` diffuse error 6.7e-5 → 1.7e-5; **no regression** on any gated
   spec (non-capped models have `th_raw == th`, so `thstar_est == cd.thstar`).

Also logged a latent Census typo (`ansub7.f:474/481` `0.5d0*2D0` for `0.5*pi`,
dead on the s-table path) in `tools/census_bugs.md`.

## 3. Files changed (C++ / tests — permanent)

- `core/src/seats/estbur.cpp` — bias correction in `is_log` block;
  `ir_i`/`sa_i` seasonal-aware formula; `seasonal_add` (additive vs log factor);
  `fcast_extend` now takes `const double* thstar`, called with the estimated
  `thstar_est`.
- `core/src/seats/estbur.hpp` — added `EstburResult::seasonal_add`.
- `core/src/seats/model_decode.hpp` / `.cpp` — added `th_raw`/`bth_raw` (raw
  pre-TRANS0 MA) to `SeatsModelOrders`, populated before the TRANS0 cap.
- `tools/x13run_seats.cpp` — `s10`/`s16` now dump `seasonal_add`; `s18` still
  dumps `seasonal_factor`.
- `tests/parity/test_seats_tables.py` — `ATOL` floor; new gated `(base, tag)`
  entries for airline_seats, airline_fixed, unrate_fixed.

The oracle tree has **no** committed changes — all instrumentation used during
investigation (spectrum.f/ansub3.f/sigsub.f probes) was reverted via
`git checkout` and both exes rebuilt from clean sources.

## 4. Open items (with re-entry points)

1. **payems_fixed-airline-seats — CLOSED (all 6 tables gated, ~5e-15).** The
   remaining gap was the residual **seeds** `aFwd` (from `armafl_last_residuals`,
   which diverges from the oracle for the near-non-invertible seasonal MA,
   bth ~ -0.978 capped to -0.99). Closed by porting Fortran CALCFX
   (`ansub1.f:964-1471`, STEP 3B–8) as `calcfx_last_residuals` in `estbur.cpp`;
   both the seeds and the FCAST recursion now use the CAPPED canonical model
   (`-cd.thstar`), with seasonal-then-regular differencing. Codex-reviewed as
   faithful (two minor fixes applied: differencing order, ith buffer sizing).

2. **expgs_fixed-airline-seats — CLOSED (all 6 tables gated, ~5e-15).** Same
   near-non-invertible seasonal-MA root cause and same `calcfx_last_residuals`
   fix as item 1 (the earlier ~7e-4 was the same capped-MA residual divergence,
   not a distinct problem). These two were the last xfailed SEATS decomposition
   specs; the SEATS decomposition corpus is now fully bit-exact.

3. **unrate_fixed s13** currently gated via the ATOL floor (abs ~4e-12, rel
   blows up at zero crossings) — fine, but note the ATOL dependence.

Broader SEATS fronts still open (pre-existing, not this session): slidingspans
span-driver (~3% off), history{} diagnostics.

## 5. How to build and test

**Windows / PowerShell only** for build + oracle (the Bash tool cannot run the
Windows `.exe`). Use `python`, not `python3`.

```powershell
# C++ build + unit tests
& .\tools\build.ps1

# Full parity suite
python -m pytest tests/parity -q

# One SEATS spec by hand (harness prints s10-s18, MDC_*, DECODE_*)
.\build\x13run_seats.exe tests\corpus\generated\airline_seats.spc

# Oracle rebuild from pristine sources (if an oracle exe is stale). build.ps1
# in oracle/ aborts on gfortran stderr warnings under ErrorActionPreference=Stop;
# drive gfortran directly instead:
cd oracle\fortran
$env:Path = "C:\rtools44\x86_64-w64-mingw32.static.posix\bin;" + $env:Path
Get-ChildItem *.o -EA SilentlyContinue | Remove-Item -Force
$files = (Get-Content objs.txt) | ForEach-Object { $_ -replace '\.o$','.f' }
& gfortran -O0 -ffp-contract=off -std=legacy -fno-automatic -c @files 2>$null
& gfortran -O0 -o x13as_ascii_O0.exe @(Get-Content objs.txt) 2>$null   # repeat -O2 for O2 exe
Get-ChildItem *.o -EA SilentlyContinue | Remove-Item -Force
```

## 6. Working rules / gotchas for the next contributor

- **No git commits** unless explicitly asked. Development runs on an uncommitted
  working tree.
- **The oracle tree must end every session clean.** Any Fortran instrumentation
  is temporary: after comparing, `git checkout -- oracle/fortran/<file>.f`,
  rebuild both exes, and confirm `git status --porcelain oracle/fortran/*.f` is
  empty. Editing a `.f` with an editor that re-encodes non-ASCII bytes (the
  Spanish comments) or trailing whitespace will show the file as modified even
  after a "revert" — use `git checkout`, not a manual re-edit.
- **The oracle is the source of truth.** The reliable localization method this
  session (used repeatedly): add a `write(6,...)` probe to the relevant oracle
  `.f`, rebuild `-O0`, run the spec, compare full-precision values to a matching
  C++ `fprintf(stderr, ...)` probe. It pinned every bug here (enot/estar/ct,
  bias2c, the extension). Which oracle file feeds a given quantity matters:
  `sigex.f:709` shows `SPECTRUM` (spectrum.f) fills `ct/cs/cc/qt1` for ESTBUR;
  `ansub7.f` has a parallel-but-dead copy.
- **Tolerances** (`tests/parity/test_seats_tables.py`): `RTOL = 1e-8`,
  `ATOL = 1e-9`. A point passes if `|v-g| <= RTOL*|g| + ATOL`.
- **Two models coexist in SEATS**: the SEATS-capped model (seasonal MA at the
  `xl` bound) drives the canonical decomposition and the ESTBUR filter
  (`ct`/`cs`/`thstar`); the uncapped estimated model drives the regARIMA forecast
  extension of `z`. Do not conflate them.
- The corpus dir `tests/corpus/generated/` mixes `genspecs.py` output and
  hand-authored specs. **Do not run `genspecs.py` standalone** — it wipes every
  `*.spc` and regenerates only its own subset, deleting hand-authored specs. Add
  new specs by hand and bless goldens.
- Temp files go in `$CLAUDE_JOB_DIR/tmp`.

## 7. Pointers

- `docs/PROJECT_SUMMARY.md` — the overall PoC report.
- `tools/seats_scope.md` — SEATS scope/trace notes.
- `tools/census_bugs.md` — ledger of faithfully-reproduced Census bugs.
- `oracle/run_oracle.py` — golden-bundle generator.
