#!/usr/bin/env python3
"""Assess complete frozen Spec 003 cells using the predeclared D001 rules."""

import argparse
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import random
import statistics

HERE = Path(__file__).resolve().parent


def digest(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def percentile(values, fraction):
    ordered = sorted(values)
    position = (len(ordered) - 1) * fraction
    lower = math.floor(position)
    upper = math.ceil(position)
    return ordered[lower] + (ordered[upper] - ordered[lower]) * (position - lower)


def summary(values):
    median = statistics.median(values)
    iqr = percentile(values, 0.75) - percentile(values, 0.25)
    return {
        "median": median,
        "p95": percentile(values, 0.95),
        "iqr": iqr,
        "iqr_over_median": iqr / median if median else (0 if iqr == 0 else None),
        "maximum": max(values),
    }


def assess_metric(before, after, rule, sampling):
    require(len(before) == len(after) == sampling["measured_pairs"], "Wrong measured count")
    require(
        all(type(v) in (int, float) and math.isfinite(v) and v >= 0 for v in before + after),
        "Invalid observation",
    )
    baseline, change = summary(before), summary(after)
    result = {"baseline": baseline, "change": change, "kind": rule["kind"]}
    passed, inconclusive, coverage_inapplicable = False, False, False
    kind = rule["kind"]
    if kind == "resource":
        allowance = max(rule["fraction"] * baseline["maximum"], rule["floor_bytes"])
        passed = change["maximum"] - baseline["maximum"] <= allowance
        result["maximum_increase_allowed"] = allowance
    elif kind == "zero":
        passed = all(v == 0 for v in after) and (
            not rule.get("baseline_must_be_positive") or all(v > 0 for v in before)
        )
    elif kind == "budget":
        passed = change["median"] <= rule["median_ms"] and change["p95"] <= rule["p95_ms"]
    elif kind == "non_regression":
        passed = change["median"] - baseline["median"] <= max(
            baseline["median"] * rule["median_fraction"], rule["median_floor_ms"]
        ) and change["p95"] - baseline["p95"] <= max(
            baseline["p95"] * rule["p95_fraction"], rule["p95_floor_ms"]
        )
    elif kind in ("improve", "reduction"):
        if any(value == 0 for value in before):
            inconclusive = True
            coverage_inapplicable = True
            result["reason"] = "Zero baseline requires a coverage/protocol decision"
        else:
            ratios = [b / a for a, b in zip(before, after)]
            result["paired_ratio"] = summary(ratios)
            if kind == "reduction":
                passed = statistics.median(ratios) <= 1 - rule["minimum_reduction_fraction"]
            else:
                rng = random.Random(sampling["bootstrap_seed"])
                resampled = [
                    statistics.median([ratios[rng.randrange(len(ratios))] for _ in ratios])
                    for _ in range(sampling["bootstrap_resamples"])
                ]
                interval = [percentile(resampled, 0.025), percentile(resampled, 0.975)]
                saved = statistics.median([a - b for a, b in zip(before, after)])
                result.update(bootstrap_ratio_interval=interval, median_paired_saved_ms=saved)
                passed = (
                    statistics.median(ratios) <= 1 - rule["minimum_gain_fraction"]
                    and saved >= rule["minimum_saved_ms"]
                    and interval[1] < rule["bootstrap_ratio_upper_must_be_below"]
                )
    else:
        raise ValueError("Unsupported metric kind")
    if kind in ("improve", "non_regression", "budget"):
        limit = sampling["dispersion_limit_iqr_over_median"]
        inconclusive |= any(
            s["iqr_over_median"] is None or s["iqr_over_median"] > limit for s in (baseline, change)
        )
    result["thresholds_passed"] = passed
    result["status"] = (
        "inconclusive"
        if coverage_inapplicable
        else "fail" if not passed else "inconclusive" if inconclusive else "pass"
    )
    return result


def assess(execution_path, observations_path):
    plan = json.loads((HERE / "benchmark-plan.json").read_text())
    execution = json.loads(execution_path.read_text())
    rows = [json.loads(line) for line in observations_path.read_text().splitlines()]
    require(rows[-1].get("collection_complete") is True, "Incomplete collection is ineligible")
    require(rows[0]["execution_sha256"] == digest(execution_path), "Execution identity changed")
    require(
        rows[0]["protocol_sha256"] == digest(HERE / "benchmark-plan.json"), "Plan identity changed"
    )
    require(
        execution["protocol_document_sha256"] == digest(HERE / "benchmark-protocol.md"),
        "Protocol changed",
    )
    comparison = next(c for c in plan["comparisons"] if c["id"] == execution["comparison_id"])
    fixture = plan["fixtures"][execution["fixture_id"]]
    if "sha256" in fixture:
        require(execution["fixture_sha256"] == fixture["sha256"], "Unapproved fixture identity")
    spec = importlib.util.spec_from_file_location("spec003_runner", HERE / "benchmark-runner.py")
    runner = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(runner)
    schedule = [(pair, arm) for pair in runner.schedule(plan) for arm in pair["order"]]
    context_path = Path(execution["context_path"])
    require(digest(context_path) == execution["context_sha256"], "Context identity changed")
    context = json.loads(context_path.read_text())
    command_only = comparison["id"] == "P01"
    payload_inapplicable = False
    if command_only:
        require(context.get("execution_mode") == "command-only", "Wrong P01 execution mode")
        require("native_profile" not in context, "Console context contains a renderer claim")
        disposition = context.get("restore_payload_disposition")
        if disposition is not None:
            require(
                execution["fixture_id"] in {"EngineBlock", "PartDesignExample"}
                and disposition
                == {
                    "decision_id": "D006",
                    "metric": "restore_payload_ms",
                    "status": "not_applicable",
                    "fixture_sha256": fixture["sha256"],
                },
                "Invalid zero-payload disposition",
            )
            payload_inapplicable = True
    else:
        require("native_profile" in context, "Missing frozen renderer profile")
    observations = rows[1:-1]
    require(len(observations) == len(schedule), "Missing/extra observations")
    measured = {"baseline": [], "change": []}
    for row, (pair, arm) in zip(observations, schedule):
        require(
            all(row[k] == v for k, v in pair.items()) and row["arm"] == arm,
            "Schedule/order changed",
        )
        record = row["record"]
        require(
            record["correctness_passed"] is True and not record.get("qualification_only", False),
            "Unqualified record",
        )
        for name in (
            "comparison_id",
            "fixture_id",
            "fixture_sha256",
            "context_sha256",
            "normative_state_sha256",
        ):
            require(record[name] == execution[name], "Worker identity changed")
        require(record["commit"] == execution[arm]["commit"], "Wrong arm commit")
        if command_only:
            require("native_profile" not in record, "Console record contains a renderer claim")
            require(
                record.get("included_file_coverage") == context["included_file_coverage"],
                "Included-file coverage changed",
            )
            require(record.get("endpoint") == context["endpoint"], "Console endpoint changed")
            require(
                record.get("generated_sql_parser") == context["generated_sql_parser"],
                "Generated parser identity changed",
            )
            require(
                type(record.get("completion_recompute_count")) is int
                and record["completion_recompute_count"] >= 0
                and record["completion_recompute_count"] == context["completion_recompute_count"],
                "Completion recompute count changed",
            )
            coverage = record["included_file_coverage"]
            if payload_inapplicable:
                require(
                    coverage == {"included_properties": 0, "included_bytes": 0},
                    "Structural-zero payload coverage changed",
                )
            else:
                require(
                    coverage["included_properties"] > 0 and coverage["included_bytes"] > 0,
                    "P01 has no meaningful payload coverage; reviewed disposition required",
                )
            hooks = record["hook_counters"]
            require(
                all(type(value) is int and value >= 0 for value in hooks.values())
                and hooks["active"] == hooks["failed"] == hooks["reused"] == 0
                and hooks["completed"] == coverage["included_properties"],
                "Unqualified native hook counts",
            )
            require(
                record["metrics"]["restore_payload_ms"] == hooks["restore_nanoseconds"] / 1e6,
                "Restore metric differs from native hook duration",
            )
            if payload_inapplicable:
                require(hooks["restore_nanoseconds"] == 0, "Unexpected zero-coverage hook duration")
        else:
            require(
                record.get("native_profile") == context["native_profile"],
                "Renderer/driver identity changed",
            )
            require(
                record.get("effective_configuration_sha256")
                == context["effective_configuration_sha256"],
                "Effective configuration changed",
            )
        require(
            record.get("namespace_sha256") == context["namespace_sha256"][arm],
            "Namespace identity changed",
        )
        require(
            record.get("runtime_module_sha256") == context["runtime_module_sha256"][arm],
            "Loaded runtime libraries changed",
        )
        require(
            all(
                type(v) in (int, float) and math.isfinite(v) and v >= 0
                for v in record["metrics"].values()
            ),
            "Invalid metric",
        )
        if pair["phase"] == "measured":
            measured[arm].append(record["metrics"])
    primary = execution["fixture_id"] in comparison["primary_fixture_ids"]
    rules = comparison["metrics"] if primary else comparison["control_metrics"]
    if primary and "primary_context_metrics" in comparison:
        rules = comparison["primary_context_metrics"][execution["context"]]
    metrics = {
        name: (
            {
                "status": "not_applicable",
                "decision_id": "D006",
                "reason": "No included-file declarations, bytes or restore hooks in this fixed control",
                "thresholds_passed": None,
            }
            if payload_inapplicable and name == "restore_payload_ms"
            else assess_metric(
                [r[name] for r in measured["baseline"]],
                [r[name] for r in measured["change"]],
                rule,
                plan["sampling"],
            )
        )
        for name, rule in rules.items()
    }
    for name, rule in rules.items():
        if rule["kind"] == "zero":
            all_before = [
                r["record"]["metrics"][name] for r in observations if r["arm"] == "baseline"
            ]
            all_after = [r["record"]["metrics"][name] for r in observations if r["arm"] == "change"]
            if any(all_after) or (
                rule.get("baseline_must_be_positive") and any(v <= 0 for v in all_before)
            ):
                metrics[name]["thresholds_passed"] = False
                metrics[name]["status"] = "fail"
    statuses = [v["status"] for v in metrics.values()]
    status = (
        "fail" if "fail" in statuses else ("inconclusive" if "inconclusive" in statuses else "pass")
    )
    return {
        "schema": "freecad-spec003-cell-assessment/v1",
        "comparison_id": execution["comparison_id"],
        "fixture_id": execution["fixture_id"],
        "context": execution["context"],
        "execution_sha256": digest(execution_path),
        "observations_sha256": digest(observations_path),
        "assessor_sha256": digest(Path(__file__)),
        "measured_pairs": len(measured["baseline"]),
        "status": status,
        "metrics": metrics,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("execution", type=Path)
    parser.add_argument("observations", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    args.out.write_text(json.dumps(assess(args.execution, args.observations), indent=2) + "\n")


if __name__ == "__main__":
    main()
