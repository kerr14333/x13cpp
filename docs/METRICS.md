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
| Parity tests passing | **5634** |
| Parity tests failing | **0** |
| Parity tests skipped | **469** |
| Parity tests xfailed | **0** |
| Unit tests (ctest) | **11/11** |
| Corpus spec files | **386** |
| Parity test modules | **26** |
| C++ non-blank lines (excl. generated) | **44136** |
| C++ files (excl. generated) | **176** |
| Fortran reference, non-blank lines | **166076** |
| Fortran reference, files | **712** |
| Fortran routines ported or gated | **401** |
| Fortran routines in scope (excl. n-a) | **690** |
| Percent of routines ported | **58.1** |
| Census bugs catalogued | **31** |
| Commits | **358** |
| Active development time | **56h 00m** |
| Calendar days worked | **12** |
| Last commit | **2026-07-29** |

Metric names for markers: `parity_pass`, `parity_fail`, `parity_skip`, `parity_xfail`, `ctest`, `corpus_specs`, `parity_modules`, `cpp_lines`, `cpp_files`, `fortran_lines`, `fortran_files`, `routines_done`, `routines_total`, `routines_pct`, `census_bugs`, `commits`, `active_time`, `calendar_days`, `last_commit`.
