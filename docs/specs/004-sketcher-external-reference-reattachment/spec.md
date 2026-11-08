---
title: Sketcher external reference reattachment
doc_type: spec
artifact_type: brief
status: archived
owner: local-developer
last_reviewed: 2026-11-08
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

Local native/Python implementation and full lifecycle qualification are complete.
D001/D002 are implemented. The earlier centre-Coincident false success is resolved
by independently reviewed solver correction 42144bc1e9. All 163 native tests pass;
16 public cases pass under each of three solvers; full TestSketcherApp runs 113
cases with one existing skip. The whole-project run has two separately owned
failures (B012/B013) and is not reported green.

Current behavior is promoted to
[the durable reference](../../reference/sketcher-external-reference-reattachment.md)
and generated help. See [T004 qualification](t004-qualification.md),
[the completion receipt](completion-validation.json), and
[the private delivery review](../../reviews/sketcher-reattachment-delivery-review.md).
This package is retained in place as history; T006 is reconciling the actual
archive commit metadata. The current API contract belongs in the durable reference.
B014 owns separate-worktree/human/platform upstream preparation. The shared
checkout and unrelated work remain preserved. No actual maintainer acceptance,
MCP bridge delivery or released upstream API is claimed.

## Durable Impact

Accepted behavior will be documented in SketchObject.pyi and a dedicated durable
reference under docs/reference/. History indexes will record closure evidence.
This spec is independent of prior crash and performance investigations.
