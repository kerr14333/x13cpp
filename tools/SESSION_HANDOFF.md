# Session handoff — 2026-07-27b (SEATS QS/NP + Tukey peaks)

Replaces the 2026-07-27a handoff. Its open items 1 and 2 both landed; the
findings it recorded are still accurate and are not repeated here.

## Where things stand

**Tree clean at `c11b05e` on `checkpoint/m5-seats-slidingspans`.** Nothing
uncommitted, no background work outstanding.

| check | result |
|---|---|
| `python -m pytest tests/parity -q` | **5261 passed / 0 failed / 529 skipped** (~4m05s) |
| `cd build && ctest` | 11/11 |
| `Rscript bindings/r/test_x13c.R` | 165/165 |

Standing constraints, unchanged: **never merge this branch to `main`, never
push**; **never run `tests/corpus/generated/genspecs.py` or
`tests/corpus/extra/genextra.py`** (both wipe committed specs they cannot
regenerate). Build with
`export PATH="/c/rtools44/x86_64-w64-mingw32.static.posix/bin:$PATH" && cmake --build build -j 6`.

## What landed this session (2 commits, +62 gated specs, ZERO new goldens)

| commit | what |
|---|---|
| `20d0115` | `genqs`/`gennpsa`'s SEATS arms — and `editor.f:517-518` with them |
| `c11b05e` | `getTPeaks`' Tukey spectral peak probabilities (`Tpeaks2`) |

`test_qs_diagnostics` 249 → 313. `test_spectrum_peaks` gained the `.tukey.*`
families **inside its existing 222 specs** — no new tests, no count change.
New: `tools/dump_diag.hpp`. Two Census bugs: **CB-27**, **CB-28**.

## The four findings worth not re-deriving

1. **`editor.f:517-518` was unported, and it is a wrong-numbers bug on every
   non-x11 run that is not taking a log.**
   `IF(.not.Lx11.and.(Fcntyp.eq.4.or.Fcntyp.eq.0.or.dpeq(Lam,1D0))) Muladd=1`
   — the mode is forced ADDITIVE, overriding the multiplicative default
   `gtinpt.f:956` just resolved. gtinpt cannot do it itself: its rule keys on
   `Fcntyp` alone and cannot see `Lx11`. Not print surface — genqs subtracts one
   from a *ratio* irregular under `Muladd != 1`, and a no-log SEATS decomposition
   gives an *additive* irregular centred on zero, so the missing rule shifted the
   series to ≈ −1 and turned a QS of 0.00000 into **1460.26951** on every
   `unrate_*-seats` spec. Now at the tail of `parse_spec.cpp`. `Tmpma` is
   deliberately not updated.
2. **The four SEATS Tukey/QS buffers are only two arrays.** `ansub9.f`'s bridge
   receives `sa` twice (1309→`Seatsa`, 1203→`Stocsa`) and `ir` twice
   (1312→`Seatir`, 1204→`Stocir`); at `sigex.f:3623-3631` they are the same two
   locals. Only `seatad.f` separates them. Hence `Stocir/100` **is** `Seatir`,
   and `qsirr == qsirrevadj` provably holds on the SEATS path in both modes.
   `publish_seats_commons` inverts those two steps from `ctx.seats_*` — read from
   ctx, not from inside `seats_decompose`, so a span replay cannot leave the last
   span's components there.
3. **A routine's FILE is not its size.** `getTPeaks` was scoped at ~850 lines
   (`specpeak.f`) and was ~130: it is nine lines wrapping `getWind` + `covWind`
   (already ported as `tukey_spectrum`) + `Tpeaks2`. Cross-check what is already
   ported before sizing a front off a line count.
4. **A mutation test can be worthless in a way that looks fine.** A 1% *scaling*
   of the published SEATS SA series fails **zero** specs, because QS differences
   the series and then takes autocorrelations — a pure scale factor is invariant
   by construction. Only a shape change discriminates (a 5% spike every 12th
   observation: 62/62 through the SA arm, 50/62 through the irregular). This
   mattered because every SEATS golden reports `qssadj`/`qsirr` as `0.00000`
   except `payems_seats`, which has `npsi==1` (`sa == z`, so `qssadj` is
   trivially `qsori`).

## Open, in the order I would take them

1. **Two published-vs-internal reads in `run_spectrum.cpp`** — `:465` on the
   `Lrbstsa==false` arm, `:401/:403` in the pseudo-additive sp0 rebuild. Found by
   a read-only audit, **not verified by hand and not fixed**. Both unreachable on
   the current corpus (no spec sets `spectrum{robustsa=no}`, none pairs
   pseudo-additive with the spectrum), so step one is an oracle on-vs-off
   measurement plus a new spec. Written up at the bottom of
   `tools/spectrum_peaks_scouting.md`.
2. **`spcdrv`'s SEATS branch** — the last thing blocking the spectrum family.
   `run_seats` still does not call `run_spectrum` (this session added only genqs
   and gennpsa to it), and spcdrv's SEATS arms read `Hvstsa`/`Hvstir` over a
   *different* construction again (`spcdrv.f:299/436`) than genqs does. 51 specs
   skip with that reason at `test_spectrum_peaks.py:188`. Note `publish_seats_commons`
   already fills the four buffers, so this may be much smaller than it looks —
   measure before scoping (see finding 3).
3. **The `Iagr==4` indirect names**, with the composite front: `qsindsadj` etc.
   (`tools/genqs_scouting.md`) and `spcindsa.tukey.*` / `peaks.tukey.*.ind`
   (3 goldens, pinned by `test_indirect_tukey_keys_are_not_claimed`). The QS
   indirect call site passes `Tblind = LSLIQS` — a *savelog* index used as a
   `Savtab` subscript — which wants checking before porting.
4. Longer-standing: `pickmdl{}`, composite `agr3s.f` (the SEATS branch),
   `history{outlier=auto}` / `x11outlier=no` / `additivesa=`, the slidingspans
   `chs` per-span prior phase, and `generated/usdeaths_automdl`'s iddiff
   d=0/d=1 defect (skipped with its measurement in three gates now).

## Staleness sweep — 2026-07-27, findings NOT yet fixed

Ran `/staleness-audit` against HEAD `c11b05e`. Ground truth = code + tests.

**CONFIRMED stale, safe to fix:**

* `tools/FABLE_REVIEW.md:69` — "`mdlchk`, `tstmd2`, `testodf`, `bkdfmd`,
  `tstmd1` … are ported but **NOT wired** into automd". False: `automd.cpp`
  calls all five (`mdlchk` at :186/:252/:264/:433/:637, `testodf` :260,
  `tstmd2` :291, `bkdfmd` :464, `tstmd1` :582).
* `CLAUDE.md:1288-1290` — says **both** `TEST_COVERAGE.md:16` and
  `FABLE_REVIEW.md:69` still carry that claim. `TEST_COVERAGE.md:16` was since
  corrected in place (struck through + "STALE as of 2026-07-26"); only
  FABLE_REVIEW still does. The correction note is itself half-stale.
* `tools/x11_scouting.md:266-272` — marks `sumry`, `avedur`, `vars`, `varlog`,
  `varian`, `issame`, `isfals` as **UNPORTED**. All seven are in
  `core/src/x11/x11summ.cpp` (x11pt4 increment 2). 27 `UNPORTED` markers in that
  file total; the rest were not individually re-checked.
* `tools/spectrum_scope.md:10` — "**596 parity pass**". Now 5261.

**Needs a human call:**

* **12 registered git worktrees under `.claude/worktrees/agent-*`**, left over
  from old parallel-agent runs. All twelve are CLEAN (`git status --porcelain`
  empty) and each is a full stale checkout carrying its own `CLAUDE.md`
  ("1033 pass") and `SESSION_HANDOFF.md` ("HEAD `13d03c2`, 1102 pass") — they
  poison every repo-wide grep and `du` on them times out. `git worktree remove`
  keeps the branch, so removal loses nothing, but whether the
  `worktree-agent-*` branch commits still hold unmerged work was not checked.
* Dated snapshot docs left alone per the audit's own rule: `tools/engine_scope.md`
  (2026-07-20), `tools/x11_regeff_handoff.md` ("LATEST STATUS (session 7)"),
  `tools/spectrum_scope.md`'s "STATUS: COMPLETE" (true of the `spectrum{}` spec
  surface, but the doc predates the peak / Tukey / model-only work).

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
  use `D:/…` inside Python, and do file surgery in bash. `ls` and shell builtins
  do understand `/d/`.
* **Do not use PowerShell here-string syntax (`-m @'…'@`) in the Bash tool** — it
  is not bash, and the `@` lands in the commit message. Use a heredoc to a file
  and `git commit -F`.
* Ad hoc engine run: `build/x13run_x11.exe <spec>`, `build/x13run_seats.exe
  <spec>`, `build/x13run_m3.exe <spec>`. Run from the spec's own directory.
* Ad hoc oracle run: copy spec+data into a dir, `cd` there, run
  `oracle/fortran/x13as_ascii_O2.exe <specbase>`.
* **BLS 403 bypass:** `curl` with a contact User-Agent
  `x13cpp-research cyg5005@gmail.com` works on www.bls.gov and download.bls.gov.
* **Methodology, reconfirmed again:** a green suite is not evidence of coverage.
  Both increments this session added keys to gates that were already green, and
  in both cases the mutation test was the only proof the new columns were read
  at all — once it took two attempts to build a mutation that discriminated.
