# SPDX-License-Identifier: LGPL-2.1-or-later
"""Generate and reopen the minimal arc fixture in an owned safe-mode Cmd process."""

import json
from pathlib import Path
import FreeCAD as App
import Part
import Sketcher

try:
    doc = App.newDocument("Spec004T001Arcs")
    source = doc.addObject("Part::Feature", "Source")
    source.Shape = Part.makeCompound([
        Part.ArcOfCircle(Part.Circle(App.Vector(x, 0, 0), App.Vector(0, 0, 1), 3), 0.2, 1.2).toShape()
        for x in (0, 20, 40)
    ])
    sketch = doc.addObject("Sketcher::SketchObject", "Sketch")
    for number in (1, 2):
        sketch.addExternal("Source", f"Edge{number}")
    for index, x in enumerate((0, 20)):
        internal = sketch.addGeometry(Part.Circle(App.Vector(x, 0, 0), App.Vector(0, 0, 1), 1), False)
        sketch.addConstraint(Sketcher.Constraint("Coincident", internal, 3, -3 - index, 3))
        sketch.addConstraint(Sketcher.Constraint("Radius", internal, 1.0))
    doc.recompute()
    assert sketch.FullyConstrained, "dependent fixture is not fully constrained"
    assert sum(len(subs) for obj, subs in sketch.ExternalGeometry) == 2, str(sketch.ExternalGeometry)
    assert all(isinstance(curve, Part.ArcOfCircle) for curve in sketch.ExternalGeo[2:]), str([type(c) for c in sketch.ExternalGeo])
    ids = [Sketcher.ExternalGeometryFacade(sketch.ExternalGeo[index]).Id for index in (2, 3)]
    path = Path("t001-arcs.FCStd").resolve()
    doc.saveAs(str(path))
    App.closeDocument(doc.Name)
    doc = App.openDocument(str(path))
    sketch = doc.getObject("Sketch")
    doc.recompute()
    assert sketch.FullyConstrained, "dependent fixture is not fully constrained"
    assert [Sketcher.ExternalGeometryFacade(sketch.ExternalGeo[index]).Id for index in (2, 3)] == ids
    assert sketch.ConstraintCount == 4
    assert all(isinstance(curve, Part.ArcOfCircle) for curve in sketch.ExternalGeo[2:]), str([type(c) for c in sketch.ExternalGeo])
    Path("arc-receipt.json").write_text(json.dumps({
        "fixture": "two referenced circular arcs plus two centre-constrained internal circles",
        "replacement_candidate": "Source.Edge3 (not attached)",
        "native_ids_before_and_after_reopen": ids,
        "constraint_count": sketch.ConstraintCount,
        "fully_constrained": sketch.FullyConstrained,
        "saved_file": str(path),
        "result": "passed (fixture qualification only, no reattachment API call)",
    }, indent=2) + "\n")
    App.closeDocument(doc.Name)
except Exception:
    import traceback
    Path("arc-failure.txt").write_text(traceback.format_exc())
    raise
