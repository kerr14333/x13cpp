# `pickmdl{}` / `automx.f` — scouting and option measurements

Status: **CLOSED 2026-07-28e** for the base algorithm, **2026-07-30** for the
per-candidate AIC-regressor tests; the remaining walls are measured and each is
a clean fatal. See `core/src/automdl/automx.hpp` for the algorithm,
`docs/WALLS.md` for what is still refused, and `tools/SESSION_HANDOFF.md` for
the session context.

## What it is

The CLASSIC X-11-ARIMA automatic model selection, the sibling of `automd.f`
(TRAMO) and a completely different algorithm. `automd` identifies orders from
the data; `automx` estimates a **fixed candidate list** and keeps the best one
that passes three screens.

```
candidates := file=<name>            (parsed with getmdl, one per line,
             else the five built-ins  a trailing `*` marks the DEFAULT)
for each candidate:
    regvar + rgarma                          (+ idotlr when identify= says so)
    mape(4) := amdfct three-year average forecast error
    blchi   := Ljung-Box p-value * 100 at lag 24 (12 quarterly)
    rma     := sum of the NONSEASONAL MA coefficients
    ACCEPT if mape(4) <= loclim and blchi > qlim and rma < overdiff
        loclim starts at fcstlim and TIGHTENS to each accepted mape(4)
        method=first -> stop at the first acceptance
if nothing accepted but a `*` candidate exists and the run needs regARIMA
    preadjustment factors -> use it with forecasting off (nofcst, hvstar==2)
```

The five built-ins (`setamx.f`), which are exactly what
`tests/corpus/extra/pickmdl.mdl` spells out:

| # | model |
|---|---|
| 1 | `(0 1 1)(0 1 1)` |
| 2 | `(0 1 2)(0 1 1)` |
| 3 | `(2 1 0)(0 1 1)` |
| 4 | `(0 2 2)(0 1 1)` |
| 5 | `(2 1 2)(0 1 1)` |

`Lseff` (stable seasonal effects in the regression) drops the seasonal
difference and MA to 0 in all five.

## Option measurements

Method: **oracle on-vs-off** (does the argument move the reference?) then
**engine-vs-oracle** (does the port follow?). Both on
`extra/airline_pickmdl`, whose baseline is bit-exact — 0 of 56 shared `.udg`
keys differ. Probe at
`scratchpad/pk_probe.py`; deltas are counts of `.udg` keys differing by more
than 1e-5 relative.

| argument | probe | oracle on-vs-off | engine-vs-oracle | state |
|---|---|---:|---:|---|
| `file=` | present vs absent | (whole model list) | 0 | **gated** (`-nofile`) |
| `qlim=` | 90 (default 5) | 160 | 0 | **gated** (`-qlim`) |
| `qlim=` | 50 | 160 | 0 | gated by the above |
| `fcstlim=` | 3 (default 15) | 160 | 0 | covered |
| `fcstlim=` | 1 | 160 | 0 | covered |
| `fcstlim=` | 0 | (no acceptance) | 0 | **gated** (`-default`) |
| `overdiff=` | 0.3 (default 0.9) | 156 | 0 | covered |
| `overdiff=` | 0.1 | 168 | 0 | **gated** (`-overdiff`) |
| `method=` | `first` (default `best`) | 156 | 0 | **gated** (`-first`) |
| `identify=` | `first` + `outlier{}` | 189 | 0 | **gated** (`-identify-first`) |
| `mode=` | `both` vs `fcst` | **0** | 0 | **INERT, provably** |
| `outofsample=` | `yes` | (different model) | 0 | **gated** (`-outofsample`) |
| `bcstlim=` | needs `maxback=` | **0** | 0 | **INERT** — CB-33; gated |
| `print=` / `savelog=` | — | — | — | print surface |

**`mode=` is inert and the reason is structural, not empirical.** `iautom` is a
LOCAL in `gtinpt.f:873`; its only use is the `> 0` test at `:878` that sets
`Lautox`, and `gtautx.f:230` forces it to 1 when no `mode=` was given. Nothing
downstream ever reads which of 1 or 2 it holds. The measured 0 confirms the
reading rather than standing alone.

**Direction matters, as always.** All three numeric screens bite when the
argument is moved so as to REJECT more models: `qlim` UP from 5, `fcstlim` DOWN
from 15, `overdiff` DOWN from 0.9. A sweep that only pushes values one way
reports INERT for at least two of them.

## The gate specs

Seven, all in `tests/corpus/extra/`. Six are hand-authored and carry a header
saying so — `genextra.py` does not produce them and running it deletes them.
The selected models spread across three shapes, which is the point: a gate
where every variant picks the same model tests only that the engine runs.

| spec | pins | oracle picks |
|---|---|---|
| `airline_pickmdl` | the base loop, `method=best` ranking | `(0 1 2)(0 1 1)` |
| `airline_pickmdl-first` | `Pck1st` early exit | `(0 1 1)(0 1 1)` |
| `airline_pickmdl-nofile` | `setamx` + the CNOTST `havfil` test | `(0 1 2)(0 1 1)` |
| `airline_pickmdl-qlim` | the Ljung-Box screen | `(0 1 1)(0 1 1)` (fallback) |
| `airline_pickmdl-overdiff` | the MA-sum screen | `(2 1 0)(0 1 1)` |
| `airline_pickmdl-default` | `hvstar==2` fallback + `nofcst` | `(0 1 1)(0 1 1)` (fallback) |
| `airline_pickmdl-identify-first` | `automx.f`'s label-20 re-estimation | `(0 1 2)(0 1 1)` |

`-overdiff` is the only one that exercises a screen reading the ESTIMATED
COEFFICIENTS rather than a forecast statistic, and it is the only one that
selects `(2 1 0)(0 1 1)`.

No save-table goldens exist for this family (the oracle writes only
`.udg`/`.out`/`.log`/`.err` for these specs), so the gates are the `.udg`
ones — `test_check_diagnostics`, `test_qs_diagnostics`, `test_spectrum_peaks`,
`test_x11_diagnostics`, `test_x11_etables`.

## Still open

> **Status here is NOT authoritative.** This section records what was *measured*
> — the cost, the preconditions, the Fortran a future increment has to
> reproduce. That is durable. Whether an item is still open is not: the
> authoritative answers are [`docs/WALLS.md`](../docs/WALLS.md) (generated from
> the engine's own refusals — if it is not walled and not gated, that is the
> dangerous case) and the open list in
> [`tools/SESSION_HANDOFF.md`](SESSION_HANDOFF.md), which is rewritten each
> session. Duplicating status into a scouting doc is what let this file
> contradict itself for several sessions.


1. ~~**`outofsample=yes` (`Outfer`)**~~ — **CLOSED 2026-07-29b**, together with
   `estimate{outofsample=}`. What it does: for each of the last three years pull
   the model span end back another year, RE-ESTIMATE, and forecast one year from
   the new span end, so the forecast is made by a model that has never seen the
   period it forecasts. Around it, `amdfct.f:71-82`/`:285-292` save and restore
   `Chlxpx`/`Chlgpg`/`Chlvwp`/`Matd`/`Armacm`/`Lndtcv`/`Lnlkhd`/`Var`, the
   ssprep snapshot and `Endmdl` -- but NOT `Nfev`/`Niter`, because the final
   `rgarma` at `:299` is commented out, so the .udg reports the LAST re-fit's
   optimizer counters. Measured and reproduced (19/6 -> 13/4).
   Two things the port had to get right beyond transcription: the arm strips
   every OUTLIER regressor dated inside the three-year window from the design
   (`dlrgef`) and subtracts its fitted contribution out of the series
   (`daxpy` -> `fotl` -> `eltfcn SUB`), because the shortened span no longer
   contains those dates; and the `ave` scale block reads the STRIPPED series,
   not the caller's `Trnsrs`. **Confirmed the scouting claim: the oracle selects
   a different model with it on** -- `(0 1 2)(0 1 1)` -> `(0 1 1)(0 1 1)`,
   `nmodel` 3 -> 2 on `extra/airline_pickmdl`. Gated by
   `extra/airline_pickmdl-outofsample` plus `generated/airline_outofsample`
   (the `estimate{}` twin, and the `nfev`/`niter` pin) and
   `generated/airline_outofsample-otl` (the outlier strip, with an
   out-of-window outlier as the negative control).
2. ~~**`bcstlim=` / `forecast{maxback=}`**~~ — **CLOSED 2026-07-29b.** The
   `Bckcst` arm is the out-of-sample one mirrored: the Xy design is time
   REVERSED so the same forward machinery extrapolates backwards, the outlier
   window is the FIRST three years (and every type is judged by its start, with
   no ramp special case), the `ave` scale reads the first three years, and the
   out-of-sample variant walks `Begmdl` FORWARD instead of `Endmdl` back, taking
   the ACTUALs from the year it is about to drop, reversed, because
   `amdfct.f:239` skips `subset` on exactly that path. Gated by
   `extra/airline_pickmdl-backcast{,-oos,-zero}` through
   `tests/parity/test_backcast_aape.py`, which reads the oracle's PRINTED block
   -- prtamd writes no savelog key for it.
   **`bcstlim=` itself is INERT, and that is CB-33:** `automx.f:922`'s
   `IF(mape(4).gt.Bcklim.and.(.not.argok))` is `.and.` where the algorithm wants
   `.or.`, so a converged model can never fail the screen. Measured: with
   `bcstlim=1` the oracle prints "MODEL 2 REJECTED: Average backcast error >
   1.00%" and then "The model chosen is (0 1 2)(0 1 1)", and keeps all twelve
   backcasts; every `.udg` key but `bcstlimit` is unchanged.
   **One narrow gap left, walled with the measurement:** out-of-sample
   BACKCASTS with an outlier regressor inside the first three years reads 6.6959
   against the oracle's printed 6.71. The strip fires (instrumented) and
   disabling it changes nothing, so the difference is downstream of the window
   test. Every other combination is bit-exact -- within-sample backcasts with
   outliers, out-of-sample FORWARD with outliers, out-of-sample backcasts with
   no outlier in the window, and the `ivalue==1` scale branch.
3. ~~**Per-candidate AIC-regressor testing**~~ (`automx.f:404-500`, `:750-870`) —
   **CLOSED 2026-07-30**, together with the two reachable **Picktd** restores
   and the post-loop `identify=first` block. `regression{aictest=}` alongside
   `pickmdl{}`: the AIC tests REPLACE the plain `rgarma` for a freshly
   identified candidate (`tdaic`/`lomaic`/`easaic` self-estimate) and `argok`
   becomes `.not.lester`. `Picktd` decides whether the program's
   length-of-month / leap-year prior is IN the series, so flipping it between
   candidates changes the series being modelled, not just the design, and the
   restore rebuilds `Trnsrs`/`Adj` (`td7var` + the Usrtad/Usrpad folds +
   `trnfcn`) with `Priadj` following (4 = program TD prior, 1 = none).
   Two structural notes worth keeping. (a) `editor.f:1151-1166`'s TD candidate
   vector is built ONCE in the oracle's editor, before any model is estimated;
   this port has no editor block for it, so each caller runs the shared
   `aictest_td_vectors` at its own equivalent point — a per-candidate rebuild
   would read a design the editor never saw, since `tdaic` itself adds and
   deletes TD groups. (b) `Setpri = Pos1bk` (`editor.f:851`, after `setxpt` at
   `:233`) is 0 during this port's model phase because `setxpt` is never called
   pre-model, so tdaic's `Sprior` writes stay guarded off.
   **Three silent-wrongness bugs came out of it, two of which are NOT specific
   to pickmdl and affect every aictest path:** `Pvaic`/`Rgaicd` were reset per
   CALLER (both `automd` and the explicit-aictest path clobbered them on entry)
   instead of in `gtinpt`, so the pickmdl path read the struct's zero-init and a
   `pvaic` of 0.0 rather than `DNOTST` turns tdaic's `chsppf(pvaic, df)`
   threshold ON, driving `Rgaicd(PTDAIC)` negative enough that the FIRST TD
   candidate always wins; and `regression{aicdiff=}` (`getreg.f:405-428`) and
   `pvaictest=` (`:474-497`) were **parsed and discarded** outright. The
   pre-existing `generated/cover_reg-aicdiff` could never have caught the last
   one — it carries `aicdiff=0.0`, which is the DEFAULT and therefore inert.
   **`pickmdl{identify=}` was structurally inert across the whole corpus** until
   this increment: `lidotl` is `Ltstao.or.Ltstls.or.Ltsttc` (`arima.f:118`), so
   an `outlier{}` spec is what makes it do anything, and no pickmdl spec carried
   one. Found by a mutation that deleted the per-candidate design restore and
   passed the entire suite; closed by `extra/airline_pickmdl-aictest-otl`.
   Gated by `extra/airline_pickmdl-aictest-{td,tdeas,first,otl}` and
   `generated/airline_aictest-td-aicdiff`. Mutations, each failing a different
   set: never run the AIC tests 30; never restore the design between candidates
   10; skip the AIC tests in the post-loop `identify=first` block 10; revert the
   `gtinpt` `pvaic` default 105; narrow the post-loop gate back to `lidotl` only
   4.
   **One corner is WALLED, not closed** (`docs/WALLS.md`): a trading-day AIC
   verdict that DIFFERS between candidates — reachable only with a
   `regression{aicdiff=}` tuned between two candidates' AICC gaps, which is why
   the spec written for it was removed rather than blessed. Characterized:
   d10, d12 and d16 are BIT-EXACT and all 52 shared `.udg` model keys agree
   (including `nreg` and every ARMA coefficient); only d11/d13 move, by exactly
   the leap-year prior on FEBRUARIES ONLY (0.885% non-leap, 2.655% leap). Ruled
   out: CB-34's assignment direction (mutation-tested, identical failures) and
   the `Sprior`/`Setpri` deferral (`x11int` copies `Adj` into `Sprior` whenever
   `Nadj>0`, which this port always satisfies). Remaining suspects, named at the
   wall: which of `Kfmt` / `Lpradj` / `Priadj` the oracle carries out of the
   LAST candidate's tdaic.
   **`aictest=(user)` and user-holiday chi-square testing stay walled**
   (`automx.f:463-500`) — `usraic.f` and `chkchi.f` have no C++ at all.
   **CB-34** was logged here: `automx.f:264` assigns `padj2=Priadj` where its
   two siblings (`:330`, `:725`) assign `Priadj=padj2`.
4. **The `!Hvmdl` no-model cleanup** (`arima.f:476-527`) — when automx accepts
   nothing AND there is no starred default, the oracle turns every regARIMA
   prior-adjustment indicator back off, drops the forecasts, restores the model
   span and disables the revision analyses. The port fatals instead. Not
   reached by any corpus spec (`fcstlim=0` still has a starred default).
