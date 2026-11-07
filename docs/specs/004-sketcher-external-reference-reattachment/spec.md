---
title: Sketcher external reference reattachment
doc_type: spec
artifact_type: brief
status: draft
owner: local-developer
last_reviewed: 2026-11-07
---

# Feature Brief

Expose a supported Python Sketcher operation to replace one external edge
reference while preserving native geometry identity, link and geometry order,
authored flags and dependent constraint roles.

The proposed first version accepts one negative geometry index and a same-document
source object/Edge selector. It supports compatible line, circle and arc projections,
rejects ambiguous or grouped replacements, participates in caller-owned transactions
and is qualified across recompute, abort, undo/redo and save/reopen.

MoE review corrections require exact constraint Orientation, ordered ExternalTypes,
dependency state, coherent observer publication and complete failure-state checks.
Ordinary calls need no active transaction; noRecomputes/edit mode and unsupported
projection states reject before no-op detection. Native feasibility remains T001.

## Read Order And Authority

[Requirements](requirements.md) -> [Design](design.md) -> [Tasks](tasks.md) ->
[Traceability](traceability.md) -> [Verification](verification.md).
Use [canonical context](canonical-context.md) for source identities, ownership and
the coordination-handoff boundary. Repository templates are authoritative.
This brief is navigation; it contains no independent acceptance criteria.

## Status and Next Action

Draft; source implementation has not started. Six pending tasks cover native
qualification, implementation, binding, lifecycle tests, review and promotion.
D001/D002 need qualification before source implementation.

The next task is T001 - Qualify the API contract and native preservation mechanism,
after the user requests implementation. No operator CAD edits, MCP delivery,
runtime changes or upstream acceptance are claimed.

Implementation will keep the shared checkout on its current branch. PR preparation
uses a separate worktree after validation; see the
[shared-worktree agreement](canonical-context.md#shared-worktree-execution).

## Durable Impact

Accepted behavior will be documented in SketchObject.pyi and a dedicated durable
reference under docs/reference/. History indexes will record closure evidence.
This spec is independent of prior crash and performance investigations.
