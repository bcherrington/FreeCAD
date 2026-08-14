---
title: Document lifecycle
doc_type: reference
status: active
owner: platform
last_reviewed: 2026-05-01
---

# Document Lifecycle

This note defines how to handle documents as they move from active guidance to retained history or removal.

## Active

Use `status: active` when a document is part of the current expected workflow, architecture explanation, contract surface, or operator process.

Examples:

- active runbooks
- current architecture overviews
- current integration guides
- current references

## Draft

Use `status: draft` while a document is being prepared but is not yet the authoritative repository guidance.

## Code-Derived

Use `status: code-derived` when a document has been assembled from implemented
code, repository configuration, and existing documentation and is intended to
describe current behavior, but still needs stakeholder sign-off.

Code-derived docs should:

- identify the implemented code, config, and source docs used as evidence
- avoid unverified business interpretation unless clearly labelled
- be updated when the implementation changes
- move to `status: active` after stakeholder sign-off

## Deprecated

Use `status: deprecated` when a document still has short-term value but should no longer be the preferred path.

The doc should point to its replacement.

## Superseded

Use `status: superseded` when a document has been replaced by a newer document or ADR and is retained only for traceability.

## Archived

Use `status: archived` when a document is kept only as history and should not guide current work.

Archived docs should be retained only when they provide decision context,
audit evidence, or design history that is not already captured in current docs,
ADRs, specs, or checklists.

## Remove Versus Move To History

Move a document to an archived location only when:

- it still explains why a decision was made
- it is useful as an audit trail
- future readers would reasonably ask for the historical context

Remove a document when:

- it duplicates current documentation without adding historical value
- it is a completed cleanup artifact with no ongoing reference value
- it points to obsolete processes and no longer helps explain the current system

## Checklist Guidance

Keep a checklist active only while it still drives live work.

- Active checklist:
  still has open items and is used to coordinate work
- Completed checklist:
  remove it from the active checklist surface
- Historically useful completed checklist:
  either move the remaining useful context into a current doc or retain a
  clearly archived copy with `status: archived`

## Related Docs

- [Documentation Templates](README.md)
