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
  outlier/ transform/ specparse/ diag/ force/ numeric/ tables/ driver/ common/`.
  `driver/run_*.cpp` are the phase harnesses (`x13run_m2/m3/x11/iddiff`).
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
defects attached, and those records are in **`docs/M5_PORT_NOTES.md`** (53
numbered entries, chronological). They used to live in this file and made it
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
  run, this harness dumps at exit. This seam has bitten the port four times
  (`/x11srs/`, `/lkhd/`, `ctx.x11_f2tests`, `ctx.d8bd9a`) — check it by
  default, not per feature.
- **A parsed-but-unread option is the silent-wrongness class.** The single most
  common defect shape in this port: the parser consumes a documented option,
  writes it nowhere, and the run returns `OUTCOME: OK` with wrong numbers.
  Before believing an option works, find the read, not just the parse.
- **Measure the ORACLE on-vs-off before porting anything**, then measure
  engine-vs-oracle separately. The first tells you a flag matters; only the
  second tells you whether the engine already honours it. Both have been
  skipped here, and both cost a wasted increment.
- **A null measured under the wrong preconditions is not a null.** If a
  feature's effect is conditional on a set being non-empty, the probe must
  assert the set is non-empty — a saturated precondition looks like a passing
  gate, not like a zero delta. Same for mutation tests: a mutation that PASSES
  usually means the precondition is saturated, not that the code is right.
- **Reachability arguments are only valid over the options the parser
  honours.** An unreachability proof was wrong once because the route it ruled
  out went through an option that was being silently discarded.
- **Measure before naming a Census bug.** Two near-CB entries turned out to be
  correct Fortran read against the wrong mode. And a comment documenting a
  Census bug is not the same as code reproducing it — check the code below it
  agrees (that was CB-23).
