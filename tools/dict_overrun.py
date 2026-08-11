"""Find dictionary entries the ORACLE can never match and this port can.

The shape (entry 106, 2026-08-11). Fortran pads a `CHARACTER*N` parameter with
blanks to its DECLARED length; C++ `string_view::substr` clamps at the end of
the literal. So when a dictionary's pointer table runs PAST the end of the
string, the Fortran's last entry carries trailing blanks -- and `cmpstr`
compares lengths exactly (cmpstr.f:11-12), so no input token can ever match it.
The port, transcribing the same literal without the padding, slices a SHORTER
substring, matches, and accepts an option the oracle refuses.

`gtxreg.f:59` is the live case: `CHARACTER ARGDIC*271` over a 269-character
literal, `argptr(36)=272`. The 36th and last entry is 'almost  ', so
`x11regression{almost=}` -- a documented option -- comes back
`Argument name "almost" not found` and halts the oracle, while this port ran it
to `OUTCOME: OK`. That is CB-43, and the fix is two trailing blanks.

The mirror defect is a pointer table transcribed WRONG rather than faithfully,
and this sweep finds it too: it reports any table whose last pointer exceeds the
literal, whichever side is at fault. Cross-check the survivors against the
Fortran's own `DATA` statement before concluding which.

**THE EXPECTED COUNT IS ZERO**, and that is not the same as "no findings".
A faithfully transcribed dictionary writes the literal at its DECLARED length,
padding included, so it does not overrun -- CB-43's ARGDIC is 271 characters
here and reports nothing. A hit therefore means one of exactly two things, and
the Fortran's own `DATA` statement says which:

  * the literal was transcribed unpadded (the port is now LOOSER than the
    oracle -- it will accept an argument the oracle refuses), or
  * the pointer table itself was mistyped (the port is now TIGHTER -- it will
    refuse an argument the oracle accepts).

Both were live when this was written (entry 106). ARGDIC in `gt_x11regression`
was the first: `x11regression{almost=}` ran to `OUTCOME: OK` where the oracle
halts. QDIC in `gt_check` was the second and the mirror image: the Fortran is
`DATA qptr/1,9,11,20,22/`, the port had {1,9,11,21,23}, so the last two entries
sliced as "boxpierceb" and "p" and `check{qtype=boxpierce}` FATALed against an
oracle that accepts it. No corpus spec had ever set qtype -- which is how a
one-character slip in a DATA statement survived. Gated by
`extra/airline_check-qtype-boxpierce`.

Run it after transcribing any dictionary and confirm the count is still 0 --
same property `walls.py`, `dup_transcription.py` and `parsed_dropped.py` have.
"""
import os
import re

CORE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                    'core', 'src')

LIT = re.compile(r'static const char (\w+)\[\]\s*=\s*((?:\s*"(?:[^"\\]|\\.)*")+)\s*;',
                 re.S)
PTR = re.compile(r'(?:static const )?int (\w+)\[[^\]]*\]\s*=\s*\{([^}]*)\}', re.S)


def main():
    hits = []
    for d, _s, fs in os.walk(CORE):
        for f in fs:
            if not f.endswith(('.cpp', '.hpp')):
                continue
            p = os.path.join(d, f)
            with open(p, encoding='utf-8', errors='replace') as fh:
                src = fh.read()
            ptrs = {}
            for m in PTR.finditer(src):
                try:
                    vals = [int(v.strip()) for v in m.group(2).split(',')
                            if v.strip()]
                except ValueError:
                    continue
                # a dictionary pointer table starts at 1 and is nondecreasing
                if vals and vals[0] == 1 and vals == sorted(vals):
                    ptrs[m.start()] = (m.group(1), vals)
            for m in LIT.finditer(src):
                text = ''.join(re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(2)))
                cands = [v for k, v in sorted(ptrs.items()) if k > m.start()]
                if not cands:
                    continue
                pname, vals = cands[0]
                if vals[-1] - 1 > len(text):
                    dead = [text[vals[i] - 1:vals[i + 1] - 1]
                            for i in range(len(vals) - 1)
                            if vals[i + 1] - 1 > len(text)]
                    hits.append((os.path.relpath(p, CORE),
                                 src[:m.start()].count('\n') + 1,
                                 m.group(1), pname, len(text), vals[-1] - 1,
                                 dead))
    print('%d dictionaries whose pointer table runs past the literal\n'
          % len(hits))
    for f, line, name, pname, n, last, dead in hits:
        print('  %s:%d  %s len=%d  %s[-1]-1=%d  entries past the end: %s'
              % (f, line, name, n, pname, last, dead))


if __name__ == '__main__':
    main()
