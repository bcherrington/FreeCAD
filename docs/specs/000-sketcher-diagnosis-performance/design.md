---
title: Sketcher diagnosis performance design
doc_type: spec
artifact_type: design
status: draft
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
If an existing guard cannot meet those conditions, record a local alternative
before implementation; a broad notification-system redesign is outside scope.

### D-002 - Optional scoped phase timings

Reuse existing Sketcher logging/reporting and time facilities where appropriate.
Keep implementation within Sketcher App and planegcs; avoid a generic telemetry
framework or a GUI dependency in the solver. Select the exact enablement mechanism
in T002 and document it before coding; no setting name in this package is an
implemented interface. Do not repurpose `lastSolveTime` to mean total operation.

A record contains an operation identifier, optional parent identifier, phase,
reason, document/sketch identity, elapsed duration and completion outcome.
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
useful for call counts and correctness but cannot establish production budgets.
Record machine, source commit, compiler/dependencies, build flags, preferences,
fixture hashes, warmups, sample order, raw samples, median and spread. Alternate
baseline/candidate ordering where feasible. Separate load, unchanged recompute,
dimensional edit, scalar/list insertion and symmetry workloads. Preserve and
restore each starting state; do not retry unknown bridge outcomes.

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
changes. Only an optional development diagnostic surface is proposed. Defaults
remain silent. No new dependency is planned. No new modelling/MCP operation.

## Operational Considerations

Branch `perf/sketcher-diagnosis` starts at integration commit `36aa20c979` and
already includes native crash fix `c9af1a45d1`. Keep changes off the integration
branch until validated and explicitly merged. A standalone upstream submission
must isolate the relevant source commits from unrelated integration history.
`docs/` is a separate repository with a matching branch; preserve both histories.

## Open Questions

- Which existing enablement surface best provides optional tracing outside edit mode?
- Which scoped guard removes only redundant diagnosis without skipping validation?
- What are the exact native test/fixture targets needed for external-link cases?
- Can diagnostic QR worker durations be collected without extra matrix copies?

T001/T002 resolve these before the affected implementation task starts.

## Durable Documentation Impact

Promote implemented behavior, trace fields, enablement and limitations to proposed
`docs/reference/sketcher-performance-diagnostics.md`. Promote the repeatable
workflow to proposed `docs/runbooks/sketcher-performance-investigation.md`.
Those paths do not exist yet and must describe verified behavior when created.
The unrelated restore/image performance spec retains its existing ownership.
