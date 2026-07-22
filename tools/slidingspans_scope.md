# `slidingspans{}` port scope

Companion to `tools/testgen_scope.md` section 2/6 (which first mapped the
spec syntax and save tags at a high level). This document goes one level
deeper: the call tree of the sub-span replay driver, the COMMON blocks
involved, the table-writing mechanism, and a phased plan for whoever
finishes the driver.

## Status as of this session

**Landed** (parser session): the M1 spec parser (`getssp.f` -> `gt_slidingspans`).

## Status as of the driver session (phases 1-6 of &sect;5 below)

**Landed**: the re-entrant sub-span replay driver
(`core/src/driver/run_x11_span.{hpp,cpp}` -- phase 2) and the slidingspans{}
orchestration (`core/src/x11/slidingspans.{hpp,cpp}` -- `sfmax_span`/
`ssprep_snapshot`/`restor_span`/`ssmdl_fix_model`/`setssp_span` = setssp.f;
`ssrit` wired into the 3 x11parts.cpp call sites; `run_slidingspans` =
sspdrv.f's loop + ssap.f's xchng/mflag/rplus for **sfs/chs only**, per
&sect;3's gating analysis -- phases 1, 3-6). `tools/x13run_x11.cpp` gained
`dump_span_table` (the wide-table emission convention phase 6 called for) and
`tests/parity/test_slidingspans_tables.py` has a real produced-side reader
(no longer a stub).

**Driver design**: `run_x11_span(ctx, trnsrs_full, has_model, nlen, nfcst,
nbcst, nbcst2, lsp)` takes ONLY the padded-buffer window description (not a
calendar span) -- mirroring ssx11a.f's own dependency order, it calls
`setxpt` first (from Lsp alone) and THEN derives Begspn/Endspn from the
resulting Pos1ob/Posfob via the ctx-global `x11opt.lyr`/`ny` anchor + the
setssp-computed constant `ssap.im`. It replays the regARIMA model with
`ctx.model.arimaf` held true (the Ssinit==1/fixmdl=yes default -- see
`ssmdl_fix_model`), so `rgarma` takes exactly one pass (Nestpm==0) recomputing
residuals/forecasts over the shorter window without re-optimizing, then runs
the ordinary x11int/x11pt1/x11pt2/x11pt3 spine. Scoped/validated for Nb==0
(no regression{}); Nb>0 fields are threaded through generically but untested.
`ctx.x11opt.lyr` (the calendar-year anchor editor.f sets once,
`Lyr=Begspn(1)`) had NO writer anywhere in this port before this session --
added to `run_x11.cpp` since slidingspans is the first consumer.

**Two real PORT bugs found and fixed** (not Census bugs -- ours, not the
oracle's -- so not logged in `census_bugs.md`; both latent because no
previously-gated spec ever exercised a replayed sub-span, i.e. `Pos1bk>1`):
1. `x11pt2`'s `Stex` (COMMON `/mq10/`) and `x11pt3`'s `Stsie` (COMMON
   `/work3/`) were function-local C arrays -- losing all history between
   calls. A single main run never noticed (Pos1bk always 1, and nothing
   downstream meaningfully reads back across a call boundary for that case);
   a replayed sub-span DOES read that history (x11pt3.f's `Ksdev>1` branch:
   `CALL copy(Stsie,Posfob,1,Stsi)` relies on `Stsie` already holding real
   data at `[1,Pos1bk-1]`, exactly as a real Fortran COMMON would). Promoted
   to ctx-persistent buffers: `ctx.mq10_stex`, `ctx.work3_stsie`
   (`x13context.hpp`). Without this fix the replay produced NaN.
2. `ssmdl.f`'s `Ssinit==1` "fix the whole model" tail writes `Fxa`/`Ap2` --
   `ssprep.cmn`'s SNAPSHOT fields, which `restor_span()` (ssx11a.f's `CALL
   restor(...)`, run before EVERY span) resets the LIVE `Arimaf`/`Arimap`
   FROM -- not just the live `model.cmn` `Arimaf`/`Arimap`. Fixing only the
   live copy is silently undone by the very next `restor_span()` call, so
   every span was re-estimating (`Nestpm==2`) instead of replaying the fixed
   model (`Nestpm==0`). Fixed in `ssmdl_fix_model` to write both.

## Status as of the follow-up audit session

**Audited and ruled out**: every function-local x11pt2/x11pt3 scratch array
(`temp`, `stime`, `ckhs`, `ststd`, `biasfc`, `sp2`, `stbase`) plus the scratch
owned by every sub-routine they call (`xtrm`'s `Stwt`/`Stdper`/`Stdev`
accumulation, `replac`, `vsfb`/`vsfa`/`averag`, `vtest`, `entsch`, `sdxtrm`)
for the same "reads the dead zone `[1,Pos1bk-1]` as if it held real prior-
call data" pattern that broke `Stex`/`Stsie`. Method: traced every
`copy`/`setdp`/direct-write into each array and confirmed each one is either
(a) fully self-contained -- written across its own full range before any
read, in the SAME call, so a fresh/uninitialized array is harmless -- or (b)
a full-range `setdp`/memset-style fill (e.g. `xtrm`'s
`for(t=1;t<=klda;++t)stwt[t-1]=ONE`, which unconditionally resets `Stwt`
from index 1 on every single call, so it never depends on cross-call
history despite starting from 1 not `Kfda`) -- or (c), for `stbase`
specifically, dead code for this gate: it lives inside x11pt3.f's
`Iyrt>0`/force{} branch, downstream of the `Issap==2` early `RETURN` this
gate always takes. **No further Stex/Stsie-class bug was found.** Also
verified by line-by-line diff against `oracle/fortran/xtrm.f`/`vtest.f`/
`entsch.f` that the `Ksdev>0` branch (the one span 1/2/4 exercise, that span
3 -- bit-exact -- does not) is a faithful, unmodified port; no algorithmic
divergence there either.

**Empirical localization** (per (date,span) diff against the golden's own
per-span columns -- the golden literally ships oracle's per-sub-span output,
so no extra oracle instrumentation was needed): **span 3 of 4 is bit-exact
to ~1e-15 for BOTH sfs and chs**, proving the driver architecture and the
Ssinit==1 fixed-model replay are fundamentally correct. Spans 1, 2, 4 are
not exact (worst relative error: span1 ~3.45%, span2 ~1.0%, span4 ~0.96%,
sfs; chs shows the same shape but with inflated relative numbers from near-
zero denominators). Span 1's error is concentrated specifically at June of
its own first year (the edge row closest to Part B's first directly-
averaged trend point, `Mfda=Pos1bk+Ny/2`) and decays geometrically across
span 1's own later years (3.45% -> 3.15% -> 2.49% -> 1.75% -> 0.95% ->
0.46% -> 0.16%) via the seasonal-MA filter's cross-year blending of the
same calendar month -- the signature of ONE mis-flagged/mis-replaced
extreme-value observation early in span 1's own re-estimated residual
series, not a broad structural bug. A diagnostic-only experiment (forcing
`Ksdev`/`Kersa` to 0 at the top of each span, then reverted) did not change
the result at all -- `entsch` recomputes them within Part B's own iteration
regardless of their pre-loop value -- which rules out that value as a
direct lever even though it correlates with which `xtrm`/`sfmsr` branch
fires.

**Not yet bit-exact, root cause not yet found within this (second)
session's budget either.** Leads for whoever continues:
- Span 1 is both the shortest-lookback-to-main-run TRANSITION span (the
  first replay executed, immediately after the differently-shaped main
  run) and the one with by far the largest error -- worth checking whether
  `rgarma`'s residual computation for the FIRST few realizations of a
  freshly re-estimated (fixed-coefficient) short ARIMA window differs from
  a hypothetical continuously-running oracle process in some way specific
  to the "process just transitioned to a new, shorter Nspobs" case (e.g. a
  differencing/backcast edge in `armafl`/`resid` not fully independent of
  the PREVIOUS call's `Nspobs`).
- Since span 3 is exact, diff span 3's OWN intermediate arrays (Sts/Stci at
  every row, Stwt, Stdper) against span 1's at the SAME relative offset
  from `Pos1bk` (not the same absolute row) -- if the bug is structural
  it should reproduce at the analogous position in span 1 even though it
  doesn't in span 3.
- `tests/parity/test_slidingspans_tables.py` xfails (not strict) with the
  measured numbers above; un-xfailing needs the remaining gap found first.

## `history{}` reuse note

`driver/run_x11_span.hpp`'s design should carry over directly:
`history{}`'s `revdrv.f` replays the SAME "model+X11 pipeline over an
in-memory sub-span of the already-parsed/estimated ctx" primitive -- a
revision-history run holds `Endspn` fixed at each of several successive
cutoff dates and re-estimates/re-adjusts up to that point, which is exactly
`run_x11_span`'s `(nlen, nfcst, nbcst, nbcst2, lsp)` window description
(`history{}` would compute a different `Lsp`/`Nlen` sequence via its own
`revchk.f`-equivalent bookkeeping, analogous to `setssp_span`, but call the
SAME `run_x11_span`). Two things `history{}`'s port will need beyond what
slidingspans{} required:
1. **The fixed-model assumption may not hold.** slidingspans{}'s default
   (`Ssinit==1`/`fixmdl=yes`) let `run_x11_span` always take the `Nestpm==0`
   ("replay, don't re-optimize") path. `history{}` re-estimates the model
   AT EACH CUTOFF by default (there is no `Ssinit`-equivalent fixing
   convention for it in the oracle) -- so `run_x11_span`'s `has_model`
   branch will need to exercise `rgarma` with `Nestpm>0` for the first time
   in a replayed-sub-span context. Given this session's still-open span-1-
   specific gap sits somewhere in the fixed-model residual/extreme-value
   path, get slidingspans{} to full bit-exactness FIRST -- history{} adds a
   free axis (re-optimization) on top of a foundation that isn't proven
   solid yet.
2. **`ssprep_snapshot`/`restor_span` are scoped to Lmodel's ARIMA-only
   fields (Nb==0)** -- `history{}` runs are far more likely to carry
   regression{} (TD/holiday/user) than slidingspans{}'s gate corpus did, so
   the `Nb>0` fields left un-ported in both functions (the `Ngrp`/`Colttl`/
   `Grpttl`/`Regfx2`/`Rgv2` group-and-column snapshot/restore) will need
   finishing before a regression-carrying `history{}` spec can be trusted.

### Files touched this session

- `core/src/specparse/lexstate.hpp` -- added `ParseSettings::has_slidingspans`
  and `ParseSettings::ssp_cut` (the non-COMMON `Sscut(5)` cutoff array;
  getssp.f's `Sscut` is a plain local threaded gtinpt.f->getssp.f->editor.f,
  not a Fortran COMMON, so it has no generated `gen/*_cmn.hpp` home -- it's
  captured the same way `transform_power` etc. already are).
- `core/src/specparse/specparse.hpp` -- declared
  `gt_slidingspans(X13Context&, bool& havesp, bool& inptok)`.
- `core/src/specparse/readers_spec.cpp` -- `gt_slidingspans()` body, mirroring
  `gt_force()`'s structure (a `gtarg` loop over the 16-argument dictionary,
  faithful to getssp.f's GOTO-per-argidx branches, including the exact
  post-loop `Inptok=Inptok.and.argok; IF(Inptok)Issap=1` tail semantics --
  `argok` is declared *outside* the loop, since its value from the *last*
  processed argument is what the oracle's tail check reads).
- `core/src/specparse/gtinpt.cpp` -- wired dispatch case 13 to call
  `gt_slidingspans`, plus the `IF(.not.Ssdiff)Ssidif=Ssdiff` line that sits
  in gtinpt.f right after the `CALL getssp(...)` (not inside getssp.f
  itself).
- `tests/parity/test_m1_parse.py` -- removed `"slidingspans"` from
  `_UNPARSED_BLOCKS` (both corpus specs using it now parse OK, matching the
  oracle's fatal/ok outcome).
- `tests/parity/test_slidingspans_tables.py` -- new gate file (xfailed --
  see &sect;6).

### Where the parsed state lands

Unlike `force{}` (whose fields all live in one real COMMON, `force.cmn`,
already generated as `force_cmn`/`ctx.force`), `slidingspans{}`'s state is
split:

| Fortran | C++ | Notes |
|---|---|---|
| `Issap` | `ctx.hiddn.issap` | `hiddn_cmn`, already in `x13context.hpp`. 0=unset, 1=parsed-ok (set by us), 2=mid-replay (driver only), 3=done (driver only). |
| `Ncol`, `Nlen`, `Ssotl`, `Ssinit`, `Ssfxrg`, `Nssfxr`, `Strtss`, `Sstran`, `Ssdiff`, `Ssidif`, `Ssxotl`, `Ssxint` | `ctx.sspinp.*` | `sspinp_cmn` (`/sspinp/`+`/ssplog/`), already generated, already in `x13context.hpp` (`ctx.sspinp`). |
| `Sscut(5)` | `ctx.captured.ssp_cut` | **Not** a COMMON in the oracle -- see above. Consumed later by `editor.f:2219-2221` to build `ssap_cmn.cut(5,4)` (the driver's job, not the parser's). |
| `Ssfxxr`, `Nssfxx` | `ctx.sspinp.ssfxxr/nssfxx` | Present in the generated struct but **not** touched by `getssp.f` at all -- they belong to the irregular-component (x11regression) fixing path, set elsewhere. Left untouched by `gt_slidingspans`. |

## 1. Call tree (oracle)

```
gtinpt.f (case 13)
  -> getssp.f            [PORTED -> gt_slidingspans]  parses slidingspans{}, Issap=1

... (later, once modeling+X11 setup for the MAIN run is complete) ...

x13amain.f / whichever driver invokes the post-model X-11 phase:
  -> sspdrv.f  (443 lines)   [NOT PORTED]  the sliding-spans driver proper
       -> setssp.f   (360 lines)  span-length/date bookkeeping, resolves
                                  Nlen/Ncol defaults, Strtss -> Pos1ob/Posfob
                                  (integer offsets into the full series),
                                  Issap = 2 once setup is valid
       -> LOOP j = 1..Ncol:
            x11int                          (already ported: x11drv.hpp)
            ssx11a.f (287 lines)  [NOT PORTED]
                -- points Lsp/Begspn/Endspn/Begmdl/Endmdl at sub-span j's
                   window (a moving Nlen-long slice ending Ncol-j years
                   before the main run's end), calls setxpt (ALREADY PORTED)
                -- if Ssinit==2 (fixmdl=clear): reset Arimap/B/Bx to DNOTST
                   (start params) before re-estimating
            chusrg/bakusr    [NOT PORTED]  user-regressor "well-defined for
                                            this span?" fixing
            x11ari.f (389 lines, ALREADY PORTED for the main run -- see
                      core/src/driver/run_x11.cpp / run_pre_model.cpp)
                -- re-estimates the regARIMA model (unless fixed) and runs
                   the X-11 spine for this span; INSIDE this call, at the
                   3 x11parts.cpp sites gated on Issap==2, ssrit.f
                   [NOT PORTED] stores this span's per-period Sa/S/Td into
                   ctx.ssap's MXLEN x MXCOL accumulator arrays
            outlier/user-regressor restore for next iteration
       Issap = 3   (all spans done)
       -> ssap.f (412 lines)  [NOT PORTED]
            -- computes month/qtr-to-month/qtr changes (xchng), flags
               extreme values (mflag), range checks (ssrng), then WRITES
               the actual save tables via svspan() for each of the 5
               estimate kinds (S, Td, Sa, c=chg, yy=y/y chg)
```

Two already-ported pieces the driver can lean on:
- **`x11int`, `setxpt`, `dfdate`** (`core/src/x11/x11drv.hpp`,
  `core/src/driver/run_x11.cpp`) -- ssx11a.f's span-pointer setup uses
  exactly these.
- **`x11ari` equivalent**: `core/src/driver/run_x11.cpp`'s `run_x11()` +
  `run_pre_model.cpp`'s `run_m2_after_parse()` together already implement
  "parse a spec, estimate the model, run the X-11 spine" for the *main*
  (single, full-length) span. **They are not directly reusable as-is for
  the replay loop**, because they parse `spec_text` from scratch each call
  (there is no lower-level "re-run over an in-memory sub-span of an
  already-parsed ctx" entry point yet) -- see &sect;4.

## 2. COMMON blocks / generated structs already available

All of the sliding-spans-specific COMMON blocks are **already generated**
(via `tools/cmn2hpp.py`, run at some earlier point for the whole oracle
tree) and wired into `X13Context`:

- `ssap_cmn` (`ctx.ssap`) -- `/sspcmn/`: the accumulator/bookkeeping common
  (`Cut`, `Iyr`, `Im`, `Nsea`, `Lyear`, `Lobs`, `Ns1`, `Sslen`, `Sslen2`,
  `Ic`, `Icyr`, `Icm`, `Ntot`, `Itot`, `Kount`, `Indssp`, `Indcol`,
  `Indlen`, `Nscomp`, `Itd`, `Ihol`, plus format strings `Ch/F1/F2/F3` --
  the format strings are printing-only, skippable for a numeric-only port).
- `sspinp_cmn` (`ctx.sspinp`) -- `/sspinp/`+`/ssplog/`: parser output, see
  table above.
- `sspdat_cmn` (no ctx member yet -- **not included in x13context.hpp,
  needs adding**) -- `/sser/`: the big `MXLEN x MXCOL` (276x4) accumulator
  arrays `Sfind/Sfinda/Saind/S/Sa/Isfadd/Td` that `ssrit.f` writes into and
  `ssap.f`/`svspan` read out of. **This is the main data the driver
  produces.**
- `ssprep_cmn` (no ctx member yet) -- `/ssprp/`: a saved-copy of the
  regression-matrix/model working arrays (`Ap2/Bb/Chx2/...`), used by
  `ssprep.f` to snapshot/restore state around user-regressor deletions
  during the per-span replay (sspdrv.f calls `ssprep(T,F,F)` after
  `dlrgef`/`bakusr`).
- `sspvec_cmn` (no ctx member yet) -- `/ssvec/`: per-span observation/
  turning-point bookkeeping (`Aobs/Ayr/Chsgn/Iturn/SSnobs/SSnyr/Csign/Per/
  Cturn`) feeding the histogram/breakdown tables (`sshist.f`/`btrit.f`) --
  these are the *summary* tables (F-tests, histograms, breakdown by month),
  lower priority than the raw span tables (sfs/chs/ads/tds/ycs) the gate
  targets.

Action for whoever picks this up: add `sspdat_cmn sspdat;`, `ssprep_cmn
ssprep;`, `sspvec_cmn sspvec;` members to `X13Context` (mirroring how
`ssap`/`sspinp` are already there) -- 3-line addition, already-generated
headers exist in `core/src/common/gen/`.

## 3. Table-writing mechanism (what the gate actually needs)

`ssap.f`'s tail (lines 328-401) is a `DO i=1,NEST` (NEST=5: seasonal
factors, TD factors, SA series, month/qtr changes, year/year changes) that,
for each estimate whose `Ntot(i)` got set (i.e. `mflag` was actually called
for it -- see the gating conditions below), calls `svspan(array, i, dmax,
ispan, Ncol, ...)` where `array` is one of `S`/`Td`/`Sa`/`c`/`yy` --
literally the **raw per-span values**, not a single summary statistic. This
is what lands in the `.sfs`/`.tds`/`.ads`/`.chs`/`.ycs` save files: one row
per date, one column per span (`Span1..SpanN`), plus a trailing
`Max_%_DIFF` column, using `-999` as the "this span doesn't cover this
date" sentinel (spans are staggered, each starting exactly one year later
than the previous one -- see `Iyr+l-1`/`Lyear+l-1` in ssap.f:307-314).

`mflag()` (extreme-value flagging, also what sets `Ntot(i)` so a table gets
written at all) is called conditionally per estimate kind:

```fortran
IF(Muladd.eq.0)THEN
 IF(Kfulsm.eq.0)CALL mflag(S,1,...)                          ! sfs: always (mult. adj)
 IF(Iagr.lt.6)THEN
  IF(Itd.eq.1)CALL mflag(Td,2,...)                            ! tds: only if Itd==1
  IF((Kfulsm.eq.0.and.(Lrndsa.or.Iyrt.gt.0.or.Itd.eq.1))
     .or.Ihol.eq.1)CALL mflag(Sa,3,...)                       ! ads: only if round=yes,
 END IF                                                       !      force{}, TD, or holiday adj
END IF
CALL mflag(c,4,...)                                           ! chs: always
IF(Lyy)CALL mflag(yy,5,...)                                   ! ycs: only if Lyy (requested)
```

**This explains the gate scope precisely**: the committed golden
`tests/golden/extra/airline_slidingspans/` (and its cutseas sibling) ship
only `.sfs`/`.chs` -- confirmed by listing the golden dir, not by parsing
oracle comments -- because that spec has no TD regressor (`Itd=0`), no
`round=yes`/`force{}` (so `ads`'s gating condition is false), and no
holiday adjustment. **This is expected oracle behavior, not a missing
golden.** `tds`/`ads` will need a *different* corpus spec (one with a TD
regressor, or `round=yes`/`force{}` layered on) to ever have a golden to
gate against. `ycs` similarly needs `Lyy` true, which needs
`print`/`save`/`savelog` to request the year-to-year table specifically (it
is not in the default `print=all` set the way sfs/chs are -- verify against
a real oracle run before assuming, per the tolerance-policy lesson in
`tools/testgen_scope.md` &sect;3: "trust the run over the static Fortran
read").

## 4. Why this is the biggest of the three (force/slidingspans/history)

`force{}` (already ported) is pure arithmetic bolted onto the *existing*
single main-span D11 -- no re-estimation, no replay. `slidingspans{}` (and
`history{}`, its sibling) instead **replay the entire regARIMA + X-11
pipeline** `numspans` times over overlapping data windows, each time
optionally re-estimating the ARIMA model (governed by `fixmdl`/`Ssinit`)
and reusing (with selective per-span exceptions) the main run's regression
setup (`fixreg=(td holiday user outlier)` lets the user hold specific
regressor classes fixed across all spans rather than re-estimating them
each time).

The core architectural question for whoever picks this up: **how to get a
re-entrant "run the model+X11 pipeline over span X, optionally with these
parameters fixed" call**, since `run_x11()` (`core/src/driver/run_x11.cpp`)
today only knows how to parse-and-run a spec text end-to-end once. Two
paths, in increasing faithfulness (and cost) order:

1. **Spec-synthesis approach**: build a per-span `spec_text` (same blocks,
   different `span{}`/`file=`-implied window) and call `run_x11()` once per
   span, capturing D10/D11/D13 buffers. Cheap to prototype, but does NOT
   reproduce `fixmdl`/`fixreg`/`Ssinit=clear-vs-yes` semantics (each call
   re-estimates independently from scratch, unlike the oracle which shares
   COMMON state -- e.g. `Ssinit.eq.1` keeps the main run's converged ARMA
   estimates as the *starting values* for each span's re-estimation, not a
   totally fresh fit) and would very likely NOT be bit-exact against the
   goldens' 15-significant-digit tolerance, since a fresh-fit optimizer run
   can converge to a different point in a flat likelihood region even when
   "close" is good enough for eyeballing.
2. **In-process re-entrant driver**: refactor `run_x11()`/`run_m2_after_parse()`
   into a lower-level function taking an already-populated `ctx` plus a
   span-window override (mirroring `ssx11a.f`'s `Lsp`/`Begspn`/`Endspn`
   pointer relocation) and an estimation-fixing mode (mirroring
   `Ssinit`/`fixreg`). This is the bit-exact-faithful path and matches how
   `history{}`'s `revdrv.f` will need the *same* refactor (both features
   are blocked on the same underlying capability: a re-entrant "re-run
   over this span with these parameters fixed" driver call). Building it
   once benefits both features.

Given the size (ssap.f 412 + sspdrv.f 443 + ssx11a.f 287 + ssrit.f 138 +
setssp.f 360 + ssprep.f 116 = ~1750 oracle lines, before touching
chusrg/bakusr/xchng/mflag/svspan/pctrit/btrit/sshist/ssrng/ssftst/ssphdr
helpers), this is comparable in scope to a new milestone, not an
incremental addition -- consistent with `tools/testgen_scope.md`'s original
sizing ("bigger lift than force").

## 5. Phased plan for the driver

1. Add the 3 missing `X13Context` members (`sspdat`, `ssprep`, `sspvec` --
   see &sect;2) -- trivial.
2. Build path 2's re-entrant span-window driver call (the shared blocker
   for both slidingspans and history) as its own reviewable unit, proven
   against a **non-replayed** sanity check first: call it once with the
   *full* span and confirm it reproduces the existing single-span
   `b1`/`d10`-`d13` gate bit-exactly (a regression guard that the
   refactor didn't change main-run behavior).
3. Port `setssp.f` (span bookkeeping: resolve `Nlen`/`Ncol` defaults from
   the series length if unset, compute `Pos1ob`/`Posfob`, bump
   `Issap=2`). Self-contained, no replay needed yet -- portable and
   testable in isolation against oracle intermediate values (`Iyr`, `Im`,
   `Ns1`, etc. in `ssap.cmn`, which the oracle's `.udg`/debug channels may
   not surface -- may need a scratch oracle build with extra WRITE
   statements, or infer indirectly from the span-count in golden headers).
4. Port `ssx11a.f`'s span-pointer relocation (reuses `setxpt`/`dfdate`,
   already ported) + wire the replay loop in `sspdrv.f`, WITHOUT ssrit
   storage yet (i.e., spans compute but their results go nowhere) -- proves
   the replay loop runs `Ncol` times without crashing/diverging.
5. Port `ssrit.f` and wire its 3 call sites in `x11parts.cpp`
   (`x11pt2.f:302`, `x11pt3.f:624`, `x11pt3.f:730` -- currently
   `x11_not_ported` stubs gated on `ctx.hiddn.issap==2`, which now finally
   becomes reachable).
6. Port `ssap.f`'s `xchng`/`mflag` (compute + flag) and `svspan` (table
   write) for just `S` (sfs) and `Sa`->`c` (chs, the month-to-month change)
   -- the two tags the committed golden actually has. Design and implement
   the `tools/x13run_x11.cpp` wide-table emission convention at this point
   (see `tests/parity/test_slidingspans_tables.py`'s docstring -- no
   convention exists yet, unlike force's flat `tag date value` lines).
   Un-xfail `sfs`/`chs` in the gate.
7. Extend to `tds`/`ads`/`ycs` once a corpus spec + golden exists that
   actually populates them (needs `fixreg`/TD regression, or `round=yes`,
   or explicit `save=(ycs)` with enough span-years -- generate + bless
   these via `tests/corpus/extra/genextra.py` + the oracle-run loop
   documented in `tools/testgen_scope.md` &sect;1).
8. Summary/diagnostic tables (F-tests `ssftst.f`, histograms `sshist.f`,
   breakdown `btrit.f`, header `ssphdr.f`) are lower priority -- not gated
   by this task, no save tag requested for them in scope.

## 6. Current gate (`tests/parity/test_slidingspans_tables.py`)

Two corpus specs qualify (`slidingspans{}` present, `.sfs`/`.chs` goldens
present): `airline_slidingspans` (defaults) and
`airline_slidingspans-cutseas` (cutseas=5.0 cutchng=5.0 variant). Both ship
only `sfs`/`chs` goldens (no `ads`/`tds` -- see &sect;3). All
`2 specs x 4 tags = 8` cases **xfail unconditionally** citing the driver
gap; confirmed empirically that `x13run_x11` does not FATAL on these specs
today (it silently runs the ordinary single-span X-11 pipeline and prints
no sliding-spans tables at all, since `Issap` never advances past 1).

The golden reader (`_read_golden`) is written to the *real* wide-table
shape (`date Span1..SpanN Max_%_DIFF`, `-999` sentinel) so it is usable
as-is once the driver lands, but the **produced-side reader is a
`NotImplementedError` stub** -- there is no `x13run_x11.cpp` emission
convention yet for a per-span table (unlike force's flat `tag date value`,
which one row per date can't represent `Span1..SpanN`). Design that
convention as part of phase 6 above.

No new Census Fortran bugs were found or logged this session (parser-only
port; no computational logic was exercised). One pre-existing documentation
inconsistency noted but *not* logged as a CB bug (no behavioral effect):
`sspinp.cmn`'s comment for `Ssotl` says "(0=keep,1=remove,2=auto
identify)", but `getssp.f`'s actual `OTLDIC='removekeepyes'` dictionary
(and the default `Ssotl=1`) means 0=remove,1=keep,2=yes(auto) -- the
*comment* has keep/remove swapped, the code does not. Ported faithfully to
the code's actual behavior, not the comment.
