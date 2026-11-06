---
title: FreeCAD performance optimization requirements
doc_type: spec
artifact_type: requirements
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Requirements

## Introduction

Reconcile implemented document-open optimizations with the remaining correctness, measurement and
review gates. Keep compact rail geometry in package 002.

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
  [Compact view rails rework](../002-compact-view-rails-rework/spec.md).
- Do not optimize startup addons, tree batching, or geometry meshing before the
  higher-confidence findings are addressed.

## Glossary


- **Document restore payloads**: Embedded files restored through
  `Base::XMLReader::readFiles()` and property-specific restore hooks.
- **Image planes**: Large raster images loaded and converted into `SoSFImage`
  texture data during document restore and display.
- **Compact rail rework**: The separate feature tracked in
  [Compact view rails rework](../002-compact-view-rails-rework/spec.md), which owns
  the rail-specific fix for the measured viewport geometry churn.

## Durable Source Baseline

| Source | Current behavior relied on | Confidence | Notes |
| --- | --- | --- | --- |
| `src/App/PropertyFile.cpp` | No durable document-open performance contract exists. The current source and dated verification define a partial baseline; the archived Sketcher package owns a different workload. | source-inspected | Revision d0644236e41f35a849b12a1e77941aa88354496f; see verification.md. |
| `src/Base/Reader.cpp` | No durable document-open performance contract exists. The current source and dated verification define a partial baseline; the archived Sketcher package owns a different workload. | source-inspected | Revision d0644236e41f35a849b12a1e77941aa88354496f; see verification.md. |
| `src/App/ElementMap.cpp` | No durable document-open performance contract exists. The current source and dated verification define a partial baseline; the archived Sketcher package owns a different workload. | source-inspected | Revision d0644236e41f35a849b12a1e77941aa88354496f; see verification.md. |
| `src/Gui/BitmapFactory.cpp` | No durable document-open performance contract exists. The current source and dated verification define a partial baseline; the archived Sketcher package owns a different workload. | source-inspected | Revision d0644236e41f35a849b12a1e77941aa88354496f; see verification.md. |
| `src/Gui/ViewProviderImagePlane.cpp` | No durable document-open performance contract exists. The current source and dated verification define a partial baseline; the archived Sketcher package owns a different workload. | source-inspected | Revision d0644236e41f35a849b12a1e77941aa88354496f; see verification.md. |
| `src/Gui/Tree.cpp` | No durable document-open performance contract exists. The current source and dated verification define a partial baseline; the archived Sketcher package owns a different workload. | source-inspected | Revision d0644236e41f35a849b12a1e77941aa88354496f; see verification.md. |

## Durable Impact

| Durable area | Action | Target | Notes |
| --- | --- | --- | --- |
| reference/design | add | `docs/reference/document-open-performance.md` | Proposed promotion destination; publish only accepted current behavior. |
| operations | add | `docs/runbooks/freecad-performance-optimization.md` | Validation, recovery and limitations before closure. |
| follow-up | modify | `docs/backlog/README.md` | Route unfinished decisions/work with owner before closure. |

## Staged Readiness

- **Current stage:** reconcile
- **Next stage:** resolve decisions, then resume one scoped task.
- **Design-first exception:** no. Legacy exploration is retained as history.
- **Downstream review needed:** design, tasks, traceability and verification after decisions.
- **Ready to implement:** no blanket readiness; use task context and resolve its blocking decisions.

## Requirements

### Requirement 1: Included-file restore compatibility

**User Story:** As a FreeCAD user who needs to open saved documents, I want to restore included
files with their original bytes and behavior, so that I can retain document compatibility while copy
overhead is reduced.

**Priority:** must-have

**Legacy reference:** FR-001. Embedded-file restore changes MUST preserve property notifications,
file permissions, failure behavior, and document compatibility.

#### Acceptance Criteria

1. GIVEN empty, binary, multi-buffer and multi-entry included payloads, WHEN restored, THEN bytes,
   entry boundaries, notifications and read-only destination behavior SHALL remain compatible;
   read/write/close failures SHALL remain observable.

### Requirement 2: Image conversion fidelity

**User Story:** As a FreeCAD user who needs to display image planes, I want to preserve color,
alpha, orientation, scale and ownership, so that I can trust optimized conversion to display the
same image.

**Priority:** must-have

**Legacy reference:** FR-002. Image conversion changes MUST preserve orientation, color, alpha,
scale, and texture ownership behavior.

#### Acceptance Criteria

1. GIVEN RGB32, ARGB32, RGBA8888, RGBX8888, premultiplied, indexed, grayscale, high-depth and
   SVG-derived inputs, WHEN converted and displayed, THEN orientation, color, alpha, scale and
   texture ownership SHALL match the baseline.

### Requirement 3: Comparable optimization evidence

**User Story:** As a FreeCAD user who needs to review performance changes, I want to receive
comparable before/after evidence, so that I can judge speed improvements separately from
correctness.

**Priority:** must-have

**Legacy reference:** FR-003. Each optimization MUST include before/after validation using a
comparable workflow and build mode.

#### Acceptance Criteria

1. WHEN an optimization is proposed for review, THE evidence SHALL identify baseline/current
   revisions, build mode, fixture, run count, cold/warm policy, before/after timings and focused
   correctness checks.

### Requirement 4: Element-map restore compatibility

**User Story:** As a FreeCAD user who needs to restore element maps, I want to preserve mapped-name
and child-ID semantics, so that I can reopen existing geometry archives compatibly.

**Priority:** must-have

**Legacy reference:** plan C1. Reconciled scope from the historical implementation plan.

#### Acceptance Criteria

1. WHEN existing dot-separated element-map entries are restored, THE parser SHALL preserve names,
   child string IDs and map semantics without changing the archive format.

### Requirement 5: Image-plane restore lifecycle

**User Story:** As a FreeCAD user who needs to restore documents with image planes, I want to load
original full-resolution textures at the correct lifecycle point, so that I can avoid premature
failed loads without changing image fidelity.

**Priority:** must-have

**Legacy reference:** plan F1/F2. Reconciled scope from the historical implementation plan.

#### Acceptance Criteria

1. WHILE restore-time image payloads are unavailable, THE view provider SHALL defer loading.
2. WHEN restore completes for a visible image plane or a hidden plane is shown, THE pending original
   full-resolution texture SHALL materialize while preserving source image and plane dimensions.

### Requirement 6: Tree restore status batching

**User Story:** As a FreeCAD user who needs to open and close documents, I want to batch restore-
time tree status work while preserving per-object effects, so that I can retain selection, focus and
observer behavior.

**Priority:** must-have

**Legacy reference:** plan G1/G2. Reconciled scope from the historical implementation plan.

#### Acceptance Criteria

1. WHILE a document is restoring, THE tree SHALL retain one pending status update and flush it after
   document open.
2. WHEN objects are deleted, THE tree SHALL preserve per-object selection, focus, child and map side
   effects; a bulk-delete bypass requires separate evidence and design.

## Correctness Properties

- **CP-001**: Buffered restore preserves payload bytes, registered entry boundaries, permissions and
  notifications. Covers Requirement 1, Requirement 4.
- **CP-002**: Image conversion/lazy loading preserves orientation, RGBA semantics, original source
  and plane dimensions. Covers Requirement 2, Requirement 5.
- **CP-003**: Tree restore batching preserves selection, focus, observer and per-object deletion
  behavior. Covers Requirement 6.

## Edge Cases

- Multiple documents or views open.
- Corrupt or partial embedded payloads.
- Uncommon `QImage` formats, premultiplied alpha, indexed images, and SVG image
  planes.
- Debug versus RelWithDebInfo timing differences.

## Technical Context

- Language/dependencies: repository C++/Python toolchain, Qt, Coin/Quarter, App/Base/Gui and OpenCascade.
- Storage: existing FCStd archive entries and original image files; no archive-format migration.
- Target: captured Linux Debug baseline; final production revisions need their declared platform gates.
- Constraints: preserve stream boundaries, notifications, permissions, image semantics and tree side effects.
- Performance: lower comparable restore/conversion timings without correctness regression; exact protocol pending D001.

## Success Criteria

- **SC-001**: Included-file restore uses bounded chunked copy or an equivalent
  safe mechanism instead of byte-at-a-time copy.
- **SC-002**: Large image conversion has a measured before/after improvement
  without visual regression.
- **SC-003**: All production changes include focused validation evidence in the
  implementation PR notes.

Success criteria remain acceptance gates, not claims of completion. Tentative budgets and
qualitative improvement targets require a recorded decision before validation.

## Related Artifacts

- [Design](design.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Historical input](legacy-spec.md)
