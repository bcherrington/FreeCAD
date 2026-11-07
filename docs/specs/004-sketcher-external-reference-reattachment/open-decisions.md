---
title: Sketcher external reference reattachment open decisions
doc_type: spec
artifact_type: open-decisions
status: draft
owner: local-developer
last_reviewed: 2026-11-07
---

# Open Decisions

## Purpose

This ledger is authoritative for decision status and ownership. Design explains
the proposed answers; none is an upstream agreement. T001 qualifies these technical
decisions locally before implementation. No extra user approval is implied.

## Decision Log

| ID | Decision Needed | Options | Owner | Needed By | Status |
| --- | --- | --- | --- | --- | --- |
| D001 | Confirm public signature/indexing and native extension point | Proposed scalar attachExternal with negative geometry index and strict native helper; revise only with explicit compatibility rationale | local-developer | T001 before T002/T003 | open |
| D002 | Demonstrate exact Orientation preservation, coherent publication and full failure restoration | Stage one compatible projection; prove notification-time links/modes/Ref/IDs, dependencies and dirty/touched/recompute/transaction/redo restoration with bounded native mechanisms; revise the spec explicitly if infeasible | local-developer | T001 before T002 | open |

## Blocking Impact

| Decision ID | Blocks | Impact If Unresolved |
| --- | --- | --- |
| D001 | T002, T003 | Public API and native compatibility contract not ready to implement |
| D002 | T002, T004 | Exact Orientation, coherent notifications, dependency/backlink and complete failure-state preservation unproven; managedoperation alone is insufficient |

## Contract Refinements From MoE Review

The following scope choices address review findings under the user's instruction
to revise this spec. They define proposed acceptance, not demonstrated native behavior:

- Preserve Constraint.Orientation exactly in this slice; reject a reversed/ambiguous
  replacement if that cannot survive normal rebuild and repeated recomputation.
- Admit only Projection-mode, live, unfrozen, attached one-to-one references.
  Both/Intersection and Frozen/Missing/Detached states reject before no-op detection.
- Support calls without an active transaction and with active/booked transactions;
  permit normal transaction materialization only for successful property edits.
- Reject native noRecomputes/edit-mode calls, including same-source requests.
- Require failure/no-op model and journal state preservation. Coherent intermediate
  failure notifications may be documented; inconsistent publication is prohibited.
- Prove the publication/restoration mechanism in T001 before closing D002. If it
  requires a broad App redesign, report infeasibility and revise the contract before
  implementing; do not silently weaken it or mark this decision resolved.

## Resolution Rule

Record owner, date, chosen option, source/probe evidence, rationale and effects on
requirements/design/tasks/verification before marking a decision resolved. A
planning proposal or structural lint result is not proof of a native mechanism.
If qualification changes scope, reconcile both directions of traceability.

## Related Artifacts

- [Design](design.md#open-questions)
- [Tasks](tasks.md)
- [Traceability](traceability.md#open-decision-impact)
- [Verification](verification.md#readiness-decision)
