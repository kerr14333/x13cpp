"""x13cpp -- Python interface to the X13cpp seasonal adjustment engine.

X13cpp is a C++17 port of the U.S. Census Bureau's X-13ARIMA-SEATS seasonal
adjustment program. This package is the Python binding layer over that
engine.

Design rules (see the parent repository, ``x13new``, for the full picture):

* **No automatic file output.** :func:`seasonal_adjust` never writes
  anything to disk. Everything it computes lives on the returned
  :class:`X13Result`; call ``result.write_outputs(path)`` yourself when you
  want files.
* **Bundled example series.** A handful of named datasets ship with the
  package under :mod:`x13cpp.datasets` for experimenting without your own
  data.

Status: the C++ estimation core this package wraps is still mid-port
(regARIMA estimation is implemented; the X-11 and SEATS decomposition paths
are not). :func:`seasonal_adjust` currently raises ``NotImplementedError``
with an explanatory message rather than returning incorrect numbers. The
rest of the public API -- the result object, its accessors, and the
datasets -- is intended to be final already.
"""

from __future__ import annotations

from x13cpp import datasets
from x13cpp._core import seasonal_adjust
from x13cpp._result import X13Result
from x13cpp._timeseries import TimeSeries

__all__ = ["seasonal_adjust", "X13Result", "TimeSeries", "datasets", "__version__"]

__version__ = "0.0.0.dev0"
