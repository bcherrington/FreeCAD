---
title: Camera-Adaptive Tessellation Canonical Context
doc_type: spec
artifact_type: canonical-context
status: draft
owner: bcherrington
last_reviewed: 2026-08-14
---

# Canonical Context

## Purpose

This package transitions viewport adaptive tessellation from closed-experiment outcomes
into explicit production contracts only after current source/tests and verified
boundaries are reconciled.

## Authority Hierarchy

- `AGENTS.md` and repository instructions.
- Current implementation files in `src/Mod/Part/Gui/ViewProviderExt.cpp` and
  `src/Gui/View3DInventorViewer.cpp/.h` for rendering behavior.
- Existing tests for runtime truth where relevant; no dedicated adaptive tessellation
  regression tests are yet present.

## Always-Canonical External Sources

| Source | Authority reason | Handling |
| --- | --- | --- |
| `docs/viewport-rendering-improvements.md` | durable experiment outcome and boundary decisions | supporting context; adaptive tessellation is currently deferred |

## Spec-Canonical Working Sources

- `requirements.md`: proposed behavior contract.
- `design.md`: architecture and integration strategy.
- `tasks.md`: implementation and evidence sequencing.
- `traceability.md`: requirement-to-task-to-verification mapping.
- `verification.md`: gate obligations and evidence matrix.

## Imported Sources

| Source | Source path | Status | Canonical scope |
| --- | --- | --- | --- |
| Source behavior | `src/Mod/Part/Gui/ViewProviderExt.cpp` | reviewed | current mesh generation and display behavior |
| Viewer integration | `src/Gui/View3DInventorViewer.cpp` and `src/Gui/View3DInventorViewer.h` | reviewed | camera/render-path integration and overlays |
| Proposed contract | `requirements.md`, `design.md` | draft | spec-local behavior target pending validation |

## Non-Canonical Background Sources

| Source | Reason non-canonical | Handling |
| --- | --- | --- |
| `viewport-rendering-improvements.md` adaptive-tessellation branch | experimental only | never used as current-behavior authority |
| Legacy branch-only notes or discussions | unanchored to this package | only referenced via traceability when they are revalidated |

## Promotion Map

| Spec-local content | Durable destination | Required before closure |
| --- | --- | --- |
| Adaptive policy/cache/fallback behavior | source code and focused tests | only after all gates in verification are met |
| Unsupported-path/experimental decisions | `docs/viewport-rendering-improvements.md` or follow-on design docs | only if behavior changes are accepted |
