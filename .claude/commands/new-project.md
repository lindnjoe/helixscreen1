---
description: Start a new multi-phase project. Researches codebase, proposes phases, creates plan doc, and sets up worktree.
argument-hint: "[rough description of what you want to build]"
---

# NEW MULTI-PHASE PROJECT

```dot
digraph new_project {
  rankdir=TB;

  // Main flow
  start [label="Start" shape=circle];
  check_super [label="Superpowers?" shape=diamond];
  requirements [label="Gather Requirements"];
  research [label="Research Codebase"];
  propose [label="Propose Phases"];
  refine [label="Refine with User"];
  plan [label="Create Plan Doc"];
  worktree_ask [label="Worktree?" shape=diamond];
  worktree_setup [label="Setup Worktree"];
  handoff [label="Output HANDOFF"];
  done [label="Done" shape=doublecircle];

  // Superpowers delegation
  sp_brainstorm [label="superpowers:brainstorming"];
  sp_plans [label="superpowers:writing-plans"];
  sp_worktree [label="superpowers:using-git-worktrees"];

  // Flow - with superpowers
  start -> check_super;
  check_super -> sp_brainstorm [label="available"];
  sp_brainstorm -> sp_plans [label="design approved"];
  sp_plans -> sp_worktree [label="plan complete"];
  sp_worktree -> handoff;

  // Flow - fallback
  check_super -> requirements [label="not available"];
  requirements -> research;
  research -> propose;
  propose -> refine;
  refine -> propose [label="needs changes"];
  refine -> plan [label="approved"];
  plan -> worktree_ask;
  worktree_ask -> worktree_setup [label="yes"];
  worktree_ask -> handoff [label="no"];
  worktree_setup -> handoff;

  handoff -> done;
}
```

## NODE DETAILS

**check_super**: Test if `superpowers:brainstorming` skill available. If yes, delegate entire flow to superpowers chain.

**requirements**: If user hasn't described feature, ask: "What feature would you like to build?"

**research**: Launch Explore agents for: similar patterns, architecture, testing conventions, existing infrastructure.

**propose**: AskUserQuestion with phase breakdown. Each phase ≈ 1 commit. Phase 0 = foundation. Include goal, deliverables, verification. Typically 3-6 phases.

**refine**: Iterate based on feedback. Clarify: priorities, patterns, testing requirements.

**plan**: Write to `docs/plans/[project-name]-plan.md`:
```markdown
# [Feature Name]
## Overview
[1-2 sentences]
## Phases
### Phase N: [Name]
**Goal**: [What] | **Deliverables**: [List] | **Verification**: [How]
## Progress
- [ ] Phase 0: [Name]
## Key Files
- `path/file.cpp` - [role]
```

**worktree_setup**: Check CLAUDE.md for conventions. Run: `git worktree add -b [branch] ../[project]-[feature] main` + init scripts.

**handoff**: Output `HANDOFF: [Project Name]`. Inform user: plan location, worktree path (if created), use `/continue` to resume.

## PLAN LOCATION
Prefer `docs/plans/` (in project, versioned). Use `~/.claude/plans/` only if project disallows.

## DEFAULTS
Delegation: main coordinates, agents implement | TDD: backend=yes, UI=skip | Reviews: proportional | Commits: 1/phase
