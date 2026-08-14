---
title: FreeCAD performance optimization plan
doc_type: spec
status: draft
owner: local-developer
last_reviewed: 2026-05-22
---

# Implementation Plan

## Summary

This plan turns the profiling findings into implementation work for FreeCAD
document-open and runtime performance optimization. The goal is to keep
optimization work evidence-led: each proposed change should have a clear
hypothesis, a small implementation scope, and a before/after measurement using
normal working mode.

The source evidence is in [research.md](research.md).
The compact view rail geometry optimization found during the same profiling
work is owned by
[Compact view rails rework](../compact-view-rails-rework/spec.md).

## Technical Context

- **Language/Version**: C++ and Python in the local FreeCAD debug build.
- **Primary Dependencies**: Qt, Coin/Quarter, OpenCascade, FreeCAD App and Gui
  internals.
- **Storage**: FreeCAD document restore payloads, embedded files, and GUI
  preferences.
- **Testing**: Focused CTest coverage where stable, performance traces, and
  manual validation for document and image behavior.
- **Target Platform**: Linux debug build for the captured evidence; fixes
  should preserve cross-platform FreeCAD behavior.
- **Project Type**: Desktop CAD application.
- **Performance Goals**: Reduce document restore cost and large image
  conversion overhead without changing document semantics.
- **Constraints**: Keep temporary instrumentation out of production unless
  intentionally designed, and preserve existing user workflows.
- **Scale/Scope**: Optimize the measured hotspots in small independently
  reviewable patches.

## Governance Check

- [x] SOLID boundaries defined: each patch keeps ownership clear between
  document restore, image conversion, and lower-priority tree-view behavior.
- [x] DRY plan defined: shared trace helpers are extracted only if
  instrumentation becomes maintained code.
- [x] Test strategy defined: focused tests, traces, and manual validation are
  mapped to each optimization.
- [x] UX consistency impact assessed: document and image behavior preserve
  existing user-visible results.
- [x] Performance budgets defined: before/after traces compare the same
  workflow and build mode.

## Project Structure

### Documentation

```text
docs/specs/freecad-performance-optimization/
|-- spec.md
|-- plan.md
|-- research.md
`-- tasks.md
```

### Source Code

Expected touchpoints depend on the selected phase:

```text
src/Gui/
src/App/
src/Base/
src/Mod/
tests/
docs/
```

**Structure Decision**: Keep this as a spec package because the work is not yet
fully implemented and the research findings should remain linked to the
implementation plan.

## Phases

1. Optimize document restore embedded-file payloads.
2. Optimize image decode, scaling, and bitmap conversion hot paths.
3. Investigate lower-priority tree/document UI update batching.
4. Re-run focused normal-mode traces and update this package with results.

## Dependencies

- A reproducible local FreeCAD build and representative documents.
- Profiling and tracing access on the host used for comparison.
- Maintainer review for any production diagnostic path.

## Review Decisions

- Keep compact-rail viewport/FBO churn out of this implementation package. It
  remains linked as related performance evidence, but its fix changes compact
  UI geometry ownership and belongs in the compact rail rework.
- Treat document restore and image-plane conversion as the first two
  independently reviewable production changes. They are local, measurable, and
  directly tied to multi-second document-open cost.
- Keep shape-map, GUI-document restore, tree batching, startup addon churn, and
  icon/SVG cache work as investigation tasks until narrower timing proves the
  dominant sub-step and the invalidation rules are clear.
- Do not land ad hoc tracing with optimization patches. Either remove temporary
  instrumentation before review or split a maintained diagnostic facility into
  its own reviewable change.
- Commit production work in small, coherent changes that can be cherry-picked
  into a later PR without also pulling unrelated tracing, documentation, or
  lower-priority investigation work.
- Prefer correctness-preserving fast paths with fallback behavior over broad
  rewrites. For example, add direct 32-bit `QImage` scanline conversion only
  for known formats and retain the existing `pixelColor()` path for uncommon
  formats.

## Risks

- Document restore optimizations must not change archive boundaries, file
  permissions, property notifications, or saved document compatibility.
- Image conversion changes can regress orientation, color, alpha, or texture
  ownership behavior.
- Debug-build profiling can exaggerate absolute costs; compare bucket ordering
  and remeasure relevant changes.

## Validation Strategy

Each patch should include a before/after measurement using the same workflow,
plus focused automated or manual validation for the touched subsystem. Broaden
to `pixi run test` when shared App/Base/Gui behavior is changed.

## Current Temporary Changes

The current workspace contains temporary opt-in profiling instrumentation. These
changes are useful for investigation, but should not be treated as production
implementation unless they are cleaned up and intentionally designed as
maintainable diagnostics.

Temporary instrumentation added for this performance package:

- `FREECAD_TREE_TRACE=1` in `src/Gui/Tree.cpp`
  - Counts tree object creation/deletion, status updates, icon generation, and
    bulk document open/close behavior.
  - Showed tree churn is frequent but tens of milliseconds for the tested
    document.
- `FREECAD_RESTORE_TRACE=1` in `src/App/Document.cpp` and
  `src/Base/Reader.cpp`
  - Breaks document restore into XML/object restore, embedded-file restore, and
    postprocess hooks.
  - Showed `Base::XMLReader::readFiles()` dominates the tested document open.
- `FREECAD_IMAGE_TRACE=1` in `src/Gui/BitmapFactory.cpp` and
  `src/Gui/ViewProviderImagePlane.cpp`
  - Counts icon/pixmap cache behavior, SVG renders, pixmap loads, pixmap
    composition, and `QImage`/`SoSFImage` conversion.
  - Showed document-open image cost is dominated by large image-plane decode
    and conversion, not toolbar icon cache misses.

Before merging production optimization work, decide whether to:

- Remove temporary trace code after each investigation.
- Keep a smaller maintained diagnostic path behind a developer preference or
  compile-time flag.
- Move repeated trace helpers into a shared utility instead of leaving several
  local ad hoc implementations.

## Moved: Compact Layout / OpenGL FBO Churn

The compact layout and OpenGL framebuffer churn finding is still important, but
its implementation work belongs to
[Compact view rails rework](../compact-view-rails-rework/spec.md). That spec
owns the rail geometry contract, drag-and-drop replacement, and validation
requirements for `FREECAD_QUARTER_GL_TRACE=1`,
`FREECAD_COMPACT_LAYOUT_TRACE=1`, and GL framebuffer allocation comparisons.

## Priority 1: Document Restore Embedded Payloads

### Evidence

For `Gridfinity_Drawer_v1_3`, restore was dominated by embedded file payloads:

- `Document.restore.total`: about `11.9s`
- `Document.restore.readFiles`: about `11.2s`
- `Document.restore.DocumentXML`: about `0.68s`
- `Document.restore.afterRestore`: about `0.36s`

Per-file tracing then identified the largest restore payload categories:

- Included PNGs through `App::PropertyFileIncluded`
- Part element maps through `Part::TopoShape`
- BREP shape payloads through `Part::PropertyPartShape`
- `GuiDocument.xml`

### Implementation Direction

Start with the lowest-risk change:

- Replace byte-at-a-time copy in `PropertyFileIncluded::RestoreDocFile()` with
  buffered copy.
- Preserve `aboutToSetValue()`, `hasSetValue()`, destination permissions, and
  existing exception behavior.
- Add write/close error checks if they can be done consistently with nearby
  code.

Then investigate shape/map restore:

- Add narrower timings in `ComplexGeoData::RestoreDocFile()` and
  `ElementMap::restore()`.
- Separate stream read, parsing, string hasher lookup, element-map allocation,
  and map insertion costs.
- Only optimize after the dominant sub-step is known.

Then investigate GUI document restore:

- Add targeted timings in `Gui::Document::RestoreDocFile()` and view-provider
  restore paths.
- Confirm whether `GuiDocument.xml` time is XML parsing, view-provider
  construction, icon/image setup, or scene graph work.

### Research Needed Before Implementation

- Confirm `Base::Reader::read()` respects embedded zip entry boundaries for
  chunked reads exactly as the byte loop does.
- Check whether any `PropertyFileIncluded` users rely on side effects of
  byte-by-byte streaming, partial reads, or EOF state.
- Review existing stream copy helpers in `Base` before adding a local buffer.
- Inspect `ElementMap::restore()` and related string hasher code before
  choosing any shape-map optimization.
- Compare debug and RelWithDebInfo timings before prioritizing deeper parser
  work.

### Validation

- Re-run `FREECAD_RESTORE_TRACE=1` on the same Gridfinity document.
- Compare the specific included-file entries:
  - `270mm slider drawer side1.png`
  - `270mm slider cabinet side1.png`
  - `IMG_2599D-processed.png`
- Confirm document opens with the image planes intact and saved document output
  remains compatible.
- Run focused App/Base tests affected by stream and property-file behavior, then
  a broader `pixi run test` if the patch touches shared stream code.

## Priority 2: Image Decode, Scaling, And Bitmap Conversion

### Evidence

Normal startup and interactive `perf` captures both showed repeated image work:

- `qt_qimageScaleAARGBA_down_xy_sse4`
- `qt_memfillXX_avx2`
- `inflate_fast`, `inflate`, and PNG decode/filter paths
- `Gui::BitmapFactoryInst::convert(QImage const&, SoSFImage&)`

Follow-up `FREECAD_IMAGE_TRACE=1` results split this into two different
problems:

- Startup icon/SVG work is broad but mostly small one-time work:
  - `loadPixmap.hit`: `237`
  - `loadPixmap.miss`: `443`
  - `pixmapFromSvg.bytes`: `240`
  - largest individual startup icon/SVG load: about `6ms`
- Document image-plane work is large and concentrated:
  - two `5712x4284` image planes took about `0.68-0.69s` each to decode
  - the same two images took about `0.85-0.96s` each to convert to `SoSFImage`
  - a `1785x1959` image took about `0.16s` to decode and `0.26s` to convert

### Hypothesis

The strongest image-related document-open cost is not icon cache misses. It is
full-resolution image-plane decode followed by full-resolution conversion into
Coin texture data. Startup icon work may still be worth cleanup, but it is lower
priority than the image-plane path.

### Implementation Direction

Keep the tracing, but change the first implementation focus:

1. Optimize `BitmapFactoryInst::convert(QImage, SoSFImage)` for common 32-bit
   `QImage` formats.
   - Avoid `QImage::pixelColor(x, y)` in the inner loop.
   - Use `constScanLine()` / direct pixel access for `Format_ARGB32`,
     `Format_ARGB32_Premultiplied`, and related 32-bit formats.
   - Preserve vertical flip behavior expected by the existing Coin image data.
   - Keep fallback behavior for indexed, grayscale, and uncommon formats.
2. Investigate image-plane load timing.
   - `ViewProviderImagePlane::loadImage()` currently attempts cheap failed
     loads before the included files exist, then loads again after restore.
   - Determine whether included file payloads can defer image load until after
     `RestoreDocFile()` has completed.
3. Consider lazy or lower-resolution image-plane texture creation.
   - Defer full-resolution `SoSFImage` conversion until the image plane is
     visible or until the 3D view first needs it.
   - Consider initial display-resolution textures for very large images.
4. Treat startup icon/SVG caching as a later cleanup.
   - Direct `BitmapFactory::pixmap()` requests are cached after first load.
   - Direct `pixmapFromSvg(name, size, colors)` calls may benefit from a cache
     keyed by name, size, color mapping, theme generation, and DPR.

Avoid broad caching until invalidation rules are clear.

### Research Needed Before Implementation

- Confirm exact pixel layout for the common `QImage` formats used by the traced
  PNGs. The trace showed format `5` for the large decoded PNGs.
- Verify `SoSFImage` component ordering expectations and alpha behavior.
- Check whether premultiplied alpha needs unpremultiplication before handing
  data to Coin, or whether current visual output relies on the existing channel
  values.
- Identify ownership and copy behavior of `SoSFImage::setValue()` /
  `startEditing()` so direct copy remains safe.
- Inspect image-plane restore lifecycle to find a clean "embedded files are now
  available" point.
- For SVG/icon caching, identify theme/style/DPR invalidation hooks before
  adding persistent cache entries.

### Validation

- Re-run `FREECAD_IMAGE_TRACE=1` on the Gridfinity document and compare:
  - `convert.QImageToSoSFImage.5712x4284`
  - `convert.QImageToSoSFImage.1785x1959`
  - `imagePlane.loadImage`
- Visually compare the image planes before/after for orientation, alpha, color,
  and scale.
- Test at least one SVG image plane and one PNG image plane.
- Compare startup traces if any icon/SVG cache changes are made.
- Test theme switch, icon size preference changes, workbench switch, document
  open/close, and high-DPI behavior for any icon cache work.

## Priority 3: Tree / Document UI Churn

### Evidence

Tree tracing for the Gridfinity document showed frequent object-level signals
but relatively small total cost in the tested document:

- `DocumentItem.slotNewObject.calls`: `127`
- `DocumentItem.createNewItem.created`: `128`
- Tree item creation/population/icon/status work: tens of milliseconds
- `TreeWidget.statusTimer.start`: `178`
- `TreeWidget.onUpdateStatus.deferred.dragOrRestore`: `27`
- `TreeWidget.slotDeleteDocument.us`: about `11ms`

### Hypothesis

Tree churn is not the main source of the current multi-second open lag, but it
has scaling risk for larger documents and creates repeated UI work that may
compound with other startup/open costs.

### Implementation Direction

Keep this lower priority unless larger documents show worse scaling.

Potential changes:

- Replace repeated status timer restarts during restore with a single
  "status update needed after restore" flag.
- Add a bulk document-delete path that removes document items and clears maps
  without calling `_slotDeleteObject()` for every object when the entire
  document is being removed.
- Cache tree icon/status decoration data by status, visibility, mode, and icon
  size where correctness is clear.

### Research Needed Before Implementation

- Inspect all callers and assumptions around `_slotDeleteObject()` before
  bypassing it for bulk delete.
- Confirm whether selection, expansion state, drag state, and Python observers
  require per-object delete notifications in the tree layer.
- Test a larger document with many more objects before committing to tree
  optimization work.

### Validation

- Re-run `FREECAD_TREE_TRACE=1` on Gridfinity and one larger document.
- Confirm creation/deletion counts and UI behavior stay correct.
- Manually test selection, tree expansion, object visibility, close document,
  close all, and document reload.

## Priority 4: Normal Startup Addon And Preference Churn

### Evidence

Normal startup loads many workbenches/addons and repeatedly logs unknown or
stale workbench preferences, including `GridfinityWorkbench`. Telemetry also
runs during startup and shutdown.

### Hypothesis

Normal startup may be doing repeated preference resolution, addon discovery, or
telemetry work that can be deferred, cached, or made less synchronous.

### Implementation Direction

Start with tracing, not code changes:

- Count workbench preference lookups and unknown-workbench warnings.
- Time addon discovery, workbench initialization, and telemetry send paths.
- Identify repeated warnings that can be coalesced without hiding useful
  diagnostics.

### Research Needed Before Implementation

- Understand addon manager/workbench discovery lifecycle and what must happen
  before the first window is shown.
- Check telemetry opt-in/opt-out behavior and whether sends are synchronous.
- Review preference pack behavior and stale workbench cleanup options.

### Validation

- Compare normal startup log and `perf` profile before/after.
- Test with the user's normal profile and with a cleaner profile to separate
  core behavior from local addon state.

## Build And Measurement Plan

Use the same document and workflows for before/after comparisons:

- Startup only.
- Open `/home/bcherrington/Projects/FreeCAD/Drawer/Gridfinity Drawer v1.3.FCStd`.
- Close the document.

Recommended measurement sequence:

1. Build with `pixi run build -j 2`.
2. Run the relevant opt-in trace for the target area.
3. Capture `perf record -F 99 -g --call-graph fp -B` for the same workflow if
   CPU profile comparison is needed.
4. Record absolute timings from FreeCAD logs and trace summaries.
5. Repeat at least twice if timings vary substantially.
6. For final decisions, compare with a RelWithDebInfo build where feasible.

## Proposed Implementation Order

1. Implement buffered restore for `PropertyFileIncluded::RestoreDocFile()`.
   This is small, local, and directly tied to measured large-file restore cost.
2. Add narrower shape-map restore timings and decide whether there is a safe
   parser/data-structure optimization.
3. Optimize the common 32-bit `QImage` to `SoSFImage` conversion path and
   remeasure the large image planes. Defer broader icon/SVG caching until a
   separate trace shows cacheable duplicate work with clear invalidation rules.
4. Revisit tree batching after testing a larger document or after the higher
   priority restore and image issues are addressed.
5. Investigate addon/preference startup churn if startup remains a user-visible
   complaint after document-open restore and image costs are improved.

## Task Breakdown

### Epic A: Restore Measurement Baseline

- **A1 - Reproduce the baseline trace**
  - Scope: run the existing `FREECAD_RESTORE_TRACE=1` workflow against the same
    Gridfinity document in the same build mode.
  - Output: before timings for `Document.restore.readFiles`, the three large
    included PNG payloads, shape-map payloads, BREP payloads, and
    `GuiDocument.xml`.
  - Exit criteria: timings are recorded in the implementation notes with build
    type, commit, document path, and run count.
- **A2 - Confirm stream boundary assumptions**
  - Scope: verify `Base::Reader::read()` and the `zipios::ZipInputStream` path
    stop at the same embedded-file boundary as the current byte loop.
  - Output: reviewer note explaining why chunked reads cannot consume the next
    registered zip entry.
  - Exit criteria: the buffered copy patch has an explicit correctness rationale
    and keeps the old behavior available conceptually through bounded reads.

### Epic B: Buffered Included-File Restore

- **B1 - Implement chunked copy in `PropertyFileIncluded::RestoreDocFile()`**
  - Scope: replace the byte-at-a-time restore loop in
    `src/App/PropertyFile.cpp` with a fixed-size binary buffer.
  - Preserve: `aboutToSetValue()`, `hasSetValue()`, read-only permissions, early
    return for existing non-writable files, and current failure semantics.
  - Include: explicit destination write and close error handling where it fits
    nearby FreeCAD error style.
  - Exit criteria: large included files restore intact and the patch does not
    change document archive format or property notifications.
- **B2 - Add focused restore-copy validation**
  - Scope: add or update the narrowest stable automated coverage available for
    `PropertyFileIncluded` binary restore behavior.
  - Validate: empty files, binary files with embedded zeros, large files, and
    the existing non-writable-file short-circuit.
  - Exit criteria: focused tests pass locally, or manual validation is clearly
    documented if a practical automated test hook is not available.
- **B3 - Remeasure included-file restore**
  - Scope: re-run `FREECAD_RESTORE_TRACE=1` after B1.
  - Compare: `270mm slider drawer side1.png`,
    `270mm slider cabinet side1.png`, `IMG_2599D-processed.png`, and total
    `Document.restore.readFiles`.
  - Exit criteria: before/after numbers are included in PR notes and any
    remaining dominant restore bucket is identified.

### Epic C: Shape-Map And GUI Restore Investigation

- **C1 - Instrument shape-map restore narrowly**
  - Scope: split timings inside `ComplexGeoData::RestoreDocFile()` and
    `ElementMap::restore()`.
  - Measure: stream read, token parsing, string-hasher lookup, map allocation,
    and insertion/update work.
  - Exit criteria: a follow-up patch is either justified by one dominant
    sub-step or deferred with evidence.
- **C2 - Instrument GUI document restore narrowly**
  - Scope: split `Gui::Document::RestoreDocFile()` and view-provider restore
    timing if `GuiDocument.xml` remains high after B1.
  - Measure: XML parse, view-provider construction, image/icon setup, and scene
    graph attachment.
  - Exit criteria: the next GUI restore task is scoped to one subsystem rather
    than a general GUI restore rewrite.

### Epic D: Image Conversion Baseline

- **D1 - Reproduce image-plane trace**
  - Scope: run `FREECAD_IMAGE_TRACE=1` on the same Gridfinity document before
    image conversion changes.
  - Output: timings for `imagePlane.loadRaster`,
    `imagePlane.convertToSFImage`, `imagePlane.loadImage`, and aggregate
    `convert.QImageToSoSFImage.*`.
  - Exit criteria: baseline records include image dimensions, `QImage::format()`
    values, build type, and run count.
- **D2 - Define image correctness fixtures**
  - Scope: select a small validation set covering opaque RGB PNG, alpha PNG,
    the traced large PNG format, premultiplied-alpha input if reachable, and SVG
    image planes.
  - Output: manual or automated comparison checklist for orientation, color,
    alpha, scale, and plane size.
  - Exit criteria: reviewers can tell which visual cases were checked and which
    remain fallback-only.

### Epic E: Fast `QImage` To `SoSFImage` Conversion

- **E1 - Add direct scanline fast path for common 32-bit formats**
  - Scope: optimize `BitmapFactoryInst::convert(const QImage&, SoSFImage&)` for
    known 32-bit formats using `constScanLine()` or equivalent direct access.
  - Preserve: vertical flip, RGBA component ordering expected by Coin,
    `SoSFImage` ownership/editing rules, and fallback behavior for indexed,
    grayscale, 16-bit, and uncommon formats.
  - Exit criteria: visual fixture comparison passes and uncommon formats still
    route through the existing conservative path.
- **E2 - Validate alpha and premultiplied behavior**
  - Scope: confirm whether the fast path must unpremultiply
    `Format_ARGB32_Premultiplied` or leave values as currently observed through
    `pixelColor()`.
  - Exit criteria: alpha behavior is documented in code comments only if the
    conversion choice is non-obvious, and the PR notes call out the tested
    alpha cases.
- **E3 - Remeasure image conversion**
  - Scope: re-run `FREECAD_IMAGE_TRACE=1` after E1.
  - Compare: `convert.QImageToSoSFImage.5712x4284`,
    `convert.QImageToSoSFImage.1785x1959`, and total `imagePlane.loadImage`.
  - Exit criteria: before/after numbers are included in PR notes and any
    remaining image-plane bottleneck is identified.

### Epic F: Image-Plane Lifecycle Follow-Ups

- **F1 - Investigate pre-restore failed image loads**
  - Scope: trace why `ViewProviderImagePlane::loadImage()` is called before
    included files are available.
  - Output: lifecycle note identifying the clean point where embedded files are
    guaranteed restored.
  - Exit criteria: either a small defer/retry patch is proposed or the failed
    loads remain documented as cheap and low priority.
- **F2 - Prototype lazy or reduced-resolution texture creation**
  - Scope: investigate deferring full-resolution `SoSFImage` creation until the
    image plane is visible or using a display-resolution texture first.
  - Constraints: do not change source file semantics, plane dimensions, or user
    expectations for image-plane fidelity without a separate design decision.
  - Exit criteria: prototype results show whether this is worth a future spec
    or should be dropped.

### Epic G: Lower-Priority UI And Startup Work

- **G1 - Tree status timer batching**
  - Scope: replace repeated status timer restarts during restore with one
    pending-status flag only if larger-document traces show meaningful cost.
  - Validate: document open, close, close all, reload, drag state, expansion
    state, selection, and Python observer behavior.
  - Exit criteria: no change is made until a larger trace proves the current
    tens-of-milliseconds cost is worth review risk.
- **G2 - Tree bulk-delete investigation**
  - Scope: inspect `_slotDeleteObject()` side effects before bypassing
    per-object deletion during whole-document removal.
  - Exit criteria: a design note lists required side effects, map invariants,
    and observer/selection implications before any patch.
- **G3 - Startup addon and preference tracing**
  - Scope: count stale workbench preference lookups, addon discovery time,
    workbench initialization time, and telemetry send behavior.
  - Exit criteria: startup work is ranked after document-open changes using a
    normal user profile and a clean control profile.

### Epic H: Review, Cleanup, And Release Notes

- **H1 - Remove or split temporary instrumentation**
  - Scope: delete one-off trace code from optimization patches, or submit a
    separate maintained diagnostic patch with opt-in controls.
  - Exit criteria: production patches do not contain accidental stderr logging,
    `atexit` summaries, or broad trace helpers.
- **H2 - Build and test final patch set**
  - Scope: run focused tests for touched subsystems and `pixi run build -j 2`.
  - Broaden: run `pixi run test` when shared Base/App/Gui behavior changes.
  - Exit criteria: PR notes list exact commands, results, and any skipped
    validation with a reason.
- **H3 - Prepare review notes**
  - Scope: document behavior preservation, before/after timings, remaining
    hotspots, and follow-up tasks.
  - Exit criteria: each production PR is independently reviewable and does not
    require accepting unrelated tracing or lower-priority cleanup.
- **H4 - Preserve cherry-pickable commit boundaries**
  - Scope: keep each production optimization, validation update, and retained
    diagnostic change in separate coherent commits.
  - Exit criteria: a future PR can cherry-pick the desired optimization commits
    without dragging in unrelated research notes, temporary trace code, or
    lower-priority follow-ups.

## Cleanup Plan

Temporary instrumentation should be removed or consolidated before production
work is proposed for review.

Cleanup checklist:

- Remove one-off signal handlers and `atexit` trace summaries unless a
  maintained diagnostic mode is intentionally kept.
- Remove large stderr trace output from normal code paths.
- Keep any retained instrumentation opt-in and documented.
- Keep optimization patches separate from profiling-instrumentation patches so
  review can evaluate behavior changes cleanly.
