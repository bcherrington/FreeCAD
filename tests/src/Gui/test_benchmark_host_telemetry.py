# SPDX-License-Identifier: LGPL-2.1-or-later
"""Telemetry ownership, raw-counter and failure-boundary checks (no GUI)."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

from benchmark_host_telemetry import (
    Collector,
    OwnedSession,
    OwnershipError,
    pressure,
    process_stat,
    run,
)


def stat_text(pid=123, parent=100, start=12345):
    fields = ["S", str(parent)] + ["0"] * 20
    fields[11:13] = ["7", "9"]
    fields[19] = str(start)
    fields[21] = "11"
    return f"{pid} (name with ) and spaces) " + " ".join(fields)


class TelemetryTest(unittest.TestCase):
    def test_proc_stat_handles_parentheses_without_logging_comm(self):
        self.assertEqual(
            process_stat(stat_text()),
            {"parent": 100, "start_ticks": 12345, "cpu_ticks": 16, "rss_pages": 11},
        )

    def test_pressure_preserves_some_and_full(self):
        self.assertEqual(
            pressure(
                "some avg10=1.25 avg60=2.00 avg300=0.50 total=123\nfull avg10=0 avg60=0 avg300=0 total=0\n"
            )["some"]["total"],
            123,
        )

    def test_missing_host_sources_are_explicitly_unavailable(self):
        with tempfile.TemporaryDirectory() as directory:
            collector = Collector(proc=Path(directory), sys=Path(directory))
            sample = collector.snapshot()
        self.assertIsNone(sample["memory"])
        self.assertIn("memory", sample["unavailable"])
        self.assertIn("frequency", sample["unavailable"])
        self.assertIn("thermal", sample["unavailable"])
        self.assertEqual(sample["owned_processes"], [])

    def test_cpu_guest_is_not_counted_twice(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "stat").write_text("cpu 1 2 3 4 5 6 7 8 100 200\n")
            sample = Collector(proc=root, sys=root).snapshot()
        self.assertEqual(sum(sample["cpu_ticks"].values()), 36)

    def test_owned_pid_reuse_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "123").mkdir()
            (root / "123/stat").write_text(stat_text(start=999))
            collector = Collector(proc=root, sys=root)
            child = unittest.mock.Mock(pid=123)
            child.poll.return_value = None
            collector.root = child, 12345
            with self.assertRaisesRegex(ValueError, "identity changed"):
                collector.processes()
            with self.assertRaisesRegex(ValueError, "identity changed"):
                collector.snapshot()

    def test_unrelated_process_cannot_be_attached(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "123").mkdir()
            (root / "123/stat").write_text(stat_text(parent=os.getpid() + 10000))
            collector = Collector(proc=root, sys=root)
            with self.assertRaisesRegex(ValueError, "directly owned"):
                collector.attach(unittest.mock.Mock(pid=123))

    def test_fast_exited_command_has_explicitly_unavailable_root(self):
        with tempfile.TemporaryDirectory() as directory:
            child = unittest.mock.Mock(pid=123)
            child.poll.return_value = 7
            collector = Collector(proc=Path(directory))
            self.assertFalse(collector.attach(child))
            self.assertIsNone(collector.root)

    def test_unknown_executable_is_not_a_verified_non_native_process(self):
        with subprocess.Popen([sys.executable, "-c", "import time;time.sleep(5)"]) as child:
            try:
                collector = Collector([Path(sys.executable)])
                collector.attach(child)
                original = Path.stat

                def stat(path, *args, **kwargs):
                    if path == Path(f"/proc/{child.pid}/exe"):
                        raise PermissionError()
                    return original(path, *args, **kwargs)

                with patch.object(Path, "stat", stat):
                    self.assertIsNone(collector.processes()[0]["native_freecad"])
            finally:
                child.terminate()
                child.wait()

    def test_cleanup_identity_change_closes_handle_without_signalling(self):
        session = OwnedSession()
        before = process_stat(stat_text(parent=os.getpid()))
        after = dict(before, start_ticks=before["start_ticks"] + 1)
        with patch.object(Path, "read_text", return_value="unused"), patch(
            "benchmark_host_telemetry.process_stat", side_effect=[before, after]
        ), patch("os.pidfd_open", return_value=1234), patch("os.close") as close, patch(
            "signal.pidfd_send_signal"
        ) as send:
            with self.assertRaises(OwnershipError):
                session.remember(123, os.getpid())
            close.assert_called_once_with(1234)
            send.assert_not_called()
            self.assertFalse(session.handles)

    def test_sidecar_exclusive_creation_precedes_launch(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "existing.jsonl"
            output.write_text("preserve")
            with patch("benchmark_host_telemetry.subprocess.Popen") as launch:
                with self.assertRaises(FileExistsError):
                    run([sys.executable, "-c", "pass"], output, baseline_seconds=0)
                launch.assert_not_called()
            self.assertEqual(output.read_text(), "preserve")

    def test_run_preserves_fast_exit_with_unavailable_root(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "fast.jsonl"
            with patch.object(Collector, "attach", return_value=False):
                self.assertEqual(
                    run([sys.executable, "-c", "raise SystemExit(7)"], output, baseline_seconds=0),
                    7,
                )
            rows = [json.loads(line) for line in output.read_text().splitlines()]
            self.assertFalse(
                next(r for r in rows if r["event"] == "command_started")["root_telemetry_available"]
            )
            self.assertEqual(rows[-1]["event"], "session_finished")
            self.assertTrue(rows[-1]["cleanup_complete"])
            self.assertEqual(rows[-1]["returncode"], 7)

    def test_cleanup_failure_never_ends_with_successful_session(self):
        original = OwnedSession.cleanup

        def fail_after_reaping(session):
            original(session)
            raise RuntimeError("injected cleanup failure")

        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "cleanup-failure.jsonl"
            with patch.object(OwnedSession, "cleanup", fail_after_reaping):
                with self.assertRaisesRegex(RuntimeError, "injected"):
                    run([sys.executable, "-c", "pass"], output, baseline_seconds=0)
            rows = [json.loads(line) for line in output.read_text().splitlines()]
            self.assertEqual(rows[-1]["event"], "cleanup_failed")
            self.assertNotIn("session_finished", [r["event"] for r in rows])

    def test_native_process_uses_executable_identity_and_no_private_argv(self):
        with subprocess.Popen(
            [sys.executable, "-c", "import time;time.sleep(5)", "private-marker"]
        ) as child:
            try:
                collector = Collector([Path(sys.executable)])
                collector.attach(child)
                snapshot = collector.snapshot()
                rows = snapshot["owned_processes"]
                self.assertEqual(len(rows), 1)
                self.assertTrue(rows[0]["native_freecad"])
                self.assertNotIn("private-marker", json.dumps(snapshot))
                self.assertNotIn(str(Path(sys.executable)), json.dumps(snapshot))
            finally:
                child.terminate()
                child.wait()

    def test_timeout_is_bounded_and_sidecar_is_private(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "timeout.jsonl"
            with self.assertRaises(TimeoutError):
                run(
                    [sys.executable, "-c", "import time;time.sleep(10)"],
                    output,
                    baseline_seconds=0,
                    timeout_seconds=0.05,
                )
            rows = [json.loads(line) for line in output.read_text().splitlines()]
            self.assertEqual(rows[-1]["event"], "session_aborted")
            self.assertEqual(output.stat().st_mode & 0o777, 0o600)

    def test_resistant_descendant_is_stopped_after_controller_exits(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            pid_file = root / "owned-child"
            # The child installs its handler before reporting readiness. Its
            # controller then exits, so cleanup cannot rely on waiting for it.
            descendant = (
                "import os,signal,time;from pathlib import Path;"
                "signal.signal(signal.SIGTERM,signal.SIG_IGN);"
                f"Path({str(pid_file)!r}).write_text(str(os.getpid()));"
                "time.sleep(20)"
            )
            controller = (
                "import subprocess,sys,time;from pathlib import Path;"
                f"subprocess.Popen([sys.executable,'-c',{descendant!r}],start_new_session=True);"
                f"p=Path({str(pid_file)!r});"
                "\nwhile not p.exists():time.sleep(0.01)\n"
            )
            self.assertEqual(
                run(
                    [sys.executable, "-c", controller],
                    root / "cleanup.jsonl",
                    baseline_seconds=0,
                    timeout_seconds=5,
                ),
                0,
            )
            pid = int(pid_file.read_text())
            deadline = time.monotonic() + 1
            while (path := Path(f"/proc/{pid}/stat")).exists():
                self.assertLess(time.monotonic(), deadline, "Owned descendant survived cleanup")
                time.sleep(0.01)


if __name__ == "__main__":
    unittest.main()
