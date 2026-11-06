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
- Additional startup, tree optimization and reduced-resolution behavior proposals
  are optional backlog work; preserve the required current tree safety and
  correctness/performance checks in this slice. Geometry meshing is outside scope.

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
- **Next stage:** T001 planning/D001 protocol, then the four-phase execution sequence.
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
2. GIVEN empty and nonempty archive payloads with injected read, write or close failures, WHEN the
   enclosing document restore runs, THEN the approved compatibility policy SHALL define
   diagnostic/failure-list state, partial destination disposition, property notifications and
   continued restoration of later entries.
3. GIVEN a failed first registered entry followed by a valid second entry, WHEN opening the archive,
   THEN the expected second-entry bytes and document-level failure outcome SHALL be verified rather
   than inferred from a property-hook exception.

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
2. BEFORE conversion or Coin allocation, THE converter SHALL validate non-null input, supported
   positive dimensions representable by the destination type, and checked allocation/offset
   arithmetic; unsupported extents SHALL follow a documented failure result without partial writes
   or memory access outside the destination.
3. GIVEN direct and fallback images at the supported extent boundary and slender images with
   dimensions 32767, 32768 and 65537, WHEN converted, THEN supported cases SHALL retain correct
   pixels and unsupported cases SHALL be rejected safely; null input SHALL have an explicit tested
   outcome.

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
2. BEFORE correctness or measurement results are accepted, THE evidence SHALL identify candidate
   source/dependency revisions, binary/build provenance, platform, toolchain and fixture identity in
   the validation manifest; evidence from existing unpinned binaries SHALL remain historical.
3. BEFORE performance samples are collected for acceptance, THE protocol SHALL define primary
   fixtures/metrics, timing endpoints, run count, statistic, warm-up/exclusion policy, variability
   treatment and improvement/non-regression decision rules; each production optimization SHALL have
   assigned comparative evidence or an explicit reviewed exclusion/routed disposition.
4. WHEN deferred image work is evaluated, THE measurements SHALL distinguish command-mode restore,
   GUI open-to-first-correct-frame, hidden-first-show-to-correct-frame and repeat-show latency,
   initial visibility, pending/materialized texture counts, and peak process/decoded/Coin image
   bytes; deferred work SHALL be reported separately from eliminated work.
5. WHEN source, dependency, build, fixture or relevant environment changes affect evidence
   applicability, THE final review SHALL rerun affected automated, archive, calibration/lifecycle,
   visual/manual and benchmark checks or record a reviewed equivalence justification tied to the
   exact final revisions; assembling old timings alone SHALL NOT satisfy acceptance.
6. WHEN lifecycle decision discovery is queried, THE package SHALL expose all still-open D001, D002
   and D003 decisions with accurate status, owner and blocking tasks; context-packet availability
   SHALL NOT authorize execution over unresolved decisions or task dependencies.

### Requirement 4: Element-map restore compatibility

**User Story:** As a FreeCAD user who needs to restore element maps, I want to preserve mapped-name
and child-ID semantics, so that I can reopen existing geometry archives compatibly.

**Priority:** must-have

**Legacy reference:** plan C1. Reconciled scope from the historical implementation plan.

#### Acceptance Criteria

1. WHEN existing dot-separated element-map entries are restored, THE parser SHALL preserve names,
   child string IDs and map semantics without changing the archive format.
2. WHEN numeric archive tokens are parsed, THE implementation SHALL avoid signed overflow and
   out-of-bounds access and document accepted/rejected behavior for decimal/hexadecimal limits,
   signs, prefixes, whitespace, invalid suffixes and oversized tokens, preserving legacy tolerance
   unless an intentional compatibility change is approved.
3. GIVEN fixtures for semicolon, colon and hashed dollar-prefix forms, decimal child IDs and
   hexadecimal mapped IDs above nine, multiple SIDs/aliases and nested/shared child maps, WHEN
   saved/restored/saved, THEN semantic identity and mapped-to-indexed/indexed-to-mapped lookups
   SHALL remain equivalent after input storage is released.
4. GIVEN a saved CAD document with features referencing dependent subelements, WHEN reopened and
   recomputed after parser changes, THEN those references SHALL retain their intended targets;
   byte-identical serialization SHALL be required only where the format promises deterministic
   ordering.

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
3. GIVEN visible and hidden image planes with stored XSize and YSize independently calibrated away
   from image DPI defaults, WHEN reopened, first shown and saved/reopened again, THEN stored
   dimensions, placement and original texture resolution SHALL remain unchanged; hidden
   copy/paste/import followed by later show SHALL preserve the same calibration.
4. WHEN a pending original texture is materialized, THE texture-load operation SHALL preserve
   existing calibration independently of transient GUI restoring or App importing flags; fresh image
   insertion or explicit user resize SHALL retain its documented sizing behavior.

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
3. WHEN a tree widget is destroyed or recreated while the application remains alive, THEN its
   application subscriptions and queued callbacks SHALL end with receiver lifetime; a later
   document-open signal SHALL NOT access the destroyed widget.
4. GIVEN idle tree timers, WHEN application open, direct document load/restore, import or copy/paste
   enters a suppressed restore path, THEN a matching completion or safely resumed scheduler SHALL
   eventually populate/status the tree and attach observers without another edit or document open;
   completion/error cleanup SHALL NOT strand pending work.

## Correctness Properties

- **CP-001**: Buffered restore preserves payload bytes, registered entry boundaries, permissions and
  notifications, and defines document-level failure outcomes. Covers Requirement 1.
- **CP-002**: Image conversion/lazy loading preserves orientation, RGBA semantics, original source
  and independently calibrated plane dimensions. Covers Requirement 2, Requirement 5.
- **CP-003**: Tree restore batching preserves selection, focus, observer and per-object deletion
  behavior, receiver lifetime and eventual updates for every suppressed restore path. Covers Requirement 6.

- **CP-004**: Image dimensions, allocation lengths and offsets are validated before narrowing or
  writing; rejected input causes no out-of-bounds access. Covers Requirement 2.
- **CP-005**: Numeric parsing is bounded and element-map aliases, child IDs and dependent CAD
  subelement identities survive restore/recompute. Covers Requirement 4.

## Edge Cases

- Multiple documents or views open.
- Corrupt or partial embedded payloads.
- Uncommon `QImage` formats, premultiplied alpha, indexed images, and SVG image
  planes.
- Debug versus RelWithDebInfo timing differences and changed cherry-pick/dependency compositions.
- Null/slender oversized images, arithmetic boundaries and decoder-versus-Coin dimension limits.
- Independently calibrated visible/hidden planes and hidden import/paste followed by first show.
- Tree destruction/recreation, idle direct load/restore/import and failed restore cleanup.
- Empty/nonempty archive-entry failures followed by valid entries and malformed numeric map tokens.

## Technical Context

- Language/dependencies: repository C++/Python toolchain, Qt, Coin/Quarter, App/Base/Gui and
  OpenCascade.
- Storage: existing FCStd archive entries and original image files; no archive-format migration.
- Target: captured Linux Debug baseline; final production revisions need their declared platform
  gates.
- Constraints: preserve stream boundaries, notifications, permissions, image semantics and tree side
  effects.
- Performance: lower comparable restore/conversion timings without correctness regression; exact
  protocol pending D001.

## Success Criteria

- **SC-001**: Included-file restore uses bounded chunked copy or an equivalent
  safe mechanism instead of byte-at-a-time copy.
- **SC-002**: Large image conversion has a measured before/after improvement
  without visual regression.
- **SC-003**: All production changes include focused validation evidence in the
  implementation PR notes, with applicability reconciled against the exact final revisions.
- **SC-004**: All MoE safety findings have regression evidence and no unresolved acceptance blocker;
  current pixel-format tests alone do not establish memory, calibration or receiver-lifetime safety.
- **SC-005**: Each retained production optimization satisfies D001's predeclared improvement and
  non-regression decision rules, including deferred-display latency and material memory effects.

Success criteria remain acceptance gates, not claims of completion. Tentative budgets and
qualitative improvement targets require a recorded decision before validation.

## Related Artifacts

- [Design](design.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Open decisions](open-decisions.md)
- [MoE finding dispositions](verification.md#moe-review-findings)
- [Baseline implementation inventory](baseline-status.md)
- [Optional backlog routes](../../backlog/spec003-follow-ups.md)
- [Historical input](legacy-spec.md)
