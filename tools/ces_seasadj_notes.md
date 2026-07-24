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

## Triage of the full production set (2026-07-24)

Reproduce with the two committed tools:

```
# inputs (contact UA required -- see above)
curl -A "x13cpp-research <you>" -O https://www.bls.gov/web/empsit/ces.spec.ae.zip
curl -A "x13cpp-research <you>" -O https://www.bls.gov/web/empsit/ces.spec.other.zip
# one ce.data.<NN>a.<name>.Employment per supersector (~75 MB total; the exact
# names come from the directory listing -- they end in `.Employment`, and a
# truncated name 404s)
curl -A "x13cpp-research <you>" -O https://download.bls.gov/pub/time.series/ce/ce.data.10a.MiningAndLogging.Employment

python tools/ces_make_corpus.py --specs <ae dir> --regressors <other dir> \n    --ce-data <data dir> --out <corpus>
python tools/ces_triage.py --corpus <corpus> \n    --oracle oracle/fortran/x13as_ascii_O2.exe --engine build/x13run_x11.exe \n    --composite build/x13run_composite.exe --jobs 10 --json triage.json
```

**Set:** 142 specs = 78 direct (`AE*`) + 14 composite totals (`CO*` with
`composite{}`) + 50 components (`CO*` with `comptype=`) -> 89 runnable jobs
(components run inside their group's metafile).

**Result: 86/89 at 1e-12, 88/89 at the 1e-6 estimation floor.**

| verdict | n | notes |
|---------|---|-------|
| bit-exact (<=1e-12) | 86 | incl. 10 of 11 composite groups: d10/d11 direct + isf/isa indirect |
| estimation-floor    | 2  | `CO6562100000` isf 5.9e-7; `AE4348100000` d10 4.1e-8 |
| outlier             | 1  | `AE7072200000` -- see below |
| not runnable        | 3  | `CO2023610000/620000/800000`: components live in the 667-spec `ces.spec.ae2.zip`, not in `ces.spec.ae.zip`, so the ORACLE fatals too (its own SIGFPE). Not a port issue. |

**`AE7072200000` (Food services and drinking places)** -- the one genuine
frontier case. d11 2.55e-6, d16 6.9e-3, d10 1.73e-1. The d10 headline is
inflated: `mode=add` centres the seasonal factors on zero, so the max ABSOLUTE
d10 error (2.27e-2 on a series of scale ~389) lands at a near-zero factor. Still,
d11 at 2.55e-6 is just outside the estimation floor. Not a TC-regressor problem:
49 of the 50 TC-carrying specs are bit-exact. Distinctive features are 18 manual
outlier `variables=` (a dense COVID LS/TC cluster) on a `(0 1 2)(0 1 1)` model
with user TD -- i.e. a hard optimisation surface. Estimation-path, not structural.

**Two real port bugs this triage surfaced** (both fixed, commit 8f0a785):
1. `composite{}` never set `Havesp` (getcmp.f:213) -- the CES totals have no
   `series{}` spec, so `x11{seasonalma=}` fatalled. All 11 groups were dead.
2. `Orig2`'s forecast region was never populated (arima.f:1433-1441). Orig2 is
   what composite adjustment aggregates, so O2/O5 -- hence the indirect seasonal
   factors -- were built from zeros over the appended forecast span. The
   census-examples corpus could not see it (no `forecast{}` there); every CES
   spec has `forecast{maxlead=24} + appendfcst=yes`.

## Disk

`ces_series/` (337 MB, mostly `ce.data.0.AllCESSeries`) was **deleted 2026-07-24**
on the user's say-so — only ~143 tiny NSA series are ever needed and those already
live in `tests/corpus/ces/data/`. Re-fetch with the curl + contact-User-Agent
recipe above when the spec generator needs the bulk file again (`ce.series`, the
3.8 MB series-id map, comes back the same way).

## Vertical slice kept as the canonical example

`tests/corpus/ces/AE1011330000_simple.spc` (stripped, uppercase BLS keywords) +
`data/AE1011330000.dat` + golden under `tests/golden/ces/AE1011330000_simple/` is
**committed as the uppercase parser-fidelity gate**. The full BLS spec
`AE1011330000.spc` (USER TD regressors + `FINAL=USER`) is committed and gated too
as of `13d03c2` — both gate b1/d10/d11/d16 bit-exact via
`tests/parity/test_ces_tables.py`. (`FINAL=USER` turned out to be inert on these
specs: the user regressors are `usertype=td`, so their effect lands in Factd, not
Facusr.)
