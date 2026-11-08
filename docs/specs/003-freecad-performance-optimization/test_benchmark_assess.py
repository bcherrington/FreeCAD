# SPDX-License-Identifier: LGPL-2.1-or-later
"""Synthetic decision-rule tests; these are never FreeCAD performance evidence."""

import importlib.util
import json
import tempfile
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location(
    "assess", Path(__file__).with_name("benchmark-assess.py")
)
assess = importlib.util.module_from_spec(spec)
spec.loader.exec_module(assess)
SAMPLING = {
    "measured_pairs": 20,
    "bootstrap_seed": 3001,
    "bootstrap_resamples": 20000,
    "dispersion_limit_iqr_over_median": 0.1,
}


class AssessmentTests(unittest.TestCase):
    def test_linear_p95(self):
        self.assertAlmostEqual(assess.percentile(list(range(1, 21)), 0.95), 19.05)

    def test_paired_bootstrap_and_saving_floor(self):
        rule = {
            "kind": "improve",
            "minimum_gain_fraction": 0.05,
            "minimum_saved_ms": 2,
            "bootstrap_ratio_upper_must_be_below": 1,
        }
        passing = assess.assess_metric([100] * 20, [90] * 20, rule, SAMPLING)
        self.assertEqual(passing["status"], "pass")
        self.assertEqual(passing["bootstrap_ratio_interval"], [0.9, 0.9])
        self.assertEqual(passing["median_paired_saved_ms"], 10)
        self.assertEqual(
            assess.assess_metric([10] * 20, [9] * 20, rule, SAMPLING)["status"], "fail"
        )

    def test_resource_uses_maximum(self):
        rule = {"kind": "resource", "fraction": 0, "floor_bytes": 1048576}
        result = assess.assess_metric([0] * 20, [0] * 19 + [1048577], rule, SAMPLING)
        self.assertEqual(result["status"], "fail")
        self.assertEqual(result["change"]["median"], 0)

    def test_non_regression_checks_tail(self):
        rule = {
            "kind": "non_regression",
            "median_fraction": 0.05,
            "median_floor_ms": 2,
            "p95_fraction": 0.1,
            "p95_floor_ms": 5,
        }
        result = assess.assess_metric([100] * 20, [100] * 18 + [200, 200], rule, SAMPLING)
        self.assertEqual(result["status"], "fail")
        self.assertEqual(result["change"]["median"], 100)

    def test_variability_prevents_pass(self):
        rule = {"kind": "budget", "median_ms": 500, "p95_ms": 1000}
        self.assertEqual(
            assess.assess_metric([10, 100] * 10, [10, 100] * 10, rule, SAMPLING)["status"],
            "inconclusive",
        )

    def test_noisy_failed_budget_stays_failed(self):
        rule = {"kind": "budget", "median_ms": 50, "p95_ms": 100}
        result = assess.assess_metric([10] * 20, [100, 500] * 10, rule, SAMPLING)
        self.assertFalse(result["thresholds_passed"])
        self.assertEqual(result["status"], "fail")

    def test_zero_baseline_no_ratio_claim(self):
        rule = {
            "kind": "improve",
            "minimum_gain_fraction": 0.05,
            "minimum_saved_ms": 2,
            "bootstrap_ratio_upper_must_be_below": 1,
        }
        self.assertEqual(
            assess.assess_metric([0] * 20, [0] * 20, rule, SAMPLING)["status"], "inconclusive"
        )

    def test_one_failed_premature_load_is_failure(self):
        rule = {"kind": "zero", "baseline_must_be_positive": True}
        self.assertEqual(
            assess.assess_metric([1] * 20, [0] * 19 + [1], rule, SAMPLING)["status"], "fail"
        )

    def synthetic_collection(self, directory):
        plan = json.loads((assess.HERE / "benchmark-plan.json").read_text())
        profile = {
            "renderer": "synthetic-only",
            "vendor": "synthetic",
            "version": "test",
            "samples": 8,
            "dpr": 1,
            "viewport": [1280, 720],
        }
        context = directory / "context.json"
        namespaces = {"baseline": "b" * 64, "change": "c" * 64}
        modules = {arm: {"lib/libFreeCADGui.so": arm + "-synthetic"} for arm in namespaces}
        context.write_text(
            json.dumps(
                {
                    "native_profile": profile,
                    "effective_configuration_sha256": "synthetic-configuration",
                    "namespace_sha256": namespaces,
                    "runtime_module_sha256": modules,
                }
            )
        )
        execution = {
            "comparison_id": "P07",
            "fixture_id": "PartDesignExample",
            "fixture_sha256": plan["fixtures"]["PartDesignExample"]["sha256"],
            "context": "synthetic-only",
            "context_path": str(context),
            "context_sha256": assess.digest(context),
            "normative_state_sha256": "a" * 64,
            "protocol_document_sha256": assess.digest(assess.HERE / "benchmark-protocol.md"),
            "baseline": {"commit": "0" * 40},
            "change": {"commit": "1" * 40},
        }
        execution_path = directory / "execution.json"
        execution_path.write_text(json.dumps(execution))
        comparison = next(c for c in plan["comparisons"] if c["id"] == "P07")
        rows = [
            {
                "execution_sha256": assess.digest(execution_path),
                "protocol_sha256": assess.digest(assess.HERE / "benchmark-plan.json"),
            }
        ]
        runner_spec = importlib.util.spec_from_file_location(
            "runner_test", assess.HERE / "benchmark-runner.py"
        )
        runner = importlib.util.module_from_spec(runner_spec)
        runner_spec.loader.exec_module(runner)
        for pair in runner.schedule(plan):
            for arm in pair["order"]:
                record = {
                    k: execution[k]
                    for k in (
                        "comparison_id",
                        "fixture_id",
                        "fixture_sha256",
                        "context_sha256",
                        "normative_state_sha256",
                    )
                }
                record.update(
                    commit=execution[arm]["commit"],
                    correctness_passed=True,
                    qualification_only=False,
                    native_profile=profile.copy(),
                    effective_configuration_sha256="synthetic-configuration",
                    namespace_sha256=namespaces[arm],
                    runtime_module_sha256=modules[arm].copy(),
                    metrics={name: 100 for name in comparison["control_metrics"]},
                )
                rows.append({**pair, "arm": arm, "record": record})
        rows.append({"collection_complete": True})
        observations = directory / "observations.jsonl"
        observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
        return execution_path, observations, rows

    def test_complete_collection_rejects_mixed_renderer(self):
        with tempfile.TemporaryDirectory() as temp:
            execution, observations, rows = self.synthetic_collection(Path(temp))
            self.assertEqual(assess.assess(execution, observations)["status"], "pass")
            rows[1]["record"]["native_profile"]["renderer"] = "different-synthetic-renderer"
            observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
            with self.assertRaisesRegex(ValueError, "Renderer/driver"):
                assess.assess(execution, observations)

    def synthetic_command_collection(self, directory):
        execution_path, observations, rows = self.synthetic_collection(directory)
        execution = json.loads(execution_path.read_text())
        context_path = Path(execution["context_path"])
        context = json.loads(context_path.read_text())
        context.pop("native_profile")
        context.pop("effective_configuration_sha256")
        context.update(
            execution_mode="command-only",
            included_file_coverage={"included_properties": 1, "included_bytes": 10},
            endpoint="synthetic-console-endpoint",
            completion_recompute_count=0,
            generated_sql_parser=None,
        )
        context_path.write_text(json.dumps(context))
        execution.update(comparison_id="P01", context_sha256=assess.digest(context_path))
        execution_path.write_text(json.dumps(execution))
        rows[0]["execution_sha256"] = assess.digest(execution_path)
        for row in rows[1:-1]:
            record = row["record"]
            record.pop("native_profile")
            record.pop("effective_configuration_sha256")
            record.update(
                comparison_id="P01",
                context_sha256=execution["context_sha256"],
                included_file_coverage=context["included_file_coverage"],
                endpoint=context["endpoint"],
                completion_recompute_count=0,
                generated_sql_parser=None,
                hook_counters={
                    "restore_nanoseconds": 1000000,
                    "completed": 1,
                    "failed": 0,
                    "reused": 0,
                    "active": 0,
                },
            )
            record["metrics"]["restore_payload_ms"] = 1
            record["metrics"]["document_open_ms"] = 10
        observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
        return execution_path, observations, rows

    def test_console_collection_requires_no_renderer_claim(self):
        with tempfile.TemporaryDirectory() as temp:
            execution, observations, rows = self.synthetic_command_collection(Path(temp))
            self.assertEqual(assess.assess(execution, observations)["status"], "pass")
            rows[1]["record"]["native_profile"] = {"renderer": "unexpected"}
            observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
            with self.assertRaisesRegex(ValueError, "renderer claim"):
                assess.assess(execution, observations)

    def test_console_coverage_mismatch_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            execution, observations, rows = self.synthetic_command_collection(Path(temp))
            rows[1]["record"]["included_file_coverage"] = {
                "included_properties": 1,
                "included_bytes": 1,
            }
            observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
            with self.assertRaisesRegex(ValueError, "coverage changed"):
                assess.assess(execution, observations)

    def test_console_endpoint_mismatch_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            execution, observations, rows = self.synthetic_command_collection(Path(temp))
            rows[1]["record"]["endpoint"] = "wrong-endpoint"
            observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
            with self.assertRaisesRegex(ValueError, "endpoint changed"):
                assess.assess(execution, observations)

    def test_console_hook_counts_and_metric_binding_rejected(self):
        for field, value in (("active", 1), ("failed", 1), ("reused", 1), ("completed", 0)):
            with self.subTest(field=field), tempfile.TemporaryDirectory() as temp:
                execution, observations, rows = self.synthetic_command_collection(Path(temp))
                rows[1]["record"]["hook_counters"][field] = value
                observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
                with self.assertRaisesRegex(ValueError, "hook counts"):
                    assess.assess(execution, observations)
        with tempfile.TemporaryDirectory() as temp:
            execution, observations, rows = self.synthetic_command_collection(Path(temp))
            rows[1]["record"]["metrics"]["restore_payload_ms"] = 2
            observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
            with self.assertRaisesRegex(ValueError, "native hook duration"):
                assess.assess(execution, observations)

    def test_zero_console_payload_requires_reviewed_disposition(self):
        with tempfile.TemporaryDirectory() as temp:
            execution, observations, rows = self.synthetic_command_collection(Path(temp))
            context_path = Path(json.loads(execution.read_text())["context_path"])
            context = json.loads(context_path.read_text())
            context["included_file_coverage"] = {"included_properties": 0, "included_bytes": 0}
            context_path.write_text(json.dumps(context))
            frozen = json.loads(execution.read_text())
            frozen["context_sha256"] = assess.digest(context_path)
            execution.write_text(json.dumps(frozen))
            rows[0]["execution_sha256"] = assess.digest(execution)
            for row in rows[1:-1]:
                row["record"]["context_sha256"] = frozen["context_sha256"]
                row["record"]["included_file_coverage"] = context["included_file_coverage"]
            observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
            with self.assertRaisesRegex(ValueError, "no meaningful payload"):
                assess.assess(execution, observations)

    def test_reviewed_zero_control_is_inapplicable_and_still_checks_hooks(self):
        with tempfile.TemporaryDirectory() as temp:
            execution_path, observations, rows = self.synthetic_command_collection(Path(temp))
            execution = json.loads(execution_path.read_text())
            context_path = Path(execution["context_path"])
            context = json.loads(context_path.read_text())
            context["included_file_coverage"] = {"included_properties": 0, "included_bytes": 0}
            context["restore_payload_disposition"] = {
                "decision_id": "D006",
                "metric": "restore_payload_ms",
                "status": "not_applicable",
                "fixture_sha256": execution["fixture_sha256"],
            }
            context_path.write_text(json.dumps(context))
            execution["context_sha256"] = assess.digest(context_path)
            execution_path.write_text(json.dumps(execution))
            rows[0]["execution_sha256"] = assess.digest(execution_path)
            for row in rows[1:-1]:
                record = row["record"]
                record["context_sha256"] = execution["context_sha256"]
                record["included_file_coverage"] = context["included_file_coverage"]
                record["hook_counters"] = dict.fromkeys(record["hook_counters"], 0)
                record["metrics"]["restore_payload_ms"] = 0
            observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
            result = assess.assess(execution_path, observations)
            self.assertEqual(result["status"], "pass")
            self.assertEqual(result["metrics"]["restore_payload_ms"]["status"], "not_applicable")
            rows[1]["record"]["hook_counters"]["restore_nanoseconds"] = 1
            rows[1]["record"]["metrics"]["restore_payload_ms"] = 1e-6
            observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
            with self.assertRaisesRegex(ValueError, "zero-coverage hook duration"):
                assess.assess(execution_path, observations)

    def test_zero_payload_route_cannot_expand_to_other_fixtures_or_decisions(self):
        plan = json.loads((assess.HERE / "benchmark-plan.json").read_text())
        for fixture_id, decision, wrong_hash in (
            ("gridfinity-drawer-v1.3", "D006", False),
            ("BIMExample", "D006", False),
            ("PartDesignExample", "D005", False),
            ("PartDesignExample", "D006", True),
        ):
            with self.subTest(fixture=fixture_id, decision=decision, wrong_hash=wrong_hash):
                with tempfile.TemporaryDirectory() as temp:
                    execution_path, observations, rows = self.synthetic_command_collection(
                        Path(temp)
                    )
                    execution = json.loads(execution_path.read_text())
                    context_path = Path(execution["context_path"])
                    context = json.loads(context_path.read_text())
                    fixture_sha = plan["fixtures"][fixture_id]["sha256"]
                    context["restore_payload_disposition"] = {
                        "decision_id": decision,
                        "metric": "restore_payload_ms",
                        "status": "not_applicable",
                        "fixture_sha256": "0" * 64 if wrong_hash else fixture_sha,
                    }
                    context_path.write_text(json.dumps(context))
                    execution.update(
                        fixture_id=fixture_id,
                        fixture_sha256=fixture_sha,
                        context_sha256=assess.digest(context_path),
                    )
                    execution_path.write_text(json.dumps(execution))
                    rows[0]["execution_sha256"] = assess.digest(execution_path)
                    observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
                    with self.assertRaisesRegex(ValueError, "Invalid zero-payload disposition"):
                        assess.assess(execution_path, observations)

    def test_loaded_library_mismatch_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            execution, observations, rows = self.synthetic_collection(Path(temp))
            rows[1]["record"]["runtime_module_sha256"][
                "lib/libFreeCADGui.so"
            ] = "wrong-synthetic-library"
            observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
            with self.assertRaisesRegex(ValueError, "runtime libraries"):
                assess.assess(execution, observations)

    def test_incomplete_collection_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            execution, observations, rows = self.synthetic_collection(Path(temp))
            observations.write_text("".join(json.dumps(row) + "\n" for row in rows[:-1]))
            with self.assertRaisesRegex(ValueError, "Incomplete"):
                assess.assess(execution, observations)

    def test_wrong_order_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            execution, observations, rows = self.synthetic_collection(Path(temp))
            rows[1], rows[2] = rows[2], rows[1]
            observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
            with self.assertRaisesRegex(ValueError, "Schedule/order"):
                assess.assess(execution, observations)

    def test_qualification_records_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            execution, observations, rows = self.synthetic_collection(Path(temp))
            rows[1]["record"]["qualification_only"] = True
            observations.write_text("".join(json.dumps(row) + "\n" for row in rows))
            with self.assertRaisesRegex(ValueError, "Unqualified"):
                assess.assess(execution, observations)

    def test_rejects_nonfinite_or_boolean(self):
        rule = {"kind": "resource", "fraction": 0, "floor_bytes": 1}
        for invalid in (float("nan"), float("inf"), -1, True):
            with self.assertRaises(ValueError):
                assess.assess_metric([1] * 20, [1] * 19 + [invalid], rule, SAMPLING)


if __name__ == "__main__":
    unittest.main()
