#!/usr/bin/env python3
"""Bounded Spec 003 collector. FreeCAD probes/qualified arms belong to T020/T004."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import selectors
import signal
import subprocess
import sys
import tempfile
import time

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PLAN = HERE / "benchmark-plan.json"


def digest(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def git(*args):
    return subprocess.check_output(["git", "-C", str(ROOT), *args], text=True).strip()


def check(plan):
    require(plan["schema"] == "freecad-spec003-benchmark/v1", "Unsupported schema")
    require(
        digest(HERE / plan["candidate_manifest"]) == plan["candidate_manifest_sha256"],
        "Intake manifest changed",
    )
    require(
        git("rev-parse", plan["intake_revision"] + ":src") == plan["intake_source_tree"],
        "Source anchor mismatch",
    )
    for comparison in plan["comparisons"]:
        for field in ("baseline_origin_commit", "optimized_origin_commit"):
            revision = comparison[field]
            require(git("rev-parse", revision + "^{commit}") == revision, "Missing origin")
            subprocess.run(
                [
                    "git",
                    "-C",
                    str(ROOT),
                    "merge-base",
                    "--is-ancestor",
                    revision,
                    plan["intake_revision"],
                ],
                check=True,
            )
        for name in comparison["primary_fixture_ids"] + comparison["control_fixture_ids"]:
            require(name in plan["fixtures"], "Unknown fixture")
    checked = 0
    for fixture in plan["fixtures"].values():
        if fixture["availability"] == "repository":
            path = ROOT / fixture["path"]
            require(
                path.stat().st_size == fixture["size_bytes"] and digest(path) == fixture["sha256"],
                "Repository fixture changed",
            )
            checked += 1
        elif fixture["availability"] == "planned-generator":
            encoded = json.dumps(fixture["recipe"], sort_keys=True, separators=(",", ":")).encode()
            require(
                hashlib.sha256(encoded).hexdigest() == fixture["recipe_sha256"],
                "Generator recipe changed",
            )
    return {
        "repository_fixtures_checked": checked,
        "comparisons": len(plan["comparisons"]),
        "external_payloads": "hash qualification required before execution",
        "generated_outputs": "not yet materialized",
    }


def schedule(plan):
    s = plan["sampling"]
    phases = (
        ["diagnostic"] * s["first_request_pairs_diagnostic_only"]
        + ["warmup"] * s["warmup_pairs_discarded"]
        + ["measured"] * s["measured_pairs"]
    )
    return [
        {
            "pair": i,
            "phase": phase,
            "order": ["baseline", "change"] if i % 2 == 0 else ["change", "baseline"],
        }
        for i, phase in enumerate(phases)
    ]


def collect(command, environment, limits, timeout):
    """Bound wall time and combined output, including descendant processes on Linux."""
    require(
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
    try:
        while selector.get_map():
            require(time.monotonic() < deadline, "Worker timeout")
            for key, _ in selector.select(min(0.1, max(0, deadline - time.monotonic()))):
                block = os.read(key.fd, 65536)
                if not block:
                    selector.unregister(key.fileobj)
                else:
                    output.extend(block)
                    require(len(output) <= limits, "Worker output limit exceeded")
        require(process.wait(timeout=max(0.001, deadline - time.monotonic())) == 0, "Worker failed")
        return hashlib.sha256(output).hexdigest()
    finally:
        # Also clean up descendants after a normal parent exit.
        try:
            os.killpg(process.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
        process.wait()
        selector.close()
        process.stdout.close()
        process.stderr.close()


def run(plan, execution_path, output_path):
    execution = json.loads(Path(execution_path).read_text())
    require(not output_path.exists(), "Output already exists; preserve prior evidence")
    tasks = (HERE / "tasks.md").read_text()
    for task in plan["source_gate_tasks"]:
        require(
            re.search(r"^- \[x\] " + re.escape(task) + r"\s", tasks, re.M),
            "Source qualification gate incomplete: " + task,
        )
    for field in plan["execution_freeze_required"]:
        require(execution.get(field), "Unfrozen field: " + field)
    require(execution["protocol_sha256"] == digest(PLAN), "Protocol identity mismatch")
    require(
        execution["protocol_document_sha256"] == digest(HERE / "benchmark-protocol.md"),
        "Protocol document changed",
    )
    require(execution["collector_sha256"] == digest(Path(__file__)), "Collector changed")
    comparison = next(c for c in plan["comparisons"] if c["id"] == execution["comparison_id"])
    require(
        execution["fixture_id"]
        in comparison["primary_fixture_ids"] + comparison["control_fixture_ids"],
        "Fixture outside declared scope",
    )
    require(
        digest(execution["fixture_path"]) == execution["fixture_sha256"], "Fixture hash mismatch"
    )
    fixture = plan["fixtures"][execution["fixture_id"]]
    if "sha256" in fixture:
        require(fixture["sha256"] == execution["fixture_sha256"], "Unapproved fixture")
    for field in ("context", "qualification_receipt"):
        require(
            digest(execution[field + "_path"]) == execution[field + "_sha256"],
            field + " receipt changed",
        )
    for arm in ("baseline", "change"):
        receipt = execution[arm]
        require(
            git("rev-parse", receipt["commit"] + "^{commit}") == receipt["commit"],
            "Arm requires exact existing commit",
        )
        require(digest(receipt["binary_path"]) == receipt["binary_sha256"], "Binary changed")
        require(
            digest(receipt["build_receipt_path"]) == receipt["build_receipt_sha256"],
            "Build receipt changed",
        )
    baseline = execution["baseline"]["commit"]
    change = execution["change"]["commit"]
    require(
        git("rev-parse", execution["comparison_base_commit"] + "^{commit}")
        == execution["comparison_base_commit"],
        "Missing exact common base",
    )
    for revision in (baseline, change):
        subprocess.run(
            [
                "git",
                "-C",
                str(ROOT),
                "merge-base",
                "--is-ancestor",
                execution["comparison_base_commit"],
                revision,
            ],
            check=True,
        )
    paths = set(git("diff", "--name-only", baseline, change).splitlines())
    require(
        paths and paths <= set(comparison["source_delta_allowlist"]),
        "Arm difference exceeds the approved source allowlist",
    )
    delta = subprocess.check_output(["git", "-C", str(ROOT), "diff", "--binary", baseline, change])
    require(
        hashlib.sha256(delta).hexdigest() == execution["performance_delta_sha256"],
        "Performance delta changed",
    )
    metrics = (
        comparison["metrics"]
        if execution["fixture_id"] in comparison["primary_fixture_ids"]
        else comparison["control_metrics"]
    )
    if (
        execution["fixture_id"] in comparison["primary_fixture_ids"]
        and "primary_context_metrics" in comparison
    ):
        metrics = comparison["primary_context_metrics"][execution["context"]]
    s = plan["sampling"]
    deadline = time.monotonic() + s["cell_timeout_seconds"]
    # Receipt semantics/source isolation need review; hashes alone do not prove them.
    with output_path.open("x") as out, tempfile.TemporaryDirectory(prefix="spec003-") as tmp:
        out.write(
            json.dumps(
                {
                    "execution_sha256": digest(execution_path),
                    "protocol_sha256": digest(PLAN),
                    "kind": "collection-only",
                }
            )
            + "\n"
        )
        for pair in schedule(plan):
            for arm in pair["order"]:
                remaining = deadline - time.monotonic()
                require(remaining > 0, "Cell timeout")
                result_path = Path(tmp) / "result.json"
                result_path.unlink(missing_ok=True)
                environment = dict(
                    os.environ,
                    SPEC003_RESULT_FILE=str(result_path),
                    SPEC003_ARM=arm,
                    SPEC003_PHASE=pair["phase"],
                    SPEC003_PAIR=str(pair["pair"]),
                )
                log_sha = collect(
                    execution[arm]["command"],
                    environment,
                    s["worker_stream_limit_bytes"],
                    min(remaining, s["worker_timeout_seconds"]),
                )
                require(
                    result_path.stat().st_size <= s["worker_stream_limit_bytes"],
                    "Worker record too large",
                )
                record = json.loads(result_path.read_text())
                for field in (
                    "comparison_id",
                    "fixture_id",
                    "fixture_sha256",
                    "context_sha256",
                    "normative_state_sha256",
                ):
                    require(record[field] == execution[field], "Worker identity mismatch: " + field)
                require(
                    record["commit"] == execution[arm]["commit"]
                    and record["correctness_passed"] is True,
                    "Unqualified worker result",
                )
                require(
                    record["metrics"]
                    and all(
                        type(v) in (int, float) and math.isfinite(v) and v >= 0
                        for v in record["metrics"].values()
                    ),
                    "Invalid metrics",
                )
                require(set(metrics) <= set(record["metrics"]), "Missing metric")
                out.write(
                    json.dumps({**pair, "arm": arm, "worker_log_sha256": log_sha, "record": record})
                    + "\n"
                )
                out.flush()
        require(time.monotonic() <= deadline, "Cell timeout")
        out.write(json.dumps({"collection_complete": True, "acceptance": "not assessed"}) + "\n")


def self_test(plan):
    pairs = schedule(plan)
    require(
        len(pairs) == 24 and sum(p["phase"] == "measured" for p in pairs) == 20, "Schedule mismatch"
    )
    collect([sys.executable, "-c", "print('synthetic')"], dict(os.environ), 1024, 2)
    for command, cap, timeout in [
        ([sys.executable, "-c", "print('x'*2048)"], 32, 2),
        ([sys.executable, "-c", "import time; time.sleep(2)"], 32, 0.1),
        ([sys.executable, "-c", "raise SystemExit(1)"], 32, 2),
    ]:
        try:
            collect(command, dict(os.environ), cap, timeout)
        except ValueError:
            continue
        raise ValueError("Expected bounded collector rejection")
    return "Synthetic schedule/output-cap/timeout/failure checks passed; no FreeCAD samples"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--schedule", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--run", type=Path, metavar="FROZEN_EXECUTION_JSON")
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    plan = json.loads(PLAN.read_text())
    if args.check:
        print(json.dumps(check(plan), indent=2))
    if args.schedule:
        print(json.dumps(schedule(plan), indent=2))
    if args.self_test:
        print(self_test(plan))
    if args.run:
        require(args.out is not None, "--run needs --out")
        check(plan)
        run(plan, args.run, args.out)
    require(args.check or args.schedule or args.self_test or args.run, "Select an operation")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, KeyError, OSError, StopIteration, subprocess.SubprocessError) as error:
        print("Protocol rejected: " + str(error), file=sys.stderr)
        sys.exit(1)
