---
title: Camera-Adaptive Tessellation Verification
doc_type: spec
status: draft
owner: bcherrington
last_reviewed: 2026-08-14
artifact_type: verification
---

# Verification

## Quality Gates

- `cmake --build build/debug --target FreeCADGui PartGui TestSources -j2`
- Run the smallest focused policy/cache/Part GUI tests in one existing FreeCAD process.
- Run selected-file pre-commit hooks while preserving existing GUI-file line endings.
- Run relevant clang-tidy or equivalent static analysis on changed C++ translation units.
- Run lifecycle lint, task audit, traceability audit, and closure check for Spec 010.

## Deterministic Matrix

| Gate | Required witness |
| --- | --- |
| VG-001 Policy | stable camera causes zero remeshes; hysteresis crossings choose expected tiers |
| VG-002 Cache | key reuse, targeted invalidation, eviction, and measured memory accounting |
| VG-003 Budgets/fallback | early abort/cancel/over-budget retain complete last-known-good/native mesh |
| VG-004 Interaction | selection, picking, visibility, navigation, save/reopen, disable/reset invariants |
| VG-005 Static quality | focused build/tests, formatting, pre-commit, static analysis |
| VG-006 Review/closure | independent review, production decision, durable promotion or explicit defer |

## Manual and Runtime Matrix

- Rounded small, medium, and large objects at near/far zoom levels.
- Orthographic and perspective cameras, stationary and continuous navigation.
- Hidden/off-screen objects and representative multi-object user documents.
- Debug versus Release remesh/frame timings and peak cache memory.
- X11 first; Wayland, Windows, and macOS evidence or explicit residual routing.

## Evidence Log

- 2026-08-10: package created from Spec 009's deferred adaptive gate. Spec 009
  retains a relative, scoped manual evaluator; no camera-adaptive production
  behavior is accepted by that package.
- 2026-08-14: downstream review completed against `requirements.md`,
  `design.md`, `tasks.md`, and `canonical-context.md`; no product-scope
  changes were introduced in the review cycle.
- Remaining evidence: Pending.

## Residual Risks

- Mesher cancellation and safe background generation may be infeasible without deeper Part changes.
- Platform-specific render and event-loop behavior is not yet measured.
- Visual benefit and acceptable resource budgets are not yet accepted.

## Closure Criteria

Close only after every requirement/property maps to direct evidence, static
analysis and independent review are recorded, durable documentation is promoted,
and production acceptance or explicit defer/reject preserves native behavior.
