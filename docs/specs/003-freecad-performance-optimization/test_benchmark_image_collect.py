"""Synthetic failure observability; never produces FreeCAD timing evidence."""

import hashlib
import importlib.util
import json
import os
from pathlib import Path
import signal
import sys
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    "image_collect", Path(__file__).with_name("benchmark-image-collect.py")
)
collector = importlib.util.module_from_spec(spec)
spec.loader.exec_module(collector)


class FailureCaptureTest(unittest.TestCase):
    def setUp(self):
        self.work = tempfile.TemporaryDirectory()
        self.addCleanup(self.work.cleanup)
        self.root = Path(self.work.name)
        self.result = self.root / "result.json"
        self.identity = {"execution_sha256": "a" * 64, "failure_collector_sha256": "b" * 64}
        self.environment = dict(
            os.environ,
            SPEC003_RESULT_FILE=str(self.result),
            SPEC003_PAIR="12",
            SPEC003_ARM="baseline",
            SPEC003_PHASE="measured",
        )

    def collect(self, source, limit=1024, timeout=5):
        return collector.collect(
            [sys.executable, "-c", source],
            self.environment,
            limit,
            timeout,
            self.root,
            self.identity,
        )

    def failure(self):
        paths = list(self.root.glob("failure-*/failure.json"))
        self.assertEqual(len(paths), 1)
        path = paths[0]
        for file in path.parent.iterdir():
            self.assertEqual(file.stat().st_mode & 0o777, 0o600)
        self.assertEqual(path.parent.stat().st_mode & 0o777, 0o700)
        record = json.loads(path.read_text())
        self.assertFalse(record["acceptance_eligible"])
        self.assertFalse(record["collection_complete"])
        self.assertEqual(record["coordinate"]["pair"], "12")
        self.assertEqual(record["run_identity"], self.identity)
        return path.parent, record

    def test_destination_binds_identity_and_cannot_be_reused(self):
        directory = self.root / "new-execution"
        collector.prepare_destination(directory, self.identity)
        self.assertEqual(json.loads((directory / "run.json").read_text()), self.identity)
        with self.assertRaises(FileExistsError):
            collector.prepare_destination(directory, {"execution_sha256": "c" * 64})
        self.assertEqual(json.loads((directory / "run.json").read_text()), self.identity)

    def test_success_hash_matches_original_and_writes_no_diagnostic(self):
        source = "import sys;sys.stdout.write('ordinary output')"
        expected = collector.runner.collect
        command = [sys.executable, "-c", source]
        self.assertEqual(self.collect(source), expected(command, self.environment, 1024, 5))
        self.assertEqual(list(self.root.iterdir()), [])

    def test_nonzero_preserves_output_and_result_before_cleanup(self):
        source = (
            "import os,pathlib,sys;"
            "pathlib.Path(os.environ['SPEC003_RESULT_FILE']).write_text('{\"correctness_passed\":false}');"
            "sys.stderr.write('private diagnostic');sys.exit(7)"
        )
        with self.assertRaisesRegex(ValueError, "Worker failed"):
            self.collect(source)
        directory, record = self.failure()
        self.assertEqual(record["category"], "worker-exit")
        self.assertEqual(record["returncode"], 7)
        self.assertEqual((directory / "combined-output.bin").read_bytes(), b"private diagnostic")
        self.assertEqual((directory / "worker-result.bin").read_bytes(), self.result.read_bytes())
        self.assertEqual(
            record["stream"]["sha256"], hashlib.sha256(b"private diagnostic").hexdigest()
        )
        self.assertNotIn("private diagnostic", json.dumps(record))

    def test_signal_is_retained(self):
        with self.assertRaisesRegex(ValueError, "Worker failed"):
            self.collect("import os,signal;os.kill(os.getpid(),signal.SIGTERM)")
        _, record = self.failure()
        self.assertEqual(record["returncode"], -signal.SIGTERM)
        self.assertEqual(record["signal"], signal.SIGTERM)

    def test_timeout_retains_bounded_partial_output(self):
        with self.assertRaisesRegex(ValueError, "Worker timeout"):
            self.collect(
                "import time;print('before timeout',flush=True);time.sleep(5)", timeout=0.5
            )
        directory, record = self.failure()
        self.assertEqual(record["category"], "timeout")
        self.assertEqual((directory / "combined-output.bin").read_bytes(), b"before timeout\n")

    def test_output_cap_preserves_prefix_without_unbounded_file(self):
        with self.assertRaisesRegex(ValueError, "Worker output limit exceeded"):
            self.collect("import sys;sys.stdout.write('x'*100000)", limit=32)
        directory, record = self.failure()
        self.assertEqual(record["category"], "output-cap")
        self.assertTrue(record["stream_truncated"])
        self.assertEqual((directory / "combined-output.bin").stat().st_size, 32)


if __name__ == "__main__":
    unittest.main()
