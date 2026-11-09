---
title: Mainstream CAD performance investigation design
doc_type: spec
artifact_type: design
status: draft
owner: local-developer
last_reviewed: 2026-11-09
---

# Technical Design

## Overview

Use a shared workload registry and qualified measurement harness, then execute
all eight streams in a stable linear order. Measure complete CAD actions first,
attribute their stages in separate bounded runs, validate the result, and rank
candidate changes. No optimization is implemented by this package. Controlled
existing QR settings and temporary opt-in instrumentation are experiment inputs,
not shipped defaults. Requirements precede this design; no design-first exception.

## Requirement Coverage

| Requirement | Acceptance Criteria | Design Coverage | Validation Approach |
| --- | --- | --- | --- |
| Requirement 1 | AC1, AC2, AC3, AC4 | design.md#workloads-and-provenance | Frozen operation/oracle, raw evidence, independent review and stream decision |
| Requirement 2 | AC1, AC2, AC3 | design.md#stream-contracts | Frozen operation/oracle, raw evidence, independent review and stream decision |
| Requirement 3 | AC1, AC2, AC3 | design.md#stream-contracts | Frozen operation/oracle, raw evidence, independent review and stream decision |
| Requirement 4 | AC1, AC2, AC3 | design.md#qr-comparison | Frozen operation/oracle, raw evidence, independent review and stream decision |
| Requirement 5 | AC1, AC2, AC3 | design.md#stream-contracts | Frozen operation/oracle, raw evidence, independent review and stream decision |
| Requirement 6 | AC1, AC2, AC3 | design.md#stream-contracts | Frozen operation/oracle, raw evidence, independent review and stream decision |
| Requirement 7 | AC1, AC2, AC3 | design.md#stream-contracts | Frozen operation/oracle, raw evidence, independent review and stream decision |
| Requirement 8 | AC1, AC2, AC3 | design.md#tree-investigation | Frozen operation/oracle, raw evidence, independent review and stream decision |
| Requirement 9 | AC1, AC2, AC3 | design.md#stream-contracts | Frozen operation/oracle, raw evidence, independent review and stream decision |
| Requirement 10 | AC1, AC2, AC3, AC4 | design.md#ranking-and-delivery | Frozen operation/oracle, raw evidence, independent review and stream decision |

## Correctness Property Coverage

| Property | Design Behavior | Validation Direction | Notes |
| --- | --- | --- | --- |
| CP-001 | Owned immutable fixture copies and reset checkpoints; isolated processes/preferences | Hash originals before/after, verify owned output/process identity | No live operator mutation |
| CP-002 | Independent semantic oracle per operation | Geometry/topology/reference/solver/joint comparisons and undo/reopen checks | Freeze tolerances with rationale before samples |
| CP-003 | Existing QR methods only, setting restore and mismatch rejection | Compare both ranks, ordered diagnostics, DoF and behavioral tests | No default/threshold/tolerance edits |
| CP-004 | Correct GUI completion and lifecycle invariants | Deterministic camera/pick/result checks, lifetime and observer tests | An event queued is not a presented result |
| CP-005 | Immutable sample identities, separate tracing, interval overlap rules | Hash manifests, raw-vs-summary reconciliation and overhead audit | No nested-duration summation |

## High-Level Design

### System Architecture

`fixture + operation recipe + reset/oracle` → `qualified owned runner` →
`untraced end-to-end observations` and `separate stage profiles` →
`correctness audit` → `stream report` → `cross-stream ranking` →
`durable guidance and separately scoped implementation proposals`.

### Components and Changes

- Registry/protocol: planned `workload-manifest.json` and `measurement-plan.json`.
  T001 records exact local/private locators separately from public fixture IDs.
- Harness: task-owned scripts, capture adapters and disposable preferences under
  the package or private build output. T002 inventories available capabilities;
  no assumed profiling hooks or external runtime bridge are treated as present.
- Probes: prefer sampling and existing supported APIs. If temporary source probes
  are necessary, bind the exact patch/build and verify overhead and trace-OFF
  behavior. Select a feature branch consistent with repository instructions.
- Reports: one per stream, a raw observation manifest and a final opportunities
  report. No permanent production API, cache, scheduler or tracing facility added.

### Data Models

A fixture record contains ID, generator/source/license or private category, hash,
model-scale counts, baseline parameters, dependencies, operation/reset recipe,
expected result/oracle, outputs and supported stream IDs.
A sample record contains run/cell/order IDs, source/patch/dependencies, actual
binary/runtime map hashes, fixture/settings hashes, operation and endpoint,
clock/unit, cache regime, warmup/traced flags, elapsed/CPU/memory and host context,
validity result, timeout/error, and links to raw profile/artifacts. Missing data
is explicit, never zero-filled. CPU totals may exceed wall time under threading.

### Data Flow

Read originals without mutation; copy to owned output; reset a verified starting
state; run exactly one frozen action sequence; observe correct completion; check
semantic output; retain raw result and failures. Public reports contain safe IDs,
counts and generated examples, not private paths, geometry or operator captures.

## Low-Level Design

### Workloads And Provenance

Proposed registry, finalized by T001 rather than assumed existing fixtures:

| Family | Representative workflow | Main reuse |
| --- | --- | --- |
| W01 machined part | Sketch → Pad → Pockets → hole/feature pattern → fillets/chamfers; dimension edit | Recompute, features, native open/save |
| W02 longer history | Valid dependent features with an early driving edit; ordinary/larger scales | Propagation, tree counts, feature scaling, persistence |
| W03 assembly | Jointed linked/repeated components; move component and edit referenced part | Assembly, viewport, tree, linked persistence |
| W04 imported detailed solid | Deterministic STEP import, camera/pick actions; native save/reopen separately | Import, tessellation, viewport, native I/O |
| W05 ordinary sketch | Common dimension/drag/add-remove constraint edits with dependent feature | Sketch latency, QR control, recompute |
| W06 complex sketch | Rank-sensitive larger constrained model plus synthetic aligned-slot/symmetric/near-rank-deficient cases | Complex sketch, QR comparison |

Prefer public reproducible generated fixtures and reuse actual existing models
only when permitted and copied immutably. W01/W02/W03/W05 need smaller controls
and representative larger variants where the stream requires scale comparison;
T001 records exact counts and rejects impossible/unrepresentative cases. An
imported single compound does not substitute for a many-object tree workload.
T001 labels edit classes explicitly: topology-preserving changes compare the
expected stable references; intentional topology changes have their own expected
feature/reference outcomes and may validly change face/edge counts. Freeze
expected outputs independently of the timed candidate. For sketch mutations,
record the exact initial state, pointer/drag trajectory or mutation sequence and
expected solution branch, so alternate valid solutions are not mistaken for
regressions or comparable inputs. Historical Spec000/003 measurements motivate
questions, not current baselines.
Choose one consistent build mode per comparison, with symbols for attribution.
T001 freezes Linux platform, display/input/theme/driver, viewport size/quality,
thread settings, addons, preferences, storage/filesystem and cache regime.
Inventory shared outputs: prior GUI/PartGui binary drift means receipts cannot
qualify whatever happens to be in build/debug. Verify actual module closure,
including embedded Python user-site libraries; package-lock identity alone is
insufficient. Use an owned profile/process and the established runtime-isolation
runbook when needed, without modifying user packages or an existing session.

### Collection And Attribution

Default screening proposal for T001: one untimed warmup and five scheduled untraced attempts
per frozen operation/cell, plus at most two separate exploratory profiles selected by a frozen
rule, or explicitly labeled posthoc diagnostics. This supports descriptive medians/ranges, not
reliable p95 or universal claims. T001 may select a different bounded plan with cost/rationale
before any samples. Require a fixed attempt schedule and minimum valid coverage (initial
proposal: all five); if coverage fails, report inconclusive/stop rather than sampling until
enough good rows exist. Retain every failed/excluded attempt. A recovery study has a new
prospective freeze and separate complete schedule; it never replaces or pools original
attempts. Initial default ceilings: 30 s ordinary interaction, 120 s complex edit or document
I/O, 300 s traced process and 30 min total per stream. Exact process, fixture and aggregate
disk budgets must be recorded before T002. A timeout is an observation/limitation, not
permission to silently increase limits or replace it.

Record timestamped host CPU utilization/load relative to CPU count, available RAM,
baseline/peak/post-operation process memory and I/O context/wait where available. Separate
logical temporary/retained payload, RSS and process-lifetime high-water marks; define the
post-operation observation point and allocator settling policy without assuming RSS
reclamation. State unavailable observations. Record frequency, thermal/power state and
GPU/driver observations if accessible; missing sensors are stated. Load alone does not
establish a cause. Define exclusion thresholds and coverage/abort rules before collection;
preserve excluded attempts with reasons and never replenish them within the original frozen
study. Keep cold/fresh-process versus warm-process regimes separate. Do not drop kernel caches
system-wide or generate pressure on the operator's active session.

Measure wall time through the actual intended endpoint. An App-only recompute, a Python call
return, the posted redraw and first correct visible result are different endpoints. Record
inclusive/exclusive span definitions; overlapping solver workers and nested kernel/model/GUI
spans are not additive. Sampling shares describe captured activity, not guaranteed removable
cost. Quantify tracing overhead with matched untraced controls; traced timings stay
descriptive. Use existing instrumentation only after source confirmation; otherwise T002
records the gap and qualifies a bounded adapter or narrower honest endpoint.

A promising candidate may receive one predeclared focused comparison extension rather than a
new full campaign. Default proposal: two warmups and 20 randomized paired scheduled attempts
per selected cell, with frozen reset/cache policy, minimum valid-pair coverage,
randomized/counterbalanced order and paired median difference/ratio intervals. Missing/invalid
pairs stay reported; no replacement pairs or optional stopping to obtain a favorable interval.
Do not promote a screening tail estimate into an SLA. If tail inference is needed, predeclare
adequate sample size/precision and an estimator; no automatic 10% variability veto or 500 ms/1
s requirement. No duplicate per-task worktrees/build pairs: follow the storage runbook, reuse
eligible outputs only, and retain evidence when outputs are later retired.



### Stream Contracts

| Stream | Task | Workload/action and attribution | Correctness and completion |
| --- | --- | --- | --- |
| R01 parametric recompute | T003 | W01/W02 early dimension edit; propagation, execution counts, solve, feature/kernel, reference mapping, tessellation/redraw | Independently expected edited shape/reference graph and undo/redo; App-only and correct visible endpoints separate |
| S01 sketch interaction | T004 | W05/W06 dimension change, drag, add/remove constraint; setup/diagnosis/solve/shape/downstream | Freeze exact mutation and immediate precursor, DoF/diagnostics/geometry/undo; forced recompute is separate |
| Q01 B001 QR | T005 | Reuse W05/W06 and rank-sensitive synthetic fixtures; paired existing-method comparison | See QR Comparison; incompatible configurations rejected |
| F01 feature operations | T006 | W01/W02 patterned holes/features, multi-body cuts/fuses, fillet/chamfer; preview vs committed work | Valid ordinary/larger cases; geometric/topological/reference/undo oracle; preserve legitimate errors |
| A01 assemblies | T007 | W03 move/joint solve and source-part edit; solving, placement, link/dependency propagation/display | Record implementation/addons and suppressed/fixed states; verify joint residuals, DoF, placements and linked part identity |
| V01 viewport/selection | T008 | W03/W04 camera trajectory, first tessellation, steady rotation/zoom, preselect/pick | Freeze camera/viewport/quality, identify selected face and correct highlight; no blank-frame or softwareGL-to-hardware extrapolation |
| T01 B011 tree | T009 | W02/W03 many objects; population/expand/status/select/delete/undo | See Tree Investigation; tree timings distinct from dependency recompute and redraw |
| I01 native persistence/import | T010 | W01/W02/W03 native open/save, W04 STEP import separately; archive/property/object/link/shape/GUI spans | Reopen owned outputs; validate features/references/joints/errors, console vs first visible completion and cache regime |

For V01, freeze timestamped input sequence and pacing: completion-gated camera
steps and fixed-rate input with event coalescing/backpressure are different cells,
not interchangeable measures. Record submitted/admitted/rendered/presented frame
identities/counts and dropped/coalesced events where observable; state gaps.
T002 source-confirms the correct-frame witness and whether it observes rendering,
readback or actual presentation. If compositor/GPU presentation cannot be
witnessed, report the narrower endpoint and never claim input-to-present latency
or smoother interaction from fewer rendered frames alone.

Assembly internal residual/DoF APIs are not assumed. D002 confirms what the
selected implementation exposes and qualifies independent joint-constraint and
placement checks where needed. Record unavailable internal metrics explicitly;
this does not waive correct motion, expected constraint state or linked geometry.

### QR Comparison

Inspect current selection plumbing and actual configured thresholds before use.
The B001 historical observation that QR dominated certain models is an opportunity
window, not a forecast for ordinary sketch edits or sparse QR superiority.
Compare existing methods in owned preferences/instances, preserving launch policy,
pivot/solver tolerances and algorithm semantics. Counterbalance/randomize method
order within paired trials with identical fixture precursor and warmup/reset.
Default to fresh owned processes for each arm; T002 may qualify per-instance
selection only if actual selection and all relevant state reset are proven.
Restoring preferences alone does not clear solver caches or allocator high-water
state. Verify actual method/settings each run and final restoration; record
pre/peak/post memory with the same boundaries, separating temporary/retained
payload from process RSS/high-water. Freeze both J/JT rank oracles,
ordered redundancy/conflict/dependency IDs and DoF, plus geometry/shape, external
reference and dependent Pad/Pocket results. Validate edit, move, undo/redo and
exceptional worker lifetime, including existing async/deferred checks where
applicable. A mismatch rejects that method/fixture scope immediately; retain the
failure instead of tuning tolerances to manufacture equivalence. Report matrix
size/sparsity, conversion/assembly/QR costs and peak memory separately. Restore
settings after each comparison and at exit. No shipped threshold/default change.

### Tree Investigation

B011 is activated as a scoped investigation, not approval for bulk deletion.
Use the many-object W02/W03 model, recording objects versus visible tree rows,
observer/update counts and event-loop work. Preserve selection/focus/hierarchy,
object/child maps, Python and diagnostic observer effects, queued callbacks and
widget subscriptions. Validate deletion/undo, destroy/recreate and eventual
finish/fallback after restore/import/failure. A future batching proposal needs an
explicit invariant/notification contract and a separate implementation task/spec;
never raw-revert existing lifetime/restore safety to obtain a counterfactual.

### Ranking And Delivery

Rank by observed absolute user latency, operation frequency (mark assumptions),
ordinary/larger model scope, confidence/dispersion/tails if measured, transient
and retained/process/GPU memory limits, concurrency, correctness/maintenance risk
and validation/implementation cost. Use the promoted eleven-field cost-benefit
contract; no numeric gain automatically justifies a numerical or geometry defect.
Recompute, sketch, QR, kernel, tree and display reports can describe the same
operation: group shared costs and do not add their percentages or savings.
For each stream choose pursue/defer/reject, with one owned destination for any
actual proposed change. A well-supported no-change result satisfies investigation
acceptance. Inadequate evidence needs a bounded explicit limitation and owned
next evidence action, not an invented optimization result or silent task closure.

### Function Signatures And Interfaces

No public API is added. Harness interfaces are internal experiment inputs:
`prepare(fixture, profile)`, `reset(checkpoint)`, `run(recipe, endpoint)`,
`validate(oracle)` and `emit(raw_record)`. T002 qualifies implementations against
current Python/C++ entry points rather than treating this notation as an API.

### Error Handling

Keep failures, timeouts, decoder/kernel/solver errors and partial output as named
ineligible/error results. Stop on oracle mismatch, preserve the failed specimen
privately and distinguish valid kernel rejection from regression. Bound and stop
only owned processes/descendants; no broad process kills or live model recovery.

### Security, Trust, And Access

Use explicit owned process/display/window identity; omit single-instance routing.
Public evidence is generated/sanitized. Never publish private fixture paths,
geometry, UI frames or credentials. Hash/version provenance does not authorize
operator-model mutation, addon installation, network communication or arbitrary
runtime capability calls. Discover actual tools and use their declared schemas.

### Migration And Compatibility

No schema/archive/solver/UI migration. B001/B011 get bidirectional scope links;
backlog status remains proposed until measured work and accepted disposition.
Closed Spec000/003/004 receipts and task records remain immutable.

### Slice Boundary And Residual Architecture

| Design target | In this slice | Out of this slice | Follow-up destination | Blocks closure? |
| --- | --- | --- | --- | --- |
| Eight streams | Bounded profiling, correctness audit, ranked findings | Production optimization or numerical-policy changes | T011 assigns one owned implementation destination per selected candidate | Investigation yes; implementing selected change no |
| Mainstream workflow scope | Six workload families with ordinary/larger controls | Image-first optimization and unrelated startup diagnosis | Existing B009/B010; local-developer | no |
| Platform scope | Exact declared local Linux runtime | Cross-platform/hardware extrapolation | Future proposal identified only if needed by T011 | no |
| Lasting guidance | New reference/runbook and B001/B011 status | Leaving accepted rules only in temporary spec | T012 promotion and history records | yes |

## Validation Strategy

| Validation | Covers | Evidence Location | Residual Risk |
| --- | --- | --- | --- |
| Registry/freeze/hash and owned-reset audit | Requirement 1, CP-001/005 | T001/T002 planned receipts | No old binary automatically eligible |
| Eight operation/oracle profiles | Requirements 2–9, CP-002/004 | Per-stream raw/report artifacts | Exploratory scope, not universal benchmark |
| QR compatibility and settings restoration | Requirement 4, CP-003 | Q01 comparison and semantic assertions | Rank sensitivity blocks incompatible config |
| Cost-benefit/deduplication review | Requirement 10 | Opportunities report and independent review | Claimed opportunity need not be realizable |
| Package/links/traceability/promotion audit | All requirements | verification.md and closure receipts | Structural checks do not prove runtime results |

## Downstream Task Guidance

- T001 resolves D001; T002 resolves D002 before any stream samples count.
- Use task order T001–T012; keep one task in progress. No speculative jump to QR/backend rewrites.
- Separate authored plan, qualified harness, profiling results, candidate selection and implementation authorization.
- If a stage or model is unavailable, resolve it in the registry/protocol and task evidence; do not quietly omit a selected area.

## Operational Considerations

Normal builds/tests use pixi. Freeze exact build/profile identities, disk/process
budgets and lock state; avoid rebuilding unchanged artifacts for prose changes.
Use native GUI profiling only in an owned session. Lifecycle/geometry/solver
changes in a future spec need focused tests then affected broader validation;
this authoring task needs no application build or benchmark.

## Open Questions

- D001: exact fixture/generator/license choices, scale/reset/oracles and initial collection budgets; owner local-developer, T001.
- D002: source-confirmed endpoints, QR method/config interfaces, tracing overhead and runtime isolation; owner local-developer, T002.
- The current request authorizes this investigation spec. New default/algorithm/architecture changes require a later scoped decision.

## Related Artifacts

- [Requirements](requirements.md)
- [Tasks](tasks.md)
- [Verification](verification.md)
- [Decisions](open-decisions.md)
