---
title: Sketcher recompute and performance diagnostics
doc_type: reference
status: code-derived
owner: local-developer
last_reviewed: 2026-11-05
applies_to:
  - perf/sketcher-diagnosis
  - src/Mod/Sketcher/App
---

# Sketcher Recompute And Performance Diagnostics

## Purpose

Describe the implemented ordinary-recompute optimization and optional developer
trace. Use the [investigation runbook](../runbooks/sketcher-performance-investigation.md)
for capture, timing and safe model handling. This is current branch behaviour,
not a claim that an installed build or upstream main contains these changes.

## Source Of Truth

Code and regression tests take precedence over prose:

| Surface | Implementation / verification |
| --- | --- |
| Recompute eligibility and validation | [SketchObject.cpp](../../src/Mod/Sketcher/App/SketchObject.cpp), `execute` / `onConstraintsChanged` |
| Authoritative solve/diagnostic handoff | [SketchObjectConstraints.cpp](../../src/Mod/Sketcher/App/SketchObjectConstraints.cpp), `solveInternal` |
| Property keys and index flags | [PropertyConstraintList.cpp](../../src/Mod/Sketcher/App/PropertyConstraintList.cpp) |
| Trace control, fields and lifetime | [SketcherTrace.h](../../src/Mod/Sketcher/App/SketcherTrace.h), [SketcherTrace.cpp](../../src/Mod/Sketcher/App/SketcherTrace.cpp) |
| Setup and numeric solve | [Sketch.cpp](../../src/Mod/Sketcher/App/Sketch.cpp) |
| Jacobian, QR, redundancy | [GCS.cpp](../../src/Mod/Sketcher/App/planegcs/GCS.cpp) |
| Count/observer/index regressions | [SketchObjectRecompute.cpp](../../tests/src/Mod/Sketcher/App/SketchObjectRecompute.cpp), [SketchObjectDiagnosis.cpp](../../tests/src/Mod/Sketcher/App/SketchObjectDiagnosis.cpp) |
| Trace regressions | [SketcherTrace.cpp tests](../../tests/src/Mod/Sketcher/App/SketcherTrace.cpp) |
| Transactions and solver workflows | [TestSketcherSolver.py](../../src/Mod/Sketcher/SketcherTests/TestSketcherSolver.py) |

Production changes are optional tracing `9bc0eb91ff` and diagnosis deferral
`77ca0b6d94`, on the local `perf/sketcher-diagnosis` branch. Validation/research
commits and evidence are recorded in the retained historical delivery package.
No FCStd property or public modelling signature was added. Internal C++ layout
and private diagnostics are not a cross-build binary compatibility guarantee;
use consistently built FreeCAD modules. This work does not change solver defaults,
tolerances, async launch policy or the existing Python solve-time getter meaning.

## Ordinary Recompute Contract

On eligible, valid ordinary document execute, full solver setup occurs once,
instead of the former acceptance-callback setup followed by another solve setup.
The context is private, one-shot and limited to execute's explicit geometry
acceptance after external-geometry rebuild. Normal unmanaged index/geometry
validation still runs before solver input is consumed.

Eligibility excludes restoration, transaction replay, managed/internal operations
and edit-mode `noRecomputes`. Invalid preferences, invalid inputs, nested callbacks
and exceptional handoff retain legacy fallback behaviour. Notification observers
that change Geometry, Constraints or external-geometry properties through normal
notifications cause the required fresh setup; removing that refresh is unsafe.
Geometry-type repairs must also refresh keys/indices before consuming constraints.
The context must reset on every exit; it must not affect the next solve/execute.

Keep these invariants:

- Geometry-key acceptance does not independently repair the invalid-index flag.
  Constraints hidden by invalid input must become visible again after a valid repair.
- Public explicit solve, direct properties, scalar insertion's immediate solve,
  list insertion's deferred behaviour, undo/redo followed immediately by solve/move,
  external references and driver/reference changes retain their required refreshes.
- One setup is not a global limit: observer edits, fallback/error paths, repeated
  block/augmented diagnosis and unrelated operations may require additional work.
- The conditional diagnostic Constraints touch is retained after authoritative
  diagnostics and before final status/numerical solving, under the internal
  transaction guard. Diagnostic IDs, touched state and notification count remain
  part of the contract. Required solver-update emission and shape/error paths remain.
- Earlier ordinary acceptance observers can see the prior diagnostic snapshot;
  the moved diagnostic notification and solver-update signal see authoritative
  results. Known GUI solver information reads solver-update. Addon dependencies
  on the earlier intermediate snapshot are not universally certified.

Raw C++ mutation without property notification is not a supported input-change
invalidation mechanism. Do not add a broad managed-operation guard, suppress all
notifications, or reuse diagnostics solely because counts/type keys are unchanged.

## Trace Control And Privacy

Set `FREECAD_SKETCHER_TRACE=1` **before launch**, and enable the normal Console
Log sink. Exactly `1` enables the trace; unset, `0` or any other value disables it.
Enablement is cached on first tracing entry: changing the environment later does
not toggle an existing process. This is a developer control, not a saved parameter,
GUI preference or modelling Python API. Legacy Minimal/Iteration logs are separate.

Tracing disabled avoids new clocks, span storage/IDs, name copies, formatting,
counter scans and output beyond bounded state/enablement checks. Existing legacy
work/logging remains. Disabling the Log sink suppresses output but does **not**
disable enabled tracing's collection work. Unset the environment variable for
untraced latency observations.

Enabled roots own bounded storage for 4096 spans. Workers write disjoint slots;
only the owner emits records, after future completion. Allocation/capacity/sink
failures must not replace modelling outcomes or exceptions. Treat missing output,
`complete=false` or nonzero dropped records as insufficient attribution evidence.
A complete trace is also not proof that the model operation succeeded: inspect
outcome/status and modelling state.

New trace records include internal document/object names but no user labels,
file paths, addresses, geometry coordinates or constraint/parameter values.
Internal names can still be identifying; review them before sharing. Other legacy
logs and private benchmark snapshots can contain model data and need separate review.

## Trace Record Dictionary

Each line starts with `SketcherPhase ` followed by JSON, schema `sketcher-phase-v1`.
This is a private development format; consumers should validate fields rather
than assume an installed release supports it.

| Field | Meaning |
| --- | --- |
| `operation_id`, `parent_operation_id` | Process-local root identity and nullable linkage; retain process/session identity when combining captures |
| `span_id`, `parent_span_id` | Span identity/tree within an operation; IDs are not an emission chronology |
| `document`, `sketch` | Escaped internal root identities, inherited by children |
| `entry`, `reason` | Actual entry and invocation context; nested solve/setup inherit callers |
| `phase`, `lane`, `caller_thread` | Phase, logical QR lane and whether execution is on the caller; a logical worker lane can run deferred on caller |
| `start_ns`, `end_ns`, `elapsed_ns` | Monotonic offsets from the root origin; closed spans satisfy end minus start equals elapsed |
| `outcome`, `status` | Success/rejection/recoverable error/exception/not-run and nullable native result; DoF for diagnosis/setup, entry-specific enums for solve/execute |
| `complete`, `dropped_records` | Batch completeness and loss, repeated on records |
| `diagnosis_ordinal`, `counters` | Present on each actual diagnosis, including repeated block/augmented analyses |

Outcomes are `success`, `rejected`, `recoverable_error`, `exception`, `not_run`.
Status codes are not interchangeable across entries. Numeric zero, null/not-run
and an exception are distinct; do not infer success from zero without entry context.

| Phase | Scope |
| --- | --- |
| `operation` | Inclusive execute, callback, restore, undo/redo, solve, setup or movement entry |
| `external_geometry`, `geometry_acceptance` | External rebuild and explicit acceptance, including nested callback work |
| `setup`, `setup_construction`, `diagnosis` | Setup, its construction/finalization work and each actual solver diagnosis |
| `jacobian` | Dense allocation, gradients, mappings and driven filtering |
| `qr_dispatch`, `qr_join` | Async expression including payload copies/launch; original explicit wait including deferred execution |
| `qr_j`, `qr_jt`, `qr_compute` | QR helpers and decomposition subspans; helpers include additional work |
| `sparse_conversion` | Existing dense-to-sparse conversion inside sparse helper |
| `redundant_analysis`, `redundant_solve` | Conflict/redundancy analysis and its distinct diagnostic solve |
| `numerical_solve`, `solve_attempt` | Main solve and primary/fallback attempts |
| `geometry_writeback`, `shape` | Result extraction/property update and shape construction |

A shared-state future destruction span has reason `future_destruction`. Production
keeps the original no-policy `std::async` and payload copies, and `wait()` rather
than `get()`. Destruction must join asynchronous work or discard an unstarted
deferred task as before; it must not force deferred work on exceptional exit.

### Structural counter stages

| Counter | Meaning |
| --- | --- |
| `input_active_driving_constraints` | Active/driving Sketcher input, before block expansion/filtering |
| `solver_equations_pre_filter`, `unknowns_pre_driven_filter` | Expanded solver equations and parameters before driven filtering |
| `diagnostic_equations`, `diagnostic_unknowns` | Filled driving rows and remaining diagnostic parameters |
| `allocated_rows`, `allocated_columns` | Actual allocated J, possibly padded or 0x0 |
| `effective_rows`, `effective_columns` | Matrix consumed by QR before transpose |
| `allocated_payload_bytes` | Checked rows × columns × sizeof(double); one dense payload estimate, excluding copies, maps, sparse storage and QR workspaces |
| `qr_method` | Actual dense/sparse choice after auto-selection and compatibility fallback |
| `rank_j`, `rank_jt`, `rank_j_status`, `rank_jt_status` | Independent rank values and computed/not_run/exception states; null is unavailable, computed zero is valid |

Parameter/equation counts are not geometry/constraint counts. Default automatic
QR selection uses diagnostic unknown count before subtracting rank, with threshold
1000 in this branch; a nearby comment mentioning 200 does not override code.
Sparse QR still starts from dense assembly. Do not change thresholds, tolerances
or method selection based on constraint count alone.

## Reading Timings And Current Evidence

Find the actual outer execute root and use its interval as denominator. Never sum
nested operations, diagnosis plus setup, or concurrent J/JT/dispatch/join durations.
Use interval unions clipped to that root for covered wall time. A long join can
contain deferred compute; it is not another independent computation. Dispatch is
an enclosing opportunity window, not an isolated matrix-copy measurement.

Local Linux evidence confirms ordinary setups two to one, native suite 149/149,
affected Python suite 97 run with one existing skip, and bounded GUI diagnostic
panel/recovery checks. The saved model comparisons preserve geometry, constraints,
DoF, diagnostics, expression bindings, shape topology and thickness restoration.
The updated frozen copy qualifies 13 sketches in 39 untraced plus 13 traced samples.
GUI checks used existing consumer binaries with current App code, not a successful
new full GUI rebuild. Windows/macOS and upstream CI are not certified here.

Representative matched optimized medians indicate about 47% lower master and 57%
lower original plate-profile target recompute time. They exclude downstream
whole-document propagation. Other load/thickness/overhead observations are noisy;
these are indications, not guaranteed speedups or proof of zero regression/overhead.
Current timing acceptance uses indicative evidence;
correctness and actual execution checks remain strict. Earlier strict confidence
gates remain recorded as inconclusive historically, not retroactively passed.

The updated model has a different plate structure and is not a paired replacement
for the original fixture. Its larger sketches show QR-heavy work; wiring/auxiliary
outlines instead show shape-heavy work. The [owned backlog](../backlog/README.md)
records the next investigations and deferred copy/metadata/reuse/sparse-assembly
work. Historical long symmetry/Equality mutation cases still need verified immediate
precursors and separate phase attribution; unchanged recompute does not reproduce them.

## How To Update

Change code, tests, this dictionary and the runbook together when trace fields,
phase ownership, control, counter stage or notification/validation semantics change.
Retain source/build/fixture identities for evidence. Run focused native/Python
checks and applicable formatting, and review async/deferred/error cases explicitly.
Do not present proposed backlog work as current implementation. Public submission
and integration are separate decisions from local specification completion.

## Related Docs

- [Investigation runbook](../runbooks/sketcher-performance-investigation.md)
- [Follow-up scope and exit checks](../backlog/sketcher-performance.md)
- [Historical spec/evidence](../specs/000-sketcher-diagnosis-performance/spec.md)
