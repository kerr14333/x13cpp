# X-11 — working notes

The measured records for this subsystem are in **`docs/M5_PORT_NOTES.md`**.
Read the relevant entry BEFORE changing anything here; each one is a bug
already paid for once.

| entries | what they cover |
|---|---|
| **3, 4, 9** | the model X-11 path; the spec-option front (`type`, `shrink`, `sigmavec`, `x11easter`); the yes/no switches (`excludefcst`, `true7term`, `sfshort`) |
| **5, 23** | `force{}` — non-original targets, the forecast-span prior, and the negative-value correction (`qmap2`) |
| **6, 8** | backcasts (`forecast{maxback=}`) and `x11{appendbcst=yes}` |
| **7, 11, 12, 18** | seasonal outliers + `centerseasonal`; holiday regressors (`Finhol`); user prior factors; `transform{constant=}` |
| **79** | `slidingspans{}` + `x11regression{}` — `ssxmdl`'s `fixx11reg=` default (parsed-but-unread), the `Ixreg` demote it has to land WITH, `x11mdl.f:874`'s missing `tds` store, and the sixth span-replay save/restore miss (`b16`/`c16`/`.xrm`) |
| **80** | the STOCK trading-day nonpositive-factor abend (`x11mdl.f:661-690`) — an unguarded `OUTCOME: OK` where the oracle refuses, and the `ELSE` that pairs with `:546`, not `:541` |
| **81** | the sliding-spans NOTEs (`ssxmdl.f:35-39`, `ssphdr.f:145-152`) — an `Itd/Ihol` demote whose only observable is a table that is NOT produced, the Mt2 channel the harness threw away on a successful run, and the two gates that now read it |
| **83** | `slidingspans{}` CLOSED bit-exact — `Setpri` is editor-only and must NOT be re-anchored per span (the leap-February `chs` gap), `arima.f:1430`'s per-span `ssprep` re-snapshot (spans CHAIN), `fixreg=` fixes nothing in the oracle because `rvfixd` never writes `Regfx2`, and `fixmdl=clear`'s `Ssinit==2` block |
| **82** | `slidingspans{fixreg=}` (parsed and dropped, behind a wall keyed on `Nssfxx` instead of `Nssfxr`), `ssmdl.f:50-121` + `ssxmdl.f:78-136`'s Iregfx/Irgxfx arms, the `loadxr` round trip that UNDOES rvfixd's store writes — and the mutation that passed because its spec was on a different arm than its comment claimed |
| **84** | `Xaicst` / `Xaicrg` — the two x11regression aictest values the editor parses back out of its own GROUP TITLES (`tdstock[15]`, `(change for before 1955.Jan)`), both read by `x11reg.cpp` and never written until now |
| **87** | `slidingspans{}` + AUTOMATIC x11regression outliers — `ssx11a.f:99-154`'s whole `loadxr(F)…loadxr(T)` block (absent, and only its `x11outlier=no` arm was walled), `ssxmdl.f:44-77`'s `rmotss` walk into the `otxrev` store, and the TWO clauses `x11mdl.f:424-425` had lost — without which `x11outlier=no` re-identified per span and blew `PB=80` |
| **86** | `slidingspans{fixreg=(outlier)}` — `setssp`'s `Otlfix` is the one fixreg flag that OUTLIVES setup (`sspdrv.f:121`, per span), the `ssx11a.f:268` disjunction it feeds is algebraically redundant with `rmotss`'s own store write, and lifting the wall falsified entry 83's "`fixreg=` fixes nothing in the oracle": with a held-back outlier `regchg` re-snapshots the post-`rvfixd` `Iregfx`/`Regfx` and the fixings survive every span |
| **89** | `getrev.f`/`putrev.f` — the revisions-history CAPTURE, at its five `x11pt3` sites plus `seatdg.f:148-181`. Which BUFFER each site hands it is the whole content: `Stci2` under `force{}`, `Stcirn` under `round=yes` (whose sliding-spans `ssrit` was missing too), and `stc2` — the LS-folded published D12 — for the trend, on `(.not.Finls).and.Adjls.eq.1` and NOT on `have_stc2` |
| **88** | `slidingspans{}` + USER REGRESSORS — the wall was keyed on `Nusxrg`, which is the `usertype=` count and not the user-column count, so it could not fire; the real holes were two BARE `abend`s on `rmfix`/`addfix`'s user arms; `bakusr.f:50/52` read past the end of `Xuserx`/`Usxtyp` (CB-40) and `sspdrv.f`'s `bfx2` is one buffer for two saves (CB-41); `chusrg` is a guaranteed no-op under the `fixmdl` default |
| **96** | `x11regression{b=}` — the spec shape that reaches `rmfix`/`addfix`'s USER arm on a MAIN run, so CB-40 is gated by measurement at last (21). `x11pt2.f:720/723`'s design swap is `IF(Ixreg.eq.1)` on BOTH lines: the transparent pass deliberately CHAINS its Kpart 2 and 3 calls, and this port's unconditional swap handed Kpart 3 the real user matrix where the oracle has the zeroed one |
| **97** | `regression{}` + `x11regression{}` in one run — `xrgdrv`'s `Ncusrx == 0` guard named a symptom: behind it sat `gtinpt.f:832`'s design `restor` (stood in for by a CLEAR, so parsing x11regression{} DELETED the regression{} columns) and `xrgdrv.f:66-74`'s own `ubkx` backup of the user matrix. x11pt2's Adjcyc wall was a refusal in front of a SEATS-only deferred print, and it fired on exactly that pairing because `chkadj` runs at `arima.f:1256`, after xrgdrv |
| **98** | CB-40's wall came down — the port matches the STOCK oracle bit-exact while restoring the CORRECT slot-0 backup, which the instrumented oracle proves it does not have: two offsetting rearrangements (aape captured after the restore, x11 factors never re-derived from `Userx`). CB-41 is STILL unreachable — its blocker was never the walls, it is `chusrg` |
| **99** | `chusrg` FIRES for the first time — three preconditions (`fixmdl=no`; a non-fixed regARIMA user column beside an `x11regression{usertype=}`; and the column whose VALUES it reads is the X11REG one, because `loadxr.f:43`'s copy-back is commented out). Behind it, the span bracket's over-restore of `Userx` — entry 71's `Ksdev` shape, third instance, and the code comment had named this exact spec shape in advance. CB-41(a) proved unreachable BY CONSTRUCTION |
| **100** | the three instrumented-Fortran questions ANSWERED — `x11ref.f:87`'s `Holgrp` guard was this port's own missing writer (`x11aic.f:318-322`, 52 gates), `Trumlt` read directly as `.true.` and filed as CB-42, and `x11mdl.f:597-602`'s stale `icol` is DEAD CODE because `addtd.f:75-77` titles every one-column TD group `1-Coefficient …` and `x11mdl.f:545` searches for `Trading Day` exactly |
| **85** | the sliding-spans HELD-BACK OUTLIERS (`ssmdl.f:124-280`'s group walk, `rmotss`/`adotss`) — a whole block that was neither ported nor walled, `ssprep`'s dropped `Lx11` argument, and the change-of-regime arm the ORACLE halts on (CB-39) |
| **93** | `x11regression{outlierspan=}` — parsed and dropped, its DEFAULT end is the SERIES end and not the span's, the observable is the CRITICAL VALUE and not the search window (`idotlr` clamps), so `editor.f:1749`'s `Critxr` derivation had to move out of `x11reg.cpp`; `cvrerr.f` was missing entirely; and `ssx11a.f:105-106`'s per-span window is kept UNGATED on transcription |
| **13, 14, 15, 24, 71, 72, 73, 76, 77** | `x11regression{}` — `tdprior`, the OLS prior TD (`xrgdrv`) on both the model and the no-model path, the Easter aictest sub-engine, the logadd tdprior bug, the `Picktd` half of gtinpt's `restor`, `prterx`'s singular-design abend, the HOLIDAY-ONLY design whose whole prior pass was skipped behind four `Axrgtd` proxies, and `reweight=` (`Lxrneg`) — whose daily-weight rewrite has to run BEFORE `x11ref`, because it writes back into `B` |
| **19, 20, 21** | x11pt4 — the F2 test battery, the Part-F summary measures + F3 quality statistics, and the Part-E tables |
| **25** | `x11pt2 tdlom Adjtd==0` — and why the original unreachability proof was invalid |
| **39, 40, 41–44, 46** | D8B/D9A, the single-line savelog canaries, and the spectrum / QS / NP / Tukey diagnostic blocks |

## The traps most likely to bite here

- **A span replay overwrites the live COMMONs in place.** x11pt1→x11pt3 is a
  full pass, so `/x11srs/`, `/adxser/`, `/x11fac/`, `/x11ptr/`, `/lkhd/`,
  `Begspn`, `ctx.x11_f2tests`, `ctx.d8bd9a` and `/orisrs/` all need
  save/restoring around the span drivers. The oracle punches its tables during the main pass, before
  `sspdrv`/`revdrv` run; this harness dumps at exit. **Any new `ctx` field
  written from inside x11pt1/x11pt2/x11pt3 joins that set** — this has bitten
  the port four separate times.

- **`/x11srs/ Sti` and `Stc` are NOT the published D13/D12 in the oracle.**
  `x11pt3.f:1089-1103` builds the restored D13 in a LOCAL and punches that;
  `Sti` keeps the outlier-REMOVED irregular, which is what x11pt4 and `genqs`
  read. This port copies the published values back over the COMMON mirror for
  the harness and keeps the oracle's live values in `ctx.x11_sti_int` /
  `ctx.x11_stc_int`. **Anything new that reads `/x11srs/` after x11pt3 must
  take the snapshot, not the mirror** (entry 43 — it flipped a QS statistic
  categorically, because `calcqs` is a step function in the sign of `r(1)`).

- **An outlier regressor and `slidingspans{}` are not independent.** `ssmdl.f`
  holds back every outlier column the span INTERSECTION does not cover and each
  span's `adotss` re-adds the ones its own window does; the store is re-tested
  per span and stripped after each one (`sspdrv.f:208-219`), unlike `history{}`'s
  `chkorv`, which drains it. Anything new that deletes or adds a design column
  between spans has to re-snapshot (`ssmdl.f:358-373`), or the next `restor`
  undoes it (entry 85). **And that re-snapshot carries more than the design:**
  it takes `Iregfx`/`Regfx` with it, so a `fixreg=` fixing that `restor` would
  otherwise erase becomes permanent the moment the walk deletes one column
  (entry 86). Anything measured "inert because `restor` puts it back" was
  measured with `regchg` false.

- **`Muladd` collapses 2→0 at `x11pt1.f:52`** for the whole prior-adjustment
  stage. A guard placed after the collapse tests a different value than one
  placed before it — that is exactly how the logadd `tdprior` bug slipped
  through two guards that each assumed the other would catch it (entry 24).
