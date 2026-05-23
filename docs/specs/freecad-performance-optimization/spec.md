---
title: FreeCAD performance optimization
doc_type: spec
status: draft
owner: local-developer
last_reviewed: 2026-05-22
---

# Feature Specification

## Summary

Improve measured FreeCAD document-open and runtime performance hotspots
identified in local profiling, focusing on document restore embedded-file
payloads and large image-plane decode/conversion costs.

## Problem

Normal working-mode traces show repeated expensive work during document open.
The strongest findings in this package are slow restore of large embedded
document payloads and expensive full-resolution image-plane conversion into
Coin texture data.

The compact UI viewport layout churn found in the same profiling work is owned
by [Compact view rails rework](../compact-view-rails-rework/spec.md), because
the required fix belongs to the compact rail geometry and drag-and-drop
redesign rather than this general performance package.

## Goals

- Reduce document restore time for large embedded payloads without changing
  saved document semantics.
- Reduce image-plane conversion overhead while preserving visual correctness.
- Keep each optimization small, evidence-led, and independently reviewable.
- Preserve existing document and image workflows.

## Non-Goals

- Do not introduce a broad UI rewrite.
- Do not change the FreeCAD document archive format.
- Do not keep temporary tracing code as production behavior unless it is
  intentionally designed and reviewed as maintainable diagnostics.
- Do not implement compact view rail geometry fixes here; those belong in
  [Compact view rails rework](../compact-view-rails-rework/spec.md).
- Do not optimize startup addons, tree batching, or geometry meshing before the
  higher-confidence findings are addressed.

## Requirements

### Functional Requirements

- **FR-001**: Embedded-file restore changes MUST preserve property
  notifications, file permissions, failure behavior, and document
  compatibility.
- **FR-002**: Image conversion changes MUST preserve orientation, color, alpha,
  scale, and texture ownership behavior.
- **FR-003**: Each optimization MUST include before/after validation using a
  comparable workflow and build mode.

### Key Entities

- **Document restore payloads**: Embedded files restored through
  `Base::XMLReader::readFiles()` and property-specific restore hooks.
- **Image planes**: Large raster images loaded and converted into `SoSFImage`
  texture data during document restore and display.
- **Compact rail rework**: The separate feature tracked in
  [Compact view rails rework](../compact-view-rails-rework/spec.md), which owns
  the rail-specific fix for the measured viewport geometry churn.

## Acceptance Criteria

1. **Given** a document with large included image files, **When** it is opened
   after embedded-file restore optimization, **Then** the same files restore
   faster and the document remains compatible.
2. **Given** image planes with large PNG payloads, **When** image conversion is
   optimized, **Then** visual orientation, alpha, color, and scale match the
   pre-change behavior.

## User Scenarios And Testing

### User Story 1 - Faster Embedded Payload Restore (Priority: P1)

As a user opening large FreeCAD documents, I want embedded payload restore to
avoid unnecessary byte-at-a-time work so documents open faster.

**Why this priority**: Restore traces show embedded payloads dominate the
tested document-open time.

**Independent Test**: Re-run restore tracing on the same document and compare
the large included PNG restore entries.

**Acceptance Scenarios**:

1. **Given** a document containing large included files, **When** the restore
   optimization is applied, **Then** included-file restore is faster and the
   restored files remain intact.

### User Story 2 - Faster Image-Plane Conversion (Priority: P2)

As a user opening documents with large image planes, I want large image
textures to load without unnecessary per-pixel overhead.

**Why this priority**: Image conversion is a strong document-open hotspot, but
it is visually sensitive and should follow the lower-risk restore copy work.

**Independent Test**: Re-run image tracing and visually compare image planes
before and after optimization.

**Acceptance Scenarios**:

1. **Given** a document with PNG image planes, **When** image conversion is
   optimized, **Then** conversion time drops and the rendered image planes match
   the previous output.

## Edge Cases

- Multiple documents or views open.
- Corrupt or partial embedded payloads.
- Uncommon `QImage` formats, premultiplied alpha, indexed images, and SVG image
  planes.
- Debug versus RelWithDebInfo timing differences.

## Success Criteria

- **SC-001**: Included-file restore uses bounded chunked copy or an equivalent
  safe mechanism instead of byte-at-a-time copy.
- **SC-002**: Large image conversion has a measured before/after improvement
  without visual regression.
- **SC-003**: All production changes include focused validation evidence in the
  implementation PR notes.

## Related Artifacts

- Compact rail performance follow-up:
  [../compact-view-rails-rework/spec.md](../compact-view-rails-rework/spec.md)
- Plan: [plan.md](plan.md)
- Research: [research.md](research.md)
- Tasks: [tasks.md](tasks.md)
