# coverage.ps1 -- formal code-coverage run for the C++ core (gcov + gcovr).
#
# Builds an INSTRUMENTED tree in build-cov (--coverage, -O0), runs the unit
# (ctest) and parity (pytest, driving the harness exes) suites so every ported
# path is exercised, then aggregates with gcovr into coverage/ (txt summary +
# browsable HTML). Kept separate from the fast `build` dir so normal work is
# undisturbed. Serial by design -- concurrent .gcda writes to the same exe race.
#
# Usage:  powershell -ExecutionPolicy Bypass -File tools/coverage.ps1
param(
    [switch]$NoBuild,        # skip configure/build, just re-run tests + report
    [switch]$UnitOnly        # only ctest (fast); skip the pytest parity suite
)
$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $RepoRoot "build-cov"
$CovDir = Join-Path $RepoRoot "coverage"

$ToolchainBin = "C:\rtools44\x86_64-w64-mingw32.static.posix\bin"
if (-not (Test-Path $ToolchainBin)) { throw "rtools44 toolchain bin not found: $ToolchainBin" }
$env:PATH = "$ToolchainBin;$env:PATH"
$Gcov = Join-Path $ToolchainBin "gcov.exe"
# The system cmake (C:\Program Files\CMake, 3.14) is < the project's 3.16 minimum
# and fails a fresh configure; prefer the newer pip cmake.
$cmake = "C:\Users\cyg50\AppData\Roaming\Python\Python314\site-packages\cmake\data\bin\cmake.exe"
if (-not (Test-Path $cmake)) { $cmake = "C:\Program Files\CMake\bin\cmake.exe" }
if (-not (Test-Path $cmake)) { $cmake = "cmake" }

if (-not $NoBuild) {
    Write-Host "== configure (instrumented) ==" -ForegroundColor Cyan
    & $cmake -S $RepoRoot -B $Build -G "Unix Makefiles" `
        -DCMAKE_BUILD_TYPE=RelWithDebInfo -DX13_COVERAGE=ON
    if ($LASTEXITCODE -ne 0) { throw "configure failed" }
    Write-Host "== build ==" -ForegroundColor Cyan
    & $cmake --build $Build -j 4
    if ($LASTEXITCODE -ne 0) { throw "build failed" }
}

Write-Host "== unit tests (ctest) ==" -ForegroundColor Cyan
Push-Location $Build
& ctest --output-on-failure
Pop-Location

if (-not $UnitOnly) {
    Write-Host "== parity tests (pytest -> instrumented harness exes) ==" -ForegroundColor Cyan
    $env:X13_BIN_DIR = $Build
    & python -m pytest (Join-Path $RepoRoot "tests") -q
    Remove-Item Env:\X13_BIN_DIR -ErrorAction SilentlyContinue
}

Write-Host "== gcovr report ==" -ForegroundColor Cyan
New-Item -ItemType Directory -Force -Path $CovDir | Out-Null
# Focus on the ported source (core/src); exclude generated common-block headers
# and the unit-test files themselves.
# NB: -j 1 is required -- gcovr's parallel workers fail on Windows Python
# (CreateProcess WinError 2). gcov is on PATH via $ToolchainBin above.
& python -m gcovr --root $RepoRoot -j 1 `
    --gcov-executable "gcov" `
    --filter "core/src/" `
    --exclude ".*/gen/.*" `
    --print-summary `
    --txt (Join-Path $CovDir "summary.txt") `
    --html-details (Join-Path $CovDir "index.html") `
    $Build
Write-Host "coverage summary -> coverage/summary.txt ; browse coverage/index.html" -ForegroundColor Green
