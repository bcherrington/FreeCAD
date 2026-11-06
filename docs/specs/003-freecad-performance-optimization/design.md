---
title: FreeCAD performance optimization technical design
doc_type: spec
artifact_type: design
status: draft
owner: local-developer
last_reviewed: 2026-11-06
---

# Technical Design

## Overview

Reconcile implemented document-open optimizations with the remaining correctness, measurement and
review gates. Keep compact rail geometry in package 002.

## Requirement Coverage

| Requirement | Acceptance Criteria | Design Coverage | Validation Approach |
| --- | --- | --- | --- |
| Requirement 1 | AC1 | Included-file restore compatibility; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 2 | AC1 | Image conversion fidelity; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 3 | AC1 | Comparable optimization evidence; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 4 | AC1 | Element-map restore compatibility; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 5 | AC1, AC2 | Image-plane restore lifecycle; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |
| Requirement 6 | AC1, AC2 | Tree restore status batching; high-level and low-level design below | Task mapping in traceability.md; verification.md gates |

## Correctness Property Coverage

| Property | Design Behavior | Validation Direction | Notes |
| --- | --- | --- | --- |
| CP-001 | Buffered restore preserves payload bytes, registered entry boundaries, permissions and notifications. | T002, T005; existing conventional tests and bounded manual checks | No new test framework |
| CP-002 | Image conversion/lazy loading preserves orientation, RGBA semantics, original source and plane dimensions. | T003, T006; existing conventional tests and bounded manual checks | No new test framework |
| CP-003 | Tree restore batching preserves selection, focus, observer and per-object deletion behavior. | T007; existing conventional tests and bounded manual checks | No new test framework |

## High-Level Design

### System Architecture

Keep archive I/O in App/Base and display work in Gui. Implemented source paths
are buffered property-file copy, element-map token parsing, direct bitmap
conversion, deferred full-resolution image-plane loading and pending tree status
updates. This package reconciles those paths and their remaining acceptance.
Compact rail geometry belongs to package 002; Sketcher diagnosis history remains
in archived package 000.

### Components and Changes

`src/App/PropertyFile.cpp`: bounded binary copy and read/write/close checks.
`src/Base/Reader.cpp`: registered archive-entry restore dispatch.
`src/App/ElementMap.cpp`: string_view dot tokenizer instead of temporary split
vectors. `src/Gui/BitmapFactory.cpp`: direct 32-bit conversion plus Qt fallback.
`src/Gui/ViewProviderImagePlane.cpp`: pending full-resolution texture lifecycle.
`src/Gui/Tree.cpp`: one pending restore status update, flushed after document open.

### Data Models And Flow

Archive entry → Base::Reader → PropertyFileIncluded::RestoreDocFile → temporary
file → read-only permissions and property notification. Image properties defer
while restoring; visible planes load on finishRestoring and hidden planes on
show. Original source bytes and plane dimensions remain unchanged. No document
format, cache schema or public API migration is intended.

## Low-Level Design

### Algorithms And Interfaces

Property restore uses bounded read/gcount/write loops, preserves the existing
non-writable-file early return and brackets changes with aboutToSetValue and
hasSetValue. Base::XMLReader::readFiles dispatches registered ZIP entries.
ElementMap scans dot-separated tokens without allocating a vector per name.
Bitmap conversion handles RGB32, ARGB32, RGBA8888 and RGBX8888 directly, flips
vertically, produces RGBA and finishes SoSFImage editing. Other formats,
including premultiplied alpha, use Qt conversion to RGBA8888 plus a vertical
flip. Do not claim a direct premultiplied scanline path.

ImageFile/XSize/YSize restore updates mark pendingFullResTexture; finishRestoring
loads visible planes and show loads pending hidden planes. Reduced-resolution
placeholders are not implemented. Tree restore batches status scheduling only;
_slotDeleteObject still preserves per-object focus, selection, children and map
side effects. Any bulk-delete bypass needs a separate measured proposal.

### Error Handling And Compatibility

Preserve binary bytes, entry boundaries, destination permissions and observable
filesystem failures. Verify image format fallback, alpha and texture ownership
rather than extrapolating from four conversion tests. Reopen saved documents,
exercise hidden/visible image transitions and preserve tree/observer behavior.
Do not add production tracing or broader caching without separately reviewed
invalidation and diagnostic contracts.

## Validation Strategy

The 2026-11-04 focused results remain imported evidence on the recorded source
revision. T001 chooses comparable benchmark revisions/fixtures; T002/T003/T005/
T006/T007 cover restore, image, parser, lifecycle and tree correctness. T004
records current timings. T010 runs the exact production revision build, focused
checks and full tests before PR evidence is assembled. Historical timings in
legacy-tasks.md/research.md are ranking signals, not current speed guarantees.
Conditional GUI profiling, startup work and lower-resolution behavior are
separate decisions; do not reopen implemented code because old checklists drifted.

## Downstream Task Guidance

Resolve the decisions blocking the selected task, read its requirement criteria
and obtain task context before coding. Preserve existing source changes. Run
focused checks for code changes and broaden according to root AGENTS.md.
Reconcile design/tasks/traceability after changing a decision. No new testing
dependency is proposed; use existing fixtures, conventional tests and bounded
manual checks.

## Operational Considerations

Promote accepted current behavior to `docs/reference/document-open-performance.md` and operational
validation
to the runbook target in requirements.md before closure. These are proposed
destinations, not claims that those documents already exist. Route residual work
to docs/backlog/README.md with local-developer ownership. Keep the active package
until acceptance, reviews, promotion and closure metadata are complete.

## Open Questions

| Decision | Resolution needed | Blocks | Owner |
| --- | --- | --- | --- |
| D001 | Choose reproducible baseline/current revisions, build modes and accessible benchmark fixtures before renewed performance claims. | T001 | local-developer |
| D002 | Decide whether reduced-resolution textures merit separate follow-up; current lazy loading always uses full resolution. | T009 | local-developer |
| D003 | Choose upstream patch revisions and platform validation scope before PR acceptance. | T010 | local-developer |

## Related Artifacts

- [Requirements](requirements.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Migration input](legacy-spec.md)
