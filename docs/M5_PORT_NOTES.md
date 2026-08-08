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

Derived from the `## N.` headings in this file -- regenerate it the
same way rather than appending by hand. Navigate by searching the
number, e.g. `## 47.` (the titles carry em dashes and backticks, which
make generated anchors fragile, so the list is deliberately link-free).

0. SEATS decomposition core -- the seasonal front
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
54. `aictest.xe*` -- `x11aic.f`'s Easter table + `x11mdl.f`'s verdict -- CLOSED (byte-identical, both arms), and it found `x11regression{aicdiff=}` being discarded.
55. The Picktd-flip corner -- ROOT-CAUSED (not fixed), and the recorded suspect list was wrong.
56. x11aic.f's TRADING-DAY branch -- and the four silent failures found getting to it.
57. Setpri moved ahead of the model stage -- the Picktd-flip corner CLOSED.
58. `ctod` is 1 ulp off on purpose -- the port was already faithful, and now it is pinned.
59. `x11regression{user=}` -- seven arguments parsed and discarded, found while scouting the aictest USER branch.
60. `Kswv==3` -- the tdprior + x11regression-TD route, absent from the port entirely.
61. `x11aic.f`'s USER branch -- the last of the three, and a Census defect that decides it.
62. The two-column user spec -- one hazard settled, one CENSUS DEFECT, and the extreme-value method was being chosen in the wrong place.
63. `x11regression{}` with no trading day and no holiday: the oracle refuses, this engine did not.
64. CB-36 closed -- and the effective regressor type x11ref classifies by is NOT Rgvrtp.
65. gtxreg.f's whole `IF(Nb.gt.0)` block was missing -- and an aictest with no `variables=` is two divergences deep.
66. `x11regression{span=}` was parsed and discarded -- and the flag it promotes has no consumer without a model.
67. The x11regression span narrowing: fit narrow, apply wide -- and the calendar array that is indexed from the BUFFER, not the span.
68. The x11regression span that ENDS early: a pointer mutation, a deliberately narrow Nofpob, and the gate that named its cases by hand.
69. Three gates that could not see their specs -- and the SEATS decomposition that had been 8.1e-7 wrong behind them.
70. The Easter AIC window set was decided in the wrong phase -- and the wall in front of it was guarding the wrong condition.
71. The no-model OLS prior TD -- and a restore that was compensating for a hoist, on the one path that had not made the move.
72. A stand-in for `restor` that restored less than `restor` does -- and turned an x11regression `td` into a prior adjustment.
73. `prterx` -- the Census routine whose name says "print" and whose job is `abend`. An unported error stop, and the class swept to exhaustion.
74. `Grpx(-1)` is not undefined behaviour -- it is documented COMMON aliasing. And measuring that found a live gap the wall was too narrow to cover.
75. Porting the alias (option B) -- and finding that the two halves of this front were never separable.
76. The "auto-AO AICC gap" was neither auto-AO nor an AICC gap -- a holiday-only x11regression skipped its whole prior pass.
77. `x11regression{reweight=}` was parsed and discarded -- and finding its readers turned up a mis-dispatched argument, a missing `regfix()`, and an ordering that silently ate `b=`.
78. Two `chs` gaps with one symptom -- and a comment that had merged them. `slidingspans{}` and `x11regression{}` had never met.
79. `fixx11reg` defaults to YES -- the per-span calendar gap was a parsed-but-unread option, and the fix's partner had already been measured and rejected
80. The stock-trading-day abend: an unguarded refusal, and an `ELSE` that pairs with a different `IF` than everyone assumed
81. The sliding-spans NOTEs: a diagnostic whose whole effect is an ABSENCE, and the channel nobody could read
82. `slidingspans{fixreg=}` was parsed and dropped, and the ssxmdl wall was keyed on the wrong variable
83. `slidingspans{}` — CLOSED bit-exact, and each of the three gaps had a different owner than its note said
84. `Xaicst` and `Xaicrg` — the two x11regression AIC-test values the oracle recovers from its own group TITLES
85. The sliding-spans held-back outliers — a whole `ssmdl` block that was neither ported nor walled
86. `slidingspans{fixreg=(outlier)}` — the wall was one line, and lifting it falsified entry 83's headline
87. `slidingspans{}` + automatic x11regression outliers — one wall, and behind it an unwalled crash

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

> **SUPERSEDED 2026-07-30 — CLOSED, all 52 gate.** The "two independent
> families (APPROX / MEAN)" reading below was wrong, and so was the open
> puzzle it describes. Instrumenting the oracle showed `ansub3.f`'s Tramo
> block (`:552-653`) is REACHABLE and rewrites `z` over the forecast span with
> `LOG(TramLin)`; the port conflated that with `extZ`, which the filter
> recursions read. Porting it closed 33 of the 39. The last 6 were a SECOND
> unreachable-code mistake in the same front: the saved tables come from
> `ansub4.f`'s refold of the deterministic factors, not from
> `sigex.f:3631-3636`, whose punches are dead under `Tramo == 1`. The two
> errors cancelled except for the length-of-month prior, which is why the
> remainder looked like a trading-day bug. The dead ends below are still valid
> and still worth not re-walking — they are now explained rather than merely
> listed. Full record: `tools/seats_forecast_scouting.md` §3.

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


## 54. `aictest.xe*` -- `x11aic.f`'s Easter table + `x11mdl.f`'s verdict -- CLOSED (byte-identical, both arms), and it found `x11regression{aicdiff=}` being discarded.

The last family behind the `aictest.` prefix, and the only one on the X-11
path. The arithmetic was already right -- `x11reg.cpp`'s `x11aic_easter` had
been bit-exact since the x11regression increment, and its `aicc_xe` canaries
matched the golden to the last digit before any of this. **All that was
missing was emission**, which is exactly the missing-key blind spot entry 52's
gate exists to make visible: six keys the oracle wrote, the engine wrote none,
and no gate compared the intersection.

**`aicind` in x11aic.f is not a local.** It does not appear in that routine's
declaration list, so Fortran case-insensitivity resolves it to the COMMON
`Aicind` of `arima.cmn` -- the same slot `easaic.f` writes for the regARIMA
Easter test. Two consequences, both reproduced:
- `x11aic.f:55`'s `aicind=-1` CLOBBERS the regARIMA Easter window on entry,
  including when the easter branch is skipped entirely.
- `x11mdl.f:280` reads that same slot back for `aictest.xe.window`, which is
  why the key is consistent despite being written by a different routine than
  the one that computed it.

**One key, two FORMATs.** `aictest.xe.window` goes through `(a,i3)` when the
Easter is accepted (`aictest.xe.window:  15`) and through a literal string
when it is rejected (`aictest.xe.window: 0`). Same key, one space instead of
three. A numeric comparison sees nothing; only the text diff does. This is the
third instance of the same shape in this block, after `easaic.f`'s colon and
the unprefixed `testalleaster`.

**The reject arm was unreachable, and that is how the real bug surfaced.**
The corpus covered only the accept arm, so the literal-string format had no
golden. Building a spec to reject meant raising the threshold -- and
`x11regression{aicdiff=}` turned out to be token-consumed and written nowhere
(`gtxreg.f:513-518` sets `Xraicd`; the port's `gt_x11regression` had it in the
"deferred" bucket). The default is `ZERO` on both sides (`gtinpt.f:477`), so
every existing spec agreed and nothing failed: the classic parsed-but-unread
shape, invisible until something needed a non-default value. Wiring it is four
lines. Proof it is now READ rather than merely parsed: two specs differing
*only* in `aicdiff=5.0` produce different engine verdicts (`xe: yes` vs
`xe: no`).

**Span-replay seam, again.** `x11aic_easter` runs from inside x11pt2 and
appends to `ctx.x11reg_aicc_xe`, so a `slidingspans{}`/`history{}` replay
would have reported the LAST SPAN's table. The oracle is immune by a route the
port cannot copy: `x11mdl.f:291` sets `Xeastr=F` after the first test, but the
port's `editor.f:1734-1757` re-derivation of `Otlxrg` reads that same flag
later, so clearing it would break a different thing. Handled at both ends
instead -- the table is cleared at the top of the test, and the three fields
joined `run_x11.cpp`'s save/restore set. Fifth time this seam has bitten.

Gated by `tests/parity/test_aictest_savelog.py`, which now runs two harnesses:
`x13run_m3` for the svaict/AICC keys and `x13run_x11` for `xe`. Mutation-
tested both ways -- perturbing one AICC digit fails, and collapsing the accept
arm's two spaces to one fails.

## 55. The Picktd-flip corner -- ROOT-CAUSED (not fixed), and the recorded suspect list was wrong.

Still walled, but no longer a mystery. The gap: a `pickmdl{}` run whose
candidates DISAGREE about trading day comes back with d11/d13 wrong on
Februaries only -- `engine = oracle * (days-in-Feb / 28.25)`, i.e. 0.885% low
on non-leap and 2.655% high on leap -- while d10, d12 and d16 stay bit-exact
and all 52 shared `.udg` model keys agree.

**Reaching it at all** needs `regression{aicdiff=}` tuned between two
candidates' AICC gaps. On `airline_pickmdl-aictest-td` those gaps are 18.33
18.82 18.85 18.49 20.20, so `aicdiff=19.0` splits them: candidate 5 accepts
trading day, the winner (candidate 2) rejects it, and `automx.f:259-296`'s
Picktd restore fires. Without a tuned aicdiff every candidate agrees and the
branch is unreachable.

**The old suspect list was wrong.** The previous note named `Kfmt` / `Lpradj` /
`Priadj` as the remaining candidates. Probing both engine and oracle at
x11pt1/x11pt3 shows all three IDENTICAL on this spec (`kfmt=1 priadj=4
lpradj=1 setpri=1`). The value that disagrees is **`Sprior`**: the oracle
reaches x11pt3 with `Sprior=0.99115` and `Adj=1.0`; this port has both at 1.0.
The `Sprior`/`Setpri` deferral had actually been recorded as RULED OUT, and it
is the whole cause.

**The chain**, traced with an instrumented build of the oracle Fortran -- a
scratchpad copy, since the vendored tree is never edited:

1. The oracle sets `Setpri` at editor time and issues x11int.f:53's
   `Adj -> Sprior` copy from **x12run.f:174**, before `arima`/`automx`.
2. `tdaic.f:600-623` then writes `Sprior` DIRECTLY when Picktd transitions
   during model selection. Nothing afterwards refreshes it from `Adj`.
3. This port assigns `ctx.adj.setpri` only in `x11_prestage`, AFTER the model
   stage -- so tdaic's write is skipped by its own `setpri >= 1` guard. The
   "deferred prior-series bookkeeping" comment in `aictst.cpp` was describing
   code that never executes.
4. The port compensates by copying `Adj -> Sprior` in the POST-model `x11int`.
   That agrees with the oracle exactly when `Adj == Sprior` at that point --
   true on every gated spec.
5. Here it stops holding. The Picktd restore puts `Adj` back to its entry value
   (all-1) while `Sprior` must keep the prior tdaic wrote.

**Confirmed from the other side**, which is what makes the diagnosis complete
rather than plausible: on the automdl baseline the oracle reaches x11pt3 with
`Sprior=1.0`, `Adj=0.99115` and `Priadj=-4`. Trading day survived into the
final model there, so x11pt2's `tdlom` CONSUMED the prior into `Factd`, negated
`Priadj` and reset `Sprior` to identity. On the flip spec TD does not survive,
`tdlom` never runs, and `Sprior` is applied directly. Two specs, opposite
routes, one explanation.

**Attempted and reverted.** Suppressing the post-model copy alone moves the
same 2.655% error onto the automdl baseline, because tdaic's write is still
dead -- the compensation was load-bearing. The real fix is to establish
`Setpri` (and the span pointers it derives from) BEFORE the model stage, as the
oracle does, after which tdaic's write runs and the copy must be suppressed.
That is a driver-ordering change, not a local one, and it is not worth risking
the spine for one walled corner; the wall in `automx.cpp` now carries the whole
chain so the next attempt starts from the answer.

**Two standing rules this earns.** First, the port's post-model `x11int` is a
COMPENSATION for an ordering difference, not a transcription -- anything that
makes `Adj` and `Sprior` diverge across the model stage will surface as wrong
Februaries with an `OUTCOME: OK`. Second, and more general: *a guard whose
precondition is never satisfied looks exactly like a ported branch.* The
`setpri >= 1` test reads as faithful, the comment above it claimed the work was
deferred, and nothing distinguished "this runs and does nothing" from "this
never runs" until the oracle was instrumented. Same family as the saturated
mutations recorded in entry 52.

## 56. x11aic.f's TRADING-DAY branch -- and the four silent failures found getting to it.

`x11regression{aictest=(td|tdstock|td1coef|tdstock1coef)}` now runs, emits
`aictest.xtd.{aicc.notd,aicc.td,reg}` + `aictest.xtd`, and is gated on both
verdict arms plus the two-test configuration. **12 keys over 3 specs, all
byte-identical.** The Easter branch had been ported alone since the
x11regression increment; this closes the routine bar its USER branch.

**Start with what was wrong, because none of it announced itself.**

1. **The `aictest=` token was parsed and DISCARDED.** `readers_spec.cpp`'s
   argidx-19 branch handled only `easter` and carried a comment saying td/user
   were "deferred". `Xtdtst` and `Xuser` are declared in the generated COMMON
   headers and were **written nowhere in the engine**. A spec asking for the
   trading-day test therefore ran the plain fixed-TD path and returned
   `OUTCOME: OK`. Third instance of the parsed-but-unread shape in this block
   after `regression{aicdiff=}` and `x11regression{aicdiff=}`.

2. **`gtinpt.f:468-469`'s two defaults were missing** -- `Xaicrg = NOTSET` and
   `Xaicst = 31`. Not cosmetic: both `addtd.f` and `mktdlb.f` test
   `Aicrgm(1).ne.NOTSET` to decide whether a change-of-regime date was given,
   and the struct's zero-init makes that test TRUE. A plain `aictest=(td)`
   would have built the two-regime TD group and labelled it `td/<date>/`.
   Found by reading the callee, not by a failure -- the same class as the
   `Pvaic`/`DNOTST` default from the pickmdl increment.

3. **`xrgtrn_td` had the `Tdgrp>0` arm hardcoded.** `xrgtrn.f:36-44` takes a
   DIFFERENT arm with no trading-day group in the model: the irregular is only
   centred (`X-1`), not rescaled by the day counts (`Xnstar*X - Xn`). Every
   x11regression spec until now carried a fixed `variables=(td)`, so the
   assumption held and nothing had ever needed `Tdgrp` -- which is also why
   the port had never derived it. `editor.f:1618-1627`'s group-pointer block
   (`Tdgrp`/`Stdgrp`/`Holgrp`) is now transcribed into `x11mdl_td`. This one
   DID show up as a number: the no-TD AICC came out +150.85 against the
   oracle's -771.73.

4. **`x11mdl.f:308-381`'s no-regressors-left early return was absent**, and
   this is the real find. If the AIC tests leave NO trading-day, stock-TD or
   holiday group in the irregular-regression model, the oracle prints a NOTE,
   punches IDENTITY factors for the requested factor tables, clears
   `Axrgtd`/`Axrghl`/`Axruhl` and **RETURNS** -- skipping the OLS, the factor
   build and the divsub. This port ran on and fitted the empty design,
   producing a c16 carrying the length-of-month prior where the oracle writes
   1.0. Only the TD reject arm can reach it, which is why it survived the
   whole x11regression front.

**And the mutation that PASSED, which is how the gate got fixed.** With that
early return deleted the ENTIRE suite stayed green. Cause:
`test_x11regression_tables.py`'s `AIC_CASES` was a hand-written one-element
tuple naming the Easter spec, so none of the three new specs' b16/c16/xrm were
ever compared. Now auto-discovered from the corpus with a floor assertion; the
same mutation then fails with the familiar 2.655e-02 leap-February signature.
*A hand-maintained case list is an allowlist that silently stops growing* --
the list-shaped twin of the missing-key blind spot from entry 54.

**A gate-semantics correction, and it is a narrowing not a loosening.**
`test_m1_parse.py::_oracle_ok` read "oracle exit code != 0" as "the oracle
rejected this spec". The reject spec is the first case where those differ: the
oracle exits 2 having parsed the spec, run the model and written a complete
`.udg`, purely because x11mdl wrote a NOTE. `_oracle_ok` now treats a nonzero
exit as a rejection only when NO `.udg` was produced. The one genuine case,
`census-examples/composite/total` (exit 3, SIGFPE), has no `.udg` and still
classifies as a rejection.

**A Census bug, measured but NOT claimed as a CB entry.** `x11mdl.f:378` and
`:883` write `'finalxreg01: none'` through FORMAT 1060, which in that scope is
the WEEKDAY-COLUMN HEADER `('         Mon      Tue ...')` and carries no data
descriptor -- so the string is dropped and the `.udg` gets a stray column
header where the key should be. Visible in this spec's own blessed golden. Not
claimed, because the engine emits the whole `nfinalxreg`/`finalxreg01` family
on NO path -- accept arm included -- so there is nothing yet to reproduce it
against. That family is a pre-existing missing-key gap, recorded here so the
next person porting it starts from the answer.

**Two structural notes worth not re-deriving.**

`Xtdtst` is the XAICDC TOKEN index (td/tdstock/td1coef/tdstock1coef = 1/2/3/4)
and `addtd`/`mktdlb` take the WIDER regressor index, which also numbers the two
`nolpyear` variants -- x11aic.f:79-82 remaps 4->6, 3->4, 2->3 with three
sequential IFs. `x11mdl.f:153-155` does the SAME remap with only two of them,
having no `tdstock1coef` case to handle.

The two tests are COUPLED. When trading day is accepted, `x11aic.f:242` sets
`estend=F` and `:244` copies `aictd` into `aichol`, so the Easter loop's first
iteration skips its own estimation and reuses that value. The observable is
that `aictest.xe.aicc.noeaster` is bit-identical to `aictest.xtd.aicc.td` on a
two-test spec -- confirmed in the oracle's own golden, and the reason
`extra/airline_x11regression-aictest-tdeas` exists rather than relying on the
two single-test specs. x11aic.f:243-247 has a dead `ELSE IF(Xeastr)` where the
surrounding code plainly wanted `ELSE IF(Xuser)`; transcribed as written, and
not claimed as a CB entry because the user branch is unported and the port
cannot measure the difference.

**Still unported here:** the USER branch (`x11aic.f:462-591`), which strips the
`Ncusrx` user columns, scores without them and restores them through seven
`adrgef` arms by `Rgvrtp`. The parser now REFUSES `aictest=(user)` rather than
dropping the token -- the mistake this entry opens with. Note that a parse-time
refusal via `inpter` is NOT inventoried in `docs/WALLS.md`, which scans only the
`*_not_ported`/`fatal` helpers; that is a pre-existing blind spot in walls.py,
shared with e.g. the `transform mode=diff` refusal.

**Mutations, four, each failing a DIFFERENT set:** drop the `Xtdtst` parse
**4**; hardcode `xrgtrn_td` back to the TD arm **4**; swap the two emitted AICC
values **3**; delete the x11mdl early return **1** (and **0** before the gate
was widened -- that is the entry above).

## 57. Setpri moved ahead of the model stage -- the Picktd-flip corner CLOSED.

Entry 55 root-caused this and stopped there, on the grounds that a driver-
ordering change was not worth risking the spine for one walled corner. The
change turned out to be small and, across the whole gated corpus, exactly
neutral: **5932 -> 5944 passed with the only deltas being the twelve gates the
new spec adds.** The wall in `automx.cpp` is gone (17 gaps -> 16).

**What moved.** `editor.f` sets `Setpri=Pos1bk` at :851 and `x12run.f:174`
issues `x11int.f:53`'s `Adj -> Sprior` copy -- both BEFORE `x11ari`, hence
before `arima`/`automd`/`automx`. This port assigned `Setpri` only in
`x11_prestage`, which runs AFTER the model stage. Three Sprior writes that the
model stage makes (`tdaic.f:600-623`, `rmlpyr.f:59`, `pass2.f:101`) are
therefore skipped by their own `Setpri >= 1` guards, and a post-model copy in
`x11int` stood in for all of them. Now: the editor geometry is factored into
`x11_editor_geometry` (`driver/x11_prestage.cpp`), `run_pre_model` calls it
just ahead of the model stage and issues the Sprior copy there, and
`x11_prestage` calls it again -- which is `x11ari.f:149`'s second `setxpt` --
but does NOT re-assign `Setpri`, because the oracle never refreshes it. The
post-model copy is suppressed on the model path via `x11int(ctx,
copy_sprior=false)`.

**Placement inside the pre-model stage is load-bearing, and not where entry 55
implied.** The copy cannot go beside the `/adjcmn/` record where `Adj` is
built: `trnaic` (the automatic transform selection, `x11ari.f:81`) REWRITES
`Adj` wholesale afterwards, and the oracle re-issues the copy after it at
`trnaic.f:278` -- a line this port does not have. Issuing it after trnaic
instead covers both. The geometry does not care where it goes: it is a pure
function of the span and the forecast/backcast counts.

**The corner itself.** `extra/airline_pickmdl-aictest-tdflip` -- the probe spec
entry 55 described, now committed: `airline_pickmdl-aictest-td` plus
`regression{aicdiff = 19.0}`, which splits the five candidates' AICC gaps
(18.33 18.82 18.85 18.49 20.20) so candidate 5 accepts trading day and the
winner, candidate 2, rejects it. `automx.f:259-296`'s Picktd restore then fires:
`Adj` goes back to all-1 while `Sprior` must keep what tdaic wrote. Twelve
gates, all byte-identical, including d10-d13/d16 through the binding.

**Both mutations fail, and they fail DIFFERENTLY** -- which is the point of
running both halves. Restoring the post-model copy (`copy_sprior=true`) and
suppressing the pre-model `Setpri` (`set_setpri=false`) each break the same
five parity gates, but the second ALSO breaks a ctest unit test. So the two
halves are not interchangeable descriptions of one switch: the port needs
`Setpri` correct during the model stage for reasons beyond this one Sprior copy.

**A spec-authoring trap worth not re-learning.** `x11{save=(b1 ...)}` is
rejected by the oracle ("Save argument is not defined"), so the b1 golden the
`test_x11_tables` discovery requires cannot be produced for a spec like this
one -- it gates through `test_bindings` (discovery keyed on d11) instead. Worse,
blessing does not notice: `run_parity.py --update` reported `PASS` and wrote a
golden bundle from the REJECTED run, and only `test_m1_parse`'s outcome gate
caught it. Bless, then look at the `.err` in the bundle.

**Standing rule this sharpens.** Entry 55 called the old arrangement "a
saturated precondition, not a proof" and left it. The cost of leaving it was
that four correctly-transcribed Fortran writes sat inert behind guards that
read as faithful. *When the fix for a saturated precondition is known, the
saturation is the bug -- the walled corner is only how it was noticed.*

## 58. `ctod` is 1 ulp off on purpose -- the port was already faithful, and now it is pinned.

Board item: "`gtdpvc` parses decimal literals 1 ulp off the nearest double
(`"0.95"` -> 0.95000000000000007 vs 0.94999999999999996). Latent everywhere a
spec supplies a decimal. Check whether the Fortran reader does the same before
changing anything." **It does.** No engine change; a unit test instead.

**The answer is in `ctod.f`, not `gtdpvc.f`.** `gtdpvc` only dispatches;
`getdbl` calls `ctod`, and `ctod` is a hand-rolled digit accumulator. The
fraction loop is

    scl = scl * 10D0
    ctod = ctod + dble(digit)/scl

-- each digit's OWN quotient, added in. Not `strtod`, and not "accumulate a
mantissa, divide once": the result carries the rounding error of every
intermediate division. `0.95` becomes `0.9 + 0.05`, whose exact sum
0.95000000000000002498... sits 4.16e-17 above the upper neighbouring double and
6.94e-17 below the lower, so it rounds UP -- to exactly the value this port
produces. `core/src/specparse/util.cpp`'s `ctod` is a verbatim transcription,
so the port has been right all along.

**Scope, measured over the corpus rather than asserted.** Of the 286 distinct
decimal literals in `tests/corpus`, **52 differ from the correctly-rounded
double, always by exactly 1 ulp, in BOTH directions** (34 up, 18 down). The
heavy users are `regression{b=}` coefficient lists (the -0.0045/-0.0135/...
family) and user-regressor data, which flow straight into arithmetic. Literals
that are dyadic rationals -- `3.5`, `19.0`, `1.96`, `0.25` -- are untouched,
which is why this never showed up as a parity failure.

**It IS gated, and that is the uncomfortable part.** Swapping the accumulator
for a correctly-rounded conversion breaks **exactly one** parity gate:
`generated/airline_user-reg-x11.d9a.05`, at the 10th significant digit
(`0.2373205748E+00` vs `...47E+00`). One key, one row, one spec, out of 5944
tests. A single accidental canary is not a guardrail -- it is a coincidence that
happens to be load-bearing, and the next person to "clean up" a hand-rolled
parser would get a green suite on the first try and a red one on the second.

So: `tests/unit/test_ctod.cpp` (ctest 11/11 -> 12/12). Four tests -- the oracle
bit patterns, the 1-ulp property stated directly, the dyadic literals that must
NOT move, and `havdbl`'s no-digits-consumed contract, which is what stops a
non-numeric token from silently reading as 0.0.

**Two methodological notes worth keeping.**

*The expected values come from the ORACLE, not from a re-transcription.*
`tools/ref_ctod.f` compiles against the vendored `oracle/fortran/ctod.f` (the
vendored tree is read, never edited) and prints each double in Z16 hex. Deriving
them from a second reading of the same Fortran -- which is what the Python probe
that found the 52 literals was -- would have proved only that I read it the same
way twice. The probe and the driver agree on all 12 literals, which is the
check, not the source.

*The test compares BIT PATTERNS, and has to.* Writing `CHECK_EQ(got, 0.95)` in
C++ compares against the compiler's own correctly-rounded parse -- i.e. against
the very value the oracle does NOT produce. The test would have failed for the
right reason and been "fixed" by loosening it.

**And the mutation was re-run against the new test**, per the standing rule that
a guardrail must be shown able to fail: 11 of its 24 checks fail, naming the
literals. Before it existed the same mutation cost one 10th-digit assertion in
a d9a row.

## 59. `x11regression{user=}` -- seven arguments parsed and discarded, found while scouting the aictest USER branch.

Board item 1 was "port `x11aic.f`'s USER branch (`aictest.xu*`)", whose stated
precondition was "needs a spec with `x11regression{user=}` first". Writing that
spec is what found this: **`user=`, `data=`, `start=`, `file=`, `format=`,
`b=` and `usertype=` all fell through `gt_x11regression`'s
`else { consume_value(ctx, nullptr); }`** -- accepted and thrown away.

**The measurement, on `extra/airline_x11regression-user` (new, hand-authored).**
One user column, a 0.05*cos(9k degrees) 40-month cycle deliberately NOT
commensurate with the 12-month seasonal so the seasonal factors cannot absorb
it, alongside `variables=(td)`:

| | oracle | engine (before) |
|---|---|---|
| `xrm` columns | **7** (Mon..Sat + u1) | **6** |
| d11 1949.01 | 123.152974066993 | 123.113261 |
| d11 1949.02 | 124.368456029538 | 124.712176 |

`OUTCOME: OK` throughout. The user column was simply absent from the irregular
regression. Textbook parsed-but-unread -- the fourth instance in this block
after `regression{aicdiff=}`, `x11regression{aicdiff=}` and
`x11regression{aictest=}`.

**What was actually missing was only the PARSE.** The downstream plumbing was
already there: `loadxr` moves `Ncxusx`/`Nrxusx`/`Usxtyp`/`Usrxtt`/`Xuserx` into
the working regression slots, and `regvar` builds the columns from them. So the
port is `gtxreg.f`'s argument arms plus its :607-800 tail -- the `b=` writeback
over `Nb + Ncxusx`, the `hvuttl`/`haveux` and coverage checks, the `adrgef`
loop, and the `centeruser=` mean/seasonal-mean removal.

**Three things worth not re-deriving.**

*`gtxreg.f`'s adrgef dispatch has FOUR arms, not `getreg.f`'s sixteen* --
Holiday / Trading Day / AO / default. The irregular-component regression has no
seasonal, constant, LOM, LOQ, leap-year, LS, SO or transitory user types.

*`start=` writes `Bgusrx`* -- the SAME slot `regression{start=}` writes. The
oracle shares one begin-date between the two specs' user matrices, so a spec
cannot give them different start dates. Transcribed as written.

*A pre-sized string is load-bearing.* `gtnmvc` writes through `putstr`/`insptr`,
which bound the write against `chrvec.size()`; a default-constructed
`std::string` therefore ABENDS rather than growing. The symptom was a bare
`OUTCOME: FATAL` with no message -- the parse fatals before anything can print
one. `usrxtt` is now sized `PUREG * PCOLCR`, matching `usrttl`.

**The second bug, and it is the more interesting one: a state leak from a
transparent pass.** With the parse fixed, the engine emitted an `outlier.user`
savelog key the oracle does not. Cause: `xrgdrv`'s transparent prior-TD pass
calls `loadxr(F)`, which copies the x11reg store's user columns into the
regARIMA slots (`Ncusrx`, `Usrtyp`, `Usrptr`, `Usrttl`, `Nrusrx`), and
`loadxr(T)` does **not** put them back -- it saves only the working MODEL. The
oracle is immune because `xrgdrv.f:207`'s `restor()` reloads the whole regARIMA
store; this port's `restor_span` resets only the x11 filter state. So `Ncusrx`
stayed at the x11reg value for the rest of the run and `savotl.f:150`'s
`IF(Ncusrx.gt.0)` guard fired.

Exactly the class already recorded for `Lterm`/`Ksdev` in this same routine:
*a transparent pass that writes state its own restore does not cover.* Third
time in `xrgdrv` alone. The user-regressor slots now join its save/restore set.

**Mutations, both halves, failing at very different volumes:** suppress the
user-column build (i.e. restore the old discard) **9 gates**; drop the `Ncusrx`
restore **1 gate** -- and that one gate is `test_check_diagnostics`, which
compares the .udg key SET, not values. Without a savelog key-set comparison the
leak would have been invisible; it is the same missing-key discipline entry 54
bought.

**Still open, and this entry does not touch it:** `x11aic.f:462-591`'s
`aictest=(user)` branch, which strips the `Ncusrx` columns, scores without them
and restores them through seven `adrgef` arms by `Rgvrtp`. The parser still
refuses that token. What this entry delivers is its precondition.

**Measured but NOT claimed as a CB entry.** `gtxreg.f:274` maps
`usertype=ao` to `PRGTAO` (13, the plain regARIMA AO type) where `getreg.f`
maps its `ao` to `PRGUAO` (61) -- and the adrgef dispatch at :740 tests for
`PRGUAO`, so an `ao` column falls through to the default arm and is titled
'User-defined' with type `PRGTUD`. The same line also sets `Havxtd`, marking the
run as carrying trading day because of an AO column. Both read as defects.
Neither is claimed, because no spec here exercises
`x11regression{usertype=ao}` yet and the rule is to measure before naming one.
Transcribed verbatim, with the reasoning in the code.

## 60. `Kswv==3` -- the tdprior + x11regression-TD route, absent from the port entirely.

`x11pt1.f:235` is one line: **`IF(Axrgtd)Kswv=Kswv+2`**. It was not ported, so
`Kswv` could never leave 1 and the four things keyed on 3 were all dead. Found
by writing the spec the scouting for entry 59 had said was needed
(`extra/airline_x11regression-tdprior-td` -- prior weights AND
`variables=(td)`); the four existing tdprior specs all omit the TD model, so
none of them reaches the bump.

**Measured, at `OUTCOME: OK`:**

| d11 | oracle | engine (before) |
|---|---|---|
| 1949.01 | 123.163850659787 | 123.516344 |
| 1949.02 | 124.849918528751 | 126.768974 (**1.5e-2**) |

**The only route to Kswv==3** is user prior weights (Kswv==1, set at
`editor.f:1502` when any `tdprior` weight is nonzero) TOGETHER WITH `Axrgtd`,
which `gtxreg.f` sets from `x11regression{variables=(td)}` or `aictest=(td)`.
Four consumers, and the port had none of them:

- `xrgtrn.f:36-40` -- the irregular transform becomes `Xnstar*X - Xnstar`, i.e.
  centred against the STANDARD month length rather than the actual one.
- `x11ref.f:117-119` -- the factor finish adds `1` instead of `Xn/Xnstar`.
- `x11mdl.f:541-572 + :786-830` -- the estimated coefficients become X-11 style
  daily weights `Dx11`, are ADDED to the user's prior weights
  (`Dx11 = Dx11 + Dwt - 1`), and Faccal/Factd are REBUILT from the sum. Note the
  rebuild passes `Kswv=4`, not 3, so `x11ref` deliberately takes its ordinary
  arm for the combined factor.
- `x11pt2.f:408-412` -- multiplies the raw Series FORECAST region by Stptd once
  per iteration. Left unported on purpose: it touches only
  `[Posfob+1, Posfob+Ny]`, never the published span, which is why it has never
  shown up. Recorded here so the next reader does not re-derive it.

**Kswv is deliberately NOT restored after `xrgdrv`.** `xrgdrv.f:57/207` call
`ssprep`/`restor` with `Lx11rg=F`, so the bump that xrgdrv's own transparent
x11pt1 makes SURVIVES into the main run -- which is precisely why the main
x11pt1 then finds `Kswv/=1`, skips the prior-TD block, and takes the `Ixreg==3`
Faccal restore instead. The span drivers DO restore it (`x12run.f:166`'s
`ssprep(...,T)` -> `ssx11a.f:160` / `revdrv.f:528`), which is why the parsed
value is snapshotted as `ctx.saved.kswv0` and reset per span in run_x11_span.
Snapshotting it in `ssprep_snapshot` would have been wrong: that runs AFTER
run_pre_model, and xrgdrv has already bumped it by then.

**The bump alone was not enough, and the second half is the more instructive
one.** With Kswv==3 live, `a4` and `b16` came back bit-exact but `c16` was 5e-3
out: the C-iteration daily weights were visibly different (Tue -0.276 vs the
oracle's -0.326). The cause was a SHORTCUT this port had taken in x11pt2 and
verified -- correctly, at the time:

```
// x11pt2.f:846-894 rebuilds Stcsi from the RAW Series; on the TD-only corpus
// path that is exactly Sto/Faccal, so the STCSI=STO shortcut is bit-equivalent
```

It stops being bit-equivalent the moment a tdprior exists. x11pt1 divides `Sto`
by the prior-TD factor **and** folds the same factor into `Faccal`, so
`Sto/Faccal` removes the prior TD TWICE. The oracle never has that problem
because it rebuilds from `Series`. The shortcut is now the real transcription
(with the six unported per-factor divsubs walled), and the equivalence argument
is recorded in the code as the thing that failed.

*The general shape, and it is worth naming:* **an equivalence that holds over
the corpus is an equivalence over the corpus, not a proof.** This one was
explicitly verified when written; what invalidated it was a feature that did not
exist yet. Same class as entry 25's unreachability proof, which was invalidated
by an option the parser was silently discarding.

**A third half: ordering in run_pre_model.** `x11ari.f` calls `xrgdrv` (:99)
before `x11pt1` (:133), so by the time the oracle's x11pt1 looks at `Kswv` it is
already 3 and the bare prior-TD divide never happens. This port had the two
blocks in the opposite order and divided the estimation input by the prior TD
AND by the combined Faccal. Swapped; the `kswv == 1` guard then does the work by
itself.

**Mutations -- every piece is load-bearing, and the volumes are informative:**

| removed | gates lost |
|---|---|
| the `Kswv+=2` bump | 10 |
| the `Dx11 + Dwt - 1` combine | 10 |
| `xrgtrn`'s Kswv==3 arm | 10 |
| `x11ref`'s Kswv==3 arm | 10 |
| the Stcsi rebuild (falls back to Sto with no Faccal divide) | **193** |
| the run_pre_model reorder | 10 |

The 193 is the interesting one: the Stcsi branch is load-bearing for every
x11regression spec in the corpus, not just this one -- it had simply never been
exercised in a configuration where the shortcut and the transcription disagree.

**Two more parsed-but-unread arguments, found on the way.** `gtxreg.f`'s
`forcecal` (argidx 24 -> `Calfrc`) and `reweight` (argidx 32 -> `Lxrneg`) both
fell through `gt_x11regression`'s discard arm. `forcecal` is now honoured,
because leaving it unread would have made the two `Calfrc` walls this entry adds
unreachable -- a wall that cannot fire is not a wall. **`reweight` is still
unread and is left open**: `Lxrneg` is READ in two ported places
(`gtinpt.cpp`'s negative-weight clamp, `run_history.cpp`'s fixreg check), both
of which therefore see a permanently-false flag, and honouring it also needs
`x11mdl.f:575-626`'s daily-weight reweighting, which is unported. That is a
separate increment, not a line.

## 61. `x11aic.f`'s USER branch -- the last of the three, and a Census defect that decides it.

Board item 1 since the aictest front opened. Its stated precondition
(`x11regression{user=}` parsing and gating, entry 59) and its measured
prerequisite (`Kswv==3`, entry 60) were both closed first, so this was
transcription plus measurement rather than discovery.

**What it does.** `aictest=(user)` sets `Xuser`. x11aic then drops `Ncusrx` to 0
and strips every user-defined column (`:66-75`, `:126-137`), scores the model
without them, restores them through SEVEN `adrgef` arms keyed on the saved
`Rgvrtp` (`:496-521`), scores again, and keeps whichever wins by `Xraicd`. The
reject arm deletes the `User-defined` group and zeroes `Ncusrx`/`Ncxusx`/
`Nrxusx`. Savelog: `aictest.xu.aicc.nouser` / `.user` from x11aic, the
`aictest.xu` verdict from `x11mdl.f:293-305` -- read off the MODEL, not a flag,
exactly like the Easter one.

**CB-35, and it is `active`.** `aicnus` -- the no-user AICC -- is an
uninitialized local with three routes to a value:

| route | when |
|---|---|
| `:470` computes it | `estend` still true at `:463` |
| `:457` seeds it from the winning Easter AICC | the Easter test ran |
| **nothing** | TD accepted, no Easter test |

The third exists because `:245` reads `ELSE IF(Xeastr)` after `:243` already
tested `Xeastr` -- unreachable; the intent was plainly `ELSE IF(Xuser)`. The
vendored -O2 oracle reads **exactly 0.0**, and the verdict is
`aicusr + Xraicd < aicnus`, so **any negative AICC wins and the user regressors
are accepted unconditionally**. Measured -785.9 accepted against a 0.0 that
means nothing. The 0.0 survives a changed ARIMA model. Reproduced by
initializing to 0.0, with the caveat in the code that this is one build's stack
value; the gate is what pins it.

Three specs, one per route, and that is the point of having three:
`-aictest-tduser` (uninitialized), `-aictest-tduser-reject` (`:470`, and the
reject half of the verdict), `-aictest-easuser` (`:457`). All three bit-exact.

**Two transcription hazards in the strip loop, both reproduced, neither
claimed.**

*The read is after the delete.* `:129` calls `dlrgef` and only THEN reads
`B(icol)` -- but `dlrgef.f:75-77` shifts `B`/`Rgvrtp`/`Regfx` down over the
hole, so `B(icol)` is the FOLLOWING column's coefficient. It is harmless only
when the user column is last (the shift copies zero elements and the slot keeps
its old value), which is the only case the corpus has.

*The index orders disagree.* The strip loop counts DOWN, filling `bu2`/`fx2`/
`typ2` in descending-column order; the restore reads them `1..Ncusrx` alongside
`getstr(Usrttl, ..., i)` in ASCENDING title order. With two or more user columns
coefficients pair with the wrong titles.

Both need a MULTI-column user spec to show. No corpus spec has one, and the rule
is to measure before naming a Census bug, so both are comments rather than CB
entries. A two-column spec would settle them and is cheap.

**A divergence NOT shipped, deliberately.** `x11regression{ variables=(td)
aictest=(user) }` -- a fixed trading day with only the USER test -- makes the
ORACLE abend at the C iteration: `ERROR: Irregular regression matrix singular
because of Mon`, with the printed design matrix carrying its six column headers
and ZERO rows. This engine runs it to `OUTCOME: OK`. The likely mechanism is
that the B iteration's `adrgef` restore and the following `regvar` each
contribute a copy of the user column into the model `loadxr(T)` saves, so the C
iteration fits a duplicated design -- but that is a hypothesis, not a
measurement. The spec was written, measured, and then REMOVED rather than gated:
a wall would have made the outcomes agree for a reason unrelated to the oracle's
own, and would mask a genuine singularity if the port ever grew one. It is on
the open board with the measurement attached.

**Mutations:**

| removed | gates lost |
|---|---|
| the strip of the user columns | 30 |
| the whole USER branch | 15 |
| `aicnus = 0.0` -> a sane sentinel (CB-35) | 13 |
| `:457`'s Easter seed | 13 |
| the seven-arm `adrgef` restore | 15 |
| the reject arm's `dlrgef` | 16 |

**The gate caught its own bookkeeping.** `test_aictest_savelog.py` classified
`aictest.xu*` as UNOWNED -- correct while the parser refused the token. The
moment the engine started emitting the keys, the both-directions key-set assert
failed in the *extra-in-engine* direction, which is the half that usually looks
redundant. Moved to `OWNED_X11`.

## 62. The two-column user spec -- one hazard settled, one CENSUS DEFECT, and the extreme-value method was being chosen in the wrong place.

Board item 1 was billed as confirmation work: write a spec with TWO
`x11regression` user columns of different `usertype=`, decide whether entry
61's two strip-loop hazards are Census bugs, and move on. It cost an increment,
because the spec walked straight into a live wrong-numbers path that had
nothing to do with x11aic.

**Hazard 1 (`x11aic.f:129` reads `B(icol)` after `dlrgef`) is UNREACHABLE, not
benign-by-luck.** `dlrgef.f:75-77` copies `noldc-1-endcol` elements, which is
ZERO when the deleted column is the last one, and the slot then keeps its own
value. The user columns are ALWAYS the trailing block: `gtxreg.f:183` adds the
`variables=` regressors inside the argument loop and `:733-743` appends the
user columns after it, and every x11aic path that puts them back (`:496-521`)
appends them last again. The strip loop counts DOWN, so each user column is the
last one at the moment it is deleted. Not a defect. The comment now says so.

**Hazard 2 IS a Census defect, but not the one that was written down.** The
`bu2/fx2/typ2` order really does disagree with the ascending title read -- but
what that produces is CB-36's neighbour, and the mechanism that MATTERS is one
line earlier, in editor.

### CB-36, and what it cost

`editor.f:1690-1716` walks the parsed x11reg columns to see whether a user
column supplies the trading-day or the holiday group. `rtype` is assigned ONLY
inside the `Rgxvtp(icol).eq.PRGUTD` arm and READ only in the `ELSE IF` -- so on
the columns the read happens for, it is either uninitialized or holds the last
user-TD column's `usertype=`. The test is `rtype.ge.PRGTUH`, and PRGUTD (57)
clears that bar while PRGTUD (18) does not.

So `usertype=(td user)` makes the SECOND column the holiday group, and
`usertype=(user td)` makes nothing the holiday group. Two specs identical but
for that one line:

| declared | Holgrp | extreme-value method | AO columns |
|---|---|---|---|
| `(user td)` | 0 | Sigxrg=2.5, tdxtrm clip | 0 |
| `(td user)` | 2 | Otlxrg, idotlr | 7 |

Every row of b16/c16/d10/d11 differs between them. A genuine
`usertype=holiday` column, the case the arm was written for, never fires it.

### The port gap the probe exposed

Three separate things, all of the parsed-but-unread family:

1. **`Nusxrg` was a LOCAL.** `gtxreg.f:265` writes the COMMON; this port
   declared `int nusxrg` in `gt_x11regression` and dropped it on return. Its
   only reader is the editor loop above, so the loop could not have run even if
   it had been ported.
2. **The extreme-value choice was being made in x11mdl, from the wrong
   inputs.** `editor.f:1727-1747` decides ONCE, at spec-read, off the PARSED
   x11reg model: `Sigxrg=2.5` only when `(Tdgrp>0 or Xtdtst>0)` AND
   `Holgrp==0 and .not.Xeastr and otlgrp==0` AND no `critical=`; otherwise
   `Otlxrg=T`. This port re-derived it at every x11mdl call and tested only
   `Xeastr` -- the AIC-TEST flag -- so an explicit `easter[8]` REGRESSOR in
   `x11regression{variables=}` took the 2.5-sigma arm where the oracle takes
   the AO arm. Wrong numbers at `OUTCOME: OK`: an 8-column design against the
   oracle's 15, c16 100% out. **No corpus spec had an explicit x11reg holiday
   regressor**, which is why five earlier x11regression increments never saw it.
3. **The `Fachol += Facxhl` fold at `x11pt2.f:299-308` was walled** behind a
   blanket `Axrghl` refusal. It is the only ARITHMETIC that flag turns on in
   x11pt2 -- `:805` and `:323` are prints, `:846`'s Stcsi feedback and the
   x11pt3 folds already read it -- and it is a no-op when no holiday column
   exists, because `x11mdl.f:707` is Facxhl's only writer and needs `Holgrp>0`.
   Ported; the wall is gone.

`airline_x11regression-easter` gates all of that bit-exact (4.7e-15 on c16).

### What is NOT shipped

**Superseded by entry 64 -- the wall is gone and CB-36 is reproduced and
gated.** The measurement below is kept because it is what located the cause:
the B/C split said the divergence was in the FACTOR build, not the fit.

CB-36's stale-rtype arm was **walled, not reproduced.** Taking it puts the run on
the oracle's branch and then diverges: on the `usertype=(td user)` spec the B
iteration's irregular regression matches the oracle coefficient for coefficient
(u1 -0.911968 / -0.9120, u2 0.536851 / 0.5369, AO1960.Mar -2.272510 / -2.2725)
and the C iteration does NOT (u2 0.330213 against 0.7441), leaving c16 1.1e-3
out. Same design, same seven AO dates, same TD coefficients to 4 dp -- so
whatever moves is between the B punch and the C fit, in the transparent
xrgdrv pass. That spec was written, measured and then removed rather than
gated; the wall is what makes the state visible, and it is proved to fire.

Also measured and NOT fixed: `x11regression{ user=... }` with no trading-day or
holiday variable makes the oracle refuse (`Must adjust for either trading day
or holiday in the x11regression spec`) and this engine returns `OUTCOME: OK`.

**Mutations:**

| removed | gates lost |
|---|---|
| the whole `xrg_editor_setup` block | 199 |
| x11mdl deriving Sigxrg itself again | 101 |
| the `Holgrp==0 && otlgrp==0` arm of the rule | 9 |
| `Nusxrg` back to a local | **0 -- saturated** |
| the `Fachol += Facxhl` fold | **0 -- saturated** |

The two zeros are reported as zeros. `Nusxrg` is read only by the editor loop,
and the only branch of that loop the corpus reaches is CB-36's, which is walled
-- so what the wiring buys today is that the WALL can fire at all, which is
proved by hand rather than by a gate. The Facxhl fold is arithmetically the
identity in every corpus spec (no x11reg holiday column, so `x11mdl.f:707`
never writes Facxhl); what it bought was the removal of a blanket `Axrghl`
refusal, and `airline_x11regression-easter` is what proves the surrounding
Axrghl path is now walked rather than refused.

**And the harness lied first.** The first mutation run reported 0-1 gates lost
for every piece, which would have meant the increment was untested. It was the
HARNESS: its `powershell -Command` string put `\x` in the middle of
`code_projects\x13new` (Python ate it as an escape) and the shell refused the
script for execution policy, so every "mutated" run used the previous binary.
The rebuild now asserts `== testing ==` appears in the build output before
pytest is allowed to run. A mutation harness that cannot tell "built and
passed" from "did not build" measures nothing -- the same class as the
`metrics.py` failure that CLAUDE.md's guardrail rule came from.

## 63. `x11regression{}` with no trading day and no holiday: the oracle refuses, this engine did not.

Small, and the shape is the port's most dangerous one -- a documented option
consumed, a number returned anyway. `gtxreg.f:891-897` refuses an irregular
regression that adjusts for NEITHER trading day NOR holiday NOR a prior-TD
weight set. `x11regression{ user= data= }` alone is exactly that, and this
engine ran it to `OUTCOME: OK`.

The check is `.not.(Axrgtd .or. Axrghl .or. neltdw.gt.0)`, and porting it
dragged in two more never-set fields:

- **`Ixrgtd`/`Ixrghl` had no initializer.** `gtinpt.f:447-448` sets both to 1;
  this port left them at the struct's zero, and `gtxreg.f:886-889` turns them
  into Axrgtd/Axrghl. `x11reg.cpp`'s x11aic TD-accept arm reads `Ixrgtd.gt.0`
  directly, so that clause was permanently false -- saturated rather than
  visible, because `Havxtd` had already set Axrgtd at parse for every corpus
  spec.
- **`noapply=` was consumed and discarded** (argidx 21). It is the ONLY writer
  of the zero into Ixrgtd/Ixrghl, so a `noapply=(td holiday)` spec the oracle
  refuses would have run here. Wired; no corpus spec uses it, so it is
  unmeasured -- stated, not implied.

**One deliberate divergence, in a flag.** `gtxreg.f:889` would set `Axrghl`
from `Ixrghl`. Not taken: this port reaches every holiday-carrying
x11regression spec bit-exact with `Axrghl` FALSE, and turning it on switches
x11pt2/x11pt3 folds nobody has measured on that path. The requirement check
reads `Ixrghl` instead, which is what Axrghl would have been. Flagged in the
code, and it belongs with the CB-36 item.

`extra/airline_x11regression-user-notd` gates it through
`test_m1_parse.py::test_outcome_matches_oracle` -- both sides now print
`Must adjust for either trading day or holiday in the x11regression spec.` and
stop. Mutating the check to `if (false)` loses that gate; the noapply wiring
and the Ixrgtd default lose none, both saturated.

## 64. CB-36 closed -- and the effective regressor type x11ref classifies by is NOT Rgvrtp.

Entry 62 walled CB-36's stale-rtype arm because taking it left c16 1.1e-3 out,
with the B iteration matching the oracle coefficient for coefficient and the C
iteration not. The wall is gone: the cause was in neither editor nor x11aic.

**Bisection, and what each step ruled out.** Dropping the wall and switching
the two flags editor sets (`Axruhl`, `Axrghl`) independently changed nothing --
all four combinations gave the same 1.084e-3. So the divergence was not the
flags but the branch, and three probes then ruled out everything the CB-36
spec has in common with specs that pass:

| probe | result |
|---|---|
| `variables=(td easter[8])` -- Otlxrg via a real holiday group | 4.7e-15 |
| `... + user=(u1)` -- user column under Otlxrg | 5.0e-15 |
| `... + aictest=(td)` -- and the aictest on top | 5.0e-15 |
| `... + user=(u1 u2) usertype=(user td)` -- a PRGUTD user column too | 5.1e-15 |

Everything passed. What was left was the CB-36 spelling itself, so the
divergence had to be in something only it does.

**The shape of the error named the cause.** b16 was out by up to 7.6e-4, and
the per-row difference divided by u2 was not noise -- it took exactly three
values, -0.019004 / -0.017318 / -0.017895, which are `-b_u2 / 28.25`,
`-b_u2 / 31` and `-b_u2 / 30`. The engine's Ftd was short by
`b_u2 * u2 / Xnstar`: the oracle puts the SECOND user column into the
trading-day factor and this port did not.

**x11mdl.f:531-540.** x11ref does not classify by `Rgvrtp`. x11mdl builds a
local `rtype` first:

```fortran
      iusr=1
      DO icol=1,Nb
       IF(Rgvrtp(icol).eq.PRGTUD.and.Ncusrx.gt.0)THEN
        rtype(icol)=Usrtyp(iusr)
        iusr=iusr+1
       ELSE
        rtype(icol)=Rgvrtp(icol)
       END IF
      END DO
```

A column carrying the generic PRGTUD ('User-defined') takes its EFFECTIVE type
from the declared `usertype=` list, which `loadxr.f:76` has already copied
`Usxtyp -> Usrtyp`. So `usertype=(td user)` gives u1 Rgvrtp PRGUTD and u2
Rgvrtp PRGTUD -- and u2's rtype is then read as `Usrtyp(1)`, which is u1's
declared `td`. Both columns end up in the trading-day factor. `iusr` advances
only on PRGTUD columns while Usrtyp is indexed by USER column number, so that
read is off by one exactly the way CB-36's is; reproduced, and mutating the
`++iusr` out costs a gate.

This is the third member of the family this spec pair has now turned up
(editor.f:1710's stale rtype, x11aic.f's descending bu2/typ2 fill, and this
one). All three are the same mistake: **an index that tracks one kind of
column being used to read an array indexed by another.**

`extra/airline_x11regression-aictest-user2swap` is back in the corpus and
bit-exact, so CB-36 is now pinned by a gate rather than by a wall. WALLS 22 ->
21 gaps.

**Mutations:**

| removed | gates lost |
|---|---|
| CB-36's stale-rtype arm (the walled behaviour) | 14 |
| the x11mdl.f:531-540 rtype remap | 11 |
| the remap's `++iusr` (its off-by-one) | 1 |

**Measured and still open:** `x11regression{ user=(u1) usertype=(td) }` -- a
single user column typed `td` and nothing else -- makes the ORACLE refuse
(`gtxreg.f:833-840`: `usertype=` requires a group titled 'User-defined' or
'User-defined Holiday' to exist, and a lone `td` column produces neither), and
this engine runs it. Same class as the td-or-holiday requirement in entry 63,
and cheap.

## 65. gtxreg.f's whole `IF(Nb.gt.0)` block was missing -- and an aictest with no `variables=` is two divergences deep.

Entry 64 left one measured refusal open: `x11regression{ user=(u1)
usertype=(td) }` runs here and is rejected by the oracle. Porting it turned up
the block it lives in -- `gtxreg.f:828-878`, fifty lines that had no C++ at all
-- and then, through the `Ixreg` line just past it, two silent-wrongness cases.

**The block reads the WORKING regARIMA COMMON.** That is why it is easy to
miss: `gtxreg` builds the irregular-regression model into `Grpttl`/`Grp`/`Nb`
and only `loadxr` moves it into `Xrgmdl`. The port therefore has to sit between
this port's `rmlnvr` call and its `loadxr` call, not after -- a stripped Leap
Year column shifts every group index the block reads. Four pieces:

| gtxreg.f | what | state before |
|---|---|---|
| :833-847 | `usertype=` refused unless a 'User-defined' or 'User-defined Holiday' group exists | absent |
| :849-855 | one `usertype=` given -> broadcast it over all `Ncxusx` columns | absent |
| :858-877 | `regfix` -> `Iregfx`, then `Userfx` from the 'User-defined' group | absent |
| :880 | `otsort` | still not ported (the corpus specifies outliers in order) |

The refusal is the measured one: a lone `td`-typed user column is titled 'User-
defined Trading Day', so neither group exists. Note it is INDEPENDENT of entry
63's td-or-holiday requirement -- the same column sets `Havxtd`, so `Axrgtd` is
true and that check passes. Two refusals, two specs.

The broadcast runs AFTER the `adrgef` loop, so it changes no group title and no
`Rgvrtp`: it edits only the `Usxtyp` list that `loadxr.f:76` copies into
`Usrtyp`, which is what entry 64's remap and `editor.f:1710` read. Combined
with the off-by-one both of those have, `Usxtyp(2)` is read only when two
`PRGTUD` columns precede it -- so the broadcast is invisible on every spec in
the corpus. Ported because it is one line and the next spec will not be.

**`gtxreg.f:883` -- `IF(Nb.gt.0.or.Xeastr.or.Xtdtst.gt.0)Ixreg=1`.** This port
tested `Nbx>0` alone. An aictest carries no regression column of its own (x11aic
adds them later), so `x11regression{aictest=(easter)}` with no `variables=` left
`Ixreg` at 0 -- and `x11parts.cpp:529` carried a comment asserting "Ixreg==0
with Axrgtd cannot occur", which is exactly what did occur. That combination hit
an unrelated x11pt2 wall, which is the only reason it was not silent.

**And what the fix exposed.** With `Ixreg` correct the wall no longer fires, and
both aictest-without-variables specs then run to `OUTCOME: OK` with wrong
numbers:

| spec | oracle | engine |
|---|---|---|
| `aictest=(td)` | `aictest.xtd.reg: td1coef`, REJECTED, x11mdl.f:308's identity-factor NOTE branch, c16 all 1.0 | tests plain `td`, ACCEPTS, writes a real TD factor |
| `aictest=(easter)` | AICC(no easter) -734.3172 | -741.9217; both still pick easter[15], d11 1.4e-2 out |

The first one has a named cause: `editor.f:1782-1790` rewrites `Xtdtst` 1 -> 3
when the TD group is a single column, and it computes that column count as
`begcol=Grpx(Tdgrp-1)`, `endcol=Grpx(Tdgrp)-1` WITHOUT testing `Tdgrp`. With no
TD group at all that reads `Grpx(-1)` out of bounds, the value compares equal to
`endcol`, and a `td` aictest silently becomes a `td1coef` aictest. That is a
Census defect, but an out-of-bounds read is not something to reproduce on one
build's evidence, so it is not claimed as a CB entry and not ported. The second
one is not yet located; it is not this block.

Both are now REFUSED (`xrg_not_ported`, WALLS 21 -> 22 gaps) rather than
answered. `editor.f:1760-1846`'s whole "Check options for AIC trading day test"
block -- three td/tdstock agreement refusals, the 1->3 rewrite, `Xaicst`,
`Xaicrg`, and the monthly-data / pre-1776 generatability refusals -- stays
unported behind that wall. It is inert for every gated aictest spec, all of
which name their regressors in `variables=` (so `Tdgrp>0`, multi-column, no
rewrite); with no variables it is not inert.

**`Noxfac` had no writer.** `x11parts.cpp:572` reads it; nothing set it.
`gtxreg.f:899` (`Haveum.and.Havxtd.and.Havxhl`) and its `noapply`-with-`umdata`
refusal are now both there. Inert while `umdata=` is unread and `Haveum` stays
false -- but a read with no write is the shape that hides a defect, not a
harmless one.

**Also swept:** `editor.f:1690-1716`'s comment in `readers_spec.cpp` still said
CB-36 was walled. Entry 64 unwalled it.

**Mutations, and four honest zeros:**

| removed | gates lost |
|---|---|
| the 'User-defined' group-existence refusal | 1 |
| gtxreg.f:883's `Xeastr`/`Xtdtst` disjuncts | 0 |
| the aictest-with-no-variables wall | 0 |
| the single-`usertype=` broadcast | 0 |
| `regfix` + `Userfx` for the irregular regression | 0 |

The zeros are saturated preconditions, not evidence the code is right, and two
of them cannot be un-saturated today. The `Ixreg` disjuncts and the wall are
reachable ONLY from an aictest with no `variables=`, and every such spec is
what the wall refuses -- so no passing gate can exist for them until the wall
lifts. They are pinned by `docs/WALLS.md` (derived from the refusal itself, so
deleting the wall deletes the entry) and by the measurements above, which is
weaker than a gate and is stated here rather than left to be inferred from a
green suite. The broadcast needs a spec with two-plus user columns and ONE
`usertype=`; `Userfx` needs a `b=` fixing in an `x11regression{}`. Both are
cheap specs and neither exists.

**Gated:** `extra/airline_x11regression-usertype-tdonly`.

## 66. `x11regression{span=}` was parsed and discarded -- and the flag it promotes has no consumer without a model.

Board item 3 (entry 65's leftover, `editor.f:1972-1976`'s second `Ixreg`
promotion) turned out to rest on an option this port never read at all. Two
fields, `Begxrg`/`Endxrg`, had readers -- `run_history.cpp:679` takes the model
span end from `Endxrg` -- and NO writer anywhere in the C++; `span=` itself fell
through `gt_x11regression`'s `consume_value`. So did `Fxprxr` and `Xdsp`, both
read (`run_history.cpp:763`/`:822`, `xrgdrv.cpp:38`) and never written.

**Measure the oracle on-vs-off first, and this is why.** On the airline series
with `x11regression{variables=(td)}`:

| spec | oracle vs its own no-span run | engine vs oracle |
|---|---|---|
| `span = (1952.01,1958.12)` | d11 1.3e-2 | 1.3e-2 |
| `span = (1952.01,0.12)` | d11 1.6e-2 | 1.6e-2 |
| `span = (1949.01,1960.12)` (the full span) | 0 | 3.2e-15 |

The engine's answer never moved: it was the unrestricted one every time, at
`OUTCOME: OK`. Textbook parsed-but-unread.

**What is ported.** `gtxreg.f:472-482` (the arg and its two-dates check),
`:629-661` (the NOTSET defaults, the `0.per` end-date form and its `Fxprxr`,
the `chkcvr` coverage refusal), and `editor.f:1970-1977` (`Xdsp` plus the
second `Ixreg` promotion, placed in `gtinpt.cpp` next to the first because the
oracle runs editor after ALL specs and `Khol` may not be parsed yet when
`x11regression{}` is).

Note the order the two promotions run in, which is load-bearing: editor's block
is `IF(Ixreg.eq.1)`, and gtinpt.f:1201's model promotion already ran. So on any
spec with a `arima{}`, `Xdsp` is never computed at all. Ported as written.

**What is walled, and the second thing that fell out.** The narrowing itself --
`x11mdl.f:115-118` moves `Begspn`/`Endspn`/`Nspobs`/`Frstsy`/`Nobspf` onto the
regression span and `:515-525` walks them back out with `setspn` plus a
`regvar` rebuild. This port's `x11mdl_td` derives `Nobspf` from the
forecast-extended buffer instead of Fortran's
`min(Nspobs+max(Nfcst-Fctdrp,0), Nomnfy)`, so that is not a two-line change,
and it has to join `run_x11.cpp`'s span-replay save/restore set. Refused
instead.

Then the promotion exposed its own hole. `span = (1949.01,0.12)` with NO
`arima{}` sets `Fxprxr`, promotes `Ixreg` to 2, and moves the ORACLE's d11
9.1e-3 -- while this engine came out bit-identical to its own no-span run,
because `xrgdrv` is only ever called from `run_pre_model`, which runs only when
there is a model. The oracle reaches it from `x12run.f:174`/`x11ari.f` on both
paths. Also refused, in `x11_prestage`.

**A wall the inventory could not see.** That second refusal was first written
as a bare `errhdr`/`writln`/`abend`, and `tools/walls.py` reported 23 gaps
where it should have said 24: the tool collects refusals by HELPER NAME, and a
hand-rolled one is invisible to it. Fixed by giving `x11_prestage.cpp` a local
`not_ported` helper. Standing rule, and it is the same one that put
`test_doc_tooling.py` in the parity suite: **a guardrail that is not inventoried
is not a guardrail.** If you write a refusal, check it appears in
`docs/WALLS.md` after `walls.py --write`.

**Gated:** `extra/airline_x11regression-span-full` (`span=` naming the full
series span) and `extra/airline_x11regression-span-0per` (the `0.per` form with
`per` equal to the series' last period). Both run bit-exact.

**Mutations -- five zeros, and they are the point of this entry:**

| removed | gates lost |
|---|---|
| the `span=` parse arm | 0 |
| the `0.per` end-date arm | 0 |
| the narrowing wall | 0 |
| editor.f:1970-1977's second Ixreg promotion | 0 |
| the no-model Ixreg>=2 wall | 0 |

Nothing in this increment is pinned by the suite, and the reason is
structural: every span that MATTERS is behind a wall, so the only gateable
spans are the ones that resolve to the series span -- where the ported code and
the old discard-everything code produce identical output by construction. The
two specs prove the parse does not crash or over-refuse; they prove nothing
about the semantics.

That was worth one more measurement rather than an assumption. `Fxprxr` has a
real consumer -- `revdrv.f:500-503` re-derives each history span's regression
end from it -- so a `history{}` spec was built on top of the `0.per` one to
give the `0.per` arm teeth. It did not: mutating the arm out still cost zero
gates. With `per` equal to the series' last period the two spellings are
indistinguishable everywhere in this port, revdrv included, and a `per` that is
NOT the last period narrows and hits the wall. The spec was DELETED rather than
committed with a coverage claim measurement had already refuted -- the corpus
already carries six history+x11regression specs, so it added nothing else.

## 67. The x11regression span narrowing: fit narrow, apply wide -- and the calendar array that is indexed from the BUFFER, not the span.

Entry 66 read `x11regression{span=}` and refused to act on it. This lifts half
that wall: a span whose START moves is now ported and gates bit-exact. A span
that ENDS early is a different mechanism and stays refused, for a reason worth
writing down.

**The shape of the Fortran.** `x11mdl.f:113-118` moves `Begspn`/`Endspn` onto
the regression span, `:186-195` re-derives `Nspobs`/`Frstsy`/`Nobspf` from
them, and the whole irregular regression -- transform, extreme values, AIC
tests, OLS -- runs there. Then `:512-528` calls `setspn` to put the span BACK
and rebuilds the design with `regvar` before the factor is built. Fit narrow,
apply wide. Skipping the rebuild would leave a factor covering only the
regression span.

**The trap that cost the most, and it is a C++-port-only trap.** The first
working version was 4.6e-2 out on d11 -- WORSE than ignoring the option
(1.3e-2). The cause was `tdset`. The oracle calls it exactly ONCE, from
`editor.f:2240`, over the whole `[Pos1bk,Posffc]` buffer with `Begbak`; it is
never called per x11mdl pass and never sees a narrowed span. This port issues
it from inside `x11mdl_td`, which was harmless while the span never moved --
and the moment it did, feeding it the narrowed `Begspn` slid `Xnstar`/`Xn` by
`nbeg` periods against a factor still indexed from the buffer. **A calendar
array indexed from the BUFFER cannot be built from a date that describes the
SPAN.** Fixed by handing `tdset` the pre-narrowing start.

That generalizes: this port has relocated several oracle one-shot calls into
the routine that needs them. Every such relocation is fine until some caller
mutates the state the original call site read.

**The restore is guarded, not just written.** `x11mdl_td` has a dozen early
returns (`lfatal`, the x11mdl.f:308 NOTE branch). A `SpanGuard` destructor puts
`Begspn`/`Nspobs`/`Frstsy`/`Nomnfy`/`Adj1st` back unconditionally; the explicit
`setspn` still runs first, before the factor build, because the factor needs
the wide design. Leaving those narrowed on an early return is the span-replay
bug this subsystem has already paid for four times.

**What is still walled: a span that ends EARLY.** `xrgdrv.f:152-158`
RE-derives `Xdsp` from `Endspn`/`Endxrg` -- overriding `editor.f:1976`, which
leaves it 0 whenever a regARIMA model promoted `Ixreg` first -- and then
shortens `Posfob`/`Posffc` for the whole transparent pass, which `x11mdl.f:515-518`
reads back. So the early-end case is a POINTER mutation across x11pt1/x11pt2,
not a span narrowing, and it belongs with the span-replay save/restore set.
Measured with the start narrowing in place: b16 1.3e-3, d11 1.4e-2. Refused.

One observable that is not yet right on that path either, noted for whoever
takes it: the oracle saves the `.xrm` design matrix at `x11mdl.f:500-509`,
i.e. BEFORE the restore, so its rows are the NARROW design (132 on the probe);
this port snapshots after, and would write 156.

**Gated:**

* `extra/airline_x11regression-span-start` -- `span=(1953.07,1960.12)`.
* `extra/airline_x11regression-span-0per-start` -- `span=(1952.01,0.12)`, which
  narrows the start AND exercises the `0.per` end form. Entry 66 could not pin
  that arm at all; this pins it.

**Mutations -- and note what changed since entry 66:**

| removed | gates lost |
|---|---|
| the START narrowing (`Begspn <- Begxrg`) | 20 |
| `tdset` fed the narrowed start instead of the buffer's | 20 |
| the setspn restore + regvar rebuild | 20 |
| the early-END wall | 0 |

Entry 66's five zeros were structural: everything that mattered was behind a
wall, so nothing was gateable. Lifting half the wall turned three of them into
20s. That is the argument for lifting walls rather than documenting them --
**a wall costs you the coverage of everything behind it**, not just the
feature.

## 68. The x11regression span that ENDS early: a pointer mutation, a deliberately narrow Nofpob, and the gate that named its cases by hand.

Entry 67 ported the half of `x11regression{span=}` that narrows the START and
refused the half that ends EARLY, on the grounds that it is a different
mechanism. It is, and this closes it. The wall count goes 24 -> 23.

**The two mechanisms, side by side.** A span that starts late is applied
INSIDE x11mdl: `x11mdl.f:113-118` moves Begspn/Nspobs, `:512-528` puts them
back. A span that ends early never reaches that code. `xrgdrv.f:151-158` runs
first, RE-derives `Xdsp` from Endspn/Endxrg (overriding `editor.f:1976`, which
leaves it 0 whenever a regARIMA model promoted Ixreg before the `0.per` clause
could fire), pulls `Posfob`/`Posffc` back by it, and moves `Endspn` onto
`Endxrg` **without touching Nspobs**. So by the time x11mdl runs, its own
`nend` is ZERO -- the span has already arrived, as shortened pointers -- and
the only place Xdsp appears again is the Kpart==3 restore at `:515-517`.

Endspn is not a field this port maintains (it derives it from
`Begspn+Nspobs-1`), so `ctx.xrg_endspn_narrow` stands in for it, set for
exactly the window between xrgdrv's shortening and its restore. x11mdl_td is
its only reader, as in the oracle.

**The trap, and it is the same trap as entry 67 rotated 180 degrees.** Entry
67 cost a session to `tdset` being fed the NARROWED span start. This one cost
the same routine's END: xrgdrv pulls `Posffc` back, but the C iteration
rebuilds the design over the FULL span, and `x11ref` indexes `Xnstar` by row.
Stopping tdset at the shortened Posffc left Xnstar ZERO over the last Xdsp
rows, the factor came out NaN there, and the run died in the prior-TD divide
with `Do not take log of a zero, y( 133)= NaN`. **A calendar array indexed
from the buffer needs the buffer's start AND its end.** Written down twice now
in two entries; treat any relocated one-shot call the same way.

**The finding that took the longest: Nofpob is left NARROW on purpose.** With
b16 and c16 bit-exact the d-tables were still 1.3e-2 out, growing toward the
end of the series. The oracle's own `.out` gave it away -- its **B 1** table
prints `Observations 132` on a 144-point span. The chain:
`x11mdl.f:126-137`'s C iteration recomputes `Nofpob = Nspobs + Nfcst` off the
still-narrow Nspobs (132 = 120 + Nfcstx), and `xrgdrv.f:166`'s restore is
CONDITIONAL -- `IF(Nfcst.ne.nf2.or.Nbcst.ne.nb2)` -- so when the spec's own
forecast horizon already equals Nfcstx the test is false and the narrow value
survives into the main run, where `x11pt1.f:70` reads it as
`Nspobs = Nofpob - Nfdrp`.

The oracle can afford that because **its editor runs once, at parse, BEFORE
x11ari reaches xrgdrv**, and `x11ari.f:149`'s setxpt is NOT unconditional --
it sits under `IF((Same.or.(.not.havmdl).or.(.not.extok)).and.Lmodel)`, i.e.
model-failure only. This port stands in for the editor TWICE
(`run_pre_model`, `x11_prestage`) and both stand-ins run AFTER xrgdrv, so both
re-derived the geometry and discarded exactly what xrgdrv had left. Fixed with
`xrg_geometry::capture/restore` around each call, armed only on this route.
Restoring the counters is not the same as skipping the call: skipping also
dropped `Lsp`/`Begbak`, and the d8b year labels came out as `1*  5z 11*`.

**And a `max(Xdsp,0)` that cost 28 gates.** Xdsp is a raw `dfdate` result. It
goes NEGATIVE whenever Endxrg is LATER than the span end -- which is every
`history{}` and `slidingspans{}` replay, where each span is a slice of a
series whose x11regression span is the whole thing. Unclamped in the Faccal
stash length it shortened the stash by |Xdsp| (71 points on the first history
span) and every replayed span came back wrong. Three separate hypotheses were
eliminated before a git-stash bisect found it. **A quantity the Fortran only
ever tests with `.gt.0` is not thereby non-negative** -- and this port stores
it, where the Fortran mostly consumes it inline.

**The gate that named its cases by hand.** Four mutations came back ZERO. The
cause was not saturation: `test_x11regression_tables.py`'s `CASES` was a
three-element literal, so the `xrm`/`b16`/`c16` gates had never been shown ANY
of the six `x11regression{span=}` specs. The same file already carries a note
about this exact defect for `AIC_CASES`, one list over, from 2026-07-31.
Auto-discovering CASES (plus a floor assertion) immediately failed three specs
for a real bug it had been hiding: **the `.xrm` rows were dated from Begspn
when savmtx.f is handed Begxy**, so every row of a late-starting span's design
was labelled `nbeg` periods early. Row COUNTS had been right all along; only a
date-keyed comparison sees it, and nothing was comparing.

**Gated:**

* `extra/airline_x11regression-span-end` -- `span=(1949.01,1958.12)`.
* `extra/airline_x11regression-span-both` -- `span=(1951.07,1958.12)`, the only
  place the two narrowing mechanisms compose (nbeg > 0 AND nend = Xdsp).
* plus the six existing span specs, which now reach the xrm/b16/c16 gates.

**Mutations** (full suite, 6219 gates):

| removed | gates lost |
|---|---|
| tdset extended past the shortened Posffc | 34 |
| `nfac += Xdsp` on the C iteration | 30 |
| the `max(Xdsp,0)` stash clamp | 28 |
| the `.xrm` save moved BEFORE the restore | 27 * |
| the geometry restore (each of the two call sites) | 24 |
| the xrgdrv Xdsp pointer shortening | 22 |
| the `.xrm` Begxy dating | 3 * |
| the b16/c16 `lastpr` extension | 2 |
| x11mdl.f:126-137's COMMON writes | **0** |
| the SpanGuard disarm | **0** |

\* measured at 6179 gates, before b16/c16 joined `_DTABLES`.

The last two zeros are reported as zeros deliberately. Both are faithful
transcriptions -- x11mdl.f:126-137 writes those COMMONs, and the Fortran's
`:518` restore genuinely does not fire when `nbeg` and `nend` are both 0 -- but
what they change is `Nofpob`, and the only observable that depends on it is the
LENGTH of the B 1 table, which nothing in the corpus saves. They are kept
because they are what the Fortran does, not because a gate proves them.

## 69. Three gates that could not see their specs -- and the SEATS decomposition that had been 8.1e-7 wrong behind them.

Entry 68 closed with a finding rather than a port: `test_x11regression_tables.py`
had named its cases by hand, and widening it immediately exposed a real dating
bug. This is the sweep that finding earned, and it found the same shape three
more times. One of the three was hiding an engine defect.

**How to look for this.** Do not read the test files for literals -- most
literal lists in this suite are TAG lists (which tables to compare), and those
are deliberate. The question is *which (spec, table) pairs does the suite
actually compare*, so ask pytest: `--collect-only -q`, keep the `[...]` ids,
and cross them against every golden on disk. Anything with a golden and no id
is ungated. That probe is ~20 lines and it is the only version of this check
that cannot itself go stale, because both sides are derived.

It reported three real holes (and one false positive worth knowing about: the
`.xrm` gate parametrises on the spec alone, so its ids carry no table name and
every xrm golden looks uncovered).

**1. The SEATS table gate, and the bug behind it.** `_discover` scanned
`generated/` only and required `base.endswith("seats")`. The four hand-authored
SEATS specs are `extra/airline_seats-{history,slidingspans,qmax-rmod,tabtables}`
and match neither half, so their s10-s18 goldens were compared to nothing.
Three were bit-exact at ~5e-15. **`airline_seats-history` was 8.1e-7 out on
all four of its tables.**

The cause is this port's most-repeated seam, for the fifth time.
`history{}` RE-ESTIMATES the model per span (`slidingspans{}` does not -- it
runs fixmdl, which is why the spec beside it was clean). Each span's estimate
leaves its own ARMA coefficients in `/mdldat/`'s `Arimap`, and
`seats_decode_model` reads `Arimap` -- so the canonical decomposition that
`tools/x13run_seats.cpp` rebuilds after `run_seats` returns was the LAST SPAN's,
published under the main run's dates. The oracle is immune for the usual reason:
it punches its s-tables inside x11ari, before x12run.f:257 ever calls revdrv.

The proof was one line of spec surgery: delete the `history{}` block, rerun the
same spec against the *same* golden, and every table drops to ~5e-15. The fix is
`ctx.model` + `ctx.mdldat.arimap` joining run_seats.cpp's span-replay restore
set, where `/lkhd/`, `/x11srs/`, `/x11fac/`, `ctx.seats_*` and `ctx.series.tsrs`
already were. Note the shape of the omission: the restore set had every
*published output* the replay overwrites and none of the *estimated model* it
re-derives them from.

Save `/mdldat/` field-by-field. A by-value copy of the whole COMMON overflows
the stack outright (Armacm + Xy + Matd are ~1.5 MB) -- the first attempt exited
`0xC00000FD` on every SEATS spec.

**2. The x11 table gate's `b1` requirement, which cost 18 specs.**
`test_x11_tables.py` discovered on `all(_CORE_TAGS)` with `_CORE_TAGS` =
`b1,d10,d11,d12,d13`. Eighteen specs ship the four D tables and no `b1` (they
do not save it), so that one tag excluded each of them ENTIRELY -- not their b1,
their whole decomposition. The list is the entire `pickmdl-*` family,
`airline_slidingspans-td`, `noapply-{ao,ls,td,holiday}`, `reg-tcrate`,
`outlier-tcrate`, `fcst-lognormal`, `reg-eastermeans`, `outofsample`. The gate
also scanned `generated/` only, while feature gates cover `extra/` piecemeal by
feature -- so a spec belonging to no feature front fell through both. All 18
measured bit-exact (~5e-15); this one was pure coverage. `b1` now gates
where-shipped, exactly as `d16`/`sac`/`tac` already did.

**3. The tdprior gate.** A three-element literal, one spec stale:
`airline_x11regression-tdprior-td` ships a full bundle and reached no `a4` gate
at all. Its d-tables were covered by `test_x11regression_tables.py`, so the only
uncompared table was `a4` -- the one table unique to that file. Bit-exact.

**The rule this leaves.** A discovery predicate is a hand-written case list
that has learned to hide. `endswith("seats")` and `all(_CORE_TAGS)` read like
generic discovery and are as brittle as a literal, with none of the visibility
-- a literal at least shows you its length. Every discovery in this suite now
carries a floor assertion (`test_*_cases_discovered`), because the failure mode
is a SHRINKING parametrisation, and a parametrisation that shrinks to nothing
reports as green.

**Gated:** the four `extra/airline_seats-*` specs (20 new (base,tag) pairs, all
bit-exact, added to `_gated`); the 18 D-table specs above plus the `extra/`
tree in `test_x11_tables.py`; `airline_x11regression-tdprior-td`'s a4. Suite
6219 -> 6468 passed, 0 failed, 0 xfailed, 492 -> 671 skipped.

**Mutation:** dropping the `ctx.model`/`Arimap` restore fails 5 gates. Before
this increment it failed none -- which is the entire point of the entry.

## 70. The Easter AIC window set was decided in the wrong phase -- and the wall in front of it was guarding the wrong condition.

Board item 1 said to measure the `aictest=(easter)` AICC gap before touching
`editor.f:1760-1846`, on the grounds that it might be the more general defect.
It was two defects, neither of them that block, and one of them was live on a
route no wall covered.

**What the wall thought it was guarding.** `readers_spec.cpp` refused
`x11regression{}` aictest when `Nbx == 0` -- "no regression variables" -- with
the easter arm measured at oracle -734.3172 vs engine -741.9217. The condition
was wrong. `x11aic.f:112-143`'s strip loop deletes the Easter columns whenever
`Xeastr` is on, so `variables=(easter[8]) aictest=(easter)` reaches the i==1
baseline with **exactly the design an empty `variables=` would have** -- and the
oracle returns the same -734.3172 for both, which is what identifies the real
condition. That spec has `Nbx == 1`. It sailed past the wall and returned
`OUTCOME: OK`.

Two things were wrong with what it returned.

**1. The window set, and the phase it was decided in.** `editor.f:1577-1591`:
when `variables=` NAMES Easter regressors, the windows the AIC test sweeps are
THEIRS -- `Neasvx = endcol-begcol+2`, and `Xeasvc(2..)` is read back out of the
column TITLES (`getstr` then `ctoi` past the `[`, so "Easter[8]" -> 8). Only
with no Easter group in the model does the default `{0,1,8,15}` sweep apply.

This port had the default half only, and had it in the **`aictest=` parser**,
where the question cannot be answered: at that moment `variables=` may not have
been read yet, so `Easgrp` is unknown. That is precisely why the oracle decides
it in the EDITOR, with the whole spec in hand. The comment above the code even
cited `editor.f:1577-1590` and said "no explicit Easter group in the model" --
describing a condition it never tested. Moved to `xrg_editor_setup`, where
`easgrp` is already computed eleven lines above. Before: four AICCs where the
oracle prints two.

This is the parsed-but-unread class with a twist worth naming: the value WAS
read, and was even correct for every gated spec. **The defect was that it was
computed in a phase that could not see its own precondition** -- and a
single-branch implementation of a two-branch Fortran `IF` looks exactly like a
working one until a spec takes the other branch.

**2. The AICC itself, which is the AO half and is still open.** `xeastr`
suppresses `editor.f:1727`'s `Sigxrg=2.5` default, so these runs take the
`Otlxrg` branch and do AUTOMATIC AO IDENTIFICATION on the irregular -- the
oracle's own `.out` adds `AO1960.Mar` at t=-5.50. So the i==1 "baseline" is not
an empty fit at all; it is a fit on an auto-AO design. With a trading-day group
present the two agree bit-exact; with none they do not, and that is where the
~7.6 lives. Still walled, now under a message that names the actual condition.

**The gateable route.** `variables=(td easter[8]) aictest=(easter)`: the strip
loop takes the Easter columns out, the TD group survives, the run never reaches
the auto-AO arm -- and it still exercises the window set, because `Easgrp > 0`.
Both AICCs and the chosen window come out bit-identical. That is the new spec.
It is worth noticing that the fix and the wall needed DIFFERENT specs, and that
the one that could be gated is the one where the second defect is absent.

**Gated:** `extra/airline_x11regression-aictest-easter8` (new).

**Mutations:** reverting the `Easgrp>0` arm to the unconditional default fails
**17** gates. The widened wall is **0**, and is reported as 0 on purpose: a
walled route cannot be corpus-gated the usual way, because the engine refuses
where the oracle succeeds, so a golden comparison would fail rather than pass.
`docs/WALLS.md` is its record (23 -> 24 gaps), and the refusal was verified by
hand to fire on `variables=(easter[8])` and NOT on `variables=(td easter[8])`.

Suite 6468 -> 6486 passed, 0 failed, 0 xfailed.

## 71. The no-model OLS prior TD -- and a restore that was compensating for a hoist, on the one path that had not made the move.

`x11ari.f:88-95` calls `xrgdrv` under `IF(Lx11)` alone:

```
IF(Ixreg.eq.2.or.Khol.eq.1)THEN
 CALL xrgdrv(Lmodel,Lx11,Kh2,Lgraf)
```

`Lmodel` is only passed THROUGH, to `ssprep`/`restor`, to widen the snapshot.
The OLS prior trading-day pass itself runs whether or not a regARIMA model was
requested. This port drives it from `run_pre_model`, which runs only when there
IS one, so a no-model spec that promoted `Ixreg` skipped it and answered as if
`Ixreg` were 1. It was WALLED (entry 65's sibling), so the outcome was a clean
refusal rather than wrong numbers.

**The measurement first.** `x11regression{ variables=(td) span=(1949.01,0.12) }`
with no `arima{}`: the oracle moves d10 9.2e-3, d11 8.3e-3, d12 8.4e-3, d13
1.6e-2 against the same spec without `span=`. `0.per` is the only writer of
`Fxprxr` and `editor.f:1976-1978` promotes `Ixreg` 1->2 on it; `0.12` on a
series ending in December resolves to the series end, so `Xdsp` is 0 and nothing
narrows -- the promotion is all this route is for.

**The port is a placement, not an algorithm.** Nothing in `xrgdrv` needed
changing; the call goes into `x11_prestage` at x11ari's own point in time --
after `x11int` (x12run.f:174), before `x11pt1`. That ORDER is the content: the
transparent pass reads `Sprior`, which on the no-model path exists only once the
`adjsrs` record has been copied by `x11int`, and it leaves `Faccal` for
x11pt1's `Ixreg==3` restore to fold.

**Then the interesting half.** With the call in place the calendar factor came
out bit-exact -- `b16`, `c16` and the TD part of `d16` all matched to 5e-15 --
and the SEASONAL factor was 0.67% out. d10 and d16 carried the SAME relative
error, which is what says the calendar half is right and the filtering is not.

It was `Ksdev`, the Bundesbank extreme-value spread control. `xrgdrv.cpp` saved
and restored it alongside `Lterm`, on the reasoning that both are
transparent-pass leakage. `restor.f` restores `Lter`, `Ktcopt` and `Tic` and
NOTHING else -- the oracle's main `x11pt2` simply inherits whatever the
transparent pass's `entsch` left, and its own re-derivation is gated `Ksdev < 4`
on that inherited value. So the restore is not in the Fortran at all.

Deleting it fails **362** gates. It is not dead code: it compensates for the
MODEL path's HOIST -- the port moves the call out of x11ari and ahead of the
editor stand-in that then sets `Kersa=0` (editor.f:1486). The no-model call
makes no such move, so it must not carry the compensation. `at_x11ari` is that
distinction, and it is the honest name for it: the flag does not describe the
spec, it describes which call site is being stood in for.

**The shape worth keeping.** A compensating restore is invisible while every
gated spec goes through the path it compensates for. The moment a second call
site appears, the compensation becomes a defect there -- and it will not look
like one, because the code is identical and correct twenty lines up. Ask of any
save/restore in this port whether it mirrors the Fortran or patches a
rearrangement; the two need different call-site conditions, and only the second
kind has to be re-derived per caller.

**Gated:** `extra/airline_x11regression-nomodel-priortd` (new). Auto-discovered
by `test_x11regression_tables.py` (name contains `x11regression` + an `.xrm`
golden) and by the x11-table / diagnostic gates.

**Mutations:** disabling the new call fails **18** gates; restoring `Ksdev`
unconditionally fails **17** (the new spec); never restoring it fails **361**
(the model path). WALLS 24 -> 23 gaps. Suite 6486 -> 6508 passed, 0 failed,
0 xfailed.

**Found on the way, NOT fixed (open).** On the no-model path the oracle's
`transform{function=log}` is a no-op for every X-11 table -- measured
bit-identical against the same spec with no `transform{}` at all, on both a bare
`x11{}` run and an `x11regression{ variables=(td) }` one. This engine agrees on
the bare run and does NOT on the x11regression one: d11 comes out 1.2e-2 out,
b16/c16 1.5e-3, at `OUTCOME: OK`. Something on the irregular-regression route
B1 is bit-identical between the two engine runs, so the divergence is inside the
X-11 spine, not the input.

Cause NOT established. Two candidates, in order: (a) `Lam` alone opens
x11pt2.f's `goodlm` gates -- :115 and :324, the makadj/tdlom fold, which also
need `Ixreg!=2 .and. Priadj>1`, so the real question may be whether this port
sets `Priadj` on a route where the oracle leaves it 0; (b) `x11mdl.f:104-109`
saves `Lam`/`Fcntyp` and forces `Lam=1, Fcntyp=4` for the whole irregular
regression, restoring at :356 and :833 -- this port does not reproduce that at
all, and it is inert on every gated MODEL spec, which is why it survived.
Measure (a) first; it is one flag away from being decided.

The new spec deliberately carries no `transform{}` so that this does not
contaminate what it gates.

## 72. A stand-in for `restor` that restored less than `restor` does -- and turned an x11regression `td` into a prior adjustment.

Entry 71 left this measured and unexplained: with no model, the oracle's
`transform{function=log}` is a no-op for every X-11 table -- bit-identical to
the same spec carrying no `transform{}` at all, on a bare `x11{}` run AND on an
`x11regression{ variables=(td) }` one. This engine agreed on the bare run and
did not on the x11regression one: d11 1.2e-2, d13 1.6e-2, b16/c16 1.5e-3, at
`OUTCOME: OK`.

**Ruling out the first candidate mattered.** Entry 71 named x11pt2.f's `goodlm`
gates as the likely route, `Lam` being the only input that changes. Forcing
`goodlm` false moved NOTHING, and neither did disabling the makadj/tdlom block
outright. That is the whole value of writing the candidate down: it was cheap
to kill, and killing it is what made the next step obvious.

**A state dump, not more reading.** One `fprintf` of the /prior/, /adj/,
/picktd/, /x11opt/ and /xtrm/ scalars at the top of the X-11 spine, on both
probes:

```
log:   lam=0 fcntyp=1 muladd=0 adjmod=0 priadj=4 kfmt=1 picktd=1 ...
nolog: lam=1 fcntyp=4 muladd=0 adjmod=0 priadj=0 kfmt=0 picktd=1 ...
```

`Priadj=4`, `Kfmt=1`: the engine was applying a LEAP-YEAR PRIOR to the series.
`Picktd=1` on both, and that is the bug -- it should have been 0.

**The chain.** `variables=(td)` inside `x11regression{}` sets `Picktd` through
the same `adpdrg.f:642` line the regARIMA parser uses. `gtinpt.f` then does:

```
 804  CALL ssprep(T,F,F)          ! Pktd2 = Picktd  (still F here)
 818  CALL gtxreg(...)            ! parses x11regression -> Picktd = T
 830  CALL loadxr(T)              ! Pckxtd = Picktd   (the x11reg model's copy)
 832  CALL restor(T,F,F)          ! Picktd = Pktd2    -> back to F
 ...
 999  IF(Picktd)THEN
1000   IF(dpeq(Lam,ZERO))THEN
1032    CALL rmlnvr(Priadj,Kfulsm,Nspobs)   ! Priadj = 4
```

Line 999 is AFTER line 832, so the flag that block reads is the RESTORED one.
`Pckxtd` is the copy that survives, and it is what `xrgdrv` and `x11mdl` read.
This port's stand-in for the :832 `restor` is `xrg_clear_working` + the parked
store; it clears the regressors and never touched `Picktd`. So an
x11regression-only `td` looked like a regARIMA one, and rmlnvr set `Priadj=4` /
`Kfmt=1` on a series the oracle never prior-adjusts.

Three conditions have to coincide, which is why nothing caught it: a log
transform (the `dpeq(Lam,ZERO)` gate at :1000), a `td` that lives ONLY in
`x11regression{}` (with `regression{ variables=(td) }` the snapshot is already
T and the restore is a no-op), and no regARIMA model to mask the result.

**The class, third appearance.** `xrgdrv` already carries a hand-written
`Ncusrx`/`Nrusrx`/`Usrtyp` restore for exactly this reason, with a comment
saying so. The general shape: **where this port stands in for `restor`, the
stand-in restores a SUBSET, and the fields it omits are invisible until
something downstream reads one.** `restor.f`'s set is `Lter`, `Ktcopt`, `Tic`,
the model parameters, `Nrxy`, `Iregfx`, `Regfx`, `Ncusrx`, `Nrusrx`, `Picktd`,
`Adjtd`..`Adjsea`. Of those, the parse-time stand-in now covers `Picktd`
explicitly; `Ncusrx`/`Nrusrx` are covered in `xrgdrv`; the `Adj*` block is
never written by `gtxreg`, and `Iregfx`/`Nrxy` are only READ there. That list
is the audit, and it is worth redoing whenever a new writer appears inside a
block a `restor` is supposed to bracket.

**Gated:** `extra/airline_x11regression-nomodel-logtd` (new). It pins the
oracle's actual invariant -- with no model, a log transform changes no X-11
table -- as a pair with `-nomodel-priortd`, which deliberately carries no
`transform{}`.

**Mutation:** dropping the restore fails **20** gates. Suite 6508 -> 6530
passed, 0 failed, 0 xfailed.

**Observed once, not reproduced:** one `-n 8` run had
`airline_pickmdl-backcast-oos` exit `0xC0000005` in `x13run_m3`. It passed
serially and on the next full parallel run, and its spec contains no
`x11regression{}`, so this increment cannot reach it. Logged because an access
violation is not a flake worth forgetting -- the harness carries large
stack-resident arrays and this project has already hit `0xC00000FD` once for
that reason.

## 73. `prterx` -- the Census routine whose name says "print" and whose job is `abend`. An unported error stop, and the class swept to exhaustion.

Board item 2 since entry 61 opened it. `x11regression{ variables=(td)
aictest=(user) }` makes the ORACLE stop:

```
 ERROR: Irregular regression matrix singular because of Mon.
        Check irregular regression model.
```

This engine ran it to `OUTCOME: OK` and published a full seasonal adjustment.

**Entry 61's recorded hypothesis was wrong, on both counts.** It guessed the B
iteration's `adrgef` restore and the following `regvar` each add a copy of the
user column, so the C iteration fits a duplicated design. The abend is at the
**B** iteration (`x11mdl.f:253`, under `Kpart.eq.2` -- the file's own comment
says "change to B iteration - march 6 2006"), and the design is EMPTY, not
duplicated. Writing the hypothesis down still paid: it named a specific thing to
disprove, and disproving it took one run.

**What actually happens.** x11aic runs its three tests in order -- trading day
(:148-297), Easter (:298-458), user (:462-591) -- and only the first two call
`regvar`. With `Xtdtst==0` and `Xeastr==F` both are skipped, so :463-464's
`IF(estend) CALL regx11(A)` fits whatever design is resident. Nothing built one:
x11mdl's own first `regvar` is at :388, AFTER the x11aic call at :253, and
`x11pt2.f:720`'s `IF(Ixreg.eq.1) CALL loadxr(F)` has just restored `Nrxy` from
`Nxrxy` (loadxr.f:84) -- parked by `gtinpt.f:830`'s `loadxr(T)` at PARSE time,
before any `regvar` ran. `Ixreg` stays 1 (editor.f:1976-1978 promotes to 2 only
for a holiday, a fixed prior or a narrowed span), so `xrgdrv` never runs either.

**TWO ROW COUNTS, and reading them as one is the trap.** `prterx.f:52` reprints
the design over `Nrxy` -- zero rows under six live column headers, which is what
the oracle's `.out` shows -- while `regx11.f:49-50` fits `Nspobs` of them. The
empty print is NOT the design `regx11` saw. The singularity is in the CONTENT of
`Xy`, which still holds the regARIMA-era matrix reinterpreted at the x11reg
model's stride.

**The port had all of this right already.** Instrumenting the C++ at the user
branch printed `nspobs=144 ncxy=7 nb=6 nrxy=0 nxcld=10 xy= 0 0 0 0 0 0 0 0` --
`nrxy=0` exactly like the oracle, `xy` all zeros -- and `regx11` returned
**false**. The engine detected the singular column. It just had nowhere to say
so:

```cpp
if (!regx11(ctx)) return;      // and the run continues to OUTCOME: OK
```

**The defect is one unported routine, and the reason it was missed is its
name.** `prterx.f` is 59 lines that resolve the offending column out of
`Colttl`, write a two-channel diagnostic, optionally reprint the matrix -- and
call `abend`. It sits in the `prt*` family, which this port defers WHOLESALE and
by design (there is no `.out` print engine here at all). Deferring the print
deferred the stop. Every `CALL regx11` in the oracle is followed by
`IF(.not.Lfatal.and.Armaer.eq.PSNGER)CALL prterx()` -- x11aic.f:171/207/338/465/
538/597, x11mdl.f:417, rgtdhl.f:63, idotlr.f:878/995 -- so this was not one
missing guard but the guard missing from all of them.

**THE CLASS IS NOW SWEPT, so nobody has to wonder again.** Eleven `prt*`
routines call `abend`. Classified by what precedes the call:

| routine | abends | preceded by a save-file open check |
|---|---|---|
| prtacf, prtd8b, prtfct, prtmdl, prtmsr, prtrev, prtrts, prtrv2, prtxrg | 1-4 each | **all of them** |
| `prterr` | 5 | none -- and it IS ported, used across automdl/regarima |
| `prterx` | 1 | none -- this entry |

Every other `prt*` abend is a save-file-open failure (`locok`/`fcnok`), and this
port writes no save files at all, so they are structurally unreachable rather
than deferred. **The two `prt*` routines that are error REPORTERS rather than
printers are exactly `prterr` and `prterx`, and both are now ported.** The class
is closed; do not re-derive it.

**What is ported here.** `prterx` + a `prterx_if_singular` helper carrying the
Fortran's own guard (`!Lfatal && Armaer==PSNGER`) rather than "regx11 returned
false" -- regx11 also returns false when olsreg already raised a fatal, and
prterx must not fire on that. Wired at all seven live call sites (rgtdhl is an
`Xhlnln=F` no-op here). `prterx.f:49-55`'s matrix reprint goes to Mt1 and stays
deferred with the rest of the print engine; the Mt2 text is byte-identical to
the oracle's `.err`.

**One test-harness consequence, and it had a mechanism waiting.**
`test_m1_parse.py::test_outcome_matches_oracle` compares the PARSE harness's
outcome against the oracle's, keyed on ERROR lines in the golden `.err`. A
post-parse abend is not a parse verdict, and that file already carved out the
same case for `edge/log-zero-series.spc` via `_POST_PARSE_FATAL`. The new spec
joins that set. A second, message-keyed mechanism was written first and thrown
away: two mechanisms for one concept is the duplicated ownership this repo's own
docs warn about.

**Gated:** `extra/airline_x11regression-aictest-usersing` (new) via
`test_x11regression_abend`, which is DISCOVERED -- every x11regression spec whose
blessed oracle `.err` carries an `ERROR:` line, with a floor assertion. The same
`_oracle_abended` predicate now also excludes abending specs from `CASES` and
`AIC_CASES`, which would otherwise have pulled this spec into table gates that
assert `OUTCOME: OK`.

**Mutation:** making `prterx_if_singular` a no-op fails **2** gates -- the new
abend gate and `test_m1_parse`. That second one is the interesting half: the
outcome-parity gate had existed all along and would have caught this defect the
day it was introduced. What was missing was never a gate. **It was a spec.**
Entry 61 wrote this spec, measured it, and then REMOVED it rather than gate it.

Suite 6530 -> 6535 passed, 0 failed, 0 xfailed, 681 skipped; ctest 12/12; WALLS
23 gaps unchanged.

## 74. `Grpx(-1)` is not undefined behaviour -- it is documented COMMON aliasing. And measuring that found a live gap the wall was too narrow to cover.

Board item 1's first half, blocked since entry 65 on "decide whether an OOB read
gets reproduced (and on what evidence)". The evidence is now in, and it changes
the question.

**The read.** `editor.f:1783-1786`, reached when `aictest=(td)` names a trading
day that `variables=` does not, so `Tdgrp==0`:

```fortran
ELSE IF (Xtdtst.eq.1.or.Xtdtst.eq.3)THEN
 begcol=Grpx(Tdgrp-1)
 endcol=Grpx(Tdgrp)-1
 IF((Xtdtst.eq.1).and.(begcol.eq.endcol))THEN
  Xtdtst=3
```

`Grpx` is `DIMENSION Grpx(0:PGRP)` with `PGRP=PB=80`, so `Grpx(-1)` is one
element below the lower bound.

**It resolves to a determined address.** `xrgmdl.cmn:49`:

```fortran
COMMON /cx11rg/ Clxptr,Grpx,Gpxptr,Nbx,Ncoltx,...
```

`Clxptr(0:PB)` is declared immediately before `Grpx(0:PGRP)` in the same COMMON,
and Fortran storage association makes a COMMON block contiguous in declaration
order. `Grpx(-1)` is therefore `Clxptr(PB)` -- 81 integers INSIDE the block, not
off the end of it. No wild pointer, no fault, no compiler roulette. The
subscript is non-conforming; the address is not.

**Proved, not argued.** A probe compiled against the vendored headers read-only
(the `tools/ref_*.f` pattern) poisons both arrays with distinguishable values
(`Clxptr(i)=1000+i`, `Grpx(i)=2000+i`) and runs editor's own two lines from a
subroutine so `-O0` cannot fold them:

```
 Clxptr(PB)          =     1080
 Grpx(0)             =     2000
 begcol = Grpx(-1)   =     1080
 alias is Clxptr(PB)? T
```

**Why the comparison then succeeds.** `Clxptr(PB)` is `Colptr(PB)`:
`loadxr.f:38` does `cpyint(Colptr(0),PB+1,1,Clxptr(0))`, copying all 81 elements
regardless of how many are meaningful. Nothing writes `Colptr(80)` in any
realistic model -- it would take 80 regressors -- so it holds the block's static
0. `Grpx(0)` is 1, so `endcol` is 0, `begcol` is 0, and `Xtdtst` flips 1 -> 3:
**`td` silently becomes `td1coef`.** The same probe over a zeroed block
reproduces exactly that.

**Stable in the real oracle, across column counts.** Three specs, all
`aictest=(td)` with no `td` in `variables=`, differing only in how many
x11regression columns they carry (`easter[8]`; + 1 user column; + 3 user
columns -- 1, 2 and 4 columns):

| spec | columns | `aictest.xtd.reg` |
|---|---|---|
| easter[8] | 1 | `td1coef` |
| easter[8] + u1 | 2 | `td1coef` |
| easter[8] + u1..u3 | 4 | `td1coef` |

Control: every gated spec that DOES name `td` in `variables=` reports
`aictest.xtd.reg: td`. The flip is keyed on the absence of a TD group and
nothing else -- consistent with a read that lands on an index the model's own
size never reaches.

**THE FINDING: the wall was narrower than the divergence.** The refusal guarding
this block tested `Nbx == 0` -- "no regression variables". But `Nbx == 0` is a
PROXY for the actual trigger, which is **no trading-day group**. A spec with one
non-TD variable has `Nbx == 1` and sailed straight through:

| key | engine | oracle |
|---|---|---|
| `aictest.xtd.reg` | `td` | `td1coef` |
| `aictest.xtd` | **yes** | **no** |
| `aictest.xtd.aicc.notd` | -744.271 | -736.359 |
| `aictest.xtd.aicc.td` | -758.367 | **-1732.145** |
| `aictest` | (absent) | `none` |

Both at `OUTCOME: OK`. A 974-unit AICC difference and an inverted accept/reject
verdict, unwalled, for as long as nobody wrote `variables=(easter[8])
aictest=(td)`. The condition is now `Xtdtst > 0 && no_td_group`, which subsumes
the old `Nbx == 0` arm (no variables implies no TD group) and closes the hole.
WALLS count unchanged at 23 -- the wall was WIDENED and reworded, not added,
which is exactly why a count is not a coverage measure.

**What is decided and what is not.** Decided: the read is deterministic, its
target is named, and reproducing it costs one explicit line -- the C++ arrays
are separate `farray1lb` objects (`xrgmdl_cmn.hpp:11-13`), not
storage-associated, so the alias has to be WRITTEN (`ctx.xrgmdl.clxptr(prm::PB)`)
rather than inherited. Not decided, and still the user's call: whether this port
reproduces a documented COMMON aliasing at all. That is a policy question about
faithfulness, not the evidence question entry 65 was blocked on. The AO half of
board item 1 (the ~7.6 AICC on an auto-AO design) is untouched by this and stays
open.

**The lesson, and it generalises past this block.** A wall keyed on a PROXY for
its trigger is narrower than the divergence it claims to cover, and the gap is
invisible because the wall LOOKS conservative -- `Nbx == 0` reads like "the
degenerate case", and it is, just not the degenerate case that matters. Same
family as the discovery-predicate rule: `endswith("seats")` and `Nbx == 0` are
both stand-ins for a condition nobody wrote down. When you wall something,
state the trigger in the condition, not a symptom of it.

Suite 6535 passed, 0 failed, 0 xfailed, 681 skipped; ctest 12/12; WALLS 23 gaps
/ 4 faithful.

## 75. Porting the alias (option B) -- and finding that the two halves of this front were never separable.

Entry 74 measured `Grpx(-1)` and left a policy call: reproduce a documented
COMMON aliasing, or stay walled. Taken: **reproduce it** (CB-37), which meant
porting `editor.f:1760-1846` rather than refusing it.

**Why B and not the cheaper shapes.** Two alternatives were rejected on
measured grounds, not taste:

- *Hardcode the flip* (`if (tdgrp == 0 && xtdtst == 1) xtdtst = 3;`). Reads
  better, and is right for every model anyone will run. But `Colptr(PB)` is NOT
  provably 0: `insptr.f:54-55` writes up to `Ptrvec(Nelt+1)` and
  `adrgef.f:363` passes `PB` as the bound, so a 79-regressor model reaches index
  80 and the comparison fails. Hardcoding manufactures the exact failure shape
  this project exists to catch -- right answer, wrong reason, silent when the
  precondition breaks.
- *Co-locate the COMMON* so `grpx(-1)` lands on `clxptr(PB)` naturally. Maximally
  faithful, and it would disarm `farray1lb::operator()`'s bounds check
  (`farray.hpp:57`) for every array in the block. That check is how a stray
  subscript surfaces as a crash instead of a wrong number. Not worth one case.

So the alias is written explicitly, which is also the honest form -- the C++
mirrors ARE separate objects, and pretending otherwise would hide the fact:

```cpp
const int begcol = (tdgrp == 0) ? xg.clxptr(prm::PB) : xg.grpx(tdgrp - 1);
```

**What else had to come with it.** Lifting the wall meant porting the whole
block, not just the read: the three td/tdstock agreement refusals
(editor.f:1765-1796), the flip, and the generatability refusals
(editor.f:1828-1845). Two details reproduced verbatim rather than corrected:

- `editor.f:1832` tests `Xtdtst.eq.3.or.Xtdtst.eq.4` -- the two "1coef"
  flavours -- while its message says **"stock trading day"**. Condition and
  message disagree; the stock flavours are 2 and 4.
- the pre-1776 refusal writes **Mt2 first and STDERR second**
  (editor.f:1839/1841/1843), the opposite channel order to every other message
  in the block.

**THE FINDING: the two halves of board item 1 were never separable, and the
handoff had them listed as independent.** The plan was to gate the flip on
`aictest.xtd.reg` / `aictest.xtd`, which the port now matches exactly. It also
had to match the AICCs, and those came out ~7.9 off. That is the AO half --
and it is not avoidable, because:

> `x11regression{}` requires a trading-day OR holiday regressor ("Must adjust
> for either trading day or holiday"). So *no TD group* forces a holiday, which
> makes `editor.f:1727` choose `Otlxrg` (automatic AO identification) over
> `Sigxrg=2.5`. **Every spec that can reach the flip also reaches the auto-AO
> path.**

There is no spec that isolates one from the other. Listing them as two
independent sub-items was wrong.

**What made it gateable anyway.** The flip has exactly one consequence that
lands at PARSE time, before any AICC exists. On QUARTERLY data the rewritten
`Xtdtst==3` walks into editor.f:1832 and the run is REFUSED:

```
 ERROR: Need monthly data to perform aictest for stock trading day.
```

A message about *stock* trading day, for a plain `td` request, on a series that
is merely quarterly -- the message only makes sense once you know the flip
happened. Without the flip `Xtdtst` is still 1, that arm does not fire, and
`Sp==4` passes the next arm cleanly. So the refusal **cannot occur unless the
aliased read occurred**, which makes it a gate for the read and nothing
downstream of it. Predicted from the Fortran, then confirmed against the
oracle.

**The second proxy-wall, in the same function, one day after the first.** Entry
74's lesson was "a wall keyed on a proxy for its trigger is narrower than the
divergence". The AO wall next to it was keyed on `Xeastr` -- also a proxy. The
real trigger is `Otlxrg`. `variables=(easter[8]) aictest=(td)` has `Xeastr`
FALSE, so the moment the flip stopped refusing that spec, it sailed past the AO
wall too and returned `OUTCOME: OK` with the 7.9-off AICCs. Fixing one proxy
exposed the next. Now `Otlxrg && no_td_group`.

That wall also had to be guarded on `inptok`: it is a PORT artifact, not
Fortran, and a spec the oracle already rejected must come back with the
oracle's message and nothing else, or the ERROR-text half of the M1 parse gate
sees two lines where the golden has one. That is a general rule for walls
placed after a refusal path.

**A third gap, found and walled rather than ported.** `Xaicst` (the stock-TD
day-of-month, editor.f:1802-1808) and `Xaicrg` (the change-of-regime date,
:1811-1822) are READ by this port already -- `x11reg.cpp:642/658` hand them to
mktdlb/addtd -- and were only ever WRITTEN to their gtinpt.cpp:226-227 defaults
(31 / NOTSET). The read-but-never-written half of the parsed-but-unread class.
Walled under `Stdgrp>0` and `Xrgmtd`; on the board.

**Gated:** `extra/expgs_x11regression-aictest-tdflip-qtr` (new), through
`test_m1_parse::test_outcome_matches_oracle` (outcome + ERROR text) and
`test_x11regression_abend` (which discovered it by itself, from the blessed
`.err`). **Mutation:** replacing the alias with a sentinel so the flip never
fires costs **2** gates.

Suite 6535 -> 6537 passed, 0 failed, 0 xfailed, 681 skipped; ctest 12/12; WALLS
23 -> 24 gaps (one wall lifted, three added: Xaicst, Xaicrg, and the widened AO
one).

## 76. The "auto-AO AICC gap" was neither auto-AO nor an AICC gap -- a holiday-only x11regression skipped its whole prior pass.

Board item 1 for two sessions, carrying a measured ~7.9 AICC divergence and a
wall. Both the diagnosis and the framing were wrong, and the way they were wrong
is the lesson.

**What it actually was.** `x11regression{variables=(easter[8])}` -- a HOLIDAY-ONLY
irregular regression -- never ran its transparent `xrgdrv` prior pass. `B1` came
back as the RAW series where the oracle had already divided the Easter factor out
(8.8e-3), the automatic AO identification found nothing where the oracle keeps
`AO1960.Mar`, and the seasonal filter selection flipped with it (`3x3` vs `3x5`,
D7 trend MA 9 vs 13). All at `OUTCOME: OK`.

**Five defects, stacked.** Each was invisible until the one above it was fixed:

1. **`Axrgtd` is a proxy, and FOUR guards used it.** The oracle's entry condition
   is `x11ari.f:91`'s `IF(Ixreg.eq.2.or.Khol.eq.1)`. `Axrgtd` stands for "the
   irregular regression has a prior to estimate" -- but `editor.f:1722` CLEARS it
   when there is no trading-day group, leaving `Axrghl` as the only flag set. The
   four: `run_pre_model.cpp` (model path), `x11_prestage.cpp` (no-model path),
   `xrgdrv.cpp`'s own entry test, and `x11parts.cpp`'s Ixreg==3 Faccal restore.
2. **`xrgdrv`'s entry test RETURNED TRUE.** A silent no-op, not a wall. This is
   why the whole thing survived: an unported path that fatals is visible in the
   corpus the day a spec reaches it; one that returns quietly is not.
3. **`Easgrp` was read and never written.** `readers_spec.cpp` computed it into a
   LOCAL and threw it away; `x11reg.cpp:1122` derives `Holgrp` from it. With
   `Holgrp==0` and no TD group, `x11mdl.f:308`'s identity-factor NOTE branch fired
   and the irregular regression was not fitted at all.
4. **`gtxreg.f:889`'s `Axrghl=T` had been deliberately skipped**, with a comment
   saying every holiday-carrying spec in the corpus was bit-exact without it.
   True -- and the corpus had no HOLIDAY-ONLY spec, which is the only shape where
   the flag is load-bearing. Taken now; no gated spec moved.
5. **`x11ref.f`'s `Tdgrp==0` arms were not ported.** The `Tdgrp>0` arm adds
   `Xn/Xnstar` -- the month-length ratio -- to Fcal; the `Tdgrp==0` arm (`:133-135`)
   adds ONE and touches Ftd only for a STOCK TD group. Having only the first put
   the month-length ratio into a factor that is supposed to be holiday-only, and
   February came out off by exactly 28/28.25.

Also: `Tdgrp/Stdgrp/Holgrp` had to become PARAMETERS of `x11ref_td`, because
`pritd.f:44` passes the literals `1,0,0` while `x11mdl.f:694/814` pass the live
COMMON. Reading ctx would have given pritd whatever the irregular regression left
behind -- the call-site rule from entry 71, hit again.

Result: `variables=(easter[8])` and `variables=(easter[8]) aictest=(td)` are now
BIT-EXACT -- every D-table at 1e-15, and all 153/157 shared udg keys, including
the two AICCs that were the whole board item (`-736.359339510335` /
`-1732.14491025233`, previously `-744.271419570893` / `-1740.01162137319`).

**Why it was misdiagnosed twice, which is the part worth keeping.**

- *First reading:* the AICCs are ~7.9 out, and with no TD group `editor.f:1727`
  picks `Otlxrg` over `Sigxrg=2.5`, so the AIC baseline must be fitted on an
  automatic-AO design -- the oracle's .out does add `AO1960.Mar` at t=-5.50.
  Plausible, and checkable in thirty seconds: `x11mdl.f` calls `x11aic` at `:253`
  and does the AO identification at `:424`. The aictest baseline never sees an AO
  design. **The evidence for the story was in the same file as its refutation.**
- *Second reading:* entry 75 recorded the AICC gap as NOT SEPARABLE from the
  CB-37 `Grpx(-1)` flip, because x11regression demands a trading-day OR holiday
  regressor, so "no TD group" forces a holiday. The inseparability argument is
  correct. The conclusion drawn from it was not: it was used to justify NOT
  building the cheaper spec. Dropping `aictest=` reproduced the entire divergence
  -- and that one probe is what located it, because it removed x11aic from the
  picture and left the plain holiday-only path standing there alone.

The standing rule this earns: **when a divergence is attributed to feature X,
build the spec WITHOUT X before porting anything.** Engine-vs-oracle was measured
here. Cheap-spec-vs-expensive-spec was not, and that is the measurement that
names the subsystem. An inseparability proof tells you two features co-occur; it
never tells you which one owns the delta.

Second rule, from defect 2: **an unported path that returns success is worse than
one that has no code at all.** Walls are inventory -- `walls.py` lists them and a
deleted one leaves the list. A silent `return true` is in neither the wall list
nor the gate count.

**Left open, honestly.** `x11ref.f:87`'s `IF(Holgrp.gt.0)` outer guard on the
Fhol fold is NOT reproduced. It is a no-op with no holiday column, so it only
bites when Fhol is nonzero while Holgrp is 0 -- exactly what an Easter AICtest
leaves behind (`x11aic.f:64` clears Holgrp; neither the accept arm at `:445` nor
the estend=F path restores it). Adding the guard costs **54 gates** (c16 3.4e-4
on `airline_x11regression-aictest-easter8` and siblings), so on the vendored
binary the fold demonstrably happens. Something restores `Holgrp` that is not
visible in `x11aic.f`. Folding unconditionally is what the measurement says, and
that is what the code does; this is an open question, not a Census-bug claim,
because the Fortran has not been instrumented directly.

**Near-CB, not claimed.** `x11ref.f:19` declares `Trumlt` LOGICAL; it is not a
dummy argument, it is in no COMMON, and nothing assigns it. Line 88 reads an
uninitialized local. It is only reachable inside `.and.Tdgrp.gt.0`, where the
vendored binary behaves as if it were `.true.`, and it cannot reach the no-TD
path at all. Reproduced by taking that arm. Not filed as a CB entry: per the
standing rule, measure before naming a Census bug, and this one has been measured
only through its effect on gates, not directly.

**Gated:** `extra/airline_x11regression-holiday-only` (new),
`extra/airline_x11regression-holiday-only-aictest` (new).
**Mutation:** restoring the `Axrgtd` proxy at the pre-model call site costs
**39** gates.
**Wall DELETED**, not widened -- WALLS 24 -> 23 gaps. Suite 6537 -> 6581 passed,
0 failed, 0 xfailed; ctest 12/12.

## 77. `x11regression{reweight=}` was parsed and discarded -- and finding its readers turned up a mis-dispatched argument, a missing `regfix()`, and an ordering that silently ate `b=`.

Board item 1. `reweight=` (`gtxreg.f:553` -> `Lxrneg`) was consumed by the
argument loop and written nowhere, while **three** ported readers took the
permanently-false flag as fact. Measured before touching anything, oracle
on-vs-off: a log-additive `tdprior = (1.4 1.4 1.4 1.4 1.4 -0.5 0.5)` with
`reweight=yes` moves `a4` by 1.2e-2 at 1949.Jan. Engine-vs-oracle then gave the
same delta at `OUTCOME: OK` -- the option did nothing at all.

**The three readers.**

1. `editor.f:1511` -- the negative prior-TD weight clamp, already ported in
   `gtinpt.cpp` and already reading `ctx.x11log.lxrneg`. The weights in that spec
   sum to exactly 7.0 as given, so without the clamp the standardization factor
   is 1 and the `-0.5` survives; with it the weight becomes 0, the total is 7.5,
   and all seven are scaled by 7/7.5.
2. `x11mdl.f:577-624` -- the reweighting proper. Unported.
3. `revdrv.f:327` -- the history reset, already ported.

**The reweight had to move up a phase.** `x11mdl.f` builds the X-11 daily
weights `Dx11` at `:541-570`, reweights at `:577-624`, and only then calls
`x11ref` at `:694`. This port had the `Dx11` build BELOW its `x11ref_td` call,
which was harmless for as long as the single consumer (the Kswv==3 combine)
re-ran `x11ref_td` for itself. The reweight is not like that: `:610` writes back
into `B`, so every factor `x11ref` builds afterwards comes off the rewritten
coefficients. The build is now hoisted above the `x11ref_td` call and the Kswv==3
block consumes the same `dx11`.

**Reaching it took a constructed spec, and the construction is the interesting
part.** `Dx11 = 1 + B`, so a negative weight needs a TD coefficient below -1.
Additive mode would give `Dx11 = B` directly, where any negative coefficient
does -- but `x11pt1`'s additive/pseudo-additive prior TD is walled, so that
route is closed. `editor.f:1640` refuses a FIXED coefficient below -1 outright.
What is left is the DERIVED Sunday weight `Dx11(7) = 1 - sum(B)`: fix five of
the six day contrasts high enough that the estimated sixth cannot pull the sum
back under 1.

The window is narrow and both edges are gated:

| five fixed at | derived Sunday | what happens |
|---|---|---|
| 0.35 | -0.9753 | `Dx11(7)` = +0.02, nothing fires |
| **0.39** | **-1.0566** | `Dx11(7)` = -0.057 < 0, Saturday's 0.107 is the one positive unfixed weight -- the reweight runs |
| 0.43 | -1.1431 | Saturday's own estimate crosses -1 too, `tdwsum` is 0, `x11mdl.f:613-623` abends |

0.39 and 0.43 are both gated. 80 `c16` lines and 290 `d11` lines separate 0.39
from the same spec with `reweight=no`.

**CB-38, measured not inferred.** `editor.f:1655` is 71 characters long and
breaks the word `when` across the fixed-form continuation, so the blank pad at
column 72 lands inside it: the oracle prints `less than zero w hen specifying`.
The neighbouring `:1663` breaks at a word boundary, where the same pad supplies
the space the text needed -- which is exactly why this one reads as a typo
rather than as a mechanism. Confirmed by running the vendored binary, not by
reading the source.

**NOT claimed: the stale `icol`.** `x11mdl.f:597-602` TESTS `Dx11(7)` and
ADDS/ZEROES `Dx11(icol)`, where `icol` is the DO variable the loop above left at
`ncol0+1`. For a six-column TD group the two agree by accident; for `td1coef`
(`ncol0==1`) it reads element 7 and writes element 2. It is transcribed verbatim
and left as an OPEN QUESTION, because the divergent case needs an UNFIXED
`td1coef` coefficient above 0.4 -- and fixing it to get there makes the group
all-fixed, which `editor.f:1660` answers by clearing `Lxrneg` before this code
runs. No spec reaches it, so per the standing rule it is not a CB entry.

### Three defects found on the way in, none of them about reweighting

**The computed GO TO was off by one.** `gtxreg.f`'s dispatch maps argidx 30 to
label 300 (`umtrimzero`, ZRODIC yes/span/no) and 31 to label 310 (`centeruser`,
URRDIC mean/seasonal). This port dispatched the CENTERUSER reader on **argidx
30**, and had a comment citing `gtxreg.f:537-543` -- which is label 310 -- right
above it. So the line numbers in the comment and the index in the code disagreed
with each other, and had for as long as the branch existed. It ran both ways:
`umtrimzero = seasonal` was accepted at `OUTCOME: OK` where the oracle errors,
and a real `centeruser=` fell through to the generic consume and was discarded.
Decoding an ARGDIC by its pointer table takes about a minute and would have
caught it; nothing else would, because both arguments were inert.

**`regfix()` was never called for the x11reg design.** `gtxreg.f:861` calls it;
`loadxr.f:49` then copies `Iregfx` into `Irgxfx`; and `editor.f:1640` (this
work), `editor.f:1675` (the stock-TD check) and `gtxreg.f:866` (`Userfx`) all
read the result. The port had the `Userfx` block -- under a comment saying
"gtxreg.f:858-877 -- Iregfx from the b= fixings, then Userfx" -- but not the
call that produces `Iregfx`. So every one of those readers was testing whatever
fix state the regARIMA parse had left behind. This is the read-but-never-written
shape again, with the distinguishing feature that the comment ASSERTED the write
happened.

Adding the call needed the matching restore, which is the `restor`-stand-in trap
for the fourth time: `restor.f:69` puts `Iregfx` back after `loadxr(T)`, and
`xrg_clear_working` -- the stand-in here -- clears `Regfx` but not `Iregfx`.
Snapshotted alongside `Picktd`.

**`rmlnvr` ran a phase too late, and `b=` paid for it.** `gtxreg.f:186-192`
strips the Leap Year column INSIDE the `variables=` branch, so `Nb` is 6 by the
time `:608` compares it against the length of the `b=` list. This port ran
`rmlnvr` after the whole argument loop, so `Nb` was still 7 there. Consequences,
both at `OUTCOME: OK`:

- a correct 6-value `b=` list took the count-mismatch branch and was DISCARDED
  (`gtxreg.f:609`'s message is a plain WRITE that never touches `Inptok`, so the
  run continues with no coefficients applied -- which is what makes it silent);
- a 7-value list, which the oracle rejects, was applied.

Hoisted into the `variables=` branch where the oracle has it. The 6-value
direction is gated by every new `b=`-carrying spec here. The 7-value direction
is NOT gated, and the reason is worth recording: `gtxreg.f:609`'s message starts
with `ERROR:` but is not a fatal, and BOTH of the parity suite's derived
predicates -- `test_m1_parse::_oracle_ok` and
`test_x11regression_tables::_oracle_abended` -- read an `ERROR:` line in a
blessed `.err` as a rejection. A spec for it reports two false failures. Rather
than bolt a name-list exception onto a deliberately derived predicate, the spec
was dropped and the measurement recorded here.

**Gated:** `extra/airline_x11regression-reweight` (the reweight itself),
`-reweight-off` (the A/B control), `-reweight-abend` (`x11mdl.f:613-623`),
`-reweight-tdprior` (`editor.f:1511`), `-reweight-allfixed` (`editor.f:1660`'s
NOTE and the `Lxrneg` clear), `-reweight-fixneg` (`editor.f:1652`'s refusal, and
CB-38), `-umtrimzero` (the argidx off-by-one). All seven new.

Suite 6582 -> 6678 passed, 0 failed, 0 xfailed. WALLS unchanged at 23 gaps.

**Still open here.** `x11mdl.f:661-690`'s ELSE arm -- the stock-trading-day
nonpositive-factor abend -- is not ported, and it is an unguarded path rather
than a wall, which is the shape entry 76 named as worse than no code at all. It
needs its own spec first: note that `:664` reads the x11reg STORE
(`Grpttx/Gpxptr/Ngrptx`) where `:545` twenty lines up reads the WORKING model
(`Grpttl/Grpptr/Ngrptl`), so which model it tests is itself a question to
settle. Also `slidingspans.cpp:395` still carries "Nbx==0 always in this port",
which x11regression makes false.

## 78. Two `chs` gaps with one symptom -- and a comment that had merged them. `slidingspans{}` and `x11regression{}` had never met.

`slidingspans.cpp:395` skipped `setssp.f:353`'s `ssxmdl` under the comment "out
of scope, Nbx==0 always in this port". True when x11regression was unported,
false ever since. Nothing caught it because **no corpus spec combined
`slidingspans{}` with `x11regression{}`** -- checked, and that absence is the
whole reason the claim stood. This is the same shape as entry 76's `xrgdrv`: not
a wall, not a gate, a silent skip justified by a fact that expired.

**What the pairing found.** `sfs` bit-exact across all four spans, every D-table
and b16/c16 bit-exact -- and `chs`, the per-span seasonally-adjusted-series
change table, wrong in **408 of 600 cells**, worst 5.0e+0. So the per-span
SEASONAL factors are right and the per-span CALENDAR factor is not.

`ssxmdl` itself turns out to be inert on this spec -- no `x11regression{span=}`,
nothing fixed, `Irgxfx==1`, so none of its four decisions (Ssxint, rvfixd's
Tdfix/Holfix, the Itd/Ihol demote, the `Lxrneg` reset) fire. The claim was
harmless HERE. It was never checked, which is the finding.

**The attribution that was wrong, and how one run settled it.** `chs` was
already a recorded KNOWN GAP for `airline_slidingspans-td` (regression{td} +
transform=log), attributed to a per-span PRIOR PHASE -- each span placing Adj[0]
at its own Setpri. `slidingspans.cpp` then claimed this family's `chs` was "the
same per-span prior-phase problem". Cheap-spec-vs-expensive-spec, two runs:

| spec | chs cells wrong |
|---|---|
| airline + slidingspans + x11regression + log | 408 / 600 |
| same, `x11regression{}` DELETED | **0 / 600 -- bit-exact** |
| same, `transform{function=log}` DELETED | 408 / 600 |

Deleting the feature the other gap is about changes nothing; deleting
x11regression fixes it completely. **Two gaps, one symptom, different owners.**
The standing rule from entry 76 -- build the spec WITHOUT X before believing X
owns the delta -- caught a misattribution that was sitting in a code comment
rather than in a diagnosis, which is the harder place to find one.

**What the defect is not.** `ssx11a.f:96-97` moves the irregular regression's
own span (`Begxrg`/`Endxrg`) onto each sliding span, and this port had them
frozen at their parse-time values while `Begspn`/`Endspn` moved underneath --
so `x11reg.cpp:1097-1098` measured each span against the full series and
`x11mdl.f:115-118` could narrow by a bogus offset. That was the obvious
candidate. It is now ported (`run_x11_span`'s `set_xrg_span`, a call-site switch
because `revdrv.f:500-511` uses different arithmetic for the same field) and it
is **measured inert**: 408/600 with it and without it, including on a spec whose
`x11regression{span=}` makes `nbeg` positive so the narrowing precondition is
genuinely non-empty. Kept because it is what ssx11a does -- without it the port
relies on `nbeg`/`nend` happening to come out non-positive -- and labelled inert
at its site so nobody reads it as the fix.

**What is measured about the real one.** The engine's `chs` DOES respond to
x11regression: 427 of 600 cells move when the spec drops it. So the irregular
regression is running per span, on wrong inputs, and lands nearer the no-TD
answer than the oracle's. That is where the next session starts.

**Gated:** `extra/airline_slidingspans-x11regression` -- sfs, b16, c16 and the
D-tables bit-exact; `chs` a KNOWN GAP with the golden committed. Suite 6678 ->
6702 passed, 0 failed, 0 xfailed.

## 79. `fixx11reg` defaults to YES -- the per-span calendar gap was a parsed-but-unread option, and the fix's partner had already been measured and rejected

Entry 78 closed with `chs` wrong in 408 of 600 cells on
`airline_slidingspans-x11regression`, `sfs` bit-exact, and the finding named as
"the per-span CALENDAR factor". That was right. This entry is how it closed, and
the two rules it cost.

**The measurement that named it.** `slidingspans{}` publishes five per-span
tables and the spec saved two. Adding `tds` (trading-day spans) and `ads` (SA
spans) to the ORACLE's save list -- one run -- turned a 408-cell symptom into a
one-line diagnosis:

| table | oracle span 1..4 at 1951.Jan |
|---|---|
| `tds` | 99.144951 / 99.144951 / 99.144951 / 99.144951 |
| main run's `c16` at 1951.Jan | **99.144951** |

The oracle's per-span trading-day factor is byte-identical across all four spans
AND equal to the main run's final C-iteration factor. It does not re-estimate the
irregular regression per span at all. The engine did.

**Why. `Ssxint` is TRUE by default.** `gtinpt.f:531` sets it; `getssp.f:257`
(`slidingspans{fixx11reg=}`) only ever overrides it. This port parsed the option
into `sspinp.ssxint` and **never read it anywhere** -- the single most common
defect shape in this port, and the standing rule says to find the READ, not the
parse. `ssxmdl.f:140-150`, skipped since entry 78 recorded it as "inert on this
spec", is that read:

```fortran
      IF(Ssxint)THEN
       CALL setlg(T,PB,Regfxx)
       IF(Irgxfx.lt.3)Irgxfx=3
```

Entry 78 checked the four decisions ssxmdl makes that are keyed on the SPEC (a
`span=`, a fixed `b=`, `Irgxfx>=2`) and found all four inert. It did not check
the one keyed on a DEFAULT. **An inertness proof that enumerates the arms a spec
can switch on is not a proof about the arms a default switches on.**

Confirmed on the binary rather than by reading: `fixx11reg=no` moves the per-span
TD factor 99.144951 -> 98.847324 at 1951.Jan. The default is load-bearing.

**THE PARTNER, AND THE RULE THIS EARNS.** With `Irgxfx=3` the fix still does
nothing unless the span actually re-enters the irregular regression, because the
factors it would re-apply are loaded by `xrgdrv`'s `loadxr(F)` and `xrgdrv` only
runs when `Ixreg==2` (`x11ari.f:93`). That demote -- `ssx11a.f:93-94`'s
`Ixreg=1; IF(Lmodel)Ixreg=2`, plus `sspdrv.f:127`'s `IF(Ixreg.eq.3)Ixreg=2` --
was **measured alone in an earlier session, found to take `sfs` from bit-exact to
4.1e+0, and rejected in a comment that told the next reader not to copy it.**

That measurement was correct and the conclusion was wrong. Alone, the demote
makes every span REFIT its own daily weights, which is strictly worse than
reusing the main run's. Together with the fix it makes every span RELOAD them:
`loadxr(F)` brings in `Bx` (the main run's C-iteration coefficients, put there by
`xrgdrv.f:206`'s `loadxr(T)`), `Iregfx==3` has `x11mdl`'s `rmfix` strike every
column, and the OLS has nothing left to estimate.

> **A feature measured with its partner missing measures the partner.** The
> earlier probe's number was real; "makes it worse" was a fact about a half
> change. When a rejected-by-measurement note sits next to an unported routine,
> re-measure it with that routine before trusting the note -- and write the note
> so it says WHAT ELSE WAS MISSING at the time, which the original did not.

**Third piece: the B seed.** `x11mdl.f:168-175`, inside its `IF(Kpart.eq.2)`,
seeds `B` from `Bx` on any `Issap==2 / Irev==4` replay when any of
`Nssfxx>0 / Nrvfxr>0 / Ssxint / Revfxx` holds, and zeroes it to `DNOTST`
otherwise. With everything fixed those seeded values ARE the applied weights, not
a starting guess.

**Fourth: a table that did not exist.** `tds` has exactly one producer on an
x11regression run -- `x11mdl.f:874`'s `ssrit(Factd,...,1)`. `x11pt2.f:136`'s
ssrit is keyed on `Adjtd.eq.1`, the regARIMA trading day, and takes its
`Itd=0; IF(Axrgtd)Itd=1` else-branch instead. Neither `x11mdl.f:874` nor
`ssap.f:208`'s `mflag(Td,...)` was ported, so the engine emitted no `tds` at all
and the parametrised gate skipped it with `"spec does not produce this tag"` --
**a parametrisation that shrank, reporting green.** `test_slidingspans_tables.py`
now carries the floor assertion the discovery rule asks for: cross the
parametrisation against the goldens on disk, both sides derived, and fail on any
blessed table with no case pointing at it.

**Fifth, and the sixth time this seam has bitten.** Once the demote landed,
`xrgdrv` runs INSIDE every span -- so `x11mdl`'s own published artefacts became
span state. `b16`/`c16`/`.xrm` came out of the run holding the LAST SPAN's 84
rows where the main run has 144. Nothing about a demote looks like it writes
those, which is the whole argument for taking the save/restore set by the rule
(whatever a consumer re-derives from) rather than by what a change appears to
touch. `run_x11.cpp`'s span-replay set now carries `x11reg_b16`, `x11reg_c16`,
`x11reg_xrm{,_ncol,_begxy}`, `x11reg_tdwt`, `x11reg_combtdwt`, `x11reg_ran`.

**What is walled rather than skipped.** Three arms of ssxmdl are unreachable
without a spec the corpus does not have, and each now refuses on its exact
trigger rather than on a proxy: the `rmotss` outlier re-check
(`slidingspans{x11outlier=no}` + automatic x11regression outliers), the
`rvfixd` / `Irgxfx>=2` fixed-design arm, and the `bakusr` user-regressor arm.
`tools/walls.py` learned the new helper name so they are inventoried; WALLS
23 -> 26 gaps.

**Gated:** `extra/airline_slidingspans-x11regression` now saves and gates
`sfs`, `chs`, `ads` AND `tds` -- all four bit-exact across all four spans,
alongside b16/c16/xrm and every D-table. The `chs` KNOWN GAP entry is DELETED
from `test_slidingspans_tables.py` rather than re-worded; the OTHER `chs` gap
(`airline_slidingspans-td`, the per-span prior phase) is untouched and still
open, which is what entry 78's cheap-spec-vs-expensive-spec separation was for.
Suite 6702 -> 6706 passed, 0 failed, 0 xfailed.

## 80. The stock-trading-day abend: an unguarded refusal, and an `ELSE` that pairs with a different `IF` than everyone assumed

`x11mdl.f:661-690` refuses a run whose STOCK trading-day irregular regression
would produce nonpositive multiplicative daily factors. It was neither ported
nor walled -- entry 77 found it while hoisting the `Dx11` build past it and
recorded it as an open item rather than fixing it. Measured before porting, on
airline + `x11regression{variables=(tdstock[15]) b=(-1.5f -0.1f ...)}`:

| | |
|---|---|
| oracle | `ERROR: At least one of the stock trading day regression coefficient ...` then abend |
| engine | `OUTCOME: OK` |

The silent-wrongness class, exactly as entry 76 named it: an unported path that
returns SUCCESS is worse than one with no code.

**THE FINDING IS THE PAIRING.** The first transcription put the block under
`ELSE` of `:541`'s `IF(Havxtd.and.(.not.Haveum))` -- which is what the handoff
note said, and what the surrounding prose implies -- guarded it on
`!havxtd && muladd==0`, and produced a branch that **could not fire**: a probe
print showed `havxtd=1` on the very spec the oracle refuses.

Decoding the `END IF` chain mechanically settles it:

```
660 ENDIF closes 578      (the Lxrneg reweight block)
661 ELSE  pairs with 546  <-- IF(igrp.gt.0), the "Trading Day" group lookup
690 ENDIF closes 546
691 ENDIF closes 541      (IF(Havxtd.and.(.not.Haveum)))
```

So the arm means: an x11regression trading day IS present, and the WORKING model
has no `Trading Day` group -- which is what a STOCK design looks like. That reads
as obviously right once seen, and it is the opposite of the first reading. This
is the same lesson as entry 77's ARGDIC decode, now in its second form:

> **Count the block; do not read the indentation, the comment, or the prose
> around it.** Fortran has no closing-token names, so an `ELSE` seven levels down
> looks identical to one at the top. A twenty-line script that walks `IF ... THEN
> / ELSE / END IF` and prints the pairing takes a minute and cannot lie.

The wrong version was caught only because a debug print was added when the branch
did not fire. Had the spec merely been slightly off instead, the dead branch
would have shipped looking correct.

**Two oracle oddities transcribed rather than tidied.** `:664` looks the group up
in the x11regression STORE (`Grpttx/Gpxptr/Ngrptx`) and then indexes the WORKING
model's `Grp` with the index it gets back, where `:545` twenty lines above uses
the working model for both. They are identical whenever nothing has mutated the
design since `loadxr(F)` -- every spec this corpus can build -- and diverge once
x11aic has added or struck a column. Same family as CB-37's `Grpx(-1)` alias, and
deliberately NOT filed as a Census bug: no spec here reaches a state where the two
disagree, and "measure before naming a Census bug" applies. Second oddity: the
test is `B < -1 .or. dpeq(B,-1)`, a `<=` written as a disjunction with the
equality half on `dpeq`; transcribed as the same two comparisons.

**The message is written to the channels directly, not through `writln`.** FORMAT
1070 carries `//` pairs, which emit EMPTY records; `writln`'s `lblnk` blank is
`(' ',a)` with a space argument -- two characters, not zero. The first version
differed from the oracle's `.err` by exactly that, on two lines. Compared byte
for byte afterwards.

**Gated on BOTH sides.** `extra/airline_x11regression-tdstock-abend` (fires;
joins `test_x11regression_tables.py`'s discovered ABEND_CASES and
`test_m1_parse.py`'s `_POST_PARSE_FATAL`) and `extra/airline_x11regression-tdstock`
(same design, coefficients that stay positive; must reach `OUTCOME: OK`, and
gates its b16/c16/xrm and D-tables). A refusal gated only on the side that fires
cannot tell "correctly refused" from "refuses everything" -- the
saturated-precondition trap in its guard form. These are also the first
x11regression STOCK trading-day designs in the corpus at all.

The ABEND_CASES floor moves 1 -> 3. A floor left at its original value stops
detecting a shrink the moment the second case exists.

Suite 6706 -> 6728 passed, 0 failed, 0 xfailed.

## 81. The sliding-spans NOTEs: a diagnostic whose whole effect is an ABSENCE, and the channel nobody could read

Board item 1, first of the three ssxmdl arms entry 79 walled -- the
`x11regression{span=}` arm at `ssxmdl.f:27-39`. It was PORTED and UNGATED: no
spec in the corpus made `Begxrg` later than `Begspn`, so the arm's three-line
NOTE and its `Itd/Ihol` demote to -1 had never been compared to anything.
`extra/airline_slidingspans-x11regression-span` closes that -- `sfs` 4.9e-15,
`chs` 4.8e-15, both 600 cells, first run.

**The arm's whole observable is a table that is NOT produced.** `Itd=-1` means
"trading day requested but not analysed": `ssap.f:206-211` then writes no `tds`
and no `ads`, and the oracle's save files simply are not there. That is exactly
the shape the save-table gate cannot see -- `test_slidingspans_table` skipped an
absent golden with the reassuring message "spec does not produce this tag", the
same sentence that hid the missing `tds` producer for months (entry 79).

Two things came out of taking that seriously.

**(a) The `.err` channel was unreadable on a successful run, so nothing on it
was gated.** `x13run_x11` dumped the Mt2 channel to stderr only when the run
FATALs. Every non-fatal NOTE and WARNING the engine writes went into a buffer
that was then thrown away. Both the x11 and seats harnesses now dump it between
`===ERR===` / `===END ERR===`, the format `x13run_m2` has always used.

**(b) With the channel readable, an unported diagnostic fell out of a golden
that has been committed for months.** `ssphdr.f:145-152` writes two NOTEs to
BOTH `Mt1` and `Mt2` when `Itd` or `Ihol` is -1 -- the text that says the TD
statistics are suppressed and why. Almost all of `ssphdr` is `Mt1`, i.e. the
deferred `.out` print engine, and the routine went with it. But
`airline_slidingspans-td`'s blessed `.err` has carried that NOTE since the spec
landed (its `Itd` is demoted by `setssp.f:47`, `fixmdl=` defaulting to yes),
and nothing read the file. Ported now, both arms, written straight to the
channel because FORMAT 2000/2001's `/` separators emit EMPTY records while
`writln`'s `lblnk` blank is `(' ',a)` -- two characters (entry 80, second time
this has mattered).

Same family as `prterx` (entry 73): a load-bearing routine swallowed by a
subsystem this port defers wholesale, because of the family it is named into.
`ssphdr` really is 95% print engine. The 8 lines that are not go to the error
channel, and they are the only record that a table was suppressed on purpose.

One faithful-scope note attached to it: the Fortran gates `ssphdr` on
`Prttab(LSSSHD).or.Savtab(LSSSHD)` and then on `Lprt`. This port has no
print-table dictionary at all (`Prttab`/`Savtab` are parsed-and-dropped by
design, `readers_val.cpp:874`), so the NOTE is emitted whenever the header
stage is reached: exact for `print=all` and for the default, over-emitting for
a `slidingspans{print=}` list that excludes the header. Recorded rather than
faked -- a fake would need the dictionary this port deliberately does not have.

**Both new gates are proven able to fail, in both directions.** That is the
point rather than a formality, because an absence-shaped assertion is the one
most likely to pass vacuously:

| mutation | result |
|---|---|
| NOTE guard `sa.itd == -1` -> `== -99` (never emits) | 2 fail (`-td`, `-span`) |
| NOTE guard -> `!= -99` (always emits) | 5 fail -- the four NOTE-free specs, plus one |
| `tds` guard `sa.itd == 1` -> `!= 0` (emits when demoted) | 1 fail (`-span`) |

The middle row is the one that matters: four of the seven discovered specs have
NO note block, so the corpus can tell a correct emitter from one that shouts on
every run. `test_slidingspans_notes_can_fail` asserts that both sides stay
non-empty, so a corpus that later drifts to all-one-side fails loudly instead of
degenerating into a trivially-true comparison.

The absent-golden skip is likewise no longer free: when the SPEC's save list
names the tag and the oracle still wrote nothing, the engine must produce zero
cells for it. Conditioned on the save list on purpose -- this harness dumps
every span table it computed regardless of `save=`, so on a spec that never
asked, an absent golden says nothing at all.

Scope of the NOTE comparison is `NOTE:` blocks only. The `WARNING:` blocks in
the same `.err` files come from the spectrum section of the deferred `.out`
print engine (the peaks themselves are gated through the savelog `spcrsd` /
`peaks` udg keys), so a whole-file comparison would fail for an unrelated and
already-tracked reason; that exclusion is stated in the test, not implied.

Suite 6728 -> 6762 passed, 0 failed, 0 xfailed.

## 82. `slidingspans{fixreg=}` was parsed and dropped, and the ssxmdl wall was keyed on the wrong variable

Board item 1's remaining two `ssxmdl` arms. One of them closed; the other is
still walled; and getting there turned up a third thing that was neither --
`slidingspans{fixreg=}` returning `OUTCOME: OK` with statistics the oracle
suppresses.

### What was walled, and what the wall actually tested

`ssxmdl_span` refused on

```cpp
if (xg.irgxfx >= 2 || si.nssfxx > 0) { ... }
```

The first disjunct is the real trigger and is exactly what `ssxmdl.f:85`
tests. **The second is dead in this port**: `Nssfxx` is only ever written by
`setssp.f:338-341`, a block the port had skipped with the comment "not
reachable, the gate corpus has no fixreg= argument". So the wall's fixreg=
half tested an output that nothing produced, while the INPUT it meant to
guard -- `Nssfxr`, which the parser fills from `fixreg=` and which
`readers_spec.cpp:2370` had been filling all along -- walked straight past it.

Measured, airline + `slidingspans{fixmdl=no fixreg=(td)}` + `regression{td}`:

| | oracle | engine before |
|---|---|---|
| `tds` rows | none written | **120** |
| `ads` rows | none written | **120** |
| `.err` NOTE | ssphdr's TD-suppressed block | none |
| outcome | OK | **OK** |

That is the parsed-but-unread class again, and the same shape as entry 79's
`fixx11reg=`: the option is documented, the parser honours it, and the read
does not exist.

### The four flags are ONE quartet, not two pairs

`setssp.f:320-341` decodes `Ssfxrg` into `tdfix/holfix/usrfix/otlfix` and
passes them to BOTH `ssmdl` (regARIMA design) and `ssxmdl` (x11regression
design). `ssmdl` writes `Tdfix`/`Holfix` **back** -- its `Iregfx==2` arm sets
them true and ANDs them down over the groups -- and `ssxmdl` then reads them
in `.not.Tdfix`. Porting either routine's block alone would have been
self-consistent and wrong, which is why `ssmdl_fix_model` now takes the pair
by reference.

`rvfixd` is called from all three sites (`revdrv.f:131-134`, `ssmdl.f:53`,
`ssxmdl.f:80`), always on the arrays rather than on `model.cmn`, precisely so
one walk can serve either design. It moved out of `run_history.cpp`'s
anonymous namespace into `core/src/regarima/rvfixd.hpp` unchanged.

### The store writes `ssxmdl`'s rvfixd makes are UNDONE four lines later

`ssxmdl` brackets its whole body with `loadxr(F)` at :42 and `loadxr(T)` at
:137. `loadxr(T)` copies `Irgxfx`/`Regfxx`/`Usrxfx` back OUT of the regARIMA
working model that `loadxr(F)` had just copied them INTO -- so every fix
`rvfixd` makes to the x11regression store is reverted before the routine
returns, and the only consumer of those fixes is the group walk twenty lines
below. The port makes neither `loadxr` call, so the arm is bracketed by an
explicit save/restore that stands in for the pair.

**What that stand-in does NOT reproduce, recorded rather than faked**: the
round trip also leaves the regARIMA WORKING model overwritten by the
x11regression design (`Ngrp`, `Grp`, `Rgvrtp`, `Nb`, `Priadj`, `Begxy`,
`Userfx`, `Tddate`, `Lma`/`Lar`/`Mxdflg`/... zeroed, and more), and
`setssp.f:356`'s `IF(Lmodel)CALL restor(Lmodel,F,F)` puts back only the
`restor` subset. This port does neither half and is therefore self-consistent
today, on every gated spec. It is a divergence in waiting, not a defect with
a witness -- and it is the same shape as the `restor`-stand-in family in entry
72.

### THE MUTATION THAT PASSED, AND WHAT IT MEANT

Five mutations, one per ported arm. Four failed loudly. The fifth --
disabling `ssmdl`'s `Iregfx==3` arm -- **passed**, on a spec written
specifically to exercise it: `regression{variables=(td) b=(six fixed)}`, every
coefficient fixed, which is the definition of `Iregfx==3`.

A debug print settled it: `nb=6`, all six `Regfx` true, `Iregfx=2`. The cause
is `getreg.f`'s Leap Year splice (ported at `readers_spec.cpp:1101`): with
`picktd` and a log transform, a Leap Year column is inserted into the `b=`
list, `regfix.f:31` sees a column whose value is `DNOTST`, `allfix` dies
there, and `regfix.f:41`'s promotion to 3 never fires -- even though `rmlnvr`
deletes that column again moments later. The spec was on the `Iregfx==2` arm
the whole time, and its header comment claimed `ssmdl.f:57-70`.

`airline_slidingspans-regallfixed` reaches the real arm, via
`variables=(tdnolpyear)`: same six contrasts, no leap-year column to splice.
With it, mutation five fails on its own spec like the other four.

The general lesson, and it is not the one already written down: **a mutation
that passes is a claim about WHICH ARM the spec is on.** The existing rule
says a passing mutation usually means the precondition is saturated. This one
was subtler -- the precondition was fine, the routine was entered, the
observable was correct, and the spec simply arrived through a different door
than its comment said. Mutation-test each ARM, not each routine; a spec that
reaches the routine is not a spec that reaches the arm.

### Two `.err` NOTEs the newly-readable Mt2 channel exposed

Entry 81 made the Mt2 buffer readable on a successful run. It keeps paying:

* `arima.f:936-960` -- "Fixed values have been assigned to some regression
  coefficients." Unported. It is on the golden `.err` of every spec here that
  fixes a regARIMA coefficient, so `_UNPORTED_NOTES` subtracts it from the
  GOLDEN side of the note gate rather than skipping those specs, and
  `test_unported_notes_still_unported` fails the day no golden carries it.
* `prtmdl.f:174-177` -- the `Nliter > 200` NOTE. It appeared on a draft of
  `airline_slidingspans-regfixed` whose fixed coefficients (0.39) were so far
  from the data that the ARMA maximisation blew past the `.udg`'s `niter`
  field width and the oracle wrote `niter: ***`, which no harness can parse.
  Plausible coefficients (0.001) removed both problems. Still unported, no
  corpus carrier -- deliberately NOT listed, because a `_UNPORTED_NOTES` entry
  no golden carries would fail its own freshness check.

A third, `ssap.f:113-116` ("Sliding spans percentages cannot be stored in a
separate diagonstics file..."), is gated on `Lsumm>0` -- a command-line
summary-file flag this port does not model at all. It showed up only on the
transient `mode=add` draft of the regallfixed spec and is out of scope by
design, not by omission.

### The gap these specs sit on, isolated the way the rules require

Four of the six new specs fail `sfs`/`chs` numerically. The divergence would
naturally have been attributed to the arms just ported. **Build the spec
WITHOUT the feature**: `airline_slidingspans-fixmdl-no` is
`airline_slidingspans-td` plus the single line `fixmdl = no`, carries no fixed
coefficients and no `fixreg=`, and fails `sfs` as well as `chs`. The owner is
`slidingspans{fixmdl=no}` + `regression{}` -- per-span RE-estimation of the
regARIMA model -- and it is a third, independent gap next to the two already
in `_KNOWN_GAPS`.

Those four specs are landed anyway, because every arm of `ssmdl.f:50-121`
needs `Ssinit/=1` to be observable at all (`setssp.f:47` demotes `Itd` first
otherwise), so `fixmdl=no` is not optional for them -- and what they DO gate
today is the demote itself, which is the entire observable of the arms in
question.

### Scoreboard

| spec | arm | what it pins |
|---|---|---|
| `x11reg-allfixed` | ssxmdl `Irgxfx==3` | walk skipped, `tds`/`ads` suppressed |
| `x11reg-partfixed` | ssxmdl `Irgxfx==2` | walk runs, one free column, NOT suppressed |
| `fixreg-td` | ssmdl `Nssfxr>0` | `fixreg=(td)` honoured at all |
| `regallfixed` | ssmdl `Iregfx==3` | the arm the obvious spec missed |
| `regfixed` | ssmdl `Iregfx==2` | all columns fixed -> suppressed |
| `regpartfixed` | ssmdl `Iregfx==2` | one column free -> NOT suppressed |
| `fixmdl-no` | none | the gap the other four sit on |

Still walled: `slidingspans{x11outlier=no}` with automatic x11regression
outliers (`ssxmdl.f:44-76`'s `rmotss`), and now `fixreg=(outlier)` -- whose
`otlfix` outlives `setssp` and reaches `ssx11a` per span (`sspdrv.f:121`),
where it decides whether a held-back outlier is re-added with its coefficient
fixed. **The WALLS count did not move**: one wall removed, one added. As in
entry 74, a gap count is not a coverage measure.

## 83. `slidingspans{}` — CLOSED bit-exact, and each of the three gaps had a different owner than its note said

Entry 82 left five specs sitting behind `_KNOWN_GAPS` and a sixth
(`airline_slidingspans-td`'s `chs`) that had been there for months. All six
now gate bit-exact on every table they ship — `sfs`/`chs`/`tds`/`ads`, all
four spans, worst **~5e-15** — and `_KNOWN_GAPS` is EMPTY. Three separate
defects, none of them where the surviving prose said to look.

### 1. `Setpri` is an EDITOR-ONLY assignment, and the span driver re-anchored it

`run_x11_span.cpp` opened with `ctx.adj.setpri = ctx.x11ptr.pos1bk;`. It reads
as obviously right — `editor.f:851` is literally `Setpri=Pos1bk`, and each
span re-runs `setxpt` — and it is the bug.

`x11int.f:53` copies the prior-adjustment factors as
`copy(Adj, PLEN-Setpri+1, -1, Sprior(Setpri))`, i.e. POSITIONALLY: `Sprior` at
data position `i` is `Adj(i-Setpri+1)`, the factor for date
`Begadj + (i-Setpri)`. `Adj` is built once, by `adjsrs` from the EDITOR
(`editor.f:849` is its only call site in the whole oracle) and anchored at the
MAIN run's `Begadj`. What keeps a span's factors aligned is therefore that
`Setpri` does NOT move while the span's data does: `Lsp` slides `Pos1ob` to the
span's absolute offset inside the same padded buffer, and the positional copy
then lands the right date on the right observation for free.

Measured on an instrumented `-O2` oracle (scratchpad copy of the vendored tree;
the tree itself is never edited), printing at `x11pt2.f:115`:

```
PROBE issap 1 begspn 1949 1 begadj 1949 1 setpri 1 nadj 156 p1bk  1 p1ob  1
PROBE issap 2 begspn 1951 1 begadj 1949 1 setpri 1 nadj 156 p1bk 25 p1ob 25
PROBE issap 2 begspn 1952 1 begadj 1949 1 setpri 1 nadj 156 p1bk 37 p1ob 37
PROBE issap 2 begspn 1953 1 begadj 1949 1 setpri 1 nadj 156 p1bk 49 p1ob 49
PROBE issap 2 begspn 1954 1 begadj 1949 1 setpri 1 nadj 156 p1bk 61 p1ob 61
```

`Setpri` pinned at 1 while `Pos1bk` slides 25/37/49/61, and the dumped
`Sprior` reads `0.991150` at February 1951 and `1.026549` at February 1952 —
date-correct in every span.

**The observable.** `Priadj==4` is `lpyear`, so `Adj` is 1.0 everywhere except
February, where it is `29/28.25` or `28/28.25`; `tdlom.f:32` folds it into
`Factd`, which divides the SA series. Re-anchoring slid the whole factor series
onto the span, so each February got the factor of a February `k` years away
with `k = 1949 - span_start_year`. Where the leap parity happened to match,
nothing showed; where it did not, the SA value was out by exactly `29/28`
(leap) or `28/29` (non-leap), and `chs` — a month-to-month percent change —
moved ~3.5 points at that February and back at the following March.
**Eighteen cells, all Feb/Mar, sign following the leap year.** The
three-years-apart pattern WITHIN each span is what identified `Adj`'s anchor
rather than a missing prior: a missing prior moves every span equally.

The note in `test_slidingspans_tables.py` had called this "a per-span phase
problem in how the prior series is indexed" — correct — and then described
"each span places Adj[0] at its own Setpri" as the mechanism rather than as the
defect. **A comment can name the right subsystem, point at the right line, and
call it correct.**

### 2. `arima.f:1430`'s `CALL ssprep` is unconditional, so the spans CHAIN

With `slidingspans{fixmdl=no}` each span re-estimates. Span 1 was bit-exact and
spans 2-4 were out by 6.2e-07 / 3.9e-07 / 3.0e-08 in `sfs` — the signature of a
value that is right the first time and stale afterwards.

`restor` (`ssx11a.f:160`) copies `Ap2`/`Bb` back over `Arimap`/`B` before each
span. The port had `Ap2` written ONCE, by the main run. The oracle writes it
again at the END of every `arima` call — `arima.f:1430`, comment "Reset model
parameters for sliding spans, revisions history" — and `sspdrv.f:180`'s
`x11ari` runs `arima` per span. So each span's starting parameters are its
PREDECESSOR's converged ones, and only span 1 starts from the main run.

Confirmed from both sides before changing anything: the instrumented oracle's
per-span `Arimap(3..4)` are `0.3026/0.5462`, `0.0506/0.5092`, `0.0376/0.5195`,
and the engine now prints the same three.

The port's `ssprep_snapshot` also stashes `ctx.saved.ksdev0/lterm0/nterm0`,
which are NOT `ssprep.cmn` members — they stand in for an editor block this
port does not have and must be taken at parse time. Hence the `capture_saved`
parameter: a per-span re-snapshot of `Ksdev` would re-open the
extreme-value-mode gap that cost 3.45% on span 1.

### 3. `slidingspans{fixreg=}` does not fix anything, and the port made it work

`ssmdl.f:53`'s `rvfixd` writes only the LIVE `Iregfx`/`Regfx`. It does NOT
mirror them into `ssprep.cmn`'s `Regfx2`/`Irfx2` — unlike `ssmdl.f:342-352`,
which explicitly writes `Ap2`/`Fxa` for `fixmdl=yes`. So the `restor` inside
`ssx11a` puts the main run's all-free flags straight back and **no span ever
sees a fixed coefficient**. `fixreg=`'s entire effect is the `Itd`/`Ihol`
demote: no `tds`/`ads` table, plus the `ssphdr` NOTE.

Entry 82 had added the mirror, reasoning from this port's own trap list ("a
structural change not mirrored into the ssprep snapshot is undone by the first
span's restor" — true for `fixmdl`, true for `history{fixreg=}`, false here).
Measured, not deduced: the instrumented oracle's per-span `Arimap` AND
`B(1..7)` on `airline_slidingspans-fixreg-td` are **byte-identical** to
`airline_slidingspans-fixmdl-no`'s, and the two blessed `sfs` goldens differ in
**zero** lines. The mirror put `sfs` 8.4e-03 out on span 1 alone.

**The rule this earns:** a trap list is a list of places to LOOK, not a list of
things that are true. Three sibling features write the snapshot; the fourth
does not, and only the Fortran says which.

### `fixmdl=clear` — the third value, now carried

`INTDIC` is `'no','yes','clear'` and `Ssinit` is `ivec(1)-1`
(`getssp.f:50/156`), so `Ssinit==2` is `slidingspans{fixmdl=clear}` and nothing
else reaches it. It drives `sspdrv.f:130-143`, which resets `Arimap`/`B`/`Bx`
to `DNOTST` so each span estimates from cold starting values.

PLACEMENT is that block's whole content, and the indentation does not settle
it: read as "after the span" it is dead code (the next `restor` overwrites it),
read as "before this span's estimation" it is live. The CALL ORDER decides —
the block sits between `ssx11a` (whose tail is the `restor`) and `x11ari` at
`:180` (which runs `arima`). New spec `extra/airline_slidingspans-fixmdl-clear`
makes the answer observable: it gates bit-exact, its `sfs` golden differs from
`fixmdl=no`'s in 240 lines, and moving the block after the span fails 2 gates.

### Mutations

| gates | mutation |
|---|---|
| **12** | drop `arima.f:1430`'s per-span `ssprep` re-snapshot |
| **7** | re-anchor `Setpri` to the span's own `Pos1bk` |
| **2** | mirror `rvfixd`'s `Regfx`/`Iregfx` into the ssprep snapshot |
| **2** | move `sspdrv.f:130-143` after the span (the "dead code" reading) |
| **0** | drop the `Chx2`/`Chg2`/`Acm2` half of `ssprep`/`restor` |

The **0** is reported rather than acted on. Those three are the estimation
workspace and every span's `rgarma` rebuilds them from its own design before
reading them, so the inherited values are dead on this corpus. They are kept
because an incomplete stand-in for `restor` has produced three separate defects
in this port already, each invisible until a later phase in a different file
read one of the omitted fields.

### What is still walled

Unchanged, and none of it is reachable from a `fixmdl=`/`fixreg=` spec:
`slidingspans{x11outlier=no}` with automatic x11regression outliers
(`ssxmdl.f:44-76`'s `rmotss`), `slidingspans{}` with `x11regression{user=}`
(`ssxmdl.f:142-148`'s `bakusr`), and `fixreg=(outlier)` (`sspdrv.f:121`).
WALLS stays at 26 gaps / 4 faithful.

## 84. `Xaicst` and `Xaicrg` — the two x11regression AIC-test values the oracle recovers from its own group TITLES

The last two walls in `xrg_editor_setup`, and the read-but-never-written half
of the parsed-but-unread class: both were already READ by this port
(`x11reg.cpp:642/658` hand them to `mktdlb`/`addtd`) and only ever written to
their `gtinpt` defaults. **Both ported, both bit-exact, WALLS 26 → 24.**

What makes the pair unusual is where the values come from. Neither is carried
forward from the spec: `gtxreg` builds a human-readable group title, and the
editor parses the number back OUT of that title.

### `Xaicst` (`editor.f:1802-1808`)

The day of the month a stock trading-day regressor is measured on.

```fortran
CALL getstr(Grpttx,Gpxptr,Ngrptx,Stdgrp,igrptl,nchr)
ipos=index(igrptl(1:nchr),'[')+1
Xaicst=ctoi(igrptl(1:nchr),ipos)
```

The title is `Stock Trading Day[15]`; `ctoi` starts one past the `[` and stops
at the `]`. Default is 31, so **a spec written with `tdstock[31]` agrees whether
the read happens or not** — the inert-at-its-default trap that hid
`x11regression{aicdiff=}` (entry 54) and `fixx11reg=` (entry 79). New spec
`extra/airline_x11regression-aictest-tdstock` uses `[15]`, which nothing else
in the run can produce.

That spec pins a second thing for free: `aictest=(td)` with a stock group
present takes `editor.f:1763-1764`'s `Xtdtst 1 -> 2` rewrite, so the oracle
reports `aictest.xtd.reg: tdstock[15]` for a request that said `td`. Asking for
`tdstock` directly would arrive at 2 without exercising the rewrite.

### `Xaicrg` (`editor.f:1811-1822`)

The change-of-regime date for the trading-day AIC test, searched for over the
WHOLE `Grpttx` buffer (up to `Gpxptr(Ngrptx)-1`), not within one group:

```fortran
ipos=Gpxptr(Ngrptx)-1
rgmgrp=index(Grpttx(1:ipos),'(before ')+8
IF(rgmgrp.eq.8)  rgmgrp=index(Grpttx(1:ipos),'(change for before ')+19
IF(rgmgrp.eq.19) rgmgrp=index(Grpttx(1:ipos),'(starting ')+10
IF(rgmgrp.eq.10) rgmgrp=index(Grpttx(1:ipos),'(change for after ')+18
CALL ctodat(Grpttx(1:ipos),Sp,rgmgrp,Xaicrg,argok)
```

The idiom is `index(...) + k` followed by `IF(rgmgrp.eq.k)` — *k means `index`
returned 0, i.e. not found* — and it FALLS THROUGH: if the fourth search also
misses, `ctodat` is called at position 18 regardless and returns `argok=F`,
which is what turns the run into a parse failure. Transcribed with that
structure intact.

New spec `extra/airline_x11regression-aictest-tdregime` (`variables=(td/1955.jan/)`).
Its titles are

```
Trading Day (after 1955.Jan) + Trading Day (change for before 1955.Jan)
```

so the FIRST search misses — `'(before '` is not a substring of
`'(change for before '` — and the SECOND hits. The spec therefore exercises the
fall-through rather than only the head of the chain, which the mutation table
below confirms is a real distinction.

### The block structure, checked rather than read

`editor.f:1828`'s `IF(Tdgrp.eq.0.and.Stdgrp.eq.0)` is a SIBLING of the two
blocks above, inside the same `IF(Readok)` at `:1802` (which closes at `:1847`)
— so a failed `Xaicrg` does not skip it. Verified by walking `IF`/`ELSE`/
`END IF` mechanically (`:1803`, `:1812`, `:1828` all at depth 4), per the
standing rule from entry 80: count the block, never read the indentation. The
port has no `Readok` re-test there, and says why in a comment, because the
alternative justification — `xrgmtd` and `no_td_group` are mutually exclusive,
since a change-of-regime trading day necessarily puts a "Trading Day" group in
the design — is a reachability argument, and this is transcribed from the
Fortran instead.

### Measured before porting, both directions

Oracle with the feature: `aictest.xtd.reg: tdstock[15]`, `aicc.notd`
-771.725427813502, `aicc.td` -1703.82540926957. Engine after the port:
byte-identical, both specs, including `aictest.xtd.reg: td/1955.Jan/` on the
regime one.

### Mutations

| gates | mutation |
|---|---|
| **26** | `Xaicrg`: keep only the FIRST title shape (drop the fall-through chain) |
| **25** | drop the `Xaicrg` read entirely |
| **7** | drop the `Xaicst` read entirely |

The first two differ, which is the point of building a spec whose title needs
the second shape: a spec on `'(before '` would have made the chain untestable
and the mutation would have passed. Same family as entry 82's "a passing
mutation is a claim about WHICH ARM the spec is on".

## 85. The sliding-spans held-back outliers — a whole `ssmdl` block that was neither ported nor walled

Board item 1 was "the three remaining `slidingspans{}` walls". Reading them
turned up something worse sitting next to them: **`ssmdl.f:124-280`'s group walk
had no C++ counterpart and no refusal either.** Every `slidingspans{}` spec that
carried an outlier regressor — user-specified or automatically identified — ran
to `OUTCOME: OK` with numbers the oracle does not produce. The three walls are
inventory; this was not.

The rule it belongs to is already in `CLAUDE.md` ("an unported path that returns
SUCCESS is worse than one that has no code"), and the reason it survived is the
usual one: **no corpus spec put an outlier and `slidingspans{}` in the same
file.** Twelve slidingspans specs, every one of them `variables=(td)`.

### Measured first, and isolated before porting

`airline` + `slidingspans{save=(sfs chs)}` + `regression{variables=(...)}`,
oracle vs engine, worst relative error per span:

| spec | span 1 | span 2 | span 3 | span 4 |
|---|---|---|---|---|
| `td` only | 4.5e-15 | 4.1e-15 | 3.7e-15 | 4.7e-15 |
| `ao1950.feb td` | 4.4e-15 | 4.7e-15 | 4.0e-15 | 4.0e-15 |
| **`ao1959.nov td`** | 5.0e-15 | 5.0e-15 | **4.3e-03** | **3.6e-03** |

and in `chs` the same spec read **4.6e+0 / 1.2e+1** on spans 3-4. Deleting the
`ao=` term reproduces nothing, so the owner is the hold-back and not the
presence of a `regression{}` group — the entry-76 discipline, applied before any
code was written this time.

The date matters because the span geometry decides the verdict. Airline
(1949.Jan-1960.Dec) gives four spans 1951-1957, 1952-1958, 1953-1959,
1954-1960, so `begss`=1951.Jan and the INTERSECTION is 1954.Jan-1957.Dec.
`rmotss` is a three-way verdict over those two windows:

* dated before `begss` → `dlrgef` outright, never stored. No span can estimate
  it and none will ever ask for it back.
* outside the intersection → title, coefficient and fix flag into the store
  (`otlrev.cmn`), column deleted. Each span's `adotss` re-adds the ones its own
  window covers.
* inside the intersection → left alone.

### The machinery, and where each piece goes

Five call sites, and the port had none of them:

| Fortran | what | ported as |
|---|---|---|
| `ssmdl.f:124` | clear the store | `intlst` on `ctx.otlrev` |
| `ssmdl.f:246-278` | the group walk: `rmotss` per outlier column, both the user arm and the automatic one | inside `ssmdl_fix_model` |
| `ssmdl.f:358-373` | `IF(regchg)` re-snapshot the design | `ss_snapshot_design` |
| `ssx11a.f:229-270` | per span: delete what this span cannot estimate, then `adotss` re-adds what it can | `ssx11a_span_outliers`, called from `run_x11_span` |
| `sspdrv.f:208-219` | after the span: strip the re-added columns, `ssprep` | `ssp_strip_span_outliers` |

Two details that are easy to get wrong and were:

**The store is not consumed.** `chkorv` (its `history{}` twin) erases each entry
as it re-adds it, because a revision history's spans only grow. Sliding spans
SLIDE, so `adotss` re-tests the whole store every span and `sspdrv.f:208-219`
takes the columns back out afterwards. Draining the store the way `chkorv` does
would give span 1 the outlier and no later span.

**`ssmdl.f:345`'s `IF(.not.regchg) CALL copy(B,PB,1,Bb)`.** The `fixmdl=yes`
tail skips its own `Bb` snapshot exactly when the walk changed the design,
because `dlrgef` has shifted every coefficient down past the deleted column and
the regchg store at `:370` is about to take `Bb` again from the NARROWED design.
Snapshotting unconditionally indexes the next span's `restor` one column out.

### The `Lx11` flag on `ssprep`, and the span-1-exact signature again

First build of the above closed `p_late` from 4.3e-03 only to 8.8e-04, and moved
the error onto spans **2, 3 and 4 with span 1 still bit-exact** — the exact
shape entry 83 documented for the missing `arima.f:1430` chain, and the same
diagnosis applies: something written at the end of span *j* is read by span
*j+1*.

`sspdrv.f:218` is `CALL ssprep(T,F,F)` — `Lmodel=T`, **`Lx11=F`**. This port's
`ssprep_snapshot` had no `Lx11` parameter and always wrote `Lt2`/`Ktc2`/`Tc2`,
which was harmless for its only other post-estimation caller (`arima.f:1430`,
which sits BEFORE `x11pt2` and therefore snapshots the auto-select `Lter`
sentinels) and wrong here: `sspdrv.f:218` runs AFTER `x11pt2` has RESOLVED them,
so span 2 started from a seasonal-filter length chosen for span 1. Adding the
flag and passing `false` took every probe to ~5e-15.

**The general form is a new one and it is nastier than "add the argument".** The
port had been getting `arima.f:1430` right by ACCIDENT of placement, not by
honouring `Lx11=F`; the parameter was omissible for exactly as long as there was
one caller. That is the same shape as entry 71's `Ksdev` restore — a
compensation that is correct at one call site and a defect at the next, with
identical code.

### The change-of-regime arm is walled, and the oracle halts there — CB-39

The other half of the same group walk (`ssmdl.f:150-241`) is NOT ported. It is
walled, and the wall is honest rather than conservative, because **the oracle
does not survive that path either**: `ssmdl.f:159` searches the group title for
`'(change from before '` and *no title producer in the tree writes it* —
`addlom.f:63`, `addtd.f:88`, `adrgim.f:72/178` all write `'(change for before '`,
and `regvar.f:334` / `savmdl.f:346` / `editor.f:1816` all SEARCH for `for`. Only
`ssmdl.f:159` and `rdregm.f:25` spell `from`. Both searches miss, the
fall-through hands `ctodat` position 20 of the title, the date parse fails, and
the run prints its NOTE and halts.

Measured: airline + `slidingspans{}` + `regression{variables=(td/1955.jan/)}`
→ oracle console `Program error(s) halt execution`, no `.sfs`, no `.chs`; this
port wrote 336 sfs cells and 332 chs cells at `OUTCOME: OK`. Full write-up in
`tools/census_bugs.md` CB-39.

**That spec could not be gated by any test in this file.** `_discover()` in
`test_slidingspans_tables.py` requires an `sfs` AND a `chs` golden, so a spec the
oracle halts on contributes zero cases and reports green by absence — the same
family as entry 79's `skip("spec does not produce this tag")`, one level up: not
an absent TABLE but an absent SPEC. The new gate derives its case list from the
blessed `.stdout.txt` ("Program error(s) halt execution"), asserts the engine
FATALs, and asserts it produced no span table. It has its own floor assertion,
because a derived list that shrinks to nothing is green.

**And the assertion it makes was, at first, one that could not fail.**
`x13run_x11` printed the `.err` buffer and `return 1`-ed on a FATAL without
dumping anything else, so "the engine produced no span table" was true of a spec
that produced nothing at all, for any reason. Fixed here rather than worked
around: on a fatal the harness now dumps everything it computed first, guarded
on the X-11 pointers actually being set so an EARLY fatal (a parse refusal)
still prints nothing rather than an uninitialised buffer. Third time this
lesson has been paid for in this subsystem — entry 81 was the Mt2 channel on a
SUCCESSFUL run, this is the table channel on a FAILED one.

**A halting spec belongs in `tests/corpus/edge/`, not `extra/`.** Blessed into
`extra/` it was discovered by a dozen table and diagnostic gates across the
suite — `d10`-`d16`, F2/F3, QS, spectrum, the bindings — every one of which
asserts the harness exited 0, which for this spec it cannot and should not.
That is 14 failures with nothing wrong underneath, and the fix is filing, not
tolerance: `edge/` is where the specs whose SUBJECT is a refusal already live,
and no table gate scans it. Note what the 14 failures also proved on the way
past: the oracle halts LATER than this port refuses — it finishes the whole
X-11 spine and writes D10-D16 and its `.udg` before dying in the sliding-spans
setup — so the wall is a real gap and not a faithful refusal, which is how
`walls.py` records it.

### Specs

* `extra/airline_slidingspans-outlier-heldback` — `ao1950.feb` (rmotss branch 1,
  deleted outright) **and** `ao1959.nov` (branch 2, held back and re-added in
  spans 3-4). The first is observationally inert on its own — `Ssinit==1` fixes
  the coefficient and the column is all zeros inside every span window — and is
  in the spec to keep the branch executed, which the header says so nobody
  "simplifies" it away.
* `extra/airline_slidingspans-outlier-auto` — the automatic arm
  (`ssmdl.f:259-278`: the `PRGTAA→PRGTAO` / `PRGTAL→PRGTLS` / `PRGTAT→PRGTTC`
  re-type, and the unconditional `regchg`). `critical = 2.5` is deliberate: at
  the default the run finds ONE outlier and it lands where every verdict is
  inert, while at 2.5 the nine finds split across all three verdicts including a
  late `AO1960.Mar` that only span 4 can take.
* `extra/airline_slidingspans-outlier-fixmdlno` — `fixmdl = no` with the same
  held-back `ao1959.nov`, so the spans RE-ESTIMATE and `adotss`'s fix flag is
  false. Added after the mutation battery, because without it "force the flag
  true" was a mutation no spec could fail.
* `edge/airline_slidingspans-regime-td` — the CB-39 halt. In `edge/` on
  purpose; see above.

### Mutations

| gates | mutation |
|---|---|
| **17** | drop the `regchg` design re-snapshot (`ssmdl.f:358-373`) |
| **5** | drop the whole group walk (no hold-back at all) |
| **4** | hold back but never re-add (drop `adotss`) |
| **4** | pass `lx11=true` at `sspdrv.f:218` — the first build's bug |
| **3** | drop only the AUTOMATIC arm (`ssmdl.f:259-278`) |
| **1** | drop the change-of-regime wall (the whole arm, refusal included) |
| **0** | drop the per-span delete of out-of-window columns (`ssx11a.f:229-263`) |
| **0** | drop the post-span strip (`sspdrv.f:208-219`) |
| **0** | drop BOTH of the above together |
| **0** | snapshot `Bb` unconditionally (drop `IF(.not.regchg)`) |
| **0** | force `adotss`'s fix flag TRUE |
| **0** | force `adotss`'s fix flag FALSE |

**The re-snapshot is the biggest single line in the increment** (17): without it
the first `restor` reinstates every held-back column and the whole walk is a
no-op, so it fails more gates than deleting the walk it protects.

**The first run of this battery reported every number ~14 too high**, because
the halting spec was still blessed into `extra/` at the time and was failing 14
gates in the baseline. Both runs agree once that is subtracted, and the
discrepancy is itself the lesson: **a mutation count is a DELTA and the harness
never measured the baseline.** It does now, by running against a green suite.

**Five zeros, reported and not acted on.** Each was chased far enough to name
the saturation rather than shrug at it:

* the per-span delete and the post-span strip are complementary — the strip
  removes what `adotss` added, the delete removes what this span's window does
  not cover — so with either live the other has nothing to do. Removing BOTH is
  still 0, because `restor_span` reinstates the held-back design at the top of
  every span anyway, and a column left over from a previous span is dated
  outside this one, i.e. all zeros. Under `fixmdl=yes` its coefficient is fixed
  and contributes exactly nothing; under `fixmdl=no` it is estimated against a
  zero column. Kept, for the reason entry 83 kept `Chx2`/`Chg2`/`Acm2`: an
  incomplete stand-in for `restor` has produced three separate defects in this
  port and each was invisible until a later phase in a different file read one
  of the omitted fields.
* `Bb` unconditional is 0 because nothing between `ssmdl.f:345` and `:370`
  writes `B`, so the two snapshots take the same array from the same source.
  It would stop being 0 the moment something did.
* the fix flag is 0 in BOTH directions, and that is measured on a spec built
  specifically to sit on the other arm — `extra/airline_slidingspans-outlier-
  fixmdlno`, whose `fixmdl = no` makes `Otlfix.or.Ssinit.eq.1` false where every
  other spec makes it true. So this is not entry 82's "the spec was on a
  different arm than its comment claimed": the arms are covered and the corpus
  still cannot separate them at 1e-6. Transcribed from the Fortran, and the
  zero left standing.

## 86. `slidingspans{fixreg=(outlier)}` — the wall was one line, and lifting it falsified entry 83's headline

The last cheap item on the sliding-spans wall list, and it cost almost no code:
`rmotss` and `adotss` had landed in entry 85, so the only thing still missing
was that `Otlfix` never left `setssp`. What the gate then found is that the
sentence this port had been repeating about `fixreg=` — *it fixes nothing in
the oracle* — is false on exactly the specs entry 85 added.

**The threading.** `setssp.f:323-334` decodes `Ssfxrg` into four flags.
Three of them (`tdfix`, `holfix`, `usrfix`) are locals of `setssp`'s caller and
die there. `Otlfix` does not: `sspdrv.f:66-67` declares it, `setssp` writes it
through, and `sspdrv.f:121` hands it to `ssx11a` **once per span**. This port's
`setssp_span` had no out-parameter, so the flag was unreachable past setup and
the option was refused. It now returns through `bool& otlfix_out` and
`run_slidingspans` forms `ssx11a.f:268`'s real disjunction,
`Otlfix.or.Ssinit.eq.1`.

**Oracle on-vs-off, and the arm that makes it observable.** Measured first, per
the standing rule, and the first measurement said the option does nothing:
adding `fixreg = (outlier)` to `extra/airline_slidingspans-outlier-heldback`
changes not one cell of `sfs` or `chs`. That is because `fixmdl` DEFAULTS to
yes, `Ssinit==1`, and the disjunction is already true. Under `fixmdl = no` it
separates hard — **192 of the `sfs` lines and 190 of the `chs` lines move**.
The gate spec therefore carries both lines, and it also saves `tds`/`ads`,
which `fixmdl = no` leaves un-demoted so all four tables are compared.

### The disjunction this port just ported is provably redundant

`adotss` computes `fx = Fixotr(icol).or.Otlfix`. `rmotss` wrote that same store
entry as `Fixotr = Regfx(icol).or.Otlfix`, from the **same run-constant flag**.
So the expression is `Regfx.or.Otlfix.or.Ssinit==1` with or without the
`Otlfix` on `ssx11a.f:268`. Measured to match, three ways: forcing `ss_otlfix`
true, forcing it false, and deleting `Otlfix` from `rmotss`'s store write are
**0 gates each**. This is not entry 82's saturated-precondition zero — the
corpus has specs on both arms — it is an algebraic identity.

Kept anyway, and the reason is entry 85's rule: the argument the Fortran
passes is the argument this port carries, because a narrowed signature is a
defect waiting for its second call site (`ssprep`'s `Lx11`). The redundancy is
a property of the two writers agreeing today, not of the routine.

### What actually moves the numbers, and why entry 83's claim was too broad

Entry 83 measured `fixreg=` against an instrumented oracle and concluded:

> `rvfixd` writes only the LIVE `Iregfx`/`Regfx`, `ssmdl.f:53` does not mirror
> them into `ssprep.cmn`, so `restor` inside `ssx11a.f:160` puts the main run's
> all-free flags back and no span ever sees a fixed coefficient.

Every clause of that is true, and the conclusion drawn from it — that
`fixreg=` fixes nothing — is not. **The measurement was taken on a spec with no
outlier regressor.** With no outlier there is nothing to hold back, the
`ssmdl.f:124-280` group walk leaves `regchg` FALSE, and nothing re-snapshots
the design. Put one held-back outlier in the same spec and `ssmdl.f:358-373`
fires: `ss_snapshot_design` copies the **post-`rvfixd`** `Iregfx`/`Regfx` into
`Irfx2`/`Regfx2`, and from then on every span's `restor` reinstates the fixings
rather than erasing them. `fixreg=` fixes coefficients after all, by a route
that belongs to the outlier walk and not to `fixreg=`.

Mutations, which is how this was localised rather than argued:

| mutation | result |
|---|---|
| `otlfix` never reaches `ssmdl`'s `rvfixd` | **4** gates fail |
| the `regchg` re-snapshot stops carrying `Irfx2`/`Regfx2` | **5** gates fail |
| `rmotss` stores `Fixotr` without the `Otlfix` half | 0 |
| `ss_otlfix` forced true / forced false / `setssp` drops the out-write | 0, 0, 0 |

So the owner is `rvfixd` plus the re-snapshot, in combination. Neither alone is
the feature: `rvfixd`'s write is transient, and the re-snapshot only preserves
whatever happens to be live when the walk ends.

**The standing rule this earns.** A measurement that a feature is INERT is
scoped to the state the probe spec was in, exactly as much as a measurement
that it is live. Entry 83's probe could not have shown otherwise — the
mechanism that makes `fixreg=` bite runs only when another feature has already
modified the design. When a note says an option does nothing, check what else
the probe spec did NOT have; "no observable effect" and "no effect" differ by
whatever the corpus was missing at the time. Same shape as entry 79's rejected
`Ixreg` demote, measured alone and correct alone, and wrong as a conclusion.

**Gated by** `extra/airline_slidingspans-outlier-fixreg` (`fixmdl = no` +
`fixreg = (outlier)` + `ao1950.feb ao1959.nov td`, saving `sfs chs tds ads`) —
hand-authored, header records both measurements. WALLS 25 -> 24 gaps.

## 87. `slidingspans{}` + automatic x11regression outliers — one wall, and behind it an unwalled crash

Board item 1's second-cheapest wall was `slidingspans{x11outlier=no}` with
automatic x11regression outlier identification. Reading it found the same shape
entry 85 found next door: **the wall guarded one arm of a branch whose OTHER
arm had no C++ counterpart either, and no refusal.** `ssx11a.f:99-154` — the
x11regression half of the per-span outlier bookkeeping, the whole
`loadxr(F) … loadxr(T)` block — was simply absent.

**No corpus spec paired the two features.** Four `slidingspans{}` specs carry
`x11regression{}` and not one sets `critical=`; twenty-two carry
`slidingspans{}` and the only `critical=` among them is the regARIMA
`outlier{}` one. So `Otlxrg` and `Issap==2` had never been true together.

### What the probe found, in the order it found it

| spec | oracle | engine, before |
|---|---|---|
| `critical=3.5` (defaults) | fine | **bit-exact** |
| `+ fixx11reg=no` | fine | **`ERROR: Adding AO1953.Feb exceeds the number of regression effects allowed in the model (80)`** |
| `+ x11outlier=no` | fine | walled (refusal) |
| `+ both` | fine | walled |

The default arm being bit-exact is the trap. `ssx11a.f:107-118` strikes the
previous span's automatically identified AO columns so this span's `x11mdl`
re-identifies from a clean design — and with `fixx11reg=` at its default yes,
nothing is refit, so nothing is re-identified, so there is nothing to strike.
**A feature measured only where its partner is absent measures the partner**
(the entry-79 rule, third instance). Turn `fixx11reg=no` on and the engine
appends a fresh AO set per span on top of the last until it hits `PB=80`, three
spans in, where the oracle finishes.

`critical=2.5` is not usable on airline: the ORACLE refuses it with the same
80-regressor message during the main run. `3.5` leaves seven automatic AOs.

### Three things had to land

**1. `ssx11a.f:99-154`, inside the `loadxr` swap.** `loadxr(false)` puts the
x11regression design into the WORKING model arrays — the same arrays
`ssx11a_span_outliers` reads for the regARIMA design 130 lines later, which is
why the two blocks cannot share a code path even though they do the same job.
`Ssxotl` (the default) strikes the `PRGTAA` columns; `.not.Ssxotl` deletes only
what this span's window misses and calls `adotss` on the **otxrev** store.

**2. `ssxmdl.f:44-77`, which FILLS that store.** `rmotss` again, into
`Botx/Otxptr/Notxtl/Fixotx/Otxttl` instead of the `Otr*` set. Both routines
took their store from `ctx` directly; the Fortran passes it as five arguments,
and now so does this port (an `ss_store` reference bundle). Note where the
bracket closes: `ssxmdl.f:136`'s `loadxr(T)` runs AFTER the `rvfixd` and
already-fixed blocks, and those read `ctx.xrgmdl` — closing early would hand
them the walked design and its shifted column indices. The same `loadxr(T)`
also overwrites `Irgxfx`/`Regfxx`/`Usrxfx` from the working model, which is
exactly what this port's three-field save/restore stand-in does on the other
arm. One or the other, never both.

**3. `x11mdl.f:424-425`, and this is the one the walls did not cover.** The
outlier-ID arm is guarded by

```
IF(Otlxrg .and. (Irev.lt.4.or.(Irev.eq.4.and.Rvxotl))
   .and. (Issap.lt.2.or.(Issap.eq.2.and.Ssxotl)))
```

and this port had ported only `Otlxrg`. Both dropped clauses are the same
statement: they are what makes `x11outlier=no` mean anything at all, on the
sliding-spans side AND on the `history{}` side. Without the `Issap` clause the
`x11outlier=no` run re-identified per span **on top of** the columns `ssxmdl`
had just held back, and died at `AO1958.Jan` — the `ssx11a` strike does not
save it, because on that arm the strike is not the branch taken.

### The compensation, named as one

`loadxr(false)` is unswapped in the oracle by whatever `restor` follows —
`ssx11a.f:160` for the per-span bracket, `setssp.f:356` for `ssxmdl`'s. This
port runs `restor_span` from the span loop, before `run_x11_span`, so both
brackets undo the swap with an explicit save/restore instead.

**That is a compensation for a MOVED call, not a mirror, and it restores
strictly MORE than `restor.f` does.** `restor` never touches `Begxy`, `Userx`,
`Usrtyp`/`Usrptr`/`Usrttl`, `Userfx`, `Easidx`, `Nusrrg`, `Bgusrx` or the
`picktd` block — all of which `loadxr(false)` overwrites and the oracle leaves
overwritten. Every one of them is identical between the two designs across the
gated corpus (one series, one span start, no user regressors on either side),
which is precisely what makes the difference invisible. Entry 71's `Ksdev`
shape exactly: correct at these call sites, and a defect at the first
x11regression design that carries its own `span=` or `user=`.

### Mutations, against a verified-0 baseline

| mutation | result |
|---|---|
| drop the `loadxr` unswap (`ss_restore_working`) | **159** |
| the whole per-span x11reg block never runs | **23** |
| `ssx11a.f:107-118`'s strike | **19** |
| `x11mdl.f:425`'s `Issap` clause | **19** |
| `ssx11a.f:119-151`'s store arm | **4** |
| `ssxmdl.f:44-77`'s `rmotss` walk (which fills that store) | **4** |
| `x11mdl.f:424`'s `Irev` clause | 0 |
| `ssx11a.f:150`'s `Ssxint` half of the fix flag | 0 |

The unswap at 159 is the number to remember: leave the x11regression design
sitting in the working model arrays and every slidingspans-with-x11regression
gate in the corpus fails, not merely the four new ones.

**Both zeros are explained, and they are different kinds.**

`Ssxint` is entry 86's identity again, one design over: `adotss` computes
`fx = Fixotx.or.(Otlfix.or.Ssxint)` and `rmotss` wrote `Fixotx =
Regfx.or.(Otlfix.or.Ssxint)` from the same run-constant pair, so the second
disjunction cannot change the result. Algebra, not coverage.

`Irev` is a coverage hole with a cause worth more than the hole. Restoring the
clause changes the `history{x11outlier=no}` probe by exactly nothing, measured
both ways — because **`ctx.hiddn.irev` is never advanced past 1 in this port.**
`revdrv.f:387` sets `Irev=4` around the history span loop; `run_history` has no
counterpart. So every `Irev`-keyed guard in the tree is currently inert: this
one, `x11reg.cpp:1142`'s x11regression coefficient seed, and `errio.cpp:21`'s
error-header suppression. The clause is transcribed anyway, because it is half
of one Fortran statement and the other half is load-bearing — but it is a
**dead** guard until `Irev` is set, and that is its own board item.

That also re-scopes the pre-existing "STILL OPEN, `x11outlier=no` at sar
7.5e-1" note in `run_history.cpp`: re-measured here at sar 4.6e+0 / trr 1.3e+1,
and the obvious candidate is not the cause.

### Also found and not fixed

`x11regression{outlierspan=}` (`gtxreg.f:666-673`, which writes the COMMON
`Begxot`/`Endxot`) is dropped by this port: `x11reg.cpp:1340-1342` derives
`begxot`/`endxot` locally from `Begspn` and `Nspobs` every time. That happens
to equal `ssx11a.f:105-106`'s per-span assignment, which is why those two lines
did not need porting — and it means the option is silently ignored on the main
run. Recorded here, not fixed.

**Gated by** four hand-authored specs, the full 2x2 of `x11outlier=` x
`fixx11reg=`: `extra/airline_slidingspans-x11reg-autootl`,
`-autootl-fixno`, `-x11outlier-no`, `-x11outlier-no-fixno`. The `x11outlier=`
column is byte-identical under `fixx11reg=yes` (nothing is refit either way) and
240 `sfs` lines apart under `fixx11reg=no`; the grid is in the fourth spec's
header. WALLS 24 -> 23 gaps.

## 88. `slidingspans{}` + user regressors — two BARE abends, a seventh span-replay leak, and an out-of-bounds read in the oracle (CB-40)

Board item 1's last real wall was `slidingspans{}` with `x11regression{user=}`,
walled at `slidingspans.cpp` on `Nusxrg > 0`. Reading it found three separate
things, none of which was the wall.

**The wall was keyed on a flag that is not what its name says, and could not
fire.** `Nusxrg` is NOT the x11regression user-COLUMN count — that is `Ncxusx`.
`Nusxrg` is the length of `x11regression{usertype=}`, written by `gtxreg.f:265`
and by nothing else in the tree. An instrumented build of the vendored oracle on
airline + `slidingspans{}` + `x11regression{user=(u1)}` prints

```
SSXMDL tail Ssxint= T Usrxfx= F Nusxrg=   0 Ncxusx=   1 Irgxfx=  0
```

so the arm the wall guarded was skipped and the spec walked straight past. Same
family as entry 74's `Nbx == 0` proxy: the wall looked conservative, which is
what hid it.

**What DOES break is one level down, and it had no message at all.**
`slidingspans{}` is what sets `Userfx` — `ssmdl.f:350`'s
`IF(.not.Userfx)Userfx=Ncusrx.gt.0`, on the `Ssinit==1` default path — and
`Userfx` is what turns on `addfix.f:73`'s `addusr` branch. So a
`regression{user=}` spec reaches `rmfix.f:89`'s `dlusrg` and `addfix.f:74`'s
`addusr` for the first time. Neither was ported: `automd_finalize.cpp` had a
BARE `abend(ctx)` on each arm, no `errhdr`, no message. The engine's whole
`===ERR===` block on such a spec was empty. That is worse than the entry-73
shape it resembles, because `walls.py` derives its inventory from refusal
MESSAGES, so neither hole was in the wall count, the gap list, or anywhere else.

Ported, all four leaves, in a new `core/src/regarima/usrbak.{hpp,cpp}`:
`bakusr.f`, `addusr.f`, `dlusrg.f`, `chusrg.f`. Plus the sliding-spans bracket
they live in — `ssmdl.f:350-352`, `ssxmdl.f:143-148`, `sspdrv.f:145-174` and its
undo at `:220-231`, and the `sspdrv.f:250-260` NOTE.

**CB-40: `bakusr.f:50/52` displace the SOURCE of two copies instead of the
destination.** Three of the five backup writes displace the destination and are
right; `Userx2` and `Usrty2` do not, so the `Rind=1` call reads
`Xuserx(PUSERX+1 …)` and `Usxtyp(PUREG+1 …)` — one past the end of each, for a
whole array's length — and writes what it finds into slot 0. A bounds-checked
rebuild of the vendored sources traps exactly there:

```
At line 50 of file bakusr.f
Fortran runtime error: Index '53041' of dimension 1 of array 'userx'
above upper bound of 53040
```

Slot 1, the one `addusr(1)` reads, is never written by any call, so the
x11regression user regressor is restored with an identically ZERO data matrix
and type 0. That effect is deterministic (the garbage goes to slot 0, read only
by `addusr(0)`), so the arm is ported rather than walled, and the one
combination where the garbage becomes observable — `regression{user=}` AND
`x11regression{usertype=}` AND a span driver — is refused with its own message.
Full write-up in `tools/census_bugs.md`; `sspdrv.f`'s shared `bfx2` buffer and
its `Nb`-sized restore of an `Nbx` array are CB-41 alongside it.

**A seventh instance of the span-replay save/restore seam, and the cleanest one
yet.** `/orisrs/ Stoap` is the REGRESSION-ADJUSTED original — what `adjreg`
leaves behind and what the `b1` table is punched from. It was not in
`run_x11.cpp`'s save set. Every slidingspans spec gated before this one had no
`regression{}`, so `Stoap` equalled the raw series and a span replay overwriting
it was invisible; put a user regressor in the spec and `b1` came back as the raw
`112` against the oracle's adjusted `224.466`, a 5.0e-1 relative error on the
first observation. Exactly what the standing rule says: **whatever a consumer
re-derives from joins the set, not only what a change obviously touches.**

**Two build-flag findings, both of which nearly became false bug reports.**
Instrumenting the oracle needs a rebuild of the vendored sources, and the first
one disagreed with the vendored binary by 600 lines on the sliding-spans section
of one probe while matching it byte-for-byte on the main run and on every
already-gated spec. Two things had to be settled before anything could be
measured:

- `gfortran -O2` alone is NOT the oracle. **`-fno-automatic` is required** — the
  f77 convention of static locals is load-bearing somewhere on this path.
  `-O2 -fno-automatic` reproduces the vendored `_O2` exactly on every probe;
  plain `-O2` does not. (`-O0` happens to as well, which is how the difference
  first surfaced as an optimisation-level artefact.)
- The vendored `_O0` and `_O2` agree with each other on all of these, so
  "two vendored builds agree" is NOT evidence against an out-of-bounds read;
  only `-fcheck=bounds` settled CB-40.

The working recipe, for the next time (scratchpad copy — the vendored tree is
never edited): derive the source list from `makefile.gf`'s `OBJS` block (690
files, not the 712 `.f` in the directory), compile with
`-O2 -std=legacy -fallow-argument-mismatch -w -fno-automatic`, and LINK FROM A
RESPONSE FILE (`gfortran -o x13.exe @objs.rsp`) — 690 objects on one command
line makes `collect2` fail with "CreateProcess: No such file or directory",
which reads like a broken toolchain and is not.

**`chusrg` cannot fire on the default path, and the corpus needed a spec that
says so.** `chusrg.f:45` only looks at columns that are NOT already fixed, and
`ssmdl.f:348`'s `Ssinit==1` block has just set `Regfx` true for all of them —
so with `fixmdl` at its default the routine is a guaranteed no-op and the
`sspdrv.f:250-260` NOTE never appears. Both arms are now gated:
`-zerospan` (default) and `-zerospan-fixmdlno`, on a user regressor that is
identically zero through 1958 and a ramp after, so that spans 1-2 see nothing
and spans 3-4 see the ramp. Measured on the instrumented oracle:
`bakusr`+`dlusrg`+`addusr` fire in spans 1 and 2 and not in 3 and 4, and the run
ends with the NOTE, which `test_slidingspans_notes` compares verbatim.

**Mutations**, baseline verified 0 first, against the five new specs: `addfix`'s
`addusr` **6**; `ssmdl`'s `bakusr(rind=0)` **4**; the `/orisrs/` restore **4**;
`sspdrv`'s `chusrg` block **3**; `chusrg`'s "fix this column" body **3**;
`sspdrv`'s undo **2**; the `sspdrv.f:250-260` NOTE **1**.

**Five zeros, and they are one finding rather than five.** Skipping
`bakusr(rind=1)`, "fixing" CB-40 to write slot 1 correctly, skipping `dlusrg`
entirely, and both of `addusr`'s transcribed quirks (`addusr.f:40`'s `Nb` where
`Ncoltl` is meant, `addusr.f:42`'s undisplaced `Usrpt2`) all score 0 gates —
and, checked the stronger way, leave the engine's ENTIRE STDOUT byte-identical
on all five probes. Two reasons, both worth carrying:

- `dlusrg`'s whole effect is undone by the `addusr` that always follows it,
  which reassigns `Ncusrx`, `Userx`, `Usrptr`, `Usrtyp` and `Usrttl` wholesale
  from the backup. It is a saturated precondition, not dead code.
- **Nothing downstream of `addfix`'s restore reads the x11regression design
  again within a span.** So the entire `rind=1` path — including this port's
  reproduction of CB-40 — is faithful by transcription and untested by
  measurement. The spec gates the RUN, not the bug. Said plainly in the CB entry
  rather than left for someone to infer from a green suite.

**Gated by** five hand-authored specs: `extra/airline_slidingspans-reg-user`,
`-x11reg-user`, `-x11reg-usertype`, `-reg-user-zerospan` and
`-reg-user-zerospan-fixmdlno`. Suite 7148 -> 7246 passed, 0 failed, 0 xfailed.
WALLS 23 gaps (one removed, CB-40's added).

## 89. `Irev` never left 1 — four walls that could not fire, the capture the port had moved out of x11pt3, and the three buffers that made the move wrong

Board item 2. `revdrv.f:387` sets `Irev=4` around the history span loop and
`:761` sets 5; `run_history` had no counterpart, so **every `Irev`-keyed
condition in the tree took its main-run branch during a replay** — the
`Issap.lt.2.and.Irev.lt.4` gudrun family, `x11mdl.f:424`'s outlier-ID arm,
`x11reg.f`'s coefficient seed, `errhdr`'s one-banner-per-span rule, and the
five `getrev` sites in `x11pt3`. Entry 87 measured the consequence from the
other end and could not test it.

### The four walls were unreachable, and they were the design

`docs/WALLS.md` listed four `x11pt3 revisions * store (getrev)` gaps. None had
ever fired, and none could: they are all guarded by `Irev==4`. That is the same
shape as entry 88's `Nusxrg` wall and entry 74's aictest proxy — **a wall keyed
on state the port never produces is not conservative, it is decorative.**

Behind them sat a structural choice nobody had written down as one. The oracle
CAPTURES a history span's result from inside `x11pt3`, by handing the finished
component to `getrev`; this port instead let `x11pt3` run to completion and then
re-read `ctx.x11srs.sts/stci/stc` in `run_history`. Those are the same numbers
**only while the buffer x11pt3 leaves behind is the buffer getrev was handed.**

### Three places where that is false

* `x11pt3.f:828-831` — with `force{}` on (`Iyrt>0`) the store takes **`Stci2`**,
  the FORCED SA. The port read `Stci`, the unforced D11, for every row.
* `x11pt3.f:915-918` — with `force{round=yes}` it takes **`Stcirn`**, the
  rounded SA. This site was in neither the ported set nor the wall list: the
  block carried the comment `(deferred: rnd table/punch; ssrit/getrev stores.)`,
  and a comment is not an inventory entry. **The sliding-spans `ssrit` at
  `:903-906` was missing with it** — the third of the three SA store sites,
  absent for as long as `slidingspans{}` + `force{round=yes}` had no spec.
* `x11pt3.f:1067-1072` — the TREND store picks **`stc2`**, the published D12
  with the level shift folded back in, when `(.not.Finls).and.Adjls.eq.1`.
  `ctx.x11srs.stc` is the LS-FREE internal trend. Every trend revision on a spec
  with an `ls` regressor was taken from the wrong series.

Note the trend selector is that arm ALONE and **not** the port's `have_stc2`:
`stc2` also exists when only the TC fold or `transform{temppriortrend=}` built
it, and on those the oracle hands `getrev` the internal `Stc` anyway —
disagreeing with its own published D12 and with the buffer it had just taken the
`transform{constant=}` out of. Transcribed at the call site; no carrier.

All three were invisible on the whole corpus because no history spec had
`force{}`, `round=yes`, or a level shift. Each now has one.

### What was ported

`core/src/x11/getrev.{hpp,cpp}` — `getrev.f` and `putrev.f`, with the composite
indirect fold, the alternate-target `DO WHILE`, the final-column block, and the
two "has ceased due to negative values" warnings. The five `x11pt3` sites and
`seatdg.f:148-181`'s three SEATS ones now call it; `run_history` reads
`/revdta/` instead of re-deriving it. Two details are load-bearing and easy to
lose: `putrev` clears the **live COMMON `Lrvch`** for the rest of the run when an
additive percent change meets a non-positive value, while `Rvdiff` is only
getrev's LOCAL copy, so the flag persists and the sentinel does not.

Two things came with it because turning `Irev` on made them reachable:

* **`revdrv.f:416-427`** — a span past `Endsa` (and not the final one) runs with
  `Lx11`/`Lseats` FALSE: estimated for the model diagnostics, no adjustment, no
  capture. This port ran X-11 on every span, which was equivalent only while
  nothing read those spans back. `run_x11_span` takes an `lx11_span` argument now.
* **`errhdr.f`** — the banner naming which hidden run produced the messages that
  follow. It was a stub ("out of M1 scope") and `writln` routes every Mt2
  message through it, so the stub was live for `slidingspans{}` too and had been
  since the sliding-spans port. Ported with `Ierhdr`, `Crvend`/`Nrvend`
  (`revdrv.f:478`), `sspdrv.f:114/237` and `revdrv.f:396/762`.

### Measurements

The whole corpus stayed **bit-exact** on the first build: 7246 passed, 0 failed
— so the rearrangement really was equivalent on everything the corpus had, and
is now faithful rather than accidentally right.

Mutations, against a verified-0 baseline (`-k history`, 612 gates):

| mutation | failures |
|---|---|
| `Irev` never advances (the state this entry closes) | **171** |
| `Revptr` not filed per span | **254** |
| trend buffer `stc2` -> `stc` always | **4** |
| forced SA `stci2` -> `stci` | **4** |
| rounded SA `stcirn` -> `stci2` | **4** |
| SEATS span getrev dropped | **2** |
| x11pt3's seasonal-site RETURN dropped | 0 |
| x11pt3's SA-site RETURN dropped | 0 |
| `Irev=5` after the loop dropped | 0 |
| `lx11_span`: run X-11 on every span | 0 |
| errhdr banner suppressed | 0 |

**The four zeros are not the same kind, and one of them is a proof.**

`lx11_span` is **structurally unobservable, and that is provable rather than
corpus-limited.** The obvious discriminator is a span past `Endsa` filing an
alternate-target row the table prints — so a spec with `endtable=` AND
`sadjlags=` was written for it (`airline_history-endtable-sadjlags`, which is
also the first spec combining the two). It still measures zero, because
`setrvp.f:26-40` widens `Endsa` by exactly the largest lag: a span past the
widened `Endsa` files `Finsa(t,i1)` with `i1 > Revnum` by construction. The
guard is a Census optimisation, and the port carries it for faithfulness only.

The two dropped RETURNs and `Irev=5` are corpus-limited but in the same
direction: what the RETURN skips is either not captured (the family was not
requested) or restored by `run_x11`'s span save/restore set — a zero there is
incidentally evidence that set is complete over those fields.

The errhdr zero is the honest one: **no gated spec emits a message from inside a
span**, so the banner is faithful by transcription and untested by measurement.
A carrier was attempted (`estimate{maxiter=2}` to make every span hit its
iteration limit) and does not work — the MAIN run fails to converge first and
the oracle stops before `revdrv` runs at all. Recorded rather than left implied.

### Landed on the side

`regression{savelog = all}` is accepted by this port and REFUSED by the oracle
("Savelog argument is not defined"), found while writing the LS spec. Not gated
— `test_m1_parse::test_outcome_matches_oracle` would fail today. It is on the
board.

**Gated by** four hand-authored specs: `extra/airline_history-force`,
`-force-round`, `-ls-trend` and `-endtable-sadjlags`. Suite 7246 -> 7363 passed,
0 failed, 0 xfailed; ctest 12/12. WALLS 23 -> 19 gaps, all four removed by
porting what they stood in front of.

## 90. `history{x11outlier=no}` — closed by two increments ago, and nobody had re-measured

Board item 2. The port needed **no code**: the arm was already bit-exact, and
the whole increment is two specs, one mutation sweep, and the deletion of three
paragraphs of prose that had been wrong since the moment entry 89 landed.

**The chain, and why it hid.** `x11mdl.f:424`'s outlier-identification arm is

```fortran
IF(Otlxrg .and. (Irev.lt.4 .or. (Irev.eq.4 .and. Rvxotl))
   .and. (Issap.lt.2 .or. (Issap.eq.2 .and. Ssxotl)))
```

and this port had ported `Otlxrg` alone, so during a `history{}` replay every
span re-identified a fresh AO set on its own data no matter what
`x11outlier=` said. Entry 87 found that omission from the SLIDING-SPANS side,
restored both clauses, and measured the history probe **both ways**: sar 4.6e+0
either way, no change. That measurement was correct and its conclusion —
recorded in a code comment as "the obvious candidate is NOT the cause" — was
not, because `ctx.hiddn.irev` was 1 for the whole span loop and the clause it
had just restored could not evaluate. Entry 89 advanced `Irev` to 4. Nothing in
entry 89 was about `x11outlier=`; it closed this as a side effect, and the board
item stayed open for an increment because closure was assumed to need work.

This is the standing "a null measured under the wrong preconditions is not a
null" rule arriving from the RE-MEASUREMENT direction, which reads differently
and is worth naming: **a candidate fix that measures zero while one of its own
preconditions is known dead has not been tested, it has been skipped.** Entry
87 wrote the precondition down in the same comment as the exculpation — the
information needed to void the measurement was already on the page.

**What was actually done here.** Two hand-authored specs, mirroring the pair
that already gate the DEFAULT: `extra/airline_history-x11outlier-no` (airline,
`transform{log}` + `arima{(0 1 1)(0 1 1)}` + `x11regression{variables=(td)
critical=3.0}` + `history{estimates=(sadj trend) start=1955.jan
x11outlier=no}`) and `-no-nomodel`, the model-free twin where nothing
re-estimates per span so the x11regression outlier bookkeeping is the only thing
moving. Both gate bit-exact on the first run.

**The option is not inert on either spec**, which is the check that makes the
gates worth having: the `yes` and `no` goldens are **708** output lines apart
with a model and **714** apart model-free.

**Mutations**, against a verified-0 baseline of 639 `-k history` gates:

| mutation | gates failed |
|---|---|
| `x11mdl.f:424`'s `Irev` clause dropped again (`Otlxrg` alone) | 8 |
| per-span `Rvxotl` forced TRUE (always `rmatot`) | 8 |
| per-span `Rvxotl` forced FALSE (never `rmatot`) | 8 |
| `rmotrv`'s hold-back flag pinned false | 8 |
| head-of-analysis `rmatot` (`revdrv.f:336-338`) skipped | **0** |

Eight is four gated tags (`sar sae trr tre`) times the two new specs, and the
arms are discriminated in both directions: forcing TRUE moves only the `no`
specs, forcing FALSE only the `yes` ones.

**The zero is structural, not a corpus gap, and that distinction is the reason
to write it down.** `revdrv.f:336-338`'s pre-loop `rmatot` deletes the
automatically identified x11reg columns once before the span loop; the per-span
block at `:731-741` runs `else if (Rvxotl) rmatot(...)` with **no `i>Begrev`
guard**, so span 1 strips the same columns again on a design the pre-loop call
has already stripped. No spec can tell the two apart because none can exist —
the second call is unconditionally reached whenever the first one is. It stays
transcribed because the oracle makes it, and the comment above it now says so,
so the next mutation sweep does not re-derive this.

**Prose deleted or corrected:** `run_history.cpp`'s 22-line "STILL OPEN and
measured" block (which named the right guard and then ruled it out),
`run_history.hpp:81-86`'s "the `no` branch is measured wrong and ungated", and
`tools/history_options_scouting.md`'s `x11outlier=` section, whose ranking table
still read `7.48e-1 — STILL OPEN`. All three were true when written; all three
became false the day entry 89 landed, and none of them is the kind of thing a
tool can check.

## 91. `savelog =` was never validated — one spec's dictionary slice is the whole option

Board item 2 (after part 41 closed the previous one). `regression{savelog =
all}` was found in entry 90 while writing an unrelated spec: this port accepts
it and returns `OUTCOME: OK`, the oracle stops with `ERROR: Savelog argument is
not defined.` The single instance was the visible end of a systematic gap.

**What `getsvl.f` actually does.** `SVLDIC` (`svltbl.prm`, 1294 chars) is ONE
dictionary carved into fourteen per-spec slices by `svllog.i`'s
`LSL<spec>`/`NSL<spec>` pairs. Every table has a LONG and a SHORT name written
adjacent, so entries 2i-1 and 2i map to table i, and `getsvl` hands `gtdcnm`
`svlptr(2*Spcdsp)` as its element 0 with `2*Nspctb` entries — **the slice is the
validation, and there is no global lookup to fall back on.** Decoding the table:

| slice | `alldiagnostics`/`all`? | names |
|---|---|---|
| transform, pickmdl, **regression**, outlier, x11regression, slidingspans | **no** | 1-2 tables each |
| automdl, estimate, check, x11, history, spectrum, composite, seats | yes | 6-20 tables |

So `savelog = all` is legal in eight specs and refused in six, and nothing about
the *word* says which — only the slice does.

**What this port did.** `getsvl` was `consume_prtsav`, the same token-faithful
consumer as `getprt`/`getsav`: it accepted any NAME, any list, and `+`/`-`
prefixes the Fortran does not allow here. Of the fourteen call sites, twelve did
not exist at all (each reader consumed its own savelog value), and the two that
did — `check{}` and `composite{}` — passed placeholder slice arguments
(`getsvl(ctx, 0, 11, ...)`, `getsvl(ctx, 0, 10, ...)`) into a routine that
ignored them. **A parameter that is passed and unread is indistinguishable from
a parameter that is wrong**, which is why the placeholders survived: nothing
could observe them.

**Ported:** `getsvl.f` in full (both arms, the NULL-comma checks, the two-line
refusal), `SVLDIC`/`svlptr` verbatim, `svllog.i`'s fourteen `LSL`/`NSL` pairs
into `namespace svllog`, and every one of the fourteen call sites. The
`Svltab(tblidx)=T` store stays deferred with the rest of the table selection —
only the LOOKUP is ported, which is the half that decides whether the run
happens.

**How the call sites were wired, and why not through the switch.** The fourteen
readers have four different dispatch shapes (capture-then-switch,
switch-then-consume, if-chain, and a nested `if (gtarg(...))`), so the savelog
arm was inserted immediately after each reader's argument-loop head — the one
line they all share — rather than into twelve different switch bodies. The two
readers that already dispatched from their switch keep doing so with corrected
slice arguments.

**No corpus spec used an illegal savelog name**, so the whole suite stayed at
7390 passed / 0 failed across the change: the 218 `estimate{savelog = all}`, 193
`x11{}`, 56 `seats{}`, 37 `history{}`, 15 `automdl{}`, 2 `spectrum{}` and 1
`check{}` uses are all inside their own slices. That is the shape of this defect
class — the validation had never once been reached in anger.

**Mutations**, against a verified-0 baseline (full suite, `-n 8`):

| mutation | gates failed |
|---|---|
| the lookup's verdict discarded (token still consumed) | 3 |
| `regression{}` routed through `estimate{}`'s slice | 1 |
| the slice ignored — every spec gets all 218 names | 2 |
| the list arm loses its lookup | 1 |
| **`check{}`'s placeholder `getsvl(ctx, 0, 11, …)` restored** | **0 → 1** |

**That last row is the increment's own finding turned back on itself.** With the
first three specs, restoring the placeholder failed NOTHING: the only `check{}`
savelog value in 473 specs is `all`, and `all` falls inside the placeholder
window too, because entries 11-12 are `automdl`'s `alldiagnostics`/`all` pair. A
wrong slice that happens to contain the one name the corpus uses is exactly as
invisible as an unread parameter. `edge/savelog-wrong-slice-check`
(`check{savelog = aic}` — a REAL name, from `estimate{}`'s slice) was written for
that and makes it 1.

**A caution about the mutation method, learned the expensive way.** The first
version of the "accept everything again" mutation deleted the whole refusal
branch, `lex()` included — and the engine **spun forever** on
`savelog = (aic bogus)`: the `CALL lex()` inside `getsvl.f`'s error arm is what
advances past a name the dictionary did not consume, so without it the list loop
re-reads the same token. It burned ten CPU-minutes in a background run before
being noticed. The mutation had to be narrowed to "discard the verdict, keep the
lex" to measure anything. A mutation that makes the program HANG measures the
harness's timeout, not the code.

**Gated** by four hand-authored `edge/` specs: `savelog-all-regression` (the
single-name arm and the found case), `savelog-list-undefined`
(`estimate{savelog = (aic bogus)}` — a spec where `all` IS legal, so it pins the
lookup rather than the slice, and exercises the list arm),
`savelog-all-slidingspans` (a second refusing spec, so the gate is not a claim
about `regression{}` alone) and `savelog-wrong-slice-check`
(`check{savelog = aic}` — a real name from the wrong slice, the only one of the
four that can see a mis-set displacement). All four match the oracle
byte-for-byte including the caret column and the `Check the available
diagnostics for this spec.` continuation.

**The sibling gap, found and NOT closed:** `getprt.f`/`getsav.f` do the same
thing with four dictionaries (`TB1DIC`..`TB4DIC`, split by displacement at
`BRKDSP`/`BRKDS2`/`BRKDS3`) plus a five-entry `LVLDIC`
(`default none brief all tables`), and refuse with `Print or level argument is
not defined.` This port still consumes both without looking. Same shape, four
times the table; on the board as its own item.

## 92. `print =` / `save =` were never validated either — and PRINT and SAVE are different dictionaries over the same table slots

Board item 2, the sibling entry 91 found while closing the savelog gap. Same
shape, four times the table, and one twist the savelog case did not have.

**The arrangement.** `getprt.f`/`getsav.f` look every name up in a per-spec
slice, exactly as `getsvl` does, but the dictionary is split in FOUR at
`BRKDSP=118` / `BRKDS2=267` / `BRKDS3=348` — `table.prm` says why: to keep each
literal "under 2000 characters, a requirement for the VAX/VMS Fortran". Each
spec's displacement (`tbllog.i`'s `LSP<spec>`/`NSP<spec>`, 18 pairs over 396
table slots) is relative to whichever piece it lands in. `getprt` additionally
tries a five-entry `LVLDIC` (`default none brief alltables all`) FIRST, and
accepts a `+`/`-` prefix on a table name.

**The twist: PRINT and SAVE are different dictionaries.** `table.prm` and
`stable.prm` are parallel — same 396 slots, same pointer counts — and
`stable.prm` holds an EMPTY STRING wherever a table can be printed but not
saved (`check{}`'s `acfplot`/`acp`, `history{}`'s `header`/`hdr`). An empty
entry can never match, because `gtdcnm` only looks anything up for a NAME
token, **so the emptiness IS the refusal.** A port that used one dictionary for
both would be wrong only on those slots.

**What this port did.** `getprt`/`getsav` were `consume_prtsav` — accept any
name, any list, any `+`/`-`. Of the 36 call sites, 30 did not exist (each
reader consumed its own value); of the six that did, `check{}` passed
`(0, 11)` and `composite{}` passed `(0, 10)` — placeholders again, the same
shape entry 91 found — and only `series{}`'s `(0, 10)` was right, by
coincidence, because `LSPSRS/NSPSRS` really is `0, 10`.

**Ported:** `core/src/specparse/tbldic.cpp` (the eight dictionaries and pointer
arrays, 11k characters, transcribed by generator from the four `.prm`/`.var`
files with every declared length asserted), `tbllog.i`'s 18 displacement pairs
into `namespace tbllog`, `getprt.f` and `getsav.f` in full — the level arm, the
prefix arm, the list arm, the NULL-comma checks — and all 36 call sites.
`Prttab`/`Savtab` and `getprt.f:205-209`'s `level()` fill stay deferred.

**A CENSUS INCONSISTENCY, ported: the two prefix errors differ by one
character.** `getprt.f:67` (single value) ends `or nothing.` and `getprt.f:148`
(inside a list) ends `or nothing` with no period.

**And a control-flow trap that the first transcription got wrong.** The list
arm's prefix error is followed by `GO TO 10` — skip to the next element. The
single-value arm at `:60-71` has **no such jump**: it reports the bad prefix,
consumes the token, and FALLS THROUGH into the table lookup, which then runs
against whatever came next. On `print = 7` the oracle therefore emits *two*
errors — the prefix one, then `Print or level argument is not defined.` with
its caret on the closing brace. The port bailed from both arms, which reads
like the obvious symmetry and is not what the Fortran does. Same family as
"count the block; never read the indentation": the two arms look identical and
differ by one statement.

**Decode verified before a line was written.** A script re-derived every spec's
slice from the four dictionaries and cross-checked it against **12,840**
`print=`/`save=` values in the corpus: all resolve. The first run of that check
reported 33 violations and every one was the CHECKER's fault — a
`(\w+)\s*\{(.*?)\n\s*\}` block regex merged a one-line `x11{ }` with the
`slidingspans{ save = (sfs chs) }` after it. A validation harness that reads
spec blocks needs a depth-tracking scan, not a regex.

**Mutations**, against a verified-0 baseline (full suite, `-n 8`):

| mutation | gates failed |
|---|---|
| the lookup's verdict discarded (token still consumed) | 4 |
| `save` routed through the PRINT dictionary | 1 |
| the per-spec displacement off by ONE TABLE | **3526** |
| the single-value prefix arm bails like the list arm | 1 |
| the `LVLDIC` level arm disabled | **5141** (+16 errors) |

The two four-figure numbers are the reassurance that this data is load-bearing;
the three ones and fours are the four new specs, each failing exactly the
mutation it was written for.

**A weak mutation, recorded because it looked strong.** The displacement probe
was first written as "widen the slice by 42 names" and measured **0** —
`x11{}`'s window grew into `force{}` and the head of `x11regression{}`, and the
spec's `chs` (a `slidingspans{}` table, 60 slots further on) still was not in
it. Widening a slice is not the same as shifting it; only the shift moved
anything.

**Gated** by four hand-authored `edge/` specs:
`print-undefined-x11` (a name in no dictionary at all — the LVLDIC-then-table
path and the print-specific two-line message), `save-wrong-slice-x11`
(`x11{save = chs}` — a REAL table name from a neighbouring slice, the shape
entry 91 had to learn to write), `save-printonly-check`
(`check{print = acfplot save = acfplot}` — printable, not savable; **the only
spec in the corpus that distinguishes `table.prm` from `stable.prm`**), and
`print-prefix-bad` (both prefix arms, pinning the one-character message
difference and the fall-through cascade).

**Deliberately NOT changed:** `series{save=}` still does not feed
`ctx.captured.save_tables`. It never did — the five readers that captured
(transform, regression, seats, force, slidingspans) still do, now through
`getsav`'s new out-parameter — and adding `series{}` would switch on `a1`
output that `run_pre_model`'s `wants_save()` has never seen. That is its own
change with its own gate, and the comment at the call site says so.

## 93. `x11regression{outlierspan=}` — parsed and dropped, and the critical value it silently owned
94. `force{}` on an X-11 composite -- agr3's forced/rounded tail, and the F-test channel that could not report it missing

Board item 2, and the third parsed-but-unread option in a row (entries 91, 92).
`gtxreg.f:487-497` parses the argument into `spnotl`; `:665-674` resolves it into
the COMMON `Begxot`/`Endxot`; `x11mdl.f:441` hands that pair to `idotlr` as the
window the AUTOMATIC AO identification inside the irregular regression searches.
This port consumed the value and re-derived the pair LOCALLY from
`Begspn`/`Nspobs` at the `idotlr` call, so the option was ignored on the main
run at `OUTCOME: OK`. Oracle on-vs-off first, as the rule says: on airline with
`variables=(td) critical=3.0` the oracle identifies **203** AO columns over the
full span, **9** with `outlierspan=(1955.1, )` and **28** with
`outlierspan=(1952.1,1957.12)`.

**The default is not what the local derivation computed.** `gtxreg.f:671`
defaults `Endxot` to `Begsrs + Nobs - 1` — the end of the SERIES — while
`Begxot` defaults to `Begspn`, the start of the span. A `series{span=}` that
stops short therefore leaves the oracle's outlier window running past the span
end, with no `outlierspan=` in the spec at all. This is the half a port fixes
without noticing it existed.

**And that difference is invisible in the obvious place.** `idotlr.f:207-212`
clamps its own test range (`ibgtst = max(..,1)`, `iedtst = min(..,Nspobs)`), so
a window ending past the span tests exactly the same points. The observable is
the CRITICAL VALUE: `editor.f:1749-1757` derives `Critxr` from the outlier-span
LENGTH when no `critical=` was given. So the port's Critxr derivation had to
move out of `x11reg.cpp` and into the editor, where the oracle does it — which
also fixes a second thing nothing had noticed: computed at the `idotlr` call, it
tracked the SPAN on a sliding-spans or history replay, where the oracle fixes it
once at spec-read from the main run's window.

**A first draft of the default-window spec measured a ZERO for a mutation that
really does change `Critxr`.** `span=(1950.1,1958.12)` puts the derived value at
3.87 against the span's 3.83, and the resulting adjustment is bit-identical:
the three AO lines the difference adds appear only in the iteration listing, and
the final model is the same. Two hours went into "the mutation must not be
compiling" before the answer turned out to be that the two verdicts agree. The
spec now uses `span=(1951.1,1954.12)` — 120 months to the series end against the
span's 48 — where d11 moves 1.8e-2 relative. **A parameter change that changes
no verdict gates nothing, and it looks exactly like a dead code path.**

**Ported:** the `outlierspan=` arm (note it differs from the `span=` arm twenty
lines up in two Census ways: `gtdtvc` is called with a hardcoded `T` for
`Havesp`, and `hvotsp` is set on the bare `ELSE`, so a date vector that failed
to parse still switches the coverage checks on); the `Begxot`/`Endxot`
resolution with both defaults; `gtxreg.f:678-695`'s two `chkcvr` refusals;
`editor.f:1749-1757`'s `Critxr` derivation, including the `Cvxtyp` arm — which
meant exposing `setcvl` from its anonymous namespace, where it had been
file-local since it was written; `ssx11a.f:105-106`'s per-span window; and
**`cvrerr.f`**, which turned out to be missing entirely.

**`cvrerr` is the second half of every coverage refusal in the program, and the
port had none of it.** Sixteen `CALL cvrerr` sites in the Fortran, each one
following an `inpter` that names the rule with a pair of lines that name the two
DATES that broke it. Not one of them is reached by a spec in the corpus, so
nothing failed; a coverage refusal simply came out one third the size the oracle
writes it. Same family as entry 81's unread Mt2 channel: an absent diagnostic on
an error path is invisible until a spec asks for the error. Its three arms are
independent `IF`s, not a chain — a span that both starts early and overruns
prints two of them.

**Mutations**, against a verified-0 baseline (full suite, `-n 8`):

| mutation | gates failed |
|---|---|
| the parsed `outlierspan=` dates discarded (both defaults taken) | 44 |
| the default END back to the span end | 20 |
| the second coverage check (against the irregular-regression span) dropped | 1 |
| `cvrerr`'s detail lines suppressed, `inpter` half kept | 2 |
| `editor.f:1749`'s `Critxr` derivation disabled | **225** |
| `ssx11a.f:105-106`'s per-span window write removed | **0** |

**The zero is real and is kept anyway.** `idotlr`'s own clamp means a main-run
window that brackets a span collapses onto that span, so the write can only
matter where the main window starts INSIDE a span —
`extra/airline_slidingspans-x11reg-outlierspan` is built to do exactly that, and
even so no span identifies an AO in the restricted region either way, so the
verdict is unchanged. Forcing one by dropping `critical=` to 2.0 kills the
ORACLE too, on the design-size limit, before any table is written; 2.8 finds
nothing new. The assignment stands on transcription, and its absence would be a
divergence waiting for a spec with a span-local outlier. Written up in the spec
header and at the call site so the next reader does not re-derive it.

**Gated** by six new specs: `extra/airline_x11regression-outlierspan`
(start-only, so one default arm still runs), `-outlierspan-both` (both dates,
neither default), `-outlierspan-default` (no `outlierspan=` at all — the
series-end default, via the critical value), `extra/airline_slidingspans-x11reg-outlierspan`
(the pairing, ungated on the per-span write as above), and two `edge/` refusals:
`x11regression-outlierspan-notinseries` (the first `chkcvr` and `cvrerr`'s
start-date arm) and `-notinmodel` (the second — a window inside the series but
outside the irregular regression's own `span=`, a different pair of dates and a
different message, which a port that wired only the first arm passes).

## 94. `force{}` on an X-11 composite -- agr3's forced/rounded tail, and the F-test channel that could not report it missing

Board item 2. `agr3s.f:218-338` was ported with the SEATS composite branch
(entry 49); `agr3.f:426-547`, the same block on the X-11 path, never was.
`agr3.cpp` ran to `:350` and jumped to `:598`, so a `force{}` on an X-11
composite total computed no `Stci2` and no `Stcirn`, emitted none of
`iaa`/`iff`/`irn`, and returned `OUTCOME: OK`. No wall: this is the silent
early-out shape, not a refusal.

**ORACLE on-vs-off first**, `census-examples/composite-fixed` plus
`force{type=denton round=yes}`: three save files appear, `adjtot` flips
`no`->`yes`, `indforce`/`adjtottype`/`adjtottarget`/`adjtotstart` appear, and
**nothing else in the run moves**. Forcing is a tail on the indirect adjustment;
it does not feed back into the decomposition. That is the whole reason the gap
could sit behind `OK` -- every table the corpus was already checking was right.

### The observable that hid it was a savelog row, not a table

`agr3.f:417` runs the residual-seasonality F-test on the indirect SA. `:493`
runs it again on the forced series and `:537` a third time on the rounded one --
**all three under the same `id11.f` / `id11.3y.f` keys**, so the last write wins
and a forced run reports 0.87565 where the unforced one reports 0.02200.

`:417` was unported too, and could not have failed anything:
**`x13run_composite` emitted no F-test row at all, on either branch**, so the
`d11.f`/`d11.3y.f`/`id11.f`/`id11.3y.f` block that `x13run_x11` has dumped since
entry 40 was simply not a channel on the composite path. Same class as entry
81's Mt2 buffer. Wiring it found two more things immediately:

- **A ninth span-replay save/restore miss.** The DIRECT `d11f` pair has been in
  `run_x11.cpp`'s set since entry 40; its INDIRECT twin was not -- the
  subset-restore shape this port keeps repeating (entry 72). On a composite
  total `Iagr` is 5 during a replay (`agr2.f:250`), so a `history{}` span's
  `ftest` files the replay's DIRECT D11 under the `i` keys and clobbers what
  agr3 wrote. `composite-history` reported `id11.f = 0.02402` -- which is that
  same spec's own `d11.f` -- against the oracle's 0.02200.
- **A SEATS-adjusted total has no `d11.f` at all**, and the engine correctly
  writes none. Gated as an absence rather than skipped.

### Two Census asymmetries between agr3 and agr3s, both transcribed

- `agr3.f:537` passes **`ib,ie`** to `ftest` -- the qmap OUTPUTS -- where
  `agr3s.f:328` passes `Pos1ob,Posfob`. And `agr3.f:44` declares `ib,ie` as plain
  locals that only the `Iyrt==1` arm writes, so `round=yes` with no
  `type=denton` reads them undefined. The port initialises them to zero and says
  so at the call.
- `agr3s.f:327` guards the rounded F-test on `Lx11`; `agr3.f` does not.
- The leading partial-year fix-up at `:467` loops `Posfob, ib-1` -- from the LAST
  observation -- so with `ib > Pos1ob` the range is empty and the intended fix-up
  never happens. `agr3s.f:257` has the identical line. Transcribed.

### `indforce=` is inert unless the components DIFFER

The `indforce=no` arm (`:476`) copies `Ci2`, the aggregate of the components' own
forced SA, instead of benchmarking the indirect series. A mutation forcing the
benchmarking arm measured **zero**, twice.

It is not a gate gap. **Benchmarking commutes with the sum**: measured on the
ORACLE, `iaa` is the same for `indforce=yes` and `indforce=no` to 4.9e-15 -- for
`type=denton`, which is linear in the annual discrepancies, and, measured, for
`type=regress` as well, so the obvious "use the nonlinear one" fix does nothing.
The arms separate only when the components are not alike: drop `force{}` from ONE
component and `iaa` moves **1.4e-05**. That is what
`composite-force-indno` is built around, and it is in the spec header, because
the next person will otherwise write the symmetric spec and conclude the arm is
dead.

Same family as entry 93's "a probe spec whose parameter change changes no VERDICT
gates nothing", one level up: here the parameter changes no VALUE, because the
operator it selects between is linear.

### Two walls, and one that walls.py could not see

`agr3.f:495-500` and `:539-546` store the forced/rounded series for sliding
spans via `ssrit` on the COMPOSITE (`Iagr>=3`); this port's `ssrit` is scoped to
`Iagr!=2` and has none of the `Indssp`/`Saind`/`Sfind` bookkeeping. Refused
rather than stored wrong. Neither is reachable while `x12run.f` runs `sspdrv`
after `x11ari`, so this is inventory. `agr3s.cpp` omits the same two arms
silently.

The first version named the helper `agr3_not_ported` and **`walls.py` did not
list either wall**: its `HELPERS` tuple is matched with `\b`, and `\bnot_ported`
cannot match inside `agr3_not_ported` because `_` is a word character. A wall
the inventory cannot see is precisely what that tool exists to prevent, and the
count would have stayed at 19 with two new gaps in the tree. Renamed
`composite_not_ported` and registered in `walls.py`; 19 -> 21 gaps.

### Mutations (verified-0 baseline, full suite `-n 8`)

| mutation | fails |
|---|---|
| A the whole force block skipped (the pre-increment behaviour) | 12 |
| B `agr3.f:417`'s ftest removed | 6 |
| C `indforce=no` forced onto the benchmarking arm | 4 |
| D `:537`'s ftest over `[Pos1ob,Posfob]` instead of `[ib,ie]` | 2 |
| E the harness's four F-test savelog rows suppressed | 26 |
| F `usefcst` dropped, `lstfrc` pinned to `Posfob` | 5 |

B measured **0** until the F-test gate was widened past the two force cases: on
a forced run `:417`'s write is overwritten twice, so a force-only gate is
byte-identical without it. The gate now discovers every composite case whose
golden carries an `id11.f` row, from disk, with a floor assertion. C measured 0
twice, for the reason above.

### Gated by

`census-examples/composite-force/` (indforce default yes; `type=denton
round=yes`, so qmap, both partial-year fix-ups, rndsa and all three F-test
writers) and `census-examples/composite-force-indno/` (`indforce=no`,
`type=regress` for the qmap2 branch, north forced and south not), via
`tests/parity/test_composite_force.py` -- which also gates the `d11.f`/`id11.f`
rows across every composite corpus, the `indforce:` savelog line, the absence of
`irn` where the spec never asked for it, and that `iaa` is distinguishable from
`isa` at all, so a no-op force cannot ride through.

Note the bless path: `run_parity.py --update` runs each `.spc` STANDALONE, which
for a `composite{}` total produces no components and therefore no indirect
tables. Metafile goldens are raw `x13as_ascii_O2 -m composite -s` output, as the
older composite cases are.
