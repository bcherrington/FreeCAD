# SPDX-License-Identifier: LGPL-2.1-or-later
"""Public external-reference contract, using generated disposable analytic models."""

import os
import tempfile
import unittest

import FreeCAD as App
import Part
import Sketcher


class TestSketchExternalReattachment(unittest.TestCase):
    def setUp(self):
        self.doc = App.newDocument("ExternalReattachmentTest")
        self.doc.UndoMode = 1
        self.tmp = tempfile.TemporaryDirectory(prefix="freecad-reattach-")
        self.source = self.doc.addObject("Part::Feature", "Source")
        self.source.Shape = self.circles()
        self.sketch = self.doc.addObject("Sketcher::SketchObject", "Sketch")
        for sub in ("Edge1", "Edge2"):
            self.sketch.addExternal("Source", sub)
        self.doc.recompute()
        self.doc.clearUndos()
        self.doc.purgeTouched()

    def tearDown(self):
        App.closeDocument(self.doc.Name)
        self.tmp.cleanup()

    @staticmethod
    def circles(offset=0, arcs=False):
        curves = [
            Part.Circle(App.Vector(x + offset, 0, 0), App.Vector(0, 0, 1), 3) for x in (0, 20, 40)
        ]
        return Part.makeCompound(
            [(Part.ArcOfCircle(c, 0.2, 1.2) if arcs else c).toShape() for c in curves]
        )

    def ids(self):
        return tuple(Sketcher.ExternalGeometryFacade(g).Id for g in self.sketch.ExternalGeo)

    def constraints(self):
        return tuple(
            tuple(
                getattr(c, name)
                for name in (
                    "Type",
                    "First",
                    "FirstPos",
                    "Second",
                    "SecondPos",
                    "Third",
                    "ThirdPos",
                    "Value",
                    "Name",
                    "Driving",
                    "IsActive",
                    "InVirtualSpace",
                )
            )
            for c in self.sketch.Constraints
        )

    def state(self):
        sk = self.sketch
        return (
            self.ids(),
            self.constraints(),
            tuple(sk.State),
            tuple(sk.ExternalTypes),
            tuple((o.Name, tuple(subs)) for o, subs in sk.ExternalGeometry),
            tuple(Sketcher.ExternalGeometryFacade(g).Ref for g in sk.ExternalGeo),
            tuple(sorted(o.Name for o in sk.OutList)),
            self.doc.UndoCount,
            self.doc.RedoCount,
            sk.Label,
            tuple((str(path), str(expr)) for path, expr in sk.ExpressionEngine),
        )

    def dependent(self):
        for index, x in enumerate((0, 20)):
            i = self.sketch.addGeometry(
                Part.Circle(App.Vector(x, 0, 0), App.Vector(0, 0, 1), 1), False
            )
            self.sketch.addConstraint(Sketcher.Constraint("Coincident", i, 3, -3 - index, 3))
            r = self.sketch.addConstraint(Sketcher.Constraint("Radius", i, 1.0))
            self.sketch.renameConstraint(r, "Radius" + str(index))
            self.sketch.setExpression("Constraints.Radius" + str(index), "1 mm")
        self.doc.recompute()
        self.assertTrue(self.sketch.FullyConstrained)

    def assert_centres(self, first=40, second=20):
        self.assertAlmostEqual(self.sketch.ExternalGeo[2].Center.x, first, places=9)
        self.assertAlmostEqual(self.sketch.ExternalGeo[3].Center.x, second, places=9)
        self.assertEqual(self.sketch.solve(), 0)
        self.assertAlmostEqual(self.sketch.Geometry[0].Center.x, first, places=7)
        self.assertAlmostEqual(self.sketch.Geometry[1].Center.x, second, places=7)

    def test_scalar_surface_and_selected_only(self):
        before = self.ids()
        self.assertIsNone(self.sketch.reattachExternal(-3, "Source", "Edge3"))
        self.assertEqual(self.ids(), before)
        self.assertAlmostEqual(self.sketch.ExternalGeo[2].Center.x, 40, places=9)
        self.assertAlmostEqual(self.sketch.ExternalGeo[3].Center.x, 20, places=9)
        self.assertEqual(self.sketch.ExternalGeometry[0][1], ("Edge3", "Edge2"))
        self.assertIn("checkpoint", self.sketch.reattachExternal.__doc__)

    def test_numeric_and_string_boundaries(self):
        before = self.state()
        for value in (True, False, -3.0, "-3", None):
            with self.subTest(value=value), self.assertRaises(TypeError):
                self.sketch.reattachExternal(value, "Source", "Edge3")
        for value in (-(2**200), 2**200, -(2**31) - 1, 2**31):
            with self.subTest(value=value), self.assertRaises(OverflowError):
                self.sketch.reattachExternal(value, "Source", "Edge3")
        for args in ((-3, None, "Edge3"), (-3, "Source", None), (-3, "Source")):
            with self.subTest(args=args), self.assertRaises(TypeError):
                self.sketch.reattachExternal(*args)
        self.assertEqual(self.state(), before)

    def test_invalid_indices_sources_and_selectors(self):
        before = self.state()
        for index in (0, 1, -1, -2, -5, -(2**31), 2**31 - 1):
            with self.subTest(index=index), self.assertRaises(ValueError):
                self.sketch.reattachExternal(index, "Source", "Edge3")
        for obj, sub in (
            ("Absent", "Edge3"),
            ("Sketch", "Edge1"),
            ("Source", "Edge99"),
            ("Source", "Vertex1"),
            ("Source", ""),
            ("Source", "Edge2"),
        ):
            with self.subTest(obj=obj, sub=sub), self.assertRaises(ValueError):
                self.sketch.reattachExternal(-3, obj, sub)
        self.assertEqual(self.state(), before)

    def test_noop_and_rejection_preserve_redo_and_earlier_edits(self):
        self.doc.openTransaction("redo marker")
        self.sketch.Label = "Changed"
        self.doc.commitTransaction()
        self.doc.undo()
        self.assertEqual(self.doc.RedoCount, 1)
        before = self.state()
        self.sketch.reattachExternal(-3, "Source", "Edge1")
        self.assertEqual(self.state(), before)
        self.doc.openTransaction("caller")
        self.sketch.Label = "Earlier edit"
        self.sketch.reattachExternal(-3, "Source", "Edge3")
        before = self.state()
        self.sketch.reattachExternal(-3, "Source", "Edge3")
        with self.assertRaises(ValueError):
            self.sketch.reattachExternal(-3, "Source", "Edge2")
        self.assertEqual(self.state(), before)
        self.doc.abortTransaction()
        self.assertAlmostEqual(self.sketch.ExternalGeo[2].Center.x, 0, places=9)

    def test_coincident_centres_recompute_undo_redo_and_abort(self):
        self.dependent()
        ids, constraints = self.ids(), self.constraints()
        self.doc.openTransaction("dedicated replacement")
        self.sketch.reattachExternal(-3, "Source", "Edge3")
        self.assertEqual(self.constraints(), constraints)
        for _ in range(2):
            self.sketch.touch()
            self.doc.recompute()
            self.assert_centres()
        self.doc.commitTransaction()
        self.doc.undo()
        self.assert_centres(0)
        self.doc.redo()
        self.assert_centres()
        self.assertEqual(self.ids(), ids)
        self.doc.openTransaction("abort replacement")
        self.sketch.reattachExternal(-3, "Source", "Edge1")
        self.doc.abortTransaction()
        self.assert_centres()
        self.assertEqual(self.ids(), ids)
        self.assertEqual(self.constraints(), constraints)

    def test_arc_endpoints_and_centres_save_reopen(self):
        self.source.Shape = self.circles(arcs=True)
        self.doc.recompute()
        self.dependent()
        ids, constraints = self.ids(), self.constraints()
        before = self.sketch.ExternalGeo[2]
        self.sketch.reattachExternal(-3, "Source", "Edge3")
        self.doc.recompute()
        after = self.sketch.ExternalGeo[2]
        self.assertAlmostEqual(after.StartPoint.x, before.StartPoint.x + 40, places=9)
        self.assertAlmostEqual(after.EndPoint.x, before.EndPoint.x + 40, places=9)
        self.assert_centres()
        name = self.doc.Name
        path = os.path.join(self.tmp.name, "retargeted.FCStd")
        self.doc.saveAs(path)
        App.closeDocument(name)
        self.doc = App.openDocument(path)
        self.sketch = self.doc.getObject("Sketch")
        self.source = self.doc.getObject("Source")
        self.doc.recompute()
        self.assertEqual(self.ids(), ids)
        self.assertEqual(self.constraints(), constraints)
        self.assert_centres()
        self.source.Shape = self.circles(offset=5, arcs=True)
        self.doc.recompute()
        self.assert_centres(45, 25)

    def test_object_dependencies_and_source_propagation(self):
        self.dependent()
        new = self.doc.addObject("Part::Feature", "Replacement")
        new.Shape = self.circles(offset=10)
        self.sketch.reattachExternal(-3, new.Name, "Edge3")
        self.assertIn(self.sketch, new.InList)
        self.assertIn(self.source, self.sketch.OutList)  # second retained reference
        self.doc.recompute()
        self.assert_centres(50)
        self.source.Shape = self.circles(offset=5)
        self.doc.recompute()
        self.assert_centres(50, 25)
        new.Shape = self.circles(offset=15)
        self.doc.recompute()
        self.assert_centres(55, 25)

    def test_signed_tangent_line_lifecycle_and_reversal(self):
        def lines():
            return Part.makeCompound(
                [Part.makeLine(App.Vector(0, y, 0), App.Vector(10, y, 0)) for y in (0, 20, 5)]
                + [Part.makeLine(App.Vector(10, 5, 0), App.Vector(0, 5, 0))]
            )

        self.source.Shape = lines()
        self.doc.recompute()
        i = self.sketch.addGeometry(Part.Circle(App.Vector(5, 1, 0), App.Vector(0, 0, 1), 1), False)
        self.sketch.addConstraint(Sketcher.Constraint("Radius", i, 1.0))
        self.sketch.addConstraint(Sketcher.Constraint("DistanceX", i, 3, 5.0))
        self.sketch.addConstraint(Sketcher.Constraint("Tangent", i, -3))
        self.doc.recompute()
        ids, constraints = self.ids(), self.constraints()
        self.doc.openTransaction("tangent replacement")
        self.sketch.reattachExternal(-3, "Source", "Edge3")
        for _ in range(2):
            self.sketch.touch()
            self.doc.recompute()
            self.assertAlmostEqual(self.sketch.Geometry[0].Center.y, 6, places=7)
            self.assertEqual(self.constraints(), constraints)
        self.doc.commitTransaction()
        self.doc.undo()
        self.assertEqual(self.sketch.solve(), 0)
        self.assertAlmostEqual(self.sketch.Geometry[0].Center.y, 1, places=7)
        self.doc.redo()
        self.assertEqual(self.sketch.solve(), 0)
        self.assertAlmostEqual(self.sketch.Geometry[0].Center.y, 6, places=7)
        before = self.state()
        with self.assertRaises(ValueError):
            self.sketch.reattachExternal(-3, "Source", "Edge4")
        self.assertEqual(self.state(), before)
        path = os.path.join(self.tmp.name, "tangent.FCStd")
        self.doc.saveAs(path)
        App.closeDocument(self.doc.Name)
        self.doc = App.openDocument(path)
        self.sketch = self.doc.getObject("Sketch")
        self.doc.recompute()
        self.assertEqual(self.ids(), ids)
        self.assertEqual(self.constraints(), constraints)
        self.assertAlmostEqual(self.sketch.Geometry[0].Center.y, 6, places=7)

    def test_nonidentity_placement(self):
        placement = App.Placement(App.Vector(7, 3, 4), App.Rotation(App.Vector(0, 1, 0), 30))
        self.source.Placement = placement
        self.sketch.Placement = placement
        self.doc.recompute()
        ids = self.ids()
        self.sketch.reattachExternal(-3, "Source", "Edge3")
        for _ in range(2):
            self.sketch.touch()
            self.doc.recompute()
            self.assertEqual(self.ids(), ids)
            self.assertAlmostEqual(self.sketch.ExternalGeo[2].Center.x, 40, places=9)
            self.assertAlmostEqual(self.sketch.ExternalGeo[2].Center.y, 0, places=9)

    def test_mode_rejection_precedes_noop_and_mixed_neighbour_is_preserved(self):
        for mode in (1, 2):
            self.sketch.ExternalTypes = [mode, 0]
            before = self.state()
            with self.assertRaises(ValueError):
                self.sketch.reattachExternal(-3, "Source", "Edge1")
            self.assertEqual(self.state(), before)
        self.sketch.ExternalTypes = [0, 2]
        self.doc.recompute()
        ids = self.ids()
        self.doc.openTransaction("mixed modes")
        self.sketch.reattachExternal(-3, "Source", "Edge3")
        self.assertEqual(self.sketch.ExternalTypes, [0, 2])
        self.assertEqual(self.ids(), ids)
        self.doc.commitTransaction()
        self.doc.undo()
        self.assertEqual(self.sketch.ExternalTypes, [0, 2])
        self.doc.redo()
        self.doc.recompute()
        self.assertEqual(self.sketch.ExternalTypes, [0, 2])
        self.assertEqual(self.ids(), ids)

    def test_document_observer_noop_and_rejection_have_no_events(self):
        events = []

        class Observer:
            def slotChangedObject(observer, obj, prop):
                if obj == self.sketch:
                    events.append(prop)

        observer = Observer()
        App.addDocumentObserver(observer)
        try:
            self.sketch.reattachExternal(-3, "Source", "Edge1")
            with self.assertRaises(ValueError):
                self.sketch.reattachExternal(-3, "Source", "Edge2")
            self.assertEqual(events, [])
            self.sketch.reattachExternal(-3, "Source", "Edge3")
            self.assertIn("ExternalGeometry", events)
            self.assertIn("ExternalGeo", events)
        finally:
            App.removeDocumentObserver(observer)

    def test_unrelated_vertex_reference_does_not_block_edge_replacement(self):
        self.sketch.delExternal(1)
        self.sketch.addExternal("Source", "Vertex1")
        ids = self.ids()
        point = self.sketch.ExternalGeo[3].X
        self.sketch.reattachExternal(-3, "Source", "Edge3")
        self.assertEqual(self.ids(), ids)
        self.assertEqual(self.sketch.ExternalGeo[3].X, point)
        self.assertAlmostEqual(self.sketch.ExternalGeo[2].Center.x, 40, places=9)

    def test_arc_dependent_start_end_roles_through_lifecycle(self):
        self.source.Shape = self.circles(arcs=True)
        self.doc.recompute()
        arc = self.sketch.ExternalGeo[2]
        i = self.sketch.addGeometry(Part.LineSegment(arc.StartPoint, arc.EndPoint), False)
        self.sketch.addConstraint(Sketcher.Constraint("Coincident", i, 1, -3, 1))
        self.sketch.addConstraint(Sketcher.Constraint("Coincident", i, 2, -3, 2))
        self.doc.recompute()
        ids, constraints = self.ids(), self.constraints()
        self.doc.openTransaction("endpoint replacement")
        self.sketch.reattachExternal(-3, "Source", "Edge3")
        self.doc.recompute()
        self.doc.commitTransaction()
        for replay in (None, self.doc.undo, self.doc.redo):
            if replay:
                replay()
            for _ in range(2):
                self.sketch.touch()
                self.doc.recompute()
                self.assertLess(
                    (
                        self.sketch.Geometry[0].StartPoint - self.sketch.ExternalGeo[2].StartPoint
                    ).Length,
                    1e-7,
                )
                self.assertLess(
                    (self.sketch.Geometry[0].EndPoint - self.sketch.ExternalGeo[2].EndPoint).Length,
                    1e-7,
                )
                self.assertEqual(self.ids(), ids)
                self.assertEqual(self.constraints(), constraints)
        path = os.path.join(self.tmp.name, "endpoints.FCStd")
        self.doc.saveAs(path)
        App.closeDocument(self.doc.Name)
        self.doc = App.openDocument(path)
        self.sketch = self.doc.getObject("Sketch")
        self.doc.recompute()
        self.assertLess(
            (self.sketch.Geometry[0].StartPoint - self.sketch.ExternalGeo[2].StartPoint).Length,
            1e-7,
        )
        self.assertLess(
            (self.sketch.Geometry[0].EndPoint - self.sketch.ExternalGeo[2].EndPoint).Length, 1e-7
        )
        self.assertEqual(self.constraints(), constraints)

    def test_cross_object_dependency_replay_abort_and_reopen(self):
        new = self.doc.addObject("Part::Feature", "Replacement")
        new.Shape = self.circles(offset=10)
        other = self.doc.addObject("Part::Feature", "Unrelated")
        other.Shape = self.circles(offset=20)
        for retain_old in (True, False):
            if not retain_old:
                self.sketch.reattachExternal(-4, other.Name, "Edge1")
            for abort in (True, False):
                self.doc.openTransaction("cross-object")
                self.sketch.reattachExternal(-3, new.Name, "Edge3")
                self.assertIn(self.sketch, new.InList)
                self.assertEqual(self.source in self.sketch.OutList, retain_old)
                if abort:
                    self.doc.abortTransaction()
                else:
                    self.doc.commitTransaction()
                    self.doc.undo()
                self.assertNotIn(self.sketch, new.InList)
                self.assertIn(self.source, self.sketch.OutList)
                self.assertIn(self.sketch, self.source.InList)
                if not abort:
                    self.doc.redo()
                    self.assertIn(self.sketch, new.InList)
                    self.assertEqual(self.source in self.sketch.OutList, retain_old)
                    self.sketch.reattachExternal(-3, "Source", "Edge1")
        self.sketch.reattachExternal(-3, new.Name, "Edge3")
        path = os.path.join(self.tmp.name, "dependencies.FCStd")
        self.doc.saveAs(path)
        App.closeDocument(self.doc.Name)
        self.doc = App.openDocument(path)
        self.sketch = self.doc.getObject("Sketch")
        self.doc.recompute()
        self.assertNotIn(self.doc.getObject("Source"), self.sketch.OutList)
        self.assertIn(self.sketch, self.doc.getObject("Replacement").InList)
        self.assertIn(self.doc.getObject("Unrelated"), self.sketch.OutList)

    def test_single_circle_coincidence(self):
        i = self.sketch.addGeometry(Part.Circle(App.Vector(), App.Vector(0, 0, 1), 1), False)
        self.sketch.addConstraint(Sketcher.Constraint("Radius", i, 1.0))
        self.sketch.addConstraint(Sketcher.Constraint("Coincident", i, 3, -3, 3))
        self.sketch.setExpression("Constraints[0]", "1 mm")
        self.doc.recompute()
        self.sketch.reattachExternal(-3, "Source", "Edge3")
        self.assertEqual(self.sketch.solve(), 0)
        self.assertAlmostEqual(self.sketch.Geometry[0].Center.x, 40, places=7)

    def test_original_checkpoint_and_legacy_add_delete(self):
        ids = self.ids()
        path = os.path.join(self.tmp.name, "original.FCStd")
        self.doc.saveAs(path)
        self.sketch.reattachExternal(-3, "Source", "Edge3")
        self.doc.restore()
        self.sketch = self.doc.getObject("Sketch")
        self.doc.recompute()
        self.assertEqual(self.ids(), ids)
        self.assertAlmostEqual(self.sketch.ExternalGeo[2].Center.x, 0, places=9)
        self.sketch.addExternal("Source", "Edge3")
        self.assertEqual(len(self.sketch.ExternalGeo), 5)
        self.sketch.delExternal(2)
        self.assertEqual(len(self.sketch.ExternalGeo), 4)
