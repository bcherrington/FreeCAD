# SPDX-License-Identifier: LGPL-2.1-or-later
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location(
    "fidelity", Path(__file__).with_name("document_performance_fidelity.py")
)
fidelity = importlib.util.module_from_spec(spec)
spec.loader.exec_module(fidelity)


HEADER = "\nCASCADE Topology V1, (c) Matra-Datavision\n"


class BrepFidelityTest(unittest.TestCase):
    def brep(self, section, line):
        return HEADER + section + " 1\n" + line + "\nTShapes 0\n"

    def test_known_basis_normalization_and_signed_zero(self):
        for section, line, index in [
            ("Curves", "1 2 3 4 1 0 0", 5),
            ("Curves", "3 2 3 4 1 0 0 0 1 0 0 0 1 7 6", 10),
            ("Surfaces", "1 2 3 4 1 0 0 0 1 0 0 0 1", 10),
        ]:
            with self.subTest(section=section, line=line):
                a = line.split()
                b = list(a)
                b[index] = "1e-16"
                self.assertTrue(
                    fidelity.equivalent_brep(
                        self.brep(section, line), self.brep(section, " ".join(b))
                    )
                )
                b[index] = "1e-12"
                self.assertFalse(
                    fidelity.equivalent_brep(
                        self.brep(section, line), self.brep(section, " ".join(b))
                    )
                )
        self.assertTrue(
            fidelity.equivalent_brep(
                self.brep("Curves", "1 2 3 4 1 -0 0"), self.brep("Curves", "1 2 3 4 1 0 -0")
            )
        )

    def test_every_protected_slot_and_unknown_grammar_fail_closed(self):
        for section, line, permitted in [
            ("Curves", "1 2 3 4 1 0 0", set(range(4, 7))),
            ("Curves", "3 2 3 4 1 0 0 0 1 0 0 0 1 7 6", set(range(4, 13))),
            ("Surfaces", "1 2 3 4 1 0 0 0 1 0 0 0 1", set(range(4, 13))),
            ("Curves", "2 2 3 4 1 0 0", set()),
            ("Curves", "1 2 3 4 1 0 0 9", set()),
        ]:
            tokens = line.split()
            for i in range(len(tokens)):
                if i in permitted:
                    continue
                with self.subTest(section=section, line=line, protected=i):
                    changed = list(tokens)
                    changed[i] = str(float(tokens[i]) + 1e-14)
                    self.assertFalse(
                        fidelity.equivalent_brep(
                            self.brep(section, line), self.brep(section, " ".join(changed))
                        )
                    )
        self.assertFalse(
            fidelity.equivalent_brep(HEADER + "TShapes 1\nFa 0 0\n", HEADER + "TShapes 2\nFa 0 0\n")
        )
        self.assertFalse(
            fidelity.equivalent_brep(HEADER + "TShapes 1\nFa 0 0\n", HEADER + "TShapes 1\nEd 0 0\n")
        )

    def test_location_rotation_only_and_frame_integrity(self):
        before = self.brep("Locations", "1\n1 0 0 8\n0 1 0 9\n0 0 1 10")
        after = self.brep("Locations", "1\n1 1e-16 0 8\n0 1 0 9\n0 0 1 10")
        self.assertTrue(fidelity.equivalent_brep(before, after))
        for line in [
            "1\n1 0 0 8.00000000000001\n0 1 0 9\n0 0 1 10",
            "1\n1 0 0 8\n0 1 0 9\n0 0 -1 10",
            "1\n1 0 0 8\n0 1 0 9",
            "2\n1 1e-16 0 8\n0 1 0 9\n0 0 1 10",
        ]:
            self.assertFalse(fidelity.equivalent_brep(before, self.brep("Locations", line)))
        self.assertFalse(
            fidelity.basis_valid(
                [[1, 0, 0], [1, 0, 0], [0, 0, 1]], [[1, 0, 0], [1, 0, 0], [0, 0, 1]]
            )
        )
        self.assertFalse(
            fidelity.basis_valid(
                [[1, 0, 0], [0, 1, 0], [0, 0, 1]], [[1, 0, 0], [0, 1, 0], [0, 0, -1]]
            )
        )

    def test_nonfinite_structure_and_version_rejected(self):
        original = self.brep("Curves", "1 2 3 4 1 0 0")
        for value in ["nan", "inf", "-Infinity", "1e999"]:
            self.assertFalse(
                fidelity.equivalent_brep(original, self.brep("Curves", "1 2 3 4 1 " + value + " 0"))
            )
            self.assertFalse(
                fidelity.equivalent_brep(
                    self.brep("Curves", "1 2 3 4 1 " + value + " 0"),
                    self.brep("Curves", "1 2 3 4 1 " + value + " 0"),
                )
            )
        self.assertFalse(fidelity.equivalent_brep(original, original.replace("V1", "V2")))
        self.assertFalse(fidelity.equivalent_brep(original, original + "\n"))

    def test_protected_numeric_collisions_remain_unequal(self):
        for before, after in [
            ("9007199254740992", "9007199254740993"),
            ("1.000000000000000000001", "1.000000000000000000002"),
        ]:
            self.assertFalse(
                fidelity.equivalent_brep(
                    self.brep("Curves", "1 " + before + " 3 4 1 0 0"),
                    self.brep("Curves", "1 " + after + " 3 4 1 0 0"),
                )
            )

    def test_secondary_invariants_never_override_structure(self):
        a = {
            "type": "Solid",
            "null": False,
            "valid": True,
            "topology": {"Faces": 6},
            "geometry": {"volume_mm3": [1.0]},
        }
        b = {**a, "geometry": {"volume_mm3": [1.0 + 1e-10]}}
        self.assertTrue(fidelity.equivalent_invariants(a, b))
        for changed in [
            {**a, "topology": {"Faces": 7}},
            {**a, "valid": False},
            {**a, "geometry": {"volume_mm3": [1.01]}},
            {**a, "geometry": {"volume_mm3": [float("nan")]}},
        ]:
            self.assertFalse(fidelity.equivalent_invariants(a, changed))


class ShapeApplicabilityTest(unittest.TestCase):
    def test_null_shape_never_reads_unavailable_attributes(self):
        class Null:
            def isNull(self):
                return True

        self.assertEqual(fidelity.shape_invariants(Null()), {"null": True})

    def test_null_geometry_end_to_end_requires_no_export(self):
        from types import SimpleNamespace

        class Null:
            def isNull(self):
                return True

            def exportBrepToString(self):
                raise AssertionError("Null should not export")

        class Object:
            Name = "Object"
            PropertiesList = ["Shape"]
            Shape = Null()

            def getTypeIdOfProperty(self, name):
                return "Part::PropertyPartShape"

        state = fidelity.geometric_state(SimpleNamespace(Objects=[Object()]), [])
        self.assertTrue(fidelity.equivalent_geometry(state, state))

    def test_compound_does_not_require_unavailable_center(self):
        from types import SimpleNamespace

        class Compound:
            ShapeType = "Compound"
            Vertexes = Edges = Wires = Faces = Shells = Solids = CompSolids = Compounds = []
            Length = Area = Volume = 0.0
            BoundBox = SimpleNamespace(XMin=0.0, YMin=0.0, ZMin=0.0, XMax=0.0, YMax=0.0, ZMax=0.0)

            def isNull(self):
                return False

            def isValid(self):
                return True

        state = fidelity.shape_invariants(Compound())
        self.assertFalse(state["center_applicable"])
        self.assertNotIn("center_mm", state["geometry"])


class ExactGeometryManifestTest(unittest.TestCase):
    def test_additional_shape_value_changes_identity_even_when_main_shape_matches(self):
        from types import SimpleNamespace

        class Shape:
            def __init__(self, text):
                self.text = text

            def isNull(self):
                return self.text is None

            def exportBrepToString(self):
                if self.text is None:
                    raise AssertionError("Null must not export")
                return self.text

        class Object:
            Name = "Object"
            PropertiesList = ["Shape", "AddSubShape", "InternalShape"]

            def getTypeIdOfProperty(self, name):
                return "Part::PropertyPartShape"

        obj = Object()
        obj.Shape = Shape("main")
        obj.AddSubShape = Shape("tool-a")
        obj.InternalShape = Shape(None)
        doc = SimpleNamespace(Objects=[obj])
        before = fidelity.geometry_manifest(doc, [])
        self.assertEqual(len(before), 3)
        obj.AddSubShape = Shape("tool-b")
        after = fidelity.geometry_manifest(doc, [])
        self.assertNotEqual(before, after)
        self.assertEqual(
            dict((tuple(k), v) for k, v in before)[("Object", "Shape")],
            dict((tuple(k), v) for k, v in after)[("Object", "Shape")],
        )
        self.assertIsNone(dict((tuple(k), v) for k, v in after)[("Object", "InternalShape")])


class PersistentStateTest(unittest.TestCase):
    def test_decompressed_members_ignore_zip_timestamps_only(self):
        import io, zipfile
        from types import SimpleNamespace

        class Object:
            Name, TypeId, PropertiesList = "Object", "App::Feature", ["Group"]
            timestamp = (2020, 1, 1, 0, 0, 0)
            data = b'<Link value="Target"/>'

            def getTypeOfProperty(self, name):
                return []

            def getPropertyStatus(self, name):
                return []

            def getTypeIdOfProperty(self, name):
                return "App::PropertyLinkList"

            def dumpPropertyContent(self, name, Compression):
                out = io.BytesIO()
                with zipfile.ZipFile(out, "w") as archive:
                    archive.writestr(zipfile.ZipInfo("Persistence.xml", self.timestamp), self.data)
                return out.getvalue()

        obj = Object()
        doc = SimpleNamespace(Objects=[obj])
        before = fidelity.persistent_state(doc)
        obj.timestamp = (2026, 1, 1, 0, 0, 0)
        self.assertEqual(before, fidelity.persistent_state(doc))
        obj.data = b'<Link value="Other"/>'
        self.assertNotEqual(before, fidelity.persistent_state(doc))


if __name__ == "__main__":
    unittest.main()
