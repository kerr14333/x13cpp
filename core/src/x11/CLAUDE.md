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
| **86** | `slidingspans{fixreg=(outlier)}` — `setssp`'s `Otlfix` is the one fixreg flag that OUTLIVES setup (`sspdrv.f:121`, per span), the `ssx11a.f:268` disjunction it feeds is algebraically redundant with `rmotss`'s own store write, and lifting the wall falsified entry 83's "`fixreg=` fixes nothing in the oracle": with a held-back outlier `regchg` re-snapshots the post-`rvfixd` `Iregfx`/`Regfx` and the fixings survive every span |
| **85** | the sliding-spans HELD-BACK OUTLIERS (`ssmdl.f:124-280`'s group walk, `rmotss`/`adotss`) — a whole block that was neither ported nor walled, `ssprep`'s dropped `Lx11` argument, and the change-of-regime arm the ORACLE halts on (CB-39) |
| **13, 14, 15, 24, 71, 72, 73, 76, 77** | `x11regression{}` — `tdprior`, the OLS prior TD (`xrgdrv`) on both the model and the no-model path, the Easter aictest sub-engine, the logadd tdprior bug, the `Picktd` half of gtinpt's `restor`, `prterx`'s singular-design abend, the HOLIDAY-ONLY design whose whole prior pass was skipped behind four `Axrgtd` proxies, and `reweight=` (`Lxrneg`) — whose daily-weight rewrite has to run BEFORE `x11ref`, because it writes back into `B` |
| **19, 20, 21** | x11pt4 — the F2 test battery, the Part-F summary measures + F3 quality statistics, and the Part-E tables |
| **25** | `x11pt2 tdlom Adjtd==0` — and why the original unreachability proof was invalid |
| **39, 40, 41–44, 46** | D8B/D9A, the single-line savelog canaries, and the spectrum / QS / NP / Tukey diagnostic blocks |

## The traps most likely to bite here

- **A span replay overwrites the live COMMONs in place.** x11pt1→x11pt3 is a
  full pass, so `/x11srs/`, `/adxser/`, `/x11fac/`, `/x11ptr/`, `/lkhd/`,
  `Begspn`, `ctx.x11_f2tests` and `ctx.d8bd9a` all need save/restoring around
  the span drivers. The oracle punches its tables during the main pass, before
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
