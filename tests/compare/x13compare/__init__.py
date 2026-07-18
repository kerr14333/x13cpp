"""x13compare -- parity comparison toolkit for the X13cpp project.

Parses the outputs of the X-13ARIMA-SEATS oracle binary (.out / save files /
.udg) into structured data and compares two "golden bundles" numerically with
per-table tolerance overrides.

Public surface
--------------
parse_out.parse_out(text)      -> ParsedOut
parse_save.parse_save(text)    -> dict[period, float]
parse_udg.parse_udg(text)      -> dict[key, value]
compare.compare_bundles(a, b)  -> CompareReport
"""

from . import parse_out, parse_save, parse_udg, compare  # noqa: F401

__all__ = ["parse_out", "parse_save", "parse_udg", "compare"]
__version__ = "0.1.0"
