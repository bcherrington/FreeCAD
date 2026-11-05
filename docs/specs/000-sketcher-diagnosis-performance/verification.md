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
B006. T002's design contract and native observer/index probes are complete;
T003 tracing is implemented and validated in
[phase-tracing-report.md](phase-tracing-report.md). T004 setup deferral and native/
Python/model correctness are verified in
[diagnosis-deferral-report.md](diagnosis-deferral-report.md). Comparative performance
acceptance satisfies the user-approved indicative standard; see
[the T005 report](performance-validation-report.md). Bounded GUI checks pass.
T001 through T007 are complete; historical strict-gate uncertainty remains disclosed.
Current guidance is promoted to the [reference](../../reference/sketcher-performance-diagnostics.md)
and [runbook](../../runbooks/sketcher-performance-investigation.md).

## Acceptance And Traceability

| Requirement | Design | Tasks | Cases | State |
| --- | --- | --- | --- | --- |
| FR-001 | D-001 | T001, T002, T004 | AC-001 | One-setup invariant verified in T004 |
| FR-002 | D-001, D-003 | T001, T002, T004, T005 | AC-002 | Linux native/Python/model and bounded GUI verified T004/T005 |
| FR-003 | D-002 | T002, T003 | AC-003 | Native/process verified in T003 |
| FR-004 | D-002 | T002, T003 | AC-003 | Native/process verified in T003 |
| FR-005 | D-002 | T002, T003, T005 | AC-004, AC-006 | AC-004 verified; indicative AC-006 satisfied with uncertainty disclosed |
| FR-006 | D-003 | T001, T005 | AC-005, AC-006 | Indicative AC-005/AC-006 satisfied; former strict gates retained as historical |
| FR-007 | D-004 | T006, T007 | AC-007 | T006 attribution/ranked owned backlog verified; T007 promotion/review complete |

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

### Indicative comparisons and variability

The current policy is [timing-evidence-decision.md](timing-evidence-decision.md),
which supersedes mandatory percentage/confidence gates. Keep comparable optimized
baseline, instrumentation-only and candidate builds, unchanged settings, frozen
operations/cache/reset boundaries and actual execution/setup checks. Keep strict
correctness validation and reject mismatched/skipped-work samples.

Use warmups where meaningful and several repeated samples per configuration;
five measured samples is a reasonable starting point, not a statistical guarantee.
Alternate configurations where practical. Keep raw observations and report medians
and spread (MAD or range), including process/cache class and machine/build identity.
Bootstrap intervals are optional context. Existing 10/30-pair samples are sufficient;
no new run is needed solely to satisfy a confidence bound.

Accept indicative improvement when repeated timings and setup counts support a
useful reduction on a representative diagnosis-heavy workload, with unchanged
modelling results. Review the other workloads and tracing-disabled comparison for
apparent repeatable material slowdowns; investigate and record their disposition
if found. Report noisy controls honestly. Timing variability or overlap with the
former 10%/5%/2% thresholds does not block completion. This is not proof of no
regression, zero overhead or a universal speedup.

The original stricter collection and analysis are preserved in
[comparison-protocol.md](comparison-protocol.md) and
[performance-results.json](performance-results.json), with their original outcomes.
Do not relabel those historical statistical gates as passed.

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
- Validation: AC-001 through AC-006 backed by actual tests and indicative measurements
  under timing-evidence-decision.md; correctness and execution checks remain strict.
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
the baseline require uncertainty disclosure. The user-approved indicative timing
policy supersedes strict confidence gates; historical outcomes remain unchanged
and correctness/execution requirements are not relaxed.

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

## T004 Implementation Evidence

[diagnosis-deferral-report.md](diagnosis-deferral-report.md) maps C-001/C-002 to
private one-shot deferral, authoritative diagnostic notification, notified-input
refresh and exception recovery. AC-001 is established by native setup spans plus
independent tracing-off execution vectors. AC-002 has native/Python and frozen-model
coverage for index repair, diagnostic IDs/touched/ordering, external references,
scalar/list transactions, noRecomputes, direct changes, immediate post-undo/redo
solve/move, driving/reference transitions and thickness propagation/restoration.
Broader GUI panel/recovery checks and all matched elapsed budgets remain T005.

## T005 Original Strict-Gate Disposition (Superseded)

[performance-validation-report.md](performance-validation-report.md),
[performance-results.json](performance-results.json) and
[comparison-protocol.md](comparison-protocol.md) record 190 matched triples,
570 timed samples, 114 warmups and zero rejected execution/state pairs. Master
and plate-profile recompute exceed the 10% improvement target across their paired
intervals. Native 149/149, affected Python 97 tests (one existing skip), three-way
full model comparisons and nine GUI panel/recovery stages pass on Linux.

AC-005 and AC-006 do not pass overall: thickness/fresh-load regression and all
five repeated tracing-off-overhead gates remain inconclusive after 30 pairs.
T005 remains partial. Its owner must resolve measurement uncertainty on a
controlled host under a separately frozen follow-up protocol; the existing
sample bound must not be extended or targets weakened. T006 dependency and T007
closure remain blocked by this acceptance gap. No permission or waiver is inferred.

## Current T005 Acceptance - User-Approved Indicative Evidence

The user revised the timing objective on 2026-11-05; see
[timing-evidence-decision.md](timing-evidence-decision.md). Existing matched samples
and two-to-one setup counts indicate improvement: approximately 47% lower master
and 57% lower plate-profile median target recompute. Native/Python/model and bounded
GUI correctness checks pass. Other workloads and tracing-disabled results have
been reviewed; their variable estimates do not establish a consistent material
slowdown attributable to the change, nor prove its absence. AC-005/AC-006 are
satisfied under the revised criteria; T005 and T006 are complete.
The earlier strict confidence gates remain inconclusive historically. No raw data
was changed, no regression-free guarantee is made and no new measurement is needed.

R-005's earlier recommendation for strict confidence gates is superseded for timing
acceptance by the explicit user decision. Its uncertainty-disclosure concern remains:
retain raw results/spread and avoid claiming no regression from noisy data. R-001
through R-004 and R-006 correctness, execution and diagnostic requirements are unchanged.

## T006 Attribution And Routing Evidence

[remaining-cost-decisions.md](remaining-cost-decisions.md) records root-relative
interval unions, measured phase opportunities, limits and B001–B007 dispositions.
The [aggregate data](remaining-cost-attribution.json) retains calculations and
stage-labelled counters; [backlog details](../../backlog/sketcher-performance.md)
define owner, prerequisites, risks and exit checks. This satisfies AC-007 without
starting a new solver rewrite. Shape-dominated small sketches and mutation-specific
stalls remain distinct from unchanged complex-sketch recompute.

The updated frozen copy qualifies 13 sketches in separate trace-off/on processes:
39 untraced and 13 traced samples retain geometry/constraints/DoF and count exactly
one execute/setup each. All sketch shapes are valid; no initial object is Invalid.
The snapshot hash remains unchanged. The historical saved checkpoint hash matches
its replay receipt, but immediate pre-symmetry authority is still unverified and
assigned B006. No operator model/runtime was mutated and no saved mutation retried.
T006 attribution and T007 durable promotion/final review are complete.


## T007 Promotion, Review And Delivery

The [diagnostics reference](../../reference/sketcher-performance-diagnostics.md)
promotes the implemented one-setup boundary, notification/fallback semantics,
trace control/schema/counters, disabled behaviour, async/deferred lifetime and
measurement limitations. The [investigation runbook](../../runbooks/sketcher-performance-investigation.md)
promotes safe copied inputs, unique profiles, build/module identity, capture and
interval-union analysis, repeated indicative timing, correctness checks and recovery.
The [backlog](../../backlog/README.md) owns B001–B008; none starts automatically.

Independent expert source/test and documentation/operations reviews found no
blocking implementation defect. The documentation command/profile correction and
optional wording improvement are applied. Additional external-property observer
coverage is explicitly owned by B008; it is not represented as an executed test.
[The private delivery review](../../reviews/sketcher-diagnosis-delivery-review.md)
records findings, disposition, platform/GUI limits and public-preparation actions.
These AI-assisted reviews are not human contributor or upstream maintainer approval.

T007 validation checks document links, embedded Python/shell syntax, JSON,
backlog schema, lint/traceability and CRLF-aware whitespace. Source/tests/profile
helpers are unchanged from `dfd398f31c`; native/build/GUI evidence is retained
from T003–T006 rather than rerun for prose changes. Existing lint authoring
advisories do not establish an acceptance gap.

Commit lineage: T001 `1efd3713a8` (baseline `b6141a373c`), T002 `6a17625f55`,
T003 `9bc0eb91ff`, T004 `77ca0b6d94`, T005 root `27d524eaaa` / docs `43e66da`,
T006 root `dfd398f31c` / docs `79e9aa3`. T007 final and cleanup commits are
resolved in the [closure log](../../history/spec-closure-log.md) and
[archive index](../../history/spec-archive-index.md), avoiding a self-referential
commit hash in this document.

Disposition: retain this package in place as explicitly archived historical
requirements/evidence because profiling helpers consume `workloads.json` here.
Current knowledge has durable destinations; it is not governed by this temporary
package after closure. Feature changes remain on `perf/sketcher-diagnosis`; no
integration merge, push, install or public submission occurs. The prior crash
fix is already in integration base `36aa20c979`. Fresh full GUI/platform CI,
upstream issue alignment, human review and source-commit AI disclosure are owned
public-preparation work, not unclaimed local acceptance. Historical mutation replay
is deferred to B006 until immediate precursor authority exists. Timing uncertainty
and intermediate observer/addon compatibility limits remain disclosed.
