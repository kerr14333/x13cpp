# Session handoff — 2026-07-29b (composite under SEATS + all of `amdfct.f` — CLOSED)

Replaces the 2026-07-29 handoff. Its findings are carried forward below where
they still matter; its open item 4 (composite `agr3s.f`) is done, and with it
the last structural piece of `composite{}` bar pseudo-additive.

## Where things stand

**Tree clean on `checkpoint/m5-seats-slidingspans`.** Nothing uncommitted, no
background work outstanding. `git log --oneline -8` for the current head — this
file is written *before* the commit that contains it, so any SHA named here is
necessarily one behind. (It has gone stale that way twice; hence no SHA.)

| check | result |
|---|---|
| `python -m pytest tests/parity -q -n 8` | **5754 passed / 0 failed / 469 skipped** (~85s) |
| `cd build && ctest` | 11/11 |
| `Rscript bindings/r/test_x13c.R` | 165/165 (not re-run; untouched surface) |

**Run the suite with `-n 8`** (pytest-xdist, installed). 262s serial → ~97s,
same counts. Safe because every gate compares stdout from a read-only
subprocess and no harness writes side files — re-verify that if a harness ever
changes. Build is ~25s; use `-k "<name>"` (~3s) while iterating.

Standing constraints, unchanged: **never merge this branch to `main`, never
push**; **never run `tests/corpus/generated/genspecs.py` or
`tests/corpus/extra/genextra.py`** (both wipe committed specs they cannot
regenerate — the six pickmdl specs, both `composite-seats*` corpora and the
three `*outofsample*` and three `*-backcast*` specs are hand-authored and say
so in a header comment). Build with
`export PATH="/c/rtools44/x86_64-w64-mingw32.static.posix/bin:$PATH" && cmake --build build -j 6`.

## This session: `composite{}` under `seats{}` — `agr3s.f`

A SEATS metafile came back **FATAL**. `X11agr` is a metafile-wide flag, not a
spec option: `aaamain.f:73` arms it TRUE once for the whole run, `gtinpt.f:594`
re-arms it on the first component, and `gtinpt.f:1170` ANDs each COMPONENT's own
`Lx11` into it — so ONE SEATS component routes the total's indirect adjustment
through **`agr3s`** (`x11ari.f:342`) instead of `agr3`. None of that existed:
`agr2_component` accumulated `Stci` unconditionally (a SEATS component has none),
`agr2_compare` hardcoded `const bool x11agr = true`, and `x13run_composite`
handed every spec to `run_x11`, which refuses a `seats{}` spec outright.

**`agr3s` is a different answer, not a variant of `agr3`.** No extreme-value
pass, no Henderson, no D8/D9 battery, no `x11pt4` behind it: the indirect SA
series IS the aggregate `Ci` of the components' own SA series, and the seasonal
factor is `O5/Ci`. Measured on the oracle, the entire output is
`isf isa ie5 ip5 ie6 ip6 i18` even when the spec asks for the full indirect
family. The gate asserts the ABSENCES, in both directions.

Landed: `core/src/composite/agr3s.{hpp,cpp}` (including the forced and rounded
indirect series, which agr3 still lacks), the `X11agr` derivation and its carry
across the metafile, `agr2`'s `!X11agr` branches, and a refactor —
`x11ari.f:329-374` is now `core/src/driver/composite_tail.cpp`, called from BOTH
drivers, because the oracle has one `x11ari` and that block sits after its
Lseats/Lx11 branch rejoins.

### Findings worth not re-deriving

1. **`spcdrv`'s `Iagr==4` branch comes BEFORE its Lseats arm** (`:302-313`). The
   indirect spectrum is taken from `Stci` — the series agr3s left — and NEVER
   from `Seatsa`, even when the TOTAL is SEATS-adjusted. The port keyed on
   `has_seats` and so reported the total's own DIRECT spectrum under the
   indirect names: `spcindsa.median` came back equal to `spcsa.median` to the
   last digit.
2. **`spcdrv.f:436`'s `goirr = goirr .and. X11agr`** — there is no `spcindirr`
   block at all on this path, because agr3s forms no indirect irregular. The
   engine was emitting twenty keys the oracle does not, and **the gate could not
   see it**: the diag comparison iterated over golden keys only. Made
   bidirectional. This is the third time a one-directional comparison has hidden
   a real defect in this port.
3. **A gate that reads N columns cannot see a wrong N.** Mutation B (force
   `x11agr` true inside `agr2_compare`) PASSED at first, because the harness
   emitted only `cmpstat 1..12` on this path. It now emits all 24 and the gate
   asserts 13..24 are exactly zero — the R2 half must be *untouched*, not merely
   unprinted.
4. **CB-32**: `agr3s` omits `agr3.f:101-108`'s store of the DIRECT seasonally
   adjusted series into the `Orig2`-aliased scratch, so `agr2`'s DIRECT
   roughness column measures the aggregate **ORIGINAL**. Confirmed numerically:
   66.777 here against 53.586 down the agr3 path, and 66.777 is R1 of the summed
   input `.dat` files to all six printed digits.
5. **Fixed coefficients are the right corpus answer for a SUM.** With the
   components' MA estimated, this port's SEATS decomposition sits **1.3e-6** from
   the oracle's on this synthetic series while every printed coefficient agrees
   to 11 digits — and the same 1.3e-6 appears on a STANDALONE component run,
   i.e. optimizer path noise, not aggregation. The indirect adjustment is a sum,
   so it would land there undiluted. Fixed, the whole metafile is ~5e-15.

### The one gap, measured and made loud

`Ci` past the observed span needs `Setfsa` — the SEATS FORECAST decomposition
(`ansub3.f:356-678`) that `seatad.f:49-54` appends into `Seatsa`. Unported, and
**not avoidable by configuration**: `editor.f:387-400` forces
`Nfcst >= max(12, 3*Sp)` on any SEATS run regardless of `forecast{maxlead=}`.
It costs i18's forecast rows (`agr3s.f:412-418` widens that punch range
unconditionally, unlike isf's `Savfct` gate) and, when the TOTAL also carries
forecasts so `Series` extends where `Ci` does not, a spurious `ita` table. The
run writes a NOTE to Mt2 rather than doing it silently; the gate compares the
observed span and says why at the skip. Everything else is bit-exact.

### Gated by

`census-examples/composite-seats/` (X-11 total — agr3s's `Lx11` true) and
`composite-seats-total/` (SEATS total — the only corpus case reaching the
composite tail from `run_seats`), through
`tests/parity/test_composite_seats.py` (50 tests). Mutation-tested three ways,
each failing a different set.

## This session, part 2: `outofsample=` — amdfct.f's out-of-sample arm

Two options walled in two different places (`estimate{outofsample=}` fataled in
`run_pre_model`, `pickmdl{outofsample=}` was rejected by the parser) because the
computation behind both did not exist. Now ported and gated; only amdfct's
BACKCAST arm is left.

**What the arm does.** For each of the last three years it pulls the model span
END back another year, **RE-ESTIMATES**, and forecasts one year from the new
span's end — so each forecast is made by a model that has never seen the period
it is forecasting, where within-sample forecasts from a past origin of the model
fitted to everything. The span shrinks cumulatively across the three passes.

**It changes the ANSWER, not just a label.** The aape is the first of pickmdl's
three acceptance screens: `extra/airline_pickmdl` selects `(0 1 2)(0 1 1)` by
default and `(0 1 1)(0 1 1)` with the flag on (`nmodel` 3 → 2). The scouting
doc's claim was right, and this confirms it engine-side.

### Findings worth not re-deriving

1. **`Nfev`/`Niter` are deliberately NOT restored.** amdfct puts back eight
   pieces of estimation state, the ssprep snapshot and `Endmdl` — but its final
   `rgarma` (`amdfct.f:299`) is COMMENTED OUT, so the `.udg` reports the LAST
   re-fit's optimizer counters. Measured: `nfev` 19 → 13, `niter` 6 → 4, on a
   run where **every other `.udg` key is byte-identical**. Reproduced by not
   restoring them; `generated/airline_outofsample` pins it.
2. **The outlier strip is the non-mechanical half.** Before each re-estimation
   every OUTLIER regressor dated INSIDE the three-year window is deleted from
   the design (`dlrgef`, backwards, because dlrgef renumbers) and its fitted
   contribution subtracted out of the series. It has to be: the shortened span
   no longer contains those dates, so the column would be identically zero. A
   RAMP is judged by its END date, everything else by its start.
3. **And then the `ave` scale block reads the STRIPPED series**, not the
   caller's `Trnsrs`. Easy to miss, because on the within-sample path the two
   are the same buffer.
4. On the automatic path `Lauto` is in/out: a failed re-estimation clears it,
   the candidate is dropped, and the Fortran returns THERE — before the restore
   block (`amdfct.f:227`). Transcribed.

### Gated by

`generated/airline_outofsample` (the `estimate{}` twin + the `nfev`/`niter`
pin), `generated/airline_outofsample-otl` (the strip, with an out-of-window
outlier as the negative control) and `extra/airline_pickmdl-outofsample`.
Mutation-tested four ways, each failing a different set: force within-sample
(11), restore the counters (1), never strip (1), strip everything (2).

**A corpus judgment worth knowing:** `-otl` carries no `x11{}` on purpose. Its
three outlier regressors put the D9A ratios ~4e-10 off the golden — measured
IDENTICAL on the within-sample twin, so it is the ordinary outlier-estimation
floor and a property of the spec, not of this feature. `test_d8b_d9a` is
byte-exact by design, so the spec simply does not carry the adjustment that
would drag it in; `airline_outofsample` covers the X-11 side.

## This session, part 3: `forecast{maxback=}` — amdfct's BACKCAST arm

The last unported piece of `amdfct.f`, and the last thing between here and a
complete `pickmdl{}`. `pickmdl{}` + `forecast{maxback=}` used to FATAL, on a
spec the oracle runs to completion.

`Bckcst` is the out-of-sample arm mirrored in every direction: the Xy design is
time REVERSED so the same forward machinery extrapolates backwards, the outlier
window is the FIRST three years (every type judged by its start — no ramp
special case), the `ave` scale reads the first three years, and the
out-of-sample variant walks `Begmdl` FORWARD instead of `Endmdl` back, taking
the ACTUALs from the year it is about to drop, in REVERSE order, because
`amdfct.f:239` skips `subset` on exactly that path.

### Findings worth not re-deriving

1. **CB-33 — `bcstlim=` cannot reject anything.** `automx.f:922`'s
   `IF(mape(4).gt.Bcklim.and.(.not.argok))` is `.and.` where the algorithm wants
   `.or.`: a model that CONVERGED passes the screen however bad its backward
   extrapolation is, and one that did not has been dealt with upstream. The
   program contradicts itself out loud — `prtamd` evaluates the screens itself
   and prints "MODEL 2 REJECTED: Average backcast error > 1.00%", after which
   the `ELSE` arm prints "The model chosen is (0 1 2)(0 1 1)" and the footer
   still reads "Includes 12 backcasts". Transcribed with `&&`.
2. **The gate had to read PRINTED output.** This is the one amdfct result with
   no savelog key at all: `prtamd` prints the four numbers and the `.udg`'s only
   trace of the block is whether `Nbcst` survived. `test_backcast_aape.py`
   parses the golden `.out`'s own table and compares the harness's new
   `bcstaape.*` lines at prtamd's two decimals — the same shape the composite
   gate uses for the roughness table.
3. **`airline_zero` earns its place twice.** `-backcast-zero` is the only spec
   in the corpus that reaches amdfct's `ivalue==1` absolute-error scale, so it
   is the only gate on the `ave`-seeded-to-ONE Census defect, forward or
   backward. And because that series is negative in its first three years and
   positive in its last, it is also what discriminates WHICH window the scale
   comes from: a mutation reading the forecast window on the backcast path
   **passed every other spec in the suite**.
4. **A stale build reads exactly like a bug.** A probe came back `Inf` where the
   oracle had 43.15; the cause was that a reverted source file had not been
   rebuilt. The environment notes already say "a reverted file may not rebuild"
   — the new part is that the symptom can be a plausible-looking numeric result,
   not a compile error.

### The one gap, walled with its measurement

Out-of-sample BACKCASTS with an outlier regressor inside the first three years
reads 6.6959 against the oracle's printed 6.71. The strip fires (instrumented:
`typ=1 beg=15 inwin=1`) and disabling it changes nothing, so the difference is
downstream of the window test — in how the stripped series feeds the reversed
per-pass design. Everything else is bit-exact and gated: within-sample backcasts
with outliers, out-of-sample FORWARD with outliers, out-of-sample backcasts with
no outlier in the window, and the `ivalue==1` branch. It is a `fatal()`, so it
appears in `docs/WALLS.md`.

### Gated by

`extra/airline_pickmdl-backcast` (within-sample), `-oos` (BOTH amdfct arms at
once, the only place they interact) and `-zero`, through
`tests/parity/test_backcast_aape.py`. Mutation-tested three ways: skip the
design reversal (11), read the forecast `ave` window (1, and only on `-zero`),
use the forecast outlier-window test (0 — which is what found the gap above).

## Previous session (2026-07-29a): `pickmdl{}` — what landed

`M5/pickmdl: port automx.f -- the classic X-11-ARIMA candidate search`.

**`pickmdl{}` was parse-only and silently model-less.** `gt_pickmdl` routed all
11 arguments through `gt_generic`, so a pickmdl spec came back `OUTCOME: OK`
having fitted **no ARIMA model at all** — `nmodel: 0`, `nefobs: 144` against
the oracle's `(0 1 2)(0 1 1)` and 131. It was the largest single source of real
feature skips (9), all now gone.

`automx.f` is the sibling of the `automd.f` front closed last session and a
completely different algorithm: it ESTIMATES a fixed candidate list (from
`file=`, else five built-ins) and keeps the best one passing three screens —
amdfct's three-year average forecast error vs `fcstlim`, the Ljung-Box p-value
at lag 24 vs `qlim`, and the nonseasonal MA coefficient sum vs `overdiff`.
`method=first` stops at the first acceptance; `method=best` tightens the bar to
each accepted model's own error. A candidate with a trailing `*` is the
DEFAULT, used anyway (forecasting off, `nofcst`) if nothing is accepted and the
run still needs regARIMA preadjustment factors.

Ported: `core/src/automdl/automx.{hpp,cpp}` — the candidate loop plus the four
routines it is the only caller of (`mdlinp.f`, `setamx.f`, `bstmdl.f`,
`bstget.f`) and `nofcst.f`; a real `gt_pickmdl` with gtautx.f's validation; and
gtinpt.f:248-257's defaults.

**Everything it needed was already ported** — `rgarma`, `regvar`, `mdlset`/
`mdlint`, `getmdl`, `ssprep`, `idotlr`, `acf` (from `check{}`), and `amdfct`
(from the aape diagnostic — `AapeDiagnostics.ok` IS the Fortran's `Fctok`).
That is why the whole front came in at one increment.

### Findings worth not re-deriving (pickmdl)

1. **The bug that was not mechanical: `bstget.f:80-96`'s effective-observation
   split belongs INSIDE `bstget`, not at the call site.** Extracted to a shared
   helper and called only from the loop head, the re-estimation of a winner
   that was not the LAST candidate estimated **crashed** — `Nintvl` still
   described the previous candidate, so `rgarma` read past the differenced
   series whenever the winner had a different differencing order. Everything
   else in the port was transcription.
2. **A probe that injects text by string-replace has to know where the block
   is.** The generated spec's own comment header says `pickmdl{}`, so
   `txt.replace("pickmdl{", ...)` put the argument in a COMMENT and the oracle
   rejected the file — **every variant read "ORACLE REJECTS", which looks like
   a finding and is a probe bug**. The sibling trap fired twice more the same
   afternoon: an unanchored `file =` match also strips `series{file=}`. Rule of
   thumb from the spec_sweep lesson applies unchanged — *a uniform result
   across every level of every factor means suspect the harness.*
3. **Measure engine-vs-oracle, not just oracle on-vs-off.** Done here for all
   11 arguments (`tools/pickmdl_scouting.md`). 8 move the oracle by 150-190
   `.udg` keys and the engine matches every one at 0 differing;
   **`mode=` is provably INERT** — `iautom` is a `gtinpt.f:873` LOCAL whose only
   use is the `> 0` test setting `Lautox`, and `gtautx.f:230` forces it to 1
   regardless.
4. **Mutation-test per HALF** (the 2026-07-28d lesson, applied): four mutations,
   four DIFFERENT failure sets — not tightening `loclim` (9/10 on the base
   spec), skipping `bstget` (9/10, disjoint), dropping the `ovrdff` screen
   (27/70), disabling the starred fallback (18/70). A single whole-routine
   mutation would have reported "covered" for all of it.
5. `gtnmvc` bounds a value by the **DESTINATION string's length**, not by its
   `maxchr` argument — an empty `std::string` rejects every filename as
   "longer than 0 characters". Pre-size the buffer (see `series.cpp:102`).
6. Carried forward from 2026-07-28d: an argument missing from the probe list
   reads exactly like one that measured INERT; a probe VALUE has a direction,
   not just a distance; a null measured under the wrong preconditions is not a
   null; `automd.f:343`'s `armats` is guarded on `.not.Lidotl`; a reverted file
   may not rebuild (`touch` after any revert-by-copy).

## Previous session: the composite DIAGNOSTICS front

Two increments after pickmdl, and the FIRST was pure harness coverage.

1. **`x13run_composite` had never emitted the QS / spectrum-peak / NP blocks at
   all.** x11ari.f reaches genqs (:277), spcdrv (:282) and gennpsa (:322) on
   EVERY spec of a metafile. Nothing in the engine was wrong -- `run_x11`
   already calls all three ahead of its composite tail, and **all 103 shared
   keys matched the moment the emit was wired in**. `dump_diag.hpp`'s helpers
   gained an optional key prefix + string sink (default arguments; the other two
   harnesses are untouched).
2. **The INDIRECT (`Iagr==4`) pass, 52 further keys.** x11ari.f:344-370 runs
   spcdrv and gennpsa a second time after `agr3` installs the aggregated
   adjustment. `run_spectrum`/`gennpsa` take an `iagr4` parameter rather than
   being duplicated. **155 of 155 shared keys now match, zero golden-only and
   zero engine-only.**

Two things not to re-derive:

- **The peak-label lists are NOT independent between the passes.** spcdrv
  appends to ONE accumulated string and savpk.f:88-115 splits it at `Nspdir`,
  the count x11ari.f:290 recorded between the calls.
- **`svtukp.f` sets `iLb=7` on the indirect tables and 4 on the direct ones**
  (:56, :66), skipping the `ind` as well as the `spc` -- so the KEY is
  `spcindsa`/`spcindirr` while the LABEL in `peaks.tukey.*.ind` is plain
  `sa`/`irr`. Last mismatch standing after everything else lined up.

**CB-31** claimed: `x11ari.f:346` hands `genqs` `LSLIQS` (=69, a SAVELOG index)
where `genqs.f:439` uses it as a TABLE-log subscript; the direct call passes
`LSPCQS` (=113) correctly and the evidently-intended `LSPQSI` (=114) is passed
nowhere. The oracle therefore emits **no `qsind*` key at all**. Reproduced by
not making the call, and pinned from both sides by
`test_composite_no_indirect_qs`.

**A measured coverage gap, recorded rather than hidden:** savpk's real
`.dir`/`.ind` peak SPLIT is unexercised. This composite finds no visually
significant peak in any table, so all four keys are `none` and only the
degenerate branch runs -- **a mutation swapping the two output halves passes the
whole suite**. Gating it needs a composite whose components carry a residual
seasonal or trading-day peak.

## The docs are now self-checking -- READ THIS BEFORE WRITING A NUMBER

A staleness audit at the end of this session found the deliverable report
quoting a parity count **five fronts out of date** ("1088 passed" vs 5,634), a
coverage ledger understating itself by **240 routines**, and a scouting doc
contradicting its own later section. All three were true when written. Three
generated artifacts now exist so it cannot recur:

| artifact | command | owns |
|---|---|---|
| `docs/METRICS.md` | `tools/metrics.py --write` | every countable claim |
| `docs/WALLS.md` | `tools/walls.py --write` | what the engine refuses (19 gaps / 4 faithful) |
| `tools/ported.yaml` | `tools/coverage_map.py --audit --promote` | which .f files are ported |

**Never type a count into prose.** Wrap it in a marker --
`<!--x13:parity_pass-->5754<!--/x13-->` -- and `--write` maintains it while
`--check` fails on drift. `docs/PROJECT_SUMMARY.md` is fully marked up.

**When they run** (`CLAUDE.md` has the table): every `build.ps1` runs the two
fast checks as WARNINGS; **session close runs
`metrics.py --write --parity <pass>,<fail>,<skip>,<xfail>` and
`walls.py --write` before the final commit** -- pass `--parity` with the suite
result you already have, or it re-runs pytest for 85s.

**Ownership rule, now binding.** This file owns *what is open*; scouting docs
own *how something works and what was measured* and must not keep status lists;
a code comment describes *its own file*. Every cross-file staleness finding was
duplicated ownership.

Two data-loss bugs were found in `coverage_map.py` by using it: `partial` was
not a recognised status and unrecognised words were silently reset (destroying
three hand-written entries and their notes), and `write_yaml` hardcoded the file
header. Both fixed; both are the same bug -- a round-trip that drops what it did
not parse.

## Open, in the order I would take them

1. **`pickmdl{}`'s ONE remaining wall** (both of its amdfct-dependent ones
   closed this session): per-candidate AIC-regressor testing
   (`automx.f:404-500`, `:750-870`) and the Picktd trading-day restore
   (`:255-292`, `:700-725`) it drags in. Reachable with `regression{aictest=}`
   alongside `pickmdl{}`. Plus the narrow amdfct corner above — out-of-sample
   backcasts with an outlier in the first three years — which is measured,
   walled and 0.2% out, not a structural gap.
2. **Two automdl BASELINE disagreements**, both isolated, neither gated:
   `generated/usdeaths_automdl` (17 keys; the known iddiff d=0/d=1 split, and
   `nreg` 1 vs 0 — the engine is missing the Constant) and `ces_amuse`
   (75 keys, model 5 vs 4 ARMA terms; localised to label 40's `amidot` arm,
   since `noautooutlier=tramo` makes it bit-exact). Both make every option
   verdict on those series uninterpretable.
3. **The SEATS FORECAST decomposition (`ansub3.f:356-678` / `Setfsa`).** Now
   wanted by two separate fronts: the composite i18 forecast tail above and the
   `hpcycle` filter (`tools/seats_hp_scouting.md`). Nothing else in
   `composite{}` needs it.
4. **`gtdpvc` parses decimal literals 1 ulp off the nearest double**: `"0.95"`
   → `0.95000000000000007` vs the correctly-rounded `0.94999999999999996`.
   Latent everywhere a spec supplies a decimal; the one place it was visibly
   biting is gone. **Check whether the Fortran reader does the same before
   changing anything** — if it does, the port is faithful and this is
   documentation, not a fix.
5. **What is left of `composite{}`**, now small: pseudo-additive (`Psuadd`) and
   the forced/rounded indirect series on the **agr3** path (`agr3.f:426-538` —
   ported for agr3s this session, still absent for agr3, and ungated on both for
   want of a `force{}` composite spec).
6. **A composite whose components carry a residual peak**, to gate savpk's real
   `.dir`/`.ind` split — only the degenerate branch runs today, and a mutation
   swapping the two output halves passes the whole suite.
7. `spectrum{altfreq=yes}` pending CB-30; `history{outlier=auto}` /
   `x11outlier=no` / `additivesa=`; the slidingspans `chs` per-span prior phase.

## Environment notes

Unchanged (`/codex:cancel` broken; codex notifications carry no findings;
Windows Python cannot read git-bash `/tmp` or `/d/` mounts; no PowerShell
here-strings in the Bash tool; oracle flag order is
`x13as_ascii_O2.exe <specbase> -s`; ad hoc runs are
`build/x13run_{m3,x11,seats}.exe <spec>.spc` from the spec's own directory;
`option_sweep.py` stages every `tests/corpus/data/*.dat` and abspaths
`--outdir`; the Bash tool's cwd persists across calls and is NOT shared with
the PowerShell tool).

New this session: **`run_parity.py --update` honours only the LAST `--filter`**,
so bless one spec per invocation. Adding a `core/src/**/*.cpp` still needs the
build run twice — the first prints `GLOB mismatch!` and stops.

## Methodology, reconfirmed

Every increment ends with a **mutation test**, per HALF, and on auto-discovering
gates it is the only evidence the new specs are compared at all. The
oracle-on-vs-off table proves a flag matters; only engine-vs-oracle says whether
the port honours it. Both were run here for all 11 arguments before anything was
declared closed.
