# SPDX-License-Identifier: LGPL-2.1-or-later
"""Run with FreeCADCmd in an isolated profile; see the T001 baseline report.

Required environment: SKETCH_PERF_COUNTER, SKETCH_PERF_APP_LIB,
SKETCH_PERF_OUTPUT_DIR. Private cases also require SKETCH_PERF_FIXTURE.
SKETCH_PERF_CASE selects a workload; SKETCH_PERF_PAIRS defaults to ten A/A pairs.
This is a baseline tool, not the production phase-tracing interface.
"""

import ctypes
import hashlib
import json
import os
from pathlib import Path
import sys
import time
import traceback

import FreeCAD as App
import Part
import PartDesign  # noqa: F401 -- registers native Body/Pad/Pocket types
import Sketcher


class NativeCounts:
    def __init__(self):
        self.library = ctypes.CDLL(os.environ["SKETCH_PERF_COUNTER"])
        self.library.skperf_bind.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        self.library.skperf_bind.restype = ctypes.c_int
        if (
            self.library.skperf_bind(
                os.fsencode(Sketcher.__file__), os.fsencode(os.environ["SKETCH_PERF_APP_LIB"])
            )
            != 1
        ):
            raise RuntimeError("counter could not bind the exact loaded native modules")
        self.library.skperf_begin.restype = None
        self.library.skperf_end.restype = ctypes.c_int
        self.library.skperf_row.argtypes = [
            ctypes.c_size_t,
            ctypes.c_void_p,
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_uint64),
            ctypes.POINTER(ctypes.c_uint64),
        ]
        self.library.skperf_row.restype = ctypes.c_int

    def begin(self):
        self.library.skperf_begin()

    def end(self):
        if self.library.skperf_end() != 1:
            raise RuntimeError("counter owner mapping, name, or capacity was invalid")
        result = {}
        for index in range(2048):
            name = ctypes.create_string_buffer(128)
            executes = ctypes.c_uint64()
            setups = ctypes.c_uint64()
            status = self.library.skperf_row(
                index, name, 128, ctypes.byref(executes), ctypes.byref(setups)
            )
            if status == 0:
                break
            if status != 1:
                raise RuntimeError("invalid counter snapshot")
            key = name.value.decode()
            row = result.setdefault(key, {"executes": 0, "setups": 0})
            row["executes"] += executes.value
            row["setups"] += setups.value
        return result


def sketch_state(document):
    return {
        obj.Name: {
            "geometry": obj.GeometryCount,
            "constraints": obj.ConstraintCount,
            "dof": obj.DoF,
            "conflicting": list(obj.ConflictingConstraints),
            "redundant": list(obj.RedundantConstraints),
            "malformed": list(obj.MalformedConstraints),
        }
        for obj in document.Objects
        if obj.isDerivedFrom("Sketcher::SketchObject")
    }


def preference_snapshot(group):
    return {
        "values": group.GetContents(),
        "groups": {name: preference_snapshot(group.GetGroup(name)) for name in group.GetGroups()},
    }


def check_document(document, thickness=0.8):
    assert list(App.listDocuments()) == [document.Name], "unexpected companion document"
    invalid = [obj.Name for obj in document.Objects if "Invalid" in obj.State]
    if invalid:
        raise AssertionError(f"invalid document objects: {invalid}")
    body = document.getObject("BottomPlate")
    if body:
        shape = body.Shape
        assert shape.isValid() and len(shape.Solids) == 1
        assert abs(shape.BoundBox.ZMin + thickness) < 1e-7
        assert abs(shape.BoundBox.ZMax) < 1e-7
        for name in ("BottomPlateSheet", "BottomPlateThroughCutouts"):
            assert abs(document.getObject(name).Length.Value - thickness) < 1e-7


def circle_document():
    document = App.newDocument("BaselineCircle")
    sketch = document.addObject("Sketcher::SketchObject", "Profile")
    sketch.addGeometry(Part.Circle(App.Vector(1, 2, 0), App.Vector(0, 0, 1), 3), False)
    document.recompute()
    return document


def line_document():
    document = App.newDocument("BaselineLines")
    sketch = document.addObject("Sketcher::SketchObject", "Profile")
    sketch.addGeometry(
        [
            Part.LineSegment(App.Vector(10 * i, 0, 0), App.Vector(10 * i + 4, 0, 0))
            for i in range(16)
        ],
        False,
    )
    document.recompute()
    return document


def main():
    output = Path(os.environ["SKETCH_PERF_OUTPUT_DIR"])
    output.mkdir(parents=True, exist_ok=True)
    case = os.environ.get("SKETCH_PERF_CASE", "forced_synthetic")
    pairs = int(os.environ.get("SKETCH_PERF_PAIRS", "10"))
    configuration = os.environ.get("SKETCH_PERF_CONFIGURATION", "baseline")
    assert configuration in ("baseline", "candidate")
    candidate = configuration == "candidate"
    assert case in {
        "clean_noop",
        "forced_synthetic",
        "forced_profile",
        "forced_master",
        "fresh_load",
        "warm_load",
        "thickness",
        "scalar_insertion",
        "list_insertion",
    }
    assert pairs >= 0
    (output / "result.json").write_text('{"status": "running"}\n')
    counter = NativeCounts()
    # Disable existing log sinks consistently, without changing solver settings.
    for observer in App.Console.GetObservers():
        App.Console.SetStatus(observer, "Log", False)
    fixture = (
        Path(os.environ["SKETCH_PERF_FIXTURE"]) if "SKETCH_PERF_FIXTURE" in os.environ else None
    )
    if fixture:
        assert hashlib.sha256(fixture.read_bytes()).hexdigest() == (
            "55a0b21e57c979fe9e53dd89c073c47b2718257e7e7b10298375379094e3d4eb"
        )

    rows = []
    expected = None

    def measure(operation, validate, label):
        nonlocal expected
        counter.begin()
        started = time.perf_counter_ns()
        try:
            value = operation()
        except BaseException:
            counter.end()
            raise
        elapsed = time.perf_counter_ns() - started
        counts = counter.end()
        if expected is None:
            expected = counts
        assert counts == expected, (case, "execution vector changed", expected, counts)
        details = validate(value, counts)
        row = {"label": label, "elapsed_ns": elapsed, "counts": counts, "state": details}
        rows.append(row)
        (output / "raw.json").write_text(json.dumps(rows, indent=2) + "\n")
        return value

    def open_fixture():
        assert fixture is not None
        return App.openDocument(str(fixture))

    if case == "fresh_load":

        def validate_load(document, counts):
            check_document(document)
            state = sketch_state(document)
            assert "BottomPlateProfile" in counts and counts["BottomPlateProfile"]["setups"] > 0
            return state

        document = measure(open_fixture, validate_load, "fresh-process")
        App.closeDocument(document.Name)
    else:
        persistent = None
        if case in ("clean_noop", "forced_synthetic"):
            persistent = circle_document()
        elif case in ("forced_profile", "forced_master"):
            persistent = open_fixture()
            persistent.recompute()

        for iteration in range(2 + pairs * 2):
            if iteration < 2:
                label = f"warmup-{iteration}"
            else:
                pair, side = divmod(iteration - 2, 2)
                order = ("A", "B") if pair % 2 == 0 else ("B", "A")
                label = f"pair-{pair}-{order[side]}"

            if case == "warm_load":
                document = measure(open_fixture, validate_private_load, label)
                App.closeDocument(document.Name)
                continue
            if case in ("scalar_insertion", "list_insertion"):
                document = line_document()
                sketch = document.Profile
                document.openTransaction("baseline insertion")

                def insert():
                    constraints = [Sketcher.Constraint("Horizontal", i) for i in range(16)]
                    if case == "scalar_insertion":
                        for constraint in constraints:
                            sketch.addConstraint(constraint)
                    else:
                        sketch.addConstraint(constraints)
                        assert sketch.solve() == 0
                    document.recompute()

                def validate_insertion(value, counts):
                    wanted = (
                        (17 if candidate else 18)
                        if case == "scalar_insertion"
                        else (2 if candidate else 3)
                    )
                    assert counts == {"Profile": {"executes": 1, "setups": wanted}}
                    check_document(document)
                    assert sketch.ConstraintCount == 16 and sketch.DoF == 48
                    return sketch_state(document)

                measure(insert, validate_insertion, label)
                document.commitTransaction()
                document.undo()
                assert sketch.ConstraintCount == 0
                App.closeDocument(document.Name)
                continue
            if case == "thickness":
                document = open_fixture()
                document.recompute()
                before = sketch_state(document)

                def change_thickness():
                    document.Dimensions.BottomPlateThickness = 0.81
                    document.recompute()

                def validate_thickness(value, counts):
                    check_document(document, 0.81)
                    assert sketch_state(document) == before
                    return before

                measure(change_thickness, validate_thickness, label)
                document.Dimensions.BottomPlateThickness = 0.8
                document.recompute()
                check_document(document, 0.8)
                App.closeDocument(document.Name)
                continue

            document = persistent
            document.recompute()
            name = "BottomPlateProfile" if case == "forced_profile" else "Sketch004"
            if case in ("clean_noop", "forced_synthetic"):
                name = "Profile"
            sketch = document.getObject(name)
            before = sketch_state(document)
            if case != "clean_noop":
                sketch.touch()  # Outside the measured/counting window.

            def validate_recompute(value, counts):
                wanted = (
                    {}
                    if case == "clean_noop"
                    else {name: {"executes": 1, "setups": 1 if candidate else 2}}
                )
                assert counts == wanted, (case, counts)
                assert sketch_state(document) == before
                check_document(document)
                return before

            measure(lambda: document.recompute([sketch]), validate_recompute, label)
        if persistent:
            App.closeDocument(persistent.Name)

    (output / "result.json").write_text(
        json.dumps(
            {
                "status": "passed",
                "case": case,
                "configuration": configuration,
                "version": App.Version(),
                "samples": len(rows),
                "run_id": os.environ.get("SKETCH_PERF_RUN_ID"),
                "sketcher_preferences": preference_snapshot(
                    App.ParamGet("User parameter:BaseApp/Preferences/Mod/Sketcher")
                ),
                "modules": {
                    "App": os.environ["SKETCH_PERF_APP_LIB"],
                    "Part": Part.__file__,
                    "Sketcher": Sketcher.__file__,
                    "PartDesign": sys.modules["_PartDesign"].__file__,
                },
            },
            indent=2,
        )
        + "\n"
    )


def validate_private_load(document, counts):
    check_document(document)
    assert "BottomPlateProfile" in counts and counts["BottomPlateProfile"]["setups"] > 0
    return sketch_state(document)


try:
    main()
except BaseException:
    output = Path(os.environ["SKETCH_PERF_OUTPUT_DIR"])
    output.mkdir(parents=True, exist_ok=True)
    (output / "result.json").write_text(
        json.dumps({"status": "failed", "traceback": traceback.format_exc()}, indent=2) + "\n"
    )
    raise
