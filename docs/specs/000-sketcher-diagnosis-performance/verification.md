---
title: Sketcher diagnosis performance verification
doc_type: spec
artifact_type: verification
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# Verification

## Current State

T001 - Reconcile source and establish the baseline is complete. The native suite
passed 109 tests in an isolated Release environment. The [workload manifest](workloads.json)
freezes nine optimized baseline cases with per-sample native counts, resets and
cache classes; [baseline-report.md](baseline-report.md) records raw evidence,
medians/MAD and A/A uncertainty. Historical symmetry remains unverified, owned by
T006. T002's design contract and native observer/index probes are complete;
T003 tracing is implemented and validated in
[phase-tracing-report.md](phase-tracing-report.md); T004 is next. Diagnosis
optimization and comparative performance acceptance remain pending. Baseline
A/A results do not satisfy candidate budgets.

## Acceptance And Traceability

| Requirement | Design | Tasks | Cases | State |
| --- | --- | --- | --- | --- |
| FR-001 | D-001 | T001, T002, T004 | AC-001 | Pending |
| FR-002 | D-001, D-003 | T001, T002, T004, T005 | AC-002 | Pending |
| FR-003 | D-002 | T002, T003 | AC-003 | Native/process verified in T003 |
| FR-004 | D-002 | T002, T003 | AC-003 | Native/process verified in T003 |
| FR-005 | D-002 | T002, T003, T005 | AC-004, AC-006 | AC-004 verified; AC-006 pending T005 |
| FR-006 | D-003 | T001, T005 | AC-005, AC-006 | Pending |
| FR-007 | D-004 | T006, T007 | AC-007 | Pending |

## Correctness Matrix

- Valid, empty and underconstrained sketches: count setup and compare geometry/DoF.
- Conflicts, redundancies, partial redundancies and malformed constraints:
  compare diagnostic constraint IDs as well as classification and recoverable
  errors. Capture `Constraints` touched state and observer sequence/count; compare
  with T002's required-effects contract and justified intermediate differences.
- Constraint indices: valid-to-invalid and invalid-to-repaired transitions after
  internal/external geometry changes. Assert constraints are neither silently
  omitted through stale `invalidIndices` nor consumed with newly invalid indices.
- External geometry update/migration and driver/reference changes: preserve links,
  values, notifications and resulting constraint validity.
- Scalar/list Python insertion in transactions: retain required scalar pre-commit
  solving and list deferred solving; undo restores both constraints and geometry.
- Edit-mode `noRecomputes` and direct property changes: retain their required
  diagnosis/refresh behaviour independently of ordinary document recompute.
- Undo/redo followed immediately by explicit solve and movement, without an
  intervening document recompute: restore geometry, constraints and solver state.
  Do not impose the one-setup invariant on these non-target paths.
- Pad/Pocket dimension perturbation/restoration: valid solid, correct lengths and extents.
- Tracing enabled/disabled and exceptions: scope closure, correlation, no disabled
  output, unchanged timing-getter semantics and safe QR worker handling. Exercise
  overlapping and deferred QR: offsets/lanes and dispatch/join spans must reconstruct
  wall time while keeping the original launch policy. Verify structural counters
  on a known small fixture; unknown rank is distinct from rank zero on failure.

## Measurement Matrix

### Executed workloads and state resets

T001 freezes a manifest with exact commands, timing boundaries, expected execute/
setup counts and reset method for each case before collecting acceptance data.

| Workload | Execution and reset contract | Interpretation |
| --- | --- | --- |
| Clean no-op recompute | Start clean; call ordinary recompute without touching; assert zero sketch executes/setups | Scheduler/no-op control only; never evidence of diagnosis speedup |
| Forced unchanged geometry | From the same clean saved state, explicitly touch the target sketch or use a verified force path; assert target execute and expected setup counts | Eligible diagnosis workload; baseline count two and optimized count one only on the targeted path |
| Dimensional edit | Restore identical disposable precursor; apply the same driver change and time its defined mutation/recompute boundary | Record expected executed sketches, then validate and restore outside timing |
| Scalar/list insertion | Restore the same precursor per pair; insert the same constraints with the declared scalar or list API and transaction boundary | Compare equivalent operations; preserve the distinct intended solve/undo semantics |
| Historical symmetry | Use exact precursor/request, reset and authority receipt | If unavailable, record unverified; never substitute a different operation silently |
| Fresh-process load | Launch a new task-owned process/profile per sample; time the declared document-open boundary | Call it fresh-process load; OS page-cache state is uncontrolled unless independently established |
| Reopened warm load | Within an initialized process, close/discard and reopen an identical disposable file after warmups | Separate from fresh-process loads and no claim of cold filesystem caches |

Capture actual execute/setup counts for each timed sample, including tracing-off
runs, with the same bounded count-only test seam enabled in both configurations;
measure/include its cost in both. Do not substitute a separate traced run for the
execution check. Exclude tracing's detailed records from disabled-path acceptance.
Reject an entire matched pair if counts, inputs or reset checks fail; retain the
raw pair and reason, and replace it under the original ordering protocol. Process
survival is insufficient: validate geometry, diagnostics and links after samples.

### Pairing, variability and budget decisions

Compare baseline, instrumentation-only and optimized configurations in equivalent
optimized builds. Attribute phases with tracing on; enforce total latency and
instrumentation overhead with tracing off. Record machine/compiler/dependency
identity, source and flags, preferences, fixture hashes, process/cache class,
workload/reset commands and sampling order. Reset disposable state outside the
measured region; never reuse a changed sketch as the next precursor.

For each comparison/workload, perform at least two untimed warmup runs per
configuration where warming is meaningful. Then collect a batch of ten matched
baseline/candidate pairs, alternating A/B then B/A order by pair. Fresh-process
loads use new processes throughout; warmups do not turn that case into reopened
load. Keep raw samples, pair IDs, order and rejection reasons. Report each
configuration's median and median absolute deviation (MAD), plus the effect and
its paired bootstrap percentile 95% confidence interval. Resample whole pairs
10,000 times with a recorded fixed seed; recompute both medians and the effect
for every resample. Never resample the two configurations independently.

For speedup use `1 - candidate_median / baseline_median` and require the entire
interval to be at least 10% on one eligible diagnosis-dominated workload. For
regression/disabled overhead use `candidate_median - baseline_median`, with
allowance `max(0.05 * baseline_median, 0.001 seconds)` and
`max(0.02 * baseline_median, 0.001 seconds)` respectively. In each bootstrap
resample recompute the allowance from that resample's baseline median; assess
its interval on `difference - allowance`. A regression/overhead gate passes
only when its entire interval is at or below zero. An interval wholly on the
unacceptable side is a failure; one crossing a threshold is inconclusive.

For an inconclusive gate, collect at most two additional independent batches of
ten matched pairs with the same manifest, warmup policy and alternating order;
report per-batch results and the pooled paired interval without dropping an
unfavourable batch. If uncertainty still crosses the threshold after thirty
pairs, retain an explicit inconclusive result: the criterion is not met. Investigate
noise or route a bounded measurement follow-up; do not weaken the target, accept
from the median alone or describe an inconclusive result as no regression.
These intervals are diagnostic estimates, not a claim to remove all measurement
bias. Record systematic process/cache or thermal variation separately.

## Commands And Evidence

T001 discovered `Sketcher_tests_run` and the exact CTest filter
`^SketchObjectTest.recompute`. Tested commands, build identity, an executable
baseline launcher and isolated native-test instructions are recorded in
[baseline-report.md](baseline-report.md) and [quickstart.md](quickstart.md).
Use the concrete Pixi build tasks there to preserve target/job arguments.
Affected suites, manual GUI checks and pre-commit outcomes must be recorded with
results. A skipped check needs a reason and remaining risk. Private traces/models
stay outside Git; promote a redacted summary and reproducible synthetic fixture.

## Review And Closure

Review the scoped guard, notifications, diagnostic semantics, timing fast path,
async lifetimes, cross-platform build risk and meaningful regression coverage.
Record each finding's disposition. T007 requires implemented diagnostic/reference
and investigation guidance plus evidence for all criteria and deferred-work routing.
Structural lint success alone does not make the package ready to implement or close.

## T002 Contract Evidence

For Requirement 1 through Requirement 5, T002 resolves the implementation
decisions in [C-001 through C-006](timing-invalidation-contract.md). It selects
deferral after normal index validation, preserves the diagnostic touch via a
private solve handoff, and explicitly permits only its documented intermediate
notification reordering. No managed-operation or global notification bypass is
approved. The source review includes the GUI solver-update consumer.

The native fixture `tests/src/Mod/Sketcher/App/SketchObjectDiagnosis.cpp` records
object/property/solver notification order and property touched state for valid,
redundant, conflicting, malformed and partially redundant cases. Its index probe
proves that geometry acceptance alone cannot clear invalid indices and that a
repaired constraint becomes visible to direct solve. These baseline assertions
remain in place until T004 updates only the explicitly allowed ordering changes.

Validation commands on 2026-11-05:

- `pixi run build-release -j 2 --target Sketcher_tests_run`: passed.
- With isolated XDG config/data/cache, `pixi run ./build/release/tests/Sketcher_tests_run --gtest_output=xml:/tmp/freecad-sketcher-T002/native-final.xml`: **115 tests passed**.
- Exact focused selection: `--gtest_filter='SketchObjectTest.diagnosis*'` (six cases, included in the passing full suite).

Private receipts are retained under the crash-coordination artifacts in
`sketch-performance-static-analysis/T002-contract/`. Initial probe expectations
omitted the managed acceptance during external-geometry rebuild; failures exposed
the additional BCP notification group. Source review confirmed it, and the final
contract/tests preserve the measured sequence. The initial compile also required
explicit Base::Vector3d construction; corrected before the passing build.

T003 must prove disabled-path behaviour, exception-safe spans, structural counts
and QR lifetimes/overlap/deferred execution. T004 must prove callback consumers,
exceptional handoff and all non-target paths. T005 still owns GUI checks and
matched performance budgets. This design evidence does not pass AC-001 through
AC-006 or claim the proposed guard is already implemented.

## Quality Gates

- Authoring: required artifacts, links and task dependency/traceability checks pass.
- Implementation: T001's executed/reset manifests and T002's timing/counter and
  index-validation/diagnostic-observer contracts are approved before T003/T004.
  Missing validation ownership or non-recompute coverage blocks T004.
- Validation: AC-001 through AC-006 backed by actual tests/measurements.
- Closure: AC-007, review disposition, promotion and residual ownership complete.

## Evidence Log

On 2026-11-05, Spec Lifecycle Manager package lint reported zero errors and two
advisories: EARS detection and optional canonical-context review. The requirements
contain explicit WHEN/THE SYSTEM SHALL clauses linked to observable AC definitions;
this does not establish delivery acceptance. Authority review is recorded in
[traceability.md](traceability.md#authority-review); no separate imported-authority
artifact is needed. Local Markdown link and whitespace checks passed.
Optimized baseline evidence now exists; candidate performance acceptance remains
T005. T003 owns production trace semantics and T005 owns tracing-off overhead.
Numbering remains provisional-confidence because older packages are unnumbered
and no history index exists; creation used the tool's revalidated proposal.

## Residual Risks

Source pointers can drift, private fixtures may become unavailable, and historical
logs do not establish remaining phase costs. T001 pinned source/fixture/build
identity. Preserve the unverified symmetry case in T006. Large A/A intervals in
the baseline require noise investigation and fresh matched comparisons in T005;
they cannot justify passing a budget or weakening acceptance.

## MoE Review Disposition - 2026-11-05

Reviewed snapshot: FreeCAD `b1713a7200`, docs `ae4fb78`. The FreeCAD developer,
numerical/CAD and performance reviewers supplied five consolidated findings and
one optional improvement. The user requested all six be addressed in the spec.

| ID | Finding | Specification disposition | Implementation evidence owner |
| --- | --- | --- | --- |
| R-001 | Guard can skip index validation and diagnostic side effects | FR-002/AC-002 and D-001 require explicit validation ownership, valid-invalid-repaired transitions and diagnostic-ID/touched/observer contract before guard selection | T002 defines; T004/T005 prove |
| R-002 | Non-recompute solve paths lack named regression gates | FR-001/AC-002, D-001 and correctness matrix name scalar/list transactions, edit-mode noRecomputes, direct property changes and immediate post-undo solve/move | T001 selects fixtures; T004/T005 prove |
| R-003 | No-op recompute and undefined cold/reset conditions can invalidate timing | FR-006 and D-003 require frozen manifests, actual execution counts and explicit process/cache classes | T001 defines; T005 measures |
| R-004 | Concurrent QR durations alone cannot reconstruct wall time | FR-003/AC-003 and D-002 require monotonic offsets, lanes, dispatch/join spans and unchanged async policy | T002 defines; T003 tests |
| R-005 | Median-only gates do not resolve threshold-overlapping noise | AC-005/AC-006 and measurement matrix require alternating pairs, MAD, paired bootstrap intervals and bounded escalation to inconclusive | T005 evaluates |
| R-006 | Optional structural counters improve attribution | FR-004/AC-003 and D-002 require counts, matrix dimensions/bytes, actual QR method and rank, without disabled-path counter work | T002 defines; T003 tests |

All six findings are addressed at specification level. Their code/test and
performance acceptance remains pending; this disposition does not claim the
proposed guard is safe, a speedup exists or benchmark budgets have passed.

### Remediation validation

Independent targeted agent re-reviews on 2026-11-05 confirmed R-001/R-002
(FreeCAD developer lens) and R-003 through R-006 (performance/instrumentation
lens) are addressed at specification level, with no introduced material gap
in their reviewed scopes. This is not implementation or numerical acceptance.
Package lint reports zero errors and the same two documented authoring advisories;
local file/anchor links and Git whitespace checks passed before commit.
No code, native test, benchmark, build or live CAD operation was part of this revision.

## T001 Investigation Evidence

This establishes baseline evidence for Requirement 1, Requirement 2 and
Requirement 6 (AC-001/AC-002/AC-005); delivery acceptance remains pending.

See [baseline-report.md](baseline-report.md): Release build and isolated native
suite passed (109 tests); all nine runnable cases completed two warmups and ten
alternating A/A pairs, with zero execution-vector mismatches or rejected pairs.
Forced synthetic/master/profile paths each execute once and set up twice;
scalar/list insertion counts are 18/3 with undo to zero constraints. Load restores
10 sketches with 19 setups and zero ordinary executes; thickness propagation
performs no sketch setups and preserves the checked solid/linked dimensions.

Raw receipts, analysis and build/dependency hashes are retained under the private
`sketch-performance-static-analysis/T001-completion/` artifact directory. The
baseline report records the ambient-config native crash in material loading and
the passing isolated rerun. Old Debug/concurrent-build/touch-included pilot timings
remain ineligible. Current A/A variation is explicit; candidate speedup, regression
and instrumentation overhead decisions still require T005's full protocol.

## T003 Implementation Evidence

Requirement 3/Requirement 4 and AC-003/AC-004 are verified by the implementation
and tests in [phase-tracing-report.md](phase-tracing-report.md). Requirement 5 is
partial until T005 evaluates AC-006. New tracing is disabled by default; the
existing two-setup and diagnostic observer baselines remain unchanged.

Release tests pass 136/136 both with tracing off and with the environment opt-in;
Debug tests pass 136/136. The 21 new tracing cases cover disabled clocks/context/
output, nested/exceptional scopes, overflow/allocation/sink failure, worker and
owner exceptions, async/deferred lanes, unchanged payload copies, actual dense/
sparse QR, structural counters, block ordinals, diagnostic failure and move/undo
reason correlation. Five isolated FreeCADCmd modes produce equal modelling state:
on emits 194 complete validated JSON records; unset/zero/invalid/sink-off emit none.
Existing timing getters and solver algorithms are retained.

Private receipts are under `sketch-performance-static-analysis/T003-phase-tracing/`.
Formatting/pre-commit, line endings, local links/anchors and CRLF-aware whitespace
checks passed. Package lint reports zero errors and the same two authoring
advisories. These checks do not pass AC-005/AC-006, T004's deferral contract or the
remaining GUI/correctness matrix owned by T004/T005.
