---
name: staleness-audit
description: Read-only sweep for stale/contradictory content — status prose (READMEs, handoff/scouting docs, CLAUDE.md), test skip/xfail reason strings, and "deferred/not-ported/TODO" code comments — that no longer matches the current committed code + test results. Treats code+tests as ground truth, reports a categorized triage list, fixes nothing. Use when the user asks "anything stale here?" after a burst of progress, or before a handoff.
---

# Staleness audit

"Stale" = prose written to describe a **past** state that the current committed
code/tests no longer match. **Ground truth is the code + tests + git HEAD, not the
prose.** Report; do not fix (a separate pass fixes, with the user's OK).

Best run in a **subagent** (read-only, e.g. `general-purpose` or `Explore`) so the
sweep doesn't burn the main thread's context. Give it the current ground-truth
numbers up front so it can judge contradictions.

## Establish ground truth first
- `git log --oneline -8` — what just landed.
- Run the suite once, read-only, for the authoritative pass/skip/xfail counts
  (here: `python -m pytest tests/parity -q`). Note **which** xfails actually fire —
  a spec that now passes but is still described as "xfailed/blocked/deferred"
  somewhere is the #1 stale pattern.

## Where staleness hides (check in this order — hit rate high→low)
1. **Status prose** — `CLAUDE.md` "current phase", `docs/*` progress/handoff files,
   `tools/*_scouting.md` / `*_handoff.md` / `coverage_plan.md`. Look for: old
   pass-counts, "Open/Pending/Partial/~N% off" on things now closed, "blocked on X"
   where X shipped. Dated session-log docs are append-only — flag only the **leading
   status** that reads as current, not every historical line.
2. **Test reason strings** — `pytest.xfail("...")` / `skip` reasons, exclusion
   comments (`if _has_outlier: continue`), unparsed-block sets. Cross-check each
   against what actually passes. A reason citing a since-fixed blocker is stale even
   if the test still (correctly or not) xfails.
3. **Code comments** — `deferred / not yet ported / not bit-exact / stub / TODO /
   FIXME` for things now implemented. Distinguish from **genuinely** unported
   branches that still fatal cleanly (those comments are accurate — leave them).
   Also stale `file:line` cross-refs (a comment citing `foo.f:1148` when it moved).
4. **Untracked clutter** — `git status --porcelain`: stray run-output / build
   artifacts written into a tracked tree (here: harness/oracle save-tables like
   `*.a1/.s10/.fct` under `tests/corpus/`). Group + count; propose a `.gitignore`
   rule (keep-only-sources: ignore `dir/**/*.*`, re-include the real source
   extensions).
5. **Stale counts in READMEs** ("N specs", "M tests") and **regen footguns** (a
   generator that silently deletes hand-authored files).

## Output
Group findings by the categories above. Per finding: `file:line`, a one-line quote/
paraphrase, and **why** it's stale (what the current state actually is). Mark
CONFIRMED vs POSSIBLY stale. End with a **triage split**: safe-to-auto-fix/delete
vs needs-human-judgment (dated snapshots, archive-vs-update calls). Never edit
`oracle/fortran/` or any vendored source; the sweep is read-only.

## Fix pass (only after the user OKs)
Safe auto-fixes: delete stray output + add the `.gitignore` rule; correct a test
reason string; rewrite a stale status bullet. Leave dated handoff/summary artifacts
and genuinely-open scouting items alone unless asked. Commit as a `chore:` with
"no engine behavior change" + the current pass count.
