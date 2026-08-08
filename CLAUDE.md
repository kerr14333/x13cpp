# X13cpp — Claude working notes

A **faithful C++17 port of the U.S. Census Bureau's X-13ARIMA-SEATS** (v1.1 b61),
packaged as embeddable **R** (Rcpp) and **Python** (pybind11) libraries over one
shared `core/`. It is a proof-of-concept under active development.

## Project vision & priority order

The endgame, in sequence:
1. **Convert Census X-13 Fortran → C++** (the `core/` engine). **This is the first
   and foremost priority — everything else needs it first.** Bit-exact with the
   oracle (see next section). This is where day-to-day work lives.
2. **R + Python wrappers as IMPORTED LIBRARIES** — `library(x13)` / `import x13`,
   called in-process to run X-13 and get **result objects** back. NOT a thin shell
   around an executable; the point is working with X-13 directly in the interpreter.
3. **Nice interfaces + all kinds of plots** built on those result objects.
4. **Far future:** tooling to review a large number of series at once.

Once the conversion is complete, the fork is either (a) refactor the C++ to be more
modern, or (b) start the R/Python interfaces — decide then.

Keep #1 the focus. #2-#4 are the reason for #1, but the engine must exist and match
Census before they mean anything. Don't rabbit-hole a single parity xfail at the
expense of overall engine progress — but the engine port is the job.

## The one rule: bit parity

The contract is **bit-exact agreement (1e-8) with the vendored Fortran oracle**
`oracle/fortran/x13as_ascii_O2.exe`. Port faithfully — **do not "improve" the
Fortran.**
- **Ported bugs stay bugs.** Reproduce Census defects verbatim, comment them, and
  log each as a `CB-N` entry in `tools/census_bugs.md` pinned to a test.
- **No auto file output** in the final package: results live on the result object;
  writing `.udg`/`.fct`/save files is explicit and caller-driven.
- Packaging must pass **CRAN + PyPI** checks — design wrappers compliant from the
  start.

## Use the project skills (`.claude/skills/`)

These carry the conventions that otherwise waste turns — read/use them, don't
improvise:
- **`build-run`** — how to build & run tests on this machine.
- **`parity-gate`** — add a corpus spec, bless its oracle golden, gate bit-exact.
- **`port-leaf`** — transcribe one Fortran subroutine with the farray / indexing /
  `DATA` / ported-bug conventions.

## Build & test (Windows) — essentials

- **Build via the PowerShell tool:** `& tools/build.ps1` (configures, builds, runs
  the CTest unit suite). Do **not** run it as `powershell -File …` (execution
  policy blocks `-File`).
- **Toolchain: rtools44** (`C:\rtools44\x86_64-w64-mingw32.static.posix\bin`).
  `build.ps1` prepends that bin to PATH — required, or the collect2→ld LTO-plugin
  link aborts with **"ld returned 9"** (rtools40 shadowing rtools44 on PATH is the
  classic cause). Drive compilers from PowerShell, not msys/Bash (msys make/xargs
  scrub `TEMP` and break gfortran).
- **Python is `python`** (3.14). `python3` is a Windows App alias → "Permission
  denied".
- **Parity tests:** `python -m pytest tests/parity -q -n 8` (**~79s; serial is
  262s**). Green = `NNN passed`, with expected `s` skips (parse-gap / no-golden
  specs). **The current counts live in `tools/SESSION_HANDOFF.md`, not here** --
  this line has gone stale three times now. Green = `NNN passed`, 0 failed,
  0 xfailed. Parallel is safe *because* the gates compare stdout from a
  read-only subprocess and no harness writes side files; re-check that before
  trusting `-n` if a harness changes. Failures: re-run that gate serially
  (`-k "<name>"`, ~3s) — xdist suppresses per-test output. The build is only
  ~25s, so **don't run the full suite on comment/doc-only edits.** See the
  `build-run` skill.
- After adding a `core/src/*.cpp`, the first build prints `GLOB mismatch!` and
  stops — just rerun once.

## Layout

- `core/` — the engine. `src/` by subsystem: `regarima/ automdl/ x11/ seats/
  composite/ transform/ specparse/ diag/ numeric/ tables/ driver/ api/
  common/`. `driver/run_*.cpp` are the phase harnesses
  (`x13run_m2/m3/x11/iddiff`). No `outlier/` or `force/` directory: outlier
  identification is `regarima/outlier.*` (+ `driver/rev_outlier.*`) and forcing
  is `x11/x11force.*`.
- `oracle/fortran/` — the Census Fortran **oracle** (the source of truth) + its
  prebuilt `_O2`/`_O0` binaries.
- `tests/` — `corpus/` (specs), `golden/` (blessed oracle output), `parity/`
  (pytest M-gates), `unit/` (ctest).
- `tools/` — `worklog.py` (dev timeline), `*_scouting.md` (port plans),
  `census_bugs.md`, `FABLE_REVIEW.md`, `TEST_COVERAGE.md`, build/coverage scripts.
- `r-pkg/`, `py-pkg/` — language wrappers.

## Keeping the docs honest (they went stale three times)

Three generated artifacts, and one ownership rule. A 2026-07-29 audit found the
deliverable report advertising a parity count five fronts out of date, a
coverage ledger understating itself by 240 routines, and a scouting doc
contradicting its own later section. All three were true when written.

- **Numbers.** `python tools/metrics.py --write` regenerates `docs/METRICS.md`
  and every `<!--x13:name-->value<!--/x13-->` marker in the tree; `--check`
  exits nonzero if any has drifted. **Never type a count into prose** — add a
  marker, or link to METRICS.md. (`--fast` skips the pytest/ctest runs.)
- **What is unported.** `python tools/walls.py --write` regenerates
  `docs/WALLS.md` from the engine's own refusal messages, split into GAPS (the
  oracle does it, we decline) and FAITHFUL refusals (the oracle declines too).
  This cannot go stale: delete a wall and it leaves the list. If a feature is
  neither walled nor gated, that is the dangerous case.
- **What is ported.** `python tools/coverage_map.py oracle/fortran --audit`
  re-derives `tools/ported.yaml` from the C++ tree rather than trusting it;
  `--promote` writes what it can prove. `--write` alone only DISCOVERS files.
**WHEN THESE RUN.** Tools nobody runs rot exactly like the docs did, so this is
explicit:

| when | what | cost |
|---|---|---|
| every `tools/build.ps1` | `walls.py --check` + `metrics.py --check --fast` | ~2s, **warns**, never fails the build |
| every parity run | `tests/parity/test_doc_tooling.py` — the tools' own health | ~5s, **fails** |
| after touching a wall or a `.f` port | `walls.py --write` | instant |
| after a `--audit --promote` | nothing else; the ledger feeds metrics | — |
| **session close, before the final commit** | `metrics.py --write --parity <pass>,<fail>,<skip>,<xfail>` using the suite run you just did, then `walls.py --write` | ~3s |

The build checks warn rather than throw on purpose: adding a wall and building
before regenerating is a normal mid-edit state, and a build that fails for a
docs reason trains you to stop reading build output. **The session-close step is
the real gate** — do it in the same breath as rewriting the handoff, and pass
`--parity` so the suite is not run a second time for 85s.

**Why there is a test for the tools themselves (2026-07-30).** `metrics.py`
had been broken under PowerShell — the shell `build.ps1` uses — for an unknown
time, and the design above is exactly what hid it. `worklog.py` prints commit
subjects; when stdout is a pipe Python encoded them with the ambient codepage
(cp1252 on Windows), and an em dash raised UnicodeEncodeError. `metrics.py`
caught the failure, returned the error text **as the command's output**, and
its regexes then simply did not match — so it fell back to placeholders and
reported `active_time: unknown`, `calendar_days: 0`. `--write` would have
committed those as measurements, and `build.ps1` exited nonzero on a docs
reason every single run, which is the outcome the warn-don't-throw rule exists
to prevent.

The encoding was incidental. **The real defect was that the check could not
distinguish "I ran and found nothing wrong" from "I crashed"** — the same class
as `run_parity.py` once reporting PASS having compared nothing, and as this
repo's own rule that a null measured under the wrong preconditions is not a
null. So: `metrics.py` now raises `MetricUnavailable` rather than substituting
a placeholder, and refuses to write anything at all if a metric cannot be
derived; both tools pin their stream encodings; and
`tests/parity/test_doc_tooling.py` asserts the tools produce real VALUES, under
a forced legacy codepage as well as UTF-8. That test lives in the parity suite
rather than the build because **the suite is run and read, and a warning that
is itself wrong is invisible.** It earned its place immediately — it found the
output-encoding half of the bug that the first fix had missed.

Standing rule this leaves: **a guardrail that cannot fail loudly is not a
guardrail.** If you add a check, add the case that proves it can fail.

- **Ownership rule.** `tools/SESSION_HANDOFF.md` owns *what is open* — it is
  rewritten each session, so it cannot rot. Scouting docs own *how something
  works and what was measured* — durable, and they must NOT keep their own
  status lists. Code comments describe *their own file*; a comment asserting
  another subsystem's status is a bug. Every cross-file staleness finding in
  that audit was duplicated ownership.

## Progress & commits

- **Track dev time** via `tools/worklog.py`; this is a PoC, so lead progress
  reports with elapsed active time. `WORKLOG.md` is a periodic snapshot; the git
  timeline is authoritative.
- Commit style: `M<n>/<subsystem>: <what>`, list the gated specs, end with the
  `Co-Authored-By:` + `Claude-Session:` trailers.

## Current phase — M5 (X-11 / SEATS)

M0–M4 are done (parse, regARIMA estimate/forecast, automatic model ID). M5 is
substantially closed: the X-11 spine (B1→D16), SEATS seasonal decomposition,
the whole X-11 diagnostics front (force / slidingspans / history), the F2/F3
and spectrum/QS/NP savelog blocks, `composite{}` for both X-11 and SEATS, the
`pickmdl{}` and `automdl{}` model-selection engines, and the R/Python
in-process bindings all gate bit-exact against the oracle. **No open xfails**
— the former estimation-frontier xfails (`unrate_automdl-aictest-x11`,
`payems_automdl-acceptdefault`) now pass.

**What is still open lives in `tools/SESSION_HANDOFF.md`**, which is rewritten
each session and therefore cannot rot. Do not restate it here.

### The archive — read it before you touch a subsystem

Every M5 feature that closed did so with measurements, traps and Census
defects attached, and those records are in **`docs/M5_PORT_NOTES.md`**
(entries 0-95, chronological). They used to live in this file and made it
~40k tokens resident in every session.

**Read the matching entry before working on a subsystem.** Several of those
entries cost a full session to derive, and more than one documents a bug that
was found twice. `core/src/seats/`, `core/src/x11/` and `core/src/driver/`
each carry a `CLAUDE.md` pointing at their entries, so the relevant ones load
automatically when you work there.

### Standing rules — these fire before you know you need them

These are the cross-cutting lessons from that archive. They stay resident
because by the time you would think to look them up, the damage is done.

- **Never run `tests/corpus/generated/genspecs.py` or
  `tests/corpus/extra/genextra.py`.** Both wipe every `*.spc` in their
  directory before regenerating, and both directories hold ~130 committed
  hand-authored specs the generators do not produce. Add a config and write
  only the new spec, or `git checkout` the directory afterwards.
- **Any new `ctx` field written from inside x11pt1/x11pt2/x11pt3 must join the
  span-replay save/restore set** in `run_x11.cpp` and `run_seats.cpp`. A
  `slidingspans{}`/`history{}` replay is a full x11pt3 pass that overwrites the
  live COMMONs in place; the oracle punches its tables before the span drivers
  run, this harness dumps at exit. This seam has bitten the port five times
  (`/x11srs/`, `/lkhd/`, `ctx.x11_f2tests`, `ctx.d8bd9a`, `/mdldat/`'s
  `Arimap`, `/orisrs/`'s `Stoap`) — check it by default, not per feature. The
  fifth one generalises the rule: the set needs **whatever a consumer
  re-derives from, not only what it publishes.** (Eighth: `/x11reg/`'s
  `Begxot`/`Endxot`, once `x11regression{outlierspan=}` stopped being dropped —
  entry 93. **Ninth: the INDIRECT `id11.f`/`id11.3y.f` F-test pair, whose DIRECT
  twin had been in the set since entry 40** — the subset-restore shape below, one
  field at a time. On a composite total `Iagr` is 5 during a replay, so `ftest`
  files the span's DIRECT D11 under the `i` keys; entry 94.) The seventh shows what that
  costs: `Stoap` is the regression-ADJUSTED original behind the `b1` table, and
  it equals the raw series on every spec with no `regression{}` — so the miss
  was invisible until one spec had both (entry 88, `b1` off by 5.0e-1). `history{}` re-estimates per span, so it leaves the last
  span's ARMA coefficients in `Arimap`, and the SEATS harness rebuilds the
  whole decomposition from `ctx` after `run_seats` returns (entry 69).
- **The mirror image of that rule, and it has now cost as much: some state must
  NOT be re-derived per span.** The span driver re-runs `setxpt`, so
  re-assigning `Setpri = Pos1bk` there looks like faithfulness — but
  `Setpri=Pos1bk` is an EDITOR-ONLY line (`editor.f:851`) and the oracle never
  repeats it. `Adj` is anchored at the MAIN run's `Begadj` and `x11int` copies
  it into `Sprior` POSITIONALLY, so a pinned `Setpri` beside a sliding `Pos1ob`
  is exactly what keeps the prior factors date-aligned. Re-anchoring put the
  wrong year's leap factor on every February for months (entry 83). Before
  re-establishing a value inside a span, find the ORACLE's assignment and check
  which phase owns it — an editor assignment repeated per span is a bug, and a
  per-span assignment hoisted to the editor is the same bug mirrored.
- **An argument the Fortran passes and this port dropped is a defect waiting
  for its SECOND call site.** `ssprep(Lmodel,Lx11,Lx11rg)` was ported without
  `Lx11`, always snapshotting `Lter`/`Ktcopt`/`Tic`. That was right for every
  caller the port had, because they all sit BEFORE `x11pt2` resolves the
  auto-select filter sentinels — right by placement, not by the flag. Adding
  `sspdrv.f:218`'s call, which runs AFTER, made span 2 start from span 1's
  chosen filter length: span 1 bit-exact, spans 2-4 out (entry 85). Same shape
  as `xrgdrv`'s `Ksdev` restore (entry 71): identical code, correct at one call
  site, a defect at the next. When you add a call site to a routine whose
  signature you narrowed, restore the argument first and ask what it gated.
- **A trap list is a list of places to LOOK, not a list of things that are
  true.** The trap above it — "a structural change not mirrored into the
  `ssprep` snapshot is undone by the first span's `restor`" — holds for
  `fixmdl` and for `history{fixreg=}`, and is FALSE for
  `slidingspans{fixreg=}`: `ssmdl.f:53`'s `rvfixd` writes only the live
  `Iregfx`/`Regfx`, so `restor` undoes it and the option fixes nothing at all
  (its whole effect is the `Itd`/`Ihol` demote). Adding the "missing" mirror by
  analogy made the engine work where the oracle does not, 8.4e-03 out on span 1
  (entry 83). Three sibling features write the snapshot and the fourth does
  not; only the Fortran says which.
- **A discovery predicate is a hand-written case list that has learned to
  hide.** `endswith("seats")` and `all(_CORE_TAGS)` read like generic discovery
  and are exactly as brittle as a literal, with none of the visibility — a
  literal at least shows you its length. The failure mode is a parametrisation
  that SHRINKS, and one that shrinks to nothing reports green, so every
  discovery carries a `test_*_cases_discovered` floor assertion. To audit
  coverage, never grep the tests: run `pytest --collect-only -q`, keep the
  `[...]` ids, and cross them against every golden on disk — both sides
  derived, so it cannot go stale. That probe found three blind gates in one
  pass, one of them hiding an 8.1e-7 engine defect (entry 69).
- **An ABSENT golden is a claim, and a `skip` never checks it.** Half the
  diagnostics in this engine act by SUPPRESSING an output — an `Itd/Ihol`
  demote's entire effect is that the `tds` table is not written. `skip("spec
  does not produce this tag")` reads like coverage and asserts nothing, which
  is how the missing `tds` producer survived months (entries 79, 81). When the
  SPEC asked for the table and the oracle wrote none, assert the engine writes
  none. Gate the absence on the request, not on the golden: this harness dumps
  every table it computed regardless of `save=`, so on a spec that never asked,
  an absent golden says nothing at all.
- **A channel nobody can read is a channel nobody gates.** `x13run_x11` dumped
  the Mt2/`.err` buffer only when the run FATALed, so every non-fatal NOTE and
  WARNING the engine emitted was discarded unread — and the moment it was piped
  out, an unported diagnostic fell out of a golden committed months earlier
  (entry 81). Before trusting that message text matches, check the harness
  surfaces it at all on the path you care about. **The mirror of it cost an
  increment too**: the same harness dumped NO tables on a FATAL, so an assertion
  that a refused run produced no output was true of any run that produced
  nothing for any reason — and a wall whose whole subject is a refusal is
  exactly where that assertion gets written (entry 85). A late refusal has a
  complete run behind it; throw it away and you cannot gate it. **A third form:
  a diagnostic ROUTINE nobody calls.** `cvrerr.f` is the second half of every
  coverage refusal in the program — sixteen call sites, each naming the two
  DATES that broke the rule — and this port had the `inpter` half at all sixteen
  and none of `cvrerr`, so a refusal came out one third the size the oracle
  writes it (entry 93). No corpus spec reached one, which is exactly why it
  survived. When you port a refusal, port the lines UNDER it. **And the trap
  recurs per harness**: `x13run_composite` still dumped Mt2 on FATAL only, long
  after `x13run_x11` was fixed, so every non-fatal diagnostic a composite run
  emitted was discarded (entry 95). Wiring it also showed the harness passing a
  bare basename where genfor.f prints the spec FILENAME — a header nobody read
  had been wrong for as long as it was unread. Fixing a channel fixes one
  harness; check the siblings in the same breath.
- **Where this port CAPTURES a result somewhere other than where the oracle
  does, the equivalence is a claim about WHICH BUFFER and at WHICH MOMENT — and
  it is only ever tested by the options the corpus happens to carry.** The
  oracle files a history span's result from inside `x11pt3`, by handing
  `getrev` the finished component; this port let `x11pt3` finish and re-read
  `ctx.x11srs` afterwards. Same numbers on 24 history specs, and wrong on three
  option combinations none of them had: `force{}` stores `Stci2`, `round=yes`
  stores `Stcirn`, and the TREND store takes `stc2`, the LS-folded published
  D12, not the internal `Stc` (entry 89). The give-away is that the moved
  capture reads a COMMON by name while the oracle reads a local by argument.
  Note also what kept it alive: all four `Irev==4` sites were WALLED, and the
  wall could not fire because the port never set `Irev` — a wall in front of a
  rearrangement looks exactly like a wall in front of a gap.
- **Know whether a save/restore mirrors the Fortran or patches a
  rearrangement.** Where this port moves a call to a different phase than the
  oracle runs it in, the compensating state save has no counterpart in the
  Fortran — and it is invisible for as long as every gated spec goes through
  the moved path. Add a SECOND call site that did not make the move and the
  compensation becomes a defect there, one that will not look like one, because
  the code is identical and correct twenty lines up. `xrgdrv`'s `Ksdev` restore
  is the worked example: deleting it fails 362 gates, keeping it on the
  un-hoisted no-model call fails 17 (entry 71). Gate such restores on the CALL
  SITE, not on the spec.
- **Where this port stands in for `restor`, the stand-in restores a SUBSET.**
  Three separate defects now (entry 72): the omitted fields are invisible until
  something downstream reads one, and the read is usually in a different file
  and a later phase. `restor.f`'s set is `Lter`, `Ktcopt`, `Tic`, the model
  parameters, `Nrxy`, `Iregfx`, `Regfx`, `Ncusrx`, `Nrusrx`, `Picktd`,
  `Adjtd`..`Adjsea` — check that list against the writers inside whatever block
  the stand-in brackets, and re-check it whenever a new writer appears there.
- **A parsed-but-unread option is the silent-wrongness class.** The single most
  common defect shape in this port: the parser consumes a documented option,
  writes it nowhere, and the run returns `OUTCOME: OK` with wrong numbers.
  Before believing an option works, find the read, not just the parse. **And
  the DEFAULT is the dangerous half**: `slidingspans{fixx11reg=}` defaults to
  yes, so the option was live on every spec that never mentioned it, and an
  inertness proof that enumerated only the arms a spec can switch on missed it
  entirely (entry 79). An inertness argument must cover the defaults.
  **Its quieter twin: an ARGUMENT that is passed and unread.** `getsvl` ignored
  its `(displacement, count)` pair for years, so the two call sites that existed
  passed placeholders — `getsvl(ctx, 0, 11, …)` — and nothing could tell. When
  the read was finally wired, the placeholder STILL failed no gate, because the
  corpus's only `check{savelog=}` value is `all` and `all` fell inside the wrong
  window too (entry 91). **A wrong dictionary slice that happens to contain the
  one name the corpus uses is as invisible as an unread parameter, and the spec
  that discriminates it carries a REAL name from the WRONG slice, never a
  nonsense one.** Any `(void)param;` at the head of a ported routine is this
  defect waiting for its first caller who cares. Entry 92 found the same thing
  one file over in `getprt`/`getsav` — 30 of 36 call sites missing, two more
  passing placeholders — so when one dictionary reader turns out to be a
  consumer, check its siblings in the same breath.
- **A feature measured with its partner missing measures the partner.** The
  `Ixreg` demote was probed alone, made `sfs` 4.1e+0 worse, and was written up
  as "do not copy this" — a correct number and a wrong conclusion, because
  alone it makes each span REFIT what the oracle RELOADS (entry 79). When a
  rejected-by-measurement note sits next to an unported routine, re-measure it
  with that routine before trusting the note, and write such notes so they say
  what else was missing at the time.
- **A measured INERTNESS is scoped to the probe spec exactly as much as a
  measured effect is, and it does not read that way.** "`fixreg=` fixes nothing
  in the oracle" was derived correctly, from a real instrumented-oracle dump,
  and stood in a code comment as a claim about the OPTION. It was a claim about
  a design the `ssmdl` group walk leaves alone: with one held-back outlier in
  the spec, `regchg` goes true, `ssmdl.f:358-373` re-snapshots the POST-`rvfixd`
  `Iregfx`/`Regfx`, and every span's `restor` now reinstates the fixings
  instead of erasing them — 192 `sfs` lines move (entry 86). When a note says a
  feature does nothing, the question to ask is what the probe spec did NOT
  have; "no observable effect" and "no effect" differ by whatever the corpus
  was missing. **The sharpest form: two arms can select between operators that
  are ALGEBRAICALLY the same thing on symmetric input.** `force{indforce=}`
  chooses between benchmarking the indirect SA and summing the components'
  benchmarked ones, and benchmarking commutes with the sum — the ORACLE gives
  the same `iaa` either way to 4.9e-15, for the linear `denton` and, measured,
  for `regress` too. A mutation between the arms fails nothing, and the reflex
  fix ("use the nonlinear option") also fails nothing. What separates them is
  making the COMPONENTS differ — force one and not the other, 1.4e-05 (entry
  94). Ask what identity the probe is sitting on, not just which option it set.
  **And distinguish SATURATED from INERT-BY-CONSTRUCTION** (entry 95): a
  saturated precondition is a gap in the corpus and the fix is a better spec; an
  inert-by-construction one is a theorem about the program — `O2` equals `O5`
  under pseudo-additive because the oracle refuses all four routes to a calendar
  factor — and the fix is to write the theorem down, having actually tried the
  routes. Both look identical from a mutation that measures zero.
- **Count the block; never read the indentation.** Fortran closes with bare
  `END IF`, so an `ELSE` seven levels deep looks exactly like a top-level one.
  `x11mdl.f:661`'s `ELSE` pairs with `IF(igrp.gt.0)` at `:546`, not with the
  `IF(Havxtd...)` at `:541` that the surrounding prose implies — and the wrong
  reading produced a guard that could NEVER fire, caught only because a debug
  print was added when the branch stayed silent (entry 80). A twenty-line
  script that walks `IF/ELSE/END IF` and prints the pairing takes a minute and
  cannot lie. Same family as the ARGDIC decode below. **And the mirror: two
  arms that read as symmetric can differ by one statement.** `getprt.f`'s list
  arm ends its bad-prefix branch with `GO TO 10`; the single-value arm twenty
  lines up has no jump and FALLS THROUGH into the table lookup, so the oracle
  emits two errors where the port emitted one (entry 92). Their messages differ
  by one character too. Diff the arms; do not assume the pair.
- **A comment naming a `.f` line is a claim, not evidence — and the two drift in
  opposite directions.** Twice in one increment (entry 77): a branch labelled
  `// centeruser (gtxreg.f:537-543)` was dispatched on the argidx of
  `umtrimzero`, the comment and the code disagreeing with each other in plain
  sight; and a block headed "Iregfx from the b= fixings, then Userfx" did the
  Userfx half while the `regfix()` that produces `Iregfx` was never called at
  all. Both readers were inert, so nothing failed. **Verify a dispatch against
  the computed `GO TO` and the ARGDIC pointer table, not against the comment
  above the branch** — decoding `argptr` takes a minute and is the only check
  that cannot lie.
- **Measure the ORACLE on-vs-off before porting anything**, then measure
  engine-vs-oracle separately. The first tells you a flag matters; only the
  second tells you whether the engine already honours it. Both have been
  skipped here, and both cost a wasted increment.
- **When a divergence is attributed to feature X, build the spec WITHOUT X
  before porting anything.** Engine-vs-oracle names a delta; only
  cheap-spec-vs-expensive-spec names the SUBSYSTEM that owns it. Entry 76's
  "auto-AO AICC gap" was neither auto-AO nor an AICC gap — deleting the
  `aictest=` line reproduced the whole thing and pointed at a skipped prior
  pass. Note also that an inseparability proof ("every spec reaching A also
  reaches B") tells you two features co-occur; it never tells you which one owns
  the delta, and entry 75 used one to justify NOT building the cheaper spec.
- **A PHASE INDICATOR the port never advances turns every guard keyed on it
  into dead code, silently and in bulk.** `revdrv.f:387` sets `Irev=4` around
  the history span loop; `run_history` has no counterpart, so `ctx.hiddn.irev`
  is 1 for the whole run and three separate guards — the x11regression
  outlier-ID gate, its coefficient seed, and the error-header suppression —
  quietly take their main-run branch during a replay (entry 87). This is the
  `walls.py` blind spot in state form: nothing refuses, nothing is listed, and
  the guards LOOK ported. When you port a condition on a mode variable, check
  the port ever writes the mode. **CLOSED in entry 89** — `Irev` now reaches 4
  and 5, and the count of what it was hiding is the point: ~40 conditions, five
  `getrev` sites, four unreachable walls and a stubbed `errhdr` that had been
  live for `slidingspans{}` all along. Mutating the assignment back fails 171
  gates. Keep the rule; the next mode variable will be somebody else's. **It
  was: `Lindot` (entry 95.)** `gtinpt.f:311` defaults `composite{indoutlier=}`
  to yes; the port assigned the flag only in the parse arm, so it was false on
  every run that did not spell the option out, and four `agr3` guards were dead
  — the indirect outlier-factor build, the level-shift refold into the
  published trend, the AO factor and the D8 divide. Hidden because every
  consumer is a CONJUNCTION with `Lindls`/`Lindao` and no composite carried an
  outlier, so both readings agreed; one component-level shift and
  `itn`/`iir`/`id8`/`id9` were 3.5e-04 out at `OUTCOME: OK`. **Generalised
  form: a DEFAULT that lives only in the Fortran's initialisation block is the
  same defect as an unadvanced mode variable.** When you port a `cmn` flag,
  find its `gtinpt.f` default, not just its reader.
- **A BARE `abend` is not a wall — it is a hole with the lights off.**
  `walls.py` derives the inventory from refusal MESSAGES, so a guard that calls
  `abend(ctx)` with no `errhdr`/`writln` is in neither the gap list nor the
  count, and the run's whole `===ERR===` block comes back EMPTY. Two of them
  sat on `rmfix`/`addfix`'s user-regressor arms for months (entry 88). Every
  refusal goes through a `*_not_ported` helper; if you write `abend` directly,
  give it a message first. **And the helper's NAME is load-bearing, not
  cosmetic**: `walls.py` matches an explicit `HELPERS` tuple with `\b`, and
  `\bnot_ported` cannot match inside `agr3_not_ported` because `_` is a word
  character — two new walls landed invisible, with the count unmoved at 19
  (entry 94). Reuse an existing helper name or add yours to `walls.py` in the
  same commit, and confirm the count MOVES.
- **An unported path that returns SUCCESS is worse than one that has no code.**
  Walls are inventory — `walls.py` lists them and deleting one leaves the list.
  A silent `return true` guard is in neither the wall list nor the gate count,
  and it is how a whole phase went missing for months (entry 76, `xrgdrv`).
  Every early-out on an unported path either fatals or is a wall.
- **A null measured under the wrong preconditions is not a null.** If a
  feature's effect is conditional on a set being non-empty, the probe must
  assert the set is non-empty — a saturated precondition looks like a passing
  gate, not like a zero delta. Same for mutation tests: a mutation that PASSES
  usually means the precondition is saturated, not that the code is right.
  **The re-measurement direction reads differently and cost an increment on its
  own: a candidate FIX that measures zero while one of its own preconditions is
  known dead has not been tested, it has been skipped.** Entry 87 restored the
  two missing clauses of `x11mdl.f:424`, measured the history probe both ways at
  sar 4.6e+0, and wrote "the obvious candidate is NOT the cause" into the code —
  in the same comment that recorded `Irev` never leaving 1, which is exactly why
  the restored clause could not evaluate. Entry 89 set `Irev`; the arm was
  bit-exact from that moment and the board item sat open for an increment
  (entry 90). When a fix measures zero, name its preconditions before believing
  the number. **And the third direction: a probe SPEC whose parameter change
  changes no VERDICT gates nothing, and reads exactly like a dead code path.**
  Entry 93's first default-outlier-window spec moved the derived critical value
  3.87 → 3.83 and produced a bit-identical adjustment, because the three
  outliers between those thresholds appear only in the iteration listing — so a
  mutation that genuinely rewrites that value measured 0, and hours went into
  "the mutation must not be compiling." Pick the probe parameters so the
  DOWNSTREAM DECISION flips, then confirm the flip in the oracle before writing
  the spec.
- **A passing mutation is also a claim about WHICH ARM the spec is on.**
  Mutation-test each ARM of a branch chain, not each routine — a spec that
  reaches the routine is not a spec that reaches the arm. Disabling `ssmdl`'s
  `Iregfx==3` arm changed nothing on a spec written for it and named after it:
  every coefficient was fixed, but `getreg`'s Leap Year splice left one column
  valueless, `regfix.f:41` never promoted `Iregfx` past 2, and the spec had
  been entering through the `==2` door all along (entry 82). The observable was
  correct, the comment was not, and only the mutation could tell them apart.
- **Reachability arguments are only valid over the options the parser
  honours.** An unreachability proof was wrong once because the route it ruled
  out went through an option that was being silently discarded.
- **A deferred SUBSYSTEM can swallow a load-bearing routine that merely
  looks like it belongs to it.** This port defers the whole `.out` print engine
  by design, and `prterx.f` -- 59 lines that name a singular regression column,
  write a diagnostic, and **call `abend`** -- went with it. The stop went too:
  every `CALL regx11` in the oracle is guarded by
  `IF(.not.Lfatal.and.Armaer.eq.PSNGER)CALL prterx()`, none of the guards were
  ported, and a singular x11regression design returned `OUTCOME: OK` (entry 73).
  When you skip something because of the family it is named into, check what it
  does BESIDES that family's job. That class is now swept to exhaustion: of the
  eleven `prt*` routines that call `abend`, nine only do so on a save-file-open
  failure (unreachable -- this port writes no save files), and the two real error
  reporters are `prterr` and `prterx`. Both are ported. Do not re-derive it.
- **A gate that has existed for months still needs the SPEC that reaches it.**
  `test_m1_parse::test_outcome_matches_oracle` would have caught the above on
  day one. Entry 61 wrote the spec that triggers it, measured the divergence,
  and then DELETED the spec rather than gate it. Measuring a divergence and not
  landing a spec leaves nothing behind but prose.
- **A wall keyed on a PROXY for its trigger is narrower than the divergence it
  claims to cover.** The x11regression aictest wall tested `Nbx == 0` ("no
  regression variables") when the real trigger was NO TRADING-DAY GROUP. One
  non-TD variable walked past it into a 974-unit AICC error and an inverted
  accept/reject verdict, at `OUTCOME: OK` (entry 74). The wall looked
  conservative, which is what hid it. **State the trigger in the condition, not
  a symptom of it** -- and note that the WALLS count did not move when this was
  fixed, because the wall was widened rather than added. A gap count is not a
  coverage measure.
- **Measure before naming a Census bug.** Two near-CB entries turned out to be
  correct Fortran read against the wrong mode. And a comment documenting a
  Census bug is not the same as code reproducing it — check the code below it
  agrees (that was CB-23).
