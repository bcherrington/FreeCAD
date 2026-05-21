---
title: CAD and mesh file thumbnailing
doc_type: spec
status: draft
owner: local-developer
last_reviewed: 2026-05-21
---

# CAD And Mesh File Thumbnailing

## Summary

FreeCAD already ships freedesktop thumbnailer plumbing for `.FCStd` files via
`src/XDGData/FreeCAD.thumbnailer.in` and `src/Tools/freecad-thumbnailer.in`.
That thumbnailer extracts the embedded `thumbnails/Thumbnail.png` from saved
FreeCAD project files.

This spec captures a future extension: generate operating-system file-manager
thumbnails for imported CAD and mesh formats such as STEP, IGES, STL, OBJ, and
3MF, without coupling that work to the compact UI branch.

## Problem

Users browsing folders of CAD exchange files currently see generic file icons
for many supported FreeCAD import formats. For design-heavy workflows, visual
file previews make it faster to identify the correct part or assembly before
opening it.

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

## Current State

Existing files:

- `src/XDGData/FreeCAD.thumbnailer.in`
- `src/Tools/freecad-thumbnailer.in`

Current behavior:

- Registers a freedesktop thumbnailer for `application/x-extension-fcstd`.
- Invokes `freecad-thumbnailer -s %s %i %o`.
- Reads the `.FCStd` zip file.
- Writes `thumbnails/Thumbnail.png` when present.
- Falls back to the FreeCAD application icon.

The current implementation is extraction-only; it does not import geometry or
render arbitrary CAD/mesh files.

## Requirements

### Functional Requirements

- **FR-001**: The thumbnailer MUST continue supporting existing `.FCStd`
  thumbnail extraction behavior.
- **FR-002**: The thumbnailer SHOULD support at least STL as the first rendered
  geometry MVP because it is simpler than BREP exchange formats.
- **FR-003**: The design SHOULD support STEP/STP and IGES/IGS after validating
  headless OpenCascade import/render behavior.
- **FR-004**: Thumbnail requests MUST be non-interactive and must not show
  dialogs, recovery prompts, or GUI windows.
- **FR-005**: Thumbnail generation MUST enforce a timeout and fail cleanly with
  a non-zero exit code when import or render fails.
- **FR-006**: Thumbnail generation MUST avoid running user macros, autoloaded
  addon code, or untrusted document code.
- **FR-007**: The thumbnailer MUST write the requested output image path in a
  format accepted by freedesktop thumbnail consumers, normally PNG.
- **FR-008**: MIME registration MUST be explicit and package-controlled.

### Candidate MIME Types

Validate these before implementation:

- `application/x-extension-fcstd`
- `model/stl`
- `model/step`
- `model/iges`
- `model/obj`
- `model/3mf`
- FreeCAD-specific or distribution-specific aliases for `.stp`, `.step`,
  `.igs`, `.iges`, `.brep`, `.brp`, `.stl`, `.obj`, and `.3mf`.

## Proposed Architecture

### Linux Freedesktop Path

Extend or add a thumbnailer descriptor installed to:

```text
${datadir}/thumbnailers/FreeCAD.thumbnailer
```

The descriptor should list supported MIME types once the helper can handle
them:

```ini
[Thumbnailer Entry]
TryExec=freecad-thumbnailer
Exec=freecad-thumbnailer -s %s %i %o
MimeType=application/x-extension-fcstd;model/stl;model/step;model/iges;
```

### Helper Modes

The existing helper can remain extraction-first:

1. If input is `.FCStd`, extract embedded thumbnail as today.
2. If input is a supported geometry file, import into a safe temporary
   document/session.
3. Fit the model to view.
4. Render a square or requested-size PNG.
5. Exit quickly and cleanly.

### Rendering Options To Evaluate

- FreeCAD command-line helper using a minimal GUI/offscreen render path.
- FreeCAD command-line helper using a non-GUI tessellation/render pipeline.
- Format-specific lightweight path for STL first, followed by OpenCascade-based
  STEP/IGES rendering later.

The preferred upstreamable approach is a FreeCAD-owned helper with strict
runtime controls rather than a collection of unrelated external renderers.

## Safety And Performance Constraints

- Thumbnail generation should target sub-second runtime for simple STL files.
- Complex files should be bounded by a timeout, with a tentative default of 5
  seconds per thumbnail request.
- Memory growth must be bounded; large files should fail safely.
- The helper should run with a clean or restricted user configuration where
  possible.
- The helper should avoid loading addons unless explicitly required for a
  supported import path.
- The helper must tolerate corrupt, partial, or hostile input files.

## User Stories

### Story 1 - STL Preview In File Manager (P1)

As a user browsing a directory of STL files, I want the file manager to show
shape thumbnails so I can identify parts without opening each file.

**Acceptance Scenario**:

Given a Linux desktop with FreeCAD installed and thumbnailing enabled, when a
folder containing valid `.stl` files is opened, then the file manager shows PNG
thumbnails generated by FreeCAD for those files.

### Story 2 - STEP And IGES Preview In File Manager (P2)

As a user browsing CAD exchange files, I want STEP and IGES files to show
geometry previews.

**Acceptance Scenario**:

Given valid `.step`, `.stp`, `.iges`, or `.igs` files, when the file manager
requests thumbnails, then FreeCAD generates bounded, non-interactive previews or
fails cleanly without blocking the file manager.

### Story 3 - Existing FCStd Thumbnails Remain Fast (P1)

As an existing FreeCAD user, I want `.FCStd` thumbnails to keep using embedded
document thumbnails so project previews remain fast.

**Acceptance Scenario**:

Given an `.FCStd` file with `thumbnails/Thumbnail.png`, when the thumbnailer is
called, then it extracts that image without importing or rendering the document.

## Edge Cases

- File has no geometry.
- File is larger than the configured maximum.
- Importer fails or crashes.
- Offscreen rendering is unavailable on the host.
- The file manager requests many thumbnails concurrently.
- MIME type is missing or distribution-specific.
- Thumbnail cache contains stale previews after file modification.

## Validation Plan

- Unit-test `.FCStd` extraction remains unchanged.
- Add CLI tests for unsupported/corrupt input.
- Add integration tests for STL thumbnail generation with a small fixture.
- Add manual desktop validation on GNOME Files and KDE Dolphin.
- Measure runtime for a small, medium, and large STL file.
- Prototype STEP/IGES only after STL behavior is stable.

## Open Questions

- Can FreeCAD render imported geometry headlessly in a robust way across X11,
  Wayland, and CI environments?
- Should rendered thumbnailing live in the existing Python helper, a new Python
  helper, or a small compiled executable?
- Which import formats can be safely supported without loading user addons?
- Should package maintainers opt in to geometry thumbnails per platform?
- What timeout and file-size limits are acceptable for default installations?

## Related Artifacts

- Existing Linux thumbnailer descriptor:
  `src/XDGData/FreeCAD.thumbnailer.in`
- Existing `.FCStd` thumbnail helper:
  `src/Tools/freecad-thumbnailer.in`
- Windows existing thumbnail integration:
  `package/WindowsInstaller/thumbnail/FCStdThumbnail.dll`
