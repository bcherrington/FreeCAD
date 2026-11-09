---
title: Spec 003 optional investigations
doc_type: reference
artifact_type: reference
status: active
owner: local-developer
last_reviewed: 2026-11-09
---

# Spec 003 Optional Investigations

The [backlog index](README.md) owns IDs/status/priority. These items are deferred
investigation proposals owned by local-developer. Routing removes conditional
new optimization work from spec 003's required delivery; it does not claim the
investigations ran or their prospective behavior was accepted. Core safety,
compatibility and per-retained-optimization measurements stay in the spec.

## B009 Reduced-Resolution Texture Exploration

- Owner/status: local-developer; deferred in the backlog index.
- Origin: spec 003 T009 and decision D002; the user authorized execution replanning.
- Current disposition: retain original/full-resolution texture semantics in the
  promoted reference. A lower-resolution first texture is an optional behavior proposal.
- Reactivation trigger: after the completed local full-resolution qualification, a new measured need justifies a separately scoped proposal.
- Required future work: define fidelity/physical calibration, original-source
  retention, texture replacement policy, latency/memory tradeoffs, compatibility
  and platform validation before implementation. Obtain a current source/build/
  fixture manifest; the old intake snapshot is not a future runtime certificate.
- Exit: an owned focused spec/issue with testable acceptance or rejection with
  rationale. No reduced-resolution texture implementation is accepted here.

## B010 Conditional GUI Restore And Startup Investigation

- Owner/status: local-developer; deferred in the backlog index.
- Origin: spec 003 T008 and historical C2/G3; this task's original conditional
  investigation acceptance remains here rather than being marked complete.
- Reactivation trigger: new current-source comparative results reveal a remaining
  dominant GUI restore substep or an independent startup complaint.
- Required future work: review fresh timings; either identify one measured
  substep with an owned follow-up or defer with evidence. For startup, compare
  normal and clean profiles and distinguish addon/user state from core costs.
  Do not reintroduce removed trace switches as if they already existed.
- Boundaries: existing parser/tree/image/restore performance claims have
  assigned comparative evidence in completed T004.3/T010; routing new investigation
  cannot waive those checks or route a current safety defect away.
- Exit: independently scoped GUI-restore or startup proposal(s), each with a
  current manifest, acceptance and owner, or an evidence-backed rejection.

## B011 Further Tree Batching Or Bulk-Delete Profiling

- Owner/status: local-developer; proposed in the backlog index, scoped for investigation in Spec005.
- Execution scope (2026-11-09): user-selected [Spec005 T009](../specs/005-mainstream-cad-performance-investigation/tasks.md) owns new profiling; no profile has run. Original invariants/exit checks below remain mandatory.
- Origin: spec 003 T007.2's conditional larger-document profiling before *further*
  batching/bulk deletion. The profile was not executed by this routing change.
- Reactivation trigger: after completed receiver-lifetime/restore-completion qualification, new larger-model evidence warrants another change.
- Required future work: profile a larger representative document before
  proposing further batching or bulk deletion; preserve selection, focus,
  hierarchy/maps and Python/diagnostic observer side effects.
- Boundaries: completed T007.1 proves local functional behavior on representative
  small and larger pinned documents; completed T004.3 records the existing retained
  tree optimization. Neither is deferred by this item.
- Exit: a measured focused proposal with explicit invariants/tests, or deferral
  with its evidence and applicability revision. No bulk-delete bypass is approved.

## Source Package

[Spec 003](../specs/003-freecad-performance-optimization/spec.md) is retained as history with all nine
MoE finding dispositions. T004/T010 local evidence is complete; these deferred
items do not reopen delivery. Current behavior and acceptance scope are in the
[promoted reference](../reference/document-open-performance.md).

## Closure Audit

Spec003 T011 verified B009/B010/B011 were deferred, owned by local-developer, with
separate activation triggers, acceptance/exit checks and non-blocking scope. No
reduced-resolution implementation, new startup investigation or larger-model
further-optimization profile was executed by routing. Numeric current timing
claims, other platforms and wider P05 memory/concurrency scopes require new
affected qualification before acceptance, as specified by the reference/runbook;
they are not promises of this local delivery.

## Subsequent Scope Selection

On 2026-11-09 the user selected B011 for the new mainstream CAD investigation
Spec005. It is now proposed and scoped to T009, with actual profiling pending
T001/T002 workload/runtime qualification. This changes future investigation
ownership/status, not Spec003 historical acceptance or permission for bulk
deletion. B009/B010 remain deferred.
