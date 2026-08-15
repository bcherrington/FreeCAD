---
title: FreeCAD Performance Investigation Canonical Context
doc_type: spec
artifact_type: canonical-context
status: draft
owner: local-developer
last_reviewed: 2026-08-14
---

# Canonical Context

## Purpose

This package resumes historical profiles and a source experiment across later
branch integration. Explicit authority is needed because Git ancestry alone no
longer describes whether the experimental behavior is present in current
source.

## Authority Hierarchy

The spec-local context is canonical only for this active investigation. It does
not override system, developer, or user instructions; `AGENTS.md`; repository
policy; current source and tests; generated contracts; live runtime evidence;
or human approval points.

## Always-Canonical External Sources

| Source | Authority reason | Handling |
| --- | --- | --- |
| root `AGENTS.md` | Repository build, branch, test, and editing rules. | Read before changing governed paths. |
| `docs/performance-optimization-findings.md` | Durable performance routing and reopening rules. | Keep the selected slice focused and freshly measured. |
| current source, tests, and Git object state | Implementation truth. | Reconcile conflicts before implementation. |
| `docs/viewport-rendering-improvements.md` | Durable renderer experiment outcomes and production boundaries. | Use as supporting context, not proof of Spec 004 behavior. |
| fresh runtime evidence | Actual behavior in the selected environment. | Record controls and residual platform limits. |

## Spec-Canonical Working Sources

| Source | Role | Scope | Notes |
| --- | --- | --- | --- |
| `requirements.md` | accepted investigation intent | Spec 004 | Defines gates, not current FreeCAD behavior. |
| `design.md` | evidence pipeline and slice boundary | Spec 004 | Does not authorize a source patch. |
| `tasks.md` | execution index | Spec 004 | Must be read with traceability and decisions. |
| `open-decisions.md` | decision authority | Spec 004 | Records selected and blocked work. |
| `research.md` | historical evidence authority | recorded 2026-05-24 investigation | Current source/runtime overrides conflicts. |
| `quickstart.md` | proposed repro authority | next controlled capture | Validate before durable promotion. |

## Imported Sources

| Spec path | Source path | Source revision or date | Status | Canonical scope | Promotion target |
| --- | --- | --- | --- | --- | --- |
| `research.md` | legacy `20260524-performance-investigation` package | preserved 2026-08-07 | adapted | historical observations and experiment record | validated findings only |
| `quickstart.md` | legacy capture notes | revised 2026-08-14 | adapted | proposed capture procedure | developer runbook selected by D005 |
| `requirements.md` | `docs/performance-optimization-findings.md` | reviewed 2026-08-14 | summarized | focused reopening and ownership rule | same durable document if clarification is needed |

## Current Reconciliation Receipt

- Historical experiment commit: `3d41e4b3c4f` (`Gui: avoid inactive dock
  overlay effects`).
- The commit is an ancestor of the current `personal/integration-testing`
  checkout as inspected on 2026-08-14.
- Current `src/Gui/OverlayWidgets.cpp` nevertheless calls `qt_blurImage(...)`
  without the experiment's zero-radius guard and lacks its inactive-effect
  behavior.
- Therefore the experiment is historical evidence, not current implementation.
  Any validation slice must recreate or reapply it on a permitted source branch
  and inspect the resulting current-source delta.
- Agent Workbench reported a stale repository snapshot and is used only for
  routing in this revision; direct file, Git, and lifecycle reads govern claims.

## Non-Canonical Background Sources

| Source | Reason non-canonical | Handling |
| --- | --- | --- |
| `/tmp/freecad-prof/**` | Temporary local profiler artifacts may expire and were not re-executed in this revision. | Use summarized findings unless a raw artifact is directly revalidated. |
| legacy package ID/path | Historical navigation only. | Preserve alias; do not treat as active package. |
| branch ancestry without current diff inspection | Merge resolution or later changes can remove behavior. | Inspect current source and exact commit delta. |
| stale Agent Workbench snapshot | Indexed paths differ from the current docs tree. | Use for navigation only and perform direct reads. |
| VTune Preferences-open capture | Mixed with a different workflow and profiler-stop warnings. | Preferences context only, never rotation evidence. |

## Promotion Map

| Spec-local content | Durable destination or route | Required before closure |
| --- | --- | --- |
| focused ownership and reopening decision | `docs/performance-optimization-findings.md` | yes if current durable routing changes |
| exercised capture procedure | developer troubleshooting/runbook destination selected by D005 | yes or explicit discard rationale |
| validated performance conclusion | performance/viewport reference selected by D005 | yes or explicit historical-only disposition |
| unselected workstreams | focused spec, issue, backlog, or explicit rejection | yes |

## Related Artifacts

- Requirements: `requirements.md`
- Design: `design.md`
- Tasks: `tasks.md`
- Traceability: `traceability.md`
- Verification: `verification.md`
- Open Decisions: `open-decisions.md`
