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
| Requirement 2 | AC1, AC2, AC3, AC4 | Admission and application; semantic state oracle | Two-reference fixture and endpoint orientation cases |
| Requirement 3 | AC1, AC2, AC3, AC4 | Error handling; atomic application | Rejection matrix, fault injection and no-op test |
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
ordered ExternalGeo projections, each extension's native ID/Ref/flags and transient
externalGeoRef/externalGeoRefMap/externalGeoMap bookkeeping. Reattachment changes
the selected source and derived projection while preserving its slot and ID.
Transient mappings must be regenerated coherently after application and restoration.

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

### Admission and Application

1. Parse without narrowing overflow; reject booleans masquerading as indices and
   invalid types with TypeError, out-of-range integers with a documented exception.
2. Resolve an allowed same-document source and non-null edge through existing
   dependency/cycle rules. Validate all indices before negation/arithmetic.
3. Resolve the selected live projection and link. Admit only an unfrozen,
   non-missing, non-detached projection whose link produces exactly one geometry.
   Preserve authored defining/construction state; intersection links are rejected.
4. Detect exact same-link requests before dirtying state. Reject attachment to
   a destination already owned by another link; do not merge link groups.
5. Prepare exactly one line segment, circle or circular arc matching the existing
   analytic type. Reject splines, ellipses, faces, wires, degenerate edges and
   cardinality changes. Use normal sketch placement and mapped reference logic.
6. For bounded curves, establish endpoint correspondence from the existing native
   projection/orientation rules, not guessed nearest endpoints. Reverse parameterization
   only when correspondence is proven; reject ambiguous cases. Full circles have
   a centre role, not line-style start/end roles. D002 qualifies the exact rule.
7. Prepare owned clones and identity/mapping changes before publishing property
   updates. Preserve old link position, native ID, authored flags and all constraints.
   Do not replace the sketch's authored internal geometry as part of this call.
8. Apply under established managed-operation/notification guards. Reuse the prepared
   projection rather than letting an unchecked rebuild allocate or delete identities.
   Any necessary rebuilding must enforce the same cardinality and preservation checks.
9. Mark solver/derived geometry stale through normal Sketcher mechanisms. Do not
   perform document recompute, save, or speculative solver retries.

### Error Handling and Atomicity

Match nearby TypeError/ValueError conventions for user input and translate native
projection exceptions consistently. Error messages identify the invalid geometry
index/source/selector and rejected condition. Missing projections cannot be logged
and treated as a successful reattachment.

Precondition rejection must occur before observable property mutation. Native
exception paths after application begins must restore operation-owned properties,
Ref associations, flags, IDs and coherent derived mappings without aborting the
caller transaction. Use existing property/managed-operation mechanisms and a scoped
rollback strategy; qualify it in T002 with injected failures. Do not promise an
exception guarantee merely because the caller could abort. Do not claim external
observer side effects can be universally undone; prevent publication of invalid
intermediate states through the normal notification boundary and review this explicitly.

### Transaction and Lifecycle Behavior

Transaction ownership stays with the caller. The method neither opens nor commits
a top-level transaction and does not abort an existing one. Changes use normal
undoable properties. Tests include a preceding unrelated edit in the transaction:
failed reattachment preserves it; explicit caller abort restores the whole transaction.

Undo/redo must rebuild transient maps and solver state before the next edit.
Persistence tests compare before/after states in separate files and separately
verify undo/redo in memory; an undo stack is not assumed to survive reopening.
Geometrically compatible references can still create ordinary solver conflicts.
Those remain the caller's responsibility to observe and resolve/abort normally.

### Semantic State Oracle

Compare link order/source/selector, projection order, native IDs, authored flags,
Ref resolution, constraint signatures including point roles, names, driving/active
states and expressions. Assert unaffected source/consumer objects stay unchanged.
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

Deliver on a named source branch, preserving local line endings. Installing a
snapshot or restarting a user/MCP runtime is a separate action, not part of this
spec. Upstream preparation needs a clean appropriate base and human review,
communication and AI disclosure under current repository policies.

## Open Questions

The [decision ledger](open-decisions.md) owns status and resolution evidence.
This table describes the proposed technical answers and their design impact.

| Decision | Proposed answer | Resolution needed | Owner | Blocks |
| --- | --- | --- | --- | --- |
| D001 | attachExternal(geo_id, obj_name, sub_name) returns None; scalar negative geometry index | Confirm naming/signature against all native callers and binding conventions; record rationale | local-developer | T002, T003 |
| D002 | Prepare one projection, reject ambiguous endpoint mapping, apply in place with scoped restoration | Demonstrate exact orientation and notification/rollback mechanism on minimal fixtures | local-developer | T002 |

Public maintainer agreement has not been obtained. T001 is an executable local
qualification task; its decisions do not require pausing spec creation for approval.

## Related Artifacts

- [Requirements](requirements.md)
- [Tasks](tasks.md)
- [Traceability](traceability.md)
- [Verification](verification.md)
- [Canonical context](canonical-context.md)
