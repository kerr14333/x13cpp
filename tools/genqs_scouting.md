# `genqs.f` — the QS seasonality statistics — scouting

**Status: CLOSED for BOTH `genqs.f` and its sibling `gennpsa.f`, on every path:
DIRECT X-11, MODEL-ONLY, SEATS and (2026-07-28f) `Iagr==4` INDIRECT. Byte-exact,
gated by `tests/parity/test_qs_diagnostics.py` for the first three and by
`tests/parity/test_composite_tables.py` for the fourth; every spec that ships a
`qs*` key, zero new goldens.** Originally written 2026-07-27.

327 of the 331 `.udg` goldens carry a `qs*` key and the port emitted none of
them.

## Where it runs, and what gates it

Two call sites, both in `x11ari.f`:

| line | when |
|---|---|
| `x11ari.f:279` | the direct run |
| `x11ari.f:347` | the INDIRECT composite pass (`Iagr==4`) |

The gate is
`IF(Prttab(LSPCQS).or.Savtab(LSPCQS).or.Svltab(LSLQS).or.Svltab(LSLDQS))`.

Both questions the original scouting flagged are now answered:

1. **There is NO `IF(Ny.eq.12)` here** — confirmed. `editor.f:855-863`'s
   monthly-only `Savtab` clear covers `LSPCS0..LSPS0C` (93..102) plus `LSPCTP`
   (115) and `LSPCQC` (116); `LSPCQS` is **113** and is not in either. Quarterly
   specs are in scope, and 41 of the corpus specs are quarterly.
2. **`Savtab(LSPCQS)` is set by the `-s` flag**, via `gtinpt.f:121-123`
   (`Lsumm>0 ⇒ Savtab := sumtab`) and `sumtab.var` entry 113 = `T`. So this is
   the same "every golden was blessed with it" leverage the last six increments
   had, reached by a different route than `Lsumm>0` being read directly.
   The 4 goldens with no `qs*` key are `edge/log-zero-series` and
   `extra/airline_x11regression-tdprior-add` (both runs the oracle refuses) and
   `generated/expgs_{seats,ar2-seats}` (the `noadmiss` specs where the oracle
   writes 0 rows) — i.e. no negative control among the runs that complete.

## Routines ported

| file | what |
|---|---|
| `calcqs.f` | the Pierce QS statistic over `Z(Iconce+1:nz)` — autocorrelations at lags `mq` and `2*mq` only, `QS = nr(nr+2) * sum r(k)^2/(nr-k*mq)`, and **zero unless `r(1) > 0`** |
| `calcqs2.f` | the same statistic plus a `PosCorr` flag: for `mq>4` it needs `r(mq)>0` AND `r(1..4)>0`; for `mq<=4` it needs `r(1..mq) > 0.2` |
| `qsdiff.f` | first-difference, then `ndif-1` more times where `ndif = max(min(2, Nnsedf+Nseadf), 1)` with a model and 1 without; mean-delete (`smeadl`); `calcQS2`; and **if `PosCorr==1 && ndif==1`, difference ONCE MORE and recompute** |
| `genqs.f:1-254` | the driver: builds six series, calls qsDiff/calcqs on each over two spans |

`chisq.f` and `smeadl.f` were already ported (`core/src/numeric/numeric.hpp`);
`chisq(QS, 2)` is the p-value column. All of the above now live in
`core/src/diag/genqs.{hpp,cpp}`; the emits are `dump_qs`/`dump_np` in
`tools/x13run_x11.cpp`.

Note the two `calc` routines differ in more than calcqs2's extra output: it
works on the whole array from element 1, divides by `nz` rather than by a
separate `nr`, walks every lag up to `2*mq`, and keys the statistic on `r(mq)`
where calcqs keys it on `r(1)`.

## The six series, and the two spans

Each statistic is computed twice — over `[Pos1ob, Posfob]` (the `qs*` keys) and
over `[ipos+1, Posfob]` (the `qss*` keys, "starting at Bgspec"), the second only
`IF((ipos+1).gt.Pos1ob)`, where `ipos = dfdate(Bgspec, Begbk2)`.

| key | series | notes |
|---|---|---|
| `qsori` | `Series` | `Llogqs` optionally logs it first |
| `qsorievadj` | `Stcsi` + the extreme-value fold | same construction as spcdrv's sp0, pseudo-additive rebuild included |
| `qsrsd` | the regARIMA residuals | computed in `arima.f:1107`, NOT in genqs |
| `qssadj` | `Stci` (X-11) or `Seatsa` (SEATS) | |
| `qssadjevadj` | `Stcime` with `Facls` divided out, or `Stocsa` | the same `Adjls==1` Facls divide as spcdrv.f:318 |
| `qsirr` / `qsirrevadj` | `Sti` / `Stime` (X-11), `Seatir` / `Stocir/100` (SEATS) | `-1` when `Muladd!=1`; these use `calcqs` DIRECTLY, not qsDiff |

`Iagr==4` swaps in the `qsind*` key names; `Iagr<4` suppresses `qsori`/
`qsorievadj`/`qsrsd` on the indirect pass.

## What actually happened

Three things came out of it, and two are worth carrying forward.

### 1. CB-25 — the residual pair is read from an uninitialised COMMON

`QsRsd`/`QsRsd2` are assigned in exactly two places, **both inside `arima`**:
the reset at `arima.f:129-130` and the computation at `:1105-1118`. `x11ari.f
:106` calls `arima` only `IF(Lmodel)`. So a model-free `x11{}` run never enters
it, reads the COMMON's static zero, and reports `qsrsd: 0.00000 1.00000` as a
statistic about residuals that do not exist.

The contrast that proves it is `extra/airline_identify`: `identify{}` sets
`Lmodel`, so the reset fires but the `:1105` block does not (`Var` is zero), and
that golden correctly carries **no** `qsrsd` row at all. Ported by defaulting
`QsStats::qsrsd`/`qsrsd2` to **0.0** and resetting to DNOTST in
`run_pre_model` under `has_model`. Full write-up in `tools/census_bugs.md`.

### 2. `/x11srs/ Sti` and `Stc` are NOT the published D13/D12 — and this port makes them so

`x11pt3.f:1089-1103` builds the AO/TC-restored D13 in a **local** `sti2` and
punches THAT; `Sti` itself is never touched, so everything downstream of x11pt3
(x11pt4, and now genqs) sees the outlier-REMOVED irregular. This port copies the
published values back over `x11srs.sti`/`.stc` at the tail of x11pt3 so the
harness can punch d13/d12 off the COMMON mirror, and keeps the oracle's live
values in `ctx.x11_sti_int` / `ctx.x11_stc_int`.

That deviation is invisible to every table gate and it is exactly what genqs
tripped over: on `generated/expgs_fixed-airline-x11` the published D13 gives
`qsirr` 0.03946 against the oracle's 0.00000, because folding the AOs back in
flips the lag-4 autocorrelation positive — and `calcqs` is a **step function in
the sign of `r(1)`**, so a small change in the input is a categorical change in
the output. **Anything new that reads `/x11srs/` after x11pt3 must take the
snapshot, not the mirror.** Verified from the goldens themselves: recomputing
calcqs in Python off the blessed `.d13` file reproduces the ENGINE's wrong
0.03946, not the oracle's `.udg` value, which is what localised it.

### 3. `Bgspec` had two resolutions and now has one

`gtspec.f:324-327` (spectrum spec present) and `gtinpt.f:1282-1286` (absent) set
the same default — eight years back from `Endspn`, clamped forward to `Begspn`.
The port had neither and `run_spectrum` computed it locally, which does not
work for genqs: the residual pair keys on `Bgspec` during ESTIMATION. It is now
resolved once at the parse tail, next to `Peakwd`, and `run_spectrum` reads it.

Found alongside it: the two `Peakwd` sites are **not** equivalent —
`gtspec.f:355` is a bare `Peakwd=1` with the `Sp.eq.4 ⇒ 3` override commented
out, so a quarterly spec carrying a `spectrum{}` block gets 1 where one without
gets 3. Inert today (only the monthly peak grid reads it) but now transcribed.
`spectrum{logqs=}` was also parsed-and-dropped; it is now applied.

## Gate

`tests/parity/test_qs_diagnostics.py`, 157 specs at the time, **zero new
goldens**,
byte-exact against the golden text through `fwrite_fmt` with genqs.f's own
formats (`1030 FORMAT(a,':',f16.5,1x,f10.5)` — note **no space after the
colon** — and `1040 FORMAT(a,': ',a)`), key sets asserted in BOTH directions.

The only numeric fallback is `max(5e-4 abs, 5e-5 rel)` on `qsrsd`/`qssrsd` for
the two `airline_automdl-x11` specs (measured 2.30823 vs 2.30858, 1.5e-4
relative), the same bound and the same reason as `test_check_diagnostics`: the
statistic is `nr(nr+2)*r(sp)^2/(nr-sp)` on an AUTO-SELECTED model's residuals,
so a 1e-6 coefficient agreement buys an `r` that agrees to ~1e-5. Restricted to
those two keys so a drift in any X-11 statistic still fails loudly.

Mutation-tested twice: a 1% perturbation of `calcqs` fails 113 of 157, of
`calcqs2` 140 of 157.

## What is still open

* ~~SEATS specs~~ -- **CLOSED**. See the section below.
* ~~Model-only specs~~ -- **CLOSED**, and it did close both fronts at once
  (`test_qs_diagnostics` 157 -> 249, `test_spectrum_peaks` 140 -> 222). See the
  section below.
* ~~The `Iagr==4` indirect names~~ (`qsindsadj`, `qssindirr`, ...) — **CLOSED
  2026-07-28f, and the suspicion recorded here was right.** The indirect call
  site passes `Tblind = LSLIQS` (=69, a *savelog* index from `spcsvl.i`) where
  `genqs.f:439` uses it as a `Savtab` subscript; the direct call one screen
  earlier passes `LSPCQS` (=113) correctly, and the evidently-intended
  `LSPQSI` (=114) is passed nowhere. **The oracle therefore emits no `qsind*`
  key at all** — confirmed against the composite total's golden, which carries a
  full set of `npind*`/`spcind*` beside it. Logged as **CB-31** and reproduced
  by NOT making the call; pinned from both sides by
  `test_composite_no_indirect_qs`. `gennpsa`'s indirect pass IS real (its call
  site passes the right index) and is ported.
  **Worth keeping as method:** checking the suspicion before porting is what
  turned a phantom gap into a two-line CB entry.
(`gennpsa.f` is CLOSED -- see the section below.)

## The SEATS path (CLOSED)

`test_qs_diagnostics` 249 -> 313; SEATS specs now run through `x13run_seats`,
which shares the emit with `x13run_x11` via `tools/dump_diag.hpp` (x11ari.f
reaches genqs at :277 and gennpsa at :322 only after the Lseats/Lx11 branch
rejoins, so both blocks belong to every adjustment path).

### The four buffers, and why only two of them are independent

The oracle fills them through the `ansub9.f` USRENTRY bridge from inside
`seats()`. `sa` arrives **twice** — 1309 -> `Seatsa` (1..Nz) and
1203 -> `Stocsa` (1..Nz+lfor) — and `ir` twice, 1312 -> `Seatir` and
1204 -> `Stocir`. At `sigex.f:3623-3631` they are literally the same two local
arrays. What separates them is `seatad.f`, which post-processes only the `Seat*`
pair:

| | seatad does | so over `[Pos1ob,Posfob]` |
|---|---|---|
| `Seatsa` vs `Stocsa` | forecast append (`Posfob+1..` only) + the `Adjsea==1` Facsea divide (`:53-56`) | identical unless a regARIMA **seasonal regressor** is present |
| `Seatir` vs `Stocir` | `/100` under `Muladd!=1` (`:27`) | `Stocir/100 == Seatir` exactly — which is why genqs divides `Stocir` by ONEHND and does not divide `Seatir` |

So **`qsirr` and `qsirrevadj` are provably identical on the SEATS path in both
modes**: multiplicative because `Stocir/100` *is* `Seatir`, additive because
`calcqs` is scale-invariant (the `r(k)` are unchanged by the `1/100`) and the
`-1` re-centring is skipped. Every corpus golden agrees.

This port has no pre-seatad buffer to copy, so `publish_seats_commons`
(`run_seats.cpp`) reconstructs the `Stoc*` pair by inverting exactly those two
steps from `ctx.seats_sa`/`ctx.seats_ir`. **The Facsea arm is transcribed, not
measured** — no corpus spec pairs a seasonal regressor with `seats{}`.

Taken from `ctx.seats_*` rather than from inside `seats_decompose` deliberately:
those are what the span drivers save/restore, so a slidingspans/history replay
cannot leave the LAST span's components here. That matches the oracle's own
`ansub9.f:110` guard (`IF(Issap.eq.2.or.Irev.eq.4)RETURN`), which skips the
`Stoc*` store on a replay outright.

### The real find: `editor.f:517-518`, and it was NOT print surface

```fortran
      IF(.not.Lx11.and.(Fcntyp.eq.4.or.Fcntyp.eq.0.or.dpeq(Lam,1D0)))
     &   Muladd=1
```

**On a run with no `x11{}` that is not taking a log, the adjustment mode is
forced ADDITIVE**, overriding the multiplicative default `gtinpt.f:956` just
resolved. `gtinpt` cannot do this itself — its rule keys on `Fcntyp` alone and
cannot see `Lx11` — so a no-transform SEATS spec comes out of the parser with
`Muladd 0`. This port had the `gtinpt` half and not the `editor` half.

Measured: genqs re-centres a ratio irregular by subtracting one under
`Muladd != 1`, and a SEATS decomposition without a log produces an **additive**
irregular centred on zero. The missing rule shifted the whole series to about
`-1` and turned a QS of `0.00000` into **1460.26951** on every `unrate_*-seats`
spec (`qssirr` 122.49252 against 0.00000). It reaches `divsub`/`addmul` on this
path too, so it is not confined to the diagnostics.

Ported at the tail of `parse_spec.cpp`, next to `editor.f:429-434`'s constant
shift — the one place in this port that is unambiguously "after gtinpt, before
everything else". `Tmpma` is deliberately NOT updated: `gtinpt.f:970` sets it
from the pre-editor value and `editor.f:518` touches only `Muladd`.

### One more ported asymmetry, fixed in passing

`genqs.f:148-160`'s SA arm: its **Lx11** arm is the one of the five that does
not set `lplog` after logging, but its **ELSE** arm — the one SEATS and
model-only runs take, keyed on `Lam==0` rather than `Muladd` — *does*. The port
was dropping it for both. Inert on this corpus (no spec sets
`spectrum{logqs=yes}` at all) but now written as the Fortran has it.

### What the SEATS goldens can and cannot discriminate

Read this before trusting a green run here. Every SEATS golden reports
`qssadj`/`qsirr` as `0.00000` except `payems_seats`, and `payems_seats` has
`npsi==1` (no seasonal component, so `sa == z` and `qssadj` is trivially
`qsori`). The one genuinely non-degenerate SEATS value in the corpus is its
`qsirr` 0.01133 / `qssirr` 0.00498. The real coverage these 64 specs add is the
`qsori`/`qsorievadj`/`qsrsd` block — nonzero and discriminating (167.64858 on
the airline family) — which the whole-spec skip had been throwing away along
with the rest.

Because of that degeneracy the arms were **mutation-tested explicitly**, and the
first mutation was a bad one worth recording: a 1% **scaling** of the published
SA series fails **zero** specs, because QS differences the series and then takes
autocorrelations — a pure scale factor is invariant by construction. Only a
SHAPE change discriminates. Injecting a 5% spike every 12th observation fails
**62 of 62** SEATS specs through the SA arm and **50 of 62** through the
irregular (the other 12 have no irregular component, i.e. `Hvstir` false).

### Not called here: `spcdrv`

The oracle runs it BETWEEN genqs and gennpsa. Its SEATS branch reads
`Hvstsa`/`Hvstir` over a different construction again (`spcdrv.f:299/436`) and
is the separate open front in `tools/spectrum_peaks_scouting.md`. `gennpsa`
reads none of `spcdrv`'s state, so the order between the two that ARE ported is
preserved.

## `gennpsa.f` -- the NP residual-seasonality verdict (CLOSED)

Same family, same call chain (`x11ari.f:322-326`, straight after spcdrv, again
with no `Ny==12` gate), gated by `Savtab(LSPNPA)` where LSPNPA is **117** and
`sumtab.var` entry 117 is `T`. 230 goldens carry `nplog`.

`npsa.f` is the whole of the arithmetic: optionally log, difference `ndif` times
(`max(min(2,d+bd),1)` with a model, 1 without -- and note there is NO
"difference once, then once more if PosCorr" retry, unlike qsdiff), mean-delete,
then threshold `kendalls` at a FIXED critical value: 24.73 monthly, 11.35
quarterly, and any other period is always 0. So the output is a yes/no verdict
rather than a statistic. `kendalls` was already ported for `check{}`'s Friedman
test and is now shared out of `checkres.cpp`'s anonymous namespace.

Only the SA series (`Stci`) and its extreme-value twin (`Stcime`, with the same
unconditional `Adjls` Facls divide) are tested -- hence **101 goldens carry no
`np*` key at all**, against 4 with no `qs*` key: the block is absent from every
model-only run and every `x11{type=}` run where `Kfulsm != 0`.

Two ported asymmetries against genqs, both left as written:

* `gennpsa.f:52-58` DERIVES `lplog` from Muladd/Lam up front, where `genqs`
  LATCHES it as a side effect of a log actually being taken. The two `*log`
  keys can therefore disagree in principle.
* `:77` passes the derived `lplog` and `:104` passes the RAW `Llogqs`, so on a
  non-log X-11 run with `logqs=yes` the two series are tested on different
  scales.

**CB-26**: `gennpsa.f:112`'s `lnps` tests two INTEGERs initialised to `NOTSET`
(-32767) against **`DNOTST`** (the DOUBLE -999.0), so it is unconditionally
true. Harmless in the savelog -- every row re-tests NOTSET individually -- but
the print branch emits a `(Series start in ...)` header on a run with no span
statistics at all. Transcribed with the widening cast made explicit.

Mutation-tested: inverting the npsa verdict fails 153 of 157; moving the
thresholds to 20.0/9.0 fails only 6 (the specs near the boundary), which is why
the inversion is the mutation that proves the keys are compared.


## The MODEL-ONLY path (CLOSED)

`x12run.f:181` reaches `x11ari` with neither `Lx11` nor `Lseats`. That path runs
the whole pre-stage -- x11pt1, and x11pt2 too, since `x11ari.f:199` is gated
`(.not.Lcmpaq).or.Lx11` -- skips `x11ari.f:253`'s `ELSE IF(Lx11)` block
(x11pt3/x11pt4), and drops through to the diagnostics, which is the entire
reason it exists. `run_x11` refused a spec without `x11{}` outright, so 95
goldens' `qs*` and 85 goldens' `spcori`/`spcrsd` had nowhere to come from.

What landed: `x11_prestage` takes `lseats` and `lx11` as INDEPENDENT arguments
(they had been complementary, `lx11 = !lseats`); `run_x11` accepts `!has_x11`
and wraps x11pt3/x11pt4 in the oracle's own `ELSE IF(Lx11)`; and the harness
suppresses the D/E table dumps when there is no `x11{}` (`b1` excepted -- x11pt1
builds it either way, and the oracle prints it on a model-only run too).

### Four defects it surfaced

1. **The sp0 detrend keys on a different test without Lx11** (spcdrv.f:193-200).
   With an X-11 spec the log is taken when `Muladd != 1`; without one there is
   no mode to read and the oracle keys on the TRANSFORM, `dpeq(Lam,ZERO)`. The
   two agree for a log X-11 run and disagree for every non-log model-only spec,
   where Muladd still carries its multiplicative default -- the engine was
   logging a sqrt-transformed series. Measured `spcori.median` +24.29 (oracle)
   vs -27.79 (engine) on `generated/airline_trans-sqrt`: a sign flip, not a
   drift.
2. **`arima.f:321`'s `IF(Ldestm)` gates the ENTIRE estimation half of arima**,
   and this port estimates whenever a model spec is present. On an identify-only
   spec the oracle sets `Lmodel` but not `Ldestm`, does no estimation, and
   leaves `Var` at zero -- so it writes neither `qsrsd` nor the `spcrsd` block,
   where the engine emitted a full residual spectrum and a QS of 255.92 off
   residuals that are essentially the series. The residual capture in
   `run_pre_model` is now gated on `ldestm`.
3. **Five of the ten `Ldestm` setters were unported.** `gtinpt.cpp` set it for
   automdl/estimate/seats only; outlier (`:704`), check (`:712`), forecast
   (`:721`), history (`:753`) and pickmdl (`:880`) were missing, as were both
   parse-tail rules: `:1151` (an adjustment run WITH a model estimates one
   whether or not any spec asked for it) and `:1252` (an X-11 run carrying a
   regression effect to remove or keep must estimate too).
   `generated/airline_user-reg-x11` needs `:1252` specifically --
   `forecast{maxlead=0}` suppresses both earlier rules, since `:721` sees
   `Nfcst==0` and `:1151` sees Nfcst already set.
4. **`gtinpt.f:398-406`'s `Adj* = 1` defaults were unported**, which is what
   rule `:1252` reads. Harmless until now only because `chkadj`'s toggles are
   idempotent from either starting value (`==1 && n==0 -> 0`, `==0 && n>0 -> 1`);
   anything that reads the indicators BEFORE chkadj sees the difference.

### CLOSED: the explicit-aictest leap-year prior

`regression{aictest=(td)}` on the EXPLICIT-model path (arima.f:569, not automdl)
loses the leap-year prior from `Stcsi`. Measured on
`generated/airline_aictest-td`: b1 1949.Feb is **118.0000** -- the RAW value --
against the oracle's ~119.05, with every other month agreeing. That is the
Februaries-only signature of a missing lpyear prior. The negative control is
`generated/airline_reg-td1coef`, which fits the SAME model chosen explicitly and
gives **119.0536**, so the model and its coefficients are right and only the
prior is lost.

It surfaced as `qsorievadj` 192.06996 vs 189.27156 and `spcori.median`
-27.52489561 vs -27.62459168 on `airline_aictest-td` and `cover_reg-aicdiff`.

**Why nothing caught it before:** no corpus spec combines an EXPLICIT aictest
with `x11{}` -- only `automdl` + aictest is covered -- so this path's B1 had
never been compared against anything, and model-only specs produced no
comparable output at all until the model-only increment.

**Root cause, and the hypothesis that was wrong.** The suspicion recorded here
was that `ssprep`/`restor` reverts `Priadj` after the winning candidate is
reinstated -- plausible, because that pair does save `Pri2` and has caused
exactly this class of bug before (the slidingspans Priadj restore). It is not
what happened: the Fortran keeps Priadj across ssprep/restor as intended
(`tdaic.f:237/252/600`, `arima.f:599`, `ssprep.f:58`, `restor.f:55`).

The defect was one branch further out, in `run_pre_model.cpp`. `tdaic` modifies
`trnsrs` IN PLACE -- it divides `Y` by `lomeff`, copies `lomeff` into `Adj` and
sets `Kfmt=1` -- and the driver's `automd` branch re-captures that buffer with
`if (out_trnsrs) *out_trnsrs = trnsrs;` while the `explicit_aictest` branch
beside it never did. `x11_prestage` therefore consumed the stale, un-prior-
adjusted transformed series. The fix is those three lines, added to the second
branch.

**Worth generalizing:** an in-place buffer mutation combined with a per-branch
handoff is a hazard, and the branches have to be DIFFED AGAINST EACH OTHER
rather than each read on its own -- a missing line is invisible when you only
read the branch that has the bug.
