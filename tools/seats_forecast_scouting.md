# The SEATS FORECAST decomposition — `ansub3.f:353-678` / the `tfd sfd ofd afd yfd` tables

**Status:** **CLOSED.** All **52** corpus specs gate bit-exact (48 of them at
1e-12 or better, worst 1.01e-11), and `KNOWN_GAP` in
`tests/parity/test_seats_forecast_tables.py` is empty. Section 3 records the
two defects that accounted for every gap -- ansub3.f's Tramo block and
ansub4.f's refold, BOTH of them live code this port had written off as
unreachable, and each of which hid the other by cancelling. This file is the
map, including the dead ends, which cost most of the time and should not be
re-walked.

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
sigex.f:3631-3636   USRENTRY 1410/1411/1413/1409 -- DEAD, guarded
                    `if (Tramo .le. 0)`, and Tramo is 1 on every X-13 run.
ansub4.f:3144-3210  the LIVE writer, log path: builds ftr/fsa/fs/fcyc by
                    REFOLDING the deterministic factors onto the antilogged
                    components; punched at :3287 and again at :3342.
ansub4.f:2284-2337  the same, non-log path, in sums; punched at :2466.
seatpr.f:199/351/444/499  savtbl -> LSEFCD{+0,+1,+3,+4} = tfd/sfd/afd/yfd
```

**The saved forecast tables are not the components.** With every `Pareg` term
the identity (why: section 3), ansub4.f's log arm is

```
fs   = sc(pct) * Paeast*PaTD*Paous                 -> sfd
fsa  = Tram / (sc(pct)/100 * Paeast*PaTD*Paous)    -> afd
fcyc = cycle(pct)                                  -> yfd
fir  = Paouir                                         (before its x100)
ftr  = (fsa/fir) / (fcyc/100)                      -> tfd
```

`fortr` is 1 unconditionally (`ansub9.f:1585 l_fortr = 1`), so `ftr` is always
that RESIDUAL of `fsa` -- which is why `tfd == afd` to the last digit whenever
there is no transitory component, and `afd == tfd*yfd/100` when there is. The
sigsub-antilogged trend is never punched at all.

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

### RETIRED -- the forecast trend has no bias factor at all

This section used to argue that the forecast trend takes `bias1c` where
`sigsub.f:1594` literally reads `bias3c`, explained as a normalization
difference. **The observation was real; the explanation was wrong, and so was
the code.** The saved `tfd` is not sigsub's antilogged trend at all: it is
ansub4.f's `ftr = (fsa/fir)/(fcyc/100)`, a residual of `fsa`, and no bias
factor enters it. `bias1c` happened to be the right fudge because `fsa`
carries exactly one factor of it, through `sc`.

Kept because the two identities used to pin it are still the right way to
check this front, and are still asserted on the GOLDENS by
`test_no_transitory_means_trend_equals_sa`: `tfd == afd` to the last digit on
the specs with no transitory component, `afd == tfd*yfd/100` on the ones with
one, and `afd = tfd + yfd` on the additive ones. They are now *derivable* from
the ansub4.f formulas rather than empirical.

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

### PORTED -- result, and the SECOND unreachable-code mistake

The Tramo block landed in `estbur.cpp` between the component recursions and
the `:660` residual. 39 gaps went to 6, the 33 that closed to ~5e-15.

The last 6 (4 `*_mean-td-seats` at 7.2e-3..6.9e-1, plus `unrate_mean-seats`
and `unrate_mean-d0-seats` near the floor) looked like two unrelated
sub-causes and were labelled `TDLIN` and `RESIDUE`. They were neither. They
were **one more piece of live code this port had recorded as unreachable**:

- The Tramo block was being fed `ctx.forecasts.trnfct`. `TramLin` is
  `Tram / TramDet` -- `Tram(i) = Orixs(i)`, the ORIGINAL-units,
  missing-value-adjusted, forecast-extended series (`ansub9.f:1395`), divided
  by the product of the deterministic factors. Wrong by exactly `TramDet`.
- The saved tables were being built as the antilogged components. They are
  built by **ansub4.f**, which refolds `TramDet` back on. Wrong by exactly
  `TramDet` the other way.

The two cancel. What does NOT cancel is the part of `trnfct` that is not a
regression effect at all -- the length-of-month/leap-year prior, which
`run_pre_model` divides out of the estimation input and which `TramDet`
therefore never carried. So the residue was `lpfac` and nothing else, and it
shows up only in the period containing February. Measured on the goldens,
engine over oracle:

| spec | period | ratio | `lpfac` |
|---|---|---|---|
| `airline_mean-td` | every Feb | 1.008928571429 | 28.25/28 |
| `payems_mean-td` | every Feb | 1.008928571429 | 28.25/28 |
| `expgs_mean-td` (quarterly) | non-leap Q1 | 1.002777777778 | 90.25/90 |
| `expgs_mean-td` (quarterly) | leap Q1 (2028) | 0.991758241758 | 90.25/91 |
| all four | every other period | 1.000000000000 | 1 |

Exactly `lpfac`, to 1e-12, with no free parameter -- which is what identified
it. The two additive `unrate` specs carried the same defect at the scale of an
additive series.

**Result: all 52 specs gate, 48 at 1e-12 or better, `KNOWN_GAP` empty.**

Two things fell out of the fix:

- `Pareg(i,0..7)` is ALWAYS the identity on the X-13 path, and that is
  measured rather than assumed. `TAKEDETTRAMO` fills it from
  `Facusr`/`Facsea`/`Faccyc` only under `if (npareg .eq. 1)`; `Npareg` comes
  from `l_npareg`, which `ansub9.f:1598` initialises to 0 and no bridge line
  ever sets. The else-branch writes `facint` (1 under log, 0 otherwise) across
  the whole array. The instrumented oracle prints all eight as exactly 1.0 on
  a const+td log spec. `analts.f:735-745`'s `Pareg(,2) *= Pareg(,6)` is a
  no-op for the same reason -- it needs `Neff(6)==1`, and TAKEDETTRAMO zeroes
  Neff.
- `ansub4.f`'s own `bias1/bias2/bias3` are dead: `:2795-2797` overwrites all
  three with 1.0 immediately after computing them, under a comment in Italian
  saying it needs checking. Reproduce that; do not "fix" it.

**Standing lesson, now twice in one front:** a reachability claim that has not
been run against the oracle is not a measurement. Both defects here were
recorded in this port as unreachable code, both were live, and because they
sat adjacent in the same data path they cancelled -- producing a 46/52 pass
rate that read as a nearly-finished port.

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
