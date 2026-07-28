# Session handoff — 2026-07-28d (automdl CLOSED)

Replaces the 2026-07-28c handoff. Its findings are carried forward below where
they still matter; its open item 1 (`pass2`) is done, and with it the whole
`automdl{}` front.

## Where things stand

**Tree clean on `checkpoint/m5-seats-slidingspans`.** Nothing uncommitted, no
background work outstanding. `git log --oneline -8` for the current head — this
file is written *before* the commit that contains it, so any SHA named here is
necessarily one behind. (It has gone stale that way twice; hence no SHA.)

| check | result |
|---|---|
| `python -m pytest tests/parity -q -n 8` | **5560 passed / 0 failed / 478 skipped** (~90s) |
| `cd build && ctest` | 11/11 |
| `Rscript bindings/r/test_x13c.R` | 165/165 (not re-run; untouched surface) |

**Run the suite with `-n 8`** (pytest-xdist, installed). 262s serial → ~90s,
same counts. Safe because every gate compares stdout from a read-only
subprocess and no harness writes side files — re-verify that if a harness ever
changes. Build is ~25s; use `-k "<name>"` (~3s) while iterating.

Standing constraints, unchanged: **never merge this branch to `main`, never
push**; **never run `tests/corpus/generated/genspecs.py` or
`tests/corpus/extra/genextra.py`** (both wipe committed specs they cannot
regenerate). Build with
`export PATH="/c/rtools44/x86_64-w64-mingw32.static.posix/bin:$PATH" && cmake --build build -j 6`.

## What landed (4 commits, 5516 → 5560 passing)

| commit | what |
|---|---|
| `dcbfc35b` | unify automd's two paths — one path, as in the Fortran; closes `noautooutlier=` |
| `aa721bc7` | gate `checkmu=` / `cancel=` / `maxdiff=`; correct the `pass2` reachability note |
| `9e1d968b` | docs refresh |
| `e9a31e6b` | **port `pass2` + the nloop re-entry** — closes `mixed=`, `maxorder=`, `ljungboxlimit=` |

## The one thing to know

**`automdl{}` is closed.** All 11 arguments that move the oracle now apply,
nothing in the block is fatal or silently dropped, and each is gated on at
least one corpus spec. It is the first spec block in the option sweep to reach
that state.

It took three increments across two sessions, and **each one revised the
previous one's sizing**:

1. The label-30 finalization tail was *already ported* and merely unreachable
   from the plain path. One added call closed `urfinal`.
2. The aictest/non-aictest BRANCH itself was the problem — `automd.f` has one
   path with the AIC tests as conditional blocks inside it. Removing the branch
   closed `noautooutlier`, `checkmu`, `cancel`, `maxdiff`.
3. `pass2` + the nloop re-entry closed `mixed`, `maxorder`, `ljungboxlimit`.

`automd.cpp` now runs `automd.f`'s real GO-TO graph over labels 10 / 50 / 40 /
30, with `pass2` returning `igo` 1/2/3 to re-enter at 10/40/50. **Three bugs
were invisible purely because `nloop` had been pinned at 1** — `lidold`
(`automd.f:167`, captured BEFORE the Lotmod override, so a re-entry must not
re-run `amdid`), the `nloop.eq.1` guards on the `nbb`/`a0` revert (`:458-467`,
`:503`), and `:472`'s `clrotl` guard reading **`nauto0`** rather than `Natotl`.

Still deferred in `automd.f`, and genuinely closed by the BIGCV scan finding
nothing: the `Lidotl` outlier-ID block on the DEFAULT model (`:280-321`).

## Findings worth not re-deriving

1. **Mutate a ported routine per HALF, not as a unit.** `pass2`'s two halves
   turned out to be covered by DISJOINT specs — disabling the `ichk` revert
   fails only `ukgas_automdl-maxorder`, disabling the `Pcr`/`igo` half fails
   the `mixed`/`ljungboxlimit` specs. A single whole-routine mutation would
   have reported "covered" and hidden that `ukgas_automdl-mixed` exercises
   neither (it pins `amdid`'s filter; said so in the spec).
2. **Verify the engine's BASELINE before reading any option verdict off a
   series.** ukgas / nottem / ces_accfood / ces_leis / co2 are clean;
   **`ces_amuse` (75 keys) and `usdeaths` (17) disagree at baseline**, so
   nothing is read off those. Related datum: with `noautooutlier=tramo`
   ces_amuse becomes bit-exact, which localises its default-path gap to the
   `amidot` arm of label 40 rather than to `iddiff`/`amdid`/`tstmd2`.
3. **A probe value has a DIRECTION, not just a distance.** `cancel` bites
   *below* its 0.1 default (0.05 moves 32 keys on nottem; 0.3/0.5/0.9 move 0).
   `ljungboxlimit` likewise moves at 0.5 and not at 0.99. A sweep that only
   pushes values up reports INERT.
4. **The transform can be load-bearing for a probe.** nottem under
   `function=log` moves 35 keys for `checkmu` and 32 for `cancel`; under
   `function=none`, 2 and 1.
5. **Blocks 2/3 of the AIC round are gated on the port's `aic` flag, not on
   `Itdtst`/`Leastr` as the Fortran gates them.** The parser/editor setup that
   fills `Tdayvc`/`Easvec`/`Neasvc` is unported and `automd_aictest_block1`
   stands in for it, so a round without block-1 indexes an uninitialised
   `Easvec` (`farray 1d index 0 out of [1,5]`). The m4 harness passes
   `do_aictest=false` and runs the AIC tests itself, which is how this crashed.
6. **`automd.f:343`'s `armats` is GUARDED on `.not.Lidotl`** — always FALSE on
   the default path, since Lotmod forces `Lidotl` on.
7. **A reverted file may not rebuild.** Restoring a `.bak` gives the source an
   mtime OLDER than the object file, so cmake skips it and the next test run
   silently uses the mutated binary. `touch` after any revert-by-copy.
8. Carried forward: an argument missing from the probe list reads exactly like
   one that measured INERT; `acceptdefault` is gated by
   `payems_automdl-acceptdefault`, not the airline spec named after it; the
   manual is not vendored (fetch from
   `www2.census.gov/software/x-13arima-seats/x13as/unix-linux/documentation/docx13as.pdf`,
   306 pages, PyMuPDF reads it) and `noautooutlier` appears nowhere in it.

## Open, in the order I would take them

1. **`pickmdl{}`** — still parse-only, and now the single largest source of
   real feature skips (9). It is `automx.f`, the X-11-ARIMA sibling of the
   `automd.f` front just closed, so the substrate (`rgarma`, `regvar`,
   `mdlset`, `mdlchk`, the outlier family) is all in place. Start by re-running
   the option sweep against it the way rounds 2-6 did for `automdl{}` —
   including the two traps that cost the most there: **probe values must be
   far from the default AND in the right direction**, and **the probe series
   must be verified clean at baseline first**.
2. **`gtdpvc` parses decimal literals 1 ulp off the nearest double**: `"0.95"`
   → `0.95000000000000007` vs the correctly-rounded `0.94999999999999996`.
   Latent everywhere a spec supplies a decimal. This was the reason the removed
   `ljungboxlimit` fatal had to compare with a tolerance rather than `==`, so
   the one place it was visibly biting is gone — it is now a latent-only issue.
   **Check whether the Fortran reader does the same before changing anything**;
   if it does, the port is faithful and this is documentation, not a fix.
3. The `Iagr==4` indirect names with the composite front (note the total's
   golden carries `npind*`/`spcind*` but no `qsind*`, and `genqs.f:439/482`
   uses a savelog index as a `Savtab` subscript — check before porting, may be
   a CB).
4. `spectrum{altfreq=yes}` pending CB-30; composite `agr3s.f`;
   `history{outlier=auto}` / `x11outlier=no` / `additivesa=`; the slidingspans
   `chs` per-span prior phase.
5. **Two baseline disagreements worth their own task**, both now isolated:
   `generated/usdeaths_automdl` (17 keys; the known iddiff d=0/d=1 split, and
   `nreg` 1 vs 0 — the engine is missing the Constant) and `ces_amuse`
   (75 keys, model 5 vs 4 ARMA terms, localised to label 40's `amidot` arm per
   finding 2). Neither is gated; both make every option verdict on those
   series uninterpretable.

## Environment notes

Unchanged (`/codex:cancel` broken; codex notifications carry no findings;
Windows Python cannot read git-bash `/tmp` or `/d/` mounts; no PowerShell
here-strings in the Bash tool; oracle flag order is
`x13as_ascii_O2.exe <specbase> -s`; ad hoc runs are
`build/x13run_{m3,x11,seats}.exe <spec>.spc` from the spec's own directory;
`option_sweep.py` stages every `tests/corpus/data/*.dat` and abspaths
`--outdir`; BLS needs a contact User-Agent, and
`download.bls.gov/pub/time.series/ce/ce.data.*` has full history where the
public API v1 silently caps an unregistered request at ~3 years; the Bash
tool's cwd persists across calls and is NOT shared with the PowerShell tool).

Adding a `core/src/**/*.cpp` still needs the build run twice — the first prints
`GLOB mismatch!` and stops.

## Methodology, reconfirmed

Every increment ended with a **mutation test**, and on auto-discovering gates it
is the only evidence the new specs are compared at all. This session added the
per-half refinement in finding 1.

**Re-measure separately, even when a shared cause is likely — and then again
after the port.** The 2026-07-28b handoff flagged "one further port explains
`mixed`/`maxdiff`/`maxorder`" as a hypothesis. Measuring split it three ways
(`maxdiff` closed on its own; `mixed` and `maxorder` failed on *disjoint*
series), and only the third increment closed the rest. The prediction that
`pass2` was the residual did hold — and the check that confirmed it was cheap
and specific: on every failing probe the oracle's FINAL model differed from its
own `automdl.first`, i.e. it had re-identified, which only `pass2`'s `igo` can
cause.
