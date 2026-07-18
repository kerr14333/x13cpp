# Test corpus

A frozen, hermetic collection of input data and X-13ARIMA-SEATS spec files used
to parity-test the C++ port against the Fortran oracle. Everything here is
committed to git so runs are reproducible without network access.

## Layout

| dir                | contents |
|--------------------|----------|
| `data/`            | Base time series (`*.dat`, X-13 free format) + per-series READMEs. Airline (Box-Jenkins), and FRED PAYEMS / UNRATE / EXPGS. |
| `census-examples/` | Canonical examples from the Census manual (basic X-11, airline model, automdl, SEATS, composite). Hand-transcribed. |
| `generated/`       | Systematically generated specs (`genspecs.py`): 4 series × 8 configs = 32 specs, decorated with `print/save/savelog`. |
| `edge/`            | Boundary and error cases: 780-obs limit, malformed spec, span/modelspan. |

## Running a spec

X-13 resolves `file=` relative to the current working directory, so run each
spec from its own directory. Example:

```
cd census-examples
x13as -i 01-basic-x11 -o /tmp/01-basic-x11
```

Specs reference data with relative paths (`../data/<series>.dat`); `edge/` and
`census-examples/composite/` reference their local data files by basename.

## Regenerating

- `generated/`: `python generated/genspecs.py` (deterministic, idempotent).
- `data/` FRED series: `python data/fetch_fred.py` (pinned snapshots).

The airline, `edge/`, and `census-examples/` files are static and committed.
