---
title: Native Sketcher reattachment implementation and evidence
doc_type: spec
artifact_type: verification
status: archived
owner: local-developer
last_reviewed: 2026-11-07
---

# T002 Native Implementation

Historical slice: T004 subsequently resolved the solver finding below through
42144bc1e9 and extended lifecycle qualification. See t004-qualification.md and
completion-validation.json for final results.

T002 adds `SketchObject::reattachExternal(int, App::DocumentObject*, const char*)`
and direct C++ regressions. The public Python binding remains T003. This is local
implementation evidence, not upstream API approval or whole-feature acceptance.
The preceding T001 qualification was committed as root `b6f8654d91` and nested
docs `e079c44`. The shared branch and unrelated GUI/spec003 work were preserved.

## Implementation Boundary

`SketchObject.h` declares the method; `SketchObjectExternal.cpp` owns it beside
the existing projection helpers. The existing vector `attachExternal` and its
callers are unchanged. No App transaction/savepoint infrastructure was added.

Admission checks execution mode, negative geometry index before negation, live
source/dependency rules, native ID/Ref maps, one-to-one cardinality, flags and
Projection mode. Missing legacy ExternalTypes entries retain their existing
implicit Projection meaning; the entire stored vector, including surplus entries,
is never resized. Sources must supply live Part feature edges resolved through
the same shape API as ordinary rebuild. Faces, wires and nonanalytic curves are
outside the admitted scope. Pending Sync is rejected alongside Frozen, Missing
and Detached. A same-source request is a no-op only after supported-state and
projection validation. Duplicate destination selectors/topological aliases on the
same source reject without merging links.

Preparation reuses `processEdge` with the normal sketch placement frame. It must
produce one analytic projection of the existing type. Circular sources must be
parallel to the sketch plane. For lines, the native parameter direction must have
a positive dot product with the previous direction, beyond angular tolerance.
For arcs, parameter-ordered endpoint chords must meet the same test and axis
sense must agree. Degenerate chords/radii reject. This is conservative direction
admission, not nearest-endpoint matching or signed-constraint re-encoding.

The method clones the authored geometry with all its extensions, replaces only
the prepared curve handle, updates the existing raw link slot, then uses the
normal normalized Ref and geometry property setter. It rebuilds the vertex index,
marks solver state stale and accepts the unchanged geometry type keys through
Sketcher's existing bookkeeping. Constraint values/expressions and authored
internal geometry are not replaced. Final checks cover cardinality, ordered
native IDs, raw links, mode vector, constraint/internal-geometry identity, flags
and selected Ref-map coherence. Ordinary intermediate notifications are allowed.

The application section deliberately has no catch/replay/transaction manipulation.
An unexpected exception propagates, and the caller must abort its own transaction
or reload its owned checkpoint. Aborting a shared transaction also reverts earlier
caller edits. The method does not solve, recompute the document or save files.

## Native Regression Coverage

The new `SketchObjectReattach` suite is registered in the existing
`Sketcher_tests_run` target via `tests/src/Mod/Sketcher/App/CMakeLists.txt`.

| Case | Evidence boundary |
| --- | --- |
| CirclesPreserveIdentityOrderModesAndConstraints | Two-circle selected-only replacement; both orders/IDs; exact authored constraint fields and expression identity; repeated recompute |
| CircleCentreDistanceFollowsNewProjection | Centre-role DistanceX follows the replacement immediately after explicit solve and recompute |
| PreservesAuthoredExtensionsAtReturn | Defining and construction modes survive the native call without reinterpretation |
| SignedLineKeepsOrientationThroughRecompute | Signed Distance retains exact Orientation and side through two recomputes; reversal rejects |
| ArcKeepsParameterizedEndpointRoles | Arc start/end coordinates and IDs through repeated recompute; reversed-axis arc rejects |
| RejectionAndNoopPreserveRedoAndCallerEdits | Extreme/axis/internal indices, null/absent/duplicate source; no notifications or dirty/journal changes; no transaction, booked transaction and earlier caller changes to the same property |
| UnsupportedStatesRejectBeforeNoop | Frozen/Missing/Detached/Sync, Intersection/Both, noRecomputes precedence |
| UnsupportedAndAmbiguousProjectionsAreNonmutating | Type change, reversed/perpendicular lines, collapsed projection, ellipse, vertex and empty selector |
| ReplacingObjectUpdatesDependenciesAtReturn | New/obsolete dependencies immediately correct; old source deletion; exact mode vector including surplus entry |
| GroupedFaceReferenceIsRejected | A multi-projection source cannot be retargeted as one geometry |
| CrossDocumentAndCyclicSourcesAreRejected | Native same-document and DAG boundaries; a subsequent valid replacement works |
| CallerAbortRecoversApplicationExceptions | Injected errors after link and projection publication propagate once, leave caller transaction open; abort restores earlier edits, IDs, constraint bindings and usable solve/edit |
| CallerReloadsCheckpointAfterUntransactedApplicationException | Injected untransacted error; explicit caller reload restores IDs/constraints and usable solve/edit |

Fixtures are disposable C++ documents and generated OCC analytic shapes. No GUI,
MCP runtime, operator CAD, new test library or Python binding is needed. Snapshot
oracles compare dirty/property flags, undo/redo counts, pending/booked transaction
state, geometry identity and exact constraint fields/ordered element roles.

## Validation Record

Exact commands, discovered/executed case manifests, source/library hashes and raw
receipts are recorded in [t002-validation.json](t002-validation.json) and
`t002-evidence/`. The standard build command is `pixi run build -j 2`. Native
validation uses the generated binary, with both the focused suite and the broader
Sketcher suite; a zero-case filter is never counted as a pass.

Applicable pre-commit checks run on the four task-owned source/test files.
Clang-format 23.1.0 formats the new test file and inserted source/declaration
regions only; whole-file formatting is intentionally avoided for legacy files.
Existing CRLF source endings and LF CMake/test endings are preserved. Diff checks
use `core.whitespace=cr-at-eol` so the retained CRLF bytes are not misreported as
new trailing whitespace. No shared-worktree stashing hooks are used for commits.

Package lint reports zero errors/warnings. The evidence-quality checker classifies
T002 evidence as concrete with zero errors; ten advisory weak-evidence warnings
refer to the historical authoring/qualification log, not the new native receipts.

Full repository CTest, public Python, cross-platform, save/reopen/undo/redo success
qualification and specialist implementation review remain T003–T005. Existing
Pivy stub-generation warnings are recorded in the build log, not treated as new
method failures.

## T004 Solver Qualification Finding

The fully constrained radius-plus-centre-Coincident circle fixture returns solve
success but leaves the internal centre at x=0 after retargeting its external
centre to x=40, including repeated document recompute. Replacing the native call
with existing `attachExternal({-3}, source, "Edge3")` produces the same result on
this branch, as well as its known raw-link reordering. See the saved native and
legacy comparison logs in `t002-evidence/`.

This is an observed branch baseline issue, not a proven root-cause diagnosis.
The committed circle identity test verifies the unchanged Coincident constraint
and its centre-role binding; it does not claim the missing solver movement works.
The separate DistanceX and signed-line tests assert actual dependent movement.
T004 owns reproducing/triaging the Coincident case and resolving or explicitly
reviewing its disposition before complete API qualification. Requirements are
unchanged; this finding must not disappear into a passing preservation test.
T002's native admission, return-state preservation and caller recovery are tested
independently. Whole-feature release/promotion remains blocked on later tasks.
