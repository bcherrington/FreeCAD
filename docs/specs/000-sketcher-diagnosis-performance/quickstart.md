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
T003 tracing, T004 setup deferral, T005 validation and T006 attribution are complete.
Timing evidence now uses the user-approved indicative standard; review
[the validation report](performance-validation-report.md) and
[timing-evidence-decision.md](timing-evidence-decision.md).

## Prerequisites

- FreeCAD branch `perf/sketcher-diagnosis`, based on `36aa20c979`.
- Matching branch in the separate `docs/` repository.
- Read [spec](spec.md), [requirements](requirements.md), [traceability](traceability.md),
  [design](design.md), [plan](plan.md), [research](research.md),
  [tasks](tasks.md) and [verification](verification.md), plus applicable AGENTS.md.
- Preserve existing unrelated docs edits. The package uses local repository templates.

## Steps

1. T001 through T006 are complete. Read the
   [baseline report](baseline-report.md), [manifest](workloads.json) and
   [contract](timing-invalidation-contract.md). Use Spec Lifecycle Manager context
   for T007, recheck source freshness through Agent Workbench and direct reads,
   and mark T007 in progress before promotion/review/closure. Read
   [remaining-cost-decisions.md](remaining-cost-decisions.md) and the
   [owned backlog](../../backlog/README.md); proposals do not start automatically.
   The existing data satisfies T005's revised indicative timing criteria; no
   additional confidence-bound measurement is required.
2. Verify fixture hashes and use disposable copies for further measurements.
   Preserve the frozen execution/reset/cache boundaries and verify counts on
   each new build; retain the distinction between no-op and actual recompute.
3. Follow T002's C-001 through C-006 decisions for trace enablement, intervals,
   counters, index validation and diagnostic notification handoff. T003 implements
   `FREECAD_SKETCHER_TRACE=1`; see [the tracing report](phase-tracing-report.md).
4. Read [the deferral report](diagnosis-deferral-report.md). Preserve the pinned
   T003 two-setup baseline and compare it with the candidate in equivalent builds.
5. If code changes in further work, build with `pixi run build -j 2` and run
   relevant checks. Use comparable optimized timings for indications of improvement;
   disclose variability and investigate apparent repeatable material slowdowns.
6. T006 decisions are recorded; promote verified guidance and reconcile review/
   delivery in T007. Preserve the historical replay gap and uncertainty limits.

## Expected Results

The affected ordinary execute path diagnoses once with identical modelling
results. Optional timings distinguish setup, QR and solve costs. Performance
results include repeated comparable samples, spread and explicit limitations.

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
per case; exclude warmup rows from reported timing. A/A results describe one configuration's repeatability and cannot satisfy the candidate speedup or overhead gates.
The default `--configuration baseline` asserts the original count vectors; select
`--configuration candidate` for T004 binaries. This selects expected counts,
not a matched baseline/candidate comparison. Candidate ordinary recompute expects
one setup; scalar/list insertion expect 17/2 rather than baseline 18/3.

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

## Matched T005 Comparisons

Use `tools/profile/run_sketcher_comparison.py` with explicit baseline,
instrumentation-only and candidate runtime roots; the executable example is in
[performance-validation-report.md](performance-validation-report.md). The
[comparison protocol](comparison-protocol.md) documents the alternating triples,
reset handshake, affinity and original statistical analysis. That analysis is
historical: the current indicative acceptance policy does not require a new
controlled-host cohort or confidence gate. Preserve the existing raw data and
separately identify any optional future measurements.
Run `pixi run python -m unittest discover -s tools/profile -p test_sketcher_comparison.py -v`
for the analytical/controller checks, and Black with 100 columns for these helpers.
