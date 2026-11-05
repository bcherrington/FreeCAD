---
title: Sketcher diagnosis performance verification
doc_type: spec
artifact_type: verification
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# Verification

## Current State

Spec authoring only. No implementation, performance benchmark, new native test
or build result is claimed. Existing crash-fix tests establish the branch's
starting context only; they do not satisfy this package's acceptance criteria.

## Acceptance And Traceability

| Requirement | Design | Tasks | Cases | State |
| --- | --- | --- | --- | --- |
| FR-001 | D-001 | T001, T004 | AC-001 | Pending |
| FR-002 | D-001, D-003 | T001, T004, T005 | AC-002 | Pending |
| FR-003 | D-002 | T002, T003 | AC-003 | Pending |
| FR-004 | D-002 | T002, T003 | AC-003 | Pending |
| FR-005 | D-002 | T002, T003, T005 | AC-004, AC-006 | Pending |
| FR-006 | D-003 | T001, T005 | AC-005, AC-006 | Pending |
| FR-007 | D-004 | T006, T007 | AC-007 | Pending |

## Correctness Matrix

- Valid, empty and underconstrained sketches: count setup and compare geometry/DoF.
- Conflicts, redundancies, partial redundancies and malformed constraints:
  compare classification and recoverable failure behaviour.
- External geometry update/migration and driver/reference changes: preserve links,
  values, notifications and resulting constraint validity.
- Undo/redo: restore geometry, constraints and solver state across those operations.
- Pad/Pocket dimension perturbation/restoration: valid solid, correct lengths and extents.
- Tracing enabled/disabled and exceptions: scope closure, correlation, no disabled
  output, unchanged timing-getter semantics and safe QR worker handling.

## Measurement Matrix

Measure load, unchanged recompute, one dimensional edit, scalar/list insertion
and the historical symmetry workload when the exact precursor is available.
A missing precursor is an explicit unverified case, not an invented replacement.
At least two warmups and ten recorded samples per configuration are required;
report cold load separately. Keep original files unchanged and reset state per run.

Record source/build identity, compiler/dependencies, hardware, solver settings,
fixture hashes, trace state, operation, raw samples, median and spread. Compare
baseline, instrumentation-only and optimized configurations. Attribute phases
with tracing enabled; enforce total latency/overhead budgets with tracing off.
Do not use process survival as a substitute for valid geometry.

## Commands And Evidence

T001 must discover the registered native targets and exact CTest filters before
writing runnable focused commands. Normal build: `pixi run build -j 2`.
Affected suites, manual GUI checks and pre-commit outcomes must be recorded with
results. A skipped check needs a reason and remaining risk. Private traces/models
stay outside Git; promote a redacted summary and reproducible synthetic fixture.

## Review And Closure

Review the scoped guard, notifications, diagnostic semantics, timing fast path,
async lifetimes, cross-platform build risk and meaningful regression coverage.
Record each finding's disposition. T007 requires implemented diagnostic/reference
and investigation guidance plus evidence for all criteria and deferred-work routing.
Structural lint success alone does not make the package ready to implement or close.

## Quality Gates

- Authoring: required artifacts, links and task dependency/traceability checks pass.
- Implementation: T001/T002 decisions recorded before T003/T004.
- Validation: AC-001 through AC-006 backed by actual tests/measurements.
- Closure: AC-007, review disposition, promotion and residual ownership complete.

## Evidence Log

On 2026-11-05, Spec Lifecycle Manager package lint reported zero errors and two
advisories: EARS detection and optional canonical-context review. The requirements
contain explicit WHEN/THE SYSTEM SHALL clauses linked to observable AC definitions;
this does not establish delivery acceptance. Authority review is recorded in
[traceability.md](traceability.md#authority-review); no separate imported-authority
artifact is needed. Local Markdown link and whitespace checks passed.
No native performance acceptance evidence exists yet. T001 creates the baseline.
Numbering remains provisional-confidence because older packages are unnumbered
and no history index exists; creation used the tool's revalidated proposal.

## Residual Risks

Source pointers can drift, private fixtures may be unavailable, and historical
logs do not establish the remaining phase costs. Resolve these in T001; preserve
explicitly unverified cases instead of changing acceptance to match available data.
