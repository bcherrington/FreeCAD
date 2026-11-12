---
title: Assembly motion suspension requirements
doc_type: spec
artifact_type: requirements
status: archived
owner: local-developer
last_reviewed: 2026-11-12
---

# Requirements

## Introduction

Provide native, introspectable motion suspension for noninteractive assembly construction,
copy, restore and remapping, followed by an explicit validated solve. User authorization:
2026-11-12 request to specify, review with FreeCAD experts, implement on a separate branch
and commit incrementally. External Spec063 handoff is evidence of need, not FreeCAD policy.

## Goals

Support scripts and workbenches independently of MCP. Preserve ordinary behavior outside
suspension and legitimate linked source geometry.

## Non-Goals

No preferences, persisted document flags,
new dependencies, automatic solve on exit, bridge receipts, arbitrary-Python sandbox,
transaction rollback or blanket touched-state restoration.

## Durable Source Baseline

| Source | Current behavior relied on | Confidence | Notes |
| --- | --- | --- | --- |
| `src/Mod/Assembly/App/AssemblyObject.cpp` | Group/status can solve; solve mutates before solver execution | high | Direct source inspection at 3a53ef2ce7 |
| `src/Mod/Assembly/JointObject.py` | Pre-position and angle/rigid callbacks write placements | high | Both preferences are insufficient |
| `src/Mod/Assembly/App/AssemblyLink.cpp` | Flexible copies/remaps and mode normalization | high | Callbacks can precede containment |
| `CONTRIBUTING.md`, `AI_POLICY.md` | Focused compatible change; human review and disclosure | high | No upstream acceptance inferred |
| none found | No durable suspension API reference | low | Promote `reference/assembly-motion-suspension.md` |

## Durable Impact

| Durable area | Action | Target | Notes |
| --- | --- | --- | --- |
| API/reference | add | `reference/assembly-motion-suspension.md` | Lifetime, status, explicit solve and limits |
| API | modify | `src/Mod/Assembly/App/AssemblyObject.pyi` | Explicit validated solve contract |
| history | add | `reviews/assembly-motion-suspension.md` | Private expert findings and disposition |

## Staged Readiness

- **Current stage:** archived local delivery
- **Next stage:** none; B015/B016 own independent follow-ups
- **Ready to design when:** criteria and motion/metadata distinction are explicit (satisfied).
- **Design-first exception:** no
- **Downstream review needed:** design, tasks, traceability, verification after MoE review.

## Requirements

### Requirement 1: Lifetime-safe document scope

**User Story:** As a script author, I want to suspend before creating an assembly so callbacks cannot move components during construction.

**Priority:** must-have

#### Acceptance Criteria

1. GIVEN a live document, WHEN suspension is entered before assembly/Group creation, THE SYSTEM SHALL expose active suspension in native and Python code for that document only.
2. Nested contexts, exceptions and context destruction SHALL restore the previous depth without changing preferences or solving on exit.
3. Concurrent scopes for different documents SHALL be isolated; scope objects are single-use, release is idempotent and garbage collection releases an active handle. Closing a document during a context SHALL be safe; a reopened same-name document SHALL not inherit suspension.

### Requirement 2: No automatic component motion

**User Story:** As a workbench author, I want construction and remapping callbacks to remain safe until references are complete.

**Priority:** must-have

#### Acceptance Criteria

1. WHILE suspended, Group/status/recompute and joint offset, connector, angle and rigid restoration callbacks SHALL not enter native solve preparation/backend execution or write existing component placements (entry into a rejecting native guard is allowed).
2. Joint metadata, schema restoration, structural copy and reference remapping SHALL remain possible. Link placement synchronization and mode conversion SHALL not silently pre-position existing components; populated Rigid property assignments (including same-value assignments) SHALL reject before mutation; empty initialization and document restoration remain allowed. Serialized restore may canonicalize array coordinates while preserving effective world component poses.
3. Simulation/frame and cached undo entrypoints SHALL not bypass suspension. Status inspection SHALL preserve cached diagnostics without a hidden solve.

### Requirement 3: Explicit validated solve

**User Story:** As a caller, I want an intentional solve after completing the graph while automatic callbacks remain suspended.

**Priority:** must-have

#### Acceptance Criteria

1. A separate explicit native/Python solve operation SHALL validate raw joint endpoints, grounding, rigid members and relevant drive targets before any solve preparation or placement write.
2. Escaped, unresolved or unsupported writable targets SHALL reject before motion and preserve source and target poses, touched state, property status and undo history. Local component instances linked to external source geometry SHALL remain supported.
3. Validated solving SHALL be restricted to the requested assembly operation; automatic callbacks remain suspended and scope exit never replays them.

### Requirement 4: Compatibility and evidence

**User Story:** As a FreeCAD maintainer, I want focused regression evidence and an additive API.

**Priority:** must-have

#### Acceptance Criteria

1. Existing ordinary assembly solve and workbench tests SHALL have no new failures outside suspension (E001 in verification.md records two independently reproduced pre-existing view-fixture errors); no serialized flag or preference migration is introduced.
2. Native tests SHALL observe solve/preparation and placement behavior, not rely only on wrapping Python solve; Python tests SHALL exercise the public context and callbacks.
3. Build/test identity, exact commands, expert-review findings and remaining unqualified platforms SHALL be recorded; bridge recovery is independently qualified.

## Correctness Properties

- **CP-001:** Suspension is local to one live document identity and nesting never releases an outer scope.
- **CP-002:** Automatic motion remains disabled from scope entry through exit, including before assembly containment exists.
- **CP-003:** Rejected explicit solve has no source or target pose effects.
- **CP-004:** Outside-scope behavior is unchanged; scope exit performs no deferred motion.

## Technical Context

C++/Python, FreeCAD App and Assembly, existing PyCXX and GTest/unittest; Linux validation,
Windows/macOS CI remains separate. No new dependencies; depth query O(1), graph validation
bounded by graph size. 

## Success Criteria

Focused regressions pass with native observation and the
normal pixi build passes. No claim of a security boundary against arbitrary Python code.
