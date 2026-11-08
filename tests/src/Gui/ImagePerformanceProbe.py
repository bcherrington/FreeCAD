# SPDX-License-Identifier: LGPL-2.1-or-later
"""Native FreeCAD macro probe. Run through the isolated launcher, never import in a live session.

This probe is not acceptance evidence until its native frame, logical storage,
model invariants and exact arm/build identities have qualification receipts.
"""

import contextlib
import ctypes
import hashlib
import json
import math
import os
from pathlib import Path
import resource
import subprocess
import time
import zipfile
import xml.etree.ElementTree as ET


def digest(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def runtime_receipt(context, qualification):
    """Bind observed application libraries and namespace to the frozen arm."""
    source = Path(os.environ["SPEC003_SOURCE_ROOT"])
    build = Path(os.environ["SPEC003_TRACE_LIBRARY"]).parent.parent
    paths = set()
    for line in Path("/proc/self/maps").read_text().splitlines():
        fields = line.split(maxsplit=5)
        if len(fields) != 6 or not fields[5].startswith("/"):
            continue
        filename = fields[5]
        native_name = Path(filename).name
        owned_native = native_name.startswith(
            ("libFreeCADBase.so", "libFreeCADApp.so", "libFreeCADGui.so", "libCoin.so")
        ) or native_name in {
            "Part.so",
            "PartGui.so",
            "Sketcher.so",
            "SketcherGui.so",
            "PartDesign.so",
            "PartDesignGui.so",
            "Mesh.so",
            "MeshGui.so",
        }
        require(
            not owned_native or filename.startswith(str(build) + "/"),
            "Foreign FreeCAD runtime library loaded",
        )
        if filename.startswith(str(build) + "/"):
            paths.add(filename)
    modules = {}
    for item in sorted(paths):
        path = Path(item)
        require(path.is_file(), "Loaded arm module disappeared")
        modules[path.relative_to(build).as_posix()] = digest(path)
    require(
        {"lib/libFreeCADBase.so", "lib/libFreeCADApp.so"} <= modules.keys(),
        "Arm core libraries not observed",
    )
    import FreeCAD as App

    if App.GuiUp:
        require("lib/libFreeCADGui.so" in modules, "Arm GUI library not observed")
    namespace = os.environ.get("SPEC003_NAMESPACE_RECEIPT")
    require(namespace is not None, "Namespace receipt is required for qualification and collection")
    namespace_sha = None
    if namespace is not None:
        namespace_sha = hashlib.sha256(namespace.encode()).hexdigest()
        require(
            namespace_sha == os.environ["SPEC003_NAMESPACE_SHA256"], "Namespace receipt changed"
        )
        receipt = json.loads(namespace)
        require(
            receipt.get("schema") == "freecad-spec003-namespace/v1",
            "Unknown namespace receipt schema",
        )
        require(
            os.environ.get("MESA_SHADER_CACHE_DIR") == receipt["mesa_shader_cache"]
            and os.environ.get("XDG_CACHE_HOME") == receipt["xdg_cache"],
            "Namespace cache policy changed",
        )
        require(
            receipt["canonical_source"] == str(source) and receipt["build_read_only"],
            "Incorrect runtime source binding",
        )
        cache = Path(build / "CMakeCache.txt").read_text()
        require(
            "CMAKE_HOME_DIRECTORY:INTERNAL=" + str(source) + "\n" in cache,
            "Wrong configured canonical source",
        )
    if not qualification or "namespace_sha256" in context:
        arm = os.environ["SPEC003_ARM"]
        require(
            namespace_sha == context["namespace_sha256"][arm],
            "Namespace differs from qualification",
        )
        require(
            modules == context["runtime_module_sha256"][arm],
            "Loaded arm libraries differ from qualification",
        )
    return {"namespace_sha256": namespace_sha, "runtime_module_sha256": modules}


def model_snapshot(doc):
    """Model/reference identity, shared by command and native GUI probes."""
    import FreeCAD as App

    with zipfile.ZipFile(os.environ["SPEC003_FIXTURE_PATH"]) as archive:
        require(
            archive.getinfo("Document.xml").file_size <= 32 * 1024 * 1024,
            "Oversized document metadata",
        )
        saved = ET.fromstring(archive.read("Document.xml"))
    saved_entries = saved.findall("./ObjectData/Object")
    saved_names = [item.get("name") for item in saved_entries]
    require(
        all(saved_names) and len(saved_names) == len(set(saved_names)),
        "Duplicate/missing saved object identity",
    )
    actual_names = [obj.Name for obj in doc.Objects]
    require(
        len(actual_names) == len(set(actual_names)) and set(actual_names) == set(saved_names),
        "Restored object identity set differs",
    )
    saved_objects = {item.get("name"): item for item in saved_entries}
    state = []
    for obj in sorted(doc.Objects, key=lambda o: o.Name):
        require(obj.isValid(), "Restored object has an error state")
        row = {"name": obj.Name, "type": obj.TypeId}
        require(obj.Name in saved_objects, "Unexpected restored object identity")
        original = saved_objects[obj.Name]
        proxy = original.find("./Properties/Property[@name='Proxy']/Python")
        if proxy is not None and proxy.get("module"):
            require(
                "Proxy" in obj.PropertiesList and obj.Proxy is not None,
                "Required Python proxy missing",
            )
            identity = [type(obj.Proxy).__module__, type(obj.Proxy).__name__]
            require(
                identity == [proxy.get("module"), proxy.get("class")],
                "Restored Python proxy identity differs",
            )
            row["python_proxy"] = identity
        if "Placement" in obj.PropertiesList:
            row["placement"] = list(obj.Placement.toMatrix().A)
        if "Shape" in obj.PropertiesList:
            row["shape_sha256"] = hashlib.sha256(
                obj.Shape.exportBrepToString().encode()
            ).hexdigest()
        links = []
        included = []
        for prop in obj.PropertiesList:
            kind = obj.getTypeIdOfProperty(prop)
            if kind == "App::PropertyFileIncluded":
                filename = getattr(obj, prop)
                included.append([prop, digest(filename) if filename else None])
            if "LinkSub" in kind:
                single = {
                    "App::PropertyLinkSub" + suffix for suffix in ("", "Child", "Global", "Hidden")
                }
                single.update(("App::PropertyXLinkSub", "App::PropertyXLinkSubHidden"))
                multiple = {
                    "App::PropertyLinkSubList" + suffix
                    for suffix in ("", "Child", "Global", "Hidden")
                }
                multiple.add("App::PropertyXLinkSubList")
                require(
                    kind in single | multiple, "Unqualified derived reference property: " + kind
                )
                value = getattr(obj, prop)
                entries = [value] if kind in single else value
                for entry in entries:
                    if not entry or entry[0] is None:
                        continue
                    linked, names = entry
                    require(linked.Document is doc, "Unfrozen external document reference")
                    resolved_references = []
                    for name in names or [""]:
                        resolved = linked.getSubObject(name, 1)
                        require(resolved is not None, "Unresolved document-object reference")
                        if not name:
                            require(resolved is linked, "Object-only reference changed target")
                        reference = [name, resolved.Name, resolved.TypeId]
                        element = linked.resolve(name)[3]
                        if (
                            element in ("H_Axis", "V_Axis", "N_Axis")
                            and resolved.TypeId == "Sketcher::SketchObject"
                        ):
                            axis_id = {"H_Axis": -1, "V_Axis": -2, "N_Axis": -3}[element]
                            axis = resolved.getAxis(axis_id)
                            reference.extend([list(axis.Base), list(axis.Direction)])
                        elif element and "Shape" in resolved.PropertiesList:
                            shape = linked.getSubObject(name)
                            require(shape is not None, "Missing referenced geometry")
                            require(
                                not shape.isNull() and shape.isValid(),
                                "Invalid geometric reference",
                            )
                            reference.append(
                                hashlib.sha256(shape.exportBrepToString().encode()).hexdigest()
                            )
                        resolved_references.append(reference)
                    links.append([prop, linked.Name, resolved_references])
        row["references"] = links
        row["included_payloads"] = included
        if obj.TypeId == "Image::ImagePlane":
            for key in ("XSize", "YSize"):
                stored = original.find("./Properties/Property[@name='" + key + "']/Float")
                require(
                    stored is not None
                    and math.isclose(
                        getattr(obj, key).Value,
                        float(stored.get("value")),
                        rel_tol=1e-10,
                        abs_tol=1e-8,
                    ),
                    "Stored image calibration changed",
                )
            stored = original.find("./Properties/Property[@name='Placement']/PropertyPlacement")
            require(stored is not None, "Stored image placement missing")
            expected = (
                App.Placement(
                    App.Vector(*[float(stored.get(k)) for k in ("Px", "Py", "Pz")]),
                    App.Rotation(*[float(stored.get(k)) for k in ("Q0", "Q1", "Q2", "Q3")]),
                )
                .toMatrix()
                .A
            )
            require(
                all(
                    math.isclose(a, b, rel_tol=1e-10, abs_tol=1e-8)
                    for a, b in zip(row["placement"], expected)
                ),
                "Stored image placement changed",
            )
            row.update(
                image_sha256=digest(obj.ImageFile), x_mm=obj.XSize.Value, y_mm=obj.YSize.Value
            )
        state.append(row)
    context = json.loads(Path(os.environ["SPEC003_CONTEXT_PATH"]).read_text())
    if "expected_image_payload_sha256s" in context:
        require(
            sorted(row["image_sha256"] for row in state if "image_sha256" in row)
            == sorted(context["expected_image_payload_sha256s"]),
            "Restored image payload identities differ",
        )
    return state


def run():
    import FreeCAD as App
    import FreeCADGui as Gui
    from PySide6 import QtCore, QtGui, QtOpenGL, QtOpenGLWidgets, QtWidgets
    from pivy import coin

    context_path = Path(os.environ["SPEC003_CONTEXT_PATH"])
    context = json.loads(context_path.read_text())
    qualification = os.environ.get("SPEC003_QUALIFY") == "1"
    require(
        qualification or "qualification_case" not in context,
        "Qualification test context cannot collect acceptance",
    )
    fixture = Path(os.environ["SPEC003_FIXTURE_PATH"])
    require(digest(fixture) == context["fixture_sha256"], "Fixture identity changed")
    require(QtGui.QGuiApplication.platformName() == "xcb", "Native X11 required")
    require(not App.listDocuments(), "Probe requires an empty fresh process")
    source = Path(os.environ["SPEC003_SOURCE_ROOT"])
    commit = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True
    ).strip()
    if not qualification:
        require(
            not subprocess.check_output(
                ["git", "-C", str(source), "status", "--porcelain", "--untracked-files=no"]
            ),
            "Dirty source arm",
        )
        require(commit == os.environ["SPEC003_COMMIT"], "Wrong arm commit")

    trace = ctypes.CDLL(os.environ["SPEC003_TRACE_LIBRARY"])
    trace.fc_image_trace_begin.argtypes = [ctypes.c_int]
    trace.fc_image_trace_begin.restype = ctypes.c_bool
    trace.fc_image_trace_snapshot.restype = ctypes.c_char_p
    trace.fc_image_trace_flush_tree.restype = ctypes.c_bool
    trace.fc_image_trace_watch_renderer.argtypes = [ctypes.c_void_p]
    trace.fc_image_trace_watch_renderer.restype = ctypes.c_bool
    trace.fc_image_trace_render_count.restype = ctypes.c_ulonglong
    trace.fc_image_trace_field_info.argtypes = [ctypes.c_void_p]
    trace.fc_image_trace_field_info.restype = ctypes.c_char_p
    trace.fc_image_trace_plane_spacing.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    trace.fc_image_trace_plane_spacing.restype = ctypes.c_char_p
    trace.fc_image_trace_replace_field.argtypes = [
        ctypes.c_void_p,
        ctypes.c_int,
        ctypes.c_int,
        ctypes.c_void_p,
        ctypes.c_int64,
    ]
    trace.fc_image_trace_replace_field.restype = ctypes.c_bool
    gl = ctypes.CDLL("libGL.so.1")
    gl.glGetString.restype = ctypes.c_char_p
    gl.glGetIntegerv.argtypes = [ctypes.c_uint, ctypes.POINTER(ctypes.c_int)]
    gl.glBindFramebuffer.argtypes = [ctypes.c_uint, ctypes.c_uint]
    gl.glBlitFramebuffer.argtypes = [ctypes.c_int] * 8 + [ctypes.c_uint, ctypes.c_uint]
    gl.glReadPixels.argtypes = [ctypes.c_int] * 4 + [ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p]
    gl.glGetError.restype = ctypes.c_uint
    for name in ("glGenFramebuffers", "glGenRenderbuffers"):
        getattr(gl, name).argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_uint)]
    gl.glBindRenderbuffer.argtypes = [ctypes.c_uint, ctypes.c_uint]
    gl.glRenderbufferStorage.argtypes = [ctypes.c_uint, ctypes.c_uint, ctypes.c_int, ctypes.c_int]
    gl.glFramebufferRenderbuffer.argtypes = [ctypes.c_uint] * 4
    gl.glCheckFramebufferStatus.argtypes = [ctypes.c_uint]
    gl.glCheckFramebufferStatus.restype = ctypes.c_uint
    gl.glIsEnabled.argtypes = [ctypes.c_uint]
    gl.glIsEnabled.restype = ctypes.c_ubyte
    for name in ("glEnable", "glDisable", "glReadBuffer"):
        getattr(gl, name).argtypes = [ctypes.c_uint]
    gl.glBindBuffer.argtypes = [ctypes.c_uint, ctypes.c_uint]
    gl.glPixelStorei.argtypes = [ctypes.c_uint, ctypes.c_int]

    def snapshot():
        raw = trace.fc_image_trace_snapshot()
        require(raw is not None, "Storage snapshot failed")
        value = json.loads(raw)
        require(value["storage_trace_balanced"], "Storage trace lost ownership identity")
        require(value["live_decoded_bytes"] == 0, "Image decode still live at boundary")
        return value

    # Quarter constructs a QSurfaceFormat rather than using defaultFormat().
    # Set the requested native-widget format before its context is created.
    class NativeFormat(QtCore.QObject):
        def eventFilter(self, obj, event):
            if event.type() in {
                QtCore.QEvent.Type.MouseMove,
                QtCore.QEvent.Type.MouseButtonPress,
                QtCore.QEvent.Type.MouseButtonRelease,
                QtCore.QEvent.Type.MouseButtonDblClick,
                QtCore.QEvent.Type.Wheel,
                QtCore.QEvent.Type.HoverMove,
                QtCore.QEvent.Type.KeyPress,
                QtCore.QEvent.Type.KeyRelease,
            }:
                return True
            if event.type() == QtCore.QEvent.Type.Polish and isinstance(
                obj, QtOpenGLWidgets.QOpenGLWidget
            ):
                if obj.context() is not None:
                    return False
                fmt = obj.format()
                fmt.setSamples(8)
                obj.setFormat(fmt)
            return False

    app = QtWidgets.QApplication.instance()
    format_filter = NativeFormat(app)
    app.installEventFilter(format_filter)
    view_preferences = App.ParamGet("User parameter:BaseApp/Preferences/View")
    view_preferences.SetInt("AntiAliasing", 4)
    for name in (
        "UseNavigationAnimations",
        "UseSpinningAnimations",
        "ShowFPS",
        "DepthAwareContrast",
    ):
        view_preferences.SetBool(name, False)
    window_preferences = App.ParamGet("User parameter:BaseApp/Preferences/MainWindow")
    window_preferences.SetBool("CompactJetBrainsLayout", False)
    window_preferences.SetBool("CompactJetBrainsFramelessWindow", False)
    require(
        not window_preferences.GetBool("CompactJetBrainsLayout", True),
        "Compact layout must be disabled",
    )
    Gui.activateWorkbench("NoneWorkbench")
    main = Gui.getMainWindow()
    main.showNormal()
    main.resize(1600, 1000)
    for dock in main.findChildren(QtWidgets.QDockWidget):
        dock.hide()
    for toolbar in main.findChildren(QtWidgets.QToolBar):
        toolbar.hide()
    app.processEvents()
    visibility = context["initial_image_visibility"]
    require(trace.fc_image_trace_begin(visibility), "Trace initialization failed")
    stages, frames, metrics = {}, {}, {}
    # Allocate and touch the single persistent CPU readback buffer before the
    # measurement window, so earlier stage captures cannot raise later RSS.
    pixels = (ctypes.c_ubyte * (1280 * 720 * 4))()
    ctypes.memset(pixels, 0, ctypes.sizeof(pixels))
    native_profile = {}
    native = None
    view = None
    doc = None
    target = None
    vrml_textures = []
    capture = [0]
    capture_fbo, capture_buffer = ctypes.c_uint(), ctypes.c_uint()

    def textures_ready():
        return all(texture.getImage().hasData() for texture in vrml_textures)

    def integer(parameter):
        value = ctypes.c_int()
        gl.glGetIntegerv(parameter, ctypes.byref(value))
        return value.value

    @contextlib.contextmanager
    def native_context():
        require(QtCore.QThread.currentThread() is app.thread(), "GL capture requires GUI thread")
        previous = QtGui.QOpenGLContext.currentContext()
        surface = previous.surface() if previous is not None else None
        bindings = (integer(0x8CAA), integer(0x8CA6)) if previous is native.context() else None
        try:
            native.makeCurrent()
            require(
                QtGui.QOpenGLContext.currentContext() is native.context(), "Wrong native GL context"
            )
            yield
        finally:
            if previous is None:
                native.doneCurrent()
            else:
                require(previous.makeCurrent(surface), "Previous GL context restore failed")
                if bindings is not None:
                    gl.glBindFramebuffer(0x8CA8, bindings[0])
                    gl.glBindFramebuffer(0x8CA9, bindings[1])

    def copy_frame():
        # Called after the endpoint/resource snapshot. Raw renderbuffer allocation
        # touches no texture or pixel-pack state. Restore every GL state we alter
        # before Qt's synchronous frameSwapped signal resumes.
        read, draw, renderbuffer = integer(0x8CAA), integer(0x8CA6), integer(0x8CA7)
        scissor, srgb = bool(gl.glIsEnabled(0x0C11)), bool(gl.glIsEnabled(0x8DB9))
        native_read_buffer = None
        try:
            if capture[0] == 0:
                gl.glGenFramebuffers(1, ctypes.byref(capture_fbo))
                gl.glGenRenderbuffers(1, ctypes.byref(capture_buffer))
                require(
                    capture_fbo.value != 0 and capture_buffer.value != 0,
                    "Capture allocation failed",
                )
                gl.glBindRenderbuffer(0x8D41, capture_buffer.value)
                gl.glRenderbufferStorage(0x8D41, 0x8058, 1280, 720)
                gl.glBindFramebuffer(0x8CA9, capture_fbo.value)
                gl.glFramebufferRenderbuffer(0x8CA9, 0x8CE0, 0x8D41, capture_buffer.value)
                require(
                    gl.glCheckFramebufferStatus(0x8CA9) == 0x8CD5, "Capture framebuffer incomplete"
                )
                capture[0] = capture_fbo.value
            gl.glDisable(0x0C11)
            gl.glDisable(0x8DB9)
            gl.glBindFramebuffer(0x8CA8, native.defaultFramebufferObject())
            native_read_buffer = integer(0x0C02)
            gl.glReadBuffer(0x8CE0)
            gl.glBindFramebuffer(0x8CA9, capture[0])
            gl.glBlitFramebuffer(0, 0, 1280, 720, 0, 0, 1280, 720, 0x4000, 0x2600)
            require(gl.glGetError() == 0, "Accepted frame copy failed")
        finally:
            if native_read_buffer is not None:
                gl.glBindFramebuffer(0x8CA8, native.defaultFramebufferObject())
                gl.glReadBuffer(native_read_buffer)
            gl.glBindFramebuffer(0x8CA8, read)
            gl.glBindFramebuffer(0x8CA9, draw)
            gl.glBindRenderbuffer(0x8D41, renderbuffer)
            (gl.glEnable if scissor else gl.glDisable)(0x0C11)
            (gl.glEnable if srgb else gl.glDisable)(0x8DB9)

    def hash_captured_frame():
        # Read the retained accepted frame outside event dispatch. A bound pixel
        # pack buffer must never reinterpret the preallocated CPU buffer pointer.
        pack = (0x0D00, 0x0D01, 0x0D02, 0x0D03, 0x0D04, 0x0D05)
        with native_context():
            read, read_buffer, buffer = integer(0x8CAA), integer(0x0C02), integer(0x88ED)
            values = [integer(parameter) for parameter in pack]
            try:
                gl.glBindBuffer(0x88EB, 0)
                for parameter in pack:
                    gl.glPixelStorei(parameter, 1 if parameter == 0x0D05 else 0)
                gl.glBindFramebuffer(0x8CA8, capture[0])
                gl.glReadBuffer(0x8CE0)
                gl.glReadPixels(0, 0, 1280, 720, 0x1908, 0x1401, pixels)
                require(gl.glGetError() == 0, "Captured framebuffer read failed")
            finally:
                gl.glBindFramebuffer(0x8CA8, read)
                gl.glReadBuffer(read_buffer)
                for parameter, value in zip(pack, values):
                    gl.glPixelStorei(parameter, value)
                gl.glBindBuffer(0x88EB, buffer)
        return hashlib.sha256(pixels).hexdigest()

    def completed_frame(label, started):
        finished, errors, accepting = [], [], [False]
        preceding = trace.fc_image_trace_render_count()
        ready = [bool(texture.getImage().hasData()) for texture in vrml_textures]
        readiness_render = [preceding]
        sensors = []

        def texture_ready(index):
            if not ready[index] and vrml_textures[index].getImage().hasData():
                ready[index] = True
                readiness_render[0] = trace.fc_image_trace_render_count()

        for index, texture in enumerate(vrml_textures):
            if not ready[index]:
                sensor = coin.SoNodeSensor(lambda *_args, index=index: texture_ready(index), None)
                sensor.setPriority(100)
                require(sensor.getPriority() == 100, "Wrong texture sensor ordering")
                sensor.attach(texture)
                sensors.append(sensor)

        def callback():
            if (
                accepting[0]
                or not all(ready)
                or trace.fc_image_trace_render_count() <= max(preceding, readiness_render[0])
            ):
                return
            accepting[0] = True
            try:
                with native_context():
                    require(
                        native.updateBehavior()
                        == QtOpenGLWidgets.QOpenGLWidget.UpdateBehavior.PartialUpdate,
                        "Post-swap capture requires preserved native framebuffer",
                    )
                    gl.glFinish()
                    finished.append(time.perf_counter_ns())
                    require(textures_ready(), "Required texture lost readiness")
                    require(native.isVisible() and main.isVisible(), "Native view is hidden")
                    require(native.devicePixelRatioF() == 1, "DPR must be one")
                    require(
                        (native.width(), native.height()) == (1280, 720),
                        "Wrong viewport dimensions",
                    )
                    require(integer(0x80A9) == 8, "Native framebuffer does not have eight samples")
                    native_profile.update(
                        renderer=gl.glGetString(0x1F01).decode(),
                        vendor=gl.glGetString(0x1F00).decode(),
                        version=gl.glGetString(0x1F02).decode(),
                        samples=8,
                        dpr=1,
                        viewport=[1280, 720],
                        framebuffer_update_behavior="PartialUpdate",
                    )
                    if not qualification or "native_profile" in context:
                        require(
                            native_profile == context["native_profile"],
                            "Native renderer/driver profile changed",
                        )
                    stages[label] = dict(
                        snapshot(),
                        peak_rss_bytes=resource.getrusage(resource.RUSAGE_SELF).ru_maxrss * 1024,
                    )
                    copy_frame()
            except BaseException as error:
                errors.append(error)

        native.frameSwapped.connect(callback, QtCore.Qt.ConnectionType.DirectConnection)
        try:
            view.redraw()
            deadline = time.monotonic() + 10
            while not finished and not errors:
                require(time.monotonic() < deadline, "Correct native frame completion timed out")
                app.processEvents(QtCore.QEventLoop.ProcessEventsFlag.AllEvents, 5)
            if errors:
                raise errors[0]
            frames[label] = hash_captured_frame()
            return (finished[0] - started) / 1e6
        finally:
            native.frameSwapped.disconnect(callback)
            for sensor in sensors:
                sensor.detach()

    started = time.perf_counter_ns()
    doc = App.openDocument(str(fixture))
    require(
        doc is not None and not doc.Restoring and not doc.Recomputing and not doc.mustExecute(),
        "Document restore/recompute did not complete",
    )
    view = Gui.activeDocument().activeView()
    require(view is not None, "No native view")
    mdi = main.findChild(QtWidgets.QMdiArea)
    require(mdi is not None, "No document MDI area")
    if qualification and context.get("qualification_case") == "inactive-mdi":
        mdi.setActiveSubWindow(None)
        require(mdi.activeSubWindow() is None, "Inactive-MDI test did not deactivate focus")
    if qualification and context.get("qualification_case") == "ambiguous-mdi":
        mdi.addSubWindow(QtWidgets.QWidget())
    require(Gui.activeDocument().Document.Name == doc.Name, "Wrong GUI document")
    deadline = time.monotonic() + 10
    while not mdi.subWindowList():
        require(time.monotonic() < deadline, "Document subwindow creation timed out")
        app.processEvents(QtCore.QEventLoop.ProcessEventsFlag.AllEvents, 5)
    windows = mdi.subWindowList()
    require(len(windows) == 1, "Ambiguous owned document subwindow")
    widgets = windows[0].findChildren(QtOpenGLWidgets.QOpenGLWidget)
    require(len(widgets) == 1, "Ambiguous native viewport")
    native = widgets[0]
    view.getViewer().setEnabledNaviCube(False)
    manager = view.getViewer().getSoRenderManager()
    require(
        trace.fc_image_trace_watch_renderer(int(manager.this)),
        "Render observer registration failed",
    )
    native.setFixedSize(1280, 720)
    image_planes = [o for o in doc.Objects if o.TypeId == "Image::ImagePlane"]
    if context.get("first_show_image_sha256"):
        ordered = sorted(image_planes, key=lambda obj: obj.Name)
        target = ordered[context["first_show_image_index"]]
        view.setCameraType("Orthographic")
        view.setCameraOrientation(target.Placement.Rotation.Q)
        camera = view.getCameraNode()
        extent = max(target.XSize.Value, target.YSize.Value)
        position = target.Placement.multVec(App.Vector(0, 0, extent * 2))
        camera.position = tuple(position)
        camera.height = max(target.YSize.Value, target.XSize.Value / (1280 / 720)) * 1.05
        camera.nearDistance = extent * 0.001
        camera.farDistance = extent * 10
    else:
        view.viewAxonometric()
        view.fitAll(1.0)
    # SoPath.getTail() hides children of VRMLShape; use its full path to
    # inspect the actual matching texture. Search only the rendered scene.
    search = coin.SoSearchAction()
    search.setType(coin.SoVRMLImageTexture.getClassTypeId())
    search.setInterest(coin.SoSearchAction.ALL)
    search.apply(view.getSceneGraph())
    paths = search.getPaths()
    vrml_textures = [coin.cast(paths[i], "SoFullPath").getTail() for i in range(paths.getLength())]
    vrml_textures = [
        texture for texture in vrml_textures if texture.url.getNum() and str(texture.url[0])
    ]
    if "required_vrml_textures" in context:
        require(
            len(vrml_textures) == context["required_vrml_textures"], "VRML texture coverage changed"
        )
    require(trace.fc_image_trace_flush_tree(), "Final tree flush failed")
    metrics["gui_open_frame_ms"] = completed_frame("open", started)
    require(stages["open"]["restore_finished_signals"] == 1, "Missing restore completion")
    require(stages["open"]["open_finished_signals"] == 1, "Missing open completion")
    require(stages["open"]["required_tree_flushes"] == 1, "Missing required tree flush")
    if visibility >= 0:
        require(
            all(bool(o.ViewObject.Visibility) == bool(visibility) for o in image_planes),
            "Incorrect initial image visibility",
        )
    if context["scenario"] == "hidden-first-show-repeat":
        require(target is not None and not target.ViewObject.Visibility, "Hidden target required")
        started = time.perf_counter_ns()
        target.ViewObject.Visibility = True
        metrics["first_show_frame_ms"] = completed_frame("first_show", started)
        target.ViewObject.Visibility = False
        completed_frame("repeat_hide", time.perf_counter_ns())
        started = time.perf_counter_ns()
        target.ViewObject.Visibility = True
        metrics["repeat_show_frame_ms"] = completed_frame("repeat_show", started)
        metrics["open_plus_first_show_ms"] = (
            metrics["gui_open_frame_ms"] + metrics["first_show_frame_ms"]
        )
        require(
            frames["first_show"] == frames["repeat_show"], "Repeat frame differs from first show"
        )
    if not qualification or "expected_boundary_counts" in context:
        expected_counts = context["expected_boundary_counts"][os.environ["SPEC003_ARM"]]
        for label, counts in expected_counts.items():
            for key, value in counts.items():
                require(stages[label][key] == value, "Unexpected pending/materialized/load state")
    window = snapshot()
    metrics.update(
        {
            k: window[k]
            for k in ("coin_image_bytes", "decoded_image_bytes", "failed_pre_restore_loads")
        }
    )
    metrics["peak_rss_bytes"] = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss * 1024

    if target is not None:
        require(
            digest(target.ImageFile) == context["first_show_image_sha256"],
            "Frozen first-show locator changed payload",
        )
    # All subsequent correctness work lies outside timing and the saved resource
    # observations. Hidden unselected images are not forcibly materialized.
    state = model_snapshot(doc)
    rows_by_name = {row["name"]: row for row in state}
    replacements = []
    for obj in sorted(doc.Objects, key=lambda o: o.Name):
        row = rows_by_name[obj.Name]
        if obj.TypeId == "Image::ImagePlane":
            row.update(
                image_sha256=digest(obj.ImageFile),
                x_mm=obj.XSize.Value,
                y_mm=obj.YSize.Value,
                final_visibility=bool(obj.ViewObject.Visibility),
            )
            search = coin.SoSearchAction()
            search.setType(coin.SoTexture2.getClassTypeId())
            search.setSearchingAll(True)
            search.apply(obj.ViewObject.RootNode)
            require(search.getPath() is not None, "Image texture missing")
            texture = search.getPath().getTail()
            field = int(texture.image.this)
            raw = trace.fc_image_trace_field_info(field)
            require(raw is not None, "Texture field inspection failed")
            info = json.loads(raw)
            if obj.ViewObject.Visibility:
                image = QtGui.QImage(obj.ImageFile)
                require(not image.isNull(), "Reference image decode failed")
                require(
                    (info["width"], info["height"]) == (image.width(), image.height()),
                    "Texture resolution differs",
                )
                require(info["components"] == 4, "Unexpected texture component count")
                reference = image.convertToFormat(QtGui.QImage.Format.Format_RGBA8888).flipped(
                    QtCore.Qt.Orientation.Vertical
                )
                expected = bytes(reference.constBits())
                require(
                    info["sha256"] == hashlib.sha256(expected).hexdigest(),
                    "Texture pixels/alpha/orientation differ",
                )
                row["source_resolution"] = [image.width(), image.height()]
                raw = trace.fc_image_trace_plane_spacing(doc.Name.encode(), obj.Name.encode())
                require(raw is not None, "Plane spacing inspection failed")
                spacing = json.loads(raw)
                require(
                    spacing["x_pixels_per_meter"] == image.dotsPerMeterX()
                    and spacing["y_pixels_per_meter"] == image.dotsPerMeterY(),
                    "Pixel spacing differs",
                )
                replacements.append((field, image.width(), image.height(), expected))
    normative = hashlib.sha256(
        json.dumps(state, sort_keys=True, separators=(",", ":")).encode()
    ).hexdigest()
    if qualification:
        reference_label = "repeat_show" if "repeat_show" in frames else "open"
        original = frames[reference_label]
        if target is not None:
            search = coin.SoSearchAction()
            search.setType(coin.SoTexture2.getClassTypeId())
            search.setSearchingAll(True)
            search.apply(target.ViewObject.RootNode)
            selected_field = int(search.getPath().getTail().image.this)
            selected = next(item for item in replacements if item[0] == selected_field)
            field, width, height, expected = selected
            changed = b"\xff\x00\xff\xff" * (width * height)
            require(
                trace.fc_image_trace_replace_field(field, width, height, changed, len(changed)),
                "Coverage texture replacement failed",
            )
            completed_frame("coverage_witness", time.perf_counter_ns())
            require(
                frames["coverage_witness"] != original,
                "Selected image does not contribute to native frame",
            )
        for field, width, height, expected in replacements:
            require(
                trace.fc_image_trace_replace_field(field, width, height, expected, len(expected)),
                "Reference texture replacement failed",
            )
        completed_frame("independent_reference", time.perf_counter_ns())
        require(
            frames["independent_reference"] == original,
            "Independent native reference frame differs",
        )
    if not qualification or "normative_state_sha256" in context:
        require(
            normative == context["normative_state_sha256"],
            "Model/reference state differs from qualification",
        )
        for label, expected in context["frame_sha256"].items():
            require(frames.get(label) == expected, "Native frame differs from qualified reference")
    require(trace.fc_image_trace_watch_renderer(None), "Render observer cleanup failed")
    configuration = {
        "workbench": Gui.activeWorkbench().name(),
        "compact_layout": window_preferences.GetBool("CompactJetBrainsLayout", True),
        "frameless_setting": window_preferences.GetBool("CompactJetBrainsFramelessWindow", True),
        "actual_frameless_window": bool(
            main.windowFlags() & QtCore.Qt.WindowType.FramelessWindowHint
        ),
        "anti_aliasing_setting": view_preferences.GetInt("AntiAliasing", -1),
        "navigation_animations": view_preferences.GetBool("UseNavigationAnimations", True),
        "spinning_animations": view_preferences.GetBool("UseSpinningAnimations", True),
        "show_fps": view_preferences.GetBool("ShowFPS", True),
        "depth_aware_contrast": view_preferences.GetBool("DepthAwareContrast", True),
        "navigation_cube": bool(view.getViewer().isEnabledNaviCube()),
        "input_policy": "owned-worker mouse/key input filtered",
        "frame_endpoint": "first-qualifying-native-swap-glfinish-v2",
        "document_window_selection": "single-owned-subwindow-independent-of-keyboard-focus",
    }
    expected_configuration = {
        "workbench": "NoneWorkbench",
        "compact_layout": False,
        "frameless_setting": False,
        "actual_frameless_window": False,
        "anti_aliasing_setting": 4,
        "navigation_animations": False,
        "spinning_animations": False,
        "show_fps": False,
        "depth_aware_contrast": False,
        "navigation_cube": False,
        "input_policy": "owned-worker mouse/key input filtered",
        "frame_endpoint": "first-qualifying-native-swap-glfinish-v2",
        "document_window_selection": "single-owned-subwindow-independent-of-keyboard-focus",
    }
    require(configuration == expected_configuration, "Unexpected benchmark UI configuration")
    configuration_sha = hashlib.sha256(
        json.dumps(configuration, sort_keys=True, separators=(",", ":")).encode()
    ).hexdigest()
    if not qualification or "effective_configuration_sha256" in context:
        require(
            configuration_sha == context["effective_configuration_sha256"],
            "Effective UI configuration changed",
        )
    runtime = runtime_receipt(context, qualification)
    return dict(
        comparison_id="P07",
        fixture_id=context["fixture_id"],
        fixture_sha256=context["fixture_sha256"],
        context_sha256=digest(context_path),
        normative_state_sha256=normative,
        commit=commit,
        correctness_passed=True,
        metrics=metrics,
        native_profile=native_profile,
        required_vrml_textures=len(vrml_textures),
        harness_storage={
            "cpu_readback_bytes": ctypes.sizeof(pixels),
            "rgba8_renderbuffer_logical_bytes": 1280 * 720 * 4,
            "allocation": "after first frame resource snapshot; reused for worker lifetime",
            "scope": "driver allocation effects included in final process RSS; no GPU residency claim",
        },
        boundary_snapshots=stages,
        frame_sha256=frames,
        qualification_only=qualification,
        effective_configuration=configuration,
        effective_configuration_sha256=configuration_sha,
        logical_storage_scope="image-plane decoded buffers and Coin fields; allocator/driver scratch excluded",
        **runtime,
    )


def run_command():
    import FreeCAD as App

    context_path = Path(os.environ["SPEC003_CONTEXT_PATH"])
    context = json.loads(context_path.read_text())
    fixture = Path(os.environ["SPEC003_FIXTURE_PATH"])
    require(not App.GuiUp and not App.listDocuments(), "Fresh command-only process required")
    require(digest(fixture) == context["fixture_sha256"], "Fixture identity changed")
    source = Path(os.environ["SPEC003_SOURCE_ROOT"])
    commit = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True
    ).strip()
    qualification = os.environ.get("SPEC003_QUALIFY") == "1"
    if not qualification:
        require(commit == os.environ["SPEC003_COMMIT"], "Wrong command arm commit")
        require(
            not subprocess.check_output(
                ["git", "-C", str(source), "status", "--porcelain", "--untracked-files=no"]
            ),
            "Dirty command source arm",
        )
    started = time.perf_counter_ns()
    doc = App.openDocument(str(fixture))
    require(doc is not None and not doc.Restoring, "Command restore did not complete")
    require(not doc.Recomputing and not doc.mustExecute(), "Command recompute is pending or active")
    restore_state = {
        "restoring": bool(doc.Restoring),
        "recomputing": bool(doc.Recomputing),
        "must_execute": bool(doc.mustExecute()),
    }
    finished = time.perf_counter_ns()
    peak_rss = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss * 1024
    state = model_snapshot(doc)
    normative = hashlib.sha256(
        json.dumps(state, sort_keys=True, separators=(",", ":")).encode()
    ).hexdigest()
    if not qualification:
        require(
            normative == context["command_normative_state_sha256"],
            "Command model/reference state differs",
        )
    runtime = runtime_receipt(context, qualification)
    App.closeDocument(doc.Name)
    return {
        **runtime,
        "comparison_id": "P07-command-auxiliary",
        "restore_state": restore_state,
        "fixture_id": context["fixture_id"],
        "fixture_sha256": context["fixture_sha256"],
        "context_sha256": digest(context_path),
        "normative_state_sha256": normative,
        "commit": commit,
        "correctness_passed": True,
        "qualification_only": qualification,
        "metrics": {"document_open_ms": (finished - started) / 1e6, "peak_rss_bytes": peak_rss},
        "endpoint": "App.openDocument return with restoring flag clear; model/reference/payload checks outside timer; no GUI initialization",
    }


def main_command():
    import traceback

    try:
        result = run_command()
    except Exception as exc:
        traceback.print_exc()
        result = {
            "correctness_passed": False,
            "exception_type": type(exc).__name__,
            "qualification_only": True,
        }
    Path(os.environ["SPEC003_RESULT_FILE"]).write_text(json.dumps(result, indent=2) + "\n")


def main():
    from PySide6 import QtCore, QtWidgets

    output = Path(os.environ["SPEC003_RESULT_FILE"])
    try:
        result = run()
        code = 0
    except Exception as exc:
        # No model paths, labels, object names or exception messages in records.
        # Full traceback belongs only to the private local diagnostic log.
        import traceback

        traceback.print_exc()
        result = {
            "correctness_passed": False,
            "exception_type": type(exc).__name__,
            "qualification_only": True,
        }
        code = 1
    library = ctypes.CDLL(os.environ["SPEC003_TRACE_LIBRARY"])
    library.fc_image_trace_watch_renderer.argtypes = [ctypes.c_void_p]
    library.fc_image_trace_watch_renderer(None)
    output.write_text(json.dumps(result, indent=2) + "\n")
    QtCore.QTimer.singleShot(0, lambda: QtWidgets.QApplication.instance().exit(code))
