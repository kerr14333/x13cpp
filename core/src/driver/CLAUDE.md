# Drivers — working notes

The measured records for this subsystem are in **`docs/M5_PORT_NOTES.md`**.
Read the relevant entry BEFORE changing anything here; each one is a bug
already paid for once.

| entries | what they cover |
|---|---|
| **1, 2, 26–30** | `history{}` — the forecast-error and model histories, `Fixper`/`Indrev`, the option surface + `fixreg=`, per-span `xrgdrv` + `fixx11reg=`, the held-back outliers (`rmotrv`/`chkorv`), and the alternate revision targets |
| **10** | `series{modelspan=}` — the narrow/restore dance and its two C++-only seams |
| **17, 48, 49** | `composite{}` — the X-11 indirect adjustment, the direct+indirect diagnostics, and the SEATS branch (`agr3s.f`) |
| **108** | a fit that never converged returned `OUTCOME: OK` — `arima.f:1046`'s `IF(Convrg)` and its bare `ELSE CALL abend` at `:1216` (NOT `:525`, which belongs to the `.NOT.Hvmdl` arm), the `prterr.f`/`itrerr.f`/`prarma.f` message half that was a stub dropping both its arguments, `arima.f:711`/`:772`'s missing prterr calls on the EXPLICIT-model path, and the harness table guard that keyed on span pointers the pre-MODEL editor sets |
| **109** | the `.err` of a run that COMPLETES was compared by no gate at all -- `spcrsd.f`'s three peak WARNINGs + `spcdrv.f:594-617`'s two, and the reason the residual spectrum now runs from `run_pre_model` (`run_residual_spectrum`): its numbers were bit-exact either way, its WARNING is a line in the Mt2 stream and the oracle writes it before x11pt2. Also CB-45, where the ORACLE's process is killed by the Fortran runtime and this engine ran on past it, and `oracle_died()` -- the third outcome class for a blessed run |
| **95** | `composite{}` PSEUDO-ADDITIVE (`agr3.f:267-272`) — plus the two unported refusal blocks probing it exposed (`editor.f:2508-2545`, `editor.f:788-847`, both in `gtinpt`'s tail because the M1 gate is parse-only), the composite harness's Mt2 channel, and `Lindot`: a `gtinpt.f:311` DEFAULT the port never wrote, which made four agr3 guards dead code |
| **94** | `composite{}` + `force{}` — agr3's forced/rounded tail (`agr3.f:426-547`), and the NINTH span-replay save/restore miss: the INDIRECT `id11.f`/`id11.3y.f` F-test pair, where the direct twin had been saved since entry 40. `Iagr` is 5 during a replay, so a `history{}` span files its own DIRECT D11 under the `i` keys |
| **31** | fixed / initial coefficients — `regression{b=}`, `arima{ar= ma= diff=}`, and the rmfix/addfix seam |
| **32** | the C / R / Python ABI, and the host floating-point-mode finding |
| **44** | the model-only diagnostics path (`x11ari` with neither `Lx11` nor `Lseats`) |
| **83** | `slidingspans{}` CLOSED bit-exact — `Setpri`, the per-span `ssprep` chain, `fixreg=`'s non-effect, and `fixmdl=clear` |
| **86, 87** | `slidingspans{fixreg=(outlier)}` (setssp's `Otlfix` is the one fixreg flag that outlives setup) and the x11regression outlier block `run_x11_span` now runs per span (`ssx11a.f:99-154`, inside the `loadxr` swap) |
| **90** | `history{x11outlier=no}` — CLOSED with no code: entry 87 restored `x11mdl.f:424`'s two missing clauses and measured zero because `Irev` was still 1, entry 89 set `Irev`, and the arm has been bit-exact since. Both arms gated; the head-of-analysis `rmatot` is provably redundant with the per-span one |
| **89** | `history{}` — `Irev` finally advances to 4/5 (`revdrv.f:387/761`), so `getrev` runs where the oracle runs it (inside `x11pt3`/`seatdg`) instead of `run_history` re-reading `/x11srs/` afterwards; `revdrv.f:416-427`'s past-`Endsa` `Lx11=F` (`run_x11_span`'s `lx11_span`); `errhdr` |
| **88** | `slidingspans{}` + user regressors — `bakusr`/`addusr`/`dlusrg`/`chusrg` (new `regarima/usrbak.cpp`), `run_x11_span`'s `ssusr` hook for `sspdrv.f:145-174`, and the SEVENTH span-replay save/restore miss: `/orisrs/ Stoap`, the regression-adjusted original the `b1` table is punched from |
| **96** | `editor.f:1348-1350` / `:1541-1544` — the two `bakusr` calls the port never had, so a MAIN run restored from a backup nobody took: `regression{user= b=…f}` came back `nreg: 0` against the oracle's 2. The rind-0 call lives in `run_pre_model`, the rind-1 one in `xrg_editor_setup`; the resulting order inversion is unobservable behind CB-40's refusal |
| **97** | the x11regression transparent pass with a non-empty regARIMA design — what `xrgdrv`'s support guard was really hiding, plus two compensators removed as redundant (`restor_span` already carries restor.f's design half) and one measured-unreachable transcription (x11pt2.f:851-859's six divsubs) |
| **93** | `x11regression{outlierspan=}` — `Begxot`/`Endxot` join the span-replay save/restore set in `run_x11.cpp` (the EIGHTH time that seam has bitten), and `ssx11a.f:105-106`'s per-span write of them is ported but ungated |
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
