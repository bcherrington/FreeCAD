---
title: Mainstream CAD performance investigation verification
doc_type: spec
artifact_type: verification
status: draft
owner: local-developer
last_reviewed: 2026-11-09
---

# Verification

## Scope

This package plans eight investigations and their cost-benefit ranking. At
creation, no fixture, harness, profile, comparison, optimization or new runtime
qualification is complete. Authoring checks validate structure and consistency
only. Actual evidence is attached to T001–T012 as they execute.

## Quality Gates

| Gate | Required? | Status | Evidence |
| --- | --- | --- | --- |
| All six mainstream areas plus B001/B011 explicitly scoped | yes | authoring reviewed; runtime pending | Requirements/design/stream mapping |
| Representative fixtures and collection/resource plan frozen | yes | passed T001; not runtime-qualified | workload-manifest.json; measurement-plan.json; t001-validation.json |
| Source/build/runtime and harness qualified | yes | pending T002 | Planned qualification.md/json |
| Eight streams and independent semantic oracles | yes | pending T003–T010 | Planned per-stream reports/raw manifests |
| Ranked deduplicated eleven-field decisions | yes | pending T011 | Planned opportunities.md/json |
| Expert result review and durable promotion | yes | pending T012 | Reference/runbook/backlog and review records |
| Closure/history and actual commit metadata | yes | pending T012 | Closure log/archive index |

## Investigation Gates

1. **Planning gate — T001:** inspect exact source and candidate fixture rights,
   generated/private copies, model counts, intended edits and independent oracles.
   Freeze fixed attempt counts/order, minimum valid coverage, no-replacement/inconclusive/stop and separate-recovery rules; freeze overhead/cache/host-load/time/disk bounds and D001.
2. **Qualification gate — T002:** verify source patch/dependency/toolchain and
   actual binary/loaded-module identity, process/profile/window ownership, reset,
   correct endpoints including viewport pacing/frame admission/presentation witness, exceptional/timeout cleanup and trace overhead. Qualify actual QR selection/reset or fresh-arm processes and memory boundaries; resolve D002.
3. **Stream gates — T003–T010:** execute the frozen recipe and all required
   ordinary/larger controls and correctness checks. Preserve unsuccessful runs.
   Report raw untraced latency separately from descriptive traced stages; no
   small-sample tail guarantee. Each stream ends with an evidenced finding or
   explicitly bounded limitation and an accountable next evidence action.
4. **Selection gate — T011:** all eight streams accounted for; no shared-stage
   double-counting or presumed removable fraction. Review absolute benefit,
   ordinary-workflow frequency assumptions, dispersion/tails, memory/overlap,
   correctness/maintenance risk and validation effort using the eleven-field
   promoted policy. Route candidate implementations to one owner/destination each.
5. **Delivery gate — T012:** independent relevant experts review actual artifacts,
   promote accepted guidance, reconcile B001/B011 truthfully, and complete actual
   closure metadata. Investigation closure is not optimization delivery.

## Validation Commands

| Command or tool | Purpose | Current result | Evidence |
| --- | --- | --- | --- |
| SLM spec_id_inventory and spec_creation_plan | Allocate next unused ID and revalidate creation fingerprint | 005 selected, ready fingerprint revalidated before atomic directory creation | Authoring tool results; no runtime claim |
| SLM lint_spec_package | Structural template/package check | passed: zero diagnostics | Authoring validation receipt |
| SLM traceability_lookup and task_context | Task/criterion/design/decision routing | passed: all 12 task traceability lookups have zero gaps; T001 context usable | Authoring validation receipt |
| Direct Markdown link/table/ID/dependency/oracle coverage audit | Package semantics and navigation | passed: 32 links and 31 tables; 10 requirements/32 AC; 12 linear pending tasks | Authoring validation receipt |
| git -c core.whitespace=cr-at-eol diff --check | Preserve whitespace and existing GUI line endings | passed root and nested docs | No GUI source edits by authoring |
| pixi run --locked build -j 2 | Build a changed harness/probe target when needed | not run; no source changed | Select affected validation in T002 |
| Scoped CTest/native/Python correctness runs | Actual changed harness/probe and stream behavior | not run | Freeze exact commands in T002/stream receipts |

## Requirement Coverage

All 10 must-have requirements and 32 acceptance criteria are mapped in
[traceability](traceability.md), not yet evidenced. Requirements 2–9 represent
the eight selected streams; Requirement 1 is shared validity, Requirement 10 is
ranking and delivery. A source/harness capability gap does not justify silently
omitting an area. Resolve the gap, or record an explicit owner-reviewed limitation
and revise coverage/scope before claiming investigation completion.

## Correctness Property Coverage

| Property | Covered by | Evidence | Residual risk |
| --- | --- | --- | --- |
| CP-001 | T001/T002 original/copy/reset/process checks | pending | Private fixture rights and ownership |
| CP-002 | Per-operation geometry/reference/solver/joint oracles | pending | Geometry counts/volume alone insufficient |
| CP-003 | T005 rank-sensitive/behavioral equivalence and settings restoration | pending | Numeric changes may reject method |
| CP-004 | Real GUI completion, selected identity and event/lifetime tests | pending | Driver/theme/input scope explicit |
| CP-005 | Identity/overhead/clock/overlap and raw-summary audit | pending | Profile shares are not guaranteed savings |

## Scope Reconciliation Before Closure

| Broad requirement or target | Implemented in this spec | Coverage state | Deferred or rejected work | Destination | Blocks closure? | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| Eight mainstream/B001/B011 investigations | Planning only at creation | not-covered | No selected stream silently deferred | T003–T010; local-developer | yes | Pending exact operation evidence |
| Production changes suggested by results | None | out-of-scope | New algorithms/defaults/cache/bulk-delete/parallelism | T011 selects one owned proposal/spec destination per candidate | no | Investigation does not authorize implementation |
| Durable measurement/reference guidance | Planned | not-covered | No accepted knowledge left only in temporary package | T012; local-developer | yes | Pending promotion |
| Cross-platform and image/startup extensions | None | out-of-scope | Outside the selected local mainstream investigation | Existing B009/B010 or separately scoped future request | no | Scope explicit; no approval inferred |

## Agent Readiness Evidence

| Field | Evidence | Residual risk |
| --- | --- | --- |
| Scope and files | Eight stream contracts, linear task queue, planned evidence files | Runtime/probe paths finalized before source edits |
| Must-read context | canonical-context, requirements, design, tasks, traceability, verification, decisions | Do not execute from task text alone |
| Permissions | Investigation spec requested; owned copies/processes only | No live operator mutation/default change |
| Validation | Registry/protocol, semantic oracles, raw observations and expert review | Authoring checks are not measurements |
| Durable impact | New mainstream reference/runbook and existing B001/B011 entries | Promotion remains future work |
| Provider evidence | Agent Workbench provides navigation/currency only | Nested docs boundaries may block/skip; direct checks govern |

## Task Evidence

At authoring, T001–T012 were pending. T001 now prepares and freezes public
fixtures and collection rules; T002–T012 remain pending. No product source change,
new application build, live GUI session or performance study was performed. Completed historical tasks remain archived and
are not implicitly reopened. Proposed file paths are not claims those artifacts exist.

## Evidence Log

| Date | Evidence | Result | Notes |
| --- | --- | --- | --- |
| 2026-11-09 | User request to create a spec for six mainstream areas plus B011/B001 | Scope authorized | Authoring only; current package records all eight streams |
| 2026-11-09 | ID inventory, creation plan and fingerprint revalidation | 005 selected; directory atomically created | No ID reused, no archived package rewritten |

## Manual Or External Verification

Independent authoring experts inspect CAD/lifecycle, numerical/performance/QA
and documentation/operations boundaries. Authoring reviews found no remaining blocker after these corrections:

- CAD/FreeCAD lifecycle: edit classes distinguish expected topology changes;
  exact sketch drag trajectories/solution branches are frozen; assembly metrics
  require confirmed interfaces or qualified independent checks.
- Performance/numerical/QA: fixed scheduled attempts and no replacement/optional
  stopping; separate recovery studies; QR order/reset/memory controls; honest
  viewport pacing/frame/presentation endpoints and profile-selection rules.
- Documentation/process/operations: exact template/authority sections and task
  routing; CP-003 reaches ranking/closure; unmeasured streams require evidenced
  limitations; T001/T002 track independently failing completion outcomes.

These are independent agent authoring reviews, not actual maintainer approval
or runtime validation. Runtime result review remains mandatory at T012.

## Residual Risks

- Actual representative fixture sizes/settings and stage probes are unknown until T001/T002; local-developer owns D001/D002.
- Historical large-sketch QR shares are not a promise for ordinary edits or sparse superiority.
- Geometry/kernel/assembly/viewer timings may depend strongly on model, cache and runtime.
- Existing shared build outputs may drift; fresh actual identity checks are mandatory.
- Do not impose an enormous full factorial experiment or a separate build pair per stream.

## Durable Promotion And Cleanup

| Spec content | Durable destination or deferral | Status | Evidence |
| --- | --- | --- | --- |
| Accepted workload/stage findings and boundaries | docs/reference/mainstream-cad-performance.md | planned T012 | Pending |
| Safe measurement/reset/oracle procedures | docs/runbooks/mainstream-cad-performance-investigation.md | planned T012 | Pending |
| QR findings | Existing Sketcher diagnostics reference/backlog | planned T005/T012 | Pending |
| Tree findings | B011 plus selected implementation destination | planned T009/T012 | Pending |
| Candidate changes | One owned backlog/spec destination per selected candidate | planned T011 | Pending |
| Audit evidence and actual final commit/cleanup state | Closure log/archive index, retained package by policy | planned T012 | Pending |

### Spec Cleanup Decision

- **Cleanup action:** keep active during investigation; retain as history after promotion under repository convention.
- **Final spec commit:** pending.
- **Closure log path:** docs/history/spec-closure-log.md.
- **Closure log entry updated:** no.
- **Closure cleanup commit:** pending.
- **Active indexes updated:** package registered, not archived.
- **Residual spec-only content:** authored plans; no accepted runtime findings yet.

## Ship Or Closure Risk

- **Risk level:** medium for investigation execution; authoring is documentation-only.
- **Breaking change:** no.
- **Blast radius checked:** runtime validation pending; scope/source boundaries authored.
- **Rollback path:** discard owned fixture/profile outputs and restore settings; preserve failures.
- **Release notes needed:** no for profiling alone.
- **Follow-up issue or spec needed:** selected by T011, not preselected implementations.

## Readiness Decision

- **T001 planning:** complete; see t001-results.md and t001-validation.json.
- **Ready for timed stream execution:** no; D001/D002 and T001/T002 must complete.
- **Ready for promotion:** no.
- **Ready for release:** no.
- **Ready for closure:** no.

## Related Artifacts

- [Requirements](requirements.md)
- [Design](design.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Decisions](open-decisions.md)

## Authoring Validation Receipt

[authoring-validation.json](authoring-validation.json) binds the final authored
package and index/backlog changes, review dispositions and direct/tool checks.
Package lint reports zero diagnostics. Task-state audit emits 12 informational
broad-task hints, with no errors/warnings: per-stream tasks each own one bounded
report, and T001/T002 now have explicit per-outcome completion gates. Those hints
do not establish or waive runtime acceptance. No investigation task is complete.

## T001 Fixture And Collection Freeze

T001 and D001 are complete. [The T001 report](t001-results.md),
[fixture preparation](t001-fixture-preparation.json) and
[validation receipt](t001-validation.json) bind actual canonical bytes and
executed preparation checks, independent reviews and prospective study contracts.
Source-code/rank/correct-frame/runtime/harness qualification remains T002/D002.
No planned row, even a warmup, has executed as a performance study. The initial
authoring-validation.json remains an unchanged historical checkpoint, resolving
its old mutable-document hashes at committed root 877ebdfb3e.
