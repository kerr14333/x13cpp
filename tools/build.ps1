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

# Resolve a CMake >= 3.16 (the project's minimum). The system CMake under
# "C:\Program Files\CMake" is pinned at 3.14 on this machine, which fails to
# *configure* new targets; a newer pip-installed CMake usually sits under the
# user's Python Scripts dir. Probe known locations + PATH, pick the first that
# meets the minimum, and derive ctest from the same bin dir.
$minCMake = [version]"3.16"
$cmakeCandidates = @(
    (Get-Command cmake.exe -ErrorAction SilentlyContinue | ForEach-Object { $_.Source }),
    (Join-Path $env:APPDATA "Python\Python314\Scripts\cmake.exe"),
    "C:\Program Files\CMake\bin\cmake.exe"
) | Where-Object { $_ -and (Test-Path $_) } | Select-Object -Unique

$cmake = $null
foreach ($cand in $cmakeCandidates) {
    $verLine = & $cand --version 2>$null | Select-Object -First 1
    if ($verLine -match "(\d+\.\d+\.\d+)" -and ([version]$Matches[1] -ge $minCMake)) {
        $cmake = $cand
        break
    }
}
if (-not $cmake) { throw "no CMake >= $minCMake found (looked in: $($cmakeCandidates -join '; '))" }

$ctest = Join-Path (Split-Path -Parent $cmake) "ctest.exe"
if (-not (Test-Path $ctest)) { $ctest = "ctest" }  # fall back to PATH

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
