---
title: Feature plan title
doc_type: spec
status: draft
owner: team-or-person
last_reviewed: YYYY-MM-DD
---

# Implementation Plan

## Summary

Summarize the primary requirement and the technical approach.

## Technical Context

- **Language/Version**:
- **Primary Dependencies**:
- **Storage**:
- **Testing**:
- **Target Platform**:
- **Project Type**:
- **Performance Goals**:
- **Constraints**:
- **Scale/Scope**:

## Governance Check

Complete this before implementation and re-check after design changes.

- [ ] SOLID boundaries defined: responsibilities, abstractions, and extension points are explicit for each planned module.
- [ ] DRY plan defined: shared validation, business rules, and contracts are centralized, with any approved duplication documented.
- [ ] Test strategy defined: unit, integration, and contract tests are mapped to changed behavior.
- [ ] UX consistency impact assessed: request, response, and error patterns align with existing conventions or include a documented exception.
- [ ] Performance budgets defined: measurable latency, throughput, or resource targets and verification method are included.

## Project Structure

### Documentation

```text
docs/specs/[###-feature]/
|-- spec.md
|-- plan.md
|-- research.md
|-- design.md
|-- quickstart.md
|-- contracts/
`-- tasks.md
```

### Source Code

Replace this with the real paths touched by the feature.

```text
src/
tests/
docs/
```

**Structure Decision**: Document the selected structure and why it fits the feature.

## Phases

1. Research and clarify open questions.
2. Design contracts, data changes, and operational behavior.
3. Implement independently testable user-story slices.
4. Validate tests, performance budget, rollout, and documentation.

## Dependencies

- Dependency

## Risks

- Risk and mitigation

## Validation Strategy

Describe how the work will be validated before completion.

## Complexity Tracking

Fill this only if the governance check has violations that must be justified.

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| Example | Current need | Why the simpler option is insufficient |
