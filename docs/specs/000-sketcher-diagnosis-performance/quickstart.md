---
title: Sketcher diagnosis performance quickstart
doc_type: spec
artifact_type: quickstart
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# Quickstart

## Purpose

Resume the performance work without touching the operator's working CAD session.
T003's optional tracing is implemented; setup deferral and performance acceptance
remain later implementation work.

## Prerequisites

- FreeCAD branch `perf/sketcher-diagnosis`, based on `36aa20c979`.
- Matching branch in the separate `docs/` repository.
- Read [spec](spec.md), [requirements](requirements.md), [traceability](traceability.md),
  [design](design.md), [plan](plan.md), [research](research.md),
  [tasks](tasks.md) and [verification](verification.md), plus applicable AGENTS.md.
- Preserve existing unrelated docs edits. The package uses local repository templates.

## Steps

1. T001 through T003 are complete. Read the
   [baseline report](baseline-report.md), [manifest](workloads.json) and
   [contract](timing-invalidation-contract.md). Use Spec Lifecycle Manager context
   for T004, recheck source freshness through Agent Workbench and direct reads,
   and mark T004 in progress before implementation.
2. Verify fixture hashes and use disposable copies for further measurements.
   Preserve the frozen execution/reset/cache boundaries and verify counts on
   each new build; retain the distinction between no-op and actual recompute.
3. Follow T002's C-001 through C-006 decisions for trace enablement, intervals,
   counters, index validation and diagnostic notification handoff. T003 implements
   `FREECAD_SKETCHER_TRACE=1`; see [the tracing report](phase-tracing-report.md).
4. Implement T004 against the retained two-setup baseline, preserving notifications.
5. Build with `pixi run build -j 2`, run the discovered focused/affected checks,
   then use matched optimized builds for T005's alternating-pair/MAD/bootstrap
   measurements and bounded inconclusive-result protocol.
6. Record the follow-up decision in T006 and promote verified guidance in T007.

## Expected Results

The affected ordinary execute path diagnoses once with identical modelling
results. Optional timings distinguish setup, QR and solve costs. Performance
results include raw repeated samples and explicit budget dispositions.

## Use Optional Phase Tracing

Launch a task-owned process with `FREECAD_SKETCHER_TRACE=1` and its normal Log
sink enabled. New output starts with `SketcherPhase ` and contains JSON spans
and diagnosis counters. Unset the variable for tracing-off measurements; do not
try to toggle it after the process has sampled the environment. Existing Minimal
solver logs are independent. Use the timestamp/lane fields for overlapping QR;
do not add inclusive phase durations or infer chronology from mixed Message/Log
sink output. [phase-tracing-report.md](phase-tracing-report.md) documents fields,
limits and verification; its example is illustrative, not a performance result.

## Reproduce The T001 Baseline

The baseline runner is Linux/ELF diagnostic tooling, separate from the proposed
production tracing interface. Use the immutable private fixture identified in
[workloads.json](workloads.json), a fresh output directory and task-owned profiles.
Builds must finish before collecting timing samples:

```sh
pixi run build-release -j 2 --target FreeCADCmd Sketcher_tests_run PartDesign
pixi run clang++ -std=c++17 -O2 -fPIC -shared tools/profile/sketcher_call_counts.cpp -o /tmp/sketcher-call-counts.so -ldl -pthread
pixi run python tools/profile/run_sketcher_baseline.py --build-root build/release --output-root /tmp/sketcher-baseline-new --counter /tmp/sketcher-call-counts.so --fixture /path/to/disposable/refined-bottom-reflow.FCStd --pairs 10
```

The concrete Pixi build task preserves the job/target arguments in this checkout;
the composite `build` task did not forward them during T001. The runner rejects
failed processes, missing/stale receipts, modules outside the selected build and
changed execution vectors. It retains two warmups and ten alternating A/A pairs
per case; exclude warmup rows from reported timing. A/A results describe baseline
repeatability and cannot satisfy the candidate speedup or overhead gates.

For native tests, keep material libraries and configuration isolated too:

```sh
env XDG_CONFIG_HOME=/tmp/sketcher-native-config XDG_DATA_HOME=/tmp/sketcher-native-data XDG_CACHE_HOME=/tmp/sketcher-native-cache pixi run ./build/release/tests/Sketcher_tests_run
pixi run ctest --test-dir build/release -N -R '^SketchObjectTest.recompute'
```

See [baseline-report.md](baseline-report.md) for the tested build identity,
results, exception observations and the baseline counter's measurement limits.

## Cleanup

Close only task-owned disposable documents/processes. Keep operator originals,
preferences and current runtime intact. Retain private measurements outside Git;
remove temporary diagnostic code unless intentionally retained and reviewed.
Do not merge into the integration branch or install a snapshot as part of merely
creating or resuming the specification.
