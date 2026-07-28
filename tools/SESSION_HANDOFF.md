# Session handoff — 2026-07-28c (automd unified; `pass2` is the residual)

Replaces the 2026-07-28b handoff. Its findings list is still accurate and is
carried forward below where it still matters; its open item 1 is what this
session worked, and it is closed.

## Where things stand

**Tree clean on `checkpoint/m5-seats-slidingspans`.** Nothing uncommitted, no
background work outstanding. `git log --oneline -8` for the current head — this
file is written *before* the commit that contains it, so any SHA named here is
necessarily one behind. (It has gone stale that way twice; hence no SHA.)

| check | result |
|---|---|
| `python -m pytest tests/parity -q -n 8` | **5542 passed / 0 failed / 478 skipped** (~80s) |
| `cd build && ctest` | 11/11 |
| `Rscript bindings/r/test_x13c.R` | 165/165 (not re-run; untouched surface) |

**Run the suite with `-n 8`** (pytest-xdist, installed). 262s serial → ~80s,
same counts. Safe because every gate compares stdout from a read-only
subprocess and no harness writes side files — re-verify that if a harness ever
changes. Build is ~25s; use `-k "<name>"` (~3s) while iterating.

Standing constraints, unchanged: **never merge this branch to `main`, never
push**; **never run `tests/corpus/generated/genspecs.py` or
`tests/corpus/extra/genextra.py`** (both wipe committed specs they cannot
regenerate). Build with
`export PATH="/c/rtools44/x86_64-w64-mingw32.static.posix/bin:$PATH" && cmake --build build -j 6`.

## What landed (2 commits, 5516 → 5542 passing)

| commit | what |
|---|---|
| `dcbfc35b` | **unify automd's two paths** — one path, as in the Fortran; closes `noautooutlier=` |
| `aa721bc7` | gate `checkmu=` / `cancel=` / `maxdiff=`; correct the `pass2` reachability note |

## The one thing to know

**`automd.cpp` no longer has an aictest branch.** `automd.f` has ONE path and
the AIC tests are conditional *blocks* inside it; this driver had mirrored them
as a branch, and that split bit twice in two sessions. Last session it was the
label-30 finalization tail, reached only from the aictest side. This session it
was label 40 itself plus three things its `tstmd1` arm reads — `tair`
(`automd.f:343`), `adj0`/`trns0` (`:369-371`) and **`bkdfmd`'s backup**
(`:379`, which `tstmd1.f:221` restores from). Hoisting them meant hoisting the
acceptdefault test, the `a0` saves, label 10 and the whole
put-the-regressors-back block as well. **Measured free** — 5516 → 5516 with no
golden moving — even though `ssprep`/`bkdfmd`/`rmfix`/`addfix`/`clrotl`/`restor`
and `amidot` now run on every automdl spec.

Where the arguments stand after re-measuring each one **separately** (the
standing instruction from the last handoff — it earned its keep, the group split
three ways and not the predicted way):

| state | arguments |
|---|---|
| applied + gated | `diff` `balanced` `acceptdefault` `urfinal` **`noautooutlier`** **`checkmu`** **`cancel`** **`maxdiff`** |
| **DIFFERS** | `mixed` (ukgas ok, 3 series wrong) · `maxorder` (3 ok, ukgas wrong) |
| **FATAL** (blocked, not silent) | `ljungboxlimit` |
| INERT even at extreme values | `ub1` `ub2` `exactdiff` `hrinitial` `armalimit` `percentrse` `reducecv` `firstar` `fcstlim` `seasonaloverdiff` |
| harness-blind | `rejectfcst` |

**All three remaining are one port: `pass2`.** And the note claiming `pass2`
was unreachable — which I wrote in `dcbfc35b` and corrected in `aa721bc7` — was
wrong. Its guard is `IF(Lidotl.and.nloop.le.2)` **and nothing else**; the
default `Lotmod` forces `Lidotl` true, so the oracle calls `pass2` on every
automdl run. Its `ichk` revert is guarded `Naut0.le.Naut`, i.e. `0.le.0` here,
so the BIGCV scan finding nothing does not close it either. It is a no-op on
every corpus spec's DEFAULT configuration, which is why the suite is green
without it.

That is the same lesson as last session, inverted: **"unported" and
"unreachable" present identically, and "unreachable" is a claim about a GUARD
— read it off the guard, not off a green suite.** A green suite is consistent
with "no-op on this corpus", which is a different statement.

## Findings worth not re-deriving

1. **Blocks 2/3 of the AIC round must be gated on the port's `aic` flag, not on
   `Itdtst`/`Leastr` as the Fortran gates them.** The parser/editor setup that
   fills `Tdayvc`/`Easvec`/`Neasvc` is unported and `automd_aictest_block1`
   stands in for it, so a round without block-1 indexes an uninitialised
   `Easvec`. This crashed the four `test_m4_aictest` gates on the first
   attempt (`farray 1d index 0 out of [1,5]`) — that harness passes
   `do_aictest=false` and runs the AIC tests itself.
2. **`automd.f:343`'s `armats` is GUARDED on `.not.Lidotl`.** The old
   aic-branch call carried a comment asserting the guard was always true there;
   it is always FALSE on the default path, since Lotmod forces `Lidotl` on.
3. **Verify the engine's BASELINE before reading any option verdict off a
   series.** Re-checked this session: ukgas / nottem / ces_accfood / ces_leis /
   co2 are clean at baseline; **`ces_amuse` (75 keys) and `usdeaths` (17)
   disagree**, so no verdict is read off those. New datum: with
   `noautooutlier=tramo` ces_amuse becomes bit-exact, which localises its
   default-path gap to the `amidot` arm of label 40 rather than to
   `iddiff`/`amdid`/`tstmd2`.
4. **A probe value has a DIRECTION, not just a distance.** `cancel` bites
   *below* its 0.1 default — 0.05 moves 32 keys on nottem, while 0.3/0.5/0.9 all
   measure exactly 0. A sweep that only pushes values up reports INERT.
5. **The transform can be load-bearing for a probe.** nottem under
   `function=log` moves 35 keys for `checkmu` and 32 for `cancel`; under
   `function=none`, 2 and 1. The spec would have exercised the plumbing and
   measured nothing.
6. **A reverted file may not rebuild.** Restoring a `.bak` gives the source an
   mtime OLDER than the object file, so cmake skips it and the next test run
   silently uses the mutated binary. `touch` the file after any revert-by-copy.
7. Carried forward from 2026-07-28b: a probe series needs strong seasonality
   AND near-tied candidates; an argument missing from the probe list reads
   exactly like one that measured INERT; `acceptdefault` is gated by
   `payems_automdl-acceptdefault`, not by the airline spec named after it; the
   manual is not vendored (fetch from
   `www2.census.gov/software/x-13arima-seats/x13as/unix-linux/documentation/docx13as.pdf`,
   306 pages, PyMuPDF reads it) and `noautooutlier` appears nowhere in it.

## Open, in the order I would take them

1. **Port `pass2` / the nloop re-entry** (`automd.f:664-672` + `pass2.f`). One
   port, three arguments: `ljungboxlimit` (fatal today), `mixed` (wrong on
   nottem/ces_leis/ces_accfood) and `maxorder` (wrong on ukgas). Two halves:
   `pass2.f:46-150`'s `ichk` revert-to-default (compares the identified model's
   Ljung-Box `Plbox`/residual variance `Rvr` against the default's, restores
   `a0`/`lmu0`/`lpr0..lqs0` and calls `bkdfmd(F)` + `regvar` + `rgarma`), and
   `:160-190`'s `Pcr` increment + `Igo` (1 → GO TO 10, 2 → GO TO 40, 3 → GO TO
   50), which is the loop `nloop` counts. **The evidence that this is the right
   target, and the check to repeat after:** on the failing probes the oracle's
   FINAL model differs from its own `automdl.first`, so it re-identified — and
   only `Igo` can cause that. `automd.cpp` already has the `nloop`/`nround`
   locals stubbed at 1. Gate with `mixed=no` on nottem/ces_leis (the oracle
   gains AR lags) and `maxorder=(1 1)` on ukgas (nefobs 103 vs 104).
2. **`gtdpvc` parses decimal literals 1 ulp off the nearest double**: `"0.95"` →
   `0.95000000000000007` vs the correctly-rounded `0.94999999999999996`. Latent
   everywhere a spec supplies a decimal, and it is why the `ljungboxlimit` fatal
   compares with a tolerance rather than `==`. **Check whether the Fortran
   reader does the same before changing anything** — if it does, the port is
   faithful and this is documentation, not a fix.
3. The `Iagr==4` indirect names with the composite front (note the total's
   golden carries `npind*`/`spcind*` but no `qsind*`, and `genqs.f:439/482`
   uses a savelog index as a `Savtab` subscript — check before porting, may be
   a CB).
4. **`pickmdl{}`** — still parse-only, and the single largest source of feature
   skips.
5. `spectrum{altfreq=yes}` pending CB-30; composite `agr3s.f`;
   `history{outlier=auto}` / `x11outlier=no` / `additivesa=`; the slidingspans
   `chs` per-span prior phase; `generated/usdeaths_automdl`'s iddiff d=0/d=1
   (baseline-broken, 17 keys — see finding 3).

## Environment notes

Unchanged (`/codex:cancel` broken; codex notifications carry no findings;
Windows Python cannot read git-bash `/tmp` or `/d/` mounts; no PowerShell
here-strings in the Bash tool; oracle flag order is
`x13as_ascii_O2.exe <specbase> -s`; ad hoc runs are
`build/x13run_{m3,x11,seats}.exe <spec>.spc` from the spec's own directory;
`option_sweep.py` stages every `tests/corpus/data/*.dat` and abspaths
`--outdir`; BLS needs a contact User-Agent, and
`download.bls.gov/pub/time.series/ce/ce.data.*` has full history where the
public API v1 silently caps an unregistered request at ~3 years).

One addition: the Bash tool's cwd persists across calls and is NOT shared with
the PowerShell tool. A `cd tests/parity` in one call leaves the next call
there; prefix with `cd /d/code_projects/x13new &&` when it matters.

## Methodology, reconfirmed

Every increment ended with a **mutation test**, and on auto-discovering gates it
is the only evidence the new specs are compared at all. Disabling the `tstmd1`
arm fails 5 of the 11 `noautooutlier` tests; hardcoding
`lchkmu`/`cancel`/`diffam` to their defaults fails 9 of the 15 for the three
newly gated arguments — including at least one gate per spec, which is the
property to check rather than the raw count.

**Re-measure separately, even when a shared cause is likely.** The last handoff
flagged that "one further port explains `mixed`/`maxdiff`/`maxorder`" was a
hypothesis. It was wrong in an informative way: `maxdiff` closed entirely,
while `mixed` and `maxorder` turned out to fail on **disjoint** series.
