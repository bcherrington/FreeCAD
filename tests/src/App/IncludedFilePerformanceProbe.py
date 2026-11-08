# SPDX-License-Identifier: LGPL-2.1-or-later
"""P01 command worker; use the isolated launcher, never a live document."""

import ctypes
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import resource
import stat
import subprocess
import sys
import time
import zipfile
import xml.etree.ElementTree as ET


def run():
    import FreeCAD as App

    source = Path(os.environ["SPEC003_SOURCE_ROOT"])
    module_spec = importlib.util.spec_from_file_location(
        "spec003_image_checks", source / "tests/src/Gui/ImagePerformanceProbe.py"
    )
    checks = importlib.util.module_from_spec(module_spec)
    module_spec.loader.exec_module(checks)
    require = checks.require
    context_path = Path(os.environ["SPEC003_CONTEXT_PATH"])
    context = json.loads(context_path.read_text())
    fixture = Path(os.environ["SPEC003_FIXTURE_PATH"])
    require(not App.GuiUp and not App.listDocuments(), "Fresh console process required")
    require(checks.digest(fixture) == context["fixture_sha256"], "Fixture changed")
    commit = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True
    ).strip()
    require(commit == os.environ["SPEC003_COMMIT"], "Wrong source commit")
    require(
        not subprocess.check_output(
            ["git", "-C", str(source), "status", "--porcelain", "--untracked-files=no"]
        ),
        "Dirty source arm",
    )
    qualification = os.environ.get("SPEC003_QUALIFY") == "1"
    library_dir = Path(os.environ["SPEC003_TRACE_LIBRARY"]).parent
    trace = ctypes.CDLL(str(library_dir / "libFreeCADApp.so"))
    for name in ("begin", "end"):
        function = getattr(trace, "fc_included_file_trace_" + name)
        function.argtypes = []
        function.restype = ctypes.c_bool
    trace.fc_included_file_trace_snapshot.argtypes = []
    trace.fc_included_file_trace_snapshot.restype = ctypes.c_char_p
    image_trace = ctypes.CDLL(os.environ["SPEC003_TRACE_LIBRARY"])
    image_trace.fc_image_trace_begin.argtypes = [ctypes.c_int]
    image_trace.fc_image_trace_begin.restype = ctypes.c_bool
    image_trace.fc_image_trace_snapshot.argtypes = []
    image_trace.fc_image_trace_snapshot.restype = ctypes.c_char_p
    require(image_trace.fc_image_trace_begin(-1), "Image trace did not start")
    require(trace.fc_included_file_trace_begin(), "Restore trace did not start")
    started = time.perf_counter_ns()
    doc = App.openDocument(str(fixture))
    require(doc is not None and not doc.Restoring, "Document still restoring")
    require(not doc.Recomputing, "Document recompute already active")
    completion_recompute_count = 0
    if doc.mustExecute():
        completion_recompute_count = doc.recompute()
        require(
            type(completion_recompute_count) is int and completion_recompute_count > 0,
            "Required recompute did not complete any features",
        )
    require(not doc.Recomputing and not doc.mustExecute(), "Document recompute incomplete")
    finished = time.perf_counter_ns()
    peak_rss = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss * 1024
    require(trace.fc_included_file_trace_end(), "Restore trace did not stop")
    raw = trace.fc_included_file_trace_snapshot()
    require(raw is not None, "Restore trace snapshot failed")
    counters = json.loads(raw)
    require(counters["active"] == 0 and counters["failed"] == 0, "Restore hook failed")
    storage_raw = image_trace.fc_image_trace_snapshot()
    require(storage_raw is not None, "Image trace snapshot failed")
    storage = json.loads(storage_raw)
    require(not App.GuiUp, "Console opened a GUI")
    require(
        storage["coin_image_bytes"] == storage["decoded_image_bytes"] == 0,
        "Unexpected console image materialization",
    )

    # Independently bind included properties to their archived bytes. XML is
    # bounded; ZIP paths are compared as names, never extracted to the filesystem.
    included_count = 0
    payload_bytes = 0
    with zipfile.ZipFile(fixture) as archive:
        require(archive.getinfo("Document.xml").file_size <= 32 * 1024 * 1024, "Metadata too large")
        saved = ET.fromstring(archive.read("Document.xml"))
        originals = {node.get("name"): node for node in saved.findall("./ObjectData/Object")}
        for obj in doc.Objects:
            for name in obj.PropertiesList:
                if obj.getTypeIdOfProperty(name) != "App::PropertyFileIncluded":
                    continue
                filename = getattr(obj, name)
                if not filename:
                    continue
                declaration = originals[obj.Name].find(
                    "./Properties/Property[@name='" + name + "']/FileIncluded"
                )
                require(declaration is not None, "Included property missing from archive")
                entry = declaration.get("file") or declaration.get("data")
                require(entry is not None, "Included entry unspecified")
                info = archive.getinfo(entry)
                require(info.file_size <= 32 * 1024 * 1024, "Included entry exceeds bound")
                target = Path(filename)
                require(target.is_file(), "Included destination missing")
                require(not target.stat().st_mode & stat.S_IWUSR, "Included destination writable")
                require(target.stat().st_size == info.file_size, "Included size differs")
                require(
                    checks.digest(target) == hashlib.sha256(archive.read(info)).hexdigest(),
                    "Included bytes differ",
                )
                included_count += 1
                payload_bytes += info.file_size
    require(counters["completed"] == included_count, "Unqualified hook/property coverage")
    require(counters["reused"] == 0, "Unexpected destination reuse in fresh worker")
    if context["fixture_id"] == "gridfinity-drawer-v1.3":
        require(included_count == 3 and payload_bytes > 0, "Primary has no meaningful payload")
    coverage = {"included_properties": included_count, "included_bytes": payload_bytes}
    if not qualification:
        require(coverage == context["included_file_coverage"], "Payload coverage changed")
        require(
            completion_recompute_count == context["completion_recompute_count"],
            "Completion recompute count changed",
        )
    state = checks.model_snapshot(doc)
    normative = hashlib.sha256(
        json.dumps(state, sort_keys=True, separators=(",", ":")).encode()
    ).hexdigest()
    if not qualification:
        require(normative == context["command_normative_state_sha256"], "Model state changed")
    parser_module = sys.modules.get("generated_sql_parser")
    parser_identity = None
    if parser_module is not None:
        parser_path = Path(parser_module.__file__).resolve(strict=True)
        expected_path = library_dir.parent / "Mod/BIM/generated_sql_parser.py"
        require(
            parser_path == expected_path.resolve(strict=True), "Foreign generated parser loaded"
        )
        parser_sha = checks.digest(parser_path)
        require(parser_sha == context["shared_sql_parser_sha256"], "Generated parser changed")
        parser_identity = {"relative_path": "Mod/BIM/generated_sql_parser.py", "sha256": parser_sha}
    if context["fixture_id"] in {"BIMExample", "ArchDetail"}:
        require(parser_identity is not None, "Required generated parser not loaded")
    if not qualification:
        require(
            parser_identity == context["generated_sql_parser"], "Loaded parser identity changed"
        )
    runtime = checks.runtime_receipt(context, qualification)
    App.closeDocument(doc.Name)
    return {
        **runtime,
        "comparison_id": "P01",
        "fixture_id": context["fixture_id"],
        "fixture_sha256": context["fixture_sha256"],
        "context_sha256": checks.digest(context_path),
        "normative_state_sha256": normative,
        "commit": commit,
        "correctness_passed": True,
        "qualification_only": qualification,
        "included_file_coverage": coverage,
        "hook_counters": counters,
        "completion_recompute_count": completion_recompute_count,
        "generated_sql_parser": parser_identity,
        "metrics": {
            "restore_payload_ms": counters["restore_nanoseconds"] / 1e6,
            "document_open_ms": (finished - started) / 1e6,
            "peak_rss_bytes": peak_rss,
            "coin_image_bytes": storage["coin_image_bytes"],
            "decoded_image_bytes": storage["decoded_image_bytes"],
        },
        "endpoint": "sum hook-entry to copy/stream-check completion; permissions/notifications excluded; console open includes one required recompute and excludes startup/fidelity/hash checks",
    }


def main_command():
    try:
        result = run()
    except Exception as exc:
        import traceback

        traceback.print_exc()
        result = {"correctness_passed": False, "exception_type": type(exc).__name__}
    Path(os.environ["SPEC003_RESULT_FILE"]).write_text(json.dumps(result, indent=2) + "\n")
