#!/usr/bin/env bash
# Build the X-13ARIMA-SEATS Fortran oracle binaries (dev-time parity reference).
#
# Two builds:
#   x13as_ascii_O0  -- golden reference: -O0, no FP contraction (deterministic)
#   x13as_ascii_O2  -- secondary build used to calibrate legitimate FP wiggle
#
# Windows: run from Git Bash with rtools44 installed (gfortran 13.3).
# Linux (rocky8/rocky9 containers): plain gfortran from the distro.
set -euo pipefail
cd "$(dirname "$0")/fortran"

# Windows/rtools44 toolchain + a writable TEMP (gfortran fails without one:
# "Cannot create temporary file in C:\WINDOWS\: Permission denied")
if [[ "${OS:-}" == "Windows_NT" ]]; then
  export PATH="/c/rtools44/x86_64-w64-mingw32.static.posix/bin:/c/rtools44/usr/bin:$PATH"
  export TMPDIR="${LOCALAPPDATA:-C:/Users/$USERNAME/AppData/Local}/Temp"
  export TEMP="$TMPDIR" TMP="$TMPDIR"
fi

JOBS="${JOBS:-8}"
COMMON_FLAGS="-ffp-contract=off -std=legacy -fno-automatic"

make -f makefile.gf -j"$JOBS" FFLAGS="-O0 $COMMON_FLAGS" LDFLAGS="" PROGRAM=x13as_ascii_O0
rm -f ./*.o
make -f makefile.gf -j"$JOBS" FFLAGS="-O2 $COMMON_FLAGS" LDFLAGS="" PROGRAM=x13as_ascii_O2
rm -f ./*.o

echo "Built:"
ls -la x13as_ascii_O0* x13as_ascii_O2*
