# Session handoff — 2026-07-27c (robustsa + the SEATS spectrum branch)

Replaces the 2026-07-27b handoff. Its open items 1 and 2 both landed; its four
findings are still accurate and are not repeated here.

## Where things stand

**Tree clean at `a5b119f` on `checkpoint/m5-seats-slidingspans`.** Nothing
uncommitted, no background work outstanding.

| check | result |
|---|---|
| `python -m pytest tests/parity -q` | **5324 passed / 0 failed / 478 skipped** (~4m20s) |
| `cd build && ctest` | 11/11 |
| `Rscript bindings/r/test_x13c.R` | 165/165 (not re-run this session; untouched surface) |

Standing constraints, unchanged: **never merge this branch to `main`, never
push**; **never run `tests/corpus/generated/genspecs.py` or
`tests/corpus/extra/genextra.py`** (both wipe committed specs they cannot
regenerate). Build with
`export PATH="/c/rtools44/x86_64-w64-mingw32.static.posix/bin:$PATH" && cmake --build build -j 6`.

## What landed this session (2 commits, +63 gated specs, ONE new golden)

| commit | what |
|---|---|
| `9d0be0e` | `spectrum{robustsa=}` — parsed-and-dropped, plus the D13 it reads |
| `a5b119f` | the spectrum block on the SEATS path — 51 specs unskipped |

`test_spectrum_peaks` 225 → 276. Suite 5261/529 → 5324/478 (the 51 SEATS skips
became passes; the twelfth new pass is the one new spec).

## The four findings worth not re-deriving

1. **An audit that finds an unreachable-looking branch should ask WHY it is
   unreachable before filing it as low priority.** The 2026-07-27b handoff's
   item 1 was "a published-vs-internal buffer read, unreachable, needs a spec".
   The reason it was unreachable is that **`spectrum{robustsa=no}` was itself
   parsed and silently dropped** — the token is in `gt_spectrum`'s ARGDIC
   (argidx 21) with no case in the switch, so `ctx.rho.lrbstsa` never left its
   default. Two independent defects stacked, and neither fix alone is enough.
   Measured, `spcirr.median`: **-44.497** neither, **-37.436** parse only,
   **-37.352** (the oracle, byte-exact on all 86 keys) both.
2. **The audit's second finding is PROVABLY INERT, and proving it was cheaper
   than gating it.** `editor.f:2508-2523` refuses a `mode=pseudoadd` run when
   any of `Adjao`/`Adjtc`/`Adjls`/`Finao`/`Finls`/`Fintc` is set or
   `Nustad > 0` — exactly the disjuncts of `have_sti2`/`have_stc2`. So under
   Psuadd the published mirror IS the internal value. Confirmed twice on the
   oracle (rejected with outlier regressors; rejected again with a temporary
   trend prior). Code changed anyway, proof recorded at the line, no spec
   written.
3. **`spcrsd` stays `spcrsd` on a SEATS run — it is not `spcextrsd`.**
   `spcrsd.f:209-215` picks the label off its own `Lseats` ARGUMENT, and there
   are two call sites: `arima.f:1126` passes **F** (the regARIMA residuals) and
   `seatpr.f:145` passes **T** for the SEATS EXTENDED residuals `Srsdex`, a
   different input. The port had read it as a property of the run. `spcextrsd`
   is correctly absent from the corpus — `seatpr.f:142` gates it on
   `Prttab`/`Savtab(LSPERS)`, **not** on `Lsumm`, so `-s` alone never makes one.
4. **The shape-vs-scale mutation trap generalizes past QS.** A pure scaling of
   the SEATS SA series fails **zero** spectrum specs: `gendff` logs before it
   differences, so a scale factor is an additive constant the differencing
   removes. A periodic 5% spike fails **51 of 51** through the SA arm and 51 of
   51 through the irregular. Same trap the 2026-07-27b handoff recorded for
   `calcqs`, different mechanism — check what the statistic is INVARIANT to
   before choosing a mutation.

## Open, in the order I would take them

1. **The `Iagr==4` indirect names**, with the composite front: `qsindsadj` etc.
   (`tools/genqs_scouting.md`) and `spcindsa.*` / `spcindirr.*` /
   `peaks.tukey.*.ind` (3 goldens, pinned by
   `test_indirect_tukey_keys_are_not_claimed`). The QS indirect call site passes
   `Tblind = LSLIQS` — a *savelog* index used as a `Savtab` subscript — which
   wants checking before porting. `mkspky.f` already has the prefix rules
   (`spccomp`/`spcindsa`/`spcindirr`).
2. **`pickmdl{}`** — still parse-only (M1), and it is now the sole reason two
   specs skip in `test_spectrum_peaks` and several elsewhere.
3. Longer-standing: composite `agr3s.f` (the SEATS branch),
   `history{outlier=auto}` / `x11outlier=no` / `additivesa=`, the slidingspans
   `chs` per-span prior phase, and `generated/usdeaths_automdl`'s iddiff
   d=0/d=1 defect (skipped with its measurement in four gates now).
4. Not urgent but worth a pass: `spectrum{}` still has parsed-and-consumed-only
   arguments (`start` → a `Bgspec` override, `peakwidth`/`altfreq`/
   `showseasonalfreq` → the mkfreq grid). Two of the last three spectrum
   defects were exactly this class, so treat the remaining ones as suspects
   rather than as deferred work — the ARGDIC-without-a-switch-case shape is now
   a known signature.

## Environment notes

* **`/codex:cancel` is broken on this machine.** The companion shells out to
  `taskkill /PID <n> /T /F` through Git Bash, which path-translates `/PID` into
  `C:/Program Files/Git/PID`; every cancel fails with
  `ERROR: Invalid argument/option`.
* **Codex task notifications do not carry findings.** The wrapper agent reports
  "completed" as soon as it hands off to a background job; the real result is in
  that job's `<task-id>.log` (`Final output`) under
  `C:\Users\cyg50\.claude\plugins\data\codex-openai-codex\state\x13new-f9c8d9ddca8ae988\jobs\`.
* **Windows Python** (`python`) does not grok git-bash `/d/` or `/tmp` mounts —
  use `D:/…` inside Python, and write scratch files to the session scratchpad,
  not `/tmp`. (Hit again this session: a heredoc'd Python script wrote to
  `/tmp/x.txt` via `sed` and then could not read it back.) `ls` and shell
  builtins do understand `/d/`.
* **Do not use PowerShell here-string syntax (`-m @'…'@`) in the Bash tool** — it
  is not bash, and the `@` lands in the commit message. Use a heredoc to a file
  and `git commit -F`.
* **The oracle's flag order is `x13as_ascii_O2.exe <specbase> -s`**, not
  `-s <specbase>` — a leading spec name is rejected as an undefined program
  option, exit 0, no `.udg`, no obvious error.
* Ad hoc engine run: `build/x13run_x11.exe <spec>.spc`, `build/x13run_seats.exe
  <spec>.spc`, `build/x13run_m3.exe <spec>.spc`. The **`.spc` extension is
  required** (the harness does not append it). Run from the spec's own directory.
* **BLS 403 bypass:** `curl` with a contact User-Agent
  `x13cpp-research cyg5005@gmail.com` works on www.bls.gov and download.bls.gov.
* **Methodology, reconfirmed again:** every increment this session ended with a
  mutation test, and one of the two needed a second attempt before it
  discriminated. A green gate over new keys is not evidence those keys are read.
