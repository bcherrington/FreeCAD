# SPDX-License-Identifier: LGPL-2.1-or-later
"""Console-only P02/P03 probe; invoke through its isolated bounded launcher."""

import ctypes
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import resource
import subprocess
import time
import xml.etree.ElementTree as ET
import zipfile


def fingerprint(value):
    return hashlib.sha256(
        json.dumps(value, sort_keys=True, separators=(",", ":")).encode()
    ).hexdigest()


def run():
    import FreeCAD as App

    harness = Path(os.environ["SPEC003_HARNESS_ROOT"])
    spec = importlib.util.spec_from_file_location(
        "spec003_checks", harness / "tests/src/Gui/ImagePerformanceProbe.py"
    )
    checks = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(checks)
    fidelity_spec = importlib.util.spec_from_file_location(
        "spec003_fidelity", harness / "tests/src/App/document_performance_fidelity.py"
    )
    fidelity = importlib.util.module_from_spec(fidelity_spec)
    fidelity_spec.loader.exec_module(fidelity)
    require = checks.require
    source = Path(os.environ["SPEC003_SOURCE_ROOT"])
    fixture = Path(os.environ["SPEC003_FIXTURE_PATH"])
    context_path = Path(os.environ["SPEC003_CONTEXT_PATH"])
    context = json.loads(context_path.read_text())
    qualification = os.environ.get("SPEC003_QUALIFY") == "1"
    comparison = os.environ.get("SPEC003_COMPARISON", "qualification")
    require(not App.GuiUp and not App.listDocuments(), "Fresh console required")
    require(checks.digest(fixture) == context["fixture_sha256"], "Fixture changed")
    commit = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True
    ).strip()
    require(commit == os.environ["SPEC003_COMMIT"], "Wrong source commit")
    require(
        not subprocess.check_output(
            ["git", "-C", str(source), "status", "--porcelain", "--untracked-files=all"]
        ),
        "Dirty source",
    )
    library = Path(os.environ["SPEC003_TRACE_LIBRARY"])
    trace = ctypes.CDLL(str(library.parent / "libFreeCADApp.so"))
    for name in ("begin", "end"):
        function = getattr(trace, "fc_document_perf_" + name)
        function.argtypes = []
        function.restype = ctypes.c_bool
    trace.fc_document_perf_snapshot.argtypes = []
    trace.fc_document_perf_snapshot.restype = ctypes.c_char_p
    image = ctypes.CDLL(str(library))
    image.fc_image_trace_begin.argtypes = [ctypes.c_int]
    image.fc_image_trace_begin.restype = ctypes.c_bool
    image.fc_image_trace_snapshot.argtypes = []
    image.fc_image_trace_snapshot.restype = ctypes.c_char_p
    require(image.fc_image_trace_begin(-1), "Image trace start failed")

    def snapshot():
        raw = trace.fc_document_perf_snapshot()
        require(raw is not None, "Trace snapshot failed")
        result = json.loads(raw)
        require(
            len(result) == 15 and all(type(v) is int and v >= 0 for v in result.values()),
            "Invalid counters",
        )
        require(result["map_active"] == result["save_active"] == 0, "Active trace")
        require(result["map_failed"] == result["save_failed"] == 0, "Failed native hook")
        require(result["map_calls"] == result["map_completed"], "Unfinished map parse")
        require(result["save_calls"] == result["save_completed"], "Unfinished save")
        return result

    def completed(doc):
        require(
            doc is not None and not doc.Restoring and not doc.Recomputing, "Incomplete document"
        )
        count = 0
        if doc.mustExecute():
            count = doc.recompute()
            require(type(count) is int and count > 0, "Recompute performed no work")
        require(not doc.Recomputing and not doc.mustExecute(), "Recompute incomplete")
        return count

    def payloads(doc, archive_path):
        rows = []
        with zipfile.ZipFile(archive_path) as archive:
            require(
                archive.getinfo("Document.xml").file_size <= 32 * 1024 * 1024, "Oversized metadata"
            )
            tree = ET.fromstring(archive.read("Document.xml"))
            entries = {n.get("name"): n for n in tree.findall("./ObjectData/Object")}
            for obj in sorted(doc.Objects, key=lambda obj: obj.Name):
                for name in obj.PropertiesList:
                    if obj.getTypeIdOfProperty(name) != "App::PropertyFileIncluded":
                        continue
                    filename = getattr(obj, name)
                    if not filename:
                        continue
                    decl = entries[obj.Name].find(
                        "./Properties/Property[@name='" + name + "']/FileIncluded"
                    )
                    require(decl is not None, "Missing included declaration")
                    entry = decl.get("file") or decl.get("data")
                    require(entry is not None, "Missing archive entry")
                    info = archive.getinfo(entry)
                    require(info.file_size <= 32 * 1024 * 1024, "Oversized payload")
                    digest = hashlib.sha256(archive.read(info)).hexdigest()
                    target = Path(filename)
                    require(
                        target.is_file() and target.stat().st_size == info.file_size,
                        "Missing payload bytes",
                    )
                    require(checks.digest(target) == digest, "Payload differs")
                    rows.append([obj.Name, name, info.file_size, digest])
        return rows

    require(trace.fc_document_perf_begin(), "Open trace start failed")
    started = time.perf_counter_ns()
    doc = App.openDocument(str(fixture))
    recompute_count = completed(doc)
    finished = time.perf_counter_ns()
    open_rss = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss * 1024
    require(trace.fc_document_perf_end(), "Open trace stop failed")
    open_trace = snapshot()
    initial_model = checks.model_snapshot(doc)
    initial_geometry_manifest = fidelity.geometry_manifest(doc, initial_model)
    initial_geometry = fidelity.geometric_state(doc, initial_model) if qualification else None
    initial_persistent = fidelity.persistent_state(doc)
    initial_state = fingerprint(initial_model)
    initial_payloads = payloads(doc, fixture)
    coverage = {
        "included_properties": len(initial_payloads),
        "included_bytes": sum(row[2] for row in initial_payloads),
        "map_calls": open_trace["map_calls"],
        "map_records": open_trace["map_records"],
        "child_records": open_trace["child_records"],
        "token_buffers": open_trace["token_buffers"],
        "token_bytes": open_trace["token_bytes"],
    }
    if qualification:
        with Path(os.environ["SPEC003_RESULT_FILE"] + ".open.json").open("x") as stream:
            json.dump(
                {
                    "coverage": coverage,
                    "open_trace": open_trace,
                    "normative_state_sha256": initial_state,
                    "completion_recompute_count": recompute_count,
                    "runtime": checks.runtime_receipt(context, True),
                    "qualification_only": True,
                },
                stream,
                indent=2,
            )
    reopened_recompute = None
    reopened_model = reopened_persistent = None
    reopened_geometry_manifest = None
    save_trace = None
    save_rss = None
    save_started = save_finished = None
    if comparison != "P03":
        saved = fixture.parent / "saved.FCStd"
        require(not saved.exists(), "Save destination already exists")
        require(trace.fc_document_perf_begin(), "Save trace start failed")
        save_started = time.perf_counter_ns()
        doc.saveAs(str(saved))
        save_finished = time.perf_counter_ns()
        save_rss = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss * 1024
        require(trace.fc_document_perf_end(), "Save trace stop failed")
        save_trace = snapshot()
        require(
            save_trace["save_calls"] - save_trace["save_empty"] == len(initial_payloads),
            "Save hook coverage differs",
        )
        require(payloads(doc, saved) == initial_payloads, "Saved payload differs")
        require(fingerprint(checks.model_snapshot(doc)) == initial_state, "Saving changed model")
        App.closeDocument(doc.Name)
        doc = App.openDocument(str(saved))
        reopened_recompute = completed(doc)
        # Keep original XML as the independent reference for calibration/proxy/object identity.
        reopened_model = checks.model_snapshot(doc)
        reopened_geometry_manifest = fidelity.geometry_manifest(doc, reopened_model)
        require(
            fidelity.without_geometry_hashes(reopened_model)
            == fidelity.without_geometry_hashes(initial_model),
            "Reopened nongeometry model differs",
        )
        if qualification:
            require(
                fidelity.equivalent_geometry(
                    initial_geometry, fidelity.geometric_state(doc, reopened_model)
                ),
                "Reopened geometry differs",
            )
        else:
            require(
                fingerprint(reopened_model)
                == context["document_identity"]["reopened_normative_state_sha256"],
                "Reopened geometry/state differs from qualification",
            )
        reopened_persistent = fidelity.persistent_state(doc)
        if reopened_persistent != initial_persistent:
            differences = []
            for i, (before, after) in enumerate(
                zip(initial_persistent["objects"], reopened_persistent["objects"])
            ):
                if before != after:
                    differences.append(
                        {
                            "object_index": i,
                            "before_sha256": fingerprint(before),
                            "after_sha256": fingerprint(after),
                            "changed_property_indices": [
                                j
                                for j, (a, b) in enumerate(
                                    zip(before["properties"], after["properties"])
                                )
                                if a != b
                            ],
                        }
                    )
            with Path(os.environ["SPEC003_RESULT_FILE"] + ".differences.json").open("x") as stream:
                json.dump({"differences": differences}, stream)
            raise RuntimeError("Reopened persistent properties differ")
        require(payloads(doc, saved) == initial_payloads, "Reopened payload differs")
    require(checks.digest(fixture) == context["fixture_sha256"], "Source copy changed")
    storage = json.loads(image.fc_image_trace_snapshot())
    require(
        storage["coin_image_bytes"] == storage["decoded_image_bytes"] == 0,
        "Console materialized images",
    )
    runtime = checks.runtime_receipt(context, True)
    maps = Path("/proc/self/maps").read_text()
    App.closeDocument(doc.Name)
    identity = {
        "normative_state_sha256": initial_state,
        "geometry_manifest_sha256": fingerprint(initial_geometry_manifest),
        "reopened_geometry_manifest_sha256": (
            fingerprint(reopened_geometry_manifest)
            if reopened_geometry_manifest is not None
            else None
        ),
        "payload_identity_sha256": fingerprint(initial_payloads),
        "persistent_state_sha256": fingerprint(initial_persistent),
        "persistent_state_inventory": initial_persistent["inventory"],
        "coverage": coverage,
        "completion_recompute_count": recompute_count,
        "reopened_recompute_count": reopened_recompute,
        "reopened_normative_state_sha256": (
            fingerprint(reopened_model) if reopened_model is not None else None
        ),
        "reopened_persistent_state_sha256": (
            fingerprint(reopened_persistent) if reopened_persistent is not None else None
        ),
    }
    if not qualification:
        require(identity == context["document_identity"], "Document differs from qualification")
    require(
        not qualification
        or not initial_geometry
        or all(v[1].get("valid", True) for v in initial_geometry.values()),
        "Invalid initial geometry",
    )
    return {
        **runtime,
        **identity,
        "comparison_id": os.environ.get("SPEC003_COMPARISON", "qualification"),
        "fixture_id": context["fixture_id"],
        "fixture_sha256": context["fixture_sha256"],
        "context_sha256": checks.digest(context_path),
        "commit": commit,
        "correctness_passed": True,
        "qualification_only": qualification,
        "open_trace": open_trace,
        "save_trace": save_trace,
        "runtime_maps": maps,
        "whole_worker_peak_rss_bytes": resource.getrusage(resource.RUSAGE_SELF).ru_maxrss * 1024,
        "metrics": (
            {
                "map_restore_ms": open_trace["map_nanoseconds"] / 1e6,
                "document_open_ms": (finished - started) / 1e6,
                "peak_rss_bytes": open_rss,
                "coin_image_bytes": storage["coin_image_bytes"],
                "decoded_image_bytes": storage["decoded_image_bytes"],
            }
            if comparison == "P03"
            else {
                "save_payload_ms": save_trace["save_nanoseconds"] / 1e6,
                "document_save_ms": (save_finished - save_started) / 1e6,
                "peak_rss_bytes": save_rss,
                "coin_image_bytes": storage["coin_image_bytes"],
                "decoded_image_bytes": storage["decoded_image_bytes"],
            }
        ),
        "rss_endpoint": "P03: process lifetime high-water through completed open/recompute before fidelity checks; P02: lifetime high-water through save including preceding fidelity preparation, neither save-incremental nor retained RAM",
        "endpoint": "native outermost same-kind scopes; console open includes required recompute; whole save ends after saveAs returns; startup and fidelity checks excluded",
    }


def main_command():
    try:
        result = run()
    except Exception as exc:
        import traceback

        traceback.print_exc()
        result = {"correctness_passed": False, "exception_type": type(exc).__name__}
    path = Path(os.environ["SPEC003_RESULT_FILE"])
    with path.open("x") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
    path.chmod(0o600)
