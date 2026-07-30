# Session handoff — 2026-07-30 (`pickmdl{}` + `regression{aictest=}` — CLOSED)

Replaces the 2026-07-29b handoff. Its findings are carried forward below where
they still matter; its open item 1 (pickmdl's last wall) is done bar one
measured corner, now walled.

## Where things stand

**Tree clean on `checkpoint/m5-seats-slidingspans`.** Nothing uncommitted, no
background work outstanding. `git log --oneline -8` for the current head — this
file is written *before* the commit that contains it, so any SHA named here is
necessarily one behind. (It has gone stale that way twice; hence no SHA.)

| check | result |
|---|---|
| `python -m pytest tests/parity -q -n 8` | **5803 passed / 0 failed / 469 skipped** (~86s) |
| `cd build && ctest` | 11/11 |
| `Rscript bindings/r/test_x13c.R` | 165/165 (not re-run; untouched surface) |

**Run the suite with `-n 8`** (pytest-xdist, installed). 262s serial → ~86s,
same counts. Safe because every gate compares stdout from a read-only
subprocess and no harness writes side files — re-verify that if a harness ever
changes. Build is ~25s; use `-k "<name>"` (~3s) while iterating.

Standing constraints, unchanged: **never merge this branch to `main`, never
push**; **never run `tests/corpus/generated/genspecs.py` or
`tests/corpus/extra/genextra.py`** (both wipe committed specs they cannot
regenerate — the six pickmdl specs, the four new `*-aictest-*` ones, both
`composite-seats*` corpora and the three `*outofsample*` and three `*-backcast*`
specs are hand-authored and say so in a header comment). Build with
`export PATH="/c/rtools44/x86_64-w64-mingw32.static.posix/bin:$PATH" && cmake --build build -j 6`.

## This session: `pickmdl{}` + `regression{aictest=}` — the per-candidate AIC tests

`pickmdl{}`'s last real wall. The AIC-regressor tests **replace** the plain
`rgarma` for a freshly identified candidate — `tdaic`/`lomaic`/`easaic`
self-estimate, and `argok` becomes `.not.lester`. That is what makes the
**Picktd** restore reachable, and the two are one piece of work: `Picktd` ("a
trading-day regressor is in the model") decides whether the program's
length-of-month / leap-year prior is **in the series**, so a verdict that
differs between candidates changes the SERIES BEING MODELLED, not just the
design. `Trnsrs` and `Adj` are rebuilt (`td7var` + the Usrtad/Usrpad folds +
`trnfcn`) with `Priadj` following: 4 = program TD prior, 1 = none.

Landed: `amx_aictest` / `amx_picktd_rebuild` in `core/src/automdl/automx.cpp`,
the two reachable restores (`automx.f:317-323`, `:700-725`), the post-loop
`identify=first` block (`:750-870`), and a shared `aictest_td_vectors` /
`aictest_eas_vectors` extracted out of `aictst.cpp` so both callers build the
candidate vectors the same way.

### The three silent-wrongness bugs it found

Two of them have nothing to do with `pickmdl{}` and affect **every** aictest
path. All three returned `OUTCOME: OK`.

1. **`Pvaic`/`Rgaicd`/`Traicd` were reset by each CALLER** — both `automd` and
   the explicit-aictest path clobbered them on entry — instead of once in
   `gtinpt` (`gtinpt.f:293-300`). So the pickmdl path read the struct's
   zero-init, and **a `pvaic` of 0.0 rather than `DNOTST` turns tdaic's
   `chsppf(pvaic, df)` threshold ON**, driving `Rgaicd(PTDAIC)` negative enough
   that the FIRST TD candidate always wins its comparison against the later,
   better ones. Symptom: the engine picked the 6-coefficient TD (`nreg` 6)
   against the oracle's `td1coef` (`nreg` 1) while **every AICC value was
   bit-exact** — the arithmetic was right and the comparison was not.
2. **`regression{aicdiff=}` (`getreg.f:405-428`) was parsed and discarded.**
   `gt_regression` had no `argidx == 15` branch at all.
3. **`pvaictest=` (`:474-497`) likewise** (`argidx == 21`). Ported together;
   they are mutually exclusive and the parser now says so.

**The pre-existing `generated/cover_reg-aicdiff` could never have caught 2 or
3** — it carries `aicdiff=0.0`, which is the DEFAULT and therefore inert. A
"coverage" spec that exercises an option at its default value covers nothing.

### Findings worth not re-deriving

1. **A mutation that PASSES is the finding.** Deleting the per-candidate design
   restore left the whole suite green, because **`pickmdl{identify=}` was
   structurally inert across the entire corpus**: `lidotl` is
   `Ltstao.or.Ltstls.or.Ltsttc` (`arima.f:118`), so an `outlier{}` spec is what
   makes `identify=` do anything, and no pickmdl spec carried one. Closed by
   `extra/airline_pickmdl-aictest-otl` (the oracle finds 5 outliers, `nreg` 6,
   `nmodel` 5); the mutation then fails 10. Same class as the amdfct
   `critical=3.5` saturation from 2026-07-29 — *when a feature's effect is
   conditional on a set being non-empty, the probe must assert the set is
   non-empty.*
2. **The AIC-test candidate vectors are built ONCE, in the editor**
   (`editor.f:1151-1166`), before any model is estimated. This port has no
   editor block for them, so each caller runs the shared helper at its own
   equivalent point — `arima.f`'s explicit path just before `tdaic`, `automx`
   once before the candidate loop. A per-candidate rebuild would read a design
   the editor never saw, because **`tdaic` itself adds and deletes TD groups**.
3. **`Setpri = Pos1bk`** (`editor.f:851`, after `setxpt` at `:233`) is 0 during
   this port's model phase, because `setxpt` is never called pre-model — hence
   tdaic's `Sprior` writes stay guarded off. Checked as a suspect for the
   walled gap below and ruled out: `x11int` copies `Adj` into `Sprior` whenever
   `Nadj>0`, which this port always satisfies.
4. **`walls.py` had a blind spot of exactly the dangerous shape.** `GAP_RE`
   knew "not ported" / "unported" / "deferred" but not "not yet
   **bit-exact**" — so a wall over code that IS transcribed but does not yet
   agree with the oracle filed as a **FAITHFUL refusal**, i.e. the worst kind of
   gap classified as a non-gap. Pattern added, with a comment naming the third
   category. `docs/WALLS.md` is now 17 gaps / 4 faithful.
5. **`run_parity.py --filter` is a GLOB and will silently re-bless neighbours.**
   `--filter "*aicdiff*"` also re-blessed `generated/cover_reg-aicdiff` (visible
   only as a changed git-LFS hash). Bless one spec per invocation and check
   `git status` after.
6. **CB-34** claimed: `automx.f:264` assigns `padj2=Priadj` where its two
   identical siblings (`:330`, `:725`) assign `Priadj=padj2` — the save slot is
   overwritten with the live value instead of the mode being restored.
   Transcribed as written. Mutation-tested in the other direction: the walled
   gap below is unchanged, so CB-34 is **not** its cause.

### The one gap, walled with its measurement

**A trading-day AIC verdict that DIFFERS between candidates**
(`automx.f:259-296`'s Picktd restore). Reachable only with a
`regression{aicdiff=}` tuned between two candidates' AICC gaps, which is why the
spec written for it was **removed rather than blessed** — a golden pinning wrong
numbers is worse than no golden.

Characterized precisely, so the next attempt starts from here:

- d10, d12 and d16 are **BIT-EXACT**, and all 52 shared `.udg` model keys agree
  — including `nreg` and every ARMA coefficient. The model and its estimation
  are right.
- Only **d11 and d13** move, by exactly the leap-year prior, on **Februaries
  only**: 0.885% non-leap, 2.655% leap.
- Ruled out: CB-34's assignment direction, and the `Sprior`/`Setpri` deferral.
- Remaining suspects, named at the wall: which of `Kfmt` / `Lpradj` / `Priadj`
  the oracle carries out of the LAST candidate's `tdaic`.

Also still walled, and untouched: **`aictest=(user)` and user-defined holiday
chi-square testing** (`automx.f:463-500`) — `usraic.f` and `chkchi.f` have no
C++ at all.

### Gated by

`extra/airline_pickmdl-aictest-{td,tdeas,first,otl}` and
`generated/airline_aictest-td-aicdiff` (where the oracle **rejects** trading
day — `nreg` 0, `aictest.td: no` — and the engine kept it before the parse fix).
`-first` is the only route to `automx.f:700-847`; `-otl` is the only spec in the
corpus that makes `identify=` mean anything.

Mutations, each failing a **different** set: never run the AIC tests **30**;
never restore the design between candidates **10**; skip the AIC tests in the
post-loop `identify=first` block **10**; revert the `gtinpt` `pvaic` default
**105**; narrow the post-loop gate back to `lidotl` only **4**.

**Noted, not acted on:** `svaict.f` — the `aictest.td` / `aictest.diff.td` /
`aictest.e.*` savelog block — has no C++ and is not emitted by `x13run_m3`. A
pre-existing, general gap affecting every aictest spec, deliberately out of
scope here.

## Previous session (2026-07-29b): composite under SEATS + all of `amdfct.f`

Three increments, all closed. Details in `CLAUDE.md`; the durable pieces:

- **`agr3s.f`** — one SEATS component routes the whole metafile's indirect
  adjustment through `agr3s` rather than `agr3` (`X11agr` is metafile-wide:
  `aaamain.f:73` arms it, `gtinpt.f:1170` ANDs each component's `Lx11` in). It
  is a **different answer**, not a variant: the indirect SA IS the aggregate
  `Ci`, output is only `isf isa ie5 ip5 ie6 ip6 i18`. **CB-32** logged.
  Remaining gap: `Ci` past the observed span needs `Setfsa` (below).
- **`amdfct.f`'s out-of-sample arm** — closes `estimate{outofsample=}` and
  `pickmdl{outofsample=}`. `Nfev`/`Niter` are deliberately NOT restored (the
  final `rgarma` at `:299` is commented out). The arm strips outlier regressors
  dated inside the window and the `ave` scale reads the STRIPPED series.
- **`amdfct.f`'s BACKCAST arm** — closes `forecast{maxback=}` under pickmdl.
  **CB-33**: `bcstlim=` is inert (`.and.` where the algorithm wants `.or.`).
  One narrow corner left open, measured: out-of-sample backcasts with an
  outlier in the first three years reads 6.6959 vs the oracle's 6.71.

## Previous session (2026-07-29a): `pickmdl{}` base algorithm

`M5/pickmdl: port automx.f -- the classic X-11-ARIMA candidate search`.
Was parse-only and silently model-less (`nmodel: 0`). See
`tools/pickmdl_scouting.md`. Findings still worth carrying:

1. **`bstget.f:80-96`'s effective-observation split belongs INSIDE `bstget`.**
   At the call site, re-estimating a winner that was not the LAST candidate
   estimated **crashed** — `Nintvl` still described the previous candidate.
2. **A probe that injects text by string-replace has to know where the block
   is.** The spec's own comment header says `pickmdl{}`, so
   `txt.replace("pickmdl{", …)` edited a COMMENT and the oracle rejected every
   variant — which looks exactly like a finding. An unanchored `file =` match
   also strips `series{file=}`. *A uniform result across every level of every
   factor means suspect the harness.*
3. **`mode=` is provably INERT** — `iautom` is a `gtinpt.f:873` LOCAL and
   `gtautx.f:230` forces it to 1 regardless.
4. `gtnmvc` bounds a value by the **destination string's length**, not by its
   `maxchr` argument — pre-size the buffer (`series.cpp:102`).

## Previous session: the composite DIAGNOSTICS front

`x13run_composite` had never emitted the QS / spectrum-peak / NP blocks; nothing
in the engine was wrong and all 103 shared keys matched the moment the emit was
wired in. Then the INDIRECT (`Iagr==4`) pass, 52 further keys — **155 of 155
shared keys match, zero golden-only and zero engine-only.** Two things not to
re-derive: the peak-label lists are ONE accumulated string split at `Nspdir`
(`savpk.f:88-115`), and `svtukp.f` sets `iLb=7` on the indirect tables against 4
on the direct ones, so the KEY is `spcindsa` while the LABEL is plain `sa`.
**CB-31** claimed (`x11ari.f:346` passes a savelog index as a table subscript,
so the oracle emits no `qsind*` key at all).

**A measured coverage gap, recorded rather than hidden:** savpk's real
`.dir`/`.ind` peak SPLIT is unexercised — this composite finds no visually
significant peak, so all four keys are `none` and **a mutation swapping the two
output halves passes the whole suite.**

## The docs are now self-checking -- READ THIS BEFORE WRITING A NUMBER

A staleness audit found the deliverable report quoting a parity count **five
fronts out of date**, a coverage ledger understating itself by **240 routines**,
and a scouting doc contradicting its own later section. All three were true when
written. Three generated artifacts now exist so it cannot recur:

| artifact | command | owns |
|---|---|---|
| `docs/METRICS.md` | `tools/metrics.py --write` | every countable claim |
| `docs/WALLS.md` | `tools/walls.py --write` | what the engine refuses |
| `tools/ported.yaml` | `tools/coverage_map.py --audit --promote` | which .f files are ported |

**Never type a count into prose.** Wrap it in a marker --
`<!--x13:parity_pass-->5803<!--/x13-->` -- and `--write` maintains it while
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

## Open, in the order I would take them

1. **The Picktd-flip corner above** — d11/d13, Februaries only, 0.885%/2.655%.
   Smallest well-characterized gap on the board: the model is proven right and
   the suspect list is down to three flags. Needs a spec with
   `regression{aicdiff=}` tuned between two candidates' AICC gaps.
2. **Two automdl BASELINE disagreements**, both isolated, neither gated:
   `generated/usdeaths_automdl` (17 keys; the known iddiff d=0/d=1 split, and
   `nreg` 1 vs 0 — the engine is missing the Constant) and `ces_amuse`
   (75 keys, model 5 vs 4 ARMA terms; localised to label 40's `amidot` arm,
   since `noautooutlier=tramo` makes it bit-exact). Both make every option
   verdict on those series uninterpretable.
3. **The SEATS FORECAST decomposition (`ansub3.f:356-678` / `Setfsa`).** Wanted
   by two separate fronts: the composite i18 forecast tail and the `hpcycle`
   filter (`tools/seats_hp_scouting.md`). Nothing else in `composite{}` needs
   it.
4. **`svaict.f`** — the `aictest.*` savelog block, absent for every aictest
   spec. Pure report surface over arithmetic that is now proven correct, so it
   should be cheap; the goldens already carry the keys.
5. **`gtdpvc` parses decimal literals 1 ulp off the nearest double**: `"0.95"`
   → `0.95000000000000007` vs the correctly-rounded `0.94999999999999996`.
   Latent everywhere a spec supplies a decimal. **Check whether the Fortran
   reader does the same before changing anything** — if it does, the port is
   faithful and this is documentation, not a fix.
6. **What is left of `composite{}`**, now small: pseudo-additive (`Psuadd`) and
   the forced/rounded indirect series on the **agr3** path (`agr3.f:426-538` —
   ported for agr3s, still absent for agr3, and ungated on both for want of a
   `force{}` composite spec).
7. **A composite whose components carry a residual peak**, to gate savpk's real
   `.dir`/`.ind` split — only the degenerate branch runs today.
8. The amdfct out-of-sample-backcast-with-outlier corner (0.2% out, measured
   and walled); `spectrum{altfreq=yes}` pending CB-30; `history{outlier=auto}` /
   `x11outlier=no` / `additivesa=`; the slidingspans `chs` per-span prior phase;
   `pickmdl{aictest=(user)}` (needs `usraic.f`/`chkchi.f`); the `!Hvmdl`
   no-model cleanup (`arima.f:476-527`).

## Environment notes

Unchanged (`/codex:cancel` broken; codex notifications carry no findings;
Windows Python cannot read git-bash `/tmp` or `/d/` mounts; **no heredocs
carrying `\n` inside C string literals in the Bash tool** — it expands them into
real newlines and the compile fails on `missing terminating "`, use the Edit
tool instead; oracle flag order is `x13as_ascii_O2.exe <specbase> -s`; ad hoc
runs are `build/x13run_{m3,x11,seats}.exe <spec>.spc` from the spec's own
directory; `option_sweep.py` stages every `tests/corpus/data/*.dat` and abspaths
`--outdir`; **the Bash tool's cwd persists across calls** and is NOT shared with
the PowerShell tool — a stray `cd build` will make a later `pytest tests/parity`
report "no tests ran").

`run_parity.py --update` honours only the LAST `--filter`, and **`--filter` is a
glob that will re-bless matching neighbours** — bless one spec per invocation
and check `git status` after. Adding a `core/src/**/*.cpp` still needs the build
run twice: the first prints `GLOB mismatch!` and stops.

Oracle input quirks hit this session: `x11{save=(b1 …)}` is rejected ("Save
argument is not defined") and so is `outlier{savelog = all}`.

## Methodology, reconfirmed

Every increment ends with a **mutation test, per HALF** — and on
auto-discovering gates it is the only evidence the new specs are compared at
all. This session added the sharpest instance yet: **a mutation that PASSES
means the precondition is saturated, not that the code is dead.** The
oracle-on-vs-off table proves a flag matters; only engine-vs-oracle says whether
the port honours it. And a "coverage" spec that exercises an option at its
DEFAULT value covers nothing — that is how `aicdiff=` stayed unparsed.
