---
title: Sketcher T005 correctness and matched performance validation
doc_type: spec
artifact_type: verification
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# T005 Performance Validation

## Status

T005 is complete under the user-approved indicative timing standard in
[timing-evidence-decision.md](timing-evidence-decision.md). Existing observations
indicate roughly 47% lower master and 57% lower plate-profile median target
recompute, supported by two-to-one setup counts and passing correctness checks.
Other timings and tracing-disabled overhead remain variable; no guaranteed
regression or overhead bound is claimed. The original strict-gate outcomes below
remain unchanged and are historical, not current acceptance blockers.

## Reproducible Configurations

The [comparison protocol](comparison-protocol.md) fixes the variants, scheduling,
execution/reset checks, machine-affinity control and unchanged statistical gates.
The frozen [T001 manifest](workloads.json) remains authoritative for workloads.
Source identities are baseline `6a17625f55`, instrumentation-only `9bc0eb91ff`
and candidate `77ca0b6d94`. Runtime module SHA-256 values are:

| Variant | Sketcher.so SHA-256 |
| --- | --- |
| Baseline | `5c12b66b394d4d7a5ed2e54c856b9f8d89cc0f49c2f153487763299d01bcdf79` |
| Instrumentation only | `3a2876d9e3a3c3696e2ad88689879d95d35c7333b2c1e2ed078a3859cc3c9c3b` |
| Candidate | `72a7a8131b781a20e2a13b7f057dff4ad183fc2a7e3288531964495ecfcaa270` |

The historical modules use isolated objects compiled from detached source
worktrees with the configured Release commands; the candidate is the existing
Pixi Release target. Generated wrappers and dependency libraries are unchanged.
Private build logs, exact command lists, source hashes and runtime identities
make this distinction explicit. Baked FreeCAD version strings refer to shared
generated metadata and are not used to identify the historical module sources.

The controller `tools/profile/run_sketcher_comparison.py` schedules all three
configurations and reuses the existing worker's operations and timing boundaries.
Its IPC acknowledgement follows validation and reset, preventing one worker's
untimed undo/restoration from overlapping another worker's timer. The original
A/A runner remains available and defaults to its original behaviour.

## Correctness Evidence

The unchanged candidate native source and regression-test hashes match the
T004 archive. Current Release verification runs pass:

- Full native Sketcher suite: 149/149.
- Full affected `TestSketcherApp` Python suite: 97 run, 96 passed, one existing
  secant-support skip (PR 9044); zero failures/errors. This includes solver,
  fillet, expressions, coincident validation, carbon-copy reverse mapping,
  internal faces and ellipse tests.
- Full private frozen-model snapshots for all three Release runtimes: baseline/
  candidate and baseline/instrumentation each pass 153,993 state checks.
  Geometry types/parameters/construction flags, complete constraint identities/
  values/flags, DoF, diagnostic IDs, expression bindings, shape validity, topology,
  bounding boxes, area/volume and vertex multisets match at absolute 1e-7 and
  relative 1e-9 numeric tolerances. Counts/identities/flags are exact.
- Master remains 104 geometries/247 constraints and 0 DoF; plate profile remains
  118/241 and 47 DoF. BottomPlate remains valid with one solid and 103 faces;
  source-linked Pad/Pocket thickness follows 0.8 -> 0.81 -> 0.8 and restores.
- The T004 [deferral report](diagnosis-deferral-report.md) provides the unchanged
  native/Python evidence for invalid/repaired internal/external indices,
  diagnostic IDs/touched/observer ordering, scalar/list transaction undo,
  noRecomputes, direct properties, immediate post-undo/redo solve/move and
  driving/reference transitions. T005 does not broaden the one-setup invariant.

### GUI panel and failure recovery

A task-owned FreeCAD Debug process on isolated Xvfb display `:95` loads the
candidate Sketcher App module. The existing GUI consumers have unchanged source
across these Sketcher-only implementation commits. The broad GUI rebuild was
stopped after it expanded to 617 build steps through generated and dependent GUI
code; no successful full
GUI rebuild is claimed. These are functional checks of the existing consumer
binaries with the current candidate App module, with module hashes retained.

Nine scripted stages and exact-window captures verify actual visible labels:

| Stage | Solver-panel status / link |
| --- | --- |
| Healthy horizontal line | Under-constrained / 3 Degrees of Freedom |
| Duplicate Horizontal | Redundant constraints / (2) |
| Remove duplicate | Under-constrained / 3 Degrees of Freedom |
| Add conflicting Angle | Over-constrained / (1) |
| Remove Angle | Under-constrained / 3 Degrees of Freedom |
| Radius on line | Malformed constraints / (2) |
| Remove Radius | Under-constrained / 3 Degrees of Freedom |
| Ordinary recompute error outside edit, then open editor | Redundant constraints / (2) |
| Repair that error in editor | Under-constrained / 3 Degrees of Freedom |

The labels and native diagnostic IDs are asserted independently. Visual inspection
of the solver-panel montage confirms status colours, links and recovery; the full
window capture also shows constraints and geometry. Captures were restricted to
PID 148839, XID `0x20003c`, class FreeCAD on the task display. The disposable
document/process closed successfully. No operator GUI/document/preferences were
used. Cross-platform GUI builds and arbitrary addon consumers remain unverified.

## Additional Checks And Attribution

Empty and reference-only sketches also pass in all three runtimes: DoF remains
0 and 3 respectively, with baseline/instrumentation setup count two and candidate
one. Reference geometry/value/driving state and shape validity are retained.
Retrospective checks confirm equal solver preferences for all 36 fresh-load
triples, including warmups, and identical hashes for all 11 common shared libraries.

Separate tracing-on attribution runs emit complete records with no dropped spans.
Master/profile setup counts are two in instrumentation-only and one in candidate.
Every corresponding diagnosis retains the same stage-labelled counters:

| Model | Active driving constraints | Allocated rows x columns | Effective rows x columns | Rank J / JT | QR |
| --- | --- | --- | --- | --- | --- |
| Master | 246 | 482 x 481 | 481 x 481 | 481 / 481 | dense |
| Plate profile | 241 | 695 x 742 | 695 x 742 | 695 / 695 | dense |

One allocated dense payload is 1,854,736 bytes for master and 4,125,520 for profile;
these estimates are not total process/QR workspace memory. Diagnosis occupies most
of setup in these attribution runs. Their elapsed values are not matched latency
samples and cannot establish a speedup or overhead gate; inclusive/overlapping
spans must not be added as independent wall time. T006 can consume the retained
records when its prerequisite acceptance is resolved.

The same active native counter seam is included in every acceptance configuration.
A separate 10-pair calibration uses 100 forced circle executes per sample, outside
the acceptance batches. Active seam versus no preload has median difference
+0.006 ms per operation and paired 95% interval [-0.233, +0.400] ms. Inactive
preload versus no preload has +0.114 ms, interval [+0.031, +0.718] ms. These noisy
synthetic calibration estimates do not prove a universal per-hook cost or zero
overhead. No calibration value is subtracted from acceptance measurements.

Six analytical/controller tests pass, covering paired covariance, distinct 2%/5%
allowances, the 1 ms floor, inconclusive intervals despite an unacceptable point
median, targeted execution vectors and owned-process cleanup on startup failure.
After collection, controller hardening added startup cleanup, optional governor
metadata and explicit fresh-load preference comparison. All recorded fresh-load
preferences were checked retrospectively; collection-source snapshots are retained.
Timing boundaries, scheduling and statistical calculations are unchanged.

## Original Measurement Results And Strict-Gate Outcomes

### Candidate latency versus baseline

Times are milliseconds; parentheses show MAD. Confidence intervals are paired
95% intervals on speedup. Historical gate decisions use full precision in
[performance-results.json](performance-results.json), which also records every
batch and the pooled intervals. Negative speedup means higher median latency.

| Case | Pairs | Baseline ms (MAD) | Candidate ms (MAD) | Speedup % [95% interval] | Former regression gate |
| --- | --- | --- | --- | --- | --- |
| clean_noop | 10 | 0.038 (0.001) | 0.040 (0.004) | -7.47 [-38.86, 2.28] | pass |
| forced_synthetic | 10 | 1.854 (0.112) | 1.964 (0.068) | -5.96 [-11.90, 7.08] | pass |
| forced_master | 30 | 108.424 (16.370) | 57.388 (3.389) | 47.07 [44.44, 68.46] | pass |
| forced_profile | 30 | 600.911 (301.320) | 258.010 (111.647) | 57.06 [42.02, 64.88] | pass |
| scalar_insertion | 10 | 7.136 (0.119) | 6.990 (0.097) | 2.05 [-0.24, 4.39] | pass |
| list_insertion | 10 | 4.797 (0.190) | 4.655 (0.110) | 2.96 [-1.50, 13.83] | pass |
| thickness | 30 | 285.236 (46.910) | 281.430 (34.146) | 1.33 [-43.04, 44.48] | inconclusive |
| warm_load | 30 | 417.813 (11.350) | 423.013 (14.029) | -1.24 [-3.51, 2.10] | pass |
| fresh_load | 30 | 945.234 (455.663) | 954.016 (408.293) | -0.93 [-54.74, 46.52] | inconclusive |

### Instrumentation disabled overhead versus baseline

The interval is on difference minus the allowed overhead, in milliseconds.
It passes only when its upper endpoint is at or below zero. This includes the
same active native counter seam in both configurations.

| Case | Instrumentation ms (MAD) | Excess overhead 95% interval ms | Former overhead gate |
| --- | --- | --- | --- |
| clean_noop | 0.041 (0.006) | [-1.002, -0.987] | pass |
| forced_synthetic | 2.064 (0.154) | [-1.113, -0.469] | pass |
| forced_master | 106.881 (7.573) | [-76.927, 1.915] | inconclusive |
| forced_profile | 605.314 (300.032) | [-70.672, 50.095] | inconclusive |
| scalar_insertion | 7.197 (0.172) | [-1.112, -0.659] | pass |
| list_insertion | 4.822 (0.083) | [-1.425, -0.759] | pass |
| thickness | 304.763 (69.911) | [-16.097, 121.078] | inconclusive |
| warm_load | 426.238 (14.676) | [-14.354, 11.914] | inconclusive |
| fresh_load | 894.208 (406.957) | [-466.977, 258.898] | inconclusive |

Across all batches: 190 matched triples, 570 timed samples, 114 warmups and zero
rejected execution/state pairs. Four cases needed one batch; master, profile,
thickness and both load cases used all three batches. No sample or unfavourable
batch was dropped. The two main diagnosis workloads pass the 10% improvement
target across their whole intervals. Thickness and fresh-load regression gates
remain inconclusive; all five repeated overhead gates remain inconclusive.
AC-005 and AC-006 therefore do **not** pass overall.

The target-only master/profile timers include sketch execute and prerequisite
work, as frozen in T001; they do not measure downstream whole-document propagation.
The separate thickness timer includes mutation and whole-document recompute and
executes no sketches. Load results retain their fresh-process/warm-reopen cache
classes; none is called a cold-filesystem measurement.

### Original Investigation And Disposition (Superseded)

The spread is material: some batches contain large latency excursions even in
the unchanged thickness/load controls, and pooled intervals still cross budgets.
The host remains shared, uses powersave/turbo policy and is not thermally or
scheduler isolated. These observations establish measurement uncertainty; they
do not identify a specific causal source or prove zero tracing overhead. Common
library hashes, solver settings, module origins, source identities and actual
execution/reset vectors agree. No correctness discrepancy or skipped-work shortcut
explains the observed diagnosis speedup.

The prescribed 30-pair bound is exhausted for every unresolved case. Do not collect
extra samples under this protocol, reduce thresholds, infer no regression from a
non-significant result, or accept from point medians. T005 retains ownership of noise attribution
and a separately specified controlled-host benchmark follow-up. T006 remains
dependent on its acceptance. T007 cannot close delivery while AC-005/AC-006 remain
inconclusive. These gates are not waived by completion of the measurement runs.

## Commands And Private Receipts

Build reconstruction uses private `build-variants.py` and detached worktrees,
with exact configured compile/link commands and two jobs. The lower-level module
build avoids changing the operator's working source or reconfiguring the existing
build tree. T004's successful normal Pixi candidate Release/Debug builds remain
applicable; no production source changes are made in T005.

The comparison command is:

```sh
pixi run python tools/profile/run_sketcher_comparison.py --baseline-root /path/to/runtimes/baseline --instrumentation-root /path/to/runtimes/instrumentation --candidate-root /path/to/runtimes/candidate --output-root /path/to/new-batch --counter /path/to/call-counts.so --fixture /path/to/disposable/refined-bottom-reflow.FCStd --cpus 4,5
```

Use `--batch 2` or `--batch 3` with explicit repeated `--case` arguments only for
cases requiring the bounded additional batches. Build/test/snapshot/GUI smoke
results collected during preparation are correctness evidence, not latency samples.

Private evidence is retained in the crash-coordination artifacts under
`sketch-performance-static-analysis/T005-performance-validation/`. It includes
build reconstruction scripts/commands/logs/identities, full snapshots/comparison
receipts, native XML and Python results, GUI macro/state/captures, and benchmark
raw/controller/analysis receipts. Full model geometry remains outside Git.

## Final Quality And Scope Checks

Black (100 columns), six analytical/controller tests, Python parsing, local
file/anchor links, JSON parsing and CRLF-aware whitespace checks pass. Package
lint has zero errors and the same two documented EARS/canonical-context advisories.
Focused pre-commit skips the profiling paths because they are outside its configured
allowlist; explicit formatting and tests cover them. A post-hardening no-op controller
smoke and original A/A-worker compatibility smoke pass and are excluded from all
acceptance statistics. The measured worker and statistical/vector functions match
the archived collection source; public statistics exactly match private results.
No production source changed, no integration merge/install occurred, and unrelated
working-tree edits are preserved. T005 tooling and documentation remain uncommitted.

## Current Indicative Acceptance And Next Step

On 2026-11-05 the user requested an indication of improvement instead of strict
performance measurement. This explicitly supersedes the earlier blocking
confidence/percentage criteria; the data and former strict outcomes above are
unchanged. The actual two-to-one setup reduction, repeated master/profile timing
reductions and unchanged modelling results satisfy AC-005. The baseline versus
instrumentation-only runs, calibration and disabled-path checks provide AC-006's
indicative overhead review, with uncertainty disclosed. Some control medians move
up and others down; these measurements do not isolate a consistent material
slowdown attributable to the change. They also do not prove zero overhead or
absence of regression. Clear repeatable slowdowns observed later should be
investigated, rather than dismissed as noise.

T005 is complete. A controlled-host rerun is optional follow-up, not a prerequisite
for T006. T006 can now attribute remaining costs and rank further work. T007 still
owns final review/promotion/closure. No additional timing runs, application source
changes or edits to the operator's model were needed for this reassessment.
