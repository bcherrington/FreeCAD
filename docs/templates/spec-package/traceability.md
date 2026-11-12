---
title: Feature traceability matrix title
doc_type: spec
artifact_type: traceability
status: draft
owner: team-or-person
last_reviewed: YYYY-MM-DD
---

# Traceability Matrix

## Purpose

Map requirements, design sections, tasks, verification evidence, durable-doc
targets, and open decisions in both directions. Use this file when a package is
large enough that agents may miss relevant context by reading `tasks.md` alone.

Before implementing a task, review the row for that task ID and the linked
requirement/design rows. If this matrix is stale or incomplete, reconcile it
against the full spec package before coding.

If the `spec-lifecycle-manager` MCP server is available, use the
`traceability_lookup` tool for this first pass. When MCP is unavailable, read
this matrix and the linked source artifacts directly; do not rely on a separate
traceability lookup script.

## Task To Context Matrix

| Task ID | Requirements | Acceptance Criteria | Design Sections | Change Impact | Verification | Durable Targets | Open Decisions |
|---------|--------------|---------------------|-----------------|---------------|--------------|-----------------|----------------|
| T001 | Requirement 1 | AC1, AC2 | `design.md#section` | `change-impact.md#delta` or none | `verification.md#gate` | `docs/path.md` or TBD | D001 or none |

## Requirement To Delivery Matrix

Use coverage state to reconcile broad requirements before closure, especially
`must-have` requirements, architectural targets, migrations, public interfaces,
and cross-module behavior. Valid values are `complete`, `partial-routed`,
`partial-blocking`, `not-covered`, and `out-of-scope`. Every partial,
not-covered, or out-of-scope row needs one destination or a rejection rationale
before closure. Priority is requirement-level context from `requirements.md`;
include it here when it helps closure reconciliation, but do not duplicate it
on every task or acceptance criterion row.

| Requirement | Priority | Acceptance Criteria | Design Sections | Tasks | Verification | Durable Targets | Coverage State | Residual Destination |
|-------------|----------|---------------------|-----------------|-------|--------------|-----------------|----------------|----------------------|
| Requirement 1 | must-have | AC1, AC2 | `design.md#section` | T001, T002 | `verification.md#gate` | `docs/path.md` | complete | none |

## Correctness Property Coverage

| Property | Requirements | Design Sections | Tasks | Tests Or Verification | Residual Risk |
|----------|--------------|-----------------|-------|-----------------------|---------------|
| CP-001 | Requirement 1 | `design.md#section` | T001 | Test, command, review, or manual check | none or risk |

## Design To Implementation Matrix

| Design Section | Requirements | Tasks | Interfaces Or Files | Verification | Coverage State | Residual Destination |
|----------------|--------------|-------|---------------------|--------------|----------------|----------------------|
| `design.md#section` | Requirement 1 | T001 | `src/path`, `tests/path` | Test, command, or review | complete | none |

## Open Decision Impact

| Decision ID | Blocks | Affected Requirements | Affected Tasks | Resolution Needed |
|-------------|--------|-----------------------|----------------|-------------------|
| D001 | Design, implementation, verification, or closure | Requirement 1 | T001 | Decision needed |

## Maintenance Notes

- Update this matrix when requirements, design sections, task IDs, verification
  gates, durable targets, or open decisions change.
- Keep entries specific enough that an agent can start from a task ID and find
  the relevant specification context before implementation.
- If a row cannot be completed, record `TBD` and the blocker rather than
  leaving the relationship implicit.
- Treat uncovered correctness properties or acceptance criteria as readiness
  gaps until they are mapped to tasks, tests, verification, or a recorded manual
  method.
- Before closure, use the coverage-state columns to compare broad requirement
  and design wording against actual task evidence. Do not rely on task checkbox
  completion alone when the requirement or design target is broader than the
  selected implementation slice.
