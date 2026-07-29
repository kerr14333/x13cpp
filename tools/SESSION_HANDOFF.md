# Session handoff — 2026-07-28f (pickmdl + the composite diagnostics CLOSED)

Replaces the 2026-07-28d handoff. Its findings are carried forward below where
they still matter; its open item 1 (`pickmdl{}`) is done, and so is the
`Iagr==4` indirect-names item that followed it.

## Where things stand

**Tree clean on `checkpoint/m5-seats-slidingspans`.** Nothing uncommitted, no
background work outstanding. `git log --oneline -8` for the current head — this
file is written *before* the commit that contains it, so any SHA named here is
necessarily one behind. (It has gone stale that way twice; hence no SHA.)

| check | result |
|---|---|
| `python -m pytest tests/parity -q -n 8` | **5634 passed / 0 failed / 469 skipped** (~95s) |
| `cd build && ctest` | 11/11 |
| `Rscript bindings/r/test_x13c.R` | 165/165 (not re-run; untouched surface) |

**Run the suite with `-n 8`** (pytest-xdist, installed). 262s serial → ~97s,
same counts. Safe because every gate compares stdout from a read-only
subprocess and no harness writes side files — re-verify that if a harness ever
changes. Build is ~25s; use `-k "<name>"` (~3s) while iterating.

Standing constraints, unchanged: **never merge this branch to `main`, never
push**; **never run `tests/corpus/generated/genspecs.py` or
`tests/corpus/extra/genextra.py`** (both wipe committed specs they cannot
regenerate — the six new pickmdl specs are hand-authored and say so in a header
comment). Build with
`export PATH="/c/rtools44/x86_64-w64-mingw32.static.posix/bin:$PATH" && cmake --build build -j 6`.

## What landed (6 commits, 5560 → 5634 passing)

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

## Findings worth not re-deriving

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

## The composite DIAGNOSTICS front, also closed

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

## Open, in the order I would take them

1. **`pickmdl{}`'s three remaining walls**, each measured against the oracle and
   each a clean fatal rather than a silent wrong answer:
   - `outofsample=yes` (`Outfer`) — needs amdfct's out-of-sample arm
     (`amdfct.f:70-90`, `:186-235`, `:270-300`), which re-fits the model three
     times and saves/restores the whole estimation state. **Measured: the
     oracle picks a DIFFERENT model there** ((0 1 1)(0 1 1) vs the default
     run's (0 1 2)(0 1 1)), so this is a real gap, not a formality. It would
     also close `estimate{outofsample=}`, walled for the same reason.
   - `bcstlim=` / `forecast{maxback=}` — the backcast acceptance pass
     (`automx.f:903-928`) needs amdfct's `Bckcst` arm. Oracle runs it fine.
   - per-candidate AIC-regressor testing (`automx.f:404-500`, `:750-870`) and
     the Picktd trading-day restore (`:255-292`, `:700-725`) it drags in.
     Reachable with `regression{aictest=}` alongside `pickmdl{}`.
2. **Two automdl BASELINE disagreements**, both isolated, neither gated:
   `generated/usdeaths_automdl` (17 keys; the known iddiff d=0/d=1 split, and
   `nreg` 1 vs 0 — the engine is missing the Constant) and `ces_amuse`
   (75 keys, model 5 vs 4 ARMA terms; localised to label 40's `amidot` arm,
   since `noautooutlier=tramo` makes it bit-exact). Both make every option
   verdict on those series uninterpretable.
3. **`gtdpvc` parses decimal literals 1 ulp off the nearest double**: `"0.95"`
   → `0.95000000000000007` vs the correctly-rounded `0.94999999999999996`.
   Latent everywhere a spec supplies a decimal; the one place it was visibly
   biting is gone. **Check whether the Fortran reader does the same before
   changing anything** — if it does, the port is faithful and this is
   documentation, not a fix.
4. **Composite `agr3s.f`** — the SEATS branch of composite adjustment, plus
   pseudo-additive and the forced/rounded indirect series. The X-11 composite
   front (direct, indirect, comparison statistics, indirect diagnostics) is now
   fully closed, so this is what is left of `composite{}`.
5. **A composite whose components carry a residual peak**, to gate savpk's real
   `.dir`/`.ind` split — see above; only the degenerate branch runs today, and
   a mutation swapping the two output halves passes the whole suite.
6. `spectrum{altfreq=yes}` pending CB-30; `history{outlier=auto}` /
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
