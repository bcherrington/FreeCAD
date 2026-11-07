---
title: Sketcher reattachment T001 native qualification
doc_type: spec
artifact_type: research
status: draft
owner: local-developer
last_reviewed: 2026-11-07
---

# T001 Native Qualification

## Outcome and Scope

T001 - Qualify the API contract and native preservation mechanism is complete.
D001 selects the additive reattachExternal API. After the probes disqualified the
inspected strict atomicity candidates, the user explicitly accepted D002 Option A:
normal FreeCAD transaction semantics. Requirements/design/tasks/tests now preserve
preflight/no-op state and success invariants, permit ordinary intermediate events,
and require caller recovery after unexpected apply errors.

The failed strict candidates remain useful historical evidence, not a proof that
all possible atomic native implementations are impossible. Generic atomic
infrastructure is outside the selected slice. T002 implements the bounded helper
and must still prove its actual success/preflight/error contract. No production
source has been changed by T001.

User authorization covered starting T001 and disposable probes in this workspace.
No branch switch, shared build/configuration change, snapshot installation, public
submission, operator document mutation or external MCP repository edit occurred.

## D001: Qualified Interface and Extension Point

Proposed public signature:

```python
def reattachExternal(self, geo_id: int, obj_name: str, sub_name: str, /) -> None: ...
```

- `geo_id` is a current negative sketch geometry index, -3 for the first non-axis
  projection. It is not an ExternalGeometry list position or native persistent ID.
- Resolve `obj_name` in the sketch's own document, as addExternal does. Resolve
  `sub_name` using native mapped Edge selectors and existing eligibility/cycle rules.
- Parse integer overflow before narrowing or negation; bool/non-integer arguments
  raise TypeError, conversion overflow raises OverflowError, invalid references
  raise ValueError. Do not introduce a generic transport/recovery result envelope.
- Return None on success or a supported exact no-op. The failure/notification
  contract remains subject to D002 and must appear in binding documentation.
- Add the declaration to SketchObject.pyi and a thin wrapper to SketchObjectPyImp.cpp.
  Put validation, projection preparation and reference updates in SketchObject's
  native external-geometry implementation, with a distinct bounded single-reference
  entry point/helper. Do not replace the existing vector attachExternal behavior.

Rationale: reattachExternal describes replacement of a live reference. The native
attachExternal accepts groups, can attach detached geometry and removes/appends
link entries. Reusing that name for a narrower scalar contract would obscure the
semantic difference. This is a local design selection, not upstream API agreement.

Targeted `rg -n '\battachExternal\b' src tests -g '!*.ts'` found only its declaration
and definition in current source. The generated Python surface probe confirms
addExternal exists and neither attachExternal nor reattachExternal is exposed.
No local call-site migration is required; maintain the old C++ method for source
compatibility and separately qualify any new helper's behavior.

Important indexing observations:

- The Python ExternalGeometry getter groups subelements by source object; two
  native link slots to one object can appear as one Python tuple. Do not compare
  Python list length with native raw slot count.
- SketchObject.getGeometryId accepts internal geometry indices only. Inspect
  external native IDs using the C++ ExternalGeometryFacade or the Python facade
  on ExternalGeo entries. The facade constructor clones its argument; reads are
  suitable for the oracle, writes through that clone do not edit the sketch.

## Executed Native Probes

[Native receipt](t001-evidence/native-receipt.json) and
[raw results](t001-evidence/native.log) bind exact seven-case discovery/execution,
source hashes, library hashes and the standalone executable hash.
All seven tests pass because they assert the observed legacy behavior below;
they are not acceptance tests for a new reattachment implementation.

| Probe | Observed result | Consequence |
| --- | --- | --- |
| legacyCircleRetargetKeepsIdsButAppendsLink | Negative-index/native IDs remain stable; raw links change Edge1,Edge2 -> Edge2,Edge3 | Direct wrapper fails the required raw slot-order contract |
| legacyElementObserverSeesMixedState | Three element notifications, two with the new Edge3 link but old selected circle projection | managedoperation/native attachment is not coherent atomic publication |
| disableNotifyStillPublishesPropertySignal | One property signal despite enableNotify(false) | DisableNotify alone does not close the observer gap |
| setterRestorationLeavesTouchedState | Link values restored; sketch/property remain touched | Setter-based value restoration is not no-call state equivalence |
| setterRestorationCannotRecoverClearedRedo | Redo count 1 -> 0 after a booked transaction materializes and values are restored | Restoring values cannot recover discarded redo history |
| legacyDuplicateAdmissionIsNonmutating | Duplicate destination rejects with unchanged links/ID and no element signal | There is a usable preflight/nonmutation pattern |
| legacyReversedLineReencodesOrientation | Constraint.Orientation changes; circle stays on its original geometric side through two recomputes | Geometric-side preservation does not prove exact Orientation preservation |

The redo probe uses Label setters to isolate the generic App transaction mechanism;
the touched/notification/identity probes exercise actual Sketcher external links.
Do not misdescribe the Label test as an injected failure in a new API.

## D002: Candidate Mechanisms and Accepted Option A

Source plus executed probes reject these assumptions:

1. **managedoperation:** SketchObject's external property callbacks update caches
   and emit element signals; normal base property/object notifications still occur.
2. **DisableNotify:** suppresses parent propagation, not Property.signalChanged or
   onBeforeChange transaction handling. Busy is internal recursion protection, not
   an established public multi-property commit mechanism.
3. **AtomicPropertyChangeInterface:** batches notifications for participating changes
   to one property. ExternalGeometry and ExternalGeo do not expose an existing
   combined atomic commit/snapshot mechanism through this interface.
4. **Copy/Paste or setter rollback:** still runs setters/notifications and leaves
   touched and transaction effects. Link setters also maintain backlinks before
   later notifications; checking only final projection values is insufficient.
5. **freeze/unfreeze:** changes read-only state, emits touched notification and
   unfreeze touches the object. It is a modelling state, not a demonstrated invisible
   transaction savepoint or redo restoration facility.
6. **Caller abort:** is normal recovery but aborting the caller's whole transaction
   cannot satisfy the current operation-local guarantee preserving earlier edits.

The user selected Option A after these alternatives were explained. The accepted
requirements now reflect that explicit scope change; the strict alternative is
not an active implementation blocker:

| Direction | Observable contract | Scope consequence |
| --- | --- | --- |
| Normal FreeCAD transaction semantics (accepted by user) | Preflight and exact supported no-op leave state unchanged. Success returns with coherent links/projections/IDs and preserves supported authoring intent. Ordinary property notifications are allowed during application; unexpected application errors require caller abort or checkpoint recovery and do not promise restored dirty/redo metadata. | Keep the API within normal Sketcher property/transaction mechanisms; revise R3 AC3/AC5, CP-002, tasks and tests explicitly before T002 |
| Retain strict atomicity (not selected) | Keep current observer and model/journal guarantees and forbid T002 until a new mechanism is demonstrated | Add and qualify a separately scoped native infrastructure/design slice; cannot call the current guards sufficient |

Exact Orientation remains a success invariant. Existing reversed-line rebuilding
re-encodes it, so the selected initial API rejects reversed/ambiguous mappings in
preflight. T002/T004 must still qualify admitted same-direction line/arc cases,
signed tangency and persistence; those are implementation tests, not open product decisions.

## Option A Recovery Qualification

The updated standalone probe adds callerAbortRecoversUnexpectedApplyFailure.
An element observer throws after native attachment starts changing the source
link. The exception propagates; the caller transaction remains open and its earlier
Label edit remains present until the caller explicitly aborts. Caller abort then
restores original links, external native ID, projected geometry and the earlier
Label value; a subsequent solve succeeds.

Eight exact cases pass in [the Option A receipt](t001-evidence/option-a-native-receipt.json)
and [raw run](t001-evidence/option-a-native.log), with stable source/library hashes.
This qualifies the ordinary caller recovery mechanism, not the future reattachExternal
implementation. The original seven-case receipts and matching source snapshots
are retained as pre-decision evidence.


## Fixtures and Runners

- **Circles:** t001-native-probe.cpp builds three source circles at x=0,20,40,
  radius 3 mm, two referenced edges and a centre/radius-constrained internal circle.
  Native retargeting chooses source Edge3; no user model is loaded.
- **Reversed line:** the same probe builds opposite directions of one line and
  an internal circle with a signed point-to-line distance of 2 mm. It observes existing
  native reorientation; it does not prove the proposed exact-preservation policy.
- **Arcs:** t001-arc-probe.py generates three radius 3 mm circular arcs, projects the
  first two, and fully constrains two internal circle centres/radii. Four constraints
  and external IDs 1/2 survive save/reopen. Source.Edge3 is a prospective replacement
  and has not been retargeted by this fixture.

Existing native runner: build/debug/tests/Sketcher_tests_run. Exact selected cases
are pinned in t001-evidence/existing-native-command.json and
t001-evidence/existing-native.xml: six discovered/executed cases, zero failures.
They cover add/delete external geometry, noRecomputes, direct solve and repairing
a missing external index. Linux CTest registers individual SketchObjectTest names;
a Sketcher-only filter is not a sufficient coverage manifest.

Existing Python runner: an owned build/debug/bin/FreeCADCmd --safe-mode process
with explicit user/system config paths, isolated user data/temp/cache directories
and the source Sketcher test module. Five exact cases in
t001-evidence/python-receipt.json execute with zero errors/failures/skips, including
transaction/undo, orientation migration and external-reference save/reopen.
PartDesign availability is asserted so the save/reopen case cannot silently do nothing.

FreeCADCmd can exit 0 after a script exception. Require the expected receipt and
case counts, not just process status. Earlier probe setup/compile errors and the
incorrect assumption about Python link grouping are preserved as harness failures;
only final named receipts establish the results above.

## Reproduction and Identity

From the FreeCAD root:

```sh
python3 docs/specs/004-sketcher-external-reference-reattachment/t001-run-probes.py
```

The script derives compiler flags from current compile_commands.json and link
libraries from current build.ninja, compiles only the standalone probe into its
new owned temp directory, checks nonempty exact discovery, and verifies source/library
hashes before/after. This lower-level diagnostic intentionally avoids a shared build.
An obsolete Makefile link.txt coexists here and names old Python/OCC libraries;
it must not be used instead of current Ninja metadata.

The owned executable lives under output/bin, with output/share, Mod and lib links
to read-only build resources. FreeCAD derives its resource root from /proc/self/exe;
placing the standalone executable at a bare temp path caused missing-material
harness failures. PYTHONDONTWRITEBYTECODE and isolated user paths prevent probe
cache/settings writes into shared source or the operator profile.

For Python and arc scripts, use the exact command in
t001-evidence/python-command.json, replacing the old temp-directory config paths
with a fresh owned directory and setting SPEC004_ROOT to this checkout.
Use the same safe-mode runner for t001-arc-probe.py and validate arc-receipt.json.
Normal build verification for future source implementation remains pixi run build -j 2.

The two native receipts pin stable loaded-library/source identities during their
seven original and eight updated Option A observations respectively. They do not certify that the entire existing shared build was
freshly rebuilt from the current Git tree. No full build, final feature acceptance,
Windows/macOS qualification or public maintainer agreement is claimed.

## Next Safe Action

D001/D002 are resolved and T001 reconciliation is complete. Obtain fresh T002 task
context and implement the bounded native path under Option A, without changing
shared branch or unrelated files. Native success and preflight preservation still
need actual implementation evidence; the baseline's successful caller abort does
not establish acceptance of the new helper or Python binding.
