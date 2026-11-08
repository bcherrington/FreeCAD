# SPDX-License-Identifier: LGPL-2.1-or-later
"""Check attribution boundaries independently of the native/Qt probe."""

import unittest

from ImagePerformanceProbe import diagnostic_frame_timing


class FrameDiagnosticTest(unittest.TestCase):
    def setUp(self):
        self.before = {
            "diagnostic_" + phase + "_" + suffix: 0
            for phase in ("load", "decode", "convert", "render")
            for suffix in ("ns", "calls")
        }
        self.before.update(load_attempts=0, empty_load_calls=0)
        self.after = dict(
            self.before,
            diagnostic_active_intervals=0,
            diagnostic_nested_same_phase=False,
            diagnostic_render_image_overlap=False,
        )
        self.after.update(diagnostic_render_calls=1, diagnostic_render_ns=2_000_000)
        self.points = dict(
            zip(
                (
                    "started",
                    "operation_returned",
                    "redraw_requested",
                    "swap_callback",
                    "glfinish_started",
                    "finished",
                ),
                (0, 4_000_000, 5_000_000, 8_000_000, 9_000_000, 10_000_000),
            )
        )

    def test_nested_load_and_final_glfinish_are_not_added_twice(self):
        self.after.update(
            diagnostic_load_ns=4_000_000,
            diagnostic_decode_ns=2_000_000,
            diagnostic_convert_ns=1_000_000,
            diagnostic_render_ns=2_000_000,
            diagnostic_load_calls=1,
            diagnostic_decode_calls=1,
            diagnostic_convert_calls=1,
            load_attempts=1,
        )
        result = diagnostic_frame_timing(self.points, self.before, self.after)
        self.assertEqual(result["unattributed_ms"], 4)
        self.assertEqual(
            sum(
                result["wall_partitions_ms"][k]
                for k in result["wall_partitions_ms"]
                if k != "glfinish_ms"
            ),
            10,
        )

    def test_overlap_prevents_residual_attribution(self):
        self.after["diagnostic_render_image_overlap"] = True
        self.assertIsNone(
            diagnostic_frame_timing(self.points, self.before, self.after)["unattributed_ms"]
        )

    def test_clock_inversion_is_rejected(self):
        self.points["operation_returned"] = 20_000_000
        with self.assertRaisesRegex(RuntimeError, "clock order"):
            diagnostic_frame_timing(self.points, self.before, self.after)

    def test_counter_reset_is_rejected(self):
        self.before["diagnostic_decode_calls"] = 1
        with self.assertRaisesRegex(RuntimeError, "counter moved backwards"):
            diagnostic_frame_timing(self.points, self.before, self.after)

    def test_running_native_span_is_rejected(self):
        self.after["diagnostic_active_intervals"] = 1
        with self.assertRaisesRegex(RuntimeError, "still active"):
            diagnostic_frame_timing(self.points, self.before, self.after)

    def test_all_zero_hook_coverage_is_rejected(self):
        self.after.update(diagnostic_render_calls=0, diagnostic_render_ns=0)
        with self.assertRaisesRegex(RuntimeError, "no coverage"):
            diagnostic_frame_timing(self.points, self.before, self.after)

    def test_missing_image_hook_is_rejected_against_lifecycle(self):
        self.after["load_attempts"] = 1
        with self.assertRaisesRegex(RuntimeError, "lifecycle coverage"):
            diagnostic_frame_timing(self.points, self.before, self.after)


if __name__ == "__main__":
    unittest.main()
