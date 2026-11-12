---
title: Assembly motion suspension traceability
doc_type: spec
artifact_type: traceability
status: archived
owner: local-developer
last_reviewed: 2026-11-12
---

# Traceability Matrix

## Requirement To Delivery Matrix

| Requirement | Priority | Acceptance Criteria | Design Sections | Tasks | Verification | Durable Targets | Coverage State | Residual Destination |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Requirement 1 | must-have | AC1, AC2, AC3 | design.md (Lifetime registry/context) | T002, T004 | verification.md | reference/assembly-motion-suspension.md | complete | none |
| Requirement 2 | must-have | AC1, AC2, AC3 | design.md (Motion gates and link policy) | T002, T003, T004 | verification.md | reference/assembly-motion-suspension.md | complete | none |
| Requirement 3 | must-have | AC1, AC2, AC3 | design.md (Explicit solve and ownership) | T003, T004 | verification.md | reference/assembly-motion-suspension.md | complete | none |
| Requirement 4 | must-have | AC1, AC2, AC3 | design.md (Validation and promotion) | T004, T005 | verification.md | reference/assembly-motion-suspension.md | complete | B015 baseline mocks; B016 upstream/platform/bridge qualification |

## Task To Context Matrix

| Task ID | Requirements | Acceptance Criteria | Design Sections | Change Impact | Verification | Durable Targets | Open Decisions |
| --- | --- | --- | --- | --- | --- | --- | --- |
| T001 | Requirement 1, Requirement 2, Requirement 3, Requirement 4 | AC1, AC2, AC3 | design.md#low-level-design | requirements.md#durable-impact | review.md | reviews/assembly-motion-suspension.md | none |
| T002 | Requirement 1, Requirement 2, Requirement 4 | AC1, AC2, AC3 | design.md#motion-gates | requirements.md#durable-impact | verification.md | reference/assembly-motion-suspension.md | none |
| T003 | Requirement 2, Requirement 3 | AC1, AC2, AC3 | design.md#explicit-solve-and-ownership | requirements.md#durable-impact | verification.md | reference/assembly-motion-suspension.md | none |
| T004 | Requirement 1, Requirement 2, Requirement 3, Requirement 4 | AC1, AC2, AC3 | design.md#validation-strategy | requirements.md#durable-impact | verification.md | reviews/assembly-motion-suspension.md | none |
| T005 | Requirement 4 | AC3 | design.md#downstream-task-guidance | requirements.md#durable-impact | verification.md | reference/assembly-motion-suspension.md | none |

## Correctness Property Coverage

| Property | Requirements | Design Sections | Tasks | Tests Or Verification | Residual Risk |
| --- | --- | --- | --- | --- | --- |
| CP-001 | Requirement 1 | Lifetime registry | T002, T004 | Native nesting/close and Python context/GC tests | none within qualified scope |
| CP-002 | Requirement 2 | Motion gates | T002, T003, T004 | Native GDB and Python callbacks/generated remap | arbitrary Python excluded |
| CP-003 | Requirement 3 | Raw graph admission | T003, T004 | Escaped/incomplete/unsupported matrix with snapshots | unsupported graphs reject |
| CP-004 | Requirement 4 | Compatibility | T002, T003, T004 | Outside-scope normal helper/solve tests and GDB controls | E001 baseline mocks |

## Design To Implementation Matrix

| Design Section | Requirements | Tasks | Interfaces Or Files | Verification | Coverage State | Residual Destination |
| --- | --- | --- | --- | --- | --- | --- |
| Motion gates | Requirement 1, Requirement 2 | T002 | MotionSuspension, AssemblyObject, bindings | verification.md | complete | none |
| Ownership and callbacks | Requirement 2, Requirement 3 | T003 | AssemblyObject, AssemblyLink, JointObject | verification.md | complete | none |
| Compatibility and promotion | Requirement 4 | T004, T005 | Native/Python tests, durable API docs | verification.md | complete | B015/B016 external qualification |

## Open Decision Impact

D001 resolved by native expert review: pre-change populated Rigid rejection, initialization/restore allowed.
E001 is a recorded local acceptance exception for two independently reproduced unchanged view mocks;
B015 owns repair. No unresolved local feature decision or unowned implementation work remains.
