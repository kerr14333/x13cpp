# `history{}`'s remaining option surface — measured before porting

Every flag below was **parsed and stored by the C++ and then never read**. That is
not automatically a wrong-numbers bug, and the difference is only knowable by
measurement — so each one was first run through the ORACLE on-vs-off before any
porting work. Probe scripts: `hist_probe.py` / `hist_probe2.py` (scratchpad;
airline, `history{start=1955.jan}`, all ten sadj/sadjchng/seasonal/trend/trendchng
save tables diffed against a per-family baseline).

**A null measured under the wrong preconditions is not a null.** Round 1 returned
0.000e+00 for four flags; three of them were structurally incapable of moving
anything in the spec they were tested in, and moved decisively once re-probed
under the conditions the Fortran actually requires. Round 2 is those re-probes.

## Results

| option | COMMON | oracle on-vs-off | precondition that makes it live |
| --- | --- | --- | --- |
| `outlier=remove` | `Otlrev` | **MOVES** sar 1.3e+2, sae 1.7e-2 | outliers carried as regression variables |
| `outlier=auto` | `Otlrev` | **MOVES** sar 7.9e+0, sae 1.8e-2 | `outlier{}` present (per-span re-identification) |
| `outlierwin=` | `Otlwin` | **MOVES** (with `auto`) | only reached from `Otlrev>0` |
| `fixreg=` | `Rvfxrg` | **MOVES** sar 6.3e+2, sae 6.9e-3 | a `regression{}` group to fix |
| `fixx11reg=yes` | `Revfxx` | **MOVES** sar 3.8e+0, sae 8.3e-3 | `x11regression{}` — **CLOSED**, see below |
| `x11outlier=no` | `Rvxotl` | **MOVES** sar 6.5e+0, sae 7.5e-3 | `Otlxrg`, i.e. `x11regression{critical=}` |
| `endtable=` | `Rvend`→`Endtbl` | **MOVES** rows 71→48 *and* values 1.1e-4 | — (but the ENGINE was already right; see below) |
| `sadjlags=`/`trendlags=` | `Targsa`/`Targtr` | **new COLUMNS** 1→3 / 2→4, existing columns unchanged | — |
| `target=concurrent` | `Cnctar` | **MOVES** the revision columns (sar/chr/trr/tcr) 6.8e+1; levels unchanged | `sadjlags=`/`trendlags=` |
| `additivesa=difference` | `Rvdiff` | **MOVES** the revision + change tables 4.0e+0; levels unchanged | `x11{mode=add}` |
| `refresh=` | `Lrfrsh` | **0.000e+00 — inert, with a mechanism** | none; see below |

### `refresh=` is structurally inert, not merely unmeasured

Probed under three families (plain, `x11regression{}`, `x11regression{critical=}`)
and identical to 0.000e+00 in all ten tables every time. The mechanism:
`revdrv.f:528` calls `restor(Lmodel,Lx11,Ixreg.gt.0)` **unconditionally at the head
of every span**, so the extra `IF(Lrfrsh)CALL restor(Lmodel,F,F)` at `revdrv.f:746`
— the loop tail — is overwritten by the next span's restore before anything reads
it. There is no configuration in which the flag can change a number. Same class as
`seats{hpcycle=}`: measured zero blast radius, so no fatal guard is warranted —
there is nothing silently wrong to guard.

### `endtable=` was ALREADY CORRECT — CLOSED by gating it, not by porting

**The methodological hole this table had:** every row above is an ORACLE
on-vs-off measurement. That proves the flag *matters*; it says nothing about
whether the engine honours it. `endtable=` was listed here as parsed-but-silent
and ranked #2 to port, and it turned out `gt_history` case 6 already parses it
into `rv.rvend` and `run_history` already derives `endsa`/`endtbl`/`revnum` from
it. Measured engine-vs-oracle with `endtable=1957.dec`: both emit 36 rows
instead of 71, and every retained value agrees at the same per-span floor as the
full-table run. **Before porting anything off this table, measure
engine-vs-oracle too** — an ungated-but-correct feature looks identical to a
silently-dropped one from the oracle side alone.

The two `revchk.f` pieces around it need no port either, both checked:

* `revchk.f:616-624`, the `.not.Revsa` warning-and-reset (`endtable=` given but
  no sadj/seasonal/sadjchng/trend/trendchng estimate requested ⇒ `Rvend` forced
  back to the end of the series), is **observationally inert**. Nothing that
  survives that branch reads `Endtbl`: the model histories `lkh`/`amh`/`tdh` run
  over `Begrev..Endrev` and the forecast history keys on `Revptr+Rfctlg(k)`.
  Measured `estimates=(aic arma)+endtable` (72 rows) and
  `estimates=(fcst)+endtable` (71 rows) — oracle and engine agree either way,
  with the oracle printing its warning and the engine not (print surface).
* `revchk.f:629`'s `IF(Irev.eq.2)` override is **dead code**. `Irev` is only ever
  assigned 0, 1 (`gtrvst.f:349`) or 4 (`revdrv.f:387`) anywhere in the Fortran;
  2 is unreachable, so the ARGDIC-position guess that put `Irev==2` in the table
  above was simply wrong.

Gated by `extra/airline_history-endtable` (all ten revision tables).

### `fixx11reg=` led to a BIGGER bug — now CLOSED, along with the flag itself

**Status: both closed and gated.** `revdrv.f:530-532`'s demote is ported in
`run_history.cpp`, `x11ari.f:88-95`'s per-span `xrgdrv` in `run_x11_span.cpp`
(via a `span_mode` on `xrgdrv`), and `revdrv.f:309-330`'s `Revfxx` on top of it.
Results after, at the gate's own tolerances:

| family | before | after |
| --- | --- | --- |
| with model, DEFAULT, `sar`/`sae` | 8.22e-1 / 8.27e-3 | **6.8e-4 / 6.7e-6** |
| with model, `fixx11reg=yes` | 8.22e-1 / 8.27e-3 | **1.3e-3 / 1.2e-5** |
| model-free, `fixx11reg=yes` | 1.55e+0 / 1.52e-2 | **5.3e-15 / 1.8e-15** |
| model-free, DEFAULT | already bit-exact | unchanged |

Three findings from doing it, each of which cost a measurement:

1. **The estimation input, not just the factor.** `arima.f:156-157` copies the
   X-11 buffer `Sto` from `Pos1ob` *after* x11pt1 divided out that span's Faccal,
   and transforms THAT. Every other path in this port can shortcut it with the
   caller's pre-transformed series; the per-span xrgdrv cannot, because this
   span's Faccal differs from the main run's. Rebuilt the oracle's way, scoped to
   that path so every gated path stays byte-identical.
2. **State leak, the same class as `Lterm`/`Ksdev`/`Priadj` before it.** The
   transparent pass is a full x11pt1/x11pt2 and resolves `Lmsr`, `Kersa`,
   `Lstabl`, `L3x5` and `Nterm` in place. On the MAIN path `run_x11`'s editor
   block re-derives all of them after `xrgdrv` returns; a span has no editor
   block. Without handing them back the last span -- which covers the whole
   series and produced a Faccal IDENTICAL to the main run's, verified -- still
   came out 1.2e-4 off. **A per-span pass has to restore everything the main
   path's editor would otherwise re-derive, not just what `restor` restores.**
3. **`slidingspans{}` is NOT the same fix, and copying it there is a
   regression.** `ssx11a.f:93-95` has the identical demote, but with a regARIMA
   model `sfs` is currently BIT-EXACT (4.7e-15) with `Ixreg` left at 3 and goes
   to 4.1e+0 with the demote added -- the oracle evidently pays the demote back
   inside `sspdrv` (`Ssinit`/`Ssxint`), which is unported. Measured before and
   after; the note is left at the call site. Still separately wrong on that
   family: `chs` (5.5e+0, the `airline_slidingspans-td` per-span prior-phase
   problem) and the whole model-free case (`sfs` 2.0e+2). Neither moves with the
   demote.

Also settled here: **`fixmdl=yes` is INERT when `x11regression{}` is present**,
and faithfully so -- `revdrv.f:309-350` ends with `CALL restor(Lmodel,F,F)`,
which reinstates `Arimaf`/`Regfx`/`Iregfx` from the ssprep snapshot before the
loop starts, and `Revfix` only ever set the LIVE copy (the re-snapshot at
`revdrv.f:380` is commented out). Measured: oracle `fixmdl=yes` and default are
BYTE-IDENTICAL on this family, where without `x11regression{}` the same flag
moves sae 3.23e-3. This port had to *suppress* its ssprep mirror to reproduce it.

Gated by `extra/airline_history-x11reg{,-fixx11reg,-nomodel-fixx11reg,-fixmdl}`.

### `sadjlags=`/`trendlags=`/`target=` — CLOSED, and it was the OUTPUT gap

**Status: ported and gated.** These were the biggest output gap on the table —
whole columns absent, not values wrong. Each surviving lag adds one column per
table of its family: "the estimate `lag` periods AFTER the revision date" in
place of the full-data final one. Ported end to end:

* `setrvp.f:26-40` — `Endsa += mxrlag` (capped at the last observation), so the
  spans that make those estimates actually run. `Endtbl`/`Revnum` are fixed
  before this, so the TABLE does not grow; only the loop and the DNOTST cutoff.
* `revchk.f:1053-1110` — `intsrt` the list ascending, drop from the top any lag
  that does not fit in the revision span, and set `Lr1y2y` when a 1-year and a
  2-year lag both survive. **Order matters and is faithful: `setrvp` runs
  BEFORE this**, so a lag that is about to be discarded still widens `Endsa`.
* `getrev.f:57-70` (SA) / `:86-99` (trend) — each span files its estimate into
  the row it is `lag` periods past, `Fin(t, Revptr-lag)` read at `Posfob-lag`.
  The sorted list lets the Fortran `DO WHILE` stop at the first lag this span
  cannot serve; the port keeps that break. It needs one guard the oracle gets
  for free: `revdrv.f:416` turns `Lx11` off past `Endsa`, so getrev never runs
  on the trailing spans, while this port adjusts every span to `Endrev`.
* `prtrev.f:174-226` — the arithmetic, `Fin(0)-Fin(lag)` by default and
  `Fin(lag)-Conc` under `target=concurrent` (`Cnctar`), each percented by its
  own base on a level table; plus the DNOTST mask for rows whose target
  estimate does not exist yet, whose cutoff `Cnctar` moves one row later.

**The one asymmetry, and it is CB-22:** the extra `(1yr-2yr)` column exists only
on the SA / SA-change / indirect tables — `revdrv.f:852/859` pass `Lr1y2y` to
the two TREND calls as a literal `F` — even though `revchk` derives the flag
from both lists, with the trend list overwriting the sadj list's verdict. That
was caught by the gate asserting COLUMN COUNTS, not values: the engine emitted
a 4th trend column against the golden's 3 while every value it did emit agreed.
**A gate that reads the first N columns of a save file cannot see a wrong N.**

Measured engine-vs-oracle after the port, all three specs, every column of all
eight tables: revisions worst 1.4e-3 absolute (tolerance 5e-3), levels worst
8.4e-6 relative (tolerance 1e-5) — the ordinary per-span re-estimation floor,
the same one column 0 already sat at. Gated by
`extra/airline_history-sadjlags` (the 1yr/2yr pair on both families),
`-sadjlags-conc` (`target=concurrent`) and `-sadjlags-drop` (an unsorted list
carrying one lag too long for the span, so `intsrt` + the drop + the
widened-then-discarded `mxrlag` are all pinned).

The original measurement that started it is kept below.

### The measurement: the whole `x11regression{} + history{}` family was wrong

Measuring this one engine-vs-oracle (not just oracle on-vs-off) turned up a
silent wrong-numbers bug in the **default** path, not in the flag. No corpus spec
combines `x11regression{}` with `history{}` or `slidingspans{}` — zero, checked —
so nothing has ever gated it. Same discovery pattern as the Priadj restore and
the ssprep regression half: a span-replay defect that needs two features at once.

Measured on `airline` + `x11regression{variables=(td)}` + `history{estimates=
(sadj sadjchng trend)}`, tolerances 5e-3 absolute on the revision tables and
1e-5 relative on the levels (the gate's own policy):

| family | oracle on-vs-off | engine vs oracle DEFAULT | engine vs oracle `fixx11reg=yes` | engine default-vs-fixed |
| --- | --- | --- | --- | --- |
| with a regARIMA model, `sar` | 8.22e-1 | **8.22e-1** | 1.28e-3 (at the floor) | 0.000 |
| with a regARIMA model, `sae` | 8.27e-3 | **8.27e-3** | 1.25e-5 (at the floor) | 0.000 |
| model-free, `sar` | 1.55e+0 | **7.87e-1** | **1.18e+0** | 0.000 |
| model-free, `sae` | 1.52e-2 | **1.40e-2** | **1.54e-2** | 0.000 |

Two separate defects, and the table separates them:

1. **With a model, the engine silently IS `fixx11reg=yes`.** It matches the
   oracle's fixed run at the per-span re-estimation floor and misses the default
   by exactly the flag's delta. Cause: `revdrv.f:530-532` demotes `Ixreg` from 3
   back to 1/2 at every span head, so each span re-runs the x11regression
   irregular OLS; this port leaves it at 3, which means "already estimated and
   removed as a prior by xrgdrv" and makes x11pt2 skip its `x11mdl` entirely. The
   `Revfxx` flag itself is then a two-line addition on top (`setlg(T,PB,Regfxx)`
   + `Irgxfx=3`, revdrv.f:310-321) — but it is meaningless until the default is
   right, because the port already does what the flag asks for.
2. **Model-free, the FIXED branch is wrong** — the flag is simply dropped, so the
   engine gives its (correct) default numbers against the oracle's fixed run.
   The model-free DEFAULT row above is misleading: it was measured on a spec
   still carrying `transform{function=log}`, and a genuinely model-free spec
   (no `transform{}`/`arima{}`/`estimate{}`) is **bit-exact, 5e-15**. Re-measure
   before trusting a row in this table; that is the same lesson as `endtable=`.

**Do not "fix" this by adding the demote alone.** Tried, measured: it makes the
modelled family worse (sar 8.2e-1 → 1.2e+0 on the default, and it breaks the
previously floor-accurate fixed branch too). In the oracle `Ixreg==2` means *run
the transparent `xrgdrv` pass*, and this port HOISTS `xrgdrv` into
`run_pre_model` (see `core/src/x11/xrgdrv.hpp`) instead of reaching it from
x11pt2, so a demoted span takes the inline `x11mdl_td` route — the `Ixreg==1`
semantic — rather than the transparent pass. The demote had to land together
with a per-span `xrgdrv`; see the CLOSED section above for how that went.

The note is left at the call site in `core/src/driver/run_history.cpp` so the
next person does not re-derive it.

## Traps found while measuring

* **`OTLDIC` is `'keepremoveauto'` — the DEFAULT is `keep`** — even though
  `gtrvst.f:249`'s own error message lists the options as "remove, keep or auto".
  `Otlrev = sum(ivec2(i)-1)`, so keep=0, remove=1, auto=2 and `gtinpt.f:510`
  defaults it to 0. Round 1 read the message rather than the dictionary and
  expected `remove` to be the no-op; it is `keep` that is.
  (The C++ `readers_spec.cpp:1933` already has the dictionary right.)
* **The current engine's default behaviour is `keep`, and that is correct.**
  Measured: on `outlier{}`+`history{}` with everything defaulted, the engine
  agrees with the oracle at sae 7.1e-6 / sar 2.1e-3, i.e. the ordinary per-span
  re-estimation floor. `outlier=auto` would be 1.8e-2 in sae — 2500x that — so the
  port is not accidentally re-identifying per span. Only the NON-default values
  are silent.
* **`Cnctar` cannot move anything without the lags.** `prtrev.f:115/135/182` gate
  on `Cnctar .or. i2.eq.0`; with no `sadjlags=`/`trendlags=` only `i2==0` exists,
  so both sides of the gate are the same column. Round 1 measured 0.000e+00 for
  exactly that reason.
* **`Rvxotl` needs `Otlxrg`**, which `gtxreg.f:314/321` sets only from
  `x11regression{critical=}` (or the aictest-easter path at `editor.f:1735`).
  A bare `x11regression{variables=(td)}` leaves it false and the flag is dead.
* **`Lrfrsh` needs `Ixreg>0`** for its other consumer (`x11mdl.f:870`'s
  `loadxr(T)`) — but see above, it is inert regardless.
* **`fixx11reg=yes` + `x11regression{critical=}` makes the ORACLE abort the whole
  history**: "ERROR: Cannot perform automatic outlier identification if the robust
  mean square error of the residuals is zero" at the first span, and it writes no
  history tables at all. Gate `fixx11reg` on a plain `x11regression{variables=(td)}`
  instead, where the delta is clean.
* `outlier=auto` measured 0.000e+00 in the round-2 family (explicit AO/LS
  regressors, no `outlier{}` spec): the default critical value is above the
  series' largest t (3.48), so per-span detection finds nothing. Round 1's family
  set `critical=3.5` and moved. Not a null — a saturated precondition.

## Suggested porting order

1. `fixreg=` — the closest sibling to the already-ported `fixmdl` (`Revfix`,
   `revdrv.f:250-262`), largest measured delta, smallest new machinery.
2. ~~`endtable=`~~ — DONE, and it needed no port at all: the engine already
   honoured it and only lacked a gate. See the section above.
3. ~~`fixx11reg=`~~ — DONE, together with the per-span `xrgdrv` the default path
   needed. `x11outlier=` (`Rvxotl`) shares the `revdrv.f:309-350` `Ixreg` block
   and is still open; it needs `rmatot.f` (as `outlier=` does) on top of what
   landed here.
4. ~~`sadjlags=`/`trendlags=`/`target=`~~ — DONE. See the section below.
5. ~~`outlier=`/`outlierwin=`~~ — mostly DONE, and the ranking here ("the
   default is already right, so this is the least urgent") was **wrong, for the
   fourth time**. See the section below: the default was the most urgent thing
   on the list.
6. `additivesa=` — additive mode only, and the additive branches of
   `putrev`/`getrev` are out of scope for the current history port generally.

`refresh=` — do not port; record the mechanism instead.

### `outlier=` — the DEFAULT was wrong, and this table said it was not

**Status: `keep` (the default) and `remove` ported and gated; `auto` now a clean
fatal.** This one is worth reading as a method failure, not just a port.

The row above claimed *"the current engine's default behaviour is `keep`, and
that is correct — measured, on `outlier{}`+`history{}` with everything
defaulted, the engine agrees with the oracle at sae 7.1e-6 / sar 2.1e-3, i.e.
the ordinary per-span re-estimation floor."* That measurement was taken with
`outlier{critical=3.5}` on airline, **where the oracle identifies no outliers at
all** — the largest t in the whole series is 3.48, printed in the run's own
"might later be identified" table. The spec exercised the flag over an empty
set. Re-measured at `critical=3.0`, where three outliers are found:

| family | before | after |
| --- | --- | --- |
| `outlier{}` present, DEFAULT (`keep`) | sar **9.20e-1** / sae **1.52e-2** | 4.3e-4 / 4.3e-6 |
| `regression{variables=(ao1957.jan ls1958.jul)}`, DEFAULT | sar **9.77e-1** / sae **9.08e-3** | 7.2e-4 / 7.1e-6 |
| same, but outliers dated BEFORE the history start | 3.5e-4 / 3.4e-6 | unchanged |
| `outlier = remove` | sar 1.12e+0 / sae 1.53e-2 | 4.0e-4 / 3.8e-6 |

against tolerances of 5e-3 absolute / 1e-5 relative. **This is the same trap the
"traps" section above already names** ("a null measured under the wrong
preconditions is not a null", "`outlier=auto` measured 0.000e+00 ... not a null,
a saturated precondition") — and it was still walked into on the DEFAULT row,
because a saturated precondition looks like a passing gate rather than like a
zero delta. When a feature's whole effect is conditional on a set being
non-empty, the probe has to assert the set is non-empty.

What was actually missing is **not** behind `outlier=` at all: `rmotrv.f`
(revdrv.f:302) and `chkorv.f` (revdrv.f:589) run on every history{} with a
regARIMA model. A span ending at date T must not know about an outlier dated
after T, so every outlier-type regressor past the first revision date comes out
of the design before the loop and is re-introduced when a span's MODEL span
(`i - nend`, not `i`) reaches it. `outlier=` only chooses whether they are SAVED
for that re-introduction or dropped. Ported in `core/src/driver/rev_outlier.cpp`,
including chkorv's singularity pass (several outliers on the same last
observation are not jointly estimable, so its `opref` table picks one).

**The structural fix underneath it:** `restor.f:50-64` restores the whole design
DICTIONARY (`Ngrp`/`Nb`/`Colttl`/`Colptr`/`Grp`/`Grpptr`/`Rgvrtp`/`Ncxy`/`Nrxy`)
and this port's `restor_span` did not — it had only ever needed the coefficient
half, because until now nothing changed the regression STRUCTURE between spans.
Both halves of the ssprep/restor pair now carry it; it is byte-identical on
every path that leaves the design alone (verified: full parity unchanged).

`outlier=auto` (`Otlrev=2`) is left **fatal**, not silent: it needs each span to
re-run the automatic identification (`Ltstao`/`Ltstls` back ON, revdrv.f:517's
`Begtst = Endspn - Otlwin` test window, rmatot's save-and-re-enter arm, and the
per-span `rmatot` at revdrv.f:723) — a sub-engine `run_x11_span` does not have.
`outlierwin=` is only reachable from it. Measured delta if it were ignored:
sar 1.2e+0.

Gated by `extra/airline_history-outlier-{reg,pre,auto-keep,remove}`.
