---
title: FreeCAD Performance Investigation Traceability Matrix
doc_type: spec
artifact_type: traceability
status: draft
owner: local-developer
last_reviewed: 2026-08-14
---

# Traceability Matrix

## Purpose

Provide task-first and requirement-first lookup across intent, design, evidence,
decisions, validation, and durable destinations. Before implementation, read the
selected task row and every linked artifact. If this matrix differs from those
artifacts, reconcile it rather than treating the matrix as an override.

## Identity And Authority

- Active package: `004-performance-investigation`
- Legacy package: `20260524-performance-investigation`
- Historical evidence authority: `research.md`
- Proposed reproduction authority: `quickstart.md`
- Current authority order: `canonical-context.md`

Legacy task aliases:

| Legacy alias | Active task ID | Status |
| --- | --- | --- |
| T007A | T025 | complete |
| T012A | T026 | pending |
| T014A | T027 | complete |
| T014B | T028 | pending |

## Task To Context Matrix

| Task ID | Requirements | Acceptance Criteria | Design Sections | Change Impact | Verification | Durable Targets | Open Decisions |
| --- | --- | --- | --- | --- | --- | --- | --- |
| T001 | Requirement 1; Requirement 6 | AC1.1-AC1.3; AC6.4 | Template Authority And Migration Decision; Components And Changes; Data Models | none - evidence preservation | VG-001; VG-002 | historical-only unless D005 promotes | D005 |
| T002 | Requirement 1; Requirement 3 | AC1.1; AC3.1, AC3.2, AC3.3, AC3.4 | Workstream Boundary Rules | none | VG-004; VG-005 | routing destinations before closure | D003; D004 |
| T003 | Requirement 1; Requirement 3 | AC1.3; AC3.3 | Template Authority And Migration Decision; Workstream Boundary Rules | none | VG-002; VG-004 | none | none |
| T004 | Requirement 2; Requirement 3 | AC2.2; AC3.3 | Data Models; Workstream Boundary Rules | none | VG-003 | profiling procedure candidate | D005 |
| T005 | Requirement 2 | AC2.2 | Data Models | none | VG-003 | none | none |
| T006 | Requirement 2 | AC2.1-AC2.4 | Data Models; Validation Strategy | none | VG-003 | none | none |
| T007 | Requirement 2 | AC2.1-AC2.4 | Data Models; Validation Strategy | none | VG-003 | none | none |
| T025 | Requirement 1; Requirement 2 | AC1.1; AC2.3 | Error Handling; Validation Strategy | none | VG-002; VG-003 | none | none |
| T008 | Requirement 1; Requirement 2 | AC1.1-AC1.3; AC2.1 | Data Flow; Validation Strategy | none | VG-002; VG-003 | validated conclusions only | D005 |
| T009 | Requirement 2 | AC2.1; AC2.4 | Error Handling | none | VG-003 | none | D001 |
| T010 | Requirement 1; Requirement 4 | AC1.1-AC1.3; AC4.1 | Template Authority And Migration Decision; Data Flow; Validation Strategy | none | VG-002; VG-004 | none | D002 |
| T011 | Requirement 1; Requirement 4 | AC1.1-AC1.3; AC4.1 | Data Flow; Validation Strategy | none | VG-002; VG-004 | none | D002 |
| T012 | Requirement 1; Requirement 4 | AC1.3; AC4.1-AC4.2 | Template Authority And Migration Decision; Migration And Compatibility | none | VG-002; VG-004 | historical experiment disposition | D002; D005 |
| T026 | Requirement 2; Requirement 4; Requirement 5 | AC2.1, AC2.2, AC2.3, AC2.4; AC4.1, AC4.2, AC4.3; AC5.1, AC5.2, AC5.3 | Workstream Boundary Rules; Algorithms And Logic; Slice Boundary And Residual Architecture; Validation Strategy | potential future GUI change, not yet accepted | VG-003; VG-004; VG-006 | profiling procedure and accepted finding | D002; D005 |
| T026.1 | Requirement 4; Requirement 5 | AC4.1, AC4.2, AC4.3; AC5.1 | Algorithms And Logic; Migration And Compatibility | experiment-only source delta | VG-004; VG-006 | none until D002 | D002 |
| T026.2 | Requirement 2; Requirement 5 | AC2.1, AC2.2, AC2.3, AC2.4; AC5.2 | Data Models; Validation Strategy | baseline evidence only | VG-003 | profiling procedure candidate | D002; D005 |
| T026.3 | Requirement 2; Requirement 5 | AC2.1, AC2.2, AC2.4; AC5.2 | Data Models; Validation Strategy | experiment evidence only | VG-003 | profiling procedure candidate | D002; D005 |
| T026.4 | Requirement 1; Requirement 5 | AC1.1, AC1.2, AC1.3; AC5.2, AC5.3 | Data Flow; Algorithms And Logic | decision only | VG-002; VG-006 | accepted conclusion or rejection record | D002; D005 |
| T027 | Requirement 1; Requirement 2 | AC1.3; AC2.1 | Error Handling | none | VG-003 | none | none |
| T028 | Requirement 2 | AC2.1-AC2.4 | Data Models; Validation Strategy | none | VG-003 | profiling procedure candidate | D005 |
| T013 | Requirement 2; Requirement 5 | AC2.1; AC5.2-AC5.3 | Workstream Boundary Rules; Slice Boundary And Residual Architecture | potential future GUI change | VG-003; VG-006 | validated attribution only | D001; D005 |
| T014 | Requirement 2; Requirement 5 | AC2.1; AC5.2-AC5.3 | Workstream Boundary Rules; Slice Boundary And Residual Architecture | potential future GUI/platform change | VG-003; VG-006 | validated attribution only | D001; D005 |
| T015 | Requirement 2; Requirement 5 | AC2.1; AC5.2-AC5.3 | Workstream Boundary Rules; Slice Boundary And Residual Architecture | potential future viewer/Part GUI change | VG-003; VG-006 | validated attribution only | D001; D005 |
| T016 | Requirement 3 | AC3.1; AC3.4 | Workstream Boundary Rules; Slice Boundary And Residual Architecture | potential future GUI/Part GUI change | VG-005 | focused owner or route | D003 |
| T017 | Requirement 3; Requirement 5 | AC3.1; AC5.2-AC5.3 | Workstream Boundary Rules; Slice Boundary And Residual Architecture | potential future GUI/Part GUI change | VG-005; VG-006 | focused owner or route | D003 |
| T018 | Requirement 3; Requirement 6 | AC3.4; AC6.2; AC6.4 | Slice Boundary And Residual Architecture; Promotion Plan | routing only | VG-005; VG-007 | explicit focused destination | D003; D005 |
| T019 | Requirement 3 | AC3.2; AC3.4 | Workstream Boundary Rules; Slice Boundary And Residual Architecture | none until ownership is known | VG-005 | focused owner or route | D004 |
| T020 | Requirement 3 | AC3.2 | Workstream Boundary Rules | none until ownership is known | VG-005 | focused owner or route | D004 |
| T021 | Requirement 3; Requirement 6 | AC3.2; AC6.2 | Security, Trust, And Access; Workstream Boundary Rules | routing only | VG-005; VG-007 | addon/core/issue/spec destination | D004; D005 |
| T022 | Requirement 4; Requirement 5 | AC4.1-AC4.3; AC5.1-AC5.3 | Algorithms And Logic; Slice Boundary And Residual Architecture | bounded target TBD | VG-004; VG-006 | selected finding/decision | D002; D005 |
| T023 | Requirement 2; Requirement 5; Requirement 6 | AC2.1-AC2.4; AC5.2; AC6.3 | Validation Strategy | validation plan only | VG-003; VG-006 | exercised validation guidance | D002; D005 |
| T024 | Requirement 3; Requirement 6 | AC3.4; AC6.2, AC6.3, AC6.4 | Slice Boundary And Residual Architecture; Promotion Plan | routing and promotion only | VG-005; VG-007 | one destination per residual | D003; D004; D005 |

## Requirement To Delivery Matrix

Coverage states describe the whole active spec, not only the documentation
revision completed on 2026-08-14.

| Requirement | Priority | Acceptance Criteria | Design Sections | Tasks | Verification | Durable Targets | Coverage State | Residual Destination |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Requirement 1 | must-have | AC1.1 (Requirement 1 AC1), AC1.2 (Requirement 1 AC2), AC1.3 (Requirement 1 AC3) | Template Authority And Migration Decision; Data Models | T001-T003, T008, T010-T012, T025, T027 | VG-001, VG-002, VG-004 | historical disposition and D005 | partial-blocking | finish selected fresh-evidence reconciliation |
| Requirement 2 | must-have | AC2.1 (Requirement 2 AC1), AC2.2 (Requirement 2 AC2), AC2.3 (Requirement 2 AC3), AC2.4 (Requirement 2 AC4) | Data Models; Error Handling; Validation Strategy | T004-T009, T025-T028, T013-T015, T023 | VG-003, VG-006 | profiling/runbook target via D005 | partial-blocking | T026 selected capture, then T023 |
| Requirement 3 | must-have | AC3.1 (Requirement 3 AC1), AC3.2 (Requirement 3 AC2), AC3.3 (Requirement 3 AC3), AC3.4 (Requirement 3 AC4) | Workstream Boundary Rules | T002-T004, T016-T021, T024 | VG-004, VG-005, VG-007 | focused destinations via D003/D004 | partial-blocking | T018, T021, T024 |
| Requirement 4 | must-have | AC4.1 (Requirement 4 AC1), AC4.2 (Requirement 4 AC2), AC4.3 (Requirement 4 AC3) | Template Authority And Migration Decision; Migration And Compatibility | T010-T012, T026, T022 | VG-004, VG-006 | accepted experiment disposition | partial-blocking | T026 and D002 |
| Requirement 5 | must-have | AC5.1 (Requirement 5 AC1), AC5.2 (Requirement 5 AC2), AC5.3 (Requirement 5 AC3) | Algorithms And Logic; Slice Boundary And Residual Architecture | T026, T013-T015, T017, T022-T023 | VG-006 | selected target or rejection record | partial-blocking | T026, D002, T022-T023 |
| Requirement 6 | must-have | AC6.1 (Requirement 6 AC1), AC6.2 (Requirement 6 AC2), AC6.3 (Requirement 6 AC3), AC6.4 (Requirement 6 AC4) | Requirement Coverage; Downstream Review Receipt; Promotion Plan | T001-T002, T018, T021-T024 | VG-001, VG-002, VG-006, VG-007 | D003-D005 and closure log | partial-blocking | review, promotion, residual routing, cleanup |

## Correctness Property Coverage

| Property | Requirements | Design Sections | Tasks | Tests Or Verification | Residual Risk |
| --- | --- | --- | --- | --- | --- |
| CP-001 | Requirement 1; Requirement 6 | Data Models | all completed tasks | task-state audit; MoE evidence review; VG-002 | future evidence can still become stale |
| CP-002 | Requirement 2; Requirement 3 | Workstream Boundary Rules | T002, T006-T009, T016-T021, T025-T028 | research section review; VG-003; VG-005 | shared Qt symbols can complicate attribution |
| CP-003 | Requirement 1; Requirement 4 | Template Authority And Migration Decision; Data Flow | T008, T010-T012, T022, T026 | direct source/Git inspection; VG-004 | branch drift requires per-slice refresh |
| CP-004 | Requirement 3; Requirement 5 | Algorithms And Logic; Slice Boundary And Residual Architecture | T022-T024, T026 | decision/task-marker audit; VG-006 | investigations may still compete for owner time |

## Design To Implementation Matrix

| Design Section | Requirements | Tasks | Interfaces Or Files | Verification | Coverage State | Residual Destination |
| --- | --- | --- | --- | --- | --- | --- |
| Template Authority And Migration Decision; Data Models | Requirement 1; Requirement 4 | T001-T003, T008, T010-T012 | package docs; current source/Git | VG-001, VG-002, VG-004 | partial-blocking | refresh during T026 |
| Data Models; Validation Strategy | Requirement 2 | T004-T009, T025-T028 | `quickstart.md`, profiler artifacts | VG-003 | partial-blocking | T026 |
| Workstream Boundary Rules | Requirement 3 | T002, T016-T021, T024 | research/decisions | VG-005, VG-007 | partial-blocking | D003, D004, T024 |
| Algorithms And Logic; Slice Boundary And Residual Architecture | Requirement 5 | T026, T022-T024 | decisions, source branch, validation plan | VG-006 | partial-blocking | D002, T022-T023 |
| Promotion Plan | Requirement 6 | T018, T021, T024 | durable docs/routes | VG-007 | not-covered | D005 and T024 |

## Open Decision Impact

| Decision ID | Blocks | Affected Requirements | Affected Tasks | Resolution Needed |
| --- | --- | --- | --- | --- |
| D001 | resolved next-investigation selection | Requirement 5 | T026 | Overlay matched validation selected on 2026-08-14. |
| D002 | implementation readiness | Requirement 2; Requirement 4; Requirement 5 | T026, T022, T023 | Keep/recreate, reject, or gather stronger evidence. |
| D003 | document-open ownership and closure | Requirement 3; Requirement 6 | T016-T018, T024 | One explicit owner/destination or rejection rationale. |
| D004 | Preferences ownership and closure | Requirement 3; Requirement 6 | T019-T021, T024 | Evidence-backed addon/environment/core classification. |
| D005 | promotion and closure | Requirement 6 | T001, T004, T008, T012-T024, T026, T028 | Durable destination or discard/routing rationale. |

## Maintenance Notes

- Update this matrix whenever requirement IDs, acceptance criteria, design
  sections, task IDs, decisions, verification gates, or destinations change.
- Use full names such as `Requirement 2` in task metadata so lifecycle lookup
  can recover the authoritative requirement section.
- `partial-blocking` and `not-covered` rows block closure until completed,
  rejected with rationale, or changed to `partial-routed` with one destination.
- Planned validation and advisory repository evidence are not proof.

## Fallback Template Authority Review

- 2026-08-14: Reviewed after removal of the repository template override. The
  bundled Spec Lifecycle Manager templates remain authoritative; the mappings
  and closure coverage above are unchanged.
