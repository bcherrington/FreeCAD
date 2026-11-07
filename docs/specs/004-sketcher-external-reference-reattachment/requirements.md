---
title: Sketcher external reference reattachment requirements
doc_type: spec
artifact_type: requirements
status: draft
owner: local-developer
last_reviewed: 2026-11-07
---

# Requirements

## Introduction

Provide a supported Python operation for replacing an existing Sketcher external
edge reference without deleting and recreating its projected geometry. Scripts,
macros and modelling integrations need to retarget a reference while preserving
the sketch's authored constraints and external geometry identity.

The user requested this specification after reviewing the native API handoff
and the local FreeCAD implementation patterns. This package authorizes planning;
it records proposed behavior, not an implemented API or upstream acceptance.

## Goals

- Support a bounded, unambiguous edge-to-edge replacement through native Sketcher.
- Preserve reference identity, ordering, flags and constraint endpoint roles.
- Integrate with caller-owned transactions, recomputation and persistence.
- Expose ordinary FreeCAD Python semantics without an MCP dependency.

## Non-Goals

- Multi-reference batches, face/wire projections, intersections or automatic matching.
- Cross-document object migration, Body transfer, binder migration or GUI commands.
- Rewriting constraints to accommodate incompatible replacement geometry.
- New transport deadlines, cancellation, solver optimization or crash fixes.
- Editing operator models, installing a build or changing the external MCP project.

## Glossary

| Term | Definition |
| --- | --- |
| Link slot | Position of a source object/subelement pair in ExternalGeometry |
| Geometry index | Negative Sketcher constraint reference index; -3 is the first non-axis external geometry |
| Native ID | Persistent identity on an external geometry extension; distinct from either slot or index |
| Ref | Native mapped reference associating a projection with its source link |
| Compatible replacement | One allowed edge producing exactly one supported projection of the same analytic type, with unambiguous endpoint correspondence |
| Authored state | Geometry identity, constraints, expressions, links and persistent user flags; excludes regenerated curve UUIDs and derived caches |

## Durable Source Baseline

Paths below are relative to the FreeCAD root unless stated otherwise. The
[source baseline](canonical-context.md#source-baseline) records inspected identities.

| Source | Current behavior relied on | Confidence | Notes |
| --- | --- | --- | --- |
| src/Mod/Sketcher/App/SketchObjectExternal.cpp | Native attachExternal changes references and rebuilds projections | high, source inspection | Existing grouping and append behavior does not establish this contract |
| src/Mod/Sketcher/App/SketchObjectPyImp.cpp | Thin addExternal/carbonCopy wrappers validate arguments and call native methods | high, source inspection | No attachExternal wrapper found during this assessment |
| src/Mod/Sketcher/App/SketchObject.pyi | Python declarations and method documentation | high, source inspection | CMake generates bindings from this file |
| src/Mod/Sketcher/SketcherTests/TestSketcherSolver.py | External geometry, transaction and undo/redo test patterns | high, source inspection | Proposed cases have not run |
| AGENTS.md; CONTRIBUTING.md | Focused changes, pixi workflows and API compatibility | high, local policy | Reconcile before implementation and public preparation |
| none found | Dedicated durable reattachment contract | documentation gap | Promote accepted behavior to docs/reference/sketcher-external-reference-reattachment.md |

The external coordination log is motivation and historical evidence, not current
runtime authority. Its reports of bridge fixes do not prove native correctness.

## Durable Impact

| Durable area | Action | Target | Notes |
| --- | --- | --- | --- |
| API/contract | modify | src/Mod/Sketcher/App/SketchObject.pyi | Document exact argument, return and error semantics |
| reference/design | add | docs/reference/sketcher-external-reference-reattachment.md | Accepted contract, indexing, supported cases, transaction example and limitations |
| navigation | modify | docs/specs/README.md | Register draft; update disposition at closure |
| history | modify at closure | docs/history/spec-closure-log.md; docs/history/spec-archive-index.md | Record actual evidence and commit identities |

## Staged Readiness

- **Current stage:** design/tasks drafted.
- **Next stage:** T001 contract qualification, then implementation.
- **Ready to design when:** satisfied for the bounded proposed scope below.
- **Design-first exception:** no; requirements precede the proposed design.
- **Optional artifacts recommended:** canonical-context.md, traceability.md, verification.md.
- **Downstream review needed:** resolve D001 and D002 before T002; public API review before delivery.

## Requirements

### Requirement 1: Supported Python entry point

**User Story:** As a script author, I want to reattach an existing external edge
using a documented Sketcher method, so that private property edits are unnecessary.

**Priority:** must-have

#### Acceptance Criteria

1. GIVEN a supported external projection, WHEN the public method receives its
   negative geometry index and an allowed replacement object name and Edge selector,
   THEN it replaces that reference through native Sketcher and returns normally.
2. THE SYSTEM SHALL document index space, supported geometry, return value,
   exceptions, caller-owned transactions and recomputation in the generated API.
3. GIVEN ordinary scripts or macros, THE SYSTEM SHALL provide the operation without
   importing MCP, requiring GUI selection or writing read-only ExternalGeometry from Python.

### Requirement 2: Preserve identity and authored relationships

**User Story:** As a model author, I want reference replacement to retain existing
constraints, so that downstream modelling intent survives a source selector change.

**Priority:** must-have

#### Acceptance Criteria

1. WHEN a compatible replacement succeeds, THEN the selected projected geometry
   retains its native ID, negative index and persistent authored flags; both link
   slot order and projected geometry order remain unchanged. The ordered ExternalTypes
   vector is preserved, including every unaffected Projection, Intersection or Both entry.
2. WHEN replacement succeeds, THEN every constraint retains its type, ordered
   geometry references, point roles, name, value and driving/active state; constraint
   expressions, Constraint.Orientation and unrelated internal/external geometry remain
   unchanged. The initial API does not silently re-encode signed-side semantics.
3. WHEN replacement succeeds and the document is recomputed, THEN its Ref resolves
   to the new source and the projected geometry matches that source in sketch coordinates;
   no stale old reference or Missing state is accepted as success.
4. GIVEN compatible line segments or circular arcs, WHEN their source orientation
   differs, THEN start/end roles retain a proven correspondence or the request is
   rejected; silent endpoint inversion or geometry reordering is prohibited. This
   guarantee must survive a second ordinary recompute and save/reopen, including
   signed point-to-line Distance and Tangent orientation.
5. WHEN the source object changes, THEN native dependency/backlink relationships
   reflect the new link; obsolete dependencies are removed unless another retained
   reference still requires them. Failure and lifecycle restoration preserve the
   corresponding InList/OutList and property dependency state.

### Requirement 3: Fail without partial changes

**User Story:** As an integration author, I want invalid replacements to fail
predictably, so that I can recover without reconstructing the sketch.

**Priority:** must-have

#### Acceptance Criteria

1. IF the index is an axis, internal geometry, out of range or not a supported
   external edge, or the source is absent, disallowed, cyclic, malformed or null,
   THEN the method raises a documented Python exception before persistent mutation.
   Frozen, Missing or Detached projections, Intersection or Both link modes, and
   noRecomputes/edit-mode operation are unsupported and reject before no-op detection.
2. IF a source link yields multiple projections, an intersection, an unsupported
   curve, a different analytic type, a degenerate projection, a duplicate destination
   link or ambiguous endpoint mapping, THEN the method rejects the request without
   expanding it to related geometry or removing constraints.
3. IF projection preparation or application fails, THEN authored state, reference
   resolution and native lookup/solver consistency are restored; unrelated changes
   in the caller's transaction are preserved and no transaction is committed or aborted.
   Document/object/property dirty/touched state, enforced or pending recompute,
   active/booked transaction identity and contents, and undo/redo availability must
   match the pre-call state. Tests include an earlier edit to the same affected property.
4. GIVEN an otherwise admitted projection in a supported execution mode and an exact
   already-attached source/selector, WHEN invoked again, THEN the operation is a no-op:
   it emits no change notification, alters no journal entry and preserves clean state.
5. WHEN any model-change notification is emitted during the operation, THEN a
   read-only observer sees coherent source links, ExternalTypes, projected geometry,
   Ref resolution and native IDs. Precondition failures emit no model-change signals.
   Operation-local restoration cannot be used to retract already published invalid state.

### Requirement 4: Transactions, recompute and persistence

**User Story:** As a FreeCAD user, I want reattachment to behave like other sketch
edits, so that undo and saved models remain reliable.

**Priority:** must-have

#### Acceptance Criteria

1. WHEN called with or without an active document transaction, THEN the method is
   supported without explicitly opening, committing or aborting a top-level transaction,
   saving a file or performing a whole-document recompute; successful edits correctly
   invalidate affected derived state. Normal FreeCAD booked/global transaction
   materialization is allowed on success; failure and no-op preserve pre-call metadata.
2. GIVEN a committed replacement, WHEN undo and redo run with normal recomputation,
   THEN both states restore the correct source, native ID, order, authored flags and
   dependent constraint roles; abort restores the original state as well.
3. GIVEN either the original or reattached state, WHEN saved and reopened, THEN the
   same semantic state is reconstructed and subsequent source edits propagate normally.
4. GIVEN a solvable acceptance fixture and a replacement with a known compatible
   solution, WHEN that replacement is recomputed,
   THEN it remains solvable with expected dependent geometry. General solver conflicts
   remain observable through normal FreeCAD solve/recompute reporting, without claiming
   that reference compatibility proves every sketch solvable.

### Requirement 5: Compatibility and qualification

**User Story:** As a maintainer, I want a small additive change with reproducible
tests, so that existing scripting and model workflows remain stable.

**Priority:** must-have

#### Acceptance Criteria

1. THE SYSTEM SHALL preserve existing addExternal/delExternal and native grouped
   attachment behavior unless a separately reviewed change explicitly requires otherwise;
   it introduces no document format migration, new dependency or default setting.
2. THE DELIVERY SHALL include a self-contained two-reference circle/arc fixture
   with dependent constraints, success and rejection tests, abort/undo/redo,
   save/reopen and unaffected-reference assertions; line endpoint cases are also covered.
3. THE DELIVERY SHALL record focused native/Python checks, pixi build, required broader
   checks and platform limitations against exact source/build identities. Operator CAD
   must not be used for destructive or unknown-outcome experiments.
4. BEFORE closure, accepted behavior SHALL be promoted to the durable targets and
   specialist review findings, deferred scope and actual commit identities recorded.

## Correctness Properties

- **CP-001:** Successful replacement preserves native identity, both orders, ExternalTypes and authored constraint roles/Orientation.
- **CP-002:** Failed replacement preserves authored/dependency state, coherent derived lookups, dirty/touched/recompute state and transaction/undo/redo metadata; observers never see inconsistent publication.
- **CP-003:** Undo, redo, abort and save/reopen preserve the corresponding semantic state.
- **CP-004:** New source geometry and dependency/backlink state agree with the preserved reference; unrelated references do not change.
- **CP-005:** Existing APIs retain their behavior and transaction ownership remains with the caller.

Use deterministic fixtures and table-driven conventional tests; no new property-test dependency.

## Technical Context

- **Language/Version:** existing FreeCAD C++ and Python toolchains; no version bump proposed.
- **Primary Dependencies:** existing App, Part/OCC and Sketcher facilities.
- **Target Platform:** supported FreeCAD platforms; Linux local evidence plus cross-platform CI.
- **Constraints:** thin generated binding, native validation, original line endings and narrow source branch.
- **Performance Goals:** no whole-document recompute or solve solely for admission; reuse prepared projection where feasible. No numerical speedup claim.

## Success Criteria

- **SC-001:** All five requirements are mapped to executed tests/review and accepted durable docs.
- **SC-002:** The bounded two-reference workflow succeeds without raw property edits or constraint reconstruction.
- **SC-003:** All rejected cases preserve semantic state and existing API regressions pass.

## Related Artifacts

- [Design](design.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Canonical context](canonical-context.md)
