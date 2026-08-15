---
title: Toolbar persistence save settings readiness record
doc_type: spec
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-07
artifact_type: note
---

# `toolbar-persistence-save-settings`

## Branch Identity

- Canonical branch ref: `toolbar-persistence-save-settings`
- Legacy package name(s): `toolbar-persistence-save-settings`
- Current fetched baseline: `upstream/main` at `7937058dededc800dd0f3197185daf75d146c9d6`
- Current disposition: `candidate-clean`

## Inventory Snapshot

- Ahead/behind vs `upstream/main`: ahead `1`, behind `990`
- Merge-commit count: `0`
- Functional commit list:
  - `3d3504041fc` `Gui: save window settings on left-click drag of toolbars`
- Non-functional or unrelated commit list: none detected in current local branch
- Current touched files:
  - `src/Gui/ToolBarManager.cpp`
  - `src/Gui/ToolBarManager.h`

## Cleanup Decision

- Direct rebase or rebuild: likely direct rebase onto current `upstream/main`
- Commits to keep: `3d3504041fc`
- Commits to drop or route elsewhere: none known yet
- Dependency or sequencing notes: good first candidate if issue research supports it

## Issue And PR Overlap

- Related issues: pending research
- Related PRs: pending research
- Need new issue before PR: unknown

## Validation Plan

- `git diff --check`: required on the rebased final PR branch
- Build command: `pixi run build` unless a narrower Gui-only build is justified
- Focused tests: determine whether toolbar persistence has existing focused GUI coverage
- Manual GUI evidence: required; show toolbar drag and persistence result before/after
- Performance evidence: not applicable

## Review Routing

- Design Working Group attention: likely advisable because toolbar behavior is visible
- Code Quality Working Group attention: probably not required unless cleanup exposes broader GUI state handling
- Cross-platform validation: Windows and macOS persistence behavior still need consideration
- Attribution and authorship notes: pending

## Human Sign-Off

- Contributor sign-off: pending
- Ready to validate: no
- Ready to prepare PR branch: no

## Blockers And Next Action

- Blockers: missing issue/PR overlap review; missing branch-specific validation evidence
- Next action: complete T007 in [tasks.md](../tasks.md) by researching the upstream issue/PR context and confirming the focused validation path
