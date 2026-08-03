# X-11 Scouting Report — Moving-Average Seasonal Adjustment (M5)

> ## READ THIS FIRST — every "UNPORTED" / "defer" below is a 2026-07-20 reading
>
> **This document is a scouting and planning record, not a status board.** It
> owns *how X-11 works* and *what was measured while scouting it*, which is
> durable. It does NOT own what is ported — `docs/WALLS.md` (generated from the
> engine's own refusals) and `tools/SESSION_HANDOFF.md` do, and they cannot go
> stale the way a hand-written list can.
>
> The banner is here because the plan sections below are dated and the reference
> sections (§1–§6) were not: a reader landing mid-file saw "**UNPORTED —
> diagnostics**" with no date on screen. Corrected in place, 2026-08-03, in the
> style of the 2026-07-27 refresh already in §2.
>
> **Landed since 2026-07-20**, i.e. every one of these now gates bit-exact and
> the claims below that call them unported or deferred are wrong:
> `x11pt1`–`x11pt4` and `x11ari` themselves; the whole diagnostics front
> (`ftest`, `mstest`, `kwtest`, `combft`, `f3cal`, `sumry`, `avedur`, plus
> `prtd8b`/`prtd9a`, which write onto `ctx` rather than printing); `chktrn`;
> `ssrit` (`slidingspans{}`) and `getrev` (`history{}`); `rndsa`, `shrink`,
> `qmap`/`qmap2` (`force{}`); `makadj`/`tdlom` and the prior-TD block;
> `xrgdrv`/`x11mdl`/`x11aic`/`x11ref` (the whole `x11regression{}`
> sub-milestone, including `span=` both halves); `spcdrv` (`spectrum{}`) and
> `agr*` (`composite{}`).
>
> **Still accurate below**, checked rather than assumed: the print/save emitters
> (`table`, `punch`, `prttrn`, `x11plt`, `prtf2`, `fgen`, `prtagr`, `pragr2`,
> `writln`) are deferred *by project policy* — results live on the result object
> — and `svchsd`, the %-change standard-deviation savelog, is genuinely still
> open. `pe5`/`pe6` being gated is not evidence otherwise: those are the E5/E6
> tables in percent, not `svchsd`'s std-dev field.

> **STATUS (2026-07-20, updated): Tiers 0–3 leaves DONE + unit-tested; Tier 4
> drivers ALL DONE (`vtc`, `sfmsr`, `si`, `tdxtrm`).** Tier 0–3 (28 routines) in
> `core/src/x11/{x11filt,x11seas,x11xtrm}.cpp`: divsub, addmul, logar, antilg,
> setmv, change, chkzro, divgud, averag, hender, apply, ends, endsf, hndend,
> hndtrn, fis, vsfa, vsfb, vsfc, xtrm, sdxtrm, wtxtrm, replac, weight, vtest,
> entsch, trbias, rho2 — gated by test_x11 / test_x11b. Tier 4 drivers `vtc`
> (Henderson-length select) + `sfmsr` (MSR global seasonal-filter selection) live
> in new `core/src/x11/x11drv.cpp` (ctx-first); `tdxtrm` (extreme-irregular
> AO/calendar) landed in `x11xtrm.cpp`. All build clean; **not yet unit-gated**
> (they need a live X13Context — gate arrives with the x11pt2 spine).
> **Tier 5 CORE now DONE (`setxpt`, `forcst`, `x11int`, `extend`)** in
> `x11drv.cpp`; `dpow_ri` (gfortran real**int) exported via numeric.hpp for
> `forcst`. NOTE `x11ref` (162) is NOT base-decomposition — it computes X-11
> **regression** factors (TD/holiday from the Xy/B matrix by Rtype) and belongs
> with the deferred x11regression sub-milestone (`x11mdl`/`x11aic`); not needed
> for `airline_x11-default`. Also landed a codegen fix: `xtrm_cmn.Stdev` was
> mis-sized 76 (should be 86) from a `PYRS` collision between `srslen.prm` (85)
> and stale `srslen.i` (75) -- see FABLE_REVIEW item A.
> **Tier 6 spine STARTED: `x11pt1` DONE** (→ B1 input) in new
> `core/src/x11/x11parts.cpp`. Base + prior-adj (Sprior) + prior calendar/holiday
> (Faccal/X11hol) paths ported with ported leaves (copy/divsub/addmul/setmv); the
> user prior-TD branch (Kswv==1, `tdprior`) now ports `pritd` bit-exact (a4 +
> d10-d13); the OLS-estimated x11-regression-TD branch (Ixreg>=2 & Axrgtd) still
> fatals via a local not_ported. Prints dropped.
> **NEXT: `x11pt2` (954 lines) — the B1→D7 iterated MA decomposition heart**,
> where si/vtc/sfmsr/vsf*/xtrm/replac get wired into the B/C/D passes. Port in
> sections (B pass → C pass → D pass). Then `x11pt3` (D8–D16 finals), `x11pt4`
> (E/F), and the `x11ari` driver + ctx population from the estimate/forecast stage
> to run end-to-end. First gate: `airline_x11-default` B1 (after x11pt1 wiring),
> then D10/D11/D12/D13 (after the full spine).
>
> **STALE-CLAIM CORRECTION (2026-07-20): there is no "MSR trio."** An earlier
> revision grouped `sfmsr`/`getsmat`/`gttrmo` as the "MSR seasonal-filter trio."
> That was wrong — reading the sources:
> - `sfmsr.f` (101) — the actual MSR global seasonal-filter selector. **DONE**
>   (`x11drv.cpp`). Sources `vsfa`/`vsfb`'s explicit `muladd/psuadd/rati/ratis`
>   and `lterm/lter/ksect/shrtsf` args from `ctx.x11opt`+`ctx.x11msc`+`ctx.work2`;
>   this alone unblocks `si` (no other routine needed).
> - `getsmat.f` — a **generic submatrix extractor** (`getSMat`/`getSRMat`); a
>   linear-algebra utility, NOT part of X-11 seasonal-filter selection. Unported;
>   pull in only if/when an actual caller needs it.
> - `gttrmo.f` — an **Edit-format series file reader** (I/O); unrelated to MSR and
>   in the deferred file-I/O bucket. Do not port for X-11 decomposition.
> - Also corrected: `setdp` was listed as "missing from Tier 0/3" but is **already
>   ported** (inlined `specparse.hpp:84`, alongside setint/setlg/cpyint).
>
> **Conventions confirmed while porting (vtc/sfmsr/tdxtrm):**
> - Tier-4 drivers take `X13Context& ctx` FIRST, alias the COMMON structs
>   (`ctx.x11opt`/`x11ptr`/`x11msc`/`work2`/`xclude`), and thread ctx table fields
>   into leaves via `.data()`. 1-based lambda views keep the arithmetic diffable.
> - `hndtrn` sig is `hndtrn(stc, stci, lfda, lldaf, nterm, tic&, lend, lsame,
>   tru7hn)` (9 args); `tru7hn` comes from `ctx.x11msc`, NOT x11opt.
> - `si` also calls deferred print/save (`table`/`punch`) — drop those (deferred).
>   Pseudo-additive (`Sti=Stsi-Sts+1`) and `Kfulsm==2` (copy SI through) bypass
>   divsub. Its `xtrm` call must source the richer ported sig's extra args
>   (imad/sigmu/sigml/lsp/stdper/csigvc) from x11opt + xtrm COMMON.


Scouted 2026-07-20 against `oracle/fortran` (v1.1 b61). Scope: the classic
X-11 seasonal decomposition (trend / seasonal / irregular) driven by
`x11ari.f` → `x11pt1/2/3/4`. (Historical note: `core/src/x11/` was empty at first
scout; Tiers 0–3 + drivers vtc/sfmsr/tdxtrm are now ported — see STATUS header.)
SEATS (`seats*.f`), sliding-spans (`ss*.f`), revisions
(`rev*.f`), aggregate/composite (`agr*.f`), and x11regression (`x11mdl/x11aic`)
are separate, later sub-milestones and are NOT in scope here.

## x11 driver assembly plan (x11ari base path -> airline B1..D7 gate) — 2026-07-20

**Status:** x11pt1, x11pt2, chktrn, adjreg ALL PORTED. What remains for the first
running gate is the **driver that assembles the pipeline** + a harness exe + the
parity test. The full `x11ari.f` (389 lines) is mostly deferred (SEATS/spcdrv/agr/
timers/prints); the base airline spine is:

```
[trnaic if Fcntyp==0]  (ported)   -- transform auto-select
 -> x11pt1              (ported)   -- prior adjust, Sto/Stcsi over OBSERVED span
 -> arima(...)          (glue)     -- estimate + forecast + EXTEND + adjreg
 -> x11pt2              (ported)   -- B1..D7
 -> x11pt3 -> x11pt4    (later)
```

**The `arima` glue is the remaining work.** The C++ `run_m2` (run_pre_model.cpp)
already does arima.f's front: prep trnsrs from Sto -> trnaic -> trnfcn transform
-> regvar -> estimate/automd/outlier -> `fcstout` (forecasts on ctx.forecasts).
The MISSING tail (arima.f l.1136-1290) is:
1. `prtfct` -> **fcstx** = the *transformed-scale* point forecasts, length Nfcst
   (NOT untfct; extend consumes the transformed values). fcstout stores the
   transformed fcst/se on ctx.forecasts -- extract fcstx from there (or call the
   forecast leaf to fill an fcstx buffer). Nbcst=0 for airline -> skip mkback/bcstx.
2. `setdp(0,PLEN,orix)` then `extend(ctx, trnsrs, begxy, orix, extok, lam, fcstx,
   bcstx)` (PORTED) -> fills padded `orix` with observed+forecast(+backcast).
   Guard: `Ldestm && ((Nfcst>0 && Nfdrp>0) || Nbcst>0)`; else `copy(trnsrs,Nobspf,
   1, orix(Pos1ob))`.
3. `adjreg(ctx, orix, orixmv, orixot, ftd..fhol=0, fcntyp, lam, Nrxy, n)` (PORTED)
   -> fills **Stcsi** (the B1 input!), Series forecast tail, Stocal. THIS is what
   makes Stcsi valid for x11pt2 (x11pt1 only set the observed span).

**Open items -- RESOLVED (2026-07-20, read-only trace):**
- **fcstx extraction -- DONE.** `ctx.forecasts.trnfct` is the transformed-scale
  forecast vector (FcstOut.trnfct, length nfcst; .fcst is original-scale). That is
  exactly the `fcstx` extend/adjreg consume. Populated by `fcstout`; the raw leaf
  `fcstxy(ctx, fctori, nfcst, fcst, se, ...)` fills the transformed fcst/se directly
  if the driver prefers to bypass fcstout's original-scale mapping.
- **setxpt / x11int placement -- DONE.** Oracle: `setxpt` is called in the INPUT
  setup (editor.f:233, `CALL setxpt(Nfdrp,lsadj,Fctdrp)`), after the bookkeeping at
  editor.f:224-233 (Frstsy/Nomnfy/Nfdrp/Nobspf/**Nofpob/Nbfpob/Lsp=1**); the
  x11ari.f:149 setxpt is only the degenerate Same/!havmdl branch. `x11int` is called
  immediately BEFORE x11ari (x12run.f:174 -> :181; sspdrv.f:116 -> :180). So the
  driver order is: [setxpt in setup] -> x11int -> trnaic -> x11pt1 -> arima-glue ->
  x11pt2. run_m2 (run_pre_model.cpp:89-99) ALREADY computes Frstsy/Nomnfy/Nfdrp/
  Nobspf; the driver only adds Nofpob/Nbfpob/Lsp + the setxpt call + x11int.
- **run_m2 reconciliation -- DONE.** No existing C++ caller of setxpt/x11int/x11pt1/
  x11ari -- run_x11 is greenfield assembly. run_m2 sources the series from
  `ctx.arima.y` (span-offset); x11pt1 reads `ctx.inpt.series`/`ctx.inpt.orig`. The
  single build-time check: ensure ctx.inpt.series/orig hold the observed series
  before x11pt1 (populated by the parse/getsrs stage). x11pt1's Sto = prior-adjusted
  original series == run_m2's padj -- same quantity.

**NEWLY-FOUND PREREQUISITE (2026-07-20): the x11-option defaults are unported.**
The C++ parse (gtinpt.cpp/readers_spec.cpp) captures only x11_mode->muladd, divpwr,
kfulsm -- but x11pt2/setxpt/x11int need the full x11opt/xtrm scalar set, and NONE of
the getx11.f (576-line option reader) defaults are populated. `ctx.x11opt.ny` is
never set in C++ at all. For `airline_x11-default` (no x11 options) ONLY the
defaults matter -- porting getx11's keyword parsing is NOT needed for the first
gate, just the default block. Exact defaults (gtinpt.f:323-381 + resolution):
```
Muladd = NOTSET -> 0 (mult; gtinpt.f:954-956 from x11 mode)   Tmpma = Muladd (:970)
Kfulsm = 0     Sigml = 1.5    Sigmu = 2.5    Ktcopt = 0        Ksdev = 1  (NOT 0!)
Imad = 0       Shrtsf = F     Psuadd = F     Noxfct = F        Tru7hn = F
Lterm = NOTSET -> 6 (auto MSR; seasonalma default)             Ny = Sp
Kersa: find default (not in the :323-381 block -- likely x11int or a later init)
Csigvc(1..Sp) = F (calendarsigma default none)
```
So the gate needs a small `getx11 defaults` step (port the ~13 scalar defaults into
the x11 parse path, gtinpt.cpp) BEFORE the run_x11 driver. Not the full 576-line
reader.

**Deliverable:** a `run_x11` driver (core/src/driver/) that runs the spine into a
live ctx + a `x13run_x11` harness exe (mirror x13run_m3.cpp) that dumps the B/C/D
save tables, then a `tests/parity/test_x11_*.py` gating airline_x11-default
`.b1 .d10 .d11 .d12 .d13` (+ the B2..D7 intermediates) bit-exact vs oracle goldens
(24 x11 golden dirs ship them at 15-digit precision).

## x11pt2 port plan (the B1->D7 heart, 954 lines) — scouted 2026-07-20

> **Refreshed 2026-08-03.** x11pt2 is ported and gated. The UNPORTED tags in the
> flow sketch below were true when scouted and are not now: `makadj`/`tdlom`
> (the TD/length-of-month prior block) and `ssrit` (sliding spans) and `ftest`
> and `chktrn` have all landed. Kept unedited because the FLOW is what this
> section is for -- the call order, the gating conditions and the
> base-case reachability check under it are still the accurate part.

Structure = the classic B/C/D iteration; **most CALLs are already-ported leaves**
(averag, divsub, addmul, setdp, si, vsfa, vsfb, vtc, forcst, logar, copy). Flow:
- **Part A-of-B TD prep** (l.116-205): `makadj` + `tdlom` (TD & length-of-month
  prior adj into Factd/Stocal) — **UNPORTED**; gated by `Adjtd`. `ssrit` (sliding
  spans) UNPORTED, off for base. Builds Faccal from Factd/Fachol/Facao/... via
  addmul.
- **Part B (l.353-460):** logadd `logar(Stcsi)`; B1 -> `averag(Stcsi,Stc,2,Ny)`
  centered 2xNy MA -> `divsub` SI ratios (Stsi) -> `ftest` seasonality F-test
  (**UNPORTED**, diagnostic/print-gated) -> trend B2.
- **Part B/C/D seasonal (l.465-560):** `si` (extreme SI replace) -> `vsfb`
  seasonal MA -> `forcst` end-fill -> `divsub` preliminary SA (Stci).
- **Section 2 trend (l.563-627):** `vtc` variable trend-cycle -> `chktrn` trend
  constraint check (**UNPORTED**, sets oktrn) -> D7 trend -> `divsub` SI ratios.
- **Part C/D refine (l.627-end):** `si` -> `vsfa`/`vsfb` -> preliminary SA,
  iterating to D7.

**Base-case reachability (CHECKED 2026-07-20):** the TD/lom block
(`makadj`+`tdlom`) is gated `IF(Ixreg.ne.2 .AND. Priadj.gt.1 .and. goodlm)`
(x11pt2.f:115) — i.e. only when length-of-month / leap-year / model-TD prior
adjustment is active. For `airline_x11-default` (Priadj<=1) it is **SKIPPED**.
`ssrit` is `Issap==2` (sliding spans) — off. So **makadj/tdlom/ssrit are NOT on
the airline base path** — stub them with `not_ported` guards keyed to those flags
(as x11pt1 does for prior-TD), don't port yet.
**The only real base-path blocker is `chktrn`** (trend-cycle constraint check,
x11pt2.f:605, sets `oktrn`) — verify it runs for airline and port it. `ftest`
(seasonality F-test, l.437) is `Prttab(LXEB1F)`-gated diagnostic — confirm it
feeds no compute state, then drop as deferred.
**Port order:** (1) `chktrn`; (2) confirm `ftest` deferrable; (3) x11pt2 base
path section by section (Part B -> B/C/D seasonal -> Section-2 trend -> C/D
refine), stubbing makadj/tdlom/ssrit; gate `airline_x11-default` B1..D7 as
sections land.

Corpus payoff: **50 / 76 specs use `x11{}`**, and **24 of them ship full B/C/D
save-table goldens** (`.b1 .d1 … .d10 .d11 .d12 .d13` at 15-digit precision).
X-11 sits directly on top of the already-ported regARIMA engine (estimate,
regvar, forecast, outlier, automdl, transform) — it consumes the modeled +
forecast-extended series and produces the seasonally-adjusted output.

## x11pt4 port plan (E1->F4) — scouted 2026-07-20

Target `oracle/fortran/x11pt4.f` (778 lines), the LAST X-11 spine chunk (PARTS
E1->F4: modified-table percent-changes, annual totals, robust SA, final adj
ratios, then PART F summary/quality measures + F2/F3 M-stats). Called from
`x11ari.f:262` after x11pt3. **Key structural fact: x11pt4 does NOT compute the
E1/E2/E3 modified series** — `Stome`(E1)/`Stcime`(E2)/`Stime`(E3) are already
built in x11pt3; x11pt4 only *emits* them (deferred print) and *consumes* them.
What x11pt4 genuinely computes into save-table state on the base path: the E5/E6/
E7/E8 percent-changes (via ported `change`), E11 robust SA + E18 final adj ratios
(inline loops), F1 MCD moving average (ported `averag`), and all the PART-F
summary/variance/M-stat diagnostics that feed the `.udg`.

### 1. Section-by-section flow (line ranges)

- **Prologue / E0 plot (l.64-80):** `dvec(1)=0`; E0 chart `x11plt` gated
  `Prttab(LX11E0)` — print, deferrable.
- **E1/E2/E3 emit + residual-seasonality test (l.82-109):** `prtagr` of `Stome`
  (E1), `Stcime` (E2), `Stime` (E3) — print/save of x11pt3-computed arrays;
  `ftest(Stcime,...)` residual-seasonality F-test (l.99, Iagr<4 branch).
- **E4 annual totals ratios (l.110-161):** `IF(Kfulsm.eq.0)` + `Prttab(fext)`;
  builds `rd1/rd2` (orig/SA annual-total ratios) -> `table` (print). No golden.
- **Zero/pseudo-add guards (l.162-176):** composite `divsub(O5..)` (Iagr==4) +
  `chkzro` — both gated OFF base.
- **E5 orig %-change (l.177-189):** `mfda=Pos1ob+1`; `change(Series,Temp,mfda,
  Posfob)` -> `pragr2` E5 -> `svchsd` savelog `e5`.
- **E6 SA %-change + E6.A/E6.R (l.190-236):** `IF(Kfulsm.eq.0)`: `change(Stci,
  Temp)` -> E6; `Iyrt>0` -> E6.A (`change(Stci2)`); `Lrndsa` -> E6.R
  (`change(Stcirn)`). Last two gated OFF base.
- **E7 trend %-change (l.237-251):** picks `Stc2` vs `Stc` (base -> `Stc`);
  `change` -> `pragr2` E7 -> `svchsd e7`.
- **E8 calendar-adj orig %-change (l.252-264):** base `change(Stocal,Temp)` ->
  E8. (Iagr==4 uses `O5`.)
- **E11 robust SA (l.269-277):** `stcirb=Series-Stome+Stcime` -> `prtagr` E11.
- **E18 final adj ratios A1/D11 (l.278-304):** loop `stcirb=thisob/Stci` (zero/
  neg handling sets `pre18b`) -> `prtagr` E18.
- **EB total adj factors (l.305-319):** `IF(pre18b.or.Savtab(LXEEEB))` -> `divsub`
  + `prtagr`. Base airline: pre18b=F and `eb` not saved -> OFF.
- **PART F header (l.320-336):** `Kpart=6`; `allgud` bookkeeping (base skip).
- **Priors/TD variance (l.337-360):** `Kfmt!=0` -> `sumry(Sprior)`+`vars(Sprior)`
  (Vp); Vtd block `sumry(Faccal)`+`vars(Faccal)`. Both gated OFF base.
- **Irregular summary (l.361-362):** `sumry(Sti,...)` (Ibar/Isq) + `avedur(Sti,
  Adri)`. **On base.**
- **Stome/Stime/Sts/Stc summaries + issame notes (l.373-440):** `sumry(Stome)`,
  `sumry(Stime)`+`vars(Stime)`->Vi, `sumry(Sts)`+`vars(Sts)`->Vs (Kfulsm!=2
  branch), `sumry(Stc)`; `issame` guards drive "diagnostics cannot be generated"
  notes and the `IF(lsame)RETURN` at l.546.
- **Linear-trend removal from C (l.441-477):** `logar(Stc)` (Muladd!=1),
  least-squares slope -> `trend[]`, `antilg`, `divsub(Temp,Stc,trend)`,
  `avedur(Stc,Adrc)`, `vars(Temp)`->Vc. All ported leaves + inline.
- **Orig variance (l.478-517):** outlier re-adj (OFF base) + `sumry(Series,Obar)`.
- **vo / lsame gate (l.527-546):** `divsub(Temp,Stome,trend)`, `vars(Temp)`->vo;
  `IF(lsame)RETURN` (base proceeds).
- **Variance decomposition + I/C (l.547-581):** normalize Vp/Vtd/Vc/Vs/Vi by vo,
  `Rv` sum, `Osq2` relative contributions, `Smic=Ibar/Cbar` I/C ratios. Pure.
- **Stci/Stcime summaries (l.582-654):** outlier re-adj (OFF base) +
  `sumry(Stci,Cibar)`+`avedur(Stci,Adrci)`, `sumry(Stcime,Cimbar)`.
- **MCD + F1 (l.664-696):** derive `Mcd` from `Smic`, `m,n`; `averag(Stci,Stmcd,
  Pos1bk,Posffc,m,n)` MCD MA; `sumry(Stmcd,Smbar)`+`avedur(Stmcd,Adrmcd)`;
  `prtagr` F1; restore Stmcd from Temp.
- **Autocorr + F3 calc (l.697-713):** `varian(Sti)`, `setdp`, `Autoc[]` loop;
  `f3cal(Sts,ifail)` computes M1-M11/Q/Q2.
- **F2/F3 emit (l.714-727):** `fgen` (print, Prttab-gated) + `svf2f3` (savelog).
- **Tail plots (l.728-778):** TD table `prtdtb`, ratio plots R1/R2 `x11plt` — all
  print-gated, deferrable.

### 2. Every CALL: PORTED vs UNPORTED

> **Refreshed 2026-07-27.** This table was written 2026-07-20, before x11pt4
> landed. Ten rows that said UNPORTED are now ported and are corrected in place:
> `sumry`, `avedur`, `vars`, `varlog`, `varian`, `issame`, `isfals`, `f3cal`
> (all `core/src/x11/x11summ.cpp`), `ftest` (`x11tests.cpp`) and `svf2f3` (the
> `f2.*`/`f3.*` savelog block, emitted from `tools/x13run_x11.cpp`). The
> remaining UNPORTED rows are print/save emitters this port defers by design,
> plus `svchsd` (the `%`-change std-dev savelog), which is genuinely still open.
> Rows elsewhere in this document were NOT re-verified.

| CALL (lines) | status | maps to / role |
|---|---|---|
| `change` (182,194,219,230,244,246,257,259) | **PORTED** | `x11filt.cpp:58` (has muladd+Gudval) |
| `divsub` (168,312,314,467,480-3,528,585-91,622,628) | **PORTED** | `x11filt.cpp` |
| `addmul` (410-1,520-3,609-15,645-51) | **PORTED** | `x11filt.cpp` |
| `divgud` (378-9,486-9,595-601,632-8) | **PORTED** | `x11filt.cpp:72` |
| `chkzro` (171,173) | **PORTED** | `x11filt.cpp:82` (OFF base) |
| `averag` (680) | **PORTED** | `x11filt.cpp:104` (MCD MA) |
| `logar`/`antilg` (451,464,465) | **PORTED** | `x11filt.cpp:44/48` |
| `copy` (377,413,485,525,593,617,630,653) | **PORTED** | x11 core copy |
| `setdp` (705) / `setlg` (267,334) | **PORTED** | `specparse.hpp:84/85` |
| `copylg` (266,334) | **PORTED** | `strvec.cpp:96` |
| `sumry` (339,358,361,382,393,425,428,517,604,641,685) | **PORTED** (x11summ.cpp) | `sumry.f` (84) — per-period summary measures (Xbar/Xbar2/Xsq/Xsd), uses Gudval+Muladd |
| `avedur` (362,476,605,686) | **PORTED** (x11summ.cpp) | `avedur.f` (51) — average duration of run |
| `vars` (fn; 340,359,394,426,477,529) | **PORTED** (x11summ.cpp) | `vars.f` (14) — dispatch: Muladd!=1 -> `varlog`, else `varian` |
| `varlog` (via vars, base) | **PORTED** (x11summ.cpp) | `varlog.f` (44) — log-variance |
| `varian` (fn; 703 + via vars add-mode) | **PORTED** (x11summ.cpp) | `varian.f` (27) — plain variance |
| `issame` (fn; 383,429,492 + Sti/Stc/Series) | **PORTED** (x11summ.cpp) | `issame.f` (29) — constant-series predicate (drives lsame/RETURN) |
| `isfals` (fn; 335) | **PORTED** (x11summ.cpp; see CB-18) | `isfals.f` (18) — all-false Gudval predicate (OFF base) |
| `f3cal` (713) | **PORTED** (x11summ.cpp) | `f3cal.f` (142) — F3 M1-M11/Q/Q2; needs `sdev.f` (46) |
| `ftest` (99,101) | **PORTED** (x11tests.cpp) | `ftest.f` (294) — residual-seasonality F-test (diagnostic) |
| `x11plt` (75,77,752,754,771,773) | **UNPORTED (print)** | `x11plt.f` — E0/R1/R2 charts, DEFER |
| `prtagr` (85,92,107,275,302,316) | **UNPORTED (print/save)** | table emitter, DEFER |
| `pragr2` (186,198,220,231,248,261) | **UNPORTED (print/save)** | E5-E8 emitter, DEFER |
| `table` (156) | **UNPORTED (print)** | E4 emitter, DEFER |
| `svchsd` (189,201,223,234,251,264) | **UNPORTED (savelog)** | %-change std-dev -> `.pe5/.pe6/.pe7/.pe8`, Lsumm-gated, DEFER |
| `fgen` (719,724) | **UNPORTED (print)** | F2/F3 table printer (`prtf2`/`f3gen`), DEFER |
| `svf2f3` (720,725) | **PORTED** (emitted by tools/x13run_x11.cpp) | writes `f2.*`/`f3.*` -> `.udg`; reads computed COMMON state |
| `prtdtb` (733) | **UNPORTED (print)** | TD-type table, DEFER |
| `writln` (many) | **UNPORTED (print)** | stderr notes, DEFER |

### 3. Base-path (airline_x11-default: mult, no priadj/x11reg/ss/composite) reachability

Confirmed flag values for the base case (Muladd=0 mult per `x11opt.cmn:13`;
Iagr<4 non-composite; Cnstnt=DNOTST; Kfulsm=0; Psuadd=F; no outliers/TD/holiday/
priors -> Adjls=Adjao=Adjtc=Adjusr=0, Adjtd<=0, Kswv=0, Axrgtd=Axrghl=F,
Adjhol!=1, Khol!=2, Ixreg<=0, Kfmt=0, Iyrt=0, Lrndsa=F):

**GATED OFF base (do NOT port / stub):**
- `chkzro` (l.169) — guard `Muladd.ne.1.and.(Psuadd.or.(.not.dpeq(Cnstnt,DNOTST)))`
  = T.and.(F.or.F) -> **F**.
- E4 composite `divsub(O5..)` (l.168) and all `Iagr.eq.4`/`Iagr.ge.4` branches — OFF.
- `copylg`/`setlg` Gudval save (l.266-7,334-5) — guard `.not.dpeq(Cnstnt,DNOTST)`
  -> **F**; so `allgud` stays T, every `IF(allgud)` takes the T-branch (which with
  Adj*=0 is a no-op) and the `divgud`/`copy`-restore ELSE arms never run. `isfals`
  never called.
- `sumry(Sprior)`/`vars(Sprior)` (l.339-40) — `Kfmt.ne.0` -> **F** (Vp=0 path).
- `sumry(Faccal)`/`vars(Faccal)` Vtd (l.358-9) — the l.349 guard is **T** for base
  -> Vtd=0 branch (Faccal summary skipped).
- E6.A (`Iyrt>0`, l.209), E6.R (`Lrndsa`, l.228), EB (`pre18b/Savtab(LXEEEB)`,
  l.309) — all OFF.
- All `Adj*`/`Fin*` outlier re-adjustment `divsub`/`addmul`/`divgud` pairs — no-ops
  (flags 0).

**ON base, compute-bearing (save-table state):**
- `change` (E5/E6/E7/E8) -> `.e5/.e6/.e7/.e8` goldens — **ported leaf, no blocker**.
- E11 (`stcirb=Series-Stome+Stcime`) -> `.e11`; E18 (`thisob/Stci`) -> `.e18` —
  inline, no leaf.
- `averag` F1 (MCD MA) -> `.f1` — ported leaf.
- `sumry`/`avedur`/`vars`(+`varlog`)/`varian`/`issame` (l.361-712) — **on base**,
  feed the PART-F variance decomposition, `Smic`->`Mcd` (which F1's m,n depend on),
  `Autoc`, and the `.udg` `f2.*` fields.
- `f3cal` (l.713) -> `.udg` `f3.m01..m11`,`f3.q` — on base.
- `ftest` (l.99) residual-seasonality — on base but **diagnostic/savelog only**
  (feeds `f2.idseasonal`/`fsb1`-type fields); no numeric table golden.

**Print-gated / deferrable (numeric core not affected):** `x11plt`, `prtagr`,
`pragr2`, `table`, `svchsd`, `fgen`/`prtf2`/`f3gen`, `svf2f3`, `prtdtb`, `writln`.
Same DEFER policy as the rest of the spine — compute into ctx, diff via harness.

**Dependencies from x11pt3 (must already be populated):** `Stome`(E1),
`Stcime`(E2), `Stime`(E3), `Stocal`(E8; = Series when no calendar adj), plus the
usual `Series/Stci/Stc/Sts/Sti` and pointers `Pos1bk/Pos1ob/Posfob/Posffc`. Also
consumes x11pt2/pt3 scalars for f3cal: `Ratic`,`Ratis`,`Test1`,`Test2`,`L3x5`,
`Lstabl`,`Vp`,`Vi`. Wire these before expecting `.f1`/`.udg` parity.

### 4. Recommended base-path port order + first blocker leaf(s)

1. **Transcribe x11pt4 l.64-319 (E-tables through E18)**, stubbing all print/save
   CALLs and the gated-OFF branches. Uses ONLY already-ported leaves (`change`,
   `divsub`). **First real blocker leaf: NONE** — this gate needs zero new leaves,
   only the spine transcription + x11pt3 arrays. Gate `.e5/.e6/.e7/.e8/.e11/.e18`.
2. **Port the summary-measures cluster** `sumry`(84) + `avedur`(51) +
   `vars`(14)/`varlog`(44)/`varian`(27) + `issame`(29)/`isfals`(18), then
   transcribe PART F l.320-712 (variance decomposition, `Smic`/`Mcd`, `Autoc`,
   MCD `averag`). Gate `.f1`. **First real blocker leaf here: `sumry`** — every
   PART-F path funnels through it (11 call sites), and `Mcd` (hence F1's filter
   length) depends on `Smic` from `sumry(Sti)`/`sumry(Stc)`.
3. **Port `f3cal`(142) + `sdev`(46)**; transcribe l.713. Then the F-test
   diagnostic cluster `ftest`(294) + `kwtest`(99) + `mstest`(158) for the
   `f2.fsb1/fsd8/kw/msf` fields. Gate the `.udg` `f2.*`/`f3.*` savelog. Emitters
   (`fgen`/`prtf2`/`f3gen`/`svf2f3`) stay deferred-print, but `svf2f3`'s VALUES
   must be produced into ctx/udg for the harness to diff.

### 5. E/F save-table goldens in the corpus

`airline_x11-default/` ships: **`.e1 .e2 .e3 .e5 .e6 .e7 .e8 .e11 .e18 .f1`** table
goldens (15-digit) + **`.udg`** carrying the full F2/F3 block (`f2.a01..a12`,
`f2.b*`, `f2.c*`, `f2.d/e/f/g`, `f2.ic`, `f2.is`, `f2.mcd`, `f2.fsb1`, `f2.fsd8`,
`f2.kw`, `f2.msf`, `f2.idseasonal`, `f3.m01..m11`, `f3.q`, `d11.f`). Also
`.pe5/.pe6/.pe7/.pe8` (svchsd savelog) and `.paf/.tad` present. **No `.e4`
golden** (E4 print-gated, not save-blessed) despite `e4` in the save list — not a
gate target. The same E/F set exists for the additive/logadd and the
expgs/payems/unrate x11 variants (24 x11 golden dirs total). **Base numeric E/F
gate (`.e5..e18`,`.f1`) needs no new leaf; the `.udg` f2/f3 gate is the driver
for porting the sumry/vars/avedur/f3cal/ftest diagnostic cluster.**

## 1. Entry point & algorithm

```
sspdrv.f:180        CALL x11ari(Lmodel,Lx11,X11agr,Lseats,Lcomp,Issap,...)
x11ari.f:4          SUBROUTINE x11ari  (389 lines) — the X-11 / ARIMA partition driver
```

`x11ari` is reached from the seasonal-adjustment driver `sspdrv` (itself called
from `gtinpt`/`x12run` after model estimation). `ssx11a.f` (287 lines) is only
the **sliding-spans span-setup wrapper** — it sets span dates then calls
`x11ari`; the real computation is entirely in the `x11pt*` "parts". Flow inside
`x11ari` (the direct-adjustment path, `Lx11=.true.`):

1. **`trnaic`** (x11ari.f:84) — automatic transform test if `Fcntyp==0`
   (already scouted for automdl; shared).
2. Prior TD/holiday adjustments via `xrgdrv` (x11regression — PORTED; the
   whole sub-milestone is in `core/src/x11/{xrgdrv,x11reg}.cpp`).
3. **`x11pt1`** (x11ari.f:101, 327 lines) — **prior adjustments → table B1**
   (the modeled, prior-adjusted, forecast-extended input series).
4. **`arima`** (x11ari.f:133) — regARIMA modeling (already ported).
5. **`x11pt2`** (x11ari.f:199, 954 lines) — **X-11 PARTS B1→D7**: the main
   iterated moving-average decomposition (B, C, D passes). This is the heart.
6. **`x11pt3`** (x11ari.f:257, 1291 lines) — **X-11 PARTS D8→D16**: final
   seasonal (D10), final SA (D11), final trend (D12), final irregular (D13),
   D-tables + seasonal F/M diagnostics.
7. **`x11pt4`** (x11ari.f:262, 778 lines) — **X-11 PARTS E1→F4**: modified
   tables, year-over-year, F2/F3 quality stats.
8. Spectral plots (`spcdrv` — PORTED, `driver/run_spectrum.cpp`),
   composite/aggregate (`agr*` — PORTED, `core/src/composite/`).

The classic B/C/D three-iteration structure lives *inside* `x11pt2` (B1→D7) and
`x11pt3` (D8→D16). Each pass is the same kernel sequence: form SI ratios
(`si`) → smooth seasonally with a seasonal MA (`vsfb`/`vsfa`/`vsfc`) → detrend
with a Henderson trend MA (`vtc`→`hndtrn`) → detect/replace extremes
(`xtrm`/`replac`) → recombine (`divsub`/`addmul`), differing only in filter
lengths and which extreme-value pass is active.

### Table storage (`x11srs.cmn`)
```
Sts   — seasonal factors (→ D10)      Stc  — trend cycle (→ D12)
Stsi  — SI ratios                     Stci — trend-cycle input
Sti   — irregular (→ D13)             Stc2 — secondary trend buffer
```
Plus `Orig/Series/Stcsi` (orig + SA series) from `orisrs.cmn`/`x11msc.cmn`, and
span pointers `Pos1bk` (first backcast), `Pos1ob` (first obs), `Posfob` (last
obs), `Posffc` (last forecast) that bracket the padded buffer. The whole
subsystem is 1-based pointer arithmetic over a forecast/backcast-padded array.

## 2. Call graph + leaf-tier port order

(Historical — at first scout nothing in `core/src/x11/` existed and every routine
below was NEW. As of 2026-07-20 Tiers 0–4 + Tier-5 core are ported into
`core/src/x11/{x11filt,x11seas,x11xtrm,x11drv}.cpp`; the "ported?" column tracks
what remains. The Tier 6 `x11pt*`/`x11ari` spine is the live frontier.)

Recommended **leaf-first** order (each tier unit-testable before the next).
**NOTE (2026-07-20): every Tier 0–3 row below is DONE** (see STATUS header) — the
per-row "new" in those rows is the original scouting state, superseded by the
STATUS block. Only the "DONE"/"defer"-marked Tier 4+ rows track live status.

| Tier | routine(s) | lines | role | ported? |
|------|-----------|-------|------|---------|
| 0 | divsub | 26 | mode ÷ (mult) or − (add): the core recombine primitive | new |
| 0 | addmul | 22 | mode × or +: inverse recombine | new |
| 0 | logar / antilg | 16/15 | log / antilog (logadd mode) | new |
| 0 | setmv / setdp / chkzro / change / divgud | 17/15/41/35/27 | small arithmetic/scale helpers | new |
| 0 | averag | 32 | generic M-of-N moving average kernel | new |
| 0 | hender | 29 | N-term Henderson weight generation (half-symmetric) | new |
| 0 | apply (fn) | 20 | apply symmetric weights at one point | new |
| 1 | hndend | 42 | Henderson **end** filters (Doherty 1993, rbeta) | new |
| 1 | ends | 50 | trend end-weight application driver | new |
| 1 | endsf | 39 | seasonal-MA end weights (3x9 / 3x15 tables) | new |
| 1 | hndtrn | 55 | Henderson trend driver (symmetric + ends + 7-term reduce) | new |
| 4 | vtc | 90 | variable trend cycle: I/C ratio → Henderson length select | **DONE** (x11drv) |
| 2 | vsfc | 54 | 2×Nyr MA over the seasonals | new |
| 2 | vsfa | 101 | preliminary seasonal MA pass | new |
| 2 | vsfb | 174 | seasonal MA: 3x3 / 3x5 / 3x9 / 3x15 / stable / 3-term | new |
| 4 | sfmsr | 101 | MSR global seasonal-filter (Lter) selection + vsfa/vsfb pass | **DONE** (x11drv) |
| — | getsmat | 102 | generic submatrix extractor (getSMat/getSRMat) — NOT MSR; linear-algebra util | defer (no caller yet) |
| — | gttrmo | 91 | Edit-format series **file reader** (I/O) — NOT MSR | defer (file-I/O bucket) |
| 3 | xtrm | 185 | extreme-value detection/adjustment (sigma limits) | new |
| 3 | sdxtrm / tdxtrm / wtxtrm | 80/126/40 | sigma computation / TD-in-irregular / extreme weights | **DONE** (tdxtrm in x11xtrm) |
| 3 | replac | 110 | replace extreme SI values with reweighted MA | new |
| 3 | weight | 123 | extreme-value weight curve | new |
| 3 | vtest / entsch | 64/? | sigma-limit (Ksdev) auto-selection | new |
| 3 | trbias | 39 | trend constant-bias correction | new |
| 4 | si | 106 | SI-ratio → seasonal driver (calls vsfa/vsfb/xtrm/replac) | **DONE** (x11drv) |
| 5 | forcst / extend | 41/115 | forecast/backcast extension into the filter window | **DONE** (x11drv) |
| 4 | tdlom / traday / makadj | 63/?/37 | TD & length-of-month prior adjustment inside B | new |
| 5 | setxpt | 25 | span/forecast pointer setup (Pos1bk/Posffc/Fctdrp) | **DONE** (x11drv) |
| 5 | x11int | 56 | X-11 array initialization | **DONE** (x11drv) |
| xr | x11ref | 162 | X-11 **regression** factors (TD/holiday) | defer (x11regression sub-milestone) |
| 5 | getx11 / gtx11d | 576/161 | x11{} arg parser + defaults | new (gt_generic stub today) |
| 6 | x11pt1 | 327 | PART B1: prior adj + forecast-extended input | new |
| 6 | x11pt2 | 954 | PARTS B1→D7: main iterated decomposition **spine** | new |
| 6 | x11pt3 | 1291 | PARTS D8→D16: D10/D11/D12/D13 finals + seasonal tests | new |
| 6 | x11pt4 | 778 | PARTS E1→F4: modified tables, F2/F3 stats | new |
| 6 | x11ari | 389 | top partition driver (wire behind `x11{}` in run_m2) | new |
| dx | ftest / mstest / kwtest / f3cal / sumry / avedur / rndsa / shrink / qmap / qmap2 | 294/158/99/142/84/51/129/79/116/328 | F-tests, M-stats, D9A, quality diagnostics (savelog fields) | **ALL PORTED + gated** (was: defer past first gate) |
| — | table / punch / prttrn / x11plt / fgen / prtf2 | | print engines — **DEFER** (like fcstout/amdprt), still policy | defer |
| — | svf2f3 / prtd8b / prtd9a | | **PORTED** — these write onto `ctx` (the `f2.*`/`f3.*` savelog block, d8b/d9a), they do not print | done |

**First 3–5 to port (why):**
1. **divsub + addmul + logar/antilg + setmv/setdp** — the mode-arithmetic
   primitives. Every table pass ends in one; four modes (mult/add/logadd +
   pseudo-additive) all funnel here. Trivial, and unit-testable against a
   handful of airline B1 values.
2. **averag** — the M×N moving-average kernel underlying every seasonal MA and
   the composite 2×12. Pure, one loop, verify against a hand-computed window.
3. **hender + apply + ends + hndend + hndtrn** — the Henderson trend chain
   (produces the D12 trend family). Numeric and verifiable in isolation for a
   fixed filter length (13-term monthly), which pins down the hardest float path
   (asymmetric end weights) before any driver exists.
4. **vsfb + vsfc + vsfa + endsf** — the seasonal MA chain (3x3/3x5/3x9/3x15/
   stable → D10 seasonals).
5. **xtrm + replac + weight + sdxtrm + vtest** — extreme-value adjustment,
   required by every B/C pass.

Then `si` + `vtc` (the per-pass drivers), then `x11pt1/2/3/4` + `x11ari` wired
behind `x11{}` and gated end-to-end.

> **This §2.1 "first 3–5 to port" list is historical** — items 1–5 and `vtc` are
> DONE. Live next step is `si`, then Tier 5/6 (see STATUS header).

## 3. Parity risks (X-11-specific)

- **Mode branching (`Muladd` 0/1/2 + `Psuadd` + `Kfulsm`).** `divsub`/`addmul`
  switch on `Muladd`, but `si.f:66` has a **pseudo-additive special case**
  (`Sti = Stsi − Sts + 1`) that bypasses `divsub`, and `Kfulsm==2` (full
  seasonal) copies SI straight through. Four modes × pseudo-additive × full-sum,
  every table. Get the branch table exactly right or one mode class diverges
  wholesale. First gate (`airline_x11-default`) is `mode=mult`; there are
  dedicated `airline_x11-additive` / `airline_x11-logadd` goldens for the others.
- **Henderson end filters (Doherty 1993).** `hndend`/`ends` use
  `rbeta = 4/(Tic²·π)` (`hndtrn.f:52`) and the special **7-term end-span
  reduction** (`hndtrn.f:36`, gated on `Tru7hn`, sets `Tic=0.001`). Asymmetric
  end weights are the classic X-11 divergence point — deeply float-sensitive and
  applied at both series ends of every trend pass. The `.udg` exposes
  `d7trendma`/`finaltrendma` (here `9`) to catch a wrong length early.
- **Trend & seasonal MA auto-selection thresholds.** `vtc` picks the Henderson
  length from the I/C ratio `r` (`r<1`→9-term, `1≤r<3.5`→keep, `r≥3.5`→23-term,
  with `r = Ratic·12/Ny`; quarterly→5/7). `vsfb` picks `Mtype` (3x3…3x15/stable)
  from `Lter`/MSR via `sfmsr`/`getsmat`. A single threshold flip changes the
  filter → cascading table mismatch. `.udg` `seasonalma:` (per-month, here `MSR`
  ×12) and `finaltrendma` are the early tripwires — verify these before diffing
  the D-tables.
- **Extreme-value sigma limits.** `xtrm`/`vtest`/`entsch` select sigma limits
  (`Ksdev`, defaults 1.5/2.5); `replac`/`weight` reweight obs on the sigma
  boundary. Off-by-epsilon at the boundary reweights an observation and shifts
  every downstream table. The B4/B9/C17/D9 tables in the goldens localize this.
- **Forecast/backcast extension bookkeeping.** X-11 filters the regARIMA
  forecast/backcast-padded series (`extend`/`forcst`); the forecast *values* come
  from the ported engine, but the drop/keep pointer math (`setxpt` with `Fctdrp`,
  `Nfcst`, `Nbcst`, `Pos1bk`/`Posffc`) must line up or every filter window shifts
  by an index. Same padded-buffer discipline as regvar (m3_scouting §7).
- **1-based `farray` over span pointers.** Keep Fortran indices for the
  `ctx.*` table arrays (`Sts/Stsi/Stc/Stci/Sti`); the routines index `Stc(i+1)`,
  `apply(X,K,W,N)` reaches `X(K±i)`, etc. Raw C scratch (`Temp`, `simon`,
  `savg`, the `w9`/`w15` weight tables) is 0-based.

## 4. First corpus gate target

`tests/corpus/generated/airline_x11-default.spc` — airline series, `x11{ mode
= mult }`, all defaults, and it **saves the full B/C/D ladder**
(`b1 c1 d1 … b10 c10 d10 b11 c11 d11 d12 b13 c13 d13 …`). Goldens live in
`tests/golden/generated/airline_x11-default/` as one file per table
(`.b1`, `.d10`, `.d11`, `.d12`, `.d13`, …) at 15-digit precision:

```
tests/golden/generated/airline_x11-default/airline_x11-default.d11
  date    airline_x11-defa.d11
  194901  +0.124546106577719E+03
  ...
```

**Gate incrementally up the ladder** (the per-table saves pinpoint the exact
pass where divergence starts): B1 (already producible from the ported prior-adj
path) → D1 → D2/D4 (trend) → D5/D6 (seasonal) → D7 → D8/D9 → **D10** (final
seasonal) → **D11** (final SA) → **D12** (final trend) → **D13** (final
irregular). Primary parity fields: the `.d10 .d11 .d12 .d13` save files, plus
`.udg` summary keys `samode`, `seasonalma`, `d7trendma`, `finaltrendma`, and
`d11.f`/`f2.*` quality stats.

`census-examples/01-basic-x11.spc` is the canonical "getting started" spec but
saves **only** the `.udg` summary (no table saves) — use it as the headline
smoke test, but drive real parity off `airline_x11-default`'s table goldens.
Additive/logadd coverage: `airline_x11-additive`, `airline_x11-logadd`.

## 5. Build/run reminder

- New `.cpp` under `core/src/x11/` is auto-globbed into the build — no manifest
  edit needed.
- Build & test ONLY via `powershell -File tools/build.ps1` (handles the
  rtools44 `ld` DLL-PATH requirement — otherwise `ld returned 9`).
- Run test/CLI exes from **PowerShell**, not `./x.exe` under Git Bash (msys
  "Exec format error").
- ALL printing / `WRITE` / `table` / `punch` / `prttrn` / `x11plt` / save-table
  output is **DEFERRED** — port the numeric core, compute the D-tables into the
  `ctx` arrays, and diff against the goldens via the harness; wire real
  save-table emission later (same policy as fcstout/amdprt).

## 6. Scale estimate

- **Core direct-adjustment compute path: ~55 routines** (the tier table above,
  excluding the deferred diagnostics/print rows) — dominated by the four
  `x11pt*` spines (~3350 lines together) plus ~35 leaves (~2900 lines).
- Diagnostics (F2/F3, M-stats, D9A: ftest/mstest/kwtest/f3cal/sumry/avedur/
  rndsa/shrink/qmap/qmap2, ~1500 lines) are needed for full savelog parity but
  **can land after the first D-table gate**.
- ~93 files touch the X-11 COMMON blocks in total; the remaining ~40 are
  print/plumbing (`prt*`, `table`, `punch`, `x11plt`) or belong to the deferred
  sub-milestones (sliding-spans `ss*`, revisions `rev*`, aggregate/composite
  `agr*`, spectrum `spc*`/`genqs`, x11regression `x11mdl`/`x11aic`/`regx11`).

## x11pt3 port plan (D8->D16 finals) — scouted 2026-07-20

> **Refreshed 2026-08-03.** x11pt3 is ported and gated. Every heading below that
> begins "UNPORTED" describes 2026-07-20: `chktrn`, the diagnostics four
> (`ftest`/`kwtest`/`mstest`/`COMBFT`), `prtd8b`/`prtd9a`, and all four of the
> "gated OFF for base" entries (`ssrit`, `getrev`, `shrink`, `qmap`/`qmap2`) have
> landed -- the last four as `slidingspans{}`, `history{}` and `force{}`, which
> is why they no longer read as flag-guarded stubs. The genuine remainder is the
> print engines, deferred by policy. The per-routine line counts, call sites and
> entry conditions recorded below were the point of the section and still hold.

`x11pt3.f` is **1292 lines** but almost entirely PRINT/SAVE + gated-off feature
branches. The base-path *compute* is a short spine of already-ported leaves plus
the same `chktrn` that x11pt2 is porting. **On the airline_x11-default base path
there is effectively ONE new leaf to write: nothing** — every numeric call is
ported or shared with x11pt2. The bulk of the file is `table`/`punch`/`x11plt`/
`prt*` (deferred) and `Iyrt`/`Issap`/`Irev`/`Adj*`/`Priadj`/`Cnstnt` branches
that are all off for the base case.

Signature: `SUBROUTINE x11pt3(Lgraf,Lttc)`. Reached from `x11ari.f:257`.

### 1. Section-by-section flow (line ranges)

- **Setup (l.67-76):** `ifac`, `nadj2=Nadj if Kfmt>0`, `copy(Sprior->sp2)`,
  `Nfcst=Posffc-Posfob`, `oktrn=oktrf=T`, `chkfct=Nfcst>0 .and. Prttab(LXETRF)`,
  `gudrun=Issap<2 .and. Irev<4 .and. Khol!=1`. **All base path.**
- **D8 unmodified SI + MSR filter (l.78-108):** `sfmsr` (re-select D10 seasonal
  filter under calendarsigma) gated `Kfulsm<2 .and. Ksdev<=1`; `addmul` builds
  unmodified SI `Stsie=Stsi*Stex`; then D8 `table`/`punch` (deferred print).
- **D8 F/M seasonality tests (l.109-134):** `ftest`+`kwtest`+`mstest`+`COMBFT`
  under `IF((.not.Lhiddn).and.Khol.ne.1)` — **this block RUNS on base path** but
  is pure diagnostic (D8 F-stat / moving-seasonality / combined-test savelog).
  Note the `addmul(Stsie,...)` at l.119/131 *rebuilds* Stsie after ftest's
  internal use. `Issap==2` alt branch (l.128) is sliding-spans-only.
- **Ksdev>1 extreme re-replace (l.135-144):** `copy`+`replac`+`sfmsr`, gated
  `Ksdev.gt.1` — **off for base** (default single-sigma Ksdev==1).
- **D8B / D9 replacement (l.145-185):** `prtd8b` (deferred); inline DO
  (l.155-162) builds `Temp` = modified SI where `Stex!=ebar` else `DNOTST`, and
  **sets `stc2(i)=ebar` for all i** (load-bearing later at Part E). D9 `table`/
  `punch`/`prtd9a` deferred.
- **Year-ahead seasonal + modified SA (l.186-377):** `forcst(Sts,0,Posffc,klda,
  Ny,1,PT5,ONE)` extends seasonal one year (gated `Kfulsm<2`). Big
  `IF(Kfulsm.eq.2)` (summary) ... **ELSE (base, Kfulsm==0)** at l.244:
  `divsub(Stci,Stcsi,Sts)` = modified SA; `Muladd==2` antilg block (logadd
  only); `Adjsea/Adjso` regARIMA-seasonal combine (off); `Ishrnk` shrink (off);
  D10 `table`/`punch`/`x11plt` deferred; `ssrit`/`getrev` (off); `Psuadd` D10b
  (off); restore `Muladd=Tmpma`.
- **Final trend via vtc (l.378-467):** `copy(Stci->Ckhs)`; `klda/k2` setup;
  `IF(Kfulsm.eq.0.or.2)` (base) → `vtc(Stc,Stci)` final trend cycle;
  `finaltrendma` WRITE (deferred); `Muladd==2` antilg+`trbias` (logadd only);
  `IF(Muladd.eq.0)` → **`chktrn(Stc,Kpart,12,trnchr,chkfct,oktrn)`**; `Adj*`
  addmul into sp2 (off); `divsub(Stci,Series,Sts)` = **final SA (D11 core)**;
  `copy(Sts->ststd)`. `Kfulsm==1` summary-only ELSE (l.468-523) off for base.
- **Calendar/TD combine (l.524-566):** holiday `Faccal` rebuild (`Khol==2`/
  x11reg/`Adjhol`) and TD `divsub(Stci,Faccal)`+`addmul(ststd,Faccal)` — all
  gated on `Adjtd/Kswv/Axrgtd/Axrghl/Adjhol` (**off for airline-no-TD base**);
  `Priadj>1` prior-lom fold-in (off), uses `dfdate`.
- **rmpadj + final irregular (l.567-603):** `rmpadj` gated `Nuspad>0 .or.
  Priadj>1` (off); **`divsub(Sti,Stci,Stc)` = final irregular (D13 core)**;
  `Fin*`/`Adj*` outlier divsubs (off, no outliers); `Nustad` user-temp-adj (off).
- **D11 write + residual-seasonality (l.604-672):** `IF(dpeq(Cnstnt,DNOTST))`
  (**base, no constant**) → D11 `table`/`punch` (deferred) + `ftest(Stci,...,1,
  ...)` residual-seasonality diagnostic. ELSE = remove-constant branch (off).
- **Store for SS/rev (l.673-695):** `ssrit`/`getrev` (off); D11 forecast portion
  `table`/`punch` (deferred, `Nfcst>0`).
- **Force yearly totals (l.696-876):** `x11plt` (deferred); big `IF(Iyrt.gt.0)`
  → `qmap`/`qmap2`/`rndsa` + `ftest`/`getrev`/`ssrit`. **Entire block off for
  base (Iyrt==0)** → ELSE l.869 `copy(Stci->Stci2)`.
- **rndsa (l.877-919):** gated `Lrndsa` — off for base.
- **D12 final trend write (l.920-1066):** LS/tc/temp-adj fold-in
  `IF(((.not.Finls).and.Adjls==1).or...)` off for base → ELSE l.1011: constant
  removal (off) + D12 `table`/`prttrn`/`punch` (deferred; `oktrn` from chktrn
  picks `table` vs `prttrn`).
- **rev / logadd bias (l.1067-1085):** `getrev` (off); `Tmpma==2` biasfc
  (logadd only).
- **D13 final irregular write (l.1086-1122):** `Adjao/Adjtc` AO/tc-restore branch
  (off) → ELSE l.1110 D13 `table`/`punch` of `Sti` (deferred).
- **D16 combined + D18 (l.1123-1189):** `Khol==1` early RETURN (base continues);
  D16 `table`/`punch` of `ststd` (deferred); `Psuadd` D16b (off); `Adjtd` D18
  (off).
- **Part E modified series (l.1190-1291):** `Kpart=5`; inline DO (l.1199-1258)
  builds `Stome`/`Stime`/`Stcime` (E1/E2/E3 modified orig/irr/SA) from `Stwt`
  extreme weights + `ebar`/`Sprior`/`Facls`; `Adjao/Adjtc` divsubs (off);
  constant removal (off); `Adj*` → restore `Sprior` (off). RETURN.

### 2. Every CALL, tagged

**PORTED (all confirmed by symbol grep in `core/src/x11/` + `specparse`):**
- `copy` → `specparse.hpp:124` (l.71,208,215,273,285,379,448,466,473,516,517,
  522,627,875,928,951,1016,1090,1285)
- `setdp` → `specparse.hpp:84` (l.209,464,529,531)
- `addmul` → `x11filt.hpp:34` (l.94,119,131,216,217,274,275,534,536,549,930,
  932,941,943,1091,1093)
- `divsub` → `x11filt.hpp:29` (l.251,264,456,539,548,574,576,577,578,581,583,
  585,587,721,850,1266,1267,1271,1272)
- `antilg` → `x11filt.hpp:41` (l.258,259,260,265,416,483)
- `forcst` → `x11drv.hpp:41` (l.190)
- `vtc` → `x11drv.hpp:51` (l.406,477)
- `sfmsr` → `x11drv.hpp:62` (l.88,142)
- `replac` → `x11xtrm.hpp:60` (l.140)
- `trbias` → `x11xtrm.hpp:84` (l.420,487) — logadd only
- `vsfc` → `x11seas.hpp:32` (l.222,280) — `Lcentr` only, off for base
- `dpeq` → `numeric.hpp` (inline, l.156,609,754,950,998,1015,1054,1204,1218,1228,
  1275) — helper, available

**UNPORTED — SHARED with x11pt2 (being ported now):**
- `chktrn` (chktrn.f, **139 lines**) — l.429,496. Negative-trend check; sets
  `oktrn`/`trnchr`. **On base path** but a *no-op for positive multiplicative
  trend*: `chktrn.f:27` sets `prtmsg=.not.ispos(Stc,Pos1bk,Posffc)`; if all
  positive it early-RETURNs (l.30) WITHOUT touching `Stc`, leaving caller's
  `oktrn=T`. It only mutates `Stc` (replaces negatives) when trend actually goes
  ≤0 — never for airline. Depends on leaf `ispos` (small; ported alongside).

**UNPORTED — diagnostics (run on base path, but only produce savelog/print
stats; do NOT feed D10/D11/D12/D13 numerics — defer past first gate):**
- `ftest` (ftest.f, **294 lines**) — l.117,620,661,813,897. Seasonality /
  residual-seasonality F-test. Same routine x11pt2 flagged deferrable. Read-only
  on the series (the `addmul` at l.119/131 re-forms Stsie around it, i.e. the
  caller treats Stsie as scratch, not ftest).
- `kwtest` (kwtest.f, **99 lines**) — l.118,130. Kruskal-Wallis nonparametric.
- `mstest` (mstest.f, **158 lines**) — l.123,132. Moving-seasonality F-test.
- `COMBFT` (combft.f, **72 lines**) — l.127,133. Combined identifiable-
  seasonality test (reads the three above's stored stats).

**UNPORTED — print/save engines (DEFER, project-wide policy):**
- `table` (many), `punch` (many), `x11plt` (l.304,697,984,1049,1103,1120),
  `prttrn` (l.961,975,989,1003,1026,1040,1059), `prtd8b` (366), `prtd9a` (112),
  `writln` (l.648...).

**UNPORTED — gated OFF for base (stub with flag guards like x11pt1/pt2):**
- `ssrit` (ssrit.f, 138) — `Issap==2` sliding spans. l.311,678,821,904.
- `getrev` (getrev.f, 149) — `Irev==4` revisions. l.313,685,829,916,1069,1071.
- `shrink` (shrink.f, 79) — `Ishrnk>0` seasonal shrinkage. l.286.
- `qmap` (qmap.f, 116) / `qmap2` (qmap2.f, 328) — `Iyrt>0` force yearly totals.
  l.725,747,771.
- `rndsa` (rndsa.f, 129) — `Lrndsa` rounded SA. l.882.
- `rmpadj` (rmpadj.f, 55) — `Nuspad>0 .or. Priadj>1` prior-adj removal. l.569.
- `dfdate` (17) / `addate` (40) — date arithmetic inside Priadj/rev branches
  (off); trivial when needed.

### 3. Base-path reachability verdict (airline_x11-default)

Base flags: `Muladd==0` (mult), `Psuadd==F`, `Kfulsm==0` (full adj), `Ksdev==1`,
`Priadj<=1`, `Issap<2`, `Irev<4`, `Ixreg!=2`, `Khol!=1`, `Iyrt==0`, `Lrndsa==F`,
`Ishrnk==0`, `Adjsea/Adjso/Adjtd/Adjhol/Adj*==0`, `Cnstnt==DNOTST`, no outliers.

The **entire numeric decomposition** on this path uses only PORTED leaves:
```
sfmsr → addmul(Stsie) → forcst(Sts) → divsub(Stci,Stcsi,Sts)   [modified SA]
      → vtc(Stc,Stci) → chktrn(Stc)  [no-op] → divsub(Stci,Series,Sts) [D11]
      → divsub(Sti,Stci,Stc) [D13]   + inline stc2=ebar, Part-E Stome/Stime loop
```
Final tables land directly in ctx arrays: **D10=`Sts`, D11=`Stci`, D12=`Stc`,
D13=`Sti`, D16=`ststd`** (= copy of `Sts` when no TD). D8=`Stsie`, D9=`Temp`.

**There is NO unported base-path *compute* leaf unique to x11pt3.** The only
unported call that executes and could touch numerics is `chktrn` — and it is a
verified no-op for the positive multiplicative airline trend, and is already on
the x11pt2 port docket. The F/M tests (`ftest`/`kwtest`/`mstest`/`COMBFT`) run
but are diagnostics that don't feed D10–D13; defer them (they gate the `.udg`
f2/f3 and D8-F savelog fields, needed only for full-summary parity later).

### 4. Recommended port order (base path)

1. **Wait on / consume `chktrn`** from the x11pt2 landing (shared blocker; port
   `ispos` with it). If x11pt3 is wired before x11pt2's chktrn merges, stub
   chktrn as `oktrn=ispos(Stc,Pos1ob,last); trnchr=' '` (exact no-op for
   positive trend) — safe for airline.
2. **Port x11pt3 straight through as `x11pt3(ctx, Lgraf, Lttc)`** in
   `core/src/x11/x11parts.cpp` (next to x11pt1/pt2), transcribing section by
   section, wiring the PORTED leaves, and `not_ported`-guarding every gated-off
   feature branch (Kfulsm==2/1, Psuadd, Adjsea/Adjso, Ishrnk, holiday/TD combine,
   rmpadj, Iyrt qmap/qmap2/rndsa, ssrit, getrev, constant, logadd antilg/trbias).
   Drop ALL `table`/`punch`/`x11plt`/`prttrn`/`prtd8b`/`prtd9a`/`writln`.
3. **Stub the diagnostics** `ftest`/`kwtest`/`mstest`/`COMBFT` as no-ops
   (confirm each is read-only on its series arg first — quick check of ftest.f /
   mstest.f return-arg list) and move on; they don't affect the D-table gate.
4. Gate `airline_x11-default` **D8 → D9 → D10 → D11 → D12 → D13 → D16** against
   the goldens. First real blocker if any surfaces: it will be a filter-length
   mismatch inherited from x11pt2 (seasonalma/finaltrendma), not new x11pt3 code.

**First real blocker leaf: none unique to x11pt3.** The shared prerequisite is
`chktrn` (+`ispos`), already owned by x11pt2.

### 5. Goldens in the parity corpus

All present at 15-digit precision, **24 goldens each** across the x11 corpus
(airline/expgs × default/additive/logadd/automdl/fixed-airline/aictest variants):
`.d8 .d9 .d10 .d11 .d12 .d13 .d16` — every x11pt3 output table has a golden.
For `airline_x11-default` specifically:
`tests/golden/generated/airline_x11-default/airline_x11-default.{d8,d9,d10,d11,
d12,d13,d16}`. (`airline_x11-additive` ships d8/d10/d11/d12/d13; `-logadd` too.)
So D8, D10, D11, D12, D13 (and D9, D16) are all directly gateable — no missing
save-table coverage for the finals.
