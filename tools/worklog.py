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
from datetime import datetime, timezone


def commits():
    out = subprocess.run(
        ["git", "log", "--format=%cI\t%h\t%s", "--reverse"],
        capture_output=True, text=True, check=True).stdout.strip().splitlines()
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
