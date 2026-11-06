---
title: Performance optimization open decisions
doc_type: spec
artifact_type: open-decisions
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Open Decisions

## Purpose

This ledger is authoritative for unresolved decision IDs/status/ownership. Design
and traceability explain their implementation impact. All three decisions remain
open; adding this ledger does not resolve them or approve a product change.

## Decision Log

| ID | Decision Needed | Options | Owner | Needed By | Status |
| --- | --- | --- | --- | --- | --- |
| D001 | Approve comparative benchmark fixtures, endpoints, per-optimization coverage, statistics, variability and improvement/non-regression rules before collecting acceptance samples | Candidate protocol with explicit numeric gates; revise fixtures/protocol before measurement | local-developer | T001 before T020/T004 measurements | open |
| D002 | Decide whether reduced-resolution initial textures merit a separate follow-up without changing current original-resolution semantics | Reject with rationale; route a separately scoped behavior proposal | local-developer | T009 before T011 closure | open |
| D003 | Select candidate source/dependency revisions, binaries, build/platform scope and final evidence-applicability policy | Pin candidate manifest before acceptance work; refresh manifest and rerun affected checks after final patch selection or document reviewed equivalence | local-developer | T019 before T002–T007 validation; reconfirm at T010.1 | open |

## Blocking Impact

| Decision ID | Blocks | Impact If Unresolved |
| --- | --- | --- |
| D001 | Requirement 3 AC1, AC3, AC4; T001, T020, T004 and final performance acceptance | Metrics, endpoints and pass rules are not approved; no current speedup acceptance |
| D002 | T009 and residual-work disposition at T011 | Reduced-resolution exploration remains a separate unimplemented behavior choice |
| D003 | Requirement 3 AC2, AC5; T019, dependent source/acceptance tasks and T010.1 | Candidate/final results cannot be attributed to an approved source/build/platform composition |

## Resolved Decisions

None. Resolve only with owner, date, chosen option, rationale and affected
artifact/task updates. Preserve IDs when promoting lasting decisions.

## Discovery Guardrail

Query spec_open_decisions and compare IDs/statuses with this ledger. Missing or
conflicting discovery is a handoff blocker; inspect this file and design directly
rather than interpreting an empty result as approval. Advisory context readiness
means usable context, not resolved decisions or satisfied task dependencies.

## Related Artifacts

- [Requirements](requirements.md)
- [Design](design.md#open-questions)
- [Tasks](tasks.md)
- [Traceability](traceability.md#open-decision-impact)
- [Verification](verification.md#moe-review-findings)
