# Session handoff — 2026-07-30 … 2026-08-01 (`pickmdl{}` + `regression{aictest=}` CLOSED; the SEATS forecast decomposition CLOSED; the WHOLE `aictest.*` savelog surface ported and gated; `x11aic.f`'s trading-day branch ported; the Picktd-flip corner FIXED; `x11regression{user=}`, the `Kswv==3` prior-TD route and `x11aic.f`'s USER branch ported -- x11aic CLOSED; `x11regression{span=}` CLOSED both halves; the parity suite swept for blind gates -- which found a SEATS decomposition 8.1e-7 wrong; **`slidingspans{}` CLOSED bit-exact on every table it ships and `_KNOWN_GAPS` is empty** -- `Setpri` is editor-only, the spans CHAIN through `arima.f:1430`, and `fixreg=` fixes nothing in the oracle)

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
| `python -m pytest tests/parity -q -n 8` | **<!--x13:parity_pass-->6925<!--/x13--> passed / <!--x13:parity_fail-->0<!--/x13--> failed / <!--x13:parity_skip-->751<!--/x13--> skipped** (~86s) |
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
branch alone). The branch is on the remote at `ccbe7bbe` with local tracking set, and
`main` is an ancestor of it (`HEAD..main` == 0) — so `main` can fast-forward
whenever wanted, with no merge commit and nothing discarded. **The remote is
now 11 commits behind: everything from `119af354` (gtxreg) through this
session's gate sweep is local only.** Push when convenient.

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
`<!--x13:parity_pass-->6925<!--/x13-->` -- and `--write` maintains it while
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

## This session, part 15: CB-36 CLOSED -- and x11ref does not classify by Rgvrtp

Part 13 walled CB-36's stale-rtype arm because taking it left c16 1.1e-3 out.
The wall is gone; the cause was in neither editor nor x11aic.
`docs/M5_PORT_NOTES.md` entry 64. The durable pieces:

**Bisection first.** Switching the two flags editor sets (`Axruhl`/`Axrghl`)
independently changed nothing -- all four combinations gave the same 1.084e-3 --
so it was the BRANCH, not the flags. Four probes then ruled out everything the
CB-36 spec shares with specs that pass: a real holiday group under Otlxrg
(4.7e-15), plus a user column (5.0e-15), plus an aictest (5.0e-15), plus a
PRGUTD user column (5.1e-15). All green.

**The shape of the error named the cause.** b16's per-row difference divided by
u2 took exactly three values -- `-b_u2/28.25`, `-b_u2/31`, `-b_u2/30`. The
engine's Ftd was short by `b_u2 * u2 / Xnstar`: the oracle puts the SECOND user
column into the trading-day factor and this port did not.

**x11mdl.f:531-540 -- x11ref does NOT classify by `Rgvrtp`.** x11mdl builds a
local `rtype` first: a column carrying the generic PRGTUD takes its effective
type from the declared `usertype=` list (`loadxr.f:76` copies
`Usxtyp -> Usrtyp`). So `usertype=(td user)` puts BOTH user columns in the TD
factor -- u1 because its Rgvrtp is PRGUTD, u2 because its rtype is read as
`Usrtyp(1)`, u1's declared type. `iusr` advances only on PRGTUD columns while
Usrtyp is indexed by user-column number: off by one, same as CB-36, reproduced.

**Third member of one family.** editor.f:1710's stale rtype, x11aic.f's
descending `bu2/typ2` fill, and this remap are all *an index that tracks one
kind of column used to read an array indexed by another*. When something in
x11regression looks off by a column, look for that.

`extra/airline_x11regression-aictest-user2swap` is back and bit-exact, so CB-36
is pinned by a gate rather than a wall. Mutations: the CB-36 arm 14, the rtype
remap 11, the remap's `++iusr` 1. WALLS 22 -> 21 gaps.

## This session, part 16: the block gtxreg.f:828-878 never had, and what the Ixreg fix uncovered

`docs/M5_PORT_NOTES.md` entry 65. Started as entry 64's leftover one-line
refusal and turned into fifty lines of unported `gtxreg.f` plus two
silent-wrongness cases behind them.

**The missing block reads the WORKING regARIMA COMMON**, which is why it was
easy to miss: gtxreg builds the irregular-regression model into
`Grpttl`/`Grp`/`Nb` and only `loadxr` moves it into `Xrgmdl`. The port has to
sit between this port's `rmlnvr` call and its `loadxr` call. Four pieces --
the `usertype=`-needs-a-user-group refusal (:833-847), the single-`usertype=`
broadcast (:849-855), `regfix`+`Userfx` (:858-877), and `otsort` (:880, still
not ported).

**`gtxreg.f:883` was `Nbx>0` here, not `Nb>0 .or. Xeastr .or. Xtdtst>0`.** An
aictest carries no column of its own, so `x11regression{aictest=(easter)}` with
no `variables=` left `Ixreg` at 0 -- and `x11parts.cpp:529` carried a comment
asserting "Ixreg==0 with Axrgtd cannot occur". It occurred. That combination
hit an unrelated x11pt2 wall, which is the ONLY reason it was not silent, and
fixing Ixreg removed the wall and exposed both aictest-without-variables specs
running to `OUTCOME: OK` with wrong numbers. Both are now refused; see board
item 1 for the two open questions.

**`Noxfac` had no writer at all** -- `x11parts.cpp:572` read a flag nothing
set. `gtxreg.f:899` plus its `noapply`-with-`umdata` refusal are now ported,
both inert while `umdata=` is unread.

Standing shape worth carrying: **a comment asserting a state "cannot occur" is
a claim about a port, not about the Fortran.** That one was true of the oracle
and false here, and the thing that made it false was a dropped disjunct three
files away.

## This session, part 17: x11regression{span=} was never read, and neither were the three fields behind it

`docs/M5_PORT_NOTES.md` entry 66. Board item 3 was `editor.f:1972-1976`'s
second `Ixreg` promotion; it turned out to rest on an option this port did not
read at all.

**Three COMMON fields had readers and no writer.** `Begxrg`/`Endxrg`
(`run_history.cpp:679`), `Fxprxr` (`run_history.cpp:763`/`:822`) and `Xdsp`
(`xrgdrv.cpp:38`) were all permanently 0, and `span=` fell through
`gt_x11regression`'s `consume_value`. Measured on the airline series: the
oracle moves d11 1.3e-2 for an explicit narrower span and 1.6e-2 for the
`0.per` form; this engine returned the unrestricted answer both times at
`OUTCOME: OK`.

Ported: the arg, the NOTSET defaults, the `0.per` end-date form and its
`Fxprxr`, the `chkcvr` coverage refusal, `Xdsp`, and the second promotion --
which goes in `gtinpt.cpp` next to the first because the oracle runs editor
after ALL specs and `Khol` may not be parsed yet when `x11regression{}` is.
Note the ORDER: editor's block is `IF(Ixreg.eq.1)` and gtinpt.f:1201 already
promoted on `lmodel`, so with an `arima{}` present `Xdsp` is never computed.

**Five mutations, five zeros, and that is the finding.** Every span that
matters is behind one of the two new walls, so the only gateable spans are the
ones resolving to the series span -- where ported and discarded code agree by
construction. A `history{}` spec was built to give the `0.per` arm teeth via
`revdrv.f:500-503`; it did not, so it was DELETED rather than committed with a
coverage claim measurement had refuted.

**And a wall the inventory could not see.** The no-model refusal was first
written as a bare `errhdr`/`writln`/`abend`; `walls.py` counts refusals by
HELPER NAME, so it reported 23 gaps where the truth was 24. Standing rule, same
family as the `metrics.py` failure that put `test_doc_tooling.py` in the parity
suite: **a guardrail that is not inventoried is not a guardrail.** After adding
a refusal, run `walls.py --write` and check it is actually listed.

## This session, part 18: fit narrow, apply wide -- and a calendar array indexed from the wrong thing

`docs/M5_PORT_NOTES.md` entry 67. Entry 66 read `x11regression{span=}` and
refused to act on it; this ports the half of the narrowing that is a span
narrowing (`x11mdl.f:113-118` + the `:512-528` setspn restore and regvar
rebuild -- fit on the regression span, build the factor on the full one) and
leaves the half that is a pointer mutation refused.

**The trap, and it is a C++-port-only trap.** The first working version came
out 4.6e-2 on d11, WORSE than ignoring the option. Cause: `tdset`. The oracle
calls it ONCE, from `editor.f:2240`, over the whole `[Pos1bk,Posffc]` buffer;
this port issues it inside `x11mdl_td`, which was harmless until the span
moved -- and then feeding it the narrowed `Begspn` slid `Xnstar`/`Xn` by nbeg
periods against a factor still indexed from the buffer. **A calendar array
indexed from the BUFFER cannot be built from a date describing the SPAN.**
More generally: every oracle one-shot call this port has relocated into the
routine that needs it is fine until some caller mutates what the original call
site read. Worth a sweep of the others some day.

The restore is a `SpanGuard` destructor as well as an explicit `setspn`, so an
early return cannot leave Begspn/Nspobs narrowed -- that is the span-replay bug
this subsystem has paid for four times already.

**And the argument for lifting walls.** Entry 66's mutations were five zeros,
structurally: everything that mattered sat behind a wall, so nothing was
gateable. Lifting half of one wall turned three of them into 20s. **A wall
costs the coverage of everything behind it, not just the feature.**

## This session, part 19: the OTHER half of the span -- and a gate that named its cases by hand

`docs/M5_PORT_NOTES.md` entry 68. Entry 67 ported the START narrowing and
refused the END; this closes the END, and the two are genuinely different
mechanisms. A late START narrows Begspn/Nspobs inside x11mdl. An early END is
applied by `xrgdrv.f:151-158` BEFORE x11mdl runs, as a Posfob/Posffc
shortening plus an Endspn move that leaves Nspobs alone -- so x11mdl's own
`nend` is zero and only its Kpart==3 restore ever sees Xdsp. WALLS 24 -> 23.

**Three things worth carrying forward.**

1. **`tdset` again, the END this time.** Entry 67 paid for feeding it the
   narrowed span START; this paid for stopping it at the shortened Posffc,
   which left `Xnstar` zero over the C iteration's last Xdsp rows and NaN'd
   the factor. A calendar array indexed from the buffer needs the buffer's
   start AND its end. Any other relocated one-shot call is suspect the same
   way.

2. **`Xdsp` goes NEGATIVE.** It is a raw `dfdate` result, and every
   `history{}`/`slidingspans{}` replay has Endxrg later than the span end.
   Unclamped in the Faccal stash length it shortened the stash by 71 points
   and broke 24 history gates. **A quantity the Fortran only ever tests with
   `.gt.0` is not thereby non-negative** -- and this port STORES it where the
   Fortran mostly consumes it inline.

3. **The oracle's editor runs BEFORE xrgdrv; this port's two stand-ins run
   after.** `x11ari.f:149`'s setxpt is model-failure-only, so the oracle's
   main X-11 inherits whatever pointers xrgdrv left -- including a
   deliberately NARROW `Nofpob` (its own B 1 table prints 132 rows on a
   144-point span). Both `run_pre_model` and `x11_prestage` now capture and
   re-impose the counters around their geometry call
   (`xrg_geometry::capture/restore`). Skipping the call outright is NOT the
   same thing -- it also drops `Lsp`/`Begbak` and the d8b year labels come out
   as `1* 5z 11*`.

**And the gate finding, which is the reusable one.**
`tests/parity/test_x11regression_tables.py`'s `CASES` was a three-element
literal, so the `xrm`/`b16`/`c16` gates had never been shown ANY of the six
`x11regression{span=}` specs -- four mutations came back 0 for that reason and
not from saturation. **The same file already carried a note about this exact
defect for `AIC_CASES`, one list over, dated 2026-07-31.** Auto-discovering
CASES immediately failed three specs for a real bug it had been hiding: the
`.xrm` rows were dated from Begspn where `savmtx.f` is handed **Begxy**, so
every row of a late-starting design was labelled `nbeg` periods early. Row
counts were right; only a date-keyed comparison sees it.

**Worth a sweep:** grep the parity suite for other hand-written case lists.
Two have now been caught in the same file; there is no reason to think it is
the only one. (Done -- part 20.)

## This session, part 20: the sweep -- three more blind gates, and the SEATS decomposition behind one of them

Entry 69. The sweep part 19 asked for, and it paid immediately.

**Do not grep for literals.** Most literal lists in this suite are TAG lists
(which tables to compare) and are deliberate. The question is which
`(spec, table)` pairs the suite actually compares, so ask pytest:
`--collect-only -q`, keep the `[...]` ids, cross them against every golden on
disk, and report anything with a golden and no id. ~20 lines, and it cannot go
stale because both sides are derived. One false positive to know about: the
`.xrm` gate parametrises on the spec alone, so its ids carry no table name and
every xrm golden looks uncovered.

**The engine bug it found.** `test_seats_tables.py::_discover` scanned
`generated/` only and required `base.endswith("seats")`. The four hand-authored
`extra/airline_seats-*` specs match neither half, so their s10-s18 goldens were
compared to nothing at all. Three were bit-exact; **`airline_seats-history` was
8.1e-7 out on all four tables.**

`history{}` re-estimates per span (`slidingspans{}` runs fixmdl, which is why
the spec beside it was clean), leaving each span's ARMA coefficients in
`/mdldat/`'s `Arimap` -- which `seats_decode_model` reads. So the canonical
decomposition `tools/x13run_seats.cpp` rebuilds AFTER `run_seats` returns was
the last span's, under the main run's dates. Fifth appearance of the
punch-before-the-span-drivers seam. Fix: `ctx.model` + `ctx.mdldat.arimap` join
run_seats.cpp's restore set. Note the shape of what was missing -- the set had
every published OUTPUT the replay overwrites and none of the estimated MODEL
they are derived from. Save `/mdldat/` field by field: a whole-COMMON copy is
~1.5 MB (Armacm/Xy/Matd) and overflows the stack (`0xC00000FD`).

The decisive measurement was one line of spec surgery: delete the `history{}`
block, rerun against the SAME golden, watch every table drop to ~5e-15.

**Two coverage holes with no bug behind them** (all measured, all bit-exact):

* `test_x11_tables.py` discovered on `all(_CORE_TAGS)` with `b1` in
  `_CORE_TAGS`. Eighteen specs ship the four D tables and no `b1`, so that one
  tag excluded each of them ENTIRELY -- the whole `pickmdl-*` family,
  `slidingspans-td`, `noapply-*`, `reg-tcrate`, `outlier-tcrate`,
  `fcst-lognormal`, `reg-eastermeans`, `outofsample`. It also scanned
  `generated/` only while feature gates cover `extra/` piecemeal by feature, so
  a spec on no feature front fell through both.
* `test_x11_tdprior_tables.py`'s three-element literal, one spec stale:
  `airline_x11regression-tdprior-td`'s `a4` -- the one table unique to that
  file -- reached no gate.

**The rule.** A discovery predicate is a hand-written case list that has
learned to hide. `endswith("seats")` and `all(_CORE_TAGS)` read like generic
discovery and are as brittle as a literal with none of the visibility -- a
literal at least shows you its length. Every discovery in this suite now
carries a floor assertion (`test_*_cases_discovered`), because the failure mode
is a SHRINKING parametrisation and a parametrisation that shrinks to nothing
reports green.

Suite 6219 -> 6468 passed, 0 failed, 0 xfailed, 492 -> 671 skipped. Mutation:
dropping the `Arimap`/`ctx.model` restore fails 5 gates; before this increment
it failed none.

## This session, part 21: the Easter AIC window set -- decided in the wrong phase, behind a wall guarding the wrong condition

Entry 70. Board item 1 said to measure the `aictest=(easter)` AICC gap before
touching `editor.f:1760-1846`, because it might be the more general defect. It
was two defects, neither of them that block, and one was LIVE on a route no wall
covered.

**The wall was keyed on `Nbx == 0`.** Wrong condition. `x11aic.f:112-143`'s
strip loop deletes the Easter columns whenever `Xeastr` is on, so
`variables=(easter[8]) aictest=(easter)` reaches the i==1 baseline with exactly
the design an empty `variables=` would have -- the oracle returns the same
-734.3172 for both, which is what identifies the real trigger: **no surviving
trading-day group**. That spec has `Nbx == 1`, sailed past, and returned
`OUTCOME: OK` with four AICCs where the oracle prints two.

**Defect 1, PORTED.** `editor.f:1577-1591`: when `variables=` NAMES Easter
regressors the AIC test sweeps THEIR windows -- `Neasvx = endcol-begcol+2`,
`Xeasvc(2..)` read back out of the column TITLES (`getstr`, then `ctoi` past the
`[`). Only with no Easter group does the default `{0,1,8,15}` apply. This port
had the default half only, **in the `aictest=` parser**, where the question
cannot be answered: `variables=` may not have been read yet, so `Easgrp` is
unknown. That is why the oracle decides it in the editor. Moved to
`xrg_editor_setup`, where `easgrp` is already computed eleven lines above.

Worth naming as a class: the value was read, and was correct for every gated
spec. **The defect was computing it in a phase that could not see its own
precondition** -- and a single-branch implementation of a two-branch Fortran
`IF` looks exactly like a working one until something takes the other branch.

**Defect 2, still walled -- and it is the AO half.** `xeastr` suppresses
`editor.f:1727`'s `Sigxrg=2.5` default, so these runs take the `Otlxrg` branch
and do AUTOMATIC AO IDENTIFICATION on the irregular (the oracle's `.out` adds
`AO1960.Mar`, t=-5.50). The i==1 "baseline" is therefore not an empty fit at
all. With a TD group present the two agree bit-exact; with none they do not, and
that is where the ~7.6 sits. Wall rewritten to name that condition.

**The gateable route is not the broken one.** `variables=(td easter[8])`: the
strip loop removes Easter, TD survives, the auto-AO arm is never reached -- and
`Easgrp > 0` still exercises the window set. Bit-identical on both AICCs and the
chosen window. New spec `extra/airline_x11regression-aictest-easter8`.

Mutations: reverting the `Easgrp>0` arm fails **17** gates. The wall is **0**
and is reported as 0 -- a walled route cannot be corpus-gated the usual way,
since the engine refuses where the oracle succeeds. Verified by hand instead:
fires on `variables=(easter[8])`, not on `variables=(td easter[8])`.

Suite 6468 -> 6486 passed, 0 failed, 0 xfailed; WALLS 23 -> 24 gaps.

## This session, part 22: the no-model OLS prior TD, and a restore that was compensating for a hoist

Entry 71. Board item 3, closed. `x11ari.f:88-95` calls `xrgdrv` under
`IF(Lx11)` alone -- `Lmodel` is only passed THROUGH to `ssprep`/`restor` -- so
the OLS prior-TD pass runs on both paths. This port drove it from
`run_pre_model`, which runs only with a model, so a no-model spec that promoted
`Ixreg` skipped it. It was walled, so the outcome was a refusal, not wrong
numbers.

**Measured first.** `x11regression{ variables=(td) span=(1949.01,0.12) }` with
no `arima{}` moves the oracle's d10 9.2e-3 / d11 8.3e-3 / d12 8.4e-3 / d13
1.6e-2 against the same spec without `span=`. (`0.per` is the only writer of
`Fxprxr`; `editor.f:1976-1978` promotes on it. `0.12` on a December-ending
series resolves to the series end, so `Xdsp` is 0 -- the promotion is all this
route is for.)

**The port is a placement.** Nothing in `xrgdrv` changed. The call goes into
`x11_prestage` at x11ari's own point: after `x11int`, before `x11pt1`. That
order is the content -- the transparent pass reads `Sprior`, which on the
no-model path exists only once `adjsrs` has been copied by `x11int`.

**Then the interesting half.** The calendar factor came out bit-exact (b16, c16
and the TD part of d16 at 5e-15) and the SEASONAL factor was 0.67% out -- d10
and d16 carrying the SAME relative error, which is what says the calendar half
is right. It was `Ksdev`. `xrgdrv.cpp` saved and restored it alongside `Lterm`;
`restor.f` restores `Lter`, `Ktcopt` and `Tic` and NOTHING else, so the restore
is not in the Fortran. Deleting it fails **362** gates -- it compensates for the
MODEL path's HOIST (the call is moved out of x11ari, ahead of the editor
stand-in that then sets `Kersa=0`). The no-model call makes no such move and
must not carry the compensation. New `at_x11ari` parameter; it names the CALL
SITE, not the spec.

Standing shape: **a compensating restore is invisible while every gated spec
goes through the path it compensates for.** Ask of any save/restore here
whether it mirrors the Fortran or patches a rearrangement -- only the second
kind has to be re-derived per caller.

**Gated:** `extra/airline_x11regression-nomodel-priortd` (new, auto-discovered).
Mutations: disabling the call fails **18**; restoring `Ksdev` unconditionally
fails **17**; never restoring it fails **361**. WALLS 24 -> 23 gaps. Suite
6486 -> 6508 passed, 0 failed, 0 xfailed.

## This session, part 23: a stand-in for `restor` that restored less than `restor` does

Entry 72, and it closes the board item part 22 had just opened. With no model
the oracle's `transform{function=log}` is a no-op for every X-11 table;
this engine agreed on a bare `x11{}` run and did not on an
`x11regression{ variables=(td) }` one -- d11 1.2e-2, d13 1.6e-2, b16/c16
1.5e-3, at `OUTCOME: OK`.

**The named candidate was wrong, and killing it was the cheap step.** Part 22
pointed at x11pt2.f's `goodlm` gates. Forcing `goodlm` false moved nothing;
neither did disabling the makadj/tdlom block outright.

**A state dump settled it.** One `fprintf` of the /prior/, /adj/, /picktd/
scalars on both probes: `priadj=4 kfmt=1 picktd=1` with the log transform,
`priadj=0 kfmt=0 picktd=1` without. The engine was applying a LEAP-YEAR PRIOR,
and `Picktd` should have been 0 on both.

**The chain.** `variables=(td)` inside `x11regression{}` sets `Picktd` via the
same `adpdrg.f:642` line the regARIMA parser uses. `gtinpt.f:804` snapshots it
(F), `:818` gtxreg sets it (T), `:830` loadxr parks the x11reg copy in `Pckxtd`
-- which is what `xrgdrv`/`x11mdl` read -- and `:832` restor puts the WORKING
flag back to F. `gtinpt.f:999-1032` then reads it, AFTER :832, under
`dpeq(Lam,ZERO)`, and calls rmlnvr. This port's stand-in for that restor
(`xrg_clear_working` + the parked store) never touched `Picktd`, so an
x11regression-only `td` looked like a regARIMA one.

Three conditions coincide, which is why nothing caught it: a log transform, a
`td` that lives ONLY in `x11regression{}`, and no regARIMA model to mask it.

**The class, third appearance.** `xrgdrv` already carries a hand-written
`Ncusrx`/`Nrusrx`/`Usrtyp` restore for the same reason. Where this port stands
in for `restor`, the stand-in restores a SUBSET, and the omitted fields are
invisible until something downstream reads one. Entry 72 carries the full
`restor.f` field list and which stand-in now covers each -- redo that audit
whenever a new writer appears inside a block a `restor` brackets.

**Gated:** `extra/airline_x11regression-nomodel-logtd` (new), pinning the
oracle's actual invariant as a pair with `-nomodel-priortd`. Mutation: dropping
the restore fails **20** gates. Suite 6508 -> 6530 passed, 0 failed, 0 xfailed.

**Observed once, not reproduced:** one `-n 8` run had
`airline_pickmdl-backcast-oos` exit `0xC0000005` in `x13run_m3`. Passed
serially and on the next parallel run; its spec has no `x11regression{}`, so
this increment cannot reach it. Logged rather than dismissed -- the harness
carries large stack-resident arrays and this project has hit `0xC00000FD` once
for that reason.

## This session, part 24: the routine named "print" whose job was `abend`

Entry 73, and it closes board item 2. `x11regression{ variables=(td)
aictest=(user) }` makes the oracle stop with `ERROR: Irregular regression
matrix singular because of Mon.`; this engine published a full seasonal
adjustment at `OUTCOME: OK`.

**Entry 61's hypothesis was wrong on both counts** -- it guessed a duplicated
user column at the C iteration; the abend is at **B** (`x11mdl.f:253`,
`Kpart.eq.2`) and the design is EMPTY. Disproving it took one run, which is the
argument for writing hypotheses down.

**Cause.** With `Xtdtst==0` and `Xeastr==F`, x11aic skips both blocks that call
`regvar`, so :463-464's `regx11` fits whatever design is resident -- and nothing
built one (x11mdl's first `regvar` is at :388, after the x11aic call at :253).
Two row counts diverge and must not be conflated: `prterx.f:52` reprints `Nrxy`
rows (zero) while `regx11.f:49-50` fits `Nspobs` (144). The singularity is in
`Xy`'s CONTENT.

**The port already detected it.** Instrumenting showed `nrxy=0`, `xy` all zeros,
and `regx11` returning **false** -- matching the oracle exactly. It had nowhere
to say so: `prterx.f` was unported, because it lives in the `prt*` family this
port defers wholesale. Deferring the print deferred the stop, at all seven live
`regx11` guard sites.

**The class is swept.** Eleven `prt*` routines call `abend`; nine only on a
save-file-open failure (unreachable -- no save files here). The two real error
reporters are `prterr` (already ported) and `prterx`. Closed; entry 73 carries
the table so nobody re-derives it.

**Gated:** `extra/airline_x11regression-aictest-usersing` (new) via a DISCOVERED
`test_x11regression_abend` -- any x11regression spec whose blessed oracle `.err`
carries `ERROR:`, with a floor. The same predicate now excludes abending specs
from `CASES`/`AIC_CASES`. The spec joins `test_m1_parse`'s existing
`_POST_PARSE_FATAL` set (a post-parse abend is not a parse verdict); a second,
message-keyed mechanism was written and thrown away rather than duplicate that
ownership.

**Mutation:** 2 gates. The instructive half is that one of them,
`test_m1_parse::test_outcome_matches_oracle`, had existed all along. What was
missing was never a gate -- **it was a spec.** Entry 61 wrote this spec,
measured it, then removed it.

Suite 6530 -> 6535 passed, 0 failed, 0 xfailed, 681 skipped.

## This session, part 25: `Grpx(-1)` measured -- and the wall in front of it was too narrow

Entry 74. Board item 1's first half had been blocked since entry 65 on "decide
whether an OOB read gets reproduced, and on what evidence". The evidence is in.

**It is not undefined behaviour.** `COMMON /cx11rg/` declares `Clxptr(0:PB)`
immediately before `Grpx(0:PGRP)`, so storage association makes
`Grpx(Tdgrp-1)` with `Tdgrp==0` resolve to `Clxptr(PB)` -- a determined address
81 integers INSIDE the block. A probe compiled read-only against the vendored
headers (`tools/ref_grpx.f`, the `ref_*.f` pattern) poisons both arrays and runs
editor's own two lines: `begcol = 1080 = Clxptr(PB)`, alias confirmed.

`Clxptr(PB)` is `Colptr(PB)` (loadxr.f:38 copies all `PB+1` elements regardless
of how many are meaningful), which no realistic model writes, so it holds the
static 0; `Grpx(0)` is 1, `endcol` is 0, and `Xtdtst` flips 1 -> 3 -- `td`
becomes `td1coef`. Measured stable in the oracle across 1, 2 and 4
x11regression columns; the control (`td` present in `variables=`) reports `td`.

**What that found.** The wall guarding this block tested `Nbx == 0`, a PROXY for
the real trigger (no trading-day group). `variables=(easter[8]) aictest=(td)`
has `Nbx == 1` and went straight through: engine `aictest.xtd.reg: td`,
`aictest.xtd: yes`, AICC(td) -758.367; oracle `td1coef`, `no`, **-1732.145**,
ending `aictest: none`. Both `OUTCOME: OK`. Condition is now
`Xtdtst > 0 && no_td_group`. WALLS stayed at 23 -- widened, not added, which is
why a gap count is not a coverage measure.

**Still the user's call**, and it is now a policy question rather than an
evidence one: does this port reproduce a documented COMMON aliasing? Cost is one
explicit line -- the C++ arrays are separate objects, so the alias must be
written (`ctx.xrgmdl.clxptr(prm::PB)`), not inherited.

Suite 6535 passed, 0 failed, 0 xfailed, 681 skipped.

## This session, part 26: the alias ported (CB-37), and the two halves were never separable

Entry 75. Option B taken on entry 74's policy call: reproduce `editor.f:1786`'s
COMMON aliasing rather than stay walled. That meant porting
`editor.f:1760-1846` -- the agreement refusals, the flip, and the
generatability refusals -- not just the one read.

```cpp
const int begcol = (tdgrp == 0) ? xg.clxptr(prm::PB) : xg.grpx(tdgrp - 1);
```

Written explicitly, because the C++ mirrors are separate objects. Hardcoding
the flip was rejected on measurement (`Colptr(PB)` IS writable by a
79-regressor model -- insptr.f:54-55 / adrgef.f:363 -- so assuming 0 breaks
silently at the limit); co-locating the COMMON was rejected because it would
disarm `farray1lb`'s bounds check for every array in the block.

**THE FINDING, and it corrects this handoff's own board.** Item 1 listed the
flip and the AO gap as two independent sub-items. They are not separable:
`x11regression{}` requires a trading-day OR holiday regressor, so *no TD group*
forces a holiday, which makes editor.f:1727 pick `Otlxrg` (auto-AO) over
`Sigxrg=2.5`. Every spec that reaches the flip also reaches the auto-AO path,
whose AICCs are ~7.9 off. No spec isolates one from the other.

**Gated anyway, through the flip's one PARSE-time consequence.** On QUARTERLY
data the rewritten `Xtdtst==3` hits editor.f:1832 and the run is refused --
"Need monthly data to perform aictest for stock trading day", a message about
*stock* TD for a plain `td` request. Without the flip `Xtdtst` is 1, that arm
never fires, and `Sp==4` passes cleanly. Predicted from the Fortran, confirmed
against the oracle, and it cannot fire unless the aliased read did.

**A second proxy wall, one day after the first.** Entry 74 fixed a wall keyed
on `Nbx==0` (a proxy for "no TD group"). The AO wall beside it was keyed on
`Xeastr` -- also a proxy; the real trigger is `Otlxrg`. The moment the flip
stopped refusing `variables=(easter[8]) aictest=(td)`, that spec walked past
the AO wall too at `OUTCOME: OK`. Fixing one proxy exposed the next. Also: that
wall needed an `inptok` guard, because a wall placed after a refusal path is a
PORT artifact and must not add a second ERROR line to a spec the oracle already
rejected with one.

**Third gap, walled not ported:** `Xaicst` (editor.f:1802-1808) and `Xaicrg`
(:1811-1822) are READ by this port (x11reg.cpp:642/658 -> mktdlb/addtd) and
were only ever WRITTEN to their gtinpt defaults. Read-but-never-written. Now
walled under `Stdgrp>0` / `Xrgmtd`; on the board below.

Suite 6535 -> 6537 passed, 0 failed, 0 xfailed. WALLS 23 -> 24.

## This session, part 27: board item 1 CLOSED -- and it was never what it said

Entry 76. The "auto-AO AICC gap" was neither auto-AO nor an AICC gap. A
HOLIDAY-ONLY `x11regression{}` (`variables=(easter[8])`, no trading-day group)
never ran its transparent `xrgdrv` prior pass at all: B1 came back as the RAW
series, the irregular regression's automatic AO identification found nothing,
and the seasonal filter choice flipped with it -- at `OUTCOME: OK`.

**Five stacked defects**, each invisible until the one above was fixed:

1. `Axrgtd` is a PROXY and **four** guards used it (`run_pre_model`,
   `x11_prestage`, `xrgdrv`'s own entry test, `x11parts`'s Ixreg==3 restore).
   The oracle's condition is `x11ari.f:91`'s `IF(Ixreg.eq.2.or.Khol.eq.1)`;
   `editor.f:1722` CLEARS Axrgtd when there is no TD group.
2. `xrgdrv`'s entry test **returned true** -- a silent no-op, not a wall. That is
   why it survived: an unported path that fatals shows up the day a spec reaches
   it; one that returns quietly does not.
3. `Easgrp` was READ (`x11reg.cpp:1122` derives Holgrp from it) and never
   WRITTEN -- the editor computed it into a local. Holgrp 0 + no TD group sent
   x11mdl_td into `x11mdl.f:308`'s identity-factor NOTE return.
4. `gtxreg.f:889`'s `Axrghl=T` had been deliberately skipped, on the evidence
   that every holiday-carrying spec was bit-exact without it -- and the corpus
   had no HOLIDAY-ONLY spec, the only shape where it is load-bearing. Taken now;
   no gated spec moved.
5. `x11ref.f`'s `Tdgrp==0` arms were unported. The `Tdgrp>0` arm adds
   `Xn/Xnstar` to Fcal, which put the MONTH-LENGTH ratio into a holiday-only
   factor: February off by exactly 28/28.25.

`Tdgrp/Stdgrp/Holgrp` also became PARAMETERS of `x11ref_td` -- `pritd.f:44`
passes the literals `1,0,0` while `x11mdl.f:694/814` pass the COMMON (the
call-site rule from entry 71, hit again).

Both new specs are BIT-EXACT: every D-table at 1e-15, all shared udg keys
including the two AICCs the board item was about.

**The lesson, and it is about method not code.** The divergence was
engine-vs-oracle measured. It was never cheap-spec-vs-expensive-spec measured.
Deleting one line -- `aictest = (td)` -- reproduced the entire thing and named
the subsystem in one run. Entry 75 had even PROVEN the flip and the AICC gap
were inseparable by spec, and used that proof to justify not building the
cheaper spec. Inseparability says two features co-occur; it never says which one
owns the delta.

**Left open, honestly:** `x11ref.f:87`'s `IF(Holgrp.gt.0)` outer guard on the
Fhol fold is not reproduced -- adding it costs 54 gates, so on the vendored
binary the fold demonstrably happens even though `x11aic.f:64` clears Holgrp and
no visible path restores it. Open question, not a CB claim. Same for
`x11ref.f:19`'s `Trumlt`: declared, never assigned, not a dummy, in no COMMON,
read at line 88 -- an uninitialized local, reachable only with `Tdgrp>0`, where
the binary behaves as `.true.`.

Suite 6537 -> 6581 passed, 0 failed, 0 xfailed. WALLS 24 -> 23 (wall DELETED,
not widened). Mutation: restoring the `Axrgtd` proxy costs 39 gates.

## This session, part 28: board item 1 (`reweight=`) CLOSED -- plus three defects found on the way in

Entry 77. `x11regression{reweight=}` (gtxreg.f:553 -> `Lxrneg`) was parsed and
DISCARDED while THREE ported readers took the permanently-false flag as fact
(`editor.f:1511`'s negative prior-TD clamp in gtinpt.cpp, `x11mdl.f:578`'s
daily-weight reweighting, `revdrv.f:327`'s history reset). Oracle on-vs-off
first: a log-additive `tdprior` with a -0.5 weight moves `a4` 1.2e-2 at
1949.Jan. Engine-vs-oracle gave the same delta at `OUTCOME: OK`.

**The reweight had to move up a phase.** `x11mdl.f:610` writes back into `B`, so
every factor `x11ref` builds afterwards comes off the rewritten coefficients.
This port had the `Dx11` build BELOW its `x11ref_td` call -- harmless only
because its single consumer (the Kswv==3 combine) re-ran `x11ref_td` for itself.
Hoisted.

**Reaching it needed a constructed spec.** `Dx11 = 1 + B`, so a negative weight
wants a coefficient below -1. Additive mode (where `Dx11 = B`) is walled;
`editor.f:1640` refuses a FIXED coefficient below -1. What is left is the
DERIVED Sunday weight `1 - sum(B)`: fix five day contrasts high enough that the
estimated sixth cannot pull the sum back under 1. The window is one notch wide
and both edges are gated -- 0.39 reweights, 0.43 abends (`x11mdl.f:613-623`),
0.35 does nothing.

**CB-38, measured on the binary.** `editor.f:1655` is 71 characters and breaks
`when` across the fixed-form continuation, so the blank pad at column 72 lands
inside the word: the oracle prints `less than zero w hen specifying`.

**Three defects found on the way in, none about reweighting:**

1. **The computed GO TO was off by one.** `centeruser` (label 310, argidx 31)
   was dispatched on argidx **30**, which is `umtrimzero` (label 300) -- under a
   comment citing `gtxreg.f:537-543`, i.e. label 310. Comment and code disagreed
   with each other in plain sight. Both ways: `umtrimzero = seasonal` accepted at
   `OUTCOME: OK` where the oracle errors, and a real `centeruser=` discarded.
2. **`regfix()` was never called for the x11reg design** (`gtxreg.f:861`), while
   the block right below it -- headed "Iregfx from the b= fixings, then Userfx"
   -- READ the `Iregfx` it produces. `loadxr` copies it into `Irgxfx`, which
   `editor.f:1640`, `editor.f:1675` and `gtxreg.f:866` all test, so all three
   were reading the regARIMA model's fix state. Adding the call needed the
   matching `restor.f:69` snapshot -- the restor-stand-in trap, fourth time.
3. **`rmlnvr` ran a phase too late.** `gtxreg.f:186-192` strips Leap Year inside
   the `variables=` branch, so `Nb` is 6 when `:608` checks the `b=` length. This
   port ran it after the argument loop, `Nb`==7: a correct 6-value `b=` list was
   silently DISCARDED and a 7-value list the oracle rejects was applied. Both at
   `OUTCOME: OK`.

**Not gatable, and worth knowing why.** `gtxreg.f:609`'s count-mismatch message
starts with `ERROR:` but never touches `Inptok` -- the oracle prints it and
carries on. Both of the suite's DERIVED predicates (`test_m1_parse::_oracle_ok`,
`test_x11regression_tables::_oracle_abended`) read an `ERROR:` line in a blessed
`.err` as a rejection, so a spec for the 7-value direction reports two false
failures. The spec was dropped rather than bolt a name-list exception onto a
deliberately derived predicate.

Suite 6582 -> 6678 passed, 0 failed, 0 xfailed. WALLS unchanged at 23 gaps.
Seven new specs, all `extra/airline_x11regression-reweight*` plus `-umtrimzero`.

**One unreproducible event, logged not chased.** A single full `-n 8` run had
`x13run_m3.exe` on `airline_pickmdl-backcast-oos` exit 0xC0000005 with EMPTY
stdout and EMPTY stderr; the same test passes serially and in every subsequent
parallel run. Empty on both streams points at process creation under xdist load
rather than the engine. If it recurs, that is a real signal.

## This session, part 29: `slidingspans{}` and `x11regression{}` had never met

Entry 78. `slidingspans.cpp:395` skipped `setssp.f:353`'s `ssxmdl` under the
comment "out of scope, Nbx==0 always in this port" -- true when x11regression
was unported, false ever since. Nothing caught it because NO corpus spec
combined the two. Same shape as entry 76's `xrgdrv`: not a wall, not a gate, a
silent skip justified by a fact that expired.

`ssxmdl` turns out to be INERT on the spec (no `x11regression{span=}`, nothing
fixed, `Irgxfx==1`), so the claim was harmless here. It was never checked, which
is the finding.

**What the pairing did find:** `sfs` bit-exact across all four spans, every
D-table and b16/c16 bit-exact -- and `chs` wrong in **408 of 600 cells**, worst
5.0e+0. Per-span seasonal factors right, per-span CALENDAR factor wrong.

**A misattribution in a code comment, settled by two runs.** `chs` was already a
KNOWN GAP for `airline_slidingspans-td`, attributed to a per-span PRIOR PHASE,
and `slidingspans.cpp` claimed this family's `chs` was "the same" problem:

| spec | chs cells wrong |
|---|---|
| airline + slidingspans + x11regression + log | 408 / 600 |
| same, `x11regression{}` DELETED | **0 / 600 -- bit-exact** |
| same, `transform{function=log}` DELETED | 408 / 600 |

Deleting the feature the OTHER gap is about changes nothing. Two gaps, one
symptom, different owners. Both now recorded separately in
`test_slidingspans_tables.py`'s `_KNOWN_GAPS`, goldens committed.

**Ruled out, and labelled inert at its site:** `ssx11a.f:96-97`'s per-span
`Begxrg`/`Endxrg` move. This port had them frozen at parse-time values while
`Begspn`/`Endspn` moved underneath, so `x11reg.cpp:1097` measured each span
against the full series. Ported (`run_x11_span`'s `set_xrg_span` -- a CALL-SITE
switch, since `revdrv.f:500-511` uses different arithmetic for the same field)
and measured inert: 408/600 either way, including on a spec whose
`x11regression{span=}` makes `nbeg` positive so the precondition is genuinely
non-empty. Kept because it is what ssx11a does.

**Where the next session starts:** the engine's `chs` DOES respond to
x11regression (427 of 600 cells move when the spec drops it), so the irregular
regression IS running per span, on wrong inputs, landing nearer the no-TD answer
than the oracle's.

Suite 6678 -> 6702 passed, 0 failed, 0 xfailed. One new spec,
`extra/airline_slidingspans-x11regression`.

## This session, part 30: `fixx11reg` defaults to YES, and the fix's partner had already been measured and rejected

Entry 79. Board item 1 closed. The per-span calendar factor entry 78 named was
`slidingspans{fixx11reg=}` -- an option this port PARSED into `sspinp.ssxint`
and never read anywhere. It defaults to YES (`gtinpt.f:531`), so it was live on
every spec that never mentioned it, and entry 78's inertness check enumerated
only the arms a SPEC can switch on.

**How one run named it.** The spec saved two of the five span tables. Adding
`tds` and `ads` to the ORACLE's save list showed the per-span trading-day factor
byte-identical across all four spans and equal to the main run's `c16`: the
oracle does not re-estimate the irregular regression per span at all.
`fixx11reg=no` moves it (99.144951 -> 98.847324 at 1951.Jan), so the default is
load-bearing.

**The rule this earns.** `ssxmdl`'s fix does nothing unless the span re-enters
the irregular regression, which needs `ssx11a.f:93-94`'s `Ixreg` demote -- and
that demote was measured ALONE in an earlier session, found to take `sfs` from
bit-exact to 4.1e+0, and written up as "do not copy this". Correct number,
wrong conclusion: alone it makes each span REFIT what the oracle RELOADS. **A
feature measured with its partner missing measures the partner.**

Four more pieces came with it: `x11mdl.f:168-175`'s `B`-from-`Bx` seed;
`x11mdl.f:874`'s `ssrit` and `ssap.f:208`'s `mflag(Td)`, the ONLY producers of
the `tds` table on an x11regression run (the engine emitted none, and the gate
skipped it as "spec does not produce this tag" -- a parametrisation that shrank,
now floored by a discovery assertion); and, once the demote landed, the SIXTH
span-replay save/restore miss -- `b16`/`c16`/`.xrm` came out holding the last
span's 84 rows against the main run's 144.

Three ssxmdl arms are walled on their exact triggers rather than skipped
(`rmotss`, the `rvfixd`/`Irgxfx>=2` fixed-design arm, `bakusr`); `walls.py`
learned the new helper name. WALLS 23 -> 26 gaps.

`extra/airline_slidingspans-x11regression` now saves and gates **sfs, chs, ads
AND tds**, all bit-exact across all four spans. The `chs` KNOWN GAP entry is
DELETED rather than re-worded. The OTHER `chs` gap
(`airline_slidingspans-td`, the per-span prior phase) is untouched and still
open -- which is what entry 78's separation was for.

Suite 6702 -> 6706 passed, 0 failed, 0 xfailed.

## This session, part 31: the stock-TD abend, and an `ELSE` that pairs with a different `IF`

Entry 80. Board item 1 closed. `x11mdl.f:661-690` refuses a run whose STOCK
trading-day irregular regression would produce nonpositive multiplicative daily
factors. It was neither ported nor walled. Measured before porting, on airline +
`x11regression{variables=(tdstock[15]) b=(-1.5f ...)}`: the oracle abends, this
engine returned `OUTCOME: OK`.

**The finding is the PAIRING.** The first transcription put the block under the
`ELSE` of `:541`'s `IF(Havxtd.and.(.not.Haveum))` -- what the old handoff note
said and what the surrounding prose implies -- and produced a guard that COULD
NOT FIRE (`havxtd=1` on the very spec the oracle refuses). Walking the `END IF`
chain mechanically settles it: `:660` closes `:578`, **`:661`'s ELSE pairs with
`:546`'s `IF(igrp.gt.0)`**, `:690` closes `:546`, `:691` closes `:541`. The arm
means "an x11regression TD is present and the WORKING model has no Trading Day
group" -- i.e. a STOCK design. Count the block; never read the indentation.

It was caught only because a debug print was added when the branch stayed
silent. A slightly-off spec instead, and a dead branch ships looking correct.

**Also measured:** FORMAT 1070's `//` pairs emit EMPTY records, while `writln`'s
`lblnk` blank is `(' ',a)` -- two characters. The first version differed from the
oracle `.err` on exactly those two lines. Written straight to the channels and
compared byte for byte.

Two oracle oddities transcribed, not tidied: `:664` looks the group up in the
x11reg STORE and indexes the WORKING model's `Grp` with the result (same family
as CB-37, NOT filed as a Census bug -- no spec here reaches a state where they
disagree); and the `<=` test written as `B < -1 .or. dpeq(B,-1)`.

**Gated on BOTH sides**, which is the point: `extra/airline_x11regression-
tdstock-abend` fires, `extra/airline_x11regression-tdstock` (same design,
positive weights) must reach `OUTCOME: OK`. A refusal gated only where it fires
cannot tell "correctly refused" from "refuses everything". These are also the
first x11regression STOCK trading-day designs in the corpus. ABEND_CASES floor
1 -> 3.

Suite 6706 -> 6728 passed, 0 failed, 0 xfailed.

## This session, part 32: the sliding-spans NOTEs, and the channel nobody could read

Entry 81. First of the three ssxmdl arms entry 79 walled -- the
`x11regression{span=}` one (`ssxmdl.f:27-39`), which was PORTED but UNGATED
because no spec made `Begxrg` later than `Begspn`. New spec
`extra/airline_slidingspans-x11regression-span`: `sfs` 4.9e-15, `chs` 4.8e-15,
600 cells each, bit-exact first run.

**The arm's whole observable is an ABSENCE.** `Itd=-1` ("requested but not
analysed") makes `ssap.f:206-211` write no `tds` and no `ads`. The save-table
gate skipped an absent golden with "spec does not produce this tag" -- the same
sentence that hid the missing `tds` producer for months.

Two things fell out of taking that seriously:

1. **The `.err`/Mt2 channel was unreadable on a successful run.** Both harnesses
   dumped it to stderr only on FATAL, so every non-fatal NOTE and WARNING went
   into a discarded buffer. They now emit it between `===ERR===` /
   `===END ERR===`, x13run_m2's format.
2. **With it readable, `ssphdr.f:145-152` turned out to be unported** -- two
   NOTEs written to BOTH Mt1 and Mt2 saying the TD statistics are suppressed
   and why. `ssphdr` is 95% Mt1, i.e. the deferred `.out` print engine, and went
   with it; the 8 lines that are not have sat in `airline_slidingspans-td`'s
   blessed `.err` since that spec landed, unread. Same family as `prterx`
   (entry 73). Ported both arms, written straight to the channel (FORMAT
   2000/2001's `/` emits EMPTY records; `writln`'s `lblnk` is two characters).

Faithful-scope note recorded, not faked: the Fortran gates `ssphdr` on
`Prttab(LSSSHD).or.Savtab(LSSSHD)`, and this port has no print-table dictionary
at all, so the NOTE is emitted whenever the header stage is reached -- exact for
`print=all` and the default, over-emitting for a `slidingspans{print=}` list
that excludes the header.

**Both new gates were mutation-tested in both directions**: never-emit fails 2,
always-emit fails 5 (four of the seven discovered specs are NOTE-free, which is
what makes the gate able to tell a correct emitter from one that shouts), and a
`tds` guard that emits while demoted fails 1.
`test_slidingspans_notes_can_fail` keeps both sides of the corpus non-empty.

Also: `docs/M5_PORT_NOTES.md`'s contents list had stopped at 53 while the file
ran to 81. Rebuilt from the `## N.` headings themselves.

Suite 6728 -> 6762 passed, 0 failed, 0 xfailed.

## This session, part 33: `fixreg=` was parsed and dropped, and a mutation that passed

Entry 82. Board item 1's remaining ssxmdl arms -- one closed, one still walled
-- plus a third thing that was neither.

**`slidingspans{fixreg=}` was a parsed-but-unread option.** The wall meant to
cover it tested `Nssfxx`, which nothing in this port ever writes; the parser
fills `Nssfxr`, and that walked straight past. Measured on airline +
`slidingspans{fixmdl=no fixreg=(td)}`: the oracle writes neither `tds` nor
`ads`, the engine wrote 120 rows of each, at `OUTCOME: OK`.

Ported: `setssp.f:320-341`'s decode, `ssmdl.f:50-121`'s three arms
(`Nssfxr>0` / `Iregfx==3` / `Iregfx==2`), `ssxmdl.f:78-136`'s rvfixd + the
`Irgxfx>=2` group walk. The four flags are ONE quartet shared by both routines
-- ssmdl writes `Tdfix`/`Holfix` back and ssxmdl reads them -- so
`ssmdl_fix_model` now takes the pair by reference. `rvfixd` moved out of
`run_history.cpp`'s anonymous namespace into `core/src/regarima/rvfixd.hpp`,
unchanged; it has three call sites in the oracle.

**Recorded rather than faked**: `ssxmdl`'s `loadxr(F)`/`loadxr(T)` pair UNDOES
every store write its `rvfixd` makes -- only the walk twenty lines down reads
them -- and the same round trip leaves the regARIMA working model overwritten
by the x11regression design, which `setssp.f:356`'s `restor` only partly puts
back. This port does neither half and is self-consistent on every gated spec.
Divergence in waiting, not a defect with a witness.

**The mutation that passed.** Five mutations, one per arm; four failed loudly.
Disabling `ssmdl`'s `Iregfx==3` arm changed nothing -- on the spec written for
it, with every coefficient fixed. `Iregfx` was 2: `getreg`'s Leap Year splice
inserts a column into the `b=` list, `regfix.f:31` finds it valueless, and the
promotion to 3 never fires. `airline_slidingspans-regallfixed`
(`variables=(tdnolpyear)`, no leap-year column to splice) reaches the real arm,
and now mutation five fails on its own spec. New standing rule in `CLAUDE.md`.

**A third gap, isolated the way the rules require.** Four of the six new specs
fail `sfs`/`chs`. `airline_slidingspans-fixmdl-no` is `airline_slidingspans-td`
plus one line and fails the same way, so the owner is `fixmdl=no` +
`regression{}` (per-span RE-estimation), not the arms just ported. Recorded in
`_KNOWN_GAPS`; the goldens are blessed and waiting.

Also: `arima.f:936-960`'s fixed-coefficient NOTE is unported and now subtracted
from the golden side of the note gate by `_UNPORTED_NOTES`, with
`test_unported_notes_still_unported` to make sure the list cannot outlive the
gap. `prtmdl.f:174-177`'s `Nliter>200` NOTE is unported with no corpus carrier.

Suite 6762 -> 6893 passed, 0 failed, 0 xfailed. WALLS unchanged at 26 gaps --
one wall removed, one (`fixreg=(outlier)`) added.

## This session, part 34: `slidingspans{}` CLOSED bit-exact -- three gaps, three different owners, none where the note said

Board item 1, and it took the whole of it: the five specs entry 82 blessed and
parked, plus `airline_slidingspans-td`'s `chs`, which had been in `_KNOWN_GAPS`
for months. **`_KNOWN_GAPS` is now empty** -- all seven slidingspans specs gate
bit-exact on every table they ship (`sfs`/`chs`/`tds`/`ads`, four spans each,
worst ~5e-15). Full record in `docs/M5_PORT_NOTES.md` entry 83; the durable
pieces:

**1. `Setpri` is EDITOR-ONLY, and `run_x11_span` re-anchored it per span.**
`editor.f:851` is `Setpri=Pos1bk` and nothing repeats it. `x11int.f:53` copies
`Adj` into `Sprior` POSITIONALLY, and `Adj` is built once by `adjsrs` from the
editor (its only call site in the oracle) anchored at the MAIN run's `Begadj` --
so a pinned `Setpri` beside a `Lsp`-slid `Pos1ob` is exactly what keeps the
prior factors date-aligned. An instrumented `-O2` oracle showed `setpri 1` while
`p1ob` slid 25/37/49/61. Re-anchoring slid the factor series onto the span,
putting the leap factor of a February `1949 - span_start_year` away on every
February: **18 cells, all Feb/Mar, out by exactly 29/28 or 28/29, sign following
the leap year.** The old note in the test file named the right subsystem and
then described the defect as the mechanism.

**2. `arima.f:1430`'s `CALL ssprep` is unconditional, so the spans CHAIN.**
`restor` resets `Arimap` from `Ap2` before each span, and every `arima` call --
including each span's, via `sspdrv.f:180`'s `x11ari` -- rewrites `Ap2` with what
that span converged to. The port had the main-run snapshot only, so every span
restarted from the main model. **Span 1 bit-exact, spans 2-4 out by
6.2e-07/3.9e-07/3.0e-08**: the signature of a value that is right the first time
and stale afterwards, and it reads exactly like an estimation-tolerance floor.
`ssprep_snapshot` gained a `capture_saved` flag because `ctx.saved.ksdev0/
lterm0/nterm0` are NOT ssprep.cmn fields -- they are parse-time stand-ins for an
editor block, and re-taking `Ksdev` per span would re-open the 3.45% span-1 gap.

**3. `slidingspans{fixreg=}` fixes NOTHING in the oracle, and entry 82 made it
work.** `ssmdl.f:53`'s `rvfixd` writes only the live `Iregfx`/`Regfx` -- it does
not mirror them into `Regfx2`/`Irfx2` the way `ssmdl.f:342-352` mirrors
`Ap2`/`Fxa` for `fixmdl=yes` -- so `restor` puts the all-free flags straight
back and no span sees a fixed coefficient. Measured: the instrumented oracle's
per-span `Arimap` AND `B(1..7)` on `-fixreg-td` are byte-identical to
`-fixmdl-no`'s, and their two blessed `sfs` goldens differ in ZERO lines. The
mirror (added by analogy with `fixmdl` and `history{fixreg=}`, both of which DO
write the snapshot) put `sfs` 8.4e-03 out on span 1. **A trap list is a list of
places to look, not a list of things that are true.**

**New spec, and it settles a reading rather than adding coverage.**
`extra/airline_slidingspans-fixmdl-clear` is the only route to `Ssinit==2`
(`INTDIC` is `'no','yes','clear'`, `Ssinit=ivec(1)-1`), which drives
`sspdrv.f:130-143`'s `DNOTST` reset. Read as "after the span" that block is dead
code; read as "before this span's estimation" it is live, and only the call
order -- it sits between `ssx11a`'s trailing `restor` and `x11ari` at `:180` --
decides. The spec gates bit-exact, its `sfs` golden differs from `fixmdl=no`'s
in 240 lines, and moving the block fails 2 gates.

**Mutations, each failing a different set:** drop the per-span `ssprep` **12**;
re-anchor `Setpri` **7**; mirror `rvfixd` into the snapshot **2**; move the
`Ssinit==2` block after the span **2**; drop the `Chx2`/`Chg2`/`Acm2` half of
`ssprep`/`restor` **0**. The zero is REPORTED, not acted on -- those three are
estimation workspace that every span's `rgarma` rebuilds before reading, so the
inherited values are dead on this corpus; they stay because an incomplete
stand-in for `restor` has produced three defects in this port already.

**Harness note.** The mutation harness reverted only the LAST edit when one
mutation touched two hunks of the same file (each backup re-read the file, so
the second backup already contained the first mutation). It also raced once:
a measurement taken right after a build in the same PowerShell pipeline read the
previous binary and reported a 6.2e-07 gap that did not exist. Both are the
entry-62 lesson again -- **assert the build ran, and re-measure after.**

Suite 6893 -> **6925 passed, 0 failed, 751 skipped**; ctest 12/12. WALLS
unchanged at 26 gaps / 4 faithful.

## Open, in the order I would take them

1. **The two cheap x11regression siblings still walled in `xrg_editor_setup`**:
   `Xaicst` (editor.f:1802-1808) and `Xaicrg` (:1811-1822), both READ by
   `x11reg.cpp:642/658` and never written except to their gtinpt defaults.
   Entry 80's stock-TD spec pair is the natural carrier for the `Xaicst` one.
2. **The three remaining slidingspans walls**, all of which fatal rather than
   lie: `slidingspans{x11outlier=no}` with automatic x11regression outliers
   (`ssxmdl.f:44-76`'s `rmotss`), `slidingspans{}` with `x11regression{user=}`
   (`ssxmdl.f:142-148`'s `bakusr`), and `fixreg=(outlier)` (whose `otlfix`
   outlives `setssp` and reaches `ssx11a` per span, `sspdrv.f:121`). The rest
   of the subsystem is closed and gated bit-exact -- see part 34 above and
   entry 83 before touching any of it.
3. **What is left of `composite{}`**, now small: pseudo-additive (`Psuadd`) and
   the forced/rounded indirect series on the **agr3** path (`agr3.f:426-538` --
   ported for agr3s, still absent for agr3, and ungated on both for want of a
   `force{}` composite spec).
4. **A composite whose components carry a residual peak**, to gate savpk's real
   `.dir`/`.ind` split — only the degenerate branch runs today.
5. **Two unported Mt2 NOTEs** (task #43): `arima.f:936-960`'s fixed-coefficient
   NOTE, subtracted from the golden side of the slidingspans note gate by
   `_UNPORTED_NOTES` with `test_unported_notes_still_unported` guarding the
   list; and `prtmdl.f:174-177`'s `Nliter>200` NOTE, unported with no corpus
   carrier and deliberately NOT listed.
6. `x11ref.f`'s `IF(Holgrp.gt.0)` fold guard and the uninitialized `Trumlt`,
   both recorded in entry 76 as open questions -- they want the Fortran
   instrumented directly (the `tools/ref_*.f` read-only probe pattern), not
   more reasoning. Same for `x11mdl.f:597-602`'s stale `icol` (entry 77).
7. The amdfct out-of-sample-backcast-with-outlier corner (0.2% out, measured
   and walled); `spectrum{altfreq=yes}` pending CB-30; `history{outlier=auto}` /
   `x11outlier=no` / `additivesa=`; `pickmdl{aictest=(user)}` (needs
   `usraic.f`/`chkchi.f`); the `!Hvmdl` no-model cleanup
   (`arima.f:476-527`).

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
