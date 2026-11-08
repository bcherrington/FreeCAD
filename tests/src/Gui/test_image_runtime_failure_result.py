# SPDX-License-Identifier: LGPL-2.1-or-later
"""Exercise the GUI entrypoint's failure serialization without launching Qt."""

import contextlib
import io
import json
import os
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch
import ImagePerformanceProbe as probe


class RuntimeFailureResultTest(unittest.TestCase):
    def test_main_preserves_receipt_and_failure_exit(self):
        observed = {"Mod/Extra.so": "observed"}
        expected = {"lib/Expected.so": "expected"}
        error = probe.RuntimeReceiptError(observed, expected)
        application = Mock()
        fake_qt = SimpleNamespace(
            QtCore=SimpleNamespace(QTimer=SimpleNamespace(singleShot=lambda _delay, fn: fn())),
            QtWidgets=SimpleNamespace(QApplication=SimpleNamespace(instance=lambda: application)),
        )
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "result.json"
            with patch.dict("sys.modules", {"PySide6": fake_qt}), patch.dict(
                os.environ,
                {"SPEC003_RESULT_FILE": str(output), "SPEC003_TRACE_LIBRARY": "mock-library"},
            ), patch.object(probe, "run", side_effect=error), patch.object(
                probe.ctypes, "CDLL", return_value=Mock()
            ), contextlib.redirect_stderr(
                io.StringIO()
            ):
                probe.main()
            result = json.loads(output.read_text())
            self.assertIs(result["correctness_passed"], False)
            self.assertEqual(result["exception_type"], "RuntimeReceiptError")
            self.assertEqual(result["runtime_failure_receipt"]["observed_module_sha256"], observed)
            application.exit.assert_called_once_with(1)


if __name__ == "__main__":
    unittest.main()
