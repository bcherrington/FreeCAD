---
title: Active specification index
doc_type: reference
artifact_type: reference
status: active
owner: local-developer
last_reviewed: 2026-11-06
---

# Specification Index

The active packages are draft delivery scaffolding. Read their briefs, then
requirements/design/tasks/traceability/verification before implementation.
These numbers replace the former unnumbered directory names; FR/PR/SC aliases
and historical evidence remain within each package.

| ID | Package | State | Next safe action |
| --- | --- | --- | --- |
| 001 | [CAD thumbnailing](001-cad-file-thumbnailing/spec.md) | Extraction baseline only | T001 — Resolve STL rendering and resource contracts |
| 002 | [Compact rails](002-compact-view-rails-rework/spec.md) | Proposed; no current rails | T001 — Decide rail scope, geometry and persistence contracts |
| 003 | [Performance](003-freecad-performance-optimization/spec.md) | Implemented paths with incomplete acceptance | T002 — Verify included-file restore compatibility on the target revision (Phase 2D; T018 complete) |

No execution dependency is established between these packages. Performance
measurements require an exact chosen source revision and fixture; rail work must
preserve the titlebar baseline. Avoid coupling thumbnailing to compact UI work.

[000 Sketcher diagnosis](000-sketcher-diagnosis-performance/spec.md) is archived
in place; current truth is in its promoted durable docs. See the
[archive index](../history/spec-archive-index.md), [closure log](../history/spec-closure-log.md)
and [migration review](../reviews/spec-format-reconciliation.md). Follow the
[authoring contract](../templates/spec-package/README.md) for new packages.
