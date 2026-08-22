# Engine findings from the Rx13cpp front end

Defects in this engine found while building the R package at
`D:\code_projects\Rx13cpp` (a separate repo). Kept here, not there, because
this is where they get fixed.

**Nothing in this file is a fix.** Each entry is a measurement plus, where the
localisation is solid, a pointer to the first place to look. Where it is not
solid, it says so.

Every measurement below was taken with **no R in the loop**: the C++ side is
`build/x13run_x11.exe`, the reference is `oracle/fortran/x13as_ascii_O2.exe`,
and both are invoked the way `tests/parity` invokes the oracle — bare spec
name, `cwd` = the spec's own directory, no flags.

The harness that swept for these is `dev/validate-vs-oracle.R` in the Rx13cpp
repo: 4 series x 16 spec combinations, comparing d10/d11/d12/d13.
**191 of 252 table comparisons agree to ~5e-15.** The exceptions are below.

---

## RX-1. `outlier{}` is silently skipped when an AIC regressor test is present

**Status:** open. Confirmed, localised, mechanism identified but not verified
by a fix.

**Severity:** high, and it is on the DEFAULT path. `outlier{}` plus
`regression{aictest=}` is what the Census default workflow composes, so a user
who writes neither gets this.

### Reproducer

Take `tests/corpus/generated/airline_automdl-aictest-x11.spc` — a spec this
suite already validates — and add one bare `outlier{ }` block. Change nothing
else.

| spec | max abs diff on d11, C++ vs Fortran |
| --- | --- |
| the corpus spec, verbatim | **5.116e-13** |
| the same spec + `outlier{ }` | **3.358** |

Both oracle builds agree with each other to **0** (`x13as_ascii_O0.exe` vs
`x13as_ascii_O2.exe`), so the reference is unambiguous and this is not an
optimisation artifact.

It also reproduces with NO `automdl` at all — an explicit
`arima{ model = (0 1 1)(0 1 1) }` plus `regression{ aictest = (td) }` plus
`outlier{ }` diverges by 3.6 on airline d11. Neither half alone diverges:
`outlier{}` alone, `aictest` alone, `automdl` alone, and a FIXED
`regression{ variables = (td) }` with `outlier{}` are all ~5e-13.

### What is actually wrong

The divergence is upstream of X-11 — `b1`, the series X-11 receives, is
already wrong:

| table | max abs diff | at |
| --- | --- | --- |
| b1 | **16.379** | 1951.05 |
| d11 | 3.358 | 1951.05 |
| d12 | 1.553 | 1951.05 |
| d13 | 0.030 | 1951.05 |
| d10 | 0.019 | 1950.05 |

At 1951.05 the raw value is 172; the C++ `b1` is **173.370** and the Fortran
`b1` is **156.991**. 173.370 is 172 with the trading-day effect removed and
nothing else; 156.991 is that divided by the AO the oracle identified
(`AutoOutlier$AO1951.May`, coefficient +0.100155824, factor 1.10534).

**The port identifies no outlier at all on this path.**

### Where to look first

`arima.f`'s control structure inside the explicit-model arm:

```
528  ELSE                                    <- explicit model
569    IF(Leastr .or. Itdtst.gt.0 .or. ...) THEN    <- the AIC regressor tests
701    ELSE
705      CALL rgarma(...)
711      CALL prterr(...)
718    END IF
719    Hvmdl=T
723    IF(.not.lester .and. lidotl) THEN     <- outlier identification
756      CALL idotlr(...)
```

`idotlr` at :723 runs after EITHER arm of the :569 / :701 if-else.

`core/src/driver/run_pre_model.cpp` collapsed the whole model-selection
decision into one if/else-if chain and put the `idotlr` call inside the FINAL
`else` only:

```
~618  } else if (ctx.arima.lautox) {            // automx -- has its own idotlr
~646  } else if (ctx.arima.itdtst > 0 || ...) { // explicit_aictest -- NO idotlr
~660  } else {
~661      rgarma(...); prterr(...);
~683      if (ctx.captured.has_outlier && ...) { ... idotlr(...); }   // <- here
~739  }
```

So a spec that takes the `explicit_aictest` arm never reaches outlier
identification.

**Do not just hoist it out of the `else` without checking the other arms.**
The two automatic arms do their own outlier identification (`automd.cpp:378`,
`automx.cpp:320`), and in the Fortran they are a SIBLING branch — `IF(lauto)`
at :325 closes at :527, and :723 lives inside the `ELSE` at :528, so the
automatic paths never reach :723 either. The Fortran also clears the aictest
flags on the automatic path (`Itdtst=0`, `Leastr=F`, `Luser=F` at :469-471),
and guards :723 with `.not.lester` — an estimation-error flag the port's
version of the block does not carry.

### Why the corpus never caught it

17 corpus specs carry an `outlier{}` block and **none of them also carries
`aictest` or `automdl`.** The one that looks like it does,
`ukgas_automdl-noautooutlier.spc`, has no `outlier{}` block at all — it
exercises `automdl{noautooutlier=tramo}`.

`tools/TEST_COVERAGE.md:130` already lists "automdl x outlier x aictest (the
03-automdl class)" under *"Interaction matrix worth building (once the pieces
land)"*. The hole was known; what was missing was the measurement.

Note this is the failure class `tools/walls.py` exists to prevent: `OUTCOME:
OK` with wrong numbers, and no wall.

### Two more for the checklist (from the x13new session, 2026-08-21)

- **`.not.lester` is a second defect, not a detail of the fix.** The port's
  outlier block genuinely carries no estimation-error flag at that point.
  Worth its own line rather than a footnote to RX-1.

- **Check `arima.f:469-471` against the port.** The Fortran clears `Itdtst`,
  `Leastr` and `Luser` on the automatic path. If the port does not, then
  `lautox` and `itdtst > 0` are NOT mutually exclusive and the if/else-if
  chain's ARM ORDER is silently deciding the outcome. That would also be a
  candidate mechanism for RX-2 below — so check this before treating RX-1 and
  RX-2 as separate causes. The "probably separate" call in RX-2 may be wrong.

**Stale lead, do not chase it.** `tools/x11_regeff_handoff.md:146-150` says
`pass2` is the only missing piece of `automd.f` and runs solely under
`Lidotl`, which would have been an elegant explanation. pass2 has since been
ported (`automd.cpp:32`), and in any case RX-1 reproduces with no `automdl`.

---

## RX-2. `automdl{}` + `outlier{}` also diverges

**Status:** open, measured, NOT localised. It takes the `automd` arm, which
has its own outlier identification, so it was initially filed as a separate
cause — but see the `arima.f:469-471` item under RX-1: if the port does not
clear the aictest flags on the automatic path, arm order in the port's
if/else-if chain could produce both. Check that before assuming two causes.

`transform{function=log}` + `automdl{}` + `outlier{}` + `x11{}`, no aictest:

| series | max relative diff on the worst table |
| --- | --- |
| unrate | **1.01** |
| expgs | 0.259 |
| payems | 0.060 |
| airline | below 1e-8 on this combination |

unrate at ~100% relative means the adjusted series is qualitatively different,
not slightly off. Worth taking before RX-1 if you want the worst case first.

---

## RX-3. `transform{function=auto}` sits just above tolerance

**Status:** open, low priority, may be nothing.

An explicit model with `transform{function=auto}` and no outlier or aictest:
**2.5e-8** relative on expgs, **4.9e-8** on airline. Every other single-option
case in the sweep is ~5e-15, so this stands out by seven orders of magnitude
while still being far too small to matter to a user. Plausibly the AICC
comparison landing near a tie and the two sides taking different branches, in
which case it is a knife-edge rather than a defect — but it has not been
looked at.

---

## Reproducing the sweep

From the Rx13cpp repo, with this repo checked out next to it:

```sh
Rscript dev/validate-vs-oracle.R      # writes dev/out/validation-vs-oracle.csv
```

`--oracle <path>` overrides the executable. It sets
`options(Rx13cpp.quiet = TRUE)`, since the package's own warning about RX-1 is
the thing being measured.
