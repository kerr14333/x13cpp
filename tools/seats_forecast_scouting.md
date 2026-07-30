# The SEATS FORECAST decomposition — `ansub3.f:353-678` / the `tfd sfd ofd afd yfd` tables

**Status:** ported and **partially closed**. 13 of the 52 corpus specs gate
bit-exact; the other 39 are measurably wrong in two families and are asserted
as such from both sides by `tests/parity/test_seats_forecast_tables.py`.
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
  ansub3.f:565-650   the TRAMO passthrough                <- unreachable, Tramo==0
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

## 3. The two open families — measured

`tests/parity/test_seats_forecast_tables.py`'s `KNOWN_GAP` carries the full
per-spec table. Summary:

| family | specs | worst rel | what it is |
|---|---|---|---|
| `APPROX` | 16 | 5.2e-7 … 4.4e-2 | SEATS caps a near-non-invertible MA to the `xl` bound before the canonical decomposition |
| `MEAN` | 23 | 5.1e-4 … 1.5e+1 | `imean != 0` / a Constant regressor; the forecast of `z` carries the wrong drift |

**The error is in the forecast of `z`, not in the decomposition of it.** On the
failing specs `tfd` and `afd` are off by the SAME amount while `sfd` is off by
much less and settles to exact after the first year, and the error grows
LINEARLY in the horizon index — a constant slope error, i.e. a drift.

### The decisive measurement, and why it is a puzzle

The oracle's saved forecast tables imply a `z` that equals the regARIMA `.fct`
forecast **to the last bit** — verified on the two specs that ship both
(`airline_fixed-airline-seats`, `expgs_fixed-airline-seats`, both `-4e-15`).
The port's ESTBUR extension of `z` equals it on `airline` (8e-13) and is
0.365% away on `expgs_fixed`.

But the port's extension is also what makes the HISTORICAL span bit-exact, and
the historical span is **not** insensitive to it: perturbing the whole
extension coherently by `+3.64e-3` in logs (the size of the disagreement) moves
`s12`'s last value by `8.3e-4` relative, and perturbing a single extension
point by `1e-6` in logs moves it by `7.6e-4` (a sensitivity of ~-755 per unit
log). So both of these are true at once and are not yet reconciled:

- the extension the port feeds the FILTER is right (s10–s13 bit-exact);
- the `z` the oracle's saved forecast tables are built from is a DIFFERENT
  series, equal to the regARIMA forecast.

`ansub3.f:660` uses `z(i)` while the filter uses `extZ(i)`, and `extZ` is a copy
of `z` taken at ESTBUR entry — so in the Fortran they are the same array unless
something modifies `z` in between. Nothing found so far does (`Tramo` is 0 on
every X-13 run; `AUTOCOMP` and `SERRORL` are read-only on all five component
arrays; the `Nsfcast`/`Sfcast` swap is dead code — `sigex.f:808`'s
`Nsfcast = 1` is commented out and `analts.f:310` sets it to 0).

### Dead ends — measured, do not repeat

1. **Extend with the RAW (pre-cap) MA** (`mo.th_raw`/`mo.bth_raw`, which
   `model_decode.hpp`'s own comment recommends). Makes the historical WORSE
   (`expgs_fixed` s12 2.9e-5 off) and does not reproduce the forecast either
   (raw gives `z(1) = 3586.36`, capped `3587.81`, oracle `3574.74`).
2. **Use `ctx.forecasts.trnfct`** (the regARIMA transformed forecast) as the
   extension. Breaks `expgs_fixed`'s historical by 1e-3 and airline's by
   3.5e-13.
3. **Keep `bias3c` on the forecast trend.** Leaves `tfd` exactly `bias2c`
   (1.0001048 on airline) off while `sfd`/`afd`/`yfd` are already exact.
4. **Deriving `bias2c` from the goldens.** It cancels: `mean(s11/s12) == 1`
   holds identically for any `bias2c`, so the historical tables cannot pin the
   normalization. Only the forecast trend can, which is why this was invisible.

### Suggested next step

Instrument the oracle (temporary `ansub3.f` write of `z(Nz+1)` and
`extZ(Nz+1)` immediately before `:660`) and settle in one run whether they are
equal. That is the same ground-truth technique that closed the CALCFX seeding
question in session 14, and every inference above is blocked on it. If they
differ, find the writer; if they agree, the oracle's own extension is the
regARIMA forecast and the historical bit-exactness of the capped extension is
the thing to explain instead.

The `MEAN` family should be probed separately — no capping is involved there
(`th == th_raw` on every one of them), so it is a second, independent defect in
the drift handling of `fcast_extend`'s `za` seed, not the same bug.

## 4. Not in scope

`ofd` (no golden). The HP filter (`tools/seats_hp_scouting.md`) needs
`HPTRCOMP` over `1..Nz+lfor` and therefore needs this front finished first —
its step 1 is exactly this file.
