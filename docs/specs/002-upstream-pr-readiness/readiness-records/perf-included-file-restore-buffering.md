---
title: Included file restore buffering readiness record
doc_type: spec
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-07
artifact_type: note
---

# `perf/included-file-restore-buffering`

## Branch Identity

- Canonical branch ref: `perf/included-file-restore-buffering`
- Legacy package name(s): `perf-included-file-restore-buffering`
- Current fetched baseline: `upstream/main` at `7937058dededc800dd0f3197185daf75d146c9d6`
- Current disposition: `candidate-rebuild`

## Inventory Snapshot

- Ahead/behind vs `upstream/main`: ahead `2`, behind `948`
- Merge-commit count: `1`
- Functional commit list:
  - `fdea634de98` `App: buffer included file restore`
- Non-functional or unrelated commit list:
  - one merge from `main`
- Related alias/composite branch note:
  - `perf-included-file-restore-buffering` also exists locally, but `git cherry -v upstream/main perf-included-file-restore-buffering` reports its `save` commit as upstream-equivalent, so the slash-named restore branch is the cleaner active source.
- Current touched files:
  - `src/App/PropertyFile.cpp`
  - `src/Mod/Test/Document.py`

## Cleanup Decision

- Direct rebase or rebuild: rebuild or cherry-pick the restore commit onto current `upstream/main`
- Commits to keep: `fdea634de98`
- Commits to drop or route elsewhere: the merge commit; the separate `save` change remains out of scope here
- Dependency or sequencing notes: treat this record as the canonical included-file candidate

## Issue And PR Overlap

- Related issues: pending research
- Related PRs: pending research
- Need new issue before PR: unknown

## Validation Plan

- `git diff --check`: required after the clean branch is rebuilt
- Build command: `pixi run build`
- Focused tests: run the relevant document restore tests in addition to any changed Python tests
- Manual GUI evidence: likely not required unless user-visible behavior changes outside document restore timing
- Performance evidence: required; document size/workload and variance notes

## Review Routing

- Design Working Group attention: probably not required
- Code Quality Working Group attention: possible if restore semantics or test coverage are controversial
- Cross-platform validation: restore correctness should not be assumed from Linux alone
- Attribution and authorship notes: pending

## Human Sign-Off

- Contributor sign-off: pending
- Ready to validate: no
- Ready to prepare PR branch: no

## Blockers And Next Action

- Blockers: overlap research and concrete restore-test plan remain missing
- Next action: complete T009 in [tasks.md](../tasks.md) and keep the composite hyphenated branch out of the active path
