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
requirement was implemented by creation of this package; T002 now implements the
native slice, recorded in t002-implementation.md. See requirements.md for
canonical acceptance and verification.md for executed evidence and pending gates.

## Task To Context Matrix

| Task ID | Requirements | Acceptance Criteria | Design Sections | Verification | Durable Targets | Open Decisions |
| --- | --- | --- | --- | --- | --- | --- |
| T001 | Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5 | R1 AC1–AC3; R2 AC1–AC5; R3 AC1–AC5; R4 AC1; R5 AC1 | design.md#python-interface; design.md#admission-and-application | V01–V05, V12–V15; qualification record | API declaration and durable reference | none |
| T002 | Requirement 2, Requirement 3, Requirement 4, Requirement 5 | R2 AC1–AC5; R3 AC1–AC5; R4 AC1; R5 AC1 | design.md#admission-and-application; design.md#error-handling-and-recovery | V01–V05, V07, V12–V15 | Durable reference | none |
| T003 | Requirement 1, Requirement 3, Requirement 5 | R1 AC1–AC3; R3 AC1–AC5; R5 AC1, AC2 | design.md#python-interface | V01, V03, V04, V09, V13 | SketchObject.pyi | none |
| T004 | Requirement 2, Requirement 3, Requirement 4, Requirement 5 | R2 AC1–AC5; R3 AC3, AC4, AC5; R4 AC1–AC4; R5 AC2 | design.md#transaction-and-lifecycle-behavior; design.md#semantic-state-oracle | V01–V09, V12–V15 | Durable reference examples | none |
| T005 | Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5 | R1 AC2; R2 AC1–AC5; R3 AC1–AC5; R4 AC1–AC4; R5 AC1–AC4 | design.md#migration-and-compatibility; design.md#validation-strategy | V09, V10, V12–V15 and review dispositions | API declaration and durable reference | public API review |
| T006 | Requirement 1, Requirement 5 | R1 AC2; R5 AC3, AC4 | design.md#operational-considerations; requirements.md#durable-impact | V11 | Durable reference, closure log and archive index | cleanup disposition |

## Requirement To Delivery Matrix

All requirements are must-have. The durable contract target is
`docs/reference/sketcher-external-reference-reattachment.md`; generated API
documentation comes from `src/Mod/Sketcher/App/SketchObject.pyi`.

| Requirement | Priority | Acceptance Criteria | Design Sections | Tasks | Verification | Coverage State | Residual Destination |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Requirement 1 | must-have | AC1, AC2, AC3 | design.md#python-interface | T001, T003, T005, T006 | V01, V03, V09, V11 | not-covered | T003 implementation and T006 promotion |
| Requirement 2 | must-have | AC1, AC2, AC3, AC4, AC5 | design.md#admission-and-application; design.md#semantic-state-oracle | T001, T002, T004, T005 | V01, V02, V06, V07, V08, V12, V15 | partial-blocking | T004 lifecycle/Coincident solver qualification and T005 review |
| Requirement 3 | must-have | AC1, AC2, AC3, AC4, AC5 | design.md#error-handling-and-recovery | T001, T002, T003, T004, T005 | V03, V04, V05, V09, V12–V14 | partial-blocking | T003 Python validation and T004 lifecycle qualification |
| Requirement 4 | must-have | AC1, AC2, AC3, AC4 | design.md#transaction-and-lifecycle-behavior | T001, T002, T004, T005 | V01, V05, V06, V07, V08, V13–V15 | partial-blocking | T004 lifecycle qualification |
| Requirement 5 | must-have | AC1, AC2, AC3, AC4 | design.md#migration-and-compatibility; design.md#validation-strategy | T001, T002, T003, T004, T005, T006 | V09, V10, V11 | partial-blocking | T005 validation and T006 promotion |

## Correctness Property Coverage

| Property | Requirements | Design Sections | Tasks | Tests Or Verification | Residual Risk |
| --- | --- | --- | --- | --- | --- |
| CP-001 | Requirement 2 | design.md#admission-and-application | T001, T002, T003, T004, T005 | V01, V02, V06, V08, V15 | T002 native return invariants tested; full lifecycle/Coincident solver qualification remains T004 |
| CP-002 | Requirement 3 | design.md#error-handling-and-recovery | T001, T002, T003, T004, T005 | V03, V04, V05, V12–V14 | Native rejection/no-op and caller recovery tested in T002; Python and full lifecycle checks remain T003/T004 |
| CP-003 | Requirement 4 | design.md#transaction-and-lifecycle-behavior | T004, T005 | V05, V06, V08, V14 | Persistence/transaction tests pending |
| CP-004 | Requirement 2, Requirement 4 | design.md#semantic-state-oracle | T002, T004, T005 | V01, V07, V08, V15 | Native Ref/dependencies and old-source deletion tested in T002; save/reopen/undo/redo success checks remain T004 |
| CP-005 | Requirement 1, Requirement 4, Requirement 5 | design.md#migration-and-compatibility | T001, T002, T003, T004, T005, T006 | V05, V09, V10, V11, V13, V14 | Existing callers and platforms require checks |

## Design To Implementation Matrix

| Design Section | Requirements | Tasks | Interfaces Or Files | Verification | Coverage State | Residual Destination |
| --- | --- | --- | --- | --- | --- | --- |
| Python interface | Requirement 1, Requirement 3 | T001, T003 | SketchObject.pyi; SketchObjectPyImp.cpp | V01, V03, V09 | not-covered | T003 |
| Admission and ordinary transaction application | Requirement 2, Requirement 3 | T001, T002 | SketchObject.h; SketchObjectExternal.cpp | V01–V05, V12–V15 | not-covered | T002 |
| Transaction, persistence and state oracle | Requirement 2, Requirement 3, Requirement 4 | T004 | Native and public Python fixtures | V05–V08, V12–V15 | not-covered | T004 |
| Compatibility and delivery | Requirement 5 | T005, T006 | Existing tests; durable reference and lifecycle records | V09–V11 | not-covered | T005, T006 |

## Open Decision Impact

| Decision ID | Blocks | Affected Requirements | Affected Tasks | Resolution Needed |
| --- | --- | --- | --- | --- |
| D001 (resolved locally) | no remaining local API decision | Requirement 1, Requirement 3, Requirement 5 | T001, T002, T003 | reattachExternal scalar negative-index interface selected; native T002 implemented; Python/public review pending |
| D002 (resolved by user) | no remaining local design decision | Requirement 2, Requirement 3, Requirement 4 | T001, T002, T004 | Option A accepted: unchanged preflight/no-op, success preservation, ordinary notifications and caller-owned apply-error recovery |

## Maintenance Notes

MoE findings F001–F008 map to acceptance, tasks and cases in
[the review disposition](verification.md#moe-review-disposition). Addressed-in-spec
did not itself change not-covered implementation states or resolve D001/D002.

T001 subsequently executed baseline/fixture qualification: see t001-validation.json
and [the report](t001-qualification.md). D001 is locally resolved and the user
accepted D002 Option A. T001 qualification/reconciliation is complete; existing
behavior observations do not advance new-API requirement coverage to complete.

Update both directions when scope, criteria or tests change. V01–V15 identify
planned verification cases, not completed evidence. Every not-covered row remains
a closure blocker until accepted evidence or an explicit reviewed disposition exists.

## T002 Native Evidence

[t002-implementation.md](t002-implementation.md) maps all thirteen direct C++
cases to the bounded native contract. [t002-validation.json](t002-validation.json)
pins the discovered/executed nonzero manifests, source/build hashes and raw
receipts. V01–V05, V07 and V12–V15 now have bounded native evidence, not complete
public/lifecycle coverage. Requirements 2–5 remain partial-blocking until later
tasks finish. The branch baseline centre-Coincident solver finding is explicitly
owned by T004 and does not waive any requirement.
