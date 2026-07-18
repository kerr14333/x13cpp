# X13cpp corpus — base series

Frozen, hermetic snapshots of real public-domain time series used as the input
data for the parity test corpus. Every file here is committed to git so a run
is fully reproducible without network access.

## File format

All `*.dat` files are **X-13 free format**: one numeric value per line, in
chronological order, no header, LF line endings. A spec consumes a series with:

```
series{
  file = "../data/<series>.dat"
  start = <YYYY.PP>     # PP = month (1-12) or quarter (1-4)
  period = <12 | 4>
}
```

No `format=` argument is needed (free format is the X-13 default when a `file`
is given without a `format`).

## Series

| file          | series                                | freq | obs | span              | units |
|---------------|---------------------------------------|------|-----|-------------------|-------|
| `airline.dat` | Box-Jenkins Series G (airline)        | 12   | 144 | 1949.01–1960.12   | passengers (thousands) |
| `payems.dat`  | US total nonfarm employment (PAYEMS)  | 12   | 308 | 2000.01–2025.08   | thousands of persons, SA-source but treated as raw |
| `unrate.dat`  | US unemployment rate (UNRATE)         | 12   | 932 | 1948.01–2025.08   | percent |
| `expgs.dat`   | US exports of goods & services (EXPGS)| 4    | 317 | 1947.Q1–2026.Q1   | billions of USD |

See the per-series `*.README.md` for exact source, vintage and provenance.

## Note on series length vs. the program limit

This build of X-13ARIMA-SEATS is compiled with `POBS = PYR1*PSP = 65*12 = 780`
(see `oracle/fortran/srslen.prm`) — the maximum monthly span it can analyze.

`unrate.dat` (932 obs) intentionally keeps its **full** history and therefore
**exceeds** that limit. Specs that analyze UNRATE apply a `series{ span = ... }`
to keep the analyzed window ≤ 780 observations. The other series are all well
within the limit.

## Provenance / access

The FRED series (`payems`, `unrate`, `expgs`) are U.S. Government works
(BLS / BEA source data via the St. Louis Fed) and are in the public domain.
The canonical export URL is
`https://fred.stlouisfed.org/graph/fredgraph.csv?id=<ID>`. Direct scripted
access to `fred.stlouisfed.org` was blocked from the build environment (TLS
reset/timeout), so the CSVs were retrieved from byte-identical Internet Archive
Wayback Machine captures. `fetch_fred.py` pins each capture timestamp and can
regenerate the `*.dat` files deterministically. Download date: **2026-07-18**.

`airline.dat` is the classic Box & Jenkins (1976) Series G, reproduced from the
public-domain published values.
