"""Find spec-file OPTIONS the Fortran readers store and this port never writes.

The shape (entry 104, 2026-08-10): `regression{trendtc=}` and
`{testalleaster=}` were both in `gt_regression`'s ARGDIC -- so the parser
ACCEPTED them, validated nothing, and fell into the generic
consume-and-discard tail. `Lttc` has live consumers in x11pt3/x11pt4/seatpr and
every port call site passed a hardcoded false: a different seasonal adjustment
at `OUTCOME: OK`.

Detector: inside each Fortran spec reader, every assignment to a COMMON member
(this codebase capitalises them and lowercases locals) is an option the reader
STORES. If nothing under `core/src/` ever writes the same field name, the port
parses that option and throws it away.

This is NOT the same sweep as `tests/parity/test_gtinpt_defaults.py`. That one
asks whether the port writes the Fortran's INITIAL value; this one asks whether
it writes the value the PARSER computes. A flag that defaults to false and is
only ever turned on by an unported argument passes that test and fails this one
-- which is exactly how both of entry 104's defects survived entry 101's audit.

DISPOSITIONS as of entry 104 -- do not re-derive, extend:
  * `Inptok` / `Havesp` / `Lmodel` / `Havreg` / `Notc` -- out-parameters and the
    `x11{title=}` line count. Not options; ignore.
  * everything reported out of `gtinpt.f` -- that file's assignments are the
    DEFAULTS block, which is entry 101's territory and has its own test.
  * `Chi2cv` (`getreg.f:455`) / `Tlimit` (`:470`) -- dropped, but their only
    readers are `chkchi.f` / `usraic.f`, which are unported and WALLED. Covered.
    Whoever ports those two lands these parse arms in the same commit.
  * `Xhlnln`, `Xelong` -- CLOSED in entry 105. `Xelong` was not merely dropped:
    six x11reg.cpp sites were reading `arima.elong`, the REGRESSION spec's copy
    of the same option. `Xhlnln` is parsed and `rgtdhl.f`'s guard transcribed
    with its body walled.
  * `Ladd1x`, `Cvxrdc`, `Cvxtyp`, `Thtapr` -- ALL CLOSED in entry 106, which
    empties board item 8. `Ladd1x` (`outliermethod=`) is idotlr's `Ladd1` and
    now reaches it; `Cvxtyp` (`defaultcritical=`) already had its reader and
    needed only the parse arm plus x12hdr's `x11irrcrtval` savelog key, without
    which its sole observable was unemitted; `Thtapr` (`x11{taper=}`) took
    `taper.f` into `sautco` with it. `Cvxrdc` (`almost=`) is the odd one: the
    option is UNREACHABLE in the oracle (CB-43, see tools/dict_overrun.py) and
    both of idotlr's almost-outlier re-scans are `.or.Lxreg -> GO TO 50`
    anyway, so it has no reachable consumer on the only path that passes it.

Run it after adding a dispatch arm and confirm the count MOVES -- same property
`walls.py` and `dup_transcription.py` have.
"""
import os
import re

ORACLE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                      'oracle', 'fortran')
CORE = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                    'core', 'src')
PORT = os.path.join(CORE, 'specparse')

# The spec-file readers gtinpt.f dispatches to, one per spec block.
READERS = ['getreg.f', 'getx11.f', 'gtxreg.f', 'getsea.f', 'getest.f',
           'getfct.f', 'gtsrs.f', 'gttrn.f', 'gtoutl.f', 'gtauto.f',
           'gtpick.f', 'gtslsp.f', 'gthist.f', 'gtfrce.f', 'gtchck.f',
           'gtspec.f', 'gtcomp.f', 'gtmdfl.f', 'gtinpt.f']

# Fixed-form Fortran: a comment is column 1 only. Stripping and testing for a
# leading 'C' eats every CALL line -- the trap entry 101 paid for.
ASSIGN = re.compile(r'^\s{6,}(?:IF\([^)]*\)\s*)?([A-Z][a-z][A-Za-z0-9]*)\s*=(?!=)')
CPP_WRITE = re.compile(r'\.([a-z_]\w*)\s*(?:\([^)]*\))?\s*=(?!=)')


def _fields(root):
    out = set()
    for d, _s, fs in os.walk(root):
        for f in fs:
            if f.endswith(('.cpp', '.hpp')):
                with open(os.path.join(d, f), encoding='utf-8',
                          errors='replace') as fh:
                    out.update(m.group(1) for m in CPP_WRITE.finditer(fh.read()))
    return out


def main():
    port_fields = _fields(PORT)
    engine_fields = _fields(CORE)

    missing = []
    for r in READERS:
        p = os.path.join(ORACLE, r)
        if not os.path.exists(p):
            continue
        with open(p, encoding='utf-8', errors='replace') as fh:
            lines = fh.read().split('\n')
        seen = {}
        for i, ln in enumerate(lines, 1):
            if ln[:1] in 'cC*!':
                continue
            m = ASSIGN.match(ln)
            if m:
                seen.setdefault(m.group(1).lower(), (m.group(1), i))
        for low, (name, i) in sorted(seen.items()):
            if low in port_fields:
                continue
            missing.append((r, name, i, 'ENGINE' if low in engine_fields
                            else 'NOWHERE'))

    nowhere = [m for m in missing if m[3] == 'NOWHERE']
    print('%d reader-stored fields with no write under specparse/\n'
          % len(missing))
    print('--- %d written NOWHERE in the engine (the dangerous half) ---'
          % len(nowhere))
    for r, name, i, _ in nowhere:
        print('  %-12s:%5d  %s' % (r, i, name))
    print('\n--- %d written elsewhere in the engine ---'
          % (len(missing) - len(nowhere)))
    for r, name, i, where in missing:
        if where == 'ENGINE':
            print('  %-12s:%5d  %s' % (r, i, name))


if __name__ == '__main__':
    main()
