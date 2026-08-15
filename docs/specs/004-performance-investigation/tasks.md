---
title: FreeCAD Performance Investigation Tasks
doc_type: spec
artifact_type: tasks
status: draft
owner: local-developer
last_reviewed: 2026-08-14
---

# Tasks

**Input:** `requirements.md`, `design.md`, `canonical-context.md`,
`open-decisions.md`, and the evidence records in `research.md`

**Prerequisites:** Read the selected task row in `traceability.md` and the
linked verification gate before acting. This file is an execution index, not a
standalone authorization for source changes.

## Task ID Migration

The previous package used suffix IDs that lifecycle tooling silently ignored.
Their historical identities are preserved as aliases while active IDs use the
supported numeric form:

| Legacy alias | Active task ID |
| --- | --- |
| T007A | T025 |
| T012A | T026 |
| T014A | T027 |
| T014B | T028 |

## Task Dependency Graph

```text
T001 -> T002 -> T003 -> T004
T004 -> T005 -> T006 -> T007 -> T025 -> T008
T008 -> T009 (VTune escalation decision)
T008 -> T010 -> T011 -> T012 -> T026
T008 -> T027 -> T028
T028 -> T013
T028 -> T014
T013 + T014 -> T015
T002 -> T016 -> T017 -> T018
T002 -> T019 -> T020 -> T021
T026 -> T022 -> T023 -> T024
```

The selected path is overlay validation through T026. Presentation, document
open, and Preferences remain separate queued tracks; they are not prerequisites
for choosing the overlay target.

**Downstream review receipt (2026-08-14):** Tasks were reviewed after the
requirements and design revision. T026 is intentionally the first pending
top-level task; its subtasks split source recreation, baseline capture,
experiment capture, and the D002 decision.

## Phase 1: Evidence Preservation And Separation

**Purpose:** Preserve historical evidence and its uncertainty before selecting
new runtime work.

- [x] T001 Preserve historical profiler findings in package artifacts.
  - Depends on: none
  - Requirement: Requirement 1, Requirement 6
  - Files: `research.md`, `verification.md`
  - Acceptance: Historical `gdb`, `pidstat`, `perf`, and VTune observations are
    preserved with enough context to resume investigation.
  - Evidence mode: validation
  - Evidence: `research.md` records the artifact paths, environments, findings,
    and workflow-specific limits.

- [x] T002 Separate rotation, document-open, and Preferences evidence.
  - Depends on: T001
  - Requirement: Requirement 1, Requirement 3
  - Files: `research.md`
  - Acceptance: Each finding has one primary workflow and mixed conclusions are
    explicitly rejected.
  - Evidence mode: validation
  - Evidence: `research.md#document-restore--open`,
    `research.md#preferences-open-hang`, and
    `research.md#qt-paint-qimage-and-rasterization` keep the workflows
    separate; the mixed VTune run is explicitly excluded from rotation proof.

- [x] T003 Record async recompute as unsupported by current traces.
  - Depends on: T001, T002
  - Requirement: Requirement 1, Requirement 3
  - Files: `research.md`
  - Acceptance: Async recompute remains a rejected rotation hypothesis unless
    fresh direct evidence appears.
  - Evidence mode: validation
  - Evidence: `research.md#async-recompute` records the waiting worker and the
    absence of a recompute signal in rotation profiles.

- [x] T004 Keep compact UI as a controlled variable only.
  - Depends on: T002
  - Requirement: Requirement 2, Requirement 3
  - Files: `research.md`, `quickstart.md`
  - Acceptance: Compact/frameless UI is logged as a control and not described as
    a confirmed cause.
  - Evidence mode: validation
  - Evidence: `research.md#compact-ui` and the capture manifest contract in
    `quickstart.md` preserve that boundary.

## Phase 2: Historical Controlled Rotation Baseline

**Purpose:** Preserve the valid comparison baseline and invalidate mixed
captures without overstating causality.

- [x] T005 Disable DockOverlay assignments and compact/frameless settings for a baseline.
  - Depends on: T004
  - Requirement: Requirement 2
  - Files: `research.md`, `quickstart.md`
  - Acceptance: The disabled configuration and its role are recorded.
  - Evidence mode: validation
  - Evidence: `research.md` records the disabled setup and capture artifacts.

- [x] T006 Run a short controlled rotation `perf` capture with DockOverlay disabled.
  - Depends on: T005
  - Requirement: Requirement 2
  - Files: `research.md`
  - Acceptance: A disabled rotation baseline records command shape, sample
    observations, and limits.
  - Evidence mode: validation
  - Evidence: `research.md#short-overlay-disabled-baseline---2026-05-24-1843`.

- [x] T007 Run a separate overlay-enabled rotation capture.
  - Depends on: T006
  - Requirement: Requirement 2
  - Files: `research.md`
  - Acceptance: The enabled capture is preserved separately from disabled and
    mixed-state captures.
  - Evidence mode: validation
  - Evidence: `research.md#long-rotation-ab-attempt---2026-05-24-1849-and-1855`.

- [x] T025 Re-run the disabled capture after mixed-state invalidation.
  - Legacy alias: T007A
  - Depends on: T007
  - Requirement: Requirement 1, Requirement 2
  - Files: `research.md`
  - Acceptance: The mixed-state run remains invalid and the corrected
    disabled-only rerun is the historical disabled baseline.
  - Evidence mode: validation
  - Evidence: The corrected `clean2` run and the invalidation rationale are
    recorded in `research.md`.

- [x] T008 Compare overlay, Qt raster, presentation, and scene-render signals.
  - Depends on: T006, T007, T025
  - Requirement: Requirement 1, Requirement 2
  - Files: `research.md`
  - Acceptance: The package records what disappeared, persisted, or changed
    without converting sample counts into causal proof.
  - Evidence mode: validation
  - Evidence: `research.md#short-overlay-disabled-baseline---2026-05-24-1843`
    and `research.md#long-rotation-ab-attempt---2026-05-24-1849-and-1855`
    record separate command/sample tables for overlay, image/memory, Qt RHI,
    GLX/Mesa, and Coin/Quarter signals.

- [x] T009 Decide whether a rotation-only VTune escalation is required.
  - Depends on: T008
  - Requirement: Requirement 2
  - Files: `research.md`, `verification.md`
  - Acceptance: The selected slice records whether `perf` is sufficient; if
    not, any VTune escalation must isolate rotation from Preferences and
    document open.
  - Evidence mode: planner
  - Evidence: D001 selected matched overlay `perf` validation. No rotation-only
    VTune capture is required for the current slice; VTune remains an explicit
    escalation if T026 cannot resolve D002.

## Phase 3: Selected Overlay Investigation

**Purpose:** Validate the only historical source experiment with a direct
hotspot match before considering a source implementation slice.

- [x] T010 Inspect `Gui::OverlayGraphicsEffect::draw` for avoidable paint work.
  - Depends on: T008
  - Requirement: Requirement 1, Requirement 4
  - Files: `src/Gui/OverlayWidgets.cpp`, `research.md`
  - Acceptance: The review records image construction, blur, repaint region,
    and graphics-effect behavior.
  - Evidence mode: validation
  - Evidence: Commit `3d41e4b3c4f` and
    `research.md#optimized-overlay-enabled-attach-run---2026-05-24-1944` record the
    unconditional zero-radius blur path and the historical experiment.

- [x] T011 Inspect `Gui::OverlayManager::eventFilter` for event-path relevance.
  - Depends on: T008
  - Requirement: Requirement 1, Requirement 4
  - Files: `src/Gui/OverlayManager.cpp`, `research.md`
  - Acceptance: The review distinguishes light event-filter sampling from the
    measured graphics-effect hotspot.
  - Evidence mode: validation
  - Evidence: `research.md` records light event-filter samples with and without
    overlay drawing; direct 2026-08-14 review found no evidence it dominates.

- [x] T012 Preserve the low-risk DockOverlay experiment as historical evidence.
  - Depends on: T010, T011
  - Requirement: Requirement 1, Requirement 4
  - Files: `src/Gui/OverlayWidgets.cpp`, `src/Gui/OverlayWidgets.h`, `research.md`
  - Acceptance: The exact experiment, commit, measured rationale, and current
    source discrepancy are recorded without claiming it is currently present.
  - Evidence mode: validation
  - Evidence: Commit `3d41e4b3c4f`, the 2026-05-24 optimized capture, and the
    2026-08-14 current-source reconciliation are recorded in `research.md` and
    `canonical-context.md`.

- [ ] T026 Run matched before/after overlay captures for the recreated experiment.
  - Legacy alias: T012A
  - Depends on: T012
  - Requirement: Requirement 2, Requirement 4, Requirement 5
  - Files: `research.md`, `quickstart.md`, `verification.md`
  - Acceptance: A permitted source branch recreates or reapplies the experiment;
    matched baseline and experiment captures use the same model, duration,
    capture shape, and recorded controls; results resolve D002 without claiming
    cross-platform generality.
  - Evidence mode: validation
  - Decision owner: local-developer with GUI/rendering reviewer
  - Evidence: Pending.
  - [ ] T026.1 Recreate or reapply the historical experiment on a permitted
    current-source branch and record the exact baseline, branch, diff, and line
    ending checks.
    - Evidence mode: implementation
    - Evidence: Pending.
  - [ ] T026.2 Capture the matched baseline run using the completed manifest in
    `quickstart.md`.
    - Evidence mode: validation
    - Evidence: Pending.
  - [ ] T026.3 Capture the matched experiment run with the same controls and
    duration.
    - Evidence mode: validation
    - Evidence: Pending.
  - [ ] T026.4 Compare behavior and profiles, record uncertainty, and resolve
    D002 as keep, reject, or gather stronger evidence.
    - Evidence mode: validation
    - Evidence: Pending.

## Phase 4: Queued Presentation And Scene-Render Investigation

**Purpose:** Attribute residual rotation cost only if the selected overlay slice
does not yield an implementation-ready cause or after it is routed.

- [x] T027 Reject in-app View3D timing instrumentation as acceptance evidence.
  - Legacy alias: T014A
  - Depends on: T008
  - Requirement: Requirement 1, Requirement 2
  - Files: `research.md`
  - Acceptance: Normal modelling validation uses external profiling; temporary
    timing code does not enter integration acceptance.
  - Evidence mode: validation
  - Evidence: `research.md#viewport-render-timing-approach---2026-05-24` records
    removal of the timing experiment and the external-tool decision.

- [ ] T028 Capture a normal modelling rotation session with external profiling.
  - Legacy alias: T014B
  - Depends on: T027
  - Requirement: Requirement 2
  - Files: `research.md`, `quickstart.md`
  - Acceptance: The capture attaches after model load, isolates rotation, and
    records the capture manifest required by AC2.1-AC2.4.
  - Evidence mode: validation
  - Evidence: Pending.

- [ ] T013 Attribute residual `QImage` and raster samples to a calling path.
  - Depends on: T028
  - Requirement: Requirement 2, Requirement 5
  - Files: `src/Gui/OverlayWidgets.cpp`, `src/Gui/View3DInventorViewer.cpp`, `research.md`
  - Acceptance: One external capture identifies whether the dominant sampled
    caller belongs to overlay/widget composition, viewer image rendering, or an
    unresolved external Qt path; unresolved attribution is recorded honestly.
  - Evidence mode: validation
  - Evidence: Pending.

- [ ] T014 Classify GLX/Mesa and Qt RHI presentation samples during rotation.
  - Depends on: T028
  - Requirement: Requirement 2, Requirement 5
  - Files: `src/Gui/View3DInventorViewer.cpp`, `research.md`
  - Acceptance: The investigation distinguishes expected active presentation
    from a measured stall and records the method and limits.
  - Evidence mode: validation
  - Evidence: Pending.

- [ ] T015 Narrow residual Coin/Quarter scene-render cost.
  - Depends on: T013, T014
  - Requirement: Requirement 2, Requirement 5
  - Files: `src/Gui/View3DInventorViewer.cpp`, `src/Mod/Part/Gui/ViewProviderExt.cpp`, `research.md`
  - Acceptance: Source review and capture evidence identify a bounded redraw or
    traversal candidate, or explicitly record that no FreeCAD-owned candidate
    was established.
  - Evidence mode: validation
  - Evidence: Pending.

## Phase 5: Queued Document Restore And Open Investigation

**Purpose:** Measure document-open behavior independently of active rotation.

- [ ] T016 Profile `Gridfinity Drawer v1.3.FCStd` document open separately.
  - Depends on: T002
  - Requirement: Requirement 3
  - Files: `research.md`, `verification.md`
  - Acceptance: A dedicated document-open capture excludes rotation and
    Preferences and records its source revision and manifest.
  - Evidence mode: validation
  - Evidence: Pending.

- [ ] T017 Narrow document-open cost between restore plumbing and geometry setup.
  - Depends on: T016
  - Requirement: Requirement 3, Requirement 5
  - Files: `src/Gui/Document.cpp`, `src/Mod/Part/Gui/ViewProviderExt.cpp`, `research.md`
  - Acceptance: Evidence classifies the dominant captured source path as
    restore plumbing, view-provider visual update/geometry setup, or unresolved.
  - Evidence mode: validation
  - Evidence: Pending.

- [ ] T018 Record the document-open ownership and routing decision.
  - Depends on: T017
  - Requirement: Requirement 3, Requirement 6
  - Files: `open-decisions.md`, `verification.md`
  - Acceptance: D003 names one explicit owner/destination or a rejection
    rationale without mixing document-open work into rotation.
  - Evidence mode: routing
  - Decision owner: local-developer with GUI/Part GUI owner
  - Evidence: Pending.

## Phase 6: Queued Preferences-Open Investigation

**Purpose:** Reproduce and classify Preferences behavior without using it as
rotation evidence.

- [ ] T019 Reproduce the Preferences-open hang independently.
  - Depends on: T002
  - Requirement: Requirement 3
  - Files: `research.md`, `verification.md`
  - Acceptance: Reproduction excludes rotation and document open and records
    addon/configuration state.
  - Evidence mode: validation
  - Evidence: Pending.

- [ ] T020 Profile Preferences page/module initialization.
  - Depends on: T019
  - Requirement: Requirement 3
  - Files: `research.md`
  - Acceptance: Evidence distinguishes FreeCAD page/module loading from profiler
    symbol-processing overhead and addon side effects.
  - Evidence mode: validation
  - Evidence: Pending.

- [ ] T021 Classify ownership of the `addFC` `ffmpeg -version` timeout.
  - Depends on: T020
  - Requirement: Requirement 3, Requirement 6
  - Files: `research.md`, `open-decisions.md`
  - Acceptance: D004 resolves to addon, environment, FreeCAD core, mixed
    ownership, or an explicit unresolved destination based on direct evidence.
  - Evidence mode: routing
  - Decision owner: local-developer with addon/core owner
  - Evidence: Pending.

## Phase 7: Selected-Slice Implementation Readiness And Closure Routing

**Purpose:** Convert one validated cause into a bounded implementation plan and
route everything else before closure.

- [ ] T022 Select one narrow implementation target from the overlay evidence.
  - Depends on: T026
  - Requirement: Requirement 4, Requirement 5
  - Files: `design.md`, `open-decisions.md`, `verification.md`
  - Acceptance: D002 either selects one bounded current-source target with an
    owner and excludes other workflows, or rejects/routes the overlay
    experiment and records the next workstream decision.
  - Evidence mode: planner
  - Decision owner: local-developer with GUI/rendering reviewer
  - Evidence: Pending.

- [ ] T023 Define before/after validation and review for the selected target.
  - Depends on: T022
  - Requirement: Requirement 2, Requirement 5, Requirement 6
  - Files: `verification.md`
  - Acceptance: The exact source branch, build/tests, matched measurement,
    regression checks, manual scenarios, platform limits, and reviewer are
    recorded before implementation begins.
  - Evidence mode: planner
  - Evidence: Pending.

- [ ] T024 Record non-selected workstream destinations and closure preparation.
  - Depends on: T023
  - Requirement: Requirement 3, Requirement 6
  - Files: `design.md`, `open-decisions.md`, `traceability.md`, `verification.md`
  - Acceptance: Every unselected track and lasting package artifact has exactly
    one explicit destination, rejection rationale, or promotion target; no
    `partial-blocking` or `not-covered` item remains at closure.
  - Evidence mode: routing
  - Decision owner: local-developer/docs owner
  - Evidence: Pending.

## Execution Rules

- Before starting a task, read its traceability row, linked requirements,
  design sections, decisions, canonical context, and verification gate.
- Mark only one source-backed implementation task `[~]` at a time. Independent
  read-only investigation may occur in parallel when evidence cannot mix.
- Use a named source branch for any source experiment. Do not edit `main` or a
  merge-only integration branch directly.
- A completed task requires concrete evidence. `planner`, `routing`, `no_op`,
  and dry-run evidence complete only tasks whose acceptance explicitly calls
  for that mode.
- Invalid or mixed captures stay visible but cannot satisfy comparison
  acceptance.
- Update requirements/design downstream artifacts if the selected workstream or
  acceptance boundary changes.
- Before closure, reconcile broad requirements and design targets against
  evidence, not checkbox state, and route every residual exactly once.

## Related Artifacts

- Requirements: `requirements.md`
- Canonical Context: `canonical-context.md`
- Design: `design.md`
- Open Decisions: `open-decisions.md`
- Traceability: `traceability.md`
- Verification: `verification.md`
- Research: `research.md`
- Quickstart: `quickstart.md`

## Fallback Template Authority Review

- 2026-08-14: Reviewed after removal of the repository template override. The
  bundled Spec Lifecycle Manager templates remain authoritative; this changes
  no task scope, ordering, acceptance criterion, or evidence requirement.
