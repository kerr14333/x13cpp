# X-11 Scouting Report — Moving-Average Seasonal Adjustment (M5, in progress)

> **STATUS (2026-07-20): Tiers 0–3 leaves DONE + unit-tested.** Ported in
> `core/src/x11/{x11filt,x11seas,x11xtrm}.cpp` (28 routines: divsub, addmul,
> logar, antilg, setmv, change, chkzro, divgud, averag, hender, apply, ends,
> endsf, hndend, hndtrn, fis, vsfa, vsfb, vsfc, xtrm, sdxtrm, wtxtrm, replac,
> weight, vtest, entsch, trbias, rho2). Gated by test_x11 / test_x11b.
> **NEXT increment = Tier 4 drivers:** `si` (106, SI-ratio→seasonal; calls the
> ported vsfa/vsfb/xtrm/replac) + `vtc` (90, Henderson-length select; calls
> ported hndtrn) + the MSR seasonal-filter trio `sfmsr`/`getsmat`/`gttrmo`
> (needed by vsfb's Mtype auto-select). Still missing from Tier 0/3: `setdp`,
> `tdxtrm`. Then Tier 5 (`setxpt`/`extend`/`forcst`/`x11int`/`x11ref`) and Tier 6
> (`x11pt1`→B1, `x11pt2`→B1–D7, `x11pt3`→D8–D16, `x11pt4`, `x11ari`) wired behind
> `x11{}`. First end-to-end gate: `airline_x11-default` D10/D11/D12/D13.


Scouted 2026-07-20 against `oracle/fortran` (v1.1 b61). Scope: the classic
X-11 seasonal decomposition (trend / seasonal / irregular) driven by
`x11ari.f` → `x11pt1/2/3/4`. `core/src/x11/` is **empty** — this is the biggest
unstarted milestone. SEATS (`seats*.f`), sliding-spans (`ss*.f`), revisions
(`rev*.f`), aggregate/composite (`agr*.f`), and x11regression (`x11mdl/x11aic`)
are separate, later sub-milestones and are NOT in scope here.

Corpus payoff: **50 / 76 specs use `x11{}`**, and **24 of them ship full B/C/D
save-table goldens** (`.b1 .d1 … .d10 .d11 .d12 .d13` at 15-digit precision).
X-11 sits directly on top of the already-ported regARIMA engine (estimate,
regvar, forecast, outlier, automdl, transform) — it consumes the modeled +
forecast-extended series and produces the seasonally-adjusted output.

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
2. Prior TD/holiday adjustments via `xrgdrv` (x11regression — defer).
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
8. Spectral plots (`spcdrv`, defer), composite/aggregate (`agr*`, defer).

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

Nothing in `core/src/x11/` exists yet. The `divsub` hit in
`run_pre_model.cpp:112` is an **inlined comment** (prior-adjust logic), not a
ported routine; `xtrm` in `common/gen/xtrm_cmn.hpp` is only the generated
COMMON-block header. **Every routine below is NEW.**

Recommended **leaf-first** order (each tier unit-testable before the next):

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
| 1 | vtc | 90 | variable trend cycle: I/C ratio → Henderson length select | new |
| 2 | vsfc | 54 | 2×Nyr MA over the seasonals | new |
| 2 | vsfa | 101 | preliminary seasonal MA pass | new |
| 2 | vsfb | 174 | seasonal MA: 3x3 / 3x5 / 3x9 / 3x15 / stable / 3-term | new |
| 2 | sfmsr / getsmat / gttrmo | 101/102/91 | MSR global seasonal-filter (Lter) selection | new |
| 3 | xtrm | 185 | extreme-value detection/adjustment (sigma limits) | new |
| 3 | sdxtrm / tdxtrm / wtxtrm | 80/126/40 | sigma computation / TD-in-irregular / extreme weights | new |
| 3 | replac | 110 | replace extreme SI values with reweighted MA | new |
| 3 | weight | 123 | extreme-value weight curve | new |
| 3 | vtest / entsch | 64/? | sigma-limit (Ksdev) auto-selection | new |
| 3 | trbias | 39 | trend constant-bias correction | new |
| 4 | si | 106 | SI-ratio → seasonal driver (calls vsfa/vsfb/xtrm/replac) | new |
| 4 | forcst / extend | 41/115 | forecast/backcast extension into the filter window | new |
| 4 | tdlom / traday / makadj | 63/?/37 | TD & length-of-month prior adjustment inside B | new |
| 5 | setxpt | ? | span/forecast pointer setup (Pos1bk/Posffc/Fctdrp) | new |
| 5 | x11int / x11ref | 56/162 | X-11 option init / refresh | new |
| 5 | getx11 / gtx11d | 576/161 | x11{} arg parser + defaults | new (gt_generic stub today) |
| 6 | x11pt1 | 327 | PART B1: prior adj + forecast-extended input | new |
| 6 | x11pt2 | 954 | PARTS B1→D7: main iterated decomposition **spine** | new |
| 6 | x11pt3 | 1291 | PARTS D8→D16: D10/D11/D12/D13 finals + seasonal tests | new |
| 6 | x11pt4 | 778 | PARTS E1→F4: modified tables, F2/F3 stats | new |
| 6 | x11ari | 389 | top partition driver (wire behind `x11{}` in run_m2) | new |
| dx | ftest / mstest / kwtest / f3cal / sumry / avedur / rndsa / shrink / qmap / qmap2 | 294/158/99/142/84/51/129/79/116/328 | F-tests, M-stats, D9A, quality diagnostics (savelog fields) | new — defer past first gate |
| — | table / punch / prttrn / x11plt / svf2f3 / fgen / prtf2 / prtd8b / prtd9a | | print/save engines — **DEFER** (like fcstout/amdprt) | defer |

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
