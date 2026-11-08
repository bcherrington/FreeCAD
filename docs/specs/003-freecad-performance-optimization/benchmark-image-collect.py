#!/usr/bin/env python3
"""D001 image collector extension: preserve bounded diagnostics after failure.

The historical collector, schedule, thresholds, worker argv/environment and
successful stream-drain path stay unchanged. A new execution must freeze this
extension separately. This is not permission to retry an aborted collection.
"""

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import selectors
import signal
import stat
import subprocess
import time

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("image_original_runner", HERE / "benchmark-runner.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


def preserve_failure(directory, environment, output, limit, process, category, identity):
    """Called only after process cleanup; raw diagnostic contents stay local."""
    coordinate = {
        "arm": environment.get("SPEC003_ARM"),
        "phase": environment.get("SPEC003_PHASE"),
        "pair": environment.get("SPEC003_PAIR"),
    }
    target = directory / ("failure-" + str(time.time_ns()))
    target.mkdir(mode=0o700)

    def write(name, data):
        path = target / name
        descriptor = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(data)
        return {"sha256": hashlib.sha256(data).hexdigest(), "size_bytes": len(data)}

    record = {
        "schema": "freecad-spec003-worker-failure/v1",
        "collection_complete": False,
        "acceptance_eligible": False,
        "run_identity": identity,
        "coordinate": coordinate,
        "category": category,
        "returncode": process.returncode,
        "signal": (
            -process.returncode
            if process.returncode is not None and process.returncode < 0
            else None
        ),
        "captured_stream_bytes": len(output),
        "captured_stream_sha256": hashlib.sha256(output).hexdigest(),
        "stream": write("combined-output.bin", bytes(output[:limit])),
        "stream_truncated": len(output) > limit,
        "worker_result": None,
    }
    result = environment.get("SPEC003_RESULT_FILE")
    if result and Path(result).is_file():
        with Path(result).open("rb") as stream:
            data = stream.read(limit + 1)
        record["worker_result"] = write("worker-result.bin", data[:limit])
        record["worker_result_truncated"] = len(data) > limit
    write("failure.json", (json.dumps(record, indent=2) + "\n").encode())


def collect(command, environment, limits, timeout, directory, identity):
    """Same bounded drain/cleanup as the original; persist only after failure."""
    runner.require(
        isinstance(command, list) and command and all(isinstance(arg, str) for arg in command),
        "Command must be an argv array",
    )
    process = subprocess.Popen(
        command,
        env=environment,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        start_new_session=True,
    )
    selector = selectors.DefaultSelector()
    selector.register(process.stdout, selectors.EVENT_READ)
    selector.register(process.stderr, selectors.EVENT_READ)
    output = bytearray()
    deadline = time.monotonic() + timeout
    category = "worker-exit"
    try:
        try:
            while selector.get_map():
                category = "timeout"
                runner.require(time.monotonic() < deadline, "Worker timeout")
                for key, _ in selector.select(min(0.1, max(0, deadline - time.monotonic()))):
                    block = os.read(key.fd, 65536)
                    if not block:
                        selector.unregister(key.fileobj)
                    else:
                        output.extend(block)
                        category = "output-cap"
                        runner.require(len(output) <= limits, "Worker output limit exceeded")
            category = "timeout"
            returncode = process.wait(timeout=max(0.001, deadline - time.monotonic()))
            category = "worker-exit"
            runner.require(returncode == 0, "Worker failed")
            return hashlib.sha256(output).hexdigest()
        finally:
            # Exactly the original descendant cleanup, including normal exit.
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            process.wait()
            selector.close()
            process.stdout.close()
            process.stderr.close()
    except BaseException as error:
        if isinstance(error, KeyboardInterrupt):
            category = "interrupted"
        elif isinstance(error, OSError):
            category = "collector-io"
        preserve_failure(directory, environment, output, limits, process, category, identity)
        raise


def prepare_destination(directory, identity):
    """A frozen execution gets one exclusive destination, including failed starts."""
    directory.mkdir(mode=0o700)
    info = directory.stat()
    runner.require(
        info.st_uid == os.getuid() and stat.S_IMODE(info.st_mode) & 0o077 == 0,
        "Diagnostics directory must be owner-only",
    )
    descriptor = os.open(directory / "run.json", os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
    with os.fdopen(descriptor, "w") as stream:
        json.dump(identity, stream, indent=2)
        stream.write("\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("execution", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--diagnostics-dir", type=Path, required=True)
    args = parser.parse_args()
    execution = json.loads(args.execution.read_text())
    runner.require(
        execution["failure_collector_sha256"] == runner.digest(Path(__file__)),
        "Failure collector changed",
    )
    runner.require(
        execution["failure_diagnostics_dir"] == str(args.diagnostics_dir.resolve()),
        "Unfrozen diagnostics destination",
    )
    runner.require(not args.out.exists(), "Output already exists; preserve prior evidence")
    identity = {
        "schema": "freecad-spec003-diagnostic-run/v1",
        "execution_sha256": runner.digest(args.execution),
        "failure_collector_sha256": runner.digest(Path(__file__)),
        "original_collector_sha256": runner.digest(HERE / "benchmark-runner.py"),
        "context_sha256": execution["context_sha256"],
        "qualification_receipt_sha256": execution["qualification_receipt_sha256"],
    }
    prepare_destination(args.diagnostics_dir, identity)
    runner.collect = lambda command, environment, limits, timeout: collect(
        command, environment, limits, timeout, args.diagnostics_dir, identity
    )
    plan = json.loads(runner.PLAN.read_text())
    runner.check(plan)
    runner.run(plan, args.execution, args.out)


if __name__ == "__main__":
    main()
