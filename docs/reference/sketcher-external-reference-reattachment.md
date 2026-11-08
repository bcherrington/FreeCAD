---
title: Sketcher external reference reattachment
doc_type: reference
status: active
owner: local-developer
last_reviewed: 2026-11-08
---

# Sketcher External Reference Reattachment

## Purpose

This checkout provides
`Sketcher.SketchObject.reattachExternal(geo_id, obj_name, sub_name, /)`, which replaces
one live projected external edge while retaining its geometry identity and
existing constraint bindings. It returns `None`.

## Source Of Truth

The implementation and generated
API help in `src/Mod/Sketcher/App/SketchObject.pyi`, backed by
`SketchObjectExternal.cpp`, are authoritative for behavior. Public regressions
live in `SketcherTests/TestSketchExternalReattachment.py`; native tests cover
bookkeeping, exact constraint Orientation and application exceptions.

## Indices and Supported Sources

`geo_id` is the current negative sketch geometry index: `-3` is the first user
external, `-4` the second. It is neither a raw external-link slot nor a persistent
geometry ID. Axes (`-1`, `-2`) and internal geometry indices are invalid here.

`obj_name` is the internal name of an allowed object in the sketch's document.
It must derive from `Part::Feature`. `sub_name` selects a single edge using the
normal shape naming mechanism. The source must satisfy Sketcher's ordinary
same-document, body/part and acyclic dependency rules. Source types supported by
other external-geometry operations are not automatically admitted here.

The selected link must produce exactly one live line segment, circle or circular
arc in Projection mode. The replacement must have the same analytic projection
type. Circular source planes must be parallel to the sketch plane. Normal sketch
placement is respected. Bounded parameter directions must agree; reversed,
perpendicular or otherwise ambiguous directions reject rather than re-encode
constraint orientation or guess nearest endpoints.

Frozen, Missing, Detached and pending Sync geometries, Intersection/Both on the
selected link, grouped projections, faces, wires, splines, ellipses and degenerate
projections are unsupported. A destination already referenced by another edge
link is rejected. Unrelated vertex/face/group or Intersection/Both references can
remain in the sketch. Edit/noRecomputes mode is unsupported, including same-link
requests. Ordinary calls need no active transaction.

## Preservation and Recompute

Successful return preserves the selected persistent geometry ID and negative
index, both raw-link and projection order, the complete ExternalTypes vector,
authored flags/modes/layer and all existing constraint definitions. Ordered
geometry references, centre/start/end roles, names, values, driving/active state,
expressions and native `Constraint::Orientation` remain unchanged. Internal and
unrelated external geometry is not replaced by the call. Ref maps and dependency
backlinks already identify the new source at return.

Normal projection rebuilding also retains the authored SketchGeometryExtension
of existing same-ID projections, including construction mode and layer. This
corrects prior loss of those modes during recomputation; no file migration is
introduced.

The method prepares the projection before changing properties. It updates the
selected link in place, refreshes Sketcher bookkeeping and marks solver state
stale. It does not solve, recompute the document or save. Use normal solve and
recompute diagnostics after success:

```python
sketch.reattachExternal(-3, replacement.Name, "Edge3")
if sketch.solve() != 0:
    raise RuntimeError("The reattached sketch could not be solved")
doc.recompute()
```

A later change to the new source follows the ordinary dependency/recompute path.
An old source remains a dependency only while another retained reference requires
it. Abort, undo/redo and save/reopen use ordinary FreeCAD document behavior; undo
history is not expected to persist in saved files.

## Errors and Caller Recovery

| Exception | Meaning |
| --- | --- |
| `TypeError` | Invalid argument types or arity, including a bool used as an index |
| `OverflowError` | Integer cannot fit the native signed integer range |
| `ValueError` | Unsupported index/source/state/mode/type/direction, duplicate, or failed projection preparation |
| FreeCAD/native exception | Unexpected error after application starts; explicit caller recovery is required |

Preflight rejection and an admitted exact same-object/same-selector no-op leave
model, dirty/recompute and transaction/undo/redo state unchanged. They emit no
model-change notification. Unsupported state/mode checks run before no-op
admission. Different aliases for the same edge are not promised to be no-ops.

Application uses ordinary property notifications. An observer can see the new
link before the projection update completes. Treat successful method return as
the completed boundary, not an intermediate property signal. This is not atomic
publication or a document savepoint. Observers should not recursively alter or
delete the objects being edited during property callbacks.

Unexpected application errors propagate once. Partial changes, dirty state and
lost redo history are possible. The method never opens, commits or aborts a
transaction and never retries. A caller wanting abort recovery can use a dedicated
transaction:

```python
doc.openTransaction("Reattach external edge")
try:
    sketch.reattachExternal(-3, replacement.Name, "Edge3")
    if sketch.solve() != 0:
        raise RuntimeError("The reattached sketch could not be solved")
    doc.recompute()
except Exception:
    doc.abortTransaction()
    doc.recompute()
    raise
else:
    doc.commitTransaction()
```

Abort also discards earlier edits in that transaction. If the operation was
untransacted, reload a caller-owned checkpoint after an unexpected application
error before continuing. Property replay is not a substitute for journal recovery.

## Compatibility and Qualification

This is an additive scalar operation. Existing `addExternal`, `delExternal` and
the native vector/grouped `attachExternal` retain their interfaces and behavior.
There is no new dependency, preference, document format or GUI/MCP requirement.
A separately reviewed solver correction preserves failed independent-subsystem
status so ordinary fallback can run; a later successful component can no longer
mask an earlier failure. This adds no solver call or retry to reattachment.

Linux validation and exact test/build evidence are retained in spec 004's
verification records. Windows/macOS CI, human upstream review and public API
acceptance remain external delivery checks; local validation does not certify
those platforms or imply a published FreeCAD release. Revisit this reference and
its regression tests whenever admission or preservation semantics change.

## Maintenance

Owner: local-developer. Reconcile this reference and generated help against the
authoritative implementation and native/public regression tests when admission,
preservation or transaction semantics change. Regenerated geometry UUIDs and
derived caches are not the persistent native-ID contract. Historical evidence is
retained in spec 004 and the closure log; current behavior belongs in this reference.
