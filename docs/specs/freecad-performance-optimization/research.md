---
title: FreeCAD profiling findings
doc_type: spec
status: draft
owner: local-developer
last_reviewed: 2026-05-22
---

# Research: FreeCAD Profiling Findings

## Question

Which normal-working-mode FreeCAD performance hotspots are worth turning into
small, evidence-led optimization patches?

This note summarizes the profiling work done against the local debug build of
FreeCAD and lists the next investigation steps. The goal is to identify
normal-working-mode optimization candidates, especially work that is invoked
often or repeatedly expensive during real use.

## Environment

- Build: `build/debug/bin/FreeCAD`
- Version: `FreeCAD 1.2.0 Revision: 46969 +53 (Git)`
- Branch reported by FreeCAD: `jetbrains-style-compact-ui`
- Graphics: OpenGL `4.6 (Compatibility Profile) Mesa 25.2.8-0ubuntu0.24.04.1`
- Profiling tool: Linux `perf`
- Capture mode used for useful results:
  - `perf record -F 99 -g --call-graph fp -B`

`perf --call-graph dwarf` was attempted first, but it wedged in `addr2line`
symbolization for large debug libraries such as `libFreeCADGui.so` and produced
an invalid `perf.data` file. Frame-pointer call graphs completed reliably, but
the stacks are less complete because not every library preserves frame pointers.

The host `kernel.perf_event_paranoid` setting was temporarily changed from `4`
to `0` to allow profiling, then restored to `4`.

## Captured Artifacts

- Non-safe startup profile:
  - `/tmp/freecad-startup-normal-fp.perf.data`
  - `/tmp/freecad-startup-normal.log`
- Interactive normal-mode profile:
  - `/tmp/freecad-interactive-fp.perf.data`
  - `/tmp/freecad-interactive.log`
- Earlier safe-mode startup profile:
  - `/tmp/freecad-startup-fp.perf.data`
  - `/tmp/freecad-perf-record-fp.log`

Safe mode is only useful as a control sample. The primary findings below are
from normal mode.

## Important Caveats

`perf record` is sampled profiling. It estimates where CPU time was spent; it
does not directly count function invocations. A function that appears heavily in
the profile may be called frequently, individually expensive, or both.

Exact invocation counts need targeted instrumentation or tracing. The log gives
some real counts for tree-view operations, but not for image scaling, PNG
decoding, or OpenGL framebuffer allocation.

Kernel and graphics-driver samples were partly unresolved because kernel symbol
maps are restricted. This affects the OpenGL/driver area: the user-space Qt
callers are visible, but the driver internals are not.

## Findings

### 1. Normal Startup Loads Many Addons

Normal startup is materially heavier than safe-mode startup because it loads
many user workbenches and addons, including examples such as:

- `Telemetry`
- `Gridfinity`
- `Curves`
- `fasteners`
- `CfdOF`
- `OSHAutoDocWorkbench`
- `freecad.gridfinity_workbench`
- `freecad.Mesh_Remodel`
- `freecad.Dynamic_Data`
- `freecad.gears`

The log repeatedly reports stale or unknown workbench preferences, especially
`GridfinityWorkbench`. This is not necessarily a dominant CPU hotspot, but it
is repeated normal-mode startup work and should be checked for avoidable
preference/workbench resolution churn.

Telemetry also runs during startup and shutdown:

- Sends version data
- Sends system info
- Sends addon info
- Sends preferences info
- Sends shutdown info

This is worth keeping in mind for latency, especially if network activity is
synchronous or partially blocking.

### 2. Image Decode, Scaling, and Bitmap Conversion Are Repeated Hot Paths

Normal startup profile, approximately 2,545 samples:

- `qt_qimageScaleAARGBA_down_xy_sse4`: about `9.8%`
- `qt_memfillXX_avx2`: about `7.1%`
- `inflate_fast`, `inflate`, `adler32_z`: significant `libz` cost
- `png_read_filter_row_*`, `png_do_*`: significant `libpng` cost

Interactive profile, approximately 5,174 samples:

- `qt_qimageScaleAARGBA_down_xy_sse4`: about `4.1%`
- `qt_memfillXX_avx2`: about `3.6%`
- PNG/zlib decode remains visible
- `Gui::BitmapFactoryInst::convert(QImage const&, SoSFImage&)`: about `0.82%`

Interpretation:

This does not prove exact call counts, but the same image path appears strongly
in both startup and interaction. It is likely repeated asset/icon/image work,
not a single isolated event. Investigate caching and duplicate scaling/decoding,
especially around toolbar/workbench icons, bitmap conversion, and any generated
icons.

Follow-up tracing with temporary `FREECAD_IMAGE_TRACE=1` instrumentation added
timing around `BitmapFactory`, `ViewProviderImagePlane`, and
`BitmapFactoryInst::convert(QImage, SoSFImage)`.

Normal startup-only trace:

- `loadPixmap.hit`: `237`
- `loadPixmap.miss`: `443`
- `pixmapFromSvg.bytes`: `240`
- Largest individual icon/SVG loads were small, generally `1-6ms`.
- The largest startup entry was `freecadsplash3.png`: `~6.1ms`.

Interpretation: startup icon loading is broad and noisy but not currently a
single dominant FreeCAD-owned hotspot in direct timing. The `loadPixmap.miss`
count is high because first-time icon lookup probes multiple possible paths and
formats before finding an icon. Most repeated named icon requests then hit
`BitmapFactory`'s pixmap cache. A direct SVG render cache by name/size/color/DPR
may still be useful for callers of `pixmapFromSvg()`, but this is not the first
optimization target from the trace.

Normal Gridfinity document-open trace:

- `imagePlane.loadRaster`, `270mm slider cabinet side1.png`:
  `692.752ms`, `5712x4284`, `~97.9MB` decoded `QImage`
- `imagePlane.convertToSFImage`, same image:
  `964.213ms`
- `imagePlane.loadImage`, same image total:
  `1662.185ms`
- `imagePlane.loadRaster`, `270mm slider drawer side1.png`:
  `677.225ms`, `5712x4284`, `~97.9MB` decoded `QImage`
- `imagePlane.convertToSFImage`, same image:
  `854.966ms`
- `imagePlane.loadImage`, same image total:
  `1537.531ms`
- `imagePlane.loadRaster`, `IMG_2599D-processed.png`:
  `161.491ms`, `1785x1959`, `~14.0MB` decoded `QImage`
- `imagePlane.convertToSFImage`, same image:
  `261.029ms`
- `imagePlane.loadImage`, same image total:
  `422.688ms`

The `BitmapFactory` aggregate confirmed the conversion cost:

- `convert.QImageToSoSFImage.5712x4284`: `2` calls, `1729.081ms` total
- `convert.QImageToSoSFImage.1785x1959`: `1` call, `245.917ms` total
- Tiny `SoSFImage` conversions were frequent but much cheaper:
  - `21x34`: `936` calls, `28.400ms` total
  - `20x34`: `468` calls, `13.568ms` total
  - `18x28`: `234` calls, `4.982ms` total

Interpretation:

- The main document-open image cost is full-resolution image-plane decode plus
  full-resolution conversion into Coin `SoSFImage` texture data.
- `BitmapFactoryInst::convert(QImage, SoSFImage)` walks every pixel using
  `QImage::pixelColor(x, y)`. That is especially expensive for the two
  `5712x4284` images because each image has about `24.5 million` pixels.
- The tiny repeated conversions are frequent and likely related to sketcher
  constraints, datum labels, or annotation images. They are worth tracking for
  interactive sketch lag, but they are not the primary document-open image cost
  in this trace.
- `ViewProviderImagePlane::loadImage()` made quick failed attempts to load the
  included image files before the embedded files existed in the transient
  directory, then loaded them successfully after restore. The failed attempts
  were cheap, but the ordering suggests image-plane view providers can be asked
  to load before `RestoreDocFile()` has completed.

Strong image optimization candidates:

- Optimize `BitmapFactoryInst::convert(QImage, SoSFImage)` for common 32-bit
  image formats using direct scanline access instead of `pixelColor()`.
- Consider deferring image-plane texture upload/conversion until the image plane
  is visible or the 3D view first needs the texture.
- Consider using a display-resolution texture for image planes initially, with
  full-resolution data kept as the source file rather than always converting the
  full image during document open.
- Avoid or defer the pre-`RestoreDocFile()` failed image loads if the image file
  is known to be an included document payload still being restored.

Additional trace artifacts:

- `/tmp/freecad-image-trace-startup.stderr`
- `/tmp/freecad-image-trace-startup.log`
- `/tmp/freecad-image-trace-gridfinity.stderr`
- `/tmp/freecad-image-trace-gridfinity.log`

### 3. OpenGL Framebuffer / Driver Work Is The Strongest Interactive Lead

The interactive profile showed the largest bucket in unresolved kernel/driver
time. The visible user-space stack routes through `ioctl` into Qt OpenGL
framebuffer setup:

- `QOpenGLFramebufferObjectPrivate::initColorBuffer`
- `QOpenGLFramebufferObjectPrivate::initDepthStencilAttachments`

Interpretation:

This points toward repeated framebuffer or render-target allocation, resize, or
attachment setup during normal interaction. It may be viewport repainting,
overlay rendering, render-target lifecycle churn, or driver synchronization.

This area is a strong candidate for targeted instrumentation because sampled
profiling cannot tell whether these framebuffer calls are frequent, individually
expensive, or both.

Follow-up tracing with a temporary `LD_PRELOAD` GL interposer narrowed this
down further. The interposer counted framebuffer and renderbuffer calls by
hooking `glXGetProcAddress*` and the relevant GL entry points.

Normal startup-only trace, 20 seconds:

- `glGenFramebuffers`: `1`
- `glDeleteFramebuffers`: `0`
- `glBindFramebuffer`: `28`
- `glGenRenderbuffers`: `1`
- `glRenderbufferStorage`: `1`
- `glRenderbufferStorageMultisample`: `0`

Interpretation: startup alone does not show repeated FBO allocation.

Interactive/document trace, 90 seconds:

- `glGenFramebuffers`: `754`
- `glDeleteFramebuffers`: `754`
- `glBindFramebuffer`: `3629`
- `glGenRenderbuffers`: `754`
- `glDeleteRenderbuffers`: `754`
- `glRenderbufferStorageMultisample`: `752`
- `glFramebufferRenderbuffer`: `1132`
- `glFramebufferTexture2D`: `378`

Renderbuffer storage sizes in that trace:

- `1720x986`, samples `8`: `242`
- `1664x986`, samples `8`: `242`
- `1720x1275`, samples `8`: `132`
- `1664x1275`, samples `8`: `134`
- `400x300`, samples `8`: `2`
- `160x160`, samples `0`: `1`
- `100x30`, samples `0`: `1`

The `160x160` allocation is attributable to NaviCube picking:
`NaviCubeImplementation::ensureFramebufferValid()`.

The repeated full-viewport multisample allocations are attributable to Qt's
`QOpenGLWidget` backing framebuffer path, not FreeCAD's explicit
`View3DInventorViewer::imageFromFramebuffer()` screenshot path. Backtraces show:

- `QOpenGLFramebufferObject::QOpenGLFramebufferObject(...)`
- `QOpenGLWidget::event(...)`
- `CustomGLWidget::event(...)`
- `QuarterWidget::viewportEvent(...)`
- Qt widget geometry/layout/change-event paths

One representative stack includes `QPropertyAnimation::updateCurrentValue`,
`QWidget::setGeometry`, `QAbstractScrollArea::viewportEvent`, and then
`QuarterWidget::viewportEvent`. This suggests repeated viewport geometry
changes or animated MDI/layout transitions are causing Qt to recreate
multisampled QOpenGLWidget backing buffers.

Current narrowed hypothesis:

- The expensive path is not general render-to-image code.
- NaviCube picking is not the source of the repeated full-size allocations.
- Repeated Qt viewport geometry/show/layout events are causing full-size
  multisampled QOpenGLWidget FBO recreation.
- The recurring dimensions differ by small amounts, for example `1720x986` vs
  `1664x986`, which points toward layout or dock/MDI/overlay geometry churn.

Temporary instrumentation was added behind `FREECAD_QUARTER_GL_TRACE=1` in
`src/Gui/Quarter/QuarterWidget.cpp` to count `CustomGLWidget::event`,
`QuarterWidget::resizeEvent`, `QuarterWidget::paintEvent`, and
`QuarterWidget::viewportEvent` activity. A 45-second normal-mode interactive
trace produced:

- `QuarterWidget.uniqueWidgets`: `1`
- `CustomGLWidget.uniqueWidgets`: `1`
- `QuarterWidget.resizeEvent`: `4032`
- `QuarterWidget.viewport.event.Resize`: `4032`
- `QuarterWidget.viewport.event.Move`: `4031`
- `QuarterWidget.paintEvent`: `1164`
- `QuarterWidget.viewport.events`: `10035`
- `QuarterWidget.viewport.geometryEvents`: `8068`

Observed viewport sizes in that trace:

- `1664x986`, DPR `1.00`: `4033`
- `1720x986`, DPR `1.00`: `4030`
- `400x300`, DPR `1.00`: `5`

Interpretation: the repeated geometry events are real for a single viewport,
not an artifact from multiple views alternating in the counter. The viewport is
oscillating between two widths, `1664` and `1720`, at the same height and DPR.
That matches the full-size multisample renderbuffer dimensions seen by the GL
interposer and is now the highest-confidence explanation for the repeated Qt
backing FBO recreation.

The next source-level target is the geometry/layout driver that alternates the
Quarter viewport width by 56 pixels. The earlier backtrace implicates Qt
geometry paths involving `QPropertyAnimation::updateCurrentValue`,
`QWidget::setGeometry`, and `QAbstractScrollArea::viewportEvent`. Investigate
dock/overlay/MDI layout updates and any animation or scrollbar visibility state
that can repeatedly change the viewport width during document interaction.

Additional compact-layout tracing identified a concrete source for the 56-pixel
width bounce. `FREECAD_COMPACT_LAYOUT_TRACE=1` was added to
`CompactMainWindowChrome` and labels calls from `MainWindow::event`.

Trace result:

- `layoutChrome`: `32`
- `layoutChrome.MainWindow.LayoutRequest`: `27`
- `layoutChrome.changed.MainWindow.LayoutRequest`: `27`
- `central.1720x1016.to.1664x1016.deltaW56.source.MainWindow.LayoutRequest`: `27`

Interpretation: `QMainWindow::event()` handles a `LayoutRequest`, Qt's normal
main-window layout expands the central widget to the full width, then
`MainWindow::event()` calls `CompactMainWindowChrome::layoutChrome()`, whose
`layoutWorkArea()` manually calls `central->setGeometry(...)` to shrink the
central widget by the compact panel strip width on both sides.

Relevant source:

- `MainWindow::event()` calls compact chrome layout after `QMainWindow::event()`
  for `Resize`, `Show`, `LayoutRequest`, and `WindowStateChange`.
- `CompactMainWindowChrome::layoutWorkArea()` sets the central widget left edge
  to `compactPanelStripWidth()` and right edge to
  `mainWindow->width() - compactPanelStripWidth() - 1`.
- `compactPanelStripWidth()` is `button width + 2 * margin + clearance`, which
  was `28` in this run. Applying it to both sides gives the observed `56` pixel
  delta.

Likely root cause: compact chrome is directly overriding the geometry of
`QMainWindow`'s central widget. Since `QMainWindow` owns and lays out its central
widget, any subsequent layout request can restore the central widget to the
normal full-width work area. Compact chrome then shrinks it again, producing the
`1720 -> 1664 -> 1720` oscillation that propagates to the Quarter viewport and
forces Qt's `QOpenGLWidget` backing FBO recreation.

Additional trace artifacts:

- `/tmp/freecad-fbo-interactive.stderr`
- `/tmp/freecad-fbo-interactive.log`
- `/tmp/freecad-fbo-bt.stderr`
- `/tmp/freecad-fbo-bt.log`
- `/tmp/freecad-quarter-combined2.stderr`
- `/tmp/freecad-quarter-combined2.log`
- `/tmp/freecad-layout-source.stderr`
- `/tmp/freecad-compact-source2.stderr`

### 4. Tree / Document UI Churn Is Frequent And Visible

The interactive log gives concrete counts:

- `TreeView Create item`: `161`
- `TreeView Delete item`: `161`
- `TreeView delete object`: `160`
- `MainWindow.cpp(2659): update actions`: `10`

The captured workflow included opening and closing documents:

- `Vesa_RAM_Ball_Mounting_Bracket_ restore time: 1.985478`
- `Vesa_RAM_Ball_Mounting_Bracket_ postprocess time: 0.060179`
- `Gridfinity_Drawer_v1_3 restore time: 10.488732`
- `Gridfinity_Drawer_v1_3 postprocess time: 0.313630`

After `Gridfinity_Drawer_v1_3` restore, many tree items were created in a tight
burst. During close/shutdown, many tree and document objects were deleted in a
similar burst.

Interpretation:

This is a good candidate for batching or signal-suppression investigation. The
question is whether tree nodes, document notifications, view-provider updates,
and main-window action updates are emitted one-by-one when they could be
coalesced during bulk document load/unload.

Follow-up tracing with temporary `FREECAD_TREE_TRACE=1` instrumentation narrowed
the cost and call pattern for a normal open/close of `Gridfinity_Drawer_v1_3`.
In that run:

- `Gridfinity_Drawer_v1_3 restore time`: `9.357467`
- `Gridfinity_Drawer_v1_3 postprocess time`: `0.336788`
- `Application::openDocuments` total: `9.722926`
- `TreeView Create item`: `128`
- `TreeView Delete item`: `128`
- `TreeView delete object`: `127`
- `MainWindow update actions`: `8`

Tree trace summary:

- `DocumentItem.slotNewObject.calls`: `127`
- `DocumentItem.createNewItem.created`: `128`
- `DocumentItem.createNewItem.us`: `69764`
- `DocumentItem.populateItem.us`: `34553`
- `DocumentObjectItem.generateIcon.us`: `17781`
- `DocumentObjectItem.getVisibilityIcon.us`: `5010`
- `DocumentObjectItem.testStatus.us`: `26441`
- `TreeWidget.slotDeleteDocument.us`: `11077`
- `TreeWidget._slotDeleteObject.calls`: `127`
- `TreeWidget._slotDeleteObject.us`: `3188`
- `TreeWidget.statusTimer.start`: `178`
- `TreeWidget.onUpdateStatus.calls`: `28`
- `TreeWidget.onUpdateStatus.deferred.dragOrRestore`: `27`
- `TreeWidget.onUpdateStatus.newObjectIds`: `127`

Interpretation:

The tree path is frequent but probably not the primary source of the multi-second
document-open lag in this capture. Tree item creation, population, status, and
icon work together were on the order of tens of milliseconds, while document
restore itself was about `9.36s`.

The strongest tree-specific findings are:

- Object creation is one signal per object (`Document::slotNewObject` ->
  `DocumentItem::slotNewObject`), but tree item creation is mostly coalesced
  into one real status pass after restore.
- During restore, `onUpdateStatus()` was called repeatedly and deferred `27`
  times because `App::GetApplication().isRestoring()` was true. This caused
  many timer restarts (`178`) but little direct work until restore finished.
- Deletion is per-object: `slotDeleteDocument()` loops through the document
  object map and calls `_slotDeleteObject()` for each object. In this document it
  was still cheap (`~11ms` total), but the shape is less batched than creation
  and may matter for much larger documents or more expensive tree hierarchies.
- Icon/status generation is a measurable fraction of tree cost. `generateIcon`,
  visibility icon construction, and status testing together accounted for about
  `49ms` across 128 items in this debug build.

Next tree/document leads:

- Avoid repeated status timer restarts while the application is restoring. A
  single "status update needed after restore" flag would be cleaner than
  repeatedly rearming the timer.
- Consider a bulk-delete path for `slotDeleteDocument()` that removes the
  document item and clears tree maps without entering `_slotDeleteObject()` once
  per object when the whole document is being deleted.
- Check whether icon/status generation can cache more aggressively by object
  status, mode, visibility-icon setting, and decoration size.
- Keep this lower priority than the compact-layout/OpenGL churn unless a larger
  document shows tree costs scaling badly.

Additional trace artifacts:

- `/tmp/freecad-tree-churn.stderr`
- `/tmp/freecad-tree-churn.log`

### 5. Document Restore Is Dominated By Embedded File Payloads

Temporary `FREECAD_RESTORE_TRACE=1` instrumentation was added around
`App::Document::restore()`, `Document::Restore()`, `Document::readObjects()`,
`Document::afterRestore()`, and `Base::XMLReader::readFiles()`.

The first normal-mode `Gridfinity_Drawer_v1_3` restore trace showed:

- `Document.restore.total`: `11893.794ms`
- `Document.restore.DocumentXML`: `678.995ms`
- `Document.readObjects`: `676.365ms`
- `Document.readObjects.addObject`: `535.804ms` across `127` objects
- `Document.readObjects.ObjectRestore`: `134.259ms` across `127` objects
- `Document.restore.readFiles`: `11211.866ms`
- `Document.restore.afterRestore`: `~362ms`
- `Document.afterRestore.onDocumentRestored`: `299.857ms` across `127` objects

Interpretation: the multi-second restore is not primarily tree creation,
object XML restore, or postprocess hooks. The dominant block is embedded file
payload restore through `Base::XMLReader::readFiles()`.

A second trace added per-file `RestoreDocFile()` timings. That trace writes one
line per embedded file and therefore inflated total runtime (`readFiles` rose to
`~32.9s`), so use the per-file ordering as a lead rather than as an exact
production timing. The main buckets were:

- Included PNG files via `App::PropertyFileIncluded`: `~5.03s` combined in the
  traced run.
  - `270mm slider drawer side1.png`: `2768.377ms`, `25.8MB`
  - `270mm slider cabinet side1.png`: `1942.280ms`, `26.0MB`
  - `IMG_2599D-processed.png`: `318.211ms`, `5.5MB`
- Part element-map payloads via `Part::TopoShape`: `~5.41s` combined.
  - `Binder.Shape.Map.txt`: `777.620ms`
  - `Chamfer002.Shape.Map.txt`: `731.512ms`
  - `Body.Shape.Map.txt`: `598.428ms`
  - `Pocket001.Shape.Map.txt`: `403.472ms`
  - `Pad004.Shape.Map.txt`: `381.164ms`
- BREP shape payloads via `Part::PropertyPartShape`: `~3.32s` combined.
  - `Binder.Shape.brp`: `469.298ms`
  - `Body.Shape.brp`: `443.997ms`
  - `AdditivePipe.Shape.brp`: `321.028ms`
  - `Chamfer002.Shape.brp`: `269.615ms`
  - `Pocket.Shape.brp`: `224.588ms`
- GUI document restore also appeared as a notable payload:
  - `GuiDocument.xml`: `1180.480ms`

Source-level observations:

- `PropertyFileIncluded::RestoreDocFile()` copies included files one byte at a
  time from `Base::Reader` to `Base::ofstream`. The two `~26MB` PNGs make this
  an obvious optimization candidate, independent of PNG decode/display cost.
- Shape data restore is split between `.Shape.brp` payloads and `.Shape.Map.txt`
  element-map payloads. The map payloads are often slower than the BREP payloads
  despite being much smaller, which points toward parsing/element-map restore
  work rather than raw I/O alone.
- `GuiDocument.xml` restore is large enough to deserve a narrower GUI restore
  trace if document-open UI latency remains a focus.

Next document-restore leads:

- Replace byte-at-a-time copy in `PropertyFileIncluded::RestoreDocFile()` with a
  buffered stream copy and remeasure the large PNG payloads.
- Add narrower timing inside `ComplexGeoData::RestoreDocFile()` /
  `ElementMap::restore()` to separate stream read, parsing, hashing/string
  lookup, and map construction.
- Add GUI restore tracing around `Gui::Document::RestoreDocFile()` and
  view-provider restore if `GuiDocument.xml` remains near one second after file
  copy overhead is reduced.
- Compare this in a RelWithDebInfo build; the absolute parse/copy costs in the
  debug build may be exaggerated, but the bucket ordering is still useful.

Additional trace artifacts:

- `/tmp/freecad-restore-gridfinity.stderr`
- `/tmp/freecad-restore-gridfinity.log`
- `/tmp/freecad-restore-files-gridfinity.stderr`
- `/tmp/freecad-restore-files-gridfinity.log`

#### Restore Follow-Up: Shape-Map Cost

After the buffered included-file restore patch, temporary
`FC_RESTORE_FOLLOWUP_TRACE=1` instrumentation was added locally around
`ComplexGeoData::RestoreDocFile()` and `ElementMap::restore()`. The temporary
code was removed before committing; only the measurements below were kept.

The measurement set intentionally used more than the Gridfinity drawer model:

- `/home/bcherrington/Projects/FreeCAD/Drawer/Gridfinity Drawer v1.3.FCStd`
- `data/examples/BIMExample.FCStd`
- `data/examples/ArchDetail.FCStd`
- `data/examples/EngineBlock.FCStd`
- `data/examples/PartDesignExample.FCStd`
- `data/examples/draft_test_objects.FCStd`

Results from the debug build:

The temporary instrumentation measured tight-loop sub-steps and therefore
adds overhead of its own. Treat the element-map and sub-step values as a ranking
signal, not as exact wall-clock accounting.

| Model | Objects | Shape maps | Shape-map bytes | Open total | Element-map restore | Notable sub-costs |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| `Gridfinity Drawer v1.3.FCStd` | 127 | 50 | 3.7MB | 3.221s | 3.415s | split 1.305s, string-id lookup 0.662s, insertion 0.514s |
| `BIMExample.FCStd` | 361 | 177 | 1.5MB | 4.659s | 1.237s | insertion 0.633s, split 0.378s |
| `ArchDetail.FCStd` | 435 | 173 | 0.5MB | 4.993s | 0.495s | insertion 0.174s, split 0.169s |
| `EngineBlock.FCStd` | 36 | 19 | 14KB | 0.655s | 0.011s | small enough to ignore for now |
| `PartDesignExample.FCStd` | 17 | 9 | 14KB | 0.499s | 0.015s | small enough to ignore for now |
| `draft_test_objects.FCStd` | 113 | 12 | 7KB | 0.711s | 0.003s | small enough to ignore for now |

Interpretation:

- Shape-map restore remains a real cost for large Part/BIM-style documents.
- The hot region is `ElementMap::restore()` name restoration. Allocation time
  was small in the trace; repeated dot-token splitting/parsing,
  `mappedNames.emplace()`, and `StringHasher::getID()` dominate the measured
  sub-steps.
- The safest next production optimization is a parser-level patch in
  `ElementMap::restore()` that avoids repeated `boost::split()` and temporary
  token-vector construction for dot-separated fields. Keep this separate from
  restore-copy and image-conversion changes so it can be reviewed and
  cherry-picked independently.
- `GuiDocument.xml` remains worth measuring separately in a real GUI open
  harness, especially for `BIMExample.FCStd` and `ArchDetail.FCStd`, where the
  archived GUI document is about 2.3-2.6MB. Command-mode opens are not enough
  evidence for a GUI restore change.

#### Future Optimization: Buffered Included-File Restore

`PropertyFileIncluded::RestoreDocFile()` currently restores included files with
a byte-at-a-time loop:

```cpp
unsigned char c;
while (reader.get((char&)c)) {
    to.put((char)c);
}
```

For large included PNGs, this creates one stream read and one stream write call
per byte. In the traced `Gridfinity_Drawer_v1_3` document, two included PNGs
were about `26MB` each, making this an obvious low-risk optimization candidate.

Proposed investigation:

- Replace the byte loop with a fixed-size buffer, for example `64KiB`, using
  `reader.read(buffer.data(), buffer.size())`, `reader.gcount()`, and
  `to.write(buffer.data(), count)`.
- Preserve the existing property notification and permission behavior:
  `aboutToSetValue()`, `to.close()`, `fi.setPermissions(ReadOnly)`, and
  `hasSetValue()`.
- Add or preserve error reporting for destination file creation and consider
  explicit write/close failure checks.
- Remeasure with `FREECAD_RESTORE_TRACE=1` and compare the specific entries:
  `270mm slider drawer side1.png`, `270mm slider cabinet side1.png`, and
  `IMG_2599D-processed.png`.

Expected effect:

- A `26MB` file would drop from roughly `26 million` per-byte read/write
  operations to roughly `400` chunked operations with a `64KiB` buffer.
- This should reduce included-file restore overhead without changing archive
  format or document semantics.

Alternatives to consider later:

- Test `to << reader.rdbuf()` if `Base::Reader` and `zipios::ZipInputStream`
  reliably stop at embedded-file boundaries in this path. The explicit buffer
  is easier to reason about for the first patch.
- Lazy-restore large included image files only when the owning object actually
  needs them.
- Store or derive smaller display representations for image-plane workflows
  when the full-resolution source image is not needed during initial document
  open.

### 6. Geometry Meshing Appears During Interaction

The interactive profile also included geometry-related samples, including:

- `BRepMesh_CircleTool::Select(gp_XY const&)`
- `TopLoc_SListOfItemLocation::Assign(...)`
- `TopLoc_ItemLocation::TopLoc_ItemLocation(...)`

These were not the largest areas in this capture, but they are real document
operation costs and may become more important in workflows dominated by model
recompute, tessellation, or document restore.

## Ranked Investigation Leads

1. OpenGL framebuffer/render-target churn during interaction.
2. Document restore embedded-file payloads, especially included-file copy and
   Part shape/element-map restore.
3. Image/icon/bitmap decode, scaling, and conversion caching.
4. Tree/document UI update batching during document open/close.
5. Addon/workbench startup and stale preference resolution in normal mode.
6. Geometry restore/meshing costs for specific heavy documents.

## Recommendation

Use [spec.md](spec.md) and [plan.md](plan.md) to turn the top findings into
separately reviewable implementation patches. Start with compact layout /
OpenGL framebuffer churn, then document restore embedded-file payloads, then
image conversion and loading work.

## Next Steps

### A. Count OpenGL Framebuffer Creation

Add temporary instrumentation around Qt/OpenGL integration points in FreeCAD's
viewport path and log:

- FBO creation count
- FBO size
- attachment type
- triggering event, if available
- current document/view name
- elapsed time for allocation/setup

Useful questions:

- Are FBOs recreated during every pan/zoom/rotate?
- Are they recreated on overlay changes or tree/action updates?
- Are they recreated because the viewport size or device-pixel-ratio changes?
- Are depth/stencil attachments created repeatedly with the same dimensions?

Follow-up: keep the temporary `FREECAD_QUARTER_GL_TRACE=1` instrumentation while
tracing the caller of the `1664x986`/`1720x986` oscillation. The GL interposer
and Quarter event counters now agree that repeated resize/move events drive the
full-size multisample FBO churn.

The next implementation experiment should remove the central-widget geometry
fight rather than suppressing the resize events after the fact. Candidate
directions:

- Stop calling `central->setGeometry()` from compact chrome and reserve side
  rail space using a `QMainWindow`-owned layout mechanism instead, such as
  contents margins or a wrapper/container layout.
- If the side rails are intended to overlay the work area, do not shrink the
  central widget at all; leave the central widget under the rails and keep the
  rails raised.
- Avoid calling `layoutChrome()` from every `MainWindow.LayoutRequest` unless
  compact chrome owns a real size input that changed.

### B. Count Image Decode, Scale, And Bitmap Conversion

Instrument or trace:

- `Gui::BitmapFactoryInst::convert(QImage const&, SoSFImage&)`
- icon load/lookup paths
- image scaling paths reachable from FreeCAD code
- generated workbench/addon icon creation

Log:

- source icon/image identity
- requested size
- source size
- output size
- cache hit/miss
- elapsed time

Useful questions:

- Are the same PNGs decoded repeatedly?
- Are icons scaled repeatedly to the same sizes?
- Does addon startup generate icons repeatedly?
- Can scaled images or `SoSFImage` conversions be cached safely?

### C. Batch Tree And Document UI Updates

Instrument tree creation/deletion and document/view-provider notification paths:

- count object-added/object-removed notifications
- count tree item creates/deletes
- count action updates
- measure elapsed time inside bulk document restore/unload

Useful questions:

- Are tree updates disabled during document restore?
- Are action updates triggered for every object instead of once per batch?
- Are view-provider updates emitted before the document is fully restored?
- Can tree sorting/layout/repaint be suspended during bulk mutation?

### D. Profile Narrow Normal-Mode Workflows

Use separate captures for each workflow instead of one broad session:

```sh
perf record -F 99 -g --call-graph fp -B \
  -o /tmp/freecad-open-gridfinity.perf.data -- \
  build/debug/bin/FreeCAD --log-file /tmp/freecad-open-gridfinity.log
```

Recommended captures:

- startup only
- open `Gridfinity_Drawer_v1_3`
- rotate/pan/zoom the viewport for 30 seconds
- close a large document
- switch workbenches
- open a document with many icons/images

The goal is to make each profile correspond to one user-visible lag source.

### E. Consider A Release-With-Debug-Info Build

The current debug build is useful for symbols, but debug builds can exaggerate
costs and change behavior. For performance work, compare with a release or
RelWithDebInfo build where possible.

Suggested target:

- optimized build
- debug symbols enabled
- frame pointers preserved if feasible

This would make `perf` results more representative while still allowing useful
symbolization.

## Commands Used

Enable perf temporarily:

```sh
pkexec /usr/sbin/sysctl -w kernel.perf_event_paranoid=0
```

Normal startup profile:

```sh
perf record -F 99 -g --call-graph fp -B \
  -o /tmp/freecad-startup-normal-fp.perf.data -- \
  timeout 20s build/debug/bin/FreeCAD \
  --log-file /tmp/freecad-startup-normal.log
```

Interactive profile:

```sh
perf record -F 99 -g --call-graph fp -B \
  -o /tmp/freecad-interactive-fp.perf.data -- \
  timeout 90s build/debug/bin/FreeCAD \
  --log-file /tmp/freecad-interactive.log
```

Report:

```sh
perf report -i /tmp/freecad-interactive-fp.perf.data \
  --stdio --no-children --sort comm,dso,symbol --percent-limit 0.7
```

Restore perf restriction:

```sh
pkexec /usr/sbin/sysctl -w kernel.perf_event_paranoid=4
```
