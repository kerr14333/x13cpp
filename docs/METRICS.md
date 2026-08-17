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
| Parity tests passing | **9057** |
| Parity tests failing | **0** |
| Parity tests skipped | **946** |
| Parity tests xfailed | **0** |
| Unit tests (ctest) | **12/12** |
| Corpus spec files | **531** |
| Parity test modules | **36** |
| Scope / trace docs under tools/ | **32** |
| C++ non-blank lines (excl. generated) | **54320** |
| C++ files (excl. generated) | **188** |
| Fortran reference, non-blank lines | **166076** |
| Fortran reference, files | **712** |
| Fortran FILES ported or gated | **421** |
| Fortran files in scope (excl. n-a) | **690** |
| Percent of files ported | **61.0** |
| Fortran ROUTINES with a same-named C++ definition | **399** |
| Fortran routines in scope (excl. n-a) | **1141** |
| Percent of routines same-named (renames count as missing) | **35.0** |
| Census bugs catalogued | **45** |
| Commits | **450** |
| Active development time | **67h 15m** |
| Calendar days worked | **27** |
| Last commit | **2026-08-16** |

Metric names for markers: `parity_pass`, `parity_fail`, `parity_skip`, `parity_xfail`, `ctest`, `corpus_specs`, `parity_modules`, `scope_docs`, `cpp_lines`, `cpp_files`, `fortran_lines`, `fortran_files`, `files_done`, `files_total`, `files_pct`, `routines_samename`, `routines_total`, `routines_pct`, `census_bugs`, `commits`, `active_time`, `calendar_days`, `last_commit`.
