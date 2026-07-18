"""Parser for X-13 ``.out`` text output (80- or 132-column, paginated).

The ``.out`` file is Fortran fixed-width paginated text. The numeric X-11 /
SEATS tables have a very regular shape (from Fortran ``table.f`` / ``prtshd.f`` /
``prtcol.f`` / ``wrttbl.f``)::

     <form-feed?>        Title of run           PAGE  12, SERIES abc   <- page header

     D 11  Final seasonally adjusted data                             <- table id line
      From Jan1990 to Dec1995
      Observations       72
    ------------------------------------------------------------------
     Year   Jan     Feb    ...   Dec      Total                       <- column header
    ------------------------------------------------------------------
     1990  123.4   456.7   ...  789.0    9999.9                       <- data rows
     ...
       AVGE 111.1   ...                                               <- summary rows

This module turns that into a structured form while keeping every line it does
*not* recognise as a numeric-table cell in ``text_lines`` for a separate
whitespace-normalised text comparison.

Structured form
---------------
``ParsedOut.tables`` maps a normalised ``table_id`` (e.g. ``"d11"``, ``"f2.1"``)
to a :class:`Table`. ``Table.cells`` is the ``{(row_period, col): value}``
mapping the harness spec asks for, where ``row_period`` is the year (as ``str``)
or a summary-row label (``"AVGE"``, ``"TOTAL"``, ``"S.D."``, ...) and ``col`` is
the column label from the header row (month/quarter abbrev, ``"Total"``, ...) or
a 0-based index string when no header could be matched.

IMPORTANT FORMAT ASSUMPTIONS (verify against real binary output):
* The table-id line looks like ``<L> <n>[.<n>]  <Title>`` with the id in the
  first ~6 columns, 2+ spaces, then a descriptive title. We normalise by
  stripping internal spaces and lowercasing (``"D 11"`` -> ``"d11"``).
* Data rows start with a 4-digit year in [1000, 9999]; summary rows start with
  a known label. Everything after the leading token that parses as a float is a
  column value; parsing stops at the first non-float token.
* Page headers are detected by a leading form-feed (0x0C) OR a line containing
  both ``PAGE`` and ``SERIES``. These are dropped from the text comparison.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

FORM_FEED = "\x0c"

# Leading labels that mark an annual-summary row rather than a year row.
SUMMARY_LABELS = {
    "AVGE",
    "AVG",
    "TOTAL",
    "S.D.",
    "SD",
    "STD",
    "AVABS",
    "MEAN",
    "MIN",
    "MAX",
}

# A table-id line: id in first columns, then 2+ spaces, then a title word.
# Letter classes cover X-11 (A-G) and SEATS (S) family tables.
_TABLE_ID_RE = re.compile(
    r"^\s{0,3}"
    r"(?P<letter>[A-GS])"
    r"\s?"
    r"(?P<num>\d{1,2}(?:\.\d{1,2})?)"
    r"(?P<suffix>[A-Za-z]?)"
    r"\.?"
    r"\s{2,}"
    r"(?P<title>\S.*\S|\S)\s*$"
)

_FROM_RE = re.compile(r"^\s*From\s+.+\s+to\s+.+$", re.IGNORECASE)
_OBS_RE = re.compile(r"^\s*Observations\s+\d+\s*$", re.IGNORECASE)
_DASH_RE = re.compile(r"^\s*-{4,}\s*$")


def _to_float(token: str):
    token = token.strip()
    if token == "":
        return None
    try:
        return float(token)
    except ValueError:
        try:
            return float(token.replace("D", "E").replace("d", "e"))
        except ValueError:
            return None


@dataclass
class Table:
    table_id: str
    title: str = ""
    columns: List[str] = field(default_factory=list)
    cells: Dict[Tuple[str, str], float] = field(default_factory=dict)
    meta_lines: List[str] = field(default_factory=list)

    def get(self, row: str, col: str):
        return self.cells.get((row, col))


@dataclass
class ParsedOut:
    tables: Dict[str, Table] = field(default_factory=dict)
    # Non-table text lines (whitespace already preserved; normalise at compare).
    text_lines: List[str] = field(default_factory=list)

    def __contains__(self, table_id: str) -> bool:
        return table_id in self.tables

    def __getitem__(self, table_id: str) -> Table:
        return self.tables[table_id]


def _normalise_id(letter: str, num: str, suffix: str) -> str:
    return (letter + num + suffix).replace(" ", "").lower()


def _is_page_header(line: str) -> bool:
    if line.startswith(FORM_FEED):
        return True
    stripped = line.lstrip(FORM_FEED)
    return ("PAGE" in stripped and "SERIES" in stripped)


def _parse_column_header(line: str) -> Optional[List[str]]:
    """Return column labels from a header row, or None if it doesn't look like
    one. A header row contains the word 'Year' and several short labels."""
    tokens = line.split()
    if not tokens:
        return None
    low = [t.lower() for t in tokens]
    if "year" not in low:
        return None
    # Drop the leading 'Year' token; the remainder are column labels.
    idx = low.index("year")
    labels = tokens[idx + 1 :]
    return labels or None


def _parse_data_row(line: str):
    """Parse a data/summary row. Returns (row_key, [values]) or None."""
    tokens = line.split()
    if not tokens:
        return None
    head = tokens[0]
    rest = tokens[1:]

    row_key: Optional[str] = None
    if head.upper() in SUMMARY_LABELS:
        row_key = head.upper()
    else:
        head_val = _to_float(head)
        if head_val is not None and float(head_val).is_integer() and 1000 <= head_val <= 9999:
            row_key = str(int(head_val))
        else:
            return None

    values: List[float] = []
    for tok in rest:
        v = _to_float(tok)
        if v is None:
            break  # stop at first non-numeric trailing token
        values.append(v)
    if not values:
        return None
    return row_key, values


def parse_out(text: str) -> ParsedOut:
    """Parse the full text of a ``.out`` file into a :class:`ParsedOut`."""
    result = ParsedOut()
    current: Optional[Table] = None

    lines = text.splitlines()
    i = 0
    n = len(lines)
    while i < n:
        raw = lines[i]
        line = raw.rstrip("\r")

        # Page headers: dropped entirely.
        if _is_page_header(line):
            i += 1
            continue

        # Strip a leading form feed if present on a content line.
        if line.startswith(FORM_FEED):
            line = line.lstrip(FORM_FEED)

        stripped = line.strip()

        # New table-id line.
        m = _TABLE_ID_RE.match(line)
        if m and not _looks_like_data(line):
            table_id = _normalise_id(m.group("letter"), m.group("num"), m.group("suffix"))
            title = m.group("title").strip()
            # Disambiguate duplicate ids across iterations by suffixing.
            key = table_id
            dup = 1
            while key in result.tables:
                dup += 1
                key = f"{table_id}#{dup}"
            current = Table(table_id=key, title=title)
            result.tables[key] = current
            i += 1
            continue

        if current is not None:
            if _FROM_RE.match(line) or _OBS_RE.match(line):
                current.meta_lines.append(stripped)
                i += 1
                continue
            if _DASH_RE.match(line):
                i += 1
                continue
            cols = _parse_column_header(line)
            if cols is not None:
                # Column header can span or replace; keep the widest seen.
                if len(cols) >= len(current.columns):
                    current.columns = cols
                i += 1
                continue
            data = _parse_data_row(line)
            if data is not None:
                row_key, values = data
                for ci, val in enumerate(values):
                    if ci < len(current.columns):
                        col = current.columns[ci]
                    else:
                        col = str(ci)
                    current.cells[(row_key, col)] = val
                i += 1
                continue

        # Not part of any table body: keep for text comparison.
        if stripped:
            result.text_lines.append(stripped)
        i += 1

    return result


def _looks_like_data(line: str) -> bool:
    """Guard so a data row like ' 1990 ...' isn't mistaken for a table id."""
    tok = line.split()
    if not tok:
        return False
    v = _to_float(tok[0])
    return v is not None


def parse_out_file(path) -> ParsedOut:
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        return parse_out(fh.read())
