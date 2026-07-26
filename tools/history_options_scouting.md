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
| `endtable=` | `Irev==2` | **MOVES** rows 71→48 *and* values 1.1e-4 | — |
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
2. `endtable=` — `Irev==2`; changes `Endtbl`, which also changes what "final"
   means, hence the value delta on top of the row-count delta.
3. `fixx11reg=` + `x11outlier=` — share the `revdrv.f:308-347` `Ixreg` block.
4. `sadjlags=`/`trendlags=`/`target=` — the alternate revision targets; the
   biggest OUTPUT gap (whole columns absent) and `target=` rides on it.
5. `outlier=`/`outlierwin=` — needs `rmatot.f` + `rmotrv.f`; the default is
   already right, so this is the least urgent of the movers.
6. `additivesa=` — additive mode only, and the additive branches of
   `putrev`/`getrev` are out of scope for the current history port generally.

`refresh=` — do not port; record the mechanism instead.
