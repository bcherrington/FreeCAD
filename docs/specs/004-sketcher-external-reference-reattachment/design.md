---
title: Sketcher external reference reattachment design
doc_type: spec
artifact_type: design
status: draft
owner: local-developer
last_reviewed: 2026-11-07
---

# Technical Design

## Overview

Add a supported Python binding backed by a bounded native reattachment path.
Reuse Sketcher's external projection and identity machinery. Qualify the existing
attachExternal implementation before choosing its internal extension point: it
currently expands related geometry, removes old links and appends the replacement.
A thin wrapper alone cannot promise the required slot preservation or failure behavior.

## Requirement Coverage

| Requirement | Acceptance Criteria | Design Coverage | Validation Approach |
| --- | --- | --- | --- |
| Requirement 1 | AC1, AC2, AC3 | Python interface; native ownership | Public Python call and generated documentation |
| Requirement 2 | AC1, AC2, AC3, AC4, AC5 | Admission and application; semantic state oracle | Two-reference fixture, orientation and dependency cases |
| Requirement 3 | AC1, AC2, AC3, AC4, AC5 | Error handling; coherent publication | Rejection matrix, fault injection, journal and observer tests |
| Requirement 4 | AC1, AC2, AC3, AC4 | Transaction and lifecycle behavior | Abort, undo/redo, persistence and propagation |
| Requirement 5 | AC1, AC2, AC3, AC4 | Compatibility; delivery and promotion | Existing regressions, build and specialist review |

## Correctness Property Coverage

| Property | Design Behavior | Validation Direction | Notes |
| --- | --- | --- | --- |
| CP-001 | Replace in place using the existing identity | Compare IDs, both orders and constraints | Ref source changes intentionally |
| CP-002 | Prepare before mutation; restore operation-owned state on failure | Snapshot comparison and immediate subsequent solve/edit | Do not roll back caller edits |
| CP-003 | Property-based persistence and caller transaction participation | Both states across lifecycle operations | Semantic comparison |
| CP-004 | New mapped source with old geometry identity | Change new source, then old source, then recompute | Assert unrelated reference stable |
| CP-005 | Additive binding and isolated strict native path | Existing API and transaction tests | No legacy grouped behavior change |

## High-Level Design

### System Architecture

Python caller -> generated SketchObject binding -> native admission/preparation ->
native in-place reference update -> normal caller solve/recompute/transaction.
There is no GUI dispatch, transport or MCP component in the implementation.

### Components and Changes

- `src/Mod/Sketcher/App/SketchObject.pyi`: typed declaration and public contract.
- `src/Mod/Sketcher/App/SketchObjectPyImp.cpp`: argument conversion, same-document
  name lookup, native invocation and consistent Python exception translation.
- `src/Mod/Sketcher/App/SketchObject.h` and `SketchObjectExternal.cpp`: bounded
  admission, projection preparation, identity retention, coherent invalidation and
  failure restoration. Prefer a strict internal overload/helper over duplicating
  projection logic or silently changing the existing grouped operation.
- `src/Mod/Sketcher/SketcherTests/TestSketchExternalReattachment.py` (new),
  `TestSketcherApp.py` and `CMakeLists.txt`: public API/lifecycle regression registration.
- `tests/src/Mod/Sketcher/App/SketchObject.cpp` and its `CMakeLists.txt` as needed:
  native state/exception coverage using the existing target conventions, confirmed in T001.

### Data Models

No new persisted schema. Distinguish the ordered ExternalGeometry link slots,
ordered ExternalGeo projections, the parallel persisted ExternalTypes vector,
each extension's native ID/Ref/flags and transient
externalGeoRef/externalGeoRefMap/externalGeoMap bookkeeping. Reattachment changes
the selected source and derived projection while preserving its slot and ID.
Transient mappings must be regenerated coherently after application and restoration.
ExternalTypes is per link, distinct from geometry flags: only Projection links
are admitted for reattachment. Unaffected Intersection/Both entries retain order
and value. Native object/property dependencies and source backlinks are part of
the state model, not merely consequences inferred from a correct final curve.

### Data Flow

Admission resolves the geometry index to its existing native ID and unique link.
Prepare the prospective projection in sketch coordinates using existing projection
rules, without publishing the new link. Validate its cardinality, analytic type
and endpoint mapping. Apply the link at its original position, update Ref and
geometry with the existing ID/flags, and invalidate derived solver/shape state.
Let the caller perform solve/recompute and decide the transaction outcome.

## Low-Level Design

### Python Interface

Proposed public signature, to be qualified in T001 before implementation:

```python
def attachExternal(self, geo_id: int, obj_name: str, sub_name: str, /) -> None: ...
```

`geo_id` is a negative sketch geometry index: -3 is the first non-axis external
geometry. It is neither a zero-based ExternalGeometry link slot nor a persistent
native ID. Native IDs remain preservation assertions, not overloaded input syntax.
The object name resolves in the sketch's own document, following addExternal.
`sub_name` is an existing Edge selector resolved by normal native shape APIs.
Successful calls return None; failure raises. No public preview, bulk form or
mode booleans are introduced in this slice.

An active transaction is not a precondition. Ordinary calls without one and calls
with active or booked/global transactions are supported under the metadata rules
below. Calls while native noRecomputes/edit mode is enabled reject before mutation,
including requests that otherwise match the current source. No GUI edit-mode
implementation is introduced by this API.

### Admission and Application

1. Parse without narrowing overflow; reject booleans masquerading as indices and
   invalid types with TypeError, out-of-range integers with a documented exception.
2. Resolve an allowed same-document source and non-null edge through existing
   dependency/cycle rules. Validate all indices before negation/arithmetic.
3. Check supported execution mode, then resolve the selected live projection and link.
   Reject native noRecomputes/edit mode before no-op detection. Admit only an unfrozen,
   non-missing, non-detached projection whose link produces exactly one geometry.
   Preserve authored defining/construction state; ExternalTypes must be Projection;
   Intersection and Both modes reject. Frozen/Missing/Detached states reject even
   if the requested source equals the current source.
4. For otherwise admitted input, detect exact same-link requests before dirtying
   state or emitting change signals. Reject attachment to
   a destination already owned by another link; do not merge link groups.
5. Prepare exactly one line segment, circle or circular arc matching the existing
   analytic type. Reject splines, ellipses, faces, wires, degenerate edges and
   cardinality changes. Use normal sketch placement and mapped reference logic.
6. For bounded curves, establish endpoint correspondence from the existing native
   projection/orientation rules, not guessed nearest endpoints. Reverse parameterization
   only when correspondence is proven; reject ambiguous cases. Full circles have
   a centre role, not line-style start/end roles. Preserve Constraint.Orientation
   exactly in this first slice: do not silently call orientation re-encoding a
   successful preservation. D002 must prove that normalization survives the normal
   rebuild/reorientConstraintsOnReversedGeometry path; otherwise reject before mutation.
   Signed Distance/Tangent semantics must survive repeated recompute and reopening.
7. Prepare owned clones and identity/mapping changes before publishing property
   updates. Preserve old link position, native ID, authored flags and all constraints.
   Do not replace the sketch's authored internal geometry as part of this call.
8. Apply only through the coherent publication mechanism demonstrated by T001/D002.
   The existing managedoperation flag alone does not suppress App observers or
   signalElementsChanged; no atomic notification guard is assumed to exist.
   Reuse the prepared projection rather than letting an unchecked rebuild allocate
   or delete identities. Rebuilding must enforce the same preservation checks.
9. Mark solver/derived geometry stale through normal Sketcher mechanisms. Do not
   perform document recompute, save, or speculative solver retries.

### Error Handling and Atomicity

Match nearby TypeError/ValueError conventions for user input and translate native
projection exceptions consistently. Error messages identify the invalid geometry
index/source/selector and rejected condition. Missing projections cannot be logged
and treated as a successful reattachment.

Precondition rejection occurs before observable property mutation or model-change
signals. For a failure after application begins, restore operation-owned properties,
Ref/ID/mode associations, dependencies/backlinks and coherent derived mappings,
plus pre-call dirty/touched/enforce/recompute state and transaction/undo/redo
metadata. Preserve caller edits, including earlier edits to the same property.
The strong guarantee is required with no transaction, an empty booked transaction
with redo available, and an existing populated transaction.

T001 must demonstrate the actual staging/publication and operation-local restoration
mechanisms before D002 can close and T002 begin. Ordinary setter-based restoration
does not establish this guarantee: setters can emit signals, dirty objects and
materialize a booked transaction or clear redo. Every emitted notification must
expose coherent links, mode, projection, Ref and IDs. A read-only diagnostic observer
must verify the sequence during success and injected failure, not just the final
snapshot. No-op and admission failures emit no model-change notifications.

External observer side effects cannot be universally undone. Consistent transient
notifications during a restored application failure are permitted and must be
documented and sequence-tested; inconsistent notifications are prohibited. CP-002
therefore concerns restored model/journal state and coherent publication, not an
impossible promise that every external observer forgets a notification.
If the proposed guarantee cannot be achieved with bounded existing mechanisms,
T001 must record that result and revise acceptance/design/traceability before any
implementation proceeds. Do not introduce generic App transaction or notification
machinery, weaken checks silently, or present an unproven mechanism as resolved.

### Transaction and Lifecycle Behavior

Transaction ownership stays with the caller. The method does not explicitly open,
commit or abort a top-level transaction. Normal FreeCAD booked/global transaction
materialization from successful property edits is permitted; it must not be described
as a hidden wrapper-owned transaction. No active transaction is required. Failure
and no-op preserve active/booked transaction identity, contents, undo/redo counts
and actual replay behavior. Test clean/no-transaction, empty booked-with-redo,
populated transaction and earlier same-property edit states independently.

Undo/redo must rebuild transient maps and solver state before the next edit.
Persistence tests compare before/after states in separate files and separately
verify undo/redo in memory; an undo stack is not assumed to survive reopening.
Geometrically compatible references can still create ordinary solver conflicts.
Those remain the caller's responsibility to observe and resolve/abort normally.

### Semantic State Oracle

Compare link order/source/selector, projection order, native IDs, authored flags,
the entire ordered ExternalTypes vector, Ref resolution, constraint signatures
including point roles, Orientation, names, driving/active states and expressions.
Compare old/new source InList/OutList and relevant property dependencies, retaining
relationships still needed by other references. On failure/no-op compare document,
object and property dirty/touched/enforce state, pending recompute and transaction
identity/contents/undo/redo behavior. Observe notification-time state as well as
post-call state. Assert unaffected source/consumer objects stay unchanged.
Compare curve type, coordinates and dependent geometry with documented tolerances.

Do not use ZIP bytes, timestamps, generated Part.Geometry.Tag UUIDs or exact BREP
serialization as sole fidelity proofs. Any exclusion must be named and narrowly
justified; authored Ref targets, IDs, flags and constraints cannot be normalized away.
Native Ref strings may be regenerated by normal mapping provided source resolution
and identity are proven. Missing/Sync are derived status, not authored flags to preserve
blindly; successful recompute must report a valid resolved reference.

### Migration and Compatibility

The API is additive with no file-format change. Preserve existing grouped C++
attachExternal semantics and addExternal/delExternal behavior. T001 must inspect
all current native callers before selecting a helper/overload. Existing read-only
properties stay protected. No new dependencies, global signal policy, thread model,
automatic save, silent repair or GUI preference is introduced.

### Security, Trust, and Access

Inputs are ordinary local model identifiers, validated by existing object/link
eligibility rules. No new network, process execution, authentication or credential
surface. Implementation and tests operate on owned disposable documents; historical
runtime identifiers in the coordination log convey no current authority.

### Slice Boundary And Residual Architecture

| Design target | In this slice | Out of this slice | Follow-up destination | Blocks closure? |
| --- | --- | --- | --- | --- |
| Public native-backed Python operation | Single supported projection, exact contract | Bulk/preview APIs | Rejected from initial scope; no committed follow-up | yes for included scope |
| Preserve identity | One-to-one compatible line/circle/arc references | Grouped, intersection, type-changing and ambiguous retargeting | Explicit rejection contract in durable reference | no for excluded cases |
| General integration | API usable by any local script | MCP schemas, matching, delivery and live retargeting | External modelling repository owns integration | no |
| Model stability | Reference and lifecycle regression tests | Historical crash/performance investigations | Existing owning investigations; no new claim | no |

## Validation Strategy

See [verification matrix](verification.md#regression-matrix). Require positive
circle/arc/line cases, invalid-input and injected-failure cases, exact no-op,
caller transaction isolation, normal solve/recompute, source propagation, undo/redo
and save/reopen. Test through the public binding and directly at native boundaries
where Python cannot expose the relevant bookkeeping. Use existing test libraries.

Record the exact source revision, build identity, dependencies, test names and
commands. Focused tests precede full pixi build and required broader tests. Local
Linux evidence does not certify Windows/macOS. No live operator mutation is needed.

## Downstream Task Guidance

T001 resolves signature/indexing and orientation/staging feasibility before T002.
T002 delivers native invariants; T003 exposes the Python method; T004 qualifies
lifecycle behavior; T005 handles compatibility/review; T006 promotes and closes.
If qualification disproves a proposed guarantee, update requirements, design,
tasks and traceability together before coding; do not silently weaken acceptance.

## Operational Considerations

The current worktree is shared with another agent. Keep its current branch checked
out; do not switch, reset or stash shared work to prepare this feature. Agree exact
file ownership before overlapping edits and commit only task-owned paths when
authorized. After validation, prepare a clean upstream PR branch in a separate
worktree and cherry-pick only feature commits. Preserve local line endings. Installing a
snapshot or restarting a user/MCP runtime is a separate action, not part of this
spec. Upstream preparation needs a clean appropriate base and human review,
communication and AI disclosure under current repository policies.

## Open Questions

The [decision ledger](open-decisions.md) owns status and resolution evidence.
This table describes the proposed technical answers and their design impact.

| Decision | Proposed answer | Resolution needed | Owner | Blocks |
| --- | --- | --- | --- | --- |
| D001 | attachExternal(geo_id, obj_name, sub_name) returns None; scalar negative geometry index | Confirm naming/signature against all native callers and binding conventions; record rationale | local-developer | T002, T003 |
| D002 | Preserve Orientation exactly or reject; demonstrate coherent publication and full model/journal restoration | Prove the mechanism with observer-sequence, dirty/redo and dependency probes; revise the spec explicitly if bounded feasibility fails | local-developer | T002, T004 |

Public maintainer agreement has not been obtained. T001 is an executable local
qualification task; its decisions do not require pausing spec creation for approval.

## Related Artifacts

- [Requirements](requirements.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Canonical context](canonical-context.md)
