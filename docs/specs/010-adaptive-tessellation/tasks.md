---
title: Camera-Adaptive Tessellation Tasks
doc_type: spec
status: draft
owner: bcherrington
last_reviewed: 2026-08-14
artifact_type: tasks
---

# Tasks

## Dependency Graph

```text
T001 -> T002 -> T003
T002 -> T004
T003 -> T005
T004 -> T005
T005 -> T006 -> T007 -> T008 -> T009 -> T010 -> T011 -> T012
```

## Downstream Review Note (2026-08-14)

Design and verification were reviewed against `requirements.md`,
`canonical-context.md`, and this task decomposition for downstream consistency.
No scope, acceptance criteria, or task contracts were changed.

The review note was also used to re-align task metadata with the design and
verification artifacts on the same date.

- [ ] T001 Reconcile Spec 009 evidence and current Part/viewer source.
  - Depends on: none.
  - Requirements: `R1`-`R6`; Properties: `CP-001`-`CP-006`.
  - Files: Spec 009 evidence, `ViewProviderExt.cpp`, `View3DInventorViewer.*`.
  - Acceptance: Current ownership, update cost, thread boundaries, and reusable evidence are documented from direct source.
  - Evidence: Pending.

- [ ] T002 Build deterministic curved-object baseline fixtures and metrics.
  - Depends on: T001.
  - Requirements: `R3`, `R6`; Properties: `CP-004`, `CP-006`.
  - Files: focused Part/Gui tests or a bounded harness.
  - Acceptance: Fixtures expose camera tier, triangles, memory estimate, elapsed time, and native restoration without repeated app launches.
  - Evidence: Pending.

- [ ] T003 Specify and test screen-space tier policy with hysteresis.
  - Depends on: T002.
  - Requirements: `R1`; Properties: `CP-001`, `CP-006`.
  - Files: policy implementation and focused unit tests.
  - Acceptance: Stable states are idempotent and threshold crossings are deterministic.
  - Evidence: Pending.

- [ ] T004 Specify cache identity, ownership, invalidation, and eviction.
  - Depends on: T002.
  - Requirements: `R2`; Properties: `CP-003`, `CP-004`.
  - Files: design, cache implementation, focused tests.
  - Acceptance: Geometry/display changes invalidate only affected entries and memory is accounted.
  - Evidence: Pending.

- [ ] T005 Prove bounded, cancellable mesh generation feasibility.
  - Depends on: T003, T004.
  - Requirements: `R3`, `R4`; Properties: `CP-002`, `CP-006`.
  - Files: Part meshing path, feasibility tests, evidence log.
  - Acceptance: Work can be rejected before publication and can abort without blocking the GUI beyond the agreed budget.
  - Evidence: Pending.

- [ ] T006 Implement atomic last-known-good/native fallback.
  - Depends on: T005.
  - Requirements: `R4`; Properties: `CP-002`, `CP-004`.
  - Files: Part/Gui integration and focused failure tests.
  - Acceptance: Pending, failed, cancelled, and over-budget work never replaces a valid mesh.
  - Evidence: Pending.

- [ ] T007 Integrate the adaptive controller behind a disabled flag.
  - Depends on: T006.
  - Requirements: `R1`-`R5`; Properties: `CP-001`-`CP-006`.
  - Files: viewer, Part view provider, preferences only if separately approved.
  - Acceptance: Normal FreeCAD can evaluate the real path without document mutation or native-default change.
  - Evidence: Pending.

- [ ] T008 Prove interaction and persistence invariants.
  - Depends on: T007.
  - Requirements: `R4`, `R5`; Properties: `CP-002`, `CP-004`, `CP-005`.
  - Files: focused GUI tests.
  - Acceptance: Selection, preselection, picking, visibility, navigation cube, reset, save, and reopen remain correct.
  - Evidence: Pending.

- [ ] T009 Run representative-model and platform validation.
  - Depends on: T008.
  - Requirements: `R3`, `R5`, `R6`; Properties: `CP-001`, `CP-005`, `CP-006`.
  - Files: `verification.md`, retained metrics/screenshots.
  - Acceptance: Visible benefit and budgets are recorded for Debug/Release and supported platforms; gaps are explicit.
  - Evidence: Pending.

- [ ] T010 Run static analysis and focused regression validation.
  - Depends on: T009.
  - Requirements: `R6`; Properties: `CP-001`-`CP-006`.
  - Files: changed source and focused tests.
  - Acceptance: Formatting, pre-commit, compiler warnings, relevant clang-tidy/static checks, and focused tests pass or have accepted findings.
  - Evidence: Pending.

- [ ] T011 Obtain independent FreeCAD implementation and test review.
  - Depends on: T010.
  - Requirements: `R5`, `R6`; Properties: `CP-001`-`CP-006`.
  - Files: complete change set and evidence.
  - Acceptance: Blockers are resolved and remaining platform/design risks are classified.
  - Evidence: Pending.

- [ ] T012 Decide production gate, promote durable docs, and close or defer.
  - Depends on: T011.
  - Requirements: `R6`; Properties: `CP-002`, `CP-004`, `CP-006`.
  - Files: requirements, design, traceability, verification, durable docs.
  - Acceptance: Accepted behavior is promoted, or rejection/defer evidence preserves native behavior; all task and traceability states reconcile.
  - Evidence: Pending.
