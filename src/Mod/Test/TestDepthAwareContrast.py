# SPDX-License-Identifier: LGPL-2.1-or-later

"""Focused GUI contract tests for depth-aware viewport contrast."""

from contextlib import suppress
import os
import tempfile
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
        self._had_navi_cube = False

        self._enabled = self.params.GetBool("DepthAwareContrast", False)
        self._strength = self.params.GetInt("DepthAwareContrastStrength", 15)
        self._anti_aliasing = self.params.GetInt("AntiAliasing", 0)

        self.doc = FreeCAD.newDocument("TestDepthAwareContrast")
        self._primary_doc = self.doc
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
        self._base_view_size = (self.graphics_view.width(), self.graphics_view.height())
        with suppress(Exception):
            self._had_navi_cube = self.viewer.isEnabledNaviCube()
        self.view.viewAxonometric()
        self.view.fitAll()

    def tearDown(self):
        with suppress(Exception):
            FreeCADGui.Selection.clearPreselection()
        with suppress(Exception):
            FreeCADGui.Selection.clearSelection()

        self._restore_parameter("DepthAwareContrast", self._had_enabled, self._enabled, "Bool")
        self._restore_parameter(
            "DepthAwareContrastStrength", self._had_strength, self._strength, "Int"
        )
        self._restore_parameter("AntiAliasing", self._had_anti_aliasing, self._anti_aliasing, "Int")
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
        first_allocations = int(self._read_view_property("depthAwareContrastAllocations"))
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

    def test_aa_off_requests_no_multisampled_path(self):
        self.params.SetInt("AntiAliasing", 0)

        native = self._capture(False, 20)
        enabled = self._capture(True, 20)

        requested_samples = int(self._read_view_property("depthAwareContrastRequestedSamples", 0))
        actual_samples = int(self._read_view_property("depthAwareContrastActualSamples", 0))

        self.assertIn(requested_samples, (0, 1))
        self.assertIn(actual_samples, (0, 1, 2, 4, 6, 8))
        self.assertLessEqual(actual_samples, max(requested_samples, 1))
        self.assertGreater(self._pixel_delta(native, enabled), 0)

    def test_preselection_and_selection_state_survives_feature_toggle(self):
        self.params.SetInt("AntiAliasing", 0)
        self.view.viewTop()
        self.view.fitAll()
        FreeCADGui.Selection.addSelection(self.box)
        selection = self._read_selection_state()
        self.assertIsNotNone(selection)

        _ = self._capture(True, 25)
        self.assertEqual(self._read_selection_state(), selection)
        FreeCADGui.Selection.setPreselection(self.box, "Face1")
        QtWidgets.QApplication.processEvents()
        expected_preselection = (self.doc.Name, self.box.Name, ("Face1",))
        self.assertEqual(self._read_preselection_signature(), expected_preselection)

        _ = self._capture(False, 25)
        self.assertEqual(self._read_selection_state(), selection)
        FreeCADGui.Selection.setPreselection(self.box, "Face1")
        QtWidgets.QApplication.processEvents()
        self.assertEqual(self._read_preselection_signature(), expected_preselection)

    def test_resize_invalidation_and_allocation_reuse(self):
        self.params.SetInt("AntiAliasing", 4)
        base_image = self._capture(True, 15)
        base_alloc = int(self._read_view_property("depthAwareContrastAllocations", 0))
        self.assertGreater(base_alloc, 0)
        base_size = (self.graphics_view.width(), self.graphics_view.height())
        base_image_size = (base_image.width(), base_image.height())

        self.graphics_view.resize(max(64, base_size[0] - 12), max(64, base_size[1] - 12))
        resized = self._capture(True, 15)
        resized_alloc = int(self._read_view_property("depthAwareContrastAllocations", 0))
        self.assertGreater(resized_alloc, 0)
        self.assertNotEqual(
            (resized.width(), resized.height()),
            base_image_size,
            msg="capture image should match the resized graphics view",
        )

        resized_again = self._capture(True, 15)
        resized_again_alloc = int(self._read_view_property("depthAwareContrastAllocations", 0))
        self.assertEqual(
            resized_alloc, resized_again_alloc, "allocation should be reused for unchanged size"
        )

        self.graphics_view.resize(*base_size)
        restored = self._capture(True, 15)
        restored_alloc = int(self._read_view_property("depthAwareContrastAllocations", 0))
        self.assertEqual((restored.width(), restored.height()), base_image_size)
        self.assertNotEqual(restored_alloc, resized_alloc)

    def test_active_view_switching_preserves_contract(self):
        self.params.SetBool("DepthAwareContrast", False)
        self.params.SetInt("AntiAliasing", 2)
        baseline_native = self._capture(False, 15)
        primary_camera = self.view.getCamera()
        self.assertEqual(
            self._read_view_property("depthAwareContrastStatus", "disabled"), "disabled"
        )

        primary_doc = self._primary_doc
        secondary_name = "DepthAwareContrastSecondary"
        secondary_doc = FreeCAD.newDocument(secondary_name)
        secondary_box = secondary_doc.addObject("Part::Box", "SecondaryBox")
        secondary_box.Length = 8
        secondary_box.Width = 8
        secondary_box.Height = 8
        secondary_doc.recompute()

        try:
            self._activate_document(secondary_doc)
            secondary_native = self._capture(False, 15)
            self.assertEqual(
                self._read_view_property("depthAwareContrastStatus", "disabled"), "disabled"
            )

            self.params.SetBool("DepthAwareContrast", True)
            secondary_enabled = self._capture(True, 15)
            self.assertTrue(
                str(self._read_view_property("depthAwareContrastStatus")).startswith("active")
            )

            self._activate_document(primary_doc)
            switched_enabled = self._capture(True, 15)
            self.assertTrue(
                str(self._read_view_property("depthAwareContrastStatus")).startswith("active")
            )
            switched = self._capture(False, 15)
            self.assertEqual(self.view.getCamera(), primary_camera)
            self.assertGreater(switched.width(), 0)
            self.assertNotEqual(self._pixel_delta(switched_enabled, switched), 0)
            self.assertNotEqual(self._pixel_delta(secondary_enabled, baseline_native), 0)
            self.assertNotEqual(self._pixel_delta(secondary_native, switched), 0)
        finally:
            with suppress(Exception):
                self._activate_document(primary_doc)
            with suppress(Exception):
                FreeCAD.closeDocument(secondary_name)

    def test_save_safety_preserves_render_after_reopen(self):
        self.params.SetInt("AntiAliasing", 4)
        self._capture(True, 15)

        with tempfile.TemporaryDirectory(prefix="fc_depth_aware_") as tmpdir:
            path = os.path.join(tmpdir, "depth-aware-test.FCStd")
            self.doc.saveCopy(path)
            self.assertTrue(os.path.exists(path), f"saved doc not found: {path}")

            reopened = FreeCAD.openDocument(path)
            try:
                self.assertIsNotNone(reopened)
                self._activate_document(reopened)
                image = self._capture(False, 15)
                self.assertGreater(image.width(), 0)
                self.assertGreater(image.height(), 0)
            finally:
                with suppress(Exception):
                    FreeCAD.closeDocument(reopened.Name)
                with suppress(Exception):
                    self._activate_document(self._primary_doc)

    def test_reset_and_disable_restore_native_state(self):
        self.params.SetInt("AntiAliasing", 4)
        native = self._capture(False, 15)
        enabled = self._capture(True, 15)
        self.assertIn("active", str(self._read_view_property("depthAwareContrastStatus")))

        self.params.RemBool("DepthAwareContrast")
        self.params.RemInt("DepthAwareContrastStrength")
        self.assertFalse(self.params.GetBool("DepthAwareContrast", False))
        self.assertEqual(self.params.GetInt("DepthAwareContrastStrength", 15), 15)

        restored = self._capture(False, 15)
        self.assertEqual(self._read_view_property("depthAwareContrastStatus"), "disabled")
        self.assertEqual(self._pixel_delta(native, restored), 0)
        self.assertNotEqual(self._pixel_delta(enabled, restored), 0)

    def test_view_teardown_with_active_feature_is_safe(self):
        self.params.SetInt("AntiAliasing", 4)
        secondary = FreeCAD.newDocument("DepthAwareContrastTeardown")
        secondary_name = secondary.Name
        secondary.addObject("Part::Box", "TeardownBox")
        secondary.recompute()
        self._activate_document(secondary)
        self._capture(True, 15)
        FreeCAD.closeDocument(secondary_name)
        self.assertNotIn(secondary_name, FreeCAD.listDocuments())
        self._activate_document(self._primary_doc)
        restored = self._capture(False, 15)
        self.assertGreater(restored.width(), 0)

    def test_idle_has_no_candidate_redraw_or_allocation_loop(self):
        self._capture(True, 15)
        self._wait_for_render_counters_to_settle()
        attempts = int(self._read_view_property("depthAwareContrastRenderAttempts", 0))
        allocations = int(self._read_view_property("depthAwareContrastAllocations", 0))

        deadline = time.monotonic() + 0.5
        while time.monotonic() < deadline:
            QtWidgets.QApplication.processEvents()
            time.sleep(0.01)

        self.assertEqual(
            int(self._read_view_property("depthAwareContrastRenderAttempts", 0)), attempts
        )
        self.assertEqual(
            int(self._read_view_property("depthAwareContrastAllocations", 0)), allocations
        )

    def test_unsupported_context_falls_back_without_allocation_and_restores_native(self):
        if os.environ.get("FREECAD_TEST_EXPECT_DEPTH_CONTRAST_FALLBACK") != "1":
            self.skipTest("requires an intentionally limited OpenGL context")

        native = self._capture(False, 15)
        allocations = int(self._read_view_property("depthAwareContrastAllocations", 0))
        fallback = self._capture(True, 15)

        self.assertEqual(
            self._read_view_property("depthAwareContrastStatus"),
            "fallback-opengl-3.2-required",
        )
        self.assertEqual(
            int(self._read_view_property("depthAwareContrastAllocations", 0)), allocations
        )
        self.assertEqual(int(self._read_view_property("depthAwareContrastActualSamples", -1)), 0)
        self.assertEqual(self._pixel_delta(native, fallback), 0)

        restored = self._capture(False, 15)
        self.assertEqual(self._pixel_delta(native, restored), 0)

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

    def _read_view_property(self, name, fallback=None):
        with suppress(Exception):
            value = self.graphics_view.property(name)
            return fallback if value is None else value
        return fallback

    def _read_preselection_state(self):
        with suppress(Exception):
            return FreeCADGui.Selection.getPreselection()
        return None

    def _read_preselection_signature(self):
        preselection = self._read_preselection_state()
        if preselection is None:
            return None
        return (
            preselection.DocumentName,
            preselection.ObjectName,
            tuple(preselection.SubElementNames),
        )

    def _read_selection_state(self):
        with suppress(Exception):
            return tuple(FreeCADGui.Selection.getSelection())
        return tuple()

    def _wait_for_render_counters_to_settle(self):
        deadline = time.monotonic() + 1.0
        stable_since = time.monotonic()
        last = (
            int(self._read_view_property("depthAwareContrastRenderAttempts", 0)),
            int(self._read_view_property("depthAwareContrastAllocations", 0)),
        )
        while time.monotonic() < deadline:
            QtWidgets.QApplication.processEvents()
            time.sleep(0.01)
            current = (
                int(self._read_view_property("depthAwareContrastRenderAttempts", 0)),
                int(self._read_view_property("depthAwareContrastAllocations", 0)),
            )
            if current != last:
                last = current
                stable_since = time.monotonic()
            elif time.monotonic() - stable_since >= 0.1:
                return
        self.fail("depth-aware contrast render counters did not settle")

    def _activate_document(self, document):
        gui_doc = FreeCADGui.getDocument(document.Name)
        self.assertIsNotNone(gui_doc, f"missing GUI document {document.Name}")
        FreeCADGui.ActiveDocument = gui_doc
        self.view = gui_doc.ActiveView
        self.viewer = self.view.getViewer()
        self.graphics_view = self.view.graphicsView()
        self._base_view_size = (self.graphics_view.width(), self.graphics_view.height())

    def _restore_parameter(self, name, existed, value, kind):
        if existed:
            getattr(self.params, f"Set{kind}")(name, value)
        else:
            getattr(self.params, f"Rem{kind}")(name)
