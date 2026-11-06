---
title: CAD and mesh file thumbnailing tasks
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
T001, T002 -> T003
T003 -> T004
T004 -> T005
T004, T005 -> T006
```

## Delivery Slices

Parent tasks are acceptance checkpoints. Execute their numbered subtasks as
bounded slices; subtask file/requirement scope is inherited explicitly. Preserve
parent evidence as partial context, and complete a parent only after all declared
outcomes are verified. A completed narrow subtask does not close the full linked
requirement by itself.

- [?] T001 Resolve STL rendering and resource contracts.
  - Depends on: none
  - Requirements: Requirement 2, Requirement 4, Requirement 5, Requirement 6, Requirement 7 (all
    acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/Tools/freecad-thumbnailer.in`, `design.md`
  - Acceptance: D001/D002 specify process, camera/output, timeout, input/memory/concurrency budgets
    and isolation with a feasible prototype.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: reasoned
  - Evidence: Source baseline has extraction only; renderer and resource policy are unverified.
  - Decision owner: local-developer
  - Open decisions: D001, D002
  - Properties: CP-002
  - [ ] T001.1 Record the STL process/import/render choice and camera/output contract for D001.
    - Depends on: none
    - Requirements: Requirement 2, Requirement 4, Requirement 5, Requirement 6, Requirement 7 (all acceptance criteria)
    - Files: `src/Tools/freecad-thumbnailer.in`, `design.md`
    - Acceptance: Record the STL process/import/render choice and camera/output contract for D001.
    - Evidence mode: reasoned
    - Evidence: Pending.
  - [ ] T001.2 Record timeout, input size, memory/concurrency budgets and the isolation acceptance method for D002.
    - Depends on: T001.1
    - Requirements: Requirement 2, Requirement 4, Requirement 5, Requirement 6, Requirement 7 (all acceptance criteria)
    - Files: `src/Tools/freecad-thumbnailer.in`, `design.md`
    - Acceptance: Record timeout, input size, memory/concurrency budgets and the isolation acceptance method for D002.
    - Evidence mode: reasoned
    - Evidence: Pending.

- [ ] T002 Add durable FCStd extraction regression fixtures.
  - Depends on: none
  - Requirements: Requirement 1 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/Tools/freecad-thumbnailer.in`, `tests/`
  - Acceptance: Embedded/fallback bytes, invalid ZIP/document, unsupported input and output errors
    have repeatable fixtures; no geometry import occurs.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: Five scratch cases were reported on 2026-11-04; no committed fixture suite was added.
  - Properties: CP-001

- [ ] T003 Deliver an isolated STL thumbnail slice.
  - Depends on: T001, T002
  - Requirements: Requirement 2, Requirement 4, Requirement 5, Requirement 6, Requirement 7 (all
    acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/Tools/freecad-thumbnailer.in`, `tests/`
  - Acceptance: STL produces PNG with approved view/size; failure, timeout, malformed/empty/oversize
    inputs and concurrent requests satisfy approved limits without user code/windows.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: Pending implementation and renderer isolation verification.
  - Open decisions: D001, D002
  - Properties: CP-002
  - [ ] T003.1 Implement the approved STL import, fit and PNG output path.
    - Depends on: T001, T002
    - Requirements: Requirement 2, Requirement 4, Requirement 5, Requirement 6, Requirement 7 (all acceptance criteria)
    - Files: `src/Tools/freecad-thumbnailer.in`, `tests/`
    - Acceptance: Implement the approved STL import, fit and PNG output path.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T003.2 Implement configuration isolation, timeout/resource enforcement and failure cleanup.
    - Depends on: T003.1
    - Requirements: Requirement 2, Requirement 4, Requirement 5, Requirement 6, Requirement 7 (all acceptance criteria)
    - Files: `src/Tools/freecad-thumbnailer.in`, `tests/`
    - Acceptance: Implement configuration isolation, timeout/resource enforcement and failure cleanup.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T003.3 Run STL success, malformed/empty/oversize, crash/timeout, output-error and concurrency fixtures.
    - Depends on: T003.2
    - Requirements: Requirement 2, Requirement 4, Requirement 5, Requirement 6, Requirement 7 (all acceptance criteria)
    - Files: `src/Tools/freecad-thumbnailer.in`, `tests/`
    - Acceptance: Run STL success, malformed/empty/oversize, crash/timeout, output-error and concurrency fixtures.
    - Evidence mode: artifact
    - Evidence: Pending.

- [ ] T004 Validate installed Linux MIME and desktop behavior.
  - Depends on: T003
  - Requirements: Requirement 1, Requirement 2, Requirement 8 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/XDGData/FreeCAD.thumbnailer.in`, `src/Tools/freecad-thumbnailer.in`
  - Acceptance: Installed helper/descriptor work in GNOME Files and KDE Dolphin; cache updates after
    file changes; only tested MIME aliases are registered; small/medium/large timings recorded.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: FCStd-only descriptor inspected; no rendered desktop acceptance exists.
  - Properties: CP-002
  - [ ] T004.1 Validate installed helper paths and explicitly supported MIME aliases.
    - Depends on: T003
    - Requirements: Requirement 1, Requirement 2, Requirement 8 (all acceptance criteria)
    - Files: `src/XDGData/FreeCAD.thumbnailer.in`, `src/Tools/freecad-thumbnailer.in`
    - Acceptance: Validate installed helper paths and explicitly supported MIME aliases.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T004.2 Record GNOME Files and KDE Dolphin preview/cache-refresh results.
    - Depends on: T004.1
    - Requirements: Requirement 1, Requirement 2, Requirement 8 (all acceptance criteria)
    - Files: `src/XDGData/FreeCAD.thumbnailer.in`, `src/Tools/freecad-thumbnailer.in`
    - Acceptance: Record GNOME Files and KDE Dolphin preview/cache-refresh results.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T004.3 Record small, medium and large STL runtime/resource results against D002.
    - Depends on: T004.2
    - Requirements: Requirement 1, Requirement 2, Requirement 8 (all acceptance criteria)
    - Files: `src/XDGData/FreeCAD.thumbnailer.in`, `src/Tools/freecad-thumbnailer.in`
    - Acceptance: Record small, medium and large STL runtime/resource results against D002.
    - Evidence mode: artifact
    - Evidence: Pending.

- [?] T005 Gate STEP and IGES feasibility and registration.
  - Depends on: T004
  - Requirements: Requirement 3, Requirement 8 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/Tools/freecad-thumbnailer.in`, `src/XDGData/FreeCAD.thumbnailer.in`
  - Acceptance: D003 records feasibility and format-specific acceptance before adding STEP/IGES;
    unsupported OBJ/3MF remain explicitly outside the selected MVP.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: reasoned
  - Evidence: STEP/IGES render support is absent from current helper.
  - Decision owner: local-developer
  - Open decisions: D003

- [ ] T006 Promote accepted thumbnail contracts and close remaining work.
  - Depends on: T004, T005
  - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
    Requirement 6, Requirement 7, Requirement 8 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `docs/reference/cad-file-thumbnailing.md`, `docs/runbooks/cad-file-thumbnailing.md`,
    `docs/backlog/README.md`
  - Acceptance: Each criterion is verified or explicitly routed; durable docs, review, closure log
    and archive metadata agree.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: Pending feature acceptance and promotion.

  - [ ] T006.1 Promote accepted thumbnail behavior and operational checks to the declared durable targets.
    - Depends on: T004, T005
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8 (all acceptance criteria)
    - Files: `docs/reference/cad-file-thumbnailing.md`, `docs/runbooks/cad-file-thumbnailing.md`, `docs/backlog/README.md`
    - Acceptance: Promote accepted thumbnail behavior and operational checks to the declared durable targets.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T006.2 Give every remaining criterion/format an accepted disposition or one owned follow-up.
    - Depends on: T006.1
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8 (all acceptance criteria)
    - Files: `docs/reference/cad-file-thumbnailing.md`, `docs/runbooks/cad-file-thumbnailing.md`, `docs/backlog/README.md`
    - Acceptance: Give every remaining criterion/format an accepted disposition or one owned follow-up.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T006.3 Record review and actual closure/archive metadata only after acceptance and promotion.
    - Depends on: T006.2
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8 (all acceptance criteria)
    - Files: `docs/reference/cad-file-thumbnailing.md`, `docs/runbooks/cad-file-thumbnailing.md`, `docs/backlog/README.md`
    - Acceptance: Record review and actual closure/archive metadata only after acceptance and promotion.
    - Evidence mode: artifact
    - Evidence: Pending.
