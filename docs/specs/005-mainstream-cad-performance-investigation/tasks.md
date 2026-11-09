---
title: Mainstream CAD performance investigation tasks
doc_type: spec
artifact_type: tasks
status: draft
owner: local-developer
last_reviewed: 2026-11-09
---

# Tasks

## Context And Execution

Read canonical-context.md, requirements.md, design.md, traceability.md,
verification.md and the decision ledger before execution. T001 is complete; T002 is next. Authored planning and fixture preparation
are not measured performance evidence. Use guarded task-state updates and
normally one task in progress. Stable IDs are identities; line numbers are not.
User scope is investigation of six mainstream areas plus B001 and B011.

Execute in the declared linear order. A blocking discovery is recorded on the
current task with the smallest corrective slice; do not jump ahead or hide
partial completion. Promote scope/task changes coherently before continuing.
No optimization, shipped preference/default change, public API or kernel/solver
architecture rewrite is authorized by a profiling task.

## Task Dependency Graph

```text
T001 -> T002 -> T003 -> T004 -> T005 -> T006 -> T007 -> T008 -> T009 -> T010 -> T011 -> T012
```

## Delivery Slices

- [x] T001 Freeze representative workloads, semantic oracles and the collection plan.
  - Depends on: none
  - Requirements: Requirement 1 (all acceptance criteria)
  - Properties: CP-001, CP-002, CP-003, CP-004, CP-005
  - Design: design.md#workloads-and-provenance
  - Files: `workload-manifest.json`, `measurement-plan.json`, `open-decisions.md`,
    `fixtures/`, `t001-fixture-preparation.json`, `t001-results.md`, `t001-validation.json`
  - Acceptance: All six workload families, ordinary/larger controls and eight stream recipes are mapped; exact source/build/runtime scope, reset/oracles, fixed scheduled-attempt counts/order, minimum valid coverage and inconclusive/stop rules (no replacement/optional stopping), cache/host-load/exclusion policy, time/disk budgets and private/public boundaries are recorded. Resolve D001 before T002; no sample is accepted under an unfrozen plan. Any recovery study has a separate prospective freeze/schedule and never replaces or pools original attempts.
  - Validation: verification.md#investigation-gates and the frozen stream/oracle contract; archive raw evidence and exact identities.
  - Evidence mode: artifact
  - Evidence: t001-results.md; workload-manifest.json; measurement-plan.json; t001-fixture-preparation.json; t001-validation.json; fixtures/canonical/. All three completion outcomes verified: 17 public FCStd plus 2 STEP canonical files (1,026,459 bytes) are preserved with exact hashes, public metadata and reproducible semantic recipes; all 17 reopened and both STEP files read with preparation checks. Eight streams map to 60 focused cells and 390 fixed scheduled rows (65 warmups, 325 future timed attempts), with independent operation/reset/oracle contracts, counterbalanced QR and round-robin order, privacy/preview/rank/host/resource gates. D001 resolved after three independent CAD, numerical/performance/QA and operations/process reviews. Final direct checks verify canonical and cross-artifact hashes, 12 prior authoring authority blobs, source preservation, schedules, limits and links; SLM lint zero diagnostics and T002 context zero gaps. No Release build, study row or performance result is qualified; D002 remains open for T002.
  - Owner: local-developer

  Completion outcomes (all required; all verified for T001):

  | Outcome | Required evidence | Status |
  | --- | --- | --- |
  | Fixture registry | All six families, rights/provenance/hash/counts and owned originals/copies | verified; t001-validation.json |
  | Operation/oracle registry | Exact edit class/reset/drag branch and independent semantic outcomes per stream | verified; t001-validation.json |
  | Collection/resource freeze | Sample/cache/host-load/exclusion/timeout/process/disk contract and D001 resolution | verified; t001-validation.json |

  Record each outcome separately; keep the task partial/in progress until all
  outcomes pass. Do not begin the next parent task from one successful outcome.

  - Status: Complete prospective workload, semantic oracle and collection/resource freeze. Next T002 qualification; v1 inputs remain immutable and no study sample is eligible yet.
- [ ] T002 Qualify the shared runner, stage attribution and runtime identities.
  - Depends on: T001
  - Requirements: Requirement 1 (all acceptance criteria)
  - Properties: CP-001, CP-002, CP-003, CP-004, CP-005
  - Design: design.md#collection-and-attribution
  - Files: `qualification.md`, `qualification.json`, `study-freeze.json`, `open-decisions.md`
  - Acceptance: Source-confirm interfaces/endpoints and existing QR controls; verify actual binaries/dependencies/loaded modules and owned preferences/session. Smoke-test reset, completion, correctness, timeout and preservation paths; quantify tracing overhead, freeze resulting harness/probe hashes and resolve D002. Read frozen workload/measurement JSON without overwriting v1; bind exact hashes in study-freeze.json. No production tracing/default change.
  - Validation: verification.md#investigation-gates and the frozen stream/oracle contract; archive raw evidence and exact identities.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer

  Completion outcomes (all required; currently pending):

  | Outcome | Required evidence | Status |
  | --- | --- | --- |
  | Runtime qualification | Source/probe/binary/dependency/module/settings identity and owned session | pending |
  | Runner qualification | Reset/correct completion/oracle/timeout/cleanup smoke checks | pending |
  | Attribution qualification | Actual endpoints/QR interfaces, overlap/overhead checks, final harness hashes and D002 resolution | pending |

  Record each outcome separately; keep the task partial/in progress until all
  outcomes pass. Do not begin the next parent task from one successful outcome.

- [ ] T003 Profile parametric edit-to-result recompute.
  - Depends on: T002
  - Requirements: Requirement 1 (all acceptance criteria), Requirement 2 (all acceptance criteria)
  - Properties: CP-001, CP-002, CP-004, CP-005
  - Design: design.md#stream-contracts
  - Files: `recompute-results.md`, `recompute-results.json`
  - Acceptance: R01 covers W01/W02 early edits with ordinary/larger controls, App/visible endpoints, dependency and object-execution counts, stage attribution and model/reference/undo oracles. Retain raw timings and issue an evidenced finding/limitation with pursue/defer/reject rationale.
  - Validation: verification.md#investigation-gates and the frozen stream/oracle contract; archive raw evidence and exact identities.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer

- [ ] T004 Profile ordinary and complex sketch interactions.
  - Depends on: T003
  - Requirements: Requirement 1 (all acceptance criteria), Requirement 3 (all acceptance criteria)
  - Properties: CP-001, CP-002, CP-004, CP-005
  - Design: design.md#stream-contracts
  - Files: `sketch-results.md`, `sketch-results.json`
  - Acceptance: S01 measures actual dimension/drag/constraint mutations and reset precursors on W05/W06, distinguishes forced recompute from edits, attributes solver/shape/downstream work and validates DoF/diagnostics/geometry/dependent/undo results. Identify exactly which cells support the QR comparison.
  - Validation: verification.md#investigation-gates and the frozen stream/oracle contract; archive raw evidence and exact identities.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer

- [ ] T005 Compare existing QR methods under B001.
  - Depends on: T004
  - Requirements: Requirement 1 (all acceptance criteria), Requirement 3 (all acceptance criteria), Requirement 4 (all acceptance criteria)
  - Properties: CP-001, CP-002, CP-004, CP-005, CP-003
  - Design: design.md#qr-comparison
  - Files: `qr-results.md`, `qr-results.json`, `../../backlog/sketcher-performance.md`
  - Acceptance: Q01 compares existing dense/sparse methods using a fixed counterbalanced/randomized paired schedule, identical precursor/reset and fresh owned processes (or T002-proven equivalent per-instance selection/reset), with per-run selection checks and baseline/peak/post memory plus rank-sensitive synthetic controls. Both ranks, ordered diagnostics, DoF, geometry, dependent/reference/undo and lifetime/exception behavior match or the configuration is explicitly rejected. Restore owned settings; B001 gets an evidence-backed disposition, with defaults/thresholds/tolerances unchanged.
  - Validation: verification.md#investigation-gates and the frozen stream/oracle contract; archive raw evidence and exact identities.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer

- [ ] T006 Profile patterns, booleans, fillets and chamfers.
  - Depends on: T005
  - Requirements: Requirement 1 (all acceptance criteria), Requirement 5 (all acceptance criteria)
  - Properties: CP-001, CP-002, CP-004, CP-005
  - Design: design.md#stream-contracts
  - Files: `feature-results.md`, `feature-results.json`
  - Acceptance: F01 covers all four feature categories on valid ordinary/larger W01/W02 variants, preview separately from accepted operation, kernel/setup/validation/reference/display spans and geometry/topology/reference/undo oracles. No geometry simplification or tolerance change is implemented.
  - Validation: verification.md#investigation-gates and the frozen stream/oracle contract; archive raw evidence and exact identities.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer

- [ ] T007 Profile assembly movement, joint solving and part updates.
  - Depends on: T006
  - Requirements: Requirement 1 (all acceptance criteria), Requirement 6 (all acceptance criteria)
  - Properties: CP-001, CP-002, CP-004, CP-005
  - Design: design.md#stream-contracts
  - Files: `assembly-results.md`, `assembly-results.json`
  - Acceptance: A01 records actual assembly implementation, small/larger linked/repeated fixtures, component move and referenced-part update, stage/execution counts and correct visible result. Placements, link targets, dependent result and restore/undo checks qualify findings; D002 must source-confirm joint status/residuals/DoF metrics or approve independent constraint/placement checks with missing internals explicitly labeled.
  - Validation: verification.md#investigation-gates and the frozen stream/oracle contract; archive raw evidence and exact identities.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer

- [ ] T008 Profile viewport interaction, tessellation and picking.
  - Depends on: T007
  - Requirements: Requirement 1 (all acceptance criteria), Requirement 7 (all acceptance criteria)
  - Properties: CP-001, CP-002, CP-004, CP-005
  - Design: design.md#stream-contracts
  - Files: `viewport-results.md`, `viewport-results.json`
  - Acceptance: V01 separates first preparation from steady rotation/zoom and preselection/pick on detailed native/imported solids; freezes input timestamps/order, pacing/backpressure, frame/event admission/drop/coalescing and a source-confirmed presentation witness or honest narrower endpoint; records actual driver/viewport/quality and limits, and validates model/camera/subelement/highlight identity without lowering display quality.
  - Validation: verification.md#investigation-gates and the frozen stream/oracle contract; archive raw evidence and exact identities.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer

- [ ] T009 Profile large-model tree operations under B011.
  - Depends on: T008
  - Requirements: Requirement 1 (all acceptance criteria), Requirement 8 (all acceptance criteria)
  - Properties: CP-001, CP-002, CP-004, CP-005
  - Design: design.md#tree-investigation
  - Files: `tree-results.md`, `tree-results.json`, `../../backlog/spec003-follow-ups.md`
  - Acceptance: T01 uses real many-object/component and small controls for population/expand/status/selection/delete-undo. Separate tree work from model/render work; validate selection/focus/maps/hierarchy/observers, receiver lifetime and queued/restore/import/failure completion. Disposition B011 without implementing bulk-delete bypass or raw-reverting safety repairs.
  - Validation: verification.md#investigation-gates and the frozen stream/oracle contract; archive raw evidence and exact identities.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer

- [ ] T010 Profile non-image native open/save and separate STEP import.
  - Depends on: T009
  - Requirements: Requirement 1 (all acceptance criteria), Requirement 9 (all acceptance criteria)
  - Properties: CP-001, CP-002, CP-004, CP-005
  - Design: design.md#stream-contracts
  - Files: `persistence-results.md`, `persistence-results.json`
  - Acceptance: I01 covers feature-rich and linked-component FCStd open/save with no image workload, and labels STEP import separately. Record storage/cache/counts and identifiable I/O/object/link/geometry/recompute/GUI stages; validate saved/reopened geometry/references/placements/joints/errors on owned copies.
  - Validation: verification.md#investigation-gates and the frozen stream/oracle contract; archive raw evidence and exact identities.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer

- [ ] T011 Rank cross-stream opportunities and scope the next changes.
  - Depends on: T010
  - Requirements: Requirement 1 (all acceptance criteria), Requirement 2 (all acceptance criteria), Requirement 3 (all acceptance criteria), Requirement 4 (all acceptance criteria), Requirement 5 (all acceptance criteria), Requirement 6 (all acceptance criteria), Requirement 7 (all acceptance criteria), Requirement 8 (all acceptance criteria), Requirement 9 (all acceptance criteria), Requirement 10 (all acceptance criteria)
  - Properties: CP-001, CP-002, CP-003, CP-004, CP-005
  - Design: design.md#ranking-and-delivery
  - Files: `opportunities.md`, `opportunities.json`, `../../backlog/README.md`
  - Acceptance: All eight streams have explicit evidence/limitation and pursue/defer/reject decisions. Rank absolute user benefit, frequency assumptions, uncertainty, memory/concurrency, correctness/maintenance and implementation/validation cost using all eleven policy fields; deduplicate inclusive/shared costs. Each selected implementation has one owned scoped destination; no production algorithm/default change is made here.
  - Validation: verification.md#investigation-gates and the frozen stream/oracle contract; archive raw evidence and exact identities.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer

- [ ] T012 Review findings, promote durable guidance and close the investigation.
  - Depends on: T011
  - Requirements: Requirement 10 (all acceptance criteria)
  - Properties: CP-001, CP-002, CP-003, CP-004, CP-005
  - Design: design.md#ranking-and-delivery
  - Files: `verification.md`, `../../reference/mainstream-cad-performance.md`, `../../runbooks/mainstream-cad-performance-investigation.md`, `../../history/spec-closure-log.md`, `../../history/spec-archive-index.md`
  - Acceptance: Independent CAD/FreeCAD lifecycle, numerical/performance, QA and operations/documentation experts inspect actual results. Resolve blocking findings, all requirement/property/stream coverage and B001/B011 ownership; promote accepted guidance, record actual commit/archive metadata and retain immutable evidence. Separate investigation completion from any future optimization delivery.
  - Validation: verification.md#investigation-gates and the frozen stream/oracle contract; archive raw evidence and exact identities.
  - Evidence mode: artifact
  - Evidence: Pending.
  - Owner: local-developer

## Planned Evidence Boundary

Named reports/manifests above are future task outputs, not files claimed to exist.
Harness/probe code paths are finalized by T001/T002 and explicitly added to task
ownership before edits. Source probes, if needed, stay opt-in and separately
qualified; normal repository build/test and line-ending rules apply.

B001/B011 retain their backlog IDs and original correctness/exit contracts.
T005/T009 own their execution evidence; T011/T012 reconcile final backlog status.
B009/B010 and other Sketcher proposals are outside this investigation.

## Related Artifacts

- [Requirements](requirements.md)
- [Design](design.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Decisions](open-decisions.md)
