---
title: Sketcher diagnosis performance requirements
doc_type: spec
artifact_type: requirements
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# Requirements

## Introduction

Reduce duplicate native sketch diagnosis, provide optional phase timings and
measure remaining cost while preserving modelling behaviour. See [spec](spec.md)
for scenarios and scope. T003/T004 implement tracing and setup deferral. T005
correctness and bounded measurements are recorded in performance-validation-report.md;
indicative timing acceptance is satisfied under timing-evidence-decision.md; final
attribution and promotion remain pending.

## Goals

Deliver the faster-recompute, phase-attribution and measured follow-up outcomes
in [spec.md](spec.md#goals), without changing modelling intent.

## Non-Goals

The exclusions in [spec.md](spec.md#non-goals) apply to every requirement.

## Durable Source Baseline

The code-derived starting baseline is commit `36aa20c979`, recorded in
[research](research.md). No dedicated durable Sketcher performance reference
exists in this documentation partition. Repository templates govern document
shape; they do not establish implemented solver semantics. Historical private
logs support candidate selection only.

## Durable Impact

Add verified behaviour and interfaces to proposed
`docs/reference/sketcher-performance-diagnostics.md` and workflow to proposed
`docs/runbooks/sketcher-performance-investigation.md` during T007.

## Staged Readiness

- T001 baseline and T002 timing/invalidation contract are complete; their evidence
  is in baseline-report.md and timing-invalidation-contract.md.
- T003 tracing and T004 setup deferral are implemented and validated. T005 bounded
  measurements and correctness checks satisfy the revised indicative AC-005/AC-006.
  T005 is complete; T006 attribution and T007 promotion/closure remain pending.
- Code changes require the task dependencies and design decisions to be satisfied.
- No design-first exception; measurable requirements precede implementation.

## Requirements

### Requirement 1: One authoritative ordinary diagnosis (FR-001)

**Priority:** must-have

For ordinary valid document recompute, the affected `SketchObject::execute()`
  path MUST perform one authoritative full setup/diagnosis before numerical solving.
  This invariant MUST NOT suppress required setups in scalar/list insertion,
  edit-mode `noRecomputes`, direct property changes, undo/redo or movement paths.
  Paths that reject input before solving MAY perform none. Deliberately required
  block-constraint or augmented-solver analysis MUST be identified separately.

#### Acceptance Criteria

1. WHEN an ordinary sketch recompute executes, THE SYSTEM SHALL satisfy AC-001 below.

### Requirement 2: Preserve modelling behaviour (FR-002)

**Priority:** must-have

The optimization MUST preserve geometry, constraints, external
  references, shape validity, degrees of freedom, diagnostics and undo/redo.
  It MUST retain constraint-index validation, required property notifications,
  diagnostic constraint identities and existing error/exception behavior. Valid,
  invalid and repaired index transitions MUST remain distinguishable. Before any
  guard is selected, T002 MUST define validation ownership and observable touched/
  observer invariants, including which intermediate notifications may differ.

#### Acceptance Criteria

1. WHEN the recompute optimization is exercised, THE SYSTEM SHALL satisfy AC-002 below.

### Requirement 3: Correlate optional timing records (FR-003)

**Priority:** must-have

Optional diagnostics MUST identify document, sketch, invocation
  reason, nested operation and phase. They MUST distinguish inclusive total time
  from child durations without presenting overlapping durations as additive.
  Records MUST include operation-relative monotonic start/end offsets and an
  execution-lane identifier; QR dispatch/copy and join/wait spans MUST distinguish
  overlapping and deferred execution without changing the existing launch policy.

#### Acceptance Criteria

1. WHEN phase tracing is enabled, THE SYSTEM SHALL satisfy AC-003 below.

### Requirement 4: Attribute every expensive phase (FR-004)

**Priority:** must-have

Diagnostics MUST cover total sketch operation, setup construction,
  Jacobian assembly, both QR decompositions, diagnostic redundancy solving,
  numerical solving, geometry writeback and shape construction. The benchmark
  MUST separately capture downstream document recompute; bridge validation is
  an external observation when that workload is measured. Once per diagnosis,
  enabled tracing MUST record driving constraint count, expanded diagnostic
  equation count, diagnostic unknown count, Jacobian rows/columns and estimated
  payload bytes, selected QR method, and resulting rank. Counts MUST identify
  their stage so changes in equation reduction are not mistaken for speedups.
  An unavailable rank on failure MUST be explicit rather than reported as zero.

#### Acceptance Criteria

1. WHEN a measured sketch operation executes, THE SYSTEM SHALL satisfy AC-003 below.

### Requirement 5: Keep diagnostics optional and compatible (FR-005)

**Priority:** must-have

Detailed diagnostics MUST be disabled by default, preserve existing
  Python timing semantics, use a monotonic clock, and avoid output/allocation
  work on the disabled path beyond a bounded enablement check.

#### Acceptance Criteria

1. WHEN tracing is disabled, THE SYSTEM SHALL satisfy AC-004 and AC-006 below.

### Requirement 6: Use matched performance evidence (FR-006)

**Priority:** must-have

Performance evidence MUST compare equivalent fixtures, operations,
  settings and build configurations, including tracing-disabled results.
  Workload execution/reset/cache protocols MUST be frozen in T001 and verified
  with execute/setup counts. Clean no-op and forced unchanged-geometry recompute
  MUST be separate cases; a skipped sketch MUST NOT count as evidence of improvement.
  Acceptance MUST use the indicative evidence protocol in verification.md: repeated
  comparable timings, spread and caveats, supported by actual setup counts and
  unchanged modelling results. Formal confidence-bound gates are not required.

#### Acceptance Criteria

1. WHEN performance acceptance is assessed, THE SYSTEM SHALL satisfy AC-005 and AC-006 below.

### Requirement 7: Select further work from measurements (FR-007)

**Priority:** must-have

Further optimizations MUST be selected only after attributing
  remaining cost; unselected proposals MUST have an explicit disposition.

#### Acceptance Criteria

1. WHEN further optimization is proposed, THE SYSTEM SHALL satisfy AC-007 below.

## Acceptance Criteria

1. **AC-001**: Given an affected ordinary recompute fixture, when it executes,
   then regression evidence counts one full setup instead of the baseline two,
   with unchanged successful output. Invalid/early-return cases remain recoverable.
2. **AC-002**: Given valid, underconstrained, conflicting, redundant and malformed
   fixtures, when recompute, external-link updates and undo/redo run, then their
   prior observable results match. Include driving/reference changes and
   source-linked Pad/Pocket thickness propagation with restoration. Assert
   valid-to-invalid and invalid-to-repaired indices after geometry/external-link
   changes; compare diagnostic constraint IDs, required touched state and observer
   sequence/count against the T002 contract. Include scalar/list insertion with
   transaction undo, edit-mode `noRecomputes`, direct property changes, and undo/
   redo immediately followed by solve/move, without an intervening full recompute.
3. **AC-003**: Given tracing enabled, when an operation succeeds or fails, then
   phase records correlate to their sketch/reason and close without dangling
   scopes; diagnostic redundancy solves are distinct from the main solve.
   Reconstruct overlapping and deferred QR timelines using offsets, lanes and
   dispatch/join spans; record the structural counters required by FR-004 once
   per diagnosis, with correct dimensions/method/rank on a known small fixture.
4. **AC-004**: Given tracing disabled, when the same operations run, then no new
   detailed records are emitted and existing timing getters retain their meaning.
5. **AC-005**: Given comparable optimized builds, the same frozen workloads and
   settings, warmups and several repeated samples per configuration, record raw
   timings, medians and spread. Demonstrate an indication of useful improvement
   on a representative diagnosis-heavy workload, supported by actual execution/
   setup counts and unchanged modelling results. Review the other measured
   workloads; investigate and explicitly disposition any apparent repeatable
   material slowdown. Disclose variability and scope. No minimum percentage,
   mandatory bootstrap interval or controlled-host validation is required.
6. **AC-006**: Given comparable baseline and instrumentation-only builds with
   tracing disabled, record indicative timing differences and variability on
   representative workloads. Investigate and disposition any apparent repeatable
   material slowdown attributable to instrumentation. Uncertain or near-flat
   measurements may be reported with their limitations without blocking delivery;
   do not claim proven zero overhead or a guaranteed upper bound.

7. **AC-007**: Given the phase results, then publish a ranked follow-up decision
   covering unchanged metadata, matrix copies, structural reuse and sparse
   assembly, with measured justification and a destination for deferred work.

## Correctness Properties

- **CP-001**: Removing a redundant setup does not change geometry, links, DoF,
  diagnostic classifications, recoverable errors or undo/redo results (FR-001/FR-002).
- **CP-002**: Timing instrumentation does not alter operation semantics, existing
  timing getters or disabled output behaviour (FR-003/FR-004/FR-005).
- **CP-003**: Comparison evidence identifies equivalent starting states and
  configurations; historical logs never substitute for new benchmarks (FR-006/FR-007).

## Success Criteria

Meet SC-001 through SC-004 in [spec.md](spec.md#success-criteria), with acceptance
evidence for AC-001 through AC-007 in [verification.md](verification.md).

## Related Artifacts

[Specification](spec.md), [Design](design.md), [Tasks](tasks.md),
[Traceability](traceability.md), [Verification](verification.md).

Timing acceptance was revised by explicit user direction on 2026-11-05; see
[timing-evidence-decision.md](timing-evidence-decision.md). Earlier strict-gate
statistics remain historical evidence, not current completion criteria.
