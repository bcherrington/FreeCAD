---
title: Feature tasks title
doc_type: spec
artifact_type: tasks
status: draft
owner: team-or-person
last_reviewed: YYYY-MM-DD
---

# Tasks

## Context And Execution

Read requirements.md, design.md, traceability.md and verification.md before
implementation. Resolve the selected task's blocking decisions and obtain task
context. Use stable task IDs as identities; file lines are navigation only.
Normally mark one implementation task `[~]` before coding. Preserve unrelated
work and split tasks that mix independent implementation/evidence outcomes.

Markers: `[ ]` pending, `[~]` in progress, `[/]` partial, `[>]` routed,
`[-]` deferred/no-op, `[?]` decision required, `[!]` attention, `[x]` complete.
Use `[P]` only for work without dependency or file conflicts. Every completed
task needs acceptance evidence. Evidence modes: command, artifact, manual,
reasoned, external. Pending or blocked evidence cannot complete implementation.

## Task Dependency Graph

```text
T001 -> T002 -> T003 -> T004
```

## Delivery Slices

- [ ] T001 Resolve design and validation contracts.
  - Depends on: none
  - Requirements: Requirement 1 AC1, AC2
  - Design: design.md#low-level-design
  - Files: design.md, verification.md
  - Acceptance: Scope, failure/compatibility rules, validation and blocking decisions are resolved for T002.
  - Validation: Review the selected design/requirement criteria and evidence plan.
  - Evidence mode: artifact
  - Evidence: Pending.

- [ ] T002 Implement the independently testable user story.
  - Depends on: T001
  - Requirements: Requirement 1 AC1, AC2
  - Properties: CP-001, when defined
  - Design: design.md#low-level-design
  - Files: Replace with exact source and test paths before implementation.
  - Acceptance: Success, failure and regression criteria pass without changing unrelated behavior.
  - Validation: Replace with focused repository commands or bounded manual checks.
  - Evidence mode: command
  - Evidence: Pending.
  - [ ] T002.1 Add meaningful behavior/failure coverage.
  - [ ] T002.2 Implement the scoped behavior.
  - [ ] T002.3 Verify linked properties with existing conventional/property tests or justified manual checks.

- [ ] T003 Checkpoint - Validate selected scope and resolve review findings.
  - Depends on: T002
  - Requirements: Requirement 1 AC1, AC2
  - Files: verification.md, traceability.md
  - Acceptance: Required checks and review are recorded; broader requirement/design coverage has explicit complete, partial-blocking, partial-routed, not-covered or out-of-scope dispositions.
  - Validation: Run focused checks; broaden under AGENTS.md according to changed boundaries.
  - Evidence mode: command
  - Evidence: Pending.

- [ ] T004 Promote durable truth, route residuals and record closure.
  - Depends on: T003
  - Requirements: Requirement 1 AC1, AC2
  - Files: Exact durable targets, closure log and archive index.
  - Acceptance: Durable docs match accepted behavior, residual work has owned destinations, and closure/cleanup commits reflect actual Git evidence.
  - Validation: Check promotion, coverage and closure metadata; structural lint alone does not satisfy the gate.
  - Evidence mode: artifact
  - Evidence: Pending.

## Related Artifacts

- [Requirements](requirements.md)
- [Design](design.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
