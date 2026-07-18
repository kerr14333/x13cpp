#!/usr/bin/env python
"""Fetch the FRED base series for the X13cpp test corpus.

The corpus is a *frozen, hermetic snapshot* committed to git.  This script
documents exactly how the committed ``*.dat`` files were produced and can
regenerate them byte-for-byte.

Provenance / access note
------------------------
The canonical source for each series is the St. Louis Fed (FRED) CSV export:

    https://fred.stlouisfed.org/graph/fredgraph.csv?id=<SERIES>

Direct scripted access to ``fred.stlouisfed.org`` from the build environment
was blocked (TLS connection reset / timeout regardless of User-Agent), so the
data is retrieved from the Internet Archive Wayback Machine, which serves a
byte-identical capture of the same FRED CSV.  Each capture timestamp is pinned
below so re-running this script is deterministic.

FRED / Wayback data is U.S. Government work (BLS / BEA source data) and is in
the public domain.

Run:  python fetch_fred.py
"""

from __future__ import annotations

import gzip
import io
import os
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))

# series id -> (pinned Wayback capture timestamp, seasonal period, trim_start)
# trim_start: keep observations on/after this "YYYY-MM-DD" (None = full history)
SERIES = {
    "PAYEMS": ("20250909155823", 12, "2000-01-01"),
    "UNRATE": ("20250927033136", 12, None),
    "EXPGS":  ("20260718181652", 4,  None),
}

UA = "Mozilla/5.0 (Windows NT 10.0; Win64; x64)"


def fetch_csv(series_id: str, timestamp: str) -> str:
    """Return the raw FRED CSV text for series_id from a pinned Wayback capture."""
    url = (
        "https://web.archive.org/web/" + timestamp + "id_/"
        "https://fred.stlouisfed.org/graph/fredgraph.csv?id=" + series_id
    )
    req = urllib.request.Request(url, headers={"User-Agent": UA,
                                               "Accept-Encoding": "gzip"})
    with urllib.request.urlopen(req, timeout=90) as resp:
        raw = resp.read()
    if raw[:2] == b"\x1f\x8b":
        raw = gzip.decompress(raw)
    return raw.decode("utf-8", "replace")


def parse_rows(csv_text: str):
    """Yield (date_str, value_str) from a FRED CSV (header: observation_date,ID)."""
    lines = csv_text.strip().splitlines()
    for line in lines[1:]:
        line = line.strip()
        if not line:
            continue
        date, _, value = line.partition(",")
        yield date.strip(), value.strip()


def write_dat(series_id: str, timestamp: str, period: int, trim_start):
    csv_text = fetch_csv(series_id, timestamp)
    rows = [(d, v) for (d, v) in parse_rows(csv_text)]
    if trim_start is not None:
        rows = [(d, v) for (d, v) in rows if d >= trim_start]
    values = [v for (_, v) in rows]

    first = rows[0][0]
    last = rows[-1][0]
    out = os.path.join(HERE, series_id.lower() + ".dat")
    with open(out, "w", newline="\n") as fh:
        for v in values:
            fh.write(v + "\n")

    # X-13 start= uses YYYY.PP (month) or YYYY.Q (quarter, 1-4).
    y, m, _d = first.split("-")
    m = int(m)
    per = m if period == 12 else ((m - 1) // 3 + 1)
    print(f"{series_id:8s} n={len(values):5d}  {first} -> {last}  "
          f"start={y}.{per:02d} period={period}  ({out})")


def main():
    for sid, (ts, period, trim) in SERIES.items():
        write_dat(sid, ts, period, trim)


if __name__ == "__main__":
    main()
