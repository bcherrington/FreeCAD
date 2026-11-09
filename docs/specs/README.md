---
title: Active specification index
doc_type: reference
artifact_type: reference
status: active
owner: local-developer
last_reviewed: 2026-11-08
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
| 003 | [Performance](003-freecad-performance-optimization/spec.md) | Implemented paths with incomplete acceptance | T011 — Promotion and whole-spec closure audit (T010 and all children complete; Linux-only acceptance) |

No execution dependency is established between these packages. Performance
measurements require an exact chosen source revision and fixture; rail work must
preserve the titlebar baseline. Avoid coupling thumbnailing to compact UI work.

[000 Sketcher diagnosis](000-sketcher-diagnosis-performance/spec.md) is archived
in place; current truth is in its promoted durable docs. See the
[archive index](../history/spec-archive-index.md), [closure log](../history/spec-closure-log.md)
and [migration review](../reviews/spec-format-reconciliation.md). Follow the
[authoring contract](../templates/spec-package/README.md) for new packages.

[004 Sketcher reference reattachment](004-sketcher-external-reference-reattachment/spec.md)
is retained in place as history. Current behavior is in
[the promoted reference](../reference/sketcher-external-reference-reattachment.md);
actual implementation/cleanup commits and qualification limits are recorded in
the closure log and archive index.
