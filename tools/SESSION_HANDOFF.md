# Session handoff — 2026-07-24 (CES / parser-fidelity)

## Where things stand

- **Branch:** `checkpoint/m5-seats-slidingspans`. **HEAD:** `13d03c2` —
  *M5/x11: gate the BLS CES production specs — x11{appendfcst/final} + d16*.
- **Parity:** 1102 pass / 0 fail / 25 skip. **Unit:** 10/10. Active dev time
  ~33h 28m.
- Build: `& D:\code_projects\x13new\tools\build.ps1` (PowerShell tool, not
  `powershell -File`). Parity: `python -m pytest tests/parity -q`.

## What landed this session

1. **SEATS mean + other regressors (mean + TD)** — `9c76f7c`.
2. **Parser case-insensitivity + save-table `name=` label** — `8e27426`. The
   parser matched enumerated argument *values* case-sensitively, so uppercase
   `TRANSFORM{FUNCTION = LOG}` (every BLS CES spec is uppercase) silently dropped
   the log transform. Fixed by folding bare NAME value-tokens to lowercase at the
   single capture seam (`push_tok` in `readers_spec.cpp`); QUOTE tokens preserved.
3. **Full BLS CES spec gated bit-exact** — `13d03c2`. See below.

## The CES spec is closed

`tests/corpus/ces/AE1011330000.spc` (11 user `usertype=td` regressors, AO
identification at `critical=10.5`, fixed `(0 1 0)(0 1 1)`, `FINAL=USER`,
`APPENDFCST=YES`) gates b1/d10/d11/d16 at ~5e-15 via the new
`tests/parity/test_ces_tables.py` (8 cases: both CES specs × 4 tables).

**The earlier "FINAL=USER makes D11 garbage (−2896, inf)" note was stale** — that
was the pre-8e27426 levels-vs-log parse bug. Once the transform parsed, the
numbers were already exact; the only real gaps were parse routing + save extent:

- `x11{appendfcst/appendbcst}` were token-consumed and dropped, so `Savfct`
  stayed false and the harness emitted 126 rows against a 150-row d10/d16 golden.
- `x11{final=}` / `x11{keepholiday}` likewise unparsed (`Finao/Finls/Finusr/
  Fintc/Finhol`). `FINAL=USER` is **inert** on this spec: the user regressors are
  `usertype=td`, so their effect lands in `Factd`, not `Facusr`.
- D16 (`ststd`) is a function-local in x11pt3; now snapshotted onto
  `ctx.x11_ststd` at the oracle's punch point (after the `Khol==1` return) and
  emitted by the harness.

## Next steps (priority order)

1. **`COMPOSITE{}` increments 2 and 3.** Increment 1 (the DIRECT composite total)
   landed bit-exact in `799d2f7` — see `tools/composite_scouting.md` for the full
   map. inc2 = the indirect adjustment (`agr3.f`/`agr3s.f`/`agrxpt.f` + the
   O1..O5/Ci/Ci2/Omod buffers of `agr2.f`); inc3 = the direct-vs-indirect
   comparison statistics (`Iagr==4`, `cmpchi.f`, `prtagr`/`pragr2`).
   Side note found while gating: the two composite COMPONENT specs
   (`region_north`/`region_south`, automdl{} on a synthetic series) sit ~1e-4
   from the oracle — an estimation-path difference unrelated to aggregation,
   worth its own look on the automdl front.
2. **CES extraction/spec generator** — one Python pass over `ce.data` to emit the
   ~143 NSA series `AE<code>.dat` + rewrite the DOS `FILE=` paths, then run the
   whole corpus through engine+oracle and triage. Parse is case-insensitive now,
   so the specs run as-shipped (uppercase).
3. **Save-table headers for x11/seats tables** (b1/d10/…): the `name=` Serlbl fix
   covers only the pre-model tables (a1/a2/a3/trn). Cosmetic until byte-identical
   save parity is gated.
4. Remaining x11 stubs (interdependent chains, each needs an upstream factor
   producer): user PRIOR factors (Nuspad/Nustad temporary adjustment), force
   non-original target (Iftrgt>0), revisions getrev.

## Cleanup — done

- `ces_series/` (337 MB of BLS flat files, untracked) **deleted 2026-07-24** on
  the user's say-so. Everything the corpus needs is in `tests/corpus/ces/data/`;
  re-fetch the bulk `ce.data.0.AllCESSeries` / `ce.series` with the curl +
  contact-User-Agent recipe in `tools/ces_seasadj_notes.md` when the spec
  generator needs them.

## Notes / gotchas carried forward

- **Windows Python** (`python`) does not grok git-bash `/d/` mounts — use `D:/…`
  paths in Python heredocs. `ls`/shell builtins do understand `/d/`.
- Ad hoc engine run: `build/x13run_x11.exe <spec>` (dumps b1/d10-d13/d16 to
  stdout, no files), `build/x13run_m3.exe <spec>` (estimate probe). Run from the
  spec's directory — data paths are relative.
- Ad hoc oracle run: copy spec+data into a dir, `cd` there, run
  `oracle/fortran/x13as_ascii_O2.exe <specbase>`.
- **BLS 403 bypass:** `curl` with a contact User-Agent
  `x13cpp-research cyg5005@gmail.com` works on www.bls.gov and download.bls.gov
  (WebFetch 403s).
- **Methodology lesson:** the all-lowercase generated corpus never exercised
  uppercase, so the parse bug hid behind a green suite — and then a stale
  diagnosis ("FINAL=USER") outlived the bug it was blamed on. Re-measure before
  acting on an inherited claim.
