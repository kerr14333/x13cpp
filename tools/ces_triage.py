#!/usr/bin/env python
"""Run a generated CES corpus through BOTH the Fortran oracle and the C++ engine
and report, per spec, whether they agree.

Pairs with tools/ces_make_corpus.py. The point is a triage list, not a gate: it
answers "which of BLS's ~142 real production specs does the port already
reproduce, and where do the rest break?" so the next port target is chosen from
measurement instead of guesswork.

Each spec is run in its own scratch directory (the oracle writes output files
next to its input, and metafile runs write a shared .log), so runs are
independent and can go in parallel.

Verdicts:
  ok         -- every compared table agrees within --rtol
  drift      -- both ran, tables differ by more than --rtol
  engine-err -- the oracle ran, the engine fatalled or crashed
  oracle-err -- the oracle itself failed (spec/data problem, not a port bug)
  no-tables  -- both ran but the oracle emitted none of the compared tables

Usage::

    python tools/ces_triage.py --corpus <dir from ces_make_corpus.py> \\
        --oracle oracle/fortran/x13as_ascii_O2.exe --engine build/x13run_x11.exe \\
        [--composite build/x13run_composite.exe] [--jobs 8] [--json out.json]
"""
from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

GOLD_RE = re.compile(r"(\d{6})\s+([+\-][0-9.EeDd+\-]+)")
# Tables the CES specs save that the C++ harnesses also emit.
DIRECT_TAGS = ["d10", "d11", "d16"]
INDIRECT_TAGS = ["isf", "isa"]


def read_punch(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    try:
        fh = open(path, encoding="utf-8", errors="replace")
    except OSError:
        return out
    with fh:
        for ln in fh:
            m = GOLD_RE.match(ln.strip())
            if m:
                out[m.group(1)] = float(m.group(2).replace("D", "E").replace("d", "e"))
    return out


def read_emitted(stdout: str, tag: str) -> dict[str, float]:
    out: dict[str, float] = {}
    for ln in stdout.splitlines():
        p = ln.split()
        if len(p) == 3 and p[0] == tag:
            out[p[1]] = float(p[2])
    return out


def worst_rel(gold: dict[str, float], got: dict[str, float]) -> tuple[float, str | None, int]:
    keys = sorted(set(gold) & set(got))
    worst, at = 0.0, None
    for k in keys:
        g, v = gold[k], got[k]
        rel = abs(v - g) / abs(g) if g else abs(v - g)
        if rel > worst:
            worst, at = rel, k
    return worst, at, len(keys)


def run_one(job: dict, args: argparse.Namespace) -> dict:
    """Run one spec (or one metafile group) both ways in an isolated scratch dir."""
    name = job["name"]
    res: dict = {"name": name, "kind": job["kind"]}
    work = tempfile.mkdtemp(prefix="ces_")
    try:
        # Stage only what this job needs: its specs + the whole data dir.
        for base in job["specs"]:
            shutil.copy2(os.path.join(args.corpus, base + ".spc"),
                         os.path.join(work, base + ".spc"))
        if job["kind"] == "composite":
            shutil.copy2(os.path.join(args.corpus, name + ".mta"),
                         os.path.join(work, name + ".mta"))
        shutil.copytree(os.path.join(args.corpus, "data"), os.path.join(work, "data"))

        if job["kind"] == "composite":
            ocmd = [os.path.abspath(args.oracle), "-m", name, "-s"]
            ecmd = [os.path.abspath(args.composite), name + ".mta"]
            tags = DIRECT_TAGS + INDIRECT_TAGS
        else:
            ocmd = [os.path.abspath(args.oracle), name, "-s"]
            ecmd = [os.path.abspath(args.engine), name + ".spc"]
            tags = DIRECT_TAGS

        o = subprocess.run(ocmd, cwd=work, capture_output=True, text=True,
                           timeout=args.timeout)
        # The oracle reports failure through its .err file, not the exit code.
        errtxt = ""
        errpath = os.path.join(work, name + ".err")
        if os.path.exists(errpath):
            errtxt = open(errpath, errors="replace").read()
        if o.returncode != 0 or " ERROR" in errtxt:
            res["verdict"] = "oracle-err"
            res["detail"] = (errtxt.strip().splitlines() or [f"exit {o.returncode}"])[-1][:160]
            return res

        e = subprocess.run(ecmd, cwd=work, capture_output=True, text=True,
                           timeout=args.timeout)
        first = (e.stdout.splitlines() or [""])[0].strip()
        if e.returncode != 0 or first != "OUTCOME: OK":
            res["verdict"] = "engine-err"
            tail = (e.stderr.strip().splitlines() or e.stdout.strip().splitlines() or [""])
            msg = next((l for l in reversed(tail) if l.strip()), "")
            res["detail"] = f"rc={e.returncode} {first} | {msg.strip()[:140]}"
            return res

        worst_all, worst_tag, worst_at, compared = 0.0, None, None, 0
        per: dict[str, float] = {}
        for t in tags:
            gold = read_punch(os.path.join(work, f"{name}.{t}"))
            if not gold:
                continue
            got = read_emitted(e.stdout, t)
            if not got:
                per[t] = float("nan")
                continue
            w, at, n = worst_rel(gold, got)
            per[t] = w
            compared += 1
            if w > worst_all:
                worst_all, worst_tag, worst_at = w, t, at
        res["per_table"] = per
        if not compared:
            res["verdict"] = "no-tables"
            return res
        res["worst"] = worst_all
        res["worst_tag"] = worst_tag
        res["worst_at"] = worst_at
        res["verdict"] = "ok" if worst_all <= args.rtol else "drift"
        return res
    except subprocess.TimeoutExpired:
        res["verdict"] = "engine-err"
        res["detail"] = f"timeout after {args.timeout}s"
        return res
    except Exception as exc:                    # noqa: BLE001 - triage must not die
        res["verdict"] = "engine-err"
        res["detail"] = f"{type(exc).__name__}: {exc}"
        return res
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--corpus", required=True)
    ap.add_argument("--oracle", required=True)
    ap.add_argument("--engine", required=True, help="x13run_x11 (single-spec runs)")
    ap.add_argument("--composite", help="x13run_composite (metafile runs)")
    ap.add_argument("--rtol", type=float, default=1e-8)
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 4)
    ap.add_argument("--timeout", type=int, default=300)
    ap.add_argument("--json", help="write the full result list here")
    ap.add_argument("--filter", help="only run specs whose name contains this")
    args = ap.parse_args()

    specs = sorted(f[:-4] for f in os.listdir(args.corpus) if f.endswith(".spc"))
    mtas = sorted(f[:-4] for f in os.listdir(args.corpus) if f.endswith(".mta"))
    in_group: set[str] = set()
    jobs: list[dict] = []
    for m in mtas:
        members = [ln.strip() for ln in open(os.path.join(args.corpus, m + ".mta"))
                   if ln.strip()]
        in_group.update(members)
        if args.composite:
            jobs.append({"name": m, "kind": "composite", "specs": members})
    for s in specs:
        if s in in_group:
            continue
        jobs.append({"name": s, "kind": "direct", "specs": [s]})
    if args.filter:
        jobs = [j for j in jobs if args.filter in j["name"]]

    print(f"running {len(jobs)} jobs ({sum(1 for j in jobs if j['kind']=='composite')} "
          f"composite) with {args.jobs} workers, rtol={args.rtol:g}")
    results: list[dict] = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as ex:
        futs = {ex.submit(run_one, j, args): j for j in jobs}
        done = 0
        for fut in concurrent.futures.as_completed(futs):
            results.append(fut.result())
            done += 1
            if done % 20 == 0 or done == len(jobs):
                print(f"  {done}/{len(jobs)}")

    results.sort(key=lambda r: (r["verdict"] != "ok", -(r.get("worst") or 0), r["name"]))
    counts: dict[str, int] = {}
    for r in results:
        counts[r["verdict"]] = counts.get(r["verdict"], 0) + 1
    print("\n=== verdicts ===")
    for k in ("ok", "drift", "engine-err", "oracle-err", "no-tables"):
        if k in counts:
            print(f"  {k:11s} {counts[k]}")
    for kind in ("drift", "engine-err", "oracle-err", "no-tables"):
        rows = [r for r in results if r["verdict"] == kind]
        if not rows:
            continue
        print(f"\n=== {kind} ({len(rows)}) ===")
        for r in rows[:60]:
            if kind == "drift":
                print(f"  {r['name']:16s} worst={r['worst']:.2e} "
                      f"({r['worst_tag']} @ {r['worst_at']})")
            else:
                print(f"  {r['name']:16s} {r.get('detail','')}")
        if len(rows) > 60:
            print(f"  ... and {len(rows)-60} more")
    if args.json:
        json.dump(results, open(args.json, "w"), indent=1)
        print(f"\nfull results -> {args.json}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
