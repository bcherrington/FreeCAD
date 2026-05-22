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

- [x] Reproduce the restore baseline for the Gridfinity document in the same
      build mode using a clean detached worktree, since the temporary
      `FREECAD_RESTORE_TRACE=1` instrumentation has been removed.
- [x] Confirm `Base::Reader::read()` chunked reads preserve embedded zip entry
      boundaries in the `XMLReader::readFiles()` restore path.
- [x] Replace byte-at-a-time copy in
      `PropertyFileIncluded::RestoreDocFile()` with bounded buffered copy.
- [x] Preserve property notifications, read-only destination permissions,
      existing non-writable-file behavior, and failure semantics.
- [x] Add focused validation for binary included-file restore behavior.
- [x] Remeasure the restore/open path for the same large-image Gridfinity
      document after the buffered included-file copy change.
- [x] Record before/after timings and build/test commands in review notes.

### Restore Baseline Measurement - 2026-05-22

The removed `FREECAD_RESTORE_TRACE=1` instrumentation means this measurement
uses a repeatable whole-document `FreeCAD.openDocument()` benchmark instead of
per-entry `Document.restore.readFiles` buckets.

Benchmark fixture:
`/home/bcherrington/Projects/FreeCAD/Drawer/Gridfinity Drawer v1.3.FCStd`

Benchmark body:

```python
import os
import time

import FreeCAD

path = os.environ["FREECAD_BENCH_FILE"]
start = time.perf_counter()
doc = FreeCAD.openDocument(path)
open_elapsed = time.perf_counter() - start
objects = len(doc.Objects)
FreeCAD.closeDocument(doc.Name)
print(f"BENCH open_seconds={open_elapsed:.6f} objects={objects} file={path}")
```

Baseline setup:

```sh
git worktree add --detach /tmp/freecad-restore-baseline HEAD
cd /tmp/freecad-restore-baseline
pixi run configure
cmake --build build/debug --target FreeCADCmd -j 2
cmake --build build/debug --target Part Sketcher PartDesign Mesh Spreadsheet Assembly Import TechDraw -j 2
ninja -C build/debug Mod/PartDesign/Init.py Mod/PartDesign/__init__.py Mod/Sketcher/Init.py Mod/Part/Init.py Mod/Material/Init.py Mod/Mesh/Init.py Mod/Spreadsheet/Init.py Mod/Import/Init.py Mod/Assembly/Init.py Mod/TechDraw/Init.py
```

The Material target graph was also built so the baseline command build could
load all 127 document objects without "Cannot create object" restore errors.

Benchmark command shape for both baseline and optimized trees:

```sh
mod_paths=
for d in "$PWD"/build/debug/Mod/*; do
    [ -d "$d" ] && mod_paths="${mod_paths}:$d"
done
export LD_LIBRARY_PATH="$PWD/build/debug/lib${mod_paths}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
FREECAD_BENCH_FILE="$HOME/Projects/FreeCAD/Drawer/Gridfinity Drawer v1.3.FCStd" \
    pixi run build/debug/bin/FreeCADCmd --safe-mode /tmp/freecad_restore_bench.py
```

Results, five fresh `FreeCADCmd --safe-mode` processes per build:

| Build | Runs, seconds | Median | Mean | Notes |
| --- | ---: | ---: | ---: | --- |
| Baseline `HEAD` detached worktree | 7.920593, 3.556128, 3.347960, 3.657738, 3.747753 | 3.657738 | 4.446034 | First run includes cold-cache/module setup cost. |
| Buffered `PropertyFileIncluded::RestoreDocFile()` | 2.931941, 2.908889, 2.874808, 4.290493, 3.613156 | 2.931941 | 3.323857 | Same fixture, object count 127, no restore errors. |

Comparison:

- Median all-runs improvement: 19.8%.
- Warm-run median improvement, excluding the first process from each build:
  9.6%.

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

- [x] Trace why `ViewProviderImagePlane::loadImage()` can run before included
      files are available.
- [x] Identify the clean lifecycle point where embedded files are guaranteed
      restored.
- [x] Decide whether failed pre-restore image loads should be deferred,
      retried, or left alone as low-cost noise.
- [ ] Prototype lazy full-resolution texture creation only after the direct
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

Validation:

- `cmake --build build/debug --target Gui_tests_run -j 2`
- `QT_QPA_PLATFORM=offscreen build/debug/tests/Gui_tests_run --gtest_filter='BitmapFactoryTest.*'`
- `xvfb-run -a timeout 45s pixi run build/debug/bin/FreeCAD
  /tmp/freecad_gui_imageplane_open.py --pass
  "/home/bcherrington/Projects/FreeCAD/Drawer/Gridfinity Drawer v1.3.FCStd"`
  reported `image_planes=3`, `missing_paths=0`, and `gui_doc=True`.

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
