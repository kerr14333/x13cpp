# X13cpp

A faithful C++ port of the U.S. Census Bureau's **X-13ARIMA-SEATS** seasonal
adjustment program (version 1.1, build 61), packaged as embeddable libraries for
**R** and **Python**.

> **Status: early development — milestone M0.** The scaffolding, oracle, and test
> corpus are being put in place. The C++ core is not yet implemented. APIs,
> layout, and behaviour will change.

## What this is

X-13ARIMA-SEATS is the world's standard tool for seasonal adjustment of economic
time series, distributed by the Census Bureau as Fortran source. X13cpp
re-implements that program in modern C++ so it can be embedded directly in data
pipelines and statistical environments, without shelling out to the Fortran
binary.

The overriding goal is **parity**: X13cpp is validated numerically against the
official Census Fortran build, which is vendored in this repository and treated
as the **oracle**. The target agreement tolerance is **1e-8** across the full
test corpus.

## Design goals

- **Faithful.** Match the Fortran program's numerical results and its diagnostic
  / error output, spec-for-spec.
- **Parity-tested.** Every result is compared against the vendored Fortran oracle
  (`oracle/fortran/`, Census X-13ARIMA-SEATS v1.1 b61) to a 1e-8 tolerance.
- **Portable.** C++17 only — no dependency on a bleeding-edge toolchain (see the
  platform matrix). Builds with stock GCC 8.5 on RHEL/Rocky 8.
- **Embeddable.** First-class R and Python bindings over one shared core.

## Planned architecture

| layer      | technology | directory  |
|------------|------------|------------|
| Core engine| **C++17**, no heavy third-party deps | `core/` |
| R package  | **Rcpp** wrapper over the core        | `r-pkg/` |
| Python pkg | **pybind11** wrapper over the core    | `py-pkg/` |
| Oracle     | Vendored Census **Fortran** (v1.1 b61)| `oracle/fortran/` |
| Tests      | Parity harness + unit tests + corpus  | `tests/` |
| CI images  | Rocky Linux 8 / 9 Dockerfiles         | `docker/` |

The single C++ core is the source of truth; the R and Python packages are thin
binding layers so both ecosystems get identical numerics.

## Repository layout

```
core/               C++17 core engine (in progress)
r-pkg/              R (Rcpp) package
py-pkg/             Python (pybind11) package
oracle/fortran/     Vendored Census X-13ARIMA-SEATS v1.1 b61 Fortran (parity oracle)
tests/
  corpus/           Frozen input data + spec files (data, census-examples,
                    generated, edge) — see tests/corpus/README.md
  parity/           Golden outputs + oracle-vs-port comparison
  compare/          Comparison tooling
  unit/             C++ unit tests
docker/             Rocky 8 / Rocky 9 build+test images
.github/workflows/  CI (ci.yml) + M11 stubs (wheels.yml, r-check.yml)
```

## Platform matrix

CI builds and parity-tests across:

| platform            | toolchain | notes |
|---------------------|-----------|-------|
| Rocky Linux 8 (RHEL8)| GCC 8.5   | oldest supported; enforces the **C++17 ceiling** (no gcc-toolset) |
| Rocky Linux 9 (RHEL9)| GCC 11    | mainstream Linux target |
| Windows             | MSVC      | `windows-latest` |
| macOS (Apple Silicon)| Apple Clang | `macos-latest` |
| macOS (Intel)       | Apple Clang | `macos-13` |

The two Rocky targets run in containers built from `docker/rocky8.Dockerfile`
and `docker/rocky9.Dockerfile`. CI is **green-by-construction**: build and test
steps are guarded so that, until the corresponding sources exist, they skip
rather than fail.

## Test corpus

`tests/corpus/` is a hermetic, frozen snapshot of real public-domain input data
(Box-Jenkins airline series; FRED PAYEMS / UNRATE / EXPGS) and a large set of
X-13 spec files — hand-written manual examples, systematically generated specs,
and edge cases. Everything is committed so runs are reproducible offline. See
[`tests/corpus/README.md`](tests/corpus/README.md).

## License

Released to the public domain under [**CC0 1.0 Universal**](LICENSE). Do whatever
you like with it.

## Disclaimer

**X13cpp is an independent project. It is not affiliated with, sponsored by, or
endorsed by the U.S. Census Bureau.** "X-13ARIMA-SEATS" refers to the Census
Bureau's program, which is used here as the reference implementation for parity
testing. The Census Bureau's software is a work of the U.S. Government and is in
the public domain; this port is provided with no warranty of any kind.
