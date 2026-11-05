---
title: Local FreeCAD development backlog
doc_type: reference
status: active
owner: local-developer
last_reviewed: 2026-11-05
planning_schema: slm-planning/v1
---

# Local Development Backlog

This index records local investigation proposals, not commitments by upstream
FreeCAD maintainers. IDs and statuses are authoritative here. Detailed scope,
evidence, risks and exit checks are in [Sketcher follow-ups](sketcher-performance.md).
The first entries come from [T006 attribution](../specs/000-sketcher-diagnosis-performance/remaining-cost-decisions.md).
Proposed/deferred items do not authorize an implementation or change a solver default.

## Items

| ID | Status | Topic | Priority | Owner | Source |
| --- | --- | --- | --- | --- | --- |
| B001 | proposed | Compare existing dense and sparse QR on frozen rank-sensitive fixtures | 1 | local-developer | docs/backlog/sketcher-performance.md#b001-qr-method-comparison |
| B002 | proposed | Isolate async payload copying before a lifetime-safe prototype | 4 | local-developer | docs/backlog/sketcher-performance.md#b002-async-payload-copying |
| B003 | deferred | Revisit unchanged geometry-key refresh only if its cost becomes material | 5 | local-developer | docs/backlog/sketcher-performance.md#b003-geometry-key-refresh |
| B004 | deferred | Design structural reuse with explicit invalidation and numeric refresh | 6 | local-developer | docs/backlog/sketcher-performance.md#b004-structural-reuse |
| B005 | deferred | Consider sparse assembly after evidence of assembly or memory pressure | 7 | local-developer | docs/backlog/sketcher-performance.md#b005-sparse-assembly |
| B006 | deferred | Recover exact mutation precursors for historical symmetry/equality stalls | 2 | local-developer | docs/backlog/sketcher-performance.md#b006-mutation-stall-replay |
| B007 | proposed | Attribute shape construction and downstream propagation on smaller sketches | 3 | local-developer | docs/backlog/sketcher-performance.md#b007-shape-construction |
| B008 | proposed | Add diagnostic-observer external-property edit regression before upstream preparation | PR prep | local-developer | docs/backlog/sketcher-performance.md#b008-external-observer-regression |

## Decision History

2026-11-05: T006 introduced these evidence-led proposals. QR work targets the
observed dominant complex-sketch phase; shape work addresses a different small-
sketch profile. Copy/metadata opportunities are bounded by measured enclosing
spans. Reuse and sparse assembly are deferred for correctness risk and limited
current attribution. Mutation replay is deferred until immediate precursor
identity is verified. Owner: local-developer; no new solver rewrite started.

2026-11-05: T007 source review found no blocking defect and requested an additional
external-property observer regression. B008 records this nonblocking test follow-up;
it does not claim that case was executed in local delivery evidence.
