# SPDX-License-Identifier: LGPL-2.1-or-later
"""A late provenance failure must retain its exact module difference, not pass."""

import json
import unittest
from ImagePerformanceProbe import RuntimeReceiptError, verify_runtime_modules


class RuntimeFailureReceiptTest(unittest.TestCase):
    def test_matching_receipt_passes(self):
        verify_runtime_modules({"lib/libFreeCADGui.so": "same"}, {"lib/libFreeCADGui.so": "same"})

    def test_missing_extra_and_changed_files_are_retained_without_acceptance(self):
        expected = {"lib/libFreeCADGui.so": "old", "Mod/Part/Part.so": "part"}
        observed = {"lib/libFreeCADGui.so": "new", "Mod/TechDraw/TechDraw.so": "draw"}
        with self.assertRaises(RuntimeReceiptError) as caught:
            verify_runtime_modules(observed, expected)
        receipt = caught.exception.receipt
        self.assertEqual(receipt["additional_modules"], ["Mod/TechDraw/TechDraw.so"])
        self.assertEqual(receipt["missing_modules"], ["Mod/Part/Part.so"])
        self.assertEqual(receipt["changed_modules"], ["lib/libFreeCADGui.so"])
        self.assertEqual(json.loads(json.dumps(receipt))["observed_module_sha256"], observed)


if __name__ == "__main__":
    unittest.main()
