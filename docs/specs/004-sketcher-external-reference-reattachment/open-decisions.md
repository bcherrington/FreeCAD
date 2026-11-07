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

No open local design decisions remain. Public maintainer agreement and new-API
acceptance are implementation/review gates, not completed by these decisions.

## Blocking Impact

| Decision ID | Blocks | Impact If Unresolved |
| --- | --- | --- |

## Resolved Decisions

### D001 — Single-reference public API and native boundary

- **Status:** resolved locally on 2026-11-07 by T001 qualification; no upstream agreement implied.
- **Owner:** local-developer, technical selection under the user's instruction to start T001.
- **Chosen option:** `reattachExternal(geo_id: int, obj_name: str, sub_name: str, /) -> None`.
  Use a negative current geometry index, same-document name lookup and native Edge
  resolution. TypeError for invalid types/bool, OverflowError before narrowing, and
  ValueError for invalid references. Keep native vector attachExternal unchanged.
- **Rationale:** this is replacement of one live reference, distinct from native
  grouped attachment/detached-reference behavior. Local source has no consumers of
  attachExternal beyond its declaration/definition. Python exposes neither name today.
- **Evidence:** [T001 qualification](t001-qualification.md#d001-qualified-interface-and-extension-point),
  native receipt and public-surface probe in t001-validation.json.
- **Limits:** no method has been implemented. D002 determines the accepted failure
  and notification contract; reversed-orientation acceptance remains unqualified.

### D002 — Normal FreeCAD transaction semantics (Option A)

- **Status:** resolved on 2026-11-07 by explicit user acceptance: "I'm happy with option A, continue".
- **Owner:** local-developer; user accepted the material contract change.
- **Chosen option:** preserve model/journal state for preflight rejection and admitted
  exact no-op. Apply through ordinary native properties; intermediate notifications
  are allowed. Successful return must satisfy all identity/order/constraint/reference
  invariants. Unexpected apply errors propagate for caller-owned abort or checkpoint
  recovery; no automatic commit, abort or retry and no post-application no-call promise.
- **Rationale:** executed probes observed mixed-state notifications, residual touched
  state and redo loss under existing setters. A generic atomic multi-property or
  savepoint infrastructure expansion is not needed for the selected API slice.
- **Recovery boundary:** caller abort rolls back the entire caller transaction,
  including earlier edits. Without an active transaction, checkpoint recovery is
  needed after an unexpected apply error. A dedicated per-operation transaction is
  recommended when the caller owns the boundary. Dirty/redo history may change once
  application begins and is not guaranteed to return to pre-call state after recovery.
- **Retained constraints:** exact Constraint.Orientation on success; preflight rejection
  of reversed/ambiguous mappings, unsupported modes/states and noRecomputes/edit mode;
  preservation of native IDs, both orders, flags, ExternalTypes, authored constraints
  and coherent reference/dependency state on success.
- **Evidence:** t001-qualification.md; t001-validation.json; the user's Option A
  instruction. Seven baseline probes, six existing native cases, five Python cases
  and an arc fixture qualify existing mechanisms and runners, not a new implementation.
- **Rejected alternative:** strict notification/model/journal atomicity and additional
  generic native infrastructure. Historical candidate failures remain preserved.
- **Change rule:** requirements/design/tasks/traceability/verification describe Option A.
  T002 must verify the selected native implementation and cannot silently weaken
  preflight/no-op or successful-operation preservation.

## Contract Refinements After Qualification

The MoE preservation and coverage findings remain applicable to preflight/no-op,
success and supported caller recovery. Their stronger every-notification and
post-application no-call guarantees were explicitly superseded by D002 Option A.
Normal intermediate notifications and full caller-transaction abort are documented;
no new atomic-publication guard or operation-local savepoint is claimed.

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
