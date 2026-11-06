---
title: CAD and mesh file thumbnailing requirements
doc_type: spec
artifact_type: requirements
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Requirements

## Introduction

Extend Linux file-manager previews to imported geometry while retaining the existing FCStd
extraction path. Rendering remains proposed.

## Goals

- Provide OS file-manager thumbnails for common CAD and mesh files supported by
  FreeCAD importers.
- Reuse FreeCAD import/render infrastructure where practical.
- Keep thumbnail generation safe, non-interactive, bounded, and suitable for
  file-manager batch requests.
- Extend the existing freedesktop thumbnailer path on Linux first.
- Keep Windows Explorer and macOS Finder/QuickLook integrations as separate
  platform follow-ups.

## Non-Goals

- Do not change the compact UI implementation.
- Do not require launching the full interactive FreeCAD GUI for each thumbnail.
- Do not execute user macros, document scripts, or workbench startup logic as
  part of thumbnail generation.
- Do not replace `.FCStd` embedded-thumbnail extraction.
- Do not promise perfect visual parity with FreeCAD's full 3D viewport.

## Glossary

FCStd: saved FreeCAD document archive. Geometry thumbnail: a rendered preview rather than an
embedded image.

## Durable Source Baseline

| Source | Current behavior relied on | Confidence | Notes |
| --- | --- | --- | --- |
| `src/Tools/freecad-thumbnailer.in` | No dedicated durable thumbnailing reference exists. Source templates define the extraction-only baseline. | source-inspected | Revision d0644236e41f35a849b12a1e77941aa88354496f; see verification.md. |
| `src/XDGData/FreeCAD.thumbnailer.in` | No dedicated durable thumbnailing reference exists. Source templates define the extraction-only baseline. | source-inspected | Revision d0644236e41f35a849b12a1e77941aa88354496f; see verification.md. |

## Durable Impact

| Durable area | Action | Target | Notes |
| --- | --- | --- | --- |
| reference/design | add | `docs/reference/cad-file-thumbnailing.md` | Proposed promotion destination; publish only accepted current behavior. |
| operations | add | `docs/runbooks/cad-file-thumbnailing.md` | Validation, recovery and limitations before closure. |
| follow-up | modify | `docs/backlog/README.md` | Route unfinished decisions/work with owner before closure. |

## Staged Readiness

- **Current stage:** reconcile
- **Next stage:** resolve decisions, then resume one scoped task.
- **Design-first exception:** no. Legacy exploration is retained as history.
- **Downstream review needed:** design, tasks, traceability and verification after decisions.
- **Ready to implement:** no blanket readiness; use task context and resolve its blocking decisions.

## Requirements

### Requirement 1: FCStd compatibility

**User Story:** As a FreeCAD user who needs to browse saved FreeCAD projects, I want to keep their
embedded previews, so that I can identify projects quickly without geometry import.

**Priority:** must-have

**Legacy reference:** FR-001. The thumbnailer MUST continue supporting existing `.FCStd` thumbnail
extraction behavior.

#### Acceptance Criteria

1. GIVEN an FCStd containing an embedded thumbnail, WHEN requested, THEN the same PNG bytes SHALL be
   extracted without geometry import; a missing image SHALL use the installed fallback.

### Requirement 2: STL preview MVP

**User Story:** As a FreeCAD user who needs to browse STL parts, I want to see shape previews in my
file manager, so that I can choose a part without opening it.

**Priority:** should-have

**Legacy reference:** FR-002. The thumbnailer SHOULD support at least STL as the first rendered
geometry MVP because it is simpler than BREP exchange formats.

#### Acceptance Criteria

1. GIVEN an approved small STL fixture, WHEN a Linux thumbnail is requested, THEN a recognizable PNG
   SHALL be produced non-interactively within the approved budget.

### Requirement 3: STEP and IGES follow-up

**User Story:** As a FreeCAD user who needs to browse exchange models, I want to get bounded STEP
and IGES previews after feasibility validation, so that I can recognize models without a blocked
file manager.

**Priority:** should-have

**Legacy reference:** FR-003. The design SHOULD support STEP/STP and IGES/IGS after validating
headless OpenCascade import/render behavior.

#### Acceptance Criteria

1. GIVEN an approved STEP/STP or IGES/IGS fixture, WHEN requested after the feasibility gate, THEN a
   bounded preview or clean failure SHALL result.

### Requirement 4: Non-interactive execution

**User Story:** As a FreeCAD user who needs to request file previews, I want to keep generation non-
interactive, so that I can continue browsing without dialogs or unexpected windows.

**Priority:** must-have

**Legacy reference:** FR-004. Thumbnail requests MUST be non-interactive and must not show dialogs,
recovery prompts, or GUI windows.

#### Acceptance Criteria

1. WHEN supported, corrupt, empty or unsupported files are requested, THE helper SHALL show no
   windows, dialogs or recovery prompts.

### Requirement 5: Bounded failures

**User Story:** As a FreeCAD user who needs to browse large or damaged models, I want to bound
thumbnail work and report failures, so that I can avoid stalled requests and abandoned temporary
state.

**Priority:** must-have

**Legacy reference:** FR-005. Thumbnail generation MUST enforce a timeout and fail cleanly with a
non-zero exit code when import or render fails.

#### Acceptance Criteria

1. WHEN import/render fails, crashes or exceeds the approved timeout or resource limits, THE helper
   SHALL terminate cleanly with non-zero status and clean temporary state.

### Requirement 6: Restricted execution

**User Story:** As a FreeCAD user who needs to preview files from others, I want to avoid running my
configured macros or addons, so that I can keep preview generation within the reviewed execution
boundary.

**Priority:** must-have

**Legacy reference:** FR-006. Thumbnail generation MUST avoid running user macros, autoloaded addon
code, or untrusted document code.

#### Acceptance Criteria

1. WHEN rendering a geometry file under a profile with macros/addons configured, THE helper SHALL
   avoid their execution using the reviewed isolation mechanism.

### Requirement 7: Output image contract

**User Story:** As a FreeCAD user who needs to use a Linux file manager, I want to receive a
readable PNG at the requested output path, so that I can display the generated preview reliably.

**Priority:** must-have

**Legacy reference:** FR-007. The thumbnailer MUST write the requested output image path in a format
accepted by freedesktop thumbnail consumers, normally PNG.

#### Acceptance Criteria

1. GIVEN a supported input and writable output path, WHEN generation succeeds, THEN the requested
   PNG SHALL be readable by the thumbnail consumer; output failures SHALL report failure.

### Requirement 8: Explicit MIME registration

**User Story:** As a FreeCAD user who needs to install FreeCAD packages, I want to register only
verified thumbnail MIME types, so that I can avoid routing unsupported files to the helper.

**Priority:** must-have

**Legacy reference:** FR-008. MIME registration MUST be explicit and package-controlled.

#### Acceptance Criteria

1. WHEN a format becomes supported, THE installed descriptor SHALL advertise only validated
   package-owned MIME registrations; unsupported formats SHALL remain unregistered.

## Correctness Properties

- **CP-001**: FCStd extraction preserves the embedded/fallback image bytes and does not invoke
  geometry import. Covers Requirement 1.
- **CP-002**: A geometry request cannot escape its approved execution/time/resource boundary or
  leave a successful partial image. Covers Requirement 4, Requirement 5, Requirement 6, Requirement
  7.

## Edge Cases

- File has no geometry.
- File is larger than the configured maximum.
- Importer fails or crashes.
- Offscreen rendering is unavailable on the host.
- The file manager requests many thumbnails concurrently.
- MIME type is missing or distribution-specific.
- Thumbnail cache contains stale previews after file modification.

## Technical Context

- Language: Python for the existing extraction helper; renderer language/interface pending D001.
- Dependencies: Python ZIP/filesystem handling for baseline; FreeCAD importer/render selection pending D001.
- Target: Linux freedesktop integration first; Windows/macOS are follow-ups.
- Constraints: approved time, memory, size and concurrency policy; non-interactive isolated execution.
- Performance: sub-second simple STL and five-second request timeout are tentative, pending D002.

## Success Criteria

- **SC-001**: Preserve FCStd extraction and deliver the approved STL MVP before later formats.
- **SC-002**: Record failure/resource, desktop integration and runtime results against approved
  budgets.

Success criteria remain acceptance gates, not claims of completion. Tentative budgets and
qualitative improvement targets require a recorded decision before validation.

## Related Artifacts

- [Design](design.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Historical input](legacy-spec.md)
