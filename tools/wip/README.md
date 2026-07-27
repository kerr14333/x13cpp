# Work in progress — spectrum peak diagnostics

**Parked, not wired.** These two files are a first pass at the spectrum peak
canaries described in `tools/spectrum_peaks_scouting.md`. They live here rather
than in `core/src/driver/` because that directory is CMake-globbed: leaving them
there would compile dead code into the engine.

`spectrum_peaks.{hpp,cpp}` ports, from the oracle:

* `shlsrt.f` (the Census shell sort) and `mkmdsx.f` (the decibel-aware median)
* `mkpeak.f`'s monthly / default-frequency / `Peakwd==1` constant tables, and
  `mkfreq.f`'s ENHANCED 67-point grid (each trading-day peak plus its
  ±Peakwd limits inserted into reserved slots, the 61 base frequencies filling
  the rest in order)
* `smpeak.f` (per-frequency peak height, `nopeak`, the family's dominant
  frequency) and `mxpeak.f` (the dominant frequency across both families,
  which reports a label only when the winner is also the spectrum's global
  maximum)
* `svpeak.f`'s median/range and the orchestration of the above

Written but NOT reviewed against the oracle's numbers, and NOT gated.

## What was tried and reverted

Wiring it into `run_spectrum` and emitting from `x13run_x11`. Reverted at the
pause because it was unvalidated and because the gate change has a real cost.
Three things were learned and are worth keeping:

1. **`run_spectrum` returning early unless `spectrum{}` is present is the
   actual bug** — `x11ari.f:282-287` gates `spcdrv` on `IF(Ny.eq.12)` alone.
   Removing that early return is the first half of the fix.
2. **`ctx.rho.peakwd` is left at `prm::NOTSET`** (`readers_spec.cpp:2854`); the
   port never ported `gtinpt.f:1286-1287`'s resolution (`Peakwd=1`, or 3 for
   quarterly). Anything keyed on Peakwd silently declines until that lands.
   Resolving it inside `run_spectrum` worked but belongs at the parse tail.
3. **Making the spectrum run on every monthly spec is expensive.** The parity
   suite went from ~200s to over 600s, because the AR spectrum is then computed
   for four series on every one of ~330 specs — and the peak pass adds a FIFTH
   evaluation on the enhanced grid. Before landing this, either hoist the
   enhanced-grid evaluation to reuse the base fit, or accept and document the
   new suite runtime. Do not discover this in CI.

After the revert the branch tip is green: parity 4724 passed / 0 failed,
ctest 11/11.

## Still unverified

The engine produced NO `spcori.*` rows even after the peakwd fix, so there is at
least one more early return or false precondition between `run_spectrum`'s entry
and the peak block (`out.have_sp0` and `grid.ok` are the two candidates that
were not yet instrumented). Start there.
