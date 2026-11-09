---
title: Mainstream CAD investigation execution decisions
doc_type: spec
artifact_type: open-decisions
status: draft
owner: local-developer
last_reviewed: 2026-11-09
---

# Open Decisions

## Purpose

This ledger governs execution readiness, not user approval for authoring. The
user selected all eight streams. The owner may resolve routine fixture/protocol
choices from repository evidence without another scope confirmation. A capability
or specimen gap is recorded explicitly; no missing answer authorizes a new solver
or default change.

## Decision Log

| ID | Decision Needed | Options | Owner | Needed By | Status |
| --- | --- | --- | --- | --- | --- |
| D001 | Exact fixtures, mutation/reset/oracles, scales and collection/resource bounds | Prefer generated public fixtures; reuse permitted immutable specimens with protected locators; freeze representative controls and bounded sampling | local-developer | T001 before T002 qualification or accepted stream samples | open |
| D002 | Source-confirmed endpoints/QR selection, available probes, overhead and actual runtime isolation | Existing hooks/sampling first; qualify an opt-in bounded adapter where necessary, or document a narrower explicit endpoint before samples | local-developer | T002 before T003–T010 | open |

## Blocking Impact

T001 is actionable planning, not blocked by its own decision assignment. D001
blocks qualification/collection until T001 records the selected workload, sampling,
cache, host-load, time/disk and correctness contracts. D002 is resolved by T002;
all streams depend transitively on that qualified contract. Neither decision is
assumed resolved because a context packet or package lint is available.

## Resolution Contract

Record owner/date, exact chosen option/rationale, affected manifest/task and
validation evidence. Preserve older frozen plans and attempts separately. A
changed protocol applies prospectively and never backfills missing observations.
Source/probe/build/runtime changes require affected qualification or reviewed
equivalence before evidence reuse.

## Resolved Scope

- User selected six mainstream CAD areas plus B001 and B011 on 2026-11-09.
- Initial proposed execution scope is the local Linux development environment;
  exact settings/build/driver/runtime are frozen by T001/T002. No new other-platform approval.
- Production optimizations/default changes are outside this investigation.
