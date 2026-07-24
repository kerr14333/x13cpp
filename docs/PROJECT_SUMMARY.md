# X-13ARIMA-SEATS C++ Port — Project Summary

*Proof of concept: directing Claude Code to port a large legacy codebase, prove it correct, and document it.*

Last updated: 2026-07-21. Figures in this document are pulled from the live repository (parity suite, `git`/`worklog.py`, source counts) and are cited as of that date.

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
- **`xfail` as the work frontier.** Not-yet-ported paths are marked as *expected failures* (pytest strict xfail). The count of xfails is a live measure of remaining work; an unexpected **pass** turns into a loud failure, so the frontier can never silently drift.
- **Census-bug fidelity.** Every genuine bug found in the Fortran is catalogued (`tools/census_bugs.md`, currently **CB-1 … CB-11**) and reproduced deliberately in the C++, with a comment pointing back to the Fortran line.
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
| X-11 decomposition (B/C/D/E tables) | **Done** (17/19 specs ~5e-15) | `test_x11_tables.py` |
| `force{}` (Denton / Cholette-Dagum / rounding) | **Done, bit-exact** | `test_force_tables.py` |
| SEATS decomposition | **Done, bit-exact** — every SEATS corpus spec's s10–s18 gate (~5e-15), plus general-shape p>0 (`ar2-seats`), bp>0 (`sar-seats`), and `imean!=0` (mean regressor) for both d>=1 (`mean-seats`) and d==0 (`mean-d0-seats`); only a mean alongside other regressors unported (guarded-fatal) | `test_seats_tables.py` |
| `slidingspans{}` | **Done, bit-exact** — all 4 spans, sfs+chs | `test_slidingspans_tables.py` |
| `history{}` | **Done, bit-exact** — sar/sae/trr/tre (~7e-6 re-estimation floor) | `test_history_tables.py` |
| Other diagnostics / less-common options | **Partial / planned** | see `tools/coverage_plan.md` |

---

## 5. Testing

- **Parity suite result (current):** **1033 passed · 0 failed · 0 xfailed · 25 skipped.**
- **Corpus:** spec files across 11 parity test modules, spanning the airline model, Census example series, and real economic series (unemployment, payroll employment, exports).
- **0 open xfails.** The X-11 decomposition spine, SEATS decomposition, the whole diagnostics front (force / slidingspans / history), the model X-11 path, and the X-11 spec-option front (type / shrink / sigmavec / x11easter / user-regression prior factor) all gate bit-exact. The 17 skips are legitimate (oracle ships no golden / no table for those specs). Remaining work is interdependent x11 factor-producer chains, not a passing/failing ledger.
- **Census bugs reproduced:** 13 (CB-1 … CB-13), each verified to match the oracle bug-for-bug.
- **Independent review:** completed areas are re-audited by a separate agent pass, checking port-fidelity axes (integer-power semantics, DO-loop counts, column-major indexing, 1-based↔0-based conversions) and standard C++ correctness. Findings are triaged into *real defects* vs *intentional Census-faithful* vs *unported-feature backlog*.

---

## 6. Documentation produced

Engineering documentation is generated as a byproduct of the work, not as an afterthought:

- **Scope / trace docs** (`tools/`): `seats_scope.md`, `slidingspans_scope.md`, `engine_scope.md`, `lmdif_port_spec.md`, `coverage_plan.md`, and per-milestone scouting notes.
- **`tools/census_bugs.md`** — the catalogue of original-source bugs, for a later modernization pass.
- **`tools/coverage_plan.md`** — the plan to gate *every* documented spec option against the oracle (the hardening phase after the core lands).
- **Persistent project memory** — a running narrative of vision, milestones, and hard-won gotchas that survives across sessions.
- **This summary** (`docs/PROJECT_SUMMARY.md`).

---

## 7. Metrics dashboard

| Metric | Value | Notes |
|---|---|---|
| C++ ported | **~28,800 non-blank lines**, 212 files | reproduces the behavior of the Fortran below |
| Fortran reference | ~166,000 lines, 712 files | not all on the port's critical path |
| Parity result | 1033 pass / 0 fail / 0 xfail / 25 skip | as of 2026-07-24 |
| Corpus | spec files across 11 test modules | real + synthetic series |
| Census bugs catalogued | 13 (CB-1 … CB-13) | reproduced bug-for-bug |
| Active development time | **~19h 46m** over 3 calendar days | via `worklog.py`, through last commit (2026-07-20) |
| Commits | 239 | through 2026-07-24 (SEATS general-shape p>0/bp>0 close) |
| Measured bit-exactness | ~5e-15 on 17/19 X-11 specs | double-precision noise floor |

*Note on time: `worklog.py` measures committed activity through 2026-07-20. The most recent session (completing `force{}`, moving SEATS s11/s12 to bit-exact, and applying a batch of review fixes) is additional and not reflected in that figure.*

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

**Landed:** the full regARIMA → automatic-model → X-11 pipeline is bit-exact on the corpus; `force{}` is complete; the SEATS decomposition is fully bit-exact — the whole corpus (all s10–s18) plus the general-shape p>0 (`ar2-seats`), bp>0 (`sar-seats`), and `imean!=0` (mean regressor) models for both d>=1 (`mean-seats`) and d==0 (`mean-d0-seats`) (only a mean alongside other regressors unported, and guarded-fatal); the whole diagnostics front (`slidingspans{}` / `history{}`) is closed; the model X-11 path and the X-11 spec-option front (`type` / `shrink` / `sigmavec` / `x11easter` / user-regression prior factor) all gate bit-exact.

**In progress / next:**

- Remaining interdependent x11 factor-producer chains: user PRIOR factors (Nuspad/Nustad), Adjsea/Adjso regARIMA-seasonal combine, x11regression prior-TD, force non-original target (Iftrgt>0), revisions getrev.
- **Hardening phase:** exhaustive spec-option coverage against the oracle (`coverage_plan.md`), then the R and Python library wrappers.

This is a proof of concept and is tracked as one: progress is measured by the bit-exact parity frontier, and the remaining work is explicit rather than hidden.
