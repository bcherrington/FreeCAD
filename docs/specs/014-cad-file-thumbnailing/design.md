---
title: CAD And Mesh File Thumbnailing Design
doc_type: spec
status: draft
owner: local-developer
last_reviewed: 2026-08-14
artifact_type: design
---

# Design

## Overview

The package migrates the legacy draft into lifecycle artifacts and preserves its
proposition:

- keep existing `.FCStd` embedded extraction unchanged;
- add a bounded FreeCAD-owned thumbnail path for CAD/mesh imports;
- enforce non-interactive, safe request handling on the thumbnail helper boundary;
- keep Linux freedesktop integration as the initial shipping path.

## Template and Migration Decision

The repository intentionally has no spec-package template override. The Spec
Lifecycle Manager's bundled fallback templates are authoritative, and this
migration follows their current lifecycle artifact set while preserving the
existing `docs/README.md` and local instructions.

- Package format: **migrate before implementation**
- Migration input: `docs/specs/cad-file-thumbnailing/spec.md`
- Scope impact: package-local migration; no repository template override is
  maintained.

## High-Level Design

### Thumbnail Entry and Descriptor

The existing freedesktop descriptor model remains the integration point:

1. Descriptor at `${datadir}/thumbnailers/FreeCAD.thumbnailer` lists supported MIME.
2. Descriptor points to `freecad-thumbnailer -s %s %i %o`.
3. Descriptor `MimeType` is expanded as render support is made safe and stable.

### Helper Behavior Strategy

`src/Tools/freecad-thumbnailer` remains the command surface. The helper can remain
single-binary and switch behavior by input MIME:

1. `.FCStd` path: continue extraction-only of `thumbnails/Thumbnail.png`.
2. Supported non-`.FCStd` path: import into constrained temporary context, fit view,
   render square PNG, and exit.

### Render Path Priorities

- Phase 0: STL render path for simple meshes with low validation complexity.
- Phase 1: STEP/IGES/IGS/STP flow after confirmed headless import/render reliability.
- Future: OBJ/3MF and other aliases where safe and bounded support is proven.

## Low-Level Design

- `descriptor`:
  - Maintain or create a `FreeCAD.thumbnailer` entry under `${datadir}/thumbnailers/`.
  - Keep `Exec=freecad-thumbnailer -s %s %i %o`.
  - Expand `MimeType` only when corresponding format support is implemented and protected.
- `helper`:
  - Identify MIME and short-circuit `.FCStd` extraction path for existing fast path.
  - For renderable formats, create temporary document/session with importer constraints, fit
    to view, and emit a square PNG.
  - Emit explicit error codes for timeout, unsupported MIME, and import/render failure.
- `runtime controls`:
  - Add or reuse configuration for timeout and max-size guard where feasible.
  - Ensure cleanup of temporary artifacts after every request path (success or failure).

## Runtime and Safety Design

### Request Isolation

- Keep helper startup minimal.
- Avoid opening interactive sessions and avoid loading non-essential UI infrastructure.
- Ensure no macro/autoload invocation path is active in thumbnail mode.

### Boundaries and Limits

- Use explicit timeout for request-level processing.
- Bound failure mode with non-zero exit code and readable error signal.
- Keep memory growth bounded; reject oversize or malformed files safely.

### MIME and Output Contract

- Keep output target format as PNG to match freedesktop expectations.
- Keep MIME entries explicit in descriptor only after helper behavior is known-good.

## Alternatives Considered

- External renderer pipeline: rejected for trust boundary and runtime complexity.
- One-off per-format wrappers: rejected while a single FreeCAD-owned helper can cover
  supported set with shared guardrails.
- Fully unbounded geometry import in helper: rejected due safety and user-management concerns.

## Failure And Edge Behavior

- Missing geometry or parse failure: return clean non-zero helper exit and no crash.
- Unsupported MIME: return clean fail, no side effects.
- Descriptor mismatch or host renderer unavailability: fail cleanly and stay safe.
- Concurrent requests: isolated per invocation, no process-wide state coupling.

## Validation And Migration Mapping

Use verification as a control gate before implementation starts:

- Preserve all legacy user stories and functional requirements in lifecycle format.
- Preserve `.FCStd` extraction behavior as `R1`.
- Record task-to-requirement/design-traceability before any implementation work.

## Operational Considerations

- This package currently represents the authoritative spec surface; durable docs
  should only receive promoted behavior once implementation evidence exists.
- Any platform follow-up (Windows/macOS) must be tracked either as explicit follow-up
  requirements or a separate package.
- If platform-specific headless rendering differs, prefer capability-gated enablement.

## Open Questions

- Which minimal runtime context gives reliable headless import on Wayland, X11, and CI?
- Should rendering for STL use a pure mesh path initially versus generic mesh-to-view conversion?
- What timeout and size limits are acceptable by default without false negatives?
- Do we need separate platform-specific feature gating for format support?
