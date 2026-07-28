# Session handoff — 2026-07-28b (the automdl option front)

Replaces the 2026-07-28 handoff. Its five findings and two CBs are still
accurate and are not repeated here; its open item 1 (`automdl{}`) is what this
session worked, and it is now closed as far as parsing can take it.

## Where things stand

**Tree clean at `d1b65033` on `checkpoint/m5-seats-slidingspans`.** Nothing
uncommitted, no background work outstanding.

| check | result |
|---|---|
| `python -m pytest tests/parity -q` | **5509 passed / 0 failed / 478 skipped** (~4m15s) |
| `cd build && ctest` | 11/11 |
| `Rscript bindings/r/test_x13c.R` | 165/165 (not re-run; untouched surface) |

Standing constraints, unchanged: **never merge this branch to `main`, never
push**; **never run `tests/corpus/generated/genspecs.py` or
`tests/corpus/extra/genextra.py`** (both wipe committed specs they cannot
regenerate). Build with
`export PATH="/c/rtools44/x86_64-w64-mingw32.static.posix/bin:$PATH" && cmake --build build -j 6`.

## What landed (5 commits, 5491 → 5509 passing)

| commit | what |
|---|---|
| `e8024c85` | re-probe automdl on seasonal near-tied series — 7 dropped options |
| `4a6f8c30` | parse the automdl arguments + fix the `diff=` engine defect |
| `386d2a5f` | gate `diff=`/`balanced=`, pin the CES probe series |
| `310a9f10` | parse `ljungboxlimit=` and fatal — its "applied" verdict was wrong |
| `d1b65033` | parse `cancel=`, and stop — the rest is one blocker |

## The one thing to know

**The whole remaining `automdl{}` option surface is gated on a single unported
port: `automd.f`'s model-adequacy stage (`:577-850`) plus the `pass2`/nloop call
at `:665`.** Five arguments now parse correctly and all five still land
somewhere other than the oracle; three more are consumed *only* there and are
now fatal. Further parse cases will close none of them.

| state | arguments |
|---|---|
| applied + gated | `diff` `balanced` `acceptdefault` |
| **DIFFERS** (read, wrong answer) | `mixed` `checkmu` `maxorder` `maxdiff` `cancel` |
| **FATAL** (blocked, previously silent) | `urfinal` `noautooutlier` `ljungboxlimit` |
| INERT even at extreme values | `ub1` `ub2` `exactdiff` `hrinitial` `armalimit` `percentrse` `reducecv` `firstar` `fcstlim` `seasonaloverdiff` |
| harness-blind | `rejectfcst` |

`tstmd1`/`bkdfmd`/`testodf` are already ported in `automdl/adqtst.cpp`. The
reason they are not wired is recorded at `automd.cpp`'s closing note: wiring
`tstmd1` alone broke parity on the non-revert cases, because the oracle
**re-estimates after** `tstmd1`, so its intermediate fit must not be the
reported one. The finalization has to land as a unit.

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

1. **Wire `automd.f`'s model-adequacy + nloop stage** (task #8). Unblocks the
   five DIFFERS and the three FATALs at once. That the stage explains all five
   is a **hypothesis, not a measurement** — isolate it, and re-measure each
   argument separately afterwards rather than declaring the group closed.
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
fails its gate; hardcoding `acceptdefault=false` fails 32.
