# SPDX-License-Identifier: LGPL-2.1-or-later
"""Freeze schedule guards must reject changes before any native worker starts."""

import tempfile
import json
from pathlib import Path
import unittest
from unittest.mock import patch
from run_image_diagnostic import validate_schedule, digest, audit


class DiagnosticScheduleTest(unittest.TestCase):
    def schedule(self, directory):
        identities = {
            "hidden": ("gridfinity-drawer-v1.3", "hidden-first-show-repeat", 0),
            "visible": ("gridfinity-drawer-v1.3", "open-only", 1),
            "part": ("PartDesignExample", "open-only", -1),
            "vrml": ("TestVRMLTextures", "open-only", -1),
        }
        self.cells = {}
        for cell, (fixture, scenario, visibility) in identities.items():
            path = Path(directory) / (cell + ".json")
            path.write_text(
                json.dumps(
                    {
                        "fixture_id": fixture,
                        "fixture_sha256": "a" * 64,
                        "scenario": scenario,
                        "initial_image_visibility": visibility,
                    }
                )
            )
            self.cells[cell] = {
                "context": str(path),
                "context_sha256": digest(path),
                "fixture_id": fixture,
                "fixture_sha256": "a" * 64,
            }
        return [
            {
                "cell": cell,
                "arm": arm,
                "result": str(Path(directory) / str(i * 4 + j)),
                "context_sha256": self.cells[cell]["context_sha256"],
            }
            for i, cell in enumerate(identities)
            for j, arm in enumerate(("baseline", "change", "change", "baseline"))
        ]

    def test_exact_schedule_is_accepted(self):
        with tempfile.TemporaryDirectory() as directory:
            validate_schedule(self.schedule(directory), self.cells)

    def test_extra_or_reordered_worker_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            rows = self.schedule(directory)
            with self.assertRaisesRegex(ValueError, "exact fixed"):
                validate_schedule(rows + rows[:1], self.cells)
            rows[0], rows[1] = rows[1], rows[0]
            with self.assertRaisesRegex(ValueError, "exact fixed"):
                validate_schedule(rows, self.cells)

    def test_existing_or_shared_destination_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            rows = self.schedule(directory)
            rows[1]["result"] = rows[0]["result"]
            with self.assertRaisesRegex(ValueError, "overlap"):
                validate_schedule(rows, self.cells)
            rows = self.schedule(directory)
            Path(rows[0]["result"]).write_text("retained")
            with self.assertRaisesRegex(ValueError, "already exists"):
                validate_schedule(rows, self.cells)

    def test_context_cannot_be_mislabeled_as_another_cell(self):
        with tempfile.TemporaryDirectory() as directory:
            rows = self.schedule(directory)
            rows[0]["context_sha256"] = self.cells["visible"]["context_sha256"]
            with self.assertRaisesRegex(ValueError, "cell identity"):
                validate_schedule(rows, self.cells)

    def audit_fixture(self, directory):
        root = Path(directory)
        arms = {}
        for arm in ("baseline", "change"):
            build = root / arm
            for name in ("bin", "lib", "Mod", "Ext", "share", "data"):
                (build / name).mkdir(parents=True)
            arms[arm] = {
                "source": str(build),
                "build": str(build),
                "commit": "known",
                "runtime_inventory": [],
            }
        identity = root / "identity.json"
        identity.write_text(
            json.dumps({"installed_packages": [{"name": "test-package", "version": "1"}]})
        )
        metadata = root / "conda-meta"
        metadata.mkdir()
        (metadata / "package.json").write_text(json.dumps({"name": "test-package", "version": "1"}))
        return {
            "file_bindings": {},
            "arms": arms,
            "dependencies": [],
            "environment_identity": str(identity),
            "conda_meta_directory": str(metadata),
            "initial_caches": {
                arm: {"root": str(root / arm / "cache"), "files": {}}
                for arm in ("baseline", "change")
            },
        }

    def test_unbound_runtime_file_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            freeze = self.audit_fixture(directory)
            (Path(freeze["arms"]["baseline"]["build"]) / "lib/foreign.so").write_bytes(b"injected")
            with self.assertRaisesRegex(ValueError, "inventory changed"):
                audit(freeze)

    def test_live_environment_drift_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            freeze = self.audit_fixture(directory)
            (Path(freeze["conda_meta_directory"]) / "package.json").write_text(
                json.dumps({"name": "test-package", "version": "2"})
            )
            with patch(
                "run_image_diagnostic.subprocess.check_output",
                side_effect=lambda args, **kwargs: "known\n" if "rev-parse" in args else b"",
            ):
                with self.assertRaisesRegex(ValueError, "environment changed"):
                    audit(freeze)

    def test_listed_runtime_file_requires_a_hash_and_rejects_mutation(self):
        with tempfile.TemporaryDirectory() as directory:
            freeze = self.audit_fixture(directory)
            path = Path(freeze["arms"]["baseline"]["build"]) / "lib/listed.so"
            path.write_bytes(b"original")
            freeze["arms"]["baseline"]["runtime_inventory"] = [str(path)]
            with self.assertRaisesRegex(ValueError, "unbound file"):
                audit(freeze)
            freeze["file_bindings"][str(path)] = digest(path)
            path.write_bytes(b"changed")
            with self.assertRaisesRegex(ValueError, "input/output changed"):
                audit(freeze)

    def test_cache_drift_rejected_before_workers_but_recorded_after(self):
        with tempfile.TemporaryDirectory() as directory:
            freeze = self.audit_fixture(directory)
            path = Path(freeze["initial_caches"]["baseline"]["root"]) / "mesa/key"
            path.parent.mkdir(parents=True)
            path.write_bytes(b"new cache entry")
            with patch(
                "run_image_diagnostic.subprocess.check_output",
                side_effect=lambda args, **kwargs: "known\n" if "rev-parse" in args else b"",
            ):
                with self.assertRaisesRegex(ValueError, "Initial runtime cache"):
                    audit(freeze, initial=True)
                self.assertEqual(audit(freeze)["runtime_cache_states"]["baseline"]["file_count"], 1)


if __name__ == "__main__":
    unittest.main()
