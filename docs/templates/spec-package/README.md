---
title: Spec package authoring contract
doc_type: reference
artifact_type: reference
status: active
owner: local-developer
last_reviewed: 2026-11-06
---

# Spec Package Authoring Contract

Use `docs/specs/NNN-lower-kebab-slug/`, allocating the next ID from active and
historical inventory. Do not reuse archived IDs. Root AGENTS.md and deeper
instructions govern execution. These repository templates are authoritative;
package-local additions may clarify scope without changing durable-doc classes.

Create requirements.md, design.md and tasks.md in that order. Add traceability.md
and verification.md when delivery coverage and evidence need separate tracking.
The optional spec.md is a brief; plan.md is legacy history. Research and
quickstart documents are optional. Preserve archived packages as history unless
explicitly reopened; navigation-only link updates may follow renamed packages. New metadata includes artifact_type as well as the fields
in the parent template catalog. Package status stays draft while product
acceptance or material design decisions remain unresolved.

Requirements need stable numbered IDs, user stories, requirement priorities,
testable acceptance criteria, source baselines and durable promotion targets.
Designs need high-level and low-level behavior, failure/compatibility boundaries,
validation and open decisions. Tasks need stable TNNN IDs, requirement/design
links, dependencies, affected files, acceptance and evidence. Use only one
in-progress implementation task normally.

Markers: `[ ]` pending, `[~]` in progress, `[/]` partial, `[>]` routed,
`[-]` deferred/no-op, `[?]` decision required, `[!]` attention, `[x]` complete.
Evidence modes: command, artifact, manual, reasoned, external. Do not emit legacy
markers. Pending evidence cannot complete a task. Completion of a narrow task
cannot establish broad requirement coverage.

Reconcile claims against source and revision-bound verification before resuming.
Record migration decisions, legacy artifact roles and canonical context when
migrating. Keep ready-to-implement, ready-to-validate, ready-to-close and
ready-to-archive separate. Passing structural lint establishes none of them.
Promote lasting behavior/decisions to durable docs and route unfinished work
before closure; preserve actual final and cleanup commit references.
