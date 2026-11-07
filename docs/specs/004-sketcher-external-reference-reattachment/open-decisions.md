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
| D002 | Establish orientation mapping and atomic application/notification mechanism | Prepare one compatible projection, preserve proven endpoint roles and apply with scoped restoration; reject unsupported/ambiguous cases | local-developer | T001 before T002 | open |

## Blocking Impact

| Decision ID | Blocks | Impact If Unresolved |
| --- | --- | --- |
| D001 | T002, T003 | Public API and native compatibility contract not ready to implement |
| D002 | T002, T004 | Endpoint preservation and operation-local failure guarantee unproven |

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
