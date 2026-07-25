# X13cpp — Claude working notes

A **faithful C++17 port of the U.S. Census Bureau's X-13ARIMA-SEATS** (v1.1 b61),
packaged as embeddable **R** (Rcpp) and **Python** (pybind11) libraries over one
shared `core/`. It is a proof-of-concept under active development.

## Project vision & priority order

The endgame, in sequence:
1. **Convert Census X-13 Fortran → C++** (the `core/` engine). **This is the first
   and foremost priority — everything else needs it first.** Bit-exact with the
   oracle (see next section). This is where day-to-day work lives.
2. **R + Python wrappers as IMPORTED LIBRARIES** — `library(x13)` / `import x13`,
   called in-process to run X-13 and get **result objects** back. NOT a thin shell
   around an executable; the point is working with X-13 directly in the interpreter.
3. **Nice interfaces + all kinds of plots** built on those result objects.
4. **Far future:** tooling to review a large number of series at once.

Once the conversion is complete, the fork is either (a) refactor the C++ to be more
modern, or (b) start the R/Python interfaces — decide then.

Keep #1 the focus. #2-#4 are the reason for #1, but the engine must exist and match
Census before they mean anything. Don't rabbit-hole a single parity xfail at the
expense of overall engine progress — but the engine port is the job.

## The one rule: bit parity

The contract is **bit-exact agreement (1e-8) with the vendored Fortran oracle**
`oracle/fortran/x13as_ascii_O2.exe`. Port faithfully — **do not "improve" the
Fortran.**
- **Ported bugs stay bugs.** Reproduce Census defects verbatim, comment them, and
  log each as a `CB-N` entry in `tools/census_bugs.md` pinned to a test.
- **No auto file output** in the final package: results live on the result object;
  writing `.udg`/`.fct`/save files is explicit and caller-driven.
- Packaging must pass **CRAN + PyPI** checks — design wrappers compliant from the
  start.

## Use the project skills (`.claude/skills/`)

These carry the conventions that otherwise waste turns — read/use them, don't
improvise:
- **`build-run`** — how to build & run tests on this machine.
- **`parity-gate`** — add a corpus spec, bless its oracle golden, gate bit-exact.
- **`port-leaf`** — transcribe one Fortran subroutine with the farray / indexing /
  `DATA` / ported-bug conventions.

## Build & test (Windows) — essentials

- **Build via the PowerShell tool:** `& tools/build.ps1` (configures, builds, runs
  the CTest unit suite). Do **not** run it as `powershell -File …` (execution
  policy blocks `-File`).
- **Toolchain: rtools44** (`C:\rtools44\x86_64-w64-mingw32.static.posix\bin`).
  `build.ps1` prepends that bin to PATH — required, or the collect2→ld LTO-plugin
  link aborts with **"ld returned 9"** (rtools40 shadowing rtools44 on PATH is the
  classic cause). Drive compilers from PowerShell, not msys/Bash (msys make/xargs
  scrub `TEMP` and break gfortran).
- **Python is `python`** (3.14). `python3` is a Windows App alias → "Permission
  denied".
- **Parity tests:** `python -m pytest tests/parity -q`. Green = `NNN passed`, with
  expected `s` skips (parse-gap / no-golden specs). Currently 1033 pass / 0 fail /
  0 xfail / 25 skip.
- After adding a `core/src/*.cpp`, the first build prints `GLOB mismatch!` and
  stops — just rerun once.

## Layout

- `core/` — the engine. `src/` by subsystem: `regarima/ automdl/ x11/ seats/
  outlier/ transform/ specparse/ diag/ force/ numeric/ tables/ driver/ common/`.
  `driver/run_*.cpp` are the phase harnesses (`x13run_m2/m3/x11/iddiff`).
- `oracle/fortran/` — the Census Fortran **oracle** (the source of truth) + its
  prebuilt `_O2`/`_O0` binaries.
- `tests/` — `corpus/` (specs), `golden/` (blessed oracle output), `parity/`
  (pytest M-gates), `unit/` (ctest).
- `tools/` — `worklog.py` (dev timeline), `*_scouting.md` (port plans),
  `census_bugs.md`, `FABLE_REVIEW.md`, `TEST_COVERAGE.md`, build/coverage scripts.
- `r-pkg/`, `py-pkg/` — language wrappers.

## Progress & commits

- **Track dev time** via `tools/worklog.py`; this is a PoC, so lead progress
  reports with elapsed active time. `WORKLOG.md` is a periodic snapshot; the git
  timeline is authoritative.
- Commit style: `M<n>/<subsystem>: <what>`, list the gated specs, end with the
  `Co-Authored-By:` + `Claude-Session:` trailers.

## Current phase — M5 (X-11 / SEATS)

M0–M4 done (parse, regARIMA estimate/forecast, automatic model ID). M5 in
progress: X-11 decomposition spine (B1→D16) is bit-exact on the no-model and
regressor-free automdl paths. **General-shape SEATS (p>0) is now bit-exact +
gated** — the `*_ar2-seats` specs `(2 1 0)(0 1 1)` gate s10–s18 at ~5e-15 on
airline/payems/unrate (the only port bug was the AR-polynomial sign in
canonical_denoms.cpp: `phis = +mo.phi`; plus the CALCFX Pstar>0 stationary-AR
filter for payems_ar2's near-non-invertible seasonal MA). **Seasonal-AR (bp>0)
is also bit-exact + gated** — the `*_sar-seats` specs `(0 1 1)(1 1 0)` gate
s10–s18 on all 4 series (same sign-fix class: `bphis = +mo.bphi`). **imean!=0
(a Constant/mean regressor) is now bit-exact + gated, and is d-AGNOSTIC** — both
`*_mean-seats` `(0 1 1)(0 1 1)+const` (d>=1 drift) and `*_mean-d0-seats`
`(2 0 0)(0 1 1)+const` (d==0) gate s10–s18 (~5e-15) on all 4 series. The decisive
fixes (the original plan was incomplete): (1) SEATS decomposes the series WITH
the mean present — run_seats restores `ctx.series.tsrs` from the clean `trnsrs`
(estimation had left the regression-ADJUSTED series, drift removed); (2) CALCFX
seeds center the differenced series by wm (forward wm, backward kd·wm,
kd=(-1)^(d+bd)); plus za=wm·(1-Σphist) in FCAST and wmf/wmb in the ESTBUR
boundary. (The "d==0 trend unit root drops" theory was wrong — non-mean d==0 is
itself bit-exact, so the mean fixes cover d==0 unchanged.) **imean ALONGSIDE
other regressors (TD) is now bit-exact + gated too** — `*_mean-td-seats`
`(0 1 1)(0 1 1)+const+td` gate s10–s18 on all 4 series. Two pieces: (1) the
decomposition input keeps the mean but removes TD — add back only the Constant's
contribution `b_const·Xconst` (the regvar case-10 ones/Diff(B) drift ramp, via
ratpos) onto the fully-adjusted `ctx.series.tsrs`; (2) s10 = seasonal-only
(linearized/sa) but s16/s18 = COMBINED = **raw a1/sa**, refolding BOTH the removed
TD effect AND the automatic lom/leap prior (`td`+log → `prioradj: lpyear`). The
`trn` base is prior-adjusted, so the combined factor needs the RAW a1 —
run_pre_model stashes it into `ctx.seats_combined_orig`, estbur forms
`combined_factor`/`combined_add` (empty ⇒ s10==s16==s18, no-reg corpus untouched).
Trap avoided: `z = trn − log(td_SAVE)` mismatches every February; use the design
βX / raw a1, never the save table. **SEATS seasonal
decomposition corpus is
fully bit-exact** — every SEATS corpus spec's s10–s18 tables gate (~5e-15),
including the near-non-invertible fixed-airline variants (payems_fixed /
expgs_fixed, closed via a faithful CALCFX forecast-residual port in
`core/src/seats/estbur.cpp`). **slidingspans{} is also now bit-exact** (all
4 spans, sfs+chs; the per-span `xtrm.ksdev` reset fix in `run_x11_span`).
**history{} revisions are now bit-exact too** (sar/sae/trr/tre, at the per-span
re-estimation floor ~7e-6; `core/src/driver/run_history.cpp`, reusing the
re-entrant span driver + a per-span `Lterm`/`Nterm` reset). The whole X-11
diagnostics front (force / slidingspans / history) is now closed.
- **`history{estimates=(fcst)}` — the out-of-sample FORECAST-ERROR history —
  CLOSED (at the per-span floor).** It was **accepted and silently dropped**:
  `gt_history` parses all nine `estimates=` tokens and sets `Lrvfct`, but
  `run_history`'s "anything to do" gate listed only the five sadj/trend/seasonal
  flags, so a spec asking for forecast history got `OUTCOME: OK` and no fce/fch
  at all. Ported: `revchk.f:424-467`'s lag-list defaults and validation
  (`Nfctlg==0` ⇒ leads `(1, Ny)`, or `(1, Nfcst)` when `Nfcst<Ny`; a lead past
  `maxlead` disables the whole analysis) plus `revchk.f:1025-1045`'s drop of any
  lead longer than the revision span; `prtfct.f:613-643`'s per-span store
  `Cncfct(k, Revptr+Rfctlg(k))` — the forecast is filed under the row of the DATE
  IT PREDICTS, not the span that made it; and `prfcrv.f`'s arithmetic (running
  sum of squares `fce`, the `(forecast, error)` pairs `fch`, and the `meanssfe`
  savelog canaries `fctss(k)/(Revptr-Rfctlg(k))`). `transformfcst=yes` (`Rvtrfc`,
  errors differenced on the transformed scale) is ported too. Two things worth
  knowing: (1) `ctx.forecasts.fcst` IS prtfct's `untfct` — prtfct only rebuilds
  it (with the `eltfcn` Facxhl/X11hol/Stptd folds) on the branch where the fct
  table is neither printed nor saved, and that branch reconstructs the same
  value the LFOROS path already has. (2) **This family AMPLIFIES the per-span
  re-estimation floor three times over**, which is why its tolerances are the
  loosest in the port and each step is measured: the forecast level runs 1.8e-5
  relative (a forecast extrapolates the coefficient difference, so above the
  1e-5 in-sample floor sae/tre gate at); `actual - forecast` is an O(270)
  cancellation leaving O(10), turning that into ~2e-4; `fce` then squares and
  accumulates it to ~3e-4. The first spans are bit-exact. Gated by
  `test_history_tables.py` (fce/fch + the meanssfe/rvfcstlag canaries), corpus
  `extra/airline_history-fcst` (explicit `fstep=(1 12)`) and
  `extra/airline_history-fcst-trans` (default lag list + `transformfcst=yes`).
- **The three MODEL histories `estimates=(aic arma td)` — CLOSED, and
  `history{}`'s whole `estimates=` surface is now covered.** Same silent-drop
  class as fcst above. All three are captured at `revdrv.f:670-690`, straight
  after each span's rgarma: `lkh` = the span's `(Olkhd, Aicc)`, `amh` = its FREE
  ARMA coefficients in Mdl/Opr order (`rvarma.f`), `tdh` = its FREE trading-day /
  length-of-period / user-TD coefficients with each TD group followed by its
  implied contrast column `-sum(b)` (`rvtdrg.f`). They share a row range
  `i=Begrev..Endrev` that is one row LONGER than the revision tables (which stop
  at `Endtbl-1`), hence a separate `mdates`. **The one real engine gap this
  found:** `run_x11_span` never called `prlkhd`, so `ctx.lkhd` still held the
  MAIN run's values and every `lkh` row printed the same number (aicc 976.5274 on
  all 71 rows, exactly the main run's `.udg`). `arima.f:742` calls prlkhd on every
  estimation pass; it is now called per span, with `run_x11.cpp` save/restoring
  `/lkhd/` around both span drivers — the oracle has the same overwrite but
  writes its `.udg` before revdrv runs, whereas this harness dumps at exit (the
  x11_f2tests / xtrm.ksdev snapshot class again). **What the tolerances say:** the
  first span is bit-exact and then `lkh` agrees to 2.6e-9 relative while
  `amh`/`tdh` only agree to ~8e-4 — the flat optimum, not an inconsistency. Near
  the maximum the surface is nearly level, so a 1e-3 parameter difference buys a
  1e-9 likelihood difference; the spread is smooth (median 4e-5, worst 2.5e-4 on
  the nonseasonal MA, no outlier span), i.e. optimizer path noise over 71
  independent re-convergences. **CB-20**: the `.tdh` save rows write their values
  into `outARMA` but their tab separators into `outTDrg`, so the emitted row's
  separator is whatever `outARMA` held — an inherited TAB when the `amh` block
  ran first, a literal NUL byte when it did not — and the file's columns do not
  line up with its own tab-separated header. Not reproduced (it is a save-FILE
  character-buffer defect and this port writes no save files); the values and
  their order are. Gated by `test_history_tables.py`, corpus
  `extra/airline_history-model`.
- **Model X-11 path — CLOSED.** The `*-aictest-x11` (airline/expgs/payems) and all
  four `*-fixed-airline-x11` specs gate bit-exact on X-11 **and** on the fct forecast:
  automd aictest selection + finalization, x11pt2's model factor combine, the
  x11pt2/x11pt3 outlier folds (AO→D13, LS→D12 trend), the fixed-model leap-year
  Sprior population, the post-idotlr regvar rebuild, and the fcstout LOM-prior
  re-application all landed. See **`tools/x11_regeff_handoff.md`**.
- **X-11 spec-option front — CLOSED (bit-exact):** `type`=summary/trend, `shrink`=
  global/local, `sigmavec`, classic `x11easter` (transparent pre-pass →
  holday/holidy/easter → Khol prior fold; codex-hardened per xrgdrv/editor.f), and
  the user-regression prior factor (Facusr) all gate. The x11 parse-seam is
  exhausted. Of the three remaining x11 stubs, user PRIOR factors (Nuspad/
  Nustad) and force non-original targets (Iftrgt>0) are now closed — see the two
  entries below; the only one still open is revisions getrev.
  (Adjsea/Adjso regARIMA-seasonal combine landed, commit 970e85c.)
- **`force{}` non-original targets + the forecast-span prior — CLOSED
  (bit-exact):** all four `target=` values gate (x11pt3.f:715-722 — `original`
  Series, `calendaradj` Stocal, `permprioradj` Stopp, `both` Stopp/Faccal), on
  specs carrying a regARIMA TD. The target port was three lines; what had walled
  it was a real numeric bug that only force can see. force's `qmap` sums the
  target-vs-SA discrepancy over the FORECAST year too, so it is the only gate
  that reads D11 past `Posfob` — and there D11 was **infinite**, behind
  `OUTCOME: OK`. Two causes, both in `run_pre_model.cpp`: (1) the prior-factor
  series was built to `Nobspf` (the estimation length) instead of adjsrs.f's
  `Nadj = Nspobs + Nbcst + max(Sp, Nfcst-Fctdrp)`, so `Sprior` was 0 across the
  forecast span and x11pt2's tdlom `Factd *= Sprior` zeroed the model TD factor
  (hence `Faccal==0`, hence `D11 = Series/0`); (2) `Kfmt` was never set to 1 on
  the model path (adjsrs.f:62,101), so adjreg.f:98 never folded the prior back
  into the forecast tail of `Series`, leaving the prior-ADJUSTED forecast there.
  d10-d13 print over the observed span only, which is why every existing gate
  stayed green through both. Gated by `test_force_tables.py` (now also gating
  d10-d13), corpus `extra/airline_force-{td,calendaradj,permprioradj,both}`.
- **`forecast{maxback=}` BACKCASTS — CLOSED (bit-exact):** backcasts **segfaulted**
  the engine on every spec that used them. Three defects stacked: (1) `Nbcst2` was
  pinned to 0, so setxpt put `Pos1bk = -Nbcst+1` — a negative buffer index (the
  crash); editor.f:206-219 sets `Nbcst2 = Nbcst` (padded to a January start when
  `Begbak` is mid-year). (2) `mkback.f` was never ported — `extend` was handed a
  pointer to a single stack `double` as the backcast vector. Ported as `bcstout`
  in `regarima/forecast.cpp`: same MMSE machinery as fcstxy, run on the TIME-
  REVERSED design (rebuild Xy over Nspobs rows, reverse the first `Nrxy-Nfcst`,
  forecast, restore). (3) `/adjcmn/ Adj` was anchored at `Begspn` instead of
  `Begadj = Begspn - Nbcst`, so the leap-year prior landed a **year late** in
  D11/D13/D16 (~3.6e-2, Februaries only). Adj is now Begadj-anchored with
  `Adj1st = Nbcst+1`; with no backcasts `Adj1st==1` and every existing path is
  byte-identical. Gated by the new `*_backcast-x11` config (all 4 series).
  Still open here: `x11{appendbcst=yes}` widens the oracle's punch range to the
  backcast span and the harness does not (b1/d10/d16 row counts differ).
- **regARIMA SEASONAL-OUTLIER (`Adjso`) through X-11 + `x11{centerseasonal=}`
  (`Lcentr`) — CLOSED (bit-exact).** Two things, closed together because neither
  is gateable without the other. (1) The `x11pt2 user/seasonal/cycle/x11reg
  factor combine+emit` wall fatalled on `Adjso==1`/`Adjsea==1` UNCONDITIONALLY,
  but on the base path those are print-only in x11pt2 — `Facso` reaches only the
  deferred A8 accumulator (x11pt2.f:204-207) and `Facsea` only the deferred A10
  table (:273), exactly like the Adjao/Adjls/Adjtc/Adjusr the comment there
  already excused. Their real arithmetic is the x11pt3 combine, ported in
  970e85c. The one place they ARE numeric in x11pt2 is the x11regression
  feedback rebuild at x11pt2.f:851-859, where the C++ takes a `STCSI = STO`
  shortcut that is only bit-equivalent when no such factor exists — so the fatal
  is now narrowed to exactly that path (`Ixreg in {1,2} && Axrgtd`) instead of
  firing everywhere. `Adjcyc` and `Axrghl` stay fatal. (2) `Lcentr` (x11pt3.f:280
  -> `vsfc`) then became reachable and is wired. `vsfc` had been ported all along
  — it is `vsfb`'s own tail in x11seas.cpp — only the call site was missing.
  **The gating trap here:** `Lcentr` runs INSIDE the Adjsea/Adjso block, so on
  any ordinary spec the oracle does not centre either and on/off are
  byte-identical; `regression{variables=(seasonal)}` measures 0.000 delta on
  every series (`Adjsea` is not 1 there — chkadj.f:165 needs nsea>0) and can
  never gate it no matter what else it does. A seasonal OUTLIER does:
  measured oracle on-vs-off d10 delta 2.06e-3 airline / 2.95e-4 payems /
  1.89e-3 expgs. Gated by `{airline,payems,expgs}_so-x11` (the narrowed wall
  alone) and `_so-centerseasonal-x11` (both). Supersedes the "UNGATED,
  currently unreachable" caveat in 9e56160.
- **`x11{appendbcst=yes}` — CLOSED, and it was the HARNESS, not the engine.**
  The last open sweep culprit. `Savbct` widens the b1/d10/d16 punch range back to
  `Pos1bk`, and `x13run_x11`'s `dump()` anchored its date arithmetic on the
  *range start* rather than on `Pos1ob` — so the leading backcast rows, which
  belong to dates BEFORE `Begspn`, pushed every label `Nbcst` periods late. A
  date-keyed diff then lined row k up against row k+Nbcst and reported 30-70%
  "drift" on columns that were bit-identical. `dump()` now takes `first`/`last`
  and a separate `anchor`. Gated by the new `*_appendbcst-x11` config (all 4
  series, `appendbcst`+`appendfcst`+real backcasts).
  **The whole spec_sweep factor space is now clean at t=3: 202/202 ok over 228
  rows, 15 factors, 26 oracle-rejected combinations.** Three fixes to
  `tools/spec_sweep.py` came out of this and are worth knowing before trusting
  its next report: (1) **pair-level attribution** — the single-level table was
  structurally blind to this bug, which needed `bcst!=none` AND
  `append in {bcst,both}`, so neither factor alone reached the 50% bar and the
  report printed *nothing* while seven rows drifted; the premise of a covering
  array is that bugs live at two-factor seams, so attribution has to be able to
  name one. (2) `--outdir` is now abspath'd: a relative one resolved against the
  per-case cwd, so every engine run died `rc=2` before doing any work and the
  report blamed 100% of every level of every factor — **if you ever see uniform
  100%-bad attribution, suspect the harness; no real defect is uniform across
  the space.** (3) a golden column whose max magnitude is under `ZERO_COL=1e-10`
  is compared on ABSOLUTE terms: with `x11{mode=add}` the force factors are
  additive differences, so `ffc` is identically zero — the oracle prints
  -1.1e-13, the engine an exact 0, and a pointwise relative test called that
  rel=1.000 on a table where both were right.
- **The x11{} yes/no switches (`excludefcst` / `true7term` / `sfshort`) — CLOSED
  (bit-exact):** all three were **accepted and silently dropped**. The numerics
  had been ported years-of-commits ago — `Noxfct` at x11pt2.f:469/636/747,
  `Tru7hn` in hndtrn, `Shrtsf` in vsfb — but `gt_x11` had no parse branch for
  them, so `ctx.x11msc.*` stayed at its gtinpt.f default and the run came back
  `OUTCOME: OK` having quietly done the default thing (excludefcst ~3.5e-3 in
  d13; true7term 1.2e-3..5.5; sfshort ~2e-3). One shared branch in
  `readers_spec.cpp` covers all of them (they share getx11.f's YSNDIC
  `flag = ivec(1).eq.1` shape). Gating them needs the right conditions or the
  switch is inert: `true7term` needs `trendma=7` (hndtrn's `while (i==7 &&
  !tru7hn)` is otherwise dead) and `sfshort` needs a span under five years
  (vsfb.f:63/65/82). `centerseasonal` (`Lcentr`) is parsed too but stays walled
  — x11pt3.f:280 is reachable only with a regARIMA seasonal/SO regressor, and
  that path is `x11_not_ported`, i.e. fatal rather than silent.
  `print1stpass` (`Prt1ps`) is print surface and is not in /x11msc/.
  Gated by `*_{excludefcst,true7term,sfshort}-x11` on all 4 series.
  **Test-harness note:** the m3 estimate gate's `_close` now takes the golden's
  PRINTED precision as an absolute floor (`_print_ulp`). The .udg prints 4-6
  significant digits, so a golden of `36.1384` pins the true value only to
  +/-5e-5; on the four-year sfshort spans the statistics are small enough that
  this rounding alone exceeds rtol 1e-6. It is a property of the golden format,
  not of the engine — the same specs' d10-d13 gate at ~5e-15 against the
  15-digit save goldens. For large values the relative bound still dominates,
  so nothing else loosened.
- **`series{modelspan=}` — CLOSED (bit-exact):** the model span was parsed
  (`Begmdl`/`Endmdl` were populated) but **never applied** — the regARIMA model
  was silently fit over the whole series span, so every coefficient, the
  forecasts, and the forecast-extended X-11 filter tail were wrong (~5.6e-4 in
  d10-d13, growing toward the series end) behind an `OUTCOME: OK`. It was the
  100%-attribution culprit in the `tools/spec_sweep.py` covering array (15/15
  rows). Ported: (1) `Ldestm` — a *local* in `gtinpt.cpp` that was `(void)`'d,
  so `ctx.arima.ldestm` was never written and the arima.f:136 gate could never
  fire; (2) arima.f:134-157's narrowing (`Begspn`/`Endspn` onto `Begmdl`/
  `Endmdl`, then `Nspobs`/`Frstsy`/`Nomnfy`/`Nobspf`/`Adj1st` recomputed, and
  the estimation input read from `Sto(Pos1ob+nbeg)`); (3) `setspn.f` as a lambda
  in `run_pre_model.cpp`, called at arima.f:1145 to restore the span **END
  before forecasting** (forecasts must start after the last observation of the
  SERIES span) and at arima.f:1181 to restore the START afterwards, each
  followed by a `trnfcn` + `regvar` rebuild; (4) editor.f:176-187's
  `nbeg>0 ⇒ Nbcst=0`. Placement is the whole trick: adjsrs (editor.f:849),
  a1/a2/a3, and trnaic (x11ari.f:84) all run BEFORE arima.f, hence on the full
  span, so the narrowing sits between trnaic and the transform. Two C++-only
  seams the Fortran does not have, both silent: `ctx.series.tsrs` is how SEATS
  gets its decomposition input, so the restore must refresh it or the whole
  decomposition slides `nbeg` periods (rel ~1.0 in s10-s13); and `x13run_m3`
  derived `nefobs` as `Nspobs-Nintvl` at print time, i.e. after the restore —
  the count is now recorded at estimation time as `ctx.est_nefobs`. Ported
  asymmetry kept: setspn.f:135 uses `max(Nfcst-Fctdrp,0)` for `Nobspf` where
  arima.f:151 uses `Nfdrp`. Gated by the new `*_modelspan-x11` (open end) and
  `*_modelspan-both-x11` (both ends) configs on all 4 series.
- **regARIMA HOLIDAY regressors + X-11 (`Finhol`) — CLOSED (bit-exact):**
  `regression{variables=(easter[N] labor[N] thanks[N])}` with `x11{}` left the
  holiday effect in D11/D13/D16 (~1.4e-2 in March/April) behind an
  `OUTCOME: OK`. Cause: `Finhol` was never initialized. gtinpt.f:407 defaults it
  **TRUE** and gtinpt.f:1240-1242 clears it at the parse tail only when no
  holiday regressor turned up (`Havhol`, set by adpdrg/getreg — or `Leastr` for
  the aictest easter). It is not a print flag: with Finhol true x11pt2 folds
  Fachol into Faccal AND x11pt3's `.not.Finhol` guard skips the divide that
  would take it back out, so the holiday belongs in the combined calendar factor.
  Both were already ported correctly — with Finhol false they cancel exactly,
  which is why the effect vanished silently. Gated by the new `*_holiday-x11`
  config (all 4 series) in `test_x11_tables.py`, which now also gates **d16**
  (optional, where the golden ships) — d16 is the table that catches a Faccal
  missing a calendar factor while d10-d13 still look right.
  **Corpus-generator hazard** found here: `generated/genspecs.py` and
  `extra/genextra.py` both wipe every `*.spc` in their directory before
  regenerating, but both directories hold committed specs the generators do not
  produce (~130 in `generated/`). Running them deletes those. Both docstrings now
  warn; add a config and write only the new specs, or `git checkout` the
  directory afterwards (which reverts the generator too).
- **`transform{}` user PRIOR-adjustment factors — CLOSED (bit-exact):** the
  `data=`/`file=` prior factor series, permanent (`Usrpad`) and temporary
  (`Usrtad`), alone or combined with the predefined `adjust=lom/loq/lpyear`
  prior. 4 specs × d10–d13 gate at ~5e-15 (`test_prior_adj_tables.py`, corpus
  `extra/airline_prior-*`). Ported: `adjsrs.f`'s factor combine (shared
  `adjsrs_factors` in `regarima/priadj.cpp`), `addadj.f`'s percent→ratio and
  tail pad-out, `getadj.f`'s `file=`/`type=`/`temppriortrend=`, and x11pt3's two
  temporary-prior folds (`:591-604` strip from D13, `:937-946` fold into D12
  when `Lprntr`). THREE silent-wrongness bugs this closed, all of which returned
  `OUTCOME: OK` with wrong numbers: `transform{file=}` was parsed but never
  read (~1.2e-1 off), the NO-MODEL path never applied any prior at all (nothing
  upstream of run_x11 runs, so it decomposed the RAW series and then threw
  indexing `Usrpad` at `Frstap==0`), and a multi-set prior silently used only
  the first. Deliberately fatal, not silent: `Nprtyp>1`, `mode=diff`/additive
  factors (`Adjmod=2`), `format=`, and the `addadj` span shift (`Frstad!=0`,
  which the oracle rejects outright too).
- **x11regression `tdprior` (user prior trading day, Kswv=1) — CLOSED (bit-exact):**
  a4 factor + d10-d13 gate at ~5e-15 (`test_x11_tdprior_tables.py`). pritd.f ported
  (parser + parse-time weight-standardize sum-7 + td6var/x11ref_td). The oracle runs
  x11pt1 pre-model (x11ari.f:99-133) so regARIMA fits the prior-adjusted series; the
  C++ mirrors by dividing the pre-model estimation input (`run_pre_model`) AND the
  X-11 buffer (`x11pt1`) by the pritd factor.
- **x11regression OLS-estimated prior TD (Ixreg>=2 / xrgdrv) — CLOSED (bit-exact):**
  the modeled `-td` spec is promoted Ixreg 1->2 (gtinpt.f:1201), so the oracle runs a
  transparent (model-free) X-11 pass (`xrgdrv`) whose x11mdl OLS estimates the TD on
  the irregular, builds Faccal, sets Ixreg=3; x11pt1 divides Sto by Faccal so regARIMA
  fits the TD-adjusted series. C++ hoists `core/src/x11/xrgdrv.cpp` ahead of the
  estimate (run_pre_model divides the estimation input by the stashed Faccal). xrm +
  d10-d13 gate at ~5e-15 (`test_x11regression_tables.py`). KEY bug: the transparent
  pass must NOT leak `Lterm` (drives the editor's per-period seasonal-filter
  re-resolution) or the Bundesbank `Ksdev` spread into the main run — save/restore
  both in xrgdrv.cpp (same class as the slidingspans/history per-span resets). Also
  landed the faithful Nfcstx forecast-extended factor. Still fatal: additive /
  pseudo-additive prior-TD and the regression-based aictest TD variant.
- **x11regression `aictest=(easter)` sub-engine — CLOSED (bit-exact):** the modeled
  `variables=(td) aictest=(easter)` spec runs x11mdl's automatic Easter AICC test on
  the irregular (x11aic easter branch: score no-Easter vs windows {1,8,15} via
  xrlkhd, keep lowest -> window 15) THEN automatic AO outlier identification (7 AOs)
  via the shared `idotlr` given a new `lxreg` path (OLS regx11 re-fits, no ARMA
  filter). Gates the 4 `aictest.xe.aicc.*` canaries + window, the 14-col xrm, and
  b16/c16 at ~5e-15 (`test_x11regression_tables.py`). Decisive gotchas: editor.f:
  1729-1736 (easter present -> Sigxrg=0/Otlxrg=T, tdxtrm skipped); Cvxalf default
  = PT5 = 0.05 (not 0.5) -> Critxr 3.89; the otlvar armafl must be `!lxreg`-gated.
  See **`tools/x11regression_aictest_scope.md`**. Still follow-on: aictest td/user.
- **BLS CES production specs — GATED bit-exact (`test_ces_tables.py`):** the real,
  unedited BLS Current Employment Statistics specs (all-UPPERCASE keywords, the
  axis that surfaced the case-sensitive value-match bug in 8e27426). Both
  `tests/corpus/ces/AE1011330000{,_simple}.spc` gate b1/d10/d11/d16 at ~5e-15,
  including the full spec's 11 user `usertype=td` regressors + AO identification
  at `critical=10.5`. Closed here: the three unparsed `x11{}` args `appendfcst`/
  `appendbcst` (Savfct/Savbct — they widen the d10/d16 punch range to the
  forecast span, no arithmetic) and `final=`/`keepholiday` (Finao/Finls/Finusr/
  Fintc/Finhol; `final=user` is inert when the user regressors are `usertype=td`,
  since their effect lands in Factd, not Facusr), plus a d16 (`ststd`) snapshot on
  ctx so the harness can emit the combined seasonal+calendar factors.
- **composite{} / indirect adjustment — CLOSED for X-11 (bit-exact), incs 1-3.**
  The only multi-spec feature: the oracle runs a metafile's specs in ONE process
  so the aggregation COMMONs persist, and `tools/x13run_composite.cpp` carries
  `agr_cmn`/`agrsrs_cmn` between per-spec contexts to match. Ported `agr.f`,
  `agr1.f`, `setapt.f`, real `getcmp.f`, and agr2's direct-`O` branch, so the
  **direct composite total** gates d10–d13. Increment 2 added the rest of
  `agr2` (O1..O5/Ci/Ci2/Omod), `agrxpt.f` and `agr3.f`'s indirect core, so the
  **indirect** isf/isa/itn/iir gate too — all 16 metafile tables (components +
  direct + indirect) at ~5e-15 (`test_composite_tables.py`, corpus
  `census-examples/composite-fixed/`). Surfaced two never-written main-path gaps:
  `Lstyr/Lstmo/L0/Ly0` (editor.f:236-237/423-424 — they ARE the Itest span
  signature) and `Orig2` (editor.f:2492 — the buffer agr2 aggregates). NOTE the
  faithful aliasing: agr3.f EQUIVALENCEs its `tempo`/`stexx` scratch onto
  Orig2/Omod. Increment 3 added the direct-vs-indirect COMPARISON STATISTICS —
  `aggmea.f` + agr2's `Iagr==4` branch, plus the two pieces agr3 had deferred:
  `/kcser/ Ckhs` (x11pt3.f:379; a COMMON exactly because agr3 reads it after
  x11pt3 returns) and the `Tem` direct trend, a forced 13/5-term Henderson on
  Ckhs. All 24 `di()` roughness values match the oracle's printed table and all
  four savelog canaries (`r1mse`/`r1rmse`/`r2mse`/`r2rmse`) + `indtrendma` match
  `total.udg`, gated at the oracle's own 3-decimal print precision.
  **Increment 4 closes the INDIRECT DIAGNOSTICS front, which x11pt4 had been
  blocking** — `agr3.f:288-350`'s indirect D8/D9 SI ratios and seasonality test
  battery, plus the second `x11pt4` pass at `x11ari.f:341`. Two Iagr==4 branches
  inside x11pt4 had to land: `O5` (the aggregate with the indirect combined
  calendar factor divided out, rebuilt at x11pt4.f:168) is the calendar-adjusted
  original behind E8, and `O` rather than `Series` is the numerator of E18/EB.
  Note that the D8 battery is **not** print surface here: ftest/kwtest/mstest/
  combft write `/tests/ Test1,Test2`, which ARE the M7 inputs the indirect
  `f3cal` reads, and vsfa's `Ratis` is `if2.is`. Gated two ways: the whole
  `if2.*`/`if3.*` block against `total.udg` at its printed precision (**zero new
  goldens** — it already shipped all of it, including `if2.fsb1`, which has no
  indirect counterpart and is the DIRECT Fpres reprinted, a premise the gate
  asserts), and 16 new save tables (`id8 id9 ie1 ie2 ie3 ie5 ip5 ie6 ip6 ie7 ip7
  ie8 ip8 iee i18 ita`) at ~5e-15. Harness gotcha found here: `x13run_composite`'s
  `dump()` anchored its dates on the RANGE START, which is wrong for the change
  tables (they begin at `Pos1ob+1`) — it now takes a separate `anchor`, the same
  fix `x13run_x11` needed for appendbcst. `prtagr`/`pragr2` are print surface this
  port defers by design. (`cmpchi.f`/`cmpstr.f` were misfiled in the scouting
  inventory: neither is composite.) Still open for composite: the SEATS branch
  (`agr3s.f`), pseudo-additive, and the forced/rounded indirect series. Map:
  **`tools/composite_scouting.md`**.
- **x11pt4 increment 1 — the F2 SEASONALITY TEST BATTERY — CLOSED (bit-exact).**
  `x11pt4.f` was never ported at all, so the whole diagnostics front the .udg
  reports (`f2.*`, `f3.m01`-`m11`/`q`/`qm2`, the E tables) was simply absent.
  Increment 1 lands the four tests: `ftest.f` (one-way ANOVA, stable
  seasonality), `kwtest.f` (Kruskal-Wallis), `mstest.f` (two-way ANOVA, MOVING
  seasonality) and `combft.f` (the combined identifiable-seasonality verdict) as
  `core/src/x11/x11tests.cpp`, plus `fvalue.f` in `numeric.cpp`. Wired at the two
  oracle call sites: the B1 test in x11pt2 (`x11pt2.f:436`, Ind=2 -> Fpres/P3)
  and the D8 battery in x11pt3 (`x11pt3.f:111-134`, Ind=0 -> Fstabl/P1, then
  kwtest/mstest/combft). **This is the floor the quality statistics stand on** --
  combft writes `/tests/ Test1,Test2`, which ARE the M7 inputs and hence feed Q.
  Gated by the new `tests/parity/test_x11_diagnostics.py`: 114 corpus specs,
  **zero new goldens blessed** -- every x11 `.udg` golden already shipped the
  canaries. Three things worth knowing:
  (1) **The gate's tolerance is the oracle's PRINT precision, by necessity.**
  svf2f3.f writes the statistic F11.3 and the probability F8.2, so the golden
  pins them only to +/-5e-4 / +/-5e-3. Same policy as test_m3_estimate's
  `_print_ulp`; there is no 15-digit golden for a savelog canary.
  (2) **A snapshot, not a live read** (`ctx.x11_f2tests`, taken in run_x11.cpp
  where x11pt4 sits). slidingspans{}/history{} replay x11pt3 per span and each
  replay OVERWRITES `/tests/`; reading it at harness-exit reported the LAST
  span's statistics. That was measurable -- `airline_slidingspans` came out
  fsd8 371.6 against the golden's 190.7 -- and is the same state-leak class as
  the xtrm.ksdev / Lterm per-span resets.
  (3) **CB-17**: `fvalue.f` ZEROES ITS OWN ARGUMENT on both "probability is 1"
  exits, and every caller stores the statistic AFTER the call -- so a series with
  no between-season variation reports `F=0.000` next to `prob=100.00`. `fvalue`
  therefore takes `double&`; the reference parameter exists only to reproduce the
  bug. Not reached by the current corpus, hence pinned in `census_bugs.md`.
- **x11pt4 increment 2 — the PART-F SUMMARY MEASURES + the F3 QUALITY
  STATISTICS — CLOSED (bit-exact at the .udg's printed precision).**
  `core/src/x11/x11summ.{hpp,cpp}`: `sumry.f`, `vars.f`/`varlog.f`/`varian.f`,
  `avedur.f`, `issame.f`, `isfals.f`, `f3cal.f` and x11pt4.f's Part-F body
  (`:320-713`). Gates the whole remaining `.udg` block — `f2.a01-a12`/`b*`/`c*`/
  `d`/`e`/`mcd`/`f`/`g`/`ic`/`is` and `f3.m01-m11`/`q`/`qm2`/`fail` — over the
  same 114 corpus specs, **again with zero new goldens blessed**
  (`test_x11_diagnostics.py`, now 342 tests). Tolerance is per-field printed
  precision: E15.8 for a/c, F8.2 for the ratio lines, `2P`+F8.2 for b (compare in
  the SCALED space, 5e-3 — halving that to 5e-5 on the fraction is wrong and was
  the first false failure), f6.3 for the M statistics, F5.2 for Q. **The b block
  and the `f3.m*` lines carry a `1x` between the colon and the first field and
  the others do not**, so a fixed-width parse of the golden has to skip it.
  Structural notes: x11pt4 works on COPIES of Series/Stci/Stcime/Stome/Stc/Sti,
  because it mutates them in place (a log/antilog round trip on Stc, divide-then-
  multiply round trips on the rest) AFTER the oracle has already punched d10-d16
  — the oracle's saved tables are the PRE-x11pt4 values, and the C++ harness
  dumps at exit. It also needs the INTERNAL (pre-publication) D13/D12, so x11pt3
  now snapshots `ctx.x11_sti_int`/`x11_stc_int` before folding the AO/TC back
  into D13 and the LS into D12. Same snapshot discipline as increment 1 for the
  outputs (`ctx.x11_f2inpt2`/`x11_f2work2`/`x11_f2mcd`/`x11_f2ratic`/`x11_f2ratis`
  — the sliding-spans replays overwrite the live COMMONs, and `Ratic` visibly
  did). **FIVE pre-existing silent-wrongness bugs this surfaced**, none of which
  any existing gate could see, because they all live in tables or COMMONs the
  port never read until now:
  (1) **`x11{mode=add}` built E1 wrong.** x11pt3.f:1227-1231's weight-zero
  replacement is `Stome = Series - Sti` additively and `Series / Sti`
  multiplicatively; only the divide was ported, so every additive spec's e1/e2/e3
  save tables were wrong (Ombar 2.41 against 0.13).
  (2) **The x11pt3 `sp2 -> Sprior` writeback was skipped as "deferred".** It is
  not: x11pt3.f:1284-1288 replaces Sprior with `Sprior*Facls*Facao*Factc*Facusr`
  and x11pt4's `Pbar`/`Psq`/`Vp` measure the result. (`nadj2` really is dead by
  that point, so only the copy and the `Kfmt=0 -> 1` matter.)
  (3) **`/adjcmn/ Adj` was only populated when a prior existed**, but adjsrs.f
  records Nadj/Begadj/Adj1st unconditionally and its no-prior ELSE fills Adj with
  the mode IDENTITY. With Nadj==0 x11int never copies it, so Sprior stayed the
  all-ZERO COMMON — and (2) then multiplied the outlier/user factors into zero,
  giving `Pbar = NaN` and `Q = NaN`. Kfmt stays keyed to a real prior; only the
  record became unconditional.
  (4) **x11pt3's Part-E `nadj2>0` Sprior fold was skipped** ("nadj2==0 base"),
  which is only true with no prior; with one, E2's weight-zero replacement is
  built from the trend and needs the prior put back (Cimbar off ~10%).
  (5) **`gtinpt.f:1239 Khol=Keastr` was never ported**, and it is READ one line
  later by the `Finhol` test: with the classic X-11 Easter on, `Khol==1` keeps
  Finhol TRUE. Without it Finhol went false, x11pt3.f:525's `.not.Finhol` gate
  opened, and `Faccal /= Fachol` cancelled the Easter factor out of Faccal
  entirely (x11pt1 folds X11hol into Faccal, x11pt2 folds the same factor into
  Fachol) — so `f2.a*` Tdbar and Vtd reported no calendar effect at all.
  Also ported here: **editor.f:2071-2097**, the rule that a 3x15 seasonal filter
  is silently downgraded to a STABLE filter on a series under twenty years —
  which sets `Lstabl`, and `Lstabl` is what f3cal reads to decide whether M8-M11
  exist at all (the engine was emitting four statistics the oracle suppresses).
  It needs `Posffc`, so it sits after setxpt, not with the rest of the
  editor.f:2042-2103 block. **CB-18** (x11pt4's `allgud` is the NEGATION of what
  its name and its six consumers mean — `isfals` is true when something is BAD)
  and **CB-19** (the E2 restore at x11pt4.f:653 has its `copy` arguments
  reversed, so the LS/user divide is never undone and `/work/ Temp` is clobbered)
  are both transcribed verbatim; both are unreachable here (they need
  `x11{constant=}`, which is walled in x11pt3).
- **x11pt4 increment 3 — the PART-E TABLES — CLOSED (bit-exact), and x11pt4 is
  now fully ported.** `x11pt4_etables` (x11pt4.f:162-319) in the same
  `core/src/x11/x11summ.cpp`: the E5-E8 change tables and their `pe*` percent
  twins, E6.A/E6.R (the forced and rounded SA series' changes), E11 (the robust
  SA series) and E18 (the final adjustment ratios A1/D11), plus the EB total-
  factor table. E1/E2/E3 needed no arithmetic at all — they ARE Stome/Stcime/
  Stime as x11pt3 leaves them, so the harness just punches them. Gated by the new
  `tests/parity/test_x11_etables.py`: 795 tests over the 61 corpus specs that
  ship the family (plus `extra/airline_automdl-x11-force` for e6a/e6r), **again
  zero new goldens blessed**; same two-tier tolerance as `test_x11_tables`
  (1e-12 arithmetic / 1e-6 estimation) with an absolute floor on the change
  tables, since a difference of neighbouring values has no relative precision
  where the change is ~0. Ordering is the whole constraint: Part E reads the LIVE
  buffers, so it must run BEFORE `x11pt4_partf` takes its working copies, and E7
  wants the PUBLISHED (LS/TC-folded) trend — `ctx.x11srs.stc`, the oracle's Stc2
  — where Part F wants the internal one. Three things worth knowing:
  (1) **`e4` has no golden and never will** — x11pt4.f:156 calls `table` with no
  `punch`, so the ratios of annual totals are print surface. Deliberately not
  produced.
  (2) **`pe5`-`pe8` are the SAME series as `e5`-`e8`, not a separate table.**
  pragr2.f passes `Muladd.ne.1` as punch's percent flag, so they are scaled x100
  in multiplicative/log-additive mode and printed UNSCALED in additive mode.
  Emitting them only when `muladd != 1` (the obvious reading) drops them from
  every `mode=add` spec.
  (3) **E11 is a plain subtract/add in EVERY mode**, multiplicative included
  (`Series - Stome + Stcime`, x11pt4.f:272-274) — there is no divsub variant.
  The Fortran precedence trap at x11pt4.f:241-243 is reproduced: `.and.` binds
  tighter than `.or.`, so `Iagr.lt.4` qualifies only the Nustad/Lprntr clause of
  the E7 trend selection, not the Finls/Adjls one.
- **The `seats{}` OPTION SURFACE — swept, and the sweep found real wrongness.**
  `tools/spec_sweep.py --suite seats` showed only 6 of 22 parsed seats options
  were ever read; 11 were parsed, stored, and silently ignored. Three parallel
  worktree agents closed the front. **What generalizes: a "parsed but unread"
  option is not automatically a wrong-numbers bug, and the difference is only
  knowable by measurement.** Each flag was first proven to move the ORACLE
  (on vs off) before any porting; the outcomes split three ways.
  - **`imean` — CLOSED bit-exact.** The `ansub9.f:1072-1080` bridge DERIVES
    `L_IMEAN` from the presence of a `Constant` regressor group; the missing
    piece was `gtseat.f:124`'s explicit override (`Kmean=2-ivec(1)`). Key
    distinction: `L_IMEAN` gates only whether SEATS MODELS a mean (the wm
    centering, `za` in FCAST, `wmf/wmb` in ESTBUR) — it does NOT change which
    series is decomposed, so run_seats' Constant add-back stays keyed on the
    regressor. Oracle deltas 3.5e-4..8.5e-1; engine ~5e-15 both directions.
  - **`hpcycle`/`hplan`/`hptarget`/`hprmls` — bridge CLOSED + gated, FILTER
    walled, and NOT a wrongness bug.** s10-s18 are bit-identical under EVERY HP
    setting (measured 0.000e+00); the entire blast radius is `.cyc`/`.ltt`,
    which the port simply does not write. So no fatal guard was added — there
    was nothing silently wrong to guard. The filter itself needs `HPTRCOMP` over
    `1..Nz+lfor`, i.e. the unported SEATS forecast decomposition
    (`ansub3.f:356-678`); map in **`tools/seats_hp_scouting.md`**.
  - **`finite` (`Lfinit`) — decomposition invariance CLOSED + gated, output
    surface scouted.** Measured over 24 oracle configurations: never moves any
    table. What it DOES gate is ten save tables (`faf/fac/ftf/ftc/gaf/gac/gtf/
    gtc/tac/ttc`) that the oracle otherwise **accepts the `save=` token for and
    writes no file** — behind `sigex.f:1502 IF(Lfinit) CALL getDiag`, a ~7.6
    kloc closure with no C++ at all. Deliberately not read; that IS faithful.
  - **`noadmiss` — the one that WAS silently wrong.** The port applied the
    inadmissibility verdict *nowhere* and decomposed anyway. On `expgs`+automdl
    the oracle writes **0 rows**; the engine emitted 1902. Now fatals via the
    two oracle tests (SPECTRU `qt1<0`, DecompSpectrum cycle `varwnc<0`).
    `statseas` is walled by a ported CHANGEMODEL *predicate* rather than a
    blanket fatal — and the trap there was that `analts.f:1125-1136` NEGATES
    `Phi/Th/Bth(1)/Bphi(1)` before the call, so the naive sign fired on exactly
    the complement of the oracle's real model rewrites.
  - **CB-14** `seats{bias=0}` is validated, accepted, then overwritten with 1
    (`analts.f:1647-1653`); **CB-15** `seats{hplan=}` silently re-enables an
    explicit `hpcycle=no`; **CB-16** `finite=yes` zeroes `pctreductionyr1..5`
    off an uninitialised COMMON.
  - **Merge lesson:** three agents editing `seatopts.{hpp,cpp}` conflicted only
    additively EXCEPT that a naive "keep both sides" resolution spliced one
    agent's `seats_resolve_options` tail into the middle of another's
    `seats_decomp_unported_reason` — after its `return`. Check FUNCTION
    BOUNDARIES after resolving, not just that the markers are gone. All three
    also independently claimed `CB-14`; renumber on merge.
  - **`tests/corpus/generated/MANIFEST` covers ONLY genspecs.py's own output**
    (21 configs x 4 series = 84), not the ~164 hand-authored specs in the same
    directory. It does not need regenerating when specs are hand-added — and
    running the generator to "refresh" it deletes them (the standing hazard).
- **No open xfails.** The former estimation-frontier xfails (`unrate_automdl-
  aictest-x11`, `payems_automdl-acceptdefault`) now pass; the suite is 0 xfail.
