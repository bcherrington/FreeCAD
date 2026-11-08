#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Run the frozen, non-repeatable 16-worker P07 diagnostic; no acceptance assessor."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import selectors
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))

from benchmark_host_telemetry import OwnedSession


def require(value, message):
    if not value:
        raise ValueError(message)


def digest(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def write(path, value, exclusive=False):
    with path.open("x" if exclusive else "w") as stream:
        json.dump(value, stream, indent=2)
        stream.write("\n")
    path.chmod(0o600)


def validate_schedule(schedule, cells):
    expected_cells = {
        "hidden": ("gridfinity-drawer-v1.3", "hidden-first-show-repeat", 0),
        "visible": ("gridfinity-drawer-v1.3", "open-only", 1),
        "part": ("PartDesignExample", "open-only", -1),
        "vrml": ("TestVRMLTextures", "open-only", -1),
    }
    require(set(cells) == set(expected_cells), "Diagnostic cell table changed")
    for cell, expected_identity in expected_cells.items():
        item = cells[cell]
        require(digest(Path(item["context"])) == item["context_sha256"], "Cell context changed")
        context = json.loads(Path(item["context"]).read_text())
        require(
            (context["fixture_id"], context["scenario"], context["initial_image_visibility"])
            == expected_identity,
            "Diagnostic cell identity changed",
        )
        require(
            context["fixture_id"] == item["fixture_id"]
            and context["fixture_sha256"] == item["fixture_sha256"],
            "Cell fixture binding changed",
        )
    expected = [
        (cell, arm)
        for cell in ("hidden", "visible", "part", "vrml")
        for arm in ("baseline", "change", "change", "baseline")
    ]
    require(
        [(row["cell"], row["arm"]) for row in schedule] == expected,
        "Diagnostic schedule must be the exact fixed AB/BA four-context schedule",
    )
    require(
        all(row["context_sha256"] == cells[row["cell"]]["context_sha256"] for row in schedule),
        "Schedule context differs from cell identity",
    )
    require(len({row["result"] for row in schedule}) == 16, "Worker result destinations overlap")
    require(
        all(not Path(row["result"]).exists() for row in schedule), "Worker output already exists"
    )


def cache_inventory(root):
    return {
        p.relative_to(root).as_posix(): digest(p)
        for directory in ("mesa", "xdg")
        for p in (root / directory).rglob("*")
        if p.is_file()
    }


def audit(freeze, initial=False):
    checked = 0
    for name, expected in freeze["file_bindings"].items():
        require(digest(Path(name)) == expected, "Frozen diagnostic input/output changed")
        checked += 1
    for arm in ("baseline", "change"):
        item = freeze["arms"][arm]
        source = item["source"]
        build = Path(item["build"])
        runtime_files = sorted(
            str(p)
            for directory in ("bin", "lib", "Mod", "Ext", "share", "data")
            for p in (build / directory).rglob("*")
            if p.is_file()
        )
        require(runtime_files == item["runtime_inventory"], "Runtime file inventory changed")
        require(
            set(runtime_files) <= set(freeze["file_bindings"]),
            "Runtime inventory contains an unbound file",
        )
        require(
            subprocess.check_output(["git", "-C", source, "rev-parse", "HEAD"], text=True).strip()
            == item["commit"],
            "Arm source revision changed",
        )
        require(
            not subprocess.check_output(
                ["git", "-C", source, "status", "--porcelain", "--untracked-files=all"]
            ),
            "Arm source has unqualified files",
        )
        for dep in freeze["dependencies"]:
            directory = str(Path(source) / dep["path"])
            require(
                subprocess.check_output(
                    ["git", "-C", directory, "rev-parse", "HEAD"], text=True
                ).strip()
                == dep["commit"],
                "Dependency source changed",
            )
            require(
                not subprocess.check_output(
                    ["git", "-C", directory, "status", "--porcelain", "--untracked-files=all"]
                ),
                "Dependency source is dirty",
            )
    identity = json.loads(Path(freeze["environment_identity"]).read_text())
    expected = identity["installed_packages"]
    keys = list(expected[0])
    installed = []
    for path in Path(freeze["conda_meta_directory"]).glob("*.json"):
        value = json.loads(path.read_text())
        installed.append({key: value.get(key) for key in keys})
    installed.sort(key=lambda value: value["name"])
    require(installed == expected, "Installed build/runtime environment changed")
    cache_states = {}
    for arm in ("baseline", "change"):
        expected_cache = freeze["initial_caches"][arm]
        current = cache_inventory(Path(expected_cache["root"]))
        if initial:
            require(current == expected_cache["files"], "Initial runtime cache state changed")
        cache_states[arm] = {"file_count": len(current), "files": current}
    return {
        "passed": True,
        "checked_hashes": checked,
        "monotonic_ns": time.monotonic_ns(),
        "runtime_cache_states": cache_states,
    }


def collect(command, environment, timeout, output_path):
    """Bounded drain; exact owned descendants cleaned across nested sessions."""
    owner = OwnedSession()
    owner.start()
    child = None
    selector = selectors.DefaultSelector()
    output = bytearray()
    try:
        child = subprocess.Popen(
            command,
            env=environment,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            start_new_session=True,
        )
        selector.register(child.stdout, selectors.EVENT_READ)
        selector.register(child.stderr, selectors.EVENT_READ)
        deadline = time.monotonic() + timeout
        while selector.get_map():
            require(time.monotonic() < deadline, "Worker execution timeout")
            for key, _ in selector.select(min(0.1, max(0, deadline - time.monotonic()))):
                block = os.read(key.fd, min(65536, 262145 - len(output)))
                if not block:
                    selector.unregister(key.fileobj)
                else:
                    output.extend(block)
                    require(len(output) <= 262144, "Worker stream cap exceeded")
        code = child.wait(timeout=max(0.001, deadline - time.monotonic()))
        require(code == 0, "Worker exited unsuccessfully")
        return hashlib.sha256(output).hexdigest()
    finally:
        try:
            if child is not None:
                reaped = owner.cleanup()
                if child.returncode is None:
                    child.returncode = reaped.get(child.pid)
                    child.wait(timeout=0)
        finally:
            selector.close()
            if child is not None:
                child.stdout.close()
                child.stderr.close()
            try:
                owner.close()
            finally:
                output_path.write_bytes(output[:262144])
                output_path.chmod(0o600)


def run(path, sha256):
    require(
        sys.flags.no_site == 1 and sys.flags.isolated == 1 and sys.flags.dont_write_bytecode == 1,
        "Controller requires isolated host Python -I -S -B",
    )
    # This entrypoint is an isolated, single-threaded controller, not a library
    # embedded in a user's process. All descendants inherit private creation.
    os.umask(0o077)
    require(digest(path) == sha256, "Diagnostic freeze identity changed")
    freeze = json.loads(path.read_text())
    require(freeze["schema"] == "freecad-spec003-p07-diagnostic-freeze/v1", "Unknown freeze schema")
    require(freeze["acceptance_eligible"] is False, "Diagnostic cannot authorize acceptance")
    require(digest(Path(__file__)) == freeze["controller_sha256"], "Diagnostic controller changed")
    validate_schedule(freeze["schedule"], freeze["cells"])
    require(
        os.environ.get("SPEC003_DIAGNOSTIC_TELEMETRY_FREEZE_SHA256") == sha256,
        "Required telemetry launcher was bypassed",
    )
    trace_path = Path(freeze["telemetry"]["output"])
    require(
        os.environ.get("SPEC003_DIAGNOSTIC_TELEMETRY_OUTPUT") == str(trace_path),
        "Wrong telemetry session",
    )
    require(
        trace_path.is_file() and trace_path.stat().st_mode & 0o777 == 0o600,
        "Private telemetry trace missing",
    )
    with trace_path.open() as telemetry_stream:
        header = json.loads(telemetry_stream.readline())
    require(
        header["schema"] == "freecad-benchmark-host-telemetry/v1"
        and header["baseline_seconds"] == 30
        and header["timeout_seconds"] == 1200
        and header["interval_seconds"] == 1
        and header["native_executable_count"] == 2,
        "Telemetry session profile changed",
    )
    art = Path(freeze["artifact_directory"])
    status = art / "diagnostic-status.json"
    write(status, {"status": "started", "acceptance_eligible": False}, exclusive=True)
    records = []
    deadline = time.monotonic() + 1200
    try:
        before = audit(freeze, initial=True)
        write(art / "diagnostic-audit-before.json", before, exclusive=True)
        with (art / "diagnostic-observations.jsonl").open("x") as stream:
            os.fchmod(stream.fileno(), 0o600)
            for index, entry in enumerate(freeze["schedule"]):
                require(time.monotonic() < deadline, "Diagnostic session deadline exceeded")
                started = time.monotonic_ns()
                env = dict(
                    os.environ,
                    PYTHONOPTIMIZE="0",
                    PYTHONDONTWRITEBYTECODE="1",
                    PYTHONNOUSERSITE="1",
                    SPEC003_RESULT_FILE=entry["result"],
                    SPEC003_ARM=entry["arm"],
                )
                raw_path = art / (f"worker-{index:02d}-output.bin")
                output_sha = collect(
                    entry["command"], env, min(60, deadline - time.monotonic()), raw_path
                )
                result_path = Path(entry["result"])
                result_path.chmod(0o600)
                require(result_path.stat().st_size <= 262144, "Worker JSON cap exceeded")
                result = json.loads(result_path.read_text())
                require(
                    result.get("correctness_passed") is True
                    and result.get("qualification_only") is False
                    and result.get("diagnostic_only") is True
                    and result.get("acceptance_eligible") is False,
                    "Worker correctness/diagnostic qualification failed",
                )
                require(
                    result["commit"] == freeze["arms"][entry["arm"]]["commit"],
                    "Worker ran different source",
                )
                require(
                    result["context_sha256"] == entry["context_sha256"],
                    "Worker used different context",
                )
                cell = freeze["cells"][entry["cell"]]
                require(
                    result["fixture_id"] == cell["fixture_id"]
                    and result["fixture_sha256"] == cell["fixture_sha256"],
                    "Worker fixture differs from frozen cell",
                )
                record = {
                    "ordinal": index,
                    "cell": entry["cell"],
                    "arm": entry["arm"],
                    "started_monotonic_ns": started,
                    "finished_monotonic_ns": time.monotonic_ns(),
                    "output_sha256": output_sha,
                    "result_sha256": digest(result_path),
                    "result": result,
                }
                stream.write(json.dumps(record, sort_keys=True) + "\n")
                stream.flush()
                records.append(
                    {
                        "ordinal": index,
                        "cell": entry["cell"],
                        "arm": entry["arm"],
                        "correctness_passed": True,
                    }
                )
                write(
                    status,
                    {"status": "in_progress", "workers": records, "acceptance_eligible": False},
                )
                print(
                    "Diagnostic worker",
                    index + 1,
                    "/16",
                    entry["cell"],
                    entry["arm"],
                    "passed",
                    flush=True,
                )
        after = audit(freeze)
        write(art / "diagnostic-audit-after.json", after, exclusive=True)
        write(
            status,
            {
                "status": "complete",
                "workers": records,
                "acceptance_eligible": False,
                "retries_replacements": 0,
            },
        )
    except BaseException as error:
        write(
            status,
            {
                "status": "aborted; do not resume or retry",
                "workers": records,
                "failed_ordinal": len(records),
                "error_type": type(error).__name__,
                "acceptance_eligible": False,
                "retries_replacements": 0,
            },
        )
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("freeze", type=Path)
    parser.add_argument("--sha256", required=True)
    args = parser.parse_args()
    run(args.freeze, args.sha256)


if __name__ == "__main__":
    main()
