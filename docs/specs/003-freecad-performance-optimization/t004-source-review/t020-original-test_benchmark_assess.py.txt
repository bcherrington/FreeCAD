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
