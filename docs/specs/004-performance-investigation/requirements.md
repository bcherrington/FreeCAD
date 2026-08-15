---
title: FreeCAD Performance Investigation Requirements
doc_type: spec
artifact_type: requirements
status: draft
owner: local-developer
last_reviewed: 2026-08-14
---

# Requirements

## Introduction

Spec 004 preserves and continues the performance investigation formerly tracked
as `20260524-performance-investigation`. It coordinates evidence and decisions;
it is not an umbrella authorization for source changes. Historical profiles
cover viewport rotation, DockOverlay painting, document restore/open, and a
separate Preferences-open hang. Current work must keep those workflows distinct,
reconcile historical findings against current source, and advance only one
narrow evidence-backed slice at a time.

## Goals

- Preserve observed evidence without upgrading hypotheses into root causes.
- Keep rotation, document-open, and Preferences-open work independently
  traceable.
- Make rotation comparisons reproducible enough to support a before/after
  decision.
- Select one narrow investigation slice without waiting for every unrelated
  track to finish.
- Require fresh source and runtime evidence before authorizing implementation.

## Non-Goals

- Do not implement or approve a FreeCAD source optimization in this revision.
- Do not treat debug-build sample counts as a release performance budget.
- Do not treat compact/frameless UI, async recompute, `QBackingStore`, an addon,
  or a graphics driver as a confirmed root cause without direct evidence.
- Do not mix document-open or Preferences activity into rotation captures.
- Do not require all queued workstreams to complete before choosing one narrow
  target.
- Do not flatten the historical research record into summary-only prose.

## Glossary

| Term | Definition |
| --- | --- |
| controlled rotation capture | An externally profiled viewport-rotation session with the model, duration, capture method, and relevant UI state recorded. |
| comparison pair | Two captures that differ only in the intended experimental variable. |
| historical evidence | The 2026-05-24 profiler observations preserved in `research.md`; useful for routing but not automatically current acceptance evidence. |
| current source | The checked-out source and Git state inspected for the active slice; it overrides assumptions based only on historical branch ancestry. |
| selected slice | The single workstream permitted to advance toward implementation readiness while other work remains queued or routed. |

## Durable Source Baseline

| Source | Current behavior relied on | Confidence | Notes |
| --- | --- | --- | --- |
| `docs/performance-optimization-findings.md` | Renewed performance work must use a focused owner, current baseline, reproducible measurements, correctness checks, and branch-specific validation. | high | Durable routing authority. It assigns GUI document restore follow-up to Spec 004. |
| current FreeCAD source and Git history | Current implementation state and whether an experimental commit is actually present. | high | Direct inspection on 2026-08-14 shows the old DockOverlay experiment commit in ancestry, but its zero-radius blur guard is not present in the current file. |
| `research.md` | Historical profiler observations, artifact paths, interpretations, and experiment history. | medium | Spec-local historical evidence, not durable current behavior. |
| `quickstart.md` | Proposed controlled capture procedure for the next slice. | medium | Must be validated during the next runtime capture before promotion. |
| `docs/viewport-rendering-improvements.md` | Durable renderer experiments and production-safety boundaries from closed viewport work. | high | Supporting context; it does not establish Spec 004's root cause. |

## Durable Impact

| Durable area | Action | Target | Notes |
| --- | --- | --- | --- |
| performance routing | clarify later | `docs/performance-optimization-findings.md` | Update only after Spec 004 selects or routes a lasting owner. |
| profiling procedure | add or clarify later | developer performance troubleshooting/runbook destination to be selected | Promote only after the procedure is exercised successfully. |
| viewport rendering guidance | unchanged | `docs/viewport-rendering-improvements.md` | Spec 004 does not change renderer policy in this revision. |
| source/API/contracts | unchanged | FreeCAD source and tests | This revision changes specification artifacts only. |

## Staged Readiness

- **Current stage:** reconcile and investigate
- **Next stage:** validate the selected overlay experiment with controlled
  before/after evidence
- **Ready to implement when:** the selected slice has a current source baseline,
  reproducible evidence identifying a source-level cause, a bounded patch,
  before/after validation, and required review.
- **Ready to validate:** yes for the package revision; no for a source change.
- **Design-first exception:** no
- **Optional artifacts used:** `research.md`, `quickstart.md`,
  `open-decisions.md`, `canonical-context.md`, `traceability.md`, and
  `verification.md`
- **Downstream review needed:** refresh tasks, traceability, and verification
  whenever requirements, design, or the selected workstream changes.

## Requirements

### Requirement 1: Preserve evidence integrity

**ID:** `R1`

**User Story:** As a future investigator, I want observations, interpretations,
and recommendations kept distinct so that uncertain historical data cannot be
mistaken for a confirmed root cause.

**Priority:** must-have

#### Acceptance Criteria

1. **AC1.1:** GIVEN a reader inspects the package, WHEN evidence is summarized,
   THEN observations, interpretations, and recommendations SHALL be labelled
   distinctly.
2. **AC1.2:** GIVEN a task is marked complete, WHEN its state is audited, THEN
   it SHALL cite concrete recorded evidence rather than checkbox history alone.
3. **AC1.3:** IF a historical result conflicts with current source or fresh
   runtime evidence, THEN the conflict SHALL be recorded and current evidence
   SHALL govern the active slice.

### Requirement 2: Keep controlled rotation comparisons reproducible

**ID:** `R2`

**User Story:** As a performance investigator, I want comparable rotation
captures so that overlay, Qt raster, scene-render, and presentation costs can be
distinguished without mixed workflows.

**Priority:** must-have

#### Acceptance Criteria

1. **AC2.1:** GIVEN two rotation captures form a comparison pair, WHEN their
   results are compared, THEN both SHALL use the same model, interaction
   duration, capture method, build class, and unrelated-dialog policy.
2. **AC2.2:** GIVEN a capture is recorded, WHEN its evidence is logged, THEN it
   SHALL include DockOverlay state, compact/frameless state, model, duration,
   tool/command shape, and whether the profiler attached before or after model
   loading.
3. **AC2.3:** GIVEN the 2026-05-24 runs are used, WHEN a clean disabled baseline
   is selected, THEN the corrected disabled-only rerun SHALL be used and the
   mixed-state run SHALL remain invalid for clean A/B conclusions.
4. **AC2.4:** WHILE rotation is being captured, document open, Preferences, and
   unrelated dialogs SHALL remain outside the measured interval.

### Requirement 3: Keep workstream boundaries explicit

**ID:** `R3`

**User Story:** As a maintainer or reviewer, I want rotation, document-open, and
Preferences evidence separated so that fixes can be reviewed and validated by
the correct owners.

**Priority:** must-have

#### Acceptance Criteria

1. **AC3.1:** GIVEN document-open evidence, WHEN conclusions or tasks are
   recorded, THEN `Gui::Document::RestoreDocFile` and
   `PartGui::ViewProviderPartExt::setupCoinGeometry` SHALL remain on the
   document-open track.
2. **AC3.2:** GIVEN Preferences-open evidence, WHEN it is summarized, THEN addon
   startup, module loading, and profiler-stop artifacts SHALL remain contextual
   to the Preferences track.
3. **AC3.3:** GIVEN compact/frameless UI or async recompute appears in evidence,
   WHEN its role is described, THEN it SHALL remain a controlled variable or
   rejected hypothesis unless fresh evidence directly implicates it.
4. **AC3.4:** WHEN a track is not selected, THEN it SHALL remain queued or be
   routed explicitly; it SHALL NOT become an implicit dependency of the selected
   slice.

### Requirement 4: Reconcile historical experiments with current source

**ID:** `R4`

**User Story:** As an implementer, I want the current source state checked before
acting on an old experiment so that branch ancestry cannot be mistaken for a
present implementation.

**Priority:** must-have

#### Acceptance Criteria

1. **AC4.1:** GIVEN a historical experiment is proposed for validation, WHEN the
   slice resumes, THEN its commit, branch, current-source delta, and dependency
   base SHALL be recorded.
2. **AC4.2:** IF an experiment commit is in current ancestry but its behavior is
   absent from current source, THEN the package SHALL classify it as historical
   evidence requiring a clean reapplication or recreation.
3. **AC4.3:** BEFORE source edits begin, the selected work SHALL use a permitted
   source branch rather than `main` or a merge-only integration branch.

### Requirement 5: Advance one narrow evidence-backed slice

**ID:** `R5`

**User Story:** As the branch owner, I want one investigation slice selected at
a time so that unrelated performance hypotheses do not become one unreviewable
change.

**Priority:** must-have

#### Acceptance Criteria

1. **AC5.1:** WHEN an investigation slice is selected, THEN the decision SHALL
   name its included workflow, excluded workflows, owner, evidence checkpoint,
   and next decision.
2. **AC5.2:** BEFORE an implementation target is accepted, THEN controlled
   evidence SHALL identify a bounded source-level cause and define a matching
   before/after check.
3. **AC5.3:** IF the selected experiment fails its evidence checkpoint, THEN it
   SHALL be rejected or routed without weakening acceptance criteria, and the
   next workstream decision SHALL be recorded.

### Requirement 6: Maintain lifecycle, review, and promotion integrity

**ID:** `R6`

**User Story:** As a future human or agent, I want complete traceability and
closure routing so that the active spec is safe to resume and does not become a
permanent source of stale truth.

**Priority:** must-have

#### Acceptance Criteria

1. **AC6.1:** GIVEN any task ID, WHEN its traceability row is read, THEN the
   linked requirements, acceptance criteria, design sections, verification,
   durable targets, and open decisions SHALL be recoverable.
2. **AC6.2:** GIVEN an unresolved decision or unselected track, WHEN its state is
   recorded, THEN it SHALL name an owner and a blocking milestone or explicit
   destination.
3. **AC6.3:** BEFORE source implementation is declared ready, independent review
   SHALL confirm evidence quality, scope, testability, branch safety, and
   residual risk.
4. **AC6.4:** BEFORE closure, lasting procedure, findings, and decisions SHALL be
   promoted or explicitly routed, and cleanup metadata SHALL be recorded.

## Correctness Properties

- **CP-001**: No completed task lacks concrete evidence appropriate to its
  acceptance criteria.
- **CP-002**: A capture or finding belongs to exactly one primary workflow:
  rotation, document open, or Preferences open.
- **CP-003**: Historical evidence remains available, but current source and fresh
  runtime evidence always override it for implementation decisions.
- **CP-004**: At most one source-backed implementation slice is active; other
  tracks remain queued or explicitly routed.

## Technical Context

- **Language/Version:** C++17-era FreeCAD GUI code, Qt 6, Coin3D/Quarter,
  OpenGL/GLX/Mesa on the recorded Linux environment
- **Primary Dependencies:** Qt Widgets/OpenGL, Coin3D/Quarter, Linux `perf`;
  VTune only as an escalation tool
- **Target Platform:** initial Linux/X11 investigation; cross-platform evidence
  is required before an upstream-ready GUI performance change
- **Constraints:** preserve CRLF in existing GUI files if a later source slice
  edits them; use named source branches; keep integration branches merge-only
- **Performance Goals:** no numeric improvement threshold is accepted yet; the
  selected slice must define one from controlled baseline evidence

## Success Criteria

- **SC-001:** Lifecycle lint, task-state audit, and Markdown checks complete with
  no package-attributable errors.
- **SC-002:** Every task is recognized by lifecycle tooling and has a
  bidirectional traceability row.
- **SC-003:** The next comparison pair records model, duration, configuration,
  capture shape, and results without reconstructing legacy context.
- **SC-004:** One narrow workstream is selected with an owner and evidence
  checkpoint; unrelated work remains explicitly separate.
- **SC-005:** No source implementation or closure readiness is claimed before
  its declared gates pass.

## Related Artifacts

- Canonical Context: `canonical-context.md`
- Design: `design.md`
- Tasks: `tasks.md`
- Traceability: `traceability.md`
- Verification: `verification.md`
- Research: `research.md`
- Quickstart: `quickstart.md`
- Open Decisions: `open-decisions.md`
