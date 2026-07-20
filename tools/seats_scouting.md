# SEATS Scouting Report — ARIMA-Model-Based Seasonal Adjustment

Scouted 2026-07-20 against `oracle/fortran` (v1.1 b61). Scope: the `seats{}`
signal-extraction subsystem — the embedded Bank-of-Spain **TRAMO-SEATS** engine
that decomposes the fitted regARIMA model into trend / seasonal / SA / transitory
/ irregular components via Wiener-Kolmogorov (WK) filtering + Burman's algorithm.
`core/src/seats/` is empty; this is the largest unstarted milestone.

Corpus payoff: **13 / 76 specs use `seats`** (vs 50 for `x11`). SEATS is ~226
routines across ~45 000 lines of Fortran — the single biggest subsystem in the
program, and almost none of it is reusable from the regARIMA/automdl port (see §2:
SEATS carries its own root finder `C02AEF`, its own polynomial arithmetic `CONV`,
its own psi-weight/spectral code). See §6 for the SEATS-vs-X11 next-milestone call.

## 1. Entry point & algorithm

```
x11ari.f:168   ELSE IF(Lseats)THEN ... eventually
x11ari.f       CALL seats(blank,blank,outd,blank,0,0,ierr,errext,Lgraf,Lwdprt)
analts.f:43    subroutine SEATS(infil,outfile,outd,graphd,ione,nver,Ierr,...) — 3444-line driver
```

SEATS was originally a standalone program; X-13 keeps it near-verbatim and feeds
it the fitted model through a namelist read by `NMLSTS` (ansub9.f:694) plus the
series. The regARIMA polynomials (phi, theta, d, D, seasonal phi/theta), the MLE
innovation variance, and the forecast horizon arrive **fixed** — in the X-13 path
`init` selects "use the given model, do not re-estimate", so the whole SEATS-side
estimation subtree (§2 tier E) is bypassed. Confirmed against
`generated/airline_seats.out`: SEATS emits only the WK/Burman decomposition; the
"ARIMA Estimates (H-R)" lines in that `.out` come from the *automdl* preamble, not
SEATS.

Hillmer–Tiao (1982) / Burman (1980) flow inside the driver:
1. **Read model + series** (NMLSTS, GETSERIES), set decomposition options
   (gtseat.f params: qmax/xl/rmod/epsphi/noadmiss/hpcycle…).
2. **(bypassed in X-13)** optional re-estimation / auto-ID: STAVAL→TRANS1→
   SEARCH/CALCFX (likelihood optimizer) / AMI (H-R auto model) / CHMODEL.
3. **Root-factor the model**: `RPQ` (→ `C02AEF`) finds AR and MA roots; roots are
   sorted/classified by frequency (0, π, seasonal, cyclical).
4. **Canonical decomposition**: allocate AR roots to components (`F1RST`/`SECOND`
   in sigsub.f), partial-fraction the pseudo-spectrum (`PARFRA`), spectral-factor
   each component's MA (`MAK1` + `SPC`/`SPCEST`), enforce the canonical
   (max-irregular) constraint. Output = component ARMA models
   (trend `tcnum/tcden`, seasonal `snum/sden`, SA `sanum/saden`, transitory
   `trnum/trden`, irregular variance) — exactly the `.mdc` save file (savmdc.f).
5. **Admissibility**: `CHECKADM`/`CHKSPCT` verify each component spectrum ≥ 0;
   if not, `APPROXIMATE` re-fits an admissible nearby model.
6. **Forecast/backcast extension** of the series for the finite WK filter: `FCAST`
   (ansub1.f) + `ExtendSeries` (ansub11.f).
7. **Signal extraction**: `SIGEX` (sigex.f, 4017-line orchestrator) → `ESTBUR`
   (ansub3.f, Burman's algorithm — the WK engine) → `AUTOCOMP` (ansub4.f, builds
   trend/sa/sc/cycle/ir series) → `Afilter`/`FinitoFilter` (ansub11.f, finite WK
   end-filters + phase).
8. **Error variances / revisions**: `VARIANCES`, `SERRORL`/`SERROR`,
   `SEBARTLETTACF`, `PINNOV`; **bias correction** for the log case (`BIASCORR`,
   `ABIASC`); **deterministic-effect allocation** (`DETCOMP`, `TAKEDETTRAMO`).
9. **HP trend/cycle option** (`HPPARAM`/`HPTRCOMP`) when `hpcycle` requested.
10. **Output** components + WK filters + diagnostics (all DEFERRED — §2 tier F).

## 2. Leaf port status (checked against core/src, 2026-07-20)

**Nothing SEATS-specific is ported yet.** Cross-checking each SEATS numeric name
against `core/src` (grep): `estbur`, `sigex`, `parfra`, `mak1`, `hppar`,
`afilter`, `c02aef`, `conv`, `fcast`, `dpsi` → all NONE. The regARIMA/automdl port
has `rpoly` (Jenkins–Traub) and `uconv`, but **SEATS does not use them** — it has
its own `C02AEF` root finder (ansub2.f:2900) and its own `CONV` polynomial multiply
(ansub2.f:341). Reusing rpoly/uconv here would break bit-exactness (see §3). The
only genuine reuse is the *infrastructure*: `farray`, `dpeq`, the ctx/series
plumbing, and the fitted-model values themselves (phi/theta/var) already sitting
in the regARIMA context.

Leaf-first tier order (portable pure-numeric leaves first, driver last). "lines"
counts the def-to-next-def span; the giant SIGEX/SECOND/DETCOMP spans are mostly
inline print — real numeric core is a fraction.

| Tier | routine | file:line | lines | role | ported? |
|------|---------|-----------|-------|------|---------|
| **A pure-numeric leaves (unit-testable, portable-first)** ||||||
| A | CONV / CONJ / MULTFN / DIVFCN / CONVM / CONJM | ansub2.f:341… | 60ea | polynomial ×, conj-×, ÷ — SEATS's own (NOT uconv) | new |
| A | C02AEF / C02AEZ | ansub2.f:2900 | 339+62 | NAG-style complex polynomial root finder (RPQ's engine) | new |
| A | Tartaglia / ROOTC / SQROOTC / CubicRoot / MulCompl / DivCompl | ansub2.f | 20-70ea | closed-form quad/cubic + complex arithmetic | new |
| A | SYMPOLY / MLTSOL | ansub2.f:2309,2565 | 82+114 | symmetric-poly build, linear solve | new |
| A | DPSI / CHBJB / BFAC / MPB/MPBF/MPBBJ / INPOL | ansub3.f | 42-125ea | psi-weights, autocovariance (BFAC), poly helpers | new |
| A | SPC / SPCEST / getSpectrum / getAR / truncaSpectra | ansub5.f:125… | 55-66ea | pseudo-spectrum evaluation | new |
| A | Parzen / KENDALLS / getVar / FFT / FFTr / sFourier | ansub11.f | 20-120ea | window, FFT, variance (spectrum diagnostics) | new |
| A | DVAR / DVARMS / DMED / DMEAN / DMU / RAIZ / DIVIDECHECK | ansub1/2/3/4.f | 15-60ea | scalar stat/util leaves | new |
| **B numeric mid-tier (need A)** ||||||
| B | RPQ | ansub2.f:16 | 169 | root-find AR/MA poly → rez,imz,modul,ar,pr (calls C02AEF) | new |
| B | PARFRA | ansub2.f:1495 | 72 | partial-fraction pseudo-spectrum → component numerators | new |
| B | MAK1 | ansub2.f:1615 | 281 | spectral factorization: autocovariances → canonical MA | new |
| B | F1RST | sigsub.f:29 | 228 | allocate AR roots (non-seas/seas/cyclical) to components | new |
| B | getPSIE / SeparaBF / DECFB | ansub3.f | 68-89 | WK psi-weights, backward/forward filter split | new |
| B | CHECKADM / CHKSPCT | ansub7.f:139,292 | 153+286 | spectrum-admissibility (non-negativity) tests | new |
| B | HPPARAM / HPTRCOMP / CONVC | ansub10.f | 124-155 | HP trend/cycle filter (hpcycle option) | new |
| B | Afilter / FinitoFilter / GetPhase / smoothH | ansub11.f | 98-359 | finite WK end-filter weights + phase delay | new |
| B | GETTHVARIANCE / PINNOV | ansub4/5.f | 50+166 | component innovation variance; pure-MA innovations | new |
| **C stateful numeric drivers (verify through corpus, not microtests)** ||||||
| C | SECOND | sigsub.f:331 | ~600* | canonical decomposition driver (F1RST + PARFRA + MAK1) | new |
| C | APPROXIMATE / KnownApprox | ansub5.f:2011 | 251 | admissible-model re-fit when decomposition fails | new |
| C | FCAST | ansub1.f:2066 | 269 | ARIMA forecast/backcast to extend series for WK filter | new |
| C | ExtendSeries / extendHP | ansub11.f:806 | 124 | apply the extension | new |
| C | ESTBUR | ansub3.f:54 | 660* | **Burman's algorithm** — the WK signal-extraction engine | new |
| C | AUTOCOMP | ansub4.f:33 | ~350* | assemble trend/sa/sc/cycle/ir component series | new |
| C | SIGEX | sigex.f:51 | ~600* | signal-extraction orchestrator (roots→decomp→Burman→comp) | new |
| C | VARIANCES / SERRORL / SERROR / SERRORF | ansub4/5.f | 75-495 | component error variances (WK squared-gain integral) | new |
| C | SEBARTLETTACF / SEBARTLETTCC | ansub4.f:5092 | 217+207 | ACF standard errors of components | new |
| C | BIASCORR / ABIASC | ansub4.f:1244 | 340+90 | log→level bias correction of forecasts/components | new |
| C | DETCOMP / TAKEDETTRAMO / DSOUT | ansub4/9.f | 162-800* | allocate deterministic (TD/Easter/outlier) effects to comps | new |
| C | Seasign / SMRFACT | sigsub.f | 78+278 | seasonal-significance / seasonal-MA-factor helpers | new |
| **D interface (regARIMA↔SEATS)** ||||||
| D | NMLSTS | ansub9.f:694 | 647 | read fitted-model namelist into SEATS locals | partial† |
| D | GTSEAT | gtseat.f:1 | 350 | parse `seats{}` options (qmax/xl/rmod/noadmiss/hpcycle…) | new |
| D | ss2rv / rv2ss / savmdc / initst | *.f | 64-215 | X-13↔SEATS state bridge; `.mdc` component-model writer | new |
| **E estimation subtree — BYPASSED in X-13 fixed-model path → DEFER** ||||||
| E | SEARCH / CALCFX / MINIM / MINIMbis / FEASI | ansub1/2.f | 278-919 | likelihood optimizer (model already fitted upstream) | defer |
| E | AUTO / AMI / CHMODEL / STAVAL / CHECK / TRANS1 / FITMODEL | ansub1/5.f | 119-919 | SEATS-internal auto-ID / starting values | defer |
| **F print / IO / diagnostics → DEFER (like fcstout)** ||||||
| F | SEATS print half, seatpr/seatdg/seatfc/seatad, OUTTABLE*, PROUT1, NMOut, USRENTRY, PROFILER, all OPEN/CLOSE/DEVICE, htmlout, TABLE1/2, Tpeaks/Spectrum plots | many | ~33 routines | tables, WK-filter dump, spectra, HTML, logging | defer |

\* SIGEX/SECOND/DETCOMP/ESTBUR/AUTOCOMP spans include large inline WRITE blocks;
the load-bearing numeric core is far smaller once print is stripped.
† `NMLSTS` is a namelist reader; the port supplies the fitted model directly from
ctx rather than re-parsing a file — most of its 647 lines become plumbing, not logic.

## 3. Parity risks (SEATS-specific)

1. **Root finder is `C02AEF`, not the ported `rpoly`.** SEATS root-finds every
   AR/MA/component polynomial with its own NAG-derived `C02AEF` (ansub2.f:2900),
   whose deflation order, starting point, and refinement differ from Jenkins–Traub
   `rpoly`. Roots feed the canonical decomposition (root→component allocation in
   `F1RST`), so a last-ULP root difference re-classifies a root and changes the
   whole decomposition. **Port `C02AEF` faithfully; do NOT substitute rpoly.**
2. **Spectral factorization `MAK1` is iterative and tolerance-gated.** The MA
   spectral factorization (autocovariances → canonical MA polynomial) converges
   on a threshold; iteration count and the "flip roots inside the unit circle"
   step are exact-equality-sensitive. The canonical-decomposition variances
   (`svar`, `savar`, `irrvar` in the `.mdc`) come straight out of it.
3. **Admissibility / approximation branch (`noadmiss`, `CHECKADM`→`APPROXIMATE`).**
   Whether a decomposition is accepted or the model is nudged to an admissible one
   hinges on a spectrum-minimum sign test (float compare near 0). `rmod`/`epsphi`
   thresholds from `gtseat` (root-modulus cutoff) decide which roots are "seasonal"
   vs "trend" — a wrong threshold silently moves a root to a different component.
   Wire the `gtseat` defaults exactly.
4. **CONV accumulation order.** SEATS's own `CONV`/`CONVM` polynomial multiply
   must match the oracle's summation order bit-for-bit; the ported `uconv` may
   accumulate differently. Use SEATS's own routines inside SEATS.
5. **Forecast-extension length & the finite WK end-filter.** `FCAST` extends the
   series by the ARIMA forecast before the (doubly-infinite, truncated) WK filter
   is applied; `Afilter`/`FinitoFilter` build the finite end-filters. The number
   of forecasts/backcasts and the truncation length must match, or the first/last
   component values (and the `.wkf` filter) diverge.
6. **Bias correction in the log case.** `BIASCORR`/`ABIASC` convert log-component
   forecasts to levels with a variance-based correction; airline (log) is the
   first gate, so this is on the critical path (`variance$mle` scaling).

## 4. First corpus gate target

**`tests/corpus/generated/airline_seats.spc`** (log airline, automdl→(0 1 1)(0 1 1),
`seats{}`). Simpler still is **`census-examples/04-seats.spc`** (same model given
explicitly via `arima{model=(0 1 1)(0 1 1)}`, so it skips the automdl preamble —
the cleanest isolation of the SEATS decomposition). Gate incrementally:

- **Step 1 — decomposition (component models).** Diff the ported `.mdc` against
  `tests/golden/generated/airline_seats/airline_seats.mdc`: `nsnum/snum` (seasonal
  MA, 12 coeffs), `nsden/sden` (seasonal summation U(B)=1+B+…+B¹¹), `svar`;
  `nsanum/sanum` + `nsaden/saden` (SA = trend part; `saden` = (1,−2,1)), `savar`;
  `irrvar`. Also the `.udg` keys `seats$nmodel`, `seats$nonseasonaldiff`,
  `seats$seasonaldiff`, `seats$MA$Nonseasonal$01$01`, `seats$MA$Seasonal$12$12`,
  `variance$mle`/`variance$se`. This validates roots→canonical decomposition
  (tiers A/B + SECOND) **before** any signal extraction.
- **Step 2 — component series.** Diff the save tables against the goldens in
  `tests/golden/generated/airline_seats/`: **`.s10`** (seasonal), **`.s11`** (SA
  series), **`.s12`** (trend), **`.s13`** (irregular), plus `.s16` (transitory),
  `.s18`/`.sfd`/`.tfd`/`.afd` (forecasts), `.wkf` (WK filter), `.se2`/`.se3`/`.tse`/
  `.ase` (component SEs). Table format: `date<TAB>+0.NNN...E+NN`, e.g. `.s12`
  starts `194901  +0.123637024258309E+03`.
- `.udg` component-variance keys (`vartrend`, `varseasadj`, `varseasonal`,
  `varirreg`, `tsetrend`…) close out the error-variance tier (C).

Goldens live under `tests/golden/{census-examples/04-seats, generated/airline_seats}/`.
Wire SEATS into `run_m2`/driver behind `seats{}`, gate Step 1 first, then Step 2.

## 5. Build/run reminder

Build & test ONLY via `powershell -File tools/build.ps1` (handles the rtools44
`ld` DLL-PATH requirement — otherwise `ld returned 9`). Run test exes from
PowerShell, not `./x.exe` in Git Bash (msys "Exec format error"). New `.cpp`
under `core/src/seats/` auto-globs into the build. Conventions: Fortran arrays are
1-based → keep 1-based `farray operator()` for ctx.* arrays (raw C scratch stays
0-based); ALL printing / WRITE / save output DEFERRED (tier F) — a first gate
compares the ported in-memory component vectors against the parsed golden tables,
it does not reproduce the SEATS `.out`.

## 6. SEATS vs X-11 — which next milestone?

**Recommend X-11 next; SEATS after.** Rationale:

- **Corpus payoff:** X-11 = 50/76 specs, SEATS = 13/76. X-11 is the default/legacy
  method most of the corpus (and most users) exercise.
- **Size / tractability:** X-11 family ≈ 6.7 k lines (`x11*.f`, `getx11`, `regx11`,
  `ssx11a`, `gtx11d`); SEATS ≈ 45 k lines / 226 routines. X-11 is moving-average
  arithmetic (Henderson filters, seasonal MAs, extreme-value replacement) — no
  complex root-finding, no spectral factorization, no admissibility search.
- **Parity risk:** X-11's exactness risks are ordinary FP-order issues; SEATS
  layers on a bespoke root finder (`C02AEF`), iterative spectral factorization
  (`MAK1`), and float-sign admissibility branches (§3) — each a fresh bit-exact
  hazard with no upstream analogue to lean on.
- Both dirs (`core/src/x11`, `core/src/seats`) are empty, so neither has sunk cost.

SEATS remains essential (it is the modern model-based method), but it is the
harder, lower-coverage climb — better tackled once X-11 has proven the
post-regARIMA seasonal-adjustment plumbing (component tables, save-table gates,
`x11ari` dispatch) that both methods share.

### First 3–5 routines to port (leaf-first)

1. **`CONV`/`CONJ`/`MULTFN`/`DIVFCN`** (ansub2.f:341+) — SEATS's own polynomial
   arithmetic; every later tier depends on it; trivially unit-testable.
2. **`C02AEF`(+`C02AEZ`)** (ansub2.f:2900) — the complex-polynomial root finder;
   unit-test against the `.udg` `roots.ma.*` values; the #1 parity hinge.
3. **`RPQ`** (ansub2.f:16) — thin driver over C02AEF producing rez/imz/modul/ar/pr;
   the exact root-classification arrays the decomposition consumes.
4. **`BFAC`/`DPSI`** (ansub3.f) — autocovariance & psi-weight generation feeding
   `MAK1` and the WK filters; pure and unit-testable.
5. **`PARFRA` + `MAK1`** (ansub2.f:1495, 1615) — partial-fraction the pseudo-spectrum
   then spectral-factor each component; together they emit the canonical component
   models (the `.mdc` output = the Step-1 gate). `F1RST`/`SECOND` orchestrate them.

Port order rationale: 1–4 are pure leaves gated by unit tests against `.udg`
roots; 5 closes the decomposition, which is the first end-to-end corpus gate
(`.mdc`) — reachable *before* any Burman signal extraction (`ESTBUR`/`SIGEX`),
letting the hardest numeric core (roots → canonical decomposition) be proven in
isolation.
