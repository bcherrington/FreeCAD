---
title: FreeCAD performance optimization tasks
doc_type: spec
status: draft
owner: local-developer
last_reviewed: 2026-05-22
---

# Task Breakdown

This checklist tracks implementation-sized work for the performance
optimization package. The detailed rationale and validation expectations are in
[plan.md](plan.md); source evidence is in [research.md](research.md).

## P1: Restore Baseline And Buffered Included Files

- [ ] Reproduce the `FREECAD_RESTORE_TRACE=1` baseline for the Gridfinity
      document in the same build mode.
- [ ] Confirm `Base::Reader::read()` chunked reads preserve embedded zip entry
      boundaries in the `XMLReader::readFiles()` restore path.
- [ ] Replace byte-at-a-time copy in
      `PropertyFileIncluded::RestoreDocFile()` with bounded buffered copy.
- [ ] Preserve property notifications, read-only destination permissions,
      existing non-writable-file behavior, and failure semantics.
- [ ] Add focused validation for binary included-file restore behavior.
- [ ] Remeasure the three large included PNG restore entries and total
      `Document.restore.readFiles`.
- [ ] Record before/after timings and build/test commands in review notes.

## P1: Image Conversion Baseline And Fast Path

- [ ] Reproduce the `FREECAD_IMAGE_TRACE=1` baseline for large image planes.
- [ ] Record dimensions and `QImage::format()` values for the traced PNG image
      planes.
- [ ] Define visual validation fixtures for RGB PNG, alpha PNG, traced large
      PNG, premultiplied-alpha input if reachable, and SVG image planes.
- [ ] Add a direct scanline fast path in
      `BitmapFactoryInst::convert(const QImage&, SoSFImage&)` for known common
      32-bit formats.
- [ ] Preserve vertical flip, component ordering, alpha behavior, and
      `SoSFImage` data ownership.
- [ ] Keep the existing conservative conversion path for uncommon formats.
- [ ] Remeasure `convert.QImageToSoSFImage.*` and `imagePlane.loadImage`.
- [ ] Capture before/after screenshots or manual notes for orientation, color,
      alpha, and scale.

## P2: Restore Follow-Up Investigation

- [ ] Add narrow timings inside `ComplexGeoData::RestoreDocFile()` and
      `ElementMap::restore()`.
- [ ] Split shape-map timing into stream read, parsing, string-hasher lookup,
      allocation, and insertion/update cost.
- [ ] Decide whether shape-map restore has a safe optimization target or should
      be deferred.
- [ ] Add narrow `Gui::Document::RestoreDocFile()` and view-provider timings if
      `GuiDocument.xml` remains a major restore bucket after buffered copy.
- [ ] Scope any GUI restore work to the dominant measured sub-step.

## P2: Image-Plane Lifecycle Follow-Ups

- [ ] Trace why `ViewProviderImagePlane::loadImage()` can run before included
      files are available.
- [ ] Identify the clean lifecycle point where embedded files are guaranteed
      restored.
- [ ] Decide whether failed pre-restore image loads should be deferred,
      retried, or left alone as low-cost noise.
- [ ] Prototype lazy full-resolution texture creation only after the direct
      conversion fast path is measured.
- [ ] Prototype reduced-resolution initial textures only as a separate behavior
      decision that preserves source image semantics.

## P3: Lower-Priority UI And Startup Work

- [ ] Re-run tree tracing on a larger document before changing tree batching.
- [ ] Investigate replacing repeated restore-time status timer restarts with a
      single pending-status flag.
- [ ] Document `_slotDeleteObject()` side effects before proposing any
      whole-document tree bulk-delete path.
- [ ] Trace stale workbench preference lookups, addon discovery, workbench
      initialization, and telemetry send behavior for normal startup.
- [ ] Rank startup work only after document-open restore and image-plane costs
      are remeasured.

## Cleanup And Review Gates

- [ ] Remove one-off trace code from production optimization patches, or split
      maintained diagnostics into a separate opt-in review.
- [ ] Keep restore, image conversion, lifecycle, tree, and startup changes in
      separate reviewable patches.
- [ ] Commit changes with coherent boundaries so production fixes can be
      cherry-picked into a later PR without unrelated tracing or follow-up work.
- [ ] Run `pixi run build -j 2` for production code changes.
- [ ] Run focused CTest coverage for touched subsystems; broaden to
      `pixi run test` when shared Base/App/Gui behavior changes.
- [ ] Include exact commands, before/after timings, manual validation notes,
      and remaining hotspots in each PR description.
