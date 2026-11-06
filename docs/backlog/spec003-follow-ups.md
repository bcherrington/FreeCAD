---
title: Spec 003 optional investigations
doc_type: reference
artifact_type: reference
status: active
owner: local-developer
last_reviewed: 2026-11-06
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
  active spec. A lower-resolution first texture is an optional behavior proposal.
- Reactivation trigger: after current full-resolution correctness/latency/memory
  qualification, a measured need justifies a separately scoped proposal.
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
- Reactivation trigger: fresh spec 003 comparative results reveal a remaining
  dominant GUI restore substep or an independent startup complaint.
- Required future work: review fresh timings; either identify one measured
  substep with an owned follow-up or defer with evidence. For startup, compare
  normal and clean profiles and distinguish addon/user state from core costs.
  Do not reintroduce removed trace switches as if they already existed.
- Boundaries: existing parser/tree/image/restore performance claims still need
  assigned comparative evidence in active T004.3; routing new investigation
  cannot waive those checks or route a current safety defect away.
- Exit: independently scoped GUI-restore or startup proposal(s), each with a
  current manifest, acceptance and owner, or an evidence-backed rejection.

## B011 Further Tree Batching Or Bulk-Delete Profiling

- Owner/status: local-developer; deferred in the backlog index.
- Origin: spec 003 T007.2's conditional larger-document profiling before *further*
  batching/bulk deletion. The profile was not executed by this routing change.
- Reactivation trigger: after current receiver-lifetime/restore-completion fixes
  and functional qualification, larger-model evidence warrants another change.
- Required future work: profile a larger representative document before
  proposing further batching or bulk deletion; preserve selection, focus,
  hierarchy/maps and Python/diagnostic observer side effects.
- Boundaries: current T007.1 still proves functional behavior on representative
  small and larger pinned documents; T004.3 still measures the existing retained
  tree optimization. Neither is deferred by this item.
- Exit: a measured focused proposal with explicit invariants/tests, or deferral
  with its evidence and applicability revision. No bulk-delete bypass is approved.

## Source Package

[Spec 003](../specs/003-freecad-performance-optimization/spec.md) retains all nine
MoE finding destinations and its four-phase execution/closure obligations.
