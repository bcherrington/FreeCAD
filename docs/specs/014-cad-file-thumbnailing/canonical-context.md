---
title: CAD And Mesh File Thumbnailing Canonical Context
doc_type: spec
status: draft
owner: local-developer
last_reviewed: 2026-08-14
artifact_type: canonical-context
---

# Canonical Context

## Purpose

This package replaces the legacy unnumbered `cad-file-thumbnailing` draft with the
current lifecycle structure while preserving all substantive behavioral intent.

## Authority Hierarchy

- Package-local lifecycle artifacts are authoritative for this migration slice.
- `AGENTS.md` and repository specs/docs are authoritative for scope, commands, and governance.
- Current source and verified runtime evidence outrank historical artifacts for behavior claims.

## Migration Decision

- Package format: **migrate before implementation**
- Template authority: repository default package behavior plus package-local additions
  where needed for migration traceability
- Reason: package is an active unnumbered draft that currently uses legacy shape.
- Impact: new package-local artifacts only; no template or repository-wide changes.

## spec.md Classification

- Role: **migration input**
- Duplicates current artifacts? no
- Carries unpromoted decisions or acceptance criteria? yes
- Action: removed after requirements/design/tasks/traceability/verification preserved substantive content

## Canonical Context Decision

- Needed: yes (migration ambiguity and imported legacy intent)
- Shape: separate `canonical-context.md`
- Reason: preserve source-of-truth for migration evidence and authority ordering.

### Always-Canonical External Sources

| Source | Authority reason | Handling |
| --- | --- | --- |
| `/home/bcherrington/Projects/CLion/FreeCAD/AGENTS.md` | Governs repository and task execution rules | Read before edits |
| `docs/specs/README.md` | Defines active package index and migration conventions | Structural rules and ID policy |
| `docs/specs/015-compact-view-rails-rework/*` | Current package shape exemplar | Formatting and task/traceability style guidance |
| `references/migration-guide.md` | Defines migration decision and `spec.md` classification rules | Process rule for this package |

## Imported Sources

| Source | Source path | Status | Scope | Promotion target |
| --- | --- | --- | --- | --- |
| Legacy brief | `docs/specs/cad-file-thumbnailing/spec.md` | preserved then removed in migration | Summary/goal/requirement concepts | local migration |
| Migration framework | `/home/bcherrington/.codex/plugins/cache/auriora-local/spec-lifecycle-manager/0.6.1+codex.20260813062002/skills/spec-lifecycle-manager/references/migration-guide.md` | active | Migration decisions and classifications | process rules only |

### Spec-Canonical Working Sources

| Source | Role | Scope |
| --- | --- | --- |
| `requirements.md` | acceptance scope and behaviors | package-local contract |
| `design.md` | migration-safe architecture and safety framing | package-local plan |
| `tasks.md` | execution graph and task evidence expectation | package-local execution |
| `traceability.md` | requirement/design/task/test mapping | package-local mapping |
| `verification.md` | validation and lint gates | package-local proof obligations |

### Non-Canonical Background Sources

| Source | Reason non-canonical | Handling |
| --- | --- | --- |
| `docs/specs/cad-file-thumbnailing/spec.md` | Legacy draft format | source-of-content only; already removed after migration |
| informal branch notes | not part of this migration slice | ignore unless verified against source |

## Promotion Map

| Spec-local content | Durable destination or route | Required before closure |
| --- | --- | --- |
| `R2` render support and safety envelope | `src/Tools/freecad-thumbnailer.in` implementation evidence and source docs | yes |
| Any accepted MIME scope | package/installation docs and desktop integration references | yes |
| Remaining open runtime questions | follow-up specification or backlog item | yes |
