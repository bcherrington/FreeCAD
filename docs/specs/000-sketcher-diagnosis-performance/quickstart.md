---
title: Sketcher diagnosis performance quickstart
doc_type: spec
artifact_type: quickstart
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# Quickstart

## Purpose

Resume the performance work without touching the operator's working CAD session.
This is a proposed implementation workflow, not an implemented diagnostic API.

## Prerequisites

- FreeCAD branch `perf/sketcher-diagnosis`, based on `36aa20c979`.
- Matching branch in the separate `docs/` repository.
- Read [spec](spec.md), [requirements](requirements.md), [traceability](traceability.md),
  [design](design.md), [plan](plan.md), [research](research.md),
  [tasks](tasks.md) and [verification](verification.md), plus applicable AGENTS.md.
- Preserve existing unrelated docs edits. The package uses local repository templates.

## Steps

1. Use Spec Lifecycle Manager preflight/task context for
   T001 - Reconcile source and establish the baseline. Recheck source freshness
   through Agent Workbench and direct reads; mark T001 in progress before work.
2. Verify fixture hashes, create disposable copies and discover native tests.
   Freeze workload execution/reset/cache manifests and capture baseline builds,
   preferences and execute/setup counts; separate no-op from actual recompute.
3. Resolve trace enablement, offsets/lanes/dispatch/join spans and structural counters
   in T002. Approve index-validation and diagnostic observer/touched invariants
   before guard selection; include non-recompute paths in the regression plan.
4. Implement T003 and T004 as separate reviewable slices, preserving notifications.
5. Build with `pixi run build -j 2`, run the discovered focused/affected checks,
   then use matched optimized builds for T005's alternating-pair/MAD/bootstrap
   measurements and bounded inconclusive-result protocol.
6. Record the follow-up decision in T006 and promote verified guidance in T007.

## Expected Results

The affected ordinary execute path diagnoses once with identical modelling
results. Optional timings distinguish setup, QR and solve costs. Performance
results include raw repeated samples and explicit budget dispositions.

## Cleanup

Close only task-owned disposable documents/processes. Keep operator originals,
preferences and current runtime intact. Retain private measurements outside Git;
remove temporary diagnostic code unless intentionally retained and reviewed.
Do not merge into the integration branch or install a snapshot as part of merely
creating or resuming the specification.
