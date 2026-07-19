# build.ps1 -- one-command configure + build + test on Windows with rtools44.
#
# Why this script exists: rtools44's g++/gfortran link stage (collect2 -> ld)
# fails with "ld returned 9 exit status" -- even on a trivial program -- unless
# the toolchain's own bin directory is on PATH, because ld.exe cannot load its
# dependent DLLs otherwise. Static libs (ar) build fine without it, so the
# failure only shows up when linking executables (the unit tests, the CLIs).
# This wrapper prepends that bin dir, then drives CMake from PowerShell (which
# also sidesteps the msys-make TEMP scrubbing that breaks gfortran under Git Bash).
#
# Usage (from the repo root):
#   powershell -ExecutionPolicy Bypass -File tools/build.ps1            # build + test
#   powershell -ExecutionPolicy Bypass -File tools/build.ps1 -NoTest    # build only
#   powershell -ExecutionPolicy Bypass -File tools/build.ps1 -Target test_numeric

param(
    [string]$Target = "",       # empty = build all
    [switch]$NoTest,            # skip ctest
    [string]$BuildDir = "build"
)

$ErrorActionPreference = "Stop"

$ToolchainBin = "C:\rtools44\x86_64-w64-mingw32.static.posix\bin"
if (-not (Test-Path $ToolchainBin)) {
    throw "rtools44 toolchain bin not found: $ToolchainBin"
}
$env:PATH = "$ToolchainBin;$env:PATH"

# Resolve repo root as the parent of this script's directory.
$RepoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$BuildPath = Join-Path $RepoRoot $BuildDir

$cmake = "C:\Program Files\CMake\bin\cmake.exe"
$ctest = "C:\Program Files\CMake\bin\ctest.exe"

# Configure if the build dir has no cache yet.
if (-not (Test-Path (Join-Path $BuildPath "CMakeCache.txt"))) {
    Write-Host "== configuring ($BuildDir) ==" -ForegroundColor Cyan
    & $cmake -S $RepoRoot -B $BuildPath -G "Unix Makefiles" `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_C_COMPILER="$ToolchainBin/gcc.exe" `
        -DCMAKE_CXX_COMPILER="$ToolchainBin/g++.exe"
    if ($LASTEXITCODE -ne 0) { throw "configure failed" }
}

Write-Host "== building ==" -ForegroundColor Cyan
$buildArgs = @("--build", $BuildPath)
if ($Target -ne "") { $buildArgs += @("--target", $Target) }
& $cmake @buildArgs
if ($LASTEXITCODE -ne 0) { throw "build failed" }

if (-not $NoTest) {
    Write-Host "== testing ==" -ForegroundColor Cyan
    # This ctest (< 3.20) does not honor --test-dir; run from inside the build dir.
    Push-Location $BuildPath
    try {
        & $ctest --output-on-failure
        if ($LASTEXITCODE -ne 0) { throw "tests failed" }
    } finally {
        Pop-Location
    }
}

Write-Host "OK" -ForegroundColor Green
