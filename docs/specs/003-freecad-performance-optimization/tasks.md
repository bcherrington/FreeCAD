---
title: FreeCAD performance optimization tasks
doc_type: spec
artifact_type: tasks
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Tasks

## Context And Execution

Read requirements.md, design.md, traceability.md and verification.md before
implementation. Task IDs below are authoritative; historical checklists/plan
epics are migration inputs only. `[?]` needs a decision, `[/]` is partial,
`[ ]` is pending. Mark one selected implementation task `[~]` before coding.
Completion requires its acceptance and evidence; imported results stay dated.

## Task Dependency Graph

```text
none -> T001
none -> T002
none -> T003
T001, T002, T003 -> T004
none -> T005
none -> T006
none -> T007
T004 -> T008
none -> T009
T002, T003, T004, T005, T006, T007 -> T010
T008, T009, T010 -> T011
```

## Delivery Slices

Parent tasks are acceptance checkpoints. Execute their numbered subtasks as
bounded slices; subtask file/requirement scope is inherited explicitly. Preserve
parent evidence as partial context, and complete a parent only after all declared
outcomes are verified. A completed narrow subtask does not close the full linked
requirement by itself.

- [?] T001 Fix the current benchmark protocol and baseline revisions.
  - Depends on: none
  - Requirements: Requirement 3 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `verification.md`, `research.md`
  - Acceptance: D001 records exact baseline/current commits, build modes, available fixtures, run
    counts, cold/warm policy and repeatable harness; no removed switch is assumed.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: reasoned
  - Evidence: May timings and transient scripts are historical; a comparable current protocol is
    absent.
  - Decision owner: local-developer
  - Open decisions: D001

- [/] T002 Verify included-file restore compatibility on the target revision.
  - Depends on: none
  - Requirements: Requirement 1 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/App/PropertyFile.cpp`, `src/Base/Reader.cpp`, `tests/src/App/PropertyFile.cpp`,
    `tests/src/Base/Reader.cpp`
  - Acceptance: Empty/binary/multi-buffer/multi-entry bytes, permissions, notifications and
    read/write/close failure behavior pass focused tests and archive compatibility checks.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: Buffered copy/error checks are present; imported Nov 4 tests and scratch roundtrips
    cover a subset, not all compatibility criteria.
  - Status: partial - implementation exists; full declared acceptance remains open.
  - Properties: CP-001
  - [ ] T002.1 Verify empty, binary, multi-buffer and multi-entry restored bytes and ZIP boundaries.
    - Depends on: none
    - Requirements: Requirement 1 (all acceptance criteria)
    - Files: `src/App/PropertyFile.cpp`, `src/Base/Reader.cpp`, `tests/src/App/PropertyFile.cpp`, `tests/src/Base/Reader.cpp`
    - Acceptance: Verify empty, binary, multi-buffer and multi-entry restored bytes and ZIP boundaries.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T002.2 Verify property notifications, read-only destination behavior and read/write/close failures.
    - Depends on: T002.1
    - Requirements: Requirement 1 (all acceptance criteria)
    - Files: `src/App/PropertyFile.cpp`, `src/Base/Reader.cpp`, `tests/src/App/PropertyFile.cpp`, `tests/src/Base/Reader.cpp`
    - Acceptance: Verify property notifications, read-only destination behavior and read/write/close failures.
    - Evidence mode: artifact
    - Evidence: Pending.

- [/] T003 Complete bitmap and rendered image correctness coverage.
  - Depends on: none
  - Requirements: Requirement 2 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/Gui/BitmapFactory.cpp`, `tests/src/Gui/BitmapFactory.cpp`
  - Acceptance: Four direct formats plus premultiplied/uncommon/SVG cases preserve orientation,
    alpha, color, scale and ownership, including rendered image planes.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: Four direct-format tests exist; full fallback/rendered visual matrix remains open.
  - Status: partial - implementation exists; full declared acceptance remains open.
  - Properties: CP-002
  - [ ] T003.1 Verify direct conversion for RGB32, ARGB32, RGBA8888 and RGBX8888.
    - Depends on: none
    - Requirements: Requirement 2 (all acceptance criteria)
    - Files: `src/Gui/BitmapFactory.cpp`, `tests/src/Gui/BitmapFactory.cpp`
    - Acceptance: Verify direct conversion for RGB32, ARGB32, RGBA8888 and RGBX8888.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T003.2 Verify premultiplied, indexed, grayscale, high-depth and SVG-derived fallback results.
    - Depends on: T003.1
    - Requirements: Requirement 2 (all acceptance criteria)
    - Files: `src/Gui/BitmapFactory.cpp`, `tests/src/Gui/BitmapFactory.cpp`
    - Acceptance: Verify premultiplied, indexed, grayscale, high-depth and SVG-derived fallback results.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T003.3 Record rendered-plane orientation, color, alpha, scale and texture ownership checks.
    - Depends on: T003.2
    - Requirements: Requirement 2 (all acceptance criteria)
    - Files: `src/Gui/BitmapFactory.cpp`, `tests/src/Gui/BitmapFactory.cpp`
    - Acceptance: Record rendered-plane orientation, color, alpha, scale and texture ownership checks.
    - Evidence mode: artifact
    - Evidence: Pending.

- [ ] T004 Remeasure restore and image conversion comparably.
  - Depends on: T001, T002, T003
  - Requirements: Requirement 1, Requirement 2, Requirement 3 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `verification.md`, `research.md`
  - Acceptance: Current baseline/change runs follow D001; before/after distributions and remaining
    bottlenecks are recorded with correctness outcomes.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: Historical May restore and image improvements cannot establish current performance
    acceptance.
  - Open decisions: D001
  - [ ] T004.1 Record comparable restore/open benchmark runs under D001.
    - Depends on: T001, T002, T003
    - Requirements: Requirement 1, Requirement 2, Requirement 3 (all acceptance criteria)
    - Files: `verification.md`, `research.md`
    - Acceptance: Record comparable restore/open benchmark runs under D001.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T004.2 Record comparable image conversion/lifecycle timings and remaining bottlenecks under D001.
    - Depends on: T004.1
    - Requirements: Requirement 1, Requirement 2, Requirement 3 (all acceptance criteria)
    - Files: `verification.md`, `research.md`
    - Acceptance: Record comparable image conversion/lifecycle timings and remaining bottlenecks under D001.
    - Evidence mode: artifact
    - Evidence: Pending.

- [/] T005 Verify element-map parser compatibility.
  - Depends on: none
  - Requirements: Requirement 4 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/App/ElementMap.cpp`, `tests/src/App/ElementMap.cpp`
  - Acceptance: Dot-separated child string IDs and mapped names preserve archive semantics across
    the representative fixture set; focused parser suite passes on selected revision.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: string_view tokenizer and parser regression test inspected; Nov 4 focused result is
    imported evidence, not a fresh run.
  - Status: partial - implementation exists; full declared acceptance remains open.
  - Properties: CP-001

- [/] T006 Validate deferred full-resolution image-plane lifecycle.
  - Depends on: none
  - Requirements: Requirement 2, Requirement 5 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/Gui/ViewProviderImagePlane.cpp`, `verification.md`
  - Acceptance: Visible/hidden restore, show after restore, edits, external/missing files and source
    dimensions are checked without a reduced-resolution semantic change.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: Pending texture, finishRestoring and show paths inspected; historical GUI smoke does
    not close the full matrix.
  - Status: partial - implementation exists; full declared acceptance remains open.
  - Properties: CP-002
  - [ ] T006.1 Verify visible-plane restore, edits and external/missing-file behavior.
    - Depends on: none
    - Requirements: Requirement 2, Requirement 5 (all acceptance criteria)
    - Files: `src/Gui/ViewProviderImagePlane.cpp`, `verification.md`
    - Acceptance: Verify visible-plane restore, edits and external/missing-file behavior.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T006.2 Verify hidden restore and show-after-restore preserve original texture resolution and plane dimensions.
    - Depends on: T006.1
    - Requirements: Requirement 2, Requirement 5 (all acceptance criteria)
    - Files: `src/Gui/ViewProviderImagePlane.cpp`, `verification.md`
    - Acceptance: Verify hidden restore and show-after-restore preserve original texture resolution and plane dimensions.
    - Evidence mode: artifact
    - Evidence: Pending.

- [/] T007 Validate tree status batching and larger-model behavior.
  - Depends on: none
  - Requirements: Requirement 6 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/Gui/Tree.cpp`, `verification.md`
  - Acceptance: Pending-status flush preserves open/close/reload, drag, expansion, selection and
    observers; larger-model profiling precedes any further batching/bulk-delete change.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: artifact
  - Evidence: Single pending status update exists; per-object delete side effects are documented;
    larger-model coverage remains open.
  - Status: partial - implementation exists; full declared acceptance remains open.
  - Properties: CP-003
  - [ ] T007.1 Verify pending-status flush and open/close/reload, drag, expansion, selection and observer behavior.
    - Depends on: none
    - Requirements: Requirement 6 (all acceptance criteria)
    - Files: `src/Gui/Tree.cpp`, `verification.md`
    - Acceptance: Verify pending-status flush and open/close/reload, drag, expansion, selection and observer behavior.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T007.2 Profile a larger representative document before proposing further batching or bulk deletion.
    - Depends on: T007.1
    - Requirements: Requirement 6 (all acceptance criteria)
    - Files: `src/Gui/Tree.cpp`, `verification.md`
    - Acceptance: Profile a larger representative document before proposing further batching or bulk deletion.
    - Evidence mode: artifact
    - Evidence: Pending.

- [?] T008 Decide whether GUI restore or startup profiling is warranted.
  - Depends on: T004
  - Requirements: Requirement 3 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `design.md`, `research.md`
  - Acceptance: Fresh timing either identifies one dominant substep with an owned follow-up or
    explicitly defers GUI restore/startup work; normal/clean profiles are distinguished.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: reasoned
  - Evidence: Historical plan C2/G3 remain conditional; no generic GUI/startup rewrite is
    authorized.
  - Decision owner: local-developer

- [?] T009 Disposition reduced-resolution texture exploration.
  - Depends on: none
  - Requirements: Requirement 2, Requirement 5 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `design.md`, `docs/backlog/README.md`
  - Acceptance: D002 explicitly rejects or routes reduced-resolution work without representing
    current full-resolution lazy loading as its implementation.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: reasoned
  - Evidence: Reduced-resolution textures remain a separate behavior decision from implemented lazy
    loading.
  - Decision owner: local-developer
  - Open decisions: D002

- [ ] T010 Verify final production revisions and prepare review evidence.
  - Depends on: T002, T003, T004, T005, T006, T007
  - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
    Requirement 6 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `verification.md`, `docs/reference/document-open-performance.md`
  - Acceptance: D003 fixes PR revisions; pixi build, focused checks and full suite results,
    visual/platform limits and comparable timings accompany independently reviewable commits.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: Focused reachable commits and dated results exist; exact PR revisions, full build/test
    and final review evidence remain unverified.
  - Open decisions: D003
  - [ ] T010.1 Record exact proposed production revisions and platform scope under D003.
    - Depends on: T002, T003, T004, T005, T006, T007
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `docs/reference/document-open-performance.md`
    - Acceptance: Record exact proposed production revisions and platform scope under D003.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T010.2 Run pixi run build -j 2 and the affected focused suites on those revisions.
    - Depends on: T010.1
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `docs/reference/document-open-performance.md`
    - Acceptance: Run pixi run build -j 2 and the affected focused suites on those revisions.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T010.3 Run pixi run test and record any failure/skip with its acceptance impact.
    - Depends on: T010.2
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `docs/reference/document-open-performance.md`
    - Acceptance: Run pixi run test and record any failure/skip with its acceptance impact.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T010.4 Assemble comparable timings, manual/platform results and focused commit boundaries for review.
    - Depends on: T010.3
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6 (all acceptance criteria)
    - Files: `verification.md`, `docs/reference/document-open-performance.md`
    - Acceptance: Assemble comparable timings, manual/platform results and focused commit boundaries for review.
    - Evidence mode: artifact
    - Evidence: Pending.

- [ ] T011 Promote performance contracts and route residual work.
  - Depends on: T008, T009, T010
  - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
    Requirement 6 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `docs/reference/document-open-performance.md`,
    `docs/runbooks/freecad-performance-optimization.md`, `docs/backlog/README.md`
  - Acceptance: All requirements/design targets have verified dispositions, durable docs and
    follow-up ownership before audited closure/archive metadata is recorded.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: No current closure or durable promotion claim; legacy plan/checklist remains
    historical.
  - [ ] T011.1 Promote accepted performance behavior/design and operational validation to durable targets.
    - Depends on: T008, T009, T010
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6 (all acceptance criteria)
    - Files: `docs/reference/document-open-performance.md`, `docs/runbooks/freecad-performance-optimization.md`, `docs/backlog/README.md`
    - Acceptance: Promote accepted performance behavior/design and operational validation to durable targets.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T011.2 Route conditional GUI/startup, reduced-resolution and other residual work to owned destinations.
    - Depends on: T011.1
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6 (all acceptance criteria)
    - Files: `docs/reference/document-open-performance.md`, `docs/runbooks/freecad-performance-optimization.md`, `docs/backlog/README.md`
    - Acceptance: Route conditional GUI/startup, reduced-resolution and other residual work to owned destinations.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T011.3 Verify acceptance/review coverage and record actual closure/archive metadata.
    - Depends on: T011.2
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6 (all acceptance criteria)
    - Files: `docs/reference/document-open-performance.md`, `docs/runbooks/freecad-performance-optimization.md`, `docs/backlog/README.md`
    - Acceptance: Verify acceptance/review coverage and record actual closure/archive metadata.
    - Evidence mode: artifact
    - Evidence: Pending.

## Legacy Task Disposition

The full 37-item checklist and May measurement tables are retained in
[legacy-tasks.md](legacy-tasks.md). Their original checkmarks are historical.
Plan epic IDs remain searchable in [plan.md](plan.md).

| Historical scope | Current task(s) | Reconciliation |
| --- | --- | --- |
| A1, B3, D1, E3; timing/review-note checkboxes | T001, T004, T010 | Dated measurements retained; current acceptance open |
| A2, B1, B2; buffered copy/notifications/fixtures | T002 | Implementation present; broader compatibility gates partial |
| C1; shape-map tracing and parser | T005 | Investigation history retained; parser exists; current compatibility evidence needed |
| C2; conditional GUI substep timing | T008 | Conditional on renewed measurements |
| D2, E1, E2; conversion/visual/format checks | T003 | Direct path exists; uncommon/visual matrix incomplete |
| F1, F2; restore ordering and lazy full-resolution prototype | T006 | Implemented; lifecycle matrix open |
| F2 reduced-resolution checkbox | T009 | Separate decision, not implemented |
| G1, G2; tree pending flag, side effects, larger trace | T007 | Batching exists; no bulk-delete bypass; larger-model evidence open |
| G3; startup tracing/ranking | T008 | Historical normal-profile observations; conditional follow-up |
| H1, H4; cleanup and commit separation | T010 | Reachable focused commits/absent switches are source evidence only |
| H2, H3; build, tests, PR gates | T010 | Current-revision final gates unverified |
| Promotion/closure not represented in legacy checklist | T011 | Required before closure |
