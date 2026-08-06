# Drivers — working notes

The measured records for this subsystem are in **`docs/M5_PORT_NOTES.md`**.
Read the relevant entry BEFORE changing anything here; each one is a bug
already paid for once.

| entries | what they cover |
|---|---|
| **1, 2, 26–30** | `history{}` — the forecast-error and model histories, `Fixper`/`Indrev`, the option surface + `fixreg=`, per-span `xrgdrv` + `fixx11reg=`, the held-back outliers (`rmotrv`/`chkorv`), and the alternate revision targets |
| **10** | `series{modelspan=}` — the narrow/restore dance and its two C++-only seams |
| **17, 48, 49** | `composite{}` — the X-11 indirect adjustment, the direct+indirect diagnostics, and the SEATS branch (`agr3s.f`) |
| **31** | fixed / initial coefficients — `regression{b=}`, `arima{ar= ma= diff=}`, and the rmfix/addfix seam |
| **32** | the C / R / Python ABI, and the host floating-point-mode finding |
| **44** | the model-only diagnostics path (`x11ari` with neither `Lx11` nor `Lseats`) |
| **83** | `slidingspans{}` CLOSED bit-exact — `Setpri`, the per-span `ssprep` chain, `fixreg=`'s non-effect, and `fixmdl=clear` |
| **86, 87** | `slidingspans{fixreg=(outlier)}` (setssp's `Otlfix` is the one fixreg flag that outlives setup) and the x11regression outlier block `run_x11_span` now runs per span (`ssx11a.f:99-154`, inside the `loadxr` swap) |
| **88** | `slidingspans{}` + user regressors — `bakusr`/`addusr`/`dlusrg`/`chusrg` (new `regarima/usrbak.cpp`), `run_x11_span`'s `ssusr` hook for `sspdrv.f:145-174`, and the SEVENTH span-replay save/restore miss: `/orisrs/ Stoap`, the regression-adjusted original the `b1` table is punched from |
| **85** | the sliding-spans held-back outliers — `run_x11_span`'s `ss_outliers` hook (`ssx11a.f:229-270`), why `history{}` must NOT take it, and `ssprep`'s `Lx11` argument |
| **78, 79** | `slidingspans{}` + `x11regression{}` — the pairing no spec had; then the per-span calendar factor it exposed, closed by `ssxmdl`'s `fixx11reg` default TOGETHER with the `Ixreg` demote a previous session had measured alone and rejected |
| **47, 50–52** | `pickmdl{}` / `automx.f`, amdfct's out-of-sample and backcast arms, and the per-candidate AIC tests |

## The traps most likely to bite here

- **A structural change that is not mirrored into the `ssprep` snapshot is
  undone by the first span's `restor`** — for `fixmdl` and for
  `history{fixreg=}`. **It is NOT true of `slidingspans{fixreg=}`**, and
  assuming it was cost an increment: `ssmdl.f:53`'s `rvfixd` writes only the
  live `Iregfx`/`Regfx`, the oracle's own `restor` undoes it, and the option
  therefore fixes no coefficient at all — its entire effect is the `Itd`/`Ihol`
  demote. Find the snapshot write in the Fortran; do not infer it (entry 83).
  **And now read the rest of that sentence (entry 86): "no coefficient at all"
  holds only while the design is unchanged.** One held-back outlier sets
  `regchg`, `ssmdl.f:358-373` re-snapshots, and the re-snapshot carries the
  POST-`rvfixd` `Iregfx`/`Regfx` into `Irfx2`/`Regfx2` — so `restor` reinstates
  the fixings instead of erasing them, and 192 `sfs` lines move. A measured
  inertness is scoped to the probe spec exactly as much as a measured effect is.

- **`arima.f:1430`'s `CALL ssprep` is unconditional, so span replays CHAIN.**
  Every span's estimation re-snapshots `Ap2`/`Bb`/`Var`, and the next span's
  `restor` therefore starts from its PREDECESSOR, not from the main run.
  Missing that call leaves span 1 bit-exact and every later span drifting a few
  1e-7 — which reads like an estimation-tolerance floor and is not one
  (entry 83).

- **`ssprep`'s `Lx11` argument was dropped from the port's signature, and it
  only became a defect when a SECOND caller appeared.** `sspdrv.f:218` passes
  `ssprep(T,F,F)`; it runs AFTER `x11pt2` has resolved the auto-select `Lter`
  sentinels, so snapshotting them hands the next span a filter length chosen for
  its predecessor (span 1 bit-exact, spans 2-4 out — entry 85). Every earlier
  caller was safe by PLACEMENT, not by the flag. Same family as the `Ksdev`
  restore below: identical code, correct at one call site, wrong at the next.

- **`Setpri` must NOT be re-established inside a span.** `Setpri=Pos1bk` is
  `editor.f:851` and nothing repeats it; `Adj` stays anchored at the main run's
  `Begadj` while `Lsp` slides `Pos1ob`, and `x11int`'s POSITIONAL copy into
  `Sprior` is what keeps the prior factors date-aligned. Re-anchoring it put
  the wrong year's leap factor on every February of the SA series (entry 83).
  General form: an editor assignment repeated per span is a bug, and a per-span
  assignment hoisted to the editor is the same bug mirrored.

- **A per-span pass must restore everything the MAIN path's editor would
  re-derive**, not merely what `restor` restores. `Lmsr`, `Kersa`, `Lstabl`,
  `L3x5`, `Nterm`, `Lterm`, `Ksdev` and `Priadj` are all resolved in place by
  a transparent x11pt1/x11pt2 pass and have no editor block behind them on a
  span. Missing `Priadj` alone put every February of D11/D16 off by the
  leap-year factor behind an `OUTCOME: OK`.

- **Order between the span drivers and the composite tail is load-bearing in
  both directions.** `x12run.f` calls `sspdrv`/`revdrv` AFTER `x11ari` (which
  includes `agr2`/`agr3`): `agr2_component` reads D-table buffers a span
  replay overwrites, and the total's `Iagr` only reaches 5 inside `agr2`.

- **Placement of a value's capture matters as much as its computation.**
  `ctx.est_nefobs` and `ctx.resid_begdate` must be recorded at ESTIMATION
  time, while a `modelspan` still has `Begspn`/`Nspobs` narrowed — `setspn.f`
  widens them back afterwards. Deriving either later slides the result.
