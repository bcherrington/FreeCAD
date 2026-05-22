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

- [x] Add narrow timings inside `ComplexGeoData::RestoreDocFile()` and
      `ElementMap::restore()`.
- [x] Split shape-map timing into stream read, parsing, string-hasher lookup,
      allocation, and insertion/update cost.
- [x] Decide whether shape-map restore has a safe optimization target or should
      be deferred.
- [ ] Add narrow `Gui::Document::RestoreDocFile()` and view-provider timings if
      `GuiDocument.xml` remains a major restore bucket after buffered copy.
- [ ] Scope any GUI restore work to the dominant measured sub-step.

### Restore Follow-Up Measurement - 2026-05-22

Temporary env-gated timings were added locally for
`ComplexGeoData::RestoreDocFile()` and `ElementMap::restore()`, then removed
before committing. The sample set included the Gridfinity drawer file plus
multiple bundled sample documents so the result was not based on one model.
The temporary sub-step timers are useful for ranking hot code inside the shape
map restore path; they should not be treated as exact wall-clock accounting
because the probe itself adds overhead in the tight parsing loops.

| Model | Objects | Shape maps | Map bytes | Open total | Element-map restore | Main measured cost |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `Gridfinity Drawer v1.3.FCStd` | 127 | 50 | 3.7MB | 3.221s | 3.415s | Name loop: split 1.305s, string-id lookup 0.662s, map insertion 0.514s |
| `data/examples/BIMExample.FCStd` | 361 | 177 | 1.5MB | 4.659s | 1.237s | Name loop: insertion 0.633s, split 0.378s |
| `data/examples/ArchDetail.FCStd` | 435 | 173 | 0.5MB | 4.993s | 0.495s | Name loop: insertion 0.174s, split 0.169s |
| `data/examples/EngineBlock.FCStd` | 36 | 19 | 14KB | 0.655s | 0.011s | Too small to optimize first |
| `data/examples/PartDesignExample.FCStd` | 17 | 9 | 14KB | 0.499s | 0.015s | Too small to optimize first |
| `data/examples/draft_test_objects.FCStd` | 113 | 12 | 7KB | 0.711s | 0.003s | Too small to optimize first |

Findings:

- Shape-map restore is still material for Part/BIM-heavy documents after the
  included-file copy fix.
- The dominant measured region is `ElementMap::restore()` name restoration, not
  allocation. In the larger models, repeated token splitting/parsing,
  `mappedNames.emplace()`, and `StringHasher::getID()` account for most of the
  narrow timing.
- A safe next optimization target exists: replace the repeated
  `boost::split()`/temporary-token path in `ElementMap::restore()` with a
  bounded parser that scans dot-separated fields without allocating a new
  `std::vector<std::string>` per name or child string-id entry. Keep that as a
  separate production patch with focused map-restore tests and the same
  multi-model measurement set.
- GUI restore timing was not committed. The first follow-up should use a real
  GUI document-open harness before changing `Gui::Document::RestoreDocFile()`;
  command-mode opens do not prove that path.

## P2: Image-Plane Lifecycle Follow-Ups

- [x] Trace why `ViewProviderImagePlane::loadImage()` can run before included
      files are available.
- [x] Identify the clean lifecycle point where embedded files are guaranteed
      restored.
- [x] Decide whether failed pre-restore image loads should be deferred,
      retried, or left alone as low-cost noise.
- [x] Prototype lazy full-resolution texture creation only after the direct
      conversion fast path is measured.
- [ ] Prototype reduced-resolution initial textures only as a separate behavior
      decision that preserves source image semantics.

### Image-Plane Restore Lifecycle - 2026-05-22

Source review showed the restore ordering:

- `App::Document::restore()` restores `Document.xml`.
- `Gui::Document::Restore()` adds `GuiDocument.xml` as an included restore file
  and marks view providers as restoring.
- `Gui::Document::RestoreDocFile()` restores view-provider properties before
  `reader.readFiles(zipstream)` extracts all included payloads.
- `App::Document::afterRestore()` later emits per-object
  `signalFinishRestoreObject`, which calls
  `ViewProviderDocumentObject::finishRestoring()` after included files exist.

`ViewProviderImagePlane::updateData(ImageFile)` now defers a restore-time image
load when the referenced file is not available yet, then retries from
`finishRestoring()`. This avoids a known failed pre-restore load without
changing normal user edits, external-file behavior, or image source semantics.

The lazy full-resolution texture prototype builds on that lifecycle decision:
restore-time `ImageFile`, `XSize`, and `YSize` updates mark the full-resolution
texture as pending instead of decoding/converting immediately. Visible image
planes materialize their original full-resolution texture at
`finishRestoring()`, while hidden image planes wait until `show()`. This does
not introduce reduced-resolution placeholder textures.

Validation:

- `cmake --build build/debug --target Gui_tests_run -j 2`
- `QT_QPA_PLATFORM=offscreen build/debug/tests/Gui_tests_run --gtest_filter='BitmapFactoryTest.*'`
- `xvfb-run -a timeout 45s pixi run build/debug/bin/FreeCAD
  /tmp/freecad_gui_imageplane_open.py --pass
  "/home/bcherrington/Projects/FreeCAD/Drawer/Gridfinity Drawer v1.3.FCStd"`
  reported `image_planes=3`, `missing_paths=0`, and `gui_doc=True`.
- The same GUI smoke after the lazy prototype reported `image_planes=3`,
  `visible=0`, `missing_paths=0`, and `gui_doc=True`; toggling one image plane
  visible after restore reported `first_visible=True` and
  `first_path_exists=True`.

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
