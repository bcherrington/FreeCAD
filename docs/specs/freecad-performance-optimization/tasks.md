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

- [x] Reproduce the `FREECAD_IMAGE_TRACE=1` baseline for large image planes.
- [x] Record dimensions and `QImage::format()` values for the traced PNG image
      planes.
- [x] Define visual validation fixtures for RGB PNG, alpha PNG, traced large
      PNG, premultiplied-alpha input if reachable, and SVG image planes.
- [x] Add a direct scanline fast path in
      `BitmapFactoryInst::convert(const QImage&, SoSFImage&)` for known common
      32-bit formats.
- [x] Preserve vertical flip, component ordering, alpha behavior, and
      `SoSFImage` data ownership.
- [x] Keep the existing conservative conversion path for uncommon formats.
- [x] Remeasure `convert.QImageToSoSFImage.*` and `imagePlane.loadImage`.
- [x] Capture before/after screenshots or manual notes for orientation, color,
      alpha, and scale.

### Image Conversion Measurement - 2026-05-22

The `FREECAD_IMAGE_TRACE=1` baseline came from the removed temporary trace
recorded in [research.md](research.md). No trace/profiling code is part of the
production patch.

Baseline trace:

| Image | Size | QImage format | Load raster | Convert to `SoSFImage` | `loadImage` total |
| --- | ---: | ---: | ---: | ---: | ---: |
| `270mm slider cabinet side1.png` | 5712x4284 | 5 (`Format_ARGB32`) | 692.752 ms | 964.213 ms | 1662.185 ms |
| `270mm slider drawer side1.png` | 5712x4284 | 5 (`Format_ARGB32`) | 677.225 ms | 854.966 ms | 1537.531 ms |
| `IMG_2599D-processed.png` | 1785x1959 | 5 (`Format_ARGB32`) | 161.491 ms | 261.029 ms | 422.688 ms |

After the direct scanline path, a throwaway benchmark executable under `/tmp`
linked against the built `FreeCADGui` library and timed fresh `QImage` loads
plus `BitmapFactory().convert()` for the same extracted PNG entries. This was
used instead of reintroducing production trace code.

| Image | Convert median | Load+convert median | Convert improvement |
| --- | ---: | ---: | ---: |
| `270mm slider cabinet side1.png` | 221.792 ms | 930.569 ms | 77.0% |
| `270mm slider drawer side1.png` | 229.304 ms | 944.450 ms | 73.2% |
| `IMG_2599D-processed.png` | 31.911 ms | 152.756 ms | 87.8% |

Validation notes:

- `tests/src/Gui/BitmapFactory.cpp` covers vertical flip, RGBA component order,
  and alpha handling for `Format_ARGB32`, `Format_RGB32`, `Format_RGBA8888`,
  and `Format_RGBX8888`.
- Premultiplied, indexed, grayscale, 64-bit, SVG-rendered, and other uncommon
  formats still use the existing `pixelColor()` fallback path.
- SVG image planes are unaffected by the fast path until their rendered
  `QImage` is one of the explicitly handled 32-bit formats.

## P2: Restore Follow-Up Investigation

- [x] Add narrow timings inside `ComplexGeoData::RestoreDocFile()` and
      `ElementMap::restore()`.
- [x] Split shape-map timing into stream read, parsing, string-hasher lookup,
      allocation, and insertion/update cost.
- [x] Decide whether shape-map restore has a safe optimization target or should
      be deferred.
- [x] Implement the parser-level shape-map restore fast path as a separate
      production patch.
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

### Shape-Map Parser Fast Path - 2026-05-22

`ElementMap::restore()` now parses dot-separated child string-id and mapped-name
entries with a small `std::string_view` tokenizer instead of repeatedly calling
`boost::split()` into a temporary `std::vector<std::string>`.

Validation:

- `build/debug/tests/App_tests_run --gtest_filter='ElementMapTest.*'`
- `build/debug/tests/App_tests_run`
- `pixi run build -j 2`
- Command-mode open smoke for:
  `Gridfinity Drawer v1.3.FCStd`, `BIMExample.FCStd`, `ArchDetail.FCStd`,
  `EngineBlock.FCStd`, `PartDesignExample.FCStd`, and
  `draft_test_objects.FCStd`.

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
