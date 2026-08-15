---
title: Upstream PR readiness record template
doc_type: spec
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-07
artifact_type: note
---

# Readiness Record Template

Use one copy of this structure per canonical candidate branch under
`readiness-records/`.

## Branch Identity

- Canonical branch ref:
- Legacy package name(s):
- Current fetched baseline:
- Current disposition:

## Inventory Snapshot

- Ahead/behind vs `upstream/main`:
- Merge-commit count:
- Functional commit list:
- Non-functional or unrelated commit list:
- Current touched files:

## Cleanup Decision

- Direct rebase or rebuild:
- Commits to keep:
- Commits to drop or route elsewhere:
- Dependency or sequencing notes:

## Issue And PR Overlap

- Related issues:
- Related PRs:
- Need new issue before PR:

## Validation Plan

- `git diff --check`:
- Build command:
- Focused tests:
- Manual GUI/startup evidence:
- Performance evidence:

## Review Routing

- Design Working Group attention:
- Code Quality Working Group attention:
- Cross-platform validation:
- Attribution and authorship notes:

## Human Sign-Off

- Contributor sign-off:
- Ready to validate:
- Ready to prepare PR branch:

## Blockers And Next Action

- Blockers:
- Next action:
