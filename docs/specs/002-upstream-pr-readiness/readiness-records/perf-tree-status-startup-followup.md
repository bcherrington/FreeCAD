---
title: Tree status startup follow-up readiness record
doc_type: spec
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-07
artifact_type: note
---

# `perf/tree-status-startup-followup`

## Branch Identity

- Canonical branch ref: `perf/tree-status-startup-followup`
- Legacy package name(s): `perf-tree-status-startup-followup`
- Current fetched baseline: `upstream/main` at `7937058dededc800dd0f3197185daf75d146c9d6`
- Current disposition: `candidate-rebuild`

## Inventory Snapshot

- Ahead/behind vs `upstream/main`: ahead `6`, behind `990`
- Merge-commit count: `0`
- Functional commit list:
  - `61031d7f8a5` `Gui: batch tree status after restore`
- Non-functional or unrelated commit list:
  - `29d09d06db9` `Docs: record startup follow-up findings`
  - `bfc7d07bfa9` `Docs: record restore follow-up findings`
  - `5ee907d4d3f` `Docs: add performance optimization tasks and link in the spec`
  - `b7c2b4d00fa` `Docs: organize specs and templates`
  - `b211afa411b` `Docs: add FreeCAD profiling and optimization implementation plan`
- Current touched files:
  - `src/Gui/Tree.cpp`
  - `src/Gui/Tree.h`
  - unrelated local docs/spec files under `docs/specs/freecad-performance-optimization/`

## Cleanup Decision

- Direct rebase or rebuild: rebuild from the functional commit only
- Commits to keep: `61031d7f8a5`
- Commits to drop or route elsewhere: the docs/spec commits listed above
- Dependency or sequencing notes: branch name drift is resolved in favor of the slash-named ref

## Issue And PR Overlap

- Related issues: pending research
- Related PRs: pending research
- Need new issue before PR: unknown

## Validation Plan

- `git diff --check`: required on the rebuilt final branch
- Build command: `pixi run build`
- Focused tests: determine whether tree-restore or startup status behavior has existing GUI tests
- Manual GUI evidence: required; verify startup/restore tree status timing
- Performance evidence: required; before/after startup workload and variance notes

## Review Routing

- Design Working Group attention: likely advisable because UI status timing is visible
- Code Quality Working Group attention: probably not needed unless overlap research finds architectural concerns
- Cross-platform validation: startup timing should be treated as platform-sensitive
- Attribution and authorship notes: pending

## Human Sign-Off

- Contributor sign-off: pending
- Ready to validate: no
- Ready to prepare PR branch: no

## Blockers And Next Action

- Blockers: docs/spec contamination; missing issue/PR overlap review; startup validation plan not yet defined
- Next action: complete T011 in [tasks.md](../tasks.md) by confirming that the single functional commit still stands on its own
