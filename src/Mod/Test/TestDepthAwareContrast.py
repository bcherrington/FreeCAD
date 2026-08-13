# SPDX-License-Identifier: LGPL-2.1-or-later

"""Focused GUI contract tests for depth-aware viewport contrast."""

from contextlib import suppress
import time
import unittest

import FreeCAD
import FreeCADGui
import Part  # noqa: F401

try:
    from PySide6 import QtWidgets
except ImportError:
    from PySide import QtGui as QtWidgets  # type: ignore


class TestDepthAwareContrast(unittest.TestCase):
    def setUp(self):
        self.params = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/View")
        contents = self.params.GetContents() or []
        parameter_names = {entry[1] for entry in contents}
        self._had_enabled = "DepthAwareContrast" in parameter_names
        self._had_strength = "DepthAwareContrastStrength" in parameter_names
        self._had_anti_aliasing = "AntiAliasing" in parameter_names
        self._enabled = self.params.GetBool("DepthAwareContrast", False)
        self._strength = self.params.GetInt("DepthAwareContrastStrength", 15)
        self._anti_aliasing = self.params.GetInt("AntiAliasing", 0)

        self.doc = FreeCAD.newDocument("TestDepthAwareContrast")
        FreeCADGui.ActiveDocument = FreeCADGui.getDocument(self.doc.Name)
        self.box = self.doc.addObject("Part::Box", "Box")
        self.box.Length = 20
        self.box.Width = 15
        self.box.Height = 10
        self.box.ViewObject.ShapeColor = (0.72, 0.78, 0.86)
        self.doc.recompute()
        self.view = FreeCADGui.ActiveDocument.ActiveView
        self.viewer = self.view.getViewer()
        self.graphics_view = self.view.graphicsView()
        self._had_navi_cube = self.viewer.isEnabledNaviCube()
        self.view.viewAxonometric()
        self.view.fitAll()

    def tearDown(self):
        self._restore_parameter("DepthAwareContrast", self._had_enabled, self._enabled, "Bool")
        self._restore_parameter(
            "DepthAwareContrastStrength", self._had_strength, self._strength, "Int"
        )
        self._restore_parameter("AntiAliasing", self._had_anti_aliasing, self._anti_aliasing, "Int")
        FreeCADGui.Selection.clearSelection()
        with suppress(Exception):
            self.viewer.setEnabledNaviCube(self._had_navi_cube)
        if FreeCAD.getDocument(self.doc.Name):
            FreeCAD.closeDocument(self.doc.Name)

    def test_default_contract_is_opt_in_with_fifteen_percent_strength(self):
        self.params.RemBool("DepthAwareContrast")
        self.params.RemInt("DepthAwareContrastStrength")

        self.assertFalse(self.params.GetBool("DepthAwareContrast", False))
        self.assertEqual(self.params.GetInt("DepthAwareContrastStrength", 15), 15)

    def test_zero_percent_matches_native_and_fifteen_percent_changes_model_pixels(self):
        self.viewer.setEnabledNaviCube(False)
        native = self._capture(False, 15)
        zero = self._capture(True, 0)
        enhanced = self._capture(True, 15)

        self.assertEqual(self._pixel_delta(native, zero), 0)
        changed = self._pixel_delta(native, enhanced)
        self.assertGreater(changed, 0)
        self.assertLess(changed, native.width() * native.height() // 2)

    def test_configured_aa_reuses_resources_and_preserves_navigation_and_selection(self):
        self.params.SetInt("AntiAliasing", 4)  # AntiAliasing::MSAA8x
        self.viewer.setEnabledNaviCube(True)
        FreeCADGui.Selection.addSelection(self.box)

        native = self._capture(False, 15)
        enhanced = self._capture(True, 15)
        first_allocations = int(self.graphics_view.property("depthAwareContrastAllocations"))
        repeated = self._capture(True, 15)

        self.assertEqual(self.graphics_view.property("depthAwareContrastRequestedSamples"), 8)
        self.assertIn(
            self.graphics_view.property("depthAwareContrastActualSamples"),
            (1, 2, 4, 6, 8),
        )
        self.assertTrue(
            str(self.graphics_view.property("depthAwareContrastStatus")).startswith("active-")
        )
        self.assertEqual(
            int(self.graphics_view.property("depthAwareContrastAllocations")), first_allocations
        )
        self.assertEqual(self._pixel_delta(enhanced, repeated), 0)
        self.assertEqual(FreeCADGui.Selection.getSelection(), [self.box])

        cube_size = min(150, native.width(), native.height())
        native_cube = native.copy(native.width() - cube_size, 0, cube_size, cube_size)
        enhanced_cube = enhanced.copy(enhanced.width() - cube_size, 0, cube_size, cube_size)
        self.assertEqual(self._pixel_delta(native_cube, enhanced_cube), 0)

        self._capture(False, 15)
        self.assertEqual(self.graphics_view.property("depthAwareContrastStatus"), "disabled")

    def _capture(self, enabled, strength):
        self.params.SetInt("DepthAwareContrastStrength", strength)
        self.params.SetBool("DepthAwareContrast", enabled)
        for _ in range(4):
            FreeCADGui.updateGui()
            QtWidgets.QApplication.processEvents()
            self.view.redraw()
            time.sleep(0.05)
        return self.viewer.grabFramebuffer()

    @staticmethod
    def _pixel_delta(first, second):
        if first.size() != second.size():
            return -1
        changed = 0
        for y in range(first.height()):
            for x in range(first.width()):
                if first.pixel(x, y) != second.pixel(x, y):
                    changed += 1
        return changed

    def _restore_parameter(self, name, existed, value, kind):
        if existed:
            getattr(self.params, f"Set{kind}")(name, value)
        else:
            getattr(self.params, f"Rem{kind}")(name)
