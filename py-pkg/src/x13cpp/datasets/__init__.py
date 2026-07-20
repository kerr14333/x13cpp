"""Bundled example series.

Four public-domain economic time series, bundled as :class:`TimeSeries`
objects for experimenting with :func:`x13cpp.seasonal_adjust` without
needing your own data. These are the same frozen series used as the base
corpus for this project's Fortran-vs-C++ parity tests
(``tests/corpus/data/`` in the ``x13new`` source repository).

>>> from x13cpp.datasets import load_airline
>>> series = load_airline()
>>> len(series)
144
>>> series.to_pandas().head()  # doctest: +SKIP
"""

from __future__ import annotations

import importlib.resources as resources

from x13cpp._timeseries import TimeSeries

def _read_dat(filename: str) -> list:
    """Read an X-13 free-format .dat file: one numeric value per line."""
    data_ref = resources.files("x13cpp.datasets").joinpath("data", filename)
    text = data_ref.read_text(encoding="ascii")
    return [float(line) for line in text.splitlines() if line.strip()]


def load_airline() -> TimeSeries:
    """International airline passengers (Box-Jenkins Series G).

    Monthly totals of international airline passengers, 1949-1960. The
    classic Box & Jenkins "Series G" dataset -- the canonical
    ``(0 1 1)(0 1 1)`` airline-model example used throughout the X-13
    documentation. Also distributed as ``AirPassengers`` in R.

    Returns:
        A :class:`TimeSeries`, monthly, 1949.01-1960.12 (144 observations),
        units of passengers in thousands.

    Source:
        Box, G.E.P. and Jenkins, G.M. (1976), *Time Series Analysis:
        Forecasting and Control*, Series G. Public domain.
    """
    values = _read_dat("airline.dat")
    return TimeSeries(
        values=values, start=(1949, 1), freq=12, name="airline",
        source="Box & Jenkins (1976), Series G. Public domain.",
    )


def load_expgs() -> TimeSeries:
    """US exports of goods and services (FRED EXPGS).

    Quarterly US exports of goods and services, 1947 Q1 onward.

    Returns:
        A :class:`TimeSeries`, quarterly, starting 1947 Q1 (317
        observations), units of billions of US dollars (seasonally
        adjusted annual rate at source; bundled here as a raw input
        series).

    Source:
        U.S. Bureau of Economic Analysis (NIPA) via FRED, St. Louis Fed,
        series ``EXPGS``. Public domain (U.S. Government work). Retrieved
        via a pinned Internet Archive Wayback Machine capture -- see
        ``tests/corpus/data/expgs.README.md`` and
        ``tests/corpus/data/fetch_fred.py`` in the ``x13new`` source
        repository for exact provenance.
    """
    values = _read_dat("expgs.dat")
    return TimeSeries(
        values=values, start=(1947, 1), freq=4, name="expgs",
        source="FRED EXPGS via pinned Wayback capture. Public domain.",
    )


def load_payems() -> TimeSeries:
    """US total nonfarm employment (FRED PAYEMS).

    Monthly count of all employees, total nonfarm, 2000-01 onward (trimmed
    from the full FRED history, which begins 1939-01).

    Returns:
        A :class:`TimeSeries`, monthly, starting 2000.01 (308
        observations), units of thousands of persons. Seasonally adjusted
        at the FRED source; bundled here as a raw input series for
        exercising code paths, not for economic analysis.

    Source:
        U.S. Bureau of Labor Statistics via FRED, St. Louis Fed, series
        ``PAYEMS``. Public domain (U.S. Government work). Retrieved via a
        pinned Internet Archive Wayback Machine capture -- see
        ``tests/corpus/data/payems.README.md`` and
        ``tests/corpus/data/fetch_fred.py`` in the ``x13new`` source
        repository for exact provenance.
    """
    values = _read_dat("payems.dat")
    return TimeSeries(
        values=values, start=(2000, 1), freq=12, name="payems",
        source="FRED PAYEMS via pinned Wayback capture. Public domain.",
    )


def load_unrate() -> TimeSeries:
    """US unemployment rate (FRED UNRATE).

    Monthly US unemployment rate, full history, 1948-01 onward.

    Returns:
        A :class:`TimeSeries`, monthly, starting 1948.01 (932
        observations), units of percent.

    Source:
        U.S. Bureau of Labor Statistics via FRED, St. Louis Fed, series
        ``UNRATE``. Public domain (U.S. Government work). Retrieved via a
        pinned Internet Archive Wayback Machine capture -- see
        ``tests/corpus/data/unrate.README.md`` and
        ``tests/corpus/data/fetch_fred.py`` in the ``x13new`` source
        repository for exact provenance.
    """
    values = _read_dat("unrate.dat")
    return TimeSeries(
        values=values, start=(1948, 1), freq=12, name="unrate",
        source="FRED UNRATE via pinned Wayback capture. Public domain.",
    )


def load_shoe_sales() -> TimeSeries:
    """Shoe/footwear retail sales series (Easter-effect testing). NOT YET BUNDLED.

    TODO(datasets): this series is planned but not yet sourced. It is meant
    to exercise Easter-effect regressors (X-13
    ``regression{ variables = easter[1] }`` and friends), which need a
    series with visible pre-Easter sales spikes. Source it the same way as
    the other FRED-derived series -- a pinned Internet Archive Wayback
    Machine capture of the FRED CSV, following the pattern in
    ``tests/corpus/data/fetch_fred.py`` -- then add the resulting
    ``shoe_sales.dat`` + README to ``tests/corpus/data/``, copy it into
    ``src/x13cpp/datasets/data/``, and replace this stub with a real
    loader matching :func:`load_airline` et al.
    """
    raise NotImplementedError(
        "load_shoe_sales() is a placeholder: no shoe-sales series has been "
        "sourced yet. See the TODO in this function's docstring."
    )


def load_retail_sales() -> TimeSeries:
    """General retail sales series (trading-day testing). NOT YET BUNDLED.

    TODO(datasets): this series is planned but not yet sourced. It is
    meant to exercise trading-day regressors (X-13
    ``regression{ variables = td }``), which need a series sensitive to
    the number of trading days per month/quarter. Source it the same way
    as the other FRED-derived series -- a pinned Internet Archive Wayback
    Machine capture of the FRED CSV, following the pattern in
    ``tests/corpus/data/fetch_fred.py`` -- then add the resulting
    ``retail_sales.dat`` + README to ``tests/corpus/data/``, copy it into
    ``src/x13cpp/datasets/data/``, and replace this stub with a real
    loader matching :func:`load_airline` et al.
    """
    raise NotImplementedError(
        "load_retail_sales() is a placeholder: no retail-sales series has "
        "been sourced yet. See the TODO in this function's docstring."
    )


__all__ = [
    "TimeSeries",
    "load_airline",
    "load_expgs",
    "load_payems",
    "load_unrate",
    "load_shoe_sales",
    "load_retail_sales",
]
