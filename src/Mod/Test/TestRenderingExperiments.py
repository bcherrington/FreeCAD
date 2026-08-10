# SPDX-License-Identifier: LGPL-2.1-or-later

"""Focused contracts for the session-only rendering experiments panel."""

from contextlib import suppress
import tempfile
import unittest
import xml.etree.ElementTree as ElementTree
import zipfile

import FreeCAD
import FreeCADGui

try:
    from PySide6 import QtCore, QtTest, QtWidgets
except ImportError:
    from PySide import QtCore  # type: ignore
    from PySide import QtTest  # type: ignore
    from PySide import QtGui as QtWidgets  # type: ignore


class TestRenderingExperiments(unittest.TestCase):
    def setUp(self):
        FreeCADGui.activateWorkbench("PartDesignWorkbench")
        self._doc_names = []
        self.doc, self.gui_doc, self.feature, self.view_provider = self._create_render_doc(
            "TestRenderingExperiments"
        )
        self.view = self.gui_doc.ActiveView
        self.native_deviation = self.view_provider.Deviation
        self.native_angular_deflection = self.view_provider.AngularDeflection
        self.native_materials = self._material_state()

        FreeCADGui.runCommand("Std_RenderingExperiments")
        self._flush_gui()
        self.dock = FreeCADGui.getMainWindow().findChild(
            QtWidgets.QDockWidget, "Rendering Experiments"
        )
        self.assertIsNotNone(self.dock)
        self.assertEqual(self.dock.windowTitle(), "Rendering Experiments")

    def tearDown(self):
        with suppress(Exception):
            self._button("RenderingExperimentsReset").click()
            self.dock.deleteLater()
            self._flush_gui()
        for doc_name in reversed(self._doc_names):
            if FreeCAD.getDocument(doc_name):
                FreeCAD.closeDocument(doc_name)

    def test_manual_tessellation_reset_and_hide_restore_native_values(self):
        self._checkbox("RenderingExperimentsEnabled").setChecked(True)
        self._checkbox("RenderingExperimentsMaterialEnabled").setChecked(True)
        self._checkbox("RenderingExperimentsTessellationEnabled").setChecked(True)
        self._combo("RenderingExperimentsTessellationProfile").setCurrentIndex(2)
        self._flush_gui()

        self.assertAlmostEqual(self.view_provider.Deviation, 0.08)
        self.assertAlmostEqual(self.view_provider.AngularDeflection, 6.0)
        experimental_materials = self._material_state()
        self.assertEqual(
            [item[:2] for item in experimental_materials],
            [item[:2] for item in self.native_materials],
        )
        self.assertNotEqual(experimental_materials, self.native_materials)

        with tempfile.TemporaryDirectory() as directory:
            path = f"{directory}/native-state.FCStd"
            self.doc.saveCopy(path)
            self._flush_gui()
            self.assertAlmostEqual(self.view_provider.Deviation, 0.08)
            self.assertAlmostEqual(self._saved_deviation(path), self.native_deviation)

            saved_document = FreeCAD.openDocument(path)
            self._doc_names.append(saved_document.Name)
            saved_feature = saved_document.getObject("RenderedFeature")
            self._assert_material_states_almost_equal(
                self._material_state(saved_feature.ViewObject), self.native_materials
            )
            FreeCADGui.setActiveDocument(self.doc.Name)
            self._flush_gui()
            self._doc_names.remove(saved_document.Name)
            FreeCAD.closeDocument(saved_document.Name)
            self.assertAlmostEqual(self.view_provider.Deviation, 0.08)

        native_button = self._button("RenderingExperimentsShowNative")
        native_button.pressed.emit()
        self._flush_gui()
        self.assertEqual(self.view_provider.Deviation, self.native_deviation)
        native_button.released.emit()
        self._flush_gui()
        self.assertAlmostEqual(self.view_provider.Deviation, 0.08)

        other_document = FreeCAD.newDocument("RenderingExperimentsOtherDocument")
        FreeCADGui.setActiveDocument(other_document.Name)
        self._flush_gui()
        self.assertEqual(self.view_provider.Deviation, self.native_deviation)
        FreeCADGui.setActiveDocument(self.doc.Name)
        self._flush_gui()
        self.assertAlmostEqual(self.view_provider.Deviation, 0.08)
        FreeCAD.closeDocument(other_document.Name)

        self._button("RenderingExperimentsReset").click()
        self._flush_gui()

        self.assertEqual(self.view_provider.Deviation, self.native_deviation)
        self.assertEqual(self.view_provider.AngularDeflection, self.native_angular_deflection)

        FreeCADGui.runCommand("Std_ViewCreate")
        self._flush_gui()
        FreeCADGui.runCommand("Std_RenderingExperiments")
        self._flush_gui()
        docks = FreeCADGui.getMainWindow().findChildren(
            QtWidgets.QDockWidget, "Rendering Experiments"
        )
        self.assertEqual(len(docks), 1)
        self.assertEqual(self._material_state(), self.native_materials)
        self.assertFalse(self._checkbox("RenderingExperimentsEnabled").isChecked())

        self._checkbox("RenderingExperimentsEnabled").setChecked(True)
        self._checkbox("RenderingExperimentsTessellationEnabled").setChecked(True)
        self._combo("RenderingExperimentsTessellationProfile").setCurrentIndex(1)
        self._flush_gui()
        self.assertAlmostEqual(self.view_provider.Deviation, 0.5)

        self.dock.close()
        self._flush_gui()

        self.assertEqual(self.view_provider.Deviation, self.native_deviation)
        self.assertEqual(self.view_provider.AngularDeflection, self.native_angular_deflection)

    def test_panel_retargets_and_restores_previous_document(self):
        self._checkbox("RenderingExperimentsEnabled").setChecked(True)
        self._checkbox("RenderingExperimentsTessellationEnabled").setChecked(True)
        self._combo("RenderingExperimentsTessellationProfile").setCurrentIndex(2)
        self._flush_gui()

        self.assertAlmostEqual(self.view_provider.Deviation, 0.08)

        second_doc, second_gui_doc, _, second_view_provider = self._create_render_doc(
            "TestRenderingExperimentsSecond"
        )
        second_native_deviation = second_view_provider.Deviation

        FreeCADGui.ActiveDocument = second_gui_doc
        FreeCADGui.setActiveDocument(second_doc.Name)
        FreeCADGui.runCommand("Std_RenderingExperiments")
        self._flush_gui()

        dock = FreeCADGui.getMainWindow().findChild(QtWidgets.QDockWidget, "Rendering Experiments")
        self.assertIs(dock, self.dock)
        self.assertEqual(self.view_provider.Deviation, self.native_deviation)
        self.assertEqual(second_view_provider.Deviation, second_native_deviation)

        self._checkbox("RenderingExperimentsEnabled").setChecked(True)
        self._checkbox("RenderingExperimentsTessellationEnabled").setChecked(True)
        self._combo("RenderingExperimentsTessellationProfile").setCurrentIndex(1)
        self._flush_gui()

        self.assertEqual(self.view_provider.Deviation, self.native_deviation)
        self.assertAlmostEqual(second_view_provider.Deviation, 0.5)

    def test_continuous_controls_are_debounced_and_domain_isolated(self):
        self._checkbox("RenderingExperimentsEnabled").setChecked(True)
        self._checkbox("RenderingExperimentsMaterialEnabled").setChecked(True)
        self._flush_gui()

        timer = self.dock.findChild(QtCore.QTimer, "RenderingExperimentsApplyTimer")
        self.assertIsNotNone(timer)
        specular = self._slider("RenderingExperimentsSpecularStrength")
        material_before_drag = self._material_state()

        for value in (20, 35, 50, 65):
            specular.setValue(value)

        self.assertTrue(timer.isActive())
        self.assertEqual(self._material_state(), material_before_drag)
        QtTest.QTest.qWait(100)
        self._flush_gui()
        self.assertFalse(timer.isActive())
        material_after_drag = self._material_state()
        self.assertNotEqual(material_after_drag, material_before_drag)

        lighting = self._slider("RenderingExperimentsLightingIntensity")
        for value in (70, 85, 100, 115):
            lighting.setValue(value)

        self.assertTrue(timer.isActive())
        QtTest.QTest.qWait(100)
        self._flush_gui()
        self.assertFalse(timer.isActive())
        self.assertEqual(self._material_state(), material_after_drag)

        specular.setValue(80)
        self.assertTrue(timer.isActive())
        self._checkbox("RenderingExperimentsMaterialEnabled").setChecked(False)
        self.assertFalse(timer.isActive())
        self.assertEqual(self._material_state(), self.native_materials)

    def _checkbox(self, name):
        widget = self.dock.findChild(QtWidgets.QCheckBox, name)
        self.assertIsNotNone(widget)
        return widget

    def _button(self, name):
        widget = self.dock.findChild(QtWidgets.QPushButton, name)
        self.assertIsNotNone(widget)
        return widget

    def _combo(self, name):
        widget = self.dock.findChild(QtWidgets.QComboBox, name)
        self.assertIsNotNone(widget)
        return widget

    def _slider(self, name):
        widget = self.dock.findChild(QtWidgets.QSlider, name)
        self.assertIsNotNone(widget)
        return widget

    def _material_state(self, view_provider=None):
        view_provider = view_provider or self.view_provider
        return [
            (
                material.DiffuseColor,
                material.Transparency,
                material.SpecularColor,
                material.Shininess,
            )
            for material in view_provider.ShapeAppearance
        ]

    def _assert_material_states_almost_equal(self, actual, expected):
        self.assertEqual(len(actual), len(expected))
        for actual_material, expected_material in zip(actual, expected):
            for actual_color, expected_color in zip(actual_material[0], expected_material[0]):
                self.assertAlmostEqual(actual_color, expected_color, places=5)
            self.assertAlmostEqual(actual_material[1], expected_material[1], places=5)
            for actual_color, expected_color in zip(actual_material[2], expected_material[2]):
                self.assertAlmostEqual(actual_color, expected_color, places=5)
            self.assertAlmostEqual(actual_material[3], expected_material[3], places=5)

    @staticmethod
    def _saved_deviation(path):
        with zipfile.ZipFile(path) as archive:
            root = ElementTree.fromstring(archive.read("GuiDocument.xml"))
        value = root.find(".//Property[@name='Deviation']/Float")
        if value is None:
            raise AssertionError("Saved GUI document did not contain Deviation")
        return float(value.attrib["value"])

    def _create_render_doc(self, name):
        doc = FreeCAD.newDocument(name)
        self._doc_names.append(doc.Name)
        gui_doc = FreeCADGui.getDocument(doc.Name)
        FreeCADGui.ActiveDocument = gui_doc
        FreeCADGui.setActiveDocument(doc.Name)

        feature = doc.addObject("Part::Box", "RenderedFeature")
        feature.Length = 12
        feature.Width = 8
        feature.Height = 5
        doc.recompute()

        view = gui_doc.ActiveView
        view.viewAxonometric()
        view.fitAll()
        return doc, gui_doc, feature, feature.ViewObject

    @staticmethod
    def _flush_gui():
        for _ in range(3):
            FreeCADGui.updateGui()
            QtWidgets.QApplication.processEvents()
