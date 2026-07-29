# composite{} / indirect adjustment — port scouting

Target: the last open CES parity blocker (14 of the 143 BLS AE specs use
`COMPOSITE{}`). Unlike every feature ported so far, composite adjustment is a
**multi-spec, driver-level** feature: it does not fit inside one spec run.

## How the oracle does it

`x13as -m <metafile>` (`aaamain.f:114-166`). The metafile lists spec basenames in
processing order; the LAST one must carry the `composite{}` spec. All specs run in
one process, so the aggregation COMMONs (`/mq11/` = `agr.cmn`, `/agreg/` =
`agrsrs.cmn`) persist across them — that persistence IS the feature.

Flow per metafile:

1. **Component specs** (`series{ comptype=add|sub|mult|div, compwt= }`).
   `gtinpt.f` sets `Iagr=1` on the first component (`lagr` from `comptype`);
   `editor.f:2245-2255` promotes `Iagr 1->2` and stamps `Itest(1..5)` =
   (Sp, BegMo, EndMo, BegYr, EndYr) — the span every later component must match
   (`editor.f:2258-2268` errors with `Iagr=-1` on a mismatch). Each component is
   seasonally adjusted normally, then `x11ari.f:373` calls **`agr2`**, which
   accumulates the component into the `/agreg/` buffers via **`agr`** (`agr.f`:
   `B(j) = B(j) +|-|*|/ (A(i)*Wt)`, `Iag` 0..3, `Wt=0 -> 1`):

   | buffer | source (agr2.f:267-281) | meaning |
   |--------|-------------------------|---------|
   | `O`    | `Orig2`                 | direct original (the composite total) |
   | `O1`   | `Stoap`                 | prior-adjusted original |
   | `O2`   | `Orig2` less Fin*/prior | indirect "SA-comparable" original |
   | `O3`   | `Orig2` less LS         | LS-removed original |
   | `O4`   | `Orig2` less AO/TC      | AO/TC-removed original |
   | `O5`   | `O2` / `Faccal`         | calendar-adjusted original |
   | `Omod` | `Stome` (X-11 only)     | modified original |
   | `Ci`   | `Stci` (or `Seatsa`)    | **indirect SA** = sum of component SAs |
   | `Ci2`  | `Stci2` (or `Setsa2`)   | forced SA |

   `agr2` also bumps `Ncomp` and records `Cmptyp/Cmpwt/Cmpfil` for the
   "how the aggregate was formed" table.

2. **The composite spec** (`total.spc`: `composite{}` INSTEAD of `series{}`).
   `getcmp.f` parses it (13 args, ARGDIC below), derives the span from
   `Itest(1..5)`, then `getcmp.f:241` calls **`agr1`** with `Iagr>0`, which sets
   `Iagr=3` and copies `Y(i) = O(i+Ind1ob-1)` — i.e. **the composite total becomes
   the series**. From there the run is an ordinary single-series adjustment
   (DIRECT adjustment of the aggregate).

3. **Indirect output.** Still inside that last run, `x11ari.f:329-373` sees
   `Iagr==3` and calls **`agr3`** (X-11) / **`agr3s`** (SEATS) to emit the INDIRECT
   tables from `Ci`/`O2`/`Omod`, then `x11pt4` again for the indirect E-tables,
   then `agr2` with `Iagr==4` for the direct-vs-indirect comparison statistics
   (`aggmea`, `cmpchi.f`'s F-tests).

`agr1` is also the initializer: called with `Iagr==0` it zeroes `O..Ci`, `Ncomp`,
the sliding-spans (`Saind/Sfind/Sfinda`) and history (`Cncisa/Finisa`) indirect
buffers.

## Fortran inventory

| file | lines | role | increment |
|------|-------|------|-----------|
| `agr.f`     | 25  | the accumulate primitive (+ - * /) | 1 |
| `agr1.f`    | 82  | init / hand `O` to the composite spec | 1 |
| `getcmp.f`  | 249 | `composite{}` parse + span from `Itest` | 1 |
| `agr2.f`    | 318 | per-component accumulation; `Iagr==4` comparison stats | 1 (O only) / 2 (rest) / 3 (stats) |
| `agr3.f`    | 618 | indirect X-11 tables | 2 |
| `agr3s.f`   | 481 | indirect SEATS tables | 2 |
| `agrxpt.f`  | 60  | indirect pointer setup | 2 |
| `aggmea.f`  | 47  | R1/R2 measures of roughness | 3 |
| `prtagr.f`  | 38  | "how the aggregate was formed" table | 3 (print) |
| `pragr2.f`  | 46  | indirect E-table print wrapper | 3 (print) |

(`cmpchi.f` and `cmpstr.f` were listed here in the first draft on the strength of
their names. Neither is composite: `cmpchi` is the regression-GROUP chi-square
test called from `prtmdl.f:709`/`prtxrg.f:552`, and `cmpstr` is the lexer's
token-aware string compare. Nothing in the composite path calls either.)

`composite{}` ARGDIC (`getcmp.f:60-64`, PARG=13): `name title print save decimals
modelspan saveprecision savelog yr2000 indoutlier appendfcst appendbcst type`.
Only `decimals/modelspan/saveprecision/yr2000/indoutlier/appendfcst/appendbcst/
type` carry state; `appendfcst/appendbcst` are the same `Savfct/Savbct` globals
just closed for `x11{}` (commit 13d03c2).

## What already exists in the port

- `agr_cmn.hpp` / `agrsrs_cmn.hpp` are generated and live on `X13Context`.
- `series{ comptype/compwt }` already parse into `ctx.agr.iag` / `ctx.agr.w`, and
  set `lagr` (`series.cpp:209-222`).
- `gtinpt.cpp:278-293` routes `composite{}` and reproduces the oracle's
  "No component series were specified for composite adjustment" error for a
  standalone composite spec. (The oracle then **SIGFPEs** — see
  `tests/golden/census-examples/composite/total/total.stderr.txt`. Candidate
  `CB-N` entry; the C++ exits cleanly, which is a deliberate divergence on an
  already-fatal path.)
- `gt_composite` is `gt_generic` — args consumed, nothing captured.
- Everything downstream (`x11ptr`, `orisrs.orig2`, `x11srs.stci`, `adxser.stci2`)
  is already ported and populated, so `agr2`'s inputs all exist.

## Increment plan (each ends bit-exact + gated)

**inc1 — direct composite.** Metafile driver + `agr.f` + `agr1.f` + `getcmp.f` +
the `O` accumulation of `agr2`. Deliverable: `total` adjusted as an ordinary
series whose data is the component sum. Gates b1/d10-d13 of `total` against a
metafile-blessed golden. This alone unblocks the CES composite specs that only
want the direct aggregate.

**inc2 — indirect adjustment.** The remaining `agr2` buffers + `agr3`/`agrxpt`.
Gates the indirect seasonal/SA/trend tables. (`agr3s` — the SEATS branch — is a
separate front; see its own section below.)

**inc3 — comparison diagnostics.** `Iagr==4` stats (`aggmea`), the direct-trend
`Tem` / `Ckhs` pair agr3 needs to feed them, and the pointer restore back onto
the direct geometry. Gate via the `.udg`/savelog canaries.

### inc3 status: LANDED (bit-exact)

`aggmea.f` + `agr2.f:66-192` are ported (`core/src/composite/agr2.cpp`
`agr2_compare`), together with the two pieces agr3 had deferred: `/kcser/ Ckhs`
(now `ctx.kcser_ckhs`, written at x11pt3.f:379 — a COMMON precisely because agr3
reads it after x11pt3 returns) and the `Tem` direct trend, a 13-term (5
quarterly) Henderson forced onto Ckhs. All 24 `di()` values reproduce the
oracle's MEASURES OF ROUGHNESS table and all four savelog canaries
(`r1mse`/`r1rmse`/`r2mse`/`r2rmse`) plus `indtrendma` match `total.udg` — gated
in `tests/parity/test_composite_tables.py` at the oracle's own printed precision
(3 decimals), which is all either output carries.

One label oddity, harmless: agr3.f:113-118 writes `indtrendma: Nterm` right after
filtering the DIRECT trend. `Ktcopt` pins Nterm to 13/5 there, so the number is
right either way and it is not worth a `CB-N`.

**What is left is NOT composite-specific.** The remaining items under the old
inc3 heading are all shared with the direct path or are pure output:

- the indirect E/F tables (`x11pt4` on the indirect side) and the `if2.*`/`im*`/
  `iq*` .udg families — `x11pt4` is unported for BOTH adjustments, so this is the
  general E/F-table gap, not a composite one;
- the indirect D8/D9 SI diagnostics (agr3.f:290-350) — same story: `ftest`/
  `kwtest`/`mstest`/`combft` are deferred no-ops on the direct side too
  (`x11parts.hpp:47`);
- `prtagr`/`pragr2` and the aggregate-composition header — print surface, which
  this port defers to the caller by design.

### The SEATS branch (`agr3s.f`) — LANDED (bit-exact over the observed span)

`X11agr` is a metafile-wide flag, not a spec option: `aaamain.f:73` arms it TRUE
once for the whole run, `gtinpt.f:594` re-arms it on the first component, and
`gtinpt.f:1170` ANDs each COMPONENT's own `Lx11` into it. One SEATS component
therefore turns it off for the total, and `x11ari.f:338-343` routes the indirect
adjustment through **`agr3s`** instead of `agr3`. Because it outlives a spec, the
metafile harness has to carry it exactly as it carries `/mq11/` and `/agreg/`.

**What agr3s actually is.** Not a variant of agr3 — a different answer. There is
no extreme-value pass, no Henderson, no D8/D9 battery and no `x11pt4` behind it:
the indirect SA series simply IS the aggregate `Ci` of the components' own SA
series, and the seasonal factor is `O5 / Ci`. Measured on the oracle, the whole
output is `isf isa ie5 ip5 ie6 ip6 i18` (plus `ita` when `pre18b` fires and the
forced/rounded family when `force{}` is on) even when the spec asks for the full
indirect list. Three consequences worth knowing:

- **`agr2` drops its whole R2 half** (`:128-131`, `:175-178`): R2 is the variance
  of the SA/trend ratio and there is no indirect trend to form it from. The
  printed header changes to "MEASURES OF ROUGHNESS R1" and the `.udg` carries
  `r1mse`/`r1rmse` with no `r2` line.
- **`spcdrv` has an `Iagr==4` branch that comes BEFORE its Lseats arm**
  (`:302-313`): the indirect spectrum is taken from `Stci` — the series agr3s
  left — and never from `Seatsa`, even when the TOTAL is SEATS-adjusted. And
  `:436`'s `goirr = goirr .and. X11agr` means there is **no `spcindirr` block at
  all** on this path (the composite-seats golden carries zero such keys where
  composite-fixed's carries twenty).
- **`agr3s` omits agr3's `tempo`/`Orig2` store** — see **CB-32**. The DIRECT
  column of the comparison statistics is the roughness of the aggregate
  ORIGINAL.

**Ported asymmetries against agr3, all transcribed:** no `Lindot` guard on the
LS/AO factor build (`agr3s.f:149-153`); the divide that would take the level
shift back out of `Stci` is commented out at `:151`; `ststd` is computed at
`:181` and then never read, so a spec asking for `ita`/`iaf` on this path gets
nothing; and the residual-seasonality test on the ROUNDED series is gated on
`Lx11` (`:327`) where agr3 has no such guard.

**The one gap, measured.** Everything over the OBSERVED span is bit-exact
(~5e-15). Past it is not: `Ci` is the aggregate of the components' `Seatsa`, and
`seatad.f:49-54` appends `Setfsa` — the SEATS FORECAST decomposition
(`ansub3.f:356-678`) — into `Seatsa` over `[Posfob+1, Posffc]`. That is unported,
so `Ci` is zero there. It matters because `agr3s.f:412-418` widens i18's punch
range to `Posffc` **unconditionally** (unlike isf, which is gated on `Savfct`),
and the forecast span is not avoidable by configuration: `editor.f:387-400`
forces `Nfcst >= max(12, 3*Sp)` on any SEATS run regardless of
`forecast{maxlead=}`. Costs: i18's forecast rows (measured 1.44 relative), and —
when the TOTAL also carries forecasts, so `Series` extends where `Ci` does not —
a spurious `ita` table, since a nonzero original over a zero SA is exactly
`pre18b`'s trigger. The run says so on Mt2 rather than doing it silently. Gated
around in `tests/parity/test_composite_seats.py`.

Two corpora: `census-examples/composite-seats/` (X-11 total, agr3s's `Lx11`
true) and `composite-seats-total/` (SEATS total, `Lx11` false — the only case
where the composite tail is reached from `run_seats`). Both pin the components'
MA coefficients: with them estimated this port's SEATS decomposition sits
**1.3e-6** from the oracle's on this synthetic series while every printed
coefficient agrees to 11 digits, and the same 1.3e-6 appears on a STANDALONE run
of the component — optimizer path noise, not aggregation. The indirect
adjustment is the sum of the component SA series, so it would land there
undiluted.

Still genuinely open for composite: pseudo-additive (`Psuadd`) and the
forced/rounded indirect series on the **agr3** path (agr3.f:426-538 — ported for
agr3s, still absent for agr3, and ungated on both for want of a `force{}`
composite spec).

## Harness / gate notes

- The parity corpus is single-spec everywhere; a metafile case needs a new
  harness (`x13run_composite`) that carries one aggregation state across N spec
  runs, plus a bless path that runs the oracle as
  `x13as_ascii_O2.exe -m <mta> -s` from the corpus directory.
- `tests/corpus/census-examples/composite/` already ships the example
  (`region_north`, `region_south`, `total`, `composite.mta`; 180 synthetic
  monthly obs, 1990.01-2004.12). Its existing golden was blessed by running each
  spec SEPARATELY, so `total`'s bundle only records the standalone error + the
  oracle's SIGFPE — it must be re-blessed in metafile mode.
- Verified 2026-07-24: `x13as_ascii_O2.exe -m composite -s` runs the example
  clean (exit 0) and `total.out` carries BOTH the direct and the indirect
  adjustment sections.


## UPDATE 2026-07-28f -- the diagnostics front is CLOSED, direct and indirect

Two increments; see `CLAUDE.md` for the full entry and the session handoff for
the traps.

**The direct half was pure harness coverage.** `x13run_composite` had never
emitted the QS / spectrum-peak / NP blocks for ANY spec of a metafile, even
though x11ari.f reaches genqs (:277), spcdrv (:282) and gennpsa (:322) on every
one of them and `run_x11` already called all three ahead of its composite tail.
All 103 shared `.udg` keys matched the moment the emit was wired in.

**The indirect half** (x11ari.f:344-370) is the same three routines run a second
time over the buffers `agr3` installs, under `Iagr==4`. `run_spectrum` and
`gennpsa` take an `iagr4` parameter rather than being duplicated; the pass skips
the ORIGINAL (spcdrv.f:158 `goori = Iagr.le.3`) and the RESIDUAL block, names its
tables via mkspky.f:22/30, and lands in `*_ind` fields. 155/155 shared keys now
match on the total, both directions.

Two structural facts worth keeping:

- **The peak-label lists are shared between the passes.** spcdrv appends to ONE
  accumulated string and savpk.f:88-115 splits it at `Nspdir`, the count
  x11ari.f:290 records between the calls. `peaks.seas` is the whole list;
  `.dir`/`.ind` are its halves.
- **`svtukp.f` uses `iLb=7` on the indirect tables and 4 on the direct ones**
  (:56, :66), so the KEY keeps `ind` (`spcindsa`) while the LABEL inside
  `peaks.tukey.*.ind` drops it (`sa`).

**CB-31**: x11ari.f:346 hands genqs `LSLIQS` (=69, a savelog index) where
genqs.f:439 uses it as a `Savtab` subscript. The oracle emits no `qsind*` key at
all. Reproduced by not making the call; pinned from both sides by
`test_composite_no_indirect_qs`.

**Measured coverage gap:** savpk's real `.dir`/`.ind` SPLIT is untested -- this
composite is peak-free, so all four keys are `none` and a mutation swapping the
two output halves passes the suite. Needs a composite whose components carry a
residual seasonal or trading-day peak.

**Still open for composite:** the SEATS branch (`agr3s.f`), pseudo-additive, and
the forced/rounded indirect series.
