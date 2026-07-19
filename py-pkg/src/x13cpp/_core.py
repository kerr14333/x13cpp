"""The main entry point: `seasonal_adjust()`.

TODO(core wiring): the C++ estimation engine this function calls into is
still mid-port (see the ``x13new`` repository: regARIMA estimation is
implemented, but the X-11 and SEATS decomposition paths are not). Once
``core/`` exposes a stable C ABI / pybind11-callable entry point for a full
run (regARIMA + X-11 or SEATS + diagnostics), replace the
``raise NotImplementedError`` below with the real call and populate
`udg`, `fct`, and `save` on the returned :class:`X13Result`. The rest of
this function's contract (argument names, the shape of the returned
object, the "never write files automatically" rule) is intended to be
final already.
"""

from __future__ import annotations

from typing import Optional

from x13cpp._result import X13Result
from x13cpp._timeseries import TimeSeries


def seasonal_adjust(
    x: TimeSeries,
    transform: str = "auto",
    arima: Optional[str] = None,
    x11: bool = False,
    seats: bool = False,
    forecast_periods: int = 0,
    **spec_kwargs,
) -> X13Result:
    """Run a seasonal adjustment.

    Runs the X-13ARIMA-SEATS pipeline (regARIMA modeling, X-11 or SEATS
    decomposition, diagnostics) over an input series and returns an
    :class:`X13Result`.

    This function never writes to disk. Call ``result.write_outputs(path)``
    explicitly if you want files.

    Args:
        x: The series to seasonally adjust.
        transform: Transformation to apply before modeling: ``"auto"``
            (choose automatically), ``"log"``, or ``"none"``.
        arima: ARIMA order specification. Not yet processed; reserved for
            the same syntax as an X-13 ``arima{ model = (p d q)(P D Q) }``
            spec, e.g. ``"(0 1 1)(0 1 1)"``.
        x11: If `True`, run the X-11 decomposition. Mutually exclusive
            with `seats`.
        seats: If `True`, run the SEATS decomposition. Mutually exclusive
            with `x11`.
        forecast_periods: Number of periods to forecast beyond the end of
            the series (0 disables forecasting).
        **spec_kwargs: Additional spec options, reserved for future use.

    Returns:
        An :class:`X13Result`.

    Raises:
        TypeError: if `x` is not a :class:`TimeSeries`.
        ValueError: if both `x11` and `seats` are requested.
        NotImplementedError: always, currently -- see the module docstring.
    """
    if not isinstance(x, TimeSeries):
        raise TypeError(
            "x must be a x13cpp TimeSeries (see x13cpp.datasets or "
            "construct one directly)."
        )
    if transform not in ("auto", "log", "none"):
        raise ValueError('transform must be one of "auto", "log", "none"')
    if x11 and seats:
        raise ValueError("Choose one of x11 or seats, not both.")

    # TODO(core wiring): call into the C++ core here once it exposes a
    # full regARIMA + X-11/SEATS run. See the module docstring for what's
    # already implemented on the C++ side (core/) vs. what's still
    # missing.
    raise NotImplementedError(
        "seasonal_adjust() is a stub: the X13cpp C++ core does not yet "
        "expose a full seasonal-adjustment run (X-11/SEATS decomposition "
        "is not implemented; regARIMA estimation alone is not sufficient "
        "for this entry point). This package's API (result object, "
        "accessors, datasets) is ready; only the core call is pending. "
        "See https://github.com/kerr14333/x13cpp for status."
    )
