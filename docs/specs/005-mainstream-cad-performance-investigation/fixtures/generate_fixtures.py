# SPDX-License-Identifier: LGPL-2.1-or-later
"""Create public Spec005 fixtures in a new owned directory, never an operator document.

Run under the locked Pixi Python with explicit build module paths. These are
fixture preparation checks, not performance observations or harness qualification.
FCStd containers include generated metadata: pin the actual produced bytes as
well as this recipe; regeneration promises semantic, not archive-byte, identity.
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path.cwd() / "build/debug/lib"))
sys.path.extend(str(p) for p in (Path.cwd() / "build/debug/Mod").iterdir() if p.is_dir())
sys.path.append(str(Path.cwd() / ".pixi/envs/default/lib/python3.13/site-packages"))

import FreeCAD as App
import Part
import Sketcher

# FreeCAD initializes workbench scripts in __main__: import helper names afterwards.
import hashlib
import json
import math
import sys
from pathlib import Path


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def close(document):
    App.closeDocument(document.Name)


def near(a, b, atol=1e-7):
    assert math.isclose(float(a), float(b), rel_tol=1e-9, abs_tol=atol), (a, b)


def rectangle(body, width, height, name="Outline"):
    sketch = body.newObject("Sketcher::SketchObject", name)
    points = [(0, 0), (width, 0), (width, height), (0, height)]
    for i in range(4):
        a, b = points[i], points[(i + 1) % 4]
        sketch.addGeometry(Part.LineSegment(App.Vector(*a, 0), App.Vector(*b, 0)), False)
    for i in range(4):
        sketch.addConstraint(Sketcher.Constraint("Coincident", i, 2, (i + 1) % 4, 1))
    for i in [0, 2]:
        sketch.addConstraint(Sketcher.Constraint("Horizontal", i))
    for i in [1, 3]:
        sketch.addConstraint(Sketcher.Constraint("Vertical", i))
    sketch.addConstraint(Sketcher.Constraint("Coincident", 0, 1, -1, 1))
    w = sketch.addConstraint(Sketcher.Constraint("Distance", 0, width))
    h = sketch.addConstraint(Sketcher.Constraint("Distance", 1, height))
    sketch.renameConstraint(w, "Width")
    sketch.renameConstraint(h, "Height")
    sketch.Document.recompute()
    assert sketch.FullyConstrained
    return sketch


def circle(sketch, x, y, radius, first=None):
    index = sketch.addGeometry(Part.Circle(App.Vector(x, y, 0), App.Vector(0, 0, 1), radius), False)
    sketch.addConstraint(Sketcher.Constraint("DistanceX", index, 3, x))
    sketch.addConstraint(Sketcher.Constraint("DistanceY", index, 3, y))
    if first is None:
        dim = sketch.addConstraint(Sketcher.Constraint("Radius", index, radius))
        sketch.renameConstraint(dim, "HoleRadius")
    else:
        sketch.addConstraint(Sketcher.Constraint("Equal", first, index))
    return index


def snapshot(document):
    shapes = {}
    sketches = {}
    for obj in document.Objects:
        assert "Invalid" not in obj.State and "RecomputeError" not in obj.State, (
            obj.Name,
            obj.State,
        )
        if obj.isDerivedFrom("Sketcher::SketchObject"):
            signature = []
            for geometry in obj.Geometry:
                kind = geometry.__class__.__name__
                if kind == "LineSegment":
                    values = list(geometry.StartPoint) + list(geometry.EndPoint)
                elif kind in {"Circle", "ArcOfCircle"}:
                    values = list(geometry.Location) + [geometry.Radius]
                    if kind == "ArcOfCircle":
                        values += [geometry.FirstParameter, geometry.LastParameter]
                elif kind == "Point":
                    values = [geometry.X, geometry.Y, geometry.Z]
                else:
                    raise AssertionError((obj.Name, "unexpected geometry", kind))
                signature.append([kind] + [round(float(v), 9) for v in values])
            constraints = [
                [
                    c.Type,
                    c.First,
                    c.FirstPos,
                    c.Second,
                    c.SecondPos,
                    c.Third,
                    c.ThirdPos,
                    round(c.Value, 9),
                ]
                for c in obj.Constraints
            ]
            sketches[obj.Name] = {
                "geometry": obj.GeometryCount,
                "constraints": obj.ConstraintCount,
                "fully_constrained": obj.FullyConstrained,
                "geometry_signature": signature,
                "constraint_signature": constraints,
            }
        if hasattr(obj, "Shape") and not obj.Shape.isNull():
            shape = obj.Shape
            assert shape.isValid(), obj.Name
            shapes[obj.Name] = {
                "solids": len(shape.Solids),
                "faces": len(shape.Faces),
                "edges": len(shape.Edges),
                "vertices": len(shape.Vertexes),
                "volume": shape.Volume,
                "area": shape.Area,
                "bbox": [
                    getattr(shape.BoundBox, x)
                    for x in ["XMin", "YMin", "ZMin", "XMax", "YMax", "ZMax"]
                ],
            }
    links = {
        obj.Name: obj.LinkedObject.Name for obj in document.Objects if obj.TypeId == "App::Link"
    }
    placements = {
        obj.Name: [round(v, 9) for v in list(obj.Placement.Base) + list(obj.Placement.Rotation.Q)]
        for obj in document.Objects
        if hasattr(obj, "Placement")
    }
    return {
        "objects": len(document.Objects),
        "types": {obj.Name: obj.TypeId for obj in document.Objects},
        "shapes": shapes,
        "sketches": sketches,
        "link_targets": links,
        "placements": placements,
    }


def save(document, fixture_id, output, expected):
    document.recompute()
    before = snapshot(document)
    path = output / (fixture_id + ".FCStd")
    document.saveAs(str(path))
    close(document)
    reopened = App.openDocument(str(path))
    reopened.recompute()
    after = snapshot(reopened)
    assert before["objects"] == after["objects"]
    assert before["types"] == after["types"]
    assert before["link_targets"] == after["link_targets"]
    assert before["placements"] == after["placements"]
    for name, row in before["shapes"].items():
        actual = after["shapes"][name]
        assert row["solids"] == actual["solids"] and row["faces"] == actual["faces"], name
        assert row["edges"] == actual["edges"] and row["vertices"] == actual["vertices"], name
        near(row["volume"], actual["volume"])
        near(row["area"], actual["area"])
        for expected_value, actual_value in zip(row["bbox"], actual["bbox"]):
            near(expected_value, actual_value)
    assert before["sketches"] == after["sketches"]
    close(reopened)
    return {
        "id": fixture_id,
        "path": str(path.relative_to(Path.cwd())),
        "sha256": digest(path),
        "bytes": path.stat().st_size,
        "baseline": after,
        "analytic_expected": expected,
        "preparation_roundtrip": "passed; not timed study/harness qualification",
    }


def part(fixture_id, output, width, occurrences):
    doc = App.newDocument(fixture_id)
    body = doc.addObject("PartDesign::Body", "Body")
    outline = rectangle(body, width, 60.0)
    pad = body.newObject("PartDesign::Pad", "BasePad")
    pad.Profile = outline
    pad.Length = 12.0
    doc.recompute()
    hole = body.newObject("Sketcher::SketchObject", "HoleSketch")
    hole.Placement.Base.z = 12
    circle(hole, 10.0, 15.0, 3.0)
    doc.recompute()
    pocket = body.newObject("PartDesign::Pocket", "FirstHole")
    pocket.Profile = hole
    pocket.Length = 12
    doc.recompute()
    pattern = body.newObject("PartDesign::LinearPattern", "HolePattern")
    pattern.Originals = [pocket]
    pattern.Direction = (doc.getObject("X_Axis"), [""])
    pattern.Length = width - 30
    pattern.Occurrences = occurrences
    doc.recompute()
    expected = width * 60 * 12 - occurrences * math.pi * 9 * 12
    near(pattern.Shape.Volume, expected)
    assert len(pattern.Shape.Solids) == 1
    outline.setDatum("Width", App.Units.Quantity(width + 4))
    doc.recompute()
    near(pattern.Shape.Volume, expected + 4 * 60 * 12)
    outline.setDatum("Width", App.Units.Quantity(width))
    doc.recompute()
    return save(
        doc,
        fixture_id,
        output,
        {
            "volume": expected,
            "edited_width": width + 4,
            "edited_volume": expected + 2880,
            "hole_radius": 3,
            "hole_count": occurrences,
            "hole_centres": [
                [10 + i * (width - 30) / (occurrences - 1), 15] for i in range(occurrences)
            ],
        },
    )


def history(fixture_id, output, holes):
    width = 20.0 * holes + 20
    doc = App.newDocument(fixture_id)
    body = doc.addObject("PartDesign::Body", "Body")
    outline = rectangle(body, width, 40.0)
    pad = body.newObject("PartDesign::Pad", "BasePad")
    pad.Profile = outline
    pad.Length = 10
    doc.recompute()
    last = pad
    for i in range(holes):
        sketch = body.newObject("Sketcher::SketchObject", "Hole%02d" % i)
        sketch.Placement.Base.z = 10
        circle(sketch, 20.0 * (i + 1), 20.0, 3.0)
        doc.recompute()
        last = body.newObject("PartDesign::Pocket", "Pocket%02d" % i)
        last.Profile = sketch
        last.Length = 10
        doc.recompute()
    expected = width * 40 * 10 - holes * math.pi * 9 * 10
    near(last.Shape.Volume, expected)
    outline.setDatum("Width", App.Units.Quantity(width + 4))
    doc.recompute()
    near(last.Shape.Volume, expected + 1600)
    outline.setDatum("Width", App.Units.Quantity(width))
    doc.recompute()
    return save(
        doc,
        fixture_id,
        output,
        {
            "pocket_count": holes,
            "width": width,
            "height": 40,
            "thickness": 10,
            "volume": expected,
            "edited_volume": expected + 1600,
            "hole_centres": [[20 * (i + 1), 20] for i in range(holes)],
        },
    )


def sketch_plate(fixture_id, output, nx, ny):
    width, height, radius = 10.0 * (nx + 1), 10.0 * (ny + 1), 2.0
    doc = App.newDocument(fixture_id)
    body = doc.addObject("PartDesign::Body", "Body")
    sketch = rectangle(body, width, height)
    geometries = []
    constraints = []
    base = sketch.GeometryCount
    first_constraint = sketch.ConstraintCount
    for y in range(ny):
        for x in range(nx):
            index = base + len(geometries)
            cx, cy = 10.0 * (x + 1), 10.0 * (y + 1)
            geometries.append(Part.Circle(App.Vector(cx, cy, 0), App.Vector(0, 0, 1), radius))
            constraints.extend(
                [
                    Sketcher.Constraint("DistanceX", index, 3, cx),
                    Sketcher.Constraint("DistanceY", index, 3, cy),
                ]
            )
            constraints.append(
                Sketcher.Constraint("Radius", index, radius)
                if index == base
                else Sketcher.Constraint("Equal", base, index)
            )
    sketch.addGeometry(geometries, False)
    sketch.addConstraint(constraints)
    sketch.renameConstraint(first_constraint + 2, "HoleRadius")
    doc.recompute()
    assert sketch.FullyConstrained
    assert sketch.GeometryCount == 4 + nx * ny and sketch.ConstraintCount == 11 + 3 * nx * ny
    for i, geometry in enumerate(sketch.Geometry[4:]):
        near(geometry.Location.x, 10 * (i % nx + 1))
        near(geometry.Location.y, 10 * (i // nx + 1))
        near(geometry.Radius, radius)
    pad = body.newObject("PartDesign::Pad", "PlatePad")
    pad.Profile = sketch
    pad.Length = 5
    doc.recompute()
    volume = (width * height - nx * ny * math.pi * radius**2) * 5
    near(pad.Shape.Volume, volume)
    sketch.setDatum("HoleRadius", App.Units.Quantity(2.25))
    doc.recompute()
    edited = (width * height - nx * ny * math.pi * 2.25**2) * 5
    near(pad.Shape.Volume, edited)
    sketch.setDatum("HoleRadius", App.Units.Quantity(radius))
    doc.recompute()
    return save(
        doc,
        fixture_id,
        output,
        {
            "nx": nx,
            "ny": ny,
            "circles": nx * ny,
            "width": width,
            "height": height,
            "radius": radius,
            "thickness": 5,
            "volume": volume,
            "edited_radius": 2.25,
            "edited_volume": edited,
            "dof": 0,
            "geometry_count": 4 + nx * ny,
            "constraint_count": 11 + 3 * nx * ny,
        },
    )


def main():
    output = Path(sys.argv[1]).resolve()
    assert output.is_relative_to(Path.cwd() / "build/spec005-t001")
    output.mkdir(parents=True, exist_ok=False)
    rows = []
    recipes = [
        (part, "W01-small", (80.0, 3)),
        (part, "W01-large", (240.0, 10)),
        (history, "W02-small", (4,)),
        (history, "W02-large", (24,)),
        (sketch_plate, "W05-ordinary", (1, 1)),
        (sketch_plate, "W06-small", (4, 4)),
        (sketch_plate, "W06-large", (16, 16)),
    ]
    for function, name, args in recipes:
        row = function(name, output, *args)
        rows.append(row)
        (output / "preparation.json").write_text(json.dumps(rows, indent=2) + "\n")
        print("PREPARED", name, row["bytes"], flush=True)
    print("All selected native part/sketch fixture preparation checks passed.", flush=True)


if __name__ == "__main__":
    main()
