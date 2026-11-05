---
title: Investigate Sketcher performance safely
doc_type: runbook
status: code-derived
owner: local-developer
last_reviewed: 2026-11-05
applies_to:
  - perf/sketcher-diagnosis
  - Linux FreeCADCmd diagnostic tooling
---

# Investigate Sketcher Performance

## Purpose

Identify whether sketch time is spent in setup/diagnosis, QR, numerical solving,
geometry/shape work or downstream recompute. Obtain an indication of improvement
with comparable repeated observations while keeping correctness and input identity
strict. Do not change operator geometry, solver defaults or live unknown operations.

## When To Use

Use for slow complex sketches, a proposed performance change, or a reported long
constraint edit. Distinguish a clean no-op, forced unchanged sketch execute,
whole-document dimensional edit, load, scalar/list insertion and mutation replay.
A current unchanged sketch is not a substitute for a historical mutation precursor.

## Prerequisites

- A branch containing optional tracing `9bc0eb91ff`; one-setup behaviour additionally
  requires `77ca0b6d94`. An operator installation is not updated by a local build.
- Read the [diagnostics reference](../reference/sketcher-performance-diagnostics.md).
- Matching source/build/dependency identity and task-owned processes/configuration.
- Pixi tasks and Python; the count interposer/comparison helpers require Linux/ELF.
  The production trace is not limited to that interposer, but local verification
  was on Linux only.
- Read-only access to an input artifact, enough space for disposable copies and
  private output. Never attach to or stop an operator process for this procedure.

## Inputs

Record source commit and dirty patch, compiler/dependency/build flags, loaded module
paths/hashes, fixture-copy hash, internal sketch name, operation/reset boundary,
solver settings, trace mode and process/cache class. Record machine/load/affinity
if material. Keep raw model state/logs private; publish aggregate counts/timings.

## Linked Docs

| Document | Why It Matters |
| --- | --- |
| [Diagnostics reference](../reference/sketcher-performance-diagnostics.md) | Control, fields, invariants and interpretation |
| [Backlog](../backlog/README.md) | Owned proposals/deferred work; not automatic changes |
| [Follow-up details](../backlog/sketcher-performance.md) | Risk and exit checks for QR, shape, copies, metadata, reuse and replay |
| [Historical collection protocol](../specs/000-sketcher-diagnosis-performance/comparison-protocol.md) | Exact prior collection method, not current strict acceptance criteria |

## Config And Code Touchpoints

| Area | Path or setting | Use | Required validation |
| --- | --- | --- | --- |
| Opt-in | `FREECAD_SKETCHER_TRACE=1` before launch | Enable phase collection; normal Log sink must also emit | Complete JSON spans/counters; no new records when disabled |
| Build | `pixi.toml` concrete configure/build tasks | Build affected targets; use `-j 2` | Selected module identity and focused tests |
| Operation/counts | `tools/profile/sketcher_call_counts.cpp` | Optional Linux native execute/setup seam in every compared sample | Bind exact native modules; reject unexpected vectors |
| A/A diagnostics | `tools/profile/run_sketcher_baseline.py` | Existing frozen cases and single-configuration repeatability | Correct configuration/counts, reset and state checks |
| Matched diagnostics | `tools/profile/run_sketcher_comparison.py` | Three prepared equivalent runtimes; explicit case scheduling | Correct input/settings/counts/state, raw results retained |
| Code | SketchObject, Sketch, planegcs/GCS | Attribute phases without changing algorithms | Geometry/DoF/diagnostics/undo/reference/shape checks |

## Procedure

### 1. Freeze the input without using the live file

Create a new task directory. Copy a potentially edited FCStd and verify its source
identity/stat did not change during copying; validate the copied ZIP and hash it.
For example, run this with repository-independent Python and replace the source:

```python
from pathlib import Path
import hashlib
import shutil
import tempfile
import zipfile

source = Path('/path/to/editable.FCStd')
work = Path(tempfile.mkdtemp(prefix='sketcher-investigation-'))
copy = work / 'input.FCStd'
before = source.stat()
shutil.copyfile(source, copy)
after = source.stat()
assert (before.st_dev, before.st_ino, before.st_size, before.st_mtime_ns) == (
    after.st_dev, after.st_ino, after.st_size, after.st_mtime_ns
), 'Source changed during copy: discard only this task copy and retry when stable'
with zipfile.ZipFile(copy) as archive:
    assert archive.testzip() is None
print(copy, hashlib.sha256(copy.read_bytes()).hexdigest())
```

Do not open the original in the diagnostic process, save back over it or silently
refresh a frozen input mid-comparison. Each mutation sample gets a fresh disposable
precursor. An archive hash identifies bytes; it does not prove a historical
pre-operation state. Preserve the original receipt and distinguish newer snapshots.

### 2. Prepare an appropriate build and profile

Normal debug verification uses `pixi run build -j 2`. For indicative optimized
observations use the concrete Release tasks, which forward target/job arguments:

```sh
pixi run configure-release
pixi run build-release -j 2 --target FreeCADCmd Sketcher_tests_run PartDesign
```

Do not install over the operator snapshot. Give each task process separate
`--user-cfg`, `--system-cfg` and `XDG_CONFIG_HOME`, `XDG_DATA_HOME`,
`XDG_CACHE_HOME` paths. Keep document caches/reset behaviour equivalent. Rebuild
and collect matching modules rather than mixing native outputs from different trees.

For before/after comparisons prepare separate builds from the intended source
revisions in isolated worktrees, using the same compiler, flags and dependencies.
The historical configuration identities are baseline `6a17625f55`, tracing-only
`9bc0eb91ff` and optimized `77ca0b6d94`. Build each with the concrete tasks above;
run the current diagnostic controller from the delivery checkout, passing explicit
runtime roots. Do not switch the working source beneath an active build/process.
Do not time workloads during compilation or other task-owned validation.

### 3. Capture one forced trace before interpreting a slow sketch

Set the trace variable before starting a new process. A headless script should
import `Part`, `PartDesign` and `Sketcher`, open only the copied model, and clean
recompute outside the selected target window. Validate the initial document and
record geometry/constraint/DoF/diagnostic/shape state before and after.

A minimal trace-only operation is:

```python
import FreeCAD as App
import Part
import PartDesign
import Sketcher

for observer in App.Console.GetObservers():
    App.Console.SetStatus(observer, 'Log', False)
doc = App.openDocument('/path/to/task-copy.FCStd')
doc.recompute()
sketch = doc.getObject('InternalSketchName')
assert sketch is not None and sketch.isDerivedFrom('Sketcher::SketchObject')
assert not [obj.Name for obj in doc.Objects if 'Invalid' in obj.State]
sketch.touch()
for observer in App.Console.GetObservers():
    App.Console.SetStatus(observer, 'Log', True)
doc.recompute([sketch])
for observer in App.Console.GetObservers():
    App.Console.SetStatus(observer, 'Log', False)
App.closeDocument(doc.Name)  # discard; no save
```

Launch the script with a task-owned profile (replace the paths):

```sh
sketch_probe_output="$(mktemp -d /tmp/sketcher-probe-XXXXXX)"
env FREECAD_SKETCHER_TRACE=1 XDG_CONFIG_HOME="$sketch_probe_output/config" XDG_DATA_HOME="$sketch_probe_output/data" XDG_CACHE_HOME="$sketch_probe_output/cache" pixi run ./build/release/bin/FreeCADCmd --user-cfg "$sketch_probe_output/user.cfg" --system-cfg "$sketch_probe_output/system.cfg" /path/to/trace-probe.py > "$sketch_probe_output/trace.log" 2>&1
```

The command creates a fresh output directory before redirection. This minimal snippet does not
replace state comparisons or count-checked timing harnesses. It only captures a
known operation for attribution. Do not use its trace-on duration as untraced latency.

Parse lines containing `SketcherPhase ` as JSON. Require schema/identity and valid
parent relationships, closed spans, zero drop counts and the expected root/phase
records. Inspect outcome/status and final model state separately. Eligible ordinary
execute has one setup; supported fallback/observer edits and other operations can
legitimately require more. A setup can contain multiple actual diagnoses.

### 4. Read wall time correctly

Use the actual outer root execute, not the sum of nested `operation` records.
Offsets share a root origin; compute unions of overlapping phase intervals,
clipped to that root. Never add setup plus its diagnosis, or J/JT plus dispatch/
join, as independent time. Group by process, operation and sketch. Use logical lane
and caller-thread flags to recognize deferred execution within join.

Read counter stages explicitly: input driving constraints are not expanded equations;
allocated rows can differ from effective rows; payload bytes are one dense allocation
estimate, not peak memory. Null/not-run rank is not rank zero. A sparse method can
still include dense assembly/conversion. Long main solve or redundancy solve is
not interchangeable with setup time; the existing Python solve timer excludes setup.

### 5. Collect indicative timings with tracing disabled

Start another process with `FREECAD_SKETCHER_TRACE` unset. Turning off the Log sink
alone does not stop trace collection overhead. Use warmups and several repeated
samples; report median and spread, with scope/settings/input identity. Preserve
actual execution/setup checks in every compared sample and reject skipped-work,
state/reset or module-origin mismatches. Keep untimed restoration/undo/validation
from overlapping another configuration's timed operation.

The Linux interposer can be built with:

```sh
pixi run clang++ -std=c++17 -O2 -fPIC -shared tools/profile/sketcher_call_counts.cpp -o /tmp/sketcher-call-counts.so -ldl -pthread
```

The existing frozen-case runner rejects any private fixture other than its pinned
`55a0b21e...` input. It is not an arbitrary-FCStd profiler: do not feed the newer
`527589a0...` model or weaken its assertion. A new fixture needs its own recorded
operation/reset/state/count expectations and a disposable harness.

For the original fixture and three prepared runtimes:

```sh
pixi run python tools/profile/run_sketcher_comparison.py --baseline-root /path/to/baseline/build/release --instrumentation-root /path/to/tracing/build/release --candidate-root /path/to/candidate/build/release --output-root /path/to/new-output --counter /tmp/sketcher-call-counts.so --fixture /path/to/pinned-task-copy.FCStd
```

The helper retains the original strict diagnostic classifications and fixed batch
size. Those classifications are not current completion requirements. Current timing
acceptance asks for an indication of useful improvement, equivalent actual work,
spread/caveats and review of apparent repeatable material slowdowns. Formal percentage
or confidence-bound gates and an isolated host are not prerequisites. Keep old
results intact; do not relabel former inconclusive gates as passed or claim no
regression/zero overhead from noisy observations. Correctness is never relaxed.

### 6. Attribute and route the next investigation

Separate target execute from downstream whole-document propagation and bridge
validation. For large sampled sketches, QR was the principal opportunity; smaller
outlines had shape-heavy work. Async dispatch and metadata were smaller envelopes.
Do not infer a universal best optimization from one shape or from raw constraint count.
Use the [backlog exit criteria](../backlog/sketcher-performance.md) before proposing
method changes, reference captures, caching or sparse assembly. Proposed/deferred
items do not authorize implementations or operator preference changes.

## Validation

Before accepting a result, compare geometry and constraint identities/counts/values,
DoF and diagnostic IDs, required observer/touched semantics, expressions and external
links, shape validity/topology/placement and dimensional perturbation/restoration.
For code changes, include scalar/list transactions, immediate undo/redo solve/move,
noRecomputes, direct property changes and exceptions. Use the native/Python suites:

```sh
sketch_native_profile="$(mktemp -d /tmp/sketcher-native-XXXXXX)"
env XDG_CONFIG_HOME="$sketch_native_profile/config" XDG_DATA_HOME="$sketch_native_profile/data" XDG_CACHE_HOME="$sketch_native_profile/cache" pixi run ./build/release/tests/Sketcher_tests_run
pixi run python -m unittest discover -s tools/profile -p test_sketcher_comparison.py -v
```

Run `TestSketcherApp` through FreeCADCmd's Python environment for the affected
workbench suite, using separate profile/temp paths and verifying loaded modules.
Do not assume the outer process exit code proves embedded Python tests passed;
inspect the unittest result/receipt. Validate GUI-only panel/recovery behaviour in
a task-owned disposable GUI/display and capture only its exact application window.

## Rollback Or Recovery

Unset the trace variable and launch a new task-owned process to disable new tracing.
Restore/discard only task-owned inputs/configs; do not alter operator documents or
preferences. Incomplete traces need a new isolated capture, not invented missing
spans. Model/diagnostic mismatches invalidate that sample and need investigation.
Do not edit fixtures, remove constraints or relax tolerances to obtain a faster result.

## Escalation

Stop a replay when the immediate precursor, identity or terminal outcome is unknown.
Reconcile any timeout read-only through its owning workflow before considering a
new operation. Never retry a live unknown or already committed mutation to obtain
another timing. A process surviving or reporting a timeout does not establish crash,
cancellation, rollback or completion. Keep historical mutation workloads separate
from an unchanged saved-model probe, and route missing authority to B006.

Retain raw evidence privately. Share only reviewed aggregates and sanitized logs.
A repeatable correctness failure or material slowdown should become a bounded bug
or investigation with source/build/input identity and a local owner. Do not change
QR defaults, thresholds, tolerances, async policy or cache contracts in this runbook.
