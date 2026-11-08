---
title: Sketcher external reference reattachment tasks
doc_type: spec
artifact_type: tasks
status: archived
owner: local-developer
last_reviewed: 2026-11-08
---

# Tasks

## Context And Execution

Read requirements.md, design.md, canonical-context.md, traceability.md and
verification.md before implementation. Obtain task_context for the selected ID;
normally mark one implementation task in progress. This package remains draft for
implementation acceptance. D001 is locally resolved and the user accepted D002
Option A. T001–T006 implementation, qualification, review and durable promotion are complete.
Actual archive commit metadata is reconciled after commit creation.

Respect AGENTS.md and freecad-project guidance, preserve unrelated work and original
line endings. Keep the shared worktree on its current branch; do not switch, reset
or stash it. Agree file ownership for overlapping edits. Once validation is complete,
prepare the clean upstream PR branch in a separate worktree and cherry-pick only
feature commits, leaving unrelated work here untouched. No operator document mutations, MCP
repository edits, install, runtime restart, commit or public submission are requested
by creating this package. The user has authorized T001's disposable qualification
probes and explicitly accepted D002 Option A. T001 completion does not establish
new-API acceptance. The user subsequently authorized T002 production changes and
local commits. Obtain fresh task context before each subsequent slice.

Markers: `[ ]` pending, `[~]` in progress, `[/]` partial, `[>]` routed,
`[-]` deferred/no-op, `[?]` decision required, `[!]` attention, `[x]` complete.
Each completion needs concrete command, artifact, manual, reasoned or external evidence.

## Task Dependency Graph

```text
T001 -> T002 -> T003 -> T004 -> T005 -> T006
```

## Delivery Slices

- [x] T001 Qualify the API contract and native preservation mechanism.
  - Depends on: none
  - Requirements: Requirement 1 AC1, AC2, AC3; Requirement 2 AC1, AC2, AC3, AC4, AC5; Requirement 3 AC1, AC2, AC3, AC4, AC5; Requirement 4 AC1; Requirement 5 AC1
  - Properties: CP-001, CP-002, CP-005
  - Design: design.md#python-interface; design.md#admission-and-application; design.md#error-handling-and-recovery
  - Files: `src/Mod/Sketcher/App/SketchObject.h`, `src/Mod/Sketcher/App/SketchObjectExternal.cpp`, `src/Mod/Sketcher/App/SketchObjectPyImp.cpp`, `src/Mod/Sketcher/App/SketchObject.pyi`, `tests/src/Mod/Sketcher/App/CMakeLists.txt`, `design.md`, `canonical-context.md`, `verification.md`
  - Acceptance: Resolve D001/D002 using actual callers and disposable probes. Pin the scalar/index/native boundary and the admitted-or-reject orientation policy. Record the observed notification/touched/redo limitations and the user's Option A decision; reconcile preflight/no-op nonmutation, successful-operation invariants and caller-owned recovery across every artifact before T002. Qualify ordinary caller abort after a native apply exception; no generic App redesign. Pin generated circle/line/arc fixtures, source/library identities, exact nonempty native/Python runner manifests and scalar naming rationale. New implementation acceptance remains T002–T005.
  - Validation: Targeted source/reference inspection and disposable native qualification probes; reconcile requirements/design/tasks if findings change scope.
  - Evidence mode: artifact
  - Evidence: t001-qualification.md and t001-validation.json: D001 API/index/native boundary selected; user explicitly accepted D002 Option A; all affected spec artifacts reconciled. Exact eight-case native probe run passed including injected application error and explicit caller abort restoring model/earlier caller edit; six existing native tests, five Python tests and arc save/reopen fixture passed. Source/library hashes stable; package lint zero findings; 55 local links, evidence hashes and Python syntax checked. No new API implementation acceptance claimed.
  - Owner: local-developer

  - Status: Complete for T001 qualification and user-approved scope reconciliation. T001 introduced no production source, shared branch/build, install or operator-document changes. T002 now implements the native slice separately.
- [x] T002 Implement native admission and in-place identity preservation.
  - Depends on: T001
  - Requirements: Requirement 2 AC1, AC2, AC3, AC4, AC5; Requirement 3 AC1, AC2, AC3, AC4, AC5; Requirement 4 AC1; Requirement 5 AC1
  - Properties: CP-001, CP-002, CP-004, CP-005
  - Design: design.md#admission-and-application; design.md#error-handling-and-recovery; design.md#semantic-state-oracle
  - Files: `src/Mod/Sketcher/App/SketchObject.h`, `src/Mod/Sketcher/App/SketchObjectExternal.cpp`, `tests/src/Mod/Sketcher/App/SketchObjectReattach.cpp`, `tests/src/Mod/Sketcher/App/CMakeLists.txt`
  - Acceptance: Native success preserves IDs, both orders, ExternalTypes and constraints including Orientation, with coherent Ref/dependency state at return. Unsupported/reversed/ambiguous inputs reject in preflight; admitted same-source calls emit no changes. Preflight failure/no-op preserves model and journal metadata including earlier caller edits. Unexpected apply errors propagate without automatic retry/commit/abort; normal intermediate notifications and dirty/redo effects are allowed and caller recovery is documented. Qualify dedicated caller-transaction abort and no-transaction checkpoint recovery. Direct C++ use meets the accepted contract independently of Python; no generic atomic infrastructure.
  - Validation: Run focused native success, rejection and exception cases from verification.md; immediate subsequent solve/edit must work after failure. Record baseline failures before the fix where practical.
  - Evidence mode: artifact
  - Evidence: t002-implementation.md and t002-validation.json: additive native reattachExternal implemented; 13/13 new direct C++ cases and 162/162 total Sketcher cases pass, pixi run build -j 2 passes, scoped hooks/clang-format 23.1.0 and CRLF checks pass. Identity/orders/modes/constraint fields/Ref dependencies, preflight/no-op journal nonmutation and caller abort/checkpoint recovery qualified. Existing centre-Coincident solver movement issue reproduced with legacy attachExternal is routed to T004; requirements unchanged. Python remains T003.
  - Owner: local-developer

  - Status: Complete for the bounded native C++ slice. Public Python is T003; full lifecycle/Coincident solver disposition is T004. Shared branch and unrelated GUI/spec003 changes preserved.
- [x] T003 Expose and document the public Python binding.
  - Depends on: T002
  - Requirements: Requirement 1 AC1, AC2, AC3; Requirement 3 AC1, AC2, AC3, AC4, AC5; Requirement 5 AC1, AC2
  - Properties: CP-001, CP-002, CP-005
  - Design: design.md#python-interface; design.md#error-handling-and-recovery
  - Files: `src/Mod/Sketcher/App/SketchObject.pyi`, `src/Mod/Sketcher/App/SketchObjectPyImp.cpp`, `src/Mod/Sketcher/SketcherTests/TestSketchExternalReattachment.py`, `src/Mod/Sketcher/TestSketcherApp.py`, `src/Mod/Sketcher/CMakeLists.txt`
  - Acceptance: Generated method is callable with the resolved signature, returns None, documents the negative index contract, validates numeric/type boundaries and translates failures consistently. Public two-reference tests prove selected-only reattachment and unchanged unrelated geometry. No raw read-only property assignment or GUI/MCP dependency is used.
  - Validation: Build generated bindings and run the registered Python cases, including bool/extreme integer/type errors, supported no-op, Frozen/Missing/Detached, Intersection/Both, and execution-mode rejection precedence; assert discovered nonzero test counts.
  - Evidence mode: command
  - Evidence: Generated scalar binding and registered public suite implemented. completion-evidence/public-matrix.json records 16 cases under each explicit solver 0/1/2 (48 passes, no skips); numeric/type/arity/no-op/selected-only checks pass. Unsupported native flags/noRecomputes are exercised by direct C++ tests because no public state mutator exists; the binding delegates directly to the same admission helper.
  - Owner: local-developer

- [x] T004 Qualify constraints, transaction recovery and saved model behavior.
  - Depends on: T003
  - Requirements: Requirement 2 AC1, AC2, AC3, AC4, AC5; Requirement 3 AC3, AC4, AC5; Requirement 4 AC1, AC2, AC3, AC4; Requirement 5 AC2
  - Properties: CP-001, CP-002, CP-003, CP-004, CP-005
  - Design: design.md#transaction-and-lifecycle-behavior; design.md#semantic-state-oracle
  - Files: `src/Mod/Sketcher/SketcherTests/TestSketchExternalReattachment.py`, `tests/src/Mod/Sketcher/App/SketchObjectReattach.cpp`, `src/Mod/Sketcher/App/SketchObjectExternal.cpp`, `src/Mod/Sketcher/App/planegcs/GCS.cpp`, `tests/src/Mod/Sketcher/App/planegcs/GCS.cpp`, `verification.md`, `traceability.md`
  - Acceptance: Two projected circles/arcs retain IDs/roles/modes across success, caller abort, undo/redo and save/reopen. Admitted direction preserves exact Orientation through two recomputes; reversed/ambiguous direction rejects in preflight. Verify ExternalTypes and dependency state at successful/recovered boundaries, including old-source deletion. Preflight/no-op preserve dirty/transaction/redo metadata in no-transaction, active/booked and earlier same-property cases. Unexpected apply errors leave the caller boundary open for explicit abort or checkpoint recovery; abort of a shared transaction also reverts earlier edits. Intermediate notifications are permitted; final success/recovered state is coherent and usable for subsequent solve/edit.
  - Validation: Execute V01 through V09 and V12 through V15 in verification.md using disposable files and explicit tolerances; record exact test names, build/source identities and results.
  - Evidence mode: command
  - Evidence: t004-qualification.md and completion evidence: all 163 native cases pass, including 13 feature cases with exact Distance/Tangent Orientation and authored-mode persistence; 16 public cases pass under all three solvers; full TestSketcherApp 113 run with one existing secant skip. Centre-Coincident false success resolved by separately reviewed GCS status fix 42144bc1e9 and before-fix/recovery regression. Cross-object abort/replay/reopen, endpoint roles, source propagation and caller-owned exception recovery qualify the accepted contract.
  - Owner: local-developer

  - Status: T004 resolves the T002 finding through the separately reviewed failed-subsystem status fix (42144bc1e9), with a before-fix regression and passing geometry assertions under all three public solver selections. See t004-qualification.md; requirements unchanged.

- [x] T005 Validate compatibility and resolve specialist review findings.
  - Depends on: T004
  - Requirements: Requirement 1 AC2; Requirement 2 AC1, AC2, AC3, AC4, AC5; Requirement 3 AC1, AC2, AC3, AC4, AC5; Requirement 4 AC1, AC2, AC3, AC4; Requirement 5 AC1, AC2, AC3, AC4
  - Properties: CP-001, CP-002, CP-003, CP-004, CP-005
  - Design: design.md#migration-and-compatibility; design.md#validation-strategy
  - Files: `verification.md`, `traceability.md`, `requirements.md`, `design.md`, `src/Mod/Sketcher/App/SketchObjectExternal.cpp`, `src/Mod/Sketcher/App/SketchObjectPyImp.cpp`, `src/Mod/Sketcher/App/SketchObject.pyi`
  - Acceptance: Existing external geometry/transaction tests and required build/broader checks pass or have explicit blocking dispositions. Review actual implementation for native lifetime, exceptions, endpoint semantics, property notifications and legacy behavior. Record exact intended/discovered/executed nonzero case counts, platform/old-document limits and public-API review status without inferring maintainer acceptance.
  - Validation: `pixi run build -j 2`, focused native/Python tests, `pixi run test`, applicable pre-commit and changed-file checks; broader/cross-platform evidence or explicit remaining blockers. No redundant reruns absent new findings.
  - Evidence mode: artifact
  - Evidence: completion-validation.json and completion-evidence/: pixi run build -j 2 passed; 163/163 native cases pass; public 16 cases x solver 0/1/2 = 48 passes; full TestSketcherApp 113 run with one existing skip and no failures/errors. Full CTest 2434 registered: 2422 pass, two fail, two skip, eight disabled; independent Measurement/Part-only selector failures are B012/B013. All four specialist findings addressed in docs/reviews/sketcher-reattachment-delivery-review.md. Scoped hooks, region clang-format23.1.0/Black100, CRLF checks and package/reference lint pass. B014 owns external human/platform/upstream delivery.
  - Owner: local-developer

  - Status: Complete for local compatibility/validation and review. Whole-project CTest is explicitly non-green; unrelated failures and external delivery gates have named owners/destinations.
- [x] T006 Promote the accepted API contract and record closure.
  - Depends on: T005
  - Requirements: Requirement 1 AC2; Requirement 5 AC3, AC4
  - Properties: CP-005
  - Design: design.md#operational-considerations; requirements.md#durable-impact
  - Files: `src/Mod/Sketcher/App/SketchObject.pyi`, `docs/reference/sketcher-external-reference-reattachment.md`, `docs/specs/README.md`, `docs/history/spec-closure-log.md`, `docs/history/spec-archive-index.md`, `docs/backlog/README.md`, `docs/backlog/reattachment-delivery-follow-ups.md`, `docs/reviews/sketcher-reattachment-delivery-review.md`, `verification.md`, `traceability.md`
  - Acceptance: Durable reference and binding docs describe implemented supported cases, errors, indexing and caller transaction/recompute example. Every requirement/property has evidence or an explicit owned disposition. Record actual source/docs commit identities, package disposition and external integration boundary. Closure does not claim bridge delivery, snapshot activation or live D600 retargeting.
  - Validation: Package lint, traceability review, local link/whitespace checks and lifecycle closure checks; human review and disclosure obligations before any separately authorized public submission.
  - Evidence mode: artifact
  - Evidence: Promoted docs/reference/sketcher-external-reference-reattachment.md and generated SketchObject.pyi; completed durable private review and owned B012-B014 follow-ups. Root implementation commit 96924c50a2, solver fix 42144bc1e9 and nested docs promotion 6bfa619 exist. Requirements/properties, V01-V15, exact test/hash receipts, package/reference lint and 88 local links reconciled. Chosen disposition is retained-as-history; actual cleanup commit references are reconciled after archive commit creation, without claiming a future hash.
  - Owner: local-developer

  - Status: Complete local implementation, qualification, promotion and acceptance. Archive commit self-reference bookkeeping follows completion; B012-B014 remain explicitly owned external delivery work.
## Related Artifacts

- [Requirements](requirements.md)
- [Design](design.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Canonical context](canonical-context.md)
