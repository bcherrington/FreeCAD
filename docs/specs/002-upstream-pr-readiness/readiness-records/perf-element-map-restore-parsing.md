---
title: Element map restore parsing readiness record
doc_type: spec
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-07
artifact_type: note
---

# `perf/element-map-restore-parsing`

## Branch Identity

- Canonical branch ref: `perf/element-map-restore-parsing`
- Legacy package name(s): `perf-element-map-restore-parsing`
- Current fetched baseline: `upstream/main` at `7937058dededc800dd0f3197185daf75d146c9d6`
- Current disposition: `candidate-rebuild`

## Inventory Snapshot

- Ahead/behind vs `upstream/main`: ahead `5`, behind `990`
- Merge-commit count: `0`
- Functional commit list:
  - `8dc855b1948` `App: speed up element map restore parsing`
- Non-functional or unrelated commit list:
  - `bfc7d07bfa9` `Docs: record restore follow-up findings`
  - `5ee907d4d3f` `Docs: add performance optimization tasks and link in the spec`
  - `b7c2b4d00fa` `Docs: organize specs and templates`
  - `b211afa411b` `Docs: add FreeCAD profiling and optimization implementation plan`
- Current touched files:
  - `src/App/ElementMap.cpp`
  - `tests/src/App/ElementMap.cpp`
  - unrelated local docs/spec files under `docs/specs/freecad-performance-optimization/`

## Cleanup Decision

- Direct rebase or rebuild: rebuild from the single functional commit only
- Commits to keep: `8dc855b1948`
- Commits to drop or route elsewhere: the docs/spec commits listed above
- Dependency or sequencing notes: likely mid-pack candidate once a smaller GUI or restore branch lands first

## Issue And PR Overlap

- Related issues: pending research
- Related PRs: pending research
- Need new issue before PR: unknown

## Validation Plan

- `git diff --check`: required on the rebuilt branch
- Build command: `pixi run build`
- Focused tests: run the touched App test target and any nearby restore-parsing coverage
- Manual GUI evidence: not expected
- Performance evidence: required; element-map restore workload and variance notes

## Review Routing

- Design Working Group attention: not expected
- Code Quality Working Group attention: possible because parser changes affect correctness and performance
- Cross-platform validation: CI or reviewer confirmation may still be useful
- Attribution and authorship notes: pending

## Human Sign-Off

- Contributor sign-off: pending
- Ready to validate: no
- Ready to prepare PR branch: no

## Blockers And Next Action

- Blockers: docs/spec contamination and missing overlap research
- Next action: complete T010 in [tasks.md](../tasks.md) with issue research and a focused App-test plan
