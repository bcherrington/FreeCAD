---
title: FreeCAD Controlled Rotation Profiling Quickstart
doc_type: spec
artifact_type: quickstart
status: draft
owner: local-developer
last_reviewed: 2026-08-14
---

# Quickstart

## Purpose

Capture one bounded viewport-rotation session with Linux `perf` after FreeCAD
and the model are already loaded. Use the same procedure for both sides of a
comparison pair. This is temporary Spec 004 guidance and has not yet been
promoted to a durable developer runbook.

## Scope

- Included: active 3D viewport rotation during a fixed 60-second attach window.
- Excluded: application startup, model loading, document restore, recompute,
  Preferences, unrelated dialogs, and configuration changes during capture.

## Prerequisites

- A permitted source or experiment branch; do not edit `main` or a merge-only
  integration branch.
- A built FreeCAD executable at `build/debug/bin/FreeCAD`.
- Linux `perf` with permission to attach to the FreeCAD process.
- The same test model for the whole comparison pair:
  `/home/bcherrington/Projects/FreeCAD/Drawer/Gridfinity Drawer v1.3.FCStd`.
- One declared variable to change between runs. For T026 this is baseline versus
  the recreated DockOverlay experiment.
- Existing GUI file line endings must be detected and preserved if the
  experiment edits `src/Gui/OverlayWidgets.cpp` or `.h`.

## Capture Manifest

Record this information in the capture log and later in `research.md`:

| Field | Required Value |
| --- | --- |
| date/time | local timestamp and timezone |
| source branch/revision | `git branch --show-current` and `git rev-parse HEAD` |
| experiment revision | baseline or exact experiment commit/diff |
| build class | Debug/Release and relevant compiler identity |
| model | exact path and stable file identity if available |
| workflow | rotation only |
| capture method | attach to loaded FreeCAD process |
| duration/frequency/call graph | 60 seconds / 99 Hz / frame pointer |
| DockOverlay state | enabled/disabled and relevant assignments |
| compact/frameless state | enabled/disabled |
| addon/safe-mode state | recorded, unchanged across pair |
| display stack | X11/Wayland, GPU, driver/Mesa, compositor when known |

## Steps

### 1. Launch And Stabilize FreeCAD

From the FreeCAD checkout:

```sh
pixi run freecad
```

Then:

1. Open the test model.
2. Wait for document loading, restore, and expected recomputes to finish.
3. Configure the declared baseline or experiment state before capture.
4. Do not change configuration again during the measured interval.

### 2. Resolve The FreeCAD Process

In a second shell in the same checkout:

```sh
freecad_pid=$(pgrep -n -x FreeCAD)
test -n "$freecad_pid"
```

Confirm the PID belongs to the intended `build/debug/bin/FreeCAD` process
before attaching.

### 3. Create The Capture Manifest

```sh
mkdir -p /tmp/freecad-prof
stamp=$(date +%Y%m%d-%H%M%S)
capture_name="perf-rotation-T026-$stamp"
out="/tmp/freecad-prof/$capture_name.data"
log="/tmp/freecad-prof/$capture_name.log"

{
    echo "Started: $(date --iso-8601=seconds)"
    echo "Branch: $(git branch --show-current)"
    echo "Revision: $(git rev-parse HEAD)"
    echo "PID: $freecad_pid"
    echo "Output: $out"
    echo "Duration: 60 seconds"
    echo "Frequency: 99 Hz"
    echo "Call graph: fp"
    echo "Model: RECORD EXACT PATH"
    echo "Baseline or experiment: RECORD MANUALLY"
    echo "DockOverlay state: RECORD MANUALLY"
    echo "Compact/frameless state: RECORD MANUALLY"
    echo "Addon/safe-mode state: RECORD MANUALLY"
    echo "Display/GPU/driver/compositor: RECORD MANUALLY"
    echo "perf_event_paranoid: $(cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null || echo unknown)"
} | tee "$log"
```

Complete every `RECORD MANUALLY` field before profiling.

### 4. Capture Rotation Only

Start the command, immediately rotate the same representative view continuously
for the 60-second window, and stop interaction when `sleep 60` ends:

```sh
perf record -F 99 -g --call-graph fp -B \
    -p "$freecad_pid" \
    -o "$out" \
    -- sleep 60 2>&1 | tee -a "$log"
```

If model loading, Preferences, another dialog, or a configuration change occurs
during the window, mark the run invalid for the comparison pair and capture it
again. Preserve the invalid-run note rather than silently deleting the history.

### 5. Produce Bounded Reports

```sh
perf report -i "$out" --stdio --no-children \
    --percent-limit 0.5 --sort dso,symbol > "$out.report-top.txt"
perf script -i "$out" > "$out.script"

rg -i "OverlayGraphicsEffect|OverlayManager|qt_blurImage|expblur|QImage|QRasterizer|QRhiGles2|QGLXContext|dri_flush|QBackingStore|Quarter|Coin|setupCoinGeometry|recomputeWorker" \
    "$out.script" > "$out.targeted.txt"
```

Record whether samples were lost and the total sample count. Compare matched
runs by percentages/call paths as well as raw counts; equal duration alone does
not guarantee identical interaction or sample volume.

### 6. Repeat The Other Side Of The Pair

Use a fresh FreeCAD process, the same model/view, the same 60-second interaction,
and the same capture method. Change only the declared experiment variable.
Record a separate manifest and artifacts.

## Expected Results

- Overlay-disabled or inactive-effect baselines should not show the selected
  overlay blur path; any contrary result must be investigated, not discarded.
- The recreated zero-radius guard should remove `qt_blurImage`/`expblur` work
  attributable to a zero blur radius while preserving expected overlay drawing.
- Qt raster/image, Qt RHI, GLX/Mesa, and Coin/Quarter work may remain; T026 does
  not claim the overlay is the only rotation cost.
- Recompute, document-open, or Preferences signals invalidate workflow isolation
  if they materially overlap the measured window.

## Validation Evidence

| Step Or Scenario | Evidence | Result | Residual Risk |
| --- | --- | --- | --- |
| capture procedure syntax/review | package validation and reviewer inspection | pending | shell/environment differences |
| matched baseline capture | manifest, `perf.data`, report, targeted extract | pending | Debug/Linux/X11 specificity |
| matched experiment capture | manifest, source revision/diff, `perf.data`, report, targeted extract | pending | interaction variance |
| behavior smoke | manual overlay interaction before/after | pending | cross-platform GUI behavior |

## Cleanup

Raw profiles may remain under `/tmp/freecad-prof` while the slice is active.
Summaries, decisions, exact command shape, and residual risk belong in the spec.
Review raw artifacts for sensitive local paths or loaded-module information
before sharing them.

## Promotion Or Discard Decision

- **Disposition:** retain temporarily
- **Durable destination:** selected by D005 after T026 exercises the procedure
- **Reason:** the procedure is useful but not yet validated as durable guidance
- **Owner or follow-up:** local-developer/docs owner through D005 and T024

## Related Artifacts

- Requirements: `requirements.md`
- Canonical Context: `canonical-context.md`
- Design: `design.md`
- Tasks: `tasks.md`
- Verification: `verification.md`
- Research: `research.md`
- Open Decisions: `open-decisions.md`
