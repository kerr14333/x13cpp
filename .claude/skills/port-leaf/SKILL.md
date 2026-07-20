---
name: port-leaf
description: Port one Census Fortran subroutine (a "leaf") from oracle/fortran/*.f into the C++ core with bit-exact parity conventions. Use when porting the next X-13 routine (X-11 x11pt* spine, SEATS PARFRA/MAK1/SECOND, adequacy stage, aictest siblings, etc.) — it carries the farray/indexing/DATA/bug conventions that must match the oracle to the last bit. NOT for greenfield C++; this is Fortran→C++ transcription for parity.
---

# Port a Fortran leaf

Transcribe one `oracle/fortran/<name>.f` into the C++ core so it reproduces the
oracle **to the last bit**. Bit parity is the contract — port faithfully, do not
improve.

## Loop

1. **Read the leaf + its caller.** `cat oracle/fortran/<name>.f`. Then find every
   call site in the Fortran that invokes it (`grep -n "<name>\b" oracle/fortran/<caller>.f`)
   — the caller tells you the real argument state (which flags are set, whether it
   loops over columns, where `ndays`/indices come from). The `.f` header comments
   document the arg types; read them.
2. **Find the C++ home + template.** Same subsystem dir under `core/src/`
   (`regarima/`, `automdl/`, `x11/`, `seats/`, `numeric/`). Find an already-ported
   sibling as the template — e.g. `core/src/regarima/adestr.cpp` is the model for
   holiday/column builders; `adhol.cpp` (adlabr/adthnk) was cloned from it.
3. **Mirror the code** (conventions below). Keep the Fortran structure and names
   recognizable — a reviewer diffs your C++ against the `.f`.
4. **Declare** in the subsystem `.hpp` (next to the template's declaration).
5. **Wire the caller** — replace the `not_ported(ctx, "...")` stub in the C++
   caller (`regvar.cpp` case NNN, etc.) with the real call. Match the caller's
   Fortran arg-derivation exactly (how it computes `ndays`, `begcol`, flags).
6. **Build + gate** — hand off to the `build-run` then `parity-gate` skills.

## Parity conventions (these bite every time)

- **farrays are 1-based.** Fortran `A(i)` → keep the 1-based index and subtract 1
  only at the raw buffer access. For a Fortran 2-D `Xy(Ncxy,Nrxy)` (column-major),
  element `(c,r)` is:
  ```cpp
  auto XY = [&](int c, int r) -> double& {
      return xy[static_cast<std::size_t>((r - 1) * ncxy + (c - 1))];
  };
  ```
- **`DATA` tables → `static const` arrays.** Transcribe the values verbatim and
  **count the length** against the Fortran `DIMENSION`. A 2-D `DATA` like
  `means(25,8:9)` becomes `static const double means[25][2]`, column 0 = first
  index (period 8). Fortran fills column-major; lay the C++ out to match the
  access pattern, not the source line wrapping.
- **`**` (Fortran power) → `std::pow`** is a **last-ULP risk** vs gfortran's `**`.
  If the result gates a convergence test or a reported value, note it in
  `tools/FABLE_REVIEW.md`. Integer powers → prefer exact (`ldexp` for `2**k`).
- **Date/period helpers** are already ported: `addate(begdat, isp, off, out)`,
  and `prm::YR`/`prm::MO` (1-based) via `using namespace prm;`. Leap-year test:
  `(year%100!=0 && year%4==0) || year%400==0`.
- **Ported bugs stay bugs.** If the `.f` has a defect (off-by-one, typo'd
  constant, dead store), reproduce it verbatim with a code comment and log it to
  `tools/census_bugs.md` (CB-N entry, pinned to a test). Never silently "fix".
- **`rgarma` clobbers `ctx.series.tsrs`** with residuals during estimation. If
  your routine reads the series AND an estimation runs between, give it its own
  copy (this bit iddiff/amdid).
- **Deferred printing.** All WRITE/output is deferred in this port; a leaf
  computes into buffers, it does not print.

## Build integration

`CMakeLists.txt` uses `GLOB_RECURSE` over `core/src/*.cpp` — a **new file is
picked up automatically**, no CMake edit. First build after adding a file prints
"GLOB mismatch!" and stops; just rerun the build once (see `build-run`).

## When done

- Move the item from FABLE_REVIEW "Found gaps" / open queue to verified, with the
  gated spec name.
- Commit message: `M<n>/<subsystem>: port <name> (<what>)`, list the gated specs,
  end with the required Co-Authored-By + Claude-Session trailers.
