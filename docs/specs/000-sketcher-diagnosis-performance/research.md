---
title: Sketcher diagnosis performance research
doc_type: spec
artifact_type: research
status: archived
owner: local-developer
last_reviewed: 2026-11-05
---

# Research

## Question

Can a local reduction of repeated diagnosis improve sketch recompute before
changing numerical algorithms or the operator's modelling intent?

## Findings

Source baseline is FreeCAD integration commit `36aa20c979` (2026-11-05).
The preceding static investigation is evidence for candidate selection, not
proof that any performance change is implemented or accepted.

- `SketchObject.cpp:318` execute calls geometry acceptance and then solve.
- `PropertyConstraintList.cpp:389` acceptance unconditionally emits a change;
  `App/Property.cpp:295` synchronously invokes the owner callback.
- `SketchObject.cpp:1123` ordinary unmanaged constraint changes trigger setup.
- `SketchObjectConstraints.cpp:78` solve performs setup again.
- `Sketch.cpp:254` setup clears/rebuilds the solver. `planegcs/GCS.cpp:522`
  invalidates diagnosis; initialization diagnoses before component partitioning.
- `planegcs/GCS.cpp:4726` constructs a dense Jacobian by evaluating every
  active equation against each diagnostic parameter. Diagnosis performs J/JT QR;
  sparse QR still starts from dense assembly. Async arguments also copy data.
- `SketchObjectPyImp.cpp:369` scalar constraint insertion calls solve; list
  insertion avoids that wrapper solve. Prior bridge work already addressed its
  scalar-loop multiplier; current adapter behaviour has not been re-audited here.
- `SketchObjectConstraints.cpp:164` and `Sketch.cpp:4964` show the existing
  last-solve timer excludes setup/diagnosis.

The historical private latency replay log contains adjacent setup durations
7.38566 s and 5.08845 s followed by a 0.000857262 s LM solve. Setup includes
allocation, QR and possible diagnostic redundancy solving; the log cannot
separate those costs or explain the entire historical long symmetry request.
The retained private static-analysis receipt contains exact excerpt/hash provenance.
Do not treat the summed setup durations as a new benchmark.

## Fixtures

| Alias | Frozen identity | Relevant saved inventory |
| --- | --- | --- |
| Master | Same Sketch004 in both private files below | 104 geometry, 247 constraints, 42 construction elements |
| Staged plate | SHA256 `7699ee464e73822cffc8d0538fef5f3ef82e3d8f5e74ae2220a0e96ad970839d` | 116 geometry, 236 constraints, 107 symmetries |
| Frozen plate | SHA256 `55a0b21e57c979fe9e53dd89c073c47b2718257e7e7b10298375379094e3d4eb` | 118 geometry, 241 constraints, 108 symmetries |

These are private fixtures, not new repository assets. Their snapshot paths and
full inventory are retained in the crash coordination artifact folder under
`sketch-performance-static-analysis/`. Resolve availability in T001; never use
an operator-owned live document. Synthetic tests must not depend on those paths.
Exact serialized duplicate constraints were not found in the two inspected
sketches; mathematical independence and current solver health were not proved.

## Recommendation

Instrument phases, eliminate duplicate setup locally, then measure remaining
work. Do not switch QR defaults, change tolerances or delete intentional symmetry
constraints on the basis of count alone. The native default automatic threshold
is 1000 parameters despite a nearby comment mentioning 200; actual runtime
preferences and rank-sensitive correctness must be recorded rather than assumed.

## Sources And Related Work

Repository paths above are navigation references; verify them again on resume.
The native backlink crash fix `c9af1a45d1`, merged as `36aa20c979`, is a separate
correctness change already included in the baseline. The existing
[restore/image performance package](../freecad-performance-optimization/spec.md)
owns other work and is not migrated or broadened by this package.

## T001 Runtime Reconciliation - 2026-11-05

T001 is complete. The native fixture confirms clean recompute performs no work
and forced unchanged execution performs two setups. The isolated Release native
suite passes 109 tests; two external-reference Python tests also pass. Nine cases
completed optimized baseline A/A batches with native counts checked per sample.
Scalar/list insertion and recoverable diagnostic probes are recorded in
[baseline-report.md](baseline-report.md). Only the frozen fixture was used; the
changed staged file remains excluded. [workloads.json](workloads.json) freezes
execution/reset/cache boundaries and vectors. Load restores 10 sketches with 19
setups but no ordinary executes; thickness propagation performs no sketch setups.
The historical symmetry precursor remains unverified, owned by T006.

Baseline A/A intervals are wide on several workloads. T005 must investigate noise
and collect fresh interleaved matched comparisons; these observations do not
establish a speedup, regression or overhead budget. Old pilot elapsed values are
ineligible, and all production acceptance requirements remain unchanged.

## T002 Contract Reconciliation - 2026-11-05

Source review rejects a broad managed-operation guard because it skips index
validation. Native probes confirm acceptance alone cannot repair invalid indices.
They also establish object/property/solver notification order for valid,
redundant, conflicting, malformed and partially redundant fixtures. External
geometry rebuild contributes a managed acceptance notification even with no
external model links; it must remain outside the proposed deferral scope.

[The T002 contract](timing-invalidation-contract.md) selects narrower deferral,
explicit diagnostic-touch handoff and the tracing/counter/QR ownership plan.
The source GUI consumer updates solver information through `signalSolverUpdate`;
earlier acceptance observers may see the prior solver snapshot under the stated
contract. Addon dependence on that intermediate snapshot remains a compatibility
risk to check during implementation. All 115 isolated Release native tests pass;
production behavior is unchanged and implementation proof remains T003/T004.

## T003 Runtime Reconciliation - 2026-11-05

Optional tracing now records setup/Jacobian/QR/redundancy/numerical/writeback/shape
phases with operation-relative offsets and lanes. Production retains the default
async policy and original payload copies. Exceptional future destruction is
measured without forcing deferred work; rank-not-run and computed rank zero remain
separate. The block fixture requires the native addConstraint API to establish
blocked geometry state. Mixed Message/Log sink output order is unsuitable as a
chronology marker; use operation identities and monotonic span fields.

[phase-tracing-report.md](phase-tracing-report.md) records 136 passing native
tests in Release off/on and Debug, plus command-line opt-in/silence checks. The
new control is not a performance optimization: duplicate diagnosis remains and
no latency/overhead budget has been evaluated. T004 and T005 remain the owners
of those changes and measurements respectively.

## T004 Runtime Reconciliation - 2026-11-05

The bounded deferral is implemented; see
[diagnosis-deferral-report.md](diagnosis-deferral-report.md). Normal callback index
and geometry validation remains before authoritative setup. Diagnostic notifications
can edit solver inputs: the implementation detects notified input edits, repairs
geometry keys/indices and retains the required second setup. This is a correctness
fallback, not a candidate for blanket suppression. Ordinary unchanged recompute
needs one setup. Native, Python and frozen-model checks establish behaviour; T005
still owns elapsed speedup, regression/overhead budgets and disposable GUI checks.
No matrix-copy, structural-reuse or sparse-assembly rewrite was started.

## Original T005 Strict-Gate Reconciliation - 2026-11-05 (Superseded)

Three optimized configurations were checked against the same native execution
and model-state protocol. Candidate master/profile target recompute median
speedups are 47.07% (paired 95% interval 44.44–68.46%) and 57.06%
(42.02–64.88%). These exclude downstream whole-document propagation by the frozen
T001 boundary. Separate thickness/load and tracing-disabled-overhead gates have
unresolved confidence intervals after the maximum 30 pairs; overall AC-005/AC-006
are inconclusive, not passed. See performance-validation-report.md and
performance-results.json for every batch, MAD, interval and disposition.

Separate tracing-on runs preserve active-constraint/equation/unknown/matrix/rank
counters while setups drop two to one. Diagnosis still dominates setup. Master
allocated/effective dimensions are 482x481 / 481x481, rank 481; profile 695x742 /
695x742, rank 695. Both use dense QR. These are stage-labelled structural facts,
not evidence for a matrix-copy or sparse-assembly rewrite. T006 is not started;
T005 acceptance must be resolved before that task proceeds.

## User-Approved Timing Evidence Revision - 2026-11-05

The user wants an indication of improvement, not strict performance measurement.
See [timing-evidence-decision.md](timing-evidence-decision.md). Existing observations
and correctness/execution checks satisfy that objective; T005 is complete and T006
can proceed. Formal percentage/confidence limits no longer block completion.
All original samples and strict outcomes remain unchanged. Load/thickness/overhead
variability remains a limitation, not evidence of guaranteed no regression. A new
controlled-host cohort is optional unless a concrete repeatable slowdown warrants it.

## T006 Attribution And Updated Snapshot - 2026-11-05

T005 committed as root `27d524eaaa` and docs `43e66da`. T006 derives actual root-
execute fractions from interval unions, without adding overlapping QR workers or
nested operations. The retained candidate QR compute unions are about 64%/85%
of master/plate execute; explicit acceptance is about 0.05%/0.02% and dispatch
0.38%/1.22%. Diagnosis remains most of setup. This selects QR-method investigation
before metadata/copy rewrites, with no claimed alternative-backend speedup.

A disposable frozen copy of the newer model (`527589a0...`) qualifies all 13 sketches:
39 untraced and 13 traced forced executes each perform one setup and retain geometry,
constraints and DoF. Updated outer/inner profiles are 72/158 at 14 DoF and 58/132
at 6 DoF. Their untraced medians are about 61.3/34.8 ms. Wiring/auxiliary outlines
have a distinct shape-dominated profile (about 81% of traced execute, untraced
medians about 15.5/13.3 ms). These are not comparisons to the different older model.

The original saved before-repair checkpoint matches replay hash `5bb891ab...`;
Sketch004 is 97/226, but immediate pre-symmetry state after intermediate edits
is not verified. A separate later ~156-second Equality solve envelope also lacks
phase attribution/immediate precursor. Both are retained unverified and owned by
B006; no mutation was retried. [remaining-cost-decisions.md](remaining-cost-decisions.md)
and [remaining-cost-attribution.json](remaining-cost-attribution.json) document the
analysis. [The backlog](../../backlog/README.md) ranks B001–B007 with owners,
validation boundaries and reasons for deferral. T006 is complete; T007 is ready.
