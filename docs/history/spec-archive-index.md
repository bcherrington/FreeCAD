# Spec Archive Index

## Entries

| Spec ID | Title | Package path | Status | Final spec commit | Cleanup commit | Closure action | Durable destinations | Verification |
|---------|-------|--------------|--------|-------------------|----------------|----------------|----------------------|--------------|
| 015-compact-view-rails-rework | JetBrains-style panel rails and placement Requirements | `docs/specs/compact-view-rails-rework/spec.md` | removed | 838de8f6ea2 | pending | removed | `docs/design/jetbrains-style-compact-ui.md` | `docs/history/spec-closure-log.md` |
| 013-compact-ui-shortcut-preservation | Compact UI Shortcut Preservation Requirements | `docs/specs/013-compact-ui-shortcut-preservation/` | removed | not retained | not retained | removed | `docs/design/jetbrains-style-compact-ui.md` | `docs/history/spec-closure-log.md` |
| 001-freecad-performance-optimization | FreeCAD Performance Optimization Requirements | `docs/specs/freecad-performance-optimization/` | removed | not retained | 6ca5cd9de9e | removed | `docs/performance-optimization-findings.md`; `docs/specs/002-upstream-pr-readiness/`; `docs/specs/004-performance-investigation/` | `docs/history/spec-closure-log.md` |
| 003-splashscreen-startup | Splashscreen Startup Requirements | `docs/specs/003-splashscreen-startup/` | removed | not retained | 6963fb3dcaa | removed | `docs/startup-splash.md` | `docs/history/spec-closure-log.md` |
| 005-async-recompute-branching | Async Recompute Branch Layering Requirements | `docs/specs/005-async-recompute-branching/` | removed | not retained | 28b6dd96b3d | removed | `docs/async-recompute-branching.md`; `src/Doc/AsyncRecompute.dox` | `docs/history/spec-closure-log.md` |
| 008-compact-ui-idle-cpu-loop | Compact UI Idle CPU Loop Requirements | `docs/specs/008-compact-ui-idle-cpu-loop/` | removed | not retained | not retained | removed | `docs/design/jetbrains-style-compact-ui.md` | `docs/history/spec-closure-log.md` |
| 006-gpu-diagnostics-msaa | GPU Diagnostics and Viewport MSAA Reliability Requirements | `docs/specs/006-gpu-diagnostics-msaa/` | removed | not retained | eea1a774ffe | removed | `docs/gpu-diagnostics.md`; `docs/brep-edge-antialiasing.md` | `docs/history/spec-closure-log.md` |
| 007-brep-edge-aa-experiments | BRep Edge Anti-Aliasing Investigation and Experiments Requirements | `docs/specs/007-brep-edge-aa-experiments/` | removed | not retained | eea1a774ffe | removed | `docs/brep-edge-antialiasing.md`; `docs/gpu-diagnostics.md` | `docs/history/spec-closure-log.md` |
| 009-viewport-rendering-improvements | Viewport Rendering Improvements Requirements | `docs/specs/009-viewport-rendering-improvements/` | removed | not retained | eea1a774ffe | removed | `docs/viewport-rendering-improvements.md`; `docs/brep-edge-antialiasing.md`; `docs/gpu-diagnostics.md` | `docs/history/spec-closure-log.md` |
| 012-depth-aware-contrast | Depth-Aware Viewport Contrast Requirements | `docs/specs/012-depth-aware-contrast/` | removed | not retained | eea1a774ffe | removed | `docs/viewport-rendering-improvements.md` | `docs/history/spec-closure-log.md` |

Migration note: `008-compact-ui-idle-cpu-loop` was originally created and
closed as `20260607-compact-ui-idle-cpu-loop` at
`docs/specs/20260607-compact-ui-idle-cpu-loop/`. The legacy identifier and path
remain valid Git-history evidence; no active package was recreated.

The original compact-UI durable guide at `docs/jetbrains-style-compact-ui.md`
was later removed by `068a7c8`. The table follows its current successor at
`docs/design/jetbrains-style-compact-ui.md`; the closure log separately records
the historical path.

## Reserved And Transferred IDs

- `011-display-preset-macros` transferred on 2026-08-14 to the implementation
  repository at
  `/home/bcherrington/Projects/CLion/freecad-appearance-macros/docs/specs/011-display-preset-macros/`.
  This repository reserves the stable ID but does not own an active package.
