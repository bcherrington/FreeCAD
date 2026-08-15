---
title: FreeCAD Performance Investigation Technical Design
doc_type: spec
artifact_type: design
status: draft
owner: local-developer
last_reviewed: 2026-08-14
---

# Technical Design

## Overview

The package is an evidence-and-decision pipeline. Historical profiles are
preserved, reconciled against current source, separated by workflow, and used to
choose one bounded investigation slice. The 2026-08-14 multi-expert review
selects same-duration validation of the DockOverlay zero-radius-blur experiment
as the next investigation slice; it does not approve a production patch.

## Template Authority And Migration Decision

Migration Decision:

- **Package format:** migrate before further implementation
- **Template authority:** Spec Lifecycle Manager bundled fallback templates;
  this repository intentionally has no spec-package template override
- **Reason:** the active package used valid core artifacts but had unparseable
  task IDs, incomplete task-level traceability, implicit decisions, and stale
  verification state
- **Impact:** package-local spec files touched; repository templates unchanged;
  durable docs unchanged in this revision
- **Follow-up:** use current lifecycle markers, numeric task IDs, decision IDs,
  evidence modes, and closure coverage from this revision forward

Canonical Context Decision:

- **Needed:** yes
- **Shape:** separate `canonical-context.md`
- **Reason:** the package resumes old profiler evidence and a historical branch
  experiment whose commit is in current ancestry while its behavior is absent
  from current source
- **Imported or proposed sources:** `research.md` as historical evidence;
  `docs/performance-optimization-findings.md` as durable routing authority;
  current source/Git as implementation truth
- **Non-canonical background:** temporary profiler paths, stale indexed
  Workbench output, and legacy branch state without direct source confirmation

`change-impact.md` is intentionally omitted: this revision changes no FreeCAD
behavior. Durable impact and later promotion targets are recorded in
`requirements.md` and `verification.md`.

## Requirement Coverage

| Requirement | Acceptance Criteria | Design Coverage | Validation Approach |
| --- | --- | --- | --- |
| R1 | AC1.1-AC1.3 | Evidence model and authority order | VG-001, VG-002, expert review |
| R2 | AC2.1-AC2.4 | Controlled capture contract | VG-003 and `quickstart.md` |
| R3 | AC3.1-AC3.4 | Workstream boundary rules | VG-004, VG-005 |
| R4 | AC4.1-AC4.3 | Current-source reconciliation and branch gate | Git/source inspection, VG-004 |
| R5 | AC5.1-AC5.3 | Slice-selection state machine | D001/D002 review, selected task evidence |
| R6 | AC6.1-AC6.4 | Traceability, review, promotion, and cleanup gates | lifecycle tools, MoE review, closure check |

## Correctness Property Coverage

| Property | Design Behavior | Validation Direction | Notes |
| --- | --- | --- | --- |
| CP-001 | Completed tasks require concrete evidence and compatible evidence mode. | Task-state audit plus manual evidence review. | Migration prose alone is insufficient. |
| CP-002 | Every runtime finding is assigned one primary workflow. | Traceability and research section review. | Cross-cutting symbols may be referenced without merging workflows. |
| CP-003 | Authority order puts current source/runtime above historical evidence. | Source/Git reconciliation before a slice starts. | `canonical-context.md` records the order. |
| CP-004 | Slice state permits only one active source-backed implementation target. | Decision log and task marker audit. | Investigations may remain queued in parallel; implementation may not. |

## High-Level Design

### System Architecture

```text
durable routing + current source/Git + historical research
                           |
                           v
                  evidence reconciliation
                           |
                           v
        workflow isolation and controlled measurement
                           |
                           v
            one selected investigation checkpoint
                           |
                  pass / reject / reroute
                           |
                           v
       bounded implementation plan + validation + review
                           |
                           v
             durable promotion and spec closure
```

### Components And Changes

- `canonical-context.md`: records the authority hierarchy and stale-source
  handling.
- `research.md`: retains historical evidence and current-source reconciliation.
- `quickstart.md`: defines the controlled external-capture contract.
- `open-decisions.md`: owns workstream and experiment decisions.
- `tasks.md`: remains the execution index, using lifecycle-compatible IDs.
- `traceability.md`: provides task-first and requirement-first lookup.
- `verification.md`: records package checks, review, residual risk, promotion,
  and closure state.

### Data Models

The investigation uses four lightweight records:

| Record | Required fields |
| --- | --- |
| capture manifest | date, source revision, branch, build class, model, workflow, duration, tool/command, UI state, artifact path |
| finding | observation, interpretation, confidence, workflow, evidence source, residual unknown |
| decision | stable ID, options, owner, needed-by milestone, status, blocking impact |
| task evidence | stable task ID, state, evidence mode, result, affected artifacts, validation or destination |

### Data Flow

1. Direct source/Git inspection establishes current code state.
2. A capture manifest fixes the experimental controls before profiling.
3. External profiling records one workflow only.
4. `research.md` separates observation from interpretation.
5. A task records evidence and updates traceability/verification.
6. A decision either advances one bounded target, rejects it, or routes it.

### Workstream Boundary Rules

| Workstream | Included evidence | Must exclude | Current disposition |
| --- | --- | --- | --- |
| overlay rotation | DockOverlay A/B, `OverlayGraphicsEffect::draw`, blur/image composition | document open and Preferences | selected next investigation slice |
| presentation/rendering | viewer redraw, Coin/Quarter traversal, Qt RHI, GLX/Mesa presentation | overlay claims not reproduced in the capture | queued after overlay decision |
| document open | `RestoreDocFile`, view-provider restore and `setupCoinGeometry` | active rotation conclusions | queued, separately owned within Spec 004 |
| Preferences open | Preferences reproduction, module/page loading, addon side effects | rotation and document-open conclusions | queued pending ownership classification |

## Low-Level Design

### Algorithms And Logic

```text
reconcile_selected_slice(candidate):
    read canonical authority and current source revision
    confirm candidate branch is permitted and source delta is present
    create matched capture manifests
    capture exactly one workflow with external tooling
    compare normalized evidence and record uncertainty
    if evidence identifies a bounded source cause:
        define implementation target, tests, before/after gate, and review
    else:
        reject or route the candidate without weakening acceptance
        select the next investigation workstream through open-decisions.md
```

### Function Signatures And Interfaces

No public FreeCAD API or file format changes are designed in this revision.
The operational interfaces are:

- Git branch and commit identity for source reconciliation;
- Linux `perf` attach capture described in `quickstart.md`;
- package task, decision, traceability, and verification records.

### Error Handling

- If profiling permission is unavailable, record the command failure and do not
  mark the runtime task complete.
- If a capture includes another workflow or mismatched controls, retain it as
  historical/mixed evidence but exclude it from the comparison pair.
- If symbols are too coarse, escalate from `perf` to a bounded rotation-only
  VTune capture; do not mix Preferences activity.
- If historical patch application conflicts with current source, recreate the
  smallest behavior on a permitted source branch and review the new delta.

### Security, Trust, And Access

- Local `/tmp` profiles may expose filenames, paths, loaded modules, or user
  configuration; do not publish them without human review.
- Addon behavior is untrusted contextual evidence and does not establish a
  FreeCAD core defect.
- Profilers execute local processes and may require host permissions; no
  privilege change is authorized by this spec.
- Agent Workbench routing evidence is advisory. Its current snapshot is stale,
  so direct reads govern this revision.

### Migration And Compatibility

- No runtime, API, document-format, preference, or dependency change occurs.
- Legacy task aliases are preserved in `tasks.md` and `traceability.md` while
  active IDs use lifecycle-compatible numeric forms.
- Any later GUI source edit must preserve existing line endings and validate
  behavior on a named source branch before integration.

### Slice Boundary And Residual Architecture

| Design target | In this slice | Out of this slice | Follow-up destination | Blocks closure? |
| --- | --- | --- | --- | --- |
| lifecycle modernization | package artifacts and MoE review | runtime behavior | none | no after validation |
| overlay experiment validation | matched before/after capture and decision | production merge or upstream PR | T026, D002 | yes |
| presentation/rendering attribution | queued task chain | renderer redesign | T013-T015 | yes until completed or routed |
| document-open investigation | separate capture and source narrowing | rotation optimization | T016-T018 | yes until completed or routed |
| Preferences investigation | reproduction and ownership classification | addon or core fix | T019-T021 | yes until completed or routed |
| durable promotion | destination and accepted content | temporary coordination detail | D005 and `verification.md` | yes |

## Validation Strategy

| Validation | Covers | Evidence Location | Residual Risk |
| --- | --- | --- | --- |
| lifecycle lint and task-state audit | R1, R6, CP-001 | `verification.md` evidence log | parser/runtime defects may need separate reporting |
| task/requirement/property traceability lookup | R6, CP-001-CP-004 | `traceability.md`, MCP output | tooling may lag package semantics |
| matched external overlay captures | R2, R4, R5 | `research.md`, `verification.md` | debug/X11 results may not generalize |
| direct source and Git inspection | R1, R4, CP-003 | `research.md`, task evidence | later branch drift requires refresh |
| independent performance and lifecycle review | R1, R5, R6 | `verification.md` | owner acceptance still required for a source patch |

## Downstream Task Guidance

- Read `canonical-context.md`, the selected task row in `traceability.md`, and
  linked decisions before acting.
- The next executable investigation task is T026, the matched overlay
  before/after capture.
- Do not start a source implementation task until T026 and D002 establish a
  bounded target and T023 defines validation.
- Refresh requirements/design downstream artifacts if the selected workstream
  changes.

### Downstream Review Receipt

The 2026-08-14 MoE revision reviewed this design after the requirements update.
The requirement coverage, property coverage, workstream boundaries, selected
overlay slice, source-authority rules, and promotion plan are reflected in the
task and traceability artifacts.

## Operational Considerations

- Historical profiles were captured on a Linux debug build and may exaggerate
  allocator, symbol, or instrumentation cost.
- User configuration, addons, compositor, display server, GPU, and driver can
  materially affect results.
- Capture manifests and repeatable duration are required to reduce, not remove,
  environmental variance.
- Rollback for this documentation revision is the package diff; no runtime
  rollback is needed.

## Open Questions

Authoritative unresolved decisions live in `open-decisions.md`. The immediate
decision after T026 is whether the historical overlay experiment merits a clean
source slice or should be rejected and routed.

## Promotion Plan

| Candidate durable content | Source today | Promotion trigger | Candidate destination |
| --- | --- | --- | --- |
| focused reopening and ownership rule | requirements/design | selected slice is accepted or routed | `docs/performance-optimization-findings.md` |
| controlled profiling workflow | `quickstart.md` | procedure is exercised successfully | developer troubleshooting/runbook document selected by D005 |
| validated overlay or rendering conclusion | `research.md` | matched evidence and review complete | performance or viewport reference selected by D005 |
| document-open or Preferences ownership | research/decision log | owner is confirmed | focused spec, issue, or durable troubleshooting reference |

## Related Artifacts

- Requirements: `requirements.md`
- Canonical Context: `canonical-context.md`
- Tasks: `tasks.md`
- Traceability: `traceability.md`
- Verification: `verification.md`
- Research: `research.md`
- Quickstart: `quickstart.md`
- Open Decisions: `open-decisions.md`
