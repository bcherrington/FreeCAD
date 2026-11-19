# SPDX-License-Identifier: LGPL-2.1-or-later

import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from typer.testing import CliRunner

from auriora_dev import cli


class IntegrationUpdateTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        self.source = root / "source"
        self.repo = root / "integration"
        self.source.mkdir()
        self.git(self.source, "init", "-b", "main")
        self.configure(self.source)
        (self.source / "upstream.txt").write_text("initial\n")
        self.git(self.source, "add", ".")
        self.git(self.source, "commit", "-m", "Initial upstream")
        self.git(root, "clone", str(self.source), str(self.repo))
        self.configure(self.repo)
        self.initial = self.git(self.repo, "rev-parse", "HEAD")
        state = {"base": "origin/main", "branches": []}
        (self.repo / ".git/personal-integration-branches.json").write_text(json.dumps(state))
        (self.source / "upstream.txt").write_text("updated\n")
        self.git(self.source, "commit", "-am", "New upstream change")
        self.latest = self.git(self.source, "rev-parse", "HEAD")
        self.addCleanup(patch.stopall)
        patch.object(cli, "_repo_root", return_value=self.repo).start()
        self.runner = CliRunner()

    def git(self, root, *args):
        return subprocess.run(
            ["git", *args], cwd=root, text=True, check=True, capture_output=True
        ).stdout.strip()

    def configure(self, root):
        self.git(root, "config", "user.name", "Integration test")
        self.git(root, "config", "user.email", "integration@example.invalid")
        self.git(root, "config", "commit.gpgsign", "false")
        self.git(root, "config", "core.hooksPath", str(Path(self.temp.name) / "no-hooks"))

    def test_default_fetches_and_merges_new_upstream_commit(self):
        result = self.runner.invoke(cli.app, ["integration", "update"])
        self.assertEqual(result.exit_code, 0, result.output)
        self.assertEqual(self.git(self.repo, "rev-parse", "HEAD"), self.latest)

    def test_no_fetch_uses_existing_remote_ref(self):
        result = self.runner.invoke(cli.app, ["integration", "update", "--no-fetch"])
        self.assertEqual(result.exit_code, 0, result.output)
        self.assertEqual(self.git(self.repo, "rev-parse", "HEAD"), self.initial)
        self.assertEqual(self.git(self.repo, "rev-parse", "origin/main"), self.initial)

    def test_fetch_failure_stops_before_merge(self):
        self.git(self.repo, "remote", "set-url", "origin", str(self.repo / "missing"))
        result = self.runner.invoke(cli.app, ["integration", "update"])
        self.assertNotEqual(result.exit_code, 0)
        self.assertEqual(self.git(self.repo, "rev-parse", "HEAD"), self.initial)

    def test_dirty_worktree_stops_before_fetch(self):
        (self.repo / "uncommitted.txt").write_text("local work\n")
        result = self.runner.invoke(cli.app, ["integration", "update"])
        self.assertNotEqual(result.exit_code, 0)
        self.assertEqual(self.git(self.repo, "rev-parse", "origin/main"), self.initial)


if __name__ == "__main__":
    unittest.main()
