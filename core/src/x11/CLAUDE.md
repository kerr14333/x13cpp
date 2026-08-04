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
| **13, 14, 15, 24, 71, 72, 73, 76, 77** | `x11regression{}` — `tdprior`, the OLS prior TD (`xrgdrv`) on both the model and the no-model path, the Easter aictest sub-engine, the logadd tdprior bug, the `Picktd` half of gtinpt's `restor`, `prterx`'s singular-design abend, the HOLIDAY-ONLY design whose whole prior pass was skipped behind four `Axrgtd` proxies, and `reweight=` (`Lxrneg`) — whose daily-weight rewrite has to run BEFORE `x11ref`, because it writes back into `B` |
| **19, 20, 21** | x11pt4 — the F2 test battery, the Part-F summary measures + F3 quality statistics, and the Part-E tables |
| **25** | `x11pt2 tdlom Adjtd==0` — and why the original unreachability proof was invalid |
| **39, 40, 41–44, 46** | D8B/D9A, the single-line savelog canaries, and the spectrum / QS / NP / Tukey diagnostic blocks |

## The three traps most likely to bite here

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

- **`Muladd` collapses 2→0 at `x11pt1.f:52`** for the whole prior-adjustment
  stage. A guard placed after the collapse tests a different value than one
  placed before it — that is exactly how the logadd `tdprior` bug slipped
  through two guards that each assumed the other would catch it (entry 24).
