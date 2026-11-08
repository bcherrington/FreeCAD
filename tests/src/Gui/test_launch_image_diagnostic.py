# SPDX-License-Identifier: LGPL-2.1-or-later
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import launch_image_diagnostic as launcher
from run_image_diagnostic import digest


class LaunchTest(unittest.TestCase):
    def setUp(self):
        self.original_guard = launcher.require_isolated_host
        self.guard_patch = patch.object(launcher, "require_isolated_host")
        self.guard_patch.start()
        self.addCleanup(self.guard_patch.stop)

    def fixture(self, directory, baseline=30):
        root = Path(directory)
        controller = root / "controller.py"
        controller.write_text("# frozen controller")
        arms = {arm: {"build": str(root / arm)} for arm in ("baseline", "change")}
        native = []
        for arm in arms:
            path = Path(arms[arm]["build"]) / "bin/FreeCAD"
            path.parent.mkdir(parents=True)
            path.write_bytes(arm.encode())
            native.append(str(path))
        bindings = {str(Path(launcher.__file__).resolve()): digest(Path(launcher.__file__))}
        bindings.update({name: digest(Path(name)) for name in native})
        freeze = {
            "acceptance_eligible": False,
            "artifact_directory": str(root),
            "file_bindings": bindings,
            "arms": arms,
            "controller_sha256": digest(controller),
            "telemetry": {
                "python": sys.executable,
                "controller": str(controller),
                "native_executables": native,
                "output": str(root / "diagnostic-host-trace.jsonl"),
                "baseline_seconds": baseline,
                "timeout_seconds": 1200,
                "interval_seconds": 1,
            },
        }
        path = root / "freeze.json"
        path.write_text(json.dumps(freeze))
        return path, freeze

    def test_launcher_uses_exact_frozen_telemetry_parameters(self):
        original = os.umask(0o077)
        try:
            with tempfile.TemporaryDirectory() as directory:
                path, freeze = self.fixture(directory)
                with patch.object(launcher.telemetry, "run", return_value=0) as run, patch.dict(
                    os.environ
                ):
                    self.assertEqual(launcher.launch(path, digest(path)), 0)
                    args, kwargs = run.call_args
                    self.assertEqual(
                        args[0],
                        [
                            sys.executable,
                            "-I",
                            "-S",
                            "-B",
                            freeze["telemetry"]["controller"],
                            str(path.resolve()),
                            "--sha256",
                            digest(path),
                        ],
                    )
                    self.assertEqual(args[1], Path(freeze["telemetry"]["output"]))
                    self.assertEqual(
                        args[2], [Path(p) for p in freeze["telemetry"]["native_executables"]]
                    )
                    self.assertEqual(kwargs, {"baseline_seconds": 30, "timeout_seconds": 1200})
                self.assertEqual(
                    (Path(directory) / "diagnostic-launch.json").stat().st_mode & 0o777, 0o600
                )
        finally:
            os.umask(original)

    def test_changed_baseline_cannot_skip_host_observation(self):
        original = os.umask(0o077)
        try:
            with tempfile.TemporaryDirectory() as directory:
                path, _ = self.fixture(directory, baseline=0)
                with patch.object(launcher.telemetry, "run") as run:
                    with self.assertRaisesRegex(ValueError, "limits changed"):
                        launcher.launch(path, digest(path))
                    run.assert_not_called()
        finally:
            os.umask(original)

    def test_missing_pidfd_api_is_rejected_before_creating_session(self):
        original = os.umask(0o077)
        try:
            with tempfile.TemporaryDirectory() as directory:
                path, _ = self.fixture(directory)
                with patch.object(launcher.os, "pidfd_open", None), patch.object(
                    launcher.telemetry, "run"
                ) as run:
                    with self.assertRaisesRegex(ValueError, "lacks Linux pidfd"):
                        launcher.launch(path, digest(path))
                    run.assert_not_called()
                self.assertFalse((Path(directory) / "diagnostic-launch.json").exists())
        finally:
            os.umask(original)

    def test_site_enabled_host_is_rejected(self):
        from types import SimpleNamespace

        with patch.object(
            launcher.sys, "flags", SimpleNamespace(no_site=0, isolated=0, dont_write_bytecode=0)
        ):
            with self.assertRaisesRegex(ValueError, "isolated host"):
                self.original_guard()


if __name__ == "__main__":
    unittest.main()
