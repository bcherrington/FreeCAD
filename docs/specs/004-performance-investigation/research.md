---
title: FreeCAD performance investigation research
doc_type: spec
status: draft
owner: local-developer
last_reviewed: 2026-08-14
artifact_type: research
---

# Research

## Scope

This artifact preserves the 2026-05-24 performance investigation and records
later reconciliation needed to act on it safely. It covers viewport rotation,
an overlay experiment, document-open stacks, and a separate Preferences-open
capture. It does not establish a current production root cause, a release
performance budget, or approval for a source change.

## Question

What is worth investigating next after local `gdb`, `perf`, and VTune captures
of FreeCAD GUI lag?

## Environment

- FreeCAD checkout: `/home/bcherrington/Projects/CLion/FreeCAD`
- Build: `build/debug/bin/FreeCAD`
- Runtime command shape: `pixi run build/debug/bin/FreeCAD`
- Test model: `/home/bcherrington/Projects/FreeCAD/Drawer/Gridfinity Drawer v1.3.FCStd`
- Graphics stack observed in traces: Qt 6, GLX, Mesa/Gallium
- Date: 2026-05-24

## Captured Artifacts

Raw profiler artifacts are local and temporary by design; conclusions are kept
in this spec.

- Overlay/compact-enabled `gdb`/`pidstat` run:
  - `/tmp/freecad-prof/previous-20260524-180237/backtraces.txt`
  - `/tmp/freecad-prof/previous-20260524-180237/pidstat.txt`
  - `/tmp/freecad-prof/previous-20260524-180237/run.log`
- Overlay assignments and compact/frameless disabled `gdb`/`pidstat` run:
  - `/tmp/freecad-prof/backtraces.txt`
  - `/tmp/freecad-prof/pidstat.txt`
  - `/tmp/freecad-prof/run.log`
- Overlay-disabled `perf` rotation run:
  - `/tmp/freecad-prof/perf-freecad-fp-20260524-181829.data`
  - `/tmp/freecad-prof/perf-freecad-fp-20260524-181829.log`
- VTune Preferences-open run, not a clean rotation run:
  - `/tmp/freecad-prof/vtune-hotspots-direct-20260524-182612`
  - `/tmp/freecad-prof/vtune-hotspots-direct-20260524-182612.log`
- Short overlay-disabled baseline rotation run:
  - `/tmp/freecad-prof/perf-rotation-overlay-disabled-20260524-184338.data`
  - `/tmp/freecad-prof/perf-rotation-overlay-disabled-20260524-184338.log`
- Long mostly-disabled rotation run, later reported to have overlay re-enabled
  during the same capture:
  - `/tmp/freecad-prof/perf-rotation-overlay-disabled-long-20260524-184946.data`
  - `/tmp/freecad-prof/perf-rotation-overlay-disabled-long-20260524-184946.log`
- Long overlay-enabled rotation run:
  - `/tmp/freecad-prof/perf-rotation-overlay-enabled-long-20260524-185511.data`
  - `/tmp/freecad-prof/perf-rotation-overlay-enabled-long-20260524-185511.log`
- Clean overlay-disabled rotation rerun:
  - `/tmp/freecad-prof/perf-rotation-overlay-disabled-clean2-20260524-190246.data`
  - `/tmp/freecad-prof/perf-rotation-overlay-disabled-clean2-20260524-190246.log`
  - `/tmp/freecad-prof/perf-rotation-overlay-disabled-clean2-20260524-190246.report-top.txt`
  - `/tmp/freecad-prof/perf-rotation-overlay-disabled-clean2-20260524-190246.script`
- Optimized overlay-enabled attach rotation run:
  - `/tmp/freecad-prof/perf-rotation-overlay-enabled-optimized-attach-20260524-194420.data`
  - `/tmp/freecad-prof/perf-rotation-overlay-enabled-optimized-attach-20260524-194420.log`
  - `/tmp/freecad-prof/perf-rotation-overlay-enabled-optimized-attach-20260524-194420.report-top.txt`
  - `/tmp/freecad-prof/perf-rotation-overlay-enabled-optimized-attach-20260524-194420.script`

## Current Source Reconciliation - 2026-08-14

The historical DockOverlay experiment is commit `3d41e4b3c4f` on branch
`perf/dock-overlay-investigation`. The commit:

- disabled the underlying graphics effect while inactive;
- skipped delayed repaints while the effect was inactive; and
- skipped `qt_blurImage()` when `effectBlurRadius` was zero.

Direct Git inspection found that the commit is an ancestor of the current
`personal/integration-testing` checkout. Direct source inspection also found
that current `src/Gui/OverlayWidgets.cpp` no longer contains those experiment
guards and again calls `qt_blurImage(...)` without a zero-radius condition.

Conclusion: branch ancestry does not make the experiment current behavior. It
remains historical evidence with a direct hotspot rationale. T026 must recreate
or reapply the smallest current-source version on a permitted source branch and
capture a new matched baseline/experiment pair before D002 can keep or reject
it. No current implementation claim is made.

## Findings

### Async Recompute

Both sampled `gdb` runs repeatedly showed
`App::Application::recomputeWorker()` waiting on its condition variable at
`src/App/Application.cpp:3218`.

No captured evidence currently supports async recompute as the direct cause of
the rotation lag.

### DockOverlay

Before disabling overlay assignments, a GUI-thread sample included
`Gui::OverlayGraphicsEffect::draw` with `QImage::copy`, `QImage::detach`, and
recursive Qt widget repainting.

After disabling overlay assignments and compact/frameless config:

- `Gui::OverlayGraphicsEffect::draw` no longer appeared in sampled stacks.
- Repeated overlay graphics-effect image-copy compositing was not seen.
- `Gui::OverlayManager::eventFilter` still appeared once during a mouse event,
  using `QCursor::pos()` and waiting on XCB.

Interpretation: DockOverlay drawing is a plausible contributor and should be
profiled with a controlled enabled/disabled A/B. The overlay event filter may
also need cheap early-outs when overlays are inactive or irrelevant.

### Compact UI

Compact/frameless UI was disabled as a controlled variable because it changes
main-window chrome and composition. The captures do not directly implicate
compact UI code. Keep it as an A/B variable only unless new traces point to
compact-specific code.

### Qt Paint, QImage, And Rasterization

The cleanest rotation `perf` run was captured with:

```sh
perf record -F 99 -g --call-graph fp -B -o /tmp/freecad-prof/perf-freecad-fp-20260524-181829.data -- pixi run build/debug/bin/FreeCAD
```

`perf --call-graph dwarf` was attempted but hit the known FreeCAD debug-library
`addr2line` failure loop. Frame-pointer call graphs completed reliably enough
for directional evidence.

The frame-pointer run recorded about 20K `cycles:P` samples with no lost
samples. The top flat CPU cost was `__memmove_evex_unaligned_erms` at about
23% self. Children attributed much of this to:

- `QImage::QImage(QImage const&)`
- `QImage::~QImage`
- `QRasterizer::rasterizeLine`
- Qt memory fill paths
- `QRhiGles2::executeCommandBuffer`

Targeted `perf script` counts from that run:

- `QImage` constructor/destructor/copy/detach related lines: 14734
- `memmove` / `memcpy` / `memfill` related lines: 11486
- `QRasterizer::rasterizeLine`: 5182
- `QRhiGles2::executeCommandBuffer`: 1242
- `QGLXContext::swapBuffers`: 79
- `dri_flush`: 74
- `QBackingStore`: 4
- `Gui::OverlayGraphicsEffect`: 0
- `Gui::OverlayManager::eventFilter`: 8
- `PartGui::ViewProviderPartExt::setupCoinGeometry`: 23
- Any `ViewProviderPartExt`: 91
- Coin/Quarter render related lines: 221
- Recompute related lines: 0

Interpretation: the strongest current rotation signal is Qt image/memory
movement, rasterization, RHI command execution, GLX/Mesa swap, and Coin/Quarter
rendering. The next question is which FreeCAD widget/style/effect or viewport
path is triggering that cost during rotation.

### Short Overlay-Disabled Baseline - 2026-05-24 18:43

A short baseline was captured after DockOverlay was disabled:

```sh
perf record -F 99 -g --call-graph fp -B \
  -o /tmp/freecad-prof/perf-rotation-overlay-disabled-20260524-184338.data \
  -- pixi run build/debug/bin/FreeCAD
```

This run exited quickly and produced a small `perf.data` file, so treat it as a
light baseline rather than a strong comparison point. It captured about 2K
samples and at least one Coin mouse event.

Targeted counter extraction:

- `QRhiGles2::executeCommandBuffer`: 25
- `QImage` constructor/destructor/copy/detach related lines: 131
- `memmove` / `memcpy` / `memfill` related lines: 249
- `QRasterizer::rasterizeLine`: 0
- `QGLXContext::swapBuffers`: 0
- `dri_flush`: 0
- `QBackingStore`: 0
- `Gui::OverlayGraphicsEffect`: 0
- `Gui::OverlayManager::eventFilter`: 0
- `PartGui::ViewProviderPartExt::setupCoinGeometry`: 19
- Any `ViewProviderPartExt`: 65
- Coin/Quarter render related lines: 111
- Mouse/event processing: 4
- Recompute related lines: 0

Interpretation: this short run is consistent with DockOverlay drawing and async
recompute being absent from the captured baseline. It is too short to compare
frame-presentation behavior meaningfully, because no GLX swap or `dri_flush`
samples were captured.

### Long Rotation A/B Attempt - 2026-05-24 18:49 And 18:55

A longer overlay-disabled capture was attempted first:

```sh
perf record -F 99 -g --call-graph fp -B \
  -o /tmp/freecad-prof/perf-rotation-overlay-disabled-long-20260524-184946.data \
  -- pixi run build/debug/bin/FreeCAD
```

The user later clarified that DockOverlay was re-enabled during that run after
about 60-90 seconds, because the instructions sounded like both states should
be captured in one performance run. Treat this result as mostly-disabled but
not a clean disabled-only baseline.

Targeted counter extraction from the mostly-disabled/mixed run:

- `QRhiGles2::executeCommandBuffer`: 241
- `QImage` constructor/destructor/copy/detach related lines: 1339
- `memmove` / `memcpy` / `memfill` related lines: 1897
- `QRasterizer::rasterizeLine`: 4
- `QGLXContext::swapBuffers`: 38
- `dri_flush`: 37
- `QBackingStore`: 3
- `Gui::OverlayGraphicsEffect`: 0
- `Gui::OverlayManager::eventFilter`: 10
- `PartGui::ViewProviderPartExt::setupCoinGeometry`: 23
- Any `ViewProviderPartExt`: 91
- Coin/Quarter render related lines: 4259
- Mouse/event processing: 74
- Recompute related lines: 0

A fresh overlay-enabled capture was then run from a new FreeCAD process:

```sh
perf record -F 99 -g --call-graph fp -B \
  -o /tmp/freecad-prof/perf-rotation-overlay-enabled-long-20260524-185511.data \
  -- pixi run build/debug/bin/FreeCAD
```

Targeted counter extraction from the overlay-enabled run:

- `QRhiGles2::executeCommandBuffer`: 114
- `QImage` constructor/destructor/copy/detach related lines: 1002
- `memmove` / `memcpy` / `memfill` related lines: 1256
- `QRasterizer::rasterizeLine`: 0
- `QGLXContext::swapBuffers`: 16
- `dri_flush`: 12
- `QBackingStore`: 0
- `Gui::OverlayGraphicsEffect`: 1
- `Gui::OverlayManager::eventFilter`: 12
- `PartGui::ViewProviderPartExt::setupCoinGeometry`: 24
- Any `ViewProviderPartExt`: 98
- Coin/Quarter render related lines: 501
- Mouse/event processing: 15
- Recompute related lines: 0

The overlay-enabled top report included
`void (anonymous namespace)::expblur<12, 10, true>(QImage&, double, bool, int)`
at about 5.23% self. This blur path was not the primary finding in the earlier
disabled/mixed summary and is a useful clue for overlay or Qt effect review.

The initial follow-up disabled run was later found to be invalid because
DockOverlay was not fully disabled. A corrected disabled-only rerun was captured
from a new FreeCAD process:

```sh
perf record -F 99 -g --call-graph fp -B \
  -o /tmp/freecad-prof/perf-rotation-overlay-disabled-clean2-20260524-190246.data \
  -- pixi run build/debug/bin/FreeCAD
```

Targeted counter extraction from the clean disabled-only rerun:

- `QRhiGles2::executeCommandBuffer`: 144
- `QImage` constructor/destructor/copy/detach related lines: 628
- `memmove` / `memcpy` / `memfill` related lines: 1056
- `QRasterizer::rasterizeLine`: 1
- `QGLXContext::swapBuffers`: 8
- `dri_flush`: 8
- `QBackingStore`: 0
- `Gui::OverlayGraphicsEffect`: 0
- `QGraphicsEffect`: 0
- `expblur`: 0
- `Gui::OverlayManager::eventFilter`: 7
- `PartGui::ViewProviderPartExt::setupCoinGeometry`: 13
- Any `ViewProviderPartExt`: 51
- Coin/Quarter render related lines: 730
- Mouse/event processing: 90
- Recompute related lines: 0

Interpretation: this corrected run changes the overlay conclusion. When
DockOverlay is genuinely disabled, the overlay graphics-effect and blur paint
path disappears from the profile. The light
`Gui::OverlayManager::eventFilter` samples remain, likely because the manager is
still installed even without active overlay drawing. The remaining disabled-run
cost is dominated by Qt image copies, memory movement/fill, Qt RHI/OpenGL work,
GLX/Mesa frame presentation, document restore/open, Part view-provider geometry
setup, and normal Coin/Quarter viewport work.

The overlay-enabled run confirms that overlay/effect work can add an extra blur
and image-composition path. It does not prove DockOverlay is the only or primary
rotation-lag cause, because substantial Qt image/memory/RHI and GLX/Mesa costs
remain when the overlay paint path is absent. The next useful overlay work is a
source review of whether this blur/composition is avoidable, lower frequency,
or bounded to smaller regions.

### Optimized Overlay-Enabled Attach Run - 2026-05-24 19:44

A source-level experiment was created on branch
`perf/dock-overlay-investigation` after reviewing `OverlayGraphicsEffect`:

- Disable the underlying `QGraphicsEffect` when the FreeCAD wrapper effect is
  inactive.
- Do not schedule delayed overlay repaints when the effect is inactive.
- Skip `qt_blurImage()` when the configured `effectBlurRadius` is zero.

The zero-radius blur skip is directly related to the measured hotspot: the
default overlay stylesheet sets `qproperty-effectBlurRadius: 0`, but the old
draw path still called Qt's blur helper.

The branch built successfully with:

```sh
pixi run cmake --build build/debug --target FreeCADGui -j2
```

An attach-style rotation capture was then taken against the already-running
FreeCAD process:

```sh
perf record -F 99 -g --call-graph fp -B \
  -p 167866 \
  -o /tmp/freecad-prof/perf-rotation-overlay-enabled-optimized-attach-20260524-194420.data \
  -- sleep 60
```

Targeted counter extraction from this optimized overlay-enabled run:

- `QRhiGles2::executeCommandBuffer`: 57
- `QImage` constructor/destructor/copy/detach related lines: 264
- `memmove` / `memcpy` / `memfill` related lines: 500
- `QRasterizer::rasterizeLine`: 1
- `QGLXContext::swapBuffers`: 6
- `dri_flush`: 6
- `QBackingStore`: 1
- `Gui::OverlayGraphicsEffect`: 1
- `QGraphicsEffect`: 0
- `expblur`: 0
- `qt_blurImage`: 0
- `Gui::OverlayManager::eventFilter`: 3
- `PartGui::ViewProviderPartExt::setupCoinGeometry`: 0
- Any `ViewProviderPartExt`: 1
- Coin/Quarter render related lines: 416
- Mouse/event processing: 26
- Recompute related lines: 0

The single remaining `Gui::OverlayGraphicsEffect::draw` sample was followed by
Qt raster image blending, not `qt_blurImage` or `expblur`.

Interpretation: the experiment appears to remove the avoidable zero-radius blur
work while preserving overlay drawing. It does not remove all overlay paint
cost, and the remaining profile is still dominated by Qt raster fill, image
copy/memory movement, RHI command execution, GLX/Mesa presentation, and normal
Coin/Quarter viewport activity. The next decision is whether this low-risk
change should stay as a measured overlay optimization, followed by a clean
before/after capture of the same interaction duration.

### GLX / Mesa / Qt RHI

Both `gdb` and `perf` showed frame-presentation paths:

- Mesa/Gallium `dri_flush`
- `QGLXContext::swapBuffers`
- `QRhiGles2::executeCommandBuffer`
- `QBackingStoreDefaultCompositor::flush`
- `QWidgetRepaintManager::paintAndFlush`

This may be expected during active rotation, but it is worth investigating if
lag correlates with swap stalls or RHI/backing-store work rather than scene
complexity.

### Coin / Quarter Rendering

`perf` saw Coin/Quarter render-related samples during rotation, including
Quarter paint/event paths. This is expected while rotating, but it remains a
valid investigation track for avoidable redraws, cache invalidations, or
expensive scene traversal.

### Document Restore / Open

The overlay-disabled `gdb` run captured a main-thread document-open stack:

- `PartGui::ViewProviderPartExt::setupCoinGeometry`
- `PartGui::ViewProviderPartExt::updateVisual`
- `Gui::Document::RestoreDocFile`
- `App::Application::openDocumentPrivate`

This was while opening `Gridfinity Drawer v1.3.FCStd`. Treat it as a separate
document restore/open investigation, not as rotation evidence.

### Preferences Open Hang

A corrected VTune run targeted `build/debug/bin/FreeCAD` directly from inside
the pixi environment, but the user later reported that the observed hang was
opening Preferences, not rotating.

Important context:

- VTune emitted timer-stop warnings and one internal assertion while stopping,
  but generated a summary report.
- The run logged an addon startup timeout:
  `ffmpeg -version` timed out after 2 seconds from
  `~/.local/share/FreeCAD/v1-2/Mod/addFC/addon/addFC/InitGui.py`.
- Capture duration: about 414 seconds elapsed, about 54 seconds CPU, 89 total
  threads.
- Top VTune hotspots:
  - `_dl_open`: 3.338 s
  - `__dynamic_cast`: 3.228 s
  - `operator new`: 2.668 s
  - `_dwarf_get_alloc`: 2.528 s
  - `_dwarf_next_die_info_ptr.constprop.0`: 2.203 s

Interpretation: this is not clean rotation evidence. It suggests a separate
Preferences/addon/module-loading investigation. DWARF-related costs may be
VTune symbol-processing overhead rather than FreeCAD behavior.

## Recommendation

Treat the corrected disabled-only run as the baseline and the earlier
mixed-state disabled run as invalid for A/B conclusions.

The 2026-08-14 multi-expert review selected this evidence-driven sequence:

1. Recreate the historical zero-radius-blur experiment on a permitted current
   source branch and run the same-duration attach-style comparison defined by
   `quickstart.md`.
2. Resolve D002: keep a bounded overlay implementation target only if the
   matched evidence and behavior smoke support it; otherwise reject or route it.
3. If the overlay result does not identify a useful target, capture document
   open separately and focus on `Gui::Document::RestoreDocFile` and
   `PartGui::ViewProviderPartExt::setupCoinGeometry`.
4. Then split residual active-rotation cost between FreeCAD/Coin scene work and
   Qt/OpenGL presentation paths in `View3DInventorViewer.cpp`.
5. Keep Preferences/addon/module-loading work isolated until its ownership is
   directly classified.

### Viewport Render Timing Approach - 2026-05-24

The next track remains active 3D view rotation cost. The useful split is:

- Quarter/Qt paint event setup and delay-queue processing.
- FreeCAD/Coin scene redraw.
- Qt graphics-view painting after the GL redraw.
- Sub-stages inside `View3DInventorViewer::renderScene()`.
- Any remaining cost that appears outside these stages, which likely belongs to
  Qt OpenGL widget composition, buffer presentation, Mesa/GLX, or compositor
  behavior.

A source-level timing experiment was briefly tried and then removed from the
branch. Do not carry timing or profiling code into the integration tests or
normal modelling validation. Use external tools instead.

Recommended validation path:

- Treat the zero-radius overlay blur patch as a historical candidate, not a
  current or accepted change.
- Recreate or reapply it only on a permitted source branch for T026.
- Run normal modelling sessions without timing instrumentation.
- If lag recurs, capture externally with `perf` or VTune around the already
  running FreeCAD process.
- Compare application symbols against Qt RHI, GLX/Mesa, and kernel/compositor
  samples.

External capture shape:

```sh
perf record -F 99 -g --call-graph fp -B -p <FreeCAD PID> -o /tmp/freecad-prof/<name>.data -- sleep 60
```

Interpretation plan:

- If application symbols dominate, inspect Coin/Quarter rendering and scene
  traversal.
- If Qt raster/image symbols dominate, inspect widget composition and overlay
  paint paths.
- If `swapBuffers`, `dri_flush`, `QRhiGles2`, or unresolved kernel samples
  dominate while application symbols are light, prioritize Mesa/GLX/Qt
  presentation and compositor behavior.

## Tradeoffs

- External attach profiling avoids carrying instrumentation into normal builds,
  but it offers less precise application-stage timing and remains sensitive to
  symbol/call-graph quality.
- A zero-radius blur guard is narrow and directly tied to a sampled hotspot, but
  removing that hotspot may not materially change perceived latency because
  scene rendering and presentation remain.
- Debug builds provide symbols and directional evidence but can distort
  allocator, DWARF, and instrumentation costs relative to release builds.
- Keeping all historical evidence supports auditability, but current-source and
  fresh runtime receipts must be prominent enough to prevent stale reuse.

## Sources

- Temporary profiler artifacts listed in this file, captured 2026-05-24.
- Current source files named by the task and design artifacts.
- Git commit `3d41e4b3c4f` and direct ancestry/delta inspection on 2026-08-14.
- Durable routing in `docs/performance-optimization-findings.md`.
- Supporting renderer outcomes in `docs/viewport-rendering-improvements.md`.

## Confidence And Unknowns

- **Confidence:** medium for routing; low for any current performance magnitude.
- **Known unknowns:** current before/after overlay effect, release-build impact,
  perceived latency, non-X11 platforms, other GPUs/drivers/configurations, and
  exact ownership of Preferences/addon behavior.
- **Assumptions:** the recorded 2026-05-24 manifests and summaries are accurate;
  raw temporary files were not revalidated in this revision.
- **Evidence gaps:** T026 matched capture, document-open-only capture,
  presentation attribution, source-regression tests, cross-platform review, and
  durable destination decisions.

## Decision Impact

- D001 is resolved in favor of T026 overlay validation as the next
  investigation slice.
- D002 remains open and blocks source-implementation readiness.
- Presentation, document-open, and Preferences tasks remain queued and distinct.
- The procedure and accepted conclusions remain spec-local until D005 selects a
  durable destination.

## Related Artifacts

- Requirements: `requirements.md`
- Canonical Context: `canonical-context.md`
- Design: `design.md`
- Tasks: `tasks.md`
- Traceability: `traceability.md`
- Verification: `verification.md`
- Quickstart: `quickstart.md`
- Open Decisions: `open-decisions.md`
