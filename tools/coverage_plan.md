# Exhaustive spec-option coverage plan (hardening phase)

Goal: gate **every documented X-13ARIMA-SEATS spec option** against the Fortran
oracle, not just the ~10 representative configs the current corpus exercises.
This is a productization/hardening step — it makes the port a *trustworthy*
library. Do it AFTER the core engine + SEATS land; the PoC ships on the
representative corpus.

## Current coverage (baseline)

`tests/corpus/generated/genspecs.py` emits ~10 CONFIGS (x11 default/logadd/
additive, seats, automdl-x11, fixed-airline-x11/seats, automdl-aictest-x11) ×
a few SERIES × a representative SAVE-tag subset; `tests/corpus/extra/genextra.py`
adds force/slidingspans/history variants. ~50-80 specs. This covers the spec
*blocks* and common paths — NOT every argument × value.

## Authoritative sources for the option space

1. **`oracle/fortran/get*.f` ARGDIC dictionaries + validation (PRIMARY).** Each
   spec's reader hardcodes its exact argument dictionary, allowed-value sub-
   dictionaries, numeric ranges, and defaults (e.g. `getfrc.f`, `getssp.f`,
   `getx11.f`). These are exact and self-consistent with what the C++ port must
   reproduce — more reliable than the manual. We already read them for every
   `gt_*` parser port.
2. **Reference Manual Ch.7 §7.1-7.20 (human cross-check).**
   `https://www.census.gov/content/dam/Census/library/working-papers/2017/adrm/docx13ashtml.pdf`
   (v1.1, 2017-01-18, 297pp). Chapter 7 documents all 20 specs with argument
   descriptions, allowed values, and defaults. Local copy of the raw text can be
   extracted with PyMuPDF (census.gov 403s the default UA — send a browser
   User-Agent, same as the BLS gotcha).
3. **Appendix B (p261) — Print and Save Tables.** Enumerates every print/save
   tag per spec (incl. seats-special B.2, spectrum-special B.3, percentage
   tables B.4). This is the save-side coverage checklist.

### The 20 documented specs (manual page refs)

| spec | §7.x p | spec | §7.x p |
|---|---|---|---|
| arima | 71 | outlier | 139 |
| automdl | 75 | pickmdl | 145 |
| check | 86 | regression | 150 |
| composite | 93 | seats | 175 |
| estimate | 101 | series | 186 |
| force | 108 | slidingspans | 197 |
| forecast | 115 | spectrum | 207 |
| history | 120 | transform | 215 |
| metadata | 130 | x11 | 226 |
| identify | 135 | x11regression | 241 |

## Method

1. **Extract the option matrix.** Parse each `get*.f` ARGDIC + validation into a
   machine-readable table: `{spec, arg, type, allowed-values|range, default,
   dependencies}`. Cross-check against the manual §7.x for that spec (catches
   parser/manual drift, itself worth logging). Output: `tools/option_matrix.json`.
2. **Generate the spec matrix.** For each argument, emit specs varying it across
   its value set, layered on a valid per-series base config. Strategy: **each-
   argument-once + pairwise for interacting args** — NOT the full cross-product
   (combinatorial explosion). Include **expected-fatal** specs for out-of-range /
   mutually-exclusive combinations (the parser's own error paths are part of the
   contract). Extend `genspecs.py` rather than replace it.
3. **Batch-bless via the oracle.** `oracle/run_oracle.py` already blesses one
   spec (save tables + manifest); scale it to a parallel batch over the whole
   matrix. The oracle is deterministic and fast, so thousands of specs is a
   minutes-to-hours job, not a blocker. `--flags=-s` (equals form; space form
   breaks argparse).
4. **Scale the harness.** `tests/parity/run_parity.py` already discovers the
   corpus tree and diffs; the `test_*_tables.py` files are the per-feature gates.
   Needs: parallel C++ runs, expected-fatal handling (compare outcome + `.err`
   text, as `test_m1_parse.py` already does), and per-spec save-tag diffing
   driven by Appendix B.
5. **Coverage tracker.** A manifest mapping each option (spec/arg/value) → the
   spec(s) exercising it → pass/xfail/uncovered, so gaps are *visible* and the
   suite stays honest as options are ported. Report % of the option matrix gated
   green.

## Scale & storage

- **Size:** 20 specs × ~5-25 args × 2-N values → **hundreds to low-thousands**
  of single-variation specs; more with pairwise combinations.
- **Storage is the real cost** (full save-table goldens × thousands). Mitigate:
  store per-spec/per-table **hashes** for the bulk, keep full goldens only for a
  curated subset + any spec whose hash diverges. The full-golden parity corpus
  stays test-only (not shipped in the package — see the packaging note).

## Sequencing

Blocked on the core engine being bit-exact first, so the matrix has a correct
target to gate against. **The SEATS half of that precondition is met** (the
decomposition, the forecast decomposition and `composite{}` under SEATS all gate
bit-exact); what remains is the X-11 / regARIMA option surface still behind a
wall — `docs/WALLS.md` is the live count, and this document deliberately does
not repeat it.

This is the QA phase that turns "the port reproduces the oracle on our examples"
into "the port reproduces the oracle on every documented option," which is the
bar for a shippable R/Python library.
