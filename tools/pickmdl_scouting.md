# `pickmdl{}` / `automx.f` — scouting and option measurements

Status: **CLOSED 2026-07-28e** for the base algorithm; three walls remain, each
measured and each a clean fatal. See `core/src/automdl/automx.hpp` for the
algorithm and `tools/SESSION_HANDOFF.md` for the session context.

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
| `outofsample=` | `yes` | (different model) | — | **WALLED** (fatal) |
| `bcstlim=` | needs `maxback=` | (runs) | — | **WALLED** (fatal) |
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


1. **`outofsample=yes` (`Outfer`).** amdfct's out-of-sample arm
   (`amdfct.f:70-90`, `:186-235`, `:270-300`) re-fits the model three times over
   successively shorter spans and saves/restores the entire estimation state
   (`Chlxpx`/`Chlgpg`/`Chlvwp`/`Matd`/`Armacm`/`Lndtcv`/`Lnlkhd`/`Var` plus the
   model span). **Measured: the oracle selects a different model with it on**,
   so this changes the answer, not just a label. Closing it also closes
   `estimate{outofsample=}`, walled for the same reason.
2. **`bcstlim=` / `forecast{maxback=}`.** `automx.f:903-928` re-scores the
   winner over the backcast span via amdfct's `Bckcst` arm and can un-select it
   for backcasting (`Nbcst=0`, `Pos1bk=Pos1ob`). The oracle runs this
   combination fine.
3. **Per-candidate AIC-regressor testing** (`automx.f:404-500`, `:750-870`) —
   `tdaic`/`lomaic`/`easaic`/`usraic`/`chkchi` inside the candidate loop, i.e.
   `regression{aictest=}` alongside `pickmdl{}`. It can flip `Picktd` between
   candidates, which is what makes the **Picktd trading-day restore**
   (`:255-292`, `:317-323`, `:700-725`) reachable: the transformed series and
   the prior-adjustment array have to be rebuilt (`td7var` + the Usrtad/Usrpad
   folds + `trnfcn`) whenever the best model's `Picktd` differs from the live
   one. The two are one piece of work; neither is reachable without the other.
4. **The `!Hvmdl` no-model cleanup** (`arima.f:476-527`) — when automx accepts
   nothing AND there is no starred default, the oracle turns every regARIMA
   prior-adjustment indicator back off, drops the forecasts, restores the model
   span and disables the revision analyses. The port fatals instead. Not
   reached by any corpus spec (`fcstlim=0` still has a starred default).
