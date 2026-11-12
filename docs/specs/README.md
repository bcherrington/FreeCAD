---
title: Active specification index
doc_type: reference
artifact_type: reference
status: active
owner: local-developer
last_reviewed: 2026-11-09
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
| 005 | [Mainstream CAD performance investigation](005-mainstream-cad-performance-investigation/spec.md) | T001 inputs frozen; no profiling results | T002 — Qualify the shared runner, stage attribution and runtime identities |

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

[003 Performance](003-freecad-performance-optimization/spec.md) is retained in place
as history after T011 local Linux closure. Current guidance is the
[promoted reference](../reference/document-open-performance.md) and
[runbook](../runbooks/freecad-performance-optimization.md). Other platforms and
current numeric timing promises remain unqualified. Actual implementation and
recorded cleanup commit state are recorded in the closure log/archive index.

Spec005 investigates mainstream CAD workloads and owns the selected B001 QR and
B011 tree investigations. It does not reopen Spec003, change solver defaults or
activate unrelated B009/B010 work. No execution dependency on 001/002 is established.

[006 Assembly motion suspension](006-assembly-motion-suspension/requirements.md) is retained as local delivery history. Current API is in [the promoted reference](../reference/assembly-motion-suspension.md); source commits f0dd422402/a73012a8cd, Linux qualification and E001/B015/B016 residuals are recorded in verification and closure history.
