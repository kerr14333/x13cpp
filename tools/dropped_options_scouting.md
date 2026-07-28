# Dropped-option sweep — 2026-07-27

**Nine spec arguments were accepted and silently ignored** — each returning
`OUTCOME: OK` with the DEFAULT numbers, none gated, none fatal, and none
previously recorded as open except `spectrum{start=}`.

**STATUS: all nine closed** (commits `1fc26bdf`, `M5/specparse`). Eight are
applied and gated over 16 new corpus specs; `estimate{outofsample=}` is now an
honest fatal. `spectrum{altfreq=}` is applied as far as it can be — see CB-30.
The findings table and the method below are kept because the method is reusable
and the two ways the script lied are easy to rebuild.

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
| ~~`regression{noapply=}`~~ | ~~70~~ | **CLOSED** — see below |
| ~~`regression{tcrate=}`~~ | 64 | **CLOSED** — `generated/airline_reg-tcrate` (+ `airline_outlier-tcrate`, the same latch) |
| ~~`forecast{lognormal=}`~~ | 33 | **CLOSED** — `generated/airline_fcst-lognormal`; only the Fcstx half was missing |
| ~~`spectrum{start=}`~~ | 30 | **CLOSED** — `extra/airline_spectrum-start` |
| ~~`regression{eastermeans=}`~~ | 26 | **CLOSED** — `generated/airline_reg-eastermeans` |
| ~~`spectrum{peakwidth=}`~~ | 21 | **CLOSED** — `extra/airline_spectrum-peakwidth{2,3,4}` |
| ~~`spectrum{altfreq=}`~~ | 15 | declines by design — **CB-30**, an uninitialised read in mkpeak.f |
| ~~`estimate{outofsample=}`~~ | 6 | **FATAL** now, instead of a mislabelled within-sample answer |
| ~~`spectrum{showseasonalfreq=}` / `{saveallfreq=}`~~ | 4 each | **CLOSED** — two more `extra/airline_spectrum-*` specs |

**`outlier{tcrate=}` measured DIFFERS**, not DROPPED — neither honouring the
argument nor reproducing the oracle's default, because automd's own
`0.7^(12/Sp)` fallback filled `Tcalfa` at a different point than
`gtinpt.f:1217` does. Same missing parse underneath; closed with the other.

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

# Round 2 — `automdl{}` re-probed properly (2026-07-28)

**Round 1's twenty INERT verdicts became SEVEN dropped options.** Nothing about
the engine changed; the probe did. Reproduce with
`python tools/option_sweep.py option_sweep_cases_automdl2`.

## The three things round 1 got wrong

1. **The series was saturated.** Fixed by picking on two measured criteria at
   once — strong seasonality *and* a near-tied top two.
2. **Three probe VALUES were rejected outright**, so those arguments were never
   tested at all. `ub1` must exceed 1, `seasonaloverdiff` takes yes/no
   (`gtauto.f:479`), `noautooutlier` takes `same`/`tramo` (`NOTDIC`,
   `gtauto.f:48`, label 180). **A REJECTED probe is an untested argument, not a
   null** — and fixing just this one found `noautooutlier`, which is DROPPED on
   four of five series.
3. **Four of the twenty-four arguments were missing from the probe list**
   (`percentrse`, `acceptdefault`, `firstar`; `savelog` is print surface).
   **An argument absent from the list reads exactly like one that measured
   INERT.** `acceptdefault` turns out to be applied — worth knowing, and
   unknowable while it was simply not asked about.

## The probe series, and why each earns its place

Oracle, `automdl{} + x11{}`. `qsori` is the QS seasonality statistic on the
original, `f3.m07` is M7 (>1 fails the identifiable-seasonality test), and the
gap is `bic2 − bic1` over automdl's own best-five list.

| series | qsori | M7 | gap | top two |
|---|---:|---:|---:|---|
| `ces_amuse` | 620.8 | 0.075 | 0.001 | `(1 1 1)(1 1 1)` / `(0 1 0)(1 1 1)` |
| `ces_leis` | 626.6 | 0.093 | 0.002 | `(1 1 1)(1 1 1)` / `(0 1 0)(1 1 1)` |
| `ces_accfood` | 612.0 | 0.106 | 0.005 | `(1 1 1)(1 1 1)` / `(1 1 1)(0 1 1)` |
| `ukgas` | 176.5 | 0.210 | 0.001 | `(1 0 2)(0 1 0)` / `(1 0 2)(0 1 1)` |
| `nottem` | 237.8 | 0.127 | 0.018 | `(1 0 0)(1 1 1)` / `(2 0 0)(1 1 1)` |
| *airline (round 1)* | 167.6 | 0.202 | **0.014** | |

The three `ces_*` series are new — BLS CES, not seasonally adjusted, see
`tests/corpus/data/ces_PROVENANCE.md`. `ces_accfood` was added specifically
because its tie is in the **seasonal AR**, which no other series offers.

**`unrate` has a 0.001 gap and is still a bad probe**: `qsori ≈ 0`, `M7 = 2.58`,
i.e. no identifiable seasonality, so its near-tie is between two *nonseasonal*
candidates and cannot exercise a seasonal threshold. Same for `payems` and
`expgs`. **Narrow gap and strong seasonality are two independent criteria and a
probe needs both.**

## Findings

| argument | verdict | where it fired |
|---|---|---|
| `mixed` | **DROPPED** | ukgas 29, nottem 28, ces_leis 30, ces_accfood 43 keys (DIFFERS on ces_amuse) |
| `noautooutlier` | **DROPPED** | ukgas 17, nottem 18, ces_leis 19, ces_accfood 50 keys (DIFFERS on ces_amuse) |
| `urfinal` | **DROPPED** | ukgas 33, **ces_accfood 66 keys** — moved `AR$Seasonal$12$12` |
| `diff` | **DROPPED** | ukgas 33, nottem 32 keys |
| `maxorder` | **DROPPED** | ukgas 34 keys |
| `balanced` | **DROPPED** | ukgas 37 keys |
| `checkmu` | **DROPPED** | ukgas 33 keys |
| `ljungboxlimit` | applied | ces_amuse, 80 keys |
| `acceptdefault` | applied | ukgas, 34 keys |

The two `applied` rows are **unpinned claims**: no corpus spec gates either, so
they are correct today with nothing stopping a refactor from breaking them. Same
class as `history{endtable=}`, which was already right and only needed a gate.

Still INERT on all five series: `maxdiff`, `ub1`, `ub2`, `cancel`, `exactdiff`,
`hrinitial`, `armalimit`, `percentrse`, `reducecv`, `firstar`, `fcstlim`,
`seasonaloverdiff`, `print`. `rejectfcst` is HARNESS-BLIND (it moves
`fcstrejected`/`mape3yr`/`rejectfcst`, none of which the harness prints).
**Given that round 1's INERT verdicts were worth seven findings, none of these
should be read as a clean bill either** — read each default out of `gtauto.f`
before choosing the next probe value.

## What the DOCUMENTATION added, having read only the Fortran first

The reference manual (`docx13as.pdf`, 306pp, Census — not vendored; fetched from
`www2.census.gov/software/x-13arima-seats/x13as/unix-linux/documentation/`)
was consulted only after the sweep, and it changed two conclusions:

* **`urfinal` is blocked, not dropped.** "Threshold value for the **final** unit
  root test. If the magnitude of an AR root for the **final model** is less than
  this number, a unit root is assumed…" — that is `chkrt1` at `automd.f:717`,
  inside the model-adequacy stage (`automd.f:577-850`) that `automd.cpp` has
  always documented as deliberately unported. No amount of parsing reaches it.
* **`noautooutlier` appears NOWHERE in the manual.** It exists only in
  `gtauto.f`'s `NOTDIC`. It selects `tstmd1` (`automd.f:577`) over the `amidot`
  path — also inside that same stage.

**A doc/code divergence worth knowing:** the manual says `maxorder`'s regular
order "must be greater than zero", but `gtauto.f:70` validates `omax(1).lt.0`
(zero is allowed) with the documented `.le.0` form sitting **commented out**
directly above it. The code was relaxed and the manual was not updated. The port
transcribes the CODE, which is what parity requires.

Defaults were audited against the quick reference at the same time and all match
(`ubfin` 1.05, `pcr` 0.95, `predcv` 0.14286, `tsig` 1.0, `cancel` 0.1,
`maxord`/`diffam` (2,1), and the five yes/no flags). No drift.

**Read the docs before sizing the work, not after.** Two of the seven findings
were never parse bugs at all, and the manual says so in one sentence each.

## Round 3 — probe VALUES, and the one blocker underneath

Round 2's values were merely *different* from the defaults; round 3 made them
**far** from the defaults, and two more arguments moved:

* **`cancel`** — DROPPED on nottem at `cancel=0.5` (default 0.1). Round 2
  probed `0.05` and read INERT. Now parsed; `iddiff.cpp` already consumed it.
* **`maxdiff`** — moves at `(1,0)` where round 2's `(1,1)` did nothing.
* **`ljungboxlimit`** — see above; `0.99` against a 0.95 default measured
  INERT, `0.5` moves three of four series.

**A probe value adjacent to the default tests nothing, and the sweep reports it
with the same word — INERT — as an argument that genuinely does nothing.** The
values are now annotated with the default they are moving away from.

### The consolidated picture: one blocker, not eleven bugs

| state | arguments |
|---|---|
| **applied + gated** | `diff` `balanced` `acceptdefault` |
| **DIFFERS** (read, wrong answer) | `mixed` `checkmu` `maxorder` `maxdiff` `cancel` |
| **FATAL** (blocked, was silent) | `urfinal` `noautooutlier` `ljungboxlimit` |
| INERT even at extreme values | `ub1` `ub2` `exactdiff` `hrinitial` `armalimit` `percentrse` `reducecv` `firstar` `fcstlim` `seasonaloverdiff` |
| HARNESS-BLIND | `rejectfcst` |

Five arguments now read correctly and all five land somewhere other than the
oracle. That is one shared cause, not five bugs: `automd.f`'s **model-adequacy
and nloop stages** (`:577-850` and the `pass2` call at `:665`), which
`automd.cpp` has always documented as unported. The three FATALs are consumed
*only* there. **Stop adding parse cases expecting matches — the remaining
automdl option surface is gated on that one port.**

### Round 4 — the blocker was one unreachable CALL, and the table above is now wrong

The shared-cause claim held. The sizing did not: there was **no unported
stage**. `automd_finalize_tail` — `mdlchk` / `pass0` / `chkrt1`+redomd /
`testodf` / the residual-mean Constant / the `tstmd2` drop loop, i.e.
`automd.f:654-983` — was already ported and already bit-exact, and was simply
never called on the plain (non-aictest) path. `automd.f` has one path with the
AIC tests as conditional blocks *inside* it; this driver had branched around it.
One added call, re-measured on the same probe set:

| argument | round 3 | round 4 |
|---|---|---|
| `urfinal` | FATAL | **applied** — fatal removed, gated |
| `checkmu` | DIFFERS | **applied** (ukgas); harness-blind elsewhere |
| `cancel` | DIFFERS | **applied** (nottem); inert elsewhere |
| `mixed` | DIFFERS | applied (ukgas); DIFFERS on 3 |
| `maxdiff` | DIFFERS | applied (ces_leis); DIFFERS on 3 |
| `maxorder` | DIFFERS | unchanged |
| `noautooutlier` | FATAL | FATAL — needs label 40's `tstmd1` arm |
| `ljungboxlimit` | FATAL | FATAL — needs `pass2`/nloop, genuinely unported |

Zero regressions (5509 → 5516 passing). Gated by
`generated/{ukgas,ces_accfood}_automdl-urfinal`, mutation-tested.

**"Unported" and "unreachable" present identically** — an option that moves the
oracle and not the engine — **and cost three orders of magnitude apart.** The
closing note in `automd.cpp` had asserted "unported" for two sessions on the
strength of a single failed experiment (wiring `tstmd1` alone, whose *missing*
re-estimate is precisely what the tail supplies). Read the call graph before
sizing.

### Round 5 — the branch is gone, and eight of eleven are closed

The rest of `automd.cpp`'s aictest branch was hoisted onto the shared path:
the acceptdefault test, the `a0`/`adj0`/`trns0` saves, **label 10**
(`ssprep`/`bkdfmd`/`rmfix`) and the whole put-the-regressors-back block. There
is one path now, and `aic` gates only the three `tdaic`/`easaic` blocks.
`bkdfmd` is why label 10 had to come too — `tstmd1.f:221` restores from its
backup, so label 40's `tstmd1` arm cannot be correct without it.

Re-measured on the same probe set (baselines re-verified first: ukgas, nottem,
ces_accfood, ces_leis, co2 clean; ces_amuse and usdeaths disagree at baseline,
so nothing is read off those):

| argument | round 4 | round 5 |
|---|---|---|
| `noautooutlier` | FATAL | **applied** — fatal removed, gated on 3 series |
| `checkmu` | applied on ukgas | **applied on all 4** — gated |
| `maxdiff` | applied on ces_leis | **applied on all 4** at `(1 1)` — gated |
| `cancel` | applied on nottem | **applied** on nottem — gated |
| `mixed` | applied on ukgas, DIFFERS on 3 | unchanged |
| `maxorder` | DIFFERS everywhere | applied on 3, **DIFFERS on ukgas** |
| `ljungboxlimit` | FATAL | FATAL — `pass2` |

`mixed` and `maxorder` fail on **disjoint** series, so the remaining two were
never one shared cause in the sense round 3 meant.

**The residual is `pass2`, and the claim that `pass2` is unreachable was
wrong.** Its guard is `IF(Lidotl.and.nloop.le.2)` and nothing else; the default
Lotmod forces `Lidotl` true, so the oracle calls it on **every** automdl run,
and its `ichk` revert is guarded `Naut0.le.Naut` = `0.le.0`, which the BIGCV
scan finding nothing does not close. The evidence that it is the residual: on
`mixed=no` and `maxorder=(1 1)` the oracle's final model differs from its own
`automdl.first`, i.e. it re-identified, which only `pass2`'s `Igo` can cause.
It is also what `ljungboxlimit=` needs (`pass2.f:160-169` increments `Pcr`), so
**one port closes all three remaining arguments.**

Round 4's lesson, inverted: **"unreachable" is a claim about a guard and has to
be read off the guard.** A green suite is consistent with "the routine is a
no-op on this corpus", which is a different statement.

### Round 6 — `pass2` ported; the automdl surface is CLOSED

Round 5's prediction held exactly: **every argument it named closed, and
nothing else moved** (5542 -> 5560 passing, zero goldens disturbed).

| argument | round 5 | round 6 |
|---|---|---|
| `mixed` | wrong on 3 of 5 | **bit-exact on all 5** |
| `maxorder` | wrong on ukgas | **bit-exact on all 5** |
| `maxdiff` at `(1 0)` | wrong on 3 of 5 | **bit-exact on all 5** |
| `cancel` at `0.9` | wrong on 2 of 5 | **bit-exact on all 5** |
| `ljungboxlimit` | FATAL | **applied** wherever the oracle moves |

**All 11 automdl arguments that move the oracle now apply. Nothing in
`automdl{}` is fatal or silently dropped**, which is the first spec block in
this sweep to reach that state. Details and the three `nloop`-dependent
`automd.cpp` bugs it surfaced are in `tools/automdl_scouting.md` UPDATE
2026-07-28d.

One method note worth carrying to the next block: **mutate a ported routine
per HALF, not as a unit.** pass2's two halves turned out to be covered by
disjoint specs, and unit-mutating it would have reported "covered" while
hiding that one gate exercised neither.

## Where the seven actually stand

| argument | after the port | why |
|---|---|---|
| `diff` | **applied** (ukgas, nottem) | needed a real engine fix — see below |
| `balanced` | **applied** (ukgas) | parse only |
| `mixed` | DIFFERS | reads it; final model still differs |
| `checkmu` | DIFFERS | " |
| `maxorder` | DIFFERS | " |
| `urfinal` | **FATAL** | blocked on the adequacy stage |
| `noautooutlier` | **FATAL** | " |

**`diff=` was a genuine engine defect, not just an unparsed argument.**
`automd.f:392` guards its `iddiff` call with `IF(Lautod)`; the port has TWO
`iddiff` call sites and the guard existed only on the **unreachable** one, so
the live site searched for the differencing orders unconditionally and `diff=`
could not have worked even once parsed. `maxdiff=` vs `diff=` differ *only* in
that `maxdiff` sets `Lautod` back on — the whole meaning of `diff=` is carried
by that one omitted assignment.

The three `DIFFERS` rows are most likely the same adequacy-stage gap (the oracle
re-estimates after `tstmd1`), but that is **a hypothesis, not a measurement** —
it has not been isolated.

**A probe-validity check that should have come first:** the engine's BASELINE
model must match the oracle's before any option verdict on that series means
anything. It does on `ces_leis` / `ces_accfood` / `ukgas` / `nottem` — and does
**not** on `ces_amuse`, where the engine picks a 4-term model against the
oracle's 5. Every `ces_amuse` row in the tables above is therefore baseline
noise, not an option finding, and was discarded.

## A tooling fix that came with it

`option_sweep.py` staged only `airline.dat` and `payems.dat` into its scratch
directory, so a case naming any other series failed on a missing file and read
as **REJECTED** — indistinguishable, in the report, from a value the oracle
refuses. It now stages every `tests/corpus/data/*.dat`.

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

## `regression{noapply=}` — CLOSED, and it was NOT parse-only

The prediction that it would be parse-only was wrong in an instructive way. The
parse is three dozen lines and `chkadj`'s `< 0` handling was indeed already
ported — but `noapply=(td)` then **fatalled the engine** on a wall that
`x11parts.cpp` carried a written-out proof was UNREACHABLE.

The proof was sound for the four routes it considered (`chkadj.f:209`,
`editor.f:2277`, `x11ari.f:110`, `xrgdrv.f:80`) and it could not consider this
one, because `noapply=` was consumed and discarded and therefore no spec could
set `Adjtd < 0`.

**A reachability argument is only valid over the options the PARSER honours.** A
dropped option silently deletes edges from the graph being reasoned about, so
the analysis proves something strictly narrower than it appears to. Any "this
branch is unreachable" claim in this port should be re-checked against the
dropped-option list above before it is trusted.

`tdlom.f:44-59` is now ported (six lines; note it sets `Priadj = 0` where the
`Adjtd==1` arm sets `-Priadj`, and ssprep/restor key on that sign). Gated by
`generated/airline_noapply-{td,ao,ls,holiday}`; **CB-29** records the dictionary
defect that makes `noapply=(seasonal)` unusable.

## What is left

Nothing from this sweep. Two follow-ons it created:

* **`spectrum{altfreq=yes}`** — needs CB-30 resolved (what the `/spcidx/` COMMON
  actually holds in `Tpeak(3)`/`Tup(3)`) before a golden means anything.
* **`automdl{}`'s 23 arguments** — re-probe on a series with near-tied
  candidates first, per the section above. The airline nulls are a saturated
  precondition, not coverage.

The sweep itself should be re-run after any new spec argument is parsed:
`python tools/option_sweep.py option_sweep_cases`.
