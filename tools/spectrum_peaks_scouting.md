# Spectrum peak diagnostics — scouting

**Status: NOT PORTED. Fully scouted, ready to execute.** Written 2026-07-27,
after the check{}/estimation/X-11 savelog increments closed everything cheaper.

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
