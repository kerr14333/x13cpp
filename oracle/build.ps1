# Build the X-13ARIMA-SEATS Fortran oracle binaries on Windows (rtools44 gfortran).
#
# Note: msys2 make/xargs scrub TEMP from native child processes, which makes
# gfortran fail with "Cannot create temporary file in C:\WINDOWS\". So on
# Windows we drive gfortran directly from PowerShell instead of using
# makefile.gf. Linux/CI uses build.sh (make works fine there).
#
# Produces:
#   x13as_ascii_O0.exe  -- golden reference (-O0, deterministic)
#   x13as_ascii_O2.exe  -- secondary build for FP-wiggle calibration
$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "fortran")
$env:Path = "C:\rtools44\x86_64-w64-mingw32.static.posix\bin;" + $env:Path

# Object list = ASCII-build members from makefile.gf (excludes HTML-variant files)
$objsLine = (Get-Content makefile.gf -Raw) -split "`n"
if (-not (Test-Path objs.txt)) {
    $inObjs = $false
    $objs = foreach ($line in $objsLine) {
        if ($line -match '^OBJS') { $inObjs = $true }
        elseif ($line -match '^SRCS') { $inObjs = $false }
        if ($inObjs) { [regex]::Matches($line, '[a-z0-9_]+\.o') | ForEach-Object { $_.Value } }
    }
    $objs | Sort-Object -Unique | Set-Content objs.txt -Encoding ascii
}
$files = (Get-Content objs.txt) -replace '\.o$', '.f'

foreach ($cfg in @(@{opt = "-O0"; out = "x13as_ascii_O0.exe" }, @{opt = "-O2"; out = "x13as_ascii_O2.exe" })) {
    Remove-Item *.o -Force -ErrorAction SilentlyContinue
    & gfortran $cfg.opt -ffp-contract=off -std=legacy -fno-automatic -c @files
    if ($LASTEXITCODE -ne 0) { throw "compile failed for $($cfg.out)" }
    & gfortran -o $cfg.out @((Get-Content objs.txt))
    if ($LASTEXITCODE -ne 0) { throw "link failed for $($cfg.out)" }
    Remove-Item *.o -Force
    Write-Host "built $($cfg.out)"
}
