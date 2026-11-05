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
coverage records completed T001 through T007 and durable promotion/review. Timing acceptance uses the user-approved indicative
standard; original strict-gate outcomes remain historical evidence.

## Task To Context Matrix

| Task ID | Requirements | Acceptance Criteria | Design Sections | Verification | Durable Targets | Open Decisions |
| --- | --- | --- | --- | --- | --- | --- |
| T001 | Requirement 1, Requirement 2, Requirement 6 | AC-001, AC-002, AC-005 | D-001, D-003 | baseline-report.md; workloads.json; verification.md | docs/reference/sketcher-performance-diagnostics.md, docs/runbooks/sketcher-performance-investigation.md | Resolved baseline; historical symmetry owned by T006 |
| T002 | Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5 | AC-001, AC-002, AC-003, AC-004, AC-006 | D-001, D-002; C-001 through C-006 | timing-invalidation-contract.md; verification.md; native diagnosis probes | docs/reference/sketcher-performance-diagnostics.md | Design resolved; implementation proof T003/T004 |
| T003 | Requirement 3, Requirement 4, Requirement 5 | AC-003, AC-004, AC-006 | D-002; C-003 through C-006 | phase-tracing-report.md; native SketcherTrace tests | docs/reference/sketcher-performance-diagnostics.md | Implemented; indicative overhead review complete T005 |
| T004 | Requirement 1, Requirement 2 | AC-001, AC-002 | D-001; C-001/C-002 | diagnosis-deferral-report.md; native/Python/model comparisons | docs/reference/sketcher-performance-diagnostics.md | Implemented and tested; bounded GUI verified in T005 |
| T005 | Requirement 2, Requirement 6 | AC-002, AC-005, AC-006 | D-003 | performance-validation-report.md; performance-results.json; comparison-protocol.md | docs/runbooks/sketcher-performance-investigation.md | Complete under user-approved indicative AC-005/AC-006; original strict outcomes retained |
| T006 | Requirement 7 | AC-007 | D-004 | remaining-cost-decisions.md; remaining-cost-attribution.json; docs/backlog/README.md | docs/backlog/README.md, docs/backlog/sketcher-performance.md | Complete; QR/shape investigations proposed, other work bounded/deferred |
| T007 | Requirement 1, Requirement 2, Requirement 3, Requirement 4, Requirement 5, Requirement 6, Requirement 7 | AC-001, AC-002, AC-003, AC-004, AC-005, AC-006, AC-007 | D-004 | verification.md quality gates | docs/reference/sketcher-performance-diagnostics.md, docs/runbooks/sketcher-performance-investigation.md | Retained history; no integration merge or public submission |

## Requirement To Delivery Matrix

| Requirement | Acceptance Criteria | Design Sections | Tasks | Coverage State | Residual Destination |
| --- | --- | --- | --- | --- | --- |
| Requirement 1 | AC-001 | D-001 | T001, T002, T004 | covered | Native and independent frozen-model counts in diagnosis-deferral-report.md |
| Requirement 2 | AC-002 | D-001, D-003 | T001, T002, T004, T005 | covered | Native/Python/model and bounded Linux GUI evidence in diagnosis-deferral-report.md and performance-validation-report.md |
| Requirement 3 | AC-003 | D-002 | T002, T003 | covered | Native/process evidence in phase-tracing-report.md; promotion T007 |
| Requirement 4 | AC-003 | D-002 | T002, T003 | covered | Known counters, dense/sparse/block/redundancy tests; promotion T007 |
| Requirement 5 | AC-004, AC-006 | D-002 | T002, T003, T005 | covered | Disabled behaviour verified; indicative overhead review satisfies revised AC-006 with uncertainty disclosed |
| Requirement 6 | AC-005, AC-006 | D-003 | T001, T005 | covered | Comparable repeated evidence satisfies revised indicative criteria; strict historical outcomes unchanged |
| Requirement 7 | AC-007 | D-004 | T006, T007 | covered | T006 measured attribution and ranked owned backlog; final promotion T007 |

## Correctness Property Coverage

| Property | Requirements | Tasks | Verification |
| --- | --- | --- | --- |
| CP-001 | Requirement 1, Requirement 2 | T001, T002, T004, T005 | Index transitions, diagnostic IDs/touched/observer contract, non-recompute paths, native geometry and external links |
| CP-002 | Requirement 3, Requirement 4, Requirement 5 | T002, T003, T005 | Reconstructable QR spans, structural counters, disabled records and timing-getter compatibility |
| CP-003 | Requirement 6, Requirement 7 | T001, T005, T006 | Executed/reset manifests, alternating pairs/MAD/intervals, inconclusive disposition and follow-up decision |

## Durable Targets

Current diagnostics reference: `docs/reference/sketcher-performance-diagnostics.md`.
Current investigation runbook: `docs/runbooks/sketcher-performance-investigation.md`.
Code/tests remain primary; these promoted documents describe current branch behaviour.

## Open Decision Impact

T001 resolved source freshness, frozen private fixture identity and exact test
targets. The changed staged file is excluded; unverified historical symmetry
belongs to T006. Baseline A/A noise is recorded for investigation in T005. T002
resolved trace control/schema, worker lifetimes, counter stages and the scoped
deferral/notification contract in C-001 through C-006. T003/T004 own proof of those
contracts in implementation. T006 resolves deeper optimization choices after measurements.

## Design To Implementation Matrix

| Design Section | Requirements | Tasks | Interfaces Or Files | Verification | Coverage State | Residual Destination |
| --- | --- | --- | --- | --- | --- | --- |
| D-001 | Requirement 1, Requirement 2 | T001, T002, T004 | SketchObject.cpp, SketchObjectConstraints.cpp, SketchObject.h | AC-001, AC-002 | covered | Native/Python/model and bounded Linux GUI verified T004/T005 |
| D-002 | Requirement 3, Requirement 4, Requirement 5 | T002, T003 | SketcherTrace.h/.cpp, SketchObject entry scopes, Sketch.cpp, planegcs/GCS.cpp | AC-003, AC-004, AC-006 | covered | Tracing/indicative overhead verified; promoted reference and runbook |
| D-003 | Requirement 2, Requirement 6 | T001, T005 | Native fixtures and private benchmark receipts | AC-002, AC-005, AC-006 | covered | Correctness and indicative timing evidence complete under timing-evidence-decision.md |
| D-004 | Requirement 7 | T006, T007 | remaining-cost-decisions.md; docs/backlog/README.md | AC-007 | covered | T006 selection complete; promotion/review/closure T007 |

## Authority Review

The optional canonical-context advisory was reviewed during authoring. Current
code, root AGENTS.md and repository templates remain their respective authorities;
the research log is explicitly historical and promoted reference/runbook describe implemented behaviour.
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
T002 through T005 are complete after correctness/GUI verification and indicative
timing review. Earlier strict-gate uncertainty remains recorded, with no promise
of regression-free behaviour or guaranteed overhead bounds. T006 is complete.

## T002 Evidence Status

[timing-invalidation-contract.md](timing-invalidation-contract.md) records the
selected design and requirements-linked validation plan. Six native probes
establish observer ordering/touched-state/diagnostic identities and independent
index-flag repair; the isolated Release suite passes 115 tests. No production
tracing or setup deferral is implemented, and no performance gate passes here.

## T003 Evidence Status

[phase-tracing-report.md](phase-tracing-report.md) maps the private helper,
SketchObject/Sketch/planegcs boundaries, schema and test evidence to C-003 through
C-006. Native suites pass 136 tests in Release (off/on) and Debug. Isolated-process
control/JSON checks and 21 native tracing tests establish AC-003/AC-004; overhead
and comparative budgets remain T005. Durable promotion remains T007.

## T004 Evidence Status

[diagnosis-deferral-report.md](diagnosis-deferral-report.md) establishes AC-001's
one-setup invariant on ordinary synthetic/master/profile recompute and AC-002's
core regression matrix. Correctness-required observer-input refresh and legacy
fallbacks retain extra setups. Native/Python/model evidence does not pass T005's
GUI and matched latency/overhead gates. T004 implementation is complete; durable
reference/runbook promotion remains T007.

## Original T005 Strict-Gate Evidence Status (Superseded)

[performance-validation-report.md](performance-validation-report.md) and
[performance-results.json](performance-results.json) provide correctness, all
batch/pooled statistics, execution/reset counts and budget dispositions.
AC-002's bounded Linux GUI gap is closed. The master/profile speedup requirement
passes, but overall AC-005 and AC-006 remain inconclusive after the full sampling
bound. T005 stays partial; T006's dependency and T007 closure are not satisfied.
The current data must be retained when defining a separate controlled-host cohort;
do not pool different machines or relabel this cohort as accepted.

## Current T005 Evidence Status

The user explicitly revised timing acceptance to indication of improvement on
2026-11-05. [timing-evidence-decision.md](timing-evidence-decision.md) links the
revised AC-005/AC-006 to the unchanged samples, two-to-one setup counts, approximately
47%/57% lower representative medians and passing correctness checks. Variability
and the original inconclusive strict-gate results remain disclosed. T005 is
complete; T006 is complete. T007 promotion and review are complete.

## T006 Evidence Status

[remaining-cost-decisions.md](remaining-cost-decisions.md) and
[remaining-cost-attribution.json](remaining-cost-attribution.json) establish AC-007:
ranked QR, shape, copy, metadata, reuse, sparse-assembly and historical-replay work
has measured opportunity, uncertainty/risk, local owner and actual
[backlog destinations](../../backlog/README.md). New snapshot qualification covers
all 13 sketches without changing the input; old symmetry starting authority is
partly recovered and the remaining precursor gap is explicitly deferred to B006.
No new solver rewrite is started. T006 and T007 are complete.


## T007 Promotion Matrix

| Knowledge | Durable destination | Review / limits |
| --- | --- | --- |
| One-setup validation/notification boundary | [Reference](../../reference/sketcher-performance-diagnostics.md) | Source/test review; observer external edit follow-up B008 |
| Trace controls/schema/counters/lifetime | [Reference](../../reference/sketcher-performance-diagnostics.md) | Actual source/schema; worker and overlap semantics retained |
| Safe capture/build/timing/recovery workflow | [Runbook](../../runbooks/sketcher-performance-investigation.md) | Operations review; unique profiles; copied inputs only |
| Ranked further work | [Backlog](../../backlog/README.md) | Owned B001–B008 proposals/deferred investigations |
| Final review / integration disposition | [Delivery review](../../reviews/sketcher-diagnosis-delivery-review.md) | Local acceptance; upstream preparation remains explicit |
| Immutable delivery provenance | [Closure log](../../history/spec-closure-log.md) | Final/cleanup commits resolved after commit; retained history |

T007 is complete. Historical per-task pending statements above describe their
checkpoint only. The current acceptance authority is the reconciled requirements,
user-approved indicative timing decision and final verification summary.
