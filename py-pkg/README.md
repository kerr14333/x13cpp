# x13cpp

Python interface to **X13cpp**, a from-scratch C++17 port of the U.S.
Census Bureau's X-13ARIMA-SEATS seasonal adjustment program.

> **Status: early development.** The C++ estimation core this package
> wraps is still mid-port (see the parent repository,
> [`x13new`](https://github.com/kerr14333/x13cpp)): regARIMA estimation is
> implemented, but the X-11 and SEATS decomposition paths are not. Calling
> `seasonal_adjust()` currently raises `NotImplementedError` with an
> explanatory message. This package establishes the public API (result
> object, accessors, bundled datasets) ahead of that wiring landing.

## Design rules

* **No automatic file output.** `seasonal_adjust()` never writes anything
  to disk. Everything it computes lives on the returned `X13Result`; call
  `result.write_outputs(path)` yourself when you want files.
* **Bundled example series.** A handful of named datasets ship with the
  package under `x13cpp.datasets` for experimenting without your own data.

## Install

```
pip install x13cpp
```

Optional extras:

* `pip install x13cpp[pandas]` -- enables `TimeSeries.to_pandas()`.

## Usage (once the core is wired in)

```python
import x13cpp
from x13cpp.datasets import load_airline

series = load_airline()
result = x13cpp.seasonal_adjust(series, transform="log",
                                 arima="(0 1 1)(0 1 1)", x11=True)

result.udg               # dict of scalar diagnostics
result.seasadj()          # seasonally adjusted series (save table "d11")
result.trend()            # trend component ("d12")
result.irregular()        # irregular component ("d13")
result.save_table("d10")  # any save table by X-13 code

result.write_outputs("out/")  # the ONLY way results reach disk
```

## Bundled datasets

`x13cpp.datasets` ships four public-domain series as `TimeSeries` objects
(a lightweight `values` + `start` + `freq` container -- see
`x13cpp.TimeSeries`; call `.to_pandas()` for a `pandas.Series` with a
`PeriodIndex` if you have pandas installed):

| loader | series | freq | start | obs |
|---|---|---|---|---|
| `load_airline()` | Box-Jenkins Series G (airline passengers) | 12 | 1949.01 | 144 |
| `load_expgs()` | US exports of goods & services (FRED EXPGS) | 4 | 1947.Q1 | 317 |
| `load_payems()` | US total nonfarm employment (FRED PAYEMS) | 12 | 2000.01 | 308 |
| `load_unrate()` | US unemployment rate (FRED UNRATE) | 12 | 1948.01 | 932 |

Two more loaders are planned but not yet sourced: `load_shoe_sales()`
(Easter-effect testing) and `load_retail_sales()` (trading-day testing).
Both currently raise `NotImplementedError`; see their docstrings for the
TODO.

## Why not a hard pandas dependency?

`TimeSeries` (a plain `numpy` array plus `start`/`freq` metadata) is enough
to represent the regularly-spaced monthly/quarterly series X-13ARIMA-SEATS
operates on, and keeps the base install lightweight. If you want a
`pandas.Series` with a proper `PeriodIndex`, install `x13cpp[pandas]` (or
plain `pandas`) and call `.to_pandas()`.

## License

CC0 1.0 Universal -- see [LICENSE](LICENSE).
