#!/usr/bin/env python3
"""D001-scheduled command-only observations, descriptive auxiliary to T020 P07.

These records cannot satisfy a GUI cell or an independent optimization speedup.
The same fresh-process schedule and collector bounds apply without invented GUI
metrics. Freeze this source and each command execution before collection.
"""

import argparse
import hashlib
import subprocess
import importlib.util
import json
import math
import os
from pathlib import Path
import tempfile
import time

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("spec003_runner", HERE / "benchmark-runner.py")
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


def collect(execution_path, output):
    plan = json.loads((HERE / "benchmark-plan.json").read_text())
    execution = json.loads(execution_path.read_text())
    require = runner.require
    digest = runner.digest
    require(execution["schema"] == "freecad-spec003-command-auxiliary/v1", "Wrong auxiliary scope")
    require(execution["collector_sha256"] == digest(Path(__file__)), "Auxiliary collector changed")
    require(execution["protocol_sha256"] == digest(HERE / "benchmark-plan.json"), "Plan changed")
    require(
        execution["original_collector_sha256"] == digest(HERE / "benchmark-runner.py"),
        "Bounded collector changed",
    )
    for kind in ("context", "qualification_receipt"):
        require(
            digest(execution[kind + "_path"]) == execution[kind + "_sha256"],
            "Auxiliary qualification/configuration changed",
        )
    require(digest(execution["fixture_path"]) == execution["fixture_sha256"], "Fixture changed")
    require(
        plan["fixtures"][execution["fixture_id"]]["sha256"] == execution["fixture_sha256"],
        "Unapproved fixture",
    )
    for arm in ("baseline", "change"):
        entry = execution[arm]
        require(digest(entry["binary_path"]) == entry["binary_sha256"], "Command binary changed")
        require(
            digest(entry["build_receipt_path"]) == entry["build_receipt_sha256"],
            "Build receipt changed",
        )
    require(
        execution["protocol_document_sha256"] == digest(HERE / "benchmark-protocol.md"),
        "Protocol document changed",
    )
    require(
        digest(execution["gui_execution_path"]) == execution["gui_execution_sha256"],
        "Referenced GUI freeze changed",
    )
    gui = json.loads(Path(execution["gui_execution_path"]).read_text())
    comparison = next(item for item in plan["comparisons"] if item["id"] == "P07")
    require(
        gui["comparison_id"] == "P07"
        and execution["fixture_id"]
        in comparison["primary_fixture_ids"] + comparison["control_fixture_ids"],
        "Wrong P07 scope",
    )
    for key in (
        "fixture_id",
        "fixture_sha256",
        "comparison_base_commit",
        "performance_delta_sha256",
        "qualification_receipt_sha256",
    ):
        require(execution[key] == gui[key], "Auxiliary/GUI provenance mismatch")
    common = execution["comparison_base_commit"]
    for arm in ("baseline", "change"):
        for key in ("commit", "build_receipt_sha256"):
            require(execution[arm][key] == gui[arm][key], "Auxiliary arm differs from GUI freeze")
        subprocess.run(
            [
                "git",
                "-C",
                str(runner.ROOT),
                "merge-base",
                "--is-ancestor",
                common,
                execution[arm]["commit"],
            ],
            check=True,
        )
    before, after = execution["baseline"]["commit"], execution["change"]["commit"]
    paths = set(runner.git("diff", "--name-only", before, after).splitlines())
    require(
        paths and paths <= set(comparison["source_delta_allowlist"]),
        "Unapproved auxiliary source delta",
    )
    delta = subprocess.check_output(
        ["git", "-C", str(runner.ROOT), "diff", "--binary", before, after]
    )
    require(
        hashlib.sha256(delta).hexdigest() == execution["performance_delta_sha256"],
        "Auxiliary delta changed",
    )
    context = json.loads(Path(execution["context_path"]).read_text())
    require(digest(gui["context_path"]) == gui["context_sha256"], "GUI context changed")
    gui_context = json.loads(Path(gui["context_path"]).read_text())
    require(
        context["namespace_sha256"] == gui_context["namespace_sha256"],
        "Auxiliary namespace differs from GUI freeze",
    )
    for arm in ("baseline", "change"):
        for path, sha in context["runtime_module_sha256"][arm].items():
            if path == "lib/FreeCADGui.so":
                # FreeCADCmd loads the console-safe Python bootstrap to restore
                # Gridfinity proxies. Native FreeCAD registers that module
                # internally, so this DSO is verified against the same arm's
                # qualified build manifest rather than its GUI process maps.
                receipt = json.loads(Path(execution[arm]["build_receipt_path"]).read_text())
                manifest_path = Path(receipt["output_manifest"])
                require(
                    digest(manifest_path) == receipt["output_manifest_sha256"],
                    "Auxiliary build manifest changed",
                )
                manifest = json.loads(manifest_path.read_text())
                require(manifest[path]["sha256"] == sha, "Auxiliary bootstrap module changed")
            elif not path.startswith("bin/"):
                require(
                    gui_context["runtime_module_sha256"][arm].get(path) == sha,
                    "Auxiliary module differs from qualified GUI arm",
                )
    sampling = plan["sampling"]
    deadline = time.monotonic() + sampling["cell_timeout_seconds"]
    with output.open("x") as stream, tempfile.TemporaryDirectory(prefix="spec003-command-") as work:
        stream.write(
            json.dumps(
                {
                    "kind": "command-restore-descriptive-auxiliary",
                    "execution_sha256": digest(execution_path),
                    "protocol_sha256": digest(HERE / "benchmark-plan.json"),
                }
            )
            + "\n"
        )
        stream.flush()
        for pair in runner.schedule(plan):
            for arm in pair["order"]:
                remaining = deadline - time.monotonic()
                require(remaining > 0, "Auxiliary cell timeout")
                result = Path(work) / "result.json"
                result.unlink(missing_ok=True)
                env = dict(
                    os.environ,
                    SPEC003_RESULT_FILE=str(result),
                    SPEC003_ARM=arm,
                    SPEC003_PHASE=pair["phase"],
                    SPEC003_PAIR=str(pair["pair"]),
                )
                log_sha = runner.collect(
                    execution[arm]["command"],
                    env,
                    sampling["worker_stream_limit_bytes"],
                    min(remaining, sampling["worker_timeout_seconds"]),
                )
                require(
                    result.stat().st_size <= sampling["worker_stream_limit_bytes"],
                    "Oversized auxiliary result",
                )
                record = json.loads(result.read_text())
                require(record["comparison_id"] == "P07-command-auxiliary", "Wrong endpoint")
                for key in (
                    "fixture_id",
                    "fixture_sha256",
                    "context_sha256",
                    "normative_state_sha256",
                ):
                    require(record[key] == execution[key], "Auxiliary identity mismatch")
                require(
                    record["commit"] == execution[arm]["commit"]
                    and record["correctness_passed"] is True
                    and record["qualification_only"] is False,
                    "Unqualified command observation",
                )
                require(
                    set(record["metrics"]) == {"document_open_ms", "peak_rss_bytes"},
                    "Wrong auxiliary metrics",
                )
                require(
                    all(
                        type(v) in (int, float) and math.isfinite(v) and v >= 0
                        for v in record["metrics"].values()
                    ),
                    "Invalid auxiliary metrics",
                )
                stream.write(
                    json.dumps({**pair, "arm": arm, "worker_log_sha256": log_sha, "record": record})
                    + "\n"
                )
                stream.flush()
        require(time.monotonic() <= deadline, "Auxiliary cell timeout")
        stream.write(
            json.dumps(
                {
                    "collection_complete": True,
                    "acceptance": "descriptive auxiliary; no GUI or independent speedup acceptance",
                }
            )
            + "\n"
        )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("execution", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    collect(args.execution, args.out)


if __name__ == "__main__":
    main()
