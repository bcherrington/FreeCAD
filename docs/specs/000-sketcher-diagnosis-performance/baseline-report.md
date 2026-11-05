---
title: Sketcher diagnosis T001 baseline investigation
doc_type: spec
artifact_type: verification
status: draft
owner: local-developer
last_reviewed: 2026-11-05
---

# T001 Baseline Investigation

## Source And Scope

Investigation started at FreeCAD `576c7c28ad`, on `perf/sketcher-diagnosis`.
No production Sketcher source changed. A baseline-only native fixture was added
in `tests/src/Mod/Sketcher/App/SketchObjectRecompute.cpp` and registered in the
adjacent `CMakeLists.txt`. It observes the existing Minimal setup diagnostic
messages rather than adding production counters. T003 must provide the bounded
count-only mechanism for tracing-off acceptance; T004 must update the targeted
baseline count from two to one while retaining correctness assertions.

The current source still follows execute -> geometry acceptance -> property
callback -> setup, followed by explicit solve -> setup. The managed callback's
index-validation difference and conditional diagnostic touch remain unresolved
implementation contracts for T002; this investigation does not approve a guard.

## Verified Results

| Check | Result | Interpretation |
| --- | --- | --- |
| Native clean synthetic circle recompute | 0 executed objects, 0 setups | No-op control |
| Native forced unchanged circle recompute | 1 executed object, 2 setups | Duplicate-path reproduction; radius, center and DoF preserved |
| Isolated synthetic pilot | 20 forced samples, 2 setups each | Diagnostic observation only |
| Isolated frozen plate-profile pilot | 20 forced samples, 2 setups each | 118 geometry and 241 constraints retained |
| 16 scalar Horizontal insertions plus recompute | 18 setups, 16 constraints, 48 DoF | Immediate scalar solving multiplies work |
| One list of 16 Horizontal constraints, checked solve, recompute | 3 setups, 16 constraints, 48 DoF | Wrapper remains deferred; explicit checked solve counted separately |
| Undo of either insertion transaction | 0 remaining constraints | Python undo returns None; assert resulting state rather than its return |
| Valid, redundant, conflicting and invalid-default-solver probes | Processes complete; bad cases retain Invalid/Touched state | Recompute returns a positive count even on failure; assert state and diagnostics |

Raw Python diagnostic arrays were retained without index normalization. They are
baseline observations, not proof that T002's observer/touched-state contracts or
all malformed/index-transition scenarios are covered.

## Build And Test Evidence

- `pixi run build-debug -j 2 --target Sketcher_tests_run`: passed.
- `pixi run ./build/debug/tests/Sketcher_tests_run --gtest_filter='SketchObjectTest.recompute*'`:
  2 passed.
- `pixi run ./build/debug/tests/Sketcher_tests_run`: 109 passed.
- `pixi run ctest --test-dir build/debug -N -R '^SketchObjectTest.recompute'`:
  both new cases registered.
- Pinned repository clang-format dry-run on the new C++ file: passed.
- Focused pre-commit hooks on the native fixture and CMake registration: passed.
- `git diff --check` on task-owned changes: passed.

The initial composite `pixi run build -j 2 --target Sketcher_tests_run` did not
forward its arguments to `build-debug`; its actual command lacked the target and
job limit and began a broad dependency rebuild. The task-owned Ninja process was
interrupted and resumed with the concrete Pixi task above. This is a focused
command-routing diagnostic exception, not a change to `pixi.toml`. Existing OCC
API deprecation and linker-path warnings were not changed by this test fixture.

## Fixtures And Evidence Location

Frozen private fixture SHA256:
`55a0b21e57c979fe9e53dd89c073c47b2718257e7e7b10298375379094e3d4eb`.
The previously staged file no longer matches its recorded hash and is excluded;
its observed hash and status are in [workloads.json](workloads.json).
No operator document/session was changed. Scratch documents ran in separate
FreeCADCmd processes with private config/data/cache paths and no socket replay.

Private scripts, binary hashes, raw samples, logs, case results and limits are
retained under the existing crash-coordination artifacts at
`sketch-performance-static-analysis/T001-baseline/`. Synthetic native tests do
not depend on these private paths or model files.

## Measurement Limits And Remaining T001 Work

These are pilot observations, not performance acceptance. The current Release
tree is stale/incomplete. The pilot used an installed Debug-style snapshot,
existing diagnostic logging and concurrent dependency compilation. Its forced
elapsed region also included touch(), whereas the frozen acceptance manifest
puts touch/reset outside timing. Raw pilot samples are explicitly ineligible
for AC-005/AC-006, and no comparative speedup or overhead claim is made.

[workloads.json](workloads.json) freezes the initial operation/reset boundaries.
Clean and forced synthetic counts and scalar/list counts are now verified.
Restore/load and whole-document propagation execution vectors still need to be
frozen from measurements. The historical symmetry precursor remains unverified.
Comparable optimized baseline capture, tracing-off counter validation and
acceptance-quality paired data remain outstanding; only diagnostic pilot data
currently exists. Do not mark T001 complete or advance dependent implementation
on the strength of the count reproduction alone.
