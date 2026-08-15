---
title: Splashscreen refactor readiness record
doc_type: spec
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-07
artifact_type: note
---

# `16264-splashscreen-refactor`

## Branch Identity

- Canonical branch ref: `16264-splashscreen-refactor`
- Legacy package name(s): `16264-splashscreen-refactor`
- Current fetched baseline: `upstream/main` at `7937058dededc800dd0f3197185daf75d146c9d6`
- Current disposition: `candidate-high-risk`

## Inventory Snapshot

- Ahead/behind vs `upstream/main`: ahead `6`, behind `990`
- Merge-commit count: `0`
- Functional commit list:
  - `34365e87fe9` `Gui: add optional early splash screen`
  - `ef9a95b8a12` `Gui: enable early splash startup path`
  - `c42e8008089` `Gui: keep early splash visible until startup completes`
  - `99636bd54f8` `Gui: start early splash with selected image`
  - `4512d3549b3` `Gui: unify startup splash implementation`
  - `a0d2a27337d` `Gui: preserve hidden startup after splash`
- Non-functional or unrelated commit list: none detected in current local branch
- Current touched files:
  - `src/Gui/Application.cpp`
  - `src/Gui/MainWindow.cpp`
  - `src/Gui/SplashScreen.cpp`
  - `src/Gui/SplashScreen.h`
  - `src/Gui/StartupProcess.cpp`
  - `src/Gui/StartupProcess.h`
  - `tests/src/Gui/SplashScreen.cpp`
  - `tests/src/Gui/CMakeLists.txt`

## Cleanup Decision

- Direct rebase or rebuild: undecided; likely a careful rebase or curated branch rebuild after overlap research
- Commits to keep: all six functional commits are currently part of the candidate
- Commits to drop or route elsewhere: none known yet
- Dependency or sequencing notes: keep late in the sequence because startup and cross-platform risk are high

## Issue And PR Overlap

- Related issues: pending research
- Related PRs: pending research
- Need new issue before PR: unknown

## Validation Plan

- `git diff --check`: required on any final PR branch
- Build command: `pixi run build`
- Focused tests: run the touched splash/startup GUI tests and any startup-related smoke coverage
- Manual GUI evidence: required; before/after startup screenshots or recordings
- Performance evidence: optional unless startup timing is part of the argument

## Review Routing

- Design Working Group attention: likely required
- Code Quality Working Group attention: possible because startup sequencing and visibility logic changed substantially
- Cross-platform validation: likely required for Windows and macOS in addition to local Linux checks
- Attribution and authorship notes: pending

## Human Sign-Off

- Contributor sign-off: pending
- Ready to validate: no
- Ready to prepare PR branch: no

## Blockers And Next Action

- Blockers: large startup surface, missing overlap research, and missing cross-platform validation strategy
- Next action: complete T014 in [tasks.md](../tasks.md) only after smaller candidates have clarified the current PR environment
