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

T001 through T006 are complete. Ordinary recompute uses one authoritative setup;
correctness and indicative timing evidence satisfy timing-evidence-decision.md.
T006 attribution and ranked follow-ups are recorded in remaining-cost-decisions.md
and the owned local backlog. T007 promotion/review/closure is next.

## Phase 1 - Baseline and contract

- [x] T001 Reconcile source and establish the baseline
  - Depends on: none
  - Requirement: Requirement 1/Requirement 2/Requirement 6; AC-001/AC-002/AC-005; D-001/D-003.
  - Files: `src/Mod/Sketcher/App/SketchObject.cpp`, `src/Mod/Sketcher/App/SketchObjectConstraints.cpp`, `src/Mod/Sketcher/App/PropertyConstraintList.cpp`, `tests/src/Mod/Sketcher/App/`, `tools/profile/sketcher_baseline.py`, `tools/profile/run_sketcher_baseline.py`, `tools/profile/sketcher_call_counts.cpp`, `baseline-report.md`, `workloads.json`
  - Inspect `src/Mod/Sketcher/App/SketchObject.cpp`, `SketchObjectConstraints.cpp`,
    `PropertyConstraintList.cpp`, native test registration and existing trace settings.
  - Create a minimal call-count fixture and identify external-link/undo test targets.
  - Freeze each workload's execution, reset and cache protocol: separate clean no-op,
    forced unchanged-geometry execution, fresh-process and reopened loads. Assert
    execute/setup counts for every sample and reject skipped/mismatched pairs.
  - Capture baseline raw paired samples and fixture/settings/build identity under
    verification.md's measurement protocol; keep originals private.
    Baseline A/A pairs characterize repeatability; candidate and tracing-overhead
    comparisons remain T005 and cannot be inferred from these samples.
  - Acceptance: reproduce two setups on the affected ordinary execute path; characterize
    exception paths and establish an executable test/benchmark command list.
  - Evidence: Release -O3 build passed at b6141a373c; isolated Sketcher_tests_run 109/109 and external-reference Python tests 2/2 passed. Nine workloads completed two warmups plus ten alternating A/A pairs each, zero rejected pairs, with actual native execute/setup vectors and model-state/reset checks. Valid/redundant/conflicting/invalid-solver Release probes verified. See baseline-report.md, workloads.json and private T001-completion build/raw/summary receipts. Formatting, Python compilation, local links/JSON and whitespace checks passed; package lint zero errors, two previously documented advisories.

  - Status: Baseline complete. T002 ready; historical symmetry explicitly unverified and owned by T006. A/A timing noise recorded for T005; candidate speedup/regression/overhead acceptance remains pending.
  - Evidence mode: artifact
- [x] T002 Finalize timing and invalidation boundaries
  - Depends on: T001
  - Requirement: Requirement 1/Requirement 2/Requirement 3/Requirement 4/Requirement 5; AC-001/AC-002/AC-003/AC-004/AC-006; D-001/D-002.
  - Files: `src/Mod/Sketcher/App/Sketch.cpp`, `src/Mod/Sketcher/App/Sketch.h`, `src/Mod/Sketcher/App/planegcs/GCS.cpp`, `src/Mod/Sketcher/App/planegcs/GCS.h`, `design.md`, `timing-invalidation-contract.md`, `tests/src/Mod/Sketcher/App/SketchObjectDiagnosis.cpp`, adjacent test registration
  - Inspect `Sketch.cpp`, `Sketch.h`, `planegcs/GCS.cpp`, `GCS.h` and existing reporting.
  - Record opt-in control, operation-relative monotonic offsets, lane/parent linkage,
    dispatch/join spans, structural counter stages, disabled fast path and worker safety.
  - Before selecting a guard, assign explicit ownership of index validation and
    required diagnostic-ID/touched/observer effects. Specify baseline sequences,
    allowed intermediate differences and valid-invalid-repaired transition tests.
  - Acceptance: resolve design open questions affecting T003/T004; approve the
    validation/observer contract before guard selection. Define test seams without
    a public modelling API, global notification bypass or async launch-policy change.
  - Evidence: C-001 through C-006 in timing-invalidation-contract.md resolve callback validation/deferral and diagnostic notification ownership, allowed intermediate differences, immutable trace opt-in/schema/disabled path, QR worker lifetime/dispatch/join with unchanged copies/policy/wait, and stage-labelled structural counters. Six native baseline probes record observer sequences/touched states/diagnostic IDs and invalid-index repair. Release Sketcher_tests_run build passed; isolated full suite 115/115 passed, native XML/receipts retained in private T002-contract artifacts. Focused pre-commit passed; local file/anchor links, JSON and whitespace checks passed; package lint zero errors and two previously documented advisories. Production src/ unchanged.

  - Evidence mode: artifact
  - Status: Design contract complete; T003 ready. Guard/tracing implementation and full non-target/exception/GUI proof remain T003/T004/T005; no production acceptance claimed.
## Phase 2 - First changes

- [x] T003 Add optional phase timings and regression coverage
  - Depends on: T002
  - Requirement: Requirement 3/Requirement 4/Requirement 5; AC-003/AC-004/AC-006; D-002.
  - Files: `src/Mod/Sketcher/App/SketchObject.cpp`, `src/Mod/Sketcher/App/SketchObjectConstraints.cpp`, `src/Mod/Sketcher/App/Sketch.cpp`, `src/Mod/Sketcher/App/Sketch.h`, `src/Mod/Sketcher/App/planegcs/GCS.cpp`, `src/Mod/Sketcher/App/planegcs/GCS.h`, `tests/src/Mod/Sketcher/App/`
  - Implement scoped timing at the approved Sketcher/planegcs boundaries and native tests
    under `tests/src/Mod/Sketcher/App/`; update target wiring only if needed.
    Follow C-003 through C-006 in `timing-invalidation-contract.md`; source scope
    includes the private SketcherTrace helper and actual movement entry in
    `SketchObjectOperations.cpp` for reason correlation. Implementation evidence
    is in `phase-tracing-report.md` and the 21 new native tracing tests.
  - Acceptance: success/failure scopes close, identity/reasons correlate, overlapping
    and deferred QR timelines reconstruct from offsets/lanes/dispatch/join spans;
    known-fixture counters match equations, unknowns, matrix size, QR method and rank.
    Worker output is safe, tracing off is silent with no counter-only work, and
    existing `lastSolveTime` meaning and async launch policy remain unchanged.
  - Evidence: Implemented C-003 through C-006 in private SketcherTrace helper and SketchObject/Sketch/planegcs entry/phase scopes. Twenty-one new tracing tests verify disabled work/output, exceptions/loss, context restoration, dense/sparse counters, block ordinals, actual invocation reasons, async/deferred intervals, unchanged payload copies and wait/deferred-discard semantics. Release full suite passes 136/136 with tracing off and on; Debug full suite passes 136/136. Five isolated FreeCADCmd control modes produce equal modelling state; on emits 194 complete validated JSON records, other modes none. Final pre-commit, line endings, local links/anchors/example JSON, CRLF-aware whitespace passed; package lint zero errors/two existing advisories. See phase-tracing-report.md, phase-trace-example.jsonl and private T003-phase-tracing source/binary/hash/raw receipts.

  - Evidence mode: artifact
  - Status: T003 complete; T004 ready. Duplicate setup and baseline observer effects retained. AC-003/AC-004 verified on Linux; AC-006 matched overhead and broader GUI/correctness/performance acceptance remain T005. Committed as root 9bc0eb91ff and docs 88b3ce6.
- [x] T004 Remove duplicate ordinary recompute diagnosis
  - Depends on: T003
  - Requirement: Requirement 1/Requirement 2; AC-001/AC-002; D-001.
  - Files: `src/Mod/Sketcher/App/SketchObject.cpp`, `src/Mod/Sketcher/App/SketchObjectConstraints.cpp`, `src/Mod/Sketcher/App/SketchObject.h`, `tests/src/Mod/Sketcher/App/`, `src/Mod/Sketcher/SketcherTests/TestSketcherSolver.py`, `tools/profile/run_sketcher_baseline.py`, `tools/profile/sketcher_baseline.py`, `diagnosis-deferral-report.md`
  - Implement C-001/C-002 in `timing-invalidation-contract.md`: one-shot deferral
    after callback validation and private authoritative-solve notification handoff.
    Retain legacy fallback/exception paths and all non-target solves.
    Add call-count and behavioural regressions in the selected native test files.
  - Test valid-to-invalid and invalid-to-repaired indices after geometry/external
    changes; assert diagnostic constraint IDs, touched state and observer sequence/
    counts against T002's approved contract for non-success cases.
  - Test scalar/list Python insertion with transaction undo, edit-mode `noRecomputes`,
    direct property changes and undo/redo immediately followed by solve/move.
  - Acceptance: one authoritative setup only in the targeted valid execute path;
    preserve required additional setups elsewhere and the validation/observer
    contract, geometry, DoF, recoverable diagnostics, external links and undo/redo.
  - Evidence: Implemented C-001/C-002 private one-shot deferral after normal index/geometry validation, authoritative diagnostic touch, notified-input refresh and legacy exception/fallback recovery. Final Release/Debug targeted builds passed. Native suite 149/149 in Release tracing off/on and Debug; Python solver suite 27 run, 26 passed and one existing skip in each build. Independent tracing-off candidate vectors verify ordinary setup 2->1, scalar 18->17 and list 3->2 with required non-target solves retained. Matched Debug frozen-model comparison and extra Release comparison each passed 153,993 state checks; constraints/geometry/diagnostics/expressions/shapes and thickness perturb/restore preserved. Focused pre-commit, Black, parsing, links/anchors, CRLF preservation and whitespace passed; lint zero errors/two existing advisories. See diagnosis-deferral-report.md and private T004-diagnosis-deferral source/hash/XML/model/count receipts.

  - Evidence mode: artifact
  - Status: T004 complete; T005 ready. AC-001 and core correctness verified. Matched elapsed speedup/regression/tracing-off overhead budgets and broader disposable GUI checks remain T005; no performance percentage claimed.
## Phase 3 - Measurement and follow-up

- [x] T005 Validate correctness and measure end-to-end impact
  - Depends on: T004
  - Requirement: Requirement 2/Requirement 6; AC-002/AC-005/AC-006; D-003.
  - Files: `verification.md`, `performance-validation-report.md`, `comparison-protocol.md`, `timing-evidence-decision.md`, `performance-results.json`, `tools/profile/run_sketcher_comparison.py`, `tools/profile/sketcher_baseline.py`, `tools/profile/test_sketcher_comparison.py`, private benchmark receipts
  - Run build/focused and affected suites; compare synthetic/master/plate workloads
    in equivalent optimized builds. Use disposable GUI processes for GUI-only checks.
  - Acceptance: correctness matrix passes; workload execution/reset checks pass.
    Record comparable repeated timings, medians/spread, execution/setup counts,
    rejected-sample reasons and caveats, including tracing-disabled observations.
    Establish an indication of improvement supported by correctness/count evidence;
    investigate apparent repeatable material slowdowns. Strict percentage and
    confidence-bound gates are not required under timing-evidence-decision.md.
  - Evidence: Reassessed on 2026-11-05 under the user's explicit request for an indication of improvement rather than strict performance measurements. Revised AC-005/AC-006 and timing-evidence-decision.md keep correctness, equivalent inputs/settings/builds and actual execution/setup validation strict, while making confidence/percentage gates diagnostic. Existing evidence suffices: setups two to one; representative master/profile medians approximately 47%/57% lower; 190 matched triples/570 timed samples retained; native 149/149, affected Python 97 run with one existing skip, full model comparisons and nine bounded GUI stages pass. Other workload and disabled-overhead observations reviewed with variability disclosed; no guaranteed absence of regression/overhead claimed. Original raw data, per-batch/pooled statistics and inconclusive strict-gate outcomes unchanged. No additional timing runs or model edits required.

  - Evidence mode: artifact
  - Status: T005 complete under user-approved indicative timing criteria; T006 ready. The former strict AC-005/AC-006 results remain historical and are not relabelled passed. Controlled-host reruns are optional follow-up. Correctness/execution contracts unchanged; final attribution/promotion/review/closure remain T006/T007. Committed as root 27d524eaaa and docs 43e66da.
- [x] T006 Attribute remaining costs and decide further work
  - Depends on: T005
  - Requirement: Requirement 7; AC-007; D-004.
  - Files: `research.md`, `design.md`, `remaining-cost-decisions.md`, `remaining-cost-attribution.json`, `docs/backlog/README.md`, `docs/backlog/sketcher-performance.md`, follow-up destinations
  - Use measured phases to evaluate metadata rebuilds, matrix copies, structural reuse
    and sparse assembly. Revisit the historical symmetry case with exact input authority.
  - Acceptance: ranked candidates identify measured cost, correctness risk, owner and
    destination; rejected/deferred work has a rationale. No unmeasured rewrite starts.
  - Evidence: Attributed retained and updated-model phases using actual root execute spans and clipped interval unions, avoiding nested-operation and overlapping QR double counting. Retained complex QR compute covers roughly64–85% of execute; explicit acceptance0.02–0.05%, dispatch0.4–1.2%. Frozen updated copy527589a0... qualifies all13 sketches:39 untraced plus13 traced samples, each one execute/setup with unchanged geometry/constraints/DoF, all sketch shapes valid and no initial Invalid objects. Updated heavy-sketch QR unions40–59%; wiring/auxiliary shape work about81%. Ranked B001–B007 backlog records measured opportunities/bounds, correctness risks, local owner, prerequisites and exit evidence for QR-method comparison, mutation replay, shape construction, copies, metadata, structural reuse and sparse assembly. Historical starting checkpoint recovered and hash5bb891ab... verified; immediate pre-symmetry/recent Equality precursors remain unverified and explicitly deferred to B006. No mutation replay or solver rewrite started. See remaining-cost-decisions.md, remaining-cost-attribution.json and docs/backlog/README.md.

  - Evidence mode: artifact
  - Status: T006 complete; T007 ready for final review/promotion/closure. Follow-ups remain proposed/deferred and owned in the valid slm-planning/v1 backlog. T005 committed root27d524eaaa/docs43e66da. T006 changes are documentation/analysis only and uncommitted; no production defaults, tolerances, async policy, caches or operator models changed.
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
