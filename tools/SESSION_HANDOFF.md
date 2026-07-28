# Session handoff — 2026-07-28b (the automdl option front)

Replaces the 2026-07-28 handoff. Its five findings and two CBs are still
accurate and are not repeated here; its open item 1 (`automdl{}`) is what this
session worked, and it is now closed as far as parsing can take it.

## Where things stand

**Tree clean on `checkpoint/m5-seats-slidingspans`.** Nothing uncommitted, no
background work outstanding. `git log --oneline -8` for the current head — this
file is written *before* the commit that contains it, so any SHA named here is
necessarily one behind. (It has gone stale that way twice; hence no SHA.)

| check | result |
|---|---|
| `python -m pytest tests/parity -q -n 8` | **5516 passed / 0 failed / 478 skipped** (~79s) |
| `cd build && ctest` | 11/11 |
| `Rscript bindings/r/test_x13c.R` | 165/165 (not re-run; untouched surface) |

**Run the suite with `-n 8`** (pytest-xdist, installed). 262s serial → 79s, same
counts. Safe because every gate compares stdout from a read-only subprocess and
no harness writes side files — re-verify that if a harness ever changes, since
several gates run the same spec and would race on a fixed filename. Build is
~25s; use `-k "<name>"` (~3s) while iterating.

Standing constraints, unchanged: **never merge this branch to `main`, never
push**; **never run `tests/corpus/generated/genspecs.py` or
`tests/corpus/extra/genextra.py`** (both wipe committed specs they cannot
regenerate). Build with
`export PATH="/c/rtools44/x86_64-w64-mingw32.static.posix/bin:$PATH" && cmake --build build -j 6`.

## What landed (9 commits, 5491 → 5516 passing)

| commit | what |
|---|---|
| `e8024c85` | re-probe automdl on seasonal near-tied series — 7 dropped options |
| `4a6f8c30` | parse the automdl arguments + fix the `diff=` engine defect |
| `386d2a5f` | gate `diff=`/`balanced=`, pin the CES probe series |
| `310a9f10` | parse `ljungboxlimit=` and fatal — its "applied" verdict was wrong |
| `d1b65033` | parse `cancel=`, and stop — the rest is one blocker |
| `fda181a6` | a handoff |
| `3e40d20e` | staleness sweep — six stale claims, incl. `TEST_COVERAGE.md` listing 4 closed subsystems as "not started" |
| `16590880` | **run the label-30 finalization tail on the plain automdl path** — the "one blocker" was an unreachable call, not an unported stage |
| `8871cfd1` | run the parity suite in parallel (`-n 8`), 262s → 79s |

## The one thing to know

**The previous handoff's headline claim was wrong, and how it was wrong is the
lesson.** It said the remaining `automdl{}` surface was gated on "a single
unported port, `automd.f`'s model-adequacy stage (`:577-850`) plus `pass2`/nloop
at `:665`". There was **no unported stage**. `automd_finalize_tail` — `mdlchk` /
`pass0` / `chkrt1`+redomd / `testodf` / `tstmd2`, i.e. `automd.f:654-983` — was
already ported and already bit-exact. It was **unreachable**: `automd.f` has ONE
path with the AIC tests as conditional *blocks* inside it, and this driver split
them into an `if (aic)` / non-aic branch and called the tail from the aic branch
only. Every plain `automdl{}` spec silently skipped the unit-root redomd, the
over-differencing check, the residual-mean Constant add and the lag drop.

One added call. Re-measured on the round-2/3 probe set:

| state | arguments |
|---|---|
| applied + gated | `diff` `balanced` `acceptdefault` **`urfinal`** |
| applied, ungated | **`checkmu`** (ukgas) **`cancel`** (nottem) |
| **DIFFERS** (partly right) | `mixed` `maxdiff` — each now applied on one series, DIFFERS on three |
| **DIFFERS** (unchanged) | `maxorder` |
| **FATAL** (blocked, previously silent) | `noautooutlier` `ljungboxlimit` |
| INERT even at extreme values | `ub1` `ub2` `exactdiff` `hrinitial` `armalimit` `percentrse` `reducecv` `firstar` `fcstlim` `seasonaloverdiff` |
| harness-blind | `rejectfcst` |

**"Unported" and "unreachable" present identically** — an option that moves the
oracle and moves the engine by zero, behind `OUTCOME: OK` — **and cost three
orders of magnitude apart.** The wrong belief came from one failed experiment
over-generalized: someone wired `tstmd1` alone, saw parity break, and concluded
the stage was unported. The actual cause was that the oracle re-estimates
*after* `tstmd1` — and that re-estimate is precisely what the unreachable tail
supplied. **Grep the call sites before sizing a port**; a sole call site inside
a conditional branch is the tell.

## Findings worth not re-deriving

1. **A probe series needs TWO independent properties, and a probe value needs a
   third.** Strong seasonality (`qsori`, `M7`) *and* near-tied automdl
   candidates — `unrate` has a 0.001 BIC gap and `qsori ≈ 0`, so its tie is
   between two nonseasonal candidates and can never exercise a seasonal
   threshold. Then the value must be **far** from the default:
   `ljungboxlimit=0.99` against a 0.95 default reads INERT; at `0.5` it moves
   three of four series. **The sweep prints the same word — INERT — for "bad
   probe" and for "does nothing".**
2. **Verify the engine's BASELINE matches the oracle before trusting any option
   verdict on that series.** `ces_amuse` has the narrowest gap in the corpus and
   a baseline that disagrees (engine 4 ARMA terms, oracle 5), so every row reads
   DIFFERS regardless of the option. It cost one wrong finding —
   `ljungboxlimit` was reported *applied* on `ces_amuse` alone and is in fact
   blocked. It is excluded from the probe set with the reason written down.
3. **Read the documentation, not only the Fortran.** Two of the seven "findings"
   were never parse bugs: the manual defines `urfinal` on the FINAL model (hence
   `chkrt1`, hence the unported stage) in one sentence, and `noautooutlier`
   **appears nowhere in the 306-page manual** — it exists only in `gtauto.f`'s
   `NOTDIC`. The manual is not vendored; fetch from
   `www2.census.gov/software/x-13arima-seats/x13as/unix-linux/documentation/docx13as.pdf`
   (`file` misreports it as 6 pages; PyMuPDF reads all 306).
4. **A doc/code divergence, deliberately resolved toward the code:** the manual
   says `maxorder`'s regular order "must be greater than zero", but
   `gtauto.f:70` validates `.lt.0` with the documented `.le.0` form commented
   out directly above. Parity follows the code.
5. **An argument missing from the probe list reads exactly like one that
   measured INERT.** `gtauto.f` has 24 arguments; round 1 probed 20.
   `acceptdefault` was one of the four omitted and turned out to be applied.
6. **`acceptdefault` is gated by `payems_automdl-acceptdefault`, NOT by
   `airline_automdl-acceptdefault`** — measured, the option does not bite on
   airline at all (identical model either way), so that spec pins nothing about
   it. A spec named after an option does not necessarily exercise it.

## Open, in the order I would take them

1. **Wire label 40's `tstmd1` arm onto the plain automdl path** (task #10).
   `automd.f:576-582`, the `ELSE IF(.not.ismd0)` arm of
   `IF(Lidotl) amidot ELSE tstmd1`. **Do `automd.f:322-344` first** — the
   default-model `rgarma`/`mdlchk`/`armats` is unconditional in the Fortran and
   sits inside `if (aic)` here, and `tstmd1` needs the
   `blpct0`/`rvr0`/`rtval0`/`adj0`/`trns0`/`tair` it produces. Unblocks
   `noautooutlier` (fatal). `Lidotl` is true by default via Lotmod's BIGCV AO
   scan, so the default path is observationally fine — this is only the
   non-default arm.
   *Then* `pass2`/nloop at `:665`, which really is unported and is what
   `ljungboxlimit` needs (`pass2.f:164-169` increments `Pcr`).
   **Re-measure `mixed`/`maxdiff`/`maxorder` separately afterwards.** Each is
   now applied on one probe series and DIFFERS on three; that a single further
   port explains all three is a hypothesis, and the last one of those cost a
   two-session detour.
2. **`gtdpvc` parses decimal literals 1 ulp off the nearest double** (task #9):
   `"0.95"` → `0.95000000000000007` vs the correctly-rounded
   `0.94999999999999996`. Latent everywhere a spec supplies a decimal. **Check
   whether the Fortran reader does the same before changing anything** — if it
   does, the port is faithful and this is documentation, not a fix.
3. The `Iagr==4` indirect names with the composite front (unchanged from the
   previous handoff; note the total's golden carries `npind*`/`spcind*` but no
   `qsind*`, and `genqs.f:439/482` uses a savelog index as a `Savtab`
   subscript — check before porting, may be a CB).
4. **`pickmdl{}`** — still parse-only, and the single largest source of feature
   skips.
5. `spectrum{altfreq=yes}` pending CB-30; composite `agr3s.f`;
   `history{outlier=auto}` / `x11outlier=no` / `additivesa=`; the slidingspans
   `chs` per-span prior phase; `generated/usdeaths_automdl`'s iddiff d=0/d=1.

## Environment notes

Unchanged from the previous handoff (`/codex:cancel` broken; codex notifications
carry no findings; Windows Python cannot read git-bash `/tmp` or `/d/` mounts;
no PowerShell here-strings in the Bash tool; oracle flag order is
`x13as_ascii_O2.exe <specbase> -s`; `option_sweep.py` rewrites its scratch dir
per case; ad hoc runs are `build/x13run_{m3,x11,seats}.exe <spec>.spc` from the
spec's own directory; BLS needs a contact User-Agent). Two additions:

* **`option_sweep.py` now stages every `tests/corpus/data/*.dat`.** It used to
  copy only `airline.dat` and `payems.dat`, so a case naming any other series
  failed on a missing file and reported **REJECTED** — indistinguishable, in the
  report, from a value the oracle refuses.
* **The BLS CES flat files are the practical source for new probe series.** The
  public API v1 caps an unregistered request at ~3 years regardless of the
  `startyear`/`endyear` given (it silently returns the most recent window);
  `download.bls.gov/pub/time.series/ce/ce.data.*` has full history.

## Methodology, reconfirmed

Every increment this session ended with a **mutation test**, and it earned its
keep twice: these parity gates auto-discover specs, so a green run after adding
a spec is not evidence the spec is compared at all. Reverting the `Lautod` guard
fails 5 tests including both `automdl-diff` gates; hardcoding `balanced=false`
fails its gate; hardcoding `acceptdefault=false` fails 32; hardcoding `chkrt1`'s
limit to the 1.05 default fails 4 of the 7 new `urfinal` tests.

**Measure the loop before optimizing it.** "The build is slow" was wrong by a
factor of ten. A one-file change is ~25s (6.4s compile+archive, 18.8s relinking
12 downstream targets — `-j 16` is no faster than `-j 6`, the links are I/O
bound). The 262s **suite** was the expensive half, now 79s at `-n 8`. One full
suite run this session was spent on a comment-only edit; that is ~4 minutes for
a change that cannot alter behavior.
