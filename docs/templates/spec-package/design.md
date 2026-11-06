---
title: Feature design title
doc_type: spec
artifact_type: design
status: draft
owner: team-or-person
last_reviewed: YYYY-MM-DD
---

# Technical Design

## Overview

Summarize the proposed design approach and how it satisfies the requirements.

## Requirement Coverage

Map requirements and success criteria to the design elements that satisfy them.

| Requirement | Acceptance Criteria | Design Coverage | Validation Approach |
|-------------|---------------------|-----------------|---------------------|
| Requirement 1 | AC1, AC2 | Component, flow, interface, or decision | Test, review, command, or manual check |

## Correctness Property Coverage

Map requirement properties to concrete design behavior before tasks are drafted.

| Property | Design Behavior | Validation Direction | Notes |
|----------|-----------------|----------------------|-------|
| CP-001 | Component, flow, invariant, interface, or guardrail | Property test, conventional test, command, review, or manual check | |

## High-Level Design

### System Architecture

Describe system-level components, boundaries, and interactions. Include diagrams
where they add clarity.

### Components and Changes

- Component:
  Change required.

### Data Models

Describe schema, storage, or data structure changes. Include entity
relationships and key attributes.

### Data Flow

Describe how data moves through the system for this feature. Source-to-output
lineage, transformation behavior, and integration boundaries.

## Low-Level Design

### Algorithms and Logic

Pseudocode or algorithmic descriptions for complex behavior.

```text
function example(input):
    validate input
    transform data
    return result
```

### Function Signatures and Interfaces

Key interfaces, function signatures, types, or contracts introduced.

```text
interface Example {
    method(param: Type): ReturnType
}
```

### Error Handling

How errors propagate, what gets surfaced to users, retry behavior, and failure
modes.

### Security, Trust, and Access

Describe authentication, authorization, workspace permissions, process
execution, network access, credential handling, sandbox assumptions, and
untrusted input boundaries.

### Migration and Compatibility

Describe schema migrations, data migrations, rollout sequencing, backward
compatibility, feature flags, deprecation behavior, or compatibility risks.

### Slice Boundary And Residual Architecture

Use this table when the design includes Must requirements, architectural
targets, migrations, public interfaces, cross-module behavior, or retained
legacy surfaces. It should be updated before closure if implementation proves
the accepted slice is narrower than the design target.

| Design target | In this slice | Out of this slice | Follow-up destination | Blocks closure? |
|---------------|---------------|-------------------|-----------------------|-----------------|
| Architecture, migration, public interface, or cross-module target | Concrete files, tasks, tests, or docs | Deferred or rejected work | backlog, roadmap, issue, follow-up spec, owner, or none | yes/no |

## Validation Strategy

Describe how the implementation will be verified. Map validation to
requirements, tasks, and risk.

| Validation | Covers | Evidence Location | Residual Risk |
|------------|--------|-------------------|---------------|
| Test, command, review, or manual check | Requirement, task, or risk | `verification.md`, task evidence, CI, log, screenshot, or review note | Risk or none |

## Downstream Task Guidance

- Required checkpoints before implementation:
- Properties or acceptance criteria that need explicit task coverage:
- Optional artifacts needed before implementation:
- Downstream review needed if this design changes after tasks are drafted:

## Operational Considerations

Describe rollout, observability, migration, and failure handling considerations.

## Open Questions

- Question

## Related Artifacts

- Requirements:
- Change Impact:
- Tasks:
- Verification:
