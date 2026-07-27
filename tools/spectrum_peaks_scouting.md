# Spectrum peak diagnostics — scouting

**Status: steps 1–3 CLOSED (byte-exact, gated by
`tests/parity/test_spectrum_peaks.py`, 140 specs, zero new goldens). Steps 4–5
(getTPeaks, genqs) still open, plus two paths this port cannot reach yet — see
"What is still open" at the bottom.** Originally written 2026-07-27.

This is the largest remaining `.udg` diagnostic block, and it is **silently
absent on every monthly run**, not merely on specs that ask for `spectrum{}`.

## The finding

`x11ari.f:282-287` and `:351-352` call `spcdrv` under **`IF(Ny.eq.12)`** — a
plain monthly test, with no dependence on the `spectrum{}` spec at all. So the
oracle computes the whole spectrum diagnostic block on every monthly run, and
289 of the 331 `.udg` goldens carry `spcori.*`.

This port gates `run_spectrum` on `ctx.spcout.requested`, which `gt_spectrum`
sets only when a `spectrum{}` block is present. Measured on
`generated/airline_x11-default` (no `spectrum{}`): the golden carries the full
`spcori.*` / `peaks.*` / `s1..s5` / `t1..t2` block and the engine emits **zero**
`sp0/sp1/sp2` rows and none of the canaries.

So the first half of the fix is a GATE change, and the second half is the peak
arithmetic that was never ported.

## What is missing, by key family

| family | goldens | source |
|---|---|---|
| `spcori.median/.range/.s1-.s5/.t1/.t2/.s.dom/.t.dom/.dom` | 289 | `svpeak.f` + `smpeak.f` + `mxpeak.f` |
| `spcrsd.*` (same shape, regARIMA residuals) | 250 | same, `Itbl=4` |
| `spcsa.*` / `spcirr.*` (same shape) | 202 | same, `Itbl=2/3` |
| `spcori.tukey.*` / `spcrsd.tukey.*` / … | 283/239 | `getTPeaks` (`specpeak.f`) |
| `peaks.seas` / `peaks.td` / `peaks.tukey.*` | 289/283 | `spcdrv.f` tail |
| `s1..s5.freq/.index`, `t1..t2.freq/.index`, `nsfreq`, `ntdfreq` | 290 | `ispeak.f` |
| `qsori` / `qslog` / `qsrsd` / `qssadj` / `qsirr` (+ `s`/`evadj` twins) | 324 | `genqs.f` (666 lines) |
| `nplog` / `npsadj` / `npsadjevadj` / `npssadj` … | 230 | `genqs.f` |
| the config echoes (`spectype`, `decibel`, `siglevel`, `peakwd`, `specmaxar`, `startspec`, `diffspec*`, `saveallspecfreq`, `showseasonalfreq`, `specseries`, `specrobustsa`, `altfreq`) | 290 | `x12hdr.f` / `spcdrv.f:117-125` |

## Routines to port (all small; total ≈ 420 Fortran lines)

| file | lines | what |
|---|---|---|
| `shlsrt.f` | 77 | shell sort of the 61-point spectrum |
| `mkmdsx.f` | 22 | median of the sorted spectrum (decibel-aware) |
| `ispeak.f` | 111 | build the peak / lower-limit / upper-limit INDEX vectors |
| `idpeak.f` | 42 | median + `pklim`, then `ispeak` for TD and seasonal |
| `smpeak.f` | 52 | per-peak star height, `nopeak`, and the family's dominant frequency |
| `svpeak.f` | 50 | `median`/`range` rows, then `smpeak` ×2 + `mxpeak` |
| `mxpeak.f` | 64 | the overall dominant frequency across TD and seasonal |

`getTPeaks` (in `specpeak.f`, 850 lines) is the Tukey half and is a bigger,
separable second step — the port already computes the Tukey spectra
(`ctx.spcout.st0/st1/st2`), so only the peak probabilities are missing.

`genqs.f` (the QS statistics, 324 goldens) is a third, independent step.

## Things already established

* **The peak indices are FIXED for monthly data.** Measured from the goldens:
  seasonal peaks sit at grid indices 10, 20, 30, 40, 50 (k/12 on the 61-point
  0..0.5 grid) and the two trading-day peaks at 42 and 52
  (`t1.freq: 0.34820000`, `t2.freq: 0.43200000`). `ispeak` still has to derive
  the surrounding low/up limits per spectrum, which is what makes each peak's
  star height data-dependent.
* **The 61-point spectra are already bit-exact and gated** (`sp0/sp1/sp2/spr`
  in `run_spectrum.cpp`, `test_spectrum_tables.py`), so `svpeak`'s input is in
  hand — this is arithmetic ON TOP of a verified base, not a new numeric path.
* **`svpeak` takes TWO arrays**, `Sxx` (the 61-point grid, for the median and
  range) and `Sxx2` (which `smpeak` indexes BY PEAK INDEX). Confirm what the
  caller passes as the second before assuming they are the same array —
  `spcdrv.f:543/551/577` passes `sasxx/sasxx2`, `irrsxx/irsxx2`,
  `orisxx/orsxx2`.
* **`mkspky.f` chooses the key PREFIX** from the table index and `Iagr`:
  `spcori`/`spcsa`/`spcirr`/`spcrsd`, or `spccomp`/`spcindsa`/`spcindirr` on a
  composite total (`Iagr>=4`), or `spcextrsd` under SEATS. One routine, four
  call sites, three naming schemes.

## Suggested order

1. **Gate change alone**: run `run_spectrum` on every monthly run, not just
   when `spectrum{}` is present. Verify the existing `test_spectrum_tables`
   gate is unmoved and that nothing else shifts — this touches a driver that
   runs before the span replays, so watch the `ctx` restore set (see the D8B
   entry in `CLAUDE.md`: anything written from inside the x11 spine needs
   adding to `run_x11`'s save/restore block).
2. `shlsrt` + `mkmdsx` + `ispeak` + `idpeak` → the `s*/t*.freq/.index`,
   `nsfreq`/`ntdfreq` keys. Gateable on their own.
3. `smpeak` + `svpeak` + `mxpeak` → the `spcXXX.*` families.
4. `getTPeaks` → the `.tukey.*` families and `peaks.tukey.*`.
5. `genqs.f` → the QS block.

Steps 2-3 alone would gate ~290 goldens' worth of keys with **zero new
goldens**, the same leverage the check{}/estimation/X-11 increments had.

## Gate

Extend `tests/parity/test_spectrum_tables.py`, or add a
`test_spectrum_canaries.py` shaped like `test_check_diagnostics.py`: read the
keys from the golden `.udg`, compare LINE-EXACT against the harness (the emit
should go through `fwrite_fmt` with `svpeak.f`'s own formats — `1010
FORMAT(a,'.',a,': ',e20.10)` and `smpeak.f`'s `1020 FORMAT(a,'.',a,': ',f6.1,'
',a)`), and assert key sets in BOTH directions. Note `smpeak` writes the
literal string `nopeak` for a peak below its base, so the value column is not
always numeric.

## What actually happened (steps 1–3, closed)

The gate change was NOT the whole of the first half. `gtinpt.f:354-371`'s entire
`/rho/` default block lived at the head of `gt_spectrum`, so a spec without a
`spectrum{}` block read the struct's ZERO-INIT — `Ldecbl` false, `Spcsrs` 0,
`Bgspec` 0 rather than NOTSET — and `Peakwd` was never resolved at all
(`gtinpt.f:1285-1288` when no spectrum spec, `gtspec.f:355` when there is one;
the port had neither). Moving the defaults into `gtinpt` is what made removing
the gate produce anything.

**Four real sp0/sp1/sp2 defects came out of it**, all previously invisible
because `test_spectrum_tables` only ran on specs that ASK for `spectrum{}` and
none of those exercise the branches:

1. **`spcdrv.f:318`'s `Facls` divide was missing.** The LEVEL SHIFT is taken
   back out of the SA series before its spectrum — unconditionally on
   `Adjls==1`, unlike x11pt4's E2 block which is gated on `.not.Finls`. Every
   spec carrying an LS regressor, explicit or automatically identified, had a
   different SA spectrum (`spcsa.range` 12.418 vs 12.608 on
   `generated/airline_regb-initial`). This one alone accounted for 13 of the 21
   value failures.
2. **`(Lx11.and.Kfulsm.eq.0).or.Lseats` (spcdrv.f:284/:406) was missing**, so
   `x11{type=summary|trend}` emitted spcsa/spcirr blocks the oracle suppresses.
3. **The pseudo-additive sp0 rebuild (spcdrv.f:166-174) was unported** —
   `Stc*(Sts+(Sti-1))`, or `Stc*Sti` at `Kfulsm==2` — and the port took the
   ordinary `addmul(Stex)` fold instead. Also the fold's own `Lx11` guard.
4. **`ispos` (spcdrv.f:187 / :290-298) was missing**: the oracle refuses to log
   a non-positive series and produces no table.

Plus one placement bug of the same class as `ctx.est_nefobs`: **`spcrsd`'s start
date must be recorded at ESTIMATION time.** `arima.f:1125` computes it from
`Begspn`/`Nspobs` while a `series{modelspan=}` still has them narrowed;
setspn.f widens them back at `arima.f:1145/:1181`, i.e. after. Deriving it in
`run_spectrum` slid the residual spectrum (`spcrsd.median` -32.32 vs -32.59 on
`generated/airline_modelspan-both-x11`).

**Performance note that decided a design choice.** Running the spectrum on every
monthly spec (which is what the oracle does) adds four AR fits per spec, and the
peak block needs a FIFTH evaluation on the 67-point enhanced grid. Re-fitting for
it took the parity suite from ~200s past 600s. `spgrh` is therefore split into
`spgrh_fit` + `spgrh_eval` and the fit is reused across both grids — legitimate
because `ifpl` (30 for monthly) never exceeds either grid's `lagh1` truncation,
so `sicp2` reads the same autocovariance prefix either way. Suite runtime is
back to ~230s.

## What is still open

* **`getTPeaks`** (specpeak.f, ~850 lines) — the `.tukey.*` families and
  `peaks.tukey.*`. The Tukey SPECTRA are already ported and bit-exact; only the
  peak probabilities are missing. `test_spectrum_peaks.py` explicitly asserts
  these keys are NOT claimed, so porting them has to come delete that assertion.
* **`genqs.f`** (the QS block, 324 goldens) — independent of everything above.
* **SEATS specs** (51 of the 278 goldens): `run_seats` never calls
  `run_spectrum`, and spcdrv's SEATS branch reads `Hvstsa`/`Hvstir`/`Stocsa`/
  `Stocir` where the X-11 branch reads Stcime/Stime. A different INPUT, not just
  a different driver.
* ~~Model-only specs~~ -- **CLOSED** (the gate went 140 -> 222 specs). The
  predicted detrend difference was real: spcdrv.f:193-200 keys on
  `dpeq(Lam,ZERO)` without Lx11 rather than on `Muladd.ne.1`, and the engine was
  logging a sqrt-transformed series (`spcori.median` +24.29 oracle vs -27.79
  engine on `generated/airline_trans-sqrt` -- a sign flip, not a drift). Full
  write-up, including the three other defects the same change surfaced, in
  `tools/genqs_scouting.md`.

Both of the last two skip with those reasons written at the skip rather than
being filtered out of the gate's discovery, so the coverage they cost is
visible in the run.

## OPEN: two published-vs-internal buffer reads in `run_spectrum.cpp`

Found by a read-only audit of every consumer of `/x11srs/ Sti` and `Stc` after
x11pt3, prompted by the genqs increment (see `tools/genqs_scouting.md` for the
deviation: this port copies the PUBLISHED D13/D12 back over `x11srs.sti`/`.stc`
at the tail of x11pt3, where the oracle leaves them internal and prints from its
own `sti2`/`stc2` locals). **Reported, NOT yet verified against the Fortran by
hand, and NOT yet fixed** -- both need a measured oracle on-vs-off before
anything changes, the standing rule on this front.

1. **`run_spectrum.cpp:465`** -- reads the published `ctx.x11srs.sti` on the
   `Lrbstsa == false` arm, where `spcdrv.f:439-445` reads the internal Sti.
   Reachable only with `spectrum{robustsa=no}` AND (`Adjao==1` or
   `Adjtc==1 && !Lttc`); with no AO/TC fold the two buffers are identical, which
   is why nothing has seen it. No corpus spec sets `robustsa=no`.
2. **`run_spectrum.cpp:401/403`** -- the PSEUDO-ADDITIVE sp0 rebuild reads the
   published `stc`/`sti` where `spcdrv.f:163-173` reads the internal pair.
   Reachable with `x11{mode=pseudoadd}` + `Spcsrs==2` (the default) when Sti
   carries an AO/TC fold or Stc carries the `!Finls && Adjls==1` LS fold.

Reported clean by the same audit, with Fortran line references: x11pt4's
etables/partf (they take `x11_stc_int`/`x11_sti_int`), genqs (via
`x11_sti_live`/`x11_stc_live`), the C/R ABI and both harnesses (which SHOULD
expose the published tables -- that is the point of the deviation), the
composite agr2/agr3 path (its own Stc/Sti semantics from agr3.f), and
run_history's span path (x11pt3 clean-fatals on `Irev==4` before the
publish-back is reached).

**Gating either one needs a new corpus spec**, since the corpus currently has
neither `robustsa=no` nor a pseudo-additive spectrum case -- so the first step is
to measure the ORACLE on-vs-off and confirm the branch moves at all before
porting anything.
