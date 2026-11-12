---
title: Feature requirements title
doc_type: spec
artifact_type: requirements
status: draft
owner: team-or-person
last_reviewed: YYYY-MM-DD
---

# Requirements

## Introduction

Brief description of the change. What problem is being solved and why it matters.

## Goals

- Goal

## Non-Goals

- Non-goal

## Glossary

| Term | Definition |
|------|-----------|
| Term | Definition |

## Durable Source Baseline

Reference durable source-of-truth docs, contracts, schemas, runbooks,
governance files, or code-derived references that describe current behavior
before this change. Durable docs should describe current accepted state unless
they are explicitly labeled proposed, planned, deferred, or historical.

| Source | Current behavior relied on | Confidence | Notes |
|--------|----------------------------|------------|-------|
| `docs/path/to/doc.md` | Current behavior summary | high | |
| none found | Documentation gap summary | low | Promote accepted behavior to `docs/path/to/new-doc.md`. |

If no durable source exists, record the gap and the durable document that should
become the source of truth after promotion.

## Durable Impact

Describe how this spec affects durable docs. Use `change-impact.md` instead
when the impact spans several documents or behavior classes.

| Durable area | Action | Target | Notes |
|--------------|--------|--------|-------|
| requirements | add\|modify\|clarify\|supersede\|unchanged | `docs/requirements/path.md` | |
| design | add\|modify\|clarify\|supersede\|unchanged | `docs/design/path.md` | |
| architecture | add\|modify\|clarify\|supersede\|unchanged | `docs/architecture/path.md` | |
| API/contract | add\|modify\|clarify\|supersede\|unchanged | `docs/api/path.md` | |
| data-flow/runbook/reference/ADR | add\|modify\|clarify\|supersede\|unchanged | `docs/path.md` | |

## Staged Readiness

- **Current stage:** requirements
- **Next stage:** design
- **Ready to design when:** goals, non-goals, user stories, acceptance
  criteria, correctness properties, durable-source baseline, durable impact,
  and open questions are coherent enough to make design decisions.
- **Design-first exception:** no | yes - record the partial requirements that
  must be completed before implementation readiness.
- **Optional artifacts recommended:** none | `research.md` | `change-impact.md`
  | `open-decisions.md`
- **Downstream review needed:** none | design | tasks | traceability |
  verification

## Requirements

### Requirement 1: Title

**User Story:** As a [role], I want [feature], so that [benefit]

**Priority:** must-have

#### Acceptance Criteria

1. GIVEN [context], WHEN [action], THEN [outcome]
2. WHERE [condition applies], THE SYSTEM SHALL [behavior]

### Requirement 2: Title

**User Story:** As a [role], I want [feature], so that [benefit]

**Priority:** should-have

#### Acceptance Criteria

1. IF [condition], THEN THE SYSTEM SHALL [behavior]
2. WHILE [state], THE SYSTEM SHALL [behavior]

Use requirement-level MoSCoW priority when it clarifies scope. Accepted
persisted values are `must-have`, `should-have`, and `could-have`. Missing
priority remains valid for compatibility. Do not duplicate priority on every
acceptance criterion; criteria inherit the parent requirement priority. Keep
excluded work in non-goals, out-of-scope text, rejected decisions, or routed
residuals rather than persisting `won't-have` as an accepted requirement
priority.

## Correctness Properties

Properties that must hold for the system to be considered correct. These inform
property-based testing and should be referenced from task-level test work when
they apply. Use the target repository's normal property-test tool when one is
already accepted; otherwise record the dependency decision in `design.md` or
`open-decisions.md`.

- **CP-001**: [Invariant or property description]
- **CP-002**: [Invariant or property description]

Each correctness property should later map to design behavior and at least one
task, test, validation command, or explicit manual verification method.

## Technical Context

- **Language/Version:**
- **Primary Dependencies:**
- **Target Platform:**
- **Constraints:**
- **Performance Goals:**

## Success Criteria

- **SC-001**: [Measurable outcome]
- **SC-002**: [Measurable outcome]

## Related Artifacts

- Change Impact:
- Design:
- Tasks:
- Verification:
