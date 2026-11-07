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
and requires qualification across recompute, abort, undo/redo and save/reopen.

MoE review corrections require exact constraint Orientation, ordered ExternalTypes,
dependency state, preflight/no-op nonmutation and caller-owned recovery. The user
accepted D002 Option A: intermediate notifications are allowed during application;
unexpected apply errors require transaction abort or checkpoint recovery. Ordinary
calls need no active transaction; unsupported states reject before no-op detection.

## Read Order And Authority

[Requirements](requirements.md) -> [Design](design.md) -> [Tasks](tasks.md) ->
[Traceability](traceability.md) -> [Verification](verification.md).
Use [canonical context](canonical-context.md) for source identities, ownership and
the coordination-handoff boundary. Repository templates are authoritative.
This brief is navigation; it contains no independent acceptance criteria.

## Status and Next Action

Draft for implementation acceptance. T001 qualification is complete: eight native
probes (including injected-failure caller abort), six existing native tests, five
Python tests and one generated arc save/reopen fixture passed. D001 selects
reattachExternal and the user accepted D002 Option A. No local design decision remains.

T002 - Implement native admission and in-place identity preservation is complete.
The additive C++ method, thirteen new direct tests and all 162 native Sketcher
tests pass; see [T002 implementation](t002-implementation.md) and its receipt.
The next task is T003 - Expose and document the public Python binding.
T004 also owns a centre-Coincident solver movement issue reproduced with legacy
retargeting on this branch. The public API and full lifecycle remain unqualified;
no operator CAD edits, MCP delivery, runtime replacement or upstream acceptance
are claimed.

Implementation will keep the shared checkout on its current branch. PR preparation
uses a separate worktree after validation; see the
[shared-worktree agreement](canonical-context.md#shared-worktree-execution).

## Durable Impact

Accepted behavior will be documented in SketchObject.pyi and a dedicated durable
reference under docs/reference/. History indexes will record closure evidence.
This spec is independent of prior crash and performance investigations.
