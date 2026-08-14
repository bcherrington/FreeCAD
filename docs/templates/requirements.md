---
title: Requirements title
doc_type: requirements
status: draft
owner: team-or-person
last_reviewed: YYYY-MM-DD
---

# Title

## Purpose

Describe the capability, policy, or runtime contract this document requires
from the current system.

## Scope

State what is in scope and what is intentionally out of scope.

## Audiences

List the teams or roles that rely on these requirements.

## Current-State Requirements

Use normative language for implemented expectations that should remain true.

| ID | Requirement | Applies To | Source Of Truth | Verification |
| --- | --- | --- | --- | --- |
| REQ-001 | The system must... | Runtime component, MCP surface, language adapter, graph/index behavior, validation path, or config family | Code, config, ADR, schema, design doc, or upstream tool contract | Test, command, query, or review method |

## Configuration Requirements

Document config values that materially affect the requirement.

| Config Source | Key Or Field | Required Behavior | Validation |
| --- | --- | --- | --- |
| Repo config, runtime config, environment variable, CLI flag, plugin metadata, or code constant | Key path | Expected behavior | Schema, test, startup check, or runtime check |

## Operational Requirements

Describe logging, diagnostics, freshness, rollback, support, security,
workspace safety, and validation requirements that apply after implementation.

## Non-Requirements

List behaviors that are intentionally not required, especially if old specs or
legacy docs implied otherwise.

## Evidence

- Code:
- Config:
- Tests:
- Runbooks:
- Technical designs:

## Related Docs

- ADRs:
- Architecture:
- Technical design:
- Runbooks:
