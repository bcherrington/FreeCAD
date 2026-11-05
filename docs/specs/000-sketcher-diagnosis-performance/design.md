---
title: Sketcher diagnosis performance design
doc_type: spec
artifact_type: design
status: archived
owner: local-developer
last_reviewed: 2026-11-05
---

# Design

## Overview

Deliver two independently reviewable slices: optional diagnostics, then removal
of duplicate diagnosis in ordinary recompute. Measure before and after using
the same diagnostics configuration; use separate tracing-off measurements for
end-user latency. Preserve the public solver and property notification contracts.

## High-Level Design

Keep SketchObject orchestration separate from solver calculations, share only a
bounded optional timing mechanism, and preserve existing solver/notification APIs.

## Low-Level Design

### D-001 - Defer the redundant diagnosis locally

Relevant sources are `src/Mod/Sketcher/App/SketchObject.cpp`,
`SketchObjectConstraints.cpp` and `PropertyConstraintList.cpp` in that directory.
`execute()` calls geometry acceptance; its property notification can enter
`onConstraintsChanged()` and `setUpSketch()`. The later `solve()` sets up again.

First prove the call sequence with a deterministic regression fixture. Prefer
an existing scoped managed-operation mechanism around the internal geometry-key
acceptance, if inspection proves it preserves validation and notifications.
Do not bypass notifications globally or mark the whole operation as managed
without examining all nested callbacks. Keep one explicit solve and its geometry
writeback. Confirm execution outside normal recompute keeps required diagnosis.
The managed branch of `onConstraintsChanged()` returns after `checkGeometry()`;
its unmanaged branch also calls `checkConstraintIndices()`. `acceptGeometry()`
clears `invalidGeometry`, not the separate `invalidIndices` flag, and `getValues()`
returns an empty list while either flag is set. A managed-operation guard is a
candidate, not a validated solution. T002 must locate equivalent index validation
before the authoritative solver consumes constraints, covering both newly invalid
indices and repaired indices after internal/external geometry changes.

The omitted `SketchObject::setUpSketch()` wrapper also conditionally touches
`Constraints` for conflict, redundancy, malformed and partial-redundancy results.
The later solve does not reproduce that conditional touch automatically. T002
must state where required diagnostic IDs, touched state and observer effects
are produced. Record a baseline observer sequence/count for each non-success
case, identify effects that downstream consumers require, and enumerate any
allowed intermediate differences with their justification and regression tests.
Until this contract and the index-validation transition tests are defined,
guard selection and T004 are blocked. Global notification suppression is excluded.
If a local guard cannot satisfy the contract, record an alternative before coding.

The one-setup invariant applies only to the targeted ordinary execute path.
Preserve scalar Python insertion's pre-commit solve for undo, list insertion's
existing deferred-solve behaviour, edit-mode `noRecomputes` refreshes and direct
property-change handling. Test transaction undo plus undo/redo immediately
followed by direct solve/move, with no intervening document recompute. These
paths may legitimately perform additional setups; do not optimize them by
applying the ordinary-execute invariant globally.

T002 resolved this candidate selection in
[C-001/C-002](timing-invalidation-contract.md#c-001---validation-ownership-and-eligibility).
Use the narrower one-shot recompute context after callback validation, rather
than `managedoperation`. Preserve the conditional diagnostic touch through the
private authoritative-solve handoff. The contract records measured observer
sequences, the permitted intermediate reordering and legacy fallback conditions.
T004 implements and proves this contract in
[diagnosis-deferral-report.md](diagnosis-deferral-report.md), including observer-input
refresh and exception recovery. No guard was implemented by T002.

### D-002 - Optional scoped phase timings

Reuse existing Sketcher logging/reporting and time facilities where appropriate.
Keep implementation within Sketcher App and planegcs; avoid a generic telemetry
framework or a GUI dependency in the solver. T002 selected the development control
`FREECAD_SKETCHER_TRACE=1`; T003 implements it as documented in
[phase-tracing-report.md](phase-tracing-report.md). Do not repurpose `lastSolveTime`
to mean total operation.

A record contains an operation identifier, optional parent identifier, phase,
reason, document/sketch identity, elapsed duration and completion outcome. It also
contains monotonic start/end offsets from the operation origin and an execution-
lane identifier. Carry the same clock origin/context into QR workers. Capture
worker execution, caller-side dispatch/copy and join/wait as distinct spans;
worker spans may overlap the caller's dispatch/join spans, so never sum these as
independent elapsed time. Preserve `std::async`'s existing launch policy and
record enough lane/timestamp evidence to identify deferred work at the join.
Include the outer `execute()` operation so timing contains geometry acceptance
and the candidate redundant setup, not only the later explicit solve.
Use RAII or equivalent exception-safe scope completion. When QR runs in parallel,
collect durations on the worker and serialize reporting through a safe owner;
do not add unsafe worker-thread console use. Never emit geometry coordinates,
constraint values or document contents in timing records.

Instrument setup construction, Jacobian assembly, J/JT QR, diagnostic redundancy
solve, main/fallback solve, geometry writeback and shape building. Record an
inclusive total at SketchObject entry. Benchmark whole-document time separately.
Initial load and explicit solves need distinct reasons; nested phases must not
be summed as independent wall time. Disabled tracing avoids per-event formatting,
identifier strings and heap allocation. Thread-safe enablement/lifetime choices
must be explicit in the implementation review.

Once per diagnosis, capture small structural counters: driving constraints,
expanded diagnostic equations, diagnostic unknowns, Jacobian rows/columns and
estimated payload bytes (`rows * columns * sizeof(double)` for the current dense
matrix), actual dense/sparse QR selection and resulting rank. Label counts as
pre/post reduction where applicable, and record unavailable rank on failure.
These are counts, not parameter values or geometry payloads. Reuse existing
sizes/results rather than assembling or scanning matrices only for logging.
Do not derive counters, format records or allocate trace state when tracing is off.
T003's source boundary includes the SketchObject entry/shape scopes and the
context handoff into `Sketch`/planegcs, in addition to the solver phase scopes.

T002's selected control, schema, lifetimes, QR boundaries and exact counter
stages are defined in
[C-003 through C-006](timing-invalidation-contract.md#c-003---trace-control-correlation-and-ownership).
The process-start control `FREECAD_SKETCHER_TRACE=1` and the private helper are
implemented in T003, with 21 tracing tests and isolated-process validation.
Indicative disabled-instrumentation assessment is complete in T005; uncertainty
is disclosed under timing-evidence-decision.md.

### D-003 - Fixtures and comparative measurement

Prefer small synthetic native fixtures committed with regressions; keep real
operator models private. Use frozen disposable copies of the master and plate
models for performance evidence. Preserve reference/driver relationships and
model placement; no performance-only simplification of acceptance fixtures.

Correctness comparisons cover constraint/geometry counts and identities, DoF,
diagnostic classifications, endpoint/tangency relationships within explicit
numeric tolerances, external-link propagation, valid solids and undo/redo.
A timer-only change must not change any of those results.

Use matching optimized builds for performance acceptance. Debug builds remain
useful for call counts and correctness; optimized builds provide indicative timings.
Record machine, source commit, compiler/dependencies, build flags, preferences,
fixture hashes, warmups, sample order, raw paired samples, median, median absolute
deviation or range. Bootstrap intervals are optional context. Alternate configurations
where practical. The normative workload/reset/cache definitions and indicative
evidence policy are in [verification.md](verification.md#measurement-matrix).

T001 freezes separate clean no-op and forced unchanged-geometry execution cases,
asserting actual execute/setup counts. Only the latter is eligible for diagnosis
speedup acceptance. Distinguish fresh-process load from reopened warm load, and
label filesystem cache state as uncontrolled unless independently established.
Freeze each edit's precursor, reset method and measured boundary before collecting
results; discard and explain any pair that did not perform the intended operation.
Preserve originals and restore disposable starting states; do not retry unknown
bridge outcomes. Disclose timing uncertainty without requiring strict confidence
bounds; review apparent repeatable material slowdowns. The explicit user-approved
policy is in [timing-evidence-decision.md](timing-evidence-decision.md).

### D-004 - Evidence-led follow-up gate

After the initial slices, attribute remaining setup cost and rank:

- Avoiding unchanged geometry-key metadata rebuilds/notifications.
- Reducing asynchronous matrix/map copies with safe immutable lifetimes.
- Reusing structure with explicit invalidation rules.
- Sparse Jacobian assembly and earlier component analysis.

A recommendation must identify affected phase, measured fraction of cost,
expected bounded saving, correctness risks and validation workload. No automatic
threshold/tolerance/default changes or further solver rewrites are authorized by
an unchecked task in this package. Scope updates need recorded rationale and
acceptance criteria; larger changes get their own follow-up package.

## Data And Contract Impact

No FCStd format, public Python API, solver result or existing timing getter
changes. T003 adds only the optional development diagnostic surface. Defaults
remain silent. No new dependency is planned. No new modelling/MCP operation.

## Operational Considerations

Branch `perf/sketcher-diagnosis` starts at integration commit `36aa20c979` and
already includes native crash fix `c9af1a45d1`. Keep changes off the integration
branch until validated and explicitly merged. A standalone upstream submission
must isolate the relevant source commits from unrelated integration history.
`docs/` is a separate repository with a matching branch; preserve both histories.

## Resolved Implementation Decisions

T001 identified native and external-reference test targets in
[baseline-report.md](baseline-report.md). T002 resolved the remaining decisions
in [the timing/invalidation contract](timing-invalidation-contract.md): immutable
environment opt-in outside edit mode, deferral after normal index validation,
explicit diagnostic notification handoff, and a small worker trace handle with
the original async payload copies/launch policy/wait semantics preserved.

Implementation validation remains mandatory. An unexpected required observer,
unsafe exceptional handoff or changed async payload lifetime blocks its affected
implementation until the contract is met; it does not authorize a broad guard,
notification suppression or altered launch policy.

## Open Questions

No unresolved design choice remains for T003/T004. The contract's exception,
observer-consumer, async lifetime and disabled-cost assertions still need their
declared implementation tests. Remaining phase attribution and performance
decisions belong to T005/T006, not design completion in T002.

## Durable Documentation Impact

Promote implemented behavior, trace fields, enablement and limitations to proposed
`docs/reference/sketcher-performance-diagnostics.md`. Promote the repeatable
workflow to proposed `docs/runbooks/sketcher-performance-investigation.md`.
Those paths do not exist yet and must describe verified behavior when created.
The unrelated restore/image performance spec retains its existing ownership.

## T006 Attribution And Follow-Up Selection

[remaining-cost-decisions.md](remaining-cost-decisions.md) resolves D-004's ranking.
QR compute covers much of the complex-sketch execute; shape construction dominates
some smaller outlines. Async dispatch and explicit geometry acceptance have small
measured envelopes, so they do not justify an assumed large copy/metadata saving.
Reuse and sparse assembly remain deferred until their prerequisites are met.
The [local backlog](../../backlog/README.md) owns B001–B007, with scope, cost bounds,
risks and exit checks in [the details](../../backlog/sketcher-performance.md).
The historical starting checkpoint hash is recovered; immediate pre-mutation
identity/phase attribution still require B006. T006 implements no solver rewrite,
cache, threshold/default change or notification shortcut. T007 promotion and review are complete; this package is retained as history.
