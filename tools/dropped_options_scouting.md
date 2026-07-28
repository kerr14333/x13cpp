# Dropped-option sweep — 2026-07-27

**Nine spec arguments are accepted and silently ignored.** Each returns
`OUTCOME: OK` and the DEFAULT numbers. None is gated, none fatals, and none was
previously recorded as open except `spectrum{start=}`.

Reproduce: `python tools/option_sweep.py option_sweep_cases`.

## Method

Three runs per argument, and the third is what makes it a finding rather than a
guess:

| | run | purpose |
|---|---|---|
| A | oracle, base spec | the default behaviour |
| B | oracle, base + `option=<non-default>` | does the option matter at all? |
| C | **engine**, same spec as B | does the engine honour it? |

* `A == B` → **INERT**: the probe never moved the oracle. **No conclusion** — the
  value or the base spec is wrong, not the engine. This is the trap
  `tools/history_options_scouting.md` walked into and it is the single most
  common outcome here, so it is reported rather than filtered.
* `A != B`, `C == A` → **DROPPED**. The oracle moved; the engine did the default
  thing.
* `A != B`, `C == B` → applied.
* `A != B`, `C` matches neither → **DIFFERS** — read, but not the oracle's answer.

## Findings, by blast radius

| argument | keys moved | what it does |
|---|---:|---|
| **`regression{noapply=}`** | **70** | estimate the regressor but do NOT remove its effect from the series. `d11.f` 0.54795 → 0.62289 |
| **`regression{tcrate=}`** | 64 | the TC outlier decay rate |
| **`forecast{lognormal=}`** | 33 | log-normal forecast bias correction |
| **`spectrum{start=}`** | 30 | `Bgspec` override — the diagnostic span start |
| **`regression{eastermeans=}`** | 26 | Easter regressor centring |
| **`spectrum{peakwidth=}`** | 21 | the mkfreq peak-frequency grid width |
| **`spectrum{altfreq=}`** | 15 | alternate TD frequency set |
| **`estimate{outofsample=}`** | 6 | out-of-sample `aape` forecast error |
| **`spectrum{showseasonalfreq=}` / `{saveallfreq=}`** | 4 each | the emitted frequency inventory |

Also **`outlier{tcrate=}` DIFFERS** (25 keys, matching neither side) — read but
wrong, which is a different and possibly worse failure than dropped. Not yet
diagnosed.

### `estimate{outofsample=}` — the doc was half right, and the wrong half

`CLAUDE.md` says the out-of-sample `aape` variant is "walled, not approximated".
The WALL is real and is in `amdfct.cpp`; **the option that reaches it is not
parsed**, so the wall never fires. Measured:

```
oracle default          aape.mode: withinsample   5.6262  2.8148  6.3759  7.6878
oracle outofsample=yes  aape.mode: outofsample    5.7533  2.9044  6.7551  7.6004
engine  outofsample=yes aape.mode: withinsample   5.6262  2.8148  6.3759  7.6878
```

The engine both reports the wrong numbers and **mislabels `aape.mode`**, which
is the part a caller would have trusted. Generalizable: *"the computation is
walled" and "the option is safe" are different claims.* A wall on the arithmetic
is worthless if the argument that selects it is dropped upstream.

## What the sweep also showed, which is not a finding

**`automdl{}`'s 23 unapplied arguments are almost all INERT on airline.** The
static reader analysis flags them (one `case` in 24) and `gt_automdl`'s own
comment admits they are token-consumed, but on this series the model choice does
not move: `maxorder`, `maxdiff`, `balanced`, `exactdiff`, `mixed`, `checkmu`,
`fcstlim` change only their own `.udg` echo. `ub2`, `cancel`, `hrinitial`,
`armalimit`, `reducecv`, `ljungboxlimit`, `urfinal` move nothing at all.

That is **not** a clean bill — it is a saturated precondition. Airline's
`(0 1 1)(0 1 1)` wins by 0.014 BIC over the runner-up and these knobs adjust
unit-root and cancellation thresholds, so testing them needs a series with
near-tied candidates or borderline roots. **Before porting any automdl option,
re-probe on such a series; the airline nulls prove nothing.**

## Two ways this script lied before it worked

Recorded because both are easy to rebuild:

1. **Comparing engine to oracle over the union of their keys** reports the
   port's entire unported savelog surface on every row, so every option looked
   like a finding. The engine comparison must be restricted to the 3-way key
   intersection — but the *oracle moved at all?* test must NOT be, or an option
   the harness is simply blind to reads as INERT. Two questions, two key sets;
   conflating them gives a confident wrong answer in both directions. The
   `HARNESS-BLIND` outcome exists to keep them apart.
2. **String comparison of values.** Re-convergence noise moves `aic` in its 9th
   digit, so every auto-selected-model row reported DIFFERS. Values are compared
   field-by-field with a 1e-6 relative bound; the question here is "did the
   engine do the same THING", and the parity gates answer bit-exactness.

## Suggested order

1. `regression{noapply=}` — biggest blast radius, and it is a documented,
   ordinary user option (`chkadj`'s Adj* indicators, which are already ported —
   this is likely a parse-only fix).
2. `forecast{lognormal=}` and `regression{tcrate=}`/`{eastermeans=}` — same
   class, self-contained.
3. `estimate{outofsample=}` — cheapest correct action is to make the existing
   wall REACHABLE (parse it, then fatal) so it stops reporting a mislabelled
   within-sample answer. Porting the arithmetic is separate.
4. The four `spectrum{}` grid arguments together — they share `mkfreq`.
5. `outlier{tcrate=}`'s DIFFERS, which needs diagnosis before it can be scoped.
