---
name: build-run
description: Build the X13cpp port and run its tests on this Windows machine, plus produce code coverage. Use whenever you need to compile after an edit or run the unit/parity suite. Carries the environment quirks (execution-policy block, GLOB reconfigure, cmake version, coverage -j 1 / -O2) that otherwise waste turns.
---

# Build & run (Windows)

## Build

Use the **PowerShell tool** (not the Bash tool):
```powershell
& tools/build.ps1
```
- Do NOT invoke it from Bash as `powershell -File tools/build.ps1` — the machine's
  execution policy blocks `-File` script loading ("running scripts is disabled").
  The PowerShell tool's dot/call-operator invocation is allowed.
- `build.ps1` configures, builds, and runs the CTest unit suite; a clean run ends
  with `100% tests passed`.

### GLOB mismatch after adding a file
`CMakeLists.txt` globs `core/src/*.cpp` with `CONFIGURE_DEPENDS`. The first build
after you add a new `.cpp` prints `-- GLOB mismatch!` and stops. **Just rerun the
build once** — the reconfigure picks the file up. No CMake edit needed.

### cmake version
System cmake is 3.14 (too old for a fresh configure). The build script knows the
right one; if configuring by hand, use pip cmake 4.4.0 at
`C:\Users\cyg50\AppData\Roaming\Python\Python314\site-packages\cmake\data\bin\cmake.exe`.

## Unit tests

Run by the build, or directly via CTest in the build dir. Current suite:
`test_farray/fstring/channels/fformat/numeric/x11/x11b/seats`.

## Parity tests (pytest)

```bash
python -m pytest tests/parity -q -n 8          # full suite, ~75-80s
python -m pytest tests/parity -q -k "<name>"   # one gate, ~3s
```
Python is `python` (3.14) — `python3` is shadowed by a Windows App alias and
fails "Permission denied". See the `parity-gate` skill for blessing goldens.

### Run it in parallel — `-n 8` (pytest-xdist)
Measured 2026-07-28: serial **262s**, `-n 8` **79s**, `-n 16` **74s**, with
identical pass/skip counts at every level. Plateaus around 8 workers — the suite
is subprocess-spawn bound, not core bound (16 cores here), so `-n 8` is the
default and `-n 16` buys ~5s. Needs `python -m pip install pytest-xdist` (3.8.0
installed; not in any requirements file).

**Why parallel is safe here, which is the part worth re-checking if the harness
ever changes:** every gate shells out as `subprocess.run([BIN, spec])` and
compares **stdout**. The corpus specs are read-only and the harnesses write no
side files — verified by running a spec and confirming `git status
tests/corpus` is clean before and after. There is no shared state for workers
to race on. **If a harness ever starts round-tripping through save files, `-n`
becomes unsafe immediately** — several gates run the *same* spec
(`test_x11_tables`, `test_qs_diagnostics`, `test_check_diagnostics`,
`test_spectrum_peaks` all do), so they would race on a fixed output filename
and produce fast *wrong* answers.

When something fails, re-run that gate **serially** to read it — xdist reorders
and suppresses per-test output, and a single `-k` filter is ~3s anyway.

### Don't run the full suite on every edit
The build is ~25s (6.4s compile+archive, 18.8s to relink 12 downstream targets;
`-j 16` is no faster than `-j 6` — the links are I/O bound). The suite was the
expensive half. Filtered gate while iterating, full suite before commit, and
**skip it entirely for comment/doc-only edits.**

## Code coverage

```powershell
& tools/coverage.ps1
```
- Builds `build-cov` as **RelWithDebInfo (-O2)** with `X13_COVERAGE`, runs CTest +
  pytest (via `X13_BIN_DIR`), then `gcovr`.
- **Must be `-O0`-free**: `-O0` breaks the C02AEF (SEATS root) last-ULP goldens.
  Coverage builds at `-O2` deliberately.
- **gcovr needs `-j 1`** — parallel workers throw `FileNotFoundError (WinError 2)`
  on Windows. gcov (rtools44) must be on PATH.
- Last known: ~73% lines / ~90% functions.

## Toolchain pins
- gfortran / gcov: rtools44 at `C:\rtools44` (drive from PowerShell — msys make/xargs
  scrub `TEMP` and break gfortran).
- Oracle binaries: `oracle/fortran/x13as_ascii_O2.exe` (parity target) + `_O0`.
