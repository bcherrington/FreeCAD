---
title: Open work reconciliation
doc_type: reference
status: code-derived
owner: local-developer
last_reviewed: 2026-11-06
---

# Open Work Reconciliation

Compared active local specs/designs against source revision
`13d601b9dd6b66026e2d0dbb5c16156f18bdb592` on
`personal/integration-testing`. This is a bounded inventory of current documents,
not an inventory of all TODOs in FreeCAD or a claim that the specs can close.
Documentation baseline before reconciliation: `c8de1c177c5516a9145ecbc758f53c447415413a`.

| Package | Current code | Remaining work |
| --- | --- | --- |
| [Performance](../specs/003-freecad-performance-optimization/verification.md) | Buffered included-file restore, faster element-map parsing/image conversion, deferred/lazy image-plane loading, tree restore batching implemented. | Current tasks T001–T011 track benchmark decisions, partial correctness/lifecycle validation, conditional investigations, final PR gates and promotion; the 37-item checklist is historical. Fresh comparable benchmarks and full visual validation remain open. |
| [Compact rails](../specs/002-compact-view-rails-rework/verification.md) | Old idle layout loop fixed; rails removed from current titlebar. | Decide whether to reintroduce rails, choose geometry/persistence contracts, implement slots/drop feedback and validate rendered viewport stability. |
| [CAD thumbnails](../specs/001-cad-file-thumbnailing/verification.md) | Existing FCStd embedded-preview extraction/fallback only. | STL rendering vertical slice, isolation/time/resource bounds, MIME packaging and desktop checks; STEP/IGES later. |
| [Standalone console / Report View](../design/python-console-standalone-window.md) | Shared-widget standalone hosts, docking actions and preferences implemented for both panels. | Real window-manager close, restart/focus/I/O/overlay/workbench validation and Linux/Windows/macOS multi-display matrix. Offscreen smoke produced no result. |

Recorded 2026-11-04 focused validation: 24 App tests, four Reader tests, four bitmap tests
and 16 compact-titlebar Qt entries passed (the latter includes fixture hooks).
Three included-file save/reopen fixture cases and five thumbnail-helper cases
also passed. These used existing debug binaries; no fresh full build, complete
CTest run, current performance benchmark or cross-platform GUI acceptance was
performed. See package evidence for commands and limits.

The three specs remain draft. On 2026-11-06 they migrated to numbered packages
with canonical requirements, design, traceability and evidence-bearing tasks.
See [the active index](../specs/README.md) and
[migration review](../reviews/spec-format-reconciliation.md). Format conformance
does not establish product acceptance or closure. No deleted specs were recreated or
inferred closed. No implementation source changed during this reconciliation.

Tool limitations: Agent Workbench reported a stale snapshot and could not plan
validation through the nested docs repository boundary; Spec Lifecycle Manager
provided structural checks rather than source acceptance. Direct source reads,
Git evidence and executed tests underpin the dispositions above.
