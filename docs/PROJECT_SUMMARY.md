# X-13ARIMA-SEATS C++ Port — Project Summary

*Proof of concept: directing Claude Code to port a large legacy codebase, prove it correct, and document it.*

Last updated: **2026-07-29**. Every figure here is pulled from the live repository (parity suite, `git` / `worklog.py`, `tools/ported.yaml`, source counts) and re-derived on each update rather than carried forward — a 2026-07-29 audit found this document quoting a suite result five times out of date, so the numbers are now dated at the point of use.

---

## 1. What this project is actually demonstrating

The deliverable is **not** primarily a seasonal-adjustment library. It is a **proof of concept** that Claude Code can take on a full software-engineering effort end to end:

1. **Language translation** — port a large, numerically intensive legacy program from one language (Fortran) to another (modern C++).
2. **Testing** — stand up the test infrastructure that *proves* the translation is correct, to a rigorous, objective bar.
3. **Documentation** — produce the engineering documentation (design notes, scope traces, bug catalogues, this summary) as the work proceeds.
4. **User (developer) experience** — capture what it is like to *direct* this work: the prompts, the decisions, the hand-offs, the friction and the wins. *(This section is intentionally left for the user to fill — see §8.)*

The porting subject was chosen precisely because it is an unusually demanding target: if the approach works here, it generalizes downward to easier codebases.

---

## 2. The subject, and why it is a hard test case

**X-13ARIMA-SEATS** is the U.S. Census Bureau's official seasonal-adjustment program — the engine behind a large share of the world's published economic statistics. The reference implementation is **~166,000 lines of Fortran across 712 files**.

Why it is a stress test for automated porting:

- **Numerically intensive and unforgiving.** It layers regARIMA modeling, automatic ARIMA-order identification, the classical **X-11** decomposition, and **SEATS** canonical ARIMA-model-based decomposition. Small numerical drift compounds through long filter chains.
- **Legacy idioms everywhere.** Column-major arrays, `COMMON` blocks, computed `GO TO`s, Fortran integer-power semantics, and DO-loop edge cases that do not map one-to-one onto C++.
- **The original has bugs that must be preserved.** Because downstream statistics were computed *with* those bugs, "correct" is defined as **matching the original's output**, not matching the underlying mathematics. Faithful bug-for-bug reproduction is a requirement, not a defect.

### The correctness bar: bit-exact oracle parity

The vendored Fortran is built as a reference **oracle**. Every ported unit is run against the oracle on real series and **diffed numerically**. The target is not "close" — it is **bit-exact** within a tiered tolerance policy:

| Class | Tolerance | Rationale |
|---|---|---|
| Pure arithmetic | 1e-12 | Reordered operations only |
| Estimation / iterative solvers | 1e-6 | Convergence-path sensitivity |
| Universal gate | 1e-8 | Default parity assertion |

Measured result on the X-11 spine: **17 of 19 specs reproduce the oracle to ~5e-15** (double-precision noise) — i.e. effectively bit-identical.

---

## 3. Method

- **Oracle-driven parity.** A Python harness (`oracle/run_oracle.py`) blesses save-tables from the Fortran; `tests/parity/` re-runs the C++ and diffs every table.
- **The frontier is a clean FATAL, not an xfail.** Early on, not-yet-ported paths were marked as strict `xfail`. That was replaced once the port got far enough that the real risk was different: the dangerous failure is not a test that fails, it is an option the engine **accepts and silently ignores**, returning `OUTCOME: OK` with wrong numbers. Unported branches now abend with a message naming the Fortran line, and options are measured against the oracle before being declared inert. The suite has carried **0 xfails** since 2026-07-27.
- **Census-bug fidelity.** Every genuine bug found in the Fortran is catalogued (`tools/census_bugs.md`, currently **CB-1 … CB-31**) and reproduced deliberately in the C++, with a comment pointing back to the Fortran line — or, where the defect is unreachable or is a save-file artefact this port does not produce, recorded with the reason it is *not* reproduced.
- **Multi-agent execution.** Independent background agents run parallel fronts (e.g. the SEATS decomposition) and perform independent **code review** of completed areas, with the main thread coordinating and triaging.
- **Layered architecture.** A bit-exact engine core underneath a modern, readable C++ API surface (`core/include/x13/api/`) that R and Python wrappers will bind to. Engine-internal modernization is deliberately deferred until the port is complete, to preserve the bit-exact anchor.

---

## 4. What has been built (area status)

| Area | Status | Gated by |
|---|---|---|
| Spec-file parser (all 20 spec blocks parse) | **Done** | `test_m1_parse.py` |
| Transform / pre-model tables | **Done, bit-exact** | `test_m2_tables.py` |
| regARIMA estimation + forecasting | **Done, bit-exact** | `test_m3_estimate.py`, `test_m3_forecast.py` |
| Automatic model ID + AIC tests | **Done for corpus** | `test_m4_aictest.py`, `test_m4_iddiff.py`, `test_m4_trnaic.py` |
| X-11 decomposition (B/C/D/E tables) | **Done, bit-exact** — the full spine plus Part E and the x11pt4 F2/F3 summary + quality statistics | `test_x11_tables.py`, `test_x11_etables.py`, `test_x11_diagnostics.py` |
| `force{}` (Denton / Cholette-Dagum / rounding) | **Done, bit-exact** — all four `target=` values, plus the negative-value correction | `test_force_tables.py` |
| SEATS decomposition | **Done, bit-exact** — every SEATS corpus spec's s10–s18 gate (~5e-15), plus general-shape p>0 (`ar2-seats`), bp>0 (`sar-seats`), and `imean!=0` (mean regressor) for both d>=1 (`mean-seats`) and d==0 (`mean-d0-seats`), plus a mean **alongside TD** regressors (`mean-td-seats`: s16/s18 refold the removed TD effect + lom/leap prior) | `test_seats_tables.py` |
| `slidingspans{}` | **Done, bit-exact** — all 4 spans, sfs+chs | `test_slidingspans_tables.py` |
| `history{}` | **Done, bit-exact** — sar/sae/trr/tre (~7e-6 re-estimation floor), the forecast + model histories, and the alternate revision targets | `test_history_tables.py` |
| Automatic model selection | **Done, bit-exact** — both engines: `automdl{}` (TRAMO, `automd.f`) with all 11 arguments applying, and `pickmdl{}` (X-11-ARIMA, `automx.f`) | `test_m4_*.py`, the `.udg` gates |
| regARIMA residual diagnostics (`check{}`) | **Done, line-exact** over 275 specs | `test_check_diagnostics.py` |
| Spectrum + peaks, QS / NP seasonality | **Done, byte-exact** — including the model-only and SEATS paths | `test_spectrum_peaks.py`, `test_qs_diagnostics.py` |
| `composite{}` / indirect adjustment (X-11) | **Done, bit-exact** — direct + indirect + comparison statistics + the diagnostics front, direct and indirect | `test_composite_tables.py` |
| R / Python in-process bindings | **Working, not yet packaged** — one shared library + two single-file loaders over a flat C ABI | `test_bindings.py`, `test_capi.cpp`, `bindings/r/test_x13c.R` |
| `composite{}` SEATS branch, pseudo-additive | **Planned** | — |
| Other less-common options | **Partial / planned** | see `tools/coverage_plan.md` |

---

## 5. Testing

- **Parity suite result:** **<!--x13:parity_pass-->7148<!--/x13--> passed · <!--x13:parity_fail-->0<!--/x13--> failed · <!--x13:parity_xfail-->0<!--/x13--> xfailed · <!--x13:parity_skip-->791<!--/x13--> skipped** (as of <!--x13:last_commit-->2026-08-05<!--/x13-->), plus <!--x13:ctest-->12/12<!--/x13--> unit tests and 165/165 R-binding tests. Runs in ~85s on 8 workers.
- **Corpus:** <!--x13:corpus_specs-->462<!--/x13--> spec files across <!--x13:parity_modules-->31<!--/x13--> parity test modules, spanning the airline model, Census example series, real economic series (unemployment, payroll employment, exports), and unedited production specs from the BLS Current Employment Statistics program.
- **0 open xfails.** Every front listed in §4 gates bit-exact. The skips are legitimate — a spec whose oracle run ships no golden for that table, or one that exercises a still-unported branch and says so with a reason.
- **Census bugs catalogued:** <!--x13:census_bugs-->39<!--/x13--> (CB-1 … CB-<!--x13:census_bugs-->39<!--/x13-->), each either reproduced bug-for-bug against the oracle or recorded with the reason it cannot be (unreachable, or a save-file artefact this port does not write).
- **Mutation testing.** A green run on an auto-discovering gate is not evidence that a newly added spec is compared at all, so each increment ends by deliberately perturbing the code it just added and confirming the gate fails — **per half of a routine, not per routine**, because the two halves often turn out to be covered by disjoint specs. Gaps this finds are recorded at the code and at the gate rather than absorbed.
- **Independent review:** completed areas are re-audited by a separate agent pass, checking port-fidelity axes (integer-power semantics, DO-loop counts, column-major indexing, 1-based↔0-based conversions) and standard C++ correctness. Findings are triaged into *real defects* vs *intentional Census-faithful* vs *unported-feature backlog*.

---

## 6. Documentation produced

Engineering documentation is generated as a byproduct of the work, not as an afterthought:

- **Scope / trace docs** (`tools/`, <!--x13:scope_docs-->32<!--/x13--> documents): `seats_scope.md`, `slidingspans_scope.md`, `engine_scope.md`, `lmdif_port_spec.md`, `coverage_plan.md`, and a per-front scouting note for each area attacked (`automdl`, `pickmdl`, `composite`, `spectrum_peaks`, `genqs`, `x11regression`, `history_options`, `dropped_options`, …). Each records what was measured, what it cost, and what was deliberately left open.
- **`tools/census_bugs.md`** — the catalogue of original-source bugs, for a later modernization pass.
- **`tools/coverage_plan.md`** — the plan to gate *every* documented spec option against the oracle (the hardening phase after the core lands).
- **Persistent project memory** — `CLAUDE.md` plus a per-session handoff (`tools/SESSION_HANDOFF.md`), carrying the vision, the milestones, and the hard-won gotchas across sessions. This is load-bearing: most of the defects found in the last week were found by re-reading a measurement someone had written down, not by re-deriving it.
- **This summary** (`docs/PROJECT_SUMMARY.md`).

---

## 7. Metrics dashboard

All figures below are **generated**, not typed: each is wrapped in a marker that
`python tools/metrics.py --write` maintains and `--check` verifies, so a stale
number here is a build failure. Full table: [`docs/METRICS.md`](METRICS.md).

| Metric | Value | Notes |
|---|---|---|
| C++ written | **<!--x13:cpp_lines-->50030<!--/x13--> non-blank lines**, <!--x13:cpp_files-->183<!--/x13--> files | excludes generated COMMON headers |
| Fortran reference | <!--x13:fortran_lines-->166076<!--/x13--> lines, <!--x13:fortran_files-->712<!--/x13--> files | not all on the port's critical path |
| Fortran routines ported | **<!--x13:routines_done-->406<!--/x13--> of <!--x13:routines_total-->690<!--/x13-->** (<!--x13:routines_pct-->58.8<!--/x13-->%) | `tools/ported.yaml`; excludes 22 not-applicable files, and counts 3 `partial` as neither |
| Parity result | <!--x13:parity_pass-->7148<!--/x13--> pass / <!--x13:parity_fail-->0<!--/x13--> fail / <!--x13:parity_xfail-->0<!--/x13--> xfail / <!--x13:parity_skip-->791<!--/x13--> skip | plus ctest <!--x13:ctest-->12/12<!--/x13-->, R bindings 165/165 |
| Corpus | <!--x13:corpus_specs-->462<!--/x13--> spec files, <!--x13:parity_modules-->31<!--/x13--> test modules | real + synthetic series |
| Census bugs catalogued | <!--x13:census_bugs-->39<!--/x13--> (CB-1 … CB-<!--x13:census_bugs-->39<!--/x13-->) | reproduced bug-for-bug, or recorded as unreachable |
| Active development time | **<!--x13:active_time-->63h 55m<!--/x13-->** over <!--x13:calendar_days-->19<!--/x13--> calendar days | `worklog.py`, gaps >45m excluded |
| Commits | <!--x13:commits-->419<!--/x13--> | 2026-07-18 → 2026-07-29 |
| Measured bit-exactness | ~5e-15 across the X-11 and SEATS table gates | double-precision noise floor |

*Two figures move for reasons worth stating. The ported-routine count jumped from an apparent 23.8% to 58.1% on 2026-07-29 — that was not a day's work, it was an **audit**: `tools/ported.yaml` recorded status by hand and its refresh command only discovered new files, so 240 routines ported over previous weeks were still marked `pending`. It is now derived from evidence in the C++ tree (`coverage_map.py --audit`). And the line count is not a productivity measure: a faithful port is often LONGER than its source, because a Fortran defect reproduced deliberately needs a paragraph explaining why it is there.*

---

## 8. User (developer) experience — *to be completed by the user*

*Placeholder for the user to fill in. Suggested prompts:*

- What did directing the port feel like versus writing it by hand? Where did it save the most time?
- Which decisions did you have to make, and which did the agent make well on its own?
- How did the background-agent / code-review hand-offs work in practice?
- Where was the friction (environment setup, build quirks, ambiguous requirements)?
- What surprised you — good and bad?
- Would you trust this approach on another codebase? Under what conditions?

---

## 9. Status and what remains

**Landed.** The whole spine is bit-exact on the corpus: regARIMA estimation and forecasting, **both** automatic model-selection engines (`automdl{}`/TRAMO and `pickmdl{}`/X-11-ARIMA), the X-11 decomposition through Part E and the F2/F3 quality statistics, the SEATS decomposition (all s10–s18, including general-shape p>0, bp>0, and a mean regressor with and without trading day), `force{}`, `slidingspans{}`, `history{}`, `composite{}`/indirect adjustment for X-11, and the full `.udg` diagnostics surface — `check{}` residual diagnostics, the spectrum and its peak tests, and the QS / NP seasonality statistics, on the X-11, SEATS and model-only paths alike. R and Python can call the engine in-process today over a flat C ABI.

**What remains, in rough order:**

- `composite{}`'s SEATS branch (`agr3s.f`), pseudo-additive, and the forced/rounded indirect series.
- `amdfct.f`'s out-of-sample arm, which closes the last `outofsample=` walls in both `pickmdl{}` and `estimate{}`.
- A long tail of less-common spec options, tracked in `tools/coverage_plan.md` against the oracle's own `ARGDIC` dictionaries rather than against the manual.
- **Priority #2:** turning the working in-process bindings into real CRAN- and PyPI-compliant packages, then the plotting layer they exist for.

**How "remaining" is decided.** Not by what fails — the suite is green — but by what is *silently accepted*. The recurring defect in this port is an option the parser consumes and the engine ignores, which looks identical to a working feature from the outside. The method is therefore to measure each option against the oracle on-vs-off, then measure the engine against the oracle, and to wall anything unported with a fatal that names the Fortran line. Several fronts closed in the last week were found this way rather than by a failing test.

This is a proof of concept and is tracked as one: progress is measured by the bit-exact parity frontier, and the remaining work is explicit rather than hidden.
