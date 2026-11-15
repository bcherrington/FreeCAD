# SPDX-License-Identifier: LGPL-2.1-or-later
"""Linux GUI save/relocation regression; run with --freecad BUILD/bin/FreeCAD --output NEW_DIR.

Each save mode and relocated reopen runs in a separate private-profile process.
Requires xvfb-run, timeout and the selected build's library environment. Never
rewrites document XML or selects an operator runtime.
"""

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import traceback
import xml.etree.ElementTree as ET
import zipfile


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def references(path):
    with zipfile.ZipFile(path) as archive:
        return [
            x.get("file")
            for x in ET.fromstring(archive.read("Document.xml")).iter("XLink")
            if x.get("file")
        ]


def geometry(obj):
    return {
        "volume": obj.Shape.Volume,
        "center": list(obj.Shape.CenterOfMass),
        "position": list(obj.Placement.Base),
    }


def create(app, gui, project, mode):
    (project / "parts").mkdir(parents=True)
    (project / "assemblies").mkdir()
    (project / "copies").mkdir()
    definitions = app.newDocument("Definitions")
    body = definitions.addObject("PartDesign::Body", "Body")
    box = body.newObject("PartDesign::AdditiveBox", "Box")
    box.Length, box.Width, box.Height = 13, 7, 5
    definitions.recompute()
    definitions.saveAs(str(project / "parts/Definitions.FCStd"))
    sub = app.newDocument("Subassembly")
    sub.saveAs(str(project / "assemblies/Subassembly.FCStd"))
    nested = sub.addObject("App::Link", "NestedOccurrence")
    nested.setLink(body)
    nested.Placement.Base = app.Vector(3, 5, 7)
    sub.recompute()
    sub.save()
    root = app.newDocument("ReferenceRoot")
    root.saveAs(str(project / "ReferenceRoot.FCStd"))
    occurrence = root.addObject("App::Link", "Occurrence")
    occurrence.setLink(body)
    occurrence.Placement.Base = app.Vector(17, 3, 2)
    occurrence.addProperty("App::PropertyString", "SavedMarker")
    occurrence.SavedMarker = "native-save"
    outer = root.addObject("App::Link", "Outer")
    outer.setLink(nested)
    root.recompute()
    shared = app.newDocument("SharedRoot")
    shared.saveAs(str(project / "SharedRoot.FCStd"))
    shared.addObject("App::Link", "Shared").setLink(body)
    shared.recompute()
    # The native commands must actually persist a dirty root.
    root.Label = "Dirty root"
    gui.getDocument(root.Name).Modified = True
    gui.setActiveDocument(root.Name)
    app.setActiveDocument(root.Name)
    if mode == "save":
        from PySide6 import QtCore, QtWidgets

        # Answer only the expected dependency question in this owned process.
        def save_dependencies():
            dialog = QtWidgets.QApplication.activeModalWidget()
            if (
                isinstance(dialog, QtWidgets.QMessageBox)
                and dialog.windowTitle() == "Save dependent files"
            ):
                dialog.done(QtWidgets.QMessageBox.Yes)

        timer = QtCore.QTimer()
        timer.timeout.connect(save_dependencies)
        timer.start(50)
        try:
            gui.runCommand("Std_Save", 0)
        finally:
            timer.stop()
        shared.save()
    elif mode == "save-all":
        gui.runCommand("Std_SaveAll", 0)
    elif mode == "save-as":
        root.saveAs(str(project / "copies/ReferenceRoot.FCStd"))
        shared.save()
    else:
        root.save()
        shared.save()
        root.Label = "Dirty checkpoint"
        gui.getDocument(root.Name).Modified = True
        original_digest = digest(root.FileName)
        before = (root.Name, root.FileName, str(root.Uid), gui.getDocument(root.Name).Modified)
        root.saveCopy(str(project / "copies/Checkpoint.FCStd"))
        assert before == (
            root.Name,
            root.FileName,
            str(root.Uid),
            gui.getDocument(root.Name).Modified,
        )
        copy_refs = references(project / "copies/Checkpoint.FCStd")
        assert digest(root.FileName) == original_digest
        assert copy_refs and all(Path(p).is_absolute() for p in copy_refs), copy_refs
        # A copy's absolute dependencies must work from its different location.
        checkpoint = app.openDocument(str(project / "copies/Checkpoint.FCStd"))
        assert math.isclose(checkpoint.Occurrence.Shape.Volume, 455, rel_tol=1e-10, abs_tol=1e-8)
        app.closeDocument(checkpoint.Name)
    files = [Path(d.FileName) for d in (definitions, sub, root, shared)]
    records = {
        str(p.relative_to(project)): {"sha256": digest(p), "references": references(p)}
        for p in files
    }
    for name, record in records.items():
        assert all(not Path(p).is_absolute() for p in record["references"]), (name, record)
    assert math.isclose(occurrence.Shape.Volume, 455, rel_tol=1e-10, abs_tol=1e-8)
    return {
        "root": str(Path(root.FileName).relative_to(project)),
        "files": records,
        "geometry": {name: geometry(root.getObject(name)) for name in ("Occurrence", "Outer")},
    }


def reopen(app, project, expected):
    root = app.openDocument(str(project / expected["root"]))
    # Native restore intentionally derives the document label from the filename.
    # Use object data to verify that the dirty root was saved.
    assert root.Occurrence.SavedMarker == "native-save"
    measured = {}
    for name, original in expected["geometry"].items():
        obj = root.getObject(name)
        assert obj.LinkedObject is not None, name
        actual = geometry(obj)
        measured[name] = actual
        for field in original:
            left, right = actual[field], original[field]
            if isinstance(left, list):
                assert all(
                    math.isclose(a, b, rel_tol=1e-10, abs_tol=1e-8) for a, b in zip(left, right)
                ), (name, field, left, right)
            else:
                assert math.isclose(left, right, rel_tol=1e-10, abs_tol=1e-8)
    shared = app.openDocument(str(project / "SharedRoot.FCStd"))
    assert shared.Shared.LinkedObject is root.Occurrence.LinkedObject
    assert root.Outer.LinkedObject.LinkedObject is root.Occurrence.LinkedObject
    target = Path(root.Occurrence.LinkedObject.Document.FileName).resolve()
    assert target == (project / "parts/Definitions.FCStd").resolve(), str(target)
    assert all(d.FileName.startswith(str(project)) for d in app.listDocuments().values())
    return {"target": str(target), "geometry": measured, "shared_identity": True}


def cell():
    import FreeCAD as app
    import FreeCADGui as gui
    import Part
    from PySide6 import QtWidgets

    project = Path(os.environ["XLINK_PROJECT"])
    report = {"pid": os.getpid(), "version": app.Version(), "occt": Part.OCC_VERSION}
    try:
        images = {Path("/proc/self/exe").resolve(), Path(Part.__file__).resolve()}
        for line in Path("/proc/self/maps").read_text().splitlines():
            p = Path(line.split()[-1])
            if p.name in ("libFreeCADApp.so", "libFreeCADGui.so"):
                images.add(p)
        report["images"] = {str(p): digest(p) for p in images}
        mode = os.environ["XLINK_MODE"]
        report["result"] = (
            reopen(app, project, json.loads(Path(os.environ["XLINK_EXPECTED"]).read_text()))
            if mode == "reopen"
            else create(app, gui, project, mode)
        )
    except Exception:
        report["failure"] = traceback.format_exc()
    finally:
        for name in list(app.listDocuments()):
            app.closeDocument(name)
        report["remaining_documents"] = len(app.listDocuments())
        Path(os.environ["XLINK_REPORT"]).write_text(json.dumps(report, indent=2) + "\n")
        QtWidgets.QApplication.instance().quit()


def launch(executable, output, mode, project, expected=None):
    output.mkdir()
    env = os.environ.copy()
    for key, folder in (
        ("HOME", "home"),
        ("TMPDIR", "tmp"),
        ("XDG_CONFIG_HOME", "config"),
        ("XDG_DATA_HOME", "data"),
        ("XDG_CACHE_HOME", "cache"),
        ("XDG_RUNTIME_DIR", "runtime"),
        ("FREECAD_USER_HOME", "freecad-home"),
        ("FREECAD_USER_DATA", "freecad-data"),
    ):
        p = output / folder
        p.mkdir(mode=0o700)
        env[key] = str(p)
    env.update(
        XLINK_PROJECT=str(project),
        XLINK_MODE=mode,
        XLINK_REPORT=str(output / "report.json"),
        XLINK_EXPECTED=str(expected),
        QT_QPA_PLATFORM="xcb",
        LIBGL_ALWAYS_SOFTWARE="1",
        PYTHONNOUSERSITE="1",
    )
    macro = output / "case.FCMacro"
    macro.write_text(
        "import runpy\nfrom PySide6 import QtCore\n"
        + "QtCore.QTimer.singleShot(500, lambda: runpy.run_path("
        + repr(str(Path(__file__).resolve()))
        + ", run_name='xlink_cell')['cell']())\n"
    )
    argv = [
        "timeout",
        "--kill-after=5s",
        "90s",
        "xvfb-run",
        "-a",
        str(executable),
        "--user-cfg",
        str(output / "user.cfg"),
        "--system-cfg",
        str(output / "system.cfg"),
        str(macro),
    ]
    with (output / "native.log").open("w") as log:
        process = subprocess.run(argv, env=env, cwd=output, stdout=log, stderr=subprocess.STDOUT)
    report = json.loads((output / "report.json").read_text())
    report.update(command=argv, exit=process.returncode)
    assert process.returncode == 0 and not report.get("failure"), report
    assert (
        report["remaining_documents"] == 0 and not Path(f"/proc/{report['pid']}").exists()
    ), report
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--freecad", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir()
    results = {}
    for mode in ("save", "save-all", "save-as", "save-copy"):
        case = output / mode
        case.mkdir()
        source = case / "source"
        saved = launch(args.freecad.resolve(), case / "create", mode, source)
        expected = case / "expected.json"
        expected.write_text(json.dumps(saved["result"]))
        relocated = case / "relocated"
        shutil.copytree(source, relocated)
        source.rename(case / "original-hidden")
        assert not source.exists()
        for name, record in saved["result"]["files"].items():
            assert digest(relocated / name) == record["sha256"]
        reopened = launch(args.freecad.resolve(), case / "reopen", "reopen", relocated, expected)
        assert saved["images"] == reopened["images"]
        results[mode] = {"create": saved, "reopen": reopened, "source_hidden": True}
        (output / "summary.json").write_text(json.dumps(results, indent=2) + "\n")
    print(
        "All four save modes passed fresh-process relocation, shared/nested identity and geometry checks."
    )


if __name__ == "__main__":
    main()
