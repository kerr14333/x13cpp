# Session handoff — 2026-07-28 (the dropped-option sweep)

Replaces the 2026-07-27c handoff. Its open items 1 and 2 both landed; its four
findings are still accurate and are not repeated here.

## Where things stand

**Tree clean at `22a605c5` on `checkpoint/m5-seats-slidingspans`.** Nothing
uncommitted, no background work outstanding.

| check | result |
|---|---|
| `python -m pytest tests/parity -q` | **5491 passed / 0 failed / 478 skipped** (~4m10s) |
| `cd build && ctest` | 11/11 |
| `Rscript bindings/r/test_x13c.R` | 165/165 (not re-run this session; untouched surface) |

Standing constraints, unchanged: **never merge this branch to `main`, never
push**; **never run `tests/corpus/generated/genspecs.py` or
`tests/corpus/extra/genextra.py`** (both wipe committed specs they cannot
regenerate). Build with
`export PATH="/c/rtools44/x86_64-w64-mingw32.static.posix/bin:$PATH" && cmake --build build -j 6`.

## What landed this session (6 commits, 5261 → 5491 passing)

| commit | what |
|---|---|
| `2a3812c6` | `spectrum{robustsa=}` — parsed-and-dropped, plus the D13 it reads |
| `a5b119f4` | the spectrum block on the SEATS path — 51 specs unskipped |
| `1bdbbf50` | **`tools/option_sweep.py`** + `tools/dropped_options_scouting.md` |
| `1fc26bdf` | `regression{noapply=}` — and the tdlom wall it proved reachable |
| `333bc1e0` | the other eight dropped options |
| `22a605c5` | sweep marked closed |

## The new tool, which is the durable output

`python tools/option_sweep.py option_sweep_cases` — finds spec arguments the
engine accepts and ignores. **It found nine.** Three runs per argument: oracle
base (A), oracle with the option (B), engine with the option (C). `A == B` is
INERT (the probe never moved the oracle — *no conclusion*); `A != B` with
`C == A` is DROPPED. Re-run it after any new argument is parsed.

## Findings worth not re-deriving

1. **A reachability argument is only valid over the options the PARSER
   honours.** `x11parts.cpp` carried a written-out proof that the `x11pt2 tdlom
   Adjtd != 1` branch was unreachable, sound for all four routes it considered.
   `regression{noapply=(td)}` is a fifth — and it was invisible *because*
   `noapply=` was dropped, which silently deletes edges from the graph being
   reasoned about. Any "this branch is unreachable" claim in this port should be
   re-checked against the dropped-option list before it is trusted.
2. **"The computation is walled" and "the option is safe" are different
   claims.** `CLAUDE.md` said out-of-sample `aape` was "walled, not
   approximated". The wall was real; the argument that reaches it was dropped,
   so the engine reported within-sample numbers **and labelled them
   `aape.mode: withinsample`** against the oracle's `outofsample`.
3. **A feature applied in two places needs both checked even when the visible
   one is right.** `forecast{lognormal=}` was parsed AND its `untfct` (fct
   table) correction applied; only `prtfct.f:96`'s `Fcstx` correction — the
   forecast X-11 extends the series with — was missing. Printed forecast right,
   d10–d13 wrong. Same shape as the x11regression tdprior logadd guard.
4. **Check what a statistic is INVARIANT to before choosing a mutation.** A pure
   scaling of the SEATS SA series fails **zero** spectrum specs (gendff logs
   before it differences, so a scale factor is an additive constant the
   differencing removes) and zero QS specs (QS differences then autocorrelates).
   A periodic 5% spike fails 51/51 on both arms.
5. **`spcrsd` stays `spcrsd` on a SEATS run.** `spcrsd.f:209-215` picks
   `extrsd` off its own `Lseats` ARGUMENT; `arima.f:1126` passes F (the regARIMA
   residuals, which is what the block is) and `seatpr.f:145` passes T for the
   SEATS EXTENDED residuals. No corpus golden carries an `spcextrsd` key —
   `seatpr.f:142` gates it on `Prttab`/`Savtab(LSPERS)`, not `Lsumm`.

Two Census bugs claimed: **CB-29** (`getreg.f:97`'s `noapply` dictionary runs
`user` and `seasonal` together, so `noapply=(seasonal)` is rejected and the only
token reaching `Adjsea` is `userseasonal`), **CB-30** (`mkpeak.f`'s `Lfqalt`
branch leaves `Tpeak(3)`/`Tup(3)` unassigned at `peakwidth > 1`).

## Open, in the order I would take them

1. **`automdl{}`'s 23 unapplied arguments.** The largest dropped surface left,
   and `gt_automdl`'s own comment admits it. **Do not trust the sweep's nulls
   here** — on airline every one measured INERT, but that is a saturated
   precondition: airline's `(0 1 1)(0 1 1)` wins by 0.014 BIC and these knobs
   adjust unit-root and cancellation thresholds. Re-probe on a series with
   near-tied candidates FIRST.
2. **The `Iagr==4` indirect names**, with the composite front: `qsind*`
   (`tools/genqs_scouting.md`) and `spcindsa.*` / `spcindirr.*` /
   `peaks.tukey.*.ind` (3 goldens, pinned by
   `test_indirect_tukey_keys_are_not_claimed`). Note the composite total's
   golden carries `npind*` and `spcind*` but **no `qsind*`** — the QS indirect
   call site passes `Tblind = LSLIQS`, a *savelog* index used as a `Savtab`
   subscript (`genqs.f:439/482`), so the block is emitted only if that unrelated
   subscript happens to be set. Check that before porting; it may be a CB.
3. **`pickmdl{}`** — still parse-only (M1), and now the single largest source of
   feature skips (9 of the ~15 real ones).
4. **`spectrum{altfreq=yes}`** — needs CB-30 resolved (what `/spcidx/` actually
   holds in `Tpeak(3)`/`Tup(3)`) before a golden means anything.
5. Longer-standing: composite `agr3s.f` (the SEATS branch),
   `history{outlier=auto}` / `x11outlier=no` / `additivesa=`, the slidingspans
   `chs` per-span prior phase, and `generated/usdeaths_automdl`'s iddiff
   d=0/d=1 defect.

## Environment notes

* **`/codex:cancel` is broken on this machine.** The companion shells out to
  `taskkill /PID <n> /T /F` through Git Bash, which path-translates `/PID` into
  `C:/Program Files/Git/PID`; every cancel fails with
  `ERROR: Invalid argument/option`.
* **Codex task notifications do not carry findings.** The wrapper reports
  "completed" as soon as it hands off; the real result is in that job's
  `<task-id>.log` (`Final output`) under
  `C:\Users\cyg50\.claude\plugins\data\codex-openai-codex\state\x13new-f9c8d9ddca8ae988\jobs\`.
* **Windows Python** (`python`) does not grok git-bash `/d/` or `/tmp` mounts —
  use `D:/…` inside Python and write scratch files to the session scratchpad.
  Hit again this session (a script `sed`-wrote `/tmp/x.txt` and could not read it
  back). `ls` and shell builtins do understand `/d/`.
* **Do not use PowerShell here-string syntax (`-m @'…'@`) in the Bash tool** — it
  is not bash and the `@` lands in the commit message. Heredoc to a file and
  `git commit -F`.
* **The oracle's flag order is `x13as_ascii_O2.exe <specbase> -s`**, not
  `-s <specbase>` — a leading spec name is rejected as an undefined program
  option, exit 0, no `.udg`, no obvious error.
* **`tools/option_sweep.py` rewrites its scratch dir every case**, so do not
  inspect `tools/_sweep/a` or `/b` after a multi-case run — re-run the single
  case. That cost one false reading this session.
* Ad hoc engine run: `build/x13run_x11.exe <spec>.spc`, `build/x13run_seats.exe
  <spec>.spc`, `build/x13run_m3.exe <spec>.spc`. The **`.spc` extension is
  required**. Run from the spec's own directory.
* **BLS 403 bypass:** `curl` with a contact User-Agent
  `x13cpp-research cyg5005@gmail.com` works on www.bls.gov and download.bls.gov.
* **Methodology, reconfirmed:** every increment this session ended with a
  mutation test, and two needed a second attempt before they discriminated. A
  green gate over new keys is not evidence those keys are read.
