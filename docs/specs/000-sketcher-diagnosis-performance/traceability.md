---
title: Sketcher diagnosis performance traceability
doc_type: spec
artifact_type: traceability
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# Traceability Matrix

## Purpose

Map each task to the full requirements/design/evidence context. All delivery
coverage is currently not-covered because production optimization and tracing
are not implemented. T001 baseline establishment is complete.

## Task To Context Matrix

| Task ID | Requirements | Acceptance Criteria | Design Sections | Verification | Durable Targets | Open Decisions |
| --- | --- | --- | --- | --- | --- | --- |
| T001 | Requirement 1, Requirement 2, Requirement 6 | AC-001, AC-002, AC-005 | D-001, D-003 | baseline-report.md; workloads.json; verification.md | Diagnostics reference, investigation runbook | Resolved baseline; historical symmetry owned by T006 |
| T002 | Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5 | AC-001, AC-002, AC-003, AC-004, AC-006 | D-001, D-002 | verification.md correctness matrix | Diagnostics reference | Trace control/schema and local guard |
| T003 | Requirement 3, Requirement 4, Requirement 5 | AC-003, AC-004, AC-006 | D-002 | verification.md timing cases | Diagnostics reference | Resolved by T002 |
| T004 | Requirement 1, Requirement 2 | AC-001, AC-002 | D-001 | verification.md correctness matrix | Diagnostics reference | Resolved by T002 |
| T005 | Requirement 2, Requirement 6 | AC-002, AC-005, AC-006 | D-003 | verification.md measurement matrix | Investigation runbook | Budget dispositions |
| T006 | Requirement 7 | AC-007 | D-004 | verification.md review/closure | Follow-up packages or backlog | Evidence-led selection |
| T007 | Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7 | AC-001, AC-002, AC-003, AC-004, AC-005, AC-006, AC-007 | D-004 | verification.md quality gates | Both proposed documents | Closure disposition |

## Requirement To Delivery Matrix

| Requirement | Acceptance Criteria | Design Sections | Tasks | Coverage State | Residual Destination |
| --- | --- | --- | --- | --- | --- |
| Requirement 1 | AC-001 | D-001 | T001, T002, T004 | not-covered | Planned work in this package |
| Requirement 2 | AC-002 | D-001, D-003 | T001, T002, T004, T005 | not-covered | Planned work in this package |
| Requirement 3 | AC-003 | D-002 | T002, T003 | not-covered | Planned work in this package |
| Requirement 4 | AC-003 | D-002 | T002, T003 | not-covered | Planned work in this package |
| Requirement 5 | AC-004, AC-006 | D-002 | T002, T003, T005 | not-covered | Planned work in this package |
| Requirement 6 | AC-005, AC-006 | D-003 | T001, T005 | not-covered | Planned work in this package |
| Requirement 7 | AC-007 | D-004 | T006, T007 | not-covered | Planned work in this package |

## Correctness Property Coverage

| Property | Requirements | Tasks | Verification |
| --- | --- | --- | --- |
| CP-001 | Requirement 1, Requirement 2 | T001, T002, T004, T005 | Index transitions, diagnostic IDs/touched/observer contract, non-recompute paths, native geometry and external links |
| CP-002 | Requirement 3, Requirement 4, Requirement 5 | T002, T003, T005 | Reconstructable QR spans, structural counters, disabled records and timing-getter compatibility |
| CP-003 | Requirement 6, Requirement 7 | T001, T005, T006 | Executed/reset manifests, alternating pairs/MAD/intervals, inconclusive disposition and follow-up decision |

## Durable Targets

Proposed diagnostics reference: `docs/reference/sketcher-performance-diagnostics.md`.
Proposed investigation runbook: `docs/runbooks/sketcher-performance-investigation.md`.
They are promotion targets, not current implemented authority.

## Open Decision Impact

T001 resolved source freshness, frozen private fixture identity and exact test
targets. The changed staged file is excluded; unverified historical symmetry
belongs to T006. Baseline A/A noise is recorded for investigation in T005. T002
resolves optional trace control/schema, worker-output safety and the scoped guard
before T003/T004. T006 resolves the deeper optimization choice after measurements.

## Design To Implementation Matrix

| Design Section | Requirements | Tasks | Interfaces Or Files | Verification | Coverage State | Residual Destination |
| --- | --- | --- | --- | --- | --- | --- |
| D-001 | Requirement 1, Requirement 2 | T001, T002, T004 | SketchObject.cpp, SketchObjectConstraints.cpp | AC-001, AC-002 | not-covered | Planned work in this package |
| D-002 | Requirement 3, Requirement 4, Requirement 5 | T002, T003 | Sketch.cpp, Sketch.h, planegcs/GCS.cpp, GCS.h | AC-003, AC-004, AC-006 | not-covered | Planned work in this package |
| D-003 | Requirement 2, Requirement 6 | T001, T005 | Native fixtures and private benchmark receipts | AC-002, AC-005, AC-006 | not-covered | Planned work in this package |
| D-004 | Requirement 7 | T006, T007 | Proposed diagnostics reference and runbook | AC-007 | not-covered | Planned work in this package |

## Authority Review

The optional canonical-context advisory was reviewed during authoring. Current
code, root AGENTS.md and repository templates remain their respective authorities;
the research log is explicitly historical and the new behaviour explicitly proposed.
No conflicting imported authority requires a separate canonical-context artifact.

## Review Finding Coverage

R-001/R-002 map to Requirement 1/Requirement 2, D-001 and T001/T002/T004/T005.
R-003/R-005 map to Requirement 6, D-003 and T001/T005. R-004/R-006 map to
Requirement 3/Requirement 4/Requirement 5, D-002 and T002/T003/T005.
The finding-by-finding disposition is in
[verification.md](verification.md#moe-review-disposition---2026-11-05).

## T001 Evidence Status

[baseline-report.md](baseline-report.md) records Release 109-test validation,
two external-reference tests, native duplicate-setup reproduction, nine optimized
baseline batches and private build/raw/analysis receipts. [workloads.json](workloads.json)
freezes each runnable execution vector and reset/cache protocol. T001 is complete;
T002 is the next pending task. T005 still owns matched candidate acceptance and
noise investigation; no delivery coverage or performance acceptance row is complete.
