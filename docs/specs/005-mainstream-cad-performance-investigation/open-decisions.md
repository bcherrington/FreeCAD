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
| D001 | Exact fixtures, mutation/reset/oracles, scales and collection/resource bounds | Prefer generated public fixtures; reuse permitted immutable specimens with protected locators; freeze representative controls and bounded sampling | local-developer | T001 before T002 qualification or accepted stream samples | resolved |
| D002 | Source-confirmed endpoints/QR selection, available probes, overhead and actual runtime isolation | Existing hooks/sampling first; qualify an opt-in bounded adapter where necessary, or document a narrower explicit endpoint before samples | local-developer | T002 before T003–T010 | open |

## Blocking Impact

D001 is resolved on 2026-11-09: T001 selected the actual public canonical
fixture/recipe/oracle registry and fixed collection/resource contract in
[workload-manifest.json](workload-manifest.json) and
[measurement-plan.json](measurement-plan.json). See
[t001-results.md](t001-results.md) and [t001-validation.json](t001-validation.json).
D002 remains open and is owned by T002. No timing study is eligible until actual
Release/runtime/harness/endpoint/oracle qualification and a study-freeze bind the
T001 hashes. A material semantic/fixture/schedule/resource change requires a
prospective new identity/amendment rather than overwriting the v1 freeze.

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

## D001 Resolution

- Owner/date: local-developer, 2026-11-09, under the user instruction to complete T001.
- Choice: 17 generated canonical FCStd files and two STEP files preserved in the
  package, public numeric recipes and independent semantic expectations; 60
  focused cells across all eight streams with 390 fixed scheduled rows.
- Policy: five eligible attempts/pairs, no replenishment; round-robin cell order
  and counterbalanced QR arms; explicit preview, privacy, rank, host and resource gates.
- Rationale: representative ordinary/larger controls and small analytic numerical
  controls permit attribution without private-model dependency or a full factorial
  campaign. Exact actual runtime realization is deliberately qualified by T002.
- Evidence: t001-results.md, t001-fixture-preparation.json and t001-validation.json.
