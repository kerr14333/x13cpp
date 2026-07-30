# M5 port notes -- the closed-work archive

Every feature closed during M5 (X-11 / SEATS), in the order it landed, with
the measurements, the traps, and the Census defects each one surfaced. Moved
out of `CLAUDE.md` so it is not resident in every session -- it was 95% of
that file (~2,100 lines, ~40k tokens).

**Read the matching entry before touching a subsystem.** These are records of
bugs already paid for once, and several of them cost a full session to find.
`tools/SESSION_HANDOFF.md` owns what is OPEN; this file owns what was
MEASURED. Nothing here is a status list -- each entry describes the state at
the moment it closed, and must not be updated to track later work.

Entries are numbered and in chronological order. The contents list below is
deliberately link-free (the titles carry em dashes and backticks, which make
generated anchors fragile) -- navigate by searching the number, e.g. `## 47.`.

## Contents

1. `history{estimates=(fcst)}` — the out-of-sample FORECAST-ERROR history — CLOSED (at the per-span floor).
2. The three MODEL histories `estimates=(aic arma td)` — CLOSED, and `history{}`'s whole `estimates=` surface is now covered.
3. Model X-11 path — CLOSED.
4. X-11 spec-option front — CLOSED (bit-exact):
5. `force{}` non-original targets + the forecast-span prior — CLOSED (bit-exact):
6. `forecast{maxback=}` BACKCASTS — CLOSED (bit-exact):
7. regARIMA SEASONAL-OUTLIER (`Adjso`) through X-11 + `x11{centerseasonal=}` (`Lcentr`) — CLOSED (bit-exact).
8. `x11{appendbcst=yes}` — CLOSED, and it was the HARNESS, not the engine.
9. The x11{} yes/no switches (`excludefcst` / `true7term` / `sfshort`) — CLOSED (bit-exact):
10. `series{modelspan=}` — CLOSED (bit-exact):
11. regARIMA HOLIDAY regressors + X-11 (`Finhol`) — CLOSED (bit-exact):
12. `transform{}` user PRIOR-adjustment factors — CLOSED (bit-exact):
13. x11regression `tdprior` (user prior trading day, Kswv=1) — CLOSED (bit-exact):
14. x11regression OLS-estimated prior TD (Ixreg>=2 / xrgdrv) — CLOSED (bit-exact):
15. x11regression `aictest=(easter)` sub-engine — CLOSED (bit-exact):
16. BLS CES production specs — GATED bit-exact (`test_ces_tables.py`):
17. composite{} / indirect adjustment — CLOSED for X-11 (bit-exact), incs 1-3.
18. `transform{constant=}` + `x11{final=(ls)}`'s Part-E fold — CLOSED (bit-exact).
19. x11pt4 increment 1 — the F2 SEASONALITY TEST BATTERY — CLOSED (bit-exact).
20. x11pt4 increment 2 — the PART-F SUMMARY MEASURES + the F3 QUALITY STATISTICS — CLOSED (bit-exact at the .udg's printed precision).
21. x11pt4 increment 3 — the PART-E TABLES — CLOSED (bit-exact), and x11pt4 is now fully ported.
22. The `seats{}` OPTION SURFACE — swept, and the sweep found real wrongness.
23. force{}'s NEGATIVE-VALUE CORRECTION + the slidingspans `ads` table — CLOSED (bit-exact), and the span-replay clobber it exposed is fixed.
24. `x11regression{tdprior=}` under `x11{mode=logadd}` — CLOSED (bit-exact), and it was a SILENT wrong-numbers bug hiding behind a fatal that never fired.
25. `x11pt2 tdlom Adjtd==0` — the unreachability proof was WRONG, and the reason is worth keeping.
26. `history{}`'s last two gaps — `Fixper` and `Indrev` — CLOSED, plus the `fixmdl` and default-`start` paths they dragged in. `history{}` is now complete except for its outlier / alternate-target surface.
27. `history{}`'s option surface — MEASURED, and `fixreg=` CLOSED.
28. PER-SPAN `xrgdrv` — CLOSED, and with it `history{fixx11reg=}`.
29. `history{}`'s HELD-BACK OUTLIERS (`rmotrv`/`chkorv`) — CLOSED, and it was a DEFAULT-path silent wrong-numbers bug that the scouting doc had explicitly measured as CORRECT.
30. `history{sadjlags= trendlags= target=}` — the ALTERNATE REVISION TARGETS — CLOSED (at the per-span floor).
31. FIXED / INITIAL COEFFICIENTS — `regression{b=}` and `arima{ar= ma= diff=}` — CLOSED (bit-exact), and both were the silent wrong-numbers class.
32. PRIORITY #2 STARTED: R and Python can now call the engine in-process.
33. `slidingspans{}` / `history{}` under `seats{}` — MEASURED SILENTLY DROPPED, partially closed.
34. `slidingspans{}` / `history{}` under `seats{}` — NOW CLOSED.
35. A 7-way Codex review pass over the engine — what it found, and what it didn't.
36. `check{}` -- the regARIMA RESIDUAL DIAGNOSTICS -- CLOSED (line-exact), and the whole spec was parsed-and-dropped.
37. The rest of the ESTIMATION savelog block -- outlier counts, ARMA roots, the ARMA coefficient table -- CLOSED (byte-exact).
38. `aape` -- the average absolute percentage FORECAST ERROR (amdfct.f) -- CLOSED (byte-exact, within-sample).
39. D8B and D9A -- CLOSED (line-exact), and the span-replay clobber came with them.
40. The remaining single-line X-11 savelog canaries -- `d11.f`, `d11.3y.f`, `sfmsr`, `autosf.msrNN`, `d7trendma`, `finaltrendma` -- CLOSED (line-exact).
41. The SPECTRUM diagnostic block is silently absent on every monthly run -- scouted, NOT ported.
42. The SPECTRUM PEAK diagnostics -- CLOSED (byte-exact), and the whole block was silently absent on every monthly run.
43. The QS SEASONALITY statistics (`genqs.f`) -- the DIRECT X-11 path CLOSED (byte-exact), and the whole block was silently absent.
44. The MODEL-ONLY diagnostics path -- CLOSED, and it unblocked BOTH the QS block and the spectrum peak block at once.
45. The QS / NP diagnostics under `seats{}` -- CLOSED (byte-exact), and it found a wrong-numbers bug on EVERY non-x11 run that is not taking a log.
46. `getTPeaks` -- the TUKEY SPECTRAL PEAK probabilities -- CLOSED (byte-exact), and it was ~130 lines, not the ~850 the scouting doc predicted.
47. `pickmdl{}` / `automx.f` — CLOSED (bit-exact), and it was parse-only AND silently model-less.
48. The composite DIAGNOSTICS front, direct and INDIRECT (`Iagr==4`) -- CLOSED (bit-exact), and the direct half was pure harness coverage.
49. `composite{}` under `seats{}` -- `agr3s.f` -- CLOSED (bit-exact over the observed span), and a SEATS metafile used to come back FATAL.
50. `estimate{outofsample=}` / `pickmdl{outofsample=}` -- amdfct.f's OUT-OF-SAMPLE arm -- CLOSED (bit-exact), and it is not a label change.
51. `pickmdl{}` + `forecast{maxback=}` -- amdfct's BACKCAST arm -- CLOSED, and `bcstlim=` turns out to be INERT (CB-33).
52. `pickmdl{}` + `regression{aictest=}` -- the PER-CANDIDATE AIC-regressor tests and the Picktd restore -- CLOSED (bit-exact), and it found three silent-wrongness bugs, two of them on paths that have nothing to do with pickmdl.
53. The SEATS FORECAST decomposition (`ansub3.f:353-678`) -- the `tfd`/`sfd`/ `afd`/`yfd` tables -- PORTED, 13 of 52 specs bit-exact, 39 measurably wrong and asserted as such.

---

## 0. SEATS decomposition core -- the seasonal front

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

---

## 1. `history{estimates=(fcst)}` — the out-of-sample FORECAST-ERROR history — CLOSED (at the per-span floor).

 It was **accepted and silently dropped**:
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

## 2. The three MODEL histories `estimates=(aic arma td)` — CLOSED, and `history{}`'s whole `estimates=` surface is now covered.

 Same silent-drop
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

## 3. Model X-11 path — CLOSED.

 The `*-aictest-x11` (airline/expgs/payems) and all
four `*-fixed-airline-x11` specs gate bit-exact on X-11 **and** on the fct forecast:
automd aictest selection + finalization, x11pt2's model factor combine, the
x11pt2/x11pt3 outlier folds (AO→D13, LS→D12 trend), the fixed-model leap-year
Sprior population, the post-idotlr regvar rebuild, and the fcstout LOM-prior
re-application all landed. See **`tools/x11_regeff_handoff.md`**.

## 4. X-11 spec-option front — CLOSED (bit-exact):

 `type`=summary/trend, `shrink`=
global/local, `sigmavec`, classic `x11easter` (transparent pre-pass →
holday/holidy/easter → Khol prior fold; codex-hardened per xrgdrv/editor.f), and
the user-regression prior factor (Facusr) all gate. The x11 parse-seam is
exhausted. Of the three remaining x11 stubs, user PRIOR factors (Nuspad/
Nustad) and force non-original targets (Iftrgt>0) are now closed — see the two
entries below; the only one still open is revisions getrev.
(Adjsea/Adjso regARIMA-seasonal combine landed, commit 970e85c.)

## 5. `force{}` non-original targets + the forecast-span prior — CLOSED (bit-exact):

 all four `target=` values gate (x11pt3.f:715-722 — `original`
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

## 6. `forecast{maxback=}` BACKCASTS — CLOSED (bit-exact):

 backcasts **segfaulted**
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

## 7. regARIMA SEASONAL-OUTLIER (`Adjso`) through X-11 + `x11{centerseasonal=}` (`Lcentr`) — CLOSED (bit-exact).

 Two things, closed together because neither
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

## 8. `x11{appendbcst=yes}` — CLOSED, and it was the HARNESS, not the engine.

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

## 9. The x11{} yes/no switches (`excludefcst` / `true7term` / `sfshort`) — CLOSED (bit-exact):

 all three were **accepted and silently dropped**. The numerics
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

## 10. `series{modelspan=}` — CLOSED (bit-exact):

 the model span was parsed
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

## 11. regARIMA HOLIDAY regressors + X-11 (`Finhol`) — CLOSED (bit-exact):

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

## 12. `transform{}` user PRIOR-adjustment factors — CLOSED (bit-exact):

 the
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

## 13. x11regression `tdprior` (user prior trading day, Kswv=1) — CLOSED (bit-exact):

a4 factor + d10-d13 gate at ~5e-15 (`test_x11_tdprior_tables.py`). pritd.f ported
(parser + parse-time weight-standardize sum-7 + td6var/x11ref_td). The oracle runs
x11pt1 pre-model (x11ari.f:99-133) so regARIMA fits the prior-adjusted series; the
C++ mirrors by dividing the pre-model estimation input (`run_pre_model`) AND the
X-11 buffer (`x11pt1`) by the pritd factor.

## 14. x11regression OLS-estimated prior TD (Ixreg>=2 / xrgdrv) — CLOSED (bit-exact):

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

## 15. x11regression `aictest=(easter)` sub-engine — CLOSED (bit-exact):

 the modeled
`variables=(td) aictest=(easter)` spec runs x11mdl's automatic Easter AICC test on
the irregular (x11aic easter branch: score no-Easter vs windows {1,8,15} via
xrlkhd, keep lowest -> window 15) THEN automatic AO outlier identification (7 AOs)
via the shared `idotlr` given a new `lxreg` path (OLS regx11 re-fits, no ARMA
filter). Gates the 4 `aictest.xe.aicc.*` canaries + window, the 14-col xrm, and
b16/c16 at ~5e-15 (`test_x11regression_tables.py`). Decisive gotchas: editor.f:
1729-1736 (easter present -> Sigxrg=0/Otlxrg=T, tdxtrm skipped); Cvxalf default
= PT5 = 0.05 (not 0.5) -> Critxr 3.89; the otlvar armafl must be `!lxreg`-gated.
See **`tools/x11regression_aictest_scope.md`**. Still follow-on: aictest td/user.

## 16. BLS CES production specs — GATED bit-exact (`test_ces_tables.py`):

 the real,
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

## 17. composite{} / indirect adjustment — CLOSED for X-11 (bit-exact), incs 1-3.

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

## 18. `transform{constant=}` + `x11{final=(ls)}`'s Part-E fold — CLOSED (bit-exact).

 `constant=` was **parsed and silently dropped**: `gt_transform`
consumed the token and never wrote `Cnstnt`, so every consumer's
`!= DNOTST` test was false and the run came back `OUTCOME: OK` with d10-d13
off by 1.3e-2..5.5e-2 (measured on airline with `constant=50`). Ported the
whole chain: the parse + `>0` validation, `editor.f:429-434` (the constant is
added to the WHOLE input series once, before the transform, the model and the
X-11 spine ever see it — done at the tail of `parse_spec`, the only place in
this port that is unambiguously "after gtinpt, before everything else"), and
x11pt3's four removal points — D11 + the original (`:621-635`, including the
`Iyrt>0` floor at zero, which is NOT print surface because force reads Stci
after it), the published trend (`:950-953` folded / `:1015-1018` plain), and
Part E's add-then-subtract round trip (`:1204-1228` / `:1275-1281`). The
pre-removal copies are the `sac`/`tac` save tables (`Stcipc`/`stc2pc`), now
gated. **Two things worth knowing.** (1) The Part-E round trip is NOT the
identity: `:1275-1281` subtracts the constant AFTER the AO/TC divides, so in
multiplicative mode the constant is divided by those factors first — verbatim.
(2) Three drivers were pinning `ctx.adj.cnstnt = DNOTST` (run_x11,
run_x11_span, xrgdrv) purely to correct the struct's zero-init; the default is
now set once in gtinpt and those three assignments had to GO, or a real user
constant would be cleared by the span replays and the transparent x11reg pass
(Cnstnt is a COMMON in the oracle and survives both). **This also makes CB-18
and CB-19 reachable** — both live in x11pt4's `Muladd!=1 && Cnstnt!=DNOTST`
branch, and `generated/airline_constant`'s f2/f3 block now matches the oracle
bit-for-bit with both defects transcribed. SEATS is walled instead
(`seatpr.f:211-390`'s constant removal is unported). Closed alongside it:
**`x11{final=(ls)}`'s Part-E Facls re-adjustment** (x11pt3.f:1243-1249), which
had been a `not_ported` fatal — six lines, and they belong INSIDE the
weight-zero branch of the Part-E loop, not after it. Gated by the new
`generated/airline_constant` (b1/d10-d13/d16/sac/tac + the E family + f2/f3)
and `generated/airline_finals-ls` (measured oracle on-vs-off 6.0e-2 in e2).

## 19. x11pt4 increment 1 — the F2 SEASONALITY TEST BATTERY — CLOSED (bit-exact).

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

## 20. x11pt4 increment 2 — the PART-F SUMMARY MEASURES + the F3 QUALITY STATISTICS — CLOSED (bit-exact at the .udg's printed precision).

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

## 21. x11pt4 increment 3 — the PART-E TABLES — CLOSED (bit-exact), and x11pt4 is now fully ported.

 `x11pt4_etables` (x11pt4.f:162-319) in the same
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

## 22. The `seats{}` OPTION SURFACE — swept, and the sweep found real wrongness.

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

## 23. force{}'s NEGATIVE-VALUE CORRECTION + the slidingspans `ads` table — CLOSED (bit-exact), and the span-replay clobber it exposed is fixed.

 Three things,
each of which had to land before the next was measurable.
(1) **The `qmap2` negative-value correction (x11pt3.f:750-782)** was the last
x11 wall that last session's `transform{constant=}` port made REACHABLE — it
sits behind `Muladd!=1 && Cnstnt!=DNOTST`, and until the constant was parsed
that guard could never fire. Subtracting the constant back out can drive D11
to or below zero (x11pt3.f:632-634 floors it there when `Iyrt>0`), so the
FORCED series can go non-positive too; the oracle clamps those to zero and
re-prorates against the target with a **second qmap2 pass at Rol=0/Lamda=0.5**
— run even when the primary pass was Denton, and taking the CLAMPED Stci2 as
its input series, not Stci, so the correction compounds on the first pass.
(2) **`frcfac` (the `ffc` save table) moved out of the harness and into
x11pt3**, because x11pt3.f:841-850 has a second branch the harness could not
see: with `negfin` (values still <= 0 after the correction) the ratio is not
formed at all and those observations get **DNOTST (-999)**. `x13run_x11` had
been recomputing ffc as a plain Stci/Stci2, i.e. the else-branch only.
Nearly logged a CB here and it would have been wrong: the negfin branch takes
a hard QUOTIENT where the other goes through `divsub` (which SUBTRACTS when
Muladd!=0), which looks like an inconsistency — but log-additive has already
collapsed `muladd` 2->0 at the D12 antilog well before the force block, so
muladd is 0 or 1 here and the branch is guarded on `!=1`. Measured on a logadd
force spec: ffc is the quotient of the published D11/D11A on both branches.
**Measure before naming a Census bug.**
(3) **x11pt3.f:815-822's `Iyrt>0` sliding-spans store was never ported** —
only its `Iyrt==0` counterpart at :678-680 was — so every force+slidingspans
run stored NO SA span at all. With it, `mflag(Sa,3,...)` becomes reachable and
the **`ads` table** is now produced. NB ads is *conditional*, not merely
unported: ssap.f:209-210 emits it only when `Kfulsm==0 && (Lrndsa || Iyrt>0 ||
Itd==1)` or `Ihol==1`, which is why the two existing slidingspans specs
correctly ship no ads golden and their gate skips.
**The real find, though, was underneath all three.** A span replay is a full
x11pt1->x11pt3 pass, so it rewrites `/x11srs/`, `/adxser/`, `/x11fac/`,
`/x11ptr/` and `Begspn` **in place**. The oracle punches all of them during
the main pass, before sspdrv/revdrv run; this harness dumps at exit. Nothing
gated it because no spec had ever combined slidingspans{}/history{} with a
table the harness prints — the moment one did, saa/ffc came back as **84 rows
starting seven years late**, i.e. the last span's values under the last span's
dates. `run_x11.cpp` now save/restores that whole set around both span
drivers, exactly as it already did for `/lkhd/`. The new spec's d10-d13/d16
goldens exist specifically to pin the restore.
Gated by `extra/airline_force-constant-{regress,denton}` (d10-d13/d16 + saa +
the DNOTST-carrying ffc) and `extra/airline_slidingspans-force` (sfs/chs/ads +
saa/ffc + d10-d13/d16). The constant specs need a series that crosses zero, so
they run on **`tests/corpus/data/airline_zero.dat`** — airline shifted down by
150 (24 observations at or below zero) and lifted back by `constant=100`.

## 24. `x11regression{tdprior=}` under `x11{mode=logadd}` — CLOSED (bit-exact), and it was a SILENT wrong-numbers bug hiding behind a fatal that never fired.

The prior-TD divide is applied in two places (the pre-model estimation input in
`run_pre_model.cpp`, and the X-11 buffer in x11pt1), because the oracle runs
x11pt1 BEFORE arima. `run_pre_model` gated its half on `muladd == 0`, marking
logadd "deferred with the x11pt1 guard" — but **x11pt1.f:52 collapses Muladd
2->0 for the whole prior-adjustment stage**, so x11pt1's guard tests the
COLLAPSED value and never fired for logadd either. The spec fell through both
and came back `OUTCOME: OK` with the prior TD missing from B1 and d10-d13
entirely — off by exactly a factor of `a4` (~2e-2..3.8e-2). **`a4` was
bit-exact the whole time**, which is what made it invisible: the factor was
computed correctly and then simply never applied. The fix is to drop the mode
gate — editor.f:1507 allows the weights for multiplicative OR log-additive and
both take the identical divide. Closed alongside it: **editor.f:1494-1530's
parse-time validation**, which had never been ported — additive /
pseudo-additive prior-TD weights and negative weights under a multiplicative
adjustment are *rejected outright* by the oracle, not merely unported, so the
x11pt1 `muladd != 0` fatal was standing in for an error the parser should have
raised. Gated by `extra/airline_x11regression-tdprior-logadd` (a4 + b1 +
d10-d13, in `test_x11_tdprior_tables.py`) and
`extra/airline_x11regression-tdprior-add` (the rejection itself, via
`test_m1_parse`). **Generalizable**: when a feature is applied in two places
and one of them is guarded "deferred, the other guard will catch it", check
that the other guard tests the same value at the same point — here one read
Muladd before the collapse and the other after.

## 25. `x11pt2 tdlom Adjtd==0` — the unreachability proof was WRONG, and the reason is worth keeping.

 It is reachable by `regression{noapply=(td)}`, which sets
`Adjtd = -1`; the branch (tdlom.f:44-59) is now ported and gated by
`generated/airline_noapply-*`. The four routes below really are closed, and
the analysis was invalid anyway: **`noapply=` was parsed and discarded**, so no
spec could set `Adjtd < 0` and the reasoning silently ran over a smaller graph
than the oracle's. *A reachability argument is only valid over the options the
parser honours.* The superseded analysis: it needs Adjtd
cleared while `Nflwtd>0` and `Priadj>1` still hold, and the oracle rejects or
bails out of every route: chkadj.f:209 (non-log/non-identity transform) is
closed because the automatic lom/leap prior only exists under `td`+log and an
explicit `transform{adjust=}` alongside `regression{variables=(td)}` is
rejected; editor.f:2277 (`.not.Lmodel`) is closed because `regression{}` with
no `arima{}` is rejected; x11ari.f:110 (constant series) makes the oracle
refuse the run and write no tables, so there is nothing to gate; xrgdrv.f:80
runs at `Ixreg==2`, which the tdlom call site already excludes. The reachability
analysis is recorded at the fatal in `x11parts.cpp` so it is not re-derived.
(Found in passing: the harness throws `basic_string::_M_create` on a constant
series where the oracle cleanly refuses — a robustness gap, not a parity one,
since the oracle produces no output to compare against.)

## 26. `history{}`'s last two gaps — `Fixper` and `Indrev` — CLOSED, plus the `fixmdl` and default-`start` paths they dragged in. `history{}` is now complete except for its outlier / alternate-target surface.

 All four were
measured on the oracle before any porting, and all four were the
silent-wrongness class (`OUTCOME: OK`, wrong numbers).
- **`Fixper` — the `series{modelspan=(,0.per)}` convention.** rev.cmn calls it
  "the period every year for which the model will be estimated"; the oracle
  implements it not by freezing coefficients but by capping each span's
  `Endmdl` at the last occurrence of that period (revdrv.f:481-489), so the
  ESTIMATION WINDOW only advances once a year while X-11 still sees the whole
  span. Measured oracle on-vs-off: **sae 2.09e-3, tre 3.73e-3, sfe 1.09e-2**;
  the engine was returning exactly the "off" numbers. Ported through a new
  `nend_mdl` argument on `run_x11_span` that replays arima.f:142-152's narrow
  and arima.f:1145 (setspn.f)'s restore-before-forecasting — the same dance
  `run_pre_model` does for the main run, restricted to the nbeg==0 case
  (revdrv never moves Begmdl). **The same block's OTHER branch was equally
  wrong and is ported with it:** with a plain `modelspan` END, revdrv.f:490-496
  caps every span at `mdl2` = the MAIN run's Endmdl (measured **sae 7.17e-4,
  tre 1.97e-3**). `mdl2` has to be captured in `run_x11.cpp` next to
  `begspn_full`, because `run_x11_span` overwrites `ctx.arima.endmdl` with each
  span's own end and slidingspans runs first. setrvp.f:64-71's Beglup shift is
  ported too, but note **it is observationally inert here**: it only adds
  pre-Begrev spans, and revdrv.f:432-453 processes just two of them (Beglup
  itself, with Lx11 OFF, and Frstsa) while every span in this port is restored
  from the main run's snapshot — so they leave no trace and the loop skips them.
- **`Indrev` — the INDIRECT composite revision history.** Real output, not
  print surface: the oracle writes `iar`/`iae` (Ind_SA_revision, Conc/Final
  Ind_SA) plus a `historyindsa: yes|no` savelog line, and iae IS the
  comptype/compwt-weighted aggregation of the components' own concurrent and
  final SA (verified equal to their sum at 4.3e-15 on the oracle). Ported:
  gtrvst.f:361-435's derivation + the three ways it gets switched off (a
  component without a sadj history, mismatched start dates, no explicit start
  at all), putrev.f:25-30's fold into `/revdta/ Cncisa/Finisa`, getrev.f:117's
  `Nrcomp` count and revchk.f:547-551's `Ncomp!=Nrcomp` drop (ported WITH its
  guard — it is the ELSE of `IF(Kfulsm.ge.1)`), and revdrv.f:838-846's
  Tbltyp=3 print. Cncisa/Finisa/Indrev/Indrvs/Nrcomp are COMMONs that outlive
  a spec, so `tools/x13run_composite.cpp` carries them exactly like `/mq11/`
  and `/agreg/`. **The one structural fix this forced:** `run_x11.cpp` ran the
  span drivers BEFORE its composite tail, but x12run.f calls sspdrv/revdrv
  AFTER x11ari — which includes agr2/agr3. Order matters in both directions:
  `agr2_component` reads the D-table buffers a span replay overwrites, and the
  total's Iagr only reaches 5 (revdrv's test for "print the indirect table")
  inside agr2. The two blocks are now in x12run.f order, with run_spectrum
  still ahead of agr3 (the oracle's direct spcdrv runs before the Iagr==3
  branch). Gated by `census-examples/composite-history/` (positive: 3 specs x
  sar/sae, the total's iar/iae, `historyindsa yes`, and an assertion that iae
  equals the component sum on BOTH sides) and
  `census-examples/composite-history-mismatch/` (negative: a component whose
  history starts a year later -> `no`, no iar/iae, components unaffected).
  Measured: iae inherits the components' per-span floor (6.5e-6 rel, median
  3.6e-9); the total's own sar/sae are bit-exact because a `composite{}` spec
  carries no model to re-estimate.
- **`history{fixmdl=yes}` (`Revfix`) — also parsed-and-dropped, also fixed, and
  it is the ONE history configuration that gates BIT-EXACT.** Measured oracle
  on-vs-off **sae 3.23e-3, tre 3.77e-3, sfe 6.48e-3**. revdrv.f:250-262 fixes
  Arimaf/Regfx/Iregfx — but the fix only sticks because revdrv.f:381 then
  re-runs `ssprep`, and every span's `restor` reinstates the flags FROM that
  snapshot. Fixing only the live `ctx.model.arimaf` would be undone by the
  first `restor_span`; `ctx.ssprep.fxa` has to be set too (the identical trap
  `ssmdl_fix_model` documents for slidingspans). With nothing re-optimizing,
  all ten tables land at ~5e-15 instead of the 1e-5 estimation floor — so
  `airline_history-fixmdl` and `-fixper-fixmdl` are gated at 1e-12/1e-11 via
  `RTOL_LEVEL_BY_SPEC`/`ATOL_REV_BY_SPEC` rather than the shared tolerance.
- **`history{}` with NO `start=` used to FATAL** ("Number of observations after
  differencing (-12) < minimum series length") because revchk.f:569-608's
  default was unported and Rvstrt stayed (0,0). Now ported:
  `strtyr(-1:5) = {6,5,6,8,12,18,6}` indexed by Ltmax (the longest seasonal MA,
  via the already-ported `sfmax_span`) — 6 years for the corpus filters, so the
  default start is 1955.01 on airline and matches the explicit spec exactly.
  **Parse-order dependency worth knowing:** gtrvst reads Rvstrt at PARSE time,
  before this default exists, so on a composite an unspecified start really
  does disable the indirect analysis (gtrvst.f:419-427) — the oracle and the
  engine both say `historyindsa: no` there.
- **CB-21** claimed: `restor.f:67` restores `Arimaf` with the bound `PB` (80)
  instead of `PARIMA` (133) — a copy-paste of the neighbouring regression
  bound. Under-copies rather than over-reads, and no admissible model has more
  than 80 ARMA lag slots, so it is unreachable; not reproduced (`restor_span`
  uses PARIMA, which is bit-equivalent here).
- **Still open in `history{}`, all parsed and still SILENT** (measured as a
  surface, not individually): `outlier=`/`outlierwin=` (Otlrev/Otlwin, the
  per-span outlier re-identification, which is also what revdrv's `addreg`
  flag gates), `refresh=` (Lrfrsh), `fixreg=` (Rvfxrg), `fixx11reg=` (Revfxx),
  `sadjlags=`/`trendlags=`/`target=` (the alternate revision targets),
  `endtable=` (Irev==2), `additivesa=` (Rvdiff, additive mode only).
  `transparent=` is print surface. Also found but out of scope: a
  `comptype=mult` FIRST component makes the composite aggregate all zeros and
  the oracle rejects the total outright ("All data values ... are equal to
  zero") while this engine happily decomposes it and emits NaN — an x11/
  composite input-validation gap, not a history one.

## 27. `history{}`'s option surface — MEASURED, and `fixreg=` CLOSED.

 All 11
remaining parsed-but-silent flags were run through the ORACLE on-vs-off before
any porting (`tools/history_options_scouting.md` has the table). 10 move real
numbers; `refresh=` is **structurally inert with a mechanism** —
`revdrv.f:528` restores unconditionally at every span head, so `revdrv.f:746`'s
`IF(Lrfrsh)CALL restor` is overwritten before anything reads it. Do not port
it. **A null measured under the wrong preconditions is not a null**: round 1
returned 0.000e+00 for four flags and three of them were structurally
incapable of moving anything in the spec tested — `Cnctar` is a no-op without
`sadjlags`/`trendlags` (prtrev.f:115 gates on `Cnctar .or. i2.eq.0`), `Rvxotl`
needs `Otlxrg` i.e. `x11regression{critical=}`, and `Otlrev` needs outliers
already carried as regressors. **`OTLDIC` is `'keepremoveauto'`, so the
DEFAULT `outlier=` is KEEP** even though gtrvst's own error message lists
remove first — measured, the engine's default behaviour is therefore already
correct and only the non-default values are silent. Two things landed:
- **`fixreg=` (`Rvfxrg`) — ported** (`rvfixd.f`, revdrv.f:112-134's group
  decode, revchk.f:230-287's rule dropping the td model history when TD is
  held). Engine now moves by exactly the oracle's measured delta. **Not yet
  gated** — no corpus spec or golden.
- **`arima.f:283`'s `rmfix(...,1)` had NO call site in the port**, so FIXED
  regression coefficients were ignored by the estimator outright. rmfix/addfix
  were ported long ago but only ever reached from automdl with `fxindx=2`.
  Wired into `run_x11_span` with arima.f:909-914's restore. The same gap on the
  MAIN estimation path is now **CLOSED** — see the FIXED/INITIAL COEFFICIENTS
  entry below.
- **The real find: `Priadj` was not restored between span replays.** x11pt2's
  tdlom NEGATES Priadj after folding the length-of-month/leap-year prior into
  the model TD factor (so nothing removes it twice); `ssprep.f:56-62` saves the
  pre-tdlom value in `Pri2` and `restor.f:55` puts it back before every span.
  This port did neither, so every replay saw `Priadj<=0`, skipped the fold, and
  built a `Factd` with no prior in it — **every February of the span's
  D11/D16 off by exactly the leap-year factor** (0.9912 non-leap / 1.0265 leap;
  sae 2.7e-2 against a 1e-5 floor) behind `OUTCOME: OK`. Affects
  slidingspans{} AND history{}, not just the new flag. Nothing gated it because
  no corpus spec combined `regression{variables=(td)}` under log with either.
  ssprep's asymmetry is ported verbatim: once tdlom HAS run the flow reverses
  and the live value is restored FROM the snapshot.
- **A SECOND span-replay bug, found by the gates added for the first:**
  `ssprep.f:81-95` / `restor.f:66-70` — the REGRESSION half of the
  snapshot/restore pair (`B`, `Regfx`, `Iregfx`) — had been skipped as
  "Nb==0", true of every span-replay spec until one carried a `regression{}`
  group. Without it each span starts its regression from whatever the PREVIOUS
  span converged to rather than from the main run (measured 8.4e-3 in sfs).
  Ported with `ssmdl.f:345-350`'s matching Ssinit==1 fixing. **That then broke
  `history{fixreg}`** until run_history mirrored its flags into the ssprep
  snapshot — the identical trap already documented for fixmdl: once
  restor_span restores Regfx/Iregfx, setting only the LIVE model before the
  loop is undone by the first span.
- **Open, measured, deliberately not hidden:** `airline_slidingspans-td`'s
  `chs` (per-span SA change table). sfs is bit-exact; chs is not. At 1956.Feb
  (a leap February) the golden's span columns are
  -0.264/+0.284/+0.680/+1.396 and the engine gives
  +3.299/+3.865/+0.680/+5.018 — the THIRD span agrees bit-for-bit and the
  others do not. A uniformly missing prior would move every span, so this is a
  per-span PHASE problem in how the prior series (anchored at the MAIN run's
  Begadj) is indexed for a span starting at a different date, NOT a repeat of
  the Priadj bug. Golden blessed and committed; the gate skips that one cell
  with the measurement written at the skip.
- **`endtable=` — CLOSED by GATING it; the engine was already right.** The
  scouting table's whole method was ORACLE on-vs-off, which proves a flag
  *matters* and says nothing about whether the engine honours it — and nobody
  had run engine-vs-oracle. `gt_history` case 6 already parses it into
  `rv.rvend` and `run_history` already derives `endsa`/`endtbl`/`revnum`; with
  `endtable=1957.dec` both sides emit 36 rows instead of 71 and every retained
  value agrees at the usual per-span floor. **Before porting anything off that
  table, measure engine-vs-oracle too.** The two neighbouring `revchk.f` pieces
  need no port either: `:616-624`'s `.not.Revsa` warning-and-reset is
  observationally INERT (nothing surviving that branch reads `Endtbl` — the
  model histories run over `Begrev..Endrev` and the forecast history keys on
  `Revptr+Rfctlg`; measured 72/71 rows agreeing either way), and `:629`'s
  `IF(Irev.eq.2)` override is DEAD CODE — `Irev` is only ever 0, 1
  (`gtrvst.f:349`) or 4 (`revdrv.f:387`). Gated by
  `extra/airline_history-endtable` (all ten revision tables).
- **`fixx11reg=` opened something much bigger: the whole
  `x11regression{} + history{}` family is SILENTLY WRONG in its DEFAULT
  configuration.** Zero corpus specs combine `x11regression{}` with a
  span-replay driver (checked), so nothing ever gated it — the same
  two-features-at-once blind spot as the Priadj restore and the ssprep
  regression half. Measured on airline + `x11regression{variables=(td)}` +
  `history{}` at the gate's own tolerances (5e-3 abs revisions / 1e-5 rel
  levels): **with a regARIMA model the engine silently IS `fixx11reg=yes`** —
  it matches the oracle's FIXED run at the floor (sar 1.3e-3, sae 1.2e-5) and
  misses the DEFAULT by the full flag delta (8.2e-1 / 8.3e-3) — because
  `revdrv.f:530-532` demotes `Ixreg` 3→1/2 at every span head so each span
  re-runs the x11reg irregular OLS, and this port leaves it at 3 (= "already
  removed as a prior by xrgdrv", so x11pt2 skips `x11mdl`). **Model-free, BOTH
  branches are wrong** (7.9e-1 / 1.4e-2 default, 1.2e+0 / 1.5e-2 fixed) — there
  `Ixreg` is never promoted, the inline `x11mdl_td` IS running per span, and it
  reproduces neither oracle branch: a second, independent defect in the same
  family. **Adding the demote is NOT the fix — tried and measured, it makes the
  modelled family worse and breaks its previously floor-accurate fixed branch**,
  because `Ixreg==2` in the oracle means "run the transparent `xrgdrv` pass" and
  this port HOISTS xrgdrv into `run_pre_model` rather than reaching it from
  x11pt2, so a demoted span takes the `Ixreg==1` inline route instead.

## 28. PER-SPAN `xrgdrv` — CLOSED, and with it `history{fixx11reg=}`.

 The demote
above landed together with `x11ari.f:88-95`'s transparent pass, run against
each span's own pointers (`xrgdrv(ctx, span_mode=true)`: no `setxpt`, no buffer
refill, `xrgdrv.f:143-146`'s in-place pointer nudge instead, undone at
`:167-178`). DEFAULT with a model went **sar 8.22e-1 → 6.8e-4, sae
8.27e-3 → 6.7e-6**, i.e. the ordinary per-span floor; `fixx11reg=yes` gates at
1.3e-3/1.2e-5 with a model and **bit-exact (5e-15) model-free**. Four things
worth knowing:
(1) **The estimation input, not just the factor.** `arima.f:156-157` copies
`Sto` from `Pos1ob` AFTER x11pt1 divided out THIS span's Faccal, and transforms
that. Every other path can shortcut it with the caller's pre-transformed
series; this one cannot, because the span's Faccal differs from the main run's.
Rebuilt the oracle's way, scoped to the xrgdrv path so gated paths stay
byte-identical.
(2) **The state leak was the whole residual, and it is the `Lterm`/`Ksdev`/
`Priadj` class again.** The transparent pass is a full x11pt1/x11pt2 and
resolves `Lmsr`, `Kersa`, `Lstabl`, `L3x5`, `Nterm` in place. On the main path
`run_x11`'s editor block re-derives all of them after xrgdrv returns; a span
has no editor block. Without handing them back, the LAST span — which covers
the whole series and produced a Faccal **verified identical** to the main
run's, with a **verified identical** estimation input — was still 1.2e-4 off.
A per-span pass must restore everything the main path's editor would re-derive,
not merely what `restor` restores.
(3) **`fixx11reg=` fixes the x11reg STORE (`Irgxfx`/`Regfxx`), not the working
model** — each span's `loadxr(F)` copies it in and `x11mdl.f:390-395/459-467`'s
`Iregfx>=2` rmfix/addfix (newly ported into `x11reg.cpp`) strikes every fixed
column. Nothing restores the store between spans, so unlike fixmdl/fixreg it
needs no ssprep mirror.
(4) **`fixmdl=yes` is INERT when `x11regression{}` is present, faithfully.**
`revdrv.f:309-350` ends with `CALL restor(Lmodel,F,F)`, which reinstates
`Arimaf`/`Regfx`/`Iregfx` from the ssprep snapshot before the loop starts, and
`Revfix` only ever set the LIVE copy (the re-snapshot at `revdrv.f:380` is
commented out in the Fortran). Oracle `fixmdl=yes` and default are
BYTE-IDENTICAL here; without `x11regression{}` the same flag moves sae 3.23e-3.
This port had to SUPPRESS its ssprep mirror — the thing that makes the fix
stick at all — to reproduce that.
**`slidingspans{}` is NOT the same fix.** `ssx11a.f:93-95` has the identical
demote, but measured: with a regARIMA model `sfs` is **bit-exact (4.7e-15) with
`Ixreg` left at 3** and goes to 4.1e+0 with the demote added — the oracle pays
it back inside `sspdrv` (`Ssinit`/`Ssxint`), unported. Left undemoted with the
measurement at the call site. Still separately wrong on that family and NOT
this seam: `chs` (5.5e+0, the `airline_slidingspans-td` per-span prior-phase
problem) and the whole model-free case (`sfs` 2.0e+2).
Gated by `extra/airline_history-x11reg{,-fixx11reg,-nomodel-fixx11reg,-fixmdl}`
in `test_history_tables.py`.

## 29. `history{}`'s HELD-BACK OUTLIERS (`rmotrv`/`chkorv`) — CLOSED, and it was a DEFAULT-path silent wrong-numbers bug that the scouting doc had explicitly measured as CORRECT.

 A span ending at date T must not know about an outlier
dated after T. `revdrv.f:302` deletes every outlier-type regressor past the
first revision date from the design before the loop, and `revdrv.f:589`
re-introduces each one when a span's MODEL span (`i - nend`, not `i`) reaches
it. **Neither is behind a flag** — `outlier=` only chooses whether the deleted
ones are SAVED for re-introduction (keep/auto) or dropped (remove). Ported in
`core/src/driver/rev_outlier.cpp`, including chkorv's singularity pass (several
outliers on one last observation are not jointly estimable, so its `opref`
table keeps one). Measured before/after, tolerances 5e-3 abs / 1e-5 rel:
`regression{variables=(ao1957.jan ls1958.jul)}` sar 9.77e-1 → 7.2e-4;
`outlier{critical=3.0}` at the DEFAULT `keep` sar 9.20e-1 → 4.3e-4;
`outlier=remove` (rmatot.f's delete arm) 1.12e+0 → 4.0e-4. Outliers dated
BEFORE the history start were already right and stay so (the negative control).
**The method failure is the point.** `tools/history_options_scouting.md` had
measured this default and called it correct at "sae 7.1e-6, the ordinary
per-span floor" — on a spec with `outlier{critical=3.5}`, where the airline
series' largest t is 3.48 and the oracle identifies NOTHING. The flag was
exercised over an empty set. That doc's own traps section already names this
("a null measured under the wrong preconditions is not a null") and it was
still walked into, because **a saturated precondition looks like a passing
gate, not like a zero delta** — when a feature's effect is conditional on a set
being non-empty, the probe has to assert the set is non-empty.
**The structural fix underneath:** `restor.f:50-64` restores the whole design
DICTIONARY (`Ngrp`/`Nb`/`Colttl`/`Colptr`/`Grp`/`Grpptr`/`Rgvrtp`/`Ncxy`/
`Nrxy`) and this port's `restor_span` restored only the coefficient half —
enough until now, because nothing had ever changed the regression STRUCTURE
between spans. Both halves of the ssprep/restor pair now carry it (verified
byte-identical on every existing path). Same trap as `fixmdl`/`fixreg`: a
structural change that is not mirrored into the ssprep snapshot is undone by
the first span's `restor`. **`outlier=auto` is now a clean FATAL rather than
silent** (measured sar 1.2e+0 if ignored): it needs each span to re-run the
automatic identification — `Ltstao`/`Ltstls` back on, `revdrv.f:517`'s
`Begtst = Endspn - Otlwin` window, rmatot's save-and-re-enter arm and the
per-span rmatot at `revdrv.f:723` — a sub-engine `run_x11_span` does not have.
`outlierwin=` is reachable only from it. Gated by
`extra/airline_history-outlier-{reg,pre,auto-keep,remove}`.
**`x11outlier=` (`Rvxotl`) landed alongside it — `revdrv.f:336-338` +
`:601-603`, the same machinery on the X11REGRESSION design via
`loadxr(false)`/`loadxr(true)` (nothing restores that store between spans, so
it sticks without an ssprep mirror, same as `fixx11reg`) — but the blocker
turned out to be in `x11regression{}`, not here.** Transcribing both blocks
moved the numbers by EXACTLY ZERO; instrumenting `ctx.xrgmdl` showed six
trading-day columns and no outliers, because the store can only carry
automatic outliers if `x11regression{critical=}` set `Otlxrg` — and **that
argument, and `sigma=`, were accepted by the parser and silently dropped**
(`x11reg.cpp` derived the critical value from the span length unconditionally
and hardcoded the 2.5-sigma `tdxtrm` limit, so `critical=3.0` identified the
DEFAULT's outlier set). A MAIN-run defect: d11 off 1.7e-4 relative on airline
with no `history{}` in the spec at all. Ported (`gtxreg.f:288-322` parse +
`editor.f:1729-1757`'s resolution, plus the `Sigxrg`/`Critxr` **DNOTST**
defaults from `gtinpt.f:457-458` that the port had left at the struct's
zero-init — `critical=0` is a MEANINGFUL value, "identify but derive the
threshold", so "not given" has to be distinguishable from it). The main run is
bit-exact after it and the history family falls out: `x11outlier=yes` (default)
sar 5.15e-1 → **4.73e-4**, model-free 5.30e-1 → **5.33e-15 (bit-exact)**.
`sigma=` is the OTHER branch of that same editor rule (an explicit `critical=`
leaves `Sigxrg` at 0 and skips `tdxtrm` entirely, so the two are alternatives
and each needs its own spec); measured separately after the fact — oracle
`sigma=2.0` vs the 2.5 default moves d11 1.6e-1 and the engine matches
bit-exactly.
**`x11outlier=no` is still wrong (7.48e-1) and is left OPEN with what is known
written down**: the engine gives the DEFAULT's delete-and-re-identify numbers,
the deletion path is the one that works, and on this corpus every x11reg
outlier predates the revision start so `rmotrv`/`chkorv` never fire on either
branch — the difference is in how the per-span `x11mdl` re-identifies against a
design that already carries AO columns. Gated by
`extra/airline_x11regression-critical` and
`extra/airline_history-x11outlier{,-nomodel}`.
**The generalizable bit:** three of the last four `history{}` options were
blocked by, or already fixed in, something that was not the option being
scouted. An oracle on-vs-off table tells you a flag matters; it cannot tell you
where the engine's disagreement lives.

## 30. `history{sadjlags= trendlags= target=}` — the ALTERNATE REVISION TARGETS — CLOSED (at the per-span floor).

 The biggest OUTPUT gap left in `history{}`:
whole columns absent, not values wrong. Each surviving lag adds one column per
table of its family — "the estimate `lag` periods AFTER the revision date" in
place of the full-data final one. Ported: `setrvp.f:26-40`'s `Endsa += mxrlag`
(the spans that make those estimates have to run; `Endtbl`/`Revnum` are fixed
before it, so the TABLE does not grow — only the loop and the DNOTST cutoff),
`revchk.f:1053-1110`'s `intsrt` + drop-what-does-not-fit + `Lr1y2y`,
`getrev.f:57-70`/`:86-99`'s per-span file-into-the-row-`lag`-periods-back, and
`prtrev.f:174-226`'s arithmetic — `Fin(0)-Fin(lag)` by default,
`Fin(lag)-Conc` under `target=concurrent` (`Cnctar`), each percented by its own
base on a level table, plus the DNOTST mask whose cutoff `Cnctar` moves one row
later. **Order is faithful and matters: `revchk` calls `setrvp` BEFORE it
validates**, so a lag about to be discarded still widens `Endsa`. One guard the
oracle gets for free: `revdrv.f:416` turns `Lx11` off past `Endsa` so getrev
never runs on the trailing spans, while this port adjusts every span to
`Endrev` — the target store is explicitly `i <= endsa`. **CB-22**: the extra
`(1yr-2yr)` column exists only on the SA / SA-change / indirect tables
(`revdrv.f:852/859` pass `Lr1y2y` to the two TREND calls as a literal `F`) even
though `revchk` derives the flag from BOTH lists, trend overwriting sadj — so a
`trendlags=` that is not a 1yr/2yr pair silently costs the SA table its column.
**How that was found is the generalizable part: the gate asserts COLUMN COUNTS
before values.** The engine emitted a 4th trend column against the golden's 3
while every value it did emit agreed — a gate that reads the first N columns of
a save file cannot see a wrong N, which is exactly what the pre-existing
`test_history_table` does (and why it kept passing). Gated by
`test_history_target_columns` over `extra/airline_history-sadjlags` (the
1yr/2yr pair on both families), `-sadjlags-conc` (`target=concurrent`) and
`-sadjlags-drop` (an UNSORTED list carrying one lag too long for the span, so
the sort, the drop and the widened-then-discarded `mxrlag` are all pinned).
- Still open, all measured as moving: `x11outlier=`
  (shares the `revdrv.f:309-350` block), `outlier=auto`/`outlierwin=` (now
  FATAL rather than silent), `additivesa=`. Suggested order is in the
  scouting doc — note it has now been wrong three times: it ranked `fixreg` as
  cheapest (it needed the rmfix seam), listed `endtable=` as unported (it was
  already correct), and its model-free `x11regression{}` row was measured on a
  spec that still carried `transform{function=log}` and is bit-exact without it.

## 31. FIXED / INITIAL COEFFICIENTS — `regression{b=}` and `arima{ar= ma= diff=}` — CLOSED (bit-exact), and both were the silent wrong-numbers class.

 Two
documented, ordinary user options whose values were **parsed and thrown away**:
`gt_regression` had no `argidx == 7` branch at all and `gt_arima` sent diff/ar/
ma to `consume_value(nullptr)`, so the model was estimated freely and the run
came back `OUTCOME: OK` with the wrong coefficients. Measured oracle
on-vs-off — `b=` fixed: 1.1e-2 d10/d11/d16, 2.3e-2 d12, 2.5e-2 d13; `ma=`
fixed: 6.2e-3..7.9e-3; `b=` under td+log: 1.2e-2..3.0e-2. What landed:
- **`gtinvl.f` / `gtrgvl.f`** (`readers_val.cpp`) — the list readers, each
  value optionally carrying a trailing `f`/`e` (`FIXDIC='fe'`, `FIXVAL=1`). A
  NULL element (bare comma) advances the cursor and writes NEITHER array: the
  Fortran's `Bvec(ielt)=PTONE` / `Arimap(ielt)=PTONE` writebacks are commented
  out, which is what makes `b=(,0.05f)` mean "leave column 1 alone".
- **`regfix.f`** (next to the already-ported `mdlfix.f` in `getmdl.cpp`) —
  derives `Iregfx` 0/1/2/3 from which coefficients have a VALUE (`B != DNOTST`)
  and which of those are FIXED. `arima.f:282/907` gate on `>= 2`.
- **`getreg.f:519-553`'s writeback**, which runs straight after the argument
  loop (Nb is only final once every `variables=` group is built) and **before**
  the user-column `adrgef` calls, because those read `B(idisp)/Regfx(idisp)`
  out of the slots it fills past Nb — that is how a user regressor gets an
  initial value. The C++ had been passing a hardcoded `0.0`/`false` there.
- **`arima.f:281-287` / `:907-914` in `run_pre_model`** — the rmfix/addfix
  strip-and-restore, previously commented "not reachable in this pre-model
  slice". Placement matters twice: rmfix sits ahead of the automd/aictest/
  rgarma branch (the Fortran does it before the branch), and addfix must
  precede the final `prlkhd` (arima.f:968), not follow it.
- **`adrgef.f:82-126`'s date-ordered outlier insertion**, which had been a
  `not_ported` fatal ("automatic outlier regression ordering (rdotlr.f)").
  `addfix` re-adds a fixed AO/LS *after* rmfix deleted its group, so the group
  no longer exists and adrgef takes the create path — where a program-supplied
  outlier is inserted by DATE (and at equal dates by OTLDIC type order), not
  appended. `rdotlr` was ported years ago; only this call site was missing.
- **Three gtinpt.f defaults that were never ported**, each of which the port
  read as its zero-init: `B = DNOTST` (:156 — regfix keys on exactly that, and
  a zero would read as a supplied coefficient of 0), `Iregfx=1`/`Imdlfx=1`
  (:269-270), and **`Convrg = T` (:267)**. That last one is the subtle one:
  rgarma only UPDATES Convrg behind `Lestim .and. Nestpm > 0` (rgarma.f:387),
  so with every ARMA parameter fixed nothing touches the flag and the DEFAULT
  is what the `.udg` reports — the engine was printing `converged: no` against
  the oracle's `yes`. (`Imdlfx >= 1` is also what `estimate{parms=fixed}`
  requires, so that argument was unusable before this too.)
**Ported asymmetry**: arima.f:281 calls regvar with a backcast count of 0, so
Fixfac is built over a design with no backcast rows — yet :283 still hands
rmfix the real `Nbcst`, shifting the subtraction by that many periods.
Transcribed as written. **Nearly logged a CB and it would have been wrong**:
the Leap Year splice (getreg.f:519-541) shifts `fixvec` up but never resets
the spliced slot, so that column inherits its neighbour's fix flag — except
the inherited flag always equals another column regfix is ANDing in anyway
(it cannot independently change `allfix`) and `rmlnvr` (gtinpt.f:1032) deletes
the column before rmfix could act on it. Not observable; not a bug. Gated by
six new `generated/` specs — `airline_regb-{fixed,mixed,initial,td-fixed,
td-seasonal-fixed}` and `airline_arima-fixedma` — through b1/d10-d13/d16, the
f2/f3 diagnostics, the bindings, and (for the two without outlier regressors)
`test_m3_estimate`, whose `_estimation_reproducible` exclusion for fixed
coefficients is now dropped. Three of those specs exist for a specific reason:
`-mixed` is the only one that exercises `Iregfx==2`'s PARTIAL strip and hence
adrgef's re-insertion beside a surviving group; `-td-seasonal-fixed` is the
only one that reaches the splice's SHIFT branch (with `variables=(td)` alone
Leap Year is the last column, `icol <= nbvec` is false, and the splice
degenerates to a write past the end); and `-initial` is the negative control
that pins the strip to `Iregfx>=2` rather than to "b= was given" — the oracle
moves 0.000e+00 there. Still open: `test_m3_estimate` skips
`airline_regb-{fixed,mixed}` because its discovery excludes any spec with an
`ao|ls|tc|so|rp|tls` regressor; that exclusion predates the outlier port and
is probably stale, but widening it would newly admit many existing corpus
specs and is its own investigation.

## 32. PRIORITY #2 STARTED: R and Python can now call the engine in-process.

Both wrapper trees were pure stubs (`py-pkg`'s `seasonal_adjust()` raised
`NotImplementedError`) because `core/` had no entry point anything but C++
could call. Deliberately NOT a package: one shared library (`x13c`) plus two
single-file loaders needing no build step or install on the caller's side.
See **`bindings/README.md`**.
- `core/include/x13/capi.h` — the flat C ABI. Opaque per-run handle; tables
  reached **by name** (the engine grows tables as the port advances, so
  enumerating them in the ABI would break it every release); caller-allocated
  buffers with a size-then-fill protocol (short buffer ⇒ return `-needed`,
  write nothing); no exception ever crosses the boundary.
- `core/src/api/x13_rabi.cpp` — a `.C()`-shaped shim (integer handle registry,
  out-parameters only) so **R needs no R headers**. `.Call` would have meant
  building against `<Rinternals.h>`.
- Covers both decomposition paths — X-11 (`b1`, `d10`-`d13`, `d16`, Part-E,
  force/x11reg outputs, F2/F3 as scalar diagnostics) and SEATS (`s10`-`s14`,
  `s16`, `s18`). `run_seats` used to compute its decomposition and throw it
  away; it now publishes onto the context so the ABI need not re-derive it.
- **THE FINDING WORTH REMEMBERING: a shared library inherits its HOST's
  floating-point mode, and hosts disagree.** Measured, same DLL, same spec,
  x87 control word: MinGW `.exe` `0x037f` and Rscript `0x037f` (PC=3, 64-bit
  extended) both bit-exact against the oracle, versus **CPython `0x027f`**
  (PC=2, 53-bit). MXCSR identical in all three. On well-conditioned specs the
  difference is ~1e-16 and invisible; the optimizer amplifies it on the
  near-non-invertible ones, and `unrate_sfshort-x11`'s d10 came back **8.2e-6**
  off — six orders past the 1e-6 floor that spec otherwise holds. Each run now
  pins PC=3 and restores the host's word on exit. **The trap:
  `_control87(_PC_64, _MCW_PC)` does NOT do this on x86-64** — the CRT ignores
  the precision mask (SSE has no such field), so the x87 register must be
  written directly with `fnstcw`/`fldcw`. `x13_host_fp_control()` is exported
  so the next host that disagrees is diagnosable rather than guesswork.
- Tested three ways: `tests/unit/test_capi.cpp` (ctest, the ABI *contract* —
  NULL handles, out-of-range indices, the short-buffer protocol, run
  independence, the FP-word restore — linked against the actual `.dll` an
  interpreter loads); `tests/parity/test_bindings.py` (the Python binding vs
  every corpus spec with a blessed golden, **keyed on the DATE the binding
  reports, not position** — a positional compare would pass even if every
  label were a year late, the exact bug class that bit `x13run_x11`'s own
  `dump()`); `bindings/r/test_x13c.R` (the R binding vs the same goldens plus
  the R surface — data.frame shapes, `ts()` conversion, use-after-close,
  double-close).

## 33. `slidingspans{}` / `history{}` under `seats{}` — MEASURED SILENTLY DROPPED, partially closed.

 The largest two-features-at-once blind spot left: ZERO
corpus specs combined SEATS with either span driver. Measured on airline
`(0 1 1)(0 1 1)`: the oracle writes `.sfs`/`.chs` (and `.sar`/`.sae`/`.trr`/
`.tre`, 73 rows) and the engine returned **`OUTCOME: OK` with no tables at
all**. Not a harness artifact — `x13_capi.cpp:347` dispatches
`wantSeats ? run_seats : run_x11` and the span drivers live inside `run_x11`,
so the R/Python bindings had the identical hole. The Fortran needs no new
driver: `sspdrv.f:2` and `revdrv.f:5` both take `Lseats` and hand it to
`x11ari`, where `:199` still runs x11pt2 (gated `(.not.Lcmpaq).or.Lx11`, true
for a non-composite SEATS run) and `:204-243` substitutes the SEATS chain for
x11pt3. Landed: `run_seats.hpp`'s `seats_restore_mean`/`seats_decompose`
split (so a span can run the decomposition), `lseats` as a run_x11_span
PARAMETER driving `seatdg.f:101-110`'s ssrit store on Seatsf/Seatsa,
`getsma.f`+`mdssln.f` (the Findley (2003) span length, replacing an
`issap = 0` wall), and run_seats' span-driver tail. **Scale note:** Seatsf is
a ratio after `seatad.f:33-35`'s `/100`, which is exactly what
`ctx.seats_seasonal_add` (= s10) already carries, and ssrit multiplies it
back. **The restore set is wider than run_x11's**: ctx.seats_*,
ctx.series.tsrs (each span's rgarma leaves its own residuals there) and
Nspobs, because `x13run_seats` re-derives the whole ESTBUR chain from ctx
AFTER the driver returns — a miss shows up as the LAST SPAN's decomposition
under the main run's dates.

## 34. `slidingspans{}` / `history{}` under `seats{}` — NOW CLOSED.

 Two things,
and the second is the one worth remembering.
- **The X-11 PRE-STAGE is shared, because the oracle has ONE adjustment
  entry.** `x11ari.f` is reached with Lx11 *or* Lseats and runs the editor's
  span/filter setup, setxpt, x11int, x11pt1 and — at `:199`, gated
  `(.not.Lcmpaq).or.Lx11`, true for any non-composite run either way —
  x11pt2, and only THEN branches (`:204-243` substitutes the SEATS chain for
  x11pt3). This port grew two drivers and `run_seats` skipped the whole
  pre-stage: invisible for the decomposition (SEATS reads `ctx.series.tsrs`
  and the fitted model, not the X-11 buffers) but it left the span GEOMETRY
  unset — `Length` (x11pt2), `Pos1ob` (setxpt), `Lyr` (editor.f:235) and the
  ssprep snapshot — which is exactly what `setssp`/`run_x11_span` read.
  Extracted verbatim into `core/src/driver/x11_prestage.{hpp,cpp}` and called
  from both drivers; the refactor is byte-neutral (3978/0 unchanged) and the
  SEATS corpus still gates at ~5e-15, i.e. running x11pt1/x11pt2 on the SEATS
  main path — as the oracle does — disturbs nothing.
- **The decisive fix: `Tsrs` is NOT refilled when the model is held fixed, and
  the oracle does not care because it never uses Tsrs for SEATS.**
  `/csrs/ Tsrs` has exactly two writers, both `resid` calls inside rgarma
  (`rgarma.f:372/436`) and both behind `lnxstp`/`Convrg`, i.e. behind
  `Nestpm > 0` (`:347`). A sliding-spans replay holds the model FIXED, so
  neither fires and Tsrs still holds the MAIN run's linearized series. The
  oracle hands SEATS `Orixs` instead (`arima.f:1337-1341`); this port
  pre-linearizes into tsrs, equal on the main run and bit-exact there, so a
  span has to rebuild it — otherwise **every span decomposes the full series
  and reports ~the main run's own factors** (measured 6.2e-3 off, vs a 5e-15
  target). Rebuilt in the TRANSFORMED scale from `orix` minus the regeff
  effect arrays, which is where `adjreg.f:50-56` works — it forms `orixa`
  that way and only inverse-transforms at `:65`. **Two traps here**: `Orixs`
  itself is `orixmv`, the ORIGINAL-scale, *missing-value-only* adjusted series
  (the oracle's SEATS linearizes internally from the PATD/PAEAST/PAOUTR/
  PAOUIR/PAOUS factor arrays `ansub9.f:1395-1406` hands it), so taking that
  buffer directly overflows a log-domain estbur (measured: seasonal factors of
  6e24); and `adjreg` inverse-transforms `orix` IN PLACE, so the
  reconstruction must run BEFORE the adjreg call, not after (measured: 1e-38).
- `history{}` then fell out for free: the per-span SEATS components are
  published into `ctx.x11srs.sts/stci/stc`, which is where `run_history`'s
  inlined getrev already reads them — `seatdg.f:148-181` passes Seatsf/Seatsa/
  Seattr to the SAME getrev x11pt3 feeds Sts/Stci/Stc, and the scales already
  agree (Seatsf is the `/100` ratio, which is what getrev's `Muladd!=1` ×100
  expects). Clobbering is safe: x11pt3 never runs on that path and both
  callers save/restore `/x11srs/` around the loop.
- **What the tolerances say:** slidingspans is bit-exact (sfs 4.6e-14, chs
  4.7e-12); history sits one amplification above the X-11 per-span floor
  (sae 1.74e-5, tre 1.21e-5, both on the two SHORTEST spans, median ~1e-8;
  sar/trr inside the shared 5e-3 absolute bound) because a SEATS span feeds
  the re-estimated coefficients through the canonical decomposition rather
  than into filter weights. Gated by `extra/airline_seats-{slidingspans,
  history}`; both gates now pick the harness from the spec (`_binary_for`)
  instead of hardcoding `x13run_x11`.

## 35. A 7-way Codex review pass over the engine — what it found, and what it didn't.

 Reported clean: the SEATS core (roots/poly/canonical denoms/estbur/
every sign hand-off), the optimizer + automdl (the Census-MODIFIED lmdif's
control flow, AICC, the amdid/tdaic/easaic tie-break rules), and the X-11
arithmetic (Muladd 2->0 collapse ordering, both divsub arms, Part-E constant
round-trip order). Real finds, all verified against the Fortran before acting:
- **CB-23** (`chkorv.f:54-58`) — the span-outlier end-date guard can never
  fire, because the second disjunct's type test groups as
  `(t/=RP and t/=TLS) or t/=QI or t/=QD`, TRUE for every type, collapsing the
  whole condition to `begotl<=Endrev`. **This had been mis-ported as the
  INTENDED ternary**: the comment named the quirk correctly and the code then
  "improved" the Fortran. A comment that documents a Census bug is not the
  same as code that reproduces it — check that the code below it agrees.
- **`x11pt1.f:229-230`'s prior-TD ENTRY condition** — the guard tested only
  `Khol>=2` and so could not distinguish "unported branch" from "branch the
  oracle declines to enter". With the classic X-11 Easter on and no
  x11-regression prior calendar the oracle SKIPS the block and adjusts
  without a prior TD; the engine FATALED on a spec the oracle runs to
  completion (measured: 144 d10 rows vs none). Now gated bit-exact (4.8e-15)
  by `extra/airline_x11regression-tdprior-x11easter`, which ships **no `a4`
  golden by design** — pritd never runs — so the tdprior gate's case filter
  keys on d10 rather than a4.
- Three `x11pt3` getrev walls omitted their `Lrvsf` / `Lrvsa||Lrvch`
  disjuncts. Tightened to match, but **unobservable**: `Irev` only ever takes
  0 or 1 in this port (`readers_spec.cpp:2271` is the sole assignment;
  `revdrv.f:387`'s `Irev=4` has no counterpart because run_history inlines
  getrev's arithmetic instead of reaching it through x11pt3).
- CB-22 gained a third face: `trendlags=` with no `sadjlags=` gives
  `Ntarsa==0` + `Lr1y2y==T`, and prtrev sizes the SA table to one column its
  own `DO i2=0,Ntargt` loop then never assigns — an uninitialised-storage
  column. The port omits it; that is now documented at the line as a
  deliberate deviation rather than reading as a transcription.
- **The C/R ABI crash class** (new code, no Fortran counterpart, so nothing in
  the parity contract covered it): a use-after-free where the R shim's
  `lookup()` released the registry mutex before the caller dereferenced the
  handle — the string accessors are the subtle half, since the returned
  `const char*` aliases a run-owned std::string, so the COPY has to happen
  under the lock too; and exceptions crossing the ABI, where `runSpec`'s own
  no-throw guarantee did not cover its std::string PARAMETERS constructed at
  the call site, `x13_run_spec_file` did its path/substr/concat work outside
  its single try, and no R `X13_CAPI` entry point had a handler at all.
- Stale doc corrected: `adqtst.cpp` is NOT dead code — `automd.cpp` calls all
  five routines. Both offending docs (`tools/TEST_COVERAGE.md:16`,
  `tools/FABLE_REVIEW.md:69`) now carry the correction in place.
- **Harness weaknesses — now CLOSED.** All five were verified before being
  changed, and each was measured to be free (no gate's result moved), which
  is the point: they were latent, waiting for the NEXT regression.
  - `run_parity.py` reported **PASS having compared nothing**. `CppEngine.run`
    raises `NotImplementedError` for every spec and the handler recorded a
    "skip", while `ParitySummary.failed` only counted fail/error — so
    `--engine cpp` skipped the whole corpus and exited 0. `make_engine` now
    rejects `cpp` outright (the C++ engine is driven by the pytest gates, not
    this script), and `failed` is additionally true when NO outcome is a
    pass-or-blessed, or when the case list is empty — a `--filter` matching
    nothing used to be a PASS too. Both verified to exit 1; blessing
    unaffected.
  - `compare.py`'s `numbers_close` returned **True for NaN vs NaN**. The
    oracle emits no NaN in valid output, so a NaN on either side is a defect,
    and the rule hid both at once. Confirmed dead before removing it: every
    one of the ~400 goldens matching /nan/i matches "Hannan" (Hannan-Quinn),
    not a numeric token. (x11pt4's Pbar/Q came back NaN once already, from an
    all-zero Sprior — that is the case this protects.)
  - Three gates compared date sets in **one direction only** (`missing`, never
    `extra`), so an engine emitting rows the oracle does not — a span loop
    past Endtbl, an off-by-one revision range — passed while looking like a
    full match: `test_x11_tables`, `test_history_tables` (five call sites,
    now one `_assert_same_dates` helper) and `test_seats_tables`' `.mdc` key
    families (where the failure mode is a component polynomial one degree too
    long). **Mutation-tested**: dropping one row from a `sar` golden now fails
    with `extra ['195712']`, and passed before the change.
    NB the COLUMN truncation in the history gates is deliberate and stays —
    both sides slice to the same leading `ncol`, and the alternate-target
    columns are gated by `test_history_target_columns`, which asserts the
    count first.
  - `test_bindings` **skipped** when the engine declined a run. Its cases are
    selected because the ORACLE produced a golden, so declining one is a
    regression; now an assert. No case reached that branch, so it was free.

## 36. `check{}` -- the regARIMA RESIDUAL DIAGNOSTICS -- CLOSED (line-exact), and the whole spec was parsed-and-dropped.

 `gt_check` routed every argument
through `gt_generic`, so `maxlag`/`acflimit`/`qtype`/`qlimit` were consumed
and thrown away -- and with them the entire front that reads them, which had
no C++ at all. Ported into `core/src/diag/checkres.{hpp,cpp}`: `acf.f`
(sample ACF + Bartlett standard errors + the Ljung-Box and Box-Pierce Q
sequences), `pacf.f` (Yule-Walker partial ACF, which OVERWRITES its `r`
argument), `acfdgn.f` (which lags are significant, at `acflimit`/`qlimit`),
`nrmtst.f` + `intrpp.f` + `nrmtst.var` (skewness / Geary's a / kurtosis, each
against a one-percent point interpolated from its own table), the
Durbin-Watson statistic inlined at `arima.f:1075-1090`, and the
Friedman/Kendall seasonality test (`ansub11.f:1303`) at `arima.f:1092-1101`.
**Why it reaches 275 corpus specs with ZERO new goldens blessed:** no spec
needs a `check{}` spec for any of it -- `editor.f:909-910` sets
`Mxcklg = 2*Sp` on ANY model run with `Lsumm > 0`, i.e. the `-s` flag every
golden here was blessed with, so the oracle emits the block regardless. 287
of the 331 `.udg` goldens already carried it. Gated by
`tests/parity/test_check_diagnostics.py` (275 specs), and the gate is
**line-exact against the golden text** on 271 of them -- the emit goes
through the port's own `fwrite_fmt` with each line's actual Fortran format,
not printf. Five things worth knowing:
(1) **Fortran FIELD OVERFLOW is part of the contract.** A partial
autocorrelation with |t| = 10.169 does not fit acfdgn's `f7.4`, so the oracle
prints `*******` -- `generated/unrate_sar-seats` `sigpacf$24`. Going through
`fwrite_fmt` reproduces it; a printf would have printed the number.
(2) **Three Mxcklg defaults, applied in three different places, and the LAST
one wins only if the earlier two left it 0**: `getchk.f:63-67` (check{}
present -> 10 or 2*Sp, BEFORE its own argument loop, so an explicit
`maxlag=` overrides it), `gtinpt.f:1169` (Lseats and still 0 -> 3*Sp), then
`editor.f:909` (Lmodel and Lsumm>0 and still 0 -> 2*Sp). The last needs
Lmodel, so it lives on the model path in `run_pre_model`, not in the parser.
(3) **`Qcheck`'s default is `PT5 = 0.05D0`** (`gtinpt.f:21`) -- a
probability, despite the name. Exactly the trap that already bit
x11regression's `Cvxalf`; the parameter name means nothing.
(4) **The acfdgn seasonal-ACF udg block (`acf$NN`, `:79-91`) is DEAD CODE.**
Its loop counter `i` is initialised only inside the *log-file* block above
it, so when that block is off `i` is whatever the preceding `DO i=1,Nlagbl`
left (Nlagbl+1); `i=i+1` then makes the first lag `Sp*(Nlagbl+2)`, far past
Mxlag. Even with the log block ON, `i` lands past the last seasonal lag. No
golden in the corpus contains a single `acf$NN` line. Not reproduced (there
is nothing to reproduce) but recorded as **CB-24**.
(5) **Tolerance is byte-exact with a measured, documented fallback**, not a
blanket bound: `max(5e-4 abs, 5e-5 rel)` on the statistics only (integers,
significance markers and lag lists stay exact). Four specs need it, all
auto-selecting their model, and both places it bites are RATIOS that resolve
what the coefficients themselves round away -- `durbinwatson` (E15.8, eight
digits, 4.7e-6..1.3e-5) and the `sigacf`/`sigpacf` t-column, `(p)acf/se`,
where `airline_seats` `sigpacf$19` is -1.6001 vs -1.6000 with both other
columns identical. That t is what `acflimit` tests for list MEMBERSHIP, so a
threshold-straddling lag could in principle differ; it does not on this
corpus, and the gate asserts key sets in BOTH directions so it would fail
loudly rather than silently.
**The gate also surfaced a real, previously ungated defect elsewhere:**
`generated/usdeaths_automdl` had no test covering it at all, and automd
selects `(1 0 1)(0 1 1)` there against the oracle's `(0 1 1)(0 1 1)` -- an
iddiff regular-differencing (d=0 vs d=1) discrepancy that only NSA data
reaches, visible as `nefobs` 60 vs 59 before any statistic differs. Already
known at `test_m4_iddiff.py`'s `_AUTOMD_EST_CASES` omission and in
`tools/automdl_scouting.md`; skipped here with the measurement written at
the skip rather than absorbed into a tolerance.

## 37. The rest of the ESTIMATION savelog block -- outlier counts, ARMA roots, the ARMA coefficient table -- CLOSED (byte-exact).

 Same class as `check{}`
above and closed the same way: `.udg` canaries the oracle writes on any model
run, with no C++ at all and no gate that could see their absence. In
`core/src/diag/estdgn.{hpp,cpp}`, emitted from `x13run_m3` through
`fwrite_fmt` with the oracle's own formats, and folded into the same
`test_check_diagnostics.py` gate -- still **zero new goldens** (287 of 331
carry the outlier counts, 268 the MA rows, 260 the roots, 41 the AR rows).
- `savotl.f:79-152` -- `outlier.ao/.ls/.tc/.so/.rp/.tls/.user/.total` and
  `autoout`. Two asymmetries transcribed rather than tidied: `iall` is
  incremented INSIDE each type test rather than summed from the others, and
  the automatic tally counts a DIFFERENT set (its `PRGTAS` seasonal-outlier
  arm is commented out in the Fortran, so a seasonal outlier is never counted
  as automatically identified).
- `prtrts.f` -- `roots.<filter>.<period>.<NN>`, four TAB-separated E22.15
  values per root. **The key is built from the operator title split on its
  LAST blank, and the two halves come out REVERSED**: "Nonseasonal MA"
  becomes `roots.ma.nonseasonal`. The trailing number is the ROOT INDEX, not
  `Oprfac`. `roots()` had been ported for years; only this call site was
  missing.
- `prtmdl.f:752-754` + `:830-934` -- `nonseasonaldiff`/`seasonaldiff`/
  `nmodel`, and the `<AR|MA>$<period>$<factor>$<lag>` coefficient rows
  carrying the estimate, its standard error (`sqrt(Var*Armacm(i,i))`, indexed
  by a counter that advances only on FREE lags) and its t-value. **This key
  keeps the title's own CASE while the `roots.` key beside it lowercases both
  halves** -- same title, two different conventions, three lines apart.
  `isfixd.f`'s SAVEd `cpntfx`/`oprfix` are reproduced: a lag is marked
  `(fixed)` only when neither its whole component nor its operator was
  already marked at a coarser level, and an all-fixed model additionally
  clears `lprtse`, which turns the se/t columns into literal zeros rather
  than dropping them.
**Mutation-tested twice** (a 1% perturbation of an emitted root modulus, and
a 2% one of an ARMA standard error): each fails 274-275 of the 275 specs and
reverting restores them. That matters because these are additive keys on an
existing gate -- without the mutation there is no evidence the new columns
are compared at all rather than silently skipped by the key regex.

## 38. `aape` -- the average absolute percentage FORECAST ERROR (amdfct.f) -- CLOSED (byte-exact, within-sample).

 The X-11-ARIMA forecast-quality
diagnostic: for each of the last three years the model forecasts one year
ahead from an origin that many years back, and the mean absolute percentage
error of those forecasts is reported. 283 of the 331 goldens carry it and the
port had none of it. `core/src/diag/amdfct.{hpp,cpp}`, called from
`run_pre_model` between the estimation savelog block and the real forecast --
the placement matters, because it forecasts from origins INSIDE the span
using the design `regvar` has already built, so anything that rebuilds `Xy`
first would invalidate it. Three things worth knowing:
(1) **Only the WITHIN-SAMPLE variant was ported at first** -- all 283 goldens
that carry the block are `aape.mode: withinsample` and the other 4 are `none`.
Two updates since. **2026-07-28: the wall was real and unreachable.**
`estimate{outofsample=}` was parsed and discarded, so the engine reported the
within-sample numbers AND labelled them `aape.mode: withinsample` against the
oracle's `outofsample` -- "the computation is walled" and "the option is safe"
are different claims. **2026-07-29b: the OUT-OF-SAMPLE arm is now ported and
gated, closing `estimate{outofsample=}` and `pickmdl{outofsample=}` together;
only the BACKCAST arm (`Bckcst`, reached from `pickmdl{bcstlim=}`) is left.**
See the dedicated entry below.
(2) **`ave` is seeded to ONE and then accumulated into**, so on the branch
that uses it the divisor is `(1 + sum|x|)/n` rather than the mean -- a Census
defect, reachable only on a series that dips to or below zero in the window
(where a percentage error is meaningless and the oracle switches to an
absolute error scaled by the window's magnitude). Transcribed verbatim.
(3) The actuals come from `subset.f` reading the DEPENDENT-VARIABLE column of
`Xy`, not from the input series: a FIXED regressor's contribution was
subtracted out during estimation, so amdfct adds `Fixfac` back into both the
actual and the forecast before differencing them.
**The gate had a silent hole this found.** `test_check_diagnostics.py`'s key
regex allowed no digits after a dot, so `aape.0`..`aape.3` were invisible on
BOTH sides -- the comparison passed while covering nothing, and only a
coverage count (`0 specs compare aape.0`) revealed it, not the green run. The
regex is widened and there is now a `test_every_owned_key_is_readable` guard
asserting `_KEY_RE` actually matches every key the gate claims. Mutation-
tested afterwards: a 1% perturbation of `aape.1` fails 271 of 276.

## 39. D8B and D9A -- CLOSED (line-exact), and the span-replay clobber came with them.

 Two X-11 diagnostics the port computed everything for and then never
reported: both call sites in x11pt3 were commented `deferred` alongside their
PRINT tables, but each also writes a `.udg` savelog block that is **not print
surface** -- prtd8b's gate is `Prttab .or. Savtab .or. (Lsumm>0 .and.
gudrun)`. 172 goldens carry `d8b.NN`, 169 carry `d9a.NN`, and neither was
emitted. `core/src/x11/d8bd9a.{hpp,cpp}`, gated by a new `test_d8b_d9a` in
`test_x11_diagnostics.py` over the same 154 specs -- **zero new goldens**.
- **D9A was free**: `vsfa` already computed all three columns (Ibar, Sbar,
  and their ratio) into `/x11opt/ Rati`; only the report was missing. It is
  emitted under the oracle's own two gates (`Khol!=1 .and. Kfulsm<2`), so a
  classic X-11 Easter run and the trend-only mode correctly emit nothing.
- **D8B** needed `numaff.f` (how many SI ratios a level shift of a given
  magnitude disturbs -- a lookup on the percent level change by trend-filter
  length) plus prtd8b's own label logic: `extind` counts REASONS (+1 for an
  X-11 extreme, +2 per regARIMA outlier) and collapses to one character,
  `*`/`#`/`@`/`&`, with an LS additionally stamping `-` on its neighbourhood
  directly rather than through `extind`. The savelog pass then rewrites `*`
  as **`z` when the weight went all the way to zero** -- the observation was
  REPLACED, not merely damped. Note the row is labelled with the DATE's
  period as it stands after the last `addate` of the inner loop, not with the
  loop index.
- **The find: 31 of the 305 new cases failed, every one of them a
  `history{}` or `slidingspans{}` spec.** prtd8b/prtd9a write straight onto
  ctx from inside x11pt3, and a span replay is a full x11pt3 pass, so every
  row came back as the LAST SPAN's (measured: `airline_slidingspans` d9a.01
  `0.3357/0.2909/1.1538` against the golden's `1.1603/0.2127/5.4561`).
  `ctx.d8bd9a` joins the save/restore set in `run_x11` and `run_seats` --
  the same discipline `/x11srs/`, `/lkhd/`, `/adxser/` and `ctx.x11_f2tests`
  already needed, and the fourth time this exact seam has bitten. **Any new
  ctx field written from inside x11pt1/x11pt2/x11pt3 needs adding to that
  set**; the pattern is now frequent enough to check by default rather than
  to discover per feature.

## 40. The remaining single-line X-11 savelog canaries -- `d11.f`, `d11.3y.f`, `sfmsr`, `autosf.msrNN`, `d7trendma`, `finaltrendma` -- CLOSED (line-exact).

Every routine behind them was already ported; only the reports were missing,
and the D11 F-test call site was deferred outright. Gated by a new
`test_x11_misc_canaries` over the same 154 specs, **zero new goldens**.
- **`d11.f` / `d11.3y.f` are the SAME ftest call gone round twice.**
  `ftest.f:65` is a `DO WHILE` the Ind==1 path re-enters with
  `kb = Ie - 3*Nyr + 1`, so the second pass measures residual seasonality
  over just the last three years. The port's ftest had computed the Ind==1
  f/prob all along and then stored neither, and had no second pass at all.
- **On a `force{}` run those two canaries do NOT describe D11.** x11pt3
  calls ftest three times with the SAME `Lsav` -- on `Stci` (:620/:661), on
  the FORCED `Stci2` (:812), and on the ROUNDED `Stcirn` (:897) -- and each
  write overwrites the last, so the surviving `d11.f` is whichever series the
  run finished with. Measured: `airline_force-td` golden 0.65551 against the
  unforced 0.64562. All three call sites are now present; the port had only
  deferred comments where the second and third belong.
- **`finaltrendma` needed a gate the port's structure had erased.**
  x11pt3.f:400 wraps the whole final-trend block in
  `Kfulsm.eq.0.or.Kfulsm.eq.2`, but this port runs `vtc` unconditionally
  (feeding it D1 instead of the modified SA when `Kfulsm==1`), so the naive
  placement emitted a key on `type=summary` runs where the oracle emits
  none -- caught only because the gate asserts key sets in BOTH directions.
- `sfmsr`/`autosf.msrNN` are the global-MSR filter-selection trace: one row
  per PASS, and the loop drops a year and retries whenever the MSR falls in
  neither decision band, so the row count is data-dependent. The trace is
  cleared at the head of each selection rather than appended to, so a span
  replay's passes cannot accumulate onto the main run's.

## 41. The SPECTRUM diagnostic block is silently absent on every monthly run -- scouted, NOT ported.

 `x11ari.f:282-287` calls `spcdrv` under a plain
`IF(Ny.eq.12)`, with no dependence on the `spectrum{}` spec at all, so the
oracle computes the whole block on every monthly run; this port gates
`run_spectrum` on `ctx.spcout.requested`, which only a `spectrum{}` block
sets. Measured on `generated/airline_x11-default` (no `spectrum{}`): the
golden carries the full `spcori.*`/`peaks.*`/`s1..s5`/`t1..t2` block and the
engine emits ZERO `sp0/sp1/sp2` rows and none of the canaries. 289 goldens
carry `spcori.*`, 324 the QS family. The 61-point spectra themselves ARE
ported and bit-exact, so what is missing is peak arithmetic on top of a
verified base: `shlsrt`/`mkmdsx`/`ispeak`/`idpeak`/`smpeak`/`svpeak`/`mxpeak`,
about 420 Fortran lines, plus the separable `getTPeaks` (Tukey) and `genqs`
(QS) steps. Full map, including the fixed monthly peak indices measured off
the goldens and the suggested order, in **`tools/spectrum_peaks_scouting.md`**.

## 42. The SPECTRUM PEAK diagnostics -- CLOSED (byte-exact), and the whole block was silently absent on every monthly run.

 `x11ari.f:282-287` calls `spcdrv`
under a plain `IF(Ny.eq.12)` -- no dependence on the `spectrum{}` spec at all
-- so the oracle computes this on EVERY monthly run; this port gated
`run_spectrum` on `ctx.spcout.requested` and had none of the peak arithmetic.
278 goldens carry `spcori.median` and the engine emitted nothing for any of
them. Ported into `core/src/driver/spectrum_peaks.{hpp,cpp}` (shlsrt, mkmdsx,
mkpeak's monthly/default/Peakwd==1 tables, mkfreq's 67-point enhanced grid,
ispeak, idpeak, smpeak, svpeak, mxpeak) plus svfreq/mkspky/savpk in the
harness. Gated by `tests/parity/test_spectrum_peaks.py` (140 specs), **zero
new goldens**; mutation-tested twice (a 1% perturbation of the median fails
138/140, of the star heights 118/140).
**The gate change was not the whole of the first half.** `gtinpt.f:354-371`'s
entire `/rho/` default block lived at the head of `gt_spectrum`, so a spec
without `spectrum{}` read the struct's ZERO-INIT (`Ldecbl` false, `Spcsrs` 0,
`Bgspec` 0 rather than NOTSET) and `Peakwd` was never resolved at all --
`gtinpt.f:1285-1288` when there is no spectrum spec, `gtspec.f:355` when
there is, and the port had neither. Moving the defaults into `gtinpt` is what
made removing the gate produce anything.
**FOUR real sp0/sp1/sp2 defects fell out**, all invisible until now because
`test_spectrum_tables` only ran on specs that ASK for `spectrum{}` and none of
those reach the branches: (1) **`spcdrv.f:318`'s `Facls` divide was missing**
-- the level shift is taken back out of the SA series before its spectrum,
unconditionally on `Adjls==1` (unlike x11pt4's E2 block, which is gated on
`.not.Finls`), so every spec carrying an LS regressor had a different SA
spectrum (`spcsa.range` 12.418 vs 12.608 on `generated/airline_regb-initial`;
13 of the 21 value failures); (2) the `(Lx11.and.Kfulsm.eq.0).or.Lseats` gate
(spcdrv.f:284/:406) was missing, so `x11{type=summary|trend}` emitted spcsa/
spcirr blocks the oracle suppresses; (3) the pseudo-additive sp0 REBUILD
(spcdrv.f:166-174, `Stc*(Sts+(Sti-1))` or `Stc*Sti` at `Kfulsm==2`) was
unported, as was the `Lx11` guard on the ordinary Stex fold; (4) `ispos`
(spcdrv.f:187/:290-298) was missing -- the oracle refuses to log a
non-positive series and produces no table.
Plus one placement bug of the `ctx.est_nefobs` class: **`spcrsd`'s start date
must be recorded at ESTIMATION time** (`ctx.resid_begdate`). `arima.f:1125`
computes it while a `series{modelspan=}` still has `Begspn`/`Nspobs`
narrowed; setspn.f widens them back at `arima.f:1145/:1181`, i.e. AFTER.
Deriving it in run_spectrum slid the residual spectrum (`spcrsd.median`
-32.32 vs -32.59 on `generated/airline_modelspan-both-x11`).
**Performance decided a design choice, and it is worth knowing.** Running the
spectrum on every monthly spec adds four AR fits per spec, and the peak block
needs a FIFTH evaluation on the enhanced grid -- re-fitting for it took the
parity suite from ~200s past 600s. `spgrh` is therefore split into
`spgrh_fit` + `spgrh_eval` and the fit is reused across both grids, which is
legitimate only because `ifpl` (30 for monthly) never exceeds either grid's
`lagh1` truncation, so `sicp2` reads the same autocovariance prefix either
way. Suite runtime is unchanged at ~228s.
**All four follow-ons listed here are now CLOSED** -- `getTPeaks` (and it was
~130 lines, not the ~850 estimated), `genqs.f`, the 51 SEATS specs and the 85
MODEL-ONLY specs, each in its own entry below. Only the `Iagr==4` indirect
names remain. Map:
**`tools/spectrum_peaks_scouting.md`**.

## 43. The QS SEASONALITY statistics (`genqs.f`) -- the DIRECT X-11 path CLOSED (byte-exact), and the whole block was silently absent.

 327 of the 331
`.udg` goldens carry a `qs*` key and the port emitted none of them. Same
"every golden was blessed with `-s`" leverage as the last six increments, but
reached by a different route worth knowing: genqs's gate is
`Savtab(LSPCQS)`, not `Lsumm>0`, and `gtinpt.f:121-123` copies `sumtab` into
`Savtab` wholesale when `Lsumm>0` -- with `sumtab.var` entry 113 (LSPCQS)
TRUE. **And unlike the spectrum block there is no `IF(Ny.eq.12)` in front of
it**: `editor.f:855-863`'s monthly-only `Savtab` clear covers
`LSPCS0..LSPS0C` (93..102) plus LSPCTP/LSPCQC (115/116) and NOT 113, so
quarterly specs are in scope (41 of the corpus). Ported into
`core/src/diag/genqs.{hpp,cpp}`: `calcqs.f`, `calcqs2.f`, `qsdiff.f` and
genqs.f's own six-series body, plus `arima.f:1105-1118`'s residual pair in
`run_pre_model`. Gated by `tests/parity/test_qs_diagnostics.py` (157 specs,
**zero new goldens**, byte-exact through `fwrite_fmt`); mutation-tested twice
(a 1% perturbation of `calcqs` fails 113/157, of `calcqs2` 140/157).
**Three things worth knowing.**
(1) **CB-25: `QsRsd`/`QsRsd2` are initialised INSIDE `arima`** (arima.f:129-
130) and computed there too (`:1105`), but `x11ari.f:106` calls `arima` only
`IF(Lmodel)` -- so a model-free `x11{}` run reads the COMMON's static zero and
reports `qsrsd: 0.00000 1.00000` as a statistic about residuals that do not
exist. The contrast that proves the mechanism is `extra/airline_identify`:
`identify{}` sets Lmodel, so the reset fires but the `:1105` block does not
(Var is zero), and that golden correctly carries no `qsrsd` row at all.
Reproduced by defaulting the pair to **0.0** and resetting to DNOTST in
run_pre_model under `has_model`, nowhere else.
(2) **`/x11srs/ Sti` and `Stc` are NOT the published D13/D12 -- and this port
makes them so.** `x11pt3.f:1089-1103` builds the AO/TC-restored D13 in a
LOCAL `sti2` and punches that; `Sti` itself is never touched, so x11pt4 and
genqs both see the outlier-REMOVED irregular. This port copies the published
values back over `x11srs.sti`/`.stc` at the tail of x11pt3 so the harness can
punch d13/d12 off the COMMON mirror, keeping the oracle's live values in
`ctx.x11_sti_int`/`ctx.x11_stc_int`. Invisible to every table gate, and
exactly what genqs tripped over: on `generated/expgs_fixed-airline-x11` the
published D13 gives `qsirr` 0.03946 against the oracle's 0.00000, because
folding the AOs back in flips the lag-4 autocorrelation positive and
**`calcqs` is a step function in the sign of `r(1)`** -- a small input change
is a categorical output change. **Anything new that reads `/x11srs/` after
x11pt3 must take the snapshot, not the mirror.** Localised by recomputing
calcqs in Python off the blessed `.d13` file: it reproduced the ENGINE's
0.03946, not the oracle's `.udg`, which said the difference was downstream of
the save rather than in the arithmetic.
(3) **`Bgspec` had two resolutions and now has one.** `gtspec.f:324-327`
(spectrum spec present) and `gtinpt.f:1282-1286` (absent) set the same default
-- eight years back from Endspn, clamped forward to Begspn -- and the port had
NEITHER, with `run_spectrum` computing it locally. That does not work for
genqs, whose residual pair keys on Bgspec during ESTIMATION; it is now
resolved once at the parse tail next to Peakwd. Found alongside it: the two
`Peakwd` sites are **not** equivalent -- `gtspec.f:355` is a bare `Peakwd=1`
with the `Sp.eq.4 -> 3` override commented out, so a quarterly spec carrying
a `spectrum{}` block gets 1 where one without gets 3 (inert today, only the
monthly peak grid reads it). `spectrum{logqs=}` was also parsed-and-dropped.
Tolerance is byte-exact on 155 of 157, with `max(5e-4 abs, 5e-5 rel)` on
`qsrsd`/`qssrsd` for the two `airline_automdl-x11` specs (measured 2.30823 vs
2.30858) -- the same bound and reason as `test_check_diagnostics`, restricted
to those two keys so an X-11 statistic drifting still fails loudly.
Nothing open here: the MODEL-ONLY path, the SEATS path and the `Iagr==4`
indirect names are all closed (see below, and the composite entry at the end
of this list for why there is deliberately no `qsind*` -- CB-31). Map:
**`tools/genqs_scouting.md`**.
**`gennpsa.f`'s NP residual-seasonality verdict landed alongside it** (the
`nplog`/`npsadj`/`npsadjevadj`/`npssadj`/`npssadjevadj` keys, 230 goldens) --
same call chain (`x11ari.f:322-326`, straight after spcdrv, again with no
`Ny==12` gate), same module, same gate. `npsa.f` differences the SA series,
mean-deletes it and thresholds `kendalls` at a FIXED critical value (24.73
monthly / 11.35 quarterly), so the output is a yes/no rather than a
statistic; `kendalls` was already ported for check{}'s Friedman test and is
now shared out of `checkres.cpp`'s anonymous namespace. Only the SA series
and its EV twin are tested, which is why 101 goldens carry no `np*` key at
all against 4 with no `qs*` key. Two ported asymmetries: `gennpsa` DERIVES
its `lplog` from Muladd/Lam up front where genqs LATCHES it as a side effect
of a log actually being taken, and the two `npsa` calls pass different flags
(`:77` the derived `lplog`, `:104` the raw `Llogqs`). **CB-26**:
`gennpsa.f:112` tests two INTEGERs initialised to NOTSET (-32767) against
**DNOTST** (the DOUBLE -999.0), so `lnps` is unconditionally true -- harmless
in the savelog, where every row re-tests NOTSET individually, but the print
branch emits a "(Series start in ...)" header on a run with no span
statistics. Mutation-tested: inverting the npsa verdict fails 153 of 157.

## 44. The MODEL-ONLY diagnostics path -- CLOSED, and it unblocked BOTH the QS block and the spectrum peak block at once.

 `x12run.f:181` reaches `x11ari`
with neither `Lx11` nor `Lseats`: a spec asking for no adjustment at all still
runs the whole X-11 PRE-STAGE (x11pt1, and x11pt2 too -- `:199` is gated
`(.not.Lcmpaq).or.Lx11`, true for any non-composite run) and comes out the
other side into `genqs`/`spcdrv`/`gennpsa`, which is the entire reason that
path exists. This port's `run_x11` refused a spec without `x11{}` outright, so
95 goldens' `qs*` block and 85 goldens' `spcori`/`spcrsd` block had nowhere to
come from. `x11_prestage` now takes `lseats` and `lx11` as INDEPENDENT
arguments (they were complementary: `lx11 = !lseats`), `run_x11` wraps
x11pt3/x11pt4 in x11ari.f:253's own `ELSE IF(Lx11)`, and the harness suppresses
the D/E table dumps when there is no x11{} (`b1` excepted -- x11pt1 builds it
either way and the oracle prints it on a model-only run too).
Gates went 157 -> 249 (`test_qs_diagnostics`) and 140 -> 222
(`test_spectrum_peaks`), still with **zero new goldens**. **Four real defects
came out of it**, none of which any existing gate could see:
(1) **`spcdrv.f:193-200`'s detrend keys on a DIFFERENT test without Lx11.**
With an X-11 spec the log is taken when `Muladd != 1`; without one there is no
mode to read and the oracle keys on the TRANSFORM instead, `dpeq(Lam,ZERO)`.
The two agree for a log X-11 run and disagree for every non-log model-only
spec, where Muladd still carries its multiplicative default -- the engine was
logging a sqrt-transformed series (`spcori.median` +24.29 oracle vs -27.79
engine on `generated/airline_trans-sqrt`, i.e. a sign flip, not a drift).
(2) **`arima.f:321`'s `IF(Ldestm)` gates the ENTIRE estimation half of arima**,
and this port estimates whenever a model spec is present. On an identify-only
spec the oracle sets `Lmodel` but not `Ldestm`, does no estimation, and leaves
`Var` at zero -- so it writes neither `qsrsd` nor the `spcrsd` block, where the
engine emitted a full residual spectrum and a QS of 255.92 off residuals that
are essentially the series. The residual CAPTURE is now gated on `ldestm`.
(3) **Five of the ten `Ldestm` setters were unported.** `gtinpt.cpp` set it for
automdl/estimate/seats only; outlier (`:704`), check (`:712`), forecast
(`:721`), history (`:753`) and pickmdl (`:880`) were missing, as were both
parse-tail rules -- `:1151` (an adjustment run WITH a model estimates one
whether or not any spec asked) and `:1252` (an X-11 run carrying a regression
effect to remove or keep must estimate too). `generated/airline_user-reg-x11`
needs the last one specifically: `forecast{maxlead=0}` suppresses both of the
earlier rules, since `:721` sees `Nfcst==0` and `:1151` sees Nfcst already set.
(4) **`gtinpt.f:398-406`'s `Adj* = 1` defaults were unported**, which is what
rule `:1252` reads. Harmless until now only because `chkadj`'s toggles are
idempotent from either starting value (`==1 && n==0 -> 0`, `==0 && n>0 -> 1`);
anything reading the indicators BEFORE chkadj sees the difference.
**The EXPLICIT-aictest leap-year prior loss it exposed -- CLOSED, in three
lines, and my root-cause hypothesis was WRONG.** `regression{aictest=(td)}` on
the explicit-model path (arima.f:569, not automdl) was dropping the leap-year
prior from `Stcsi`: `generated/airline_aictest-td` gave b1 1949.Feb = 118.0000
-- the RAW value -- against the oracle's ~119.05, every other month agreeing,
the Februaries-only signature. The negative control was
`generated/airline_reg-td1coef`, which fits the SAME model chosen explicitly
and gives 119.0536, proving the model and its coefficients were right and only
the prior was lost. I guessed `ssprep`/`restor` was reverting `Priadj` (it
saves `Pri2`, and that mechanism has bitten this port before, in the
slidingspans Priadj bug). **It was not.** The Fortran keeps Priadj across
ssprep/restor exactly as intended; the port's defect was one branch further
out. `tdaic` modifies `trnsrs` IN PLACE (dividing out `lomeff`), and
`run_pre_model`'s automd branch re-captures that buffer into `out_trnsrs`
while its `explicit_aictest` branch never did -- so `x11_prestage` consumed the
stale, un-prior-adjusted transformed series. The fix is the same
`if (out_trnsrs) *out_trnsrs = trnsrs;` handoff the automd branch already had.
**The generalizable bit: an in-place buffer mutation plus a per-branch handoff
is a hazard, and the two branches have to be diffed against each other rather
than each read on its own** -- the missing line is invisible reading only the
branch that has the bug. Both gates' skips are deleted and the four cases pass.
**No corpus spec combines an EXPLICIT aictest with `x11{}`** (only
automdl+aictest is covered), which is why nothing had ever compared this
path's B1 until the model-only path started producing output.

## 45. The QS / NP diagnostics under `seats{}` -- CLOSED (byte-exact), and it found a wrong-numbers bug on EVERY non-x11 run that is not taking a log.

`test_qs_diagnostics` 249 -> 313, still **zero new goldens**. SEATS specs now
run through `x13run_seats`, which shares the emit with `x13run_x11` via the
new `tools/dump_diag.hpp` -- x11ari.f reaches genqs (`:277`) and gennpsa
(`:322`) only after the Lseats/Lx11 branch REJOINS, so both blocks belong to
every adjustment path, not to X-11.
**The four buffers, and why only two are independent.** The oracle fills them
through the `ansub9.f` USRENTRY bridge from inside `seats()`: `sa` arrives
TWICE, as 1309 -> `Seatsa` and 1203 -> `Stocsa`, and `ir` twice as
1312 -> `Seatir` and 1204 -> `Stocir`. At `sigex.f:3623-3631` they are
literally the same two local arrays; what separates them is `seatad.f`, which
post-processes only the `Seat*` pair -- `:27`'s `/100` on `Seatir` under
`Muladd!=1`, and on `Seatsa` the forecast append (`Posfob+1..` only) plus the
`Adjsea==1` Facsea divide. So over the observed span `Stocir/100` IS `Seatir`
(hence genqs's ONEHND divide on one and not the other) and `Stocsa == Seatsa`
unless a regARIMA seasonal regressor is present. **`qsirr` and `qsirrevadj`
are therefore provably identical on the SEATS path in BOTH modes** --
multiplicative by that identity, additive because `calcqs` is scale-invariant
and the `-1` re-centring is skipped. `publish_seats_commons` (run_seats.cpp)
reconstructs the `Stoc*` pair by inverting exactly those two steps from
`ctx.seats_sa`/`ctx.seats_ir`; the Facsea arm is TRANSCRIBED, not measured (no
corpus spec pairs a seasonal regressor with `seats{}`). Taken from `ctx.seats_*`
rather than from inside `seats_decompose` on purpose -- those are what the span
drivers save/restore, matching the oracle's own `ansub9.f:110`
`IF(Issap.eq.2.or.Irev.eq.4)RETURN`, which skips the `Stoc*` store on a replay.
**The real find: `editor.f:517-518` was unported.**
`IF(.not.Lx11.and.(Fcntyp.eq.4.or.Fcntyp.eq.0.or.dpeq(Lam,1D0))) Muladd=1` --
on a run with NO `x11{}` that is not taking a log, the adjustment mode is
forced ADDITIVE, overriding the multiplicative default `gtinpt.f:956` just
resolved. gtinpt cannot do it itself: its rule keys on `Fcntyp` alone and
cannot see `Lx11`. This port had the gtinpt half and not the editor half, so a
no-transform SEATS spec carried `Muladd 0`. Not print surface -- genqs
re-centres a ratio irregular by subtracting one under `Muladd != 1`, and a
SEATS decomposition without a log produces an ADDITIVE irregular centred on
ZERO, so the missing rule shifted the series to about `-1` and turned a QS of
0.00000 into **1460.26951** on every `unrate_*-seats` spec. It reaches
`divsub`/`addmul` on this path too. Ported at the tail of `parse_spec.cpp`
beside `editor.f:429-434`'s constant shift, the one place unambiguously "after
gtinpt, before everything else". `Tmpma` is deliberately NOT updated
(`gtinpt.f:970` sets it pre-editor; `:518` touches only Muladd).
Fixed in passing: `genqs.f:148-160`'s SA arm sets `lplog` on its ELSE (SEATS /
model-only, keyed on `Lam==0`) arm and not on its Lx11 arm; the port was
dropping it for both. Inert -- no corpus spec sets `spectrum{logqs=yes}`.
**What the SEATS goldens can and cannot discriminate, and a mutation that was
WRONG.** Every SEATS golden reports `qssadj`/`qsirr` as `0.00000` except
`payems_seats`, and that one has `npsi==1` (`sa == z`, so `qssadj` is trivially
`qsori`) -- the corpus's one non-degenerate SEATS value is its `qsirr` 0.01133.
The real coverage the 64 specs add is the nonzero `qsori`/`qsorievadj`/`qsrsd`
block the whole-spec skip had been discarding along with the rest. Because of
that degeneracy the arms were mutation-tested explicitly, and **the first
mutation was worthless: a 1% SCALING of the published SA series fails ZERO
specs, because QS differences the series and then takes autocorrelations -- a
pure scale factor is invariant by construction.** Only a SHAPE change
discriminates; a 5% spike every 12th observation fails **62 of 62** through
the SA arm and **50 of 62** through the irregular (the other 12 have no
irregular, i.e. `Hvstir` false). Map: **`tools/genqs_scouting.md`**.

## 46. `getTPeaks` -- the TUKEY SPECTRAL PEAK probabilities -- CLOSED (byte-exact), and it was ~130 lines, not the ~850 the scouting doc predicted.

The `.tukey.*` families (283 goldens for `spcori`, 239 `spcrsd`, 198 each for
`spcsa`/`spcirr`) plus `peaks.tukey.{seas,td,p90.seas,p90.td}` (283).
**Zero new goldens AND zero new tests** -- the keys were previously excluded
from BOTH sides of `test_spectrum_peak_block` and are now compared inside the
same 222 specs, so the mutation test below is the only evidence they are read.
**The size estimate was wrong by a factor of six, in a way worth
generalizing: a routine's FILE is not its size.** `getTPeaks` is nine lines --
`getWind` + `covWind` + `Tpeaks2` -- and the first two were ALREADY PORTED as
`tukey_spectrum` (they produce the bit-exact st0/st1/st2 save tables). Only
`Tpeaks2` (specpeak.f:400-544) and `dfPeaks` (:333-399) were missing, plus the
`Fcdf` -> `BetaInc` -> `LogGamma`/`BetaCfra` chain from `special.f` (now in
`numeric.cpp`, flagged there as TRAMO/SEATS code rather than Census). The rest
of specpeak.f is the AR-spectrum peak front, which is print surface here.
Cross-check what is already ported before sizing a front off a line count.
**What these are:** not star heights against a median (that is svpeak/smpeak)
but F tests on the ratio of each candidate ordinate to its neighbours, with
the four degrees of freedom interpolated from the window size and series
length. Three things to know. (1) The pi-radian peak is one-sided
(`H(i)/H(i-1)`, df3/df4 rather than df1/df2) and is **filed at `ps[MQ/2]`, not
at the next free slot** -- which is the only reason `spcXXX.tukey.s6` exists
for monthly data. (2) **`Tpeaks2` reads the RAW spectrum, not the decibel
one**: savstp applies the `10*log10` when it punches st0/st1/st2, and these
statistics are ratios that a log would turn into differences -- so the peaks
are scored off `tukey_spectrum`'s internal `p`. (3) `spcdrv.f:252` gates the
Tukey block on `nsrs.gt.80` and `spcrsd.f:112` on `ntmp.ge.80` -- same
routine, two call sites, strict vs non-strict -- so the bound is a parameter.
**Two Census bugs, both plumbing rather than arithmetic. CB-27**
(`svtukp.f:43/85`): `oriIdx` is a TABLE index (the original series' slot in
`Itukey`) and the loop that tests it is over the six seasonal FREQUENCIES. The
intent is to keep an unadjusted original out of the `peaks.tukey.*` lists on a
model-only run; the effect is to drop one seasonal frequency -- whichever
number matches that slot -- from EVERY table's counts. Confined to model-only
specs, since `oriIdx` stays NOTSET whenever `Lx11.or.Lseats`. **CB-28**
(`spcrsd.f:113-119`): the residual diagnostic span is repacked into `Temp` and
then `a` is passed to getTPeaks, so only the LENGTH reflects `ipos` and the
residual Tukey spectrum is always taken from the FIRST `ntmp` residuals rather
than the last. The three spcdrv call sites do the identical repack correctly,
which is what makes it a slip rather than a convention.
Mutation-tested: a 1% perturbation of `fcdf` fails **218 of 224**, the six
survivors being specs whose series is too short for any Tukey window. Map:
**`tools/spectrum_peaks_scouting.md`**.

## 47. `pickmdl{}` / `automx.f` — CLOSED (bit-exact), and it was parse-only AND silently model-less.

 `gt_pickmdl` routed all 11 arguments through
`gt_generic`, so a pickmdl spec returned `OUTCOME: OK` having fitted **no
ARIMA model at all** — `nmodel: 0`, `nefobs: 144` against the oracle's
`(0 1 2)(0 1 1)` and 131 on `extra/airline_pickmdl`. It was the largest
single source of real feature skips (9), all now gone.
`automx.f` is the CLASSIC X-11-ARIMA selection, the sibling of `automd.f`
(TRAMO) and a completely different algorithm: instead of identifying orders
from the data it ESTIMATES a fixed candidate list (from `file=`, else five
built-ins) and keeps the best one passing three screens — amdfct's three-year
average forecast error vs `fcstlim`, the Ljung-Box p-value at lag 24 vs
`qlim`, and the NONSEASONAL MA coefficient sum vs `overdiff`. `method=first`
stops at the first acceptance; `method=best` tightens the bar to each accepted
model's own error, so the last acceptance is the lowest. A candidate carrying
a trailing `*` is the DEFAULT: with nothing accepted, and a run that still
needs regARIMA preadjustment factors, it is used anyway with forecasting
switched off (`nofcst`, `hvstar==2`). Ported into
`core/src/automdl/automx.{hpp,cpp}` with the four routines it is the only
caller of — `mdlinp.f` (point the lexer at the `.mdl` file; this port's lexer
reads an in-memory line vector, so the equivalent of the Fortran's REWIND is
to replace it and re-run `intinp.f`'s reset), `setamx.f`, `bstmdl.f`/
`bstget.f` — plus `nofcst.f`, a real `gt_pickmdl` carrying gtautx.f's range
validation, and gtinpt.f:248-257's defaults.
**Everything it needed was already ported** (`rgarma`, `regvar`, `mdlset`/
`mdlint`, `getmdl`, `ssprep`, `idotlr`, `acf` from `check{}`, and `amdfct`
from the aape diagnostic — `AapeDiagnostics.ok` IS the Fortran's `Fctok`),
which is why the whole front landed in one increment.
**The one non-mechanical bug: `bstget.f:80-96`'s effective-observation split
belongs INSIDE `bstget`, not at the call site.** Extracted to a shared helper
and called only from the loop head, the re-estimation of a winner that was not
the LAST candidate estimated **crashed** — `Nintvl` still described the
previous candidate, so `rgarma` read past the differenced series whenever the
winner had a different differencing order.
**Measured both directions for all 11 arguments** (oracle on-vs-off, then
engine-vs-oracle over all 56 shared `.udg` keys): `identify` 189, `overdiff`
168, `qlim` 160, `fcstlim` 160, `method` 156, `file` present-vs-absent — and
the engine matches every one at **0** keys differing. All three numeric
screens bite only in the REJECT-more direction (`qlim` up from 5, `fcstlim`
down from 15, `overdiff` down from 0.9). **`mode=` is provably INERT**:
`iautom` is a `gtinpt.f:873` LOCAL whose only use is the `> 0` test setting
`Lautox`, and `gtautx.f:230` forces it to 1 regardless.
Walled, each measured first and each a clean fatal rather than a silent wrong
answer: `outofsample=yes` (`Outfer` — amdfct's out-of-sample arm is unported,
and **the oracle picks a DIFFERENT model with it on**, so this is a real gap);
`bcstlim=`/`forecast{maxback=}` (the backcast acceptance pass needs amdfct's
`Bckcst` arm); per-candidate AIC-regressor testing together with the Picktd
restore it drags in; and `arima.f:476-527`'s no-model cleanup.
Gated by seven corpus specs — `extra/airline_pickmdl` plus six hand-authored
ones (`-first`, `-nofile`, `-qlim`, `-overdiff`, `-default`,
`-identify-first`) whose selected models spread across `(0 1 1)(0 1 1)`,
`(0 1 2)(0 1 1)` and `(2 1 0)(0 1 1)`. **They are NOT produced by
`genextra.py`** and say so in a header comment; running it deletes them (the
standing corpus-generator hazard). Mutation-tested four ways, each failing a
DIFFERENT set — not tightening `loclim` 9/10, skipping `bstget` 9/10 on a
disjoint set, dropping the `ovrdff` screen 27/70, disabling the starred
fallback 18/70. **Probe-harness trap worth keeping:** the generated spec's own
comment header contains the string `pickmdl{}`, so a `replace("pickmdl{", …)`
injects the probe argument into a COMMENT and the oracle rejects the file —
every variant read "ORACLE REJECTS", which looks exactly like a finding. Same
afternoon, an unanchored `file =` match also stripped `series{file=}`. Map:
**`tools/pickmdl_scouting.md`**.

## 48. The composite DIAGNOSTICS front, direct and INDIRECT (`Iagr==4`) -- CLOSED (bit-exact), and the direct half was pure harness coverage.

 Two increments.
(1) **`x13run_composite` had never emitted the QS / spectrum-peak / NP blocks
at all.** x11ari.f reaches genqs (:277), spcdrv (:282) and gennpsa (:322) on
EVERY spec of a metafile -- the oracle writes one `.udg` per spec and all of
them carry these -- so a composite run reported none of the ~100 keys per spec
the other two harnesses had gated since the spectrum-peak and QS increments.
Nothing in the ENGINE was wrong: `run_x11` already calls all three ahead of its
composite tail (the oracle's own ordering) and **all 103 shared keys matched
the moment the emit was wired in**. `dump_diag.hpp`'s three helpers gained an
optional key prefix and an optional string sink (the composite harness buffers
its output so `OUTCOME:` can lead it, and prints components as
`<base>:<key>`); default arguments, so the X-11 and SEATS harnesses are
untouched.
(2) **The INDIRECT pass, 52 further keys.** x11ari.f:344-370 runs spcdrv and
gennpsa a SECOND time after `agr3` has replaced the D-table buffers with the
aggregated adjustment. `run_spectrum` and `gennpsa` take an `iagr4` parameter
rather than being duplicated: the indirect pass skips the ORIGINAL
(spcdrv.f:158 `goori = Iagr.le.3`) and the RESIDUAL block (spcrsd belongs to
the regARIMA phase), names its two tables `spcindsa`/`spcindirr` (mkspky.f:22/
30), and lands in `*_ind` fields so the direct results survive.
**Two things worth knowing.** (a) The peak-label lists are NOT independent
between the passes: spcdrv appends to ONE accumulated string and savpk.f:88-115
splits it at `Nspdir`, the count x11ari.f:290 recorded between the two calls --
so `peaks.seas` is the whole list and `peaks.seas.dir`/`.ind` are its halves.
(b) **`svtukp.f` sets `iLb=7` on the indirect tables where it is 4 on the
direct ones** (:56, :66), which skips the `ind` as well as the `spc` -- so the
KEY is `spcindsa`/`spcindirr` while the LABEL inside `peaks.tukey.*.ind` is
plain `sa`/`irr`. That was the single remaining mismatch after everything else
lined up (`peaks.tukey.p90.seas.ind: irr`, not `indirr`).
**CB-31**: `x11ari.f:346` hands `genqs` **`LSLIQS`** (=69, a SAVELOG index from
`spcsvl.i`) where `genqs.f:439` uses it as `Savtab(Tblind)`, a TABLE-log
subscript; the direct call one screen earlier correctly passes `LSPCQS` (=113).
`LSPQSI` (=114) exists in `spctbl.i`, is plainly the intended argument, and is
passed nowhere. So the oracle emits **no `qsind*` key at all** -- confirmed
against the golden, which carries a full set of `npind*` and `spcind*` beside
it. Reproduced by not making the call; there is nothing to compute. The
neighbouring calls are all correct (`gennpsa` gets `LSPNPA`/`LSPNPI`, both
from `spctbl.i`), which is what makes it a slip rather than a convention.
Result: **155 of 155 shared diagnostic keys on the composite total match, zero
golden-only and zero engine-only**, with zero new goldens. Gated by
`test_composite_{direct,indirect}_diag_block` and `test_composite_no_indirect_qs`
(which pins CB-31 from both sides). **A MEASURED COVERAGE GAP, recorded rather
than hidden:** savpk's real `.dir`/`.ind` SPLIT is unexercised -- this composite
finds no visually significant peak in any table, so all four keys are `none`
and only the degenerate branch runs. A mutation swapping the two output halves
**passes the whole suite**. Gating it needs a composite whose components carry
a residual seasonal or trading-day peak; the note is at the code and at the
gate.

## 49. `composite{}` under `seats{}` -- `agr3s.f` -- CLOSED (bit-exact over the observed span), and a SEATS metafile used to come back FATAL.

 `X11agr` is a
metafile-wide flag, not a spec option: `aaamain.f:73` arms it TRUE once for the
whole run and `gtinpt.f:1170` ANDs each COMPONENT's own `Lx11` into it, so ONE
SEATS component routes the total's indirect adjustment through **`agr3s`**
(`x11ari.f:342`) instead of `agr3`. Nothing of that existed: `agr2_component`
accumulated `Stci` unconditionally (a SEATS component has none),
`agr2_compare` hardcoded `const bool x11agr = true`, and the metafile harness
handed every spec to `run_x11`, which refuses a `seats{}` spec outright.
**`agr3s` is a different answer, not a variant.** No extreme-value pass, no
Henderson, no D8/D9 battery, no `x11pt4` behind it: the indirect SA series IS
the aggregate `Ci` of the components' own SA series and the seasonal factor is
`O5/Ci`. Measured on the oracle, the entire output is `isf isa ie5 ip5 ie6 ip6
i18` even when the spec asks for the full indirect family -- the gate asserts
the ABSENCES in both directions. Three things that fall out of it and were
each a real defect until now: (1) `agr2` drops its whole R2 half
(`:128-131`/`:175-178`) because R2 is the variance of the SA/trend ratio and
there is no indirect trend; (2) **`spcdrv`'s `Iagr==4` branch comes BEFORE its
Lseats arm** (`:302-313`) -- the indirect spectrum is taken from `Stci`, the
series agr3s left, and NEVER from `Seatsa` even when the total is
SEATS-adjusted, which the port got wrong by keying on `has_seats`
(`spcindsa.median` came back equal to `spcsa.median` to the last digit); and
(3) `:436`'s `goirr = goirr .and. X11agr` means there is **no `spcindirr`
block at all** here -- the engine was emitting twenty keys the oracle does not,
and the gate could not see it until the diag comparison was made
bidirectional. Ported asymmetries, all transcribed: no `Lindot` guard on the
LS/AO factor build, the `Stci /= flsind` divide commented out at
`agr3s.f:151`, `ststd` computed at `:181` and never read, and the rounded-
series ftest gated on `Lx11` where agr3 has no such guard. **CB-32**: agr3s
omits `agr3.f:101-108`'s store of the direct SA into the `Orig2`-aliased
scratch, so `agr2`'s DIRECT roughness column measures the aggregate ORIGINAL
-- 66.777 against 53.586 down the agr3 path, and 66.777 is R1 of the summed
input `.dat` files to all six printed digits. **The one gap, measured and made
loud rather than silent:** `Ci` past the observed span needs `Setfsa`, the
SEATS forecast decomposition (`ansub3.f:356-678`) `seatad.f:49-54` appends into
`Seatsa`, which is unported -- and the forecast span is NOT avoidable by
configuration, since `editor.f:387-400` forces `Nfcst >= max(12,3*Sp)` on any
SEATS run regardless of `forecast{maxlead=}`. It costs i18's forecast rows
(agr3s.f:412-418 widens that punch range unconditionally, unlike isf's
`Savfct` gate) and, when the TOTAL also carries forecasts, a spurious `ita`;
the run writes a NOTE to Mt2 saying so. Gated by
`census-examples/composite-seats/` (X-11 total) and `composite-seats-total/`
(SEATS total -- the only corpus case reaching the composite tail from
`run_seats`), both with **fixed MA coefficients**: estimated, this port's SEATS
decomposition sits 1.3e-6 from the oracle's on this synthetic series while
every printed coefficient agrees to 11 digits, and the same 1.3e-6 appears on
a STANDALONE component run -- optimizer path noise, and the indirect
adjustment is a SUM, so it would land there undiluted. Mutation-tested three
ways (perturb `Ci`, force `x11agr` inside `agr2_compare`, drop `gtinpt.f:1170`)
-- the second one PASSED at first, because the harness emitted only `cmpstat
1..12` and the gate could not tell an uncomputed R2 half from a suppressed
one; it now emits all 24 and asserts 13..24 are exactly zero. Refactor that
came with it: `x11ari.f:329-374` is now `driver/composite_tail.cpp`, called
from BOTH drivers, because the oracle has one x11ari and this block sits after
its Lseats/Lx11 branch rejoins.

## 50. `estimate{outofsample=}` / `pickmdl{outofsample=}` -- amdfct.f's OUT-OF-SAMPLE arm -- CLOSED (bit-exact), and it is not a label change.

Both options were walled, one in the parser and one in the driver, because the
computation behind them did not exist. What the arm does: for each of the last
three years it pulls the model span END back another year, **RE-ESTIMATES**,
and forecasts one year from the new span's end -- so each forecast is made by
a model that has never seen the period it is forecasting, where the
within-sample variant forecasts from a past origin of the model fitted to
everything. The span shrinks CUMULATIVELY across the three passes.
Ported in `core/src/diag/amdfct.cpp` (`amdfct.f:70-90`, `:186-235`,
`:270-300`) plus `gtinpt.f:1203-1216`'s resolution of the two switches into
`Outfct`/`Outfer` -- whichever spec supplies one sets BOTH flags, and given
both, each sets its own. Four things worth knowing:
(1) **It changes the ANSWER on the pickmdl path, not just a diagnostic.** The
aape is the first of pickmdl's three acceptance screens, so
`extra/airline_pickmdl` selects `(0 1 2)(0 1 1)` by default and
`(0 1 1)(0 1 1)` with `outofsample=yes` (`nmodel` 3 -> 2).
(2) **`Nfev`/`Niter` are deliberately NOT restored.** amdfct saves and puts
back eight pieces of estimation state, the ssprep snapshot and `Endmdl`, but
its final `rgarma` (`amdfct.f:299`) is COMMENTED OUT -- so the `.udg` reports
the LAST re-fit's optimizer counters, not the main run's. Measured on the
oracle: `nfev` 19 -> 13 and `niter` 6 -> 4 with the flag on, on a run where
**every other `.udg` key is byte-identical**. Reproduced by not restoring them.
(3) **The outlier strip is the non-mechanical half.** Before each re-estimation
every OUTLIER regressor DATED INSIDE the three-year window is deleted from the
design (`dlrgef`, backwards over the columns because dlrgef renumbers) and its
fitted contribution subtracted out of the series (`daxpy` into `fotl`, then
`eltfcn SUB`) -- it has to be, because the shortened span no longer contains
those dates and the column would be identically zero. A RAMP is judged by its
END date and everything else by its start. **And the `ave` scale block then
reads the STRIPPED series, not the caller's `Trnsrs`** -- easy to miss, since
the two are the same buffer on the within-sample path.
(4) On the automatic path `Lauto` is in/out: a failed re-estimation clears it
and the candidate is dropped, and the Fortran returns THERE, before the
restore block (`amdfct.f:227`). Transcribed.
Gated by `generated/airline_outofsample` (the `estimate{}` twin + the
`nfev`/`niter` pin), `generated/airline_outofsample-otl` (the strip, with an
out-of-window outlier as the negative control) and
`extra/airline_pickmdl-outofsample`. Mutation-tested four ways, each failing a
different set: force within-sample (11), restore the counters (1), never strip
(1), strip everything (2). **Corpus note:** the `-otl` spec carries no `x11{}`
on purpose -- three outlier regressors put its D9A ratios ~4e-10 off the
golden, measured IDENTICAL on the within-sample twin, i.e. the ordinary
outlier-estimation floor and a property of the spec rather than of this
feature; `test_d8b_d9a` is byte-exact by design, so the spec simply does not
carry the adjustment that would drag it in.

## 51. `pickmdl{}` + `forecast{maxback=}` -- amdfct's BACKCAST arm -- CLOSED, and `bcstlim=` turns out to be INERT (CB-33).

 The combination used to FATAL on a
spec the oracle runs to completion. `Bckcst` is the out-of-sample arm
mirrored: the Xy design is time REVERSED so the same forward machinery
extrapolates backwards, the outlier window is the FIRST three years (every type
judged by its start -- no ramp special case), the `ave` scale reads the first
three years, and the out-of-sample variant walks `Begmdl` FORWARD instead of
`Endmdl` back, taking the ACTUALs from the year it is about to drop, in REVERSE
order, because `amdfct.f:239` skips `subset` on exactly that path.
**CB-33**: `automx.f:922`'s `IF(mape(4).gt.Bcklim.and.(.not.argok))` is `.and.`
where the algorithm wants `.or.`, so a model that CONVERGED can never fail the
screen -- and `prtamd`, which evaluates the screens itself, says otherwise out
loud. Measured with `bcstlim=1`: the `.out` prints "MODEL 2 REJECTED: Average
backcast error > 1.00%" and then "The model chosen is (0 1 2)(0 1 1)", and the
footer still reads "Includes 12 backcasts". Transcribed with `&&`.
**The gate had to read PRINTED output**, because this is the one amdfct result
with no savelog key at all: `tests/parity/test_backcast_aape.py` parses the
golden `.out`'s own table and compares the harness's new `bcstaape.*` lines at
prtamd's two decimals -- the same shape the composite gate uses for the
roughness table. Corpus: `extra/airline_pickmdl-backcast` (within-sample),
`-oos` (both amdfct arms at once, the only place they interact) and `-zero`,
which runs on `airline_zero.dat` and is **the only spec in the corpus that
reaches the `ivalue==1` absolute-error scale** -- and hence the only gate on
the `ave`-seeded-to-ONE defect. It earns its place twice over: airline_zero is
negative in its first three years and positive in its last, so it is also what
discriminates WHICH window the scale comes from. A mutation reading the
forecast window on the backcast path passed every other spec in the suite.
**One narrow gap, walled with its measurement rather than shipped:**
out-of-sample BACKCASTS with an outlier inside the first three years reads
6.6959 against the oracle's printed 6.71. The strip fires (instrumented:
`typ=1 beg=15 inwin=1`) and disabling it changes nothing, so the difference is
downstream of the window test, in how the stripped series feeds the reversed
per-pass design. Everything else is bit-exact: within-sample backcasts with
outliers, out-of-sample FORWARD with outliers, out-of-sample backcasts with no
outlier in the window, and the ivalue==1 branch. **amdfct is now complete apart
from that corner.**

## 52. `pickmdl{}` + `regression{aictest=}` -- the PER-CANDIDATE AIC-regressor tests and the Picktd restore -- CLOSED (bit-exact), and it found three silent-wrongness bugs, two of them on paths that have nothing to do with pickmdl.

 The AIC tests REPLACE the plain `rgarma` for a freshly identified
candidate (`tdaic`/`lomaic`/`easaic` self-estimate, and `argok` becomes
`.not.lester`), which is what makes the **Picktd** restore reachable:
`Picktd` decides whether the program's length-of-month / leap-year prior is
IN the series, so a verdict that differs between candidates changes the
SERIES BEING MODELLED, not just the design, and `Trnsrs`/`Adj` have to be
rebuilt (`td7var` + the Usrtad/Usrpad folds + `trnfcn`) with `Priadj`
following (4 = program TD prior, 1 = none). Ported with the two reachable
restores (`automx.f:317-323`, `:700-725`) and the post-loop `identify=first`
block (`:750-870`).
- **The three bugs.** (1) `Pvaic`/`Rgaicd`/`Traicd` were reset by each CALLER
  -- both `automd` and the explicit-aictest path clobbered them on entry --
  instead of once in `gtinpt` (`gtinpt.f:293-300`), so the pickmdl path read
  the struct's zero-init, and a `pvaic` of 0.0 rather than `DNOTST` turns
  tdaic's `chsppf(pvaic, df)` threshold ON, driving `Rgaicd(PTDAIC)` negative
  enough that the FIRST TD candidate always wins its comparison against the
  later, better ones. (2) `regression{aicdiff=}` (`getreg.f:405-428`) and
  (3) `pvaictest=` (`:474-497`) were **parsed and discarded**. Both are
  ordinary documented options and both affect EVERY aictest path. The
  pre-existing `generated/cover_reg-aicdiff` could never have caught them:
  it carries `aicdiff=0.0`, which is the DEFAULT and therefore inert.
- **A saturated precondition, found by a mutation that PASSED.** Deleting the
  per-candidate design restore left the whole suite green, because
  `pickmdl{identify=}` was structurally inert across the entire corpus --
  `lidotl` is `Ltstao.or.Ltstls.or.Ltsttc` (`arima.f:118`), so an `outlier{}`
  spec is what makes it do anything, and no pickmdl spec carried one. Closed
  by `extra/airline_pickmdl-aictest-otl` (oracle finds 5 outliers); the
  mutation then fails 10.
- **Two structural notes.** `editor.f:1151-1166`'s TD candidate vector is
  built ONCE in the oracle's editor before any model is estimated; this port
  has no editor block for it, so both callers run the shared
  `aictest_td_vectors` at their own equivalent point -- a per-candidate
  rebuild would read a design the editor never saw, since `tdaic` itself adds
  and deletes TD groups. And `Setpri = Pos1bk` (`editor.f:851`, after
  `setxpt` at `:233`) is 0 during this port's model phase because `setxpt` is
  never called pre-model, so tdaic's `Sprior` writes stay guarded off.
- **WALLED, with the measurement**: a trading-day AIC verdict that DIFFERS
  between candidates (`automx.f:259-296`), reachable only with a
  `regression{aicdiff=}` tuned between two candidates' AICC gaps. d10, d12
  and d16 are BIT-EXACT and all 52 shared `.udg` model keys agree (including
  `nreg` and every ARMA coefficient); only d11/d13 move, by exactly the
  leap-year prior on FEBRUARIES ONLY (0.885% non-leap, 2.655% leap). Ruled
  out: CB-34's assignment direction (mutation-tested -- identical failures)
  and the `Sprior`/`Setpri` deferral (`x11int` copies `Adj` into `Sprior`
  whenever `Nadj>0`, always true here). Remaining suspects, named at the
  wall: which of `Kfmt` / `Lpradj` / `Priadj` the oracle carries out of the
  LAST candidate's tdaic. `aictest=(user)` and user-holiday chi-square
  testing stay walled too -- `usraic.f`/`chkchi.f` have no C++ at all.
- **CB-34**: `automx.f:264` assigns `padj2=Priadj` where its two identical
  siblings (`:330`, `:725`) assign `Priadj=padj2`, so the save slot is
  overwritten with the live value instead of the mode being restored.
  Transcribed as written.
- **`walls.py` had a matching blind spot**, fixed here: `GAP_RE` knew
  "not ported"/"unported"/"deferred" but not "not yet **bit-exact**", so a
  wall over code that IS transcribed but does not yet agree filed as a
  FAITHFUL refusal -- the most dangerous kind of gap classified as a
  non-gap. `docs/WALLS.md` is 17 gaps / 4 faithful after it.
- Gated by `extra/airline_pickmdl-aictest-{td,tdeas,first,otl}` and
  `generated/airline_aictest-td-aicdiff` (where the oracle REJECTS trading
  day -- `nreg` 0 -- and the engine kept it before the parse fix). Mutations,
  each failing a different set: never run the AIC tests 30; never restore the
  design between candidates 10; skip the AIC tests in the post-loop block 10;
  revert the `gtinpt` `pvaic` default 105; narrow the post-loop gate back to
  `lidotl` only 4.

## 53. The SEATS FORECAST decomposition (`ansub3.f:353-678`) -- the `tfd`/`sfd`/ `afd`/`yfd` tables -- PORTED, 13 of 52 specs bit-exact, 39 measurably wrong and asserted as such.

> **SUPERSEDED 2026-07-30 — 46 of 52 now gate.** The "two independent
> families (APPROX / MEAN)" reading below was wrong, and so was the open
> puzzle it describes. Instrumenting the oracle showed `ansub3.f`'s Tramo
> block (`:552-653`) is REACHABLE and rewrites `z` over the forecast span with
> `LOG(TramLin)`; the port conflated that with `extZ`, which the filter
> recursions read. Porting it closed 33 of the 39. The dead ends below are
> still valid and still worth not re-walking — they are now explained rather
> than merely listed. Current state and the two remaining sub-causes:
> `tools/seats_forecast_scouting.md` §3.

 Map: **`tools/seats_forecast_scouting.md`**. The
Burman recursions are continued past Nz, `sigsub.f:1586-1605`'s antilog
applied, and the tables punched; **zero new goldens** (every SEATS spec
already ships them). Two real bugs closed on the way, and one puzzle left
open with its measurements rather than a tolerance.
- **`editor.f:389-401` was unported: an EXPLICIT short `forecast{maxlead=}`
  is RAISED to `max(12,3*Sp)` on a seats{} run.** `gtinpt.f:1151` applies
  that floor only as the DEFAULT when no maxlead was given, so the four
  corpus specs carrying `forecast{maxlead = 12}` alongside `seats{}` ran with
  `nfcst: 12` against the golden's `nfcst: 36`. Silent, because nothing
  downstream of the HISTORICAL decomposition read it -- the forecast tables
  punched 24 rows where the oracle punches 36. Ported at the `parse_spec.cpp`
  tail beside the other two editor rules.
- **The forecast trend's bias factor is `bias1c` where `sigsub.f:1594`
  literally reads `bias3c`, and the reason is a NORMALIZATION degeneracy.** A
  SEATS decomposition is unique only up to a constant log shift between the
  seasonal and the trend, and the bias block absorbs exactly such a shift:
  this port's raw `sc_i` sits `ln(bias1c)` above the oracle's and its raw
  `trend_i` `ln(bias3c)` below, so the HISTORICAL transform lands on the same
  s10-s13 either way (all four gate bit-exact, airline `bias1c` 1.00882 /
  `bias2c` 1.00010 / `bias3c` 1.00893). The FORECAST trend is not built by
  the filter at all -- it is the residual `z - sc - cycle` (`ansub3.f:660`,
  with `ir` identically zero there) -- so it inherits the shift from `sc`,
  one factor of `bias1c`. **The historical tables cannot pin this**:
  `mean(s11/s12) == 1` holds identically for any `bias2c`, which is why it
  was invisible until the forecast span existed.
- **What the goldens' own identities say**, asserted by the gate ON THE
  GOLDENS rather than on the engine: `tfd == afd` to the last digit on the 39
  specs with no transitory component, `afd = tfd*yfd/100` on the
  multiplicative ones that have one, `afd = tfd + yfd` on the additive ones.
- **The open puzzle, and both halves are measured.** On the 39 failing specs
  the error is in the forecast of `z`, not in its decomposition (`tfd` and
  `afd` move together, `sfd` settles exact after the first year, and the
  error grows LINEARLY in the horizon -- a drift). The oracle's saved tables
  imply a `z` equal to the regARIMA `.fct` **to the last bit** (verified on
  the two specs shipping both). But the port's extension is ALSO what makes
  the historical span bit-exact, and that span is NOT insensitive to it: a
  coherent `+3.64e-3` log shift of the whole extension moves `s12` by 8.3e-4,
  and a single-point `1e-6` shift by 7.6e-4 (~-755 per unit log). Both facts
  hold at once and are not yet reconciled. **Dead ends, all measured, do not
  repeat**: extending with the RAW pre-cap MA (makes the historical worse AND
  still misses the forecast), using `ctx.forecasts.trnfct` as the extension
  (breaks the historical), and keeping `bias3c` on the forecast trend. The
  next step is oracle instrumentation of `z(Nz+1)` vs `extZ(Nz+1)` at
  `ansub3.f:660` -- the same technique that closed the CALCFX seeding
  question. The two failing families are independent: `APPROX` (16 specs, a
  near-non-invertible MA capped to the `xl` bound) and `MEAN` (23 specs,
  `imean != 0`, no capping involved -- `th == th_raw` on every one).
- Gated by `tests/parity/test_seats_forecast_tables.py`, which asserts the
  gap list from BOTH sides -- a KNOWN_GAP spec must stay ABOVE tolerance, so
  fixing one fails the test and says to move its row. It cannot rot into a
  silent allowlist. `ofd` (LSEFCD+2) has no golden anywhere in the corpus and
  is deliberately not produced.

