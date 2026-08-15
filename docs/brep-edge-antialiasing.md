---
title: BRep Edge Anti-Aliasing
doc_type: guide
status: active
owner: bcherrington
last_reviewed: 2026-08-13
---

# BRep Edge Anti-Aliasing

## Purpose

This guide records the implemented FreeCAD viewport strategy for investigating
and improving jagged BRep edges when the configured multisample setting does not
produce samples in Qt's live `QOpenGLWidget` framebuffer.

The owned-framebuffer live path is **experimental** and developer-gated. It is
accepted for this personal development branch, not enabled by default and not
represented as ready for upstream FreeCAD without an issue, GUI/rendering design
consensus, and cross-platform evidence.

## Current State

The native `SoBrepEdgeSet` renderer remains authoritative for BRep edges.
`View3DInventorViewer` has an optional live presentation path that:

1. renders the normal Coin scene into one viewer-owned multisample color/depth
   framebuffer;
2. includes live selection, preselection, Navigation Cube, and other viewer
   decorations in that normal interactive traversal;
3. resolves the completed color frame on the GPU into Qt's actual default
   framebuffer; and
4. uses the existing native renderer in the same frame when any prerequisite
   or rendering step fails.

No public API, user preference, or default behavior was added. Existing
Framebuffer and Image render modes retain their capture/snapshot semantics.

## Invariants

- With the developer gate unset, live rendering follows the existing native
  path.
- The live path never calls `toImage()` or performs CPU framebuffer readback.
- In normal mode, a Qt widget that already reports actual samples stays on its
  native path. Force mode requests a single-sample widget and may remain active
  if a platform supplies a matching multisample widget anyway.
- Selection, preselection, highlight, foreground geometry, depth, screenshots,
  and object rendering continue through the production viewer traversal.
- Navigation Cube, axis cross, dimensions, graphics items, and rubber-band
  content use the same interactive traversal inside the owned framebuffer;
  there is no second decoration traversal with mismatched depth state.
- Owned resources are private to one viewer and are reused only while context,
  physical viewport size, device-pixel ratio, and requested samples match.
- Failed configurations are cached until an input changes, preventing a failed
  allocation from becoming an idle or per-frame allocation loop.

## Developer Activation And Rollback

Configure the normal FreeCAD anti-aliasing preference, then start a new process:

```sh
FREECAD_SCENE_AA_LIVE=1 pixi run freecad
```

The environment gate is read when each viewer is created. Unset it and restart
FreeCAD to roll back:

```sh
unset FREECAD_SCENE_AA_LIVE
pixi run freecad
```

`FREECAD_SCENE_AA_LIVE_STATS=1` additionally reports activation/fallback reason,
widget and owned sample state, physical dimensions, allocations, and
GPU-complete stage timings. `render_us` covers the complete interactive frame,
including decorations; `decorations_us` remains zero for this combined path.
The statistics mode calls `glFinish()` and is for diagnosis only; do not enable
it during ordinary interaction or use its timings as release benchmarks.

To exercise the owned framebuffer on a system that normally supplies a native
multisampled widget, use the explicit developer-only force value:

```sh
FREECAD_SCENE_AA_LIVE=force pixi run freecad
```

Force mode is a comparison tool, not an automatic recommendation. It requests
a single-sample Qt widget when the viewer is created, then renders and resolves
the requested MSAA level through the owned framebuffer. The owned framebuffer
hosts the normal interactive Coin traversal, including live selection,
preselection, and viewport decorations; it does not use the raster-capture
traversal. Compare a forced run with a separate normal run; the harness proves
that the forced owned-resolve path activates, not native-versus-owned image
parity. If a platform ignores the single-sample request and still creates a
multisampled widget, its sample count must match the owned buffer; otherwise the
same-frame native fallback is used with reason `forced-sample-mismatch`. That
defensive, platform-dependent branch is not exercised by the deterministic
harness test.

## Activation And Failure Contract

The owned path requires all of the following:

- a positive requested sample count;
- a valid current `QOpenGLWidget` context whose live framebuffer reports no
  actual samples in normal mode, or whose samples match the owned framebuffer
  in force mode;
- a non-empty physical viewport;
- framebuffer-blit support; and
- a valid owned FBO with actual multisample storage.

Zero requested samples, native widget MSAA in normal mode, invalid
widget/context, missing blit support, allocation/bind failure, unavailable
multisample storage, a forced sample-count mismatch, rendering failure, or
incomplete resolve framebuffers select the existing native renderer for that
frame. Resize, DPR, context, or sample-count changes invalidate and recreate
owned resources; context teardown releases them while the widget context is
current.

## Validation

Use the focused executable, which links the production FreeCAD libraries and
avoids full application startup/shutdown overhead:

```sh
cmake --build build/debug --target GpuDiagnosticsHarness_run -j6
ctest --test-dir build/debug -R '^GpuDiagnostics\.' --output-on-failure
ctest --test-dir build/debug -R '^GpuDiagnosticsHarness_' --output-on-failure
```

The harness covers native/capture behavior plus live 0x fallback, live 8x,
simulated DPR 2, resize-driven reallocation, a deterministic 100-cube dense
scene, and forced owned-FBO activation with a single-sample widget. A document
fixture adds a fully filleted housing with cylindrical holes, a rounded top
boss, a hidden rounded solid, real face selection/preselection, and the
Navigation Cube. Its X11 frame check requires substantial visible center
geometry, rejects a large red region from the hidden object or selection
artifacts, independently requires deterministic magenta selection and cyan
preselection pixels, and confirms that the Navigation Cube remains present. The
suite also covers overlay/decorations and bounded X11 dialog teardown.
Screenshot and frame-stat arguments remain diagnostic evidence surfaces;
generated PNG and trace files are not repository documentation.

On Intel TigerLake GT1 with Mesa/X11 in a Debug build, the owned path reported
eight actual samples while the Qt widget reported zero. The synthetic dense
scene measured a 7,637.5 microsecond median versus 4,900 microseconds native
(+55.9%), with high variance. This supports the opt-in branch decision only;
it is not a cross-platform or release performance budget.

## Alternatives Considered

- Fixed-function `GL_LINE_SMOOTH` was rejected after controlled captures were
  pixel-identical on the tested stack.
- Projected segment de-duplication removed deliberate duplicate opacity but can
  merge legitimate close edges without stronger topology semantics.
- Batched screen-space shader edges remain a research fallback; joins, styles,
  persistent buffers, and topology behavior need a larger design.
- FXAA/SMAA was not selected because owned MSAA improved scene edges without
  filtering Qt UI or fine detail.

The diagnostic-only alternative render modes remain experiment aids, not
supported production choices.

## Limitations And Review Boundary

- Runtime evidence is Linux/X11/Mesa only; Wayland, Windows, and macOS remain
  unverified.
- Simulated DPR 2 verifies physical sizing, not another display platform.
- The dense fixture stresses repeated production BRep edges but is not a curved
  or representative full CAD document.
- Explicit GL-context recreation, long interactive sessions, memory pressure,
  and release-build frame-time distributions remain unmeasured.
- Enabling by default or proposing upstream requires a linked problem statement,
  before/after evidence, and appropriate GUI/rendering consensus.

## Change Rules

When changing the live owned-FBO path:

1. preserve the disabled native path and Framebuffer/Image semantics;
2. preserve per-file line endings in existing GUI files;
3. update the focused harness for activation, fallback, resize/DPR, decoration,
   and teardown changes;
4. run the build and CTest commands above plus applicable pre-commit and static
   analysis checks;
5. update this guide when activation, failure behavior, diagnostics, supported
   platforms, or measured constraints change; and
6. obtain current GUI/rendering design review before adding a preference or
   changing defaults.

## Related Documents

- [Viewport Rendering Improvement Experiments](viewport-rendering-improvements.md)
- [GPU Diagnostics](gpu-diagnostics.md)
- [Spec closure log](history/spec-closure-log.md)
- Historical implementation package: Git history at final spec commit
  `5602abf`.
