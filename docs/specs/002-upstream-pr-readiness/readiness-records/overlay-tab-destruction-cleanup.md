---
title: Overlay tab destruction cleanup readiness record
doc_type: spec
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-07
artifact_type: note
---

# `overlay-tab-destruction-cleanup`

## Branch Identity

- Canonical branch ref: `overlay-tab-destruction-cleanup`
- Legacy package name(s): `overlay-tab-destruction-cleanup`
- Current fetched baseline: `upstream/main` at `7937058dededc800dd0f3197185daf75d146c9d6`
- Current disposition: `candidate-rebuild`

## Inventory Snapshot

- Ahead/behind vs `upstream/main`: ahead `2`, behind `946`
- Merge-commit count: `1`
- Functional commit list:
  - `7da830e4b34` `Gui: clear overlay tab globals on destruction`
- Non-functional or unrelated commit list:
  - `6e04d88c4fb` `Merge branch 'main' into overlay-tab-destruction-cleanup`
- Current touched files:
  - `src/Gui/OverlayManager.cpp`
  - `src/Gui/OverlayWidgets.cpp`
  - `src/Gui/OverlayWidgets.h`

## Cleanup Decision

- Direct rebase or rebuild: rebuild or cherry-pick the functional commit onto current `upstream/main`
- Commits to keep: `7da830e4b34`
- Commits to drop or route elsewhere: local merge commit `6e04d88c4fb`
- Dependency or sequencing notes: good early candidate after the toolbar branch if overlap research is favorable

## Issue And PR Overlap

- Related issues: pending research
- Related PRs: pending research
- Need new issue before PR: unknown

## Validation Plan

- `git diff --check`: required after the branch is rebuilt cleanly
- Build command: `pixi run build` unless a narrower GUI build is justified
- Focused tests: determine whether overlay-widget behavior has existing GUI coverage
- Manual GUI evidence: required; verify tab destruction and overlay cleanup behavior
- Performance evidence: not applicable

## Review Routing

- Design Working Group attention: advisable because the change affects visible dock/overlay behavior
- Code Quality Working Group attention: possible if lifetime/cleanup changes are non-trivial
- Cross-platform validation: event-order differences may need at least reviewer awareness on Windows/macOS
- Attribution and authorship notes: pending

## Human Sign-Off

- Contributor sign-off: pending
- Ready to validate: no
- Ready to prepare PR branch: no

## Blockers And Next Action

- Blockers: merge-commit cleanup still only planned; issue/PR overlap research still missing
- Next action: complete T008 in [tasks.md](../tasks.md) with overlap research and a concrete clean-branch strategy
