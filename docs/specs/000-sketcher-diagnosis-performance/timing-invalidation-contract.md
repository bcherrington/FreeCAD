---
title: Sketcher timing and invalidation contract
doc_type: spec
artifact_type: design
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# T002 Timing And Invalidation Contract

## Decision And Authority

This is the implementation contract selected in T002 - Finalize timing and
invalidation boundaries, for Requirement 1 through Requirement 5,
AC-001/AC-002/AC-003/AC-004/AC-006 and D-001/D-002. It specifies proposed T003/T004
behaviour; no production tracing or setup deferral is implemented by T002.
The T002 source inspection followed T001 commit `1efd3713a8`. T003 now implements
C-003 through C-006; [phase-tracing-report.md](phase-tracing-report.md) records
the implementation and tests. C-001/C-002 setup deferral remains T004 work.
Native baseline probes are in
`tests/src/Mod/Sketcher/App/SketchObjectDiagnosis.cpp`. This contract supplements
[design.md](design.md); requirements and acceptance budgets remain authoritative.

## C-001 - Validation Ownership And Eligibility

Reject using `managedoperation` to hide the geometry-acceptance callback. It skips
`Constraints.checkConstraintIndices()`, and accepting geometry does not reset
`invalidIndices`. The selected mechanism is a private, one-shot recompute context
that defers only the callback's final full setup, after its existing validation.

The execution order required of T004 is:

1. Preserve `Part2DObject::execute()` and `rebuildExternalGeometry()` outside the
   deferral scope, including attachment, reference migration and their callbacks.
2. Establish eligibility: ordinary execute, not restoring, transaction replay,
   managed/internal transaction or edit-mode `noRecomputes`. Preflight the solver
   preference without changing the stored solver. An invalid preference takes the
   original callback/setup/error path; do not return earlier or erase its diagnostics.
3. Enter a private RAII context only around the explicit
   `Constraints.acceptGeometry(getCompleteGeometry())` call. Preserve its
   `aboutToSetValue()/hasSetValue()` and all property/document notifications.
4. In the first eligible `onConstraintsChanged()` callback, run the existing
   `checkConstraintIndices(getHighestCurveIndex(), -getExternalGeometryCount())`,
   geometry-key checks and acceptance/error handling in their original order.
   Defer the final `setUpSketch()` only if indices and geometry keys are valid.
   Mark that deferral in the execute-owned context. Consume the one-shot request
   before recursive work; nested callbacks must take their normal path.
5. Leave the context before applying the existing solver preference and solving.
   Keep the original `solve(true)` setup authoritative, consuming `getValues()`
   only after callback validation. Never use `getValuesForce()` as solver input.
6. Preserve `retrieveSolverDiagnostics()`, status priority, numerical solve,
   geometry writeback, `signalSolverUpdate()` and shape/error return paths.

Validation belongs to the existing unmanaged callback, not the tracing layer or
an assumed-valid managed branch. Invalid indices keep raw constraints intact and
hide them from solver input as before. A repaired transition must explicitly
refresh indices; clearing geometry-key invalidity alone is insufficient.

If eligibility fails, validation fails, the callback does not reach the intended
skip, or an unexpected nested callback performs setup, retain the legacy path
and its diagnostic effects. Only a confirmed deferred setup authorizes the
notification handoff below. RAII must restore any prior context on all exits.
If acceptance throws after deferral, restore its omitted setup/diagnostic effects
under the original internal-transaction guard before the existing exception
handler proceeds. Preserve the original exception/status policy; tracing failures
must not replace it. If a faithful handoff cannot be proved, keep the legacy
path for that case rather than enabling deferral there.
Do not leave a pending deferral for the next execute/solve or retry the model.
T004 must test this exceptional handoff before enabling the optimization.

Use a private `solveInternal(updateGeometry, notifyDeferredDiagnosis)` helper or
private overload, with the existing public `solve(bool)` forwarding `false`.
Only the execute caller with a confirmed skipped setup passes `true`. This keeps
scalar insertion, list deferred solving, direct solve/property changes, movement,
undo/redo, restoration and `noRecomputes` out of the changed notification policy.
No public C++/Python modelling signature or FCStd property is added.

## C-002 - Diagnostic Notification Contract

`SketchObject::setUpSketch()` retrieves diagnostics and calls `Constraints.touch()`
for redundancy, negative DoF, conflict, malformed or partial redundancy.
`Property::touch()` calls the owning object's early/change callbacks and then
sets its Touched bit. It does not emit the property's `signalChanged` or an
about-to-change event. Thus a touch is not equivalent to silently setting a bit
or to `hasSetValue()`. `solve()` currently retrieves diagnostics but does not
perform this conditional touch.

When C-001 actually defers setup, the private solve helper must perform the same
conditional `Constraints.touch()` immediately after its authoritative setup and
`retrieveSolverDiagnostics()`, before status selection, numerical solve,
geometry writeback and `signalSolverUpdate()`. Use an `internaltransaction` RAII
guard around this touch, preserving notification delivery while preventing
recursive diagnosis. The condition includes partial redundancy even when the
ultimate recompute succeeds. Do not add touches to unrelated public solve calls.

Required observables:

- Preserve stored/raw constraints, post-operation diagnostic IDs and classification,
  DoF, solver result, geometry, external links and status/error outcomes.
- Preserve acceptance's before-change and property-value notifications. Preserve
  the conditional diagnostic touch and its object/document notification count.
- At the diagnostic-touch notification, all authoritative diagnostic IDs are
  already available. At `signalSolverUpdate()`, they and the final solver outcome
  are available. Preserve one such solver-update emission on these fixtures.
- Preserve the Touched bit being set by the diagnostic touch. Document recompute
  may clear it on a successful completion; an invalid return retains it.
- Do not suppress global notifications, bypass expression/transaction bookkeeping,
  copy only part of the diagnostic flags, or touch all constraints unconditionally.

One allowed intermediate difference is explicit: the conditional diagnostic-touch
notification moves from inside geometry acceptance to after authoritative setup.
Acceptance's ordinary change/value observers may therefore see the previous
solver snapshot until the later diagnostic notification/solver-update signal.
Final identities and the diagnosis-bearing notification must agree. No promise
is made that removing an entire diagnosis preserves a callback's obsolete
intermediate solver snapshot. Tests must make this difference visible.

The inspected GUI consumer supports this boundary:
`ViewProviderSketch::updateData()` does not update solver information for a
Constraints property change; `slotSolverUpdate()` calls
`UpdateSolverInformation()` and redraws after reading the final diagnostics.
This is bounded source evidence, not proof about every addon. T004/T005 must
exercise diagnostic panel updates and failure recovery in a disposable GUI.
Custom observers that depend on diagnostics during the earlier acceptance event
are a documented compatibility risk; any newly identified required consumer
blocks T004 until its notification contract is preserved or explicitly resolved.

### Baseline observer vocabulary and fixtures

Native probes observe an unchanged forced recompute after establishing each
fixture's diagnostics, clearing only the Constraints property's Touched bit and
touching the sketch. Observations exclude fixture construction and initial solve.
`B` = object's Constraints before-change; `C` = object's Constraints changed;
`P` = Constraints property's own value-change signal; `S` = solver-update signal.
An isolated diagnostic touch adds `C`, not `B` or `P`. C-bit values are sampled
inside each C callback, before the notifying touch/setter finishes.

| Fixture | Baseline sequence | C-bit values | Final property Touched | Raw diagnostic IDs |
| --- | --- | --- | --- | --- |
| Horizontal line | BCPBCPBCPS | 0,1,1 | false | none; 3 DoF |
| Two identical Horizontal constraints | BCPBCCPS | 0,1,1 | true | redundant `[2]` |
| Horizontal plus Angle pi/4 | BCPBCCPS | 0,1,1 | true | conflicting `[1]` |
| Radius on a line | BCPBCCPS | 0,1,1 | true | malformed `[1]` |
| Full-circle arc, coincident endpoints | BCPBCCPBCPS | 0,1,1,1 | false | partially redundant `[1]` |

The partial-redundancy fixture is an arc centred at (0,0), radius 5, range 0 to
2*pi, with its start/end coincident. It checks notification semantics on a
synthetic degenerate boundary, not production geometry robustness or timing.
The leading BCP comes from managed geometry acceptance at the end of
`rebuildExternalGeometry()`; it does not perform full setup and must be retained.
T004's expected change for diagnostic rows is BCPBCP**C**S or BCPBCP**C**BCPS,
moving the diagnostic C after explicit acceptance's P; success remains BCPBCPBCPS. Counts and final
Touched states remain the same. Candidate C-bit samples may change with the
permitted ordering, but the diagnostic C must finish with Touched true.
Additional overconstraint and solver-error tests use the same condition/status
rules; negative DoF is independently included in the notification predicate.

### Index transition tests

The native probe retains a Horizontal constraint referring to missing geometry
index 1 while only index 0 exists. `getValuesForce()` retains one constraint;
`getValues()` is empty. `acceptGeometry()` alone cannot repair that index flag.
Adding geometry 1 and checking indices repairs the flag; acceptance and direct
solve then consume the constraint, producing 7 DoF on two independent lines.

T004 must add valid -> invalid -> repaired transitions for internal and external
indices, using the actual `getHighestCurveIndex()`/external count. Assert raw
identity preservation, visibility via `getValues()`, validation-before-setup,
recoverable diagnostics, and the absence of a falsely successful solve of an
empty list when the indices have actually been repaired. Invalid indices retain
the legacy filtered-input/error-log path; do not invent a new public error code
in this optimization. The baseline's filtering behaviour must remain distinct
from malformed solver constraints. Include geometry type-key invalid/repair and
missing/reversed/loaded external-reference cases identified in T001.

## C-003 - Trace Control, Correlation And Ownership

T003 implements the private Sketcher/planegcs trace helper in
`src/Mod/Sketcher/App/SketcherTrace.h` and `.cpp`. Keep the helper independent of App/Gui
types; SketchObject supplies identity at the root boundary.
Enablement is process-start environment `FREECAD_SKETCHER_TRACE=1`; every other
value or absence is off. Sample at the first entry into an immutable, thread-safe local static; set the
variable before launching the process.
Runtime environment changes are not supported. This is a development diagnostic
control, not a saved modelling preference, Python API or existing debug-mode
change. Existing Minimal/Iteration logs and timing getters retain their meaning.
The existing Sketch debug mode defaults to Minimal and cannot be used as the
new tracing opt-in without enabling it by default. Compile-time solver reporting
also includes matrix/parameter dumps and is not a suitable worker-safe phase
sink. The immutable environment control works in headless processes without a
new persisted preference or GUI control; detailed trace rendering remains on
the owner thread through the existing Console routing.

A disabled root performs only the cached enablement check. Nested/worker sites
check a nullable context; they do not add clock reads, build IDs, copy names,
allocate buffers, scan counters, lock, format or emit new records. One-time
static initialization is part of the bounded enablement check; existing legacy
timers/logging are unchanged. Leave existing
solver work, copies and logging alone. Unit tests need a private test-only
context/sink and clock seam; neither becomes an exported modelling API.

Enabled roots own the operation context; same-thread children reference it
through a scoped thread-local binding that restores the previous binding.
Independent nested operations have explicit parent IDs; separate documents and
operations never share an active global context. Only enabled roots allocate
operation IDs using a process-wide atomic. Root identity uses internal document
and object names, never file paths, labels, geometry, constraint values, addresses
or parameter values. Escape names during owner-side output, not in worker spans.

Use `std::chrono::steady_clock`, an origin per root, integer nanosecond offsets
and integer lane IDs. Fields for every span are:

| Field | Contract |
| --- | --- |
| schema | `sketcher-phase-v1` |
| operation_id, parent_operation_id | Process-local identity; parent nullable |
| span_id, parent_span_id | Unambiguous tree within operation |
| document, sketch | Root internal identities; children inherit |
| entry, reason | Actual entry/caller, never inferred from elapsed time |
| phase, lane | Enumerated phase and logical owner/QR lane |
| start_ns, end_ns, elapsed_ns | Relative to root origin; end-start equals elapsed |
| outcome | success, rejected, recoverable_error, exception or not_run |
| status | Existing solver/diagnostic result when available; otherwise null |
| complete, dropped_records | Explicit loss/overflow state; incomplete traces cannot validate attribution |

Root entries/reasons cover execute/document_recompute, solve/explicit_solve,
callback/constraint_change or geometry_change, restore/document_restore,
undo_redo/transaction_replay, move/geometry_move, and additional_setup/diagnosis.
Use the existing caller context for nested setup/solve; record the actual fallback
entry if a call has no more specific context. Establish restore and transaction
reasons at their SketchObject entry points, not by guessing inside planegcs.
The actual movement entry is in `SketchObjectOperations.cpp`; tracing it does not
authorize changing movement semantics.

Each enabled operation uses bounded private record storage (4096 span slots,
including root), allocated only after enablement, with a reserved root slot.
Start/end update the same slot. Reserve slots atomically for concurrent writers;
workers write disjoint slots. Use enumerations and numeric fields in hot paths.
Overflow marks the root incomplete and counts dropped spans; no partial trace
may be interpreted as complete. Allocation/serialization failures disable/drop
tracing for that operation and must not change modelling outcomes or exceptions.
Trace finalizers are noexcept. Record success/rejection explicitly; RAII records
exception unwinding for scopes without an explicit result. No output from workers.

After children/futures finish, the owner serializes newline-delimited JSON through
`Base::Console().log`, prefix `SketcherPhase `. A private emission mutex serializes
these new operation batches. Preserve the normal Log sink routing; if the Log
sink is off, new output is suppressed. A trace requires both the environment
control and an enabled Log sink. This control does not enable legacy debug logs.
An internal capture sink lets tests inspect records without parsing human logs.

## C-004 - Phase And Async QR Boundaries

| Phase | Native boundary and meaning |
| --- | --- |
| operation | Inclusive outer execute/solve/callback/restore/move lifetime |
| external_geometry, geometry_acceptance | `execute()` rebuild/accept calls; includes any nested callback setup |
| setup | Entire `Sketch::setUpSketch()`, including block post-analysis |
| setup_construction | Setup work excluding nested diagnosis spans; report inclusive boundaries, derive exclusive time only with non-overlapping intervals |
| diagnosis | Every actual `System::diagnose()`, including repeated block/augmented analysis with distinct ordinal/reason |
| jacobian | `System::makeReducedJacobian()` allocation, filling, maps and driven-parameter filtering |
| qr_dispatch | Original `std::async` expression, including its matrix/map/list argument copies |
| qr_jt, qr_j | Actual QR helper on caller lane and dependent-parameter task lane respectively |
| sparse_conversion | Existing dense-to-sparse conversion inside sparse QR helper; included in parent QR helper |
| qr_join | Original `future.wait()` duration, including deferred execution when applicable |
| redundant_analysis, redundant_solve | Conflict-group analysis and its `solve(SubSystem*, alg, true)` distinctly nested |
| numerical_solve, solve_attempt | `Sketch::solve()` and existing primary/fallback attempts in `internalSolve()` |
| geometry_writeback | Extraction/property update in `SketchObject::solve()` |
| shape | `SketchObject::buildShape()` |

Record both QR helper inclusive spans and the decomposition compute subspan so
copies, sparse conversion and rank extraction are not confused with QR compute.
Dispatch, join and worker spans can overlap. Never sum them to reconstruct wall
time; use the common origin and lane intervals. Parent spans are inclusive.

Retain the original no-policy `std::async` call and `wait()`, not `get()`.
Preserve the existing J/map/parameter argument copies and `silent=true` worker
behaviour. Do not add copies of these payloads or capture them by reference.
A thin callable wrapper may capture a small trace lane/context handle while
accepting the original decayed async arguments by const reference; no extra
matrix copy is permitted. If tracing is off, preserve the original data flow.

T003's private future wrapper also spans destruction of the same shared state.
It does not call wait in its destructor: async work joins and unstarted deferred
work is discarded, including on exceptional owner exit. The destruction span
has reason `future_destruction`; it is distinct from the explicit wait span.

Before dispatch, reserve the worker's parent/lane information. Bind its context
inside the callable and restore it on return/unwind. Worker writes are private
record-slot updates, with no Console, Python, Qt notification or preference access.
The root/context and referenced parent records outlive the future on both normal
and exceptional owner exits; future destruction/join must precede context teardown.
Merge/read worker records only after waiting. Do not add a catch/rethrow of worker
exceptions to `wait()`; its existing exception-propagation semantics remain intact.
Worker RAII can record an exception even when the future stores it silently.

Use a logical QR-task lane even if the task is deferred; additionally record
whether it executes on the caller thread. Do not call `wait_for()` merely to
probe deferred state. Offset/thread relationship to the join span distinguishes
deferred work. Tests may inject a private launch seam choosing async or deferred;
production retains the original default policy. Deterministic barriers/fake clock
verify overlap/deferred interval reconstruction without flaky sleep assertions.

## C-005 - Structural Counters

Capture a diagnosis summary once per actual diagnosis, including explicit
not-reached/null fields on early exit or exception. Do not turn null into zero.
Read existing sizes/results at their owning stage; only enabled tracing may count
input driving constraints. Preserve the diagnostic ordinal for repeated analyses.

| Counter | Stage and exact meaning |
| --- | --- |
| input_active_driving_constraints | Sketcher input list, active and driving; pre block/group expansion/filtering |
| solver_equations_pre_filter | `clist.size()` before Jacobian filtering; expanded solver equations, not Sketcher constraint count |
| unknowns_pre_driven_filter | `plist.size()` before excluding driven value parameters |
| diagnostic_equations | `jacobianconstraintmap.size()`, tag >= 0 and driving rows actually filled |
| diagnostic_unknowns | `pdiagnoselist.size()` after driven-value exclusion |
| allocated_rows, allocated_columns | Actual J dimensions; allocation can include zero padding or resize to 0x0 |
| allocated_payload_bytes | Checked `rows * columns * sizeof(double)`; dense payload only, excludes maps, copies, QR workspaces and sparse storage |
| effective_rows, effective_columns | `J.topRows(diagnostic_equations)` dimensions, before transpose |
| qr_method | Actual dense/sparse after auto-choice and compatibility fallback; not merely configured preference |
| rank_j, rank_jt | Existing local result after the respective compute/threshold/rank call succeeds; nullable independently |
| rank_j_status, rank_jt_status | computed, not_run or exception, independently; null rank is unavailable and valid computed zero is distinct |

The current Jacobian is allocated with `clist.size()` rows and then only driving
rows are filled; QR uses `topRows(jacobianconstraintmap.size())`. Reporting only
one row count would hide this distinction. Sparse QR still converts that dense
matrix. No new matrix assembly, rank computation or parameter scanning is allowed
solely for diagnostics. Block post-analysis may perform multiple diagnoses;
record them honestly rather than treating every setup as exactly one QR pair.

Known T003 counter fixture: one independent 2D line plus Horizontal. Expect one
input active driving constraint, one expanded driving equation, four geometric
unknowns, effective Jacobian 1x4, dense payload 32 bytes when allocation is 1x4,
QR-J rank 1, QR-JT rank 1 and 3 DoF. Add a driven-only case to verify 0x0/not-run
handling, and block/redundancy cases to verify ordinals and nested solve reasons.

## C-006 - Implementation Validation Plan

| Case | Requirement / criterion | Owner and required evidence |
| --- | --- | --- |
| Baseline observer/diagnostic fixtures and invalid -> repaired indices | Requirement 1/2; AC-001/AC-002 | T002 native probes establish current sequences and flags; T004 updates the explicitly moved notification expectations |
| Valid/empty/underconstrained execute, nested callback and exception | Requirement 1/2; AC-001/AC-002 | T004 proves one setup only on eligible path, RAII reset and legacy fallback elsewhere |
| Internal/external index and geometry-key transitions | Requirement 2; AC-002 | T004 asserts validation before solver consumption and raw identities preserved |
| Invalid solver preference, overconstraint, malformed, redundancy, partial redundancy | Requirement 2; AC-002 | T004 checks IDs, conditional touch timing/count, final flags and error priority |
| Scalar/list undo, direct property change, noRecomputes, undo/redo then solve/move | Requirement 1/2; AC-002 | T004/T005 preserve non-target setup/notification semantics and final geometry |
| GUI diagnostics and error repair | Requirement 2; AC-002 | T005 isolated GUI verifies panel/notification consumers; no operator session |
| Off/on scopes, errors, dropped records, allocation failure, nested roots | Requirement 3/5; AC-003/AC-004 | T003 internal capture-sink/clock tests; no detailed output or disabled-only clock/allocation/format/counter activity |
| Dense/sparse QR, async/deferred and exceptional owner/worker | Requirement 3/4/5; AC-003 | T003 deterministic lifetime/lane/overlap tests; no launch-policy, wait/get or payload-copy change |
| Known fixture, driven-only, block/redundancy counters | Requirement 4; AC-003 | T003 verifies stage-labelled sizes, actual QR method, null rank and diagnosis ordinals |
| Existing timing getters and tracing-off latency | Requirement 5; AC-004/AC-006 | T003 preserves getters and initial tests; T005 applies unchanged matched-build overhead/uncertainty gate |

T003/T004 must include these tests in their own changes; T002's design completion
is not evidence that those implementation tests pass. Use T001's Linux native
counter seam for per-sample benchmark execution checks, verified on both builds;
it does not replace portable native unit tests or establish trace overhead.
