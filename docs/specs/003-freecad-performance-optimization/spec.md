---
title: FreeCAD performance optimization
doc_type: spec
artifact_type: brief
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Feature Brief

Reconcile implemented document-open optimizations with the remaining correctness, measurement and
review gates. Keep compact rail geometry in package 002.

## Read Order And Authority

[Requirements](requirements.md) → [Design](design.md) → [Tasks](tasks.md) →
[Traceability](traceability.md) → [Verification](verification.md).

Canonical acceptance lives in requirements.md; the old FR/PR/SC IDs remain aliases.
The [legacy specification](legacy-spec.md) preserves the original content and
2026-11-04 reconciliation edits as historical input.

## Migration Decision

- Package format: migrate before implementation.
- Template authority: repository templates with package-local additions.
- Reason: resumed packages need current artifact roles, traceable tasks and truthful evidence.
- Impact: existing docs touched yes; templates touched yes; durable index updated.
- Follow-up: execute the next scoped task after its readiness blockers are resolved.

## spec.md Classification

- Role: feature brief.
- Duplicates current artifacts: no.
- Carries unpromoted decisions or acceptance criteria: no; migrated to requirements/design.
- Action: retain as brief; legacy-spec.md is historical migration input.

## Canonical Context Decision

- Needed: yes.
- Shape: [canonical-context.md](canonical-context.md), with summaries in this brief.
- Reason: dated measurements and proposed behavior must be separated from current code.
- Imported sources: legacy-spec.md and existing verification → requirements/design/tasks → migration
  input and dated evidence → promotion targets in requirements.md.
- Always-canonical sources: root AGENTS.md, docs/templates/spec-package/README.md and current source
  for implemented behavior.
- Non-canonical background: historical plan/research/checklists and unavailable temporary logs.

## Status

Draft. See [open decisions](open-decisions.md) and
[readiness](verification.md#readiness-decision).

## MoE Review Follow-Up

The 2026-11-06 specialist review found three static source-safety/calibration/
receiver-lifetime blockers and five acceptance requests plus a lifecycle
interoperability suggestion. All are owned in this package as F001–F009; see
[dispositions](verification.md#moe-review-findings) and the T012–T021 corrective
queue. Requirements, design and task dependencies include the expanded gates.
T021 completed discovery/guardrail repair; T019 must pin candidate evidence before source work.
Updating these documents does not complete the source fixes or runtime tests.
