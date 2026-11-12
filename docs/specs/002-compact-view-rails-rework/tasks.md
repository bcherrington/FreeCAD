---
title: Compact view rails rework tasks
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
T001 -> T002
T002 -> T003
T003 -> T004
T004 -> T005
```

## Delivery Slices

Parent tasks are acceptance checkpoints. Execute their numbered subtasks as
bounded slices; subtask file/requirement scope is inherited explicitly. Preserve
parent evidence as partial context, and complete a parent only after all declared
outcomes are verified. A completed narrow subtask does not close the full linked
requirement by itself.

- [?] T001 Decide rail scope, geometry and persistence contracts.
  - Depends on: none
  - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
    Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11,
    Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `design.md`, `requirements.md`
  - Acceptance: D001/D002/D003 resolve reintroduction, geometry ownership, invalid panel IDs,
    ordering policy and measurable GL comparison gate.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: reasoned
  - Evidence: Current code has no rails; choices remain proposed.
  - Decision owner: local-developer
  - Open decisions: D001, D002, D003
  - [ ] T001.1 Resolve D001: rail reintroduction and Qt-owned reservation versus overlay geometry.
    - Depends on: none
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `design.md`, `requirements.md`
    - Acceptance: Resolve D001: rail reintroduction and Qt-owned reservation versus overlay geometry.
    - Evidence mode: reasoned
    - Evidence: Pending.
  - [ ] T001.2 Resolve D002: stable panel IDs, slot restoration, invalid IDs and conditional ordering.
    - Depends on: T001.1
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `design.md`, `requirements.md`
    - Acceptance: Resolve D002: stable panel IDs, slot restoration, invalid IDs and conditional ordering.
    - Evidence mode: reasoned
    - Evidence: Pending.
  - [ ] T001.3 Resolve D003: comparable viewport/GL protocol and quantitative allocation-regression gate.
    - Depends on: T001.2
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `design.md`, `requirements.md`
    - Acceptance: Resolve D003: comparable viewport/GL protocol and quantitative allocation-regression gate.
    - Evidence mode: reasoned
    - Evidence: Pending.

- [ ] T002 Define slot and action integration regression coverage.
  - Depends on: T001
  - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
    Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11,
    Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `tests/src/Gui/CompactMainWindowChrome.cpp`
  - Acceptance: Tests cover six slots, empty/same/populated/cancel drops, action synchronization,
    restore, theme/DPI, compact-disabled and overlay behavior.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: Existing tests cover titlebar convergence only; rail tests are absent.
  - Properties: CP-001, CP-002
  - [ ] T002.1 Define six-slot placement, insertion/cancellation and same-slot drop regression cases.
    - Depends on: T001
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `tests/src/Gui/CompactMainWindowChrome.cpp`
    - Acceptance: Define six-slot placement, insertion/cancellation and same-slot drop regression cases.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T002.2 Define action synchronization, persistence and compact-disabled restoration cases.
    - Depends on: T002.1
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `tests/src/Gui/CompactMainWindowChrome.cpp`
    - Acceptance: Define action synchronization, persistence and compact-disabled restoration cases.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T002.3 Define layout-convergence/theme/DPI cases and real desktop/GL acceptance checks.
    - Depends on: T002.2
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `tests/src/Gui/CompactMainWindowChrome.cpp`
    - Acceptance: Define layout-convergence/theme/DPI cases and real desktop/GL acceptance checks.
    - Evidence mode: artifact
    - Evidence: Pending.

- [ ] T003 Implement explicit rail slots over dock actions.
  - Depends on: T002
  - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
    Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11,
    Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `src/Gui/CompactMainWindowChrome.cpp`, `src/Gui/CompactMainWindowChrome.h`,
    `src/Gui/MainWindow.cpp`
  - Acceptance: All selected slot/action/persistence requirements pass new tests; geometry never
    fights QMainWindow and titlebar regression tests remain valid.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: Rail implementation absent; baseline titlebar remains implemented.
  - Properties: CP-001, CP-002
  - [ ] T003.1 Implement explicit slot containers and stable Qt-owned or overlay geometry under D001.
    - Depends on: T002
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `src/Gui/CompactMainWindowChrome.cpp`, `src/Gui/CompactMainWindowChrome.h`, `src/Gui/MainWindow.cpp`
    - Acceptance: Implement explicit slot containers and stable Qt-owned or overlay geometry under D001.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T003.2 Implement QAction-backed launchers and insertion/hover/cancellation feedback.
    - Depends on: T003.1
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `src/Gui/CompactMainWindowChrome.cpp`, `src/Gui/CompactMainWindowChrome.h`, `src/Gui/MainWindow.cpp`
    - Acceptance: Implement QAction-backed launchers and insertion/hover/cancellation feedback.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T003.3 Implement slot persistence and the approved missing-ID/ordering policy under D002.
    - Depends on: T003.2
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `src/Gui/CompactMainWindowChrome.cpp`, `src/Gui/CompactMainWindowChrome.h`, `src/Gui/MainWindow.cpp`
    - Acceptance: Implement slot persistence and the approved missing-ID/ordering policy under D002.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T003.4 Run new slot/action tests and existing titlebar convergence regressions.
    - Depends on: T003.3
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `src/Gui/CompactMainWindowChrome.cpp`, `src/Gui/CompactMainWindowChrome.h`, `src/Gui/MainWindow.cpp`
    - Acceptance: Run new slot/action tests and existing titlebar convergence regressions.
    - Evidence mode: artifact
    - Evidence: Pending.

- [ ] T004 Validate desktop interaction and rendered viewport stability.
  - Depends on: T003
  - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
    Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11,
    Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `tests/src/Gui/CompactMainWindowChrome.cpp`, `verification.md`
  - Acceptance: Real drag/drop and restart, overlays, workbench/theme/DPI/frameless cases pass;
    equivalent before/after resize/FBO captures satisfy D003.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: 2026-11-04 offscreen titlebar results do not prove rails or GL acceptance.
  - Open decisions: D003
  - Properties: CP-001, CP-002
  - [ ] T004.1 Record real drag/drop, restart, overlay, workbench, theme, DPI and frameless checks.
    - Depends on: T003
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `tests/src/Gui/CompactMainWindowChrome.cpp`, `verification.md`
    - Acceptance: Record real drag/drop, restart, overlay, workbench, theme, DPI and frameless checks.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T004.2 Record equivalent before/after viewport resize and GL allocation traces under D003.
    - Depends on: T004.1
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `tests/src/Gui/CompactMainWindowChrome.cpp`, `verification.md`
    - Acceptance: Record equivalent before/after viewport resize and GL allocation traces under D003.
    - Evidence mode: artifact
    - Evidence: Pending.

- [ ] T005 Promote rail design and record closure dispositions.
  - Depends on: T004
  - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5,
    Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11,
    Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
  - Design: design.md#high-level-design; design.md#low-level-design
  - Files: `docs/design/jetbrains-style-compact-ui.md`,
    `docs/runbooks/compact-view-rails-rework.md`, `docs/backlog/README.md`
  - Acceptance: Accepted behavior is promoted, conditional ordering has a disposition, residuals
    have owners, review and closure metadata are complete.
  - Validation: See verification.md#quality-gates and the linked criteria.
  - Evidence mode: command
  - Evidence: Pending feature acceptance; no closure is claimed.

  - [ ] T005.1 Promote accepted panel-access design and operational validation.
    - Depends on: T004
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `docs/design/jetbrains-style-compact-ui.md`, `docs/runbooks/compact-view-rails-rework.md`, `docs/backlog/README.md`
    - Acceptance: Promote accepted panel-access design and operational validation.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T005.2 Disposition conditional ordering and route remaining work with an owner.
    - Depends on: T005.1
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `docs/design/jetbrains-style-compact-ui.md`, `docs/runbooks/compact-view-rails-rework.md`, `docs/backlog/README.md`
    - Acceptance: Disposition conditional ordering and route remaining work with an owner.
    - Evidence mode: artifact
    - Evidence: Pending.
  - [ ] T005.3 Record reviewed requirement coverage and actual closure/archive metadata.
    - Depends on: T005.2
    - Requirements: Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7, Requirement 8, Requirement 9, Requirement 10, Requirement 11, Requirement 12, Requirement 13, Requirement 14 (all acceptance criteria)
    - Files: `docs/design/jetbrains-style-compact-ui.md`, `docs/runbooks/compact-view-rails-rework.md`, `docs/backlog/README.md`
    - Acceptance: Record reviewed requirement coverage and actual closure/archive metadata.
    - Evidence mode: artifact
    - Evidence: Pending.
