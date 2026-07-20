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
python -m pytest tests/parity/test_m3_estimate.py -q
```
Python is `python` (3.14) — `python3` is shadowed by a Windows App alias and
fails "Permission denied". See the `parity-gate` skill for blessing goldens.

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
