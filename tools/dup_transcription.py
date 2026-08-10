"""Find Fortran blocks this port transcribed TWICE.

Run: python tools/dup_transcription.py

The defect shape, from entry 101 and then again in entry 103: a block that has a
faithful transcription in one file ALSO exists as a private inline copy at one
call site, hardcoded to the case its author had. Fixing the original does not
fix the copy, and the copy is correct-looking code that no gate distinguishes
until a spec reaches the arm it dropped. `automd.cpp` had two of them in one
function -- editor.f:1151-1166 (trading day) and editor.f:1354-1452 (Easter) --
and both picked a different regressor than the oracle at OUTCOME: OK.

Signal: the same `<name>.f:<lo>[-<hi>]` citation in two or more .cpp files where
BOTH files also write >=2 of the same fields nearby. Citing a Fortran line from
two places is ordinary cross-reference; writing the same COMMON fields for it in
two places is a second implementation.

This is a LEAD generator, not a verdict. Two classes of false positive are known
and expected, so read each hit before acting:

  * the Fortran itself has parallel blocks. `automx.f:163-177` keeps its own
    design snapshot whose field set matches `restor.f:50-64` almost exactly, and
    the port mirrors both -- two transcriptions of two different routines.
  * one site is the gtinpt DEFAULT and the other is the parse arm that overrides
    it (see tests/parity/test_gtinpt_defaults.py). That pairing is correct by
    design.

What to look for instead: two sites citing the SAME range where one is a named
routine and the other is inline, and the inline one is missing an arm.
"""
import collections
import os
import re

ROOT = 'D:/code_projects/x13new/core/src'
CITE = re.compile(r'\b([a-z][a-z0-9_]{1,10})\.f:(\d+)(?:\s*-\s*(\d+))?')
ASSIGN = re.compile(r'\.([a-z_]\w*)\s*(?:\([^)]*\))?\s*=(?!=)')
WINDOW = 22

files = {}
for root, _d, fs in os.walk(ROOT):
    for f in fs:
        if f.endswith('.cpp'):
            p = os.path.join(root, f)
            rel = p.replace(os.sep, '/').split('core/src/')[1]
            files[rel] = open(p, encoding='utf-8', errors='replace').read().split('\n')

# (fortran file, lo, hi) -> {cppfile: (citation line, frozenset fields)}
sites = collections.defaultdict(dict)
for rel, lines in files.items():
    for i, line in enumerate(lines):
        for m in CITE.finditer(line):
            lo = int(m.group(2))
            hi = int(m.group(3)) if m.group(3) else lo
            fields = set()
            for j in range(i, min(i + WINDOW, len(lines))):
                for a in ASSIGN.finditer(lines[j]):
                    fields.add(a.group(1))
            if fields:
                key = (m.group(1), lo, hi)
                prev = sites[key].get(rel)
                if prev is None or len(fields) > len(prev[1]):
                    sites[key][rel] = (i + 1, frozenset(fields))

def overlaps(a, b):
    return not (a[2] < b[1] or b[2] < a[1])

keys = sorted(sites)
used = set()
found = []
for i, k in enumerate(keys):
    if k in used:
        continue
    cluster = [k]
    used.add(k)
    for k2 in keys[i + 1:]:
        if k2 in used or k2[0] != k[0]:
            continue
        if any(overlaps(k2, c) for c in cluster):
            cluster.append(k2)
            used.add(k2)
    merged = {}
    for c in cluster:
        for rel, (ln, fields) in sites[c].items():
            if rel not in merged or len(fields) > len(merged[rel][1]):
                merged[rel] = (ln, fields, c)
    if len(merged) < 2:
        continue
    rels = sorted(merged)
    best = None
    for a in range(len(rels)):
        for b in range(a + 1, len(rels)):
            shared = merged[rels[a]][1] & merged[rels[b]][1]
            if len(shared) >= 2 and (best is None or len(shared) > len(best[2])):
                best = (rels[a], rels[b], shared)
    if best:
        lo = min(c[1] for c in cluster)
        hi = max(c[2] for c in cluster)
        found.append((len(best[2]), k[0], lo, hi, best, merged))

found.sort(reverse=True, key=lambda x: x[0])
print(f'{len(found)} ranges written from 2+ files with >=2 shared fields\n')
for n, fname, lo, hi, best, merged in found:
    a, b, shared = best
    print(f'=== {fname}.f:{lo}-{hi}   shared fields ({n}): {sorted(shared)}')
    for rel in (a, b):
        ln, fields, c = merged[rel]
        print(f'    {rel}:{ln}   writes {sorted(fields)[:9]}')
    print()
