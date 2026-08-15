---
title: Upstream PR Readiness for FreeCAD Topic Branches Canonical Context
doc_type: spec
artifact_type: canonical-context
status: draft
owner: FreeCAD contributor
last_reviewed: 2026-08-14
---

# Canonical Context

## Purpose

This package is the active, scoped source of truth for branch readiness and upstream PR prep coordination, keeping branch inventory, cleanup policy, and validation planning consistent while preventing scope drift.

## Authority Hierarchy

- `AGENTS.md` and repository-level instructions in this workspace.
- This package’s lifecycle artifacts for plan and validation routing.
- Current source and tests only for runtime truth; this package does not authorize source changes.

## Always-Canonical External Sources

| Source | Authority reason | Handling |
| --- | --- | --- |
| `CONTRIBUTING.md` | binding contribution-process baseline | Treat as policy gate for package scope and PR expectations |
| `.github/pull_request_template.md` | binding PR workflow and disclosure expectations | Treat as process gate for PR claims and evidence formatting |
| `AI_POLICY.md` | binding human/AI disclosure policy | Treat as mandatory policy for package evidence and signoff context |

## Spec-Canonical Working Sources

- `requirements.md`: package scope, gates, and required outcomes.
- `design.md`: workflow and process model.
- `research.md`: baseline and branch inventory snapshot.
- `tasks.md`: execution graph and task completion expectations.
- `verification.md`: package-level lint/build/test/documentation obligations.
- `traceability.md`: requirement/task/verification mapping.
- `readiness-record-template.md` and `readiness-records/*.md`: branch-specific state.

## Imported Sources

| Source | Source path | Status | Canonical scope |
| --- | --- | --- | --- |
| Contributor policy baseline | `CONTRIBUTING.md`, `.github/pull_request_template.md`, `AI_POLICY.md` | current | PR-process predicates for this package |
| Readiness artifacts | `docs/specs/002-upstream-pr-readiness/*` | in package | branch readiness workflow and evidence tracking |
| Developers handbook | `freecad.github.io/DevelopersHandbook/...` | contextual | review framing and committee expectations |

## Non-Canonical Background Sources

| Source | Reason non-canonical | Handling |
| --- | --- | --- |
| Prior lint advisory log text | transient operational notes | use only for history, not contract |
| Unrelated branch notes | out of package scope | route to branch records only if revalidated |

## Promotion Map

| Spec-local content | Durable destination | Required before closure |
| --- | --- | --- |
| Reusable readiness workflow | long-lived contributor-process docs | only after repeated use or explicit policy codification |
| Branch cleanup conventions | branch records and archive log | whenever a candidate is promoted |
