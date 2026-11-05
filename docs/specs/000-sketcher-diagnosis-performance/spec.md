---
title: Sketcher diagnosis performance specification
doc_type: spec
artifact_type: spec
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# Feature Specification

## Summary

Reduce repeated Sketcher setup and diagnosis during document recompute, add
optional phase timings, and use comparable measurements to select further work.
The user requested this package after discussing these two initial changes.
This package authorizes their implementation plan; implementation has not started.

## Problem

Static analysis identifies an ordinary recompute path that diagnoses a sketch
when geometry keys are accepted and then reconstructs and diagnoses it again in
`SketchObject::solve()`. Historical diagnostics record two setup passes taking
7.38566 s and 5.08845 s before a numerical solve taking 0.000857262 s.
These are historical measurements, not benchmarks of the current plate model.
`lastSolveTime` excludes setup, so it cannot explain the whole operation.

## Goals

- Remove the avoidable setup pass without altering modelling semantics.
- Attribute remaining cost to phases and identify repeated calls by sketch/reason.
- Establish reproducible correctness and performance baselines.
- Rank further optimizations from evidence and route them to bounded work.

## Non-Goals

- Change QR defaults, tolerances, solver fallbacks or conflict semantics.
- Introduce solver caching, sparse assembly or matrix lifetime changes initially.
- Simplify the user's constraints or edit the working CAD document.
- Change MCP deadlines, transport, late-result recovery or the crash fix.
- Expand the separate document-open/image performance package.

## Requirements And Acceptance

The numbered requirements and AC-001 through AC-007 are authoritative in
[requirements.md](requirements.md). This entry point retains the repository's
local spec template; lifecycle requirements and traceability are explicit
companion artifacts rather than duplicated normative text.

## User Scenarios And Testing

### User Story 1 - Faster recompute (Priority: P1)

A user changing a sketch or a dependent dimension avoids duplicate diagnosis
while retaining identical geometry and recoverable diagnostic failures.
Independent test: count setups on the targeted execute path, verify invalid/
repaired index transitions and diagnostic IDs/touched/observer behaviour, and
preserve scalar/list insertion, edit-mode and immediate post-undo solve/move. Maps to FR-001/FR-002, AC-001/AC-002.

### User Story 2 - Explain slow operations (Priority: P1)

A developer can distinguish reconstruction, diagnosis and nonlinear convergence.
Independent test: enable tracing on a small fixture, reconstruct concurrent and
deferred QR spans and verify structural counters, then disable it and verify
silence and unchanged getter semantics.
Maps to FR-003/FR-004/FR-005, AC-003/AC-004/AC-006.

### User Story 3 - Select the next optimization (Priority: P2)

A maintainer receives matched timing evidence and bounded follow-up proposals.
Independent test: reproduce an executed/reset-checked paired benchmark receipt,
apply uncertainty/inconclusive gates and inspect the decision table.
Maps to FR-006/FR-007, AC-005/AC-007.

## Edge Cases

- Geometry-key acceptance following external geometry rebuild or migration.
- Empty sketches, reference-only constraints and undo/redo transaction callbacks.
- Partially redundant, invalid or rank-sensitive symmetric configurations.
- Failures before geometry writeback, nested diagnostic solves and asynchronous QR.
- Existing advanced solver settings and alternate primary/fallback solvers.

## Success Criteria

- **SC-001**: AC-001 and AC-002 pass before performance gains are accepted.
- **SC-002**: AC-003, AC-004 and AC-006 establish usable optional diagnostics.
- **SC-003**: AC-005 has reproducible before/after evidence and budget disposition.
- **SC-004**: AC-007 and durable documentation promotion complete before closure.

## Related Artifacts

- [Requirements](requirements.md), [Design](design.md), [Plan](plan.md), [Tasks](tasks.md).
- [Traceability](traceability.md).
- [Research](research.md), [Verification](verification.md), [Quickstart](quickstart.md).
