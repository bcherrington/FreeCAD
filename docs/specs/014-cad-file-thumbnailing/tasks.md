---
title: CAD And Mesh File Thumbnailing Tasks
doc_type: spec
status: draft
owner: local-developer
last_reviewed: 2026-08-14
artifact_type: tasks
---

# Tasks

## Task Dependency Graph

```text
T001 -> T002 -> T003 -> T004 -> T005 -> T006 -> T007 -> T008 -> T009 -> T010 -> T011
```

- [x] T001 Classify and migrate legacy input into lifecycle requirements.
  - Depends on: none.
  - Requirements: `R1`-`R7`; Properties: `FRR-001`-`FRR-007`.
  - Files: `docs/specs/cad-file-thumbnailing/spec.md` (source), `requirements.md`
  - Acceptance: Legacy requirement narrative is migrated without semantic loss, and
    `spec.md` is recorded as migration input.
  - Evidence mode: command and reasoned.
  - Evidence: `2026-08-14` `spec.md` content migrated into lifecycle sections in
    `requirements.md` and `design.md`; `FR-*` language preserved as lifecycle
    requirements.

- [x] T002 Preserve existing goals, non-goals, and success criteria in design.
  - Depends on: T001.
  - Requirements: `R1`-`R7`; Properties: `FRR-001`-`FRR-007`.
  - Files: `requirements.md`, `design.md`
  - Acceptance: Design documents the same behavior contract with explicit migration
    and runtime safety framing.
  - Evidence mode: command and reasoned.
  - Evidence: `2026-08-14` `design.md` retains legacy goals, constraints, and
    non-interactive behavior commitments, including safety and runtime boundary
    framing.

- [x] T003 Produce traceability and verification scaffolding for implementation-readiness.
  - Depends on: T002.
  - Requirements: `R1`-`R7`; Properties: `FRR-001`-`FRR-007`.
  - Files: `traceability.md`, `verification.md`
  - Acceptance: Each requirement and task maps to at least one verification route and
    durable promotion target.
  - Evidence mode: artifact.
  - Evidence: `2026-08-14` `traceability.md` and `verification.md` created with
    requirement/task/design gates and evidence placeholders.

- [x] T004 Add canonical context for migration ambiguity and external authority.
  - Depends on: T003.
  - Requirements: `R6`; Properties: `FRR-006`-`FRR-007`.
  - Files: `canonical-context.md`
  - Acceptance: legacy `spec.md` role is explicitly classified, and non-canonical/
    always-canonical authority is stated before migration cleanup.
  - Evidence mode: reasoned.
  - Evidence: `2026-08-14` `canonical-context.md` records migration scope,
    authority hierarchy, imported/working sources, and removed-file posture.

- [x] T005 Run lifecycle lint for migrated package and clean diff hygiene.
  - Depends on: T004.
  - Requirements: `R1`-`R7`; Properties: `FRR-001`-`FRR-007`.
  - Files: `verification.md`
  - Acceptance: `spec_runtime.py lint` reports no structural errors for Spec 014 and
    `git diff --check` succeeds for this path set.
  - Evidence mode: command.
  - Evidence: `2026-08-14` lifecycle lint reports were clean (0 errors,
    0 warnings); `git diff --check` passed for 014 scope.

- [x] T006 Remove legacy input file only after successful preservation and checks.
  - Depends on: T005.
  - Requirements: `R1`; Properties: `FRR-001`.
  - Files: `cad-file-thumbnailing/spec.md`, `014-cad-file-thumbnailing/*`
  - Acceptance: no content loss is observed in requirements/design/design artifacts,
    old legacy file is removed, and package now exists in canonical `014` form.
  - Evidence mode: command and reasoned.
  - Evidence: `2026-08-14` `docs/specs/cad-file-thumbnailing/spec.md` was
    deleted after migration artifacts were created.

- [ ] T007 Complete implementation architecture and research decision for thumbnail
  generation path.
  - Depends on: T006.
  - Requirements: `R2`, `R3`, `R4`, `R5`.
  - Files: `requirements.md`, `design.md`.
  - Acceptance: one implementation architecture (single helper flow with constrained
    geometry import/render path) and runtime research decision is selected with
    explicit risk posture.
  - Evidence mode: reasoned, command, and artifact.
  - Evidence: Pending.

- [ ] T008 Implement STL MVP while preserving existing `.FCStd` thumbnail behavior.
  - Depends on: T007.
  - Requirements: `R1`; `R2`; `R3`; `R4`; `R5`.
  - Files: `src/Tools/freecad-thumbnailer.in`, `src/Tools/freecad-thumbnailer.py`
  - Acceptance: valid STL files produce FreeCAD thumbnails in helper flow, while
    `.FCStd` extraction remains the same fast extraction path with unchanged output
    shape.
  - Evidence mode: command, artifact, and manual.
  - Evidence: Pending.

- [ ] T009 Integrate MIME registrations and packaging wiring for supported formats.
  - Depends on: T008.
  - Requirements: `R2`; `R6`; `R7`.
  - Files: `src/XDGData/FreeCAD.thumbnailer.in`.
  - Acceptance: descriptor `MimeType` is updated to cover declared supported formats
    only where runtime support is present, and descriptor contract still points at
    `freecad-thumbnailer`.
  - Evidence mode: command and manual.
  - Evidence: Pending.

- [ ] T010 Add automated and manual safety/performance validation for the MVP path.
  - Depends on: T008, T009.
  - Requirements: `R3`; `R4`; `R5`; `R7`.
  - Files: `tests/` (CLI/integration harness), thumbnail fixtures, desktop
    validation notes in package docs.
  - Acceptance: automation covers unsupported/corrupt input, timeout/failure
    behavior, bounded STL runtime envelope, and a bounded manual file-manager check.
  - Evidence mode: command, manual, and artifact.
  - Evidence: Pending.

- [ ] T011 Execute promotion and closure prep after implementation evidence.
  - Depends on: T010.
  - Requirements: `R1`-`R7`.
  - Files: `requirements.md`, `design.md`, `tasks.md`, `traceability.md`,
    `verification.md`, durable docs designated by implementation outcome.
  - Acceptance: durable promotion targets are updated, closure criteria are mapped,
    and remaining follow-up work is explicitly routed.
  - Evidence mode: artifact and external.
  - Evidence: Pending.

## Notes
Tasks T007-T011 are now the active implementation slice after migration completion.

## Fallback Template Authority Review

- 2026-08-14: Reviewed after removal of the repository template override. The
  bundled Spec Lifecycle Manager templates remain authoritative; this changes
  no task scope, dependencies, acceptance criteria, or evidence requirements.
