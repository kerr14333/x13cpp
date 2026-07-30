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
| **47, 50–52** | `pickmdl{}` / `automx.f`, amdfct's out-of-sample and backcast arms, and the per-candidate AIC tests |

## The four traps most likely to bite here

- **A structural change that is not mirrored into the `ssprep` snapshot is
  undone by the first span's `restor`.** This is the single most repeated bug
  in the span drivers — it caught `fixmdl`, `fixreg`, and the regression half
  of the snapshot. Setting only the LIVE model before the loop is not enough.

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
