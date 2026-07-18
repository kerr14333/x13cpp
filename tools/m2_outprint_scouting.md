# M2 chunk 1 — `.out` table print engine: scouting map

Read-only scout of the shared `.out` text-table printer (the deferred M2 chunk 1).
Goal for the port: `.out` tables byte-identical to the oracle. This is a **text
formatting** parity problem, not a numeric one — the numbers already match
(a1/a2/a3/trn gate green); what must match now is column layout, decimal counts,
multi-row wrapping, and yearly totals.

## Entry point & call tree

`table(Z, Ib, Ie, Ktabl, Itype, Nop, Y, Tblptr)` — `oracle/fortran/table.f` (561 lines).
Writes one table to unit Mt1. Shared utility: called from ~20 sites (arima.f,
x11pt1-4.f, sigex/seatpr for SEATS, si.f, prtadj/prttrn, agr*, etc.), so porting
it unlocks `.out` output across ALL later milestones, not just M2.

Call tree (from `table.f` CALL sites):
- `getdes` (table.f:232) — look up the table's description/title from the dictionaries
- `tblhdr` (:256,:297) — `tblhdr(Ktabl,Itype,Ixreg,Nobs,Begtbl,Nny,Y,Tbltit)`, builds header block (214 lines; pulls units.cmn, x11reg/x11adj/x11msc, title.cmn)
- `getstr` (:263,:283,:363,:367) — extract a substring/field from a packed dictionary
- `cnvfmt` (:265,:285,:365,:369) — **the format compiler** (see below)
- `setchr` (:266) — set fill characters
- `prtcol` (:303,:502) — `prtcol(L,Nline,Tblcol,Tblwid,Ny,Mt1,Nop,Noplbl,Disp2,...)` — column-header line (78 lines)
- `addate` (:347,:479,:508) — advance a (year,period) date
- `makttl` (:471,:473) — assemble the title string
- `prtshd` (:485) — print sub-header
- `wrttbl` (:375,:394,:431,:544) — **the row writer** (180 lines; Dave Paletz 9/91), emits data rows + yearly total column, suppresses the Fortran short-year error

## The format machinery (parity-critical)

- **`tfmts.prm`** — `TFMDIC`, a packed CHARACTER*771 dictionary of **PTFM=24** base
  format templates, indexed by `tfmptr(0:24)`. Templates use two placeholder chars:
  - `@` = slot for the per-observation numeric descriptor
  - `#` = slot for the yearly-summary/total descriptor
  Example template: `(2x,i4,3x,3(3(1x,@):,/,9x),3(1x,@),4x,#)` — a year label `i4`,
  monthly values in groups with `/,9x` line-wrapping, and a `#` total column.
- **`cnvfmt(Base,Xfmt,Fobs,Fsum,Fpos,Nfmt)`** — `cnvfmt.f`. Walks the base template,
  replacing each `@` with the observation format `Fobs` and each `#` with the summary
  format `Fsum`, producing the final runtime FORMAT string `Xfmt`. `Fobs`/`Fsum` are
  F/E descriptors chosen from the table's decimal count (Kdec) and value magnitude.
- Emission then goes through Fortran FORMAT — **use the existing `fformat.hpp` engine**
  (already gfortran-validated for I/F/E/G/A/X/T/S/SP/SS). Do NOT hand-roll number
  formatting; feed the `cnvfmt`-built descriptor string to fformat.

## Dictionaries to vendor (packed CHARACTER + ptr arrays)

- `tbltitle.prm`, `desfct.prm`, `desfc2.prm` — table titles / descriptions (getdes/getstr).
- `table.prm` (134 lines) / `table.var` (94 lines) — table-code constants + the
  per-table variable metadata (type, decimals, width).
- `tfmts.prm` + `tfmts.cmn` — format templates + runtime format COMMON.
  Port these the same way as the existing generated dictionaries (packed string +
  integer pointer array, 1-based slicing via farray/fstring helpers).

## COMMON blocks needed in ctx

table.f: x11ptr, extend, tfmts, units (+ srslen/notset/tfmts/tbltitle/desfct/desfc2 prm).
tblhdr.f adds: x11reg, x11adj, x11msc, priusr, title, error.
wrttbl.f adds: error. Most already exist in X13Context (generated from the 87 .cmn).

## Suggested leaf-first port order

1. `getstr`, `getdes`, `setchr`, `addate` — small pure string/date helpers (leaves).
2. `cnvfmt` — format compiler; unit-test by comparing built FORMAT strings to a
   gfortran run of the template on sample descriptors.
3. `makttl`, `prtshd`, `prtcol` — header/label emitters.
4. `wrttbl` — row writer (depends on cnvfmt output + fformat).
5. `tblhdr` — full header block.
6. `table` — top driver; wire into run_m2 to emit the `.out` table sections that
   currently only produce save files.

## Parity risks (text, not FP)

- **Kdec / descriptor selection**: the F-vs-E choice and decimal width must match
  exactly — one digit off = whole-line diff. Trace how Fobs/Fsum are chosen per table.
- **Line wrapping**: the `:,/,9x` continuations in templates set where a row breaks
  across print lines; column count per line (3/4/6/8/12) is template-specific.
- **Yearly total column** (`#` slot) present only on some templates; wrttbl computes
  the total and suppresses the short-final-year error — replicate the suppression.
- **Trailing whitespace / blank-line placement** between title, header, and data —
  the comparator masks timestamps but NOT layout, so spacing must be exact.
- **`i4` year labels vs `a5` row labels** — the odd (2nd,4th,...) TFMDIC entries are
  the `a5`-label variants; pick the same index the Fortran picks (Nfmt/Itype).

## Collision note

`table.f` and `wrttbl.f` live in `core/src/tables/` territory (same dir as the
regression-matrix agent's savtbl work). Do NOT dispatch this concurrently with an
agent editing `core/src/tables/` or `run_m2` — sequence it AFTER the rmx chunk lands.
