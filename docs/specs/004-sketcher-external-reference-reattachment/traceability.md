---
title: Sketcher external reference reattachment traceability
doc_type: spec
artifact_type: traceability
status: draft
owner: local-developer
last_reviewed: 2026-11-07
---

# Traceability Matrix

## Purpose

Connect proposed requirements to design, tasks, validation and promotion. No
requirement is implemented by creation of this package. See requirements.md for
canonical acceptance and verification.md for executed evidence and pending gates.

## Task To Context Matrix

| Task ID | Requirements | Acceptance Criteria | Design Sections | Verification | Durable Targets | Open Decisions |
| --- | --- | --- | --- | --- | --- | --- |
| T001 | Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5 | R1 AC1–AC3; R2 AC1–AC4; R3 AC1–AC4; R4 AC1; R5 AC1 | design.md#python-interface; design.md#admission-and-application | V01, V02, V03, V04; qualification record | API declaration and durable reference | D001, D002 |
| T002 | Requirement 2, Requirement 3, Requirement 4, Requirement 5 | R2 AC1–AC4; R3 AC1–AC4; R4 AC1; R5 AC1 | design.md#admission-and-application; design.md#error-handling-and-atomicity | V01–V05, V07 | Durable reference | resolved D001, D002 |
| T003 | Requirement 1, Requirement 3, Requirement 5 | R1 AC1–AC3; R3 AC1–AC4; R5 AC1, AC2 | design.md#python-interface | V01, V03, V04, V09 | SketchObject.pyi | resolved D001 |
| T004 | Requirement 2, Requirement 3, Requirement 4, Requirement 5 | R2 AC1–AC4; R3 AC3, AC4; R4 AC1–AC4; R5 AC2 | design.md#transaction-and-lifecycle-behavior; design.md#semantic-state-oracle | V01–V09 | Durable reference examples | none after T001 |
| T005 | Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5 | R1 AC2; R2 AC1–AC4; R3 AC1–AC4; R4 AC1–AC4; R5 AC1–AC4 | design.md#migration-and-compatibility; design.md#validation-strategy | V09, V10 and review dispositions | API declaration and durable reference | public API review |
| T006 | Requirement 1, Requirement 5 | R1 AC2; R5 AC3, AC4 | design.md#operational-considerations; requirements.md#durable-impact | V11 | Durable reference, closure log and archive index | cleanup disposition |

## Requirement To Delivery Matrix

All requirements are must-have. The durable contract target is
`docs/reference/sketcher-external-reference-reattachment.md`; generated API
documentation comes from `src/Mod/Sketcher/App/SketchObject.pyi`.

| Requirement | Priority | Acceptance Criteria | Design Sections | Tasks | Verification | Coverage State | Residual Destination |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Requirement 1 | must-have | AC1, AC2, AC3 | design.md#python-interface | T001, T003, T005, T006 | V01, V03, V09, V11 | not-covered | T003 implementation and T006 promotion |
| Requirement 2 | must-have | AC1, AC2, AC3, AC4 | design.md#admission-and-application; design.md#semantic-state-oracle | T001, T002, T004, T005 | V01, V02, V06, V07, V08 | not-covered | T002 native implementation and T004 qualification |
| Requirement 3 | must-have | AC1, AC2, AC3, AC4 | design.md#error-handling-and-atomicity | T001, T002, T003, T004, T005 | V03, V04, V05, V09 | not-covered | T002 safeguards and T004 qualification |
| Requirement 4 | must-have | AC1, AC2, AC3, AC4 | design.md#transaction-and-lifecycle-behavior | T001, T002, T004, T005 | V01, V05, V06, V07, V08 | not-covered | T004 lifecycle qualification |
| Requirement 5 | must-have | AC1, AC2, AC3, AC4 | design.md#migration-and-compatibility; design.md#validation-strategy | T001, T002, T003, T004, T005, T006 | V09, V10, V11 | not-covered | T005 validation and T006 promotion |

## Correctness Property Coverage

| Property | Requirements | Design Sections | Tasks | Tests Or Verification | Residual Risk |
| --- | --- | --- | --- | --- | --- |
| CP-001 | Requirement 2 | design.md#admission-and-application | T001, T002, T003, T004, T005 | V01, V02, V06, V08 | Native semantics not yet qualified |
| CP-002 | Requirement 3 | design.md#error-handling-and-atomicity | T001, T002, T003, T004, T005 | V03, V04, V05 | Application failure restoration unproven |
| CP-003 | Requirement 4 | design.md#transaction-and-lifecycle-behavior | T004, T005 | V05, V06, V08 | Persistence/transaction tests pending |
| CP-004 | Requirement 2, Requirement 4 | design.md#semantic-state-oracle | T002, T004, T005 | V01, V07, V08 | Ref/source propagation unproven |
| CP-005 | Requirement 1, Requirement 4, Requirement 5 | design.md#migration-and-compatibility | T001, T002, T003, T004, T005, T006 | V05, V09, V10, V11 | Existing callers and platforms require checks |

## Design To Implementation Matrix

| Design Section | Requirements | Tasks | Interfaces Or Files | Verification | Coverage State | Residual Destination |
| --- | --- | --- | --- | --- | --- | --- |
| Python interface | Requirement 1, Requirement 3 | T001, T003 | SketchObject.pyi; SketchObjectPyImp.cpp | V01, V03, V09 | not-covered | T003 |
| Admission and atomic application | Requirement 2, Requirement 3 | T001, T002 | SketchObject.h; SketchObjectExternal.cpp | V01–V05 | not-covered | T002 |
| Transaction, persistence and state oracle | Requirement 2, Requirement 3, Requirement 4 | T004 | Native and public Python fixtures | V05–V08 | not-covered | T004 |
| Compatibility and delivery | Requirement 5 | T005, T006 | Existing tests; durable reference and lifecycle records | V09–V11 | not-covered | T005, T006 |

## Open Decision Impact

| Decision ID | Blocks | Affected Requirements | Affected Tasks | Resolution Needed |
| --- | --- | --- | --- | --- |
| D001 | implementation | Requirement 1, Requirement 3, Requirement 5 | T001, T002, T003 | Confirm proposed scalar signature, negative indexing and native helper compatibility |
| D002 | implementation | Requirement 2, Requirement 3, Requirement 4 | T001, T002, T004 | Prove orientation mapping and staged atomic application/notification boundary |

## Maintenance Notes

Update both directions when scope, criteria or tests change. V01–V11 identify
planned verification cases, not completed evidence. Every not-covered row remains
a closure blocker until accepted evidence or an explicit reviewed disposition exists.
