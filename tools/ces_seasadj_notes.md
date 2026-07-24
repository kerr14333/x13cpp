# CES (Current Employment Statistics) seasonal-adjustment corpus — working notes

Goal: use the BLS CES program's **real** X-13 spec files + NSA data to (a) build a
robustness corpus and (b) surface Fortran↔C++ discrepancies. NSA "All Employees"
series only.

## BLS resources — how to fetch (bls.gov 403s default agents; a CONTACT UA works)

`curl` with a contact-info User-Agent bypasses the 403 (do NOT use WebFetch — it
403s). UA used: `x13cpp-research cyg5005@gmail.com`.

- **Time-series flat files:** `https://download.bls.gov/pub/time.series/ce/`
  - `ce.data.0.AllCESSeries` (349 MB — ALL series values, tab-sep:
    `series_id \t year \t period \t value \t footnote`; period `M01`..`M12`,
    `M13`=annual avg (skip)). Per-supersector files (`ce.data.NNa.*.Employment`)
    are smaller alternatives.
  - `ce.series` (series metadata: series_id, supersector, industry_code(8),
    data_type_code(2), seasonal(S/U), title, begin/end).
  - `ce.datatype` (01 = ALL EMPLOYEES, THOUSANDS), `ce.seasonal` (U = NSA, S = SA).
- **Seasonal-adjustment spec files + regressors:**
  `https://www.bls.gov/web/empsit/cesseasadj.htm` links 7 zips:
  - `ces.spec.ae.zip` → dir `ces.spec.ae/` with **143** `AE<code>.spc` files (All
    Employees). ← the target set.
  - `ces.spec.aehe.zip` (hours&earnings), `ces.spec.nonae.zip` (prod/nonsup), the
    `*2.zip` variants (2nd spec set), `ces.spec.other.zip`.
  - `ces.spec.other.zip` → the **regressor** files + methodology:
    - `FDUM8606.dat` (11-col user TD regressors, the "4-vs-5 week effect",
      monthly from 1986.01, 540 rows). Referenced by AE specs as
      `REGRESSION{ USER=(dum1..dum11) USERTYPE=TD FILE='c:\FDUM8606.dat' }`.
    - `Fdumpc96/Fdumpcw6/Fdumw96/Fdumel06/Fdumel96/DUMlp06/DUMlpel6.dat` — poll
      workers, postal, Easter/Labor-Day, 10/11-day variants (see readme.other.txt).
    - `outliers.xlsx` (manual outliers since last annual run — already baked into
      the shipped .spc REGRESSION blocks), `prior_adjustment_file.xlsx` (strike /
      decennial-census priors, applied to NSA data BEFORE X-13; most series none).

## Series-id ↔ spec mapping

BLS spec file `AE<code>` where `<code>` is 10 digits; the CES **NSA** series is
`CEU + <first 8 digits of code> + 01` (data_type 01 = All Employees).
E.g. `AE1011330000` (Logging) → `CEU1011330001`. The `.spc` `NAME` field is
`'<code> - AE'`.

## Extraction recipe (Python, one pass over ce.data)

For each series: filter `series_id`, `period != M13`, `year >= SERIES START year`,
sort by (year, month), write values one-per-line to `data/AE<code>.dat`. X-13
free-format (no FORMAT arg) reads whitespace-separated values in order; the
`SERIES{ START=... PERIOD=12 }` supplies the dating.

## Spec rewrite

Only rewrite the DOS `FILE = 'c:\...dat'` paths to repo-relative
(`"data/AE<code>.dat"`, `"data/FDUM8606.dat"`). Keep model / regression / outlier /
x11 verbatim (BLS's own choices).

## Option surface across the 143 AE specs (all RUN — none hard-fatal)

Blocks: SERIES, TRANSFORM(log), REGRESSION(128; USER TD + manual AO/LS
`variables=`), ARIMA(142; wide model variety incl (0 1 0)(0 1 1),(0 2 1)(0 1 1),
(3 1 1)(0 1 1), seasonal (1 0 1) etc.), ESTIMATE(maxiter 3000), FORECAST(maxlead
24), OUTLIER(auto, critical 10.5, types AO), X11(mode add/mult; seasonalma s3x5
(104)/s3x3(36)/s3x9(2); **final=user** (124); appendfcst=yes), **COMPOSITE{}**(14;
compwt/comptype).

### RESOLVED — the "tail discrepancy" was a MISDIAGNOSIS (parser bug)

The reported ~1-5e-3 recent-years "tail forecast-extension bug" **did not exist**.
Root cause: **the spec parser matched enumerated argument VALUES case-sensitively**,
so uppercase `TRANSFORM{ FUNCTION = LOG }` (every BLS CES spec is uppercase) matched
no branch and the **log transform was silently dropped** — the engine decomposed the
series in LEVELS while the (case-insensitive) oracle used logs. On this Logging
series the levels-vs-log seasonal MA happened to sit close (0.815 vs 0.846), so it
*looked* like a small tail error rather than the gross mismatch it was (airline
(0 1 0)(0 1 1) uppercase gave 0.156 vs 0.602 — the tell). FIXED by folding bare NAME
value-tokens to lowercase at the single capture seam (`readers_spec.cpp`
`push_tok`), matching the Fortran's case-insensitive dictionary lookups. With a
correctly-parsed (or now, any uppercase) spec the CES series is **bit-exact
end-to-end** — `AE1011330000_simple` gates a1/d10/d11 at ~1e-15, seasonal MA
0.845939 / 28 iters / logL 270.4279 == oracle. Committed as the M5 parser-fidelity
fix + the `tests/corpus/ces/AE1011330000_simple` uppercase gate. A second, smaller
gap CES surfaced and also fixed in the same pass: the save-table column HEADER used
the run base name, not the `series{ name=... }` field (the oracle's Serlbl) — now
threaded through `ctx.captured.series_name` -> `run_pre_model` serlbl (corpus specs
omit `name=`, so unchanged).

### Still open to reproduce the FULL 143-spec corpus BIT-EXACT

1. **FINAL = USER** (124/143). Runs but D11 (final SA) is GARBAGE (e.g. -2896, inf
   vs ~51). User-defined final seasonal adjustment / the user-TD regressor final
   combine is not reproduced. (Related to the CLAUDE.md "user PRIOR factors
   (Nuspad/Nustad)" / force non-orig frontier.) This is the real remaining blocker
   for most AE specs.
2. **COMPOSITE{}** (14 specs). Indirect/aggregate adjustment (adjust an aggregate
   from its components). Verify ported / port.
3. **Save-table headers for x11/seats tables** (b1/d10/d11/...): the `name=` Serlbl
   fix currently covers the pre-model tables (a1/a2/a3/trn via `run_pre_model`); the
   X-11 / SEATS save paths still label with the base name. Only a1 is gated
   byte-identical today, so this is cosmetic-until-gated, but thread `serlbl` there
   too for full byte-identical save parity.

## Disk

`ces_series/ce.data.0.AllCESSeries` is 349 MB; only ~143 tiny NSA series are
needed. Plan: extract those to `tests/corpus/ces/data/`, then delete the 349 MB
file (+ the unused per-datatype metadata / xlsx) from `ces_series/`.

## Vertical slice kept as the canonical example

`tests/corpus/ces/AE1011330000_simple.spc` (stripped, uppercase BLS keywords) +
`data/AE1011330000.dat` + golden under `tests/golden/ces/AE1011330000_simple/` is
**committed as the uppercase parser-fidelity gate** — bit-exact a1/d10/d11. The full
BLS spec `AE1011330000.spc` (USER TD regressors + `FINAL=USER`) is kept WIP
(uncommitted) pending the FINAL=USER port (open item 1 above); it parses + runs
OUTCOME OK and its a1 is bit-exact, but D11 is not (FINAL=USER).
