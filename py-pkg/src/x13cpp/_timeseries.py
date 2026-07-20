"""A minimal, dependency-light time series container.

``x13cpp`` avoids a hard dependency on pandas (see the package README for
the rationale): a numpy array plus a `start`/`freq` pair is enough to
represent the kind of regularly-spaced monthly/quarterly economic series
X-13ARIMA-SEATS operates on, and it keeps the install footprint small. If
you do have pandas installed (or install ``x13cpp[pandas]``), call
:meth:`TimeSeries.to_pandas` to get a `pandas.Series` with a `PeriodIndex`
for convenient plotting/resampling/joining.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Tuple

import numpy as np


@dataclass
class TimeSeries:
    """A regularly-spaced time series: values plus start date and frequency.

    Attributes:
        values: 1-D array of observations, in chronological order.
        start: ``(year, period)`` of the first observation, e.g. ``(1949, 1)``
            for January 1949, or ``(1947, 1)`` for 1947 Q1. `period` is
            1-indexed within the year.
        freq: Number of periods per year (``12`` for monthly, ``4`` for
            quarterly).
        name: Short series identifier, e.g. ``"airline"``.
        source: One-line provenance note.
    """

    values: np.ndarray
    start: Tuple[int, int]
    freq: int
    name: str = ""
    source: str = ""

    def __post_init__(self) -> None:
        self.values = np.asarray(self.values, dtype=float)
        if self.values.ndim != 1:
            raise ValueError("TimeSeries.values must be 1-dimensional")
        if self.freq <= 0:
            raise ValueError("TimeSeries.freq must be a positive integer")
        year, period = self.start
        if not (1 <= period <= self.freq):
            raise ValueError(
                f"start period {period} is out of range for freq={self.freq}"
            )

    def __len__(self) -> int:
        return len(self.values)

    def __repr__(self) -> str:  # pragma: no cover - cosmetic
        y, p = self.start
        return (
            f"TimeSeries(name={self.name!r}, n={len(self)}, "
            f"start={y}.{p:02d}, freq={self.freq})"
        )

    def to_pandas(self):
        """Return this series as a `pandas.Series` with a `PeriodIndex`.

        Raises:
            ImportError: if pandas is not installed. Install it with
                ``pip install x13cpp[pandas]`` or plain ``pip install pandas``.
        """
        try:
            import pandas as pd
        except ImportError as exc:  # pragma: no cover - exercised only
            # without pandas installed
            raise ImportError(
                "to_pandas() requires pandas. Install it with "
                "`pip install x13cpp[pandas]` or `pip install pandas`."
            ) from exc

        freq_alias = {12: "M", 4: "Q", 1: "A"}.get(self.freq)
        year, period = self.start
        if freq_alias is not None:
            start_str = f"{year}-{period}" if self.freq != 1 else str(year)
            index = pd.period_range(start=start_str, periods=len(self), freq=freq_alias)
        else:
            # Unrecognized frequency: fall back to a plain integer index
            # rather than guessing at a period alias.
            index = range(len(self))
        return pd.Series(self.values, index=index, name=self.name or None)
