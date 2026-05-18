---
title: Documentation templates
doc_type: reference
status: active
owner: platform
last_reviewed: 2026-05-01
---

# Documentation Templates

This directory defines the standard document classes for this repository, the template to use for each class, and the minimum metadata expected on new documents.

## Core Rules

1. Every new document should use one primary document class.
2. If a document mixes multiple classes, split it instead of stretching one template too far.
3. Current-state operator guidance belongs in active docs folders such as `getting-started/`, `runbooks/`, `architecture/`, `integrations/`, `api/`, and `reference/`.
4. Historical rationale, superseded material, and design history should be captured in ADRs, current-state docs, active specs, or archived docs only when the context is still useful.
5. Runtime contracts should remain in machine-readable formats where possible, with companion prose only when needed.
6. Generated documents may use a simplified structure when a script owns most of their content, but they should still be clearly identified as generated and linked from the appropriate class folder.

## Required Metadata

Use this frontmatter on new markdown documents unless a template says otherwise or the document is generated:

```yaml
---
title: Short descriptive title
doc_type: guide|runbook|architecture|design|requirements|adr|integration|reference|checklist|history|spec
status: draft|active|code-derived|deprecated|superseded|archived
owner: team-or-person
last_reviewed: YYYY-MM-DD
---
```

Use `status: code-derived` only for documentation generated or assembled from
implemented code, repository config, and existing docs that is intended to
describe current behavior but is still awaiting stakeholder sign-off.

Additional ADR fields:

```yaml
decision_date: YYYY-MM-DD
deciders:
  - name-or-role
supersedes:
superseded_by:
```

## Document Classes

### ADR

Use [adr.md](adr.md) for a durable architectural or technical decision that should remain understandable after implementation details change.

Store ADRs under `docs/adr/` using `NNNN-short-title.md`.

### Generic Document

Use [generic.md](generic.md) when none of the specific document classes below
fit cleanly. Prefer a specific template when the document is an operational
procedure, integration guide, architecture overview, ADR, reference, checklist,
history note, or feature spec.

### Architecture Overview

Use [architecture-overview.md](architecture-overview.md) for stable explanations of system shape, component responsibilities, and key flows.
Store architecture overviews under `docs/architecture/`.

### Requirements

Use [requirements.md](requirements.md) for current-state requirements that
describe implemented or accepted behavior without organizing the content as an
implementation backlog. Requirements should link to source code, config,
schemas, tests, runbooks, technical designs, and ADRs as evidence.
Store requirements under `docs/requirements/`.

### Technical Design

Use [technical-design.md](technical-design.md) for current-state design details
that are more specific than an architecture overview but should not be kept as
a feature implementation spec. Technical designs should explain components,
data/control flow, contracts, configuration, validation, security, and
operations.
Store technical designs under `docs/design/`.

### Getting Started Guide

Use [getting-started-guide.md](getting-started-guide.md) for setup and onboarding material aimed at developers or operators starting a task for the first time.

### Runbook

Use [runbook.md](runbook.md) for operational procedures with clear triggers, steps, validation, and escalation.
Runbooks that change or investigate implementation behavior should include linked docs and config/code touchpoints so the procedure stays traceable to schemas, runtime settings, code paths, and validation evidence.

### Integration Guide

Use [integration-guide.md](integration-guide.md) for MCP surface, language
adapter, parser/LSP, validation tool, plugin, or local runtime integration
behavior, contracts, configuration, and failure handling.

### API Contract Guide

Use [api-contract-guide.md](api-contract-guide.md) for prose that explains how
to read or apply a machine-readable contract such as an MCP tool schema,
resource schema, JSON schema, or API definition.

### Reference

Use [reference.md](reference.md) for factual reference material, assumptions,
limits, naming rules, capability matrices, or runtime dictionaries.

### Checklist

Use [checklist.md](checklist.md) for bounded review or rollout checklists with explicit scope and completion criteria.

### History Note

Use [history-note.md](history-note.md) for retained historical context that should not be mistaken for current guidance.

### Document Lifecycle

Use [document-lifecycle.md](document-lifecycle.md) for rules on when to keep a doc active, move it to history, or remove it.

### Feature Spec Package

Use the [spec-package](spec-package) templates for structured feature work that needs specification, design, planning, and tasks tracked together.

## When To Split A Document

Split a document when it combines:

- current-state requirements and detailed technical design
- accepted decision rationale and current architecture explanation
- integration overview and step-by-step incident handling
- reference data and operational procedure
- history or backlog notes and current operator guidance

## Naming Guidance

- Use kebab-case file names.
- Prefer role-based names such as `system-architecture.md` or `deployment-runbook.md`.
- Use `NNNN-short-title.md` for ADRs.
- Avoid vague names such as `notes.md`, `misc.md`, or `design-v2.md`.
