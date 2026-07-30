# The SEATS FORECAST decomposition — `ansub3.f:353-678` / the `tfd sfd ofd afd yfd` tables

**Status:** **46 of the 52** corpus specs gate bit-exact (most at ~5e-15); 6
remain and are asserted from both sides by
`tests/parity/test_seats_forecast_tables.py`. §3 records the defect that
accounted for 33 of them — ansub3.f's Tramo block, believed unreachable and
therefore unported — and the two sub-causes still open (`TDLIN`, `RESIDUE`).
This file is the map for finishing it — including the dead ends, which cost
most of the time and should not be re-walked.

Status here is NOT authoritative for *what is open* (that is
`tools/SESSION_HANDOFF.md`); what is durable here is *what was measured*.

---

## 1. What it is, and where every piece lives

```
analts.f:2773   fhi = max(fh, qbqMQ + max(qbqMQ, p+bp*mq))      the ESTBUR horizon
analts.f:2778   FCAST(..., z, ..., fhi, ...)                    fills z(Nz+1..Nz+fhi)
sigex.f:426     lfor = max(fh, max(8, 2*mq))                    the SAVE horizon
sigex.f:1395    ESTBUR(..., trend, sc, cycle, sa, ir, ..., fhi) ansub3.f
  ansub3.f:355-361   zero trend/sc/cycle over Nz+1..Nz+lf
  ansub3.f:363-405   FORECAST SEASONALS   (npsi != 1)
  ansub3.f:407-444   FORECAST TREND       (Nchi != 1)   <- dead, see below
  ansub3.f:447-478   FORECAST CYCLE       (varwnc>1e-10 && (ncycth!=0 || Ncyc!=1))
  ansub3.f:519-521   ir(Nz+1..) = 0
  ansub3.f:552-653   the TRAMO block   <- REACHABLE, and now PORTED; section 3.
                     Rewrites z(Nz+1..) = LOG(TramLin), shrinks lf by mq/2, and
                     folds the discrepancy d1 into sc / cycle / ir. Closed 33 of
                     the 39 gaps. ("unreachable, Tramo==0" was wrong.)
  ansub3.f:651-672   trend = z - sc - cycle - ir  (floored at 1e-15*max|z|)
                     sa    = z - sc
sigsub.f:1586-1605  SECOND's antilog for the forecast span (lamd==0 only)
sigex.f:3631-3636   USRENTRY 1410/1411/1413/1409 -> Setftr/Setfsf/Setfcy/Setfsa
seatpr.f:199/351/444/499  savtbl -> LSEFCD{+0,+1,+3,+4} = tfd/sfd/afd/yfd
```

`ofd` (LSEFCD+2, the ORIGINAL-series forecast) has no golden anywhere in the
corpus and is not produced.

**Three horizons, one input.** `fh` is `Nfcst`, and `l_fh = 8`
(`ansub9.f:1609`) is overwritten on every X-13 run by `ansub9.f:1116`
`L_fh=Nfcst`. `lf` (= `fhi`) is what ESTBUR decomposes; `lfor` is what gets
saved. They coincide on every corpus spec.

**The trend forecast block is DEAD.** `ansub3.f:660` overwrites
`trend(Nz+1..Nz+lf)` with the residual `z - sc - cycle - ir`, and `ir` is zero
there. It is transcribed anyway (it is what fills `fortbias` past `lf`), but no
saved value depends on it.

**The port narrows two loops.** The Fortran runs the seasonal/trend forecast
recursions to `kp` (= `PFCST`, the ARRAY bound) and stores into `sc`/`trend`
only when `k <= Nz+lf`; everything past that lands in `forsbias`/`fortbias`,
which feed only `ABIASC`'s over-limit print flag and the annual-average rate
tables. Running to `lf` changes no saved value, and it removes the need for the
`forbias` fallback at `ansub3.f:424-428` (for `k <= Nz+lf`, the index `k-j+1`
is never past `Nz+lf`).

## 2. What landed, and the two real bugs it found

### `editor.f:389-401` — an EXPLICIT short `forecast{maxlead=}` is raised

`gtinpt.f:1151` applies the SEATS floor `max(12,3*Sp)` only as the DEFAULT when
no `maxlead` was given. The oracle ALSO raises an explicit one, in the editor,
with a NOTE. The port had only the first half, so
`*_fixed-airline-seats`, `*_finite-seats`, `airline_seats-qmax-rmod` and
`airline_seats-tabtables` (all `forecast{maxlead = 12}`) ran with `nfcst: 12`
against the golden's `nfcst: 36`. Invisible until now because nothing
downstream of the HISTORICAL decomposition read it. Ported at the tail of
`parse_spec.cpp`, next to the other two editor rules; gated by the row COUNT
assertion in the new test (24 rows vs the golden's 36).

### The forecast trend's bias factor is `bias1c`, where the Fortran reads `bias3c`

`sigsub.f:1594` is literally `trend(k) = EXP(trend(k)) * bias3c`, and that is
wrong for this port by exactly `bias2c`. The reason is a NORMALIZATION
difference, not a transcription choice, and it is only visible here:

- a SEATS decomposition is unique only up to a constant log shift between the
  seasonal and the trend, and the bias block absorbs exactly such a shift;
- this port's raw `sc_i` sits `ln(bias1c)` above the oracle's and its raw
  `trend_i` `ln(bias3c)` below, so the HISTORICAL transform lands on the same
  s10/s11/s12/s13 either way — all four gate bit-exact, on airline with
  `bias1c = 1.00882`, `bias2c = 1.00010`, `bias3c = 1.00893`;
- the FORECAST trend is not built by the filter at all — it is the residual
  `z - sc - cycle` — so it inherits the shift from `sc`, i.e. one factor of
  `bias1c`.

Pinned by two identities that hold across the whole corpus and are asserted on
the GOLDENS (not on the engine) by `test_no_transitory_means_trend_equals_sa`:
`tfd == afd` to the last digit on the 39 specs with no transitory component,
`afd = tfd*yfd/100` on the multiplicative ones that have it, and
`afd = tfd + yfd` on the additive ones.

### Emit conditions

`ansub9.f:71-92` — `Havfsf`/`Havfcy` are `.not.allzro`, so the oracle writes no
file for a slice that is identically zero. That is the ONLY suppression on
`sfd` (an `npsi==1` LOG run still writes it, as a flat 100 — `payems_seats`);
`yfd` additionally needs `sigex.f:3634`'s guard, whose Fortran precedence is
`(varwnc>1e-10 && ncycth>0) || Ncyc>1` — **not** the same test as the ESTBUR
forecast block's `ncycth != 0 || Ncyc != 1`, so a spec can decompose a cycle
and still not save it.

## 3. CLOSED (2026-07-30) -- the TRAMO block is REACHABLE, and it rewrites `z`

Settled by instrumenting the oracle: a probe immediately after the `extZ` copy
(`ansub3.f:115`) and a second at `:660`, on a rebuilt oracle first VALIDATED
against the vendored binary (every table of `generated/airline_seats` PASSED,
0 diffs, including `.tfd`). Build recipe and the exact finding below.

### What the probe showed

| point | `z(Nz+1)` | `extZ(Nz+1)` |
|---|---|---|
| straight after the copy | 8.1852958887756362 | 8.1852958887756362 |
| at `:660` | **8.1816488391166384** | 8.1852958887756362 |

`z` is overwritten between the two, and `lf` drops 12 to 10 in the same window.
`lf = lf - mq/2` is `ansub3.f:571`, which is **inside `if (Tramo.ne.0)`** --
so the block this port documents as "the TRAMO passthrough, unreachable,
Tramo==0" **runs on an ordinary X-13 SEATS run**. That premise was wrong, and
it is the whole bug. Same class as entry 25 in `docs/M5_PORT_NOTES.md`: a
reachability argument never tested against a running oracle.

`exp(8.1816488391166384)` = 3574.74 -- the regARIMA forecast, which is exactly
the value the goldens were measured to imply. So the two facts the previous
version of this section called "true at once and not reconciled" are simply
about two DIFFERENT ARRAYS:

- the filter recursions (`:386`, `:427`, `:469`) read **`extZ`**, and the
  port's `zextFwd` reproduces it -- which is why the historical span is
  bit-exact and why perturbing the extension breaks it;
- `:660`'s residual reads **`z`**, which by then holds `LOG(TramLin)`.

The port's `FCX` lambda serves both, so it feeds the right series to the
filter and the wrong one to the residual.

### What the TRAMO block actually does (`ansub3.f:552-653`)

Four things, in order, none of them ported:

1. `d1(i) = LOG(TramLin(i)) - (trend+sc+cycle+ir)(i)` over the forecast span --
   the discrepancy between the regARIMA forecast and the filter's own
   reconstruction. That is the `ILAM.eq.0` arm; the non-log arm drops the LOG.
2. `z(i) = LOG(TramLin(i))`, and `forbias(i-nz) = z(i)`.
3. **`lf = lf - mq/2`.** Note `nz1` was fixed at `Nz + lf` BEFORE this, at
   `:507`, so the `:660` loops still run to the ORIGINAL horizon while every
   loop in the block below runs to the SHRUNK one.
4. `d1` is folded into exactly ONE component -- `sc` if `npsi>1`, else `cycle`
   if `.not.isCloseToTD .and. varwnc>1e-10 .and. (ncycth/=0 .or. Ncyc/=1)`,
   else `ir` -- and when `nchi>1` it goes through a centered seasonal moving
   average rather than being added raw. `mq==3` has its own three-term arm.

Then `:660` recomputes `trend = z - sc - cycle - ir` from the REPLACED `z` and
the UPDATED components. That fold is why `sfd` is off by much less than `tfd`
and settles after the first year: the smoother decays.

### Why the old two-family split was a red herring

`zmean` (`ansub3.f:121-134`, gated `d+bd==0 .and. imean==1`) looked like the
MEAN family's mechanism and is not: the probe reports `zmean = 0.0` on
`airline_mean-d0-seats`, `airline_mean-seats` AND `expgs_fixed-airline-seats`,
because `airline_mean-d0` is `(2 0 0)(0 1 1)` -- `d+bd = 1`, not 0. APPROX vs
MEAN only ever tracked how far each spec's filter reconstruction drifts from
its regARIMA forecast, i.e. the SIZE of `d1`, not two different causes.
The family labels in the test's `KNOWN_GAP` should be collapsed when this
closes.

### PORTED -- result

Landed in `estbur.cpp` between the component recursions and the `:660`
residual. All four steps together, as warned below. **39 gaps -> 6**, and the
33 that closed went to ~5e-15, not merely under tolerance. Full suite
5818 -> 5851 passed with 0 failed and no change to any historical table.

Two sub-causes remain, both now named in `KNOWN_GAP`:

| sub-cause | specs | worst | what it is |
|---|---|---|---|
| `TDLIN` | the 4 `*_mean-td-seats` | 7.2e-3 .. 6.9e-1 | `TramLin = Tram/TramDet` divides out every DETERMINISTIC factor, TD included; the port feeds the block `ctx.forecasts.trnfct`, which still HAS the TD effect. That exactly the four TD specs are left is the signature. Fix: subtract the forecast-span TD contribution first -- the same split `run_seats` already does historically (`seats_combined_orig`, "add back only the Constant's contribution"). |
| `RESIDUE` | `unrate_mean-seats`, `unrate_mean-d0-seats` | 1.4e-10, 9.1e-8 | no TD regressor, so NOT the above. Both improved 6-9 orders when the block landed (from 1.51e+01 and 3.81e-01). unrate is the additive/`lam==1` series -- suspect `ansub3.f:565-568`, the non-log arm, which drops the LOG rather than taking it. |

### How it was implemented

The port needs `TramLin` over the forecast span. That is the regARIMA forecast
in ORIGINAL units (the LOG is taken here), i.e. `ctx.forecasts.fcst` -- NOT
`trnfct`, and not `zextFwd`. Dead end 2 below tried substituting the regARIMA
forecast for the WHOLE extension, which breaks the filter; the fix is to keep
`zextFwd` for the recursions and use `LOG(TramLin)` only from step 1 onward.
Steps 3 and 4 must land with it -- a partial port that replaces `z` without
folding `d1` into a component would make `tfd` WORSE, since `:660` would then
attribute the entire discrepancy to the trend.

Answer this first: confirm what fills `TramLin` on the X-13 path (the
`ansub9.f` USRENTRY bridge is the likely writer) rather than assuming
`ctx.forecasts.fcst` equals it. The probe can print `TramLin(Nz+1)` directly
in one more run.

### Reproducing the instrumented oracle

Do NOT edit `oracle/fortran/` -- copy it to a scratch dir first. gfortran 13
rejects the vendored source with plain `-O2` ("More actual than formal
arguments", `adpdrg.f:409`), so `FFLAGS` needs
`-O2 -std=legacy -fallow-argument-mismatch -w`. Build from **PowerShell**, not
Bash: msys `make` scrubs `TEMP` and gfortran dies with "Cannot create
temporary file in C:\WINDOWS\: Permission denied". Then VALIDATE before
trusting any probe -- `run_parity.py --binary <instrumented> --engine oracle
--filter "generated/airline_seats"` must report 0 diffs on every table. The
probe output file shows up as `ONLY_B`, which is expected.

### Dead ends -- measured, do not repeat

1. **Extend with the RAW (pre-cap) MA** (`mo.th_raw`/`mo.bth_raw`). Makes the
   historical WORSE (`expgs_fixed` s12 2.9e-5 off) and does not reproduce the
   forecast either. Now explained: the capped extension IS what the oracle's
   filter uses.
2. **Use `ctx.forecasts.trnfct` as the extension.** Breaks `expgs_fixed`'s
   historical by 1e-3 and airline's by 3.5e-13 -- because it replaces the
   FILTER's input too. The regARIMA forecast belongs only at `:660`.
3. **Keep `bias3c` on the forecast trend.** Leaves `tfd` exactly `bias2c` off.
   The `bias1c` choice in section 2 stands and is independent of this finding.
4. **Deriving `bias2c` from the goldens.** It cancels: `mean(s11/s12) == 1`
   holds identically for any `bias2c`.

## 4. Not in scope

`ofd` (no golden). The HP filter (`tools/seats_hp_scouting.md`) needs
`HPTRCOMP` over `1..Nz+lfor` and therefore needs this front finished first —
its step 1 is exactly this file.
