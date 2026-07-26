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
| `fixx11reg=yes` | `Revfxx` | **MOVES** sar 3.8e+0, sae 8.3e-3 | `x11regression{}` |
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

### `fixx11reg=` led to a BIGGER bug: the whole `x11regression{} + history{}` family is wrong

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
2. **Model-free, BOTH branches are wrong** (7.9e-1 / 1.4e-2 default, 1.2e+0 /
   1.5e-2 fixed). Here `Ixreg` is never promoted, so the demote is irrelevant and
   x11pt2's inline `x11mdl_td` IS running per span — it just does not reproduce
   either oracle branch. That is a second, independent defect in the same family.

**Do not "fix" this by adding the demote.** Tried, measured: it makes the modelled
family worse (sar 8.2e-1 → 1.2e+0 on the default, and it breaks the previously
floor-accurate fixed branch too). In the oracle `Ixreg==2` means *run the
transparent `xrgdrv` pass*, and this port HOISTS `xrgdrv` into `run_pre_model`
(see `core/src/x11/xrgdrv.hpp`) instead of reaching it from x11pt2, so a demoted
span takes the inline `x11mdl_td` route — the `Ixreg==1` semantic — rather than
the transparent pass. Closing it needs `xrgdrv` per span, against that span's
pointers, plus the span's pre-model divide by that span's `Faccal`, plus a check
that xrgdrv's existing `Lterm`/`Ksdev` save-restore survives nesting inside a
span replay. That is its own increment, and it subsumes `fixx11reg=` and
`x11outlier=` (both live in the same `revdrv.f:309-350` `Ixreg` block).

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
3. `fixx11reg=` + `x11outlier=` — share the `revdrv.f:308-347` `Ixreg` block, and
   **both are blocked on a per-span `xrgdrv`**, which is the real work: the whole
   `x11regression{} + history{}` family is currently wrong in its DEFAULT
   configuration, so the flags cannot be gated until that lands. See the section
   above. Promote this to the front if `x11regression{}` matters to a caller,
   since it is a silent wrong-numbers bug rather than a missing option.
4. `sadjlags=`/`trendlags=`/`target=` — the alternate revision targets; the
   biggest OUTPUT gap (whole columns absent) and `target=` rides on it.
5. `outlier=`/`outlierwin=` — needs `rmatot.f` + `rmotrv.f`; the default is
   already right, so this is the least urgent of the movers.
6. `additivesa=` — additive mode only, and the additive branches of
   `putrev`/`getrev` are out of scope for the current history port generally.

`refresh=` — do not port; record the mechanism instead.
