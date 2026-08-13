# Project metrics

**Generated -- do not edit.** `python tools/metrics.py --write`

Every countable claim about this project is derived here and nowhere else. Prose
should LINK to this file rather than restate a number; where a number genuinely
has to appear inline, wrap it in a marker and let `--write` maintain it:

    the suite is at <!--x13:parity_pass-->NNNN<!--/x13--> passing

`python tools/metrics.py --check` fails if any marker anywhere has drifted, so a
stale figure is a build error instead of something a reader finds first.


| Metric | Value |
|---|---|
| Parity tests passing | **9034** |
| Parity tests failing | **0** |
| Parity tests skipped | **942** |
| Parity tests xfailed | **0** |
| Unit tests (ctest) | **12/12** |
| Corpus spec files | **530** |
| Parity test modules | **36** |
| Scope / trace docs under tools/ | **32** |
| C++ non-blank lines (excl. generated) | **53814** |
| C++ files (excl. generated) | **188** |
| Fortran reference, non-blank lines | **166076** |
| Fortran reference, files | **712** |
| Fortran routines ported or gated | **406** |
| Fortran routines in scope (excl. n-a) | **690** |
| Percent of routines ported | **58.8** |
| Census bugs catalogued | **45** |
| Commits | **443** |
| Active development time | **66h 55m** |
| Calendar days worked | **26** |
| Last commit | **2026-08-13** |

Metric names for markers: `parity_pass`, `parity_fail`, `parity_skip`, `parity_xfail`, `ctest`, `corpus_specs`, `parity_modules`, `scope_docs`, `cpp_lines`, `cpp_files`, `fortran_lines`, `fortran_files`, `routines_done`, `routines_total`, `routines_pct`, `census_bugs`, `commits`, `active_time`, `calendar_days`, `last_commit`.
