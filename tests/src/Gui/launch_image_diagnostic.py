#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Launch the frozen P07 controller inside its required host telemetry session."""

import argparse
import json
import os
from pathlib import Path
import signal
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))

import benchmark_host_telemetry as telemetry
from run_image_diagnostic import digest, require, write


def require_isolated_host():
    require(
        sys.flags.no_site == 1 and sys.flags.isolated == 1 and sys.flags.dont_write_bytecode == 1,
        "Telemetry requires isolated host Python -I -S -B",
    )


def launch(path, sha256):
    require_isolated_host()
    os.umask(0o077)
    require(digest(path) == sha256, "Diagnostic freeze changed")
    freeze = json.loads(path.read_text())
    config = freeze["telemetry"]
    require(freeze["acceptance_eligible"] is False, "Diagnostic cannot accept performance")
    require(
        digest(Path(__file__)) == freeze["file_bindings"][str(Path(__file__).resolve())],
        "Telemetry launcher changed",
    )
    require(
        Path(sys.executable).resolve() == Path(config["python"]).resolve(),
        "Wrong telemetry interpreter",
    )
    require(
        callable(getattr(os, "pidfd_open", None))
        and callable(getattr(signal, "pidfd_send_signal", None)),
        "Telemetry interpreter lacks Linux pidfd support",
    )
    descriptor = os.pidfd_open(os.getpid())
    try:
        signal.pidfd_send_signal(descriptor, 0)
    finally:
        os.close(descriptor)
    expected = [
        str(Path(freeze["arms"][arm]["build"]) / "bin/FreeCAD") for arm in ("baseline", "change")
    ]
    require(config["native_executables"] == expected, "Telemetry native executable binding changed")
    require(
        config["baseline_seconds"] == 30
        and config["timeout_seconds"] == 1200
        and config["interval_seconds"] == 1,
        "Telemetry limits changed",
    )
    art = Path(freeze["artifact_directory"])
    require(
        Path(config["output"]) == art / "diagnostic-host-trace.jsonl",
        "Telemetry destination changed",
    )
    require(
        digest(Path(config["controller"])) == freeze["controller_sha256"],
        "Diagnostic controller changed",
    )
    for name in expected:
        require(
            digest(Path(name)) == freeze["file_bindings"][name],
            "Telemetry native executable changed",
        )
    require(
        not Path(config["output"]).exists() and not (art / "diagnostic-status.json").exists(),
        "Diagnostic launch already attempted",
    )
    command = [
        config["python"],
        "-I",
        "-S",
        "-B",
        config["controller"],
        str(path.resolve()),
        "--sha256",
        sha256,
    ]
    write(
        art / "diagnostic-launch.json",
        {
            "freeze_sha256": sha256,
            "launcher_sha256": digest(Path(__file__)),
            "telemetry": config,
            "controller_argv": command,
            "acceptance_eligible": False,
        },
        exclusive=True,
    )
    os.environ["SPEC003_DIAGNOSTIC_TELEMETRY_FREEZE_SHA256"] = sha256
    os.environ["SPEC003_DIAGNOSTIC_TELEMETRY_OUTPUT"] = config["output"]
    return telemetry.run(
        command,
        Path(config["output"]),
        [Path(p) for p in expected],
        baseline_seconds=30,
        timeout_seconds=1200,
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("freeze", type=Path)
    parser.add_argument("--sha256", required=True)
    args = parser.parse_args()

    def terminate(signum, _frame):
        raise SystemExit(128 + signum)

    signal.signal(signal.SIGTERM, terminate)
    return launch(args.freeze, args.sha256)


if __name__ == "__main__":
    raise SystemExit(main())
