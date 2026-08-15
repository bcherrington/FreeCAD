---
title: CAD And Mesh File Thumbnailing Requirements
doc_type: spec
status: draft
owner: local-developer
last_reviewed: 2026-08-14
artifact_type: requirements
---

# Requirements

## Context

FreeCAD already ships freedesktop thumbnailer plumbing for `.FCStd` files via:

- `src/XDGData/FreeCAD.thumbnailer.in`
- `src/Tools/freecad-thumbnailer.in`

Current behavior extracts `thumbnails/Thumbnail.png` from `.FCStd` and only supports
that MIME family.

## Problem

Users browsing folders of CAD exchange and mesh files currently see generic file icons
for many supported FreeCAD import formats. Without rendered previews, many workflows
require opening each file to identify correct geometry.

## Goals

- Add OS file-manager thumbnail support for common CAD/mesh formats while keeping `.FCStd`
  behavior unchanged.
- Reuse existing FreeCAD import/render infrastructure where practical.
- Keep thumbnail generation bounded, non-interactive, and suitable for batch requests.
- Keep Linux freedesktop thumbnail integration as the first production target.
- Keep Windows and macOS as follow-up follow-on platform tracks.

## Non-Goals

- Do not change compact UI behavior.
- Do not launch full interactive GUI for every thumbnail request.
- Do not execute user macros, document startup scripts, or addon code in thumbnail flow.
- Do not replace `.FCStd` embedded-thumbnail extraction.
- Do not promise exact visual parity with interactive viewport rendering.

## Candidate MIME Types

- `application/x-extension-fcstd`
- `model/stl`
- `model/step`
- `model/iges`
- `model/obj`
- `model/3mf`
- distribution aliases for `.stp`, `.step`, `.igs`, `.iges`, `.brep`, `.brp`, `.stl`,
  `.obj`, and `.3mf`.

## Durable Source Baseline

- `src/XDGData/FreeCAD.thumbnailer.in` provides freedesktop thumbnailer registration for
  `.FCStd`.
- `src/Tools/freecad-thumbnailer.in` currently extracts embedded thumbnails from `.FCStd`
  archives.
- No existing production thumbnail path exists yet for non-`.FCStd` import/render generation.

### Requirement 1: Preserve `.FCStd` extraction behavior

**ID:** `R1`

**User Story:** As an existing FreeCAD user, I want `.FCStd` thumbnails to stay
fast and unchanged.

- **AC1.1:** GIVEN a valid `.FCStd` file with `thumbnails/Thumbnail.png`, WHEN the
  thumbnail helper runs, THEN it SHALL extract and return the embedded image without
  creating a document/import session.

### Requirement 2: Add CAD/mesh render-based thumbnailing

**ID:** `R2`

**User Story:** As a user browsing CAD and mesh files, I want file-manager
previews for common exchange formats.

- **AC2.1:** GIVEN a supported mesh or CAD file, WHEN the thumbnail request is
  made, THEN the helper SHALL generate a PNG preview via a bounded import/render
  pipeline or fail cleanly with an error.
- **AC2.2:** The implementation SHALL prioritize STL (`*.stl`) as the first render
  MVP.
- **AC2.3:** STEP/STP and IGES/IGS support SHALL follow only after explicit
  validation of headless OpenCascade import/render behavior.

### Requirement 3: Keep thumbnail requests non-interactive

**ID:** `R3`

**User Story:** As a user, I want thumbnail requests to never block or prompt.

- **AC3.1:** The helper SHALL never open GUI windows during normal operation.
- **AC3.2:** The helper SHALL never show dialogs, recovery prompts, or macro prompts.
- **AC3.3:** Thumbnail generation SHALL run under a clean runtime context that does not
  alter workspace-level interactive state.

### Requirement 4: Bounded failure behavior

**ID:** `R4`

**User Story:** As an operator, I want thumbnailing to fail safely under bad input.

- **AC4.1:** Thumbnail generation SHALL enforce a timeout and fail with a non-zero
  exit code when import/render does not complete in time.
- **AC4.2:** Corrupt, partial, or unsupported input SHALL produce a clean failure path.
- **AC4.3:** On failure, no crashes or permanent user-visible state changes SHALL occur.

### Requirement 5: Runtime trust boundaries

**ID:** `R5`

**User Story:** As a maintainer, I want thumbnailing to avoid unsafe code execution.

- **AC5.1:** The helper SHALL not run user macros, autoloaded addon modules, or
  untrusted document code.
- **AC5.2:** The helper SHALL avoid unbounded working-set growth and large-file
  memory collapse.
- **AC5.3:** The helper SHALL tolerate parallel/frequent requests without leaking
  process-local state between runs.

### Requirement 6: Descriptor and format registration

**ID:** `R6`

**User Story:** As a Linux user, I want file-manager MIME plumbing to trigger
FreeCAD thumbnails for the intended file types.

- **AC6.1:** The desktop MIME registration SHALL be explicit and source-of-truth in
  package-controlled descriptors.
- **AC6.2:** `model/stl`, `application/x-extension-fcstd`, and initial format aliases
  SHALL be declared when helper support is present.
- **AC6.3:** Output SHALL be PNG, written to the output path provided by desktop
  thumbnail consumers.

### Requirement 7: User-visible behavior coverage

**ID:** `R7`

**User Story:** As a user, I want predictable file-manager preview behavior for supported
formats.

- **AC7.1:** For valid STL files on a Linux desktop with thumbnailing enabled,
  file managers SHALL display geometry previews without opening files first.
- **AC7.2:** For missing geometry, import failure, or unsupported MIME aliases,
  thumbnailing SHALL fail cleanly or fall back according to freedesktop behavior.
- **AC7.3:** For STEP/IGES flows, previewing SHALL be attempted only with explicit,
  validated runtime support; otherwise clean failure is acceptable until behavior is stable.

## User-Perceived Constraints

- If geometry is missing or malformed, output may be absent or fallback.
- If render is unavailable on the host, the helper shall fail safely.
- If file-manager requests arrive concurrently, runs must remain bounded and
  independently fail-safe.

## Success Criteria

The package succeeds when:

- `.FCStd` thumbnail extraction continues unchanged and fast.
- STL thumbnails are production-planned and implementation-ready on Linux with bounded,
  non-interactive render behavior.
- Safe timeout/memory boundaries and clear fail-clean semantics are defined before rollout.
- MIME descriptors include the migrated legacy `application/x-extension-fcstd` path plus
  supported CAD/mesh aliases.

## Correctness Properties

- `CP-001`: Existing `.FCStd` extraction remains unchanged when requested.
- `CP-002`: Supported non-`.FCStd` thumbnails are produced only through bounded and
  non-interactive helper execution.
- `CP-003`: Thumbnailing failures are clean and non-destructive.
- `CP-004`: Explicit trust boundaries prevent macro/addon execution during thumbnailing.
- `CP-005`: MIME declarations and output format remain explicit and deterministic.
