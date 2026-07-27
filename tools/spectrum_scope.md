# spectrum{} compute — port plan (execution-ready)

## STATUS: COMPLETE — all increments landed (periodogram + arspec, all 7 tables, bit-exact)
Increment 4 (arspec, spgrh/sautco/sicp2): the AR-spectrum type. sautco =
smeadl mean-delete + crosco autocov (Thtapr=0, no taper); sicp2 = Levinson-
Durbin, but the AIC order-selection is dead code (CB-12) so it returns the
full order l=min(Mxarsp,n-1); spgrh = the AR transfer-function spectrum
sgme2/|1+sum coef_k e^{-i2pi k f}|^2. Wired via a spec_est selector (spgrh for
spctyp==0, spgrh2 for ==1) across sp0/sp1/sp2/spr; st0/st1/st2 stay Tukey.
gt_spectrum captures maxar. **spectrum{} fully closed** (596 parity pass at the
time of writing; 5261 as of 2026-07-27).

> **Scope note added 2026-07-27.** "Closed" here means the `spectrum{}` SPEC
> SURFACE — the seven tables and their options. It predates three later fronts
> that are tracked in `tools/spectrum_peaks_scouting.md`, not here: the peak
> savelog block, `getTPeaks`' Tukey peak probabilities, and the fact that the
> oracle runs the whole spectrum block on EVERY monthly run rather than only
> when `spectrum{}` is present. The remaining open item is spcdrv's SEATS
> branch.

## (historical) increments 1+2 LANDED (periodogram sp0/sp1/sp2 + spr, bit-exact ~5e-14)
`core/src/driver/run_spectrum.{cpp,hpp}` + `gt_spectrum` option capture +
`x11parts.cpp` persists E3 (`ctx.mq5a_stime`). Gated by
`tests/parity/test_spectrum_tables.py` on `airline_spectrum.spc` (arspec spec
xfailed). **KEY correction to the plan below:** spcdrv runs AFTER x11pt4, so its
`Stci`/`Sti` hold the *modified-for-extremes* E2 (`Stcime`) / E3 (`Stime`) tables
(x11pt3.f Part E, lines 1199-1289), NOT the D11/D13 saves. sp1 uses
`ctx.adxser.stcime`, sp2 uses `ctx.mq5a_stime`. sp0 uses `Stcsi`+`Stex` (survives).
Increment 2 (spr, spcrsd.f): periodogram of the regARIMA residuals `a` (no
detrend/log), captured off the estimate as `ctx.resid_a`/`resid_na`; residual
start = Begspn + (Nspobs - na); span [rpos, na], rpos = dfdate(Bgspec,Begrsd)+1.
Increment 3 (Tukey st0/st1/st2, getTPeaks/covWind/crosco): the Tukey-windowed
autocovariance spectrum of the SAME detrended AdjOri/SA/Irr series, window
m=tukey_window(nz) (79 here), 40 pts on the i/m grid. crosco = biased autocov
(no mean removal); covWind's p(0) has the factor-1 Census quirk; savstp does
10log10(|.|) with the freq as single-precision float(i)/float(m).
Remaining: increment 4 (arspec -- spgrh/sautco/sicp2 AR spectrum, replaces
spgrh2 for the sp0/sp1/sp2/spr tables; st0/st1/st2 stay Tukey).



Parse is DONE (gt_spectrum, commit 73dca96). This note scopes the COMPUTE (the
save tables) so it lands as one focused pass. Ground truth: `oracle/fortran/
spcdrv.f` (997 lines, the driver) + the leaf routines. Corpus specs:
`tests/corpus/extra/airline_spectrum.spc` (type=periodogram) and
`airline_spectrum-arspec.spc` (type=arspec, maxar=30, qcheck=yes). Both save
`sp0 sp1 sp2 spr st0 st1 st2` (7 tables). Goldens under `tests/golden/extra/…`.

## KEY: spectru.cpp is NOT reusable
`core/src/seats/spectru.cpp` is the SEATS *theoretical* (model-based, func0/fbis/
minim) component spectrum. The spectrum{} diagnostic is the *empirical* spectrum of
data series — different math. Port fresh.

## The 7 tables (spcdrv.f), each = spectrum of a series over a span, at 61 freqs
| table | header | series (Muladd path) |
|---|---|---|
| sp0 | 10*Log(Spectrum_AdjOri) | adjusted original: srs=Stcsi (B1) for Spcsrs>=2, + addmul Stex back if Lx11 & Spcsrs==2 (spcdrv.f:163-177); else Series with Adj{ls,ao,tc,so} divided out |
| sp1 | 10*Log(Spectrum_SA)  | SA series: Stci (D11) [or Stcime if modified] (spcdrv.f:301-320) |
| sp2 | 10*Log(Spectrum_Irr) | irregular: Sti (D13) (spcdrv.f ~440-465) |
| spr | 10*Log(Spectrum_Rsd) | regARIMA residuals via spcrsd.f (269 lines) |
| st0/st1/st2 | Tukey(Spectrum_*) | Tukey-smoothed spectrum of the SAME 3 series (AdjOri/SA/Irr). 41 freqs, not 61. |

## Frequency grid (VERIFIED against golden) — specpeak.f:135-143
1-based frq(i)=(i-1)/120 for i=1..61 (Pos 0..60, 0→0.5), EXCEPT the trading-day
substitutions: frq(43)=0.3482 (Pos42), frq(42)=0.3482-1/120, frq(44)=0.3482+1/120;
frq(53)=0.432 (Pos52), frq(52)=0.432-1/120, frq(54)=0.432+1/120. savspp.f prints
Sx(0:60)/Frq(0:60) — 0-based Pos, so table Pos i = spgrh/spgrh2 Sxx(i+1). (The Tukey
tables use a different 41-point grid — read the Tukey/getTPeaks path.)

## Compute pipeline per table (periodogram type=1)
1. build srs (per the table above).
2. detrend: `gendff(srs, l0, Posfob, tmpsrs, l2, Taklog=(Muladd!=1), Logten=F, Spdfor)`
   — gendff.f: optionally log, then difference Spdfor times (in place, high→low).
   Spdfor default: Lmodel ? max(Nnsedf+Nseadf-1,1) : 1 → **1 for airline** (d=1,D=1).
   Span: ipos=dfdate(Bgspec,Begbk2)+1; l0=ipos-Spdfor (clamped to Pos1ob), l1=ipos.
3. `spgrh2(tmpsrs, orisxx, frq, l1, Posfob, 61, Ldecbl=T)` — spgrh2.f: periodogram
   Sxx(i)=(Σx·cos)²+(Σx·sin)² /n, then 10*log10 (PORTED-READY, trivial, 46 lines).
4. emit orisxx(1:61) as Pos 0..60 with frq.

## arspec type=0 path
Replace spgrh2 with `spgrh` (spgrh.f, 100 lines) — AR spectrum via `sautco`
(autocovariance) + `sicp2` (Levinson-Durbin AR fit w/ AIC order select, ifpl=Mxarsp
or 30*Ny/12) + `decibl`. Needs sautco.f + sicp2.f + decibl.f ported too.

## Leaf routines to port (sizes)
gendff.f(35, trivial) · spgrh2.f(46, trivial) · spgrh.f(100) · sautco.f · sicp2.f ·
decibl.f · spcrsd.f(269, residual spectrum) · the Tukey smoother (in spcdrv/getTPeaks).

## Wiring
- gt_spectrum currently only parses; add a spcidx-style capture of the resolved
  options (Spctyp/Spcsrs/Spdfor/Mxarsp/Ldecbl/qcheck/…) onto ctx.
- Call a new `run_spectrum(ctx)` from the x11 driver AFTER x11pt3 (like spcdrv is
  called from x11ari.f:287 after the adjustment) — the SA/Irr/AdjOri series are the
  already-bit-exact Stci/Sti/Stcsi. Residual (spr) needs the regARIMA residuals
  (ctx has them post-estimate) via spcrsd.
- Emit tables in tools/x13run_x11.cpp (sp0/…/st2). Add tests/parity/
  test_spectrum_tables.py gating each saved table vs the golden (arithmetic →
  ~1e-10). Start with periodogram sp0/sp1/sp2 (pipeline above), then spr (spcrsd),
  then Tukey st0/st1/st2, then the arspec spec (spgrh/sautco/sicp2).

## Suggested increments (each committable)
1. gendff + spgrh2 + run_spectrum + harness emit + test → gate sp0/sp1/sp2 for the
   periodogram spec.
2. spcrsd → gate spr.
3. Tukey → gate st0/st1/st2.
4. spgrh/sautco/sicp2/decibl → gate the whole arspec spec.
