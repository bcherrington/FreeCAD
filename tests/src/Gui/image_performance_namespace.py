#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Bind a qualified physical arm to its configured canonical build/source paths.

Copied common build artifacts are reused only with verified identical inputs and
an actual incremental compile/relink. This wrapper never edits Ninja receipts.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def digest(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--physical-source", type=Path, required=True)
    parser.add_argument("--canonical-source", type=Path, required=True)
    parser.add_argument("--cache-dir", type=Path, required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--build-write", action="store_true")
    parser.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    command = args.command[1:] if args.command[:1] == ["--"] else args.command
    if not command:
        parser.error("a command argv is required")
    physical = args.physical_source.resolve(strict=True)
    canonical = args.canonical_source.resolve(strict=True)
    cache = args.cache_dir.resolve(strict=True)
    receipt = {
        "schema": "freecad-spec003-namespace/v1",
        "physical_source": str(physical),
        "canonical_source": str(canonical),
        "build_read_only": not args.build_write,
        "expected_commit": args.commit,
        "cache_dir": str(cache),
        "mesa_shader_cache": str(cache / "mesa"),
        "xdg_cache": str(cache / "xdg"),
        "bwrap_sha256": digest(Path("/usr/bin/bwrap")),
        "bwrap_version": subprocess.check_output(
            ["/usr/bin/bwrap", "--version"], text=True
        ).strip(),
        "wrapper_sha256": digest(Path(__file__)),
        "temporary_storage": "/tmp (host temporary filesystem; fresh worker directories)",
    }
    encoded = json.dumps(receipt, sort_keys=True, separators=(",", ":"))
    env = os.environ.copy()
    env["SPEC003_NAMESPACE_RECEIPT"] = encoded
    env["SPEC003_NAMESPACE_SHA256"] = hashlib.sha256(encoded.encode()).hexdigest()
    env["GIT_OPTIONAL_LOCKS"] = "0"
    env["MESA_SHADER_CACHE_DIR"] = str(cache / "mesa")
    env["XDG_CACHE_HOME"] = str(cache / "xdg")
    # No shell and no private argv/fixture locators are printed by this wrapper.
    checker = (
        "import os,subprocess,sys;"
        "root=sys.argv[1];expected=sys.argv[2];"
        "actual=subprocess.check_output(['git','-C',root,'rev-parse','HEAD'],text=True).strip();"
        "dirty=subprocess.check_output(['git','-C',root,'status','--porcelain','--untracked-files=no']);"
        "assert actual==expected and not dirty, 'Unqualified namespace source';"
        "os.execvpe(sys.argv[3],sys.argv[3:],os.environ)"
    )
    argv = [
        "/usr/bin/bwrap",
        "--die-with-parent",
        "--ro-bind",
        "/",
        "/",
        "--dev-bind",
        "/dev",
        "/dev",
        "--proc",
        "/proc",
        "--bind",
        "/tmp",
        "/tmp",
        "--bind" if args.build_write else "--ro-bind",
        str(physical),
        str(canonical),
        "--bind",
        str(cache),
        str(cache),
        "--chdir",
        str(canonical),
        "--",
        os.sys.executable,
        "-c",
        checker,
        str(canonical),
        args.commit,
        *command,
    ]
    return subprocess.run(argv, env=env).returncode


if __name__ == "__main__":
    raise SystemExit(main())
