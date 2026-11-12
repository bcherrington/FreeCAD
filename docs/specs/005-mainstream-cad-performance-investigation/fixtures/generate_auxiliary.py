# SPDX-License-Identifier: LGPL-2.1-or-later
"""Public jointed assembly, real-CAD tree, detailed STEP and QR control fixtures.

Preparation only. Run in a new task-owned directory with the same locked,
headless module environment as generate_fixtures.py. No timing result is emitted.
"""
import sys
from pathlib import Path

sys.path.insert(
    0, str(Path.cwd() / "docs/specs/005-mainstream-cad-performance-investigation/fixtures")
)
from generate_fixtures import App, Part, Sketcher, near, save, close, digest

import json
import math
import sys
from pathlib import Path
import Assembly
import JointObject


def assembly(fixture_id, output, count):
    doc = App.newDocument(fixture_id)
    obj = doc.addObject("Assembly::AssemblyObject", "Assembly")
    joints = obj.newObject("Assembly::JointGroup", "Joints")
    source = doc.addObject("Part::Box", "SharedPart")
    source.Length, source.Width, source.Height = 10, 5, 4
    links = []
    for i in range(count):
        link = obj.newObject("App::Link", "Component%03d" % i)
        link.setLink(source)
        link.LinkPlacement.Base = App.Vector(i * 20, 0, 0)
        links.append(link)
    doc.recompute()
    ground = joints.newObject("App::FeaturePython", "Grounded")
    JointObject.GroundedJoint(ground, links[0])
    preferences = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Assembly")
    had_previous = any(
        entry[1] == "SolveInJointCreation" for entry in (preferences.GetContents() or [])
    )
    previous = preferences.GetBool("SolveInJointCreation", True)
    preferences.SetBool("SolveInJointCreation", False)
    try:
        for i in range(1, count):
            joint = joints.newObject("App::FeaturePython", "Joint%03d" % i)
            JointObject.Joint(joint, 1 if i == count - 1 else 0)
            joint.Offset1 = App.Placement(App.Vector(i * 20, 0, 0), App.Rotation())
            joint.Proxy.setJointConnectors(
                joint, [[links[0], ["Face6", "Vertex7"]], [links[i], ["Face6", "Vertex7"]]]
            )
        doc.recompute()
        assert obj.solve() == 0
        for i, link in enumerate(links):
            assert link.LinkedObject == source
            near(link.Placement.Base.x, i * 20)
            near(link.Placement.Base.y, 0)
            near(link.Placement.Base.z, 0)
            near(link.Shape.Volume, 200)
        # A fixed-joint perturbation must resolve to the analytically fixed pose.
        links[1].LinkPlacement.Base += App.Vector(0, 1, 0)
        assert obj.solve() == 0
        near(links[1].Placement.Base.y, 0)
        # Source edits propagate through all links without changing their target identity.
        source.Length = 12
        doc.recompute()
        assert obj.solve() == 0
        for link in links:
            near(link.Shape.Volume, 240)
        source.Length = 10
        doc.recompute()
        assert obj.solve() == 0
    finally:
        if had_previous:
            preferences.SetBool("SolveInJointCreation", previous)
        else:
            preferences.RemBool("SolveInJointCreation")
        assert (
            any(entry[1] == "SolveInJointCreation" for entry in (preferences.GetContents() or []))
            == had_previous
        )
    return save(
        doc,
        fixture_id,
        output,
        {
            "components": count,
            "link_targets": "SharedPart",
            "fixed_joints": count - 2,
            "revolute_joints": 1,
            "grounded_components": 1,
            "mechanical_dof": 1,
            "reported_solver_dof": "not assumed exposed",
            "baseline_placements": [[20 * i, 0, 0] for i in range(count)],
            "source_dimensions": [10, 5, 4],
            "edited_dimensions": [12, 5, 4],
            "component_volume": 200,
            "edited_component_volume": 240,
            "joint_connector": [10, 5, 4],
            "placement_tolerance": 1e-6,
        },
    )


def tree(fixture_id, output, count):
    doc = App.newDocument(fixture_id)
    root = doc.addObject("App::Part", "CADAssembly")
    nested = doc.addObject("App::DocumentObjectGroup", "Subassembly")
    root.addObject(nested)
    source = doc.addObject("Part::Box", "PartSource")
    source.Length, source.Width, source.Height = 10, 5, 4
    for i in range(count):
        parent = nested if i < count // 2 else root
        link = doc.addObject("App::Link", "Part%04d" % i)
        parent.addObject(link)
        link.setLink(source)
        link.LinkPlacement.Base = App.Vector(20 * (i % 32), 12 * (i // 32), 0)
    doc.recompute()
    assert len([o for o in doc.Objects if o.TypeId == "App::Link"]) == count
    assert len(doc.Objects) == count + 11
    return save(
        doc,
        fixture_id,
        output,
        {
            "instances": count,
            "objects": count + 11,
            "framework_origin_objects": 8,
            "link_target": "PartSource",
            "part_dimensions": [10, 5, 4],
            "placement_formula": "[20*(i%32),12*(i//32),0]",
            "nested_members": count // 2,
            "images": 0,
            "usage": "CAD link/tree/viewport control, not a joint-solving assembly",
        },
    )


def detailed(fixture_id, output, nx, ny):
    width, height, depth, radius = 10.0 * (nx + 1), 10.0 * (ny + 1), 8.0, 2.0
    base = Part.makeBox(width, height, depth)
    tools = [
        Part.makeCylinder(radius, depth, App.Vector(10 * (x + 1), 10 * (y + 1), 0))
        for y in range(ny)
        for x in range(nx)
    ]
    shape = base.cut(Part.makeCompound(tools))
    volume = (width * height - nx * ny * math.pi * radius**2) * depth
    near(shape.Volume, volume)
    assert shape.isValid() and len(shape.Solids) == 1
    assert len(shape.Faces) == 6 + nx * ny
    doc = App.newDocument(fixture_id)
    feature = doc.addObject("Part::Feature", "DetailedPlate")
    feature.Shape = shape
    step = output / (fixture_id + ".step")
    Part.export([feature], str(step))
    imported = Part.read(str(step))
    near(imported.Volume, volume)
    assert imported.isValid() and len(imported.Solids) == 1
    top = [
        (i + 1, face)
        for i, face in enumerate(imported.Faces)
        if face.Surface.__class__.__name__ == "Plane" and abs(face.CenterOfMass.z - depth) < 1e-7
    ]
    assert len(top) == 1
    assert top[0][1].distToShape(Part.Vertex(App.Vector(5, 5, depth)))[0] < 1e-7
    expected = {
        "nx": nx,
        "ny": ny,
        "holes": nx * ny,
        "dimensions": [width, height, depth],
        "radius": radius,
        "volume": volume,
        "solids": 1,
        "faces": 6 + nx * ny,
        "top_face_area": width * height - nx * ny * math.pi * radius**2,
        "pick_landmark": [5, 5, depth],
        "semantic_face": "unique planar face at z=8",
        "observed_STEP_top_face": "Face%d" % top[0][0],
    }
    row = save(doc, fixture_id, output, expected)
    row["step"] = {
        "path": str(step.relative_to(Path.cwd())),
        "sha256": digest(step),
        "bytes": step.stat().st_size,
        "preparation_import": "Part.read passed; GUI importer T002 pending",
    }
    return row


def symmetric(fixture_id, output, epsilon):
    doc = App.newDocument(fixture_id)
    sketch = doc.addObject("Sketcher::SketchObject", "Control")
    sketch.addGeometry(
        [Part.Point(App.Vector(-10, epsilon, 0)), Part.Point(App.Vector(10, -epsilon, 0))], False
    )
    sketch.addConstraint(
        [
            Sketcher.Constraint("Symmetric", 0, 1, 1, 1, -1, 1),
            Sketcher.Constraint("DistanceX", 1, 1, 10.0),
            Sketcher.Constraint("DistanceY", 0, 1, epsilon),
        ]
    )
    doc.recompute()
    assert sketch.FullyConstrained
    near(sketch.Geometry[0].X, -10)
    near(sketch.Geometry[0].Y, epsilon)
    return save(
        doc,
        fixture_id,
        output,
        {
            "points": [[-10, epsilon, 0], [10, -epsilon, 0]],
            "dof": 0,
            "parameters": 4,
            "scalar_constraint_rank": 4,
            "conflicts": [],
            "redundancies": [],
            "epsilon": epsilon,
            "warning": "point-symmetry full-rank control, not a reproduction of historical mutation stalls",
        },
    )


def aligned_slot(fixture_id, output):
    doc = App.newDocument(fixture_id)
    sketch = doc.addObject("Sketcher::SketchObject", "Slot")
    # Straight segments and semicircles with exact analytic tangent endpoints.
    sketch.addGeometry(
        [
            Part.LineSegment(App.Vector(0, -5, 0), App.Vector(20, -5, 0)),
            Part.ArcOfCircle(
                Part.Circle(App.Vector(20, 0, 0), App.Vector(0, 0, 1), 5), -math.pi / 2, math.pi / 2
            ),
            Part.LineSegment(App.Vector(20, 5, 0), App.Vector(0, 5, 0)),
            Part.ArcOfCircle(
                Part.Circle(App.Vector(0, 0, 0), App.Vector(0, 0, 1), 5),
                math.pi / 2,
                3 * math.pi / 2,
            ),
        ],
        False,
    )
    sketch.addConstraint(
        [
            Sketcher.Constraint("Tangent", 0, 2, 1, 1),
            Sketcher.Constraint("Tangent", 1, 2, 2, 1),
            Sketcher.Constraint("Tangent", 2, 2, 3, 1),
            Sketcher.Constraint("Tangent", 3, 2, 0, 1),
            Sketcher.Constraint("Horizontal", 0),
            Sketcher.Constraint("Horizontal", 2),
            Sketcher.Constraint("Distance", 0, 20.0),
            Sketcher.Constraint("Radius", 1, 5.0),
            Sketcher.Constraint("DistanceX", 3, 3, 0.0),
            Sketcher.Constraint("DistanceY", 3, 3, 0.0),
        ]
    )
    doc.recompute()
    assert sketch.FullyConstrained
    near(sketch.Shape.Length, 40 + 10 * math.pi)
    return save(
        doc,
        fixture_id,
        output,
        {
            "straight_length": 20,
            "radius": 5,
            "perimeter": 40 + 10 * math.pi,
            "area": 200 + 25 * math.pi,
            "dof": 0,
            "role": "relational tangent aligned capsule; analytic geometry/rank full-column oracle, not a historical bug replay",
        },
    )


def near_tangent(fixture_id, output, epsilon):
    doc = App.newDocument(fixture_id)
    sketch = doc.addObject("Sketcher::SketchObject", "NearTangent")
    sketch.addGeometry(
        [
            Part.Point(App.Vector(-10, 0, 0)),
            Part.Point(App.Vector(10, 0, 0)),
            Part.Point(App.Vector(0, epsilon, 0)),
        ],
        False,
    )
    distance = math.sqrt(100 + epsilon**2)
    sketch.addConstraint(
        [
            Sketcher.Constraint("Block", 0),
            Sketcher.Constraint("Block", 1),
            Sketcher.Constraint("Distance", 0, 1, 2, 1, distance),
            Sketcher.Constraint("Distance", 1, 1, 2, 1, distance),
        ]
    )
    doc.recompute()
    assert sketch.FullyConstrained
    near(sketch.Geometry[2].X, 0)
    near(sketch.Geometry[2].Y, epsilon)
    return save(
        doc,
        fixture_id,
        output,
        {
            "fixed_points": [[-10, 0], [10, 0]],
            "positive_solution": [0, epsilon],
            "distance": distance,
            "dof": 0,
            "independent_reduced_rank": 2,
            "independent_jacobian": [
                [10 / distance, epsilon / distance],
                [-10 / distance, epsilon / distance],
            ],
            "analytic_condition_number": 10 / epsilon,
            "rank_basis": "nonzero determinant 20*epsilon/distance**2; rank(J)=rank(JT)=remaining parameter count after constant elimination; JT need not have full column rank",
        },
    )


def main():
    output = Path(sys.argv[1]).resolve()
    assert output.is_relative_to(Path.cwd() / "build/spec005-t001")
    output.mkdir(parents=True, exist_ok=False)
    recipes = [
        (assembly, "W03-small", (4,)),
        (assembly, "W03-large", (24,)),
        (tree, "W03-tree-small", (16,)),
        (tree, "W03-tree-large", (512,)),
        (detailed, "W04-small", (8, 6)),
        (detailed, "W04-large", (16, 12)),
        (symmetric, "Q-symmetry", (2.0,)),
        (symmetric, "Q-near-axis", (1e-4,)),
        (aligned_slot, "Q-aligned-slot", ()),
        (near_tangent, "Q-near-tangent", (1e-4,)),
    ]
    rows = []
    for function, fixture_id, args in recipes:
        row = function(fixture_id, output, *args)
        rows.append(row)
        (output / "preparation.json").write_text(json.dumps(rows, indent=2) + "\n")
        print("PREPARED", fixture_id, row["bytes"], flush=True)


if __name__ == "__main__":
    main()
