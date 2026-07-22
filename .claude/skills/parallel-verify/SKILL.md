---
name: parallel-verify
description: Run an independent second attempt at a change in parallel — you in an isolated git worktree, Codex (codex:codex-rescue) in the main tree — then compare the two solutions and keep the better one. Use when the user asks for a cross-check / independent verification, or for subtle/high-stakes work where two agreeing solutions add real confidence. Carries the isolation + Codex-prompt + result-fetch gotchas that otherwise waste turns.
---

# Parallel independent verification (you ‖ Codex)

Goal: get **two independently-derived solutions** to the same task and compare them.
Convergence on the same approach is strong evidence the fix is right; a divergence is
a flag to dig. Use when the user says "have codex also try / verify in parallel", or
when you judge a subtle change is worth an independent cross-check.

## The one hard constraint: ISOLATE your copy

`codex:codex-rescue` runs in the **shared runtime = the main working tree**. If you
also edit the main tree you clobber each other. So **do your attempt in a separate git
worktree**:

```bash
git worktree add -b myattempt-<slug> ../<repo>-<slug> HEAD
```

Codex edits the main tree; you edit the worktree. Fully independent, no races. Each
tree has its **own `build/`** — expect a full rebuild in the worktree (see `build-run`;
run `& tools/build.ps1` from the worktree dir).

### GOTCHA: the Agent tool's `isolation: "worktree"` does NOT reliably branch from HEAD
Observed: spawning subagents with `isolation: "worktree"` put them on **stale old
commits** (it reused pre-existing `worktree-agent-*` branches, not current HEAD) — so
their work targeted a divergent codebase (missing files that exist at HEAD, missing
test infra) and could not be merged back. One agent's whole premise ("wire the
existing `spectru.cpp`") was void because that file didn't exist on its base.
**Before trusting a worktree-isolated agent's output, verify its base:**
`git -C <worktree> log --oneline -1` — if it's not HEAD, the work is on the wrong
base. Prefer provisioning the worktree YOURSELF from HEAD (`git worktree add … HEAD`)
and pointing the agent at that path, or just do parallel novel-port work **inline in
the main tree** (only one editor per file at a time). If an agent already produced good
logic on a stale base, salvage it by hand-porting the additive pieces onto HEAD rather
than merging the branch. And: **novel bit-exact ports** (vs *verification* of a known
fix) are riskier for cold subagents — they must re-derive the whole port convention;
weigh doing them inline.

Shell cwd resets to the main tree between tool calls — use `Set-Location <worktree>`
(PowerShell) or absolute paths for every worktree command.

## Spawn Codex (background)

`Agent` tool, `subagent_type: "codex:codex-rescue"`, `run_in_background: true`. The
prompt must be **fully self-contained** — Codex starts cold. Include:
- repo path + the contract (e.g. bit-exact 1e-8 vs the oracle);
- **build/test commands + env quirks**: PowerShell `& tools/build.ps1` (not
  `powershell -File`), rtools44 first on PATH, `python` not `python3`, Bash can't run
  the Windows `.exe`; parity via `python -m pytest tests/parity -q`;
- the exact task + the file:line leads you already have;
- **precise acceptance criteria and how to verify** (which harness, which golden,
  which gates must stay green — name them);
- guardrails: **do NOT commit**, leave `oracle/fortran/` pristine, revert any
  instrumentation.

## GOTCHA: you must verify Codex's result yourself

The `codex:codex-rescue` wrapper **cannot poll or report** the underlying Codex task —
`SendMessage` to it just says "can't inspect the task". So don't wait on it for a
verdict. When Codex's edits land in the main tree (you'll see them via
`git status --porcelain` / `git diff`), **verify them yourself**: build + run the named
gates in the main tree. That IS the verification of Codex's attempt.

## Compare + land

1. Diff both solutions (`git diff` in each tree). Note: same approach? same code? any
   divergence in correctness or cleanliness (dead code, docs, edge cases)?
2. Confirm **both** pass the acceptance gates independently (you already ran yours in
   the worktree; run Codex's in the main tree).
3. Keep the better one. If Codex's is in the main tree and you keep it, just stage +
   commit the real source/test files (leave stray run-output untracked). If you keep
   yours, copy the worktree's changed files over (or cherry-pick the branch).
4. **Tear down**: `git worktree remove ../<repo>-<slug> --force` then
   `git branch -D myattempt-<slug>`.
5. Report the comparison to the user: approach agreement, the key diff, and that both
   verified green. The independent-agreement cross-check is the deliverable, not just
   the passing test.

## Notes
- Spawn Codex only when the user asks (or names it). One well-scoped `task` per fix.
- If the two diverge, don't silently pick one — surface the divergence; it usually
  means the spec/acceptance criteria were ambiguous.
