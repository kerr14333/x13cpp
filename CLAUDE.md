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
  `total.udg`, gated at the oracle's own 3-decimal print precision. What is left
  under the old inc3 heading is NOT composite-specific — the indirect E/F tables
  need `x11pt4`, and the indirect D8/D9 F-tests need `ftest`/`kwtest`/`mstest`/
  `combft`, both unported on the DIRECT side too; `prtagr`/`pragr2` are print
  surface this port defers by design. (`cmpchi.f`/`cmpstr.f` were misfiled in the
  scouting inventory: neither is composite.) Still open for composite: the SEATS
  branch (`agr3s.f`) and pseudo-additive. Map:
  **`tools/composite_scouting.md`**.
- **No open xfails.** The former estimation-frontier xfails (`unrate_automdl-
  aictest-x11`, `payems_automdl-acceptdefault`) now pass; the suite is 0 xfail.
