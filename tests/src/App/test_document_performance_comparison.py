# SPDX-License-Identifier: LGPL-2.1-or-later
import importlib.util
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location(
    "comparison", Path(__file__).with_name("run_document_performance_comparison.py")
)
c = importlib.util.module_from_spec(spec)
spec.loader.exec_module(c)


class CollectorContractTest(unittest.TestCase):
    def test_fixed_original_cells_and_pairs(self):
        for comparison, cells in [
            ("P02", ["gridfinity-drawer-v1.3", "ProjectTest", "PartDesignExample"]),
            (
                "P03",
                [
                    "BIMExample",
                    "ArchDetail",
                    "EngineBlock",
                    "PartDesignExample",
                    "draft_test_objects",
                ],
            ),
        ]:
            rows = c.schedule(comparison)
            self.assertEqual(len(rows), 48 * len(cells))
            self.assertEqual(list(dict.fromkeys(r[0] for r in rows)), cells)
            self.assertEqual(sum(r[3] == "measured" for r in rows), 40 * len(cells))
            self.assertEqual(
                rows[:4],
                [
                    (cells[0], 0, "baseline", "diagnostic"),
                    (cells[0], 0, "change", "diagnostic"),
                    (cells[0], 1, "change", "warmup"),
                    (cells[0], 1, "baseline", "warmup"),
                ],
            )
        with self.assertRaises(ValueError):
            c.schedule("P08")

    def test_destinations_reject_existing_files_symlinks_and_public_cache(self):
        with tempfile.TemporaryDirectory() as tmp:
            cache = Path(tmp)
            cache.chmod(0o700)
            freeze = {"initial_caches": {"change": {"root": str(cache)}}}
            dest = c.validate_destination(freeze, 0, "change")
            dest.write_text("{}")
            with self.assertRaises(ValueError):
                c.validate_destination(freeze, 0, "change")
            dest.unlink()
            dest.symlink_to(cache / "absent")
            with self.assertRaises(ValueError):
                c.validate_destination(freeze, 0, "change")
            dest.unlink()
            cache.chmod(0o755)
            with self.assertRaises(ValueError):
                c.validate_destination(freeze, 0, "change")

    def test_policy_mismatch_stops_before_outputs_or_worker_identity(self):
        with tempfile.TemporaryDirectory() as tmp:
            policy_root = Path(tmp) / "docs/specs/003-freecad-performance-optimization"
            policy_root.mkdir(parents=True)
            policies = [
                policy_root / name
                for name in [
                    "acceptance-policy-v2.md",
                    "acceptance-policy-v2.json",
                    "cost-benefit-review.md",
                    "cost-benefit-review.json",
                    "t0043-execution-plan.md",
                ]
            ]
            for policy in policies:
                policy.write_text('{"version":2}')
            art = Path(tmp) / "campaign"
            art.mkdir()
            freeze = {
                "schema": "freecad-spec003-document-performance-freeze/v1",
                "controller_sha256": c.digest(Path(c.__file__)),
                "sampling": {
                    "pairs": 24,
                    "diagnostic_pairs": 1,
                    "warmup_pairs": 3,
                    "measured_pairs": 20,
                },
                "limits": {
                    "worker_seconds": 60,
                    "worker_output_bytes": 262144,
                    "cell_seconds": 1200,
                },
                "project_root": tmp,
                "policy_bindings": {str(policy): "0" * 64 for policy in policies},
                "file_bindings": {str(policy): "0" * 64 for policy in policies},
                "artifact_directory": str(art),
            }
            with patch.object(c.shared, "execution_identity") as execution, patch.object(
                c, "audit"
            ) as audit:
                with self.assertRaises(ValueError):
                    c.preflight(freeze)
                execution.assert_not_called()
                audit.assert_not_called()
            self.assertEqual(list(art.iterdir()), [])

    def test_freeze_checksum_is_checked_before_json_decode(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "freeze.json"
            path.write_text("invalid JSON")
            with self.assertRaisesRegex(ValueError, "Freeze identity changed"):
                c.shared.load_freeze(path, "0" * 64)

    def test_environment_uses_frozen_allowlist(self):
        freeze = {
            "initial_caches": {"change": {"root": "/private/cache"}},
            "activation_environment": {"PATH": "/frozen/bin", "LANG": "C.UTF-8"},
        }
        with patch.dict(os.environ, {"PYTHONPATH": "/untrusted", "SPEC003_UNEXPECTED": "secret"}):
            env = c.shared.worker_environment(freeze, "change")
        self.assertNotIn("PYTHONPATH", env)
        self.assertNotIn("SPEC003_UNEXPECTED", env)
        self.assertEqual(env["PATH"], "/frozen/bin")
        self.assertEqual(env["HOME"], "/private/cache/home")

    def test_failed_partial_map_cannot_be_successful_coverage(self):
        trace = {
            "save_active": 0,
            "map_active": 0,
            "save_failed": 0,
            "map_failed": 1,
            "save_calls": 0,
            "save_completed": 0,
            "map_calls": 2,
            "map_completed": 1,
            "map_records": 20,
        }
        row = {
            **{k: None for k in c.IDENTITY_KEYS},
            "correctness_passed": True,
            "qualification_only": False,
            "comparison_id": "P03",
            "fixture_id": "BIMExample",
            "commit": "commit",
            "namespace_sha256": "namespace",
            "runtime_module_sha256": {},
            "runtime_maps": "",
            "metrics": {
                "map_restore_ms": 1,
                "document_open_ms": 2,
                "peak_rss_bytes": 3,
                "coin_image_bytes": 0,
                "decoded_image_bytes": 0,
            },
            "open_trace": trace,
        }
        expected = {
            "identity": c.identity(row),
            "namespace_sha256": "namespace",
            "runtime_module_sha256": {},
            "runtime_mappings": [],
        }
        freeze = {
            "comparison": "P03",
            "cells": {"BIMExample": {"qualified": {"change": expected}}},
            "arms": {"change": {"commit": "commit", "source": "/source"}},
            "canonical_source": "/canonical",
        }
        with self.assertRaisesRegex(ValueError, "Failed/unbalanced"):
            c.validate_row(freeze, row, "BIMExample", "change")

    def test_zero_timing_or_rss_cannot_pass(self):
        trace = {
            "save_active": 0,
            "map_active": 0,
            "save_failed": 0,
            "map_failed": 0,
            "save_calls": 0,
            "save_completed": 0,
            "map_calls": 2,
            "map_completed": 2,
        }
        row = {
            **{k: None for k in c.IDENTITY_KEYS},
            "correctness_passed": True,
            "qualification_only": False,
            "comparison_id": "P03",
            "fixture_id": "BIMExample",
            "commit": "commit",
            "namespace_sha256": "namespace",
            "runtime_module_sha256": {},
            "runtime_maps": "",
            "metrics": {
                "map_restore_ms": 1,
                "document_open_ms": 2,
                "peak_rss_bytes": 3,
                "coin_image_bytes": 0,
                "decoded_image_bytes": 0,
            },
            "open_trace": trace,
        }
        expected = {
            "identity": c.identity(row),
            "namespace_sha256": "namespace",
            "runtime_module_sha256": {},
            "runtime_mappings": [],
        }
        freeze = {
            "comparison": "P03",
            "cells": {"BIMExample": {"qualified": {"change": expected}}},
            "arms": {"change": {"commit": "commit", "source": "/source"}},
            "canonical_source": "/canonical",
        }
        c.validate_row(freeze, row, "BIMExample", "change")
        for metric in ["map_restore_ms", "document_open_ms", "peak_rss_bytes"]:
            value = row["metrics"][metric]
            row["metrics"][metric] = 0
            with self.assertRaisesRegex(ValueError, "zero timing"):
                c.validate_row(freeze, row, "BIMExample", "change")
            row["metrics"][metric] = value

    def test_result_file_obeys_frozen_cap_and_private_mode(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "result.json"
            path.write_text("{}")
            path.chmod(0o600)
            self.assertEqual(c.read_result(path, 262144), {})
            path.write_text(json.dumps({"padding": "x" * 262144}))
            with self.assertRaisesRegex(ValueError, "oversized"):
                c.read_result(path, 262144)
            path.write_text("{}")
            path.chmod(0o644)
            with self.assertRaisesRegex(ValueError, "private"):
                c.read_result(path, 262144)

    def test_review_fields_and_production_scope_cannot_be_weakened(self):
        with tempfile.TemporaryDirectory() as tmp:
            policyroot = Path(tmp) / "docs/specs/003-freecad-performance-optimization"
            policyroot.mkdir(parents=True)
            names = [
                "acceptance-policy-v2.md",
                "acceptance-policy-v2.json",
                "cost-benefit-review.md",
                "cost-benefit-review.json",
                "t0043-execution-plan.md",
            ]
            for name in names:
                (policyroot / name).write_text("fixture")
            policy = {
                "schema": "freecad-spec003-cost-benefit-policy/v2",
                "decision": "D007",
                "status": "owner-selected",
                "policy_id": "fixture-v2",
                "policy_document_sha256": c.digest(policyroot / "acceptance-policy-v2.md"),
                "required_review_fields": ["benefit"],
            }
            (policyroot / "acceptance-policy-v2.json").write_text(json.dumps(policy))
            bindings = {str(policyroot / name): c.digest(policyroot / name) for name in names}
            freeze = {
                "schema": "freecad-spec003-document-performance-freeze/v1",
                "controller_sha256": c.digest(Path(c.__file__)),
                "sampling": {
                    "pairs": 24,
                    "diagnostic_pairs": 1,
                    "warmup_pairs": 3,
                    "measured_pairs": 20,
                },
                "limits": {
                    "worker_seconds": 60,
                    "worker_output_bytes": 262144,
                    "cell_seconds": 1200,
                },
                "project_root": tmp,
                "policy_bindings": bindings,
                "file_bindings": bindings,
                "review_policy_id": "fixture-v2",
                "required_review_fields": ["benefit"],
                "production_accepted": False,
                "v1_statistics_are_diagnostics_only": True,
            }
            for key, value in [
                ("required_review_fields", []),
                ("production_accepted", True),
                ("v1_statistics_are_diagnostics_only", False),
            ]:
                bad = {**freeze, key: value}
                with patch.object(c.shared, "execution_identity") as execution:
                    with self.assertRaisesRegex(ValueError, "acceptance scope"):
                        c.preflight(bad)
                    execution.assert_not_called()


if __name__ == "__main__":
    unittest.main()
