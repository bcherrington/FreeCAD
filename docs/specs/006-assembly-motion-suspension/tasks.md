---
title: Assembly motion suspension tasks
doc_type: spec
artifact_type: tasks
status: archived
owner: local-developer
last_reviewed: 2026-11-12
---

# Tasks

## Context And Execution

Read requirements, design, traceability and verification. Source branch:
`feature/assembly-motion-suspension`, base `3a53ef2ce7`; docs branch:
`spec/assembly-motion-suspension`, base `87965c6`. Existing dirty docs checkout is untouched.
Commit reviewable slices with Assisted-by disclosure. No push, merge, install or public submission.

## Task Dependency Graph

T001 -> T002 -> T003 -> T004 -> T005

## Delivery Slices

- [x] T001 Review and reconcile the specification with FreeCAD experts.
  - Depends on: none
  - Requirements: Requirement 1 AC1–AC3; Requirement 2 AC1–AC3; Requirement 3 AC1–AC3; Requirement 4 AC3
  - Files: requirements.md, design.md, review.md, traceability.md, verification.md
  - Acceptance: Native, Python/workbench and validation experts review artifacts; blocking findings have recorded fixes.
  - Validation: Independent reviewer messages and source reconciliation.
  - Evidence mode: artifact
  - Evidence: artifact: review.md records native, Python/workbench and validation expert artifact reviews; N1/N2/V1/V2/V3 blockers resolved in requirements/design, including finite admission matrix, pre-change mode rejection and native observation oracle.

- [x] T002 Implement native scope, bindings and motion gates.
  - Depends on: T001
  - Requirements: Requirement 1 AC1–AC3; Requirement 2 AC1, AC3; Requirement 4 AC1
  - Properties: CP-001, CP-002, CP-004
  - Design: design.md#motion-gates
  - Files: src/Mod/Assembly/App/MotionSuspension.h, MotionSuspension.cpp, AppAssemblyPy.cpp, CMakeLists.txt, AssemblyObject.cpp, AssemblyObject.h, tests/src/Mod/Assembly/App/AssemblyObject.cpp
  - Acceptance: Native/Python scopes nest and close safely; automatic solve/status/write paths are blocked before effects.
  - Validation: Native lifetime and implicit-motion tests; pixi build.
  - Evidence mode: command
  - Evidence: command: pixi run build -j 2 exit 0; native scope/isolation/exception/close/restore tests pass within 10/10; GDB guarded phase preparation=0 and writes=0 with positive controls. Production f0dd422402, tests a73012a8cd; evidence/validation-results.json.

- [x] T003 Implement callback/link guards and validated explicit solve.
  - Depends on: T002
  - Requirements: Requirement 2 AC1–AC3; Requirement 3 AC1–AC3
  - Properties: CP-002, CP-003, CP-004
  - Design: design.md#explicit-solve-and-ownership
  - Files: src/Mod/Assembly/JointObject.py, App/AssemblyLink.cpp, App/AssemblyObject.cpp, App/AssemblyObject.h, App/AssemblyObject.pyi, App/AssemblyObjectPyImp.cpp, AssemblyTests/TestCore.py
  - Acceptance: Complete supported graph rejects before motion; safe explicit solve works with automatic guards active; link policy is tested.
  - Validation: Native and Python callback, valid-link, escaped-reference and mode/copy tests.
  - Evidence mode: command
  - Evidence: command: pixi run ctest --test-dir build/debug -R '^AssemblyObjectTest\.' --output-on-failure passed 10/10; isolated GUI AssemblyTests.TestCore passed 39/39. Production f0dd422402; tests a73012a8cd. evidence/native-tests.log, evidence/python-core-tests.log and evidence/validation-results.json record generated remapping, callbacks, failed admission, safe explicit solve and deferred grounding results.

- [x] T004 Validate implementation and resolve final expert findings.
  - Depends on: T003
  - Requirements: Requirement 4 AC1–AC3; Requirement 1 AC1–AC3; Requirement 2 AC1–AC3; Requirement 3 AC1–AC3
  - Files: tests/src/Mod/Assembly/App/AssemblyObject.cpp, src/Mod/Assembly/AssemblyTests/TestCore.py, review.md, verification.md
  - Acceptance: Focused native/Python tests and build pass; final reviewers inspect code; findings resolved or honestly block completion.
  - Validation: pixi run build -j 2; native/Python Assembly tests; applicable pre-commit and diff checks.
  - Evidence mode: command
  - Evidence: command: native10/10, Python core39/39, GDB three cases and probe passed, applicable pre-commit and diff checks passed; final independent lifetime review resolved I001-I006. Broad suite59/61 has only independently reproduced unchanged E001/B015 mock errors, explicitly excepted and routed; verification.md records exact identities/limits.

- [x] T005 Promote API reference and reconcile delivery state.
  - Depends on: T004
  - Requirements: Requirement 4 AC3
  - Files: reference/assembly-motion-suspension.md, reviews/assembly-motion-suspension.md, verification.md, traceability.md, history/spec-closure-log.md, history/spec-archive-index.md
  - Acceptance: Durable docs match tested behavior; residuals and actual commits recorded; no unsupported bridge/platform qualification claims.
  - Validation: Lifecycle evidence, promotion and closure checks.
  - Evidence mode: artifact
  - Evidence: artifact: docs commit 4b6a821 promotes reference/assembly-motion-suspension.md and reviews/assembly-motion-suspension.md; source f0dd422402/a73012a8cd recorded, B015/B016 own residuals, history records retain package in place. verification.md and evidence/validation-results.json record exact local identity and exception E001; final metadata commit hash is resolved after creation.
