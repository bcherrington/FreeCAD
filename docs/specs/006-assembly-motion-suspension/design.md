---
title: Assembly motion suspension design
doc_type: spec
artifact_type: design
status: archived
owner: local-developer
last_reviewed: 2026-11-12
---

# Technical Design

## Overview

Native document-scoped suspension shared by C++ and Python; a context owns a lifetime-safe
RAII handle. A separate explicit validated solve operates without disabling automatic guards.

## Requirement Coverage

| Requirement | Acceptance Criteria | Design Coverage | Validation Approach |
| --- | --- | --- | --- |
| Requirement 1 | AC1–AC3 | Shared native lifetime state, close signal, Python context | Native/Python nesting, exceptions, close/name reuse |
| Requirement 2 | AC1–AC3 | Guards at motion writers and status entry | Copy/remap, callback, simulation and undo regressions |
| Requirement 3 | AC1–AC3 | Raw graph preflight before explicit solve | Escaped references and valid linked geometry |
| Requirement 4 | AC1–AC3 | Additive API, no persisted state | Build and existing Assembly tests |

## Correctness Property Coverage

| Property | Design Behavior | Validation Direction |
| --- | --- | --- |
| CP-001 | Shared state invalidated on document deletion | Nested/exception/close/reopen tests |
| CP-002 | Guard by joint.Document before containment; guard native entry | Native entry and pose observation |
| CP-003 | Nonmutating validation before all solve preparation | Pose snapshots for every rejected target class |
| CP-004 | No exit replay, unchanged default code paths | Outside-scope success and existing tests |

## High-Level Design

Add a native motion-suspension component under `src/Mod/Assembly/App/`. A registry maps live
App::Document identities to shared depth state. Actual document-map removal erases/inactivates that
state; content-reset Delete/New notifications preserve suspension depth during restore; outstanding handles release safely without dereferencing deleted documents. Reuse
of document names, restored UUIDs or memory addresses must not inherit state.

Expose `AssemblyApp.suspendAutomaticMotion(document)` as a Python context manager and
`AssemblyApp.isAutomaticMotionSuspended(document)` as introspection. Entry, release,
exception and GC behavior are exact and idempotent. Native callers use the same RAII state.
All operations follow FreeCAD's main-thread document mutation convention; this is not a
cross-thread document lock.

## Low-Level Design

### Motion gates

Guard ordinary `solve()` before ensureIdentityPlacements, grounding and rigid synchronization.
Return -7 for suspended ordinary solve/simulation/frame operations; void motion writers no-op. Return -8 for failed validated graph admission. Entry into a native rejecting guard is allowed; no solve preparation/backend execution is allowed.
Guard `updateSolveStatus()` before resetting cached diagnostics. Guard recompute, drag,
simulation generation/frame application, undo and public placement-normalization/write
helpers. Python Joint.matchJCS, undoPreSolve, preventParallel and rigid restoration check
joint.Document before backup, suppression or component writes. Grounded property-status
changes during incomplete reference construction must not mutate source objects while
suspended. Joint metadata and Motion property migration continue.

### Explicit solve and ownership

Add `AssemblyObject.solveValidated(enableUndo=False)`. Inspect raw references without calling
helpers that delete invalid joints, normalize placements, sync grounded joints or build the
solver. Define allowed writable components from assembly containment including generated
local link elements and flexible instances, never by following linked geometry into sources.
Validate endpoint, ground, rigid-member and relevant drive references against that closure.
Reject unresolved/unsupported graphs before preparation. Return a distinct validation failure.
Only the internal work of this explicit solve receives permission to write; ordinary solve
entry and Python callbacks remain blocked. A generic 'allow motion' public boolean is rejected.

### Supported admission matrix

Admission supports native AssemblyObject with local Group tree; plain local Part/Body/Feature
components, local App::Link instances (external geometry allowed), local generated LinkElement
components, rigid local AssemblyLink instances and recursively generated flexible AssemblyLink
components. Use active/visited sets before recursive collection; cycles, cross-document Group
members, unresolved links, missing required property types, and unsupported moving roots reject.
Raw JointGroup members are examined even when suppressed: standard two-ended JointObject
Reference1/Reference2 (PropertyXLinkSub local roots and resolvable subelement strings), grounded
ObjectToGround (PropertyLink local admitted component or local origin/root datum), rigid-group
ObjectsToRigidGroup (PropertyLinkList local admitted members), and SimulationGroup members
with Motion.Joint (PropertyXLinkSubHidden, a PropertyXLinkSub root to an admitted standard joint). Incomplete endpoints, escaped
roots, unknown solver-affecting records and unsupported drive forms reject rather than repair.
Raw reference reads must not call getJoints/getRigidGroups filtering/repair. A valid local link's
source geometry is read-only input, never added as an allowed writable target by dereferencing.
Empty/malformed/suppressed records cannot establish permission. Preflight itself writes no
properties, transaction records, caches or diagnostics. Validated solve reconciles grounded
ReadOnly status only after admission. Public ordinary entrypoints still reject while its
private internal implementation runs; final status refresh is observational without lazy solve.

### Link conversion and synchronization

Structural component creation and joint remapping must remain usable while suspended.
Suppress placement synchronization of existing components. Override AssemblyLink::onBeforeChange to reject populated Rigid assignments before value, grounding or pose mutation while suspended. Same-value assignments also reject because this callback cannot inspect the proposed value. Empty initialization and document restore are allowed. No deferred conversion is replayed. Initial copied/serialized
placements are construction data, not permission to execute pre-position callbacks. Tests
must distinguish initializing new objects from moving existing/source objects.

### Failures and compatibility

No preference edits, new transaction behavior, new touched-state purging to simulate recovery, or persisted flags. Existing accepted solve cleanup behavior is retained. Status returns cached
information while suspended; freshness limitation is documented. A caller must refresh/solve
explicitly when ready. This is motion control, not arbitrary-code isolation or complete rollback.
Existing user behavior is preserved outside scope. API naming was resolved in expert review and matches the implemented bindings.

## Slice Boundary And Residual Architecture

| Design target | In this slice | Out of this slice | Follow-up destination | Blocks closure? |
| --- | --- | --- | --- | --- |
| Native suspension | Scope, bindings, writer guards and tests | General App-wide mutation policy | none | yes |
| Ownership | Validated explicit solve for supported Assembly graphs | MCP receipt/admission protocol | bridge repository Spec063 | yes |
| Recovery | Safe scope lifetime | Exact touched/transaction rollback | bridge repository recovery qualification | no |
| External acceptance | Local expert review and test evidence | Actual maintainer approval/platform CI | human PR preparation | no |

## Validation Strategy

Build via `pixi run build -j 2`; focused `ctest --test-dir build/debug -R '^AssemblyObjectTest\.' --output-on-failure`.
Run Assembly Python tests using a private profile/process and Xvfb if required. Cover native
implicit Group solve, offsets/angle/rigid/undo, pre-containment copy, nested errors, close,
external-link positive and escaped-reference negative cases, simulation/frame, structural
remap and mode conversion. Existing normal solve is the outside-scope positive control.
Run applicable formatting/pre-commit checks and record all environmental failures honestly.

## Downstream Task Guidance

Implementation starts after recorded MoE review and disposition of mode conversion and
explicit validation design. Review actual code again after changes; broaden tests only as
needed. Promote durable reference and API docs; retain spec as history only after evidence.

## Open Questions

None blocking. D001 resolved: pre-change populated conversion rejection, empty/restore initialization allowed. Preflight reads raw graph and detects cycles; it must not call getRigidGroups(), which repairs membership. Grounded property status suppression is reconciled by validated native preparation, with positive and escaped-ground tests.

## Native observation oracle

Regression fixtures give a local LinkGroup nonidentity placement before entering suspension;
ordinary solve preparation normalizes it before checking grounding. Assert its placement and
native cached DoF/solver status remain unchanged for Group/status/recompute guarded paths.
Additionally instrument actual C++ solve preparation and placement-write sites under GDB in a
private process; distinguish guard entry from admitted solve execution. Record positive-control
hits outside scope/under solveValidated, and no preparation hits for automatic guarded work.

## Operational Considerations

Additive scripting API, no install/profile migration. Callers must use a single live document,
finish remapping before explicit admission and handle -7/-8 results. Scope exit never solves.

## Implementation Review Reconciliation

- Python context special methods require type-level tp_methods and readyType; instance getattr
  alone is insufficient for `with`. Initial probe reproduced TypeError; corrected probe passes.
- Raw AssemblyLink source chains are checked iteratively before recursive helpers; malformed
  SimulationGroup children and unsupported MotionTypes are rejected structurally.
- Ground retarget/delete queues native deferred unlock intents while suspended. Each intent
  uses a weak object handle plus immutable object ID, is local to the live document, and is
  applied only after successful admission for affected admitted targets no longer grounded.
  Unrelated manual ReadOnly locks and unrelated-assembly intents remain intact; rejected
  graphs consume nothing; scope exit does not replay intent. Scope close clears observers.
- Restoring a serialized App::Link array may canonicalize its local array/element coordinates
  even in plain App::Part (control reproduction without Assembly). This is structural restore
  initialization, permitted when world component poses and source poses remain unchanged.
  Copy retains exact local-pose checks; restore compares effective world component poses.
  Native preparation instrumentation remains a separate no-implicit-motion acceptance check.
