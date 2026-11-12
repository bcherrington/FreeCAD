---
title: Mainstream CAD performance investigation requirements
doc_type: spec
artifact_type: requirements
status: draft
owner: local-developer
last_reviewed: 2026-11-09
---

# Requirements

## Introduction

Investigate performance in mainstream parametric CAD: everyday part editing,
complex sketches, common features, assemblies, viewport interaction and normal
model persistence. The user explicitly selected all six areas plus B011 tree
profiling and B001 QR comparison. This is an investigation package, not a promise
of speedups or authorization to change production algorithms/defaults.

## Goals

- Measure complete user operations before optimizing individual stages.
- Cover ordinary and larger representative models, reusing fixtures across streams.
- Find high-value implementation candidates with strict correctness and explicit cost-benefit decisions.
- Complete investigation, ranking and durable routing without reopening archived Spec000/003/004.

## Non-Goals

- Image decoding, lazy textures or reduced-resolution previews; B009 remains separate.
- A new solver, global parallel recompute, caching framework, geometry-kernel replacement or blanket event suppression.
- Automatic changes to solver defaults, tolerances, tessellation quality, archive formats, public APIs or shipped behavior.
- A universal latency SLA, whole-project acceleration claim, upstream delivery or cross-platform certification.
- Executing separate implementation specs, unrelated startup investigation B010, or deleting shared build outputs.

## Glossary

| Term | Definition |
| --- | --- |
| End-to-end latency | The frozen user action through the correct completed/visible result, including necessary dependent work |
| Attribution | Descriptive stage cost in a separate traced run, not an automatically removable saving |
| Screening | Small bounded samples used to identify candidates, without tail guarantees |
| QR comparison | Controlled selection among existing implementations; no default or numerical-policy change |
| Correctness oracle | Independent expected geometry, solver, reference and UI outcome for a workload |

## Durable Source Baseline

| Source | Current behavior relied on | Confidence | Notes |
| --- | --- | --- | --- |
| `docs/reference/document-open-performance.md` | Mandatory compatibility/lifetime/provenance and cost-benefit contract | promoted local contract | Spec003 closed; numeric P01/P07 observations are historical |
| `docs/runbooks/freecad-performance-optimization.md`; `docs/runbooks/freecad-benchmark-storage.md` | Affected qualification, isolation, bounded collection and eligibility-based storage reuse | promoted procedure | No automatic reuse of changed binaries |
| `docs/reference/sketcher-performance-diagnostics.md`; `docs/runbooks/sketcher-performance-investigation.md` | Sketcher diagnosis contracts and bounded profiling | promoted local contract | Current source interfaces must be reconfirmed |
| `docs/backlog/sketcher-performance.md#b001-qr-method-comparison` | Existing QR comparison scope and rank sensitivity | owned proposal | Earlier QR cost shares are motivation, not new results |
| `docs/backlog/spec003-follow-ups.md#b011-further-tree-batching-or-bulk-delete-profiling` | Larger-model tree profiling and invariant boundary | owned proposal | No bulk deletion implementation approved |
| `src/App`, `src/Mod/Sketcher`, `src/Mod/PartDesign`, `src/Mod/Part`, `src/Mod/Assembly`, `src/Gui` | Candidate subsystem boundaries | navigation evidence | Actual stages, symbols, build and runtimes are T001/T002 work |

## Durable Impact

| Durable area | Action | Target | Notes |
| --- | --- | --- | --- |
| reference | add | `docs/reference/mainstream-cad-performance.md` | Accepted workload/stage findings, limits and ranked decisions |
| operations | add | `docs/runbooks/mainstream-cad-performance-investigation.md` | Repeatable safe profiling and correctness procedures |
| solver evidence | clarify | `docs/reference/sketcher-performance-diagnostics.md` | Only accepted QR findings, no silent default change |
| follow-up | modify | `docs/backlog/README.md`; `docs/backlog/sketcher-performance.md`; `docs/backlog/spec003-follow-ups.md` | B001/B011 ownership plus separately scoped implementation candidates |
| history | modify at closure | `docs/history/spec-closure-log.md`; `docs/history/spec-archive-index.md` | Actual commit identities and retained evidence |

## Staged Readiness

- **Current stage:** T001 fixture/oracle/collection freeze complete; no timed study started.
- **Next stage:** T002 qualifies the shared runner and actual Release/runtime identity, resolving D002.
- **Ready to design when:** selected eight streams and correctness/provenance boundaries are mapped; initial design below is subject to expert authoring review.
- **Design-first exception:** no.
- **Optional artifacts recommended:** canonical-context.md and open-decisions.md for ownership and execution gates.
- **Downstream review needed:** CAD/geometry, FreeCAD lifecycle, numerical/performance, QA and operations/documentation.

## Requirements

### Requirement 1: Representative workloads and trustworthy evidence

**User Story:** As a CAD user, I want representative operations measured on identified models and binaries, so that priorities reflect everyday modelling.

**Priority:** must-have

#### Acceptance Criteria

1. GIVEN the six proposed workload families, WHEN T001 freezes the fixture registry, THEN each SHALL have a repeatable operation/reset recipe, geometry/object/constraint scale, provenance, content hash and public-or-private disposition; image planes SHALL NOT be a required workload.
2. BEFORE timed collection, THE investigation SHALL freeze source/dependency/build/runtime identities, settings, stage endpoints, fixed scheduled-attempt count/order, minimum valid coverage and inconclusive/stop rules, warmups, cache policy, time/resource limits, host-load collection and correctness oracles, and SHALL qualify the harness against those identities.
3. WHEN observations are collected, THE evidence SHALL preserve raw untraced timings separately from traced attribution, include wall/CPU, baseline/peak/post-operation memory or explicit missing observations, and host contention, account for instrumentation overhead, and retain failures/timeouts without replacement or pooling; failed coverage SHALL yield an inconclusive/stop result, and any recovery study SHALL use a separate prospective freeze without replacing or pooling original attempts.
4. WHEN source, binary, dependency, fixture or relevant environment changes, THE investigation SHALL requalify affected evidence or record independently reviewed equivalence; installed package metadata alone SHALL NOT prove runtime closure.

### Requirement 2: Parametric edit-to-result recompute

**User Story:** As a part designer, I want to understand delays after changing an early dimension, so that feature-history responsiveness can be improved.

**Priority:** must-have

#### Acceptance Criteria

1. GIVEN the machined-part and long-history workloads, WHEN an early driving dimension changes, THEN measurements SHALL cover edit acceptance through completed dependent recompute and the first correct rendered result, with separate application-only and GUI endpoints.
2. WHEN recompute is profiled, THE report SHALL identify executed/touched objects, dependency propagation, sketch solve, downstream feature/kernel work, reference/topology processing, tessellation and redraw, distinguishing overlapping spans and legitimate repeated execution from proven redundant work.
3. BEFORE recompute evidence is accepted, THE edited and restored models SHALL match the expected edited geometry/topology, dependent references and feature error states for the frozen topology-preserving or topology-changing edit class and transaction/undo/redo behavior against the frozen oracle.

### Requirement 3: Ordinary and complex sketch interaction

**User Story:** As a sketch author, I want dimension changes, dragging and constraint edits investigated, so that frequent interactions drive priorities.

**Priority:** must-have

#### Acceptance Criteria

1. GIVEN an ordinary constrained sketch and a complex sketch, WHEN the frozen dimension-edit, drag and add/remove-constraint recipes run, THEN the report SHALL measure complete interaction latency and identify diagnosis/setup, numerical solve, geometry/shape generation and downstream update costs.
2. WHEN a forced recompute is used for attribution, THE report SHALL distinguish it from actual mutation/drag behavior and SHALL NOT extrapolate unchanged-model timing to interactive edits without evidence.
3. BEFORE accepting a sketch observation, THE oracle SHALL check DoF, constraint/diagnostic identity, geometry, dependent feature results and undo/redo/external-reference behavior; restore the intended starting state between runs.

### Requirement 4: B001 existing QR method comparison

**User Story:** As a solver investigator, I want a bounded comparison of existing dense and sparse QR choices, so that their cost and numerical compatibility can be assessed without changing defaults.

**Priority:** must-have

#### Acceptance Criteria

1. GIVEN current source-confirmed QR selection interfaces and frozen representative/rank-sensitive fixtures, WHEN the existing methods are compared, THEN the evidence SHALL record both J/JT ranks, ordered diagnostic identities, DoF, actual selected method and tolerance/settings, alongside paired full-operation time, QR attribution and baseline/peak/post-operation memory. Pair order SHALL be counterbalanced/randomized, precursors/reset identical, and selection verified per run; use fresh owned processes unless T002 proves equivalent per-instance selection and complete relevant state reset, including cache/allocator carryover.
2. WHEN a method changes rank/diagnostic/geometry/dependent-reference outcomes or fails safe lifetime/exception behavior, THE configuration SHALL be rejected for that scope regardless of speed; aligned-slot, symmetric and nearly rank-deficient synthetic cases SHALL be included.
3. AFTER comparison, THE investigation SHALL restore owned settings and issue a pursue/defer/reject decision tied to B001; shipped automatic thresholds, pivot tolerances, defaults and launch policy SHALL remain unchanged, with any implementation/default change requiring a separate approved scope.

### Requirement 5: Common feature and kernel operations

**User Story:** As a mechanical designer, I want patterns, booleans, fillets and chamfers attributed, so that expensive common features receive evidence-based attention.

**Priority:** must-have

#### Acceptance Criteria

1. GIVEN the part workloads, WHEN representative patterned holes/features, multi-body booleans, fillets and chamfers execute, THEN the report SHALL measure preview separately from accepted operation and separate kernel construction, repeated setup, validation, topology/reference work and display where identifiable.
2. WHEN feature complexity is varied, THE report SHALL include a small/ordinary control and a larger valid case with explicit instance/edge/face counts, and SHALL avoid summing inclusive nested spans or inferring a universal speedup from one pathological model.
3. BEFORE accepting measurements, THE feature oracle SHALL check shape validity, expected dimensions/topology/geometric invariants, persistent downstream references and error/undo outcomes; matching volume alone SHALL NOT prove identity.

### Requirement 6: Assembly movement and dependent updates

**User Story:** As an assembly user, I want component movement, joint solving and referenced-part updates investigated, so that multi-component modelling is represented.

**Priority:** must-have

#### Acceptance Criteria

1. GIVEN a valid jointed assembly with linked/repeated components, WHEN a component is moved and a referenced part is edited, THEN the report SHALL measure joint/placement solving, dependency propagation, recomputed objects, and correct visible completion separately.
2. WHEN repeated or apparently unnecessary assembly updates are found, THE evidence SHALL distinguish shared geometry, unique instances and external-link resolution, include a small control and representative larger assembly, and identify the actual assembly implementation/addons used.
3. BEFORE accepting assembly results, THE oracle SHALL check placements, link targets, dependent part results and restore/undo behavior, with joint status/residuals and remaining DoF where exposed; D002 SHALL qualify independent constraint/placement checks for unavailable internal metrics and label those limitations without changing joint solver policy.

### Requirement 7: Viewport responsiveness and selection

**User Story:** As a CAD user, I want rotation, zoom, preselection and face picking investigated, so that interaction with detailed solid models improves.

**Priority:** must-have

#### Acceptance Criteria

1. GIVEN detailed native and imported solids, WHEN deterministic camera and pick recipes execute, THEN the evidence SHALL distinguish first tessellation/scene preparation, steady camera interaction, preselection and accepted selection, with per-frame/pick observations and explicit viewport/driver/backend/settings. T001/T002 SHALL freeze event timestamps/order, input pacing/backpressure, admitted/presented/dropped/coalesced frame accounting and the source-confirmed correct-frame presentation witness.
2. WHEN application, CPU, GPU or presentation work cannot be separated, THE report SHALL name the narrower witnessed endpoint and its unavailable presentation/GPU/compositor evidence, and SHALL NOT equate update scheduling, render submission or an empty frame with a presented correct model.
3. BEFORE accepting viewport data, THE oracle SHALL verify visible model/camera identity, selected subelement identity, highlighting and selection/tree synchronization; display quality and tessellation tolerances SHALL remain frozen.

### Requirement 8: B011 large-model tree profiling

**User Story:** As a large-model user, I want the model tree investigated, so that further batching proposals preserve editing semantics.

**Priority:** must-have

#### Acceptance Criteria

1. GIVEN a representative many-object/component model and a smaller control, WHEN population, expand/collapse, selection synchronization, status changes and deletion/undo run, THEN evidence SHALL separate tree/event/observer work from model recompute and rendering and SHALL record object/row/update counts.
2. BEFORE a batching or bulk-deletion opportunity is accepted, THE oracle SHALL verify per-object selection, focus, hierarchy/maps and observer effects, receiver lifetime, queued callbacks and eventual restore/import/failure completion; no bulk-delete bypass SHALL be implemented by this investigation.
3. AFTER profiling, THE report SHALL disposition B011 as pursue/defer/reject with measured scope and owner; existing tree correctness repairs SHALL NOT be raw-reverted to manufacture a baseline.

### Requirement 9: Non-image document open and save

**User Story:** As a CAD user, I want feature-rich and component-rich models investigated, so that loading and persistence costs reflect ordinary CAD data.

**Priority:** must-have

#### Acceptance Criteria

1. GIVEN native feature-history and linked-component documents without image payloads, WHEN open and save run, THEN the report SHALL distinguish native FCStd persistence from STEP import, and separate archive I/O, object/property restoration, links/dependencies, geometry/reference processing, recompute and GUI population where observable.
2. WHEN load/save data is reported, THE evidence SHALL identify bytes/object/feature/component counts, cache state and storage context, distinguish console completion from first correct GUI result, and preserve original documents by writing owned copies.
3. BEFORE accepting persistence timing, THE oracle SHALL save/reopen and compare geometry/topology/references/placements/joints and transaction/error state as applicable; roundtrip format normalization SHALL be distinguished from deterministic byte identity.

### Requirement 10: Ranked decisions and durable delivery

**User Story:** As the development owner, I want a ranked cost-benefit report, so that the next implementation targets proven mainstream CAD costs.

**Priority:** must-have

#### Acceptance Criteria

1. AFTER the eight streams have bounded evidence, THE report SHALL give each a pursue/defer/reject disposition, owner, endpoint, absolute cost where validly measured (otherwise an explicit evidenced limitation or unavailability bound), frequency assumption, variability/uncertainty, memory, correctness/maintenance risk and one explicit implementation destination or rejection rationale.
2. WHEN opportunities share inclusive work or fixtures, THE ranking SHALL prevent double-counting, distinguish observed stage cost from attainable saving, and compare ordinary controls with larger models without inventing a universal response budget or numeric veto.
3. WHEN future implementation work is recommended, THE investigation SHALL carry the promoted cost-benefit policy forward, keep numerical/geometry/lifetime/error/provenance correctness mandatory and specify affected validation before any optimization or solver-default change.
4. BEFORE closure, THE accepted workload/measurement guidance and findings SHALL be promoted to durable reference/runbook/backlog destinations, B001/B011 SHALL retain truthful statuses/evidence, and independent CAD, numerical/performance, QA and documentation/operations reviews plus actual closure metadata SHALL be recorded.

## Correctness Properties

- **CP-001:** Fixture originals, source models and live operator sessions remain unchanged; resets and outputs are owned and repeatable.
- **CP-002:** Geometry/topology, persistent references, placements, joint/constraint/DoF and error state match the frozen independent oracle; volume alone is insufficient.
- **CP-003:** Alternative QR configurations preserve ranks, diagnostic identities, dependent behavior and numerical/lifetime/exception contracts; no shipped numerical policy changes.
- **CP-004:** GUI endpoints represent the correct model, selection and eventual completion; queued/observer/receiver lifetime and undo effects remain intact.
- **CP-005:** Every timing belongs to identified source/build/runtime/fixture/settings; traced spans and untraced user latency are never silently pooled or double-counted.

## Technical Context

- Language/toolchain: FreeCAD C++/Python/Qt/Coin/OpenCascade and existing solver/assembly components, reconfirmed by T001/T002.
- Target platform: initial local Linux investigation; exact GUI/driver/build/profile scope frozen before samples. Other platforms remain unqualified.
- Dependencies: no new mandatory runtime dependency; select installed profiling tools or record an explicit tool gap.
- Performance goals: measured rankings and defensible implementation proposals, not a preselected speedup percentage or new response-time budget.
- Storage: use existing qualified outputs only after inventory; avoid per-stream worktrees or build pairs.

## Success Criteria

- **SC-001:** All eight investigation streams have reproducible workload/endpoints, bounded evidence and explicit decisions; unmeasured hypotheses remain labeled.
- **SC-002:** Ordinary and larger model controls cover all six mainstream workflow families without image-centric fixtures.
- **SC-003:** B001 and B011 have exact source-bound results or evidenced limitation/rejection, with their correctness contracts preserved.
- **SC-004:** A cross-stream cost-benefit ranking names the next implementation destination(s), uncertainty and validation cost without double-counting.
- **SC-005:** Independent review, durable promotion and owned residual/closure records are complete; no change to production behavior is implied by investigation closure.

## Related Artifacts

- [Design](design.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Canonical context](canonical-context.md)
- [Decisions](open-decisions.md)
