# Calling the engine from R and Python

These are **in-process bindings, not packages.** They exist so you can run
X-13ARIMA-SEATS from an interpreter and get result objects back, which is the
whole point of the port (see the project vision in `CLAUDE.md`). Packaging comes
later; nothing here needs `pip install` or `R CMD INSTALL`.

## Build

```powershell
& tools/build.ps1        # builds everything, including the `x13c` shared library
```

That produces `build/x13c.dll` (`libx13c.so` / `.dylib` elsewhere). Both loaders
find it automatically from a source checkout; set `X13C_LIBRARY` to override.

## Python

```python
import sys; sys.path.insert(0, "bindings/python")
import x13c

run = x13c.adjust("tests/corpus/generated/airline_x11-default.spc")
print(run)                       # <x13c.Run 144 obs, period=12, ...>

sa = run.table("d11")            # seasonally adjusted
for (year, period), value in zip(sa.dates[:3], sa.values[:3]):
    print(year, period, value)

print(run.table_names())         # what this run actually produced
print(run.diagnostics()["f3.q"]) # the Q quality statistic
```

`Run` is a context manager (`with x13c.adjust(...) as run:`) and frees its native
handle on `close()` or garbage collection.

## R

```r
source("bindings/r/x13c.R")

run <- x13_adjust("tests/corpus/generated/airline_x11-default.spc")
run                              # <x13 run: 144 obs, period=12, ...>

d11 <- x13_table(run, "d11")     # data.frame(year, period, value)
plot(as_ts(d11))

x13_table_names(run)
x13_diagnostics(run)[["f3.q"]]
x13_close(run)
```

## Things worth knowing before you rely on these

**Nothing is written to disk.** That is the project's standing rule and the
bindings keep it: a run produces values on a handle, never files.

**Tables do not share a start date.** `d10`'s seasonal factors are projected a
year past the data; backcasts precede it. Every table carries its own dates —
join on them, don't assume `d10[i]` and `d11[i]` are the same month.

**Runs are independent, but not concurrent.** Each handle owns its own copy of
the results. However, resolving a spec's relative data paths means changing the
process working directory for the duration of the run, so don't run two at once
from threads. Use the text entry point with absolute paths if you need to.

**The library pins its own floating-point mode.** The engine is validated
bit-exact against the Fortran oracle in a MinGW-built process, where the x87 unit
runs at 64-bit extended precision. R on Windows is also MinGW-built and matches;
CPython is MSVC-built and starts at 53-bit. That difference is invisible on
well-conditioned specs (~1e-16) but the optimizer amplifies it on the
near-non-invertible ones — measured **8.2e-6** in `d10` on
`unrate_sfshort-x11`, six orders past the 1e-6 estimation floor. Each run
therefore forces the validated mode and restores the host's afterwards. If you
port these bindings to another host, check `x13_host_fp_control()` first.

## Scope

Both decomposition paths. The binding parses the spec, sees which one it asks
for, and dispatches — so a run exposes one family or the other, never both.

* **X-11** (`x11{}`): `b1`, `d10`–`d13`, `d16`, the Part-E tables, the
  `force{}` / `x11regression{}` outputs (`saa`, `ffc`, `rnd`, `a4`,
  `sac`/`tac`), and the F2/F3 quality statistics as scalar diagnostics.
* **SEATS** (`seats{}`): `s10`–`s14`, `s16`, `s18`.

Not exposed yet: the `history{}` / `slidingspans{}` revision tables, the
composite/metafile aggregation (it spans several specs in one process, so it
needs a different handle shape), and forecasts.

## Tests

| what | how |
| --- | --- |
| C ABI contract (NULL safety, buffer protocol, FP restore) | `ctest -R test_capi` |
| Python binding vs. the oracle goldens, whole corpus | `python -m pytest tests/parity/test_bindings.py -q` |
| R binding vs. the oracle goldens + the R surface | `Rscript bindings/r/test_x13c.R` |

## Layout

| file | what it is |
| --- | --- |
| `core/include/x13/capi.h` | the flat C ABI — opaque handle, name-keyed tables |
| `core/src/api/x13_capi.cpp` | its implementation; the only place engine internals stop |
| `core/src/api/x13_rabi.cpp` | a `.C()`-shaped shim so R needs no R headers |
| `bindings/python/x13c.py` | `ctypes` loader |
| `bindings/r/x13c.R` | `dyn.load` loader |
