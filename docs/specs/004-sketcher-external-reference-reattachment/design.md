---
title: Sketcher external reference reattachment design
doc_type: spec
artifact_type: design
status: draft
owner: local-developer
last_reviewed: 2026-11-08
---

# Technical Design

## Overview

Add a supported Python binding backed by a bounded native reattachment path.
Reuse Sketcher's external projection and identity machinery. Qualify the existing
attachExternal implementation before choosing its internal extension point: it
currently expands related geometry, removes old links and appends the replacement.
A thin wrapper alone cannot promise the required slot preservation or failure behavior.

T001 executed baseline and fixture probes. D001 selects reattachExternal; the user
accepted D002 Option A, normal FreeCAD transaction semantics, after existing guards
failed the stronger model/journal/publication contract. The implementation below
uses existing native properties and caller-owned recovery; it introduces no generic
atomic commit machinery. T002–T005 now implement and qualify the local API. See
[the qualification report](t001-qualification.md).

## Requirement Coverage

| Requirement | Acceptance Criteria | Design Coverage | Validation Approach |
| --- | --- | --- | --- |
| Requirement 1 | AC1, AC2, AC3 | Python interface; native ownership | Public Python call and generated documentation |
| Requirement 2 | AC1, AC2, AC3, AC4, AC5 | Admission and application; semantic state oracle | Two-reference fixture, orientation and dependency cases |
| Requirement 3 | AC1, AC2, AC3, AC4, AC5 | Preflight nonmutation; application errors and caller recovery | Rejection/no-op state checks, application fault injection, caller abort and notification observations |
| Requirement 4 | AC1, AC2, AC3, AC4 | Transaction and lifecycle behavior | Abort, undo/redo, persistence and propagation |
| Requirement 5 | AC1, AC2, AC3, AC4 | Compatibility; delivery and promotion | Existing regressions, build and specialist review |

## Correctness Property Coverage

| Property | Design Behavior | Validation Direction | Notes |
| --- | --- | --- | --- |
| CP-001 | Replace in place using the existing identity | Compare IDs, both orders and constraints | Ref source changes intentionally |
| CP-002 | Prepare before mutation; propagate unexpected apply errors for caller recovery | Preflight/no-op snapshots; failure propagation and caller abort/checkpoint tests | No automatic rollback or no-call guarantee after application starts |
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
  failure reporting. Prefer a bounded internal overload/helper over duplicating
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
Transient mappings must be coherent on successful return and after supported caller
abort/undo/redo; an interrupted application may require caller recovery.
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

Public signature implemented and tested locally; not upstream-approved:

```python
def reattachExternal(self, geo_id: int, obj_name: str, sub_name: str, /) -> None: ...
```

`geo_id` is a negative sketch geometry index: -3 is the first non-axis external
geometry. It is neither a zero-based ExternalGeometry link slot nor a persistent
native ID. Native IDs remain preservation assertions, not overloaded input syntax.
The object name resolves in the sketch's own document, following addExternal.
`sub_name` is an existing Edge selector resolved by normal native shape APIs.
Successful calls return None; failure raises. No public preview, bulk form or
mode booleans are introduced in this slice.

The name distinguishes live single-reference replacement from native vector
attachExternal's grouped/detached semantics. Use TypeError for invalid types/bool,
OverflowError for conversion overflow and ValueError for invalid references.
The Python ExternalGeometry getter groups subelements by object; its tuple count
is not the native raw link-slot count. Existing getGeometryId is internal-only;
use external geometry facades for external native-ID assertions.

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
   successful preservation. T001 showed that native reversed-line attachment
   re-encodes Orientation; reject reversed or ambiguous mappings in preflight rather
   than relying on that path. T002/T004 must qualify admitted same-orientation
   line/arc projections through normal rebuild and repeated recomputation.
   Signed Distance/Tangent semantics must survive repeated recompute and reopening.
7. Prepare owned clones and identity/mapping changes before publishing property
   updates. Preserve old link position, native ID, authored flags and all constraints.
   Do not replace the sketch's authored internal geometry as part of this call.
8. Apply through normal native property setters and Sketcher bookkeeping. Ordinary
   property/element notifications can expose intermediate link/projection state;
   managedoperation is not an atomic publication guard. Reuse the prepared projection
   where feasible. Before returning success, verify cardinality, both orders, IDs,
   flags, constraints, ExternalTypes, Ref resolution and dependency consistency.
   A rebuild that logs an error or leaves Missing state must not be treated as success.
   Surface unexpected apply errors for caller recovery; do not retry or manipulate
   the caller's transaction boundaries.
9. Mark solver/derived geometry stale through normal Sketcher mechanisms. Do not
   perform document recompute, save, or speculative solver retries.

### Error Handling and Recovery

Match nearby TypeError/ValueError conventions for user input and translate native
projection exceptions consistently. Error messages identify the invalid geometry
index/source/selector and rejected condition. Missing projections cannot be logged
and treated as a successful reattachment.

Preflight rejection and admitted exact no-op occur before observable property
mutation or model-change signals. All known incompatibilities, including projection
cardinality/type/direction changes, must be checked before application starts.
Those paths preserve model, dependencies, dirty/touched/recompute state, booked or
active transaction contents, and undo/redo behavior.

Once application starts, normal property and transaction behavior applies. Setters
may touch the document, materialize a booked/global transaction, clear redo and emit
intermediate notifications. On successful return, native state must meet every
success invariant. Unexpected application errors must propagate with a diagnostic;
never return success, retry, commit or abort automatically. Do not infer unchanged
state merely from the exception type. No operation-local rollback or restoration
of dirty/redo metadata is promised after application starts.

The caller handles recovery: abort its transaction or restore an owned checkpoint
when no transaction is available. Even if values can be restored, notifications
and discarded redo history cannot be retracted. This is the explicit user-approved
D002 Option A scope change, not proof that a stronger atomic mechanism exists.
Generic App transaction/savepoint/notification infrastructure is out of this slice.

### Transaction and Lifecycle Behavior

Transaction ownership stays with the caller. No active transaction is required;
the method does not explicitly open, commit or abort one. Normal booked/global
transaction materialization is allowed once application starts. Admission failure
and no-op must leave transaction metadata unchanged. After an unexpected apply
error, the caller must stop using the possibly partial result and recover before
continuing. There is no automatic retry.

Prefer one transaction per operation when the caller owns that boundary. An
illustrative caller-controlled sequence is:

```python
document.openTransaction("Reattach external reference")
try:
    sketch.reattachExternal(-3, "Source", "Edge3")
    document.recompute()
    # Validate sketch and dependent features before committing.
    document.commitTransaction()
except Exception:
    document.abortTransaction()
    raise
```

A caller already composing edits in a transaction should retain its own boundary;
the example is not a request to nest transactions. Aborting a transaction also
reverts earlier edits in it. This operation does not provide a savepoint preserving
those edits. Without a transaction, use an owned checkpoint for recovery; source
files and operator documents must not be overwritten to implement this policy.

Undo/redo and caller abort must rebuild transient maps and solver state before
the next edit. Persistence tests compare before/after states in separate files;
undo history is not assumed to survive reopening. Reference compatibility does
not guarantee global solver consistency: the caller observes solve/recompute,
validates its expected geometry and commits or recovers accordingly.

### Semantic State Oracle

On successful return, compare link order/source/selector, projection order, native
IDs, authored flags, the entire ordered ExternalTypes vector, Ref resolution,
constraint signatures including point roles, Orientation, names, driving/active
states and expressions. Compare old/new InList/OutList and property dependencies,
retaining edges still needed by other references. Compare expected dependent
geometry with documented tolerances and assert unrelated objects stay unchanged.

For preflight rejection and admitted no-op, compare model/dependency state plus
document/object/property dirty/touched/enforce state, pending recompute and actual
transaction/undo/redo behavior; assert no change notifications. For unexpected
apply failure, assert an exception and no implicit transaction commit/abort/retry,
record partial state, then recover through the caller boundary. Test that caller
abort restores authored/dependency state and permits subsequent solve/edit; do
not demand restoration of discarded redo history. Test a no-transaction recovery
through an owned checkpoint separately.

Observe notification sequences to distinguish ordinary intermediate events from
successful return. Intermediate coherent multi-property snapshots are not required.
The implementation must still respect native ownership and lifetime, and the final
successful state must meet all success invariants.

Do not use ZIP bytes, timestamps, generated Part.Geometry.Tag UUIDs or exact BREP
serialization as sole fidelity proofs. Name narrowly justified exclusions; authored
Refs, IDs, flags and constraints cannot be normalized away. Native Ref strings may
be regenerated if source resolution and identity are proven. Missing/Sync are
derived status; successful recompute must report a valid resolved reference.

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
preflight/no-op transaction isolation, caller recovery, normal solve/recompute, source propagation, undo/redo
and save/reopen. Test through the public binding and directly at native boundaries
where Python cannot expose the relevant bookkeeping. Use existing test libraries.

Record the exact source revision, build identity, dependencies, test names and
commands. Focused tests precede full pixi build and required broader tests. Local
Linux evidence does not certify Windows/macOS. No live operator mutation is needed.

## Downstream Task Guidance

T001 resolves signature/indexing, bounded orientation admission and D002 failure
semantics before T002. It records why strict atomicity was not adopted.
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
| D001 (resolved locally) | reattachExternal(geo_id, obj_name, sub_name) returns None; scalar negative geometry index | Qualified API/naming boundary; implementation and public review remain pending | local-developer | no remaining local design blocker |
| D002 (resolved by user) | Option A: normal FreeCAD transaction semantics; unchanged preflight/no-op, success invariants, caller recovery after unexpected application error | User accepted Option A; contract/tests reconciled, implementation remains pending | local-developer | no remaining local design blocker |

Public maintainer agreement has not been obtained. T001 is an executable local
qualification task; its decisions do not require pausing spec creation for approval.

## Related Artifacts

- [Requirements](requirements.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Canonical context](canonical-context.md)

## T002 Native Implementation Record

The additive C++ method and direct native tests are implemented as described in
[t002-implementation.md](t002-implementation.md). Preparation reuses processEdge;
application retains normal property notifications and caller-owned recovery.
The admitted native sources are live Part feature edges; circular planes must
be parallel, bounded parameter directions must agree beyond angular tolerance,
and pending Sync is unsupported. The Python binding remains T003.

T004 resolved the centre-Coincident false-success finding through the separately
reviewed GCS status propagation fix (42144bc1e9). A later successful subsystem no
longer hides an earlier failure; existing normal fallback is enabled. The
reattachment method still performs no solver call or retry. Strong geometry
assertions pass with public solver selections 0/1/2.

## Final Implementation and Promotion

The native helper, generated binding and registered fixtures are implemented.
See t004-qualification.md, completion-validation.json and the promoted reference
for executed evidence and current behavior. Same-ID projection rebuilding now
retains the old SketchGeometryExtension (construction/blocked modes, layer and
internal type) while retaining the freshly resolved external Ref separately.
Duplicate detection compares edge siblings without requiring unrelated face or
vertex references to become edges. Both corrections have dedicated lifecycle
regressions and specialist dispositions.

These shared correctness fixes and the GCS status fix are explicitly reviewed
exceptions to preserving accidental prior behavior; no grouped/vector API or
default was changed. B012/B013 own unrelated repository-check failures, and B014
owns separate human/platform/upstream preparation. Current truth is promoted to
docs/reference/sketcher-external-reference-reattachment.md and SketchObject.pyi;
this package will be retained as history.
