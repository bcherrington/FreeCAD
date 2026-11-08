---
title: Sketcher reattachment lifecycle qualification
doc_type: spec
artifact_type: verification
status: draft
owner: local-developer
last_reviewed: 2026-11-08
---

# T004 Lifecycle Qualification

The native and public operation is qualified using generated, disposable documents.
No operator CAD, runtime replacement, installation or external repository change
was used. The accepted Option A transaction contract remains unchanged.

## Solver Finding Resolved

T002 observed a fully constrained centre-Coincident circle staying at its old
position despite a reported successful solve, with both new and legacy retargeting.
Isolation identified the inherited Levenberg–Marquardt preference (algorithm 1)
as the differing execution path; the clean default DogLeg profile (algorithm 2)
passed the same strong centre-motion assertion. A controlled public run with
algorithm 1 reproduced four geometric failures independently of the C++ fixture.

A separate deterministic GCS regression proves the reporting defect: two
disconnected equalities contain an unsolved moved-X component followed by an
already-satisfied Y component. With one LM iteration, the old outer solve loop
returns the later component's Success and hides the earlier failure. The regression
fails against the original implementation; the retained before-fix receipt is
`completion-evidence/gcs-before.txt`.

The five-line correction preserves the first non-success subsystem result,
allowing Sketch's existing solver fallback to run. It does not change preferences,
add a reattachment retry, or make the API solve. The regression also retries with
normal DogLeg limits and verifies X=40/Y=0 after applying the solution. The fix is
separately committed as `42144bc1e9`, reviewed by the native specialist, and is
covered by the broader native/Python suites. The original centre-motion assertion
is retained and now passes; the T002 finding is resolved rather than waived.

All sixteen public cases pass with each explicit solver selection 0, 1 and 2:
48 executions, zero failures/errors/skips. The full public `TestSketcherApp` module
runs 113 cases successfully with its one existing secant-support skip. Native
qualification runs all 163 cases, including thirteen reattachment cases and the
new GCS regression. Exact final manifests, receipts and build/source identities
are in the completion evidence/receipt.

## Lifecycle and Preservation

The native suite compares exact constraint fields, ordered roles and native
Orientation. The signed-line fixture checks signed Distance and Tangent through
repeated recompute, undo/redo and restoration. Public cases independently verify
Tangent side, centre coincidence, and actual dependent line endpoints constrained
to arc start/end across recompute, replay and reopen. Projected analytic coordinates
use 1e-9 mm checks; solved internal coordinates use the existing T001/Sketcher-test
1e-7 mm bound for nonlinear solution tolerances. IDs, roles, mode vectors and
orientation are exact; no tolerance is used to accept inversion or false movement.

Two-reference cases preserve the unaffected reference, both orders, IDs and
ExternalTypes. Mixed Projection/Both neighbours remain ordered. Nonidentity
placement is exercised. Cross-object cases cover retained and removed old-source
relationships, explicit abort, undo/redo, reopen, source edits and old-source
deletion. Native graph assertions account for OutList's per-link multiplicity.

Preflight/no-op cases compare dirty/property flags, pending/booked transactions,
undo/redo counts and earlier caller edits to the same property. Read-only observers
see no property events for admitted no-op/rejection and ordinary notifications
for successful application. Injected native errors after link and projection
publication propagate once and leave the caller transaction open. Explicit abort
restores the earlier edit, IDs, outgoing dependencies and destination backlinks,
with and without a retained old-source reference. Untransacted checkpoint reload
restores a usable sketch; subsequent solve and geometry addition work.

## Review Corrections

- Duplicate detection now compares generic sibling subshapes only when they are
  edges. An unrelated vertex/face/group from the same source no longer rejects a
  valid edge replacement; a public vertex-sibling regression passes.
- Shared projection rebuilding retains the old SketchGeometryExtension when
  replacing an existing same-ID projection. The newly resolved external Ref is
  retained separately. Construction/defining state and layer 7 survive forced
  recompute and save/restore. This explicitly reviewed correction changes prior
  mode loss during ordinary rebuilding without changing grouping or file format.
- The generated Python help states the Part::Feature and parallel-circle-plane
  restrictions. The test module is registered in CMake and TestSketcherApp.

Private native, API/maintainer-style, transaction and test specialist reviews found
no remaining implementation/test blocker after these corrections. These were
simulated expert reviews, not consultation or approval by actual FreeCAD maintainers.

## Remaining Delivery Boundaries

Whole-project CTest results and inherited failures are recorded separately in the
completion report; they are not reported as a clean project result. Windows/macOS
CI, human upstream API review, issue alignment and public preparation remain
external delivery gates. This qualification establishes the requested local
implementation/lifecycle contract, not a released upstream API or universal
solver correctness for arbitrary models.
