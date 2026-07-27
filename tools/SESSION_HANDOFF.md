# Session handoff — 2026-07-27 (QS / NP / model-only diagnostics)

Replaces the 2026-07-24 CES handoff, which was stale (it still said parity 1102
and HEAD `13d03c2`). Everything it listed as "next steps" has since landed;
composite increments 2–4 and the x11 stubs it named are closed — see `CLAUDE.md`.

## Where things stand

**Tree clean at `bbb2eb3` on `checkpoint/m5-seats-slidingspans`.** Nothing
uncommitted, nothing half-written, no background work outstanding.

| check | result |
|---|---|
| `python -m pytest tests/parity -q` | **5199 passed / 0 failed / 591 skipped** (~4m20s) |
| `cd build && ctest` | 11/11 |
| `Rscript bindings/r/test_x13c.R` | 165/165 |

Standing constraints, unchanged: **never merge this branch to `main`, never
push**; **never run `tests/corpus/generated/genspecs.py` or
`tests/corpus/extra/genextra.py`** (both wipe committed specs they cannot
regenerate). Build with
`export PATH="/c/rtools44/x86_64-w64-mingw32.static.posix/bin:$PATH" && cmake --build build -j 6`.

## What landed this session (4 commits, +335 tests, ZERO new goldens)

| commit | what |
|---|---|
| `7cd2190` | `genqs.f` — the QS seasonality statistics, X-11 path (+157) |
| `c36c673` | `gennpsa.f` — the NP residual-seasonality verdict, same gate |
| `6119d1d` | the MODEL-ONLY diagnostics path — unblocked QS **and** spectrum peaks (+174) |
| `bbb2eb3` | the explicit-aictest leap-year prior loss that `6119d1d` exposed (+4) |

New: `core/src/diag/genqs.{hpp,cpp}`, `tests/parity/test_qs_diagnostics.py`.
Two Census bugs logged: **CB-25**, **CB-26** (`tools/census_bugs.md`).
Narrative in `CLAUDE.md`; per-front detail in `tools/genqs_scouting.md` and
`tools/spectrum_peaks_scouting.md`.

## The three findings worth not re-deriving

1. **`/x11srs/ Sti` and `Stc` are NOT the published D13/D12 — and this port
   makes them so.** `x11pt3.f:1089-1103` builds the AO/TC-restored D13 in a
   LOCAL `sti2` and punches that; the COMMON stays internal. This port copies
   the published values back at the tail of x11pt3 (`x11parts.cpp:1455-1458`) so
   the harness can punch d13/d12 off the mirror, keeping the oracle's live
   values in `ctx.x11_sti_int` / `ctx.x11_stc_int`. **Anything new reading
   `/x11srs/` after x11pt3 must take the snapshot.** Invisible to every table
   gate; it only surfaced because `calcqs` is a step function in the sign of
   `r(1)`, so `qsirr` jumped 0.00000 → 0.03946 on `expgs_fixed-airline-x11`.
   What localised it: recomputing calcqs in Python off the *blessed* `.d13`
   reproduced the ENGINE's wrong value, proving the disagreement was downstream
   of the save rather than in the arithmetic.
2. **CB-25 — a COMMON initialised inside a routine that may never run.**
   `QsRsd`/`QsRsd2` are only ever assigned inside `arima` (`arima.f:129-130`),
   which `x11ari.f:106` calls only `IF(Lmodel)`. A model-free run reports the
   COMMON's static zero as a residual statistic. `extra/airline_identify` is the
   control that proves it (Lmodel set, Ldestm not: the reset fires, the
   computation does not).
3. **An in-place buffer mutation plus a per-branch handoff has to be caught by
   DIFFING the branches against each other.** `tdaic` modifies `trnsrs` in
   place; `run_pre_model`'s automd branch re-captured it into `out_trnsrs` and
   the `explicit_aictest` branch beside it did not. The missing line is
   invisible reading only the branch that has the bug. The recorded hypothesis
   (ssprep/restor reverting `Priadj`) was **wrong** — the Fortran keeps Priadj
   correctly; the defect was one branch further out.

## Open, in the order I would take them

1. **`genqs`/`gennpsa` under SEATS** (61 goldens). The SEATS arms read
   `Seatsa`/`Seatir`/`Stocsa`/`Stocir` behind `Hvstsa`/`Hvstir` — COMMONs this
   port's SEATS chain does not fill, because it publishes onto ctx instead.
   `Seatsa`/`Seatir` map onto `ctx.seats_sa`/`ctx.seats_ir`; **`Stocsa`/`Stocir`
   (the `/100` outlier-adjusted pair) need scouting first.** Same blocker as the
   spectrum peak block's SEATS skip — closing one likely closes both.
   `genqs(ctx, lseats=true)` returns false by design today.
2. **`getTPeaks`** (`specpeak.f`, ~850 lines) — the `.tukey.*` families,
   272 goldens. The Tukey SPECTRA are already ported and bit-exact; only the
   peak probabilities are missing. `test_spectrum_peaks.py` has a
   `test_tukey_keys_are_not_claimed` assertion that must be deleted
   deliberately when this lands.
3. **Two published-vs-internal reads in `run_spectrum.cpp`** — `:465` on the
   `Lrbstsa==false` arm, `:401/:403` in the pseudo-additive sp0 rebuild. Found
   by a read-only audit, **not verified by hand and not fixed**. Both are
   unreachable on the current corpus (no spec sets `spectrum{robustsa=no}`,
   none pairs pseudo-additive with the spectrum), so step one is an oracle
   on-vs-off measurement plus a new spec. Written up at the bottom of
   `tools/spectrum_peaks_scouting.md`.
4. **The `Iagr==4` indirect QS/NP names** (`qsindsadj` etc.), with the composite
   front. The indirect call site passes `Tblind = LSLIQS` — a *savelog* index
   used as a `Savtab` subscript — which wants checking before porting.
5. Longer-standing: `pickmdl{}`, composite `agr3s.f` (the SEATS branch),
   `history{outlier=auto}` / `x11outlier=no` / `additivesa=`, the slidingspans
   `chs` per-span prior phase, and `generated/usdeaths_automdl`'s iddiff
   d=0/d=1 defect (now skipped with its measurement in two gates).

## Environment notes

* **`/codex:cancel` is broken on this machine.** The companion shells out to
  `taskkill /PID <n> /T /F` through Git Bash, which path-translates `/PID` into
  `C:/Program Files/Git/PID`; every cancel fails with
  `ERROR: Invalid argument/option`. Reported with and without a job id.
* `task-ms2dbc43-ilqopx` shows **queued at 19h** in `/codex:status`. Its process
  (PID 317868) is **dead** — verified with `Get-Process`. A stale registry row
  that cannot wake up. Harmless; to clear it by hand, the job state lives at
  `C:\Users\cyg50\.claude\plugins\data\codex-openai-codex\state\x13new-f9c8d9ddca8ae988\jobs\`.
* **Codex task notifications do not carry findings.** The wrapper agent reports
  "completed" as soon as it hands off to a background codex job; the real result
  is in that job's `<task-id>.log` (the `Final output` section) under the path
  above — not in the subagent transcript.
* **Verify agent claims before acting.** One reported a `test_m1_parse` hang on
  `census-examples/composite/total.spc`; that binary exits in 4s here — its run
  had collided with a build replacing the exe.

## Carried forward from the previous handoff (still true)

* **Windows Python** (`python`) does not grok git-bash `/d/` mounts — use `D:/…`
  paths inside Python. `ls` and shell builtins do understand `/d/`.
* Ad hoc engine run: `build/x13run_x11.exe <spec>` (dumps b1/d10-d13/d16 etc. to
  stdout, no files), `build/x13run_m3.exe <spec>` (estimate probe). Run from the
  spec's own directory — data paths are relative.
* Ad hoc oracle run: copy spec+data into a dir, `cd` there, run
  `oracle/fortran/x13as_ascii_O2.exe <specbase>`.
* **BLS 403 bypass:** `curl` with a contact User-Agent
  `x13cpp-research cyg5005@gmail.com` works on www.bls.gov and download.bls.gov
  (WebFetch 403s).
* **Methodology lesson, reconfirmed twice this session:** a green suite is not
  evidence of coverage. `genqs`/`gennpsa`/the spectrum peaks were all absent
  behind 100% green because no gate read those keys; and a stale diagnosis can
  outlive the bug it was blamed on. Re-measure before acting on an inherited
  claim — including one written down by a previous session.
