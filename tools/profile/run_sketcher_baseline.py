# SPDX-License-Identifier: LGPL-2.1-or-later
"""Launch isolated Linux baseline workloads from a Pixi Python environment."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import uuid


CASES = (
    "clean_noop",
    "forced_synthetic",
    "forced_master",
    "forced_profile",
    "scalar_insertion",
    "list_insertion",
    "thickness",
    "warm_load",
    "fresh_load",
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--output-root", type=Path, required=True)
    parser.add_argument("--counter", type=Path, required=True)
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument("--pairs", type=int, default=10)
    parser.add_argument("--configuration", choices=("baseline", "candidate"), default="baseline")
    parser.add_argument("--case", choices=CASES, action="append", dest="cases")
    args = parser.parse_args()
    if sys.platform != "linux" or args.pairs < 1:
        parser.error("requires Linux/ELF and at least one measured A/A pair")
    build = args.build_root.resolve(strict=True)
    output = args.output_root.resolve()
    counter = args.counter.resolve(strict=True)
    fixture = args.fixture.resolve(strict=True)
    executable = build / "bin/FreeCADCmd"
    if not executable.is_file():
        parser.error("FreeCADCmd is missing from the selected build")
    if (output / "run.json").exists():
        parser.error("output root already contains a run; choose a new root")
    output.mkdir(parents=True, exist_ok=True)
    worker = Path(__file__).with_name("sketcher_baseline.py").resolve()
    libdirs = [build / "lib"]
    libdirs += sorted(path for path in (build / "Mod").iterdir() if path.is_dir())
    libdirs.append(Path(sys.prefix) / "lib")
    receipt = {
        "status": "running",
        "purpose": "single-configuration A/A repeatability and execution checks, not a speedup comparison",
        "configuration": args.configuration,
        "build_root": str(build),
        "fixture_sha256": hashlib.sha256(fixture.read_bytes()).hexdigest(),
        "counter_sha256": hashlib.sha256(counter.read_bytes()).hexdigest(),
        "worker_sha256": hashlib.sha256(worker.read_bytes()).hexdigest(),
        "frozen_native_vectors": {},
        "records": [],
    }

    def save():
        (output / "run.json").write_text(json.dumps(receipt, indent=2) + "\n")

    save()
    try:
        for case in args.cases or CASES:
            labels = ["batch"]
            if case == "fresh_load":
                labels = ["warmup-0", "warmup-1"]
                for pair in range(args.pairs):
                    order = ("A", "B") if pair % 2 == 0 else ("B", "A")
                    labels.extend(f"pair-{pair}-{side}" for side in order)
            for label in labels:
                target = output / case / label
                target.mkdir(parents=True)
                run_id = uuid.uuid4().hex
                selected_env = {
                    "SKETCH_PERF_RUN_ID": run_id,
                    "SKETCH_PERF_COUNTER": str(counter),
                    "SKETCH_PERF_APP_LIB": str(build / "lib/libFreeCADApp.so"),
                    "SKETCH_PERF_OUTPUT_DIR": str(target),
                    "SKETCH_PERF_CASE": case,
                    "SKETCH_PERF_CONFIGURATION": args.configuration,
                    "SKETCH_PERF_PAIRS": str(args.pairs),
                    "SKETCH_PERF_FIXTURE": str(fixture),
                    "LD_PRELOAD": str(counter),
                    "LD_LIBRARY_PATH": ":".join(map(str, libdirs)),
                    "PYTHONNOUSERSITE": "1",
                    "XDG_CONFIG_HOME": str(target / "config"),
                    "XDG_DATA_HOME": str(target / "data"),
                    "XDG_CACHE_HOME": str(target / "cache"),
                }
                env = os.environ.copy()
                env.update(selected_env)
                command = [
                    str(executable),
                    "--user-cfg",
                    str(target / "user.cfg"),
                    "--system-cfg",
                    str(target / "system.cfg"),
                    str(worker),
                ]
                with (target / "console.log").open("w") as console:
                    process = subprocess.run(
                        command, env=env, stdout=console, stderr=subprocess.STDOUT, check=False
                    )
                result_path = target / "result.json"
                result = json.loads(result_path.read_text()) if result_path.exists() else None
                record = {
                    "case": case,
                    "label": label,
                    "command": command,
                    "environment": selected_env,
                    "returncode": process.returncode,
                    "result": result,
                }
                receipt["records"].append(record)
                save()
                if (
                    process.returncode != 0
                    or not result
                    or result.get("status") != "passed"
                    or result.get("run_id") != run_id
                ):
                    raise RuntimeError(f"workload failed: {case}/{label}; see {target}")
                for module in result["modules"].values():
                    if not Path(module).resolve().is_relative_to(build):
                        raise RuntimeError(f"module loaded outside selected build: {module}")
                rows = json.loads((target / "raw.json").read_text())
                vector = receipt["frozen_native_vectors"].setdefault(case, rows[0]["counts"])
                if any(row["counts"] != vector for row in rows):
                    raise RuntimeError(f"native execution vector changed for {case}/{label}")
                save()
                print(f"passed {case}/{label}", flush=True)
        receipt["status"] = "passed"
    except BaseException as error:
        receipt["status"] = "failed"
        receipt["error"] = str(error)
        raise
    finally:
        save()


if __name__ == "__main__":
    main()
