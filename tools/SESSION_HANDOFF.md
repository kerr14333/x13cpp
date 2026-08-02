# Session handoff — 2026-07-30 … 2026-08-01 (`pickmdl{}` + `regression{aictest=}` CLOSED; the SEATS forecast decomposition CLOSED; the WHOLE `aictest.*` savelog surface ported and gated; `x11aic.f`'s trading-day branch ported; the Picktd-flip corner FIXED; `x11regression{user=}`, the `Kswv==3` prior-TD route and `x11aic.f`'s USER branch ported -- x11aic CLOSED)

Replaces the 2026-07-29b handoff. Its findings are carried forward below where
they still matter; its open item 1 (pickmdl's last wall) is done bar one
measured corner, now walled, and its open item 3 (the SEATS forecast
decomposition) is closed — see the second "this session" section.

## Where things stand

**Tree clean on `checkpoint/m5-seats-slidingspans`.** Nothing uncommitted, no
background work outstanding. `git log --oneline -8` for the current head — this
file is written *before* the commit that contains it, so any SHA named here is
necessarily one behind. (It has gone stale that way twice; hence no SHA.)

| check | result |
|---|---|
| `python -m pytest tests/parity -q -n 8` | **<!--x13:parity_pass-->6000<!--/x13--> passed / <!--x13:parity_fail-->0<!--/x13--> failed / <!--x13:parity_skip-->474<!--/x13--> skipped** (~86s) |
| `cd build && ctest` | <!--x13:ctest-->12/12<!--/x13--> |
| `Rscript bindings/r/test_x13c.R` | 165/165 (not re-run; untouched surface) |

**Run the suite with `-n 8`** (pytest-xdist, installed). 262s serial → ~86s,
same counts. Safe because every gate compares stdout from a read-only
subprocess and no harness writes side files — re-verify that if a harness ever
changes. Build is ~25s; use `-k "<name>"` (~3s) while iterating.

Standing constraints: **never merge this branch to `main`.** Pushing THIS
BRANCH is authorised as of 2026-08-02 (the repo is PUBLIC —
`github.com/kerr14333/x13cpp` — and `origin/main` is still at M1, so the branch
push publishes the whole port on a new remote branch and leaves the default
branch alone). **DONE — the branch is on the remote at `ccbe7bbe`**, local
tracking set, nothing ahead or behind. `main` is an ancestor of this branch
(`HEAD..main` == 0), so that one push carried every local commit and `main` can
fast-forward whenever wanted, with no merge commit and nothing discarded.

Mechanics for the next push, because this cost two minutes to rediscover: the
assistant's `git push` is blocked by the permission system, AND
`credential.helper=manager` (Git Credential Manager) hangs inside this harness
waiting on a GUI dialog nothing can answer — so the command times out rather
than failing. The user runs it in a real terminal window, or wires gh's token
first with `gh auth setup-git`.
Also: **never run `tests/corpus/generated/genspecs.py` or
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

## This session, part 2: the SEATS FORECAST decomposition — CLOSED

`ansub3.f:353-678` + `sigsub.f:1586-1605` + **`ansub4.f`'s refold** are ported
and the `tfd`/`sfd`/`afd`/`yfd` tables punched, with **zero new goldens** (all
52 SEATS specs already ship them). **All 52 gate bit-exact** — 48 at 1e-12 or
better, worst `payems_mean-td-seats` at 1.01e-11 — and `KNOWN_GAP` in
`tests/parity/test_seats_forecast_tables.py` is now **empty**. It is kept, and
still asserts from both sides, so a stale row would fail loudly rather than
quietly excuse a spec. Full record: **`tools/seats_forecast_scouting.md` §3**.

**Two defects, both of them live code this port had recorded as unreachable,
and they cancelled each other.**

1. `ansub3.f:552-653`, the Tramo block. `Tramo` is 1 on every X-13 SEATS run,
   so it executes; it rewrites `z` over the forecast span with `LOG(TramLin)`,
   shrinks `lf` by `mq/2`, and folds the discrepancy into `sc`/`cycle`/`ir`.
   Closed 33 of the then-39 gaps.
2. The saved tables do NOT come from `sigex.f:3631-3636` — those USRENTRY
   1409/1410/1411/1413 calls sit inside `if (Tramo .le. 0)` and never fire.
   The live writer is `ansub4.f` (`:3144-3210` log, `:2284-2337` non-log),
   which **refolds the deterministic preadjustment factors** back onto the
   antilogged components. Probing all five call sites showed only ansub4.f's
   fire, and its `fsa(1)`/`fs(1)` are the golden `afd`/`sfd` to the last digit.

Feeding the block `ctx.forecasts.trnfct` is wrong by exactly `TramDet`; the
missing refold is wrong by `TramDet` the other way. What survives the
cancellation is the one thing `trnfct` carries and `TramDet` does not — the
length-of-month/leap prior — so the residue was exactly `lpfac`, in the period
containing February, and nothing else. That is why it presented as a
trading-day bug on four `*_mean-td-seats` specs plus two unexplained ones near
the floor.

Also settled, measured not assumed: `Pareg(i,0..7)` is always the identity on
the X-13 path (`Npareg` is `l_npareg`, initialised 0 at `ansub9.f:1598` and
never set), and `ansub4.f`'s own `bias1/bias2/bias3` are dead — `:2795-2797`
overwrites all three with 1.0 right after computing them.

**Retired by this:** the earlier "the forecast trend takes `bias1c` where
`sigsub.f:1594` reads `bias3c`" finding. The saved `tfd` is not the antilogged
trend at all — it is `ftr = (fsa/fir)/(fcyc/100)`, a residual of `fsa`
(`fortr` is 1 unconditionally), so no bias factor enters it. That also makes
the two golden identities the test asserts (`tfd == afd` with no transitory
component, `afd == tfd*yfd/100` with one) derivable rather than empirical.

**One test-metric change, and it is a change of yardstick, not of tolerance.**
`_worst` now normalises by `max|golden|` over the table rather than by each
value. On the non-log path `sfd`/`yfd` are additive DIFFERENCES that cross
zero — `unrate_mean-d0`'s `yfd` runs down to 1.4e-08 — so per-value relative
error measures cancellation, not accuracy: an absolute agreement of 1.3e-15
there read as 9.1e-08. Same yardstick `ansub3.f:653-663` uses on the trend
itself. It does not loosen the multiplicative tables, whose values all sit
within an order of magnitude of their own max.

**Dead ends, measured, do not repeat:** extending with the RAW pre-cap MA
(makes the historical WORSE and still misses the forecast), using
`ctx.forecasts.trnfct` as the *extension* (breaks the historical — it belongs
only at `:660`), deriving `bias2c` from the goldens (it cancels).

**Standing lesson, twice in one front:** a reachability claim that has not been
run against the oracle is not a measurement. Both defects here were written off
as unreachable, both were live, and because they sat adjacent in one data path
they cancelled into a 46/52 pass rate that read as a nearly-finished port.

**Unblocks two fronts that were waiting on it:** the composite `i18` forecast
tail, and the `hpcycle` filter (`tools/seats_hp_scouting.md`, whose step 1 was
exactly this).

## This session, part 3: both automdl BASELINE gaps were already closed

`usdeaths_automdl` and `ces_amuse_automdl` **both match the oracle on every
shared `.udg` key**, and had since the UPDATE-2026-07-28c wiring. They were
carried as open for two more sessions because the record was prose, not a gate.

- **usdeaths** — `(0 1 1)(0 1 1)`, `nefobs` 59, 48/48 keys. Recorded as
  `(1 0 1)(0 1 1)` / `nefobs` 60 / `nreg` 1-vs-0. Removed from
  `test_check_diagnostics._WRONG_MODEL` and from `_AUTOMD_IDDIFF_GAP` in
  `test_qs_diagnostics` and `test_spectrum_peaks`; those three gates had been
  SKIPPING and now compare and pass.
- **ces_amuse** — oracle final `(3 1 1)(0 1 1)`, 105/105 keys. It had no
  corpus spec at all: the disagreement was only ever seen through
  `option_sweep.py`. Added `generated/ces_amuse_automdl.spc` (hand-authored,
  header says so) and blessed it.

That lifts the exclusion which made every `ces_amuse` row in
`tools/dropped_options_scouting.md` "baseline noise" — those option rows are
now **unmeasured rather than false**, and worth re-running.

**The mutation record is mostly negative and is written down so it is not
re-walked.** Removing the `amidot` call, removing `automd_finalize_tail`, and
perturbing `iddiff`'s `ids` all leave ces_amuse's baseline model unchanged;
only the `lds` mutation at `automd.cpp`'s call site moves usdeaths (3 gates
fail). `amidot` is a no-op because the BIGCV AO scan finds nothing on this
corpus and the finalize tail is a no-op on both series, so those two test
nothing — saturated preconditions, the same shape as the `critical=3.5` case.
The real datum is that ces_amuse's `(3 1 1)(0 1 1)` survives both an `iddiff`
and a BIC perturbation: it comes out of the adequacy stage restoring from
`bkdfmd`'s backup, not out of the search.

The new spec's gates were proven live the other way — corrupting three keys of
its blessed `.udg` fails all three blocks, so it is compared and not silently
skipped. Model-sensitivity on this series is carried by the `-noautooutlier`
sibling, which fails under two independent mutations.

**Standing rule this reinforces:** an exclusion recorded in prose is not a
gate, and it does not re-measure itself. Both of these were true-when-written
and wrong for two sessions.

## This session, part 4: `svaict.f` — and the missing-key blind spot it lit up

The `aictest.*` savelog block is ported (`core/src/automdl/svaict.cpp`, with
`mktdlb.f` / `mklnlb.f` / `mkealb.f`), emitted by `x13run_m3` through the
oracle's own FORMATs, and gated by a new
`tests/parity/test_aictest_savelog.py` — **16 specs, 54 keys, all bit-exact**.

**Every one of those keys was previously uncompared.** The goldens have carried
them since the corpus was blessed, the engine emitted none, and no gate looked
at the intersection. That is the missing-key shape of the parsed-but-unread
class: not a wrong value, an ABSENT one, and absence is invisible to a gate
that only diffs what both sides produce.

**It found a real defect on its first run.** `automx.f:308` guards the
per-candidate design restore with `(Lidotl .or. Itdtst.gt.0)`; the port had
only `lidotl`. So a spec with `aictest=` but no `outlier{}` never restored, and
whatever regressor the previous candidate's tdaic/easaic had selected stayed in
the design. Invisible while only `aictest=(td)` was exercised — tdaic replaces
the TD group itself each round — and it surfaced the moment `aictest.diff.td`
became comparable: on `extra/airline_pickmdl-aictest-tdeas` the leaked EASTER
column moved the last candidate's TD test to 22.3677 against the oracle's
20.1970, which is exactly the value its easter-free sibling reports. One
clause. Note the Fortran's guard here has no `Leastr`, unlike label 20's, which
carries all three; transcribed as written.

**A second bug, mine, caught by sweeping rather than by the suite.** `itoc` is
transcribed from a Fortran `CHARACTER*(N)` and writes INTO a fixed-length
buffer, abending when it is too short. Passing it an empty `std::string`
abended, so five easter-carrying specs went `OUTCOME: FATAL`. Pre-size any
buffer handed to `itoc` — `mkealb.f` uses `CHARACTER cwin*2`, `mktdlb.f` a
30-char `tdstr`, and both are now reproduced literally.

**What is still NOT ported behind the same `aictest.` prefix** — the gate names
each with its routine so the set cannot rot into an allowlist:
`aictest.td.{num,reg,reg2}` and `aictest.td.aicc.*` (`tdaic.f`),
`aictest.easter.num` / `aictest.e.aicc.*` (`easaic.f`),
`aictest.lom.aicc.*` (`lomaic.f`), `aictest.xe*` (`x11aic.f`, the
x11regression Easter test). `aictest.pv` is written by svaict's CALLER
(`arima.f:463`) and needs `regression{pvaictest=}`, which no corpus spec sets,
so it has no golden to gate against.

Also transcribed as written rather than "fixed": `svaict.f:79-81` gates the
length-of-month group's `cvaic` line on `Rgaicd(PTDAIC)` — the TRADING-DAY
threshold — while writing `Rgaicd(PLAIC)`. Every other group gates on its own
index. Not reachable on this corpus (no golden carries `aictest.cvaic.lom`), so
it is a CB candidate, not a claimed one.

**The AICC tables landed too, in the same shape.** `tdaic.f`'s
`aictest.td.{num,reg,reg2}` + `td.aicc.*`, `easaic.f`'s `easter.num` +
`e.aicc.*` (and its unprefixed `testalleaster`), and `lomaic.f`'s
`lom.aicc.*`. **86 lines now compared across 15 specs, 0 differing, and every
one is byte-identical including ORDER** -- the tables are written from inside
each test, so they precede svaict's verdicts in the `.udg`.

What made that tractable: all three take an `lsumm` parameter now, mirroring
the oracle's `Lsumm`, and it is true only from `explicit_aictest`. Every
tdaic/easaic/lomaic call outside `arima.f` passes a literal 0 (automd.f x3,
automx.f x2), which is exactly why an automdl or pickmdl golden carries
`aictest.td` but never `aictest.td.num`. Threading the flag through all nine
call sites is what makes that suppression visible at each one rather than
hidden in a context flag.

Two formatting details that are load-bearing and would have been silently
wrong under a numeric comparison: `easaic.f`'s 1020 is `(a,':',i5)` with NO
space before the colon where `tdaic.f`'s 1020 is `(a,1x,i6)` with one; and
`testalleaster` has no `aictest.` prefix at all, so a prefix-scoped sweep
misses it -- it is carried in the struct and owned by the gate for that reason.

**Still unemitted behind the prefix, and now the only two:** `aictest.xe*`
(`x11aic.f`, the x11regression Easter test, which runs on the X-11 path and so
needs a different harness) and `aictest.pv` (`arima.f:463`, needs
`regression{pvaictest=}`, which no corpus spec sets, so it has no golden).

**Standing rule this adds to the pile:** a golden key nothing emits is not a
passing test, it is an unmeasured one. When porting a savelog block, sweep the
goldens for the whole key PREFIX and classify every member — ported, or named
with the routine that owns it.

## Previous session (2026-07-29b): composite under SEATS + all of `amdfct.f`

Three increments, all closed. Details in `docs/M5_PORT_NOTES.md` (entries
49-51); the durable pieces:

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
`<!--x13:parity_pass-->6000<!--/x13-->` -- and `--write` maintains it while
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

## This session, part 5: `aictest.xe*` — the last family behind the prefix

`x11aic.f`'s Easter AICC table plus `x11mdl.f`'s verdict, six keys on two
specs. **The arithmetic was already bit-exact** — `x11reg.cpp`'s
`x11aic_easter` has matched the oracle since the x11regression increment and
its `aicc_xe` canaries proved it. Only emission was missing, which is the same
missing-key blind spot the svaict gate was built to expose. This family needs
`x13run_x11` rather than `x13run_m3`, so the gate now drives two harnesses.

**Two durable findings, both in `docs/M5_PORT_NOTES.md` entry 54.**

`aicind` in `x11aic.f` is NOT a local — it is absent from the declaration
list, so Fortran case-insensitivity resolves it to COMMON `Aicind` in
`arima.cmn`, the slot `easaic.f` writes for the *regARIMA* Easter test.
Entering x11aic clobbers that window (`x11aic.f:55`), and `x11mdl.f:280` reads
the same slot back. Both halves reproduced.

`aictest.xe.window` is written through TWO FORMATs on the same key: `(a,i3)`
on accept (`window:  15`), a literal string on reject (`window: 0`). One space
versus three, invisible to any numeric comparison. Third instance of that
shape in this block after `easaic.f`'s colon and the unprefixed
`testalleaster`.

**And the real bug, found by insisting the reject arm be reachable.** The
corpus covered only the accept arm. Rejecting means raising the threshold —
and `x11regression{aicdiff=}` was token-consumed and written nowhere, while
`gtxreg.f:513-518` sets `Xraicd`. Default is `ZERO` on both sides
(`gtinpt.f:477`), so every existing spec agreed and nothing failed: the
parsed-but-unread shape, invisible until something needed a non-default value.
Now wired, and proved READ rather than parsed — two specs differing *only* in
`aicdiff=5.0` give different engine verdicts (`xe: yes` vs `xe: no`).

Also joined `run_x11.cpp`'s span-replay save/restore set (fifth time that seam
has bitten): the table is appended to from inside x11pt2, so a
`slidingspans{}`/`history{}` replay would report the last span's. The oracle is
immune by a route the port cannot copy — `x11mdl.f:291` clears `Xeastr`, but
the port's `editor.f:1734-1757` re-derivation of `Otlxrg` still reads it.

New spec `tests/corpus/extra/airline_x11regression-aicdiff.spc`, hand-authored
— **not** produced by `genextra.py`, and its header says so.

**Nothing is left behind the `aictest.` prefix that has a golden.** What
remains is `aictest.xtd*` / `aictest.xu*` (x11aic.f's trading-day and user
branches, which the engine does not port — the Easter branch only) and
`aictest.pv` (`arima.f:463`, needs `regression{pvaictest=}`, which no corpus
spec sets). All three are named in the gate's UNOWNED table with their routine.

## This session, part 6: the Picktd-flip corner — ROOT-CAUSED (fixed in part 8)

No engine change, and that is the honest result. The wall in `automx.cpp` now
carries the whole chain instead of a suspect list, and the suspect list it
replaces was **wrong**.

**The measurement.** Reaching the branch needs `regression{aicdiff=}` tuned
between two candidates' AICC gaps — 18.33 18.82 18.85 18.49 20.20 on
`airline_pickmdl-aictest-td`, so `aicdiff=19.0` splits them: candidate 5
accepts trading day, the winner (candidate 2) rejects it. The error is exactly
`engine = oracle * (days-in-Feb / 28.25)`, Februaries only, with d10/d12/d16
bit-exact.

**The old note named `Kfmt` / `Lpradj` / `Priadj`. All three are IDENTICAL to
the oracle's on this spec.** The value that disagrees is `Sprior` — and the
`Sprior`/`Setpri` deferral had been recorded as *ruled out*.

Traced with an INSTRUMENTED build of the oracle Fortran (scratchpad copy; the
vendored tree is never edited). The chain:

1. The oracle sets `Setpri` at editor time and issues x11int.f:53's
   `Adj -> Sprior` copy from **x12run.f:174**, before `arima`/`automx`.
2. `tdaic.f:600-623` then writes `Sprior` directly on a Picktd transition, and
   nothing afterwards refreshes it from `Adj`.
3. This port assigns `ctx.adj.setpri` only in `x11_prestage`, AFTER the model
   stage — so tdaic's write is skipped by its own `setpri >= 1` guard. The
   "deferred prior-series bookkeeping" comment in `aictst.cpp` was describing
   **code that never executes**.
4. The port compensates with a post-model `Adj -> Sprior` copy in `x11int`,
   correct exactly when `Adj == Sprior` there — true on every gated spec.
5. Here it stops holding: the Picktd restore puts `Adj` back to all-1 while
   `Sprior` must keep the prior tdaic wrote.

**Confirmed from the other side.** On the automdl baseline the oracle reaches
x11pt3 with `Sprior=1.0`, `Adj=0.99115`, `Priadj=-4`: TD survived, so x11pt2's
`tdlom` consumed the prior into `Factd` and reset `Sprior`. On the flip spec TD
does not survive, `tdlom` never runs, `Sprior` applies directly. Two specs,
opposite routes, one explanation.

**Attempted and reverted.** Suppressing the post-model copy alone moves the same
2.655% onto the automdl baseline, because tdaic's write is still dead — the
compensation is load-bearing. The real fix is to establish `Setpri` and the span
pointers it derives from BEFORE the model stage, which is a driver-ordering
change, not a local one. Not worth risking the spine for one walled corner.

**Standing rule this earns:** *a guard whose precondition is never satisfied
looks exactly like a ported branch.* `setpri >= 1` reads as faithful, the
comment above it claimed deferral, and nothing distinguished "runs and does
nothing" from "never runs" until the oracle was instrumented. Full record in
`docs/M5_PORT_NOTES.md` entry 55.

## This session, part 7: `x11aic.f`'s TRADING-DAY branch -- CLOSED

`x11regression{aictest=(td)}` and its three siblings run, emit
`aictest.xtd.{aicc.notd,aicc.td,reg}` + `aictest.xtd`, and gate on BOTH verdict
arms plus the two-test configuration. **12 keys over 3 specs, byte-identical.**
Full record in `docs/M5_PORT_NOTES.md` entry 56; the durable pieces:

**Four silent defects, none of which announced itself.** The `aictest=` token
was parsed and DISCARDED (`Xtdtst`/`Xuser` were written NOWHERE in the engine,
so the spec ran the plain fixed-TD path and returned `OUTCOME: OK`);
`gtinpt.f:468-469`'s `Xaicrg = NOTSET` / `Xaicst = 31` defaults were missing,
which would have made `addtd`/`mktdlb` build a two-regime TD group for a plain
`aictest=(td)`; `xrgtrn_td` had `xrgtrn.f`'s `Tdgrp>0` arm HARDCODED, so the
no-TD candidate rescaled by the day counts instead of just centring (+150.85
against the oracle's -771.73); and `x11mdl.f:308-381`'s no-regressors-left early
return was absent entirely, so on a TD REJECT the engine fitted the empty design
and produced a c16 carrying the length-of-month prior where the oracle punches
identity factors and returns.

`editor.f:1618-1627`'s `Tdgrp`/`Stdgrp`/`Holgrp` derivation is now transcribed
into `x11mdl_td` -- the port had never needed it because every x11regression
spec until now carried a fixed `variables=(td)`.

**The mutation that PASSED is the one to remember.** Deleting the x11mdl early
return left the WHOLE SUITE green, because `test_x11regression_tables.py`'s
`AIC_CASES` was a hand-written one-element tuple naming only the Easter spec.
None of the three new specs' b16/c16/xrm were compared. Now auto-discovered with
a floor assertion, and the same mutation fails with the 2.655e-02
leap-February signature. *A hand-maintained case list is an allowlist that
silently stops growing.*

**One gate NARROWED, deliberately.** `test_m1_parse.py::_oracle_ok` treated
"oracle exit code != 0" as "the oracle rejected this spec". The reject spec is
the first case where those differ -- the oracle exits 2 having parsed, run and
written a complete `.udg`, purely because x11mdl wrote a NOTE. A nonzero exit
now counts as a rejection only when NO `.udg` was produced, which still catches
the one genuine case (`census-examples/composite/total`, exit 3 on a SIGFPE,
no `.udg`).

**New specs, all hand-authored and saying so in their headers:**
`extra/airline_x11regression-aictest-{td,tdrej,tdeas}`.

**Census bug measured, NOT claimed.** `x11mdl.f:378`/`:883` write
`'finalxreg01: none'` through FORMAT 1060, which in that scope is the
weekday-column header and has no data descriptor -- the string is dropped and
the `.udg` gets a stray column header. Visible in this spec's own golden. Not a
CB entry because the engine emits the `nfinalxreg`/`finalxreg01` family on NO
path, so there is nothing to reproduce it against yet.

**Noticed, not fixed:** a parse-time refusal issued through `inpter` (this
increment's `aictest=(user)`, and e.g. the older `transform mode=diff`) is NOT
inventoried in `docs/WALLS.md` -- walls.py scans only the `*_not_ported`/`fatal`
helpers. Pre-existing blind spot in the wall inventory, not in the engine.

## This session, part 8: `Setpri` moved ahead of the model stage -- CLOSED

The change part 6 root-caused and declined to make. It is small, and across the
whole corpus **exactly neutral**: the only deltas are the twelve gates the new
probe spec adds. `docs/M5_PORT_NOTES.md` entry 57 has the record; the durable
pieces:

**The wall in `automx.cpp` is gone** (17 gaps -> 16). `extra/airline_pickmdl-
aictest-tdflip` -- the probe spec part 6 described, now committed -- gates the
Picktd restore on twelve tests, d10-d13/d16 through the binding included.

**What actually moved.** The editor geometry is factored into
`x11_editor_geometry` (`driver/x11_prestage.cpp`). `run_pre_model` calls it just
ahead of the model stage and issues x11int.f:53's `Adj -> Sprior` copy there;
`x11_prestage` calls it again -- that second call IS `x11ari.f:149`'s second
`setxpt` -- but does NOT re-assign `Setpri`, because the oracle never refreshes
it after editor.f:851. The post-model copy is suppressed on the model path
(`x11int(ctx, copy_sprior=false)`). Three model-stage Sprior writes go live with
it: `tdaic.f:600-623`, `rmlpyr.f:59`, `pass2.f:101`.

**Placement inside the pre-model stage is load-bearing, and NOT where part 6
implied.** The copy cannot sit beside the `/adjcmn/` record: `trnaic`
(x11ari.f:81) rewrites `Adj` wholesale afterwards and the oracle re-issues the
copy after it at `trnaic.f:278`, a line this port does not have. Issuing it
after trnaic covers both.

**Both mutations fail, and DIFFERENTLY.** Restoring the post-model copy and
suppressing the pre-model `Setpri` each break the same five parity gates -- but
the second also breaks a ctest unit test. The two halves are not one switch
described twice.

**Spec-authoring trap.** `x11{save=(b1 ...)}` is REJECTED by the oracle, so a
spec like this cannot ship the b1 golden `test_x11_tables` discovery requires;
it gates through `test_bindings` (discovery keyed on d11) instead. And blessing
does not notice: `run_parity.py --update` reported `PASS` and wrote a bundle
from the rejected run. Only `test_m1_parse`'s outcome gate caught it. **Bless,
then read the `.err` in the bundle.**

## This session, part 9: the `ctod` 1-ulp question -- ANSWERED, no engine change

The board item asked whether the Fortran reader is also 1 ulp off the nearest
double before changing anything. **It is**, so the port was already faithful and
the item closes as a test rather than a fix. `docs/M5_PORT_NOTES.md` entry 58;
the durable pieces:

**The cause is `ctod.f`, not `gtdpvc.f`** -- a hand-rolled accumulator that adds
each fractional digit's own quotient (`val = val + digit/scl`), so the result
carries every intermediate division's rounding error. Not `strtod`, and not
"accumulate a mantissa then divide once" either; both alternatives give
different doubles. `core/src/specparse/util.cpp` transcribes it verbatim.

**Scope: 52 of the 286 distinct decimal literals in `tests/corpus`** differ from
the correctly-rounded double, always by exactly 1 ulp, in both directions. The
heavy users are `regression{b=}` coefficient lists and user-regressor data.
Dyadic literals (`3.5`, `19.0`, `1.96`) are untouched -- which is why this was
never a parity failure.

**It was gated by one accidental canary.** Swapping in a correctly-rounded
conversion breaks exactly ONE gate out of 5944:
`generated/airline_user-reg-x11.d9a.05`, at the 10th significant digit. Now
pinned directly by `tests/unit/test_ctod.cpp` (ctest **11/11 -> 12/12**), whose
expected values come from `tools/ref_ctod.f` compiled against the vendored
`oracle/fortran/ctod.f` -- **not** from re-transcribing the algorithm, which
would only prove I read it the same way twice. It compares BIT PATTERNS: writing
`CHECK_EQ(got, 0.95)` would compare against the C++ compiler's own correctly-
rounded parse, i.e. against the value the oracle does not produce.

Mutation re-run against the new test: 11 of its 24 checks fail, naming the
literals.

## This session, part 10: `x11regression{user=}` -- seven arguments parsed and DISCARDED

Board item 1 said the aictest USER branch "needs a spec with
`x11regression{user=}` before anything else". Writing that spec is what found
this. `docs/M5_PORT_NOTES.md` entry 59; the durable pieces:

**`user=`, `data=`, `start=`, `file=`, `format=`, `b=` and `usertype=` all fell
through `gt_x11regression`'s `else { consume_value(ctx, nullptr); }`.** Measured
on `extra/airline_x11regression-user` (new, hand-authored): the oracle's `xrm`
carries **7** columns, this engine's carried **6** -- the user column simply
absent -- and d11 was ~3e-4 out, at `OUTCOME: OK`. Fourth parsed-but-unread in
this block after `regression{aicdiff=}`, `x11regression{aicdiff=}` and
`x11regression{aictest=}`.

**Only the PARSE was missing.** `loadxr` already moves the x11reg user store
into the working slots and `regvar` already builds the columns; the port is
`gtxreg.f`'s argument arms plus its :607-800 tail. Note its adrgef dispatch has
**four** arms, not `getreg.f`'s sixteen, and `start=` writes the SAME `Bgusrx`
that `regression{start=}` does.

**A pre-sized `std::string` is load-bearing.** `gtnmvc` writes through
`putstr`/`insptr`, which bound against `chrvec.size()`, so a default-constructed
string ABENDS instead of growing -- and the symptom is a bare `OUTCOME: FATAL`
with NO message, because the parse dies before it can print one. Size these
buffers like `usrttl` (`PUREG * PCOLCR`) or lose an hour.

**Second bug: another state leak from `xrgdrv`'s transparent pass.** With the
parse fixed the engine emitted an `outlier.user` key the oracle does not.
`loadxr(F)` copies the x11reg user columns into the regARIMA slots (`Ncusrx`,
`Usrtyp`, `Usrptr`, `Usrttl`, `Nrusrx`) and `loadxr(T)` does NOT put them back;
the oracle is covered by `xrgdrv.f:207`'s `restor()`, this port's `restor_span`
is not. **Third leak of this exact shape in `xrgdrv` alone** (after `Lterm` and
`Ksdev`) -- when adding anything to that routine, assume the restore is
incomplete until checked.

**Mutations, both halves:** suppress the user-column build **9 gates**; drop the
`Ncusrx` restore **1** -- and that one is `test_check_diagnostics`, which
compares the .udg key SET rather than values. Without a key-set gate the leak
was invisible.

**Not claimed as a CB entry, deliberately:** `gtxreg.f:274` maps `usertype=ao`
to `PRGTAO` (13) where the :740 dispatch tests `PRGUAO` (61), so an `ao` column
is titled 'User-defined' -- and the same line sets `Havxtd`. Both read as
defects; no spec exercises `x11regression{usertype=ao}` yet, and the rule is to
measure before naming one.

## This session, part 11: `Kswv==3` -- one unported line, four dead consumers

`x11pt1.f:235` is `IF(Axrgtd)Kswv=Kswv+2`, and it was not ported -- so `Kswv`
could never leave 1 and everything keyed on 3 was dead. `docs/M5_PORT_NOTES.md`
entry 60; the durable pieces:

**It is a live wrong-numbers bug, not a theoretical one.** New spec
`extra/airline_x11regression-tdprior-td` (prior weights AND `variables=(td)` --
the four existing tdprior specs all omit the TD model, which is why nothing
caught it): d11 1949.02 came back **126.769 against the oracle's 124.850**, at
`OUTCOME: OK`.

**Four consumers, all now ported:** `xrgtrn.f:36`'s `Xnstar*X - Xnstar`,
`x11ref.f:117`'s `+1` instead of `Xn/Xnstar`, and `x11mdl.f:541-572 + :786-830`
-- the estimated coefficients become X-11 daily weights `Dx11`, get ADDED to the
user's priors, and Faccal/Factd are rebuilt from the sum through a second
`x11ref` that is passed **Kswv=4, not 3**, so it takes the ordinary arm.

**`Kswv` is deliberately NOT restored after `xrgdrv`** -- `xrgdrv.f:57/207` pass
`Lx11rg=F`, so the bump its own transparent x11pt1 makes survives into the main
run, which is exactly why the main x11pt1 then skips the prior-TD block and
takes the `Ixreg==3` Faccal restore instead. The SPAN drivers do restore it
(`x12run.f:166` -> `ssx11a.f:160`/`revdrv.f:528`) = `ctx.saved.kswv0`, captured
at PARSE because `ssprep_snapshot` runs after xrgdrv has already bumped it.

**The half worth remembering is the other one.** With the bump in, `a4`/`b16`
were bit-exact but `c16` was 5e-3 out. Cause: x11pt2 had a SHORTCUT --
`Stcsi = Sto/Faccal` instead of `x11pt2.f:846-894`'s rebuild from the raw
`Series` -- which had been explicitly verified equivalent when written, and IS
equivalent until a tdprior exists. Then x11pt1 divides `Sto` by the prior-TD
factor *and* folds the same factor into `Faccal`, so the shortcut removes it
twice. **An equivalence that holds over the corpus is an equivalence over the
corpus, not a proof** -- same class as entry 25's unreachability proof.
Mutating that branch costs **193** gates; every other piece here costs 10.

**Third half: ordering.** `x11ari.f` runs `xrgdrv` (:99) BEFORE `x11pt1` (:133),
so the oracle's `Kswv` is already 3 by the time x11pt1 could divide by the bare
prior TD. run_pre_model had the two blocks the other way round and divided
twice. Swapped; the `kswv == 1` guard then does the work by itself.

**`x11pt2.f:408-412` is left unported ON PURPOSE** (`Series *= Stptd` over the
forecast region, once per iteration): it touches only `[Posfob+1, Posfob+Ny]`,
never a published span. Recorded so it is not re-derived.

**Two more parsed-but-unread arguments.** `forcecal` (argidx 24 -> `Calfrc`) is
now honoured -- leaving it unread would have made this increment's two `Calfrc`
walls unreachable. **`reweight` (argidx 32 -> `Lxrneg`) is STILL unread** -- see
the open board.

## This session, part 12: `x11aic.f`'s USER branch -- CLOSED, and CB-35

The last of x11aic's three tests, and the oldest item on the board. Both of its
prerequisites were closed first (`x11regression{user=}` in part 10, `Kswv==3` in
part 11), so this was transcription plus measurement.
`docs/M5_PORT_NOTES.md` entry 61; the durable pieces:

**CB-35 is `active` and it DECIDES the test.** `aicnus` -- the AICC of the model
WITHOUT the user regressors -- is an uninitialized local with three routes to a
value: `:470` computes it (estend still true), `:457` seeds it from the winning
Easter AICC, or NOTHING does, when the trading-day test is accepted with no
Easter test alongside. The third exists because `:245` reads `ELSE IF(Xeastr)`
after `:243` already tested `Xeastr`; the intent was plainly `ELSE IF(Xuser)`.
The vendored -O2 oracle reads **exactly 0.0**, and the verdict is
`aicusr + Xraicd < aicnus` -- so **any negative AICC wins and the user
regressors are accepted unconditionally.** Reproduced by initializing to 0.0;
the caveat that this is one build's stack value is in the code, and the gate is
what pins it. Mutating it to a sentinel costs %(aicnus0)s gates.

**Three specs, one per aicnus route**, which is the whole reason there are
three: `-aictest-tduser` (uninitialized), `-aictest-tduser-reject`
(`:470` computes it, and the reject half of the verdict), `-aictest-easuser`
(`:457` seeds it). All bit-exact.

**Two transcription hazards, reproduced, NOT claimed as CB entries.** (1) `:129`
reads `B(icol)` AFTER `dlrgef` has shifted B down over the hole, so the saved
coefficient is the FOLLOWING column's -- harmless only because the corpus's
single user column is last. (2) The strip loop counts DOWN while the restore
reads `bu2/fx2/typ2` ascending against `getstr(Usrttl, ..., i)`, so with two or
more user columns coefficients pair with the wrong titles. **Both need a
MULTI-COLUMN user spec to show, and that spec is cheap -- it is on the board.**

**A divergence deliberately NOT shipped.** `x11regression{ variables=(td)
aictest=(user) }` -- fixed TD, USER test only -- makes the ORACLE abend at the C
iteration (`Irregular regression matrix singular because of Mon`, with the
printed design carrying six headers and ZERO rows); this engine runs it to
`OUTCOME: OK`. Hypothesis, unmeasured: the B iteration's `adrgef` restore and
the following `regvar` each put a copy of the user column into the model
`loadxr(T)` saves, so the C iteration fits a duplicated design. The spec was
written, measured and then DELETED rather than walled -- a wall would have made
the outcomes agree for a reason unrelated to the oracle's own, and would mask a
real singularity if the port ever grew one. On the board with the measurement.

**The gate caught its own bookkeeping.** `test_aictest_savelog.py` listed
`aictest.xu*` as UNOWNED, correct while the parser refused the token. The moment
the engine emitted the keys, the both-directions key-set assert failed in the
*extra-in-engine* direction -- the half that usually looks redundant. Moved to
`OWNED_X11`.

## This session, part 13: the two-column user spec -- and the extreme-value method was being chosen in the WRONG PLACE

Board item 1 was billed as confirmation work. It cost an increment, because the
spec walked into a live wrong-numbers path that had nothing to do with x11aic.
`docs/M5_PORT_NOTES.md` entry 62; the durable pieces:

**Entry 61's hazard 1 is UNREACHABLE, and now proved rather than assumed.**
`dlrgef.f:75-77` copies `noldc-1-endcol` elements -- zero when the deleted
column is the last -- and the user columns are ALWAYS the trailing block
(`gtxreg.f:183` adds `variables=` inside the argument loop, `:733-743` appends
the user columns after it, `x11aic.f:496-521` re-appends them last). The strip
loop counts DOWN, so each user column IS last when deleted. Not a defect.

**CB-36 is `active`.** `editor.f:1690-1716` assigns `rtype` ONLY on a
`Rgxvtp==PRGUTD` column and READS it only in the `ELSE IF`, so on the columns
that read it, it is uninitialized or holds the previous user-TD column's
`usertype=`. The read is `rtype.ge.PRGTUH`; PRGUTD (57) clears it, PRGTUD (18)
does not. So **`usertype=(td user)` declares the SECOND column the holiday
group** -- and `Holgrp>0` disqualifies the 2.5-sigma clip, putting the whole run
on automatic AO outlier identification. Two specs differing only in that one
line: 0 AO columns vs 7, and every row of b16/c16/d10/d11 different. A genuine
`usertype=holiday` column, the case the arm was written for, never fires it.

**Three port gaps behind it, all parsed-but-unread:**
1. **`Nusxrg` was a LOCAL** in `gt_x11regression` -- `gtxreg.f:265` writes the
   COMMON, and the editor loop is its only reader, so that loop could not have
   run even if it had been ported.
2. **The extreme-value choice was made in x11mdl, from the wrong inputs.**
   `editor.f:1727-1747` decides ONCE at spec-read off the PARSED x11reg model;
   this port re-derived it every x11mdl call and tested only `Xeastr`, the
   AIC-TEST flag. So an explicit `easter[8]` REGRESSOR in
   `x11regression{variables=}` took the 2.5-sigma arm where the oracle takes the
   AO arm -- 8 design columns against the oracle's 15, c16 100%% out, at
   `OUTCOME: OK`. **No corpus spec had an explicit x11reg holiday regressor**,
   which is why five earlier x11regression increments never saw it. Now
   `xrg_editor_setup` in `readers_spec.cpp`, gated by
   `extra/airline_x11regression-easter` at 4.7e-15.
3. **The `Fachol += Facxhl` fold (`x11pt2.f:299-308`) was walled** behind a
   blanket `Axrghl` refusal. It is the only arithmetic that flag turns on in
   x11pt2; ported, wall gone.

**Mutations: 199 / 101 / 9 for the block, the Sigxrg read and the Holgrp arm;
0 for the `Nusxrg` wiring and the Facxhl fold, both saturated** -- reported as
zeros, with why, in entry 62.

**The mutation harness lied first.** Its `powershell -Command` string put `\x`
in the middle of `code_projects\x13new` (Python ate the escape) and the shell
refused the script for execution policy, so every "mutated" run reused the
previous binary and reported 0-1. The harness now asserts `== testing ==`
appears in the build output before pytest runs. Same class as the `metrics.py`
failure CLAUDE.md's guardrail rule came from.

## This session, part 14: the x11regression td-or-holiday requirement

`gtxreg.f:891-897` refuses an irregular regression that adjusts for neither
trading day nor holiday nor a prior-TD weight set; this engine ran
`x11regression{ user= data= }` to `OUTCOME: OK`. Ported, and it dragged in two
never-set fields: **`Ixrgtd`/`Ixrghl` had no initializer** (gtinpt.f:447-448
sets both to 1, and `x11aic`'s TD-accept arm reads `Ixrgtd.gt.0` directly), and
**`noapply=` was consumed and discarded** -- the only writer of the zero into
them. `extra/airline_x11regression-user-notd` gates it; mutating the check off
loses that gate, the other two are saturated. `docs/M5_PORT_NOTES.md` entry 63.

**Still divergent in a FLAG:** `gtxreg.f:889` would set `Axrghl` from `Ixrghl`.
Not taken -- every holiday-carrying x11regression spec is bit-exact with it
false, and turning it on switches unmeasured x11pt2/x11pt3 folds. Belongs with
the CB-36 item.

## Open, in the order I would take them

1. **CB-36's stale-rtype arm is WALLED, and the wall hides a measured 1.1e-3.**
   Taking the arm -- setting Holgrp from the stale local, as the oracle does --
   puts the run on the right branch and then diverges: on a two-column
   `usertype=(td user)` spec the B iteration's irregular regression matches the
   oracle **coefficient for coefficient** (u1 -0.911968/-0.9120, u2
   0.536851/0.5369, AO1960.Mar -2.272510/-2.2725) and the C iteration does NOT
   (u2 0.330213 against 0.7441). Same design, same seven AO dates, TD
   coefficients agreeing to 4 dp -- so whatever moves is **between the B punch
   and the C fit inside the transparent xrgdrv pass**, not in x11aic. Rebuild
   the spec (`extra/airline_x11regression-aictest-user2` with `usertype` order
   reversed), drop the wall, and chase the B->C step. Entry 62 has the numbers.
   - Also measured there and NOT fixed: `x11regression{ user=... }` with no
     trading-day or holiday variable makes the ORACLE refuse (`Must adjust for
     either trading day or holiday in the x11regression spec`) and this engine
     returns `OUTCOME: OK`. One `inpter` in `xrg_editor_setup`, once someone
     checks the exact Fortran guard.
2. **What is left of `composite{}`**, now small: pseudo-additive (`Psuadd`) and
   the forced/rounded indirect series on the **agr3** path (`agr3.f:426-538` —
   ported for agr3s, still absent for agr3, and ungated on both for want of a
   `force{}` composite spec).
3. **`x11regression{reweight=}` (argidx 32 -> `Lxrneg`) is parsed and
   DISCARDED** -- found while porting Kswv==3, deliberately not fixed there
   because it is not a one-liner. `Lxrneg` is READ in two ported places
   (`gtinpt.cpp`'s negative-tdprior-weight clamp, `run_history.cpp:591`'s
   fixreg check), so both see a permanently-false flag; honouring it also needs
   `editor.f:1640-1667`'s fixed-coefficient check and `x11mdl.f:575-626`'s
   daily-weight reweighting, neither ported. Wire the parse, port or wall both
   readers, and gate a spec with a NEGATIVE tdprior weight.
4. **A composite whose components carry a residual peak**, to gate savpk's real
   `.dir`/`.ind` split — only the degenerate branch runs today.
5. The amdfct out-of-sample-backcast-with-outlier corner (0.2% out, measured
   and walled); `spectrum{altfreq=yes}` pending CB-30; `history{outlier=auto}` /
   `x11outlier=no` / `additivesa=`; the slidingspans `chs` per-span prior phase;
   `pickmdl{aictest=(user)}` (needs `usraic.f`/`chkchi.f`); the `!Hvmdl`
   no-model cleanup (`arima.f:476-527`).

## Environment notes

**`codex:codex-rescue` cannot return findings to this conversation, structurally
— stop dispatching it and waiting.** It is a ONE-SHOT FORWARDER: it launches a
Codex background task, is prohibited from calling `status`/`result`/`cancel`, and
returns only the launch handle. Resuming it with `SendMessage` does not help; it
says so itself. The findings exist only under `/codex:status <handle>` +
`/codex:result <handle>`, which **the user must run** — they are outside the
subagent's command set and outside mine. Measured 2026-08-01: two agents
dispatched, one returned a bare handle, the other returned real findings only
because it answered inline instead of forwarding. Budget accordingly, or do the
work directly.

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
