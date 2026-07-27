# `genqs.f` — the QS seasonality statistics — scouting

**Status: NOT PORTED. Scouted (read-through complete), nothing written yet.**
Written 2026-07-27, straight after the spectrum peak increment closed.

The largest remaining `.udg` diagnostic block: **316 of the 320 goldens carry a
`qs*` key** and the port emits none of them.

## Where it runs, and what gates it

Two call sites, both in `x11ari.f`:

| line | when |
|---|---|
| `x11ari.f:279` | the direct run |
| `x11ari.f:347` | the INDIRECT composite pass (`Iagr==4`) |

The gate is
`IF(Prttab(LSPCQS).or.Savtab(LSPCQS).or.Svltab(LSLQS).or.Svltab(LSLDQS))`.

**Two things to check before assuming this mirrors the spectrum increment:**

1. **There is NO `IF(Ny.eq.12)` here.** The spectrum block sits behind one;
   genqs does not, and `arima.f:1105`'s residual QS is gated on `Sp.gt.1`. So
   quarterly specs are in scope, unlike the peak block. Do not copy the
   monthly-only structure across.
2. **The savelog write is gated on `Savtab(Tblind)`, not on `Lsumm>0`** — the
   opposite of every canary the last five increments leaned on. It is
   nevertheless emitted on a spec with no `spectrum{}` block at all (verified:
   `generated/airline_x11-default`'s golden carries all 14 `qs*` keys and its
   spec never mentions spectrum), so `Savtab(LSPCQS)` must be set by the `-s`
   blessing flag. **Confirm how before porting** — and find out which 4 of the
   320 goldens carry no `qs*` key, since that is the negative control.

## Routines to port (all small)

| file | what |
|---|---|
| `calcqs.f` | the Pierce QS statistic over `Z(Iconce+1:nz)` — autocorrelations at lags `mq` and `2*mq` only, `QS = nr(nr+2) * sum r(k)^2/(nr-k*mq)`, and **zero unless `r(1) > 0`** |
| `calcqs2.f` | the same statistic plus a `PosCorr` flag: for `mq>4` it needs `r(mq)>0` AND `r(1..4)>0`; for `mq<=4` it needs `r(1..mq) > 0.2` |
| `qsdiff.f` | first-difference, then `ndif-1` more times where `ndif = max(min(2, Nnsedf+Nseadf), 1)` with a model and 1 without; mean-delete (`smeadl`); `calcQS2`; and **if `PosCorr==1 && ndif==1`, difference ONCE MORE and recompute** |
| `genqs.f` | the driver: builds six series, calls qsDiff/calcqs on each over two spans, then prints and saves |

`chisq.f` and `smeadl.f` are **already ported** (`core/src/numeric/numeric.hpp`);
`chisq(QS, 2)` is the p-value column.

## The six series, and the two spans

Each statistic is computed twice — over `[Pos1ob, Posfob]` (the `qs*` keys) and
over `[ipos+1, Posfob]` (the `qss*` keys, "starting at Bgspec"), the second only
`IF((ipos+1).gt.Pos1ob)`, where `ipos = dfdate(Bgspec, Begbk2)`.

| key | series | notes |
|---|---|---|
| `qsori` | `Series` | `Llogqs` optionally logs it first |
| `qsorievadj` | `Stcsi` + the extreme-value fold | **same construction as spcdrv's sp0**, pseudo-additive rebuild included |
| `qsrsd` | the regARIMA residuals | computed in `arima.f:1107`, NOT in genqs |
| `qssadj` | `Stci` (X-11) or `Seatsa` (SEATS) | |
| `qssadjevadj` | `Stcime` with `Facls` divided out, or `Stocsa` | **the same `Adjls==1` Facls divide the spectrum increment just found missing at spcdrv.f:318** |
| `qsirr` / `qsirrevadj` | `Sti` / `Stime` (X-11), `Seatir` / `Stocir/100` (SEATS) | `-1` when `Muladd!=1`; these use `calcqs` DIRECTLY, not qsDiff |

`Iagr==4` swaps in the `qsind*` key names; `Iagr<4` suppresses `qsori`/
`qsorievadj`/`qsrsd` on the indirect pass.

## Things already established

* **`qslog` is a real key** (`yes`/`no`), and it is the `lplog` flag — set as a
  side effect of whichever series actually took a log. Note the SA branch at
  `genqs.f:147-152` **does not set `lplog`** on its `Lx11` arm where every other
  branch does; transcribe it, do not tidy it.
* **`QSrsd`'s span is estimation-time.** `arima.f:1110` derives `idate` from
  `Begspn`/`Nspobs` before setspn.f restores them — exactly the trap the
  spectrum increment hit. `ctx.resid_begdate` (added in 3b0e986) already holds
  the right value; reuse it rather than re-deriving.
* **`Bgspec` has to be resolved before `run_pre_model`**, because `QSrsd2`'s span
  keys on it. `run_spectrum` currently computes the `gtspec.f:324-327` default
  (eight years back from Endspn, clamped to Begspn) LOCALLY. Either hoist that
  into `gtinpt`'s parse tail next to the new Peakwd resolution, or factor it
  into a helper both callers use. Do NOT leave two copies.
* The `Series`/`Stcsi` construction duplicates spcdrv's sp0 exactly (pseudo-
  additive rebuild, `addmul(Stex)` fold, `Lx11` guard). Share it with
  `run_spectrum` rather than transcribing twice — that block was wrong in three
  ways until the spectrum increment fixed it.

## Suggested order

1. `calcqs` + `calcqs2` + `qsDiff` as leaves, with the `arima.f:1107` residual
   QS wired first — it is one key pair (`qsrsd`/`qssrsd`) and needs no X-11
   buffers, so it gates on its own.
2. `genqs`'s direct path (`Iagr<4`), which is the 316 goldens.
3. The `Iagr==4` indirect names, alongside the composite front.
4. SEATS (`Seatsa`/`Seatir`/`Stocsa`/`Stocir`) — same blocker as the spectrum
   peak block: `run_seats` does not call this driver at all.

## Gate

Shaped like `tests/parity/test_spectrum_peaks.py`: read the `qs*` keys from the
golden `.udg`, compare LINE-EXACT through `fwrite_fmt` with genqs's own formats
(`1030 FORMAT(a,':',f16.5,1x,f10.5)` and `1040 FORMAT(a,': ',a)` — note 1030 has
**no space after the colon**), assert key sets in BOTH directions, and
mutation-test before believing it.
