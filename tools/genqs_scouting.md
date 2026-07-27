# `genqs.f` — the QS seasonality statistics — scouting

**Status: the DIRECT X-11 path is CLOSED (byte-exact, gated by
`tests/parity/test_qs_diagnostics.py`, 157 specs, zero new goldens). SEATS, the
model-only path and the `Iagr==4` indirect names are still open — see "What is
still open".** Originally written 2026-07-27.

327 of the 331 `.udg` goldens carry a `qs*` key and the port emitted none of
them.

## Where it runs, and what gates it

Two call sites, both in `x11ari.f`:

| line | when |
|---|---|
| `x11ari.f:279` | the direct run |
| `x11ari.f:347` | the INDIRECT composite pass (`Iagr==4`) |

The gate is
`IF(Prttab(LSPCQS).or.Savtab(LSPCQS).or.Svltab(LSLQS).or.Svltab(LSLDQS))`.

Both questions the original scouting flagged are now answered:

1. **There is NO `IF(Ny.eq.12)` here** — confirmed. `editor.f:855-863`'s
   monthly-only `Savtab` clear covers `LSPCS0..LSPS0C` (93..102) plus `LSPCTP`
   (115) and `LSPCQC` (116); `LSPCQS` is **113** and is not in either. Quarterly
   specs are in scope, and 41 of the corpus specs are quarterly.
2. **`Savtab(LSPCQS)` is set by the `-s` flag**, via `gtinpt.f:121-123`
   (`Lsumm>0 ⇒ Savtab := sumtab`) and `sumtab.var` entry 113 = `T`. So this is
   the same "every golden was blessed with it" leverage the last six increments
   had, reached by a different route than `Lsumm>0` being read directly.
   The 4 goldens with no `qs*` key are `edge/log-zero-series` and
   `extra/airline_x11regression-tdprior-add` (both runs the oracle refuses) and
   `generated/expgs_{seats,ar2-seats}` (the `noadmiss` specs where the oracle
   writes 0 rows) — i.e. no negative control among the runs that complete.

## Routines ported

| file | what |
|---|---|
| `calcqs.f` | the Pierce QS statistic over `Z(Iconce+1:nz)` — autocorrelations at lags `mq` and `2*mq` only, `QS = nr(nr+2) * sum r(k)^2/(nr-k*mq)`, and **zero unless `r(1) > 0`** |
| `calcqs2.f` | the same statistic plus a `PosCorr` flag: for `mq>4` it needs `r(mq)>0` AND `r(1..4)>0`; for `mq<=4` it needs `r(1..mq) > 0.2` |
| `qsdiff.f` | first-difference, then `ndif-1` more times where `ndif = max(min(2, Nnsedf+Nseadf), 1)` with a model and 1 without; mean-delete (`smeadl`); `calcQS2`; and **if `PosCorr==1 && ndif==1`, difference ONCE MORE and recompute** |
| `genqs.f:1-254` | the driver: builds six series, calls qsDiff/calcqs on each over two spans |

`chisq.f` and `smeadl.f` were already ported (`core/src/numeric/numeric.hpp`);
`chisq(QS, 2)` is the p-value column. All of the above now live in
`core/src/diag/genqs.{hpp,cpp}`; the emit is `dump_qs` in `tools/x13run_x11.cpp`.

Note the two `calc` routines differ in more than calcqs2's extra output: it
works on the whole array from element 1, divides by `nz` rather than by a
separate `nr`, walks every lag up to `2*mq`, and keys the statistic on `r(mq)`
where calcqs keys it on `r(1)`.

## The six series, and the two spans

Each statistic is computed twice — over `[Pos1ob, Posfob]` (the `qs*` keys) and
over `[ipos+1, Posfob]` (the `qss*` keys, "starting at Bgspec"), the second only
`IF((ipos+1).gt.Pos1ob)`, where `ipos = dfdate(Bgspec, Begbk2)`.

| key | series | notes |
|---|---|---|
| `qsori` | `Series` | `Llogqs` optionally logs it first |
| `qsorievadj` | `Stcsi` + the extreme-value fold | same construction as spcdrv's sp0, pseudo-additive rebuild included |
| `qsrsd` | the regARIMA residuals | computed in `arima.f:1107`, NOT in genqs |
| `qssadj` | `Stci` (X-11) or `Seatsa` (SEATS) | |
| `qssadjevadj` | `Stcime` with `Facls` divided out, or `Stocsa` | the same `Adjls==1` Facls divide as spcdrv.f:318 |
| `qsirr` / `qsirrevadj` | `Sti` / `Stime` (X-11), `Seatir` / `Stocir/100` (SEATS) | `-1` when `Muladd!=1`; these use `calcqs` DIRECTLY, not qsDiff |

`Iagr==4` swaps in the `qsind*` key names; `Iagr<4` suppresses `qsori`/
`qsorievadj`/`qsrsd` on the indirect pass.

## What actually happened

Three things came out of it, and two are worth carrying forward.

### 1. CB-25 — the residual pair is read from an uninitialised COMMON

`QsRsd`/`QsRsd2` are assigned in exactly two places, **both inside `arima`**:
the reset at `arima.f:129-130` and the computation at `:1105-1118`. `x11ari.f
:106` calls `arima` only `IF(Lmodel)`. So a model-free `x11{}` run never enters
it, reads the COMMON's static zero, and reports `qsrsd: 0.00000 1.00000` as a
statistic about residuals that do not exist.

The contrast that proves it is `extra/airline_identify`: `identify{}` sets
`Lmodel`, so the reset fires but the `:1105` block does not (`Var` is zero), and
that golden correctly carries **no** `qsrsd` row at all. Ported by defaulting
`QsStats::qsrsd`/`qsrsd2` to **0.0** and resetting to DNOTST in
`run_pre_model` under `has_model`. Full write-up in `tools/census_bugs.md`.

### 2. `/x11srs/ Sti` and `Stc` are NOT the published D13/D12 — and this port makes them so

`x11pt3.f:1089-1103` builds the AO/TC-restored D13 in a **local** `sti2` and
punches THAT; `Sti` itself is never touched, so everything downstream of x11pt3
(x11pt4, and now genqs) sees the outlier-REMOVED irregular. This port copies the
published values back over `x11srs.sti`/`.stc` at the tail of x11pt3 so the
harness can punch d13/d12 off the COMMON mirror, and keeps the oracle's live
values in `ctx.x11_sti_int` / `ctx.x11_stc_int`.

That deviation is invisible to every table gate and it is exactly what genqs
tripped over: on `generated/expgs_fixed-airline-x11` the published D13 gives
`qsirr` 0.03946 against the oracle's 0.00000, because folding the AOs back in
flips the lag-4 autocorrelation positive — and `calcqs` is a **step function in
the sign of `r(1)`**, so a small change in the input is a categorical change in
the output. **Anything new that reads `/x11srs/` after x11pt3 must take the
snapshot, not the mirror.** Verified from the goldens themselves: recomputing
calcqs in Python off the blessed `.d13` file reproduces the ENGINE's wrong
0.03946, not the oracle's `.udg` value, which is what localised it.

### 3. `Bgspec` had two resolutions and now has one

`gtspec.f:324-327` (spectrum spec present) and `gtinpt.f:1282-1286` (absent) set
the same default — eight years back from `Endspn`, clamped forward to `Begspn`.
The port had neither and `run_spectrum` computed it locally, which does not
work for genqs: the residual pair keys on `Bgspec` during ESTIMATION. It is now
resolved once at the parse tail, next to `Peakwd`, and `run_spectrum` reads it.

Found alongside it: the two `Peakwd` sites are **not** equivalent —
`gtspec.f:355` is a bare `Peakwd=1` with the `Sp.eq.4 ⇒ 3` override commented
out, so a quarterly spec carrying a `spectrum{}` block gets 1 where one without
gets 3. Inert today (only the monthly peak grid reads it) but now transcribed.
`spectrum{logqs=}` was also parsed-and-dropped; it is now applied.

## Gate

`tests/parity/test_qs_diagnostics.py`, 157 specs, **zero new goldens**,
byte-exact against the golden text through `fwrite_fmt` with genqs.f's own
formats (`1030 FORMAT(a,':',f16.5,1x,f10.5)` — note **no space after the
colon** — and `1040 FORMAT(a,': ',a)`), key sets asserted in BOTH directions.

The only numeric fallback is `max(5e-4 abs, 5e-5 rel)` on `qsrsd`/`qssrsd` for
the two `airline_automdl-x11` specs (measured 2.30823 vs 2.30858, 1.5e-4
relative), the same bound and the same reason as `test_check_diagnostics`: the
statistic is `nr(nr+2)*r(sp)^2/(nr-sp)` on an AUTO-SELECTED model's residuals,
so a 1e-6 coefficient agreement buys an `r` that agrees to ~1e-5. Restricted to
those two keys so a drift in any X-11 statistic still fails loudly.

Mutation-tested twice: a 1% perturbation of `calcqs` fails 113 of 157, of
`calcqs2` 140 of 157.

## What is still open

* **SEATS specs.** genqs's SEATS arms read `Seatsa`/`Seatir`/`Stocsa`/`Stocir`
  behind `Hvstsa`/`Hvstir` — COMMONs this port's SEATS chain does not fill,
  because it publishes its decomposition onto ctx instead. The same blocker the
  spectrum peak increment hit: a different INPUT, not just a different driver.
  `Seatsa`/`Seatir` map onto `ctx.seats_sa`/`ctx.seats_ir`; `Stocsa`/`Stocir`
  (the `/100` outlier-adjusted pair) need scouting first.
* **Model-only specs.** `x12run.f:181` reaches `x11ari` with neither Lx11 nor
  Lseats, so the oracle emits `qsori`/`qsorievadj`/`qsrsd` (plus twins) on a
  spec asking for no adjustment. `qsorievadj` reads `Stcsi`, which only x11pt1
  fills — and this port's model-only harness runs no part of the X-11
  pre-stage. Same blocker as the spectrum peak block's model-only skip; closing
  either one probably closes both.
* **The `Iagr==4` indirect names** (`qsindsadj`, `qssindirr`, ...), which belong
  with the composite front. Note the indirect call site passes `Tblind = LSLIQS`
  — a *savelog* index used as a `Savtab` subscript — which wants checking before
  it is ported.
* `gennpsa.f` (the `nplog`/`npsadj`/`npsadjevadj`/`npssadj`/`npssadjevadj` keys,
  230 goldens) is a separate routine in the same family, not part of genqs.
