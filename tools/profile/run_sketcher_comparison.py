# SPDX-License-Identifier: LGPL-2.1-or-later
"""Compare three Linux/ELF Sketcher runtimes with frozen, alternating workloads.

All builds must finish first. This controller reuses the T001 worker's operation,
reset and count boundaries; only the scheduling of samples differs from A/A mode.
It never builds runtimes or changes preferences, CPU policy or operator documents.
"""

import argparse
import hashlib
import json
import os
import platform
from pathlib import Path
import queue
import random
import statistics
import subprocess
import sys
import threading
import uuid

from run_sketcher_baseline import CASES


NAMES = ("baseline", "instrumentation", "candidate")
SEED = 576001
RESAMPLES = 10000


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def interval(values):
    values = sorted(values)
    return [values[int(0.025 * (len(values) - 1))], values[int(0.975 * (len(values) - 1))]]


def summarize(pairs, overhead=False):
    """Resample complete matched pairs; recompute medians and allowances each time."""
    left, right = zip(*pairs)
    median_left, median_right = statistics.median(left), statistics.median(right)
    fraction = 0.02 if overhead else 0.05
    allowance = max(fraction * median_left, 0.001)
    rng = random.Random(SEED)
    effects, excesses = [], []
    for _ in range(RESAMPLES):
        sample = [pairs[rng.randrange(len(pairs))] for _ in pairs]
        a = statistics.median(row[0] for row in sample)
        b = statistics.median(row[1] for row in sample)
        effects.append(1 - b / a)
        excesses.append(b - a - max(fraction * a, 0.001))
    effect_ci, excess_ci = interval(effects), interval(excesses)
    return {
        "pairs": len(pairs),
        "baseline_median_s": median_left,
        "candidate_median_s": median_right,
        "baseline_mad_s": statistics.median(abs(x - median_left) for x in left),
        "candidate_mad_s": statistics.median(abs(x - median_right) for x in right),
        "speedup": 1 - median_right / median_left,
        "speedup_ci95": effect_ci,
        "speedup_gate": (
            "pass" if effect_ci[0] >= 0.10 else ("fail" if effect_ci[1] < 0.10 else "inconclusive")
        ),
        "allowance_s": allowance,
        "difference_s": median_right - median_left,
        "difference_minus_allowance_ci95_s": excess_ci,
        "regression_gate": (
            "pass" if excess_ci[1] <= 0 else ("fail" if excess_ci[0] > 0 else "inconclusive")
        ),
    }


class Worker:
    def __init__(self, build, target, case, counter, fixture, controlled, name):
        self.build, self.target, self.case = build, target, case
        target.mkdir(parents=True, exist_ok=False)
        self.run_id = uuid.uuid4().hex
        libdirs = [build / "lib"] + sorted(p for p in (build / "Mod").iterdir() if p.is_dir())
        libdirs.append(Path(sys.prefix) / "lib")
        self.environment = {
            "SKETCH_PERF_RUN_ID": self.run_id,
            "SKETCH_PERF_COUNTER": str(counter),
            "SKETCH_PERF_APP_LIB": str(build / "lib/libFreeCADApp.so"),
            "SKETCH_PERF_OUTPUT_DIR": str(target),
            "SKETCH_PERF_CASE": case,
            "SKETCH_PERF_CONFIGURATION": "candidate" if name == "candidate" else "baseline",
            "SKETCH_PERF_PAIRS": "0",
            "SKETCH_PERF_FIXTURE": str(fixture),
            "LD_PRELOAD": str(counter),
            "LD_LIBRARY_PATH": ":".join(map(str, libdirs)),
            "PYTHONNOUSERSITE": "1",
        }
        for key, child in (
            ("XDG_CONFIG_HOME", "config"),
            ("XDG_DATA_HOME", "data"),
            ("XDG_CACHE_HOME", "cache"),
            ("TMPDIR", "temp"),
        ):
            (target / child).mkdir()
            self.environment[key] = str(target / child)
        if controlled:
            self.environment["SKETCH_PERF_CONTROLLED"] = "1"
        env = os.environ.copy()
        env.pop("FREECAD_SKETCHER_TRACE", None)
        env.update(self.environment)
        self.command = [
            str(build / "bin/FreeCADCmd"),
            "--user-cfg",
            str(target / "user.cfg"),
            "--system-cfg",
            str(target / "system.cfg"),
            str(Path(__file__).with_name("sketcher_baseline.py").resolve()),
        ]
        self.messages = queue.Queue()
        self.process = subprocess.Popen(
            self.command,
            env=env,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )

        def read_output():
            with (target / "console.log").open("w") as log:
                for line in self.process.stdout:
                    log.write(line)
                    log.flush()
                    if line.startswith("SKETCH_PERF_"):
                        self.messages.put(line.rstrip())
            self.messages.put("EOF")

        self.reader = threading.Thread(target=read_output, daemon=True)
        self.reader.start()
        try:
            if controlled and self.messages.get(timeout=600) != "SKETCH_PERF_READY":
                raise RuntimeError(f"worker did not become ready: {target}")
        except BaseException:
            self.abort()
            raise

    def sample(self, label):
        self.process.stdin.write(json.dumps(label) + "\n")
        self.process.stdin.flush()
        message = self.messages.get(timeout=600)
        if not message.startswith("SKETCH_PERF_ROW "):
            raise RuntimeError(f"worker failed during {label}: {self.target}")
        row = json.loads(message.removeprefix("SKETCH_PERF_ROW "))
        assert row["label"] == label
        return row

    def finish(self, controlled):
        if controlled:
            self.process.stdin.write('"stop"\n')
            self.process.stdin.flush()
        code = self.process.wait(timeout=600)
        self.reader.join(timeout=5)
        result = json.loads((self.target / "result.json").read_text())
        if code or result.get("status") != "passed" or result.get("run_id") != self.run_id:
            raise RuntimeError(f"worker failed: {self.target}")
        for module in result["modules"].values():
            if not Path(module).resolve().is_relative_to(self.build):
                raise RuntimeError(f"module outside selected runtime: {module}")
        (self.target / "receipt.json").write_text(
            json.dumps(
                {
                    "command": self.command,
                    "environment": self.environment,
                    "returncode": code,
                    "result": result,
                },
                indent=2,
            )
            + "\n"
        )
        return result

    def abort(self):
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()


def expected_vector(manifest, case, name):
    workload = next(row for row in manifest["workloads"] if row.get("runner_case") == case)
    vector = json.loads(json.dumps(workload["baseline_execution_vector"]))
    if name == "candidate":
        if case in ("forced_synthetic", "forced_master", "forced_profile"):
            for row in vector.values():
                row["setups"] = 1
        elif case == "scalar_insertion":
            vector["Profile"]["setups"] = 17
        elif case == "list_insertion":
            vector["Profile"]["setups"] = 2
    return vector


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in NAMES:
        parser.add_argument(f"--{name}-root", type=Path, required=True)
    parser.add_argument("--output-root", type=Path, required=True)
    parser.add_argument("--counter", type=Path, required=True)
    parser.add_argument("--fixture", type=Path, required=True)
    parser.add_argument(
        "--manifest",
        type=Path,
        default=Path("docs/specs/000-sketcher-diagnosis-performance/workloads.json"),
    )
    parser.add_argument("--case", choices=CASES, action="append", dest="cases")
    parser.add_argument("--batch", type=int, choices=(1, 2, 3), default=1)
    parser.add_argument(
        "--cpus", help="comma-separated CPU affinity for this controller and its workers"
    )
    args = parser.parse_args()
    if sys.platform != "linux":
        parser.error("requires Linux/ELF")
    if args.cpus:
        os.sched_setaffinity(0, {int(cpu) for cpu in args.cpus.split(",")})
    builds = {name: getattr(args, name + "_root").resolve(strict=True) for name in NAMES}
    # Configuration names are explicit, independent of the runtime folder's basename.
    counter, fixture = args.counter.resolve(strict=True), args.fixture.resolve(strict=True)
    manifest = json.loads(args.manifest.read_text())
    assert sha256(fixture) == manifest["fixtures"]["frozen_plate"]["sha256"]
    output = args.output_root.resolve()
    output.mkdir(parents=True, exist_ok=False)
    governor = Path("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor")
    receipt = {
        "machine": {
            "platform": platform.platform(),
            "affinity": sorted(os.sched_getaffinity(0)),
            "load_average_start": list(os.getloadavg()),
            "governor": governor.read_text().strip() if governor.exists() else "unavailable",
        },
        "status": "running",
        "batch": args.batch,
        "warmups": 2,
        "pairs": 10,
        "order": "baseline,instrumentation,candidate then reverse on odd pairs",
        "trace_control": "FREECAD_SKETCHER_TRACE unset in all measured processes",
        "bootstrap_seed": SEED,
        "bootstrap_resamples": RESAMPLES,
        "controller_sha256": sha256(Path(__file__)),
        "counter_sha256": sha256(counter),
        "fixture_sha256": sha256(fixture),
        "worker_sha256": sha256(Path(__file__).with_name("sketcher_baseline.py")),
        "builds": {
            name: {"root": str(root), "sketcher_sha256": sha256(root / "Mod/Sketcher/Sketcher.so")}
            for name, root in builds.items()
        },
        "rejected_pairs": [],
        "cases": {},
    }

    def save():
        (output / "run.json").write_text(json.dumps(receipt, indent=2) + "\n")

    save()
    live = []
    try:
        for case in args.cases or CASES:
            rows = {name: [] for name in NAMES}
            warmups = {name: [] for name in NAMES}
            workers = {}
            fresh = case == "fresh_load"
            if not fresh:
                for name in NAMES:
                    worker = Worker(
                        builds[name], output / case / name, case, counter, fixture, True, name
                    )
                    live.append(worker)
                    workers[name] = worker
            try:
                for index in range(-2, 10):
                    label = f"warmup-{index + 2}" if index < 0 else f"pair-{index}"
                    order = NAMES if index % 2 == 0 else tuple(reversed(NAMES))
                    triple = {}
                    preferences = {}
                    for name in order:
                        if fresh:
                            worker = Worker(
                                builds[name],
                                output / case / name / label,
                                case,
                                counter,
                                fixture,
                                False,
                                name,
                            )
                            live.append(worker)
                            result = worker.finish(False)
                            preferences[name] = result["sketcher_preferences"]
                            row = json.loads((worker.target / "raw.json").read_text())[0]
                            row["label"] = label
                        else:
                            row = workers[name].sample(label)
                        if row["counts"] != expected_vector(manifest, case, name):
                            raise RuntimeError(f"execution vector mismatch: {case}/{label}/{name}")
                        triple[name] = row
                    if not (
                        triple["baseline"]["state"]
                        == triple["instrumentation"]["state"]
                        == triple["candidate"]["state"]
                    ):
                        raise RuntimeError(f"matched state mismatch: {case}/{label}")
                    if fresh:
                        assert all(
                            value == preferences["baseline"] for value in preferences.values()
                        )
                    for name in NAMES:
                        (warmups if index < 0 else rows)[name].append(triple[name])
                    receipt["cases"][case] = {"rows": rows, "warmups": warmups}
                    save()
                if not fresh:
                    results = [workers[name].finish(True) for name in NAMES]
                    assert all(
                        r["sketcher_preferences"] == results[0]["sketcher_preferences"]
                        for r in results
                    )
            except BaseException as error:
                receipt["rejected_pairs"].append(
                    {
                        "case": case,
                        "label": label,
                        "reason": str(error),
                        "disposition": "run stopped",
                    }
                )
                raise
            comparisons = {}
            for left, right, overhead in (
                ("baseline", "candidate", False),
                ("baseline", "instrumentation", True),
                ("instrumentation", "candidate", False),
            ):
                pairs = [
                    (a["elapsed_ns"] / 1e9, b["elapsed_ns"] / 1e9)
                    for a, b in zip(rows[left], rows[right])
                ]
                comparisons[left + "_vs_" + right] = summarize(pairs, overhead)
            receipt["cases"][case]["comparisons"] = comparisons
            save()
            print(f"passed {case}: " + json.dumps(comparisons), flush=True)
        receipt["machine"]["load_average_end"] = list(os.getloadavg())
        receipt["status"] = "passed"
    except BaseException as error:
        receipt["status"] = "failed"
        receipt["error"] = str(error)
        raise
    finally:
        for worker in live:
            worker.abort()
        save()


if __name__ == "__main__":
    main()
