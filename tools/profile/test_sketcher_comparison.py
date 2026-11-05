# SPDX-License-Identifier: LGPL-2.1-or-later
"""Analytical checks for the performance acceptance calculations (no FreeCAD needed)."""

import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import MagicMock, patch

from run_sketcher_comparison import Worker, expected_vector, summarize


class ComparisonAcceptanceTests(unittest.TestCase):
    def test_startup_failure_terminates_owned_process(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "build/Mod").mkdir(parents=True)
            process = MagicMock()
            process.stdout = []
            process.poll.return_value = None
            with patch("run_sketcher_comparison.subprocess.Popen", return_value=process):
                with self.assertRaisesRegex(RuntimeError, "did not become ready"):
                    Worker(
                        root / "build",
                        root / "sample",
                        "forced_synthetic",
                        root / "counter.so",
                        root / "fixture.FCStd",
                        True,
                        "candidate",
                    )
            process.terminate.assert_called_once()

    def test_preserves_pair_covariance(self):
        result = summarize([(1.0, 0.5), (2.0, 1.0)] * 5)
        self.assertEqual(result["speedup_ci95"], [0.5, 0.5])
        self.assertEqual(result["speedup_gate"], "pass")

    def test_distinguishes_overhead_from_regression_allowance(self):
        pairs = [(0.1, 0.103)] * 10
        self.assertEqual(summarize(pairs)["regression_gate"], "pass")
        self.assertEqual(summarize(pairs, overhead=True)["regression_gate"], "fail")

    def test_applies_one_millisecond_floor(self):
        result = summarize([(0.001, 0.0019)] * 10, overhead=True)
        self.assertEqual(result["allowance_s"], 0.001)
        self.assertEqual(result["regression_gate"], "pass")
        self.assertEqual(result["speedup_gate"], "fail")

    def test_overlap_is_inconclusive_despite_median_failure(self):
        result = summarize([(0.1, 0.09), (0.1, 0.13)] * 5)
        self.assertGreater(result["difference_s"], result["allowance_s"])
        self.assertLess(result["difference_minus_allowance_ci95_s"][0], 0)
        self.assertGreater(result["difference_minus_allowance_ci95_s"][1], 0)
        self.assertEqual(result["regression_gate"], "inconclusive")

    def test_only_changes_targeted_execution_vectors(self):
        root = Path(__file__).resolve().parents[2]
        manifest = json.loads(
            (root / "docs/specs/000-sketcher-diagnosis-performance/workloads.json").read_text()
        )
        for case in ("clean_noop", "fresh_load", "warm_load", "thickness"):
            self.assertEqual(
                expected_vector(manifest, case, "candidate"),
                expected_vector(manifest, case, "baseline"),
            )
        for case, name, before, after in (
            ("forced_synthetic", "Profile", 2, 1),
            ("forced_master", "Sketch004", 2, 1),
            ("forced_profile", "BottomPlateProfile", 2, 1),
            ("scalar_insertion", "Profile", 18, 17),
            ("list_insertion", "Profile", 3, 2),
        ):
            self.assertEqual(expected_vector(manifest, case, "baseline")[name]["setups"], before)
            self.assertEqual(expected_vector(manifest, case, "candidate")[name]["setups"], after)


if __name__ == "__main__":
    unittest.main()
