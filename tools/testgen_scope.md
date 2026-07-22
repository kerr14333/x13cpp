# Test-gen scope: `force{}`, `slidingspans{}`, `history{}` parity coverage

Research + one proof for gating the unported x11pt3 Part-E / diagnostics work
in `core/src/x11/x11parts.cpp` (~24 `x11_not_ported` stubs). Corpus is at
`D:\code_projects\x13new\tests\corpus\`, goldens at `D:\code_projects\x13new\tests\golden\`.

## 1. Oracle-run loop: verified working

`oracle/run_oracle.py`'s header called its invocation an unverified assumption.
Confirmed against the real binary:

```
python oracle\run_oracle.py --binary oracle\fortran\x13as_ascii_O2.exe \
    --spec tests\corpus\generated\airline_automdl-x11.spc \
    --outdir <bundle> --flags=-s
```

produces `<binary> airline_automdl-x11 -s` run with `cwd=<bundle>` (spec copied
in, data file copied in, `file=` refs rewritten to basenames) -- exactly what
the docstring assumed. Its `.d11` is **byte-identical** to the committed
`tests/golden/generated/airline_automdl-x11/airline_automdl-x11.d11`
(`Compare-Object` empty diff). **No code fix needed in `run_oracle.py`.**

One CLI usability wrinkle (not a bug in the invocation logic): `argparse`
chokes on `--flags "-s"` (space-separated) because `-s` looks like an
unrecognized option -- `error: argument --flags: expected one argument`. Use
`--flags=-s` (equals form) from the command line. This doesn't affect
`run_parity.py` or any library caller, which pass `flags` as a `List[str]`
directly to `run_oracle()`, bypassing argparse entirely.

Also confirmed the two vendored oracle builds agree bit-for-bit on a force
spec (`x13as_ascii_O0.exe` vs `x13as_ascii_O2.exe` -> identical `.saa`/`.ffc`),
so goldens are build-independent for this feature.

## 2. Spec syntax (from the oracle Fortran parsers)

All three are **top-level specs**, not sub-arguments of `x11{}`. Confirmed via
`tbllog.i`'s internal spec-name table (`FRC`, `SSP`, internally-named `REV`
for `history`) and their argument parsers.

### `force{}` -- `oracle/fortran/getfrc.f`
```
force{
  type      = none | denton | regress     (default inferred from other args)
  round     = yes | no                    (default no)
  target    = original | calendaradj | permprioradj | both
  start     = <month/quarter name>        (needs series period set)
  lambda    = -3..3                       (regress method only, default 0)
  rho       = 0 < rho <= 1                (regress method only, default 0.9
                                            monthly / 0.9^(12/sp) quarterly)
  mode      = ratio | diff
  indforce  = yes | no
  usefcst   = yes | no
  print = all | (...)
  save  = (...)
}
```
`type=denton` -> `Iyrt=1` (qmap.f Denton benchmarking). `type=regress` ->
`Iyrt=2` (qmap2.f Cholette-Dagum regression benchmarking). This is exactly the
`frc.iyrt` field gating `core/src/x11/x11parts.cpp`'s
`x11_not_ported(ctx, "x11pt3 force yearly totals (qmap/qmap2)")` stub at
line 741 (`if (frc.iyrt > 0) { x11_not_ported(...); return; }`).

### `slidingspans{}` -- `oracle/fortran/getssp.f`
```
slidingspans{
  start        = <date>
  length       = <int, >= 3 years, <= MXYR years>
  numspans     = <int, 2..MXCOL>          (the "spans" argument keyword)
  cutseas      = <float > 0>              (default varies)
  cutchng      = <float > 0>
  cuttd        = <float > 0>
  outlier      = no | keep | yes
  fixmdl       = no | clear | yes
  fixreg       = (td holiday user outlier)
  additivesa   = difference | percent
  transparent  = no | yes
  x11outlier   = no | yes
  fixx11reg    = no | yes
  print = all | (...)
  save  = (...)
  savelog = (...)                          (NOT "all" -- see below)
}
```
Sets `Issap=1` (`hid.issap`/`frc.issap`? -- ported field is `hid.issap`),
which is what `x11pt2.f`/`x11pt3.f` check via `Issap.eq.2` (the "2" happens
during the hidden re-run passes the sliding-spans driver performs, not from
the raw spec parse -- `Issap=1` after parsing, bumped to 2 internally when the
sliding-spans driver replays x11 for each sub-span). The relevant stubs:
`x11pt2.f:302` ("x11pt2 sliding-spans trading-day capture (ssrit)"),
`x11pt3.f:624` ("x11pt3 sliding-spans seasonal store (ssrit)"),
`x11pt3.f:730` ("x11pt3 sliding-spans SA store (ssrit)").

### `history{}` (internal name `REV`, revisions history) -- `oracle/fortran/gtrvst.f`
```
history{
  estimates    = (sadj seasonal sadjchng aic fcst trend trendchng arma td)
  sadjlags     = (...)
  trendlags    = (...)
  fstep        = (...)
  start        = <date>
  endtable     = <date>
  fixmdl       = no | yes
  transparent  = no | yes
  refresh      = no | yes
  outlier      = remove | keep | auto
  outlierwin   = <int >= 0>
  target       = concurrent | final
  fixreg       = (td holiday user outlier)
  x11outlier   = no | yes
  fixx11reg    = no | yes
  additivesa   = difference | percent
  transformfcst = no | yes
  print = all | (...)
  save  = (...)
  savelog = all | (...)                    (history IS in the 'all' set)
}
```
Sets `hid.irev=1` after parsing (bumped to 4 internally during the hidden
re-estimation passes, same pattern as sliding-spans' `Issap`). Relevant stubs:
`x11pt3.f:628` ("x11pt3 revisions seasonal store (getrev)"),
`x11pt3.f:734` ("x11pt3 revisions SA store (getrev)"),
`x11pt3.f:760` ("x11pt3 revisions trend store (getrev)").

## 3. Save-table tags (decoded from `stable.prm`/`stable.var` + `tbllog.i`)

`getsav.f` addresses a per-spec slice of one of four concatenated dictionary
strings (`TB1DIC`..`TB4DIC`) using `tbllog.i`'s per-spec `(LSP*, NSP*)` base/
count pairs; entries alternate (long-name, short-tag) per table. I wrote a
one-off decoder (`stable.prm`/`stable.var` regex-parsed, ~90 lines) to walk
this mechanically instead of hand-counting dictionary offsets -- **cross-
validated**: decoding `x11`'s own range reproduced `genspecs.py`'s existing
73-tag `SAVE["x11"]` list exactly, and decoding `force`/`slidingspans`/
`history` reproduced `genextra.py`'s existing (narrower) `SAVE["force"]`,
`SAVE["sspans"]`, `SAVE["history"]` lists exactly. The decoder is not
committed (throwaway, scratchpad-only); rerun by pointing the method in this
section's description at `stable.prm`'s `PARAMETER(TBnDIC=...)` blocks and
`stable.var`'s `DATA tbNptr/.../` arrays if the dictionary ever needs
re-verification (e.g. after an oracle Fortran table-dictionary edit).

| spec | `tbllog.i` base | table dict | tag | table # | description | notes |
|---|---|---|---|---|---|---|
| force (`LSPFRC=208`, `NSPFRC=9`) | TB2DIC | 209 | `saa` | seasadjtot | D11A forced SA series; always written when `Iyrt>0` |
| | | 210 | `rnd` | saround | rounded forced SA; **only when `round=yes`** |
| | | 211 | `e6a` | revsachanges | forced-SA period diff vs. prior forced SA |
| | | 212 | `p6a` | revsachangespct | same, percent |
| | | 213 | `e6r` | rndsachanges | rounded-SA period diff |
| | | 214 | `p6r` | rndsachangespct | same, percent |
| | | 215 | `cr` | cratio | Cholette-Dagum correction ratio; **`type=regress` only** (`qmap2.f`) |
| | | 216 | `rr` | rratio | same, second ratio series |
| | | 217 | `ffc` | forcefactor | `Stci/Stci2` per-obs forcing factor |
| slidingspans (`LSPSSP=267`, `NSPSSP=21`) | TB3DIC (offset 0, `BRKDS2=267`) | 280 | `sfs` | sfspans | seasonal-factor spans |
| | | 281 | `sis` | indsfspans | indirect sf spans (composite) |
| | | 282 | `chs` | chngspans | month/qtr-to-month/qtr change spans |
| | | 283 | `cis` | indchngspans | indirect change spans |
| | | 284 | `ads` | saspans | SA spans |
| | | 285 | `ais` | indsaspans | indirect SA spans |
| | | 286 | `ycs` | ychngspans | year-over-year change spans |
| | | 287 | `yis` | indychngspans | indirect y/y change spans |
| | | 288 | `tds` | tdspans | trading-day spans |
| | (12 leading table slots 268-279 are zero-length/reserved -- not user-addressable) | | | | | |
| history/REV (`LSPREV=239`, `NSPREV=28`) | TB2DIC | 241 | `rot` | outlierhistory |  |
| | | 242 | `sfh` | sfilterhistory |  |
| | | 243 | `sar` | sarevisions | SA revisions |
| | | 245 | `sae` | saestimates | SA estimates |
| | | 246 | `chr` | chngrevisions |  |
| | | 248 | `che` | chngestimates |  |
| | | 249 | `iar` | indsarevisions | composite indirect SA |
| | | 251 | `iae` | indsaestimates |  |
| | | 252 | `trr` | trendrevisions |  |
| | | 254 | `tre` | trendestimates |  |
| | | 255 | `tcr` | trendchngrevisions |  |
| | | 257 | `tce` | trendchngestimates |  |
| | | 258 | `sfr` | sfrevisions |  |
| | | 260 | `sfe` | sfestimates |  |
| | | 261 | `lkh` | lkhdhistory | likelihood |
| | | 262 | `fce` | fcsterrors |  |
| | | 263 | `fch` | fcsthistory |  |
| | | 264 | `smh` | seatsmdlhistory | SEATS-model runs only |
| | | 265 | `ssh` | seasonalfcthistory |  |
| | | 266 | `amh` | armahistory |  |
| | | 267 | `tdh` | tdhistory |  |
| | (7 leading table slots 240/244/247/250/253/256/259 reserved -- not user-addressable) | | | | | |

**Important correction to a hunch I made mid-research and then disproved by
actually running the oracle**: I initially assumed force's `e6a`/`p6a`/`e6r`/
`p6r` were composite-only (gated by `qmap2.f`'s `Iagr` param). Running the
proof spec below showed **all 9 force tags populate on a single series** with
`type=regress round=yes` -- always trust the oracle run over static Fortran
reading when the two disagree.

## 4. The proof: `force{}` on `airline_automdl-x11`

Added `spec_force_automdl_x11()` to `tests/corpus/extra/genextra.py` (that
file already covers `history`/`slidingspans`/`force` denton+regress variants
from an earlier session, commit `a797b35` -- see &sect;5 for why those don't
close the gap). The new builder takes the **exact same `automdl-x11` config**
`genspecs.py` already emits as `tests/corpus/generated/airline_automdl-x11.spc`
(whose `b1`/`d10`-`d13` already pass the C++ `x13run_x11` gate, unxfailed) and
layers `force{ type=regress target=original round=yes rho=0.9 save=(all 9 tags) }`
on top:

```
tests/corpus/extra/airline_automdl-x11-force.spc
```

Ran it through the verified oracle-run loop against `x13as_ascii_O2.exe`,
blessing:

```
tests/golden/extra/airline_automdl-x11-force/   (manifest.json run_ok=true, exit=0)
```

`.saa` (D11A, forced seasonally-adjusted series with revised yearly totals),
first rows:
```
date	airline_automdl-.saa
------	-----------------------
194901	+0.124422856019168E+03
194902	+0.124494809419089E+03
194903	+0.124757250343753E+03
194904	+0.128938420027398E+03
194905	+0.125003071757468E+03
194906	+0.126641021263326E+03
```
All 9 requested tags (`saa rnd e6a p6a e6r p6r cr rr ffc`) came back non-empty;
only two mild spectral-peak WARNINGs in `.err`, no ERROR.

**C++ side confirmed FATAL**: `build\x13run_x11.exe tests\corpus\extra\airline_automdl-x11-force.spc`
prints `OUTCOME: FATAL` (exit 1). By elimination this is the `frc.iyrt > 0`
stub at `x11parts.cpp:741` -- every earlier not-ported guard in the D11/D13
tail (outlier/user re-adjustment, temp-constant removal, sliding-spans/
revisions stores) is false for this spec (no `outlier{}`, no user regressors,
no `slidingspans{}`/`history{}` block), and the same automdl-x11 base config
already clears `b1`/`d10`-`d13` in C++ without `force{}` attached (it's not
in `test_x11_tables.py`'s xfail list). I did **not** modify `tools/x13run_x11.cpp`
to dump the exact err-channel text and re-verify by string match, because that
file (and several other core/x11 files) currently has uncommitted in-flight
changes from a concurrent session -- touching it risked clobbering someone
else's work mid-edit. **Recommended follow-up** (cheap, ~3 lines, do it when
that file is quiescent): mirror `tools/x13parse.cpp:71-74`'s
`ctx.channels_.unit(ctx.units.mt2).str()` dump in `x13run_x11.cpp` on the
`OUTCOME: FATAL` path, so the harness prints the oracle-style `ERROR: ...`
line straight to stdout -- turns "FATAL by elimination" into "FATAL, and here
is the exact stub name" for every future xfail.

Confirmed the same FATAL outcome on the two other already-existing extra/
goldens too (`airline_slidingspans.spc`, `airline_history.spc`) -- the whole
family is currently gated, as expected.

### Side-effect caught and reverted
Running `python tests/corpus/extra/genextra.py` (its normal, documented,
idempotent regen step) deletes **every** `*.spc` in that directory first, then
rewrites only what's in its `BUILDERS` list. Two pre-existing committed specs,
`airline_automdl.spc` and `airline_iddiff.spc`, are **not** in `BUILDERS` (nor
in `MANIFEST`) -- they predate the current generator or were hand-added
outside it. Both regen runs during this session silently deleted them; both
times I `git checkout --` restored them before proceeding. This is a
pre-existing landmine in `genextra.py`, not something I introduced, but
whoever next touches that generator should either add builders for those two
specs or move them out of the directory `genextra.py` claims to own
exclusively.

## 5. Why this gap exists despite goldens already existing

`tests/corpus/extra/` and `tests/golden/extra/` already carry `force{}`
(denton + regress variants), `history{}`, and `slidingspans{}` (default +
cutseas variants) specs and goldens, committed in `a797b35`. **The real gap
is test wiring, not corpus/golden absence.** `tests/parity/test_x11_tables.py`
-- the only pytest file that runs `x13run_x11.exe` against goldens and diffs
numerically -- hard-codes `_CORPUS = tests/corpus/generated` and
`_GOLDEN = tests/golden/generated`, and its `_discover()` only accepts specs
shipping the `b1/d10/d11/d12/d13` tag set. It never walks `extra/`, so these
three features currently have **zero pytest coverage** even though the fixture
data has existed for a while. (`tests/parity/run_parity.py`'s generic
`discover_specs()` *does* walk the whole `tests/corpus/` tree including
`extra/`, but its `CppEngine.run()` is still an unimplemented placeholder --
`NotImplementedError` -- so it can't be used for the C++ side yet; all
existing C++ gates, including `test_x11_tables.py`, shell out to the
`build/x13run_*.exe` binaries directly instead of going through that engine
abstraction. The new tests should follow that same direct pattern.)

## 6. Rollout plan (NOT executed -- scope says prove one, not mass-generate)

### 6a. Extend `genspecs.py` for full 4-series x 8-config-ish systematic coverage
Add to `SAVE` (genspecs.py):
```python
"force": ["saa", "rnd", "e6a", "p6a", "e6r", "p6r", "cr", "rr", "ffc"],
"slidingspans": ["sfs", "sis", "chs", "cis", "ads", "ais", "ycs", "yis", "tds"],
"history": ["rot", "sfh", "sar", "sae", "chr", "che", "iar", "iae", "trr",
            "tre", "tcr", "tce", "sfr", "sfe", "lkh", "fce", "fch", "smh",
            "ssh", "amh", "tdh"],
```
`PRINT_SPECS` already lists all three (no change). `SAVELOG_ALL` already lists
`history` (no change); do **not** add `slidingspans` (its savelog range does
not include the `all` shortcut -- already correctly excluded).

Add 3 new `CONFIGS` entries building on `cfg_automdl_x11` (the config whose
`b1`/`d10`-`d13` already gate cleanly in C++, same reasoning as &sect;4):
```python
def cfg_force_x11(s):
    blocks = cfg_automdl_x11(s)
    blocks.append(spec("force",
        ["type = regress", "target = original", "round = yes", "rho = 0.9"],
        save_key="force"))
    return blocks

def cfg_slidingspans_x11(s):
    blocks = cfg_automdl_x11(s)
    blocks.append(spec("slidingspans", [], save_key="slidingspans"))
    return blocks

def cfg_history_x11(s):
    blocks = cfg_automdl_x11(s)
    blocks.append(spec("history",
        ["estimates = (sadj seasonal trend)"], save_key="history"))
    return blocks
```
Register as `("force-x11", cfg_force_x11)`, `("slidingspans-x11", cfg_slidingspans_x11)`,
`("history-x11", cfg_history_x11)` in `CONFIGS`. This produces 4 series x 3
new configs = 12 new specs; run each through the verified oracle loop to
bless goldens (`python run_parity.py --engine oracle --update --filter
"generated/*-x11" ...` or a targeted filter on the three new config suffixes).
`unrate`'s span note (`span=(1961.01,)`, POBS=780 cap) and unrate being a rate
series (no `transform{}`, `x11_block("add")` explicit mode) already flow
through `cfg_automdl_x11` unchanged, so no special-casing needed for the base;
only watch `history{start=...}` interacting with `unrate`'s span start (pick
a `start` well inside the span, e.g. the same relative offset used in
`airline_history.spc`'s `1955.jan`, scaled to unrate's post-1961 span).

### 6b. Wire parity tests (new files, xfailed from day one)
Mirror `test_x11_tables.py`'s structure exactly (same `_find_binary()`,
`_read_golden()` regex, `_discover()` glob-and-check-tag-files pattern, same
`rtol=1e-8` numeric compare) in three new files:

- `tests/parity/test_force_tables.py` -- tags `["saa", "rnd", "cr", "rr", "ffc"]`
  minimum (the always-populated set; add `e6a/p6a/e6r/p6r` once confirmed
  populated for every series, not just airline -- verify per-series before
  trusting, per &sect;3's "trust the run over the static read" lesson).
- `tests/parity/test_slidingspans_tables.py` -- tags
  `["sfs", "chs", "ads", "tds"]` (skip `sis/cis/ais/yis` -- indirect/composite
  variants, not reachable from these single-series specs; `ycs` needs >= 5
  years of spans depending on `length`, verify before including).
- `tests/parity/test_history_tables.py` -- tags `["sar", "sae", "trr", "tre",
  "sfr", "sfe"]` (the subset `genextra.py`'s existing `airline_history.spc`
  already requests and whose golden already exists -- extend once the other
  14 REV tags are verified per-series).

Every `test_*_table` function should **start `pytest.xfail(...)`
unconditionally** (like this doc's proof: `x13run_x11` returns `OUTCOME:
FATAL` on all of them today), citing the specific `x11_not_ported` message
each hits (from &sect;2's stub-to-line mapping) -- e.g.:
```python
def test_force_table(base, tag):
    pytest.xfail("x11pt3 force yearly totals (qmap/qmap2) not ported "
                 "(x11parts.cpp:741)")
    ...  # same shape as test_x11_tables.py otherwise, ready to un-xfail
```
This keeps the suite green (433 passed / 9 skipped / 84 xfailed baseline
grows by exactly the new xfail count, no new failures) while giving the
x11pt3 Part-E porting work concrete, numerically-precise targets to flip
green one stub at a time -- e.g. porting `qmap`/`qmap2` first turns
`test_force_table[...-saa]` / `[...-ffc]` / `[...-cr]` / `[...-rr]` green
without needing `ssrit`/`getrev` at all, since `frc.iyrt>0` is checked and
returns *before* the sliding-spans/history store checks later in the same
function (see `x11parts.cpp` lines 730/734 vs. 741 -- force's stub is
reached first for a spec that has none of `slidingspans{}`/`history{}`
attached, so it's a strictly smaller/earlier unlock than the other two).

### 6c. Order of work this unlocks (for whoever picks up x11pt3 Part-E)
1. `qmap`/`qmap2` (Denton + Cholette-Dagum benchmarking) -- closes `force{}`,
   the smallest of the three (no hidden re-run passes, just one more
   benchmarking pass over the final D11).
2. `ssrit` (sliding-spans storage, called from 3 sites: `x11pt2.f:302`,
   `x11pt3.f:624`, `x11pt3.f:730`) -- closes `slidingspans{}`; needs the
   hidden re-run driver (`sspdrv.f`/`ssap.f`) that replays x11 per sub-span,
   which is a bigger lift than force.
3. `getrev` (revisions-history storage, 3 sites: `x11pt3.f:628/734/760`) --
   closes `history{}`; needs the hidden re-estimation driver (`revdrv.f`)
   replaying the model+x11 per vintage, comparable size to sliding-spans.

Force is the cheapest first unlock and should go first; it's also the one
this session proved end-to-end.
