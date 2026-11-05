---
title: Sketcher diagnosis performance tasks
doc_type: spec
artifact_type: tasks
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# Tasks

**Input**: [Specification](spec.md), [Requirements](requirements.md),
[Traceability](traceability.md), [Design](design.md), [Plan](plan.md),
[Research](research.md), [Verification](verification.md).

All implementation tasks are pending. This package records proposed work;
no performance source changes, new benchmarks or builds have been completed.

## Phase 1 - Baseline and contract

- [ ] T001 Reconcile source and establish the baseline
  - Depends on: none
  - Requirement: Requirement 1/Requirement 2/Requirement 6; AC-001/AC-002/AC-005; D-001/D-003.
  - Files: `src/Mod/Sketcher/App/SketchObject.cpp`, `src/Mod/Sketcher/App/SketchObjectConstraints.cpp`, `src/Mod/Sketcher/App/PropertyConstraintList.cpp`, `tests/src/Mod/Sketcher/App/`
  - Inspect `src/Mod/Sketcher/App/SketchObject.cpp`, `SketchObjectConstraints.cpp`,
    `PropertyConstraintList.cpp`, native test registration and existing trace settings.
  - Create a minimal call-count fixture and identify external-link/undo test targets.
  - Capture baseline raw samples and fixture/settings/build identity; keep originals private.
  - Acceptance: reproduce two setups on the affected ordinary execute path; characterize
    exception paths and establish an executable test/benchmark command list.
  - Evidence: baseline receipts, call graph/counts, fixture hashes and exact commands.

- [ ] T002 Finalize timing and invalidation boundaries
  - Depends on: T001
  - Requirement: Requirement 3/Requirement 4/Requirement 5; AC-003/AC-004/AC-006; D-001/D-002.
  - Files: `src/Mod/Sketcher/App/Sketch.cpp`, `src/Mod/Sketcher/App/Sketch.h`, `src/Mod/Sketcher/App/planegcs/GCS.cpp`, `src/Mod/Sketcher/App/planegcs/GCS.h`, `design.md`
  - Inspect `Sketch.cpp`, `Sketch.h`, `planegcs/GCS.cpp`, `GCS.h` and existing reporting.
  - Record exact opt-in control, trace schema, reason/parent linkage, disabled fast path,
    worker lifetime/output safety and scope nesting. Select the local recompute guard.
  - Acceptance: resolve design open questions affecting T003/T004; define test seams
    without adding a public modelling API or global notification bypass.
  - Evidence: updated design decisions and a requirements-linked validation plan.

## Phase 2 - First changes

- [ ] T003 Add optional phase timings and regression coverage
  - Depends on: T002
  - Requirement: Requirement 3/Requirement 4/Requirement 5; AC-003/AC-004/AC-006; D-002.
  - Files: `src/Mod/Sketcher/App/Sketch.cpp`, `src/Mod/Sketcher/App/planegcs/GCS.cpp`, `tests/src/Mod/Sketcher/App/`
  - Implement scoped timing at the approved Sketcher/planegcs boundaries and native tests
    under `tests/src/Mod/Sketcher/App/`; update target wiring only if needed.
  - Acceptance: success/failure scopes close, identity/reasons correlate, worker output is
    safe, tracing off is silent, existing `lastSolveTime` meaning is unchanged.
  - Evidence: focused test results, example redacted trace and disabled-path inspection.

- [ ] T004 Remove duplicate ordinary recompute diagnosis
  - Depends on: T003
  - Requirement: Requirement 1/Requirement 2; AC-001/AC-002; D-001.
  - Files: `src/Mod/Sketcher/App/SketchObject.cpp`, `tests/src/Mod/Sketcher/App/`
  - Make the bounded change in `SketchObject.cpp` using the approved scoped mechanism.
    Add call-count and behavioural regressions in the selected native test files.
  - Acceptance: one authoritative setup in the affected valid path; unchanged errors,
    notifications, geometry, DoF, diagnostics, external updates and undo/redo.
  - Evidence: before/after deterministic counts, regression results and reviewed diff.

## Phase 3 - Measurement and follow-up

- [ ] T005 Validate correctness and measure end-to-end impact
  - Depends on: T004
  - Requirement: Requirement 2/Requirement 6; AC-002/AC-005/AC-006; D-003.
  - Files: `verification.md`, private benchmark receipts
  - Run build/focused and affected suites; compare synthetic/master/plate workloads
    in equivalent optimized builds. Use disposable GUI processes for GUI-only checks.
  - Acceptance: correctness matrix passes; raw runs, median/spread and all budget
    dispositions recorded, including tracing-off overhead. No gains inferred from logs alone.
  - Evidence: exact commands, output receipts, geometry/diagnostic comparisons and timing data.

- [ ] T006 Attribute remaining costs and decide further work
  - Depends on: T005
  - Requirement: Requirement 7; AC-007; D-004.
  - Files: `research.md`, `design.md`, follow-up destinations
  - Use measured phases to evaluate metadata rebuilds, matrix copies, structural reuse
    and sparse assembly. Revisit the historical symmetry case with exact input authority.
  - Acceptance: ranked candidates identify measured cost, correctness risk, owner and
    destination; rejected/deferred work has a rationale. No unmeasured rewrite starts.
  - Evidence: decision table and bounded follow-up spec/backlog references when needed.

- [ ] T007 Promote verified knowledge and reconcile delivery
  - Depends on: T006
  - Requirement: Requirement 1 through Requirement 7; AC-001 through AC-007; D-004.
  - Files: `docs/reference/sketcher-performance-diagnostics.md`, `docs/runbooks/sketcher-performance-investigation.md`, `verification.md`
  - Create `docs/reference/sketcher-performance-diagnostics.md` and proposed runbook
    with implemented interfaces, measured limits, commands and recovery guidance.
  - Acceptance: traceability/verification states match evidence; source/docs commits,
    remaining work and integration disposition are recorded before closure.
  - Evidence: durable docs, review disposition and lifecycle closure checks.

## Dependencies And Execution Order

T001 -> T002 -> T003 -> T004 -> T005 -> T006 -> T007.
Mark a task in progress before implementation; complete it only after its declared
evidence and acceptance pass. Read the full package on resume, not this file alone.
