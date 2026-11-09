---
title: Mainstream CAD performance investigation traceability
doc_type: spec
artifact_type: traceability
status: draft
owner: local-developer
last_reviewed: 2026-11-09
---

# Traceability Matrix

## Task To Context Matrix

| Task ID | Requirements | Acceptance Criteria | Design Sections | Change Impact | Verification | Durable Targets | Open Decisions |
| --- | --- | --- | --- | --- | --- | --- | --- |
| T001 | Requirement 1 | Requirement 1 AC1,AC2,AC3,AC4 | design.md#workloads-and-provenance | requirements.md#durable-impact | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | D001 |
| T002 | Requirement 1 | Requirement 1 AC1,AC2,AC3,AC4 | design.md#collection-and-attribution | requirements.md#durable-impact | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | D001, D002 |
| T003 | Requirement 1, Requirement 2 | Requirement 1 AC1,AC2,AC3,AC4; Requirement 2 AC1,AC2,AC3 | design.md#stream-contracts | requirements.md#durable-impact | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | D001, D002 |
| T004 | Requirement 1, Requirement 3 | Requirement 1 AC1,AC2,AC3,AC4; Requirement 3 AC1,AC2,AC3 | design.md#stream-contracts | requirements.md#durable-impact | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | D001, D002 |
| T005 | Requirement 1, Requirement 3, Requirement 4 | Requirement 1 AC1,AC2,AC3,AC4; Requirement 3 AC1,AC2,AC3; Requirement 4 AC1,AC2,AC3 | design.md#qr-comparison | requirements.md#durable-impact | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | D001, D002 |
| T006 | Requirement 1, Requirement 5 | Requirement 1 AC1,AC2,AC3,AC4; Requirement 5 AC1,AC2,AC3 | design.md#stream-contracts | requirements.md#durable-impact | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | D001, D002 |
| T007 | Requirement 1, Requirement 6 | Requirement 1 AC1,AC2,AC3,AC4; Requirement 6 AC1,AC2,AC3 | design.md#stream-contracts | requirements.md#durable-impact | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | D001, D002 |
| T008 | Requirement 1, Requirement 7 | Requirement 1 AC1,AC2,AC3,AC4; Requirement 7 AC1,AC2,AC3 | design.md#stream-contracts | requirements.md#durable-impact | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | D001, D002 |
| T009 | Requirement 1, Requirement 8 | Requirement 1 AC1,AC2,AC3,AC4; Requirement 8 AC1,AC2,AC3 | design.md#tree-investigation | requirements.md#durable-impact | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | D001, D002 |
| T010 | Requirement 1, Requirement 9 | Requirement 1 AC1,AC2,AC3,AC4; Requirement 9 AC1,AC2,AC3 | design.md#stream-contracts | requirements.md#durable-impact | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | D001, D002 |
| T011 | Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10 | Requirement 1 AC1,AC2,AC3,AC4; Requirement 2 AC1,AC2,AC3; Requirement 3 AC1,AC2,AC3; Requirement 4 AC1,AC2,AC3; Requirement 5 AC1,AC2,AC3; Requirement 6 AC1,AC2,AC3; Requirement 7 AC1,AC2,AC3; Requirement 8 AC1,AC2,AC3; Requirement 9 AC1,AC2,AC3; Requirement 10 AC1,AC2,AC3,AC4 | design.md#ranking-and-delivery | requirements.md#durable-impact | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | none |
| T012 | Requirement 10 | Requirement 10 AC1,AC2,AC3,AC4 | design.md#ranking-and-delivery | requirements.md#durable-impact | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | none |

## Requirement To Delivery Matrix

Coverage is not-covered until task evidence exists. The destination identifies
the owning planned task, not an approved omission from this investigation.

| Requirement | Priority | Acceptance Criteria | Design Sections | Tasks | Verification | Durable Targets | Coverage State | Residual Destination |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Requirement 1 | must-have | AC1, AC2, AC3, AC4 | design.md#workloads-and-provenance | T001, T002, T011 | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | not-covered | T001, T002, T011; local-developer; investigation pending |
| Requirement 2 | must-have | AC1, AC2, AC3 | design.md#stream-contracts | T003, T011 | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | not-covered | T003, T011; local-developer; investigation pending |
| Requirement 3 | must-have | AC1, AC2, AC3 | design.md#stream-contracts | T004, T005, T011 | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | not-covered | T004, T005, T011; local-developer; investigation pending |
| Requirement 4 | must-have | AC1, AC2, AC3 | design.md#qr-comparison | T005, T011 | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | not-covered | T005, T011; local-developer; investigation pending |
| Requirement 5 | must-have | AC1, AC2, AC3 | design.md#stream-contracts | T006, T011 | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | not-covered | T006, T011; local-developer; investigation pending |
| Requirement 6 | must-have | AC1, AC2, AC3 | design.md#stream-contracts | T007, T011 | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | not-covered | T007, T011; local-developer; investigation pending |
| Requirement 7 | must-have | AC1, AC2, AC3 | design.md#stream-contracts | T008, T011 | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | not-covered | T008, T011; local-developer; investigation pending |
| Requirement 8 | must-have | AC1, AC2, AC3 | design.md#tree-investigation | T009, T011 | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | not-covered | T009, T011; local-developer; investigation pending |
| Requirement 9 | must-have | AC1, AC2, AC3 | design.md#stream-contracts | T010, T011 | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | not-covered | T010, T011; local-developer; investigation pending |
| Requirement 10 | must-have | AC1, AC2, AC3, AC4 | design.md#ranking-and-delivery | T011, T012 | verification.md#investigation-gates | docs/reference/mainstream-cad-performance.md; docs/runbooks/mainstream-cad-performance-investigation.md | not-covered | T011, T012; local-developer; investigation pending |

## Correctness Property Coverage

| Property | Requirements | Design Sections | Tasks | Tests Or Verification | Residual Risk |
| --- | --- | --- | --- | --- | --- |
| CP-001 | Requirement 1, Requirement 9 | design.md#workloads-and-provenance | T001, T002, T003–T010 | Original/copy hashes, ownership/reset checks | Pending actual fixtures |
| CP-002 | Requirements 2–6, Requirement 9 | design.md#stream-contracts | T003–T007, T010 | Frozen independent geometry/reference/solver/joint oracles | Model scope explicit |
| CP-003 | Requirement 4 | design.md#qr-comparison | T001, T002, T005, T011, T012 | Both ranks/diagnostics/DoF, numerical/geometry/lifetime checks, settings restore and final ranking/closure audit | Rank sensitivity |
| CP-004 | Requirements 2, 3, 6, 7, 8 | design.md#stream-contracts; design.md#tree-investigation | T002–T004, T007–T009 | Correct rendered/select result, undo/observer/queued/lifetime checks | Driver/profile limited |
| CP-005 | Requirement 1, Requirement 10 | design.md#collection-and-attribution | T001–T012 | Identity/raw summary, trace overhead and overlap audit | Traced shares are not savings |

## Design To Implementation Matrix

This spec implements investigation artifacts, not optimized production behavior.

| Design Section | Requirements | Tasks | Interfaces Or Files | Verification | Coverage State | Residual Destination |
| --- | --- | --- | --- | --- | --- | --- |
| design.md#workloads-and-provenance | Requirement 1 | T001, T002 | Planned fixture/protocol/qualification manifests | verification.md#investigation-gates | not-covered | T001/T002; local-developer |
| design.md#stream-contracts | Requirements 2, 3, 5, 6, 7, 9 | T003, T004, T006–T008, T010 | Planned per-stream reports | verification.md#investigation-gates | not-covered | Named stream tasks; local-developer |
| design.md#qr-comparison | Requirement 4 | T005 | B001 and planned Q01 report | verification.md#investigation-gates | not-covered | T005; local-developer |
| design.md#tree-investigation | Requirement 8 | T009 | B011 and planned T01 report | verification.md#investigation-gates | not-covered | T009; local-developer |
| design.md#ranking-and-delivery | Requirement 10 | T011, T012 | Planned opportunities report, durable reference/runbook | verification.md#durable-promotion-and-cleanup | not-covered | T011/T012; local-developer |

## Open Decision Impact

| Decision ID | Blocks | Affected Requirements | Affected Tasks | Resolution Needed |
| --- | --- | --- | --- | --- |
| D001 | Timed collection/harness qualification | Requirement 1 and all stream requirements | T001 resolves; T002–T010 depend transitively | Actual representative fixtures/oracles, collection and resource bounds |
| D002 | Accepted stream observations | Requirements 1–9 | T002 resolves; T003–T010 depend transitively | Current interfaces, endpoint/harness/runtime/trace qualification |

## Stream And Backlog Coverage

| Stream | Selected area | Owner task | Existing proposal | Current state |
| --- | --- | --- | --- | --- |
| R01 | Parametric recompute | T003 | New mainstream investigation | pending |
| S01 | Sketch interaction | T004 | Broader than unchanged forced recompute | pending |
| Q01 | QR comparison | T005 | B001 | scoped here; not executed |
| F01 | Patterns/booleans/fillets/chamfers | T006 | New mainstream investigation | pending |
| A01 | Assembly operations | T007 | New mainstream investigation | pending |
| V01 | Viewport/tessellation/selection | T008 | New mainstream investigation | pending |
| T01 | Large-model tree | T009 | B011 | scoped here; not executed |
| I01 | Native model I/O and distinct STEP import | T010 | Broader than Spec003 payload/image evidence | pending |

## Success Criteria Coverage

| Criterion | Tasks | Verification | Current disposition |
| --- | --- | --- | --- |
| SC-001 | T003–T011 | All eight reports and completeness audit | pending |
| SC-002 | T001–T010 | Workload registry and ordinary/larger controls | pending |
| SC-003 | T005, T009, T011 | B001/B011 exact results or explicit evidence limitations | pending |
| SC-004 | T011 | Deduplicated ranked opportunity report | pending |
| SC-005 | T012 | Independent review, promotion and owned closure metadata | pending |

## Maintenance Notes

Update both sides when tasks, criteria, endpoints or scope change. Do not mark
coverage complete from an authored plan, source presence or old benchmark shares.
Investigative limitations need explicit review and one owned next evidence action.
