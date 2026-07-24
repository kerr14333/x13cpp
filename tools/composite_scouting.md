# composite{} / indirect adjustment — port scouting

Target: the last open CES parity blocker (14 of the 143 BLS AE specs use
`COMPOSITE{}`). Unlike every feature ported so far, composite adjustment is a
**multi-spec, driver-level** feature: it does not fit inside one spec run.

## How the oracle does it

`x13as -m <metafile>` (`aaamain.f:114-166`). The metafile lists spec basenames in
processing order; the LAST one must carry the `composite{}` spec. All specs run in
one process, so the aggregation COMMONs (`/mq11/` = `agr.cmn`, `/agreg/` =
`agrsrs.cmn`) persist across them — that persistence IS the feature.

Flow per metafile:

1. **Component specs** (`series{ comptype=add|sub|mult|div, compwt= }`).
   `gtinpt.f` sets `Iagr=1` on the first component (`lagr` from `comptype`);
   `editor.f:2245-2255` promotes `Iagr 1->2` and stamps `Itest(1..5)` =
   (Sp, BegMo, EndMo, BegYr, EndYr) — the span every later component must match
   (`editor.f:2258-2268` errors with `Iagr=-1` on a mismatch). Each component is
   seasonally adjusted normally, then `x11ari.f:373` calls **`agr2`**, which
   accumulates the component into the `/agreg/` buffers via **`agr`** (`agr.f`:
   `B(j) = B(j) +|-|*|/ (A(i)*Wt)`, `Iag` 0..3, `Wt=0 -> 1`):

   | buffer | source (agr2.f:267-281) | meaning |
   |--------|-------------------------|---------|
   | `O`    | `Orig2`                 | direct original (the composite total) |
   | `O1`   | `Stoap`                 | prior-adjusted original |
   | `O2`   | `Orig2` less Fin*/prior | indirect "SA-comparable" original |
   | `O3`   | `Orig2` less LS         | LS-removed original |
   | `O4`   | `Orig2` less AO/TC      | AO/TC-removed original |
   | `O5`   | `O2` / `Faccal`         | calendar-adjusted original |
   | `Omod` | `Stome` (X-11 only)     | modified original |
   | `Ci`   | `Stci` (or `Seatsa`)    | **indirect SA** = sum of component SAs |
   | `Ci2`  | `Stci2` (or `Setsa2`)   | forced SA |

   `agr2` also bumps `Ncomp` and records `Cmptyp/Cmpwt/Cmpfil` for the
   "how the aggregate was formed" table.

2. **The composite spec** (`total.spc`: `composite{}` INSTEAD of `series{}`).
   `getcmp.f` parses it (13 args, ARGDIC below), derives the span from
   `Itest(1..5)`, then `getcmp.f:241` calls **`agr1`** with `Iagr>0`, which sets
   `Iagr=3` and copies `Y(i) = O(i+Ind1ob-1)` — i.e. **the composite total becomes
   the series**. From there the run is an ordinary single-series adjustment
   (DIRECT adjustment of the aggregate).

3. **Indirect output.** Still inside that last run, `x11ari.f:329-373` sees
   `Iagr==3` and calls **`agr3`** (X-11) / **`agr3s`** (SEATS) to emit the INDIRECT
   tables from `Ci`/`O2`/`Omod`, then `x11pt4` again for the indirect E-tables,
   then `agr2` with `Iagr==4` for the direct-vs-indirect comparison statistics
   (`aggmea`, `cmpchi.f`'s F-tests).

`agr1` is also the initializer: called with `Iagr==0` it zeroes `O..Ci`, `Ncomp`,
the sliding-spans (`Saind/Sfind/Sfinda`) and history (`Cncisa/Finisa`) indirect
buffers.

## Fortran inventory

| file | lines | role | increment |
|------|-------|------|-----------|
| `agr.f`     | 25  | the accumulate primitive (+ - * /) | 1 |
| `agr1.f`    | 82  | init / hand `O` to the composite spec | 1 |
| `getcmp.f`  | 249 | `composite{}` parse + span from `Itest` | 1 |
| `agr2.f`    | 318 | per-component accumulation; `Iagr==4` comparison stats | 1 (O only) / 2 (rest) / 3 (stats) |
| `agr3.f`    | 618 | indirect X-11 tables | 2 |
| `agr3s.f`   | 481 | indirect SEATS tables | 2 |
| `agrxpt.f`  | 60  | indirect pointer setup | 2 |
| `aggmea.f`  | 47  | R1/R2 measures of roughness | 3 |
| `prtagr.f`  | 38  | "how the aggregate was formed" table | 3 (print) |
| `pragr2.f`  | 46  | indirect E-table print wrapper | 3 (print) |

(`cmpchi.f` and `cmpstr.f` were listed here in the first draft on the strength of
their names. Neither is composite: `cmpchi` is the regression-GROUP chi-square
test called from `prtmdl.f:709`/`prtxrg.f:552`, and `cmpstr` is the lexer's
token-aware string compare. Nothing in the composite path calls either.)

`composite{}` ARGDIC (`getcmp.f:60-64`, PARG=13): `name title print save decimals
modelspan saveprecision savelog yr2000 indoutlier appendfcst appendbcst type`.
Only `decimals/modelspan/saveprecision/yr2000/indoutlier/appendfcst/appendbcst/
type` carry state; `appendfcst/appendbcst` are the same `Savfct/Savbct` globals
just closed for `x11{}` (commit 13d03c2).

## What already exists in the port

- `agr_cmn.hpp` / `agrsrs_cmn.hpp` are generated and live on `X13Context`.
- `series{ comptype/compwt }` already parse into `ctx.agr.iag` / `ctx.agr.w`, and
  set `lagr` (`series.cpp:209-222`).
- `gtinpt.cpp:278-293` routes `composite{}` and reproduces the oracle's
  "No component series were specified for composite adjustment" error for a
  standalone composite spec. (The oracle then **SIGFPEs** — see
  `tests/golden/census-examples/composite/total/total.stderr.txt`. Candidate
  `CB-N` entry; the C++ exits cleanly, which is a deliberate divergence on an
  already-fatal path.)
- `gt_composite` is `gt_generic` — args consumed, nothing captured.
- Everything downstream (`x11ptr`, `orisrs.orig2`, `x11srs.stci`, `adxser.stci2`)
  is already ported and populated, so `agr2`'s inputs all exist.

## Increment plan (each ends bit-exact + gated)

**inc1 — direct composite.** Metafile driver + `agr.f` + `agr1.f` + `getcmp.f` +
the `O` accumulation of `agr2`. Deliverable: `total` adjusted as an ordinary
series whose data is the component sum. Gates b1/d10-d13 of `total` against a
metafile-blessed golden. This alone unblocks the CES composite specs that only
want the direct aggregate.

**inc2 — indirect adjustment.** The remaining `agr2` buffers + `agr3`/`agrxpt`
(and `agr3s` if a SEATS composite spec shows up). Gates the indirect
seasonal/SA/trend tables.

**inc3 — comparison diagnostics.** `Iagr==4` stats (`aggmea`), the direct-trend
`Tem` / `Ckhs` pair agr3 needs to feed them, and the pointer restore back onto
the direct geometry. Gate via the `.udg`/savelog canaries.

### inc3 status: LANDED (bit-exact)

`aggmea.f` + `agr2.f:66-192` are ported (`core/src/composite/agr2.cpp`
`agr2_compare`), together with the two pieces agr3 had deferred: `/kcser/ Ckhs`
(now `ctx.kcser_ckhs`, written at x11pt3.f:379 — a COMMON precisely because agr3
reads it after x11pt3 returns) and the `Tem` direct trend, a 13-term (5
quarterly) Henderson forced onto Ckhs. All 24 `di()` values reproduce the
oracle's MEASURES OF ROUGHNESS table and all four savelog canaries
(`r1mse`/`r1rmse`/`r2mse`/`r2rmse`) plus `indtrendma` match `total.udg` — gated
in `tests/parity/test_composite_tables.py` at the oracle's own printed precision
(3 decimals), which is all either output carries.

One label oddity, harmless: agr3.f:113-118 writes `indtrendma: Nterm` right after
filtering the DIRECT trend. `Ktcopt` pins Nterm to 13/5 there, so the number is
right either way and it is not worth a `CB-N`.

**What is left is NOT composite-specific.** The remaining items under the old
inc3 heading are all shared with the direct path or are pure output:

- the indirect E/F tables (`x11pt4` on the indirect side) and the `if2.*`/`im*`/
  `iq*` .udg families — `x11pt4` is unported for BOTH adjustments, so this is the
  general E/F-table gap, not a composite one;
- the indirect D8/D9 SI diagnostics (agr3.f:290-350) — same story: `ftest`/
  `kwtest`/`mstest`/`combft` are deferred no-ops on the direct side too
  (`x11parts.hpp:47`);
- `prtagr`/`pragr2` and the aggregate-composition header — print surface, which
  this port defers to the caller by design.

Still genuinely open for composite: the SEATS branch (`agr3s.f`, `Seatsa`/
`Setsa2`) and pseudo-additive (`Psuadd`).

## Harness / gate notes

- The parity corpus is single-spec everywhere; a metafile case needs a new
  harness (`x13run_composite`) that carries one aggregation state across N spec
  runs, plus a bless path that runs the oracle as
  `x13as_ascii_O2.exe -m <mta> -s` from the corpus directory.
- `tests/corpus/census-examples/composite/` already ships the example
  (`region_north`, `region_south`, `total`, `composite.mta`; 180 synthetic
  monthly obs, 1990.01-2004.12). Its existing golden was blessed by running each
  spec SEPARATELY, so `total`'s bundle only records the standalone error + the
  oracle's SIGFPE — it must be re-blessed in metafile mode.
- Verified 2026-07-24: `x13as_ascii_O2.exe -m composite -s` runs the example
  clean (exit 0) and `total.out` carries BOTH the direct and the indirect
  adjustment sections.
