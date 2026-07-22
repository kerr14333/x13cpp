# spectrum{} compute — port plan (execution-ready)

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
