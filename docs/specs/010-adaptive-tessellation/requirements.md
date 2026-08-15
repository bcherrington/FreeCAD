---
title: Camera-Adaptive Tessellation Requirements
doc_type: spec
status: draft
owner: bcherrington
last_reviewed: 2026-08-13
artifact_type: requirements
---

# Requirements

## Context

Spec 009 proved that FreeCAD Part view providers can be remeshed through
deviation and angular-deflection controls, but it also showed why changing
those properties is not a production adaptive-tessellation system. FreeCAD
currently lacks a camera-driven invalidation policy, bounded mesh generation,
stable cache ownership, and live fallback. This package owns those deeper
contracts; Spec 009 retains only a disabled-by-default manual evaluator.

## Durable Source Baseline

- `src/Mod/Part/Gui/ViewProviderExt.cpp` owns current Part visual remeshing.
- `src/Gui/View3DInventorViewer.*` owns camera/viewer integration.
- `viewport-rendering-improvements.md` records the durable experiment outcome,
  not production authority.
- Durable implementation behavior must be promoted to normal source
  documentation and tests before this package closes.

## Goals

- Improve curved-surface appearance only when screen-space benefit justifies it.
- Bound latency, triangle count, and memory before committing a replacement mesh.
- Avoid repeated remeshing during ordinary camera interaction.
- Preserve stable rendering through cancellation, budget breach, and failure.

## Non-Goals

- Changing BRep geometry, export tessellation, or document serialization.
- Shipping adaptive behavior through Spec 009's manual panel.
- Treating post-mesh measurement as an enforceable resource budget.

## Glossary

- **Native mesh:** mesh produced by the object's captured/default view-provider values.
- **Adaptive mesh:** display-only mesh selected from camera and object state.
- **Last-known-good:** most recent complete mesh accepted within all budgets.
- **Hysteresis:** thresholds that prevent adjacent camera states from remeshing repeatedly.

### Requirement 1: Camera and Screen-Space Policy

**ID:** `R1`

**User Story:** As a modeller, I want curved objects to refine only when their
screen-space size makes the improvement visible.

- **AC1.1:** GIVEN a stable camera and object, WHEN no threshold is crossed, THEN no remesh SHALL occur.
- **AC1.2:** GIVEN zoom or projection changes, WHEN a quality threshold is crossed, THEN policy SHALL choose a deterministic target with hysteresis.
- **AC1.3:** GIVEN hidden or off-screen objects, THEN they SHALL not consume adaptive remesh work.

### Requirement 2: Cache and Invalidation

**ID:** `R2`

- **AC2.1:** Cache identity SHALL include geometry revision, display-relevant state, and tessellation tier.
- **AC2.2:** Geometry or relevant display changes SHALL invalidate stale entries without invalidating unrelated objects.
- **AC2.3:** Cache lifetime and memory ownership SHALL be explicit and observable.

### Requirement 3: Enforceable Budgets

**ID:** `R3`

- **AC3.1:** Triangle, memory, and wall-time budgets SHALL be checked before an adaptive mesh replaces the current mesh.
- **AC3.2:** Work SHALL support early abort or cancellation rather than relying only on post-mesh classification.
- **AC3.3:** Interaction SHALL remain responsive under representative large documents.

### Requirement 4: Stable Fallback

**ID:** `R4`

- **AC4.1:** Failure, cancellation, or budget breach SHALL retain a last-known-good or native mesh.
- **AC4.2:** Partial meshes SHALL never replace a valid displayed mesh.
- **AC4.3:** Disable/reset SHALL restore native behavior without document mutation.

### Requirement 5: Interaction and Compatibility

**ID:** `R5`

- **AC5.1:** Selection, preselection, picking, visibility, and navigation-cube rendering SHALL remain correct.
- **AC5.2:** Save/reopen SHALL not persist session-only adaptive state unless a separately approved preference contract exists.
- **AC5.3:** Unsupported platforms or render paths SHALL report fallback and use native behavior.

### Requirement 6: Evidence and Production Gate

**ID:** `R6`

- **AC6.1:** Deterministic fixtures SHALL prove tier choice, invalidation, cancellation, budget, and fallback relations.
- **AC6.2:** Representative models SHALL record frame latency, remesh time, triangles, memory, and visible benefit.
- **AC6.3:** Production enablement SHALL require static analysis, focused tests, platform evidence, and independent review.

## Correctness Properties

- `CP-001`: An unchanged camera/object state never causes repeated remeshing.
- `CP-002`: No incomplete or over-budget mesh replaces a valid displayed mesh.
- `CP-003`: Cache entries cannot outlive the geometry/display state they represent.
- `CP-004`: Adaptive state never mutates document geometry or saved appearance.
- `CP-005`: Selection, picking, visibility, and navigation remain invariant.
- `CP-006`: Reported limits and fallback state describe actual runtime outcomes.

## Success Criteria

The package succeeds when an opt-in production-path prototype demonstrates a
visible curved-surface benefit on representative models, stays within approved
latency/triangle/memory budgets, does not remesh at rest, preserves interaction
invariants, and falls back safely on every rejected or unsupported path.
