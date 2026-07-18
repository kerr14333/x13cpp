"""Parser for X-13 ``save`` files (RDB / tab-separated tables).

Format (from Fortran ``savtbl.f``)::

    date<TAB><series>.<ext>
    ------<TAB>-----------------------
    <YYYYMM|YYYY><TAB><value>
    ...

* Exactly two header lines: a ``date``-prefixed label row and a dashed
  separator row. Some table variants (matrices, ACF tables) emit extra
  columns; we keep every numeric column but expose the first as the scalar
  value for the common single-series case.
* The date key is an integer ``YYYYMM`` for monthly / ``YYYYQ``-style
  ``100*year+period`` for other frequencies, or bare ``YYYY`` for annual
  series. We preserve it verbatim as the period key (string), so comparison
  is period-exact regardless of frequency.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict, List


# Sentinel the Fortran writes for missing/unavailable observations. Values at
# or above this magnitude are treated as "not set" rather than real data.
MISSING_SENTINEL = 1e15


@dataclass
class ParsedSave:
    """Structured contents of a single save file."""

    label: str = ""
    # period-key (str, verbatim date field) -> first numeric column
    values: Dict[str, float] = field(default_factory=dict)
    # period-key -> full list of numeric columns (>=1)
    rows: Dict[str, List[float]] = field(default_factory=dict)
    # header lines kept for provenance / text diffing
    header_lines: List[str] = field(default_factory=list)

    def __getitem__(self, period: str) -> float:
        return self.values[period]

    def __contains__(self, period: str) -> bool:
        return period in self.values

    def __len__(self) -> int:
        return len(self.values)

    def keys(self):
        return self.values.keys()

    def items(self):
        return self.values.items()


def _looks_like_header(line: str) -> bool:
    stripped = line.strip().lower()
    if stripped.startswith("date"):
        return True
    # dashed separator row: only dashes / tabs / spaces
    body = stripped.replace("-", "").replace("\t", "").strip()
    return stripped != "" and body == "" and "-" in stripped


def _to_float(token: str):
    token = token.strip()
    if token == "":
        return None
    try:
        return float(token)
    except ValueError:
        # Fortran can emit values like "1.0D+05"; normalise the exponent char.
        try:
            return float(token.replace("D", "E").replace("d", "e"))
        except ValueError:
            return None


def parse_save(text: str) -> ParsedSave:
    """Parse the text of a save file into a :class:`ParsedSave`."""
    result = ParsedSave()
    for raw in text.splitlines():
        line = raw.rstrip("\n").rstrip("\r")
        if line.strip() == "":
            continue
        # Header rows (label + dashed separator).
        if _looks_like_header(line):
            result.header_lines.append(line)
            if line.strip().lower().startswith("date"):
                parts = line.split("\t")
                if len(parts) >= 2:
                    result.label = parts[1].strip()
            continue

        # Data row: split on tabs first (canonical); fall back to whitespace
        # for tolerant parsing of hand-written fixtures.
        cols = line.split("\t")
        if len(cols) < 2:
            cols = line.split()
        if len(cols) < 2:
            continue
        period = cols[0].strip()
        nums = [v for v in (_to_float(c) for c in cols[1:]) if v is not None]
        if not nums:
            continue
        result.rows[period] = nums
        result.values[period] = nums[0]
    return result


def parse_save_file(path) -> ParsedSave:
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        return parse_save(fh.read())
