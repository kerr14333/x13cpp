#!/usr/bin/env python
"""Extract the exact Fortran source of the SEATS routines that PARFRA/MAK1 need
from oracle/fortran/ansub2.f, so a ref driver can be linked against the *oracle*
code to bless golden bits. Not part of the build -- a one-shot golden helper."""
import re
import sys

SRC = "oracle/fortran/ansub2.f"
OUT = "tools/ref_seatsfact_snip.f"

# Routines to pull (case-insensitive match against the Fortran definition line).
NEEDED = [
    "PARFRA", "CONVM", "CONJM", "MLTSOL",
    "MAK1", "SYMPOLY", "RPQ", "C02AEF", "C02AEZ",
    "X02AAF", "X02AJF", "X02ALF",
    "ROOTC", "SQROOTC",
    "grRoots", "getRoot", "getRootc", "closestRoot", "JoinRoot", "halfRoots",
    "MPBC", "CONJ",
]

def_re = re.compile(
    r"^\s+(?:(?:double\s+precision|real\*8|real|integer|complex\*16|logical)\s+)?"
    r"(?:subroutine|function)\s+([A-Za-z]\w*)",
    re.IGNORECASE,
)
end_re = re.compile(r"^\s+end(?:\s+(?:subroutine|function))?\s*$", re.IGNORECASE)

lines = open(SRC).read().splitlines()

# Split file into (name, start, end) routine blocks.
blocks = {}
i = 0
n = len(lines)
while i < n:
    m = def_re.match(lines[i])
    if m:
        name = m.group(1)
        j = i
        while j < n and not end_re.match(lines[j]):
            j += 1
        blocks.setdefault(name.lower(), (i, j))
        i = j + 1
    else:
        i += 1

# CONJ collides with CONJM/CONJUGATE-style names -- match exact.
out = []
for want in NEEDED:
    key = want.lower()
    if key not in blocks:
        sys.stderr.write("MISSING routine: %s\n" % want)
        sys.exit(1)
    s, e = blocks[key]
    out.append("C ==== %s (ansub2.f:%d-%d) ====" % (want, s + 1, e + 1))
    out.extend(lines[s:e + 1])
    out.append("")

open(OUT, "w").write("\n".join(out) + "\n")
sys.stderr.write("wrote %s (%d routines)\n" % (OUT, len(NEEDED)))
