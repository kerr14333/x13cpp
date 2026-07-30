#!/usr/bin/env python
"""Project time tracker for the X13cpp proof-of-concept.

Reports wall-clock elapsed since the first commit, plus an "active" estimate
that sums inter-commit gaps up to a cap (so long idle breaks don't inflate the
number). Source of truth = git commit timestamps; nothing to maintain by hand.

    python tools/worklog.py            # summary
    python tools/worklog.py --gap 60   # treat >60 min between commits as a break

This exists because the project is a PoC: "can Claude recode Census
X-13ARIMA-SEATS from Fortran to C++?" — dev time is the headline metric.
"""
from __future__ import annotations

import argparse
import subprocess
import sys
from datetime import datetime, timezone

# This tool PRINTS commit subjects, which are arbitrary text -- this repo's are
# full of em dashes and at least one carries U+2016. When stdout is a pipe
# (metrics.py runs this as a subprocess; tools/build.ps1 does so from
# PowerShell) Python encodes it with the ambient codepage, cp1252 on Windows,
# and any such character raises UnicodeEncodeError. That crash is the ROOT of
# the metrics.py placeholder bug fixed in 184a8301: metrics swallowed the
# failure and reported `active_time: unknown` as though it were a measurement.
# Fixing it at the data end is not enough -- a future commit subject would
# reintroduce it -- so the OUTPUT STREAM is pinned instead.
for _stream in (sys.stdout, sys.stderr):
    try:
        _stream.reconfigure(encoding="utf-8", errors="replace")
    except (AttributeError, ValueError):      # not a reconfigurable stream
        pass


def commits():
    # encoding is PINNED. `text=True` alone decodes with
    # locale.getpreferredencoding() -- UTF-8 under this repo's Bash, cp1252
    # under PowerShell. This repo's commit subjects are full of em dashes, so
    # under PowerShell `git log` output hit bytes cp1252 cannot decode and this
    # raised UnicodeDecodeError. That is the ROOT of the metrics.py failure
    # fixed in 184a8301: metrics runs this file as a subprocess, so the crash
    # surfaced there as a placeholder rather than an error.
    out = subprocess.run(
        ["git", "log", "--format=%cI\t%h\t%s", "--reverse"],
        capture_output=True, text=True, encoding="utf-8", errors="replace",
        check=True).stdout.strip().splitlines()
    rows = []
    for ln in out:
        iso, h, subj = ln.split("\t", 2)
        rows.append((datetime.fromisoformat(iso), h, subj))
    return rows


def fmt_dur(secs: float) -> str:
    h, rem = divmod(int(secs), 3600)
    m = rem // 60
    return f"{h}h {m:02d}m"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--gap", type=float, default=45.0,
                    help="minutes between commits above which time is a break "
                         "(not counted as active); default 45")
    args = ap.parse_args()

    rows = commits()
    if not rows:
        print("no commits yet")
        return
    start, end = rows[0][0], rows[-1][0]
    now = datetime.now(timezone.utc)
    span = (end - start).total_seconds()
    since_start_now = (now - start).total_seconds()

    cap = args.gap * 60
    active = 0.0
    breaks = 0
    for (a, *_), (b, *_) in zip(rows, rows[1:]):
        gap = (b - a).total_seconds()
        if gap <= cap:
            active += gap
        else:
            breaks += 1

    print("X13cpp - proof-of-concept work log")
    print("=" * 44)
    print(f"start (first commit) : {start.astimezone():%Y-%m-%d %H:%M %Z}")
    print(f"latest commit        : {end.astimezone():%Y-%m-%d %H:%M %Z}")
    print(f"commits              : {len(rows)}")
    print(f"span, first->latest   : {fmt_dur(span)}")
    print(f"span, first->now      : {fmt_dur(since_start_now)}")
    print(f"active (gaps <={int(args.gap)}m)  : {fmt_dur(active)}   "
          f"[{breaks} break(s) excluded]")
    days = len({r[0].astimezone().date() for r in rows})
    print(f"calendar days worked : {days}")
    print()
    print("commit timeline:")
    for dt, h, subj in rows:
        print(f"  {dt.astimezone():%m-%d %H:%M}  {h}  {subj[:60]}")


if __name__ == "__main__":
    main()
