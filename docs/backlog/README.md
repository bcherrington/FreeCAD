---
title: Local FreeCAD development backlog
doc_type: reference
status: active
owner: local-developer
last_reviewed: 2026-11-09
planning_schema: slm-planning/v1
---

# Local Development Backlog

This index records local investigation proposals, not commitments by upstream
FreeCAD maintainers. IDs and statuses are authoritative here. Detailed scope,
evidence, risks and exit checks are in [Sketcher follow-ups](sketcher-performance.md)
and [Spec 003 optional investigations](spec003-follow-ups.md).
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
| B009 | deferred | Explore reduced-resolution initial textures only through a separate measured behavior proposal | later | local-developer | docs/backlog/spec003-follow-ups.md#b009-reduced-resolution-texture-exploration |
| B010 | deferred | Revisit GUI restore/startup profiling only after fresh results justify a focused investigation | later | local-developer | docs/backlog/spec003-follow-ups.md#b010-conditional-gui-restore-and-startup-investigation |
| B011 | proposed | Profile larger models before proposing further tree batching or bulk deletion | later | local-developer | docs/backlog/spec003-follow-ups.md#b011-further-tree-batching-or-bulk-delete-profiling |
| B012 | done | Repair isolated Part area-handler initialization in Measure CTest | PR prep | local-developer | docs/backlog/reattachment-delivery-follow-ups.md#b012-isolated-measurement-handler-registration |
| B013 | done | Repair Part loft task-selector fixture eligibility | PR prep | local-developer | docs/backlog/reattachment-delivery-follow-ups.md#b013-part-loft-task-selector-test |
| B014 | proposed | Prepare focused separate-worktree reattachment PR and platform/human review | PR prep | local-developer | docs/backlog/reattachment-delivery-follow-ups.md#b014-upstream-reattachment-delivery |
| B015 | proposed | Repair existing Assembly view-test Label mocks | later | local-developer | docs/backlog/assembly-motion-suspension-follow-ups.md#b015-view-test-fixtures |
| B016 | proposed | Qualify upstream/platform and MCP consumption of motion suspension | PR prep | local-developer | docs/backlog/assembly-motion-suspension-follow-ups.md#b016-upstream-and-bridge-qualification |

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

2026-11-06: user-authorized spec 003 execution replanning routed reduced-resolution
texture exploration (B009), conditional GUI/startup investigation (B010), and
profiling for additional tree optimization (B011) to owned deferred follow-ups.
The active spec retains original-resolution semantics, all current safety and
compatibility corrections, and measurements for every retained optimization.
Routing records scope/ownership, not completed investigation or implementation.

2026-11-08: spec 004 local implementation/lifecycle qualification is complete.
B012/B013 own the two independently scoped whole-project CTest failures; B014
owns external upstream delivery. This records a non-green project run and future
human/platform gates; it does not weaken reattachment acceptance or authorize
changes to other agents' work.

2026-11-09: T010 commit `21903ca31f` resolves B012 through real Measure module
initialization and B013 through restored finite-edge loft admission. Existing
assertions remain intact. Both isolated cases and the unfiltered qualified Linux
full graph pass (2426 passes, zero failures, two existing skips, eight disabled).
Earlier spec004 failed runs remain historical evidence. B014 stays proposed;
At Spec003 closure B009/B010/B011 were deferred and owned by local-developer;
the subsequent Spec005 selection below updates B011 to proposed.

2026-11-09: the user selected six mainstream CAD investigation areas plus B001
QR comparison and B011 large-model tree profiling. Their execution scope is now
[Spec005](../specs/005-mainstream-cad-performance-investigation/spec.md), with
T005 owning B001 and T009 owning B011. Both are proposed/scoped, not executed;
B011 moves from deferred to proposed. Original numerical/tree invariants and exit
checks remain. B009 reduced-resolution images and B010 unrelated startup/GUI
investigation remain deferred. No implementation/default change is selected.
