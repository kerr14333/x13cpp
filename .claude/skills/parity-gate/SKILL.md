---
name: parity-gate
description: Add a corpus spec, bless its oracle golden, and gate the C++ port against it bit-exact. Use after porting a leaf or when adding parity coverage for a new X-13 input/interaction (a regressor, outlier, transform, model, span, forecast). Carries the exact run_parity.py + pytest commands and the --filter/-O2 gotchas.
---

# Gate a parity spec against the oracle

The port's contract is bit parity with `oracle/fortran/x13as_ascii_O2.exe`. To
cover a new feature: write a spec, bless the oracle golden, run the matching
M-gate.

## 1. Write the spec

Drop a `.spc` in `tests/corpus/` (subdir by kind — `generated/`, `extra/`,
`census-examples/`). Clone a sibling (e.g. `generated/airline_reg-easter.spc`).
Keep it minimal and estimation-reproducible: explicit `arima{}` model, ported
regressors only, no unported auto-selection unless that's what you're gating.
Data paths are relative to the spec (`file = "../data/airline.dat"`).

## 2. Bless the golden from the oracle

```bash
cd tests/parity
python run_parity.py --binary ../../oracle/fortran/x13as_ascii_O2.exe \
    --update --filter "generated/<spec-id>"
```

Gotchas:
- The flag is **`--filter`**, NOT `--pattern`.
- Use the **`_O2`** binary (the parity target), not `_O0`.
- `<spec-id>` is the corpus-relative path without `.spc`
  (`generated/airline_reg-labor`). Golden lands in `tests/golden/<id>/`.
- Expect `[BLESSED] ... result: PASS`.

## 3. Run the matching M-gate

pytest, filtered to the new spec:
```bash
python -m pytest tests/parity/test_m3_estimate.py -q -k "<name>"
```
Pick the gate by feature: `test_m2_tables` (save-file tables), `test_m3_estimate`
(regARIMA estimation — regressors/outliers/models), `test_m3_forecast`,
`test_m4_iddiff` / `test_m4_aictest` / `test_m4_trnaic` (automdl). The M3
estimate gate auto-discovers every reproducible corpus spec, so a new
estimation spec is picked up with no test edit.

## 4. Confirm no regression

Run the full relevant gate(s) before committing:
```bash
python -m pytest tests/parity/test_m3_estimate.py tests/parity/test_m2_tables.py -q
```
All green (some `s` skips are expected — unported-feature specs self-skip).

## Notes

- If M2 fatals too, it's a **parse** gap (skip is legitimate). If M2 passes but
  M3 fatals, it's an **estimation** regression (real failure).
- Tolerances live in the parity tolerances file; `.udg` goldens print 4–6 sig
  digits, so the pytest gates use rtol 1e-6 with EXACT integer canaries
  (niter/nfev/nreg/nefobs).
- Coverage runs point the gates at a different binary via `X13_BIN_DIR`.
