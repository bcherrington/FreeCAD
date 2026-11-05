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
  - Freeze each workload's execution, reset and cache protocol: separate clean no-op,
    forced unchanged-geometry execution, fresh-process and reopened loads. Assert
    execute/setup counts for every sample and reject skipped/mismatched pairs.
  - Capture baseline raw paired samples and fixture/settings/build identity under
    verification.md's measurement protocol; keep originals private.
  - Acceptance: reproduce two setups on the affected ordinary execute path; characterize
    exception paths and establish an executable test/benchmark command list.
  - Evidence: baseline receipts, call graph/counts, fixture hashes and exact commands.

- [ ] T002 Finalize timing and invalidation boundaries
  - Depends on: T001
  - Requirement: Requirement 1/Requirement 2/Requirement 3/Requirement 4/Requirement 5; AC-001/AC-002/AC-003/AC-004/AC-006; D-001/D-002.
  - Files: `src/Mod/Sketcher/App/Sketch.cpp`, `src/Mod/Sketcher/App/Sketch.h`, `src/Mod/Sketcher/App/planegcs/GCS.cpp`, `src/Mod/Sketcher/App/planegcs/GCS.h`, `design.md`
  - Inspect `Sketch.cpp`, `Sketch.h`, `planegcs/GCS.cpp`, `GCS.h` and existing reporting.
  - Record opt-in control, operation-relative monotonic offsets, lane/parent linkage,
    dispatch/join spans, structural counter stages, disabled fast path and worker safety.
  - Before selecting a guard, assign explicit ownership of index validation and
    required diagnostic-ID/touched/observer effects. Specify baseline sequences,
    allowed intermediate differences and valid-invalid-repaired transition tests.
  - Acceptance: resolve design open questions affecting T003/T004; approve the
    validation/observer contract before guard selection. Define test seams without
    a public modelling API, global notification bypass or async launch-policy change.
  - Evidence: updated design decisions and a requirements-linked validation plan.

## Phase 2 - First changes

- [ ] T003 Add optional phase timings and regression coverage
  - Depends on: T002
  - Requirement: Requirement 3/Requirement 4/Requirement 5; AC-003/AC-004/AC-006; D-002.
  - Files: `src/Mod/Sketcher/App/SketchObject.cpp`, `src/Mod/Sketcher/App/SketchObjectConstraints.cpp`, `src/Mod/Sketcher/App/Sketch.cpp`, `src/Mod/Sketcher/App/Sketch.h`, `src/Mod/Sketcher/App/planegcs/GCS.cpp`, `src/Mod/Sketcher/App/planegcs/GCS.h`, `tests/src/Mod/Sketcher/App/`
  - Implement scoped timing at the approved Sketcher/planegcs boundaries and native tests
    under `tests/src/Mod/Sketcher/App/`; update target wiring only if needed.
  - Acceptance: success/failure scopes close, identity/reasons correlate, overlapping
    and deferred QR timelines reconstruct from offsets/lanes/dispatch/join spans;
    known-fixture counters match equations, unknowns, matrix size, QR method and rank.
    Worker output is safe, tracing off is silent with no counter-only work, and
    existing `lastSolveTime` meaning and async launch policy remain unchanged.
  - Evidence: focused test results, example redacted trace and disabled-path inspection.

- [ ] T004 Remove duplicate ordinary recompute diagnosis
  - Depends on: T003
  - Requirement: Requirement 1/Requirement 2; AC-001/AC-002; D-001.
  - Files: `src/Mod/Sketcher/App/SketchObject.cpp`, `tests/src/Mod/Sketcher/App/`
  - Make the bounded change in `SketchObject.cpp` using the approved scoped mechanism.
    Add call-count and behavioural regressions in the selected native test files.
  - Test valid-to-invalid and invalid-to-repaired indices after geometry/external
    changes; assert diagnostic constraint IDs, touched state and observer sequence/
    counts against T002's approved contract for non-success cases.
  - Test scalar/list Python insertion with transaction undo, edit-mode `noRecomputes`,
    direct property changes and undo/redo immediately followed by solve/move.
  - Acceptance: one authoritative setup only in the targeted valid execute path;
    preserve required additional setups elsewhere and the validation/observer
    contract, geometry, DoF, recoverable diagnostics, external links and undo/redo.
  - Evidence: before/after deterministic counts, regression results and reviewed diff.

## Phase 3 - Measurement and follow-up

- [ ] T005 Validate correctness and measure end-to-end impact
  - Depends on: T004
  - Requirement: Requirement 2/Requirement 6; AC-002/AC-005/AC-006; D-003.
  - Files: `verification.md`, private benchmark receipts
  - Run build/focused and affected suites; compare synthetic/master/plate workloads
    in equivalent optimized builds. Use disposable GUI processes for GUI-only checks.
  - Acceptance: correctness matrix passes; workload execution/reset checks pass.
    Record raw alternating pairs, medians, MAD, bootstrap intervals, rejected-pair
    reasons and budget dispositions, including tracing-off overhead. Apply the
    bounded additional-batch rule; unresolved threshold overlap is inconclusive
    and cannot pass acceptance. No gains inferred from logs or medians alone.
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
