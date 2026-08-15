---
title: Bitmap conversion fast path readiness record
doc_type: spec
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-07
artifact_type: note
---

# `perf/bitmap-conversion-fast-path`

## Branch Identity

- Canonical branch ref: `perf/bitmap-conversion-fast-path`
- Legacy package name(s): `perf-bitmap-conversion-fast-path`
- Current fetched baseline: `upstream/main` at `7937058dededc800dd0f3197185daf75d146c9d6`
- Current disposition: `candidate-rebuild`

## Inventory Snapshot

- Ahead/behind vs `upstream/main`: ahead `5`, behind `990`
- Merge-commit count: `0`
- Functional commit list:
  - `80b03911461` `Gui: speed up 32-bit image conversion`
- Non-functional or unrelated commit list:
  - `bfc7d07bfa9` `Docs: record restore follow-up findings`
  - `5ee907d4d3f` `Docs: add performance optimization tasks and link in the spec`
  - `b7c2b4d00fa` `Docs: organize specs and templates`
  - `b211afa411b` `Docs: add FreeCAD profiling and optimization implementation plan`
- Current touched files:
  - `src/Gui/BitmapFactory.cpp`
  - `tests/src/Gui/BitmapFactory.cpp`
  - `tests/src/Gui/CMakeLists.txt`
  - unrelated local docs/spec files under `docs/specs/freecad-performance-optimization/`

## Cleanup Decision

- Direct rebase or rebuild: rebuild from the functional/test commit only
- Commits to keep: `80b03911461`
- Commits to drop or route elsewhere: the docs/spec commits listed above
- Dependency or sequencing notes: re-check upstream overlap before spending cleanup effort

## Issue And PR Overlap

- Related issues: pending research
- Related PRs: pending research
- Need new issue before PR: unknown

## Validation Plan

- `git diff --check`: required on the rebuilt branch
- Build command: `pixi run build`
- Focused tests: run the touched GUI test target for `BitmapFactory`
- Manual GUI evidence: helpful if visual output changes are user-visible
- Performance evidence: required; conversion workload and variance notes

## Review Routing

- Design Working Group attention: maybe, depending on whether output differences are visible
- Code Quality Working Group attention: possible because of low-level conversion logic
- Cross-platform validation: advisable because image-path behavior may vary
- Attribution and authorship notes: pending

## Human Sign-Off

- Contributor sign-off: pending
- Ready to validate: no
- Ready to prepare PR branch: no

## Blockers And Next Action

- Blockers: overlap research may show diminished standalone value; docs/spec contamination still needs cleanup planning
- Next action: complete T013 in [tasks.md](../tasks.md) by confirming the branch still offers value beyond current upstream behavior
