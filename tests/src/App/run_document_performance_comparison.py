#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-2.1-or-later
"""Frozen, bounded P02/P03 collector; original statistics and D007 review stay separate."""

import argparse
import json
import math
import os
from pathlib import Path
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "Gui"))
import run_image_conversion_comparison as shared
from run_image_diagnostic import collect, digest, require

CELLS = {
    "P02": ["gridfinity-drawer-v1.3", "ProjectTest", "PartDesignExample"],
    "P03": ["BIMExample", "ArchDetail", "EngineBlock", "PartDesignExample", "draft_test_objects"],
}
IDENTITY_KEYS = [
    "normative_state_sha256",
    "geometry_manifest_sha256",
    "reopened_geometry_manifest_sha256",
    "payload_identity_sha256",
    "persistent_state_sha256",
    "persistent_state_inventory",
    "coverage",
    "completion_recompute_count",
    "reopened_recompute_count",
    "reopened_normative_state_sha256",
    "reopened_persistent_state_sha256",
]


def identity(row):
    return {key: row[key] for key in IDENTITY_KEYS}


def schedule(comparison):
    require(comparison in CELLS, "Unknown document comparison")
    return [
        (cell, pair, arm, "diagnostic" if pair == 0 else "warmup" if pair < 4 else "measured")
        for cell in CELLS[comparison]
        for pair in range(24)
        for arm in (["baseline", "change"] if pair % 2 == 0 else ["change", "baseline"])
    ]


def worker_command(freeze, cell, arm, context=None, qualify=False):
    item = freeze["arms"][arm]
    canonical = freeze["canonical_source"]
    command = [
        freeze["native_python"],
        "-I",
        "-S",
        "-B",
        freeze["namespace_wrapper"],
        "--isolated-python",
        "--physical-source",
        item["source"],
        "--canonical-source",
        canonical,
        "--cache-dir",
        freeze["initial_caches"][arm]["root"],
        "--commit",
        item["commit"],
        "--",
        freeze["native_python"],
        "-I",
        "-S",
        "-B",
        freeze["launcher"],
        "--build-dir",
        canonical + "/build/relWithDebInfo",
        "--source-root",
        canonical,
        "--harness-root",
        freeze["project_root"],
        "--context",
        context or freeze["cells"][cell]["context"],
        "--fixture",
        freeze["cells"][cell]["fixture"],
        "--gridfinity-addon",
        freeze["gridfinity_addon"],
        "--command-only",
        "--comparison",
        freeze["comparison"],
        "--commit",
        item["commit"],
    ]
    return command + (["--qualify"] if qualify else [])


def result_path(freeze, ordinal, arm):
    return Path(freeze["initial_caches"][arm]["root"]) / f"result-{ordinal:04d}.json"


def validate_destination(freeze, ordinal, arm):
    cache = Path(freeze["initial_caches"][arm]["root"])
    path = result_path(freeze, ordinal, arm)
    require(cache.is_dir() and not cache.is_symlink(), "Invalid cache")
    require(cache.stat().st_mode & 0o777 == 0o700, "Cache must be private")
    require(path.parent.resolve() == cache.resolve(), "Result escaped cache")
    require(not path.exists() and not path.is_symlink(), "Worker result already exists")
    return path


def audit(freeze, initial=False):
    result = shared.base_audit(freeze, initial=False)
    states = {}
    for arm, cache in freeze["initial_caches"].items():
        files = shared.full_cache_inventory(cache["root"])
        if initial:
            require(files == cache["files"], "Initial complete writable state changed")
        states[arm] = {"file_count": len(files), "files": files}
    result["runtime_cache_states"] = states
    return result


def validate_cells(freeze):
    comparison = freeze["comparison"]
    require(
        comparison in CELLS and list(freeze["cells"]) == CELLS[comparison], "Cell set/order changed"
    )
    for cell, config in freeze["cells"].items():
        require(digest(Path(config["fixture"])) == config["fixture_sha256"], "Fixture changed")
        context_data = Path(config["context"]).read_bytes()
        import hashlib

        require(
            hashlib.sha256(context_data).hexdigest() == config["context_sha256"], "Context changed"
        )
        context = json.loads(context_data)
        require(
            context["fixture_id"] == cell and context["fixture_sha256"] == config["fixture_sha256"],
            "Context identity changed",
        )
        require(set(config["qualified"]) == {"baseline", "change"}, "Missing arm qualification")
        a, b = (config["qualified"][arm] for arm in ["baseline", "change"])
        require(
            a["identity"] == b["identity"] == context["document_identity"],
            "Arm model/coverage differs",
        )
        coverage = a["identity"]["coverage"]
        if comparison == "P03":
            require(
                coverage["map_calls"] > 0
                and coverage["map_records"] > 0
                and coverage["token_buffers"] > 0,
                "Uncovered parser fixture",
            )
        elif cell == "gridfinity-drawer-v1.3":
            require(
                coverage["included_properties"] == 3 and coverage["included_bytes"] == 57246043,
                "Uncovered save primary",
            )
        else:
            require(
                coverage["included_properties"] == coverage["included_bytes"] == 0,
                "Zero-payload control changed",
            )
            require(
                config["save_payload_applicability"] == "structurally-inapplicable-zero-payload",
                "Undeclared control inapplicability",
            )


def validate_row(freeze, row, cell, arm):
    expected = freeze["cells"][cell]["qualified"][arm]
    require(
        row["correctness_passed"] is True and row["qualification_only"] is False,
        "Unqualified worker",
    )
    require(
        row["comparison_id"] == freeze["comparison"] and row["fixture_id"] == cell,
        "Wrong worker cell",
    )
    require(row["commit"] == freeze["arms"][arm]["commit"], "Wrong source")
    require(identity(row) == expected["identity"], "Model/property/coverage identity changed")
    require(row["namespace_sha256"] == expected["namespace_sha256"], "Namespace changed")
    require(
        row["runtime_module_sha256"] == expected["runtime_module_sha256"], "Runtime modules changed"
    )
    require(
        shared.mappings(row["runtime_maps"], freeze["arms"][arm], freeze["canonical_source"])
        == expected["runtime_mappings"],
        "Loaded runtime mapping changed",
    )
    require(
        row["metrics"]["coin_image_bytes"] == row["metrics"]["decoded_image_bytes"] == 0,
        "Console image materialization",
    )
    metric_names = (
        {
            "map_restore_ms",
            "document_open_ms",
            "peak_rss_bytes",
            "coin_image_bytes",
            "decoded_image_bytes",
        }
        if freeze["comparison"] == "P03"
        else {
            "save_payload_ms",
            "document_save_ms",
            "peak_rss_bytes",
            "coin_image_bytes",
            "decoded_image_bytes",
        }
    )
    require(
        set(row["metrics"]) == metric_names
        and all(
            type(value) in (int, float) and math.isfinite(value) and value >= 0
            for value in row["metrics"].values()
        ),
        "Invalid metric schema/value",
    )
    timing_names = (
        ["map_restore_ms", "document_open_ms"]
        if freeze["comparison"] == "P03"
        else ["document_save_ms"]
        + (["save_payload_ms"] if cell == "gridfinity-drawer-v1.3" else [])
    )
    require(
        all(row["metrics"][name] > 0 for name in timing_names)
        and row["metrics"]["peak_rss_bytes"] > 0,
        "Uncovered/zero timing or process RSS",
    )
    if freeze["comparison"] == "P02" and cell != "gridfinity-drawer-v1.3":
        require(
            row["metrics"]["save_payload_ms"] == 0 and row["save_trace"]["save_calls"] == 0,
            "Nonzero payload in structurally empty control",
        )
    for name in ["open_trace"] + (["save_trace"] if freeze["comparison"] == "P02" else []):
        trace = row[name]
        require(
            trace["save_active"]
            == trace["map_active"]
            == trace["save_failed"]
            == trace["map_failed"]
            == 0,
            "Failed/unbalanced hook",
        )
        require(
            trace["save_calls"] == trace["save_completed"]
            and trace["map_calls"] == trace["map_completed"],
            "Incomplete hook",
        )


def preflight(freeze):
    require(freeze["schema"] == "freecad-spec003-document-performance-freeze/v1", "Wrong freeze")
    require(digest(Path(__file__)) == freeze["controller_sha256"], "Controller changed")
    require(
        freeze["sampling"]
        == {"pairs": 24, "diagnostic_pairs": 1, "warmup_pairs": 3, "measured_pairs": 20},
        "Sampling changed",
    )
    require(
        freeze["limits"]
        == {"worker_seconds": 60, "worker_output_bytes": 262144, "cell_seconds": 1200},
        "Bounds changed",
    )
    policy_root = Path(freeze["project_root"]) / "docs/specs/003-freecad-performance-optimization"
    policy_names = {
        "acceptance-policy-v2.md",
        "acceptance-policy-v2.json",
        "cost-benefit-review.md",
        "cost-benefit-review.json",
        "t0043-execution-plan.md",
    }
    require(
        set(freeze["policy_bindings"]) == {str(policy_root / name) for name in policy_names},
        "Missing/extra D007 policy/rubric binding",
    )
    for path, expected in freeze["policy_bindings"].items():
        require(
            digest(Path(path)) == expected and freeze["file_bindings"].get(path) == expected,
            "D007 policy/rubric changed or unbound",
        )
    policy = json.loads((policy_root / "acceptance-policy-v2.json").read_text())
    require(
        policy["schema"] == "freecad-spec003-cost-benefit-policy/v2"
        and policy["decision"] == "D007"
        and policy["status"] == "owner-selected",
        "Wrong review policy",
    )
    require(
        policy["policy_document_sha256"] == digest(policy_root / "acceptance-policy-v2.md")
        and freeze["review_policy_id"] == policy["policy_id"],
        "Policy document/id mismatch",
    )
    require(
        freeze["required_review_fields"] == policy["required_review_fields"]
        and freeze["production_accepted"] is False
        and freeze["v1_statistics_are_diagnostics_only"] is True,
        "Review fields/acceptance scope changed",
    )
    shared.execution_identity(freeze)
    shared.validate_environment(freeze)
    validate_cells(freeze)
    for cell in CELLS[freeze["comparison"]]:
        for arm in ["baseline", "change"]:
            require(
                worker_command(freeze, cell, arm) == freeze["worker_commands"][cell][arm],
                "Worker argv changed",
            )
    art = Path(freeze["artifact_directory"])
    names = [
        "status.json",
        "audit-before.json",
        "audit-after.json",
        "observations.jsonl",
        "host.jsonl",
    ]
    destinations = [art / name for name in names] + [
        art / f"worker-{i:04d}.bin" for i in range(len(schedule(freeze["comparison"])))
    ]
    require(
        all(not p.exists() and not p.is_symlink() for p in destinations),
        "Campaign output already exists",
    )
    for ordinal, (_, _, arm, _) in enumerate(schedule(freeze["comparison"])):
        validate_destination(freeze, ordinal, arm)
    return audit(freeze, initial=True)


def read_result(destination, limit):
    require(
        destination.is_file()
        and not destination.is_symlink()
        and destination.stat().st_size <= limit,
        "Invalid/oversized result",
    )
    require(destination.stat().st_mode & 0o777 == 0o600, "Result must be private")
    return json.loads(destination.read_text())


def run(path, sha256):
    require(
        sys.flags.isolated == sys.flags.no_site == sys.flags.dont_write_bytecode == 1,
        "Requires isolated host Python -I -S -B",
    )
    os.umask(0o077)
    freeze = shared.load_freeze(path, sha256)
    before = preflight(freeze)
    art = Path(freeze["artifact_directory"])
    shared.write(art / "audit-before.json", before, exclusive=True)
    shared.write(
        art / "status.json", {"status": "in_progress", "freeze_sha256": sha256}, exclusive=True
    )
    completed, current, cell_start = 0, None, None
    try:
        with (art / "observations.jsonl").open("x") as output, (art / "host.jsonl").open(
            "x"
        ) as host:
            for ordinal, (cell, pair, arm, phase) in enumerate(schedule(freeze["comparison"])):
                if cell != current:
                    current, cell_start = cell, time.monotonic()
                require(time.monotonic() - cell_start < 1200, "Cell timeout")
                host.write(json.dumps(shared.host_snapshot()) + "\n")
                host.flush()
                destination = validate_destination(freeze, ordinal, arm)
                env = shared.worker_environment(freeze, arm)
                env["SPEC003_RESULT_FILE"] = str(destination)
                env["SPEC003_ARM"] = arm
                command = worker_command(freeze, cell, arm)
                collect(command, env, 60, art / f"worker-{ordinal:04d}.bin")
                row = read_result(destination, freeze["limits"]["worker_output_bytes"])
                validate_row(freeze, row, cell, arm)
                require(time.monotonic() - cell_start < 1200, "Cell timeout")
                row.update(ordinal=ordinal, cell=cell, pair=pair, arm=arm, phase=phase)
                output.write(json.dumps(row, sort_keys=True) + "\n")
                output.flush()
                completed += 1
                if completed % 8 == 0:
                    print(
                        freeze["comparison"],
                        completed,
                        "/",
                        len(schedule(freeze["comparison"])),
                        flush=True,
                    )
        shared.write(art / "audit-after.json", audit(freeze), exclusive=True)
        shared.write(
            art / "status.json",
            {
                "status": "complete",
                "completed": completed,
                "failures": 0,
                "retries": 0,
                "freeze_sha256": sha256,
            },
        )
    except BaseException as exc:
        shared.write(
            art / "status.json",
            {
                "status": "failed",
                "completed": completed,
                "error_type": type(exc).__name__,
                "freeze_sha256": sha256,
            },
        )
        raise


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--freeze", type=Path, required=True)
    parser.add_argument("--freeze-sha256", required=True)
    args = parser.parse_args()
    run(args.freeze, args.freeze_sha256)
