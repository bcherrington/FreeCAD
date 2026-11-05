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

Spec authoring only. No implementation, performance benchmark, new native test
or build result is claimed. Existing crash-fix tests establish the branch's
starting context only; they do not satisfy this package's acceptance criteria.

## Acceptance And Traceability

| Requirement | Design | Tasks | Cases | State |
| --- | --- | --- | --- | --- |
| FR-001 | D-001 | T001, T002, T004 | AC-001 | Pending |
| FR-002 | D-001, D-003 | T001, T002, T004, T005 | AC-002 | Pending |
| FR-003 | D-002 | T002, T003 | AC-003 | Pending |
| FR-004 | D-002 | T002, T003 | AC-003 | Pending |
| FR-005 | D-002 | T002, T003, T005 | AC-004, AC-006 | Pending |
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

T001 must discover the registered native targets and exact CTest filters before
writing runnable focused commands. Normal build: `pixi run build -j 2`.
Affected suites, manual GUI checks and pre-commit outcomes must be recorded with
results. A skipped check needs a reason and remaining risk. Private traces/models
stay outside Git; promote a redacted summary and reproducible synthetic fixture.

## Review And Closure

Review the scoped guard, notifications, diagnostic semantics, timing fast path,
async lifetimes, cross-platform build risk and meaningful regression coverage.
Record each finding's disposition. T007 requires implemented diagnostic/reference
and investigation guidance plus evidence for all criteria and deferred-work routing.
Structural lint success alone does not make the package ready to implement or close.

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
No native performance acceptance evidence exists yet. T001 creates the baseline.
Numbering remains provisional-confidence because older packages are unnumbered
and no history index exists; creation used the tool's revalidated proposal.

## Residual Risks

Source pointers can drift, private fixtures may be unavailable, and historical
logs do not establish the remaining phase costs. Resolve these in T001; preserve
explicitly unverified cases instead of changing acceptance to match available data.

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
