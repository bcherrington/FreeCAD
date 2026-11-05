---
title: Sketcher diagnosis T001 baseline investigation
doc_type: spec
artifact_type: verification
status: archived
owner: local-developer
last_reviewed: 2026-11-05
---

# T001 Baseline Investigation

## Outcome And Scope

T001 - Reconcile source and establish the baseline is complete. The ordinary
forced-recompute path reproducibly performs one execute and two full setups.
Nine runnable workloads now have optimized baseline samples, deterministic native
execution vectors, reset/cache boundaries and reproducible commands. Historical
symmetry is explicitly unverified and routed to T006; no surrogate was substituted.
T002 can proceed with validation ownership and diagnostic observer contracts.

No production Sketcher source changed. Baseline revision is `b6141a373c`, on
`perf/sketcher-diagnosis`, including the native count fixture previously committed
there. The original source investigation started at `576c7c28ad`; production
sources are unchanged between these revisions. The crash fix remains included.
This baseline does not deliver the optimization or satisfy AC-001 through AC-007.

## Optimized Baseline

Release build: Clang 21.1.8, `-O3 -DNDEBUG`, Linux x86_64, Intel Core i7-11850H.
The private build-identity receipt records compiler, flags, dependency versions,
Pixi lock hash, CPU/affinity, exact module hashes and unchanged production sources.
All loaded App/Part/Sketcher/PartDesign paths were verified under `build/release`.
Fresh task-owned user/system configurations and XDG paths were used, with no
Sketcher preference overrides; compiled defaults belong to the pinned source.
Existing console Log sinks were disabled consistently. No task-owned compilation
or tests ran during the final measurement batch. Desktop activity, frequency and
thermal state were uncontrolled; these are local baseline observations.

Each case completed two warmups and ten alternating A/A pairs (20 timed samples),
with zero rejected pairs. Both A and B use the same baseline binary. Warmup rows
are retained but excluded below. Counter reset/snapshot, touch, fixture reset,
validation, transaction commit/undo, file close and restoration are outside the
timer. Constraint construction is inside the insertion timer. Fresh-load timing
covers `App.openDocument()` only, with startup/imports outside it; it uses a new
process/profile per sample. Fixture hashing can warm filesystem caches: neither
load case establishes a cold filesystem cache.

| Workload | Median (ms) | MAD (ms) | Executes / setups |
| --- | ---: | ---: | --- |
| Clean no-op | 0.014 | 0.000 | 0 / 0 |
| Forced synthetic circle | 0.550 | 0.014 | 1 / 2 |
| Forced master Sketch004 | 112.712 | 13.498 | 1 / 2 |
| Forced BottomPlateProfile | 314.625 | 33.775 | 1 / 2 |
| 16 scalar insertions + recompute | 7.277 | 0.116 | 1 / 18 |
| 16 list insertions + checked solve + recompute | 5.066 | 0.314 | 1 / 3 |
| Thickness edit + whole-document recompute | 254.106 | 14.067 | 0 / 0 |
| Reopened warm load | 986.502 | 87.338 | 0 / 19 |
| Fresh-process load | 468.658 | 9.824 | 0 / 19 |

Counts are totals for the defined operation. The complete per-sketch vectors are
in [workloads.json](workloads.json) and every raw sample. Load performs one setup
for Sketch004 and two for each of the nine other sketches, with zero ordinary
`execute()` calls. It is a restore path, not ordinary execute duplication.
Thickness change 0.8 -> 0.81 -> 0.8 performs no sketch execution/setup. Validation
checks unchanged sketch counts/DoF/diagnostic IDs, a valid single BottomPlate solid,
Pad/Pocket linked lengths and Z extents within 1e-7. Scalar/list insertion both
produce 16 constraints and 48 DoF; undo restores zero constraints.

## Variability And Interpretation

The baseline A/A effect uses `1 - median(B)/median(A)` and paired percentile 95%
intervals: 10,000 whole-pair bootstrap resamples, seed 576001, linear percentile
interpolation. Both per-label medians/MAD and raw ordering are retained. These are
repeatability observations for identical binaries, not speedup or budget decisions.

| Workload | A median / MAD (ms) | B median / MAD (ms) | A/A effect 95% interval |
| --- | ---: | ---: | ---: |
| Clean no-op | 0.014 / 0.000 | 0.014 / 0.000 | -24.55% to 1.72% |
| Forced synthetic circle | 0.546 / 0.014 | 0.570 / 0.030 | -8.95% to 0.52% |
| Forced master Sketch004 | 112.712 / 6.011 | 220.928 / 119.290 | -235.72% to 6.02% |
| Forced BottomPlateProfile | 395.758 / 114.908 | 310.735 / 20.582 | -2.89% to 37.65% |
| 16 scalar insertions + recompute | 7.310 / 0.191 | 7.216 / 0.097 | -3.82% to 2.74% |
| 16 list insertions + checked solve + recompute | 5.066 / 0.243 | 5.050 / 0.331 | -14.49% to 11.20% |
| Thickness edit + whole-document recompute | 262.967 / 16.418 | 246.189 / 14.912 | -4.04% to 55.50% |
| Reopened warm load | 986.502 / 51.500 | 963.000 / 108.300 | -10.87% to 14.61% |
| Fresh-process load | 466.060 / 8.357 | 471.442 / 12.287 | -3.91% to 3.13% |

The master and profile samples show substantial temporal variation; warm and
fresh load medians also differ. The baseline does not establish the cause, and
these timings must not be used to claim that one load/cache mode is faster.
T005 owns noise investigation and fresh interleaved matched-build comparisons,
including the unchanged bounded escalation/inconclusive protocol. No performance,
regression or tracing-overhead gate passes from this A/A capture. T003 owns the
production tracing implementation and its semantics; T005 evaluates its overhead.
Those later gates do not prevent completion of baseline establishment in T001.

## Count Mechanism And Limits

The native regression fixture in
`tests/src/Mod/Sketcher/App/SketchObjectRecompute.cpp` observes the existing Minimal
setup messages: clean recompute executes nothing; forced circle execution performs
two setups and preserves radius, center and DoF. T004 must update two to one.

The timed runner uses [sketcher_call_counts.cpp](../../../tools/profile/sketcher_call_counts.cpp),
a Linux/ELF `LD_PRELOAD` interposer for actual native `SketchObject::execute()` and
`Sketch::setUpSketch()` calls. Scoped owner tracking also wraps solve/object setup.
It resolves exact loaded modules, rejects owner/name/capacity failures, and uses
fixed counter storage without per-call formatting. Every timed sample is checked
against the frozen execution vector, including samples with detailed logging off.
The launcher rejects failed/missing/stale receipts and modules outside its build.

Interposition adds bounded call/counting overhead to these measurements. Its cost
has not been proven below AC-006, and it is not a portable production tracing API.
Both future comparison builds must use and validate the same counting seam; no
unmeasured overhead is subtracted. Optimized interposition was checked against
the known no-op and two-setup paths before the final batch. Production sources,
solver preferences and numerical algorithms remain unchanged.

## Exceptions And Regression Targets

The isolated Release probes repeat the earlier characterization and retain raw diagnostic IDs: valid line succeeds;
redundant Horizontal constraints report redundant `[2]`; Horizontal/Angle conflict
reports conflict `[1]`; `DefaultSolver=99` is recoverable. Invalid cases retain
Invalid/Touched state even when `doc.recompute()` returns a positive count. Process
survival or recompute's return count alone is insufficient correctness evidence.
These are characterization probes, not full malformed/index-transition coverage.

Source still follows execute -> geometry acceptance -> property callback -> setup,
then explicit solve -> setup. T002 must resolve the managed callback's omitted
index validation and conditional diagnostic constraint touch before choosing a
guard. T004/T005 own valid-invalid-repaired indices, diagnostic observer sequences,
edit-mode noRecomputes, direct property edits and immediate post-undo solve/move.

Executable native baseline filter:
`--gtest_filter='SketchObjectTest.recompute*'`.
External-link targets identified in `SketcherTests.TestSketcherSolver.TestSketcherSolver`:
`testReversedExternalGeometryLeavesSketchInPlace` and
`testSaveLoadWithExternalGeometryReference`, plus migration/removal/TNP cases in
that class. The runner's scalar/list transactions supply the baseline undo check;
they do not replace the later immediate solve/move regressions.

## Build, Test And Reproduction Commands

- `pixi run build-release -j 2 --target FreeCADCmd Sketcher_tests_run PartDesign`: passed.
- `env XDG_CONFIG_HOME=/tmp/freecad-sketcher-T001-complete/native-config XDG_DATA_HOME=/tmp/freecad-sketcher-T001-complete/native-data XDG_CACHE_HOME=/tmp/freecad-sketcher-T001-complete/native-cache pixi run ./build/release/tests/Sketcher_tests_run`: **109 passed**.
- `pixi run ctest --test-dir build/release -N -R '^SketchObjectTest.recompute'`: both native cases registered.
- Isolated Release external-reference tests: `testReversedExternalGeometryLeavesSketchInPlace` and `testSaveLoadWithExternalGeometryReference`: **2 passed**. Use `FreeCADCmd --python-path /home/bcherrington/Projects/CLion/FreeCAD/src/Mod/Sketcher --run-test SketcherTests.TestSketcherSolver.TestSketcherSolver.testReversedExternalGeometryLeavesSketchInPlace --run-test SketcherTests.TestSketcherSolver.TestSketcherSolver.testSaveLoadWithExternalGeometryReference` with the same isolated config/data/cache and explicit user/system config options. Initial attempts without `--python-path` failed module discovery; no tests ran in those attempts.
- Isolated Release valid/redundant/conflicting/invalid-default-solver probes: expected state and raw diagnostic IDs verified; receipt `edge-cases-result.json`.
- Prior Debug focused two cases and full 109-test suite passed (previous baseline commit).
- Black (line length 100), pinned repository clang-format dry-run, Python compile checks and task-owned whitespace checks passed. Root pre-commit's path allowlist excludes `tools/profile` and docs; skipped hooks are not format-validation evidence.

The first ambient-config Release native-suite invocation crashed in
`Materials::ModelLoader::dereference()` during sketch construction. The same binary
passed all 109 tests with isolated XDG config/data/cache. Backtrace and both logs
are retained; the user environment was not edited and a material-loader fix is
outside T001. Use isolated native-test paths for repeatability.

Concrete Pixi tasks are required here because the composite `build` task did not
forward target/job arguments during initial investigation. No task definition was
changed. Existing OCC deprecation/link-path warnings remain outside this slice.

The exact final command was:

```sh
pixi run python tools/profile/run_sketcher_baseline.py --build-root build/release --output-root /tmp/freecad-sketcher-T001-complete/release-final --counter /tmp/freecad-sketcher-T001-complete/call-counts.so --fixture /tmp/freecad-sketcher-T001/fixtures/refined-bottom-reflow.FCStd --pairs 10
```

Build the interposer and reproduce with fresh output paths using
[quickstart.md](quickstart.md#reproduce-the-t001-baseline). The launcher records
commands and selected environment for every child. Native fixtures require no
private CAD file. No GUI was launched, operator session touched, snapshot installed
or branch merged for T001.

## Fixture Authority And Retained Evidence

Frozen private fixture SHA256:
`55a0b21e57c979fe9e53dd89c073c47b2718257e7e7b10298375379094e3d4eb`.
Only an unchanged disposable copy was opened. The staged user-suggested file has
changed from its initial hash and remains excluded; [workloads.json](workloads.json)
records both identities. No original document was saved or modified.

Private evidence root:
`/home/bcherrington/Projects/ie/skunk-works/freecad-modelling-mcp/artifacts/freecad-crash-coordination/sketch-performance-static-analysis/T001-completion/`.
It retains `release-final/run.json`, per-child `raw.json`/`result.json`/console logs,
`summary.json`, `build-identity.json`, build/native logs, diagnostic source/binary
and the summarizer. Original temporary paths in receipts map to the corresponding
archived child paths. No model files or operator configuration were copied there.

Historical pilot evidence remains under sibling `T001-baseline/`. Its Debug-style
snapshot, concurrent compilation and touch-included timer make pilot elapsed
values ineligible for performance acceptance. The current optimized baseline
supersedes pilot timing, while retaining the diagnostic/exception observations.

Historical symmetry replay receipts and a mutation recipe exist, but the exact
saved pre-request checkpoint/hash is not established. T006 owns recovery and a
verified replay if possible; the case remains unverified. This explicit disposition
follows the measurement matrix and does not relax any performance acceptance gate.
