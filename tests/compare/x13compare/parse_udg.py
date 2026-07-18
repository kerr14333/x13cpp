"""Parser for X-13 ``.udg`` diagnostics files.

The ``-s`` flag makes the binary emit a ``.udg`` file: an ASCII key/value
diagnostics database, colon-delimited. From the Fortran the emit formats are::

    FORMAT(a,': ',a)          ! string value
    FORMAT(a,': ',f12.6)      ! single float
    FORMAT(a,2e21.14)         ! (some keys) multiple floats

so a line is ``key: value`` with a single space after the colon in the common
case. Values are auto-typed:

* a single numeric token -> ``float``
* several numeric tokens  -> ``list[float]``
* anything else           -> ``str`` (whitespace-stripped)

Keys may themselves contain a colon only in the value portion; we split on the
*first* ``": "`` (colon+space) and fall back to the first bare colon.
"""

from __future__ import annotations

from typing import Dict, List, Union

Value = Union[float, List[float], str]


def _try_float(token: str):
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


def _auto_type(raw: str) -> Value:
    raw = raw.strip()
    if raw == "":
        return ""
    tokens = raw.split()
    floats = [_try_float(t) for t in tokens]
    if all(f is not None for f in floats) and floats:
        if len(floats) == 1:
            return floats[0]
        return floats  # type: ignore[return-value]
    return raw


def _split_key_value(line: str):
    # Prefer the canonical "key: value" (colon followed by whitespace).
    idx = line.find(": ")
    if idx != -1:
        return line[:idx], line[idx + 2 :]
    # Line ending exactly at a colon with empty value, e.g. "key:".
    if line.rstrip().endswith(":"):
        return line.rstrip()[:-1], ""
    # Fall back to first bare colon.
    idx = line.find(":")
    if idx != -1:
        return line[:idx], line[idx + 1 :]
    return None, None


def parse_udg(text: str) -> Dict[str, Value]:
    """Parse ``.udg`` text into an ordered dict of typed values.

    Duplicate keys keep the *last* occurrence (matching a plain DB load), but
    duplicates are rare in practice.
    """
    out: Dict[str, Value] = {}
    for raw in text.splitlines():
        line = raw.rstrip("\r\n")
        if line.strip() == "":
            continue
        key, val = _split_key_value(line)
        if key is None:
            # Not a key/value line; skip silently (comment / banner).
            continue
        out[key.strip()] = _auto_type(val)
    return out


def parse_udg_file(path) -> Dict[str, Value]:
    with open(path, "r", encoding="utf-8", errors="replace") as fh:
        return parse_udg(fh.read())
