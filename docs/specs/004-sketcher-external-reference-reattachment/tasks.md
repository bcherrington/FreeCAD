---
title: Sketcher external reference reattachment tasks
doc_type: spec
artifact_type: tasks
status: draft
owner: local-developer
last_reviewed: 2026-11-07
---

# Tasks

## Context And Execution

Read requirements.md, design.md, canonical-context.md, traceability.md and
verification.md before implementation. Obtain task_context for the selected ID;
normally mark one implementation task in progress. This package is draft: only
T001 contract qualification is ready to start. All implementation evidence is pending.

Respect AGENTS.md and freecad-project guidance, preserve unrelated work and original
line endings, and use a named source branch. No operator document mutations, MCP
repository edits, install, runtime restart, commit or public submission are requested
by creating this package. Source execution requires the user's implementation instruction.

Markers: `[ ]` pending, `[~]` in progress, `[/]` partial, `[>]` routed,
`[-]` deferred/no-op, `[?]` decision required, `[!]` attention, `[x]` complete.
Each completion needs concrete command, artifact, manual, reasoned or external evidence.

## Task Dependency Graph

```text
T001 -> T002 -> T003 -> T004 -> T005 -> T006
```

## Delivery Slices

- [ ] T001 Qualify the API contract and native preservation mechanism.
  - Depends on: none
  - Requirements: Requirement 1 AC1, AC2, AC3; Requirement 2 AC1, AC2, AC3, AC4; Requirement 3 AC1, AC2, AC3, AC4; Requirement 4 AC1; Requirement 5 AC1
  - Properties: CP-001, CP-002, CP-005
  - Design: design.md#python-interface; design.md#admission-and-application; design.md#error-handling-and-atomicity
  - Files: `src/Mod/Sketcher/App/SketchObject.h`, `src/Mod/Sketcher/App/SketchObjectExternal.cpp`, `src/Mod/Sketcher/App/SketchObjectPyImp.cpp`, `src/Mod/Sketcher/App/SketchObject.pyi`, `tests/src/Mod/Sketcher/App/CMakeLists.txt`, `design.md`, `canonical-context.md`, `verification.md`
  - Acceptance: Resolve D001 and D002 with actual callers, index spaces, projected orientation, property notification/transaction boundaries and a viable strong failure guarantee. Pin minimal circle/arc/line fixtures, exact native test location and commands. Record why a strict helper/overload preserves legacy behavior. No unsupported preservation claim remains implicit.
  - Validation: Targeted source/reference inspection and disposable native qualification probes; reconcile requirements/design/tasks if findings change scope.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer

- [ ] T002 Implement native admission and in-place identity preservation.
  - Depends on: T001
  - Requirements: Requirement 2 AC1, AC2, AC3, AC4; Requirement 3 AC1, AC2, AC3, AC4; Requirement 4 AC1; Requirement 5 AC1
  - Properties: CP-001, CP-002, CP-004, CP-005
  - Design: design.md#admission-and-application; design.md#error-handling-and-atomicity; design.md#semantic-state-oracle
  - Files: `src/Mod/Sketcher/App/SketchObject.h`, `src/Mod/Sketcher/App/SketchObjectExternal.cpp`, `tests/src/Mod/Sketcher/App/SketchObject.cpp`, `tests/src/Mod/Sketcher/App/CMakeLists.txt`
  - Acceptance: Native success preserves IDs, both orders, authored flags and constraints. Invalid/grouped/duplicate/degenerate/type-changing requests reject, exact same source is a clean no-op, and injected application failures restore coherent state without undoing preceding caller edits. Direct C++ use meets these guarantees independently of Python.
  - Validation: Run focused native success, rejection and exception cases from verification.md; immediate subsequent solve/edit must work after failure. Record baseline failures before the fix where practical.
  - Evidence mode: command
  - Evidence: Pending.
  - Owner: local-developer

- [ ] T003 Expose and document the public Python binding.
  - Depends on: T002
  - Requirements: Requirement 1 AC1, AC2, AC3; Requirement 3 AC1, AC2, AC3, AC4; Requirement 5 AC1, AC2
  - Properties: CP-001, CP-002, CP-005
  - Design: design.md#python-interface; design.md#error-handling-and-atomicity
  - Files: `src/Mod/Sketcher/App/SketchObject.pyi`, `src/Mod/Sketcher/App/SketchObjectPyImp.cpp`, `src/Mod/Sketcher/SketcherTests/TestSketchExternalReattachment.py`, `src/Mod/Sketcher/TestSketcherApp.py`, `src/Mod/Sketcher/CMakeLists.txt`
  - Acceptance: Generated method is callable with the resolved signature, returns None, documents the negative index contract, validates numeric/type boundaries and translates failures consistently. Public two-reference tests prove selected-only reattachment and unchanged unrelated geometry. No raw read-only property assignment or GUI/MCP dependency is used.
  - Validation: Build generated bindings and run the registered Python cases, including bool/extreme integer/type errors, no-op and unsupported inputs.
  - Evidence mode: command
  - Evidence: Pending.
  - Owner: local-developer

- [ ] T004 Qualify constraints, transaction recovery and saved model behavior.
  - Depends on: T003
  - Requirements: Requirement 2 AC1, AC2, AC3, AC4; Requirement 3 AC3, AC4; Requirement 4 AC1, AC2, AC3, AC4; Requirement 5 AC2
  - Properties: CP-001, CP-002, CP-003, CP-004, CP-005
  - Design: design.md#transaction-and-lifecycle-behavior; design.md#semantic-state-oracle
  - Files: `src/Mod/Sketcher/SketcherTests/TestSketchExternalReattachment.py`, `tests/src/Mod/Sketcher/App/SketchObject.cpp`, `verification.md`, `traceability.md`
  - Acceptance: Two projected circles/arcs with dependent constraints retain roles across one selector replacement, caller abort, undo/redo and separate before/after save/reopen. Line/arc reversed orientation is either demonstrably mapped or cleanly rejected. New source changes propagate; old and unrelated sources have no unintended influence. Failure preserves earlier edits in the same transaction and subsequent solve/edit works.
  - Validation: Execute V01 through V09 in verification.md using disposable files and explicit tolerances; record exact test names, build/source identities and results.
  - Evidence mode: command
  - Evidence: Pending.
  - Owner: local-developer

- [ ] T005 Validate compatibility and resolve specialist review findings.
  - Depends on: T004
  - Requirements: Requirement 1 AC2; Requirement 2 AC1, AC2, AC3, AC4; Requirement 3 AC1, AC2, AC3, AC4; Requirement 4 AC1, AC2, AC3, AC4; Requirement 5 AC1, AC2, AC3, AC4
  - Properties: CP-001, CP-002, CP-003, CP-004, CP-005
  - Design: design.md#migration-and-compatibility; design.md#validation-strategy
  - Files: `verification.md`, `traceability.md`, `requirements.md`, `design.md`, `src/Mod/Sketcher/App/SketchObjectExternal.cpp`, `src/Mod/Sketcher/App/SketchObjectPyImp.cpp`, `src/Mod/Sketcher/App/SketchObject.pyi`
  - Acceptance: Existing external geometry/transaction tests and required build/broader checks pass or have explicit blocking dispositions. Review actual implementation for native lifetime, exceptions, endpoint semantics, property notifications and legacy behavior. Record platform/old-document limits and public-API review status without inferring maintainer acceptance.
  - Validation: `pixi run build -j 2`, focused native/Python tests, `pixi run test`, applicable pre-commit and changed-file checks; broader/cross-platform evidence or explicit remaining blockers. No redundant reruns absent new findings.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer

- [ ] T006 Promote the accepted API contract and record closure.
  - Depends on: T005
  - Requirements: Requirement 1 AC2; Requirement 5 AC3, AC4
  - Properties: CP-005
  - Design: design.md#operational-considerations; requirements.md#durable-impact
  - Files: `src/Mod/Sketcher/App/SketchObject.pyi`, `docs/reference/sketcher-external-reference-reattachment.md`, `docs/specs/README.md`, `docs/history/spec-closure-log.md`, `docs/history/spec-archive-index.md`, `verification.md`, `traceability.md`
  - Acceptance: Durable reference and binding docs describe implemented supported cases, errors, indexing and caller transaction/recompute example. Every requirement/property has evidence or an explicit owned disposition. Record actual source/docs commit identities, package disposition and external integration boundary. Closure does not claim bridge delivery, snapshot activation or live D600 retargeting.
  - Validation: Package lint, traceability review, local link/whitespace checks and lifecycle closure checks; human review and disclosure obligations before any separately authorized public submission.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer

## Related Artifacts

- [Requirements](requirements.md)
- [Design](design.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Canonical context](canonical-context.md)
