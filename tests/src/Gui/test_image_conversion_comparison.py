# SPDX-License-Identifier: LGPL-2.1-or-later
"""Coverage and provenance guards for the finite conversion campaigns."""

import copy
import hashlib
import json
import tempfile
from unittest.mock import patch
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_image_conversion_comparison as comparison


class ConversionComparisonTest(unittest.TestCase):
    def test_exact_coverage_and_phase_counts(self):
        for name, cells in [("P04", 3), ("P05", 10)]:
            rows = comparison.schedule(name)
            self.assertEqual(len(rows), cells * 48)
            self.assertEqual(len({row[0] for row in rows}), cells)
            for cell in {row[0] for row in rows}:
                selected = [row for row in rows if row[0] == cell]
                self.assertEqual(sum(row[3] == "measured" for row in selected), 40)
                self.assertEqual(sum(row[3] == "warmup" for row in selected), 6)
                self.assertEqual(sum(row[3] == "diagnostic" for row in selected), 2)
                self.assertEqual(
                    [r[2] for r in selected[:4]], ["baseline", "change", "change", "baseline"]
                )

    def test_all_fallback_formats_and_sizes(self):
        cells = {row[0] for row in comparison.schedule("P05")}
        self.assertEqual(
            cells,
            {
                f"{recipe}-{size}"
                for recipe in [
                    "indexed-gray",
                    "indexed-gray-alpha",
                    "premultiplied",
                    "gray16",
                    "svg",
                ]
                for size in [512, 4096]
            },
        )

    def test_runtime_mapping_rebases_only_canonical_root(self):
        maps = (
            "1-2 r-xp 0 0 1 /canonical/lib/x.so\n2-3 r-xp 0 0 1 /system/y.so\n3-4 rw-p 0 0 0 [heap]"
        )
        self.assertEqual(
            comparison.mappings(maps, {"source": "/physical"}, "/canonical"),
            ["/physical/lib/x.so", "/system/y.so"],
        )
        with self.assertRaises(ValueError):
            comparison.mappings(
                "1-2 r-xp 0 0 1 /system/x.so (deleted)", {"source": "/physical"}, "/canonical"
            )

    def valid_fallback_freeze(self):
        cells = {}
        for name in {row[0] for row in comparison.schedule("P05")}:
            recipe, size = name.rsplit("-", 1)
            extent = int(size)
            row = self.valid_row()
            row.update(
                recipe=recipe,
                width=extent,
                height=extent,
                qt_platform="xcb",
                source_format_name={
                    "indexed-gray": "Format_Indexed8",
                    "indexed-gray-alpha": "Format_Indexed8",
                    "gray16": "Format_Grayscale16",
                    "premultiplied": "Format_ARGB32_Premultiplied",
                    "svg": "Format_ARGB32_Premultiplied",
                }[recipe],
            )
            qualified = {
                arm: {"identity": comparison.identity(row), "coin_components": 4}
                for arm in ["baseline", "change"]
            }
            qualified["baseline"]["coin_components"] = {
                "indexed-gray": 1,
                "indexed-gray-alpha": 2,
                "gray16": 2,
                "premultiplied": 4,
                "svg": 4,
            }[recipe]
            cells[name] = {
                "recipe": recipe,
                "extent": extent,
                "input": "-",
                "refinement_id": "spec003-p05-materialized-"
                + ("large" if extent == 4096 else "small")
                + "-20261108-v1",
                "qualified": qualified,
            }
        return {"comparison": "P05", "cells": cells}

    def test_fixed_cell_definitions_reject_drift(self):
        freeze = self.valid_fallback_freeze()
        comparison.validate_cells(freeze)
        for field, value in [
            ("recipe", "payload"),
            ("extent", 512),
            ("input", "/substitute"),
            ("refinement_id", "old-ambiguous-recipe"),
        ]:
            changed = copy.deepcopy(freeze)
            changed["cells"]["gray16-4096"][field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                comparison.validate_cells(changed)
        for field, value in [("coin_components", 4)]:
            changed = copy.deepcopy(freeze)
            changed["cells"]["gray16-4096"]["qualified"]["baseline"][field] = value
            with self.assertRaises(ValueError):
                comparison.validate_cells(changed)
        changed = copy.deepcopy(freeze)
        changed["cells"]["gray16-4096"]["qualified"]["baseline"]["identity"]["width"] = 512
        with self.assertRaises(ValueError):
            comparison.validate_cells(changed)
        with self.assertRaises(ValueError):
            comparison.schedule("unknown")

    def test_freeze_bytes_loaded_once_and_hash_required(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "freeze.json"
            data = b'{"selected":true}'
            path.write_bytes(data)
            self.assertEqual(
                comparison.load_freeze(path, hashlib.sha256(data).hexdigest()), {"selected": True}
            )
            with self.assertRaises(ValueError):
                comparison.load_freeze(path, "0" * 64)
            with patch.object(Path, "read_bytes", return_value=data) as read:
                comparison.load_freeze(path, hashlib.sha256(data).hexdigest())
                self.assertEqual(read.call_count, 1)

    def test_stale_outputs_and_pre_audit_failure_create_no_campaign(self):
        with tempfile.TemporaryDirectory() as directory:
            art = Path(directory)
            freeze = {"artifact_directory": directory}
            plan = [1, 2]
            with patch.object(comparison, "audit", side_effect=ValueError("drift")):
                with self.assertRaises(ValueError):
                    comparison.preflight(freeze, plan)
            self.assertEqual(list(art.iterdir()), [])
            for name in [
                "status.json",
                "audit-before.json",
                "audit-after.json",
                "observations.jsonl",
                "host.jsonl",
                "worker-0000.bin",
                "worker-0001.bin",
            ]:
                path = art / name
                path.write_text("retained")
                with patch.object(comparison, "audit") as audit:
                    with self.assertRaises(ValueError):
                        comparison.preflight(freeze, plan)
                    audit.assert_not_called()
                self.assertEqual(path.read_text(), "retained")
                path.unlink()

    def test_both_python_namespace_stages_isolated(self):
        freeze = {
            "comparison": "P04",
            "pixi_executable": "pixi",
            "namespace_wrapper": "wrapper.py",
            "canonical_source": "/canonical",
            "arms": {"baseline": {"source": "/source", "commit": "sha"}},
            "initial_caches": {"baseline": {"root": "/cache"}},
            "cells": {"cell": {"recipe": "svg", "extent": 512, "input": "-"}},
        }
        argv = comparison.worker_command(freeze, "cell", "baseline")
        index = argv.index("python")
        self.assertEqual(argv[index + 1 : index + 4], ["-I", "-S", "-B"])
        self.assertIn("--isolated-python", argv)
        namespace = (Path(__file__).parent / "image_performance_namespace.py").read_text()
        self.assertIn('*(["-I", "-S", "-B"] if args.isolated_python else [])', namespace)

    def activation_freeze(self, directory):
        native = Path(directory) / "python"
        native.write_bytes(b"native")
        git = Path(directory) / "git"
        git.write_bytes(b"git")
        values = {"PATH": "/captured/bin", "DISPLAY": ":0", "LANG": "C.UTF-8"}
        files = {}
        for name in ["capture.py", "pixi", "pixi.toml", "pixi.lock"]:
            path = Path(directory) / name
            path.write_text(name)
            files[name] = str(path)
        freeze = {
            "comparison": "P05",
            "project_root": directory,
            "activation_environment": values,
            "activation_environment_sha256": hashlib.sha256(
                json.dumps(values, sort_keys=True, separators=(",", ":")).encode()
            ).hexdigest(),
            "native_python": str(native),
            "native_python_sha256": comparison.digest(native),
            "git_executable": str(git),
            "git_sha256": comparison.digest(git),
            "pixi_executable": files["pixi"],
            "pixi_sha256": comparison.digest(Path(files["pixi"])),
            "pixi_version": "test",
            "pixi_manifest": files["pixi.toml"],
            "pixi_lock": files["pixi.lock"],
            "manifest_sha256": comparison.digest(Path(files["pixi.toml"])),
            "lock_sha256": comparison.digest(Path(files["pixi.lock"])),
            "env_executable_sha256": comparison.digest(Path("/usr/bin/env")),
            "activation_capture_script": files["capture.py"],
            "capture_script_sha256": comparison.digest(Path(files["capture.py"])),
            "native_python_closure": {str(native): comparison.digest(native)},
            "file_bindings": {},
            "initial_caches": {"baseline": {"root": "/cache"}},
        }
        for path in [str(native), str(git), *files.values(), "/usr/bin/env"]:
            freeze["file_bindings"][path] = comparison.digest(Path(path))
        receipt = Path(directory) / "activation.json"
        record = dict(
            freeze,
            schema="freecad-spec003-frozen-pixi-activation/v1",
            exit_code=0,
            argv=[freeze["pixi_executable"], "run", "--locked", "/usr/bin/env", "-0"],
            cwd=directory,
            stderr_size_bytes=0,
            stderr_sha256=hashlib.sha256(b"").hexdigest(),
        )
        receipt.write_text(json.dumps(record, sort_keys=True))
        freeze["activation_receipt"] = str(receipt)
        freeze["activation_receipt_sha256"] = comparison.digest(receipt)
        freeze["file_bindings"][str(receipt)] = comparison.digest(receipt)
        return freeze

    def test_activation_value_extra_variable_and_binary_drift_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            freeze = self.activation_freeze(directory)
            with patch.object(comparison.shutil, "which", return_value=freeze["git_executable"]):
                comparison.validate_environment(freeze)
                for key, value in [("LANG", "different"), ("USER", "operator")]:
                    changed = copy.deepcopy(freeze)
                    changed["activation_environment"][key] = value
                    with self.subTest(key=key), self.assertRaises(ValueError):
                        comparison.validate_environment(changed)
                Path(freeze["native_python"]).write_bytes(b"changed")
                with self.assertRaises(ValueError):
                    comparison.validate_environment(freeze)

    def test_worker_environment_never_inherits_operator_values(self):
        with tempfile.TemporaryDirectory() as directory:
            freeze = self.activation_freeze(directory)
            with patch.object(
                comparison.shutil, "which", return_value=freeze["git_executable"]
            ), patch.dict(
                comparison.os.environ, {"UNRELATED_TEST_VALUE": "ignored", "HOME": "/operator"}
            ):
                env = comparison.worker_environment(freeze, "baseline")
            self.assertNotIn("UNRELATED_TEST_VALUE", env)
            self.assertEqual(env["HOME"], "/cache/home")
            self.assertEqual(env["XDG_CONFIG_HOME"], "/cache/config")
            self.assertEqual(env["PATH"], "/captured/bin")

    def test_wrong_controller_interpreter_rejected_before_ownership(self):
        with tempfile.TemporaryDirectory() as directory:
            fake = Path(directory) / "wrong-python"
            fake.write_bytes(b"wrong")
            with patch.object(comparison, "OwnedSession") as owner:
                with self.assertRaises(ValueError):
                    comparison.execution_identity({"host_interpreter": str(fake)})
                owner.assert_not_called()

    def test_missing_pidfd_rejected_before_ownership(self):
        actual = Path(sys.executable).resolve()
        freeze = {"host_interpreter": str(actual), "file_bindings": {str(actual): "hash"}}
        with patch.object(comparison, "digest", return_value="hash"), patch.object(
            comparison.os, "pidfd_open", None, create=True
        ), patch.object(comparison, "OwnedSession") as owner:
            with self.assertRaises(ValueError):
                comparison.execution_identity(freeze)
            owner.assert_not_called()

    def test_real_preflight_sequence_and_nonisolated_worker_argv_rejected(self):
        actual = Path(sys.executable).resolve()
        freeze = {
            "comparison": "P05",
            "host_interpreter": str(actual),
            "file_bindings": {str(actual): "hash"},
            "cells": {"cell": {}},
            "worker_commands": {
                "cell": {"baseline": ["native", "-S", "-B"], "change": ["native", "-I", "-S", "-B"]}
            },
        }
        with patch.object(comparison, "digest", return_value="hash"), patch.object(
            comparison.os, "pidfd_open", return_value=123, create=True
        ) as opened, patch.object(
            comparison.signal, "pidfd_send_signal", create=True
        ) as send, patch.object(
            comparison.os, "close"
        ) as close, patch.object(
            comparison, "OwnedSession"
        ) as owner, patch.object(
            comparison, "validate_environment"
        ), patch.object(
            comparison, "worker_command", return_value=["native", "-I", "-S", "-B"]
        ):
            with self.assertRaises(ValueError):
                comparison.execution_identity(freeze)
            owner.return_value.start.assert_called_once()
            owner.return_value.close.assert_called_once()
            opened.assert_called_once_with(comparison.os.getpid())
            send.assert_called_once_with(123, 0)
            close.assert_called_once_with(123)

    def test_activation_receipt_must_match_and_closure_must_be_bound(self):
        with tempfile.TemporaryDirectory() as directory:
            freeze = self.activation_freeze(directory)
            with patch.object(comparison.shutil, "which", return_value=freeze["git_executable"]):
                changed = copy.deepcopy(freeze)
                changed["pixi_version"] = "unrelated"
                with self.assertRaises(ValueError):
                    comparison.validate_environment(changed)
                changed = copy.deepcopy(freeze)
                del changed["file_bindings"][changed["native_python"]]
                with self.assertRaises(ValueError):
                    comparison.validate_environment(changed)
                original = comparison.digest
                with patch.object(
                    comparison,
                    "digest",
                    side_effect=lambda p: "changed" if str(p) == "/usr/bin/env" else original(p),
                ):
                    with self.assertRaises(ValueError):
                        comparison.validate_environment(freeze)

    def test_all_writable_trees_are_in_initial_audit(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            freeze = {
                "comparison": "P05",
                "initial_caches": {"baseline": {"root": directory, "files": {}}},
            }
            with patch.object(comparison, "base_audit", return_value={"passed": True}):
                comparison.audit(freeze, initial=True)
                for name in ["home", "config", "data", "temp", "mesa", "xdg"]:
                    path = root / name
                    path.mkdir()
                    file = path / "state"
                    file.write_text("unexpected")
                    with self.subTest(tree=name), self.assertRaises(ValueError):
                        comparison.audit(freeze, initial=True)
                    self.assertEqual(
                        comparison.audit(freeze)["runtime_cache_states"]["baseline"]["file_count"],
                        1,
                    )
                    file.unlink()

    def test_interpreter_alias_without_resolved_binding_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            actual = Path(sys.executable).resolve()
            alias = Path(directory) / "alias"
            alias.symlink_to(actual)
            freeze = {"host_interpreter": str(alias), "file_bindings": {str(alias): "hash"}}
            with patch.object(comparison, "digest", return_value="hash"), patch.object(
                comparison, "OwnedSession"
            ) as owner:
                with self.assertRaises(ValueError):
                    comparison.execution_identity(freeze)
                owner.assert_not_called()

    def test_activation_capture_operation_is_exact(self):
        with tempfile.TemporaryDirectory() as directory:
            freeze = self.activation_freeze(directory)
            path = Path(freeze["activation_receipt"])
            original = json.loads(path.read_text())
            with patch.object(comparison.shutil, "which", return_value=freeze["git_executable"]):
                for field, value in [
                    ("cwd", "/wrong"),
                    ("argv", ["pixi", "run", "unlocked"]),
                    ("exit_code", 1),
                    ("stderr_size_bytes", 1),
                ]:
                    data = dict(original)
                    data[field] = value
                    path.write_text(json.dumps(data))
                    changed = copy.deepcopy(freeze)
                    changed["activation_receipt_sha256"] = comparison.digest(path)
                    changed["file_bindings"][str(path)] = comparison.digest(path)
                    with self.subTest(field=field), self.assertRaises(ValueError):
                        comparison.validate_environment(changed)

    def test_environment_construction_is_pure_after_preflight(self):
        with patch.object(
            comparison, "validate_environment", side_effect=AssertionError("repeated IO")
        ):
            freeze = {
                "activation_environment": {"PATH": "frozen"},
                "initial_caches": {"baseline": {"root": "/cache"}},
            }
            for _ in range(3):
                self.assertEqual(
                    comparison.worker_environment(freeze, "baseline")["PATH"], "frozen"
                )

    def test_writable_symlinks_are_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "link"
            path.symlink_to("/tmp", target_is_directory=True)
            with self.assertRaises(ValueError):
                comparison.full_cache_inventory(directory)

    def valid_row(self):
        row = {
            key: "fixed"
            for key in [
                "recipe",
                "source_format_name",
                "source_color_space",
                "source_pixel_sha256",
                "source_palette_sha256",
                "qt_version",
                "qt_platform",
            ]
        }
        row.update(
            width=512,
            height=512,
            source_format=3,
            source_bytes_per_line=512,
            source_bytes=262144,
            source_has_alpha=False,
            source_color_space_valid=False,
            schema="freecad-image-conversion-observation/v1",
            correctness_passed=True,
            max_rgba_error=0,
            owns_pixels_after_source_write_and_destruction=True,
            coin_components=1,
            coin_pixel_sha256="coin",
            runtime_maps="1-2 r-xp 0 0 1 /system/x.so",
            trace={
                "storage_trace_balanced": True,
                "live_coin_bytes": 0,
                "live_decoded_bytes": 0,
                "diagnostic_active_intervals": 0,
            },
        )
        return row

    def test_rejects_fixture_component_output_runtime_and_trace_drift(self):
        row = self.valid_row()
        expected = {
            "identity": comparison.identity(row),
            "coin_components": 1,
            "coin_pixel_sha256": "coin",
            "runtime_mappings": ["/system/x.so"],
        }
        comparison.validate_row(row, expected, {"source": "/physical"}, "/canonical")
        for key, value in [
            ("width", 511),
            ("source_pixel_sha256", "different"),
            ("coin_components", 4),
            ("coin_pixel_sha256", "different"),
            ("runtime_maps", ""),
            ("correctness_passed", False),
            ("max_rgba_error", 2),
            ("owns_pixels_after_source_write_and_destruction", False),
        ]:
            changed = copy.deepcopy(row)
            changed[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                comparison.validate_row(changed, expected, {"source": "/physical"}, "/canonical")
        changed = copy.deepcopy(row)
        changed["trace"]["live_decoded_bytes"] = 1
        with self.assertRaises(ValueError):
            comparison.validate_row(changed, expected, {"source": "/physical"}, "/canonical")


if __name__ == "__main__":
    unittest.main()
