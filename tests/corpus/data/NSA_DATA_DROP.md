# Dropping non-seasonally-adjusted (NSA) data for seasonal testing

The current corpus series **PAYEMS / UNRATE / EXPGS are seasonally adjusted at
source** (see their `*.README.md`); only `airline.dat` is genuinely NSA. Real
seasonal-ARIMA / seasonal-adjustment / automdl-with-seasonal testing needs NSA
input. Network is blocked from this build environment (FRED returns 403,
web.archive.org is unreachable), so the data has to be dropped in by hand.

## Where to put it

Drop each series as a plain-text file in **this directory**
(`tests/corpus/data/`):

```
tests/corpus/data/<name>.dat
```

Suggested names + the FRED NSA series to pull:

| file                | FRED series id | what it is                         | period |
|---------------------|----------------|------------------------------------|--------|
| `paynsa.dat`        | `PAYNSA`       | Total nonfarm employment, NSA      | 12     |
| `unratensa.dat`     | `UNRATENSA`    | Unemployment rate, NSA             | 12     |
| `retailnsa.dat`     | `MRTSSM44000USN` (or any NSA monthly) | Retail sales, NSA | 12 |
| `<quarterly>nsa.dat`| any NSA quarterly series | e.g. an NSA exports/GDP-component | 4 |

(Any NSA series works — these are just sensible seasonal test cases. A strongly
seasonal monthly series is the most useful.)

## File format (important)

X-13 **free format**: one numeric value per line, in date order, **no header,
no dates, no commas**. Exactly like the existing `airline.dat`. Example:

```
112
118
132
129
...
```

The FRED CSV is `DATE,VALUE` rows with a header — strip the header and the date
column (keep only VALUE, one per line). `fetch_fred.py` in this directory already
does this conversion; if you have network on your machine, add the series id to
its `SERIES` dict and run it. Otherwise convert the CSV by hand / with a one-liner.

## Tell me these three things per series (or add a `<name>.README.md`)

1. **start** — first observation's date, `YYYY.MM` monthly or `YYYY.Q` quarterly
   (e.g. `2000.01`, `1990.1`).
2. **period** — `12` monthly, `4` quarterly.
3. **trim** (optional) — if you want only a sub-span (e.g. `2000.01` onward),
   say so; otherwise I use the whole file.

## What I'll do once the data is here

The full pipeline is already working (just used it for the 13 fixed-model specs):

1. Write specs (`tests/corpus/<...>/<name>_*.spc`) — fixed seasonal models AND
   `automdl{}` runs.
2. Generate authoritative goldens with the committed oracle
   (`oracle/fortran/x13as_ascii_O2.exe` via `oracle/run_oracle.py -s`).
3. Wire them into the parity gates — the M1/M3 gates auto-discover fixed-model
   specs; I'll extend the M4 automdl gate (`tests/parity/test_m4_iddiff.py`) to
   check that automdl now finds seasonal differencing (D>0) on genuinely
   seasonal data, plus the full estimation.
4. Run the suite and report bit-exactness vs the oracle.

## How to trigger me

Just say "NSA data is in" (or drop it and ping). No other setup needed — the
oracle binary and the generate→run→gate tooling are already in the repo.
