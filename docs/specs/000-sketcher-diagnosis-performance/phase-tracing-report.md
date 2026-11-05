---
title: Sketcher T003 phase tracing implementation
doc_type: spec
artifact_type: verification
status: archived
owner: local-developer
last_reviewed: 2026-11-05
---

# T003 Phase Tracing

## Implemented Behaviour

T003 - Add optional phase timings and regression coverage implements Requirement
3, Requirement 4 and the tracing/compatibility part of Requirement 5, following
C-003 through C-006 in [the contract](timing-invalidation-contract.md).
AC-003/AC-004 have native and isolated-process evidence below. AC-006's matched
instrumentation-overhead budget remains T005; no speedup or latency gate is
claimed here. Duplicate diagnosis is retained for T004: ordinary forced execution
still performs two setups, with the previous observer sequences unchanged.

Set `FREECAD_SKETCHER_TRACE=1` before launching a task-owned FreeCAD process and
enable its normal Console Log sink. The environment value is sampled once at
first tracing entry. Absence, `0` or any value other than exactly `1` disables new
tracing; changing the environment later does not toggle it. Disabling the Log
sink suppresses output. This control does not enable or replace legacy solver
Minimal/Iteration logging, change a preference or add a modelling Python API.

Records are newline-delimited JSON prefixed `SketcherPhase `, schema
`sketcher-phase-v1`. Each contains operation/parent IDs, span/parent IDs, internal
document/sketch identities, actual entry and invocation reason, phase, logical
lane, caller-thread flag, monotonic operation-relative nanosecond start/end and
elapsed duration, outcome/status and completeness/drop metadata. Status is native
to the entry: setup/diagnosis report signed DoF; solve/execute report their
respective result enums. Null means no result was supplied. Nested internal
solve/setup entries inherit their caller's reason, so an execute's solve is not
mislabelled as a user-requested explicit solve. Block post-analysis and augmented
system analyses retain their own reason and diagnosis ordinal.

No geometry coordinates, constraint values, parameter values, file paths, object
addresses or user labels are emitted by the new trace. Internal document/object
names are intentionally included and JSON-escaped. Existing unrelated logs retain
their existing routing/content. Concurrent worker intervals can overlap dispatch,
caller QR and join; do not sum inclusive spans as independent elapsed time.

## Implementation Boundaries

[SketcherTrace.h](../../../src/Mod/Sketcher/App/SketcherTrace.h) and its
[implementation](../../../src/Mod/Sketcher/App/SketcherTrace.cpp) are private
Sketcher/planegcs diagnostics. Native test hooks supply a clock, capture sink,
allocation failure, capacity and forced async/deferred policy; production uses
the immutable environment setting, steady_clock, normal allocation/default async
policy and the existing Console sink. The header is not installed as a modelling
interface; private exports support native test linkage.

- Outer execute, callback, explicit solve, additional diagnosis, restore,
  undo/redo and movement entry points establish correlation. The actual movement
  entry is `SketchObjectOperations.cpp`; no movement algorithm changes were made.
- Setup construction/finalization, each real diagnosis, Jacobian assembly,
  QR dispatch, J/JT helper and compute spans, sparse conversion, explicit join,
  future-state destruction, redundancy analysis/solve, numerical solve attempts,
  geometry writeback and shape construction are covered.
- Enabled roots allocate fixed storage for 4096 spans. Atomic reservation gives
  workers disjoint slots; workers never emit Console output or access Python,
  Qt notifications or preferences. Collection is read only after futures finish.
- RAII restores bindings on success, rejection and exceptions. Allocation failure
  suppresses nested trace roots until that operation exits. A lost nested root
  marks its parent incomplete. Buffer overflow keeps a closed root and explicit
  drop counts; incomplete traces cannot validate attribution. Sink/serialization
  failures cannot replace modelling exceptions or outcomes.
- Disabled roots/nested sites perform bounded state/enablement checks. New clocks,
  storage/IDs, name copies, formatting, counter scans, atomic reservations and
  output are guarded by active context. Existing legacy timers/logging remain.

### QR lifetime and payload preservation

Production still calls the original no-policy `std::async` overload, with the
same decayed matrix/map/parameter-list arguments and `silent=true`. A callable
wrapper captures only the small context/lane handle and forwards original
arguments without new payload copies. Tests compare copy count to plain
`std::async` (one copy for the lvalue fixture in both cases).

Explicit joins still use `wait()`, not `get()`. A private future wrapper spans
release of the same future shared state on destruction, including exceptional
owner exit. It never forces `wait()` in its destructor: an unstarted deferred
task is discarded as before. Owner context outlives release/join on all exits.
Logical QR lanes and `caller_thread` distinguish worker execution from deferred
execution inside join without probing/changing the launch policy. Worker exceptions
are recorded while preserving wait-only exception propagation semantics.

### Counter stages

Each actual diagnosis has one ordinal-labelled summary. The known Horizontal
line fixture verifies one active driving Sketcher constraint, one expanded
solver equation, four pre/post-filter geometric unknowns, allocated/effective
Jacobian 1x4, 32 dense payload bytes, selected dense method and J/JT ranks 1.
The resulting 3 DoF and legacy timing-getter relationship are preserved.

Summaries separately report pre-filter solver equation/unknown counts, filled
driving rows, diagnostic unknowns, allocated and effective matrix dimensions,
checked dense payload bytes, actual QR choice after fallback, and independent
J/JT ranks/statuses. Fields not reached are null; rank zero is a computed value,
not a substitute for not-run/exception. Payload bytes exclude copies, maps,
sparse storage and QR workspaces. Driven-only, sparse QR, block post-analysis,
redundancy and byte-overflow cases verify these distinctions.

## Verification Evidence

The native suite contains 21 new tracing tests in
[SketcherTrace.cpp](../../../tests/src/Mod/Sketcher/App/SketcherTrace.cpp), alongside
T002's unchanged diagnostic observer/index probes and prior tests.

| Surface | Evidence |
| --- | --- |
| Disabled path | No context, new clock calls or output with tracing disabled; source inspection guards scans/formatting/IDs/reservation |
| Completion and failure | Nested spans, explicit negative status, exception unwinding, escaped identity, capacity loss, allocation/sink failure and nested-root restoration |
| Thread/lane isolation | Independent owner threads; deterministic async overlap and deferred-in-join intervals using promises and an injected monotonic clock |
| Future semantics | Worker exception remains stored by wait; exceptional owner waits before context teardown; deferred destruction does not run task; payload copy count unchanged |
| Native solver integration | Known dense counters; actual sparse QR/conversion; driven-only null ranks; malformed failure before writeback; separate redundancy solve; repeated block diagnosis ordinals |
| Compatibility | Prior observer sequences and two-setup invariant pass; existing last-solve timing getter agrees with the solver's getter; movement and undo retain explicit entry reasons |
| Process control/output | Five isolated FreeCADCmd cases: unset, on, zero, invalid value and sink off; modelling states agree across cases |

Commands and final results:

- `pixi run build-release -j 2 --target Sketcher_tests_run FreeCADCmd`: passed.
- `pixi run build-debug -j 2 --target Sketcher_tests_run`: passed.
- Full isolated Release suite with tracing off: 136 tests passed.
- Full isolated Release suite with `FREECAD_SKETCHER_TRACE=1`: 136 tests passed.
- Full isolated Debug suite: 136 tests passed.
- Focused filter: `SketcherTrace.*:SketchObjectTest.trace*` (21 tests, included in both full suites).
- Focused pre-commit checks, source line-ending preservation, local links/anchors
  and CRLF-aware whitespace checks passed. Package lint has zero errors and the
  two previously documented authoring advisories.

All native test commands use task-owned `XDG_CONFIG_HOME`, `XDG_DATA_HOME` and
`XDG_CACHE_HOME`, as established in T001. No operator GUI/document was used.
The initial block test incorrectly bypassed the native constraint API that sets
blocked geometry state; the corrected fixture uses `addConstraint()`. The first
process harness incorrectly treated Message/Log output ordering across sinks as
a chronology marker; final checks use operation identities and monotonic spans.
Both failures and corrected results are retained, not reported as successful runs.

## Runtime Example And Limits

The isolated command-line check emits 194 complete records with tracing on,
including restore and recoverable malformed-operation reasons. Every parsed
span has `end_ns - start_ns == elapsed_ns`, valid parent containment, correct
schema and internal identity. Unset, zero, invalid-value and disabled-sink cases
emit zero new phase records; late environment changes do not alter cached control.
Both geometry/DoF and malformed diagnostic states match across all five cases.

A redacted synthetic diagnosis record is retained in
[phase-trace-example.jsonl](phase-trace-example.jsonl). Its timings illustrate the
schema only; development builds ran during smoke verification. The example is
a single diagnosis record; its parent/peer spans remain in the full private trace. They do not
establish a latency baseline, overhead budget or performance improvement.

Private raw logs, XML, command-line scripts/results and source/binary hashes are
retained in the existing crash-coordination artifacts under
`sketch-performance-static-analysis/T003-phase-tracing/`. They use disposable
synthetic models, not operator files. No FCStd files are committed with this task.
T004 owns diagnosis deferral; T005 still owns matched optimized performance,
disabled-overhead budgets and the broader correctness/GUI matrix.
